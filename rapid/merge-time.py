import pandas as pd
import numpy as np
import argparse
import time
from datetime import datetime
import sys
import os
import csv
from scipy.stats import t
from scipy import sqrt
from statistics import variance, mean 
import itertools 
######## ARGS definition ######
parser = argparse.ArgumentParser()
parser.add_argument("--data","-a",help="absolute path of dataset.csv",required=True)
parser.add_argument("--output","-b",help="absolute path of second dataset.csv",required=True)
parser.add_argument("--id","-i",help="absolute path of second dataset.csv",required=True)
parser.add_argument("--rtt","-r",help="absolute path of second dataset.csv",required=True)
args = parser.parse_args()
##############################
Data = args.data
Data2 = args.output
sid = args.id
rt = args.rtt
#dataset = pd.read_csv(Data)
#dataset.columns = ['time','cwnd','rtt','throughput','ranBw','tbs','inflight','state','dst','src']
#dataset2 = pd.read_csv(args.output)
#dataset2.columns = ['time','cwnd','rtt','throughput','ranBw','tbs','inflight','state','dst','src']
tr1 = open(Data,"r")
tr2 = open(Data2,"r")
f = open("results/merged-"+sid+"."+rt+".csv","w")
csv_reader1 = csv.reader(tr1, delimiter=',')
csv_reader2 = csv.reader(tr1, delimiter=',')
for (rowx, rowy) in itertools.zip_longest(csv_reader1,csv_reader2, fillvalue=0):
    rtt1 = float(rowx[2]) + float(rowy[2])
    time = 0
    infl = 0
    if ( (float(rowx[8])*float(rowy[8])) <= 0):
        time = max(float(rowx[0]),float(rowy[0]))
        infl = max(float(rowx[8]),float(rowy[8]))
    else:
        time = min(float(rowx[0]),float(rowy[0]))
        infl = min(float(rowx[8]),float(rowy[8]))
    f.write("{},{},{}\n".format(time,rtt,infl))
tr1.close()
tr2.close()
f.close()
