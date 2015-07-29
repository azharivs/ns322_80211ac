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
#include <sstream>
#include "ns3/packet.h"
#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/uinteger.h"
#include "ns3/callback.h"
#include "ns3/config.h"
#include "wifi-phy-state-helper.h"
#include "wifi-mac-header.h"
#include "ns3/simulator.h"
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
  : m_recordTx (false),
    m_idle (0), m_busy (0), m_lastBeacon (0), m_beaconInterval(0),
    m_avgIdleTimePerBeacon (0), m_avgBusyTimePerBeacon (0), m_avgBeaconInterval (0),
    m_samples (0)
  {//TODO make trace sinks null
    m_perStaQInfo = NULL;
  }

  BssPhyMacStats::BssPhyMacStats(std::string path)
  : m_recordTx (false),
    m_idle (0), m_busy (0), m_lastBeacon (0), m_beaconInterval (0),
    m_avgIdleTimePerBeacon (0), m_avgBusyTimePerBeacon (0),  m_avgBeaconInterval (0),
    m_samples (0)
  {
    m_perStaQInfo = NULL;

    Ptr<WifiPhyStateHelper> wifiPhyStateHelper;
    std::ostringstream stateLoggerPath;
    stateLoggerPath << path << "/State";
    Config::ConnectWithoutContext(stateLoggerPath.str(), MakeCallback (&BssPhyMacStats::PhyStateLoggerSink, this));

    std::ostringstream txTracePath;
    txTracePath << path << "/Tx";
    Config::ConnectWithoutContext(txTracePath.str(), MakeCallback (&BssPhyMacStats::PhyTxStartSink, this));

  }

  BssPhyMacStats::~BssPhyMacStats()
  {
    m_idleTimeHistory.clear();
    m_busyTimeHistory.clear();
    m_beaconIntervalHistory.clear();
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

  Time
  BssPhyMacStats::GetAvgBeaconInterval (void)
  {
    return m_avgBeaconInterval;
  }

  void
  BssPhyMacStats::PhyStateLoggerSink (const Time start, const Time duration, const WifiPhy::State state)
  {
    //std::cout << "New state is " << state << " from " << start.GetSeconds()*1000 << " for " << duration.GetSeconds()*1000 << " msec\n";
    switch (state)
    {
      case WifiPhy::IDLE:
        RecordIdle(duration);
        break;
      case WifiPhy::CCA_BUSY:
        RecordBusy(duration);
        break;
      case WifiPhy::TX:
        RecordBusy(duration);
        if (m_recordTx)
          {
            m_recordTx = false;
            RecordTx(duration);
          }
        break;
      case WifiPhy::RX:
        RecordBusy(duration);
        break;
      case WifiPhy::SWITCHING:
        RecordBusy(duration);
        break;
      case WifiPhy::SLEEP:
        RecordIdle(duration);
        break;
      default:
        NS_FATAL_ERROR("Undefined WifiPhy::State " << state);
    }
  }

  void
  BssPhyMacStats::PhyTxStartSink (const Ptr<const Packet> packet, const WifiMode mode,
                                  const WifiPreamble preamble, const uint8_t power)
  {
    Time now = Simulator::Now();
    m_curPacket = packet;
    WifiMacHeader hdr;
    packet->PeekHeader(hdr);
    if (hdr.IsBeacon())
      {
        //std::cout << "@ " << now.GetSeconds() << " BEACON TX \n";
        RecordBeacon(now);
      }
    else if ((hdr.IsData() || hdr.IsQosData()) && !hdr.GetAddr1().IsBroadcast() ) //update time allowance for that station's queue
      {
        m_recordTx = true;
      }
  }

  void
  BssPhyMacStats::RecordIdle (Time duration)
  {
    m_idle += duration;
  }

  void
  BssPhyMacStats::RecordBusy (Time duration)
  {
    m_busy += duration;
  }

  void
  BssPhyMacStats::RecordTx (Time duration)
  {
    if (!m_perStaQInfo)//return if no PerStaQInfo capability is defined (probably a station)
      {
#ifdef SVA_DEBUG_DETAIL
      std::cout << "BssPhyMacStats::RecordTx No PerStaQInfo Capability Defined \n";
#endif
        return ;
      }
    WifiMacHeader hdr;
    m_curPacket->PeekHeader(hdr);
    Ptr<PerStaQInfo> staQInfo = m_perStaQInfo->GetByMac(hdr.GetAddr1());
    NS_ASSERT_MSG(staQInfo, "BssPhyMacStats::RecordTx, No station found by that MAC address" << hdr.GetAddr1());
    Time leftOver = staQInfo->DeductTimeAllowance(duration);
#ifdef SVA_DEBUG_DETAIL
    if (leftOver < 0)
      {
        std::cout << "BssPhyMacStats::RecordTx Negative Time Allowance Left Over \n";
      }
#endif
  }

  void
  BssPhyMacStats::RecordBeacon (Time tstamp)
  {

    m_beaconInterval = tstamp - m_lastBeacon;
    m_lastBeacon = tstamp;

    if (m_samples >= 2)
      {
        if (m_idleTimeHistory.size() == m_histSize)
          {//make sure old samples are discarded
            m_idleTimeHistory.pop_back();
          }
        m_idleTimeHistory.push_front(m_idle);

        if (m_busyTimeHistory.size() == m_histSize)
          {//make sure old samples are discarded
            m_busyTimeHistory.pop_back();
          }
        m_busyTimeHistory.push_front(m_busy);

        if (m_beaconIntervalHistory.size() == m_histSize)
          {//make sure old samples are discarded
            m_beaconIntervalHistory.pop_back();
          }
        m_beaconIntervalHistory.push_front(m_beaconInterval);
        Update();
      }
    //resetting these after Update() allows for proper logging
    m_idle = Seconds(0);
    m_busy = Seconds(0);
    m_samples ++;

  }

  void
  BssPhyMacStats::Reset (void)
  {
    m_samples = 0;

    m_idle = Seconds(0);
    m_busy = Seconds(0);
    m_lastBeacon = Seconds(0);
    m_avgIdleTimePerBeacon = Seconds(0);
    m_avgBusyTimePerBeacon = Seconds(0);
    m_avgBeaconInterval = Seconds(0);

    m_idleTimeHistory.clear();
    m_busyTimeHistory.clear();
    m_beaconIntervalHistory.clear();

  }

  /*
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


    tmp = Seconds(0);
    for (std::deque<Time>::iterator it=m_beaconIntervalHistory.begin(); it != m_beaconIntervalHistory.end(); ++it)
      {
        tmp += *it;
      }
    m_avgBeaconInterval = tmp / (double) m_beaconIntervalHistory.size();


#ifdef SVA_DEBUG
    std::cout << Simulator::Now().GetSeconds() << " BssPhyMacStats::Update Idle= " << m_idle.GetSeconds()*1000 << " msec, avgIdle= "
        << m_avgIdleTimePerBeacon.GetSeconds()*1000 << " msec, Busy= " << m_busy.GetSeconds()*1000 << " msec, avgBusy= "
        << m_avgBusyTimePerBeacon.GetSeconds()*1000 << " msec\n";
#endif

  }



} // namespace ns3
