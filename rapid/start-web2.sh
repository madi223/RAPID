#!/bin/bash

if [ $# -lt 8 ]
then
   echo "[usage]: ./start-mix.sh <scen> <cca> <run1,runn> <simTime> <data> <stream> <buff> <rtt>"
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
logdir="web-proxy-cubic/"
trace=$logdir""$scen"-"$cca"-All."$buff"."$rtt".csv"
WORK="/home/mdiarra/bbr/proxy/webcc/"
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
comm=" ./waf --run \"web_5g --simTime="$simTime" --data="$data" --stream="$stream" --buff="$buff" --serverDelay="$rtt" --run="

for t in ${array[@]}; do
    echo "********************* 5G-Umi TEST Run["$t"] ************************************"
    echo "--------------------------------------------------------------------------------"
    comm2=$comm""$t"\" 2>&1 | tee "$tmpfile;
    echo $comm2;
    rm -rf $tcplog;
    rm -rf $tmpfile;
    eval "$comm2";   #2>&1 | tee tmp-res;
    newlog=$logdir""$scen"-"$cca"Run"$t"."$buff"."$rtt".csv"
    e2elog=$logdir"e2e-"$scen"-"$cca"Run"$t".csv"
    port="1233"
    END=$tream
    j="1"
    for i in $(seq $stream); do
      echo $i; 
      streamid=$(echo $port + $i | bc);
      streamlog=$logdir""$scen"-"$cca"Run"$t"stream-"$i"."$buff"."$rtt".csv"
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
      line="f"$i","$scen"."$buff"M,"$cca","$t","$Mbytes","$duration","$rate","$buff","$rtt;
      echo -e $line >> $trace;
      #mv $tcplog $newlog;
          
    done
    mv $tcplog $newlog;
    #sudo mv $delaylog $e2elog

done

