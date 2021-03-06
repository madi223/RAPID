/*/*-*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 * Copyright (c) 2016, University of Padova, Dep. of Information Engineering, SIGNET lab
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Jaume Nin <jnin@cttc.cat>
 *         Nicola Baldo <nbaldo@cttc.cat>
 *
 * Modified by Michele Polese <michele.polese@gmail.com>
 *     (support for RRC_CONNECTED->RRC_IDLE state transition + support for real S1AP link)
 *
 * Modified by Goodsol Lee <gslee2@netlab.snu.ac.kr>
 *     (Add Proxy function for TCP proxy based handover)
 */


#include "epc-enb-proxy-application.h"
#include "ns3/log.h"
#include "ns3/mac48-address.h"
#include "ns3/ipv4.h"
#include "ns3/inet-socket-address.h"
#include "ns3/uinteger.h"
#include "ns3/internet-module.h"

#include "ns3/tcp-socket-base.h"
#include "ns3/tcp-tx-buffer.h"
#include "epc-gtpu-header.h"
#include "eps-bearer-tag.h"
#include <algorithm>
#include <cmath>
#include "ns3/config-store.h"
#include "ns3/core-module.h"
#include <fstream>
//#include "contrib-module.h"
#include "ns3/mmwave-helper.h"


namespace ns3 {
std::ofstream fPlotQueue ("clean-e2e.csv", std::ios::out | std::ios::app);
ProxyFlow::ProxyFlow(void)
{
m_avail=0;
m_epoch=0;
m_sstime=0;
m_rtt=0;
m_bdp=0;
m_rw=0;
m_elapsed=0;
m_data = 0;
m_isslow=false;
m_delay=0;
m_thresh = 0;
m_prev = 1400;
m_mss = 1400;
m_snd = 14400;
m_krt = 0;
m_arrival_rate = 0;
m_expected_rate = 0;
m_ema_prev = 0;

}
ProxyFlow::ProxyFlow(double m_rtt)
{
m_avail=0;
m_epoch=0;
m_sstime=0;
this->m_rtt=m_rtt;
m_bdp=0;
m_rw=0;
m_data = 0;
m_elapsed = 0;
m_isslow=false;
m_delay = 0;
m_thresh = 0;
m_prev = 1400;
m_mss = 1400;
m_snd = 14400;
m_krt = 0;
m_arrival_rate = 0;
m_expected_rate = 0;
m_ema_prev = 0;
}
ProxyFlow::ProxyFlow(double m_rtt,double m_delay)
{
m_avail=0;
m_epoch=0;
m_sstime=0;
this->m_rtt=m_rtt;
m_bdp=0;
m_rw=0;
m_data = 0;
m_elapsed = 0;
m_thresh = 0;
m_prev = 1400;
m_mss = 1400;
m_snd = 14400;
m_isslow=false;
this->m_delay=m_delay;
m_krt = 0;
m_arrival_rate = 0;
m_expected_rate = 0;
m_ema_prev = 0;
}
ProxyFlow::ProxyFlow(const ProxyFlow &flow)
{
this->m_avail=flow.m_avail;
this->m_epoch=flow.m_epoch;
this->m_sstime=flow.m_sstime;
this->m_rtt=flow.m_rtt;
this->m_bdp=flow.m_bdp;
this->m_rw=flow.m_rw;
this->m_isslow=flow.m_isslow;
this->m_elapsed=flow.m_elapsed;
this->m_data=flow.m_data;
this->m_delay=flow.m_delay;
}
ProxyFlow::~ProxyFlow(void)
{
}

void ProxyFlow::SetRtt(double m_rtt,double m_delay)
{
this->m_rtt=m_rtt;
this->m_delay=m_delay;
}

void ProxyFlow::SetAvail(double m_avail)
{
this->m_avail = m_avail;
}

double ProxyFlow::GetRtt()
{
return m_rtt;
}

double ProxyFlow::GetRw()
{
if(m_isslow)
 return m_bdp;
return m_rw;
}

double ProxyFlow::GetAvail()
{
return m_avail;
}

void ProxyFlow::UpdateSndWindow(double m_snd)
{
 this->m_snd = m_snd;
}

bool ProxyFlow::IsSlow()
{

 return m_isslow;
} 

bool ProxyFlow::DetectSlow()
{
    if ((m_epoch == 0) && (m_rtt !=0))
   {
    m_epoch = TcpOptionTS::NowToTsValue ();
    m_elapsed = TcpOptionTS::NowToTsValue ();
    m_prev = m_mss;
    //if(m_rw!=packet->GetSize()*10)
    // m_prev = m_thresh;
     m_thresh = m_isslow == true ? std::min(m_snd,m_rw) : std::max(m_snd,m_rw);
   // m_thresh = std::min(m_snd,m_rw);
    m_delay = m_delay == 0 ? 1 : m_delay;
    double expected_data = std::abs(m_thresh /*- m_prev*/);
    //*1 m_sstime = ((m_delay*2)*(std::ceil(1+std::log2(m_thresh/packet->GetSize()))))/1000; 
     m_krt = std::ceil(1+std::log2(expected_data/m_mss));
     m_sstime = ((m_delay*2)*(m_krt))/1000;  
    //m_sstime = ((m_delay*2)*(1+std::log2(m_thresh/packet->GetSize())))/1000;
      
   }
 if ((m_epoch != 0)&& (m_rtt !=0))
   {
     m_delay = m_delay == 0 ? 1 : m_delay;
     //if(m_rw!=packet->GetSize()*10)
    //m_sstime = (m_rtt)*(std::log2(m_rw/packet->GetSize()));
    //m_data+=packet->GetSize();
    //1* double arrival_rate = m_data/TcpOptionTS::ElapsedTimeFromTsValue(m_epoch).GetSeconds();
    m_arrival_rate = (m_data)/((2*m_delay/1000));
    m_expected_rate = /*(std::pow(2,m_krt)-1)*/((m_thresh/*m_mss*/)/((2*m_delay/1000))); //m_thresh
    double rcv_data = m_data;
    //int k = 0
    if (TcpOptionTS::ElapsedTimeFromTsValue(m_elapsed).GetSeconds()>=m_sstime/m_krt)
       {
        m_data = 0;
        m_elapsed = TcpOptionTS::NowToTsValue (); 
        double alpha = (2/(m_krt+1));
        alpha = 0.4;
        //m_ema_prev = m_ema_prev == 0 ? m_arrival_rate : ((m_arrival_rate)*(2/(m_krt+1))) + (m_ema_prev)*(1-(2/(m_krt+1))); 
        m_ema_prev = m_ema_prev == 0 ? m_arrival_rate : ((m_arrival_rate)*(1-alpha)) + (m_ema_prev)*(alpha); 
       }
    //m_bdp = std::abs(arrival_rate*(m_delay/1000) /*- m_rw*/);
    std::cout<<"arrival: "<<m_arrival_rate<<"[Arrival EMA: "<<m_ema_prev<<"]"<<" expected: "<<m_expected_rate<<" rcv data: "<<rcv_data<<std::endl;
   //std::cout << "m_rate: "<<(m_bdp)/((m_rtt/2)/1000)<<" mbdp: "<<m_bdp<< " data: "<<m_data<<" elapsed_ms: "<<TcpOptionTS::ElapsedTimeFromTsValue(m_elapsed).GetMilliSeconds() <<std::endl;
    if(TcpOptionTS::ElapsedTimeFromTsValue(m_epoch).GetSeconds() >= m_sstime)
      {  //m_data = 0;
        if ((/*m_ema_prev*/m_arrival_rate/m_expected_rate)<0.85)
           {
            m_isslow = true;
             //*1 m_bdp = arrival_rate*(m_delay/1000);
            m_bdp = (/*m_arrival_rate*/m_ema_prev/m_expected_rate)*m_thresh;
            //m_bdp = rcv_data;
            m_avail = std::abs( m_rw - m_bdp); 
           //m_avail = std::abs( m_rw - m_bdp);  
            std::cout<<"SLOW: sstime="<<m_sstime<<" elapsed: "<<TcpOptionTS::ElapsedTimeFromTsValue(m_elapsed).GetSeconds()<<" NEW BDP: "<<m_bdp<<"thresh: "<<m_thresh<<" rcv data: "<<rcv_data<<std::endl;        
           }
         else 
          {
            m_isslow = false;
            m_avail = 0;
          }
         //m_thresh = m_rw;
          //m_prev = m_thresh;
         m_thresh = m_isslow == true ? std::min(m_snd,m_rw): std::max(m_snd,m_rw);
        // m_sstime = ((m_delay*2)*(1+std::log2(m_thresh/packet->GetSize())))/1000;
         //1* m_sstime = ((m_delay*2)*(std::ceil(1+std::log2(m_thresh/packet->GetSize()))))/1000;
     double expected_data = std::abs(m_thresh /*- m_prev*/); 
     m_krt = std::ceil(1+std::log2(expected_data/m_mss));
     m_sstime = ((m_delay*2)*(m_krt))/1000;  
     m_elapsed = TcpOptionTS::NowToTsValue ();
     m_epoch = TcpOptionTS::NowToTsValue (); 
     m_ema_prev = 0;  
       }
     }
 //m_arrival_rate = 0;
 //m_expected_rate = 0;
 return m_isslow;
} 

void ProxyFlow::ComputeRw(uint16_t conn, uint16_t mss)
{
 double tti = 0.000125;
 double tbsmax = 11352;
 std::string scheduler ="MmWaveFlexTtiMacScheduler";
 //double allowedtbs = std::abs(11352 - CURRENT_TBS + CURRENT_TBS); 
 // double allowedtbs =CURRENT_TBS;

 Ptr<MmWaveHelper> mmwaveHelper1 = CreateObject<MmWaveHelper> ();
mmwaveHelper1->SetSchedulerType ("ns3::"+scheduler);
mmwaveHelper1->SetSchedulerType ("ns3::MmWaveFlexTtiMacScheduler");
mmwaveHelper1->SetAttribute ("PathlossModel", StringValue ("ns3::MmWave3gppPropagationLossModel"));
mmwaveHelper1->Initialize();
Ptr<MmWaveAmc> amc = CreateObject <MmWaveAmc> ( mmwaveHelper1->GetPhyMacConfigurable ());
 //int tbs = 0;
 double maxtbs = amc->GetTbSizeFromMcs (CURRENT_MCS,50);
 double allowedtbs = maxtbs;
 std::cout<<"MCS: "<<CURRENT_MCS<<std::endl;
 //double maxtbs = 
 m_rw = m_rtt == 0 ? /*mss*10*/std::pow(2,14): std::max (double(/*mss*10*/std::pow(2,14)),((allowedtbs/*CURRENT_TBS*//tti)*((m_rtt)/1000))/conn);
 DetectSlow();
}

void ProxyFlow::UpdateState(uint16_t conn, Ptr<Packet> packet)
{
 m_delay = m_delay == 0 ? 1 : m_delay;
 m_mss = packet->GetSize();
 m_data+=packet->GetSize();
 ComputeRw(conn,packet->GetSize());
 
   /* if (TcpOptionTS::ElapsedTimeFromTsValue(m_elapsed).GetSeconds() >= double(m_rtt/(1000*2)))
      {
       //double rate = (m_data)/(m_rtt/1000);
       //m_bdp = rate*(m_rtt/1000);
       m_bdp = m_data;
       m_elapsed = TcpOptionTS::NowToTsValue();
       //m_data = 0;
      }*/
   //double arrival_rate = m_data/TcpOptionTS::ElapsedTimeFromTsValue(m_epoch).GetSeconds() /*(m_bdp/2)/((m_rtt)/1000)*/; 
   //double expected_rate = (m_rw)/(m_rtt/1000);
   //}


}

void ProxyFlow::DetectStalled()
{
  std::cout<<" CHECK *-* elapsed="<<TcpOptionTS::ElapsedTimeFromTsValue(m_epoch).GetSeconds()<<" SStime++="<<m_sstime+(m_delay/1000)<<std::endl;
 if ((m_epoch != 0) && (TcpOptionTS::ElapsedTimeFromTsValue(m_epoch).GetSeconds() > m_sstime+(m_delay/1000)))
   {
     double arrival_rate = (m_data)/((2*m_delay/1000));
    double expected_rate = /*(std::pow(2,m_krt)-1)*/((m_thresh/*m_mss*/)/((2*m_delay/1000))); //m_thresh
             if ((arrival_rate==0)||(expected_rate==0)||((m_arrival_rate/m_expected_rate)<0.85))
           {
            m_isslow = true;
             //*1 m_bdp = arrival_rate*(m_delay/1000);
            m_bdp = (m_arrival_rate/m_expected_rate)*m_thresh;
            //m_bdp = rcv_data;
            m_avail = std::abs( m_rw - m_bdp); 
           //m_avail = std::abs( m_rw - m_bdp);  
            std::cout<<"STALLED FLOW: elapsed="<<TcpOptionTS::ElapsedTimeFromTsValue(m_epoch).GetMilliSeconds()<<std::endl; 
     m_thresh = m_isslow == true ? std::min(GetRw(),m_rw): std::max(GetRw(),m_rw);
     double expected_data = std::abs(m_thresh /*- m_prev*/); 
     m_krt = std::ceil(1+std::log2(expected_data/m_mss));
     m_sstime = ((m_delay*2)*(m_krt))/1000;  
     m_elapsed = TcpOptionTS::NowToTsValue ();
     m_epoch = TcpOptionTS::NowToTsValue (); 
     m_data = 0;       
           }
         else 
          {
            m_isslow = false;
            m_avail = 0;
          }
         //m_thresh = m_rw;
          //m_prev = m_thresh;
        
        // m_sstime = ((m_delay*2)*(1+std::log2(m_thresh/packet->GetSize())))/1000;
         //1* m_sstime = ((m_delay*2)*(std::ceil(1+std::log2(m_thresh/packet->GetSize()))))/1000;

   }
}

 double EpcEnbProxyApplication::Getfree(){
  double free_bdp = 0;
  for(auto it = m_proxyTab.begin(); it != m_proxyTab.end(); it++)
              {   
                    m_proxyTab[it->first].DetectStalled();
                   if (m_proxyTab[it->first].IsSlow()/*DetectSlow()*/)
                   free_bdp+=m_proxyTab[it->first].GetAvail();
                   //if (m_proxyTab[it->first].GetAvail() > 0)
                             //slow++;
                }
return free_bdp;
}
uint16_t EpcEnbProxyApplication::GetSlowN(){
uint16_t  slow = 0;
  for(auto it = m_proxyTab.begin(); it != m_proxyTab.end(); it++)
              {
                  if (m_proxyTab[it->first].IsSlow())
                             slow++;
                }
return slow;
}

static void
AddOptionTimestamp (TcpHeader& header, Ptr<const TcpOptionTS> ts)
{
  //NS_LOG_FUNCTION (this << header);
   if(header.HasOption(TcpOption::TS)){
     std::cout<<"NEW HEADER HAS TS OPTION---------------"<<std::endl;
     
   }
  Buffer buffer;
  Buffer buffer2(24);
  buffer2.AddAtStart (24);
  TcpHeader dummy;
  buffer.AddAtStart (header.GetSerializedSize ());
  //buffer2.Allocate (20);
  header.Serialize(buffer.Begin());
  //buffer.CopyData(buffer2,20);
  //header.Serialize(buffer2.Begin());
  //buffer2 = buffer.CreateFragment(2,32);
 // buffer2 = buffer;
  uint8_t * buff = (uint8_t*) malloc(20 * sizeof(uint8_t));
  buffer.CopyData(buff,20);
  buffer2.Deserialize(buff,20);
  //buffer2.RemoveAtEnd(32);
  //buffer2.Write(buffer.Begin(),buffer.Begin().Next(20));
 // buffer2.RemoveAtEnd(header.GetSerializedSize ()-12);
  //header.Deserialize(buffer2.Begin());
  dummy.Deserialize(buffer2.Begin());
  std::filebuf fb;
  fb.open ("test.txt",std::ios::out);
  std::ostream os(&fb);
  os<<"-----orig--------"<<std::endl;
  header.Print(os);
  os<<"--------------"<<std::endl;
  dummy.Print(os);
 // header.Serialize (buffer.CreateFragment(0,10).Begin ());
  std::cout<<"ORIG SIZE ="<<header.GetSerializedSize () <<std::endl;
  std::cout<<"MODIFIED SIZE ="<<dummy.GetSerializedSize () <<std::endl;
     if(dummy.HasOption(TcpOption::TS)){
     std::cout<<"MODIFIED HEADER HAS TS OPTION---------------"<<std::endl;
     
   }
 // Ptr<TcpOptionTS> option = CreateObject<TcpOptionTS> ();

  //option->SetTimestamp (TcpOptionTS::NowToTsValue ());
  //option->SetEcho (ts->GetTimestamp ());

 //header.AppendOption (option);
  //NS_LOG_INFO (m_node->GetId () << " Add option TS, ts=" <<
   //            option->GetTimestamp () << " echo=" << m_timestampToEcho);
}


	static void
		Ssthresh (Ptr<OutputStreamWrapper> stream, uint32_t oldSsthresh, uint32_t newSsthresh)
		{
			*stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << oldSsthresh << "\t" << newSsthresh << std::endl;
		}
	/*
	static void
		RttChange (Ptr<OutputStreamWrapper> stream, Time oldRtt, Time newRtt)
		{
			*stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << oldRtt.GetSeconds () << "\t" << newRtt.GetSeconds () << std::endl;
		}
	*/


	NS_LOG_COMPONENT_DEFINE ("EpcEnbProxyApplication");

	TypeId
		EpcEnbProxyApplication::GetTypeId (void)
		{
			static TypeId tid = TypeId ("ns3::EpcEnbProxyApplication")
				.SetParent<Object> ()
				.SetGroupName("Lte");

			return tid;
		}

	void
		EpcEnbProxyApplication::DoDispose (void)
		{
			NS_LOG_FUNCTION (this);
			for(auto it = m_proxyTcpSocketMap.begin(); it != m_proxyTcpSocketMap.end(); it++)
			{
				it->second = 0;
			}
                        
                        for(auto it = m_proxyRttSyn.begin(); it != m_proxyRttSyn.end(); it++)
			{
				it->second = 0;
			}
                        for(auto it = m_proxyRttAck.begin(); it != m_proxyRttAck.end(); it++)
			{
				it->second = 0;
			}
			m_proxyEnbSocket = 0;
		}


