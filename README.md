# RAPID
RAPID (RAN-Aware Proxy-based flow control for HIgh througput and low Delay eMBB) is a TCP proxy that aims to mitigate self-inflicted bufferbloat and maximize link utilization in today and future cellular networks. RAPID exploits real-time radio information and arrival rates of the concurrent flows in order to distribute proportionally the available RAN bandidth. Find below the required steps in order to reproduce the results shown in the paper.

<img src="ns3-testbed-git.png" alt="My cool logo"/>
<br/>

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
$mkdir cubic.1ms && cp LOS-results/scen-0-Run*stream-1.10.1.csv cubic.1ms/
$./allci.sh cubic.1ms
$mv rtt.ci.csv cubic.1ms.csv

$mkdir bbr.1ms && cp LOS-results/scen-0-Run*stream-2.10.1.csv bbr.1ms/
$./allci.sh bbr.1ms
$mv rtt.ci.csv bbr.1ms.csv
```
These commands allow to calculate the average RTT for the 2 flows (cubic and BBR) only in case of 1ms RTT, you need to repeat the same process for the other RTT configurations (2,4, and 8). After that you can plot the CDF using our provided python script or your own:
  
```
$python3 cdf.py --c1 cubic.1ms.csv --c8 cubic.8ms.csv --b1 bbr.1ms.csv --b8 bbr.8ms.csv
```

The **start-los.sh** aims to facilitate the simulations, but you can also run the program with waf or create your own simulation script:
```
cd ns3-mmwave/
./waf --run "standard200Mhz_5g --simTime=20 --data=200 --stream=2 --buff=10 --scen=0 --serverDelay=1 --run=1"

```
### 2.1.2 With RAPID
#### Step 1: launch the simulation
From the root repository (RAPID), go to the rapid directory and launch the script **"start-los.sh"** with the required parameters (scen, runlist, simTime, data, #stream, buff, rtt):

```
$cd rapid/
$./start-los.sh -h
[usage]: ./start-los.sh <scen>  <run1,runn> <simTime> <data> <stream> <buff> <rtt>
$./start-los.sh 0 1,2,3,4,5,6,7,8,9,10 20 200 2 10 1

```
You can also run RAPID with waf or create your own simulation script:
```
cd rapid/
./waf --run "pep2_5g --simTime=20 --data=200 --stream=2 --buff=10 --scen=0 --serverDelay=1 --run=1"

```
#### Step 2: Get the results
Same as in **2.1.1**

#### Step 3: Calculate average RTT variations for each flow
Here, since the proxy splits each individual flow into 2 subflows, we need first to calculate the end-to-end RTT by combining the RTT of the 2 subflows. For that we use the following scripts:

```
$./parse-mix.sh LOS-results
$cd LOS-results/results
$mkdir cubic.1ms && cp *S1*.csv cubic.1ms/
$mkdir bbr.1ms && cp *S2*.csv bbr.1ms/
  
$cd ../../
$./allci.sh LOS-results/results/cubic.1ms
$mv rtt.ci.csv cubic.1ms.csv

$./allci.sh LOS-results/results/bbr.1ms
$mv rtt.ci.csv bbr.1ms.csv
```
Repeat the same process for the other RTT configurations (2,4, and 8). After that you can plot the CDF using our provided python script or your own:
```
$python3 cdf.py --c1 cubic.1ms.csv --c8 cubic.8ms.csv --b1 bbr.1ms.csv --b8 bbr.8ms.csv --new
```
## 2.2 Fast Download in NLOS
### 2.2.1 Without RAPID
In all the steps of **2.1.1** , Replace **start-los.sh** by **start-nlos.sh**
```
$cd ns3-mmwave/
$./start-nlos.sh 0 1,2,3,4,5,6,7,8,9,10 20 200 2 10 1
```
### 2.2.2 With RAPID
In all the steps of **2.1.2** , Replace **start-los.sh** by **start-nlos.sh**
```
$cd rapid/
$./start-nlos.sh 0 1,2,3,4,5,6,7,8,9,10 20 200 2 10 1
```
## 2.3 Fast and app-limited (web) Download in LOS
### 2.3.1 Without RAPID
In all the steps of **2.1.1** , Replace **scen=0** by **scen=11**, **data=200** by **data=0** and **simTime=20** by **simTime=10**
```
$cd ns3-mmwave/
$./start-los.sh 11 1,2,3,4,5,6,7,8,9,10 10 0 2 10 1
```
  
### 2.3.2 With RAPID
In all the steps of **2.1.2** , Replace **scen=0** by **scen=11**, **data=200** by **data=0** and **simTime=20** by **simTime=10**
```
$cd rapid/
$./start-nlos.sh 11 1,2,3,4,5,6,7,8,9,10 10 0 2 10 1
```
## 2.4 Fast and app-limited (web) Download in NLOS
### 2.4.1 Without RAPID
In all the steps of **2.3.1** , Replace **start-los.sh** by **start-nlos.sh**
```
$cd ns3-mmwave/
$./start-nlos.sh 11 1,2,3,4,5,6,7,8,9,10 10 0 2 10 1
```
  
### 2.3.2 With RAPID
In all the steps of **2.3.2** , Replace **start-los.sh** by **start-nlos.sh**
```
$cd rapid/
$./start-nlos.sh 11 1,2,3,4,5,6,7,8,9,10 10 0 2 10 1
```
  
# 3. Evaluating RAPID with other Congestion Controllers

Scenario ID   | Description
------------- | -------------
0             | Cubic and BBR downloading same data simultaneously
1             | NewReno, Vegas and Westwood downloading same data simultaneously
3             | NewReno, Cubic and BBR downloading same data simultaneously
4             | NewReno, Cubic, YeAh, Westwood, Vegas and BBR downloading same data simultaneously
11            | Cubic downloading large file while BBR limited at 16 Mbps (Web browsing) 
  
## 3.1 Simulating scenario 4
### 3.1.1 Without RAPID
For LOS conditions
```
$cd ns3-mmwave/
$./start-los.sh 4 1,2,3,4,5,6,7,8,9,10 20 200 6 10 1
```
For NLOS conditions
```
$cd ns3-mmwave/
$./start-nlos.sh 4 1,2,3,4,5,6,7,8,9,10 20 200 6 10 1
```
  
### 3.1.2 With RAPID
For LOS conditions
```
$cd rapid/
$./start-los.sh 4 1,2,3,4,5,6,7,8,9,10 20 200 6 10 1
```
For NLOS conditions
```
$cd rapid/
$./start-nlos.sh 4 1,2,3,4,5,6,7,8,9,10 20 200 6 10 1
```
### 3.1.3 Analysing the results
<img src="ns3-testbed-git.png" alt="My cool logo"/>
<br/>
<img src="ns3-testbed-git.png" alt="My cool logo"/>
<br/>
