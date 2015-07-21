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
  ~BssPhyMacStats ();

  /**
   * Set the maximum number of samples kept for calculating statistics.
   * Currently not needed because I'm relying on Attribute base initialization
   * \param histSize the maximum sample history size
   */
  //void SetHistorySize (uint32_t histSize);


  /**
   * Computes and returns average idle time during a beacon interval
   *
   * \return the average idle time
   */
  Time GetAvgIdleTimePerBeacon (void);

  /**
   * Computes and returns average busy (CCA+RX+TX) time during a beacon interval
   *
   * \return the average busy time
   */
  Time GetAvgBusyTimePerBeacon (void);

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

  std::deque<Time> m_idleTimeHistory; //!< Array of samples of total idle times per beacon interval
  std::deque<Time> m_busyTimeHistory; //!< Array of samples of total busy times per beacon
  double m_avgIdleTimePerBeacon; //!< Last updated average total idle time per beacon interval
  double m_avgBusyTimePerBeacon; //!< Last updated average total busy time per beacon interval
};

} // namespace ns3

#endif /* BSS_PHY_MAC_STATS_H */
