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
#include "wifi-phy-state-helper.h"
#include "bss-phy-mac-stats.h"
#include "per-sta-q-info.h"

namespace ns3 {

  NS_OBJECT_ENSURE_REGISTERED (BssPhyMacStats);

  TypeId
  BssPhyMacStats::GetTypeId (void)
  {
    static TypeId tid = TypeId ("ns3::BssPhyMacStats")
        .SetParent<Object> ()
        .AddConstructor<BssPhyMacStats> ()
        .AddAttribute ("HistorySize", "Number of Samples Kept for Calculating Statistics.",
                       UintegerValue (200),
                       MakeUintegerAccessor (&BssPhyMacStats::m_histSize),
                       MakeUintegerChecker<uint32_t> ())

        ;
    return tid;
  }

  BssPhyMacStats::BssPhyMacStats()
  : m_idle (0), m_busy (0), m_lastBeacon (0),
    m_avgIdleTimePerBeacon (0), m_avgBusyTimePerBeacon (0)
  {
    Ptr<WifiPhyStateHelper> wifiPhyStateHelper;
    bool result = ipv4L3Protocol->TraceConnectWithoutContext ("Tx", MakeCallback (&Ipv4L3ProtocolRxTxSink));
    NS_ASSERT_MSG (result == true, "InternetStackHelper::EnablePcapIpv4Internal():  "
                   "Unable to connect ipv4L3Protocol \"Tx\"");

  }

  BssPhyMacStats::~BssPhyMacStats()
  {
    m_idleTimeHistory.clear();
    m_busyTimeHistory.clear();
  }

  Time
  BssPhyMacStats::GetAvgIdleTimePerBeacon (void)
  {
    return m_avgIdleTimePerBeacon;
  }

  Time
  BssPhyMacStats::GetAvgBusyTimePerBeacon (void)
  {
    return m_avgBusyTimePerBeacon;
  }

  void
  BssPhyMacStats::PhyStateLoggerSink (const Time start, const Time duration, const WifiPhy::State state)
  {//TODO

  }

  void
  BssPhyMacStats::PhyTxStartSink (const Ptr<const Packet> packet, const WifiMode mode,
                                  const WifiPreamble preamble, const uint8_t power)
  {//TODO

  }

  void
  BssPhyMacStats::RecordIdle (Time duration)
  {//TODO:
  }

  void
  BssPhyMacStats::RecordBusy (Time duration)
  {//TODO:
  }

  void
  BssPhyMacStats::RecordBeacon (Time tstamp)
  {//TODO
    /*
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

    */

    Update();
  }

  void
  BssPhyMacStats::Reset (void)
  {
    m_idle = Seconds(0);
    m_busy = Seconds(0);
    m_lastBeacon = Seconds(0);
    m_avgIdleTimePerBeacon = Seconds(0);
    m_avgBusyTimePerBeacon = Seconds(0);

    m_idleTimeHistory.clear();
    m_busyTimeHistory.clear();

  }

  /*
   * TODO: still need to calculate average arrival rate. This requires to store
   * the arrival instances in another deque. This deque will have a depth which
   * is specified in terms of time rather than number of samples.
   * May be the other sample histories should also be made according to a time depth??
   *
   */
  void
  BssPhyMacStats::Update(void)
  {
    Time tmp(0);

    for (std::deque<Time>::iterator it=m_idleTimeHistory.begin(); it != m_idleTimeHistory.end(); ++it)
      {
        tmp += *it;
      }
    m_avgIdleTimePerBeacon = tmp / (double) m_idleTimeHistory.size();

    tmp = Seconds(0);
    for (std::deque<Time>::iterator it=m_busyTimeHistory.begin(); it != m_busyTimeHistory.end(); ++it)
      {
        tmp += *it;
      }
    m_avgBusyTimePerBeacon = tmp / (double) m_busyTimeHistory.size();


#ifdef SVA_DEBUG
    std::cout << "Idle= " << m_idle.GetSeconds()*1000 << " msec, avgIdle= " << m_avgIdleTimePerBeacon.GetSeconds()*1000
        << " msec, Busy= " << m_busy.GetSeconds()*1000 << " msec, avgBusy= " << m_avgBusyTimePerBeacon.GetSeconds()*1000 << "\n";
#endif

  }



} // namespace ns3
