from __future__ import division
import csv
import numpy as np
import sys, os, getopt
from numpy import genfromtxt
from array import *
import math
import time
import matplotlib.pyplot as plt
from tqdm import tqdm
from collections import OrderedDict

def load_M_PDF(scenario, profile, model, m):
   if model == 'Intervals':
     fileName = profile + scenario + '_IntervalsOnly_m' + str(m) + '.csv' 
     fileName_m1 = profile + scenario + '_IntervalsOnly_m1.csv' 
   elif model == 'Sizes':
     fileName = profile + scenario + '_SizesOnly_m' + str(m) + '.csv'
   else:
     fileName = profile + scenario + '_m' + str(m) + '.csv'
     
   M_fileName = 'M_' + fileName
   PDF_fileName = 'PDF_' + fileName
   PMF_fileName = 'PDF_' + fileName_m1

   with open('./M_matrix/' + M_fileName, 'r') as SRCfile:
     M = list(csv.reader(SRCfile))
   M = np.array(M)

   with open('./PDF/' + PDF_fileName, 'r') as SRCfile:
     PDF = list(csv.reader(SRCfile))
   PDF = np.array(PDF)

   with open('./PDF/' + PMF_fileName, 'r') as SRCfile:
     PMF_original = list(csv.reader(SRCfile))
   PMF_original = np.array(PMF_original)

   return M, PDF, PMF_original


def main():
   Simu_Len = None
   seed = None
   try:
      opts, args = getopt.getopt(sys.argv[1:],"ht:s:",["help","duration=","seed="])
   except getopt.GetoptError as err:
      print (str(err))
      sys.exit(2)
   for opt, arg in opts:
      if opt in ('-h', '--help'):
         print ('trace_generation.py')
         print ('-h, --help                      Help!')
         print ('-t, --duration                  Length of ns-3 simulation [s]')
         print ('-s, --seed                      Seed number')
         sys.exit()
      elif opt in ('-t', '--duration'):
         Simu_Len = float(arg)
      elif opt in ('-s', '--seed'):
         seed = int(arg)
      else:
         assert False, "unhandled option"

   if Simu_Len == None:
     print ('Insert the ns-3 simulation length [s]! -h for help')
     sys.exit()
   if seed != None:
     np.random.seed(seed)

   # length of ns-3 simulation, with a 10% buffer 
   Simu_Len += Simu_Len*0.1  
   
   # The minimum number of CAMs to fill the entire length of the ns-3 simulation
   n_cam = int(Simu_Len*1000/100)

   # scenario ('Highway', 'Suburban', 'Urban' or 'Universal')
   scenario = 'Highway'

   if not(scenario in ('Highway', 'Suburban', 'Urban', 'Universal')):
     print ('Invalid scenario: use "Highway", "Suburban", "Urban" or "Universal"')
     sys.exit()

   # Car manufacturer profile ('Volkswagen' or 'Renault')
   profile = 'Volkswagen'

   if not(profile in ('Renault', 'Volkswagen')):
     print ('Invalid profile: use "Renault" or "Volkswagen"')
     sys.exit()

   # generate complete model (intervals+sizes), only intervals, or only sizes
   # (use 'Complete', 'Intervals' or 'Sizes')
   model = 'Intervals'

   if not(model in ('Complete', 'Intervals', 'Sizes')):
     print ('Invalid model: use "Complete" or "Intervals" or "Sizes"')
     sys.exit()
   if model != 'Intervals':
     print ('Works only with intervals')
     sys.exit()

   # Parameter m: number of states, including present state and previous ones, 
   # which are taken into account to calculate the transition to the new state
   # (valid values for m are 1 and 5) 
   m = 5
 
   if not(m in (1, 5)):
     print ('Invalid number of states: use m = 1 or m = 5')
     sys.exit()

   # Load the transition matrix M and PDF 
   # Load the transition matrix M and the PDF for the corresponding traces,
   # depending on the model, scenario and profile selected
   M, pdf, pmf_original = load_M_PDF(scenario, profile, model, m)
  
   M = M.astype('float64')
   pdf = pdf.astype('float64')
   pmf_original = pmf_original.astype('float64')

   # Number of possible sequences of 'm' symbols in the Markov chain model, 'N'
   N = pdf.shape[0]

   currentTime = 0.0 # in [ms]
   if model == 'Intervals':
     # Array to store the generated intervals
     CAMintervals = np.zeros(n_cam-1) # For 'n' cams, there are 'n-1' intervals

     # Array to store symbols [symbol = interval(ms) / 100]
     CAMsymbols = np.zeros(n_cam-1);

     r = np.random.uniform()
     for i in range(N):
       if r <= np.sum( pdf[0:i+1, m] ):
         current_symbols = pdf[i,0:m]
         break
     
     for i in range(len(current_symbols)):
       CAMsymbols[i] = int(current_symbols[i])
       CAMintervals[i] = int(current_symbols[i]) *100
       currentTime += int(current_symbols[i]) *100

  #   print (CAMintervals)

     lastValueIndex = 0
     for k in range(m,n_cam-1):
       if currentTime >Simu_Len *1000:
         lastValueIndex = k
         break
       Match = []
       P_trans = []
       for M_rowIndex in range(M.shape[0]):
         if (M[M_rowIndex,0:m] == current_symbols).all():
           Match.append(M[M_rowIndex,:])
           P_trans.append(M[M_rowIndex,m+1])
       Match = np.array(Match)
       P_trans = np.array(P_trans)
       r = np.random.uniform()
       for i in range(len(P_trans)):
         if r <= np.sum(P_trans[0:i+1]):
           next_symbol = Match[i,m]
           break
       CAMsymbols[k] = int(next_symbol)
       CAMintervals[k] = int(next_symbol) * 100
       currentTime += int(next_symbol) * 100
       current_symbols = CAMsymbols[k-m+1:k+1]

 #  print (CAMintervals)
   
   for index in range(len(CAMintervals)):
     if CAMintervals[index] == 0:
       print (index)
       index_break = index
       break

   CAMintervals = np.array(CAMintervals)
   CAMintervals_short = CAMintervals[:index_break]

   X = []
   sample_index = 0
   Y = []
   for CAM in CAMintervals_short:
     sample_index += 1
     Y.append(CAM)
     X.append(sample_index)

   plt.figure()
   plt.plot(X,Y, linewidth=2)
   plt.grid()
   plt.ylim(50,550)
   plt.xticks(fontsize=30)
   plt.yticks(fontsize=30)
   plt.ylabel('$T_{GenCAM}$ [ms]', rotation=90, fontsize=30)
   plt.xlabel('Sample index', fontsize=30)
   plt.show()
   

if __name__ == "__main__":
   main()
