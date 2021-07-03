#!/bin/bash

if [ $# -lt 8 ]
then
   echo "[usage]: ./start-test.sh <scen> <cca> <run1,runn> <simTime> <data> <stream> <buff> <rtt>"
   exit 1
fi

scen=$1
cca=$2
runcsv=$3
simTime=$4
data=$5
stream=$6
buff=$7
rtt=$8
runlist=()
rmax=$#
rfirst=$((rmax-3));
tmpfile="res.tmp"
tcplog="clean-tcp.csv"
delaylog="clean-e2e.csv"
logdir="scen1-pepem-iw2/"
trace=$logdir""$scen"-"$cca"-All."$buff"."$rtt".csv"
WORK="/home/mdiarra/bbr/proxy/pepiw/"
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
comm=" ./waf --run \"pepx_5g --simTime="$simTime" --data="$data" --stream="$stream" --buff="$buff" --scen=1 --serverDelay="$rtt" --run="

for t in ${array[@]}; do
    echo "********************* 5G-Umi TEST Run["$t"] ************************************"
    echo "--------------------------------------------------------------------------------"
    comm2=$comm""$t"\" 2>&1 | tee "$tmpfile;
    echo $comm2;
    rm -rf $tcplog;
    rm -rf $tmpfile;
    eval "$comm2";   #2>&1 | tee tmp-res;
    newlog=$logdir""$scen"-"$cca"Run"$t"."$buff"."$rtt".csv"
    e2elog=$logdir"e2e-"$scen"-"$cca"Run"$t"."$buff"."$rtt".csv"
    port="1233"
    port2="8199"
    END=$((stream+0))
    j="1"
    #COUNTER=1
    START=1
    END=$tream
                      
    #for COUNTER in {$START..$END}; do 
    #for (( COUNTER=1; COUNTER=<$stream;COUNTER++ )); do
    for COUNTER in $(seq $stream); do
      #echo $i;
      echo $COUNTER; 
      streamid=$(echo $port + $COUNTER | bc);
      id2=$(echo $port2 + $COUNTER | bc);
      streamlog=$logdir""$scen"-"$cca"Run"$t"stream-"$COUNTER"."$buff"."$rtt".csv"
      streamid=","$streamid",49153\|,"$streamid","$id2;
      grep $streamid $tcplog > $streamlog;
      ConnStart=$(head -n 1 $streamlog | awk -F , '{print $1}');
      ConnEnd=$(tail -n 1 $streamlog | awk -F , '{print $1}');
      duration=$(echo $ConnEnd - $ConnStart | bc);
      k="1";
      num=$(echo $COUNTER - $j | bc);
      echo "num ="$num
      mline=$(grep -n -i "stream \["$num"\]" $tmpfile | awk -F : '{print $1}');
      numf=$(echo $mline + $k | bc);
      echo "numf ="$numf
      Mbytes=$(head -n $numf $tmpfile | tail -n 1 | awk -F : '{print $2}');
       echo "Mbytes ="$Mbytes
      rate=$(echo $Mbytes*8 / $duration | bc);
      algo=""
      if [ $COUNTER -eq 1 ]; then
         algo="NewReno"
      elif [ $COUNTER -eq 2 ]; then
         algo="Vegas"
      else
         algo="Westwood"
      fi

      line=$algo","$scen"."$buff"M,"$algo","$t","$Mbytes","$duration","$rate","$buff","$rtt;
      echo -e $line >> $trace;
      #mv $tcplog $newlog;
          
    done
    mv $tcplog $newlog;
    mv $delaylog $e2elog;
 done

