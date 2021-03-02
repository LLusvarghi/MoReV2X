from __future__ import division
import csv, math
import numpy as np
import sys, os, getopt, time
import matplotlib.pyplot as plt
from tqdm import tqdm

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
   verbose = False
   try:
      opts, args = getopt.getopt(sys.argv[1:],"hv",["help","verbose"])
   except getopt.GetoptError as err:
      print (str(err))
      sys.exit(2)
   for opt, arg in opts:
      if opt in ('-h', '--help'):
         print ('traces_generation.py   Options:')
         print ('-h, --help               Help!')
         print ('-v, --verbose            Display the output')
         sys.exit()
      elif opt in ('-v', '--verbose'):
         verbose = True
      else:
         assert False, "unhandled option"

   # Number of CAMs that will be generated
   n_cam = int(1e4)

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

   # Number of possible sequences of 'm' symbols in the Markov chain model, 'N'
   N = pdf.shape[0]

   # Number of possible sizes |S| and intervals |G|
   if profile == 'Volkswagen':
     S = 4
   elif profile == 'Renault':
     S = 5
   else:
     print ('Error')
     sys.exit()
 
   G = 10

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

     t = tqdm(range(m,n_cam-1))
     for k in range(m,n_cam-1):
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
       current_symbols = CAMsymbols[k-m+1:k+1]

       t.update(1)
     t.close()
 
     if verbose:
       CAMs = {100:0, 200:0, 300:0, 400:0, 500:0, 600:0, 700:0, 800:0, 900:0, 1000:0}
       Sum = 0
       for value in CAMintervals:
         CAMs[value] += 1
         Sum += 1

       PMF = []
       for key in CAMs:
         CAMs[key] /= Sum
         PMF.append(CAMs[key])
     
       pmf_original = pmf_original.astype('float64')

       plt.figure(1)
       width = 0.4
       X = np.arange(10)
       plt.bar(X-width/2,PMF,width,color='forestgreen', label='Simu')
       plt.bar(X+width/2,pmf_original[:,1],width,color='firebrick', label='Originale')
       plt.xticks(X,['100','200','300','400','500','600','700','800','900','1000']) 
       plt.xlabel('TgenCAM')
       plt.ylabel('p(x)')
       plt.legend()
       plt.show()


if __name__ == "__main__":
   main()