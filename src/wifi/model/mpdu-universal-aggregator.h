/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013
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
 * Iran University of Science & Technology
 */
#ifndef MPDU_UNIVERSAL_AGGREGATOR_H
#define MPDU_UNIVERSAL_AGGREGATOR_H

#include "mpdu-aggregator.h"
#include "aggregation-controllers.h"
#include "ns3/enum.h"
#include "ns3/nstime.h"
#include "wifi-mac-queue.h"
#include "ns3/per-sta-q-info-container.h"
#include "mac-low.h"
#include "wifi-phy.h"


namespace ns3 {

class AggregationController;
class MacLow;
class WifiPhy;

/**
 * \ingroup wifi
 * Universal MPDU aggregator
 *
 */

typedef enum
{
  STANDARD,
  DEADLINE,
  TIME_ALLOWANCE, //to be used with PerStaWifiMacQueue::ServicePolicyType MAX_REMAINING_TIME_ALLOWANCE
  /*sva-design: add for new aggregation algorithm AGG_ALG
  AGG_ALG,
  sva-design*/
  PER_BITRATE_TIME_ALLOWANCE //TODO to be used with PerStaWifiMacQueue::ServicePolicyType ???
} AggregationType;

/*
 *   ____________________________________________________________________
 *   ________________________ Design Approach ___________________________
 *  | We will create our own version of CanBeAggregated() that works     |
 *  | by calling different private member functions XxxCanBeAggregated() |
 *  | for each specific scheduling algorithm. Aggregate() will also be   |
 *  | changed to call CanBeAggregated() with a blockAckSize=0. Any other |
 *  | physical and link layer issues such as current bitrate will have   |
 *  | to be checked in XxxCanBeAggregated(). This can be tricky and may  |
 *  | eventually require modification of MacLow::StopAggregation().      |
 *  |____________________________________________________________________|
 *
 */

class MpduUniversalAggregator : public MpduAggregator
{
public:
  static TypeId GetTypeId (void);
  MpduUniversalAggregator ();
  ~MpduUniversalAggregator ();

  /*
   * Enables access to container by initializing member pointer
   */
  bool EnablePerStaQInfo(PerStaQInfoContainer &c, Ptr<PerStaWifiMacQueue> queue, Ptr<MacLow> low, Ptr<WifiPhy> phy);

  /**
   * \param packet packet we have to insert into <i>aggregatedPacket</i>.
   * \param aggregatedPacket packet that will contain <i>packet</i>, if aggregation is possible.
   * \return true if <i>packet</i> can be aggregated to <i>aggregatedPacket</i>, false otherwise.
   *
   * This method performs an MPDU aggregation.
   * Returns true if <i>packet</i> can be aggregated to <i>aggregatedPacket</i>, false otherwise.
   */
  virtual bool Aggregate (Ptr<const Packet> packet, Ptr<Packet> aggregatedPacket);
  /**
   * Adds A-MPDU subframe header and padding to each MPDU that is part of an A-MPDU before it is sent.
   */
  virtual void AddHeaderAndPad (Ptr<Packet> packet, bool last);
  /**
   * \param peekedPacket the packet we want to insert into <i>aggregatedPacket</i>.
   * \param peekedHeader the header that will eventually be added to this packet. Can not be obtained from packet because it is probably not AddHeader()'ed yet
   * \param aggregatedPacket packet that will contain the packet of size <i>packetSize</i>, if aggregation is possible.
   * \param blockAckSize size of the piggybacked block ack request
   * \param duration is the duration of the transmission. This has to be provided as input to prevent
   *        additional call to WifiPhy::CalculateTsDuration() as it will cause state inconsistency
   *        TODO: find a better remedy in the future
   * \return true if the packet can be aggregated to <i>aggregatedPacket</i>, false otherwise.
   *
   * This method is used to determine if a packet could be aggregated to an A-MPDU
   */
  virtual bool CanBeAggregated (Ptr<const Packet> peekedPacket, WifiMacHeader peekedHeader, Ptr<Packet> aggregatedPacket, uint16_t blockAckSize, Time duration);
  /**
   * \return padding that must be added to the end of an aggregated packet
   *
   * Calculates how much padding must be added to the end of an aggregated packet, after that a new packet is added.
   * Each A-MPDU sub-frame is padded so that its length is multiple of 4 octets.
   */
  virtual uint32_t CalculatePadding (Ptr<const Packet> packet);

  /*
   * Called at the beginning of a new pending service interval
   * Calls any update procedure that is required for aggregation parameters
   *
   */
  void PendingServiceInterval (void);

  /*
   * returns true if a service interval is pending, false otherwise
   */
  bool IsPendingServiceInterval (void);

