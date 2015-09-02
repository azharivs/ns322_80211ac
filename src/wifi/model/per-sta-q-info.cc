/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) Jul 2, 2015 IUST
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
 * Author: SEYED VAHID AZHARI <azharivs@iust.ac.ir>
 * Iran University of Science & Technology
 */
#include <list>
#include <deque>
#include <utility>
#include <math.h>
#include "ns3/packet.h"
#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
//#include "wifi-mac-header.h"
//#include "wifi-mac-queue.h"
#include "per-sta-q-info.h"

namespace ns3 {

  NS_OBJECT_ENSURE_REGISTERED (PerStaQInfo);

  TypeId
  PerStaQInfo::GetTypeId (void)
  {
    static TypeId tid = TypeId ("ns3::PerStaQInfo")
        .SetParent<Object> ()
        .AddConstructor<PerStaQInfo> ()
        .AddAttribute ("HistorySize", "Number of Samples Kept for Calculating Statistics.",
                       UintegerValue (25),
                       MakeUintegerAccessor (&PerStaQInfo::m_histSize),
                       MakeUintegerChecker<uint32_t> ())
        .AddAttribute ("ObservationInterval", "Length of Interval in Seconds Over Which Some Parameters Such as Arrival Rate Deficit/Excess Are Monitored.",
                       DoubleValue (2.0),
                       MakeDoubleAccessor (&PerStaQInfo::m_observationInterval),
                       MakeDoubleChecker<double> ())
        .AddAttribute ("TID", "Traffic Indication Map of Interest.",
                       UintegerValue (UP_VI),
                       MakeUintegerAccessor (&PerStaQInfo::m_tid),
                       MakeUintegerChecker<uint32_t> ())

        ;
    return tid;
  }

  PerStaQInfo::PerStaQInfo()
  : m_addrs (),
    m_queueSize (0), m_queueBytes (0), m_servedBytes (0), m_servedPackets(0),
    m_avgQueueSize (0.0), m_avgQueueBytes (0.0),
    m_avgQueueWait (0.0), m_avgArrivalRate (0.0), m_avgArrivalRateBytes (0.0),
    m_dvp (0.0), m_prEmpty (0),
    m_avgServedBytes (0.0), m_avgServedPackets(0.0),
    m_curArrivalRateSurplus (0), m_targetQueueSize(0),
    m_insufficientTimeAllowance (false)
  {
  }

  PerStaQInfo::~PerStaQInfo()
  {
    m_queueSizeHistory.clear();
    m_queueBytesHistory.clear();
    m_queueWaitHistory.clear();
    m_arrivalHistory.clear();
    m_queueDelayViolationHistory.clear();
    m_servedBytesHistory.clear();
    m_servedPacketsHistory.clear();
    m_arrivalSurplus.clear();
    m_arrivalDeficit.clear();

  }

  PerStaQInfo::Item::Item(uint32_t bytes, Time tstamp)
    : bytes(bytes), tstamp(tstamp)
  {
  }

  void
  PerStaQInfo::SetMac (const Mac48Address &addrs)
  {
    uint8_t buff[6];
    addrs.CopyTo(buff);
    m_addrs.CopyFrom(buff);
  }

  void
  PerStaQInfo::SetTid(uint8_t tid)
  {
    m_tid = tid;
  }

  void
  PerStaQInfo::SetTargetQueueSize (double target)
  {
    m_targetQueueSize = target;
  }

  double
  PerStaQInfo::GetTargetQueueSize (void)
  {
    return m_targetQueueSize;
  }

  double
  PerStaQInfo::GetArrivalRateSurplus (void)
  {
    return m_curArrivalRateSurplus;
  }

  Mac48Address&
  PerStaQInfo::GetMac (void)
  {
    return m_addrs;
  }

  uint8_t
  PerStaQInfo::GetTid (void)
  {
    return m_tid;
  }

