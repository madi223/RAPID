#!/bin/bash

if [ $# -lt 7 ]
then
   echo "[usage]: ./start-los.sh <scen>  <run1,runn> <simTime> <data> <stream> <buff> <rtt>"
   exit 1
fi

scen=$1
#cca=$2
runcsv=$2
simTime=$3
data=$4
stream=$5
buff=$6
rtt=$7
runlist=()
rmax=$#
rfirst=$((rmax-3));
tmpfile="res.tmp"
tcplog="clean-tcp.csv"
delaylog="clean-e2e.csv"
logdir="LOS-results/"
trace=$logdir"scen"$scen"-All."$buff"."$rtt".csv"
WORK=$(pwd)
cd $WORK
mkdir $logdir 2>/dev/null
rm -rf clean-*

if [ ! -e $$trace ]; then
    echo -e "stream,scen,cca,run,data,duration,goodput,buffer,rtt" > $trace
fi
#str=
delimiter=,
s=$runcsv$delimiter
array=();
while [[ $s ]]; do
    array+=( "${s%%"$delimiter"*}" );
    s=${s#*"$delimiter"};
done;

#comm="NS_GLOBAL_VALUE=\"RngRun=" 
comm=" ./waf --run \"standard200Mhz_5g --simTime="$simTime" --data="$data" --stream="$stream" --buff="$buff" --scen="$scen" --serverDelay="$rtt" --run="

for t in ${array[@]}; do
    echo "********************* 5G-Umi TEST Run["$t"] ************************************"
    echo "--------------------------------------------------------------------------------"
    comm2=$comm""$t"\" 2>&1 | tee "$tmpfile;
    echo $comm2;
    rm -rf $tcplog;
    rm -rf $tmpfile;
    eval "$comm2";   #2>&1 | tee tmp-res;
    newlog=$logdir"scen-"$scen"-Run"$t"."$buff"."$rtt".csv"
    e2elog=$logdir"e2e-scen-"$scen"-"$cca"Run"$t".csv"
    port="1233"
    END=$tream
    j="1"
    for i in $(seq $stream); do
      echo $i; 
      streamid=$(echo $port + $i | bc);
      streamlog=$logdir"scen-"$scen"-Run"$t"stream-"$i"."$buff"."$rtt".csv"
      streamid=$streamid",49153";
      grep $streamid $tcplog > $streamlog;
      ConnStart=$(head -n 1 $streamlog | awk -F , '{print $1}');
      ConnEnd=$(tail -n 1 $streamlog | awk -F , '{print $1}');
      duration=$(echo $ConnEnd - $ConnStart | bc);
      k="1";
      num=$(echo $i - $j | bc);
      echo "num ="$num
      mline=$(grep -n -i "stream \["$num"\]" $tmpfile | awk -F : '{print $1}');
      numf=$(echo $mline + $k | bc);
      echo "numf ="$numf
      Mbytes=$(head -n $numf $tmpfile | tail -n 1 | awk -F : '{print $2}');
       echo "Mbytes ="$Mbytes
      rate=$(echo $Mbytes*8 / $duration | bc);
      algo="f"$i
      if [ $scen -eq 0 ]; then
         if [ $i -eq 1 ]; then
            algo="Cubic"
         else
            algo="BBR"
         fi
      elif [ $scen -eq 1 ]; then
         if [ $i -eq 1 ]; then
            algo="NewReno"
         elif [ $i -eq 2 ]; then
            algo="Vegas"
         else
            algo="Westwood"
         fi
      elif [ $scen -eq 4 ]; then
         if [ $i -eq 1 ]; then
            algo="NewReno"
         elif [ $i -eq 2 ]; then
            algo="Cubic"
         elif [ $i -eq 3 ]; then
            algo="Yeah"
         elif [ $i -eq 4 ]; then
            algo="Westwood"
         elif [ $i -eq 5 ]; then
            algo="Vegas"
         else
            algo="BBR"
         fi

      elif [ $scen -eq 11 ]; then
         if [ $i -eq 1 ]; then
            algo="Cubic"
         else
            algo="BBR"
         fi
      else
        algo="f"$i
      fi
      line=$algo","$scen"."$buff"M,"$algo","$t","$Mbytes","$duration","$rate","$buff","$rtt;
      echo -e $line >> $trace;
      #mv $tcplog $newlog;
          
    done
    mv $tcplog $newlog;
    #sudo mv $delaylog $e2elog

done