  /*
   * Called at the actual beginning of a new service interval
   * Calls any update procedure that is required for aggregation parameters
   *
   */
  void BeginServiceInterval (void);

private:
  /**
   * \param peekedPacket the packet we want to insert into <i>aggregatedPacket</i>.
   * \param peekedHeader packet header that will eventually be added when aggregation is approved
   * \param aggregatedPacket packet that will contain the packet of size <i>packetSize</i>, if aggregation is possible.
   * \param blockAckSize size of the piggybacked block ack request
   * \param duration is the duration of the transmission. This has to be provided as input to prevent
   *        additional call to WifiPhy::CalculateTsDuration() as it will cause state inconsistency
   *        TODO: find a better remedy in the future
   * \return true if the packet can be aggregated to <i>aggregatedPacket</i>, false otherwise.
   *
   * This method is used to determine if a packet could be aggregated to an A-MPDU such that length does not exceed m_maxAmpduLenth.
   * it is called by CanBeAggregated() to deal with aggregation when m_aggregationAlgorithm is set to STANDARD
   */
  bool StandardCanBeAggregated (Ptr<const Packet> peekedPacket, WifiMacHeader peekedHeader, Ptr<Packet> aggregatedPacket, uint16_t blockAckSize, Time duration);

  /**
   * This method is used to determine if a packet could be aggregated to an A-MPDU
   * based on the condition that the packet deadline is going to be expired in
   * the next m_serviceInterval seconds.
   * It is called by CanBeAggregated() to deal with aggregation when
   * m_aggregationAlgorithm is set to DEADLINE
   */
  bool DeadlineCanBeAggregated (Ptr<const Packet> peekedPacket, WifiMacHeader peekedHeader, Ptr<Packet> aggregatedPacket, uint16_t blockAckSize, Time duration);


  /**
   * This method is used to determine if a packet could be aggregated to an A-MPDU
   * based on the condition that the flow to which the packet belongs has not
   * exhausted its time allowance during the current service interval.
   * It is called by CanBeAggregated() to deal with aggregation when
   * m_aggregationAlgorithm is set to TIME_ALLOWANCE
   */
  bool TimeAllowanceCanBeAggregated (Ptr<const Packet> peekedPacket, WifiMacHeader peekedHeader, Ptr<Packet> aggregatedPacket, uint16_t blockAckSize, Time duration);

  /* Called for PER_BITRATE_TIME_ALLOWANCE aggregation algorithm
   * It is called by CanBeAggregated() to deal with aggregation when
   * m_aggregationAlgorithm is set to PER_BITRATE_TIME_ALLOWANCE
   */
  bool PerBitrateTimeAllowanceCanBeAggregated (Ptr<const Packet> peekedPacket, WifiMacHeader peekedHeader, Ptr<Packet> aggregatedPacket, uint16_t blockAckSize, Time duration);

  /*sva-design: add for new aggregation algorithm AGG_ALG
   * Called for AGG_ALG aggregation algorithm
   * It is called by CanBeAggregated() to deal with aggregation when
   * m_aggregationAlgorithm is set to AGG_ALG
   *
  bool XxxCanBeAggregated (Ptr<const Packet> peekedPacket, WifiMacHeader peekedHeader, Ptr<Packet> aggregatedPacket, uint16_t blockAckSize, Time duration);
   *
  sva-design*/

  /*
   * Updates the amount of time allowance for each PerStaQInfo
   * element to be used in the current service interval.
   * Called by BeginServiceInterval()
   */
  void DoUpdate (void);

  /*
   * Called for TIME_ALLOWANCE aggregation algorithm
   * Resets all time allowances in the entire PerStaQInfoContainer
   *
   */
  void ResetTimeAllowance (void);

  /* Called for PER_BITRATE_TIME_ALLOWANCE aggregation algorithm
   * Does book keeping/etc. at beginning of new service interval
   */
  void ResetPerBitrateTimeAllowance (void);

  /*sva-design: add for new aggregation algorithm AGG_ALG
   * Called for AGG_ALG aggregation algorithm
   * Does book keeping/etc. at beginning of new service interval
   *
  void ResetXxx (void);
   *
  sva-design*/

  /*
   * Returns true if the aggregator is finished serving all queues
   * and is ready to proceed to the next service interval.
   * Called by PendingServiceInterval()
   */
  bool IsReadyForNextServiceIntervalTimeAllowance (void);

  bool IsReadyForNextServiceIntervalPerBitrateTimeAllowance (void);

  /*sva-design: add for new aggregation algorithm ???
   *
  bool IsReadyForNextServiceInterval??? (void);
   *
  sva-design*/

  AggregationType m_aggregationAlgorithm; //!< Type of aggregation algorithm: STANDARD, DEADLINE, ...
  uint32_t m_maxAmpduLength; //!< Maximum length in bytes of A-MPDUs (used for STANDARD)
  double m_serviceInterval; //!< Interval in seconds with which packet queues are guaranteed to be served at least once. (used for DEADLINE)
  bool m_pendingServiceInterval; //!< Flag marking whether a new service interval is pending
  Time m_currentServiceIntervalStart; //!< starting time of current service interval
  Ptr<AggregationController> m_controller; //!< Pointer to aggregation controller class
  Ptr<PerStaWifiMacQueue> m_queue; //!< Pointer to queue over which this aggregation algorithm is applied (NOT USED!!)
  PerStaQInfoContainer *m_perStaQInfo; //!< Pointer to PerStaQInfoContainer, needed for cross layer operation
  Ptr<MacLow> m_low; //!< Pointer to MacLow, needed for cross layer operation
  Ptr<WifiPhy> m_phy; //!< Pointer to WifiPhy, needed for cross layer operation
};

}  // namespace ns3

#endif /* MPDU_STANDARD_AGGREGATOR_H */
