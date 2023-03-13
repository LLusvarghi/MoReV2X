from __future__ import division
import csv, math
import numpy as np
import sys, os, getopt, time
import matplotlib.pyplot as plt
from tqdm import tqdm


def sizeBytes(profile, sizeIndex):
   VolkswagenSizes = [200, 300, 360, 455]
   RenaultSizes = [200, 330, 480, 600, 800]

   if profile == 'Volkswagen':
     return VolkswagenSizes[sizeIndex-1]
   elif profile == 'Renault':
     return RenaultSizes[sizeIndex-1]
   else:
     print ('Invalid profile!')
     sys.exit()


def load_M_PDF(scenario, profile, model, m):
   if model == 'Intervals':
     fileName = profile + scenario + '_IntervalsOnly_m' + str(m) + '.csv' 
   elif model == 'Sizes':
     fileName = profile + scenario + '_SizesOnly_m' + str(m) + '.csv'
   else:
     fileName = profile + scenario + '_m' + str(m) + '.csv'
     
   M_fileName = 'M_' + fileName
   PDF_fileName = 'PDF_' + fileName
#   PMF_fileName = 'PDF_' + fileName

   with open('./M_matrix/' + M_fileName, 'r') as SRCfile:
     M = list(csv.reader(SRCfile))
   M = np.array(M)

   with open('./PDF/' + PDF_fileName, 'r') as SRCfile:
     PDF = list(csv.reader(SRCfile))
   PDF = np.array(PDF)

   return M, PDF