  uint32_t
  PerStaQInfo::GetSize (void)
  {
    return m_queueSize;
  }

  uint32_t
  PerStaQInfo::GetSizeBytes (void)
  {
    return m_queueBytes;
  }

  double
  PerStaQInfo::GetAvgSize (void)
  {
    return m_avgQueueSize;
  }

  double
  PerStaQInfo::GetAvgSizeBytes (void)
  {
    return m_avgQueueBytes;
  }

  double
  PerStaQInfo::GetAvgWait (void)
  {
    return m_avgQueueWait;
  }

  double
  PerStaQInfo::GetAvgArrivalRate (void)
  {
    return m_avgArrivalRate;
  }

  double
  PerStaQInfo::GetAvgArrivalRateBytes (void)
  {
    return m_avgArrivalRateBytes;
  }

  double
  PerStaQInfo::GetDvp (void)
  {
    return m_dvp;
  }

  double
  PerStaQInfo::GetPrEmpty (void)
  {
    return m_prEmpty;
  }

  uint32_t
  PerStaQInfo::GetServedPackets(void)
  {
    return m_servedPackets;
  }

  double
  PerStaQInfo::GetAvgServedPackets(void)
  {
    return m_avgServedPackets;
  }

  uint32_t
  PerStaQInfo::GetServedBytes(void)
  {
    return m_servedBytes;
  }

  double
  PerStaQInfo::GetAvgServedBytes(void)
  {
    return m_avgServedBytes;
  }

  struct PerStaStatType
  PerStaQInfo::GetAllStats (void)
  {
    struct PerStaStatType stats;
    stats.avgArrival = GetAvgArrivalRate();
    stats.avgArrivalBytes = GetAvgArrivalRateBytes();
    stats.avgBytes = GetAvgSizeBytes();
    stats.avgQueue = GetAvgSize();
    stats.avgWait = GetAvgWait();
    stats.dvp = GetDvp();
    stats.prEmpty = GetPrEmpty();

    return stats;
  }

  Time
  PerStaQInfo::GetTimeAllowance(void)
  {
    return m_timeAllowance;
  }

  Time
  PerStaQInfo::GetRemainingTimeAllowance(void)
  {
    return m_remainingTimeAllowance;
  }

  bool
  PerStaQInfo::IsInsufficientTimeAllowanceEncountered (void)
  {
    if (m_timeAllowance == 0)
      m_insufficientTimeAllowance = true;
    return m_insufficientTimeAllowance;
  }


  void
  PerStaQInfo::SetInsufficientTimeAllowanceEncountered (void)
  {
    m_insufficientTimeAllowance = true;
  }

  Time
  PerStaQInfo::DeductTimeAllowance(Time allowance)
  {
    m_remainingTimeAllowance -= allowance;
    if (m_remainingTimeAllowance <= 0)
      {
        SetInsufficientTimeAllowanceEncountered();
      }
    return m_remainingTimeAllowance;
  }

  void
  PerStaQInfo::SetRemainingTimeAllowance(Time allowance)
  {
    m_remainingTimeAllowance = allowance;
  }

  void
  PerStaQInfo::SetTimeAllowance(Time allowance)
  {
    m_timeAllowance = allowance;
    if (m_timeAllowance == 0)
      m_insufficientTimeAllowance = true;
    else
      m_insufficientTimeAllowance = false;
  }

  void
  PerStaQInfo::ResetTimeAllowance(Time allowance)
  {
    m_timeAllowance = allowance;
    //in this version we carry the unused part of the time allowance to the next service interval
    //this is only if it was unused due to small size
    if (IsInsufficientTimeAllowanceEncountered() && m_remainingTimeAllowance > 0)
      {
        m_timeAllowance += m_remainingTimeAllowance;
      }
#ifdef SVA_DEBUG_DETAIL
    std::cout << Simulator::Now().GetSeconds() << " " << this->GetMac() << " PerStaQInfo::ResetTimeAllowance last remaining " <<
        m_remainingTimeAllowance.GetSeconds()*1000 << " msec, Reset to " << m_timeAllowance.GetSeconds()*1000 << " msec \n";
#endif
    m_remainingTimeAllowance = m_timeAllowance;
    if (m_timeAllowance == 0)
      m_insufficientTimeAllowance = true;
    else
      m_insufficientTimeAllowance = false;
  }

