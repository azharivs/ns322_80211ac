/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015, SEYED VAHID AZHARI
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
 * Author: Seyed Vahid Azhari <azharivs@iust.ac.ir>
 */


#ifndef BSS_PHY_MAC_STATS_H
#define BSS_PHY_MAC_STATS_H

#include <list>
#include <deque>
#include <utility>
#include "ns3/packet.h"
#include "ns3/nstime.h"
#include "ns3/object.h"
#include "wifi-mac-header.h"
#include "ns3/simulator.h"
#include "wifi-phy-state-helper.h"
#include "ns3/per-sta-q-info-container.h"

namespace ns3 {

/**
 * \ingroup wifi
 *
 *
 */

class BssPhyMacStats : public Object
{
public:
  static TypeId GetTypeId (void);
  BssPhyMacStats ();
  BssPhyMacStats (std::string path);//path of the WifiPhyStateHelper used as trace source
  ~BssPhyMacStats ();

  /**
   * Set the maximum number of samples kept for calculating statistics.
   * Currently not needed because I'm relying on Attribute base initialization
   * \param histSize the maximum sample history size
   */
  //void SetHistorySize (uint32_t histSize);

  bool SetPerStaQInfo (PerStaQInfoContainer *c);

  /**
   * returns average idle time during a beacon interval
   *
   * \return the average idle time
   */
  Time GetAvgIdleTimePerBeacon (void);

  /**
   * returns average busy (CCA+RX+TX) time during a beacon interval
   *
   * \return the average busy time
   */
  Time GetAvgBusyTimePerBeacon (void);

  /**
   * returns average beacon interval
   *
   * \return the average beacon interval
   */
  Time GetAvgBeaconInterval (void);

  /**
   * Trace sink for WifiPhyStateHelper::m_stateLogger
   *
   * \param [in] start Time when the \p state started.
   * \param [in] duration Amount of time we've been in (or will be in)
   *             the \p state.
   * \param [in] state The state.
   */
  void PhyStateLoggerSink (const Time start, const Time duration, const WifiPhy::State state);

  /**
   * Trace sink for transmit event WifiPhyStateHelper::m_txTrace
   *
   * \param [in] packet The packet to be transmitted.
   * \param [in] mode The transmission mode of the packet.
   * \param [in] preamble The preamble of the packet.
   * \param [in] power  The transmit power level.
   */
  void PhyTxStartSink(const Ptr<const Packet> packet, const WifiMode mode, const WifiPreamble preamble, const uint8_t power);


  /**
   * Reset all statistics and flush sample history
   */
  void Reset (void);

private:

  /**
   * Computes all average values from sample history
   *
   */
  void Update (void);

  /*
   * Is called whenever a new idle time is detected
   * and will update latest idle time during the current beacon interval
   *
   * \param [in] duration is the length of the idle period experienced
   */
  void RecordIdle (Time duration);


  /*
   * Is called whenever a new busy time is detected
   * and will update latest busy time during the current beacon interval
   *
   * \param [in] duration is the length of the busy period experienced
   */
  void RecordBusy (Time duration);


  /*
   * Is called whenever a new Tx time is detected
   * and will update remaining time allowance for the
   * respective PerStaQInfo
   *
   * \param [in] duration is the length of the Tx period experienced
   */
  void RecordTx (Time duration);


  /*
   * Is called whenever a new beacon is transmitted
   * and will mark the start of a new beacon interval
   * it will add idle and busy times to the sample history
   * and will call update()
   *
   * \param [in] tstamp is time of beacon transmission start
   */
  void RecordBeacon (Time tstamp);

  PerStaQInfoContainer *m_perStaQInfo; //!< Pointer to PerStaQInfoContainer for updating some statistics
  Ptr<const Packet> m_curPacket;
  bool m_recordTx;//!< Flag that indicates the duration of the Tx should be accounted for m_curPacket at the next call to PhyStateLoggerSink with Tx state

  Time m_idle; //!< Current total idle times during current beacon interval
  Time m_busy; //!< Current total busy times during current beacon interval
  Time m_lastBeacon; //!< Time of last beacon transmission start
  Time m_beaconInterval; //!< Last beacon interval
  std::deque<Time> m_idleTimeHistory; //!< Array of samples of total idle times per beacon interval
  std::deque<Time> m_busyTimeHistory; //!< Array of samples of total busy times per beacon
  std::deque<Time> m_beaconIntervalHistory; //!< Array of samples of beacon interval length
  Time m_avgIdleTimePerBeacon; //!< Last updated average total idle time per beacon interval
  Time m_avgBusyTimePerBeacon; //!< Last updated average total busy time per beacon interval
  Time m_avgBeaconInterval; //!< Average beacon interval
  uint32_t m_histSize; //!< Size of history buffer
  uint32_t m_samples; //!< Number of samples PROCESSED since initialization. Used to discard first two samples
};

} // namespace ns3

#endif /* BSS_PHY_MAC_STATS_H */
