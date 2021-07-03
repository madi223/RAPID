/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 Natale Patriciello <natale.patriciello@gmail.com>
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
 */
#include "tcp-congestion-ops.h"
#include "tcp-socket-base.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TcpCongestionOps");

NS_OBJECT_ENSURE_REGISTERED (TcpCongestionOps);

TypeId
TcpCongestionOps::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpCongestionOps")
    .SetParent<Object> ()
    .SetGroupName ("Internet")
  ;
  return tid;
}

TcpCongestionOps::TcpCongestionOps () : Object ()
{
}

TcpCongestionOps::TcpCongestionOps (const TcpCongestionOps &other) : Object (other)
{
}

TcpCongestionOps::~TcpCongestionOps ()
{
}


// RENO

NS_OBJECT_ENSURE_REGISTERED (TcpNewReno);

TypeId
TcpNewReno::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpNewReno")
    .SetParent<TcpCongestionOps> ()
    .SetGroupName ("Internet")
    .AddConstructor<TcpNewReno> ()
  ;
  return tid;
}

TcpNewReno::TcpNewReno (void) : TcpCongestionOps ()
{
  NS_LOG_FUNCTION (this);
}

TcpNewReno::TcpNewReno (const TcpNewReno& sock)
  : TcpCongestionOps (sock)
{
  NS_LOG_FUNCTION (this);
}

TcpNewReno::~TcpNewReno (void)
{
}

/**
 * \brief Tcp NewReno slow start algorithm
 *
 * Defined in RFC 5681 as
 *
 * > During slow start, a TCP increments cwnd by at most SMSS bytes for
 * > each ACK received that cumulatively acknowledges new data.  Slow
 * > start ends when cwnd exceeds ssthresh (or, optionally, when it
 * > reaches it, as noted above) or when congestion is observed.  While
 * > traditionally TCP implementations have increased cwnd by precisely
 * > SMSS bytes upon receipt of an ACK covering new data, we RECOMMEND
 * > that TCP implementations increase cwnd, per:
 * >
 * >    cwnd += min (N, SMSS)                      (2)
 * >
 * > where N is the number of previously unacknowledged bytes acknowledged
 * > in the incoming ACK.
 *
 * The ns-3 implementation respect the RFC definition. Linux does something
 * different:
 * \verbatim
u32 tcp_slow_start(struct tcp_sock *tp, u32 acked)
  {
    u32 cwnd = tp->snd_cwnd + acked;

    if (cwnd > tp->snd_ssthresh)
      cwnd = tp->snd_ssthresh + 1;
    acked -= cwnd - tp->snd_cwnd;
    tp->snd_cwnd = min(cwnd, tp->snd_cwnd_clamp);

    return acked;
  }
  \endverbatim
 *
 * As stated, we want to avoid the case when a cumulative ACK increases cWnd more
 * than a segment size, but we keep count of how many segments we have ignored,
 * and return them.
 *
 * \param tcb internal congestion state
 * \param segmentsAcked count of segments acked
 * \return the number of segments not considered for increasing the cWnd
 */
uint32_t
TcpNewReno::SlowStart (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  NS_LOG_FUNCTION (this << tcb << segmentsAcked);

  if (segmentsAcked >= 1)
    {
      tcb->m_cWnd += tcb->m_segmentSize;
      NS_LOG_INFO ("In SlowStart, updated to cwnd " << tcb->m_cWnd << " ssthresh " << tcb->m_ssThresh);
      return segmentsAcked - 1;
    }

  return 0;
}

/**
 * \brief NewReno congestion avoidance
 *
 * During congestion avoidance, cwnd is incremented by roughly 1 full-sized
 * segment per round-trip time (RTT).
 *
 * \param tcb internal congestion state
 * \param segmentsAcked count of segments acked
 */
