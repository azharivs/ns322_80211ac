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
    .AddAttribute ("Algorithm", "The aggregation algorithm used for selecting packets to join the A-MPDU.",
                   EnumValue (DEADLINE),
                   MakeEnumAccessor (&MpduUniversalAggregator::m_aggregationAlgorithm),
                   MakeEnumChecker (ns3::STANDARD, "ns3::STANDARD",
                                    ns3::DEADLINE, "ns3::DEADLINE",
                                    /*sva-design: add for new aggregation algorithm AGG_ALG
                                    ns3::AGG_ALG, "ns3::AGG_ALG",
                                    sva-design*/
                                    ns3::TIME_ALLOWANCE, "ns3::TIME_ALLOWANCE"))
;
  return tid;
}

MpduUniversalAggregator::MpduUniversalAggregator ()
{
  m_perStaQInfo = NULL; //should be initialized later if required
}

MpduUniversalAggregator::~MpduUniversalAggregator ()
{
}

bool
MpduUniversalAggregator::EnablePerStaQInfo (PerStaQInfoContainer &c, Ptr<PerStaWifiMacQueue> queue, Ptr<MacLow> low, Ptr<WifiPhy> phy)
{
  if (c.IsEmpty())
    {
      NS_ASSERT_MSG(true,"Initializing with Empty Container !!");
      return false;
    }
  m_perStaQInfo = &c;
  m_queue = queue;
  m_low = low;
  m_phy = phy;
  return true;
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
  bool result = false;
  switch (m_aggregationAlgorithm)
  {
    case STANDARD:
      result = StandardCanBeAggregated(peekedPacket, aggregatedPacket, blockAckSize);
      break;
    case DEADLINE:
      result = DeadlineCanBeAggregated(peekedPacket, aggregatedPacket, blockAckSize);
      break;
    case TIME_ALLOWANCE:
      result = TimeAllowanceCanBeAggregated(peekedPacket, aggregatedPacket, blockAckSize);
      break;
      /*sva-design: add for new aggregation algorithm AGG_ALG
    case AGG_ALG:
      result = XxxCanBeAggregated(peekedPacket, aggregatedPacket, blockAckSize);
      break;
      sva-design*/
    default:
      NS_FATAL_ERROR("Unspecified Aggregation Algorithm" << m_aggregationAlgorithm);
  }
  if (m_pendingServiceInterval)
    {//if in pending state then keep checking every time CanBeAggregated is called since things may have changed
      if (!result)
        {
          Simulator::ScheduleNow(&MpduUniversalAggregator::PendingServiceInterval, this); //make sure current line of execution is finished
        }
    }
  return result;
}

uint32_t
MpduUniversalAggregator::CalculatePadding (Ptr<const Packet> packet)
{
  return (4 - (packet->GetSize () % 4 )) % 4;
}

bool
MpduUniversalAggregator::IsReadyForNextServiceIntervalTimeAllowance(void)
{
  PerStaQInfoContainer::Iterator it;
  Time allowance;
  bool flag = true;

  for (it = m_perStaQInfo->Begin(); it != m_perStaQInfo->End(); ++it )
    {
      if ( !(*it)->IsInsufficientTimeAllowanceEncountered() && !(*it)->IsEmpty())
        {
          flag = false;
        }
    }
  return flag;
}

void
MpduUniversalAggregator::PendingServiceInterval(void)
{
  m_pendingServiceInterval = true;
  bool ready=false;
  switch (m_aggregationAlgorithm)
  {
    case TIME_ALLOWANCE:
      ready = IsReadyForNextServiceIntervalTimeAllowance();
      break;
      /*sva-design: add for new algorithm AGG_ALG
    case AGG_ALG:
      ResetXxx();
      break;
      sva-design*/
    default:
      ready = true;
      break;//do nothing
  }
  if (ready)
    {
      BeginServiceInterval();
    }

  //if still in pending state then it will be handled by future calls to CanBeAggregated()

}

void
MpduUniversalAggregator::BeginServiceInterval(void)
{
  m_pendingServiceInterval = false;
  //run the appropriate aggregator controller algorithm
  //that updates all aggregation parameters to their new values
  DoUpdate ();
  //now perform appropriate book keeping
  //depending on the type of aggregation algorithm
  //These are initializations required at the start of a service interval
  switch (m_aggregationAlgorithm)
  {
    case TIME_ALLOWANCE:
      ResetTimeAllowance();
      break;
      /*sva-design: add for new algorithm AGG_ALG
    case AGG_ALG:
      ResetXxx();
      break;
      sva-design*/
    default:
      break;//do nothing
  }
  m_currentServiceIntervalStart = Simulator::Now();
  //NOTE: scheduling of next event is done when all queues are served at PerStaWifiMacQueue::BeginServiceInterval()
  m_queue->BeginServiceInterval();
}

void
MpduUniversalAggregator::ResetTimeAllowance (void)
{
  NS_ASSERT_MSG(!m_perStaQInfo,"MpduUniversalAggregator::ResetTimeAllowance, m_perStaQInfo = NULL! \n");

  for (PerStaQInfoContainer::Iterator it = m_perStaQInfo->Begin(); it != m_perStaQInfo->End(); ++it)
    {
      (*it)->ResetTimeAllowance();
    }
}

/*sva-design: add for new aggregation algorithm AGG_ALG
void
MpduUniversalAggregator::ResetXxx (void)
{
  NS_ASSERT_MSG(!m_perStaQInfo,"MpduUniversalAggregator::ResetXxx, m_perStaQInfo = NULL! \n");

}
sva-design*/

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
{
  WifiMacHeader peekedHdr;
  peekedPacket->PeekHeader(peekedHdr);
  WifiPreamble preamble;
  WifiTxVector dataTxVector = m_low->GetDataTxVector (peekedPacket, &peekedHdr);
  if (m_phy->GetGreenfield () && m_low->m_stationManager->GetGreenfieldSupported (peekedHdr.GetAddr1 ()))
    {
      preamble = WIFI_PREAMBLE_HT_GF;
    }
  else
    {
      preamble = WIFI_PREAMBLE_HT_MF;
    }

  Time duration = m_phy->CalculateTxDuration (aggregatedPacket->GetSize () + peekedPacket->GetSize () + peekedHdr.GetSize () +WIFI_MAC_FCS_LENGTH,dataTxVector, preamble, m_phy->GetFrequency(), 0, 0);

  Ptr<PerStaQInfo> staQInfo;
  staQInfo = m_perStaQInfo->GetByMac(peekedHdr.GetAddr1());

  TimestampTag deadline;

  if (!peekedPacket->FindFirstMatchingByteTag(deadline))
    {//TODO: when there is no deadline tag then probably other type of packets such as a BLOCK_ACK_REQUEST control packet. So just let it pass.
      //TODO: This should not cause a problem since its just like FCFS aggregation policy
#ifdef DEBUG_SVA_DETAIL
      cout << "MpduUniversalAggregator: No deadline in packet! \n";
#endif
      return true;
    }
  if (duration <= staQInfo->GetRemainingTimeAllowance()) //if there is still time allowance then aggregate. time allowance will be updated once actually transmitted
    {
      return true;
    }
  else
    {
      return false;
    }

}

/*sva-design: add for new aggregation algorithm AGG_ALG
bool
MpduUniversalAggregator::XxxCanBeAggregated (Ptr<const Packet> peekedPacket, Ptr<Packet> aggregatedPacket, uint16_t blockAckSize)
{
}
sva-design*/

void
MpduUniversalAggregator::DoUpdate(void)
{
  m_controller->Update();
}

}  // namespace ns3
