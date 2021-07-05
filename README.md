# RAPID
RAPID (RAN-Aware Proxy-based flow control for HIgh througput and low Delay eMBB) is a TCP proxy that aims to mitigate self-inflicted bufferbloat and maximize link utilization in today and future cellular networks. RAPID exploits real-time radio information and arrival rates of the concurrent flows in order to distribute proportionally the available RAN bandidth. Find below the required steps in order to reproduce the results shown in the paper.

<img src="ns3-testbed-git.png" alt="My cool logo"/>

# Building legacy mmWave-ns3 and RAPID  

## 1.1 Launch nodes images on r2lab

From your computer, launch the following commands

```

cd meld-demos/

python3 meld-deploy -s <slicename>

```

## 1.1 Start FlexRan Controller (fit14)

Connect to the server and start FlexRAN controller using the commands below:
