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

def main():
   numVehicles = None
   dirPath = None
   try:
      opts, args = getopt.getopt(sys.argv[1:],"hn:p:",["help","numVehicles=","dirPath="])
   except getopt.GetoptError as err:
      print (str(err))
      sys.exit(2)
   for opt, arg in opts:
      if opt == '-h':
         print ('CAMtraceReselection.py')
         print ('-n,--numVehicles                   The number of ns-3 vehicles')
         print ('-p,--dirPath                       The directory path')
         sys.exit()
      elif opt in ("-n", "--numVehicles"):
         numVehicles = int(arg)
      elif opt in ("-p", "--dirPath"):
         dirPath = arg
      else:
         assert False, "unhandled option"
   if numVehicles == None:
     print ('Insert the number of vehicles!')
     sys.exit()
   if dirPath == None:
     print ('Directory Path is mandatory!')
     sys.exit()

   print ('-------------------------------------------------')
   print ('Directory Path is ', dirPath)
   print ('The number of vehicles is ', numVehicles)
   print ('-------------------------------------------------')
       
   print ('Starting...')

   CAMs = {100:0, 200:0, 300:0, 400:0, 500:0, 600:0, 700:0, 800:0, 900:0, 1000:0}
   Cresels = {1:0, 2:0, 3:0, 4:0, 5:0, 6:0, 7:0, 8:0, 9:0, 10:0, 11:0, 12:0, 13:0, 14:0, 15:0, 16:0}

   totalNumber = 0
   totalVariations = 0
   Resel_counters = [] 

   for ID in range(1,numVehicles+1):
     print ('Processing Node {}'.format(ID))
   #  print (dirPath + 'CAMtrace_' + str(ID) + 'Resel.csv')
     numLine = 0
     with open(dirPath + 'CAMtrace_' + str(ID) + 'Resel.csv', 'r') as SRCfile:
       CAMtrace = list(csv.reader(SRCfile))
       for rowIndex in range(len(CAMtrace)):
     #    print (CAMtrace[rowIndex])
         CAMs[int(CAMtrace[rowIndex][1])] += 1
         if numLine > 0:
      #     print ('{} vs {}'.format(int(CAMtrace[rowIndex-1][1]),int(CAMtrace[rowIndex][1]))) 
           if int(CAMtrace[rowIndex][1]) != int(CAMtrace[rowIndex-1][1]):
             Resel_counters.append(int(CAMtrace[rowIndex][2]))
             totalNumber += 1
             totalVariations += 1
           else:
             totalNumber += 1
      #   time.sleep(0.2)
         else:
           Resel_counters.append(int(CAMtrace[rowIndex][2]))
         numLine += 1

   print ('Percentage of variations {}'.format(totalVariations/totalNumber))
   print ('Average reselection counter {}'.format(np.mean(Resel_counters)))

   for value in Resel_counters:
     if value >= 16:
       Cresels[16] += 1
     else:
       Cresels[value] += 1

   Sum_CAMs = 0
   for key in CAMs:
     Sum_CAMs += CAMs[key]

   Y_CAMs = []
   for key in CAMs:
     CAMs[key] /= Sum_CAMs
     Y_CAMs.append(CAMs[key])

   Y_CAMs = np.array(Y_CAMs)

   plt.figure(1)
   X = np.arange(10)
   width = 0.8
   plt.bar(X,Y_CAMs)
   plt.xticks(X,['100','200','300','400','500','600','700','800','900','1000'])

   Sum_Cresels = 0
   for key in Cresels:
     Sum_Cresels += Cresels[key]

   Y_Cresels = []
   for key in Cresels:
     Cresels[key] /= Sum_Cresels
     Y_Cresels.append(Cresels[key])
   
   Y_Cresels = np.array(Y_Cresels)

   plt.figure(2)
   X = np.arange(1,17)
   plt.bar(X,Y_Cresels)
   plt.xticks(X,['1','2','3','4','5','6','7','8','9','10','11','12','13','14','15','16'])
   plt.show()


   print ('Success.')

if __name__ == "__main__":
   main()