	EpcEnbProxyApplication::EpcEnbProxyApplication (Ptr<Node> proxyNode, Ipv4Address proxyAddress, uint16_t proxyTcpPort, Ptr<Socket> proxyEnbSocket, Ipv4Address proxyToEnbAddress,uint32_t proxyBufferSize)
		: //m_proxyTcpSocket (proxyTcpSocket),
			m_proxyEnbSocket (proxyEnbSocket),
			m_proxyToEnbAddress (proxyToEnbAddress),
			m_proxyToEnbUdpPort (8434),
			m_proxyNode (proxyNode),
			m_proxyAddress (proxyAddress),
			m_proxyTcpPort (proxyTcpPort),
			m_proxyBufferSize (proxyBufferSize)// fixed by the standard
	{
		NS_LOG_FUNCTION (this << proxyNode << proxyAddress << proxyTcpPort<< proxyEnbSocket << proxyToEnbAddress);
		//m_proxyTcpSocket->SetRecvCallback (MakeCallback (&EpcEnbProxyApplication::RecvFromTcpSocket, this));
		m_proxyEnbSocket->SetRecvCallback (MakeCallback (&EpcEnbProxyApplication::RecvFromEnbSocket, this));
		//m_proxyTcpSocket->GetObject<TcpSocketBase>()->SetSndBufSize(15*1024*1024);
		m_totalRx = 0;
		m_lastTotalRx = 0;
		m_currentAvailable = 0;
		m_lastAvailable = 0;
		m_count = 0;
		m_count_dep = 0;
		//Simulator::Schedule(Seconds(0.5),&EpcEnbProxyApplication::GetArrivalRate,this);
		m_delay = 0.03;
	}


	EpcEnbProxyApplication::~EpcEnbProxyApplication (void)
	{
		NS_LOG_FUNCTION (this);
	}
	/*
	   void 
	   EpcEnbProxyApplication::RecvFromTcpSocket (Ptr<Socket> socket)
	   {
	   NS_LOG_FUNCTION (this);  
	   NS_ASSERT (socket == m_proxyTcpSocket);
	   Ptr<Packet> packet = socket->Recv ();

	/// \internal
	/// Workaround for \bugid{231}
	//SocketAddressTag satag;
	//packet->RemovePacketTag (satag);
	Ptr<Packet> pCopy = packet-> Copy();

	TcpHeader tempTcpHeader;
	pCopy -> RemoveHeader(tempTcpHeader);
	}
	 */

