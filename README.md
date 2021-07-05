# RAPID
RAPID (RAN-Aware Proxy-based flow control for HIgh througput and low Delay eMBB) is a TCP proxy that aims to mitigate self-inflicted bufferbloat and maximize link utilization in today and future cellular networks. RAPID exploits real-time radio information and arrival rates of the concurrent flows in order to distribute proportionally the available RAN bandidth. Find below the required steps in order to reproduce the results shown in the paper.

<img src="ns3-testbed-git.png" alt="My cool logo"/>

# 1. Building the simulation environment

## 1.1 Legacy ns3 with mmWave

The commands below build ns3 with mmWave module and Full-duplex CSMA support. All the simulations that don't involve RAPID should be run on this legacy version. 

```

cd ns3-mmwave/
./waf clean
CXXFLAGS="-Wall" ./waf configure --build-profile=optimized
./waf

```

## 1.2 Modified ns3 with RAPID

The commands below build a modified ns3 with mmWave and RAPID support. The simulations that require RAPID should be run on this modified version.

```
cd rapid/
./waf clean
CXXFLAGS="-Wall" ./waf configure --build-profile=optimized
./waf

```
# 2. Reproducing the results shown in the paper: Cubic vs BBR
## 2.1 Fast Download in LOS
### 2.1.1 Without RAPID
#### Step 1: launch the simulation
From the root repository (RAPID), go to the legacy ns3 directory (ns3-mmwave) and launch the script **"start-los.sh"** with the required parameters (scen, runlist, simTime, data, #stream, buff, rtt):

```
$cd ns3-mmwave/
$./start-los.sh -h
[usage]: ./start-los.sh <scen>  <run1,runn> <simTime> <data> <stream> <buff> <rtt>
$./start-los.sh 0 1,2,3,4,5,6,7,8,9,10 20 200 2 10 1

```
Parameters    | Description
------------- | -------------
Scen          | Scenario number (e.g., 0 corresponds to Cubic-vs-BBR )
run           | Comma-separated list of the desired ns3 run numbers (e.g. 1,2,3,4,5 for five independent runs)
simTime       | ns3 simulation time (e.g. 20 for a 20 seconds simulation, make sure to choose a simTime always greater than the download duration unless the data is unlimitted)
data          | The size of data to download in MegaByte (e.g. 200 for 200MB data). You can indicate 0 for a continuous download
stream        | The number of concurrent flows (e.g. 2 for cubic and BBR)
buff          | The size of RLC and TCP buffers in MegaByte (e.g. 10 for 10MB buffer)
rtt           | The end-to-end RTT in ms (e.g 1 for 1ms RTT)

<br/>

In order to reproduce the results shown in the original paper, you need to run **start-los.sh** 4 times with different **rtt** (i.e. 1, 2, 4 and 8).<br/>
After running **start-los.sh**, you'll find in csv format, all the logs about the different flows and the acheived goodputs in a new directory named **LOS-results**.<br/>
#### Step 2: Get the results
The results in **LOS-results** are based on the following naming scheme:
* **scen\<scen>-All.\<buff>.\<rtt>.csv** : contains the goodput and duration of all the flows for all the runs of scenario \<scen> and RTT \<rtt>. For instance the file **scen0-All.10.1.csv** contains the goodputs and flow durations for scenario 0 (i.e. Cubic vs BBR) in case of 1ms RTT.
```
$cd LOS-results/
$cat scen0-All.10.1.csv
stream,scen,cca,run,data,duration,goodput,buffer,rtt
Cubic,0.10M,Cubic,1,209715200,1.960671,855687466,10,1
BBR,0.10M,BBR,1,209715200,7.278764,230495397,10,1
Cubic,0.10M,Cubic,2,209715200,1.960171,855905734,10,1
BBR,0.10M,BBR,2,42897400,14.898754,23034087,10,1
....  
  
```

* **scen-\<scen>-Run\<run>stream-\<stream>.\<buff>.\<rtt>.csv** : Contains the cwnd, rtt, bytesinflight variations over time of a given flow <stream> of scenario <scen>. For instance the file **scen-0-Run2stream-1.10.1.csv ** contains the information of the Cubic flow (stream 1) in scenario 0 and rtt=1.

```  
$cd LOS-results/
$cat scen-0-Run2stream-1.10.1.csv
time,cwnd,rtt,throughput,ran,tbs,BytesInflight,CCAstate,dstport,srcport
0.101096,14000,1,0,0,0,0,0,1235,49153
0.101096,14000,1,11.2,0,0,1400,0,1235,49153
0.101096,14000,1,22.4,0,0,2800,0,1235,49153
0.101096,14000,1,33.6,0,0,4200,0,1235,49153
0.101096,14000,1,44.8,0,0,5600,0,1235,49153
0.101096,14000,1,56,0,0,7000,0,1235,49153
0.101096,14000,1,67.2,0,0,8400,0,1235,49153
0.101096,14000,1,78.4,0,0,9800,0,1235,49153
...
```
#### Step 3: Calculate average RTT variations for each flow

After running **start-los.sh** four times (e.g. for rtt=1, rtt=2, rtt=4 and rtt=8), you can compute the EMA of the rtt-increase for each flow with 95% CI by using the following commands and scripts (keep in mind that you can still use your own script for that): 

```
```

The **start-los.sh** aims to facilitate the simulations, but you can also run the program with waf or create your own simulation script:
```
./waf --run "standard200Mhz_5g --simTime=20 --data=200 --stream=2 --buff=10 --scen=0 --serverDelay=1 --run=1"

```