def main():
   N_Vehicles = None
   Simu_Len = None
   dirPath = None
   profile = None
   scenario = None
   model = None
   m = None
   try:
      opts, args = getopt.getopt(sys.argv[1:],"hn:t:p:m:",["help","NumVehicles=","duration=","dirPath=","profile=","scenario=","model="])
   except getopt.GetoptError as err:
      print (str(err))
      sys.exit(2)
   for opt, arg in opts:
      if opt in ('-h', '--help'):
         print ('NS3_traces_generation.py')
         print ('-h, --help                      Help!')
         print ('-n, --NumVehicles               Number of ns-3 vehicles')
         print ('-t, --duration                  Length of ns-3 simulation [s]')
         print ('-p, --dirPath                   Directory path')
         print ('--profile                       OEM profile (Volkswagen or Renault)')
         print ('--scenario                      Scenario (Highway, Suburban or Urban)')
         print ('--model                         Markov model (Complete, Intervals or Sizes)')
         print ('-m                              Number of symbols (1 or 5)')
         sys.exit()
      elif (opt == '-n') or  (opt == '--NumVehicles'):
         N_Vehicles = int(arg)
      elif (opt == '-t') or  (opt == '--duration'):
         Simu_Len = float(arg)
      elif (opt == '-p') or  (opt == '--dirPath'):
         dirPath = arg
      elif opt == '--profile':
         profile = arg
      elif opt == '--scenario':
         scenario = arg
      elif opt == '--model':
         model = arg
      elif opt == '-m':
         m = int(arg)
      else:
         assert False, "unhandled option"

   if N_Vehicles == None:
     print ('Insert a number of ns-3 vehicles! -h for help')
     sys.exit()
   if Simu_Len == None:
     print ('Insert the ns-3 simulation length [s]! -h for help')
     sys.exit()
   if dirPath == None:
     print ('Insert the destination directory path! -h for help')
     sys.exit()
   if profile == None:
     print ('Insert the CAM traces OEM profile! -h for help')
     sys.exit()
   if scenario == None:
     print ('Insert the evaluation scenario! -h for help')
     sys.exit()
   if model == None:
     print ('Insert the Markov model! -h for help')
     sys.exit()
   if m == None:
     print ('Insert the number of symbols! -h for help')
     sys.exit()

   if not(profile in ['Volkswagen', 'Renault']):
     print ('Insert a valid OEM profile! -h for help')
     sys.exit()
   if not(scenario in ['Highway', 'Suburban', 'Urban']):
     print ('Insert a valid scenario! -h for help')
     sys.exit()
   if not(model in ['Complete', 'Intervals', 'Sizes']):
     print ('Insert a valid scenario! -h for help')
     sys.exit()
   if not(m in [1, 5]):
     print ('Insert a valid number of symbols! -h for help')
     sys.exit()


   print ('Starting')
   print ('.')
   time.sleep(0.5)
   print ('.')
   time.sleep(0.5)
   print ('.')
   time.sleep(0.5)
   print ('.')
   time.sleep(0.5)

   fileList = os.listdir(dirPath)
  # print (fileList)
   if len(fileList) > 0: 
     for fileID in fileList:
       filePath = os.path.join(dirPath, fileID)
       os.remove(filePath)

   # length of ns-3 simulation, with a 10% buffer 
   Simu_Len += Simu_Len*0.1  
   
   # The minimum number of CAMs to fill the entire length of the ns-3 simulation
   n_cam = int(Simu_Len*1000/100)


   # Scenario: Highway, Suburban, Urban and Universal (TBD)

   # Model type: Complete (intervals+sizes), only intervals, or only sizes

   # Parameter m: number of states, including present state and previous ones, 
   # which are taken into account to calculate the transition to the new state
   # (valid values for m are 1 and 5) 

   if profile == 'Volkswagen':
     S = 4
   else: # it's Renault
     S = 5

   G = 10
   # Load the transition matrix M and PDF 
   # Load the transition matrix M and the PDF for the corresponding traces,
   # depending on the model, scenario and profile selected
   M, pdf = load_M_PDF(scenario, profile, model, m)
  
   M = M.astype('float64')
   pdf = pdf.astype('float64')

   # Number of possible sequences of 'm' symbols in the Markov chain model, 'N'
   N = pdf.shape[0]

   for ID in range(1,N_Vehicles+1):
     currentTime = 0.0 # in [ms]
     # Array to store the generated intervals
     CAMintervals = np.zeros(n_cam)
     # Array to store the generated CAM sizes
     CAMsizes = np.zeros(n_cam)
     CAMsizesIndex = np.zeros(n_cam);
     # Array to store symbols [symbol = interval(ms) / 100]
     CAMsymbols = np.zeros(n_cam);

     if model == 'Intervals':

       r = np.random.uniform()
       for i in range(N):
         if r <= np.sum( pdf[0:i+1, m] ):
           current_symbols = pdf[i,0:m]
           break
     
       for i in range(len(current_symbols)):
         CAMsymbols[i] = int(current_symbols[i])
         CAMintervals[i] = int(current_symbols[i]) *100
         CAMsizes[i] = 0
         currentTime += int(current_symbols[i]) *100
       
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
         CAMsizes[k] = 0
         currentTime += int(next_symbol) * 100
         current_symbols = CAMsymbols[k-m+1:k+1]

     elif model == 'Sizes':

       r = np.random.uniform()
       for i in range(N):
         if r <= np.sum( pdf[0:i+1, m] ):
           current_symbols = pdf[i,0:m]
           break
     
       for i in range(len(current_symbols)):
         CAMsymbols[i] = int(current_symbols[i])
         CAMintervals[i] = 300
         CAMsizes[i] = sizeBytes(profile, int(current_symbols[i]) )
         currentTime += 300

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
         CAMintervals[k] = 300
         CAMsizes[k] = sizeBytes(profile, int(next_symbol))
         currentTime += 300
         current_symbols = CAMsymbols[k-m+1:k+1]

     else:  # It's the complete model, no need to check again
       r = np.random.uniform()
       for i in range(N):
         if r <= np.sum( pdf[0:i+1, m] ):
           current_symbols = pdf[i,0:m]
           break

       for i in range(len(current_symbols)):
         CAMsymbols[i] = int(current_symbols[i])
         CAMsizes[i] = sizeBytes(profile, int( (current_symbols[i]-1) % (G*S/10)  ))
      #   CAMsizesIndex[i] = int( (current_symbols[i]-1) % (G*S/10)  )
         if i > 0:
           CAMintervals[i-1] = (math.floor( int( (current_symbols[i]-1) / (G*S/10)  )  ) +1)*100
           currentTime += CAMintervals[i-1]

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

         CAMsizes[k] = sizeBytes(profile, int( (CAMsymbols[k]-1) % (G*S/10)  ))
         CAMintervals[k-1] = (math.floor( int( (CAMsymbols[k]-1) / (G*S/10)  )  )+1) *100

         currentTime += CAMintervals[k-1]
         current_symbols = CAMsymbols[k-m+1:k+1]



     print ('Node ID: {}'.format(ID))

     filePath = os.path.join(dirPath,  'CAMtrace_' + str(ID) + '.csv')

     with open(filePath,'w') as DSTfile:
       pass

     with open(filePath,'a') as DSTfile:
       currentTime = 0.0
       for i in range(lastValueIndex):
         DSTfile.write(str(currentTime/1000) + ',' + str(int(CAMintervals[i])) + ',' + str(int(CAMsizes[i])) + '\r\n')
         currentTime += CAMintervals[i]

     

if __name__ == "__main__":
   main()