  void
  PerStaQInfo::ResetTimeAllowance(void)
  {
    //in this version we carry the unused part of the time allowance to the next service interval
    //this is only if it was unused due to small size
    Time leftOver(Seconds(0));
    if (IsInsufficientTimeAllowanceEncountered() && m_remainingTimeAllowance > 0)
      {
        leftOver = m_remainingTimeAllowance;
      }
#ifdef SVA_DEBUG_DETAIL
    std::cout << Simulator::Now().GetSeconds() << " " << this->GetMac() << " PerStaQInfo::ResetTimeAllowance last remaining " <<
        m_remainingTimeAllowance.GetSeconds()*1000 << " msec, Reset to " << (m_timeAllowance + leftOver).GetSeconds()*1000 << " msec \n";
#endif
    m_remainingTimeAllowance = m_timeAllowance + leftOver;
    m_insufficientTimeAllowance = false;
  }


  void
  PerStaQInfo::CollectServedPackets(void)
  {
    if (m_servedPacketsHistory.size() == m_histSize)
      {//make sure old samples are discarded
        m_servedPacketsHistory.pop_front();
      }
    m_servedPacketsHistory.push_back(m_servedPackets);

    if (m_servedBytesHistory.size() == m_histSize)
      {//make sure old samples are discarded
        m_servedBytesHistory.pop_front();
      }
    m_servedBytesHistory.push_back(m_servedBytes);

    m_servedBytes = 0;
    m_servedPackets = 0;
  }
  void
  PerStaQInfo::Arrival (uint32_t bytes, Time tstamp)
  {//TODO: record time of arrival as well for avgArrival and avgWait calculation
    m_queueSize ++;
    m_queueBytes += bytes;

    if (m_queueSizeHistory.size() == m_histSize)
      {//make sure old samples are discarded
        m_queueSizeHistory.pop_front();
      }
    m_queueSizeHistory.push_back(Item(m_queueSize,tstamp));

    if (m_queueBytesHistory.size() == m_histSize)
      {//make sure old samples are discarded
        m_queueBytesHistory.pop_front();
      }
    m_queueBytesHistory.push_back(m_queueBytes);

    if (m_arrivalHistory.size() == m_histSize)
      {//make sure old samples are discarded
        m_arrivalHistory.pop_front();
      }
    m_arrivalHistory.push_back(Item(bytes,tstamp));

    if (m_queueSize > m_targetQueueSize)
      {
        m_arrivalSurplus.push_back(tstamp);
        m_curArrivalSurplus ++;
      }
    Time observationStartTime = Simulator::Now() - Seconds(m_observationInterval);
    //remove old samples
    while (!m_arrivalSurplus.empty() && m_arrivalSurplus.front() < observationStartTime )
      {
        m_arrivalSurplus.pop_front();
        m_curArrivalSurplus --;
      }

    Update();
  }

