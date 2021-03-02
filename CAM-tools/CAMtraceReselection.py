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

   for ID in range(1,numVehicles+1):
     print ('Processing Node {}'.format(ID))
     inputFile = 'CAMtrace_' + str(ID) + '.csv'
     elemCounter = 0
     destFile = ''
     for character in inputFile:
        if character == '.':
           break 
        elemCounter += 1
     for i in np.arange(elemCounter):
        destFile += inputFile[i]
     destFile += 'Resel.csv'

     with open(dirPath + destFile, 'w') as TRfile:
        pass

     with open(dirPath + inputFile, 'r') as SRCfile:
        SRCreader = csv.reader(SRCfile) 
        row_count = sum(1 for row in SRCreader)  # fileObject is your csv.reader
   
     lineCounter = 0
     counter = 1
     with open(dirPath + inputFile, 'r') as SRCfile:
        CAMtrace = list(csv.reader(SRCfile))
        for i in np.arange(len(CAMtrace)):
           counter = 1
           actualValue = int(CAMtrace[i][1])
           for j in np.arange(i+1,len(CAMtrace)):
              if int(CAMtrace[j][1]) == actualValue:
                 counter += 1
              else:
                 break
           open(dirPath + destFile, 'a').write(CAMtrace[i][0] + ',' + CAMtrace[i][1] + ',' + str(counter) + '\r\n')


   print ('Success.')

if __name__ == "__main__":
   main()