	//All receiving packet processes are handled by this function
	void 
		EpcEnbProxyApplication::RecvFromEnbSocket (Ptr<Socket> socket)
		{
			NS_LOG_FUNCTION (this << socket);  
			NS_ASSERT (socket == m_proxyEnbSocket);
			Ptr<Packet> packet = socket -> Recv ();
                        //double rtt_first = ((Time)(socket->GetObject<TcpSocketBase>()->m_lastRtt)).GetMilliSeconds();
                        uint32_t pkt_time = TcpOptionTS::NowToTsValue ();

			//m_jitterEstimate.PrepareTx(packet);

			NS_LOG_LOGIC("Packet size before remove header: "<<packet->GetSize());
			GtpuHeader tempGtpuHeader;
			Ipv4Header tempIpv4Header;
			TcpHeader tempTcpHeader;
                       
                        

			packet -> RemoveHeader (tempGtpuHeader);
                        Ptr<Packet> p = packet;
			packet -> RemoveHeader (tempIpv4Header);
			packet -> RemoveHeader (tempTcpHeader);
                        TcpHeader tc = tempTcpHeader;

                          Ptr<const TcpOptionTS> ts;
                          Ptr<const TcpOptionWinScale> ws;
                          Ptr < TcpOptionWinScale> wsc;
                        Time oneway;
                        double mtime;
                        ts = DynamicCast<const TcpOptionTS> (tempTcpHeader.GetOption (TcpOption::TS));
                        if(tempTcpHeader.HasOption(TcpOption::WINSCALE)){
                        ws = DynamicCast<const TcpOptionWinScale> (tempTcpHeader.GetOption (TcpOption::WINSCALE));
                        
                        //wsc = DynamicCast<TcpOptionWinScale>(ws);
                        wsc = DynamicCast<TcpOptionWinScale> (ns3::TcpOption::CreateOption(TcpOption::WINSCALE));
                        uint8_t factor = 6;
                        wsc->SetScale(factor);
                        tempTcpHeader.AppendOption(DynamicCast< const TcpOption >(wsc));
                        std::cout<<"Find WinScale option"<<std::endl;
                        }


			Ipv4Header newIpv4Header = tempIpv4Header;
			//Set Ipv4 Header
			m_source = tempIpv4Header.GetDestination();
			m_dest = tempIpv4Header.GetSource();

			newIpv4Header.SetDestination(m_dest);
			newIpv4Header.SetSource(m_source);
			newIpv4Header.SetProtocol(6);
			newIpv4Header.SetTtl(64);

			TcpHeader newTcpHeader = tempTcpHeader;

                        oneway = TcpOptionTS::ElapsedTimeFromTsValue (ts->GetTimestamp());
                         std::cout<<"ONE WAY = "<<  oneway.GetMilliSeconds() <<std::endl;
                        
                        //Ptr< const TcpOption > tsoption = tempTcpHeader.GetOption(8);
                        //uint32_t tsval = (Ptr<const TcpOptionTS>(tsoption))->GetTimestamp();
                       // option->SetTimestamp (TcpOptionTS::NowToTsValue ());
                        
                        
			//Set TCP ack header
			SequenceNumber32 dataSeqNum = tempTcpHeader.GetSequenceNumber();
			uint16_t destPort = tempTcpHeader.GetSourcePort();
			uint16_t srcPort = tempTcpHeader.GetDestinationPort();
			newTcpHeader.SetSourcePort(srcPort);
			newTcpHeader.SetDestinationPort(destPort);
                        //newTcpHeader.SetWindowSize(static_cast<uint32_t>(1024*3));
                          //mtime = m_proxyRttSyn.find(srcPort) != m_proxyRttSyn.end() ? ts->GetTimestamp()-m_proxyRttSyn.find(srcPort)->second : 0;
                          //m_proxyRttSyn.insert(std::make_pair(destPort,ts->GetTimestamp()));
                          //m_proxyRttSyn[srcPort ] = ts->GetTimestamp(); 
                          //if ((mtime!=0) && (mtime > m_proxyRttAck.find(srcPort)->second/2))
                          //if (m_proxyRttAck.find(srcPort) != m_proxyRttAck.end())
                              mtime = /*std::min((double)*/oneway.GetMilliSeconds()/*,m_proxyRttAck.find(srcPort)->second)*/;
                                  
                          m_proxyRttAck[ srcPort ] = mtime <= 1 ? 1 : mtime; // 2*mtime; //(std::make_pair(destPort,ts->GetTimestamp()));
                          std::cout<<"RIGHT RTT = "<<2*m_proxyRttAck.find(srcPort)->second<<": "<<ts->GetTimestamp()<<std::endl;
                          TcpHeader testTCP = newTcpHeader; 

                         // Create Receive Buffer

                        if (m_rxBuffer.find(srcPort) == m_rxBuffer.end()){
                            m_rxBuffer[srcPort] = CreateObject<TcpRxBuffer> ();
                            m_rxBuffer[srcPort]->SetMaxBufferSize (900000);
                        }
                        /***************************** end Create Buffer ***********/
                         
                        //  AddOptionTimestamp(testTCP,ts);

			//#1 classify by tcp header: SYN
			if(tempTcpHeader.GetFlags()==TcpHeader::SYN)
			{       // Update Receive Buffer
                                m_rxBuffer[srcPort]->SetNextRxSequence (tempTcpHeader.GetSequenceNumber () + SequenceNumber32 (1));
                                
                                /***************************** end Update Buffer ***********/
				//Start Proxy TCP communication
				NS_LOG_LOGIC("First packet from Server");

                                // create TECC TBS CCA socket
                                //ConfigStore Config;
                                TypeId tid = TypeId::LookupByName ("ns3::TcpTecc");
                                std::stringstream nodeId;
                                nodeId << m_proxyNode->GetId ();
                                std::string specificNode = "/NodeList/" + nodeId.str () + "/$ns3::TcpL4Protocol/SocketType";
                                Config::Set (specificNode, TypeIdValue (tid));

				Ptr<Socket> proxyTcpSocket = Socket::CreateSocket (m_proxyNode, TypeId::LookupByName ("ns3::TcpSocketFactory"));
				int retval = proxyTcpSocket->Bind (InetSocketAddress (m_proxyAddress, m_proxyTcpPort));
				m_proxyTcpPort++;
				NS_ASSERT (retval == 0);
				proxyTcpSocket->GetObject<TcpSocketBase>()->SetSndBufSize(m_proxyBufferSize);
				
				/*
				std::ostringstream fileName;
				fileName<<"proxyRtt"<<srcPort<<".txt";
				AsciiTraceHelper asciiTraceHelper;
				Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream (fileName.str ().c_str ());
				proxyTcpSocket->GetObject<TcpSocketBase>()->TraceConnectWithoutContext ("RTT", MakeBoundCallback (&RttChange,stream));
				*/
				std::ostringstream fileName2;
				fileName2<<"proxySst"<<srcPort<<".txt";
				AsciiTraceHelper asciiTraceHelper2;
				Ptr<OutputStreamWrapper> stream2 = asciiTraceHelper2.CreateFileStream (fileName2.str ().c_str ());
				proxyTcpSocket->GetObject<TcpSocketBase>()->TraceConnectWithoutContext ("SlowStartThreshold", MakeBoundCallback (&Ssthresh,stream2));
				

				Simulator::Schedule(Simulator::Now() + Seconds(0.5),&EpcEnbProxyApplication::GetDepartureRate, this, proxyTcpSocket);

				Address tcpToEnbAddress (InetSocketAddress(m_source,srcPort));
				proxyTcpSocket->Connect(tcpToEnbAddress);

				m_proxyTcpSocketMap.insert(std::make_pair(srcPort,proxyTcpSocket));

				//Send SYN|ACK packet to server, set SYN|ACK packet
				SequenceNumber32 dataSeqNum = SequenceNumber32(0);
				newTcpHeader.SetSequenceNumber(dataSeqNum);
				newTcpHeader.SetFlags(TcpHeader::SYN|TcpHeader::ACK);
				newTcpHeader.SetAckNumber(SequenceNumber32(1));
                                //newTcpHeader.SetWindowSize(static_cast<uint32_t>(1024*3));
                        wsc = DynamicCast<TcpOptionWinScale> (ns3::TcpOption::CreateOption(TcpOption::WINSCALE));
                        wsc->SetScale(14);
                        //newTcpHeader.AppendOption(DynamicCast< const TcpOption >(wsc));
                        std::cout<<"Set WinScale option"<<std::endl;
                       // }
                                                               // proxyTcpSocket->GetObject<TcpSocketBase>()->SetRcvBufSize(static_cast<uint32_t>(awndSize));
                               TcpHeader dummy;
                               dummy.SetSequenceNumber(dataSeqNum);
                               dummy.SetFlags(TcpHeader::SYN|TcpHeader::ACK);
			       //dummy.SetWindowSize(static_cast<uint32_t>(awndSize));
                               dummy.SetAckNumber(SequenceNumber32(1));
                               //dummy.SetSequenceNumber(newTcpHeader.GetSequenceNumber());
                               dummy.SetDestinationPort(newTcpHeader.GetDestinationPort());
                               dummy.SetSourcePort(newTcpHeader.GetSourcePort());
                               Ptr<TcpOptionTS> option = CreateObject<TcpOptionTS> ();
                               Ptr<TcpOptionSackPermitted> sackopt = CreateObject<TcpOptionSackPermitted> ();

                             option->SetTimestamp (TcpOptionTS::NowToTsValue ());
                             option->SetEcho (ts->GetTimestamp ());
                             //option->SetEcho (TcpOptionTS::NowToTsValue ());
                             dummy.AppendOption (option);
                             dummy.AppendOption(DynamicCast< const TcpOption >(wsc));
                             //dummy.AppendOption(sackopt);

				Ptr<Packet> ackPacket = Create<Packet> ();
                               // AddOptionTimestamp(newTcpHeader,ts);
				//ackPacket->AddHeader(newTcpHeader);
                                ackPacket->AddHeader(dummy);
                             
				ackPacket->AddHeader(newIpv4Header);
				ackPacket->AddHeader(tempGtpuHeader);

				NS_LOG_LOGIC("Packet size: "<<packet->GetSize());
				NS_LOG_LOGIC("Ipv4 Header: "<<newIpv4Header);
				NS_LOG_LOGIC("Tcp Header: "<<newTcpHeader);
				NS_LOG_LOGIC("Gtpu Header: "<<tempGtpuHeader);
                                
                                

				uint32_t flags = 0;
                                
				m_proxyEnbSocket->SendTo (ackPacket, flags, InetSocketAddress (m_proxyToEnbAddress, m_proxyToEnbUdpPort));
			}
			//#2 ACK after SYN|ACK
			else if(packet->GetSize() == 0)
			{
				//Do nothing.. just wait for data packet
                           //std::cout<<"ONEWAY RTTAck = "<<oneway.GetMilliSeconds()<<": "<<ts->GetTimestamp()<<std::endl;
                              // Update Receive Buffer
                                m_rxBuffer[srcPort]->SetNextRxSequence (tempTcpHeader.GetSequenceNumber () + SequenceNumber32 (1));
                                
                                /***************************** end Update Buffer ***********/
			}
			//#3 receive data packet
			else
			{      
                                 // Update Receive Buffer
                                m_rxBuffer[srcPort]->Add(p,tc);
                                uint32_t w;
                                //SequenceNumber32 expectedSeq = m_rxBuffer->NextRxSequence ();
                                w = (m_rxBuffer[srcPort]->MaxRxSequence () > m_rxBuffer[srcPort]->NextRxSequence ()) ? m_rxBuffer[srcPort]->MaxRxSequence () - m_rxBuffer[srcPort]->NextRxSequence () : 0;
                                
                                /***************************** end Update Buffer ***********/
                                
                                // AddOptionTimestamp(newTcpHeader,ts);

				Ptr<Socket> proxyTcpSocket = m_proxyTcpSocketMap.find(srcPort)->second;
				//Send data packet from proxy tcp to user
                                /******** MADI ***************/
                                uint32_t awndSize = 0;
                                double rtt_last = ((Time)(proxyTcpSocket->GetObject<TcpSocketBase>()->m_tcb->m_lastRtt)).GetMilliSeconds();
                                double rtt_min =  proxyTcpSocket->GetObject<TcpSocketBase>()->m_minRtt;
                                rtt_min = rtt_min  == 0 ? /*4*/ std::max(rtt_last,1.0) : rtt_min;
                                proxyTcpSocket->GetObject<TcpSocketBase>()->m_minRtt = std::min(rtt_min,rtt_last);
                                rtt_min = proxyTcpSocket->GetObject<TcpSocketBase>()->m_minRtt;
                               double tti = 0.125/1000;
                               // uint16_t active_flows = std::abs(double(m_proxyTcpSocketMap.size()-((uint16_t)(m_proxyStopped.size()))));
                                 uint16_t active_flows = std::abs(double(m_proxyTcpSocketMap.size()-(m_dead_flows)));
                                //awndSize = rtt_min == 0 ? awndSize : (CURRENT_TBS/tti)*(rtt_min/1000);
                                //awndSize = static_cast<uint32_t>(double(1024*3));
                                //awndSize = rtt_min == 0 ? packet->GetSize()*10: std::max (double(packet->GetSize()),((CURRENT_TBS/tti)*((rtt_min+m_proxyRttAck.find(destPort)->second)/1000))/active_flows);
                                /****************************/
                                  std::cout<<"src port = "<<srcPort<<std::endl;
                                  if (m_proxyTab.find(srcPort) == m_proxyTab.end()){
                                  m_proxyTab[srcPort] = ProxyFlow(rtt_min+2*m_proxyRttAck.find(srcPort)->second,m_proxyRttAck.find(srcPort)->second);
                                 // m_proxyTab[srcPort].rxBuffer = CreateObject<TcpRxBuffer> ();
                                  }
                                  m_proxyTab[srcPort].SetRtt(/*rtt_min,*/rtt_min+2*m_proxyRttAck.find(srcPort)->second,m_proxyRttAck.find(srcPort)->second);
                                  //if(m_proxyTcpSocketMap.find(srcPort)->second->GetObject<TcpSocketBase>()->m_state <= ns3::TcpSocket::TcpStates_t::ESTABLISHED)
                                 m_proxyTab[srcPort].UpdateState(active_flows,packet);
                                 awndSize = m_proxyTab[srcPort].IsSlow() == true ? m_proxyTab[srcPort].GetRw() : static_cast<uint32_t>(m_proxyTab[srcPort].GetRw()+(/*(m_proxyAvail)*/this->Getfree()/(m_proxyTcpSocketMap.size() - this->GetSlowN())));

                               

                                //m_proxyTab[srcPort].UpdateSndWindow(current);

                                  //m_proxyTab[srcPort].UpdateState(active_flows,packet);
#if 0
/*std::min(((CURRENT_TBS/tti)*((rtt_min+m_proxyRttAck.find(srcPort)->second)/1000))/active_flows, m_proxyTab[srcPort].GetRw()+((m_proxyAvail)/(active_flows/*-m_proxySlowFlows*/)))*/
#endif
                                
                               // proxyTcpSocket->GetObject<TcpSocketBase>()->m_tcb->m_rbw=awndSize;
                                proxyTcpSocket->GetObject<TcpSocketBase>()->m_tcb->m_rbw=awndSize;
				proxyTcpSocket->Send(packet);
				//Set advertise window
				Ptr<TcpTxBuffer> proxyTxBuffer = proxyTcpSocket->GetObject<TcpSocketBase>()->GetTxBuffer();

				/*OLD*/ //awndSize = proxyTxBuffer->Available()-m_delay*m_departureRate;
                               // uint32_t awndSize = static_cast<uint32_t>(1024*3);

				std::cout<<"Max: "<<proxyTxBuffer->MaxBufferSize() <<" Avail: "<<proxyTxBuffer->Available()<<" Awnd: "<<awndSize<<std::endl;