  void
  PerStaQInfo::Departure (uint32_t bytes, Time wait, Time deadline)
  {
    m_queueSize --;
    m_queueBytes -= bytes;
    m_servedPackets ++;
    m_servedBytes += bytes;

    if (m_queueSizeHistory.size() == m_histSize)
      {//make sure old samples are discarded
        m_queueSizeHistory.pop_front();
      }
    m_queueSizeHistory.push_back(Item(m_queueSize,Simulator::Now ()));

    if (m_queueBytesHistory.size() == m_histSize)
      {//make sure old samples are discarded
        m_queueBytesHistory.pop_front();
      }
    m_queueBytesHistory.push_back(m_queueBytes);

    if (m_queueWaitHistory.size() == m_histSize)
      {//make sure old samples are discarded
        m_queueWaitHistory.pop_front();
      }
    m_queueWaitHistory.push_back(wait.GetSeconds());


    if (m_queueDelayViolationHistory.size() == m_histSize)
      {//make sure old samples are discarded
        m_queueDelayViolationHistory.pop_front();
      }
    m_queueDelayViolationHistory.push_back( (deadline - Simulator::Now()).GetSeconds() );

#ifdef SVA_DEBUG_DETAIL
    std::cout << m_queueDelayViolationHistory.front()*1000 << ".......... Time to deadline (msec)\n";
#endif

    if (m_queueSize < m_targetQueueSize)
      {
        m_arrivalDeficit.push_back(Simulator::Now());
        m_curArrivalSurplus --;
      }
    Time observationStartTime = Simulator::Now() - Seconds(m_observationInterval);
    //remove old samples
    while (!m_arrivalDeficit.empty() && m_arrivalDeficit.front() < observationStartTime )
      {
        m_arrivalDeficit.pop_front();
        m_curArrivalSurplus ++;
      }

    Update();
  }

  bool
  PerStaQInfo::IsEmpty (void)
  {
    return (m_queueSize == 0);
  }

  void
  PerStaQInfo::Reset (void)
  {
    m_queueSize = 0;
    m_queueBytes = 0;
    m_servedBytes = 0;
    m_servedPackets = 0;
    m_avgQueueSize = 0;
    m_avgQueueBytes = 0;
    m_avgQueueWait = 0;
    m_avgArrivalRate = 0;
    m_avgArrivalRateBytes = 0;
    m_dvp = 0;
    m_prEmpty = 0;
    m_avgServedBytes = 0;
    m_avgServedPackets = 0;
    m_curArrivalRateSurplus = 0;
    m_insufficientTimeAllowance = false;

    m_queueSizeHistory.clear();
    m_queueBytesHistory.clear();
    m_servedBytesHistory.clear();
    m_servedPacketsHistory.clear();
    m_queueWaitHistory.clear();
    m_arrivalHistory.clear();
    m_queueDelayViolationHistory.clear();
    m_arrivalSurplus.clear();
    m_arrivalDeficit.clear();

  }