void
TcpNewReno::CongestionAvoidance (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  NS_LOG_FUNCTION (this << tcb << segmentsAcked);

  if (segmentsAcked > 0)
    {
      double adder = static_cast<double> (tcb->m_segmentSize * tcb->m_segmentSize) / tcb->m_cWnd.Get ();
      adder = std::max (1.0, adder);
      tcb->m_cWnd += static_cast<uint32_t> (adder);
      NS_LOG_INFO ("In CongAvoid, updated to cwnd " << tcb->m_cWnd <<
                   " ssthresh " << tcb->m_ssThresh);
    }
}

/**
 * \brief Try to increase the cWnd following the NewReno specification
 *
 * \see SlowStart
 * \see CongestionAvoidance
 *
 * \param tcb internal congestion state
 * \param segmentsAcked count of segments acked
 */
void
TcpNewReno::IncreaseWindow (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  NS_LOG_FUNCTION (this << tcb << segmentsAcked);

  if (tcb->m_cWnd < tcb->m_ssThresh)
    {
      segmentsAcked = SlowStart (tcb, segmentsAcked);
    }

  if (tcb->m_cWnd >= tcb->m_ssThresh)
    {
      CongestionAvoidance (tcb, segmentsAcked);
    }

  /* At this point, we could have segmentsAcked != 0. This because RFC says
   * that in slow start, we should increase cWnd by min (N, SMSS); if in
   * slow start we receive a cumulative ACK, it counts only for 1 SMSS of
   * increase, wasting the others.
   *
   * // Uncorrect assert, I am sorry
   * NS_ASSERT (segmentsAcked == 0);
   */
}

std::string
TcpNewReno::GetName () const
{
  return "TcpNewReno";
}

uint32_t
TcpNewReno::GetSsThresh (Ptr<const TcpSocketState> state,
                         uint32_t bytesInFlight)
{
  NS_LOG_FUNCTION (this << state << bytesInFlight);

  return std::max (2 * state->m_segmentSize, bytesInFlight / 2);
}

Ptr<TcpCongestionOps>
TcpNewReno::Fork ()
{
  return CopyObject<TcpNewReno> (this);
}

// TECC

NS_OBJECT_ENSURE_REGISTERED (TcpTecc);

TypeId
TcpTecc::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpTecc")
    .SetParent<TcpCongestionOps> ()
    .SetGroupName ("Internet")
    .AddConstructor<TcpTecc> ()
  ;
  return tid;
}

TcpTecc::TcpTecc (void) : TcpCongestionOps ()
{
  NS_LOG_FUNCTION (this);
}

TcpTecc::TcpTecc (const TcpTecc& sock)
  : TcpCongestionOps (sock)
{
  NS_LOG_FUNCTION (this);
  //conn++;
  
}

TcpTecc::~TcpTecc (void)
{

}

uint32_t
TcpTecc::SlowStart (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  NS_LOG_FUNCTION (this << tcb << segmentsAcked);
  /*fPlotQueue3 << Simulator::Now ().GetSeconds () << "," << tcb->m_cWnd.Get ()  << "," << ((Time)(tcb->m_lastRtt)).GetMilliSeconds () << "," << (tcb->m_cWnd.Get()*8)/(((Time)(tcb->m_lastRtt)).GetSeconds ()*1000000)<< "," << (double(CURRENT_TBS)*8)/(0.001*1000000) << "," << CURRENT_TBS << "," <<(uint32_t)(tcb->m_bytesInFlight) << ",SS" << std::endl;*/

  if (segmentsAcked >= 1)
    {
      /********** Min RTT**************/
      //double rttmin = tcb->m_minRtt;

      /********** Current RTT**************/
     // double curr_RTT = ((Time)(tcb->m_lastRtt)).GetSeconds (); 
      
      /********** Delay overhead computation **************/
      /***************************************************/
                                                        // Update RTTmin
      
      /********** Compute expected RAN BW  ***************/
      /********** update CWND and pacing rate from RAN BW ***/
      /***************************************************/
      tcb->m_cWnd =  static_cast<uint32_t> (tcb->m_rbw);                             // Update CWND 

      return segmentsAcked - 1;
    }

  std::cout << "SS: RNTI=" << CURRENT_RNTI << " Flow BDP=" << tcb->m_cWnd << std::endl;

  return 0;
}

