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
 */
#include <list>
#include <deque>
#include <utility>
#include "ns3/packet.h"
#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/uinteger.h"
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
                       UintegerValue (200),
                       MakeUintegerAccessor (&PerStaQInfo::m_histSize),
                       MakeUintegerChecker<uint32_t> ())
        .AddAttribute ("TID", "Traffic Indication Map of Interest.",
                       UintegerValue (UP_VI),
                       MakeUintegerAccessor (&PerStaQInfo::m_tid),
                       MakeUintegerChecker<uint32_t> ())

        ;
    return tid;
  }

  PerStaQInfo::PerStaQInfo()
  : m_addrs (),
    m_queueSize (0), m_queueBytes (0), m_avgQueueSize (0.0), m_avgQueueBytes (0.0),
    m_avgQueueWait (0.0), m_avgArrivalRate (0.0), m_avgArrivalRateBytes (0.0), m_dvp (0.0)

  {
  }

  PerStaQInfo::~PerStaQInfo()
  {
    m_queueSizeHistory.clear();
    m_queueBytesHistory.clear();
    m_queueWaitHistory.clear();
    m_arrivalHistory.clear();
    m_queueDelayViolationHistory.clear();
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

    return stats;
  }

  void
  PerStaQInfo::Arrival (uint32_t bytes, Time tstamp)
  {//TODO: record time of arrival as well for avgArrival and avgWait calculation
    m_queueSize ++;
    m_queueBytes += bytes;

    if (m_queueSizeHistory.size() == m_histSize)
      {//make sure old samples are discarded
        m_queueSizeHistory.pop_back();
      }
    m_queueSizeHistory.push_front(m_queueSize);

    if (m_queueBytesHistory.size() == m_histSize)
      {//make sure old samples are discarded
        m_queueBytesHistory.pop_back();
      }
    m_queueBytesHistory.push_front(m_queueBytes);

    if (m_arrivalHistory.size() == m_histSize)
      {//make sure old samples are discarded
        m_arrivalHistory.pop_back();
      }
    m_arrivalHistory.push_front(Item(bytes,tstamp));

    Update();
  }

  void
  PerStaQInfo::Departure (uint32_t bytes, Time wait, Time deadline)
  {
    m_queueSize --;
    m_queueBytes -= bytes;

    if (m_queueSizeHistory.size() == m_histSize)
      {//make sure old samples are discarded
        m_queueSizeHistory.pop_back();
      }
    m_queueSizeHistory.push_front(m_queueSize);

    if (m_queueBytesHistory.size() == m_histSize)
      {//make sure old samples are discarded
        m_queueBytesHistory.pop_back();
      }
    m_queueBytesHistory.push_front(m_queueBytes);

    if (m_queueWaitHistory.size() == m_histSize)
      {//make sure old samples are discarded
        m_queueWaitHistory.pop_back();
      }
    m_queueWaitHistory.push_front(wait.GetSeconds());


    if (m_queueDelayViolationHistory.size() == m_histSize)
      {//make sure old samples are discarded
        m_queueDelayViolationHistory.pop_back();
      }
    m_queueDelayViolationHistory.push_front( (deadline - Simulator::Now()).GetSeconds() );
#ifdef SVA_DEBUG_DETAIL
    std::cout << m_queueDelayViolationHistory.front()*1000 << ".......... Time to deadline (msec)\n";
#endif

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
    m_avgQueueSize = 0;
    m_avgQueueBytes = 0;
    m_avgQueueWait = 0;
    m_avgArrivalRate = 0;
    m_avgArrivalRateBytes = 0;
    m_dvp = 0;

    m_queueSizeHistory.clear();
    m_queueBytesHistory.clear();
    m_queueWaitHistory.clear();
    m_arrivalHistory.clear();
    m_queueDelayViolationHistory.clear();

  }

  /*
   * TODO: still need to calculate average arrival rate. This requires to store
   * the arrival instances in another deque. This deque will have a depth which
   * is specified in terms of time rather than number of samples.
   * May be the other sample histories should also be made according to a time depth??
   *
   */
  void
  PerStaQInfo::Update(void)
  {
    double tmp=0;

    for (std::deque<uint32_t>::iterator it=m_queueSizeHistory.begin(); it != m_queueSizeHistory.end(); ++it)
      {
        tmp += *it;
      }
    m_avgQueueSize = tmp / (double) m_queueSizeHistory.size();

    tmp = 0;
    for (std::deque<uint32_t>::iterator it=m_queueBytesHistory.begin(); it != m_queueBytesHistory.end(); ++it)
      {
        tmp += *it;
      }
    m_avgQueueBytes = tmp / (double) m_queueBytesHistory.size();

    tmp = 0;
    for (std::deque<double>::iterator dit=m_queueWaitHistory.begin(); dit != m_queueWaitHistory.end(); ++dit)
      {
        tmp += *dit;

      }
    m_avgQueueWait = tmp / (double) m_queueWaitHistory.size();

    tmp = 0;
    for (std::deque<Item>::iterator ait=m_arrivalHistory.begin(); ait != m_arrivalHistory.end(); ++ait)
      {
        tmp += (*ait).bytes;
      }
    double timespan = (m_arrivalHistory.begin()->tstamp - m_arrivalHistory.end()->tstamp).GetSeconds();
    m_avgArrivalRateBytes = tmp / timespan;
    m_avgArrivalRate = m_arrivalHistory.size() / timespan;

    tmp = 0;
    for (std::deque<double>::iterator dit=m_queueDelayViolationHistory.begin(); dit != m_queueDelayViolationHistory.end(); ++dit)
      {
        if (*dit < 0) tmp ++;//count number of violations
      }
    m_dvp = tmp / m_queueDelayViolationHistory.size();

#ifdef SVA_DEBUG
    std::cout << Simulator::Now().GetSeconds() << " PerStaQInfo::Update " << GetMac() << " [TID " << (int) m_tid << "] " ;
    std::cout << "Q= " << m_queueSize << " Pkts( " << (double)m_queueBytes/1000000 << " MB) " ;
    std::cout << "avgQ= " << m_avgQueueSize << " Pkts( " << m_avgQueueBytes/1000000 << " MB) " ;
    std::cout << "avgW= " << m_avgQueueWait*1000 << " msec arrRate= " << m_avgArrivalRate << " pps( "
        << m_avgArrivalRateBytes*8/1000000 << " Mbps) DVP= " << m_dvp << " History= " << m_queueSizeHistory.size() << "\n" ;
#endif

  }



} // namespace ns3