  /*
   * TODO: Change deque to have a depth specified in terms of
   * time rather than number of samples.
   *
   */
  void
  PerStaQInfo::Update(void)
  {
    double tmp=0;
    double timespan = 0;
    Time emptyTime(0);
    Time lastEmptyStart(0);
    bool emptyStart = false;

    if (!m_queueSizeHistory.empty())
      {
        for (std::deque<Item>::iterator ait=m_queueSizeHistory.begin(); ait != m_queueSizeHistory.end(); ++ait)
          {
            tmp += (*ait).bytes; //number of packets
            if ((*ait).bytes == 0 && !emptyStart)
              {
                emptyStart = true;
                lastEmptyStart = (*ait).tstamp;
              }
            else if((*ait).bytes != 0 && emptyStart)
              {
                emptyStart = false;
                emptyTime += (*ait).tstamp - lastEmptyStart;
              }
          }
        m_avgQueueSize = tmp / (double) m_queueSizeHistory.size();
        timespan = (m_arrivalHistory.back().tstamp - m_arrivalHistory.front().tstamp).GetSeconds();
        if (!m_queueSizeHistory.empty() && m_queueSizeHistory.back().bytes == 0)
          {//if at last sample time the queue is still empty then need to update emptyTime
            emptyStart=false;
            emptyTime += m_queueSizeHistory.back().tstamp - lastEmptyStart;
          }
        m_prEmpty = emptyTime.GetSeconds()/timespan;
      }

    if (!m_queueBytesHistory.empty())
      {
        tmp = 0;
        for (std::deque<uint32_t>::iterator it=m_queueBytesHistory.begin(); it != m_queueBytesHistory.end(); ++it)
          {
            tmp += *it;
          }
        m_avgQueueBytes = tmp / (double) m_queueBytesHistory.size();
      }

    if (!m_servedBytesHistory.empty())
      {
        tmp = 0;
        for (std::deque<uint32_t>::iterator it=m_servedBytesHistory.begin(); it != m_servedBytesHistory.end(); ++it)
          {
            tmp += *it;
          }
        m_avgServedBytes = tmp / (double) m_servedBytesHistory.size();
      }


    if (!m_servedPacketsHistory.empty())
      {
        tmp = 0;
        for (std::deque<uint32_t>::iterator it=m_servedPacketsHistory.begin(); it != m_servedPacketsHistory.end(); ++it)
          {
            tmp += *it;
          }
        m_avgServedPackets = tmp / (double) m_servedPacketsHistory.size();
      }

    if (!m_queueWaitHistory.empty())
      {
        tmp = 0;
        for (std::deque<double>::iterator dit=m_queueWaitHistory.begin(); dit != m_queueWaitHistory.end(); ++dit)
          {
            tmp += *dit;

          }
        m_avgQueueWait = tmp / (double) m_queueWaitHistory.size();
      }

    if (!m_arrivalHistory.empty())
      {
        tmp = 0;
        for (std::deque<Item>::iterator ait=m_arrivalHistory.begin(); ait != m_arrivalHistory.end(); ++ait)
          {
            tmp += (*ait).bytes;
          }
        timespan = (m_arrivalHistory.back().tstamp - m_arrivalHistory.front().tstamp).GetSeconds();
        m_avgArrivalRateBytes = tmp / timespan;
        m_avgArrivalRate = m_arrivalHistory.size() / timespan;
      }

    if (!m_queueDelayViolationHistory.empty())
      {
        tmp = 0;
        for (std::deque<double>::iterator dit=m_queueDelayViolationHistory.begin(); dit != m_queueDelayViolationHistory.end(); ++dit)
          {
            if (*dit < 0) tmp ++;//count number of violations
          }
        m_dvp = tmp / m_queueDelayViolationHistory.size();
      }

    tmp = 0;
    Time start = Seconds(0);
    Time stop = Seconds(0);
    if (!m_arrivalSurplus.empty())
      {
        start = m_arrivalSurplus.front();
        stop = m_arrivalSurplus.back();
      }
    if (!m_arrivalDeficit.empty())
      {
        start = std::min(start, m_arrivalDeficit.front());
        stop = std::max(stop,m_arrivalDeficit.back());
      }

    if (stop > start)
      m_curArrivalRateSurplus = m_curArrivalSurplus / (stop-start).GetSeconds();
    else
      m_curArrivalRateSurplus = 0;


#ifdef SVA_DEBUG
    std::cout << Simulator::Now().GetSeconds() << " PerStaQInfo::Update " << GetMac() << " [TID " << (int) m_tid << "] " ;
    std::cout << "Q= " << m_queueSize << " Pkts( " << (double)m_queueBytes/1000000 << " MB) " ;
    std::cout << "avgQ= " << m_avgQueueSize << " Pkts( " << m_avgQueueBytes/1000000 << " MB) " ;
    std::cout << "avgW= " << m_avgQueueWait*1000 << " msec arrRate= " << m_avgArrivalRate << " pps( "
        << m_avgArrivalRateBytes*8/1000000 << " Mbps) DVP= " << m_dvp
        << " History= " << m_queueSizeHistory.size() << " ProbEmpty= " << m_prEmpty
        << " avgServedPackets= " << m_avgServedPackets << " avgServedBytes= " << m_avgServedBytes/1000000
        << " arrivalRateSurplus= " << m_curArrivalRateSurplus << " pps" << "\n" ;
#endif

  }



} // namespace ns3