#if 0

				/*OLD*/if(proxyTxBuffer->Available() < m_delay*m_departureRate||awndSize<1400)
				{
					awndSize =packet->GetSize();
				}/*OLD*/
#endif

				
                                NS_LOG_LOGIC("Proxy tcp's awnd size is "<< awndSize);
			        std::cout<<"awndSize = "<<awndSize<<std::endl;

				std::cout<<"Tail Sequence: "<<proxyTxBuffer->TailSequence() << std::endl;

				//Send Early ACK packet to server, set ack number

                                // awndSize = 10;
                                //awndSize = std::ceil(awndSize/64);
                                //uint32_t current = awndSize*64;
                                //awndSize = awndSize<=14400 ? 14400 : std::ceil(awndSize/16384);
                                awndSize = awndSize<=16384 ? 1 : std::ceil(awndSize/16384);
                               // uint32_t current = awndSize<=14400 ? 14400 : awndSize*16384;
                               // uint32_t current = /*awndSize<=14400 ? 14400 :*/awndSize*64;
                                uint32_t current = awndSize*16384;
                                m_proxyTab[srcPort].UpdateSndWindow(current);

				uint32_t dataSize = packet->GetSize();
				SequenceNumber32 AckNum = dataSeqNum + dataSize;
				newTcpHeader.SetAckNumber(AckNum);
				newTcpHeader.SetFlags(TcpHeader::ACK);
				newTcpHeader.SetWindowSize(static_cast<uint32_t>(awndSize));
                        //wsc = DynamicCast<TcpOptionWinScale> (ns3::TcpOption::CreateOption(TcpOption::WINSCALE));
                        //wsc->SetScale(6);
                        //newTcpHeader.AppendOption(DynamicCast< const TcpOption >(wsc));
                               // proxyTcpSocket->GetObject<TcpSocketBase>()->SetRcvBufSize(static_cast<uint32_t>(awndSize));
                               TcpHeader dummy = TcpHeader();
                               //if (w!=0)
                                // AckNum = 
                                
                               dummy.SetAckNumber(m_rxBuffer[srcPort]->NextRxSequence ()); //dummy.SetAckNumber(AckNum);
                               dummy.SetSequenceNumber(SequenceNumber32(1));//dummy.SetSequenceNumber(newTcpHeader.GetSequenceNumber());
                               dummy.SetFlags(TcpHeader::ACK);
			       dummy.SetWindowSize(static_cast<uint32_t>(awndSize));
                               dummy.SetDestinationPort(newTcpHeader.GetDestinationPort());
                               dummy.SetSourcePort(newTcpHeader.GetSourcePort());
                               Ptr<TcpOptionTS> option = CreateObject<TcpOptionTS> ();
                               
                             option->SetTimestamp (TcpOptionTS::NowToTsValue ());
                             option->SetEcho (ts->GetTimestamp ());
                             //option->SetEcho (TcpOptionTS::NowToTsValue ());
                             dummy.AppendOption (option);
                             dummy.UpdateHeaderLength();
                             std::cout << "ACK length === : "<<dummy.GetLength ()<<std::endl;

				Ptr<Packet> ackPacket = Create<Packet> ();
				//ackPacket->AddHeader(newTcpHeader);
                                ackPacket->AddHeader(dummy);
                                newIpv4Header.SetPayloadSize(ackPacket->GetSize());
				ackPacket->AddHeader(newIpv4Header);
				ackPacket->AddHeader(tempGtpuHeader);

				m_totalRx += dataSize;
                                /********************************/
                                 if (m_rxBuffer[srcPort]->Available() > 10)
                                     m_rxBuffer[srcPort]->Extract(m_rxBuffer[srcPort]->Available());
                                /********************************/
				uint32_t flags = 0;
				m_proxyEnbSocket->SendTo (ackPacket, flags, InetSocketAddress (m_proxyToEnbAddress, m_proxyToEnbUdpPort));
                                std::cout<< "** PKT time = "<<TcpOptionTS::ElapsedTimeFromTsValue(pkt_time).GetNanoSeconds()<<" us"<<std::endl;

                                std::cout << "TBS : "<<CURRENT_TBS<< " -- RTT_last = "<<rtt_last<< "ms"<<" RTT_min = "<<proxyTcpSocket->GetObject<TcpSocketBase>()->m_minRtt<<" ms"<<static_cast<double>(proxyTcpSocket->GetObject<TcpSocketBase>()->GetSndBufSize())<<" awsnd: "<<current/*awndSize*16384*/<<std::endl;
 std::cout << "src: "<<newIpv4Header.GetSource()<<" dst: "<<newIpv4Header.GetDestination()<<std::endl;
 fPlotQueue << Simulator::Now ().GetSeconds () << "," << current/*awndSize*/ << "," << rtt_last + 2*m_proxyRttAck.find(srcPort)->second  << "," << srcPort<< std::endl;

			}
                 //std::vector<uint16_t> v;
                uint16_t conn = 0;
                double free_bdp = 0;
                uint16_t slow = 0;
             	for(auto it = m_proxyTcpSocketMap.begin(); it != m_proxyTcpSocketMap.end(); it++)
			{
				if (it->second->GetObject<TcpSocketBase>()->m_state > ns3::TcpSocket::TcpStates_t::ESTABLISHED)
                                   {
                                   //m_proxyStopped.push_back(it->first);
                                   // m_proxyTab[it->first].SetAvail(0);
                                    conn++;
                                    std::cout << "state: "<<it->second->GetObject<TcpSocketBase>()->m_state<<std::endl;
                                    }
			}
                    /* for(auto it = m_proxyTab.begin(); it != m_proxyTab.end(); it++)
                       {
                         free_bdp+=m_proxyTab[it->first].GetAvail();
                         if (m_proxyTab[it->first].GetAvail() > 0)
                             slow++;
                       }*/
                     m_dead_flows = conn;
                     //m_proxyAvail =  free_bdp; 
                   //  m_proxySlowFlows = slow;
                     std::cout << "AVAILABLE ==== "<<m_proxyAvail<<std::endl;   
                    //for (auto i = v.begin(); i != v.end(); ++i)
			//m_proxyTcpSocketMap.erase(*i);
                        
		}



	void
		EpcEnbProxyApplication::GetArrivalRate ()
		{
			NS_LOG_FUNCTION (this);
			m_arrivalRate = (m_totalRx - m_lastTotalRx)/(double)(0.1);
			m_count++;
			m_lastTotalRx = m_totalRx;
			//std::cout<<"Arrival rate is "<<m_arrivalRate<<std::endl;
			Simulator::Schedule(MilliSeconds(100),&EpcEnbProxyApplication::GetArrivalRate,this);
		}

	void
		EpcEnbProxyApplication::GetDepartureRate (Ptr <Socket> proxyTcpSocket)
		{
			NS_LOG_FUNCTION (this);
			m_currentAvailable = proxyTcpSocket->GetObject<TcpSocketBase>()->GetTxBuffer()->Available();
			m_departureRate = (m_lastAvailable - m_currentAvailable)/(double)(0.1);
			if(m_currentAvailable > m_lastAvailable)
			{
				m_departureRate = 0;
			}
			m_count_dep++;
			m_lastAvailable = m_currentAvailable;
			//std::cout<<"Current: "<<m_currentAvailable<<" Departure rate is "<<m_departureRate<<std::endl;
			//m_departureRate = 0;
			Simulator::Schedule(MilliSeconds(100),&EpcEnbProxyApplication::GetDepartureRate,this,proxyTcpSocket);
		}

	void 
		EpcEnbProxyApplication::SendToEnbSocket (Ptr<Packet> packet)
		{       
			NS_LOG_FUNCTION (this << packet << packet->GetSize ());
                         // madi
       Ptr<Packet> copy = packet->Copy ();
       Ipv4Header iph;
       copy->RemoveHeader (iph);
       std::cout << "src: "<<iph.GetSource()<<" dst: "<<iph.GetDestination()<<std::endl;
      // Ptr<Node> mdev = this->GetNode();
      // Ptr<Ipv4> ipv4 = mdev->GetObject<Ipv4> (); // Get Ipv4 instance of the node
      // Ipv4Address addr = ipv4->GetAddress (1, 0).GetLocal (); // Get Ipv4InterfaceAddress of xth interface.
     //  std::cout << "Node addr: "<<addr<<std::endl;
      /***********************/
			int sentBytes = m_proxyEnbSocket->Send (packet);
			NS_ASSERT (sentBytes > 0);
		}

}  // namespace ns3
