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
                   EnumValue (TIME_ALLOWANCE),
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
  m_controller = CreateObject<TimeAllowanceAggregationController>();//should be generalized
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
  m_controller->SetQueue(m_queue, c);
  m_controller->SetAggregator(this);
  return true;
}

//sva: This is the function where our aggregation algorithm should be implemented
bool
MpduUniversalAggregator::Aggregate (Ptr<const Packet> packet, Ptr<Packet> aggregatedPacket)
{
  NS_LOG_FUNCTION (this);
  Ptr<Packet> currentPacket;
  AmpduSubframeHeader currentHdr;

  WifiMacHeader peekedHdr;
  //the packet has correct header installed before this function is called --> ok
  packet->PeekHeader(peekedHdr);

  uint32_t padding = CalculatePadding (aggregatedPacket);
  //old code: uint32_t actualSize = aggregatedPacket->GetSize ();

  //sva: checks whether adding this packet will exceed max aggregation size
  //sva: old code was: if ((4 + packet->GetSize () + actualSize + padding) <= m_maxAmpduLength)
#ifdef SVA_DEBUG_DETAIL
  std::cout << "MpduUniversalAggregator::Aggregate Packet: " << packet->ToString();// << " Aggregated Packet: " << aggregatedPacket->ToString();
  std::ostringstream os;
  peekedHdr.Print(os);
  std::cout << " Header : " << os.str() << "\n";
#endif

  if ( CanBeAggregated(packet, peekedHdr, aggregatedPacket, 0, Seconds(0)) )//0: means no block ack request bits, 0: means duration is zero (could cause a bug!)
    //sva note:the duration field is not known at this point and only a size check is performed
    //however, this is after StopAggregation() is called which calls CanBeAggregated() with the right duration field
    //so shouldn't be a problem. Nevertheless there is one independent call to this function for the first packet in an A-MPDU
    //Hopefully this does not require duration check.
    //sva bug: but it does!
    //sva bug: when supplying this with zero duration, it will cause failure of TimeAllowanceCanBeAggregate
    //TODO: Leaving it as is for now
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
  //This is called to prepare packets from the aggregate queue to be sent so no need to check total size since it has already been
  //done before, when deciding how many packets to add to the queue
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
MpduUniversalAggregator::CanBeAggregated (Ptr<const Packet> peekedPacket, WifiMacHeader peekedHeader, Ptr<Packet> aggregatedPacket, uint16_t blockAckSize, Time duration)
{
  bool result = false;
  //only perform specialized aggregation for QoS_DATA packets, the rest just use conventional method of aggregation
  if (!peekedHeader.IsQosData())
    {
      result = StandardCanBeAggregated(peekedPacket, peekedHeader, aggregatedPacket, blockAckSize, duration);
      return result;
    }
  if (m_perStaQInfo)//if supposed to support aggregation on per queue basis
    {
      switch (m_aggregationAlgorithm)
      {
        case STANDARD:
          result = StandardCanBeAggregated(peekedPacket, peekedHeader, aggregatedPacket, blockAckSize, duration);
          break;
        case DEADLINE:
          result = DeadlineCanBeAggregated(peekedPacket, peekedHeader, aggregatedPacket, blockAckSize, duration);
          break;
        case TIME_ALLOWANCE:
          result = TimeAllowanceCanBeAggregated(peekedPacket, peekedHeader, aggregatedPacket, blockAckSize, duration);
          break;
          /*sva-design: add for new aggregation algorithm AGG_ALG
    case AGG_ALG:
      result = XxxCanBeAggregated(peekedPacket, peekedHeader, aggregatedPacket, blockAckSize, duration);
      break;
      sva-design*/
        default:
          NS_FATAL_ERROR("Unspecified Aggregation Algorithm" << m_aggregationAlgorithm);
      }
    }
  else//no support for per queue aggregation then just perform the standard version
    {
      result = StandardCanBeAggregated(peekedPacket, peekedHeader, aggregatedPacket, blockAckSize, duration);
#ifdef SVA_DEBUG_DETAIL
      std::cout << Simulator::Now().GetSeconds() << " MpduUniversalAggregator::CanBeAggregated m_perStaQInfo not supported at "
          << peekedHeader.GetAddr2() << " \n";
#endif
    }

  if (m_pendingServiceInterval)
    {//if in pending state then keep checking every time CanBeAggregated is called since things may have changed
      if (!result)
        {
          Simulator::ScheduleNow(&MpduUniversalAggregator::PendingServiceInterval, this); //make sure current line of execution is finished
#ifdef SVA_DEBUG_DETAIL
      std::cout << Simulator::Now().GetSeconds() << " MpduUniversalAggregator::CanBeAggregated pending service interval, CAN NOT BE AGGREGATED so re-schedule PendingServiceInterval \n";
#endif
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

#ifdef SVA_DEBUG_DETAIL
      std::cout << Simulator::Now().GetSeconds() << " MpduUniversalAggregator::IsReadyForNextServiceIntervalTimeAllowance: service interval ";
#endif

  if (!m_perStaQInfo)
    {//does not support per station queues
#ifdef SVA_DEBUG_DETAIL
      if (m_low)
        {
          std::cout << "m_perStaQInfo not initialized on "<< m_low->GetAddress() << " ";
        }
      else
        {
          std::cout << "m_perStaQInfo not initialized on "<< "UNKNOWN-MAC" << " ";
        }
#endif
      return flag;
    }

  for (it = m_perStaQInfo->Begin(); it != m_perStaQInfo->End(); ++it )
    {
      if ( !(*it)->IsInsufficientTimeAllowanceEncountered() && !(*it)->IsEmpty())
        {
          flag = false;
#ifdef SVA_DEBUG_DETAIL
      std::cout << "time allowance (" << (*it)->GetRemainingTimeAllowance().GetSeconds()*1000
          << ") msec not used up at non-empty queue of " << (*it)->GetMac() << " ";
#endif
        }
    }
  std::cout << "\n";
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
#ifdef SVA_DEBUG_DETAIL
      std::cout << Simulator::Now().GetSeconds() << " MpduUniversalAggregator::PendingServiceInterval pending service interval, ready for begin is = "
          << ready << "\n";
#endif
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
  if(!m_perStaQInfo)
    {
#ifdef SVA_DEBUG_DETAIL
      if (m_low)
        {
          std::cout << Simulator::Now().GetSeconds() << " MpduUniversalAggregator::ResetTimeAllowance on "
              << m_low->GetAddress() << " m_perStaQInfo not initialized\n";
        }
      else
        {
          std::cout << Simulator::Now().GetSeconds() << " MpduUniversalAggregator::ResetTimeAllowance on "
              << "UNKNOWN-MAC" << " m_perStaQInfo not initialized\n";
        }
#endif
      return ;
    }

  for (PerStaQInfoContainer::Iterator it = m_perStaQInfo->Begin(); it != m_perStaQInfo->End(); ++it)
    {
      (*it)->Update();//update statistics
      (*it)->ResetTimeAllowance();
      (*it)->CollectServedPackets();
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
MpduUniversalAggregator::StandardCanBeAggregated (Ptr<const Packet> peekedPacket, WifiMacHeader peekedHeader, Ptr<Packet> aggregatedPacket, uint16_t blockAckSize, Time duration)
{
  uint32_t padding = CalculatePadding (aggregatedPacket);
  uint32_t actualSize = aggregatedPacket->GetSize ();
  uint32_t packetSize = peekedPacket->GetSize () + peekedHeader.GetSize () + WIFI_MAC_FCS_LENGTH;
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
MpduUniversalAggregator::DeadlineCanBeAggregated (Ptr<const Packet> peekedPacket, WifiMacHeader peekedHeader, Ptr<Packet> aggregatedPacket, uint16_t blockAckSize, Time duration)
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
MpduUniversalAggregator::TimeAllowanceCanBeAggregated (Ptr<const Packet> peekedPacket, WifiMacHeader peekedHeader, Ptr<Packet> aggregatedPacket, uint16_t blockAckSize, Time duration)
{
  TimestampTag deadline;

  if (!peekedPacket->FindFirstMatchingByteTag(deadline))
    {//TODO: BLOCK_ACK_REQUEST
#ifdef DEBUG_SVA_DETAIL
      cout << "MpduUniversalAggregator::TimeAllowanceCanBeAggregated No deadline in packet! return true;\n";
#endif
      return true;
    }

  Ptr<PerStaQInfo> staQInfo;
  staQInfo = m_perStaQInfo->GetByMac(peekedHeader.GetAddr1());
#ifdef SVA_DEBUG_DETAIL
      std::cout << Simulator::Now().GetSeconds() << " RTA MpduUniversalAggregator::TimeAllowanceCanBeAggregated "
          << "Deadline is " << deadline.GetTimestamp().GetSeconds() << " "
          << peekedPacket->ToString() << " ";
      std::ostringstream os;
      peekedHeader.Print(os);
      std::cout << "Header : " << os.str() << " ";
#endif
  NS_ASSERT_MSG(staQInfo,"MpduUniversalAggregator::TimeAllowanceCanBeAggregated no matching station queue found for packet!\n");

#ifdef SVA_DEBUG_DETAIL
  std::cout << "duration= " << duration.GetSeconds()*1000 << " RTA= " << staQInfo->GetRemainingTimeAllowance().GetSeconds()*1000 << "\n";
#endif
  if (duration <= staQInfo->GetRemainingTimeAllowance()) //if there is still time allowance then aggregate. time allowance will be updated once actually transmitted
    {
      return true;
    }
  else
    {
      staQInfo->SetInsufficientTimeAllowanceEncountered();
      return false;
    }

}

/*sva-design: add for new aggregation algorithm AGG_ALG
bool
MpduUniversalAggregator::XxxCanBeAggregated (Ptr<const Packet> peekedPacket, WifiMacHeader peekedHeader, Ptr<Packet> aggregatedPacket, uint16_t blockAckSize, Time duration)
{
}
sva-design*/

void
MpduUniversalAggregator::DoUpdate(void)
{
  m_controller->Update();
}

}  // namespace ns3
