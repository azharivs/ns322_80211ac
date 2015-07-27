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
#include "ns3/log.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/udp-client.h" //used for TimestampTag. Should move so somewhere else TODO
#include "ns3/simulator.h"

#include "ampdu-subframe-header.h"
#include "mpdu-universal-aggregator.h"
#include "wifi-mac-header.h"
#include "wifi-mac-trailer.h"
#include "per-sta-q-info.h"

//sva TODO: Aggregation size should be allowed to increase to 11ac values. (I think 4MB?!)

NS_LOG_COMPONENT_DEFINE ("MpduUniversalAggregator");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (MpduUniversalAggregator);

TypeId
MpduUniversalAggregator::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::MpduUniversalAggregator")
    .SetParent<MpduAggregator> ()
    .AddConstructor<MpduUniversalAggregator> ()
    .AddAttribute ("MaxAmpduSize", "Max length in bytes of an A-MPDU",
                   UintegerValue (65535),
                   MakeUintegerAccessor (&MpduUniversalAggregator::m_maxAmpduLength),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("ServiceInterval", "Periodicity with which queues are guaranteed to be serviced (in seconds).",
                   DoubleValue (0.1), //sva: the default value should be later changed to beacon interval
                   MakeDoubleAccessor (&MpduUniversalAggregator::m_serviceInterval),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("AggregationAlgorithm", "The aggregation algorithm used for selecting packets to join the A-MPDU.",
                   EnumValue (DEADLINE),
                   MakeEnumAccessor (&MpduUniversalAggregator::m_aggregationAlgorithm),
                   MakeEnumChecker (ns3::STANDARD, "ns3::STANDARD",
                                    ns3::DEADLINE, "ns3::DEADLINE"))
;
  return tid;
}

MpduUniversalAggregator::MpduUniversalAggregator ()
{
}

MpduUniversalAggregator::~MpduUniversalAggregator ()
{
}

//sva: This is the function where our aggregation algorithm should be implemented
bool
MpduUniversalAggregator::Aggregate (Ptr<const Packet> packet, Ptr<Packet> aggregatedPacket)
{
  NS_LOG_FUNCTION (this);
  Ptr<Packet> currentPacket;
  AmpduSubframeHeader currentHdr;

  uint32_t padding = CalculatePadding (aggregatedPacket);
  //old code: uint32_t actualSize = aggregatedPacket->GetSize ();

  //sva: checks whether adding this packet will exceed max aggregation size
  //sva: old code was: if ((4 + packet->GetSize () + actualSize + padding) <= m_maxAmpduLength)
  if ( CanBeAggregated(packet, aggregatedPacket, 0) )//0: means no block ack request bits
    {
      if (padding)
        {
          Ptr<Packet> pad = Create<Packet> (padding);
          aggregatedPacket->AddAtEnd (pad);
        }
      currentHdr.SetCrc (1);
      currentHdr.SetSig ();
      currentHdr.SetLength (packet->GetSize ());
      currentPacket = packet->Copy ();

      currentPacket->AddHeader (currentHdr);
      aggregatedPacket->AddAtEnd (currentPacket);
      return true;
    }
  return false;
}

void
MpduUniversalAggregator::AddHeaderAndPad (Ptr<Packet> packet, bool last)
{
  NS_LOG_FUNCTION (this);
  AmpduSubframeHeader currentHdr;
  //This is called to prepare packets from the aggregte queue to be sent so no need to check total size since it has already been
  //done before when deciding how many packets to add to the queue
  currentHdr.SetCrc (1);
  currentHdr.SetSig ();
  currentHdr.SetLength (packet->GetSize ());
  packet->AddHeader (currentHdr);
  uint32_t padding = CalculatePadding (packet);

  if (padding && !last)
    {
      Ptr<Packet> pad = Create<Packet> (padding);
      packet->AddAtEnd (pad);
    }
}

bool
MpduUniversalAggregator::CanBeAggregated (Ptr<const Packet> peekedPacket, Ptr<Packet> aggregatedPacket, uint16_t blockAckSize)
{
  switch (m_aggregationAlgorithm)
  {
    case STANDARD:
      return StandardCanBeAggregated(peekedPacket, aggregatedPacket, blockAckSize);
      break;
    case DEADLINE:
      return DeadlineCanBeAggregated(peekedPacket, aggregatedPacket, blockAckSize);
      break;
    case TIME_ALLOWANCE:
      return TimeAllowanceCanBeAggregated(peekedPacket, aggregatedPacket, blockAckSize);
      break;
    default:
      NS_FATAL_ERROR("Unspecified Aggregation Algorithm" << m_aggregationAlgorithm);
  }
}

uint32_t
MpduUniversalAggregator::CalculatePadding (Ptr<const Packet> packet)
{
  return (4 - (packet->GetSize () % 4 )) % 4;
}

void
MpduUniversalAggregator::BeginServiceInterval(void)
{//TODO
  return;
}

bool
MpduUniversalAggregator::StandardCanBeAggregated (Ptr<const Packet> peekedPacket, Ptr<Packet> aggregatedPacket, uint16_t blockAckSize)
{
  WifiMacHeader peekedHdr;
  peekedPacket->PeekHeader(peekedHdr);
  uint32_t padding = CalculatePadding (aggregatedPacket);
  uint32_t actualSize = aggregatedPacket->GetSize ();
  uint32_t packetSize = peekedPacket->GetSize () + peekedHdr.GetSize () + WIFI_MAC_FCS_LENGTH;
  if (blockAckSize > 0)
    {
      blockAckSize = blockAckSize + 4 + padding;
    }
  if ((4 + packetSize + actualSize + padding + blockAckSize) <= m_maxAmpduLength)
    {
      return true;
    }
  else
    {
      return false;
    }
}


bool
MpduUniversalAggregator::DeadlineCanBeAggregated (Ptr<const Packet> peekedPacket, Ptr<Packet> aggregatedPacket, uint16_t blockAckSize)
{
  TimestampTag deadline;

  if (!peekedPacket->FindFirstMatchingByteTag(deadline))
    {//TODO: when there is no deadline tag then probably other type of packets such as a BLOCK_ACK_REQUEST control packet. So just let it pass.
      //TODO: This should not cause a problem since its just like FCFS aggregation policy
#ifdef DEBUG_SVA_DETAIL
      cout << "MpduUniversalAggregator: No deadline in packet! \n";
#endif
      return true;
    }
  if (deadline.GetTimestamp() <= Simulator::Now()+Seconds(m_serviceInterval)) //if deadline will be violated by the next service interval then aggregate
    {
      return true;
    }
  else
    {
      return false;
    }

}


bool
MpduUniversalAggregator::TimeAllowanceCanBeAggregated (Ptr<const Packet> peekedPacket, Ptr<Packet> aggregatedPacket, uint16_t blockAckSize)
{//TODO needs implementation
  TimestampTag deadline;

  if (!peekedPacket->FindFirstMatchingByteTag(deadline))
    {//TODO: when there is no deadline tag then probably other type of packets such as a BLOCK_ACK_REQUEST control packet. So just let it pass.
      //TODO: This should not cause a problem since its just like FCFS aggregation policy
#ifdef DEBUG_SVA_DETAIL
      cout << "MpduUniversalAggregator: No deadline in packet! \n";
#endif
      return true;
    }
  if (deadline.GetTimestamp() <= Simulator::Now()+Seconds(m_serviceInterval)) //if deadline will be violated by the next service interval then aggregate
    {
      return true;
    }
  else
    {
      return false;
    }

}

void
MpduUniversalAggregator::UpdateTimeAllowance(void)
{//TODO
  return;
}

}  // namespace ns3