/**
 * \brief NewReno congestion avoidance
 *
 * During congestion avoidance, cwnd is incremented by roughly 1 full-sized
 * segment per round-trip time (RTT).
 *
 * \param tcb internal congestion state
 * \param segmentsAcked count of segments acked
 */
void
TcpTecc::CongestionAvoidance (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  NS_LOG_FUNCTION (this << tcb << segmentsAcked);

  if (segmentsAcked > 0)
    {
     tcb->m_cWnd =  static_cast<uint32_t> (tcb->m_rbw); 
      NS_LOG_INFO ("In CongAvoid, updated to cwnd " << tcb->m_cWnd <<" ssthresh " << tcb->m_ssThresh);

    }
  std::cout << "CA: RNTI=" << CURRENT_RNTI << " Flow BDP=" << tcb->m_cWnd<< std::endl;
}

/**
 * \brief Try to increase the cWnd following the NewReno specification
 *
 * \see SlowStart
 * \see CongestionAvoidance
 *
 * \param tcb internal congestion state
 * \param segmentsAcked count of segments acked
 */
void
TcpTecc::IncreaseWindow (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  NS_LOG_FUNCTION (this << tcb << segmentsAcked);

  if (tcb->m_cWnd < tcb->m_ssThresh)
    {
      segmentsAcked = SlowStart (tcb, segmentsAcked);
    }

  if (tcb->m_cWnd >= tcb->m_ssThresh)
    {
      CongestionAvoidance (tcb, segmentsAcked);
    }

  /* At this point, we could have segmentsAcked != 0. This because RFC says
   * that in slow start, we should increase cWnd by min (N, SMSS); if in
   * slow start we receive a cumulative ACK, it counts only for 1 SMSS of
   * increase, wasting the others.
   *
   * // Incorrect assert, I am sorry
   * NS_ASSERT (segmentsAcked == 0);
   */
}

std::string
TcpTecc::GetName () const
{
  return "TcpTecc";
}

uint32_t
TcpTecc::GetSsThresh (Ptr<const TcpSocketState> state,
                         uint32_t bytesInFlight)
{
  NS_LOG_FUNCTION (this << state << bytesInFlight);

  return std::max (2 * state->m_segmentSize, bytesInFlight / 2);
}

void
TcpTecc::ReduceCwnd (Ptr<TcpSocketState> tcb)
{
  NS_LOG_FUNCTION (this << tcb);

  //tcb->m_cWnd = std::max (tcb->m_cWnd.Get () / 2, tcb->m_segmentSize);


}

Ptr<TcpCongestionOps>
TcpTecc::Fork ()
{
  return CopyObject<TcpTecc> (this);
}

// TECCv2

NS_OBJECT_ENSURE_REGISTERED (TcpTeccv2);

TypeId
TcpTeccv2::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpTeccv2")
    .SetParent<TcpCongestionOps> ()
    .SetGroupName ("Internet")
    .AddConstructor<TcpTeccv2> ()
  ;
  return tid;
}

TcpTeccv2::TcpTeccv2 (void) : TcpCongestionOps ()
{
  NS_LOG_FUNCTION (this);
}

TcpTeccv2::TcpTeccv2 (const TcpTeccv2& sock)
  : TcpCongestionOps (sock)
{
  NS_LOG_FUNCTION (this);
  //conn++;
  
}

TcpTeccv2::~TcpTeccv2 (void)
{

}

