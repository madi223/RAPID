# RAPID
RAPID (RAN-Aware Proxy-based flow control for HIgh througput and low Delay eMBB) is a TCP proxy that aims to mitigate self-inflicted bufferbloat and maximize link utilization in today and future cellular networks. RAPID exploits real-time radio information and arrival rates of the concurrent flows in order to distribute proportionally the available RAN bandidth. Find below the required steps in order to reproduce the results shown in the paper.

<img src="ns3-testbed-git.png" alt="My cool logo"/>

# 1. Building the simulation envioronment

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
