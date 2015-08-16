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
 */
#ifndef MPDU_UNIVERSAL_AGGREGATOR_H
#define MPDU_UNIVERSAL_AGGREGATOR_H

#include "mpdu-aggregator.h"
#include "ns3/enum.h"


namespace ns3 {

/**
 * \ingroup wifi
 * Universal MPDU aggregator
 *
 */

typedef enum
{
  STANDARD,
  DEADLINE
} AggregationType;


class MpduUniversalAggregator : public MpduAggregator
{
public:
  static TypeId GetTypeId (void);
  MpduUniversalAggregator ();
  ~MpduUniversalAggregator ();
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
   * \param aggregatedPacket packet that will contain the packet of size <i>packetSize</i>, if aggregation is possible.
   * \param blockAckSize size of the piggybacked block ack request
   * \return true if the packet can be aggregated to <i>aggregatedPacket</i>, false otherwise.
   *
   * This method is used to determine if a packet could be aggregated to an A-MPDU
   */
  virtual bool CanBeAggregated (Ptr<const Packet> peekedPacket, Ptr<Packet> aggregatedPacket, uint16_t blockAckSize);
  /**
   * \return padding that must be added to the end of an aggregated packet
   *
   * Calculates how much padding must be added to the end of an aggregated packet, after that a new packet is added.
   * Each A-MPDU subframe is padded so that its length is multiple of 4 octets.
   */
  virtual uint32_t CalculatePadding (Ptr<const Packet> packet);

private:
  /**
   * \param peekedPacket the packet we want to insert into <i>aggregatedPacket</i>.
   * \param aggregatedPacket packet that will contain the packet of size <i>packetSize</i>, if aggregation is possible.
   * \param blockAckSize size of the piggybacked block ack request
   * \return true if the packet can be aggregated to <i>aggregatedPacket</i>, false otherwise.
   *
   * This method is used to determine if a packet could be aggregated to an A-MPDU such that length does not exceed m_maxAmpduLenth.
   * it is called by CanBeAggregated() to deal with aggregation when m_aggregationAlgorithm is set to STANDARD
   */
  bool StandardCanBeAggregated (Ptr<const Packet> peekedPacket, Ptr<Packet> aggregatedPacket, uint16_t blockAckSize);

  /**
   * This method is used to determine if a packet could be aggregated to an A-MPDU
   * based on the condition that the packet deadline is going to be expired in
   * the next m_serviceInterval seconds.
   * It is called by CanBeAggregated() to deal with aggregation when
   * m_aggregationAlgorithm is set to DEADLINE
   */
  bool DeadlineCanBeAggregated (Ptr<const Packet> peekedPacket, Ptr<Packet> aggregatedPacket, uint16_t blockAckSize);

  AggregationType m_aggregationAlgorithm; //!< Type of aggregation algorithm: STANDARD, DEADLINE, ...
  uint32_t m_maxAmpduLength; //!< Maximum length in bytes of A-MPDUs (used for STANDARD)
  double m_serviceInterval; //!< Interval in seconds with which packet queues are guaranteed to be served at least once. (used for DEADLINE)
};

}  // namespace ns3

#endif /* MPDU_STANDARD_AGGREGATOR_H */