uint32_t
TcpTeccv2::SlowStart (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  NS_LOG_FUNCTION (this << tcb << segmentsAcked);
  /*fPlotQueue3 << Simulator::Now ().GetSeconds () << "," << tcb->m_cWnd.Get ()  << "," << ((Time)(tcb->m_lastRtt)).GetMilliSeconds () << "," << (tcb->m_cWnd.Get()*8)/(((Time)(tcb->m_lastRtt)).GetSeconds ()*1000000)<< "," << (double(CURRENT_TBS)*8)/(0.001*1000000) << "," << CURRENT_TBS << "," <<(uint32_t)(tcb->m_bytesInFlight) << ",SS" << std::endl;*/

  if (segmentsAcked >= 1)
    {
      /********** Min RTT**************/
      //double rttmin = tcb->m_minRtt;

      /********** Current RTT**************/
     // double curr_RTT = ((Time)(tcb->m_lastRtt)).GetSeconds (); 
      
      /********** Delay overhead computation **************/
      /***************************************************/
                                                        // Update RTTmin
      
      /********** Compute expected RAN BW  ***************/
      /********** update CWND and pacing rate from RAN BW ***/
      /***************************************************/
      tcb->m_cWnd =  static_cast<uint32_t> (tcb->m_rw);                             // Update CWND 

      return segmentsAcked - 1;
    }

  std::cout << "SS: RNTI=" << CURRENT_RNTI << " Flow BDP=" << tcb->m_cWnd << std::endl;

  return 0;
}

/**
 * \brief NewReno congestion avoidance
 *
 * During congestion avoidance, cwnd is incremented by roughly 1 full-sized
 * segment per round-trip time (RTT).
 *
 * \param tcb internal congestion state
 * \param segmentsAcked count of segments acked
 */
void
TcpTeccv2::CongestionAvoidance (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  NS_LOG_FUNCTION (this << tcb << segmentsAcked);

  if (segmentsAcked > 0)
    {
     tcb->m_cWnd =  static_cast<uint32_t> (tcb->m_rw);  
      NS_LOG_INFO ("In CongAvoid, updated to cwnd " << tcb->m_cWnd <<" ssthresh " << tcb->m_ssThresh);

    }
  std::cout << "CA: RNTI=" << CURRENT_RNTI << " Flow BDP=" << tcb->m_cWnd<< std::endl;
}

/**
 * \brief Try to increase the cWnd following the NewReno specification
 *
 * \see SlowStart
 * \see CongestionAvoidance
 *
 * \param tcb internal congestion state
 * \param segmentsAcked count of segments acked
 */
void
TcpTeccv2::IncreaseWindow (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  NS_LOG_FUNCTION (this << tcb << segmentsAcked);

  if (tcb->m_cWnd < tcb->m_ssThresh)
    {
      segmentsAcked = SlowStart (tcb, segmentsAcked);
    }

  if (tcb->m_cWnd >= tcb->m_ssThresh)
    {
      CongestionAvoidance (tcb, segmentsAcked);
    }

  /* At this point, we could have segmentsAcked != 0. This because RFC says
   * that in slow start, we should increase cWnd by min (N, SMSS); if in
   * slow start we receive a cumulative ACK, it counts only for 1 SMSS of
   * increase, wasting the others.
   *
   * // Incorrect assert, I am sorry
   * NS_ASSERT (segmentsAcked == 0);
   */
}

std::string
TcpTeccv2::GetName () const
{
  return "TcpTeccv2";
}

uint32_t
TcpTeccv2::GetSsThresh (Ptr<const TcpSocketState> state,
                         uint32_t bytesInFlight)
{
  NS_LOG_FUNCTION (this << state << bytesInFlight);

  return std::max (2 * state->m_segmentSize, bytesInFlight / 2);
}

void
TcpTeccv2::ReduceCwnd (Ptr<TcpSocketState> tcb)
{
  NS_LOG_FUNCTION (this << tcb);

  //tcb->m_cWnd = std::max (tcb->m_cWnd.Get () / 2, tcb->m_segmentSize);


}

Ptr<TcpCongestionOps>
TcpTeccv2::Fork ()
{
  return CopyObject<TcpTeccv2> (this);
}


} // namespace ns3

