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
   dirPath = None
   try:
      opts, args = getopt.getopt(sys.argv[1:],"hp:",["help","dirPath="])
   except getopt.GetoptError as err:
      print (str(err))
      sys.exit(2)
   for opt, arg in opts:
      if opt == '-h':
         print ('NS3_CAMtraceStats.py')
         print ('-p,--dirPath                       The directory path')
         sys.exit()
      elif (opt == '-p') or (opt == '--dirPath'):
         dirPath = arg
      else:
         assert False, "unhandled option"

   if dirPath == None:
     print ('Directory Path is mandatory!')
     sys.exit()

   print ('Starting...')

   CAM_sizes = {}
   CAM_Tgens = {100:0, 200:0, 300:0, 400:0, 500:0, 600:0, 700:0, 800:0, 900:0, 1000:0}


   fileList = os.listdir(dirPath)
 
   t = tqdm(range(len(fileList)))
   for fileName in fileList:
     t.update(1)

     filePath = os.path.join(dirPath, fileName)
     with open(filePath, 'r') as SRCfile:
       csvFile = csv.reader(SRCfile)
       for row in csvFile:
         Tgen = int(row[1])
         size = int(row[2])
         if Tgen != 0:
           CAM_Tgens[Tgen] += 1
         if not(size in CAM_sizes):
           CAM_sizes[size] = 1
         else:
           CAM_sizes[size] += 1

   t.close()

   Sum_sizes = 0
   for key in CAM_sizes:
     Sum_sizes += CAM_sizes[key]

   Y_sizes = []
   for key in CAM_sizes:
     CAM_sizes[key] /= Sum_sizes
     Y_sizes.append(CAM_sizes[key])
   Y_sizes = np.array(Y_sizes)

   plt.figure(1)
   X_labels = []
   for key in CAM_sizes:
     X_labels.append(str(key))
   X = np.arange(len(CAM_sizes))
   width = 0.8
   plt.bar(X,Y_sizes)
   plt.xticks(X,X_labels)

   Sum_Tgens = 0
   for key in CAM_Tgens:
     Sum_Tgens += CAM_Tgens[key]

   Y_Tgens = []
   for key in CAM_Tgens:
     CAM_Tgens[key] /= Sum_Tgens
     Y_Tgens.append(CAM_Tgens[key])
   Y_Tgens = np.array(Y_Tgens)

   plt.figure(2)
   X_labels = []
   for key in CAM_Tgens:
     X_labels.append(str(key))
   X = np.arange(len(CAM_Tgens))
   width = 0.8
   plt.bar(X,Y_Tgens)
   plt.xticks(X,X_labels)


   plt.show()


   print ('Success.')

if __name__ == "__main__":
   main()
