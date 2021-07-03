#!/bin/bash

if [ $# -lt 1 ]
then
   echo "[usage]: ./start-test.sh <dir>"
   exit 1
fi

dir=$1
WORK="results/"
cp merge-time.py $dir"/"
cd $dir
mkdir $WORK

array=(1 2 4 8);
for t in ${array[@]}; do
    echo "********************* 5G-Umi TEST Run["$t"] ************************************"
    echo "--------------------------------------------------------------------------------"
    for COUNTER in $(seq 10); do
    newlog="mix-cubic+bbrRun"$COUNTER".10".$t".csv"
    if [ -f "$newlog" ]; then
       newlog1="Run"$COUNTER"."$t".S1wan.csv"
       streamid=",1234,49153";
       grep $streamid $newlog > $newlog1;
       newlog2="Run"$COUNTER"."$t".S1ran.csv"
       streamid=",1234,8200";
       grep $streamid $newlog > $newlog2;
       python3 merge-time.py -a $newlog1 -b $newlog2 -r $t -i "S1.R"$COUNTER 
       
       newlog1="Run"$COUNTER"."$t".S2wan.csv"
       streamid=",1235,49153";
       grep $streamid $newlog > $newlog1;
       newlog2="Run"$COUNTER"."$t".S2ran.csv"
       streamid=",1235,8201";
       grep $streamid $newlog > $newlog2;
       python3 merge-time.py -a $newlog1 -b $newlog2 -r $t -i "S2.R"$COUNTER
    fi
   done
done
    
