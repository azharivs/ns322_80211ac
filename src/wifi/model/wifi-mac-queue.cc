/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005, 2009 INRIA
 * Copyright (c) 2009 MIRKO BANCHI
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 * Author: Mirko Banchi <mk.banchi@gmail.com>
 * Added support for PerStaWifiMacQueue by,
 * Author: SEYED VAHID AZHARI <azharivs@iust.ac.ir>
 * Iran University of Science & Technology
 */

#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/string.h"
#include "wifi-mac-queue.h"
#include "wifi-tx-vector.h"
#include "per-bitrate-timeallowance.h"
#include "mac-low.h"
#include "qos-blocked-destinations.h"
#include "ns3/per-sta-q-info-container.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (WifiMacQueue);

WifiMacQueue::Item::Item (Ptr<const Packet> packet,
                          const WifiMacHeader &hdr,
                          Time tstamp)
  : packet (packet),
    hdr (hdr),
    tstamp (tstamp)
{
}

TypeId
WifiMacQueue::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::WifiMacQueue")
    .SetParent<Object> ()
    .AddConstructor<WifiMacQueue> ()
    .AddAttribute ("MaxPacketNumber", "If a packet arrives when there are already this number of packets, it is dropped.",
                   UintegerValue (30000),
                   MakeUintegerAccessor (&WifiMacQueue::m_maxSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("MaxDelay", "If a packet stays longer than this delay in the queue, it is dropped.",
                   TimeValue (Seconds (40.0)),
                   MakeTimeAccessor (&WifiMacQueue::m_maxDelay),
                   MakeTimeChecker ())
  ;
  return tid;
}

WifiMacQueue::WifiMacQueue ()
  : m_size (0)
{
}

WifiMacQueue::~WifiMacQueue ()
{
  Flush ();
}

void
WifiMacQueue::SetMaxSize (uint32_t maxSize)
{
  m_maxSize = maxSize;
}

void
WifiMacQueue::SetMaxDelay (Time delay)
{
  m_maxDelay = delay;
}

uint32_t
WifiMacQueue::GetMaxSize (void) const
{
  return m_maxSize;
}

Time
WifiMacQueue::GetMaxDelay (void) const
{
  return m_maxDelay;
}

void
WifiMacQueue::Enqueue (Ptr<const Packet> packet, const WifiMacHeader &hdr)
{
  Cleanup ();
  if (m_size == m_maxSize)
    {
      return;
    }
  Time now = Simulator::Now ();
  m_queue.push_back (Item (packet, hdr, now));
  m_size++;
}

void
WifiMacQueue::Cleanup (void)
{
  if (m_queue.empty ())
    {
      return;
    }

  //std::cout << "WifiMacQueue::CleanUp()\n";
  Time now = Simulator::Now ();
  uint32_t n = 0;
  for (PacketQueueI i = m_queue.begin (); i != m_queue.end ();)
    {
      if (i->tstamp + m_maxDelay > now)
        {
          i++;
        }
      else
        {
          i = m_queue.erase (i);
          n++;
        }
    }
  m_size -= n;
}

Ptr<const Packet>
WifiMacQueue::Dequeue (WifiMacHeader *hdr)
{
  Cleanup ();
  if (!m_queue.empty ())
    {
      Item i = m_queue.front ();
      m_queue.pop_front ();
      m_size--;
      *hdr = i.hdr;
      return i.packet;
    }
  return 0;
}

Ptr<const Packet>
WifiMacQueue::Peek (WifiMacHeader *hdr)
{
  Cleanup ();
  if (!m_queue.empty ())
    {
      Item i = m_queue.front ();
      *hdr = i.hdr;
      return i.packet;
    }
  return 0;
}

Ptr<const Packet>
WifiMacQueue::DequeueByTidAndAddress (WifiMacHeader *hdr, uint8_t tid,
                                      WifiMacHeader::AddressType type, Mac48Address dest)
{
  Cleanup ();
  Ptr<const Packet> packet = 0;
  if (!m_queue.empty ())
    {
      PacketQueueI it;
      for (it = m_queue.begin (); it != m_queue.end (); ++it)
        {
          if (it->hdr.IsQosData ())
            {
              if (GetAddressForPacket (type, it) == dest
                  && it->hdr.GetQosTid () == tid)
                {
                  packet = it->packet;
                  *hdr = it->hdr;
                  m_queue.erase (it);
                  m_size--;
                  break;
                }
            }
        }
    }
  return packet;
}

Ptr<const Packet>
WifiMacQueue::PeekByTidAndAddress (WifiMacHeader *hdr, uint8_t tid,
                                   WifiMacHeader::AddressType type, Mac48Address dest, Time *timestamp)
{
  Cleanup ();
  if (!m_queue.empty ())
    {
      PacketQueueI it;
      for (it = m_queue.begin (); it != m_queue.end (); ++it)
        {
          if (it->hdr.IsQosData ())
            {
              if (GetAddressForPacket (type, it) == dest
                  && it->hdr.GetQosTid () == tid)
                {
                  *hdr = it->hdr;
                  *timestamp=it->tstamp;
                  return it->packet;
                }
            }
        }
    }
  return 0;
}

bool
WifiMacQueue::IsEmpty (void)
{
  Cleanup ();
  return m_queue.empty ();
}

uint32_t
WifiMacQueue::GetSize (void)
{
  return m_size;
}

void
WifiMacQueue::Flush (void)
{
  m_queue.erase (m_queue.begin (), m_queue.end ());
  m_size = 0;
}

Mac48Address
WifiMacQueue::GetAddressForPacket (enum WifiMacHeader::AddressType type, PacketQueueI it)
{
  if (type == WifiMacHeader::ADDR1)
    {
      return it->hdr.GetAddr1 ();
    }
  if (type == WifiMacHeader::ADDR2)
    {
      return it->hdr.GetAddr2 ();
    }
  if (type == WifiMacHeader::ADDR3)
    {
      return it->hdr.GetAddr3 ();
    }
  return 0;
}

bool
WifiMacQueue::Remove (Ptr<const Packet> packet)
{
  PacketQueueI it = m_queue.begin ();
  for (; it != m_queue.end (); it++)
    {
      if (it->packet == packet)
        {
#ifdef SVA_DEBUG_DETAIL
          if (it->hdr.IsData()) std::cout << "@Remove ------WifiMacQueue = " << m_size-1 << " TID= " <<  (int) it->hdr.GetQosTid() << "\n";
          else std::cout << "@Remove ------WifiMacQueue = " << m_size-1 << " TID= ?\n";
#endif
          m_queue.erase (it);
          m_size--;
          return true;
        }
    }
  return false;
}

void
WifiMacQueue::PushFront (Ptr<const Packet> packet, const WifiMacHeader &hdr)
{
  Cleanup ();
  if (m_size == m_maxSize)
    {
      return;
    }
  Time now = Simulator::Now ();
  m_queue.push_front (Item (packet, hdr, now));
  m_size++;
}

//sva: could be optimized using PerStaQInfoContainer
uint32_t
WifiMacQueue::GetNPacketsByTidAndAddress (uint8_t tid, WifiMacHeader::AddressType type,
                                          Mac48Address addr)
{
  Cleanup ();
  uint32_t nPackets = 0;
  if (!m_queue.empty ())
    {
      PacketQueueI it;
      for (it = m_queue.begin (); it != m_queue.end (); it++)
        {
          if (GetAddressForPacket (type, it) == addr)
            {
              if (it->hdr.IsQosData () && it->hdr.GetQosTid () == tid)
                {
                  nPackets++;
                }
            }
        }
    }
  return nPackets;
}

Ptr<const Packet>
WifiMacQueue::DequeueFirstAvailable (WifiMacHeader *hdr, Time &timestamp,
                                     const QosBlockedDestinations *blockedPackets)
{
  //std::cout << "WifiMacQueue::DequeueFirstAvailable \n";
  Cleanup ();
  Ptr<const Packet> packet = 0;
  for (PacketQueueI it = m_queue.begin (); it != m_queue.end (); it++)
    {
      if (!it->hdr.IsQosData ()
          || !blockedPackets->IsBlocked (it->hdr.GetAddr1 (), it->hdr.GetQosTid ()))
        {
          *hdr = it->hdr;
          timestamp = it->tstamp;
          packet = it->packet;
          m_queue.erase (it);
          m_size--;
          return packet;
        }
    }
  return packet;
}

Ptr<const Packet>
WifiMacQueue::PeekFirstAvailable (WifiMacHeader *hdr, Time &timestamp,
                                  const QosBlockedDestinations *blockedPackets)
{
  //std::cout << "WifiMacQueue::PeekFirstAvailable \n";
  Cleanup ();
  for (PacketQueueI it = m_queue.begin (); it != m_queue.end (); it++)
    {
      if (!it->hdr.IsQosData ()
          || !blockedPackets->IsBlocked (it->hdr.GetAddr1 (), it->hdr.GetQosTid ()))
        {
          *hdr = it->hdr;
          timestamp = it->tstamp;
          return it->packet;
        }
    }
  return 0;
}



/**
 * PerStaWifiMacQueue implementation starts here
 *
 *
 */

NS_OBJECT_ENSURE_REGISTERED (PerStaWifiMacQueue);

TypeId
PerStaWifiMacQueue::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PerStaWifiMacQueue")
    .SetParent<WifiMacQueue> ()
    .AddConstructor<PerStaWifiMacQueue> ()
    .AddAttribute ("ServiceInterval", "Service interval period in seconds with which queues are served.",
                   DoubleValue (0.1),
                   MakeDoubleAccessor (&PerStaWifiMacQueue::m_serviceInterval),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("ServicePolicy", "The Service Policy Applied to Each AC Queue.",
                   EnumValue (MAX_REMAINING_TIME_ALLOWANCE),
                   MakeEnumAccessor (&PerStaWifiMacQueue::m_servicePolicy),
                   MakeEnumChecker (ns3::FCFS, "ns3::FCFS",
                                    ns3::EDF, "ns3::EDF",
                                    ns3::PER_BITRATE_TIME_ALLOWANCE_RR, "ns3::PER_BITRATE_TIME_ALLOWANCE_RR",
                                    ns3::MAX_REMAINING_TIME_ALLOWANCE, "ns3::MAX_REMAINING_TIME_ALLOWANCE",
                                    ns3::MAX_QUEUE_SURPLUS,"ns3::MAX_QUEUE_SURPLUS",
                                    ns3::EDF_RR, "ns3::EDF_RR"))
  ;
  return tid;
}

PerStaWifiMacQueue::PerStaWifiMacQueue ()
{
  //sva: should be set to appropriate value by EnablePerStaQInfo () method if support is required
  PerStaWifiMacQueue::m_perStaQInfo = NULL;
}

PerStaWifiMacQueue::~PerStaWifiMacQueue ()
{
  Flush ();
}

bool
PerStaWifiMacQueue::EnablePerStaQInfo(PerStaQInfoContainer &c)
{
  if (c.IsEmpty())
    {
      NS_ASSERT_MSG(true,"Initializing with Empty Container !!"); //TODO: this is useless!
      return false;
    }
  m_perStaQInfo = &c;
  Simulator::Schedule(NanoSeconds(1), &PerStaWifiMacQueue::PendingServiceInterval, this);
  m_lastServed = c.Begin();//used for RR-based service policies
  return true;
}

bool
PerStaWifiMacQueue::SetMpduAggregator(Ptr<MpduUniversalAggregator> agg)
{
  NS_ASSERT_MSG(agg,"Initializing with NULL Aggregator !!");
  m_mpduAggregator = agg;
  return true;
}


bool
PerStaWifiMacQueue::SetMacLow(Ptr<MacLow> low)
{
  NS_ASSERT_MSG(low,"Initializing with NULL MacLow !!");
  m_low = low;
  return true;
}

void
PerStaWifiMacQueue::Enqueue (Ptr<const Packet> packet, const WifiMacHeader &hdr)
{
  Cleanup ();
  if (m_size == m_maxSize)
    {
      return;
    }
  Time now = Simulator::Now ();
  m_queue.push_back (Item (packet, hdr, now));
  m_size++;
  //NS_ASSERT_MSG(m_perStaQInfo,"PerStaQInfoContainer not initialized!");
  if (m_perStaQInfo)
    {
      m_perStaQInfo->Arrival(packet, hdr, now);//sva: deal with PerStaQInfo issues
    }
#ifdef SVA_DEBUG_DETAIL
  if (hdr.IsData()) std::cout << "@Enqueue PerStaWifiMacQueue = " << m_size << " TID= " <<  (int) hdr.GetQosTid() << "\n";
#endif
}

void
PerStaWifiMacQueue::Cleanup (void)
{
  if (m_queue.empty ())
    {
      return;
    }

  Time now = Simulator::Now ();
  uint32_t n = 0;
  //NS_ASSERT_MSG(m_perStaQInfo,"PerStaQInfoContainer not initialized!");
  for (PacketQueueI i = m_queue.begin (); i != m_queue.end ();)
    {
      if (i->tstamp + m_maxDelay > now)
        {
          i++;
        }
      else
        {
          //sva: should I differentiate between lost packets and those dequeued?
          //TODO: right now they are treated the same. Could cause false average waiting times
          if (m_perStaQInfo)
            {
              m_perStaQInfo->Departure(i->packet,i->hdr,i->tstamp);
            }
#ifdef SVA_DEBUG_DETAIL
          if (i->hdr.IsData()) std::cout << "@Cleanup PerStaWifiMacQueue = " << m_size-n-1 << " TID= " << (int) i->hdr.GetQosTid() << "\n";
#endif
          i = m_queue.erase (i);
          n++;
        }
    }
  m_size -= n;
}

Ptr<const Packet>
PerStaWifiMacQueue::Peek (WifiMacHeader *hdr)
{
  //NS_ASSERT_MSG(m_perStaQInfo,"PerStaQInfoContainer not initialized!");
  Cleanup ();
  if (!m_queue.empty ())
    {
      Item i = m_queue.front ();
      *hdr = i.hdr;
      return i.packet;
    }
  return 0;
}

Ptr<const Packet>
PerStaWifiMacQueue::Dequeue (WifiMacHeader *hdr)
{
  Time now = Simulator::Now();
  Cleanup ();
  if (!m_queue.empty ())
    {
      Item i = m_queue.front ();
      //NS_ASSERT_MSG(m_perStaQInfo,"PerStaQInfoContainer not initialized!");
      if (m_perStaQInfo)
        {
          m_perStaQInfo->Departure(i.packet,i.hdr,i.tstamp);//sva: deal with PerStaQInfo issues
        }
      m_queue.pop_front ();
      m_size--;
#ifdef SVA_DEBUG_EDFBUG
      if (i.hdr.IsData()) std::cout << "@Dequeue PerStaWifiMacQueue = " << m_size << " MAC= " <<  i.hdr.GetAddr1() << "\n";
#endif
      *hdr = i.hdr;
      return i.packet;
    }
  return 0;
}


Ptr<const Packet>
PerStaWifiMacQueue::DequeueByTidAndAddress (WifiMacHeader *hdr, uint8_t tid,
                                      WifiMacHeader::AddressType type, Mac48Address dest)
{
  Time now = Simulator::Now();
  //NS_ASSERT_MSG(m_perStaQInfo,"PerStaQInfoContainer not initialized!");
  Cleanup ();
  Ptr<const Packet> packet = 0;
  if (!m_queue.empty ())
    {
      PacketQueueI it;
      for (it = m_queue.begin (); it != m_queue.end (); ++it)
        {
          if (it->hdr.IsQosData ())
            {
              if (GetAddressForPacket (type, it) == dest
                  && it->hdr.GetQosTid () == tid)
                {
                  packet = it->packet;
                  *hdr = it->hdr;
                  if (m_perStaQInfo)
                    {
                      m_perStaQInfo->Departure(it->packet,it->hdr,it->tstamp);//sva: deal with PerStaQInfo issues
                    }
#ifdef SVA_DEBUG_DETAIL
                  if (it->hdr.IsData()) std::cout << "@DequeueByTidAndAddress PerStaWifiMacQueue = " << m_size-1 << " TID= " <<  (int) it->hdr.GetQosTid() << "\n";
#endif
                  m_queue.erase (it);
                  m_size--;
                  break;
                }
            }
        }
    }
  return packet;
}


void
PerStaWifiMacQueue::Flush (void)
{
  //NS_ASSERT_MSG(m_perStaQInfo,"PerStaQInfoContainer not initialized!");
  if (m_perStaQInfo)
    {
      m_perStaQInfo->Reset();//sva: deal with PerStaQInfo issues
    }
  m_queue.erase (m_queue.begin (), m_queue.end ());
  m_size = 0;
}


bool
PerStaWifiMacQueue::Remove (Ptr<const Packet> packet)
{
  Time now = Simulator::Now();
  //NS_ASSERT_MSG(m_perStaQInfo,"PerStaQInfoContainer not initialized!");
  PacketQueueI it = m_queue.begin ();
  for (; it != m_queue.end (); it++)
    {
      if (it->packet == packet)
        {
          if (m_perStaQInfo)
            {
              m_perStaQInfo->Departure(it->packet,it->hdr,it->tstamp);//sva: deal with PerStaQInfo issues
            }
#ifdef SVA_DEBUG_DETAIL
          if (it->hdr.IsData()) std::cout << "@Remove PerStaWifiMacQueue = " << m_size-1 << " TID= " <<  (int) it->hdr.GetQosTid() << "\n";
          else std::cout << "@Remove PerStaWifiMacQueue = " << m_size-1 << " TID= ?\n";
#endif
          m_queue.erase (it);
          m_size--;
          return true;
        }
    }
  return false;
}

void
PerStaWifiMacQueue::PushFront (Ptr<const Packet> packet, const WifiMacHeader &hdr)
{
  //NS_ASSERT_MSG(m_perStaQInfo,"PerStaQInfoContainer not initialized!");
  Cleanup ();
  if (m_size == m_maxSize)
    {
      return;
    }
  Time now = Simulator::Now ();
  m_queue.push_front (Item (packet, hdr, now));
  m_size++;
  if (m_perStaQInfo)
    {
      m_perStaQInfo->Arrival(packet, hdr, now);//sva: deal with PerStaQInfo issues
    }
#ifdef SVA_DEBUG_DETAIL
  if (hdr.IsData()) std::cout << "@PushFront PerStaWifiMacQueue = " << m_size << " TID= " <<  (int) hdr.GetQosTid() << "\n";
#endif
}


bool
PerStaWifiMacQueue::GetStaHol (PacketQueueI &it, uint8_t tid, Mac48Address dest,
                               const QosBlockedDestinations *blockedPackets)
{
  if (!m_queue.empty ())
    {
      PacketQueueI i;
      for (i = m_queue.begin (); i != m_queue.end (); ++i)
        {
          if (i->hdr.IsQosData ())
            {
              if (GetAddressForPacket (WifiMacHeader::ADDR1, i) == dest
                  && i->hdr.GetQosTid () == tid)
                {
                  it = i;
                  return true;
                }
            }
        }
    }
  return false;
}


//Modified Version with Arbitration
Ptr<const Packet>
PerStaWifiMacQueue::DequeueFirstAvailable (WifiMacHeader *hdr, Time &timestamp,
                                           const QosBlockedDestinations *blockedPackets)
{
  Time now = Simulator::Now();
  //NS_ASSERT_MSG(m_perStaQInfo,"PerStaQInfoContainer not initialized!");
  Cleanup ();
  Ptr<const Packet> packet = 0;
  PacketQueueI it;
  bool found = false;
  switch (m_servicePolicy)
  {
    case FCFS:
      found = PeekFcfs(it, blockedPackets);
      break;
    case EDF:
      found = PeekEdf(it,blockedPackets);
      break;
    case EDF_RR:
      found = PeekEdfRoundRobin(it,blockedPackets);
      break;
    case MAX_REMAINING_TIME_ALLOWANCE:
      found = PeekMaxRemainingTimeAllowance(it,blockedPackets);
      break;
    case PER_BITRATE_TIME_ALLOWANCE_RR:
      found = PeekPerBitrateTimeAllowanceRoundRobin(it,blockedPackets);
      break;
    case MAX_QUEUE_SURPLUS:
      found = PeekMaxQueueSurplus(it,blockedPackets);
      break;
      /*sva-design: add for appropriate service policy ?
    case ?:
      found = Peek?(it,blockedPackets);
      break;
      sva-design*/
    default: NS_FATAL_ERROR("Unrecongnized Queue Arbitration Algorithm : " << m_servicePolicy);
  }
  if (found)
    {
      *hdr = it->hdr;
      timestamp = it->tstamp;
      packet = it->packet;
      if (m_perStaQInfo)
        {
          m_perStaQInfo->Departure(it->packet,it->hdr,it->tstamp);//sva: deal with PerStaQInfo issues
        }
#ifdef SVA_DEBUG_DETAIL
  if (it->hdr.IsData()) std::cout << "@DequeueFirstAvailable PerStaWifiMacQueue = " << m_size-1 << " TID= " <<  (int) it->hdr.GetQosTid() << "\n";
#endif
      m_queue.erase (it);
      m_size--;
    }
  return packet;
}

//Modified Version with Arbitration
Ptr<const Packet>
PerStaWifiMacQueue::PeekFirstAvailable (WifiMacHeader *hdr, Time &timestamp,
                                  const QosBlockedDestinations *blockedPackets)
{
  Cleanup ();
  //NS_ASSERT_MSG(m_perStaQInfo,"PerStaQInfoContainer not initialized!");
  Ptr<const Packet> packet = 0;
  PacketQueueI it;
  bool found = false;
  switch (m_servicePolicy)
  {
    case FCFS:
      found = PeekFcfs(it, blockedPackets);
      break;
    case EDF:
      found = PeekEdf(it,blockedPackets);
      break;
    case EDF_RR:
      found = PeekEdfRoundRobin(it,blockedPackets);
      break;
    case MAX_REMAINING_TIME_ALLOWANCE:
      found = PeekMaxRemainingTimeAllowance(it,blockedPackets);
      break;
    case PER_BITRATE_TIME_ALLOWANCE_RR:
      found = PeekPerBitrateTimeAllowanceRoundRobin(it,blockedPackets);
      break;
    case MAX_QUEUE_SURPLUS:
      found = PeekMaxQueueSurplus(it,blockedPackets);
      break;
      /*sva-design: add for appropriate service policy ?
    case ?:
      found = Peek?(it,blockedPackets);
      break;
      sva-design*/
    default: NS_FATAL_ERROR("Unrecongnized Queue Arbitration Algorithm : " << m_servicePolicy);
  }
  if (found)
    {
      *hdr = it->hdr;
      timestamp = it->tstamp;
      packet = it->packet;
    }
  return packet;
}

//TODO sva: can be optimized
uint32_t
PerStaWifiMacQueue::GetNPacketsByTidAndAddress (uint8_t tid, WifiMacHeader::AddressType type,
                                          Mac48Address addr)
{
  Cleanup ();
  uint32_t nPackets = 0;
  if (!m_queue.empty ())
    {
      PacketQueueI it;
      for (it = m_queue.begin (); it != m_queue.end (); it++)
        {
          if (GetAddressForPacket (type, it) == addr)
            {
              if (it->hdr.IsQosData () && it->hdr.GetQosTid () == tid)
                {
                  nPackets++;
                }
            }
        }
    }
  return nPackets;
}

bool
PerStaWifiMacQueue::IsEmpty (void)
{
  Cleanup ();
  return m_queue.empty ();
}

//TODO sva: what is the justification for handling service interval scheduling at the WifiMacQueue? Can't it be done at the Aggregator?
void
PerStaWifiMacQueue::PendingServiceInterval (void)
{
  m_pendingServiceIntervalStart = Simulator::Now();
  m_serviceIntervalPending = true;
  if (m_mpduAggregator)//if an aggregator is initialized (for stations there isn't one currently)
    {
      //check if aggregator has finished serving the current service interval's quota
      m_mpduAggregator->PendingServiceInterval();
    }
}

void
PerStaWifiMacQueue::BeginServiceInterval (void)
{
  m_currentServiceIntervalStart = Simulator::Now();
  m_serviceIntervalPending = false;
  Simulator::Schedule(Seconds(m_serviceInterval), &PerStaWifiMacQueue::PendingServiceInterval, this);
#ifdef SVA_DEBUG
  std::cout << Simulator::Now().GetSeconds() << " PerStaWifiMacQueue::BeginServiceInterval service interval was pending since "
      << m_pendingServiceIntervalStart.GetSeconds() << "\n";
#endif
}

bool
PerStaWifiMacQueue::PeekFcfs (PacketQueueI &it, const QosBlockedDestinations *blockedPackets)
{
  for (PacketQueueI i = m_queue.begin (); i != m_queue.end (); i++)
    {
      if (!i->hdr.IsQosData ()
          || !blockedPackets->IsBlocked (i->hdr.GetAddr1 (), i->hdr.GetQosTid ()))
        {
          it = i;
#ifdef SVA_DEBUG_EDFBUG
          std::cout << Simulator::Now().GetSeconds() << " PeekFcfs: STA "
              << it->hdr.GetAddr1() << " selected\n";// with deadline " << earliestDeadline.GetSeconds() << "\n";
#endif
          return true;
        }
    }
  return false;
}


bool
PerStaWifiMacQueue::PeekEdf (PacketQueueI &it, const QosBlockedDestinations *blockedPackets)
{
  PerStaQInfoContainer::Iterator sta;
  PacketQueueI qi;
  PacketQueueI qiServed; //the one that will eventually be served
  Time earliestDeadline=Simulator::Now()+Seconds(100); //set to some distant future time
  TimestampTag deadline;
  bool found=false;

  if (m_perStaQInfo)//only if PerStaQInfo is supported on this queue
    {
      for (sta = m_perStaQInfo->Begin(); sta != m_perStaQInfo->End(); ++sta)
        {
          if (GetStaHol(qi,(*sta)->GetTid(),(*sta)->GetMac(),blockedPackets))
            {
              NS_ASSERT_MSG(qi->packet->FindFirstMatchingByteTag(deadline),"Did not find TimestampTag in packet!");
              if (deadline.GetTimestamp() < earliestDeadline)
                { //update selected STA for service
                  qiServed = qi;
                  earliestDeadline = deadline.GetTimestamp();
                  found = true;
#ifdef SVA_DEBUG_EDFBUG
  std::cout << Simulator::Now().GetSeconds() << " @PeekEdf cur sta= " << (*sta)->GetMac() << " Deadline= " <<  earliestDeadline.GetSeconds() << "\n";
#endif
                }
            }
        }
      if (found)
        {
          it = qiServed;
          return true;
        }
    }
  //if no packet found based on EDF or perStaQInfo not supported then resort to FCFS scheduling
  //I'm not sure if (no packet found based on EDF) happens.
  //May happen when there are only non-QoS packets in the queue
  found = PeekFcfs(qiServed,blockedPackets);
  if (found)
    {
      it = qiServed;
#ifdef SVA_DEBUG_EDFBUG
          std::cout << Simulator::Now().GetSeconds() << " PeekEdf: STA "
              << it->hdr.GetAddr1() << " selected with deadline " << earliestDeadline.GetSeconds() << "\n";
#endif
      return true;
    }
  return false;
}

bool
PerStaWifiMacQueue::PeekEdfRoundRobin (PacketQueueI &it, const QosBlockedDestinations *blockedPackets)
{
  PerStaQInfoContainer::Iterator sta;
  PacketQueueI qi;
  PacketQueueI qiServed; //the one that will eventually be served
  Time earliestDeadline=Simulator::Now()+Seconds(100); //set to some distant future time
  Time targetDeadline=Simulator::Now()+Seconds(m_serviceInterval); //approximate beginning of next service interval
  TimestampTag deadline;
  bool found=false;

  if (m_perStaQInfo)//only if PerStaQInfo is supported on this queue
    {
      for (sta = m_perStaQInfo->Begin(); sta != m_perStaQInfo->End(); ++sta)
        {
          if (GetStaHol(qi,(*sta)->GetTid(),(*sta)->GetMac(),blockedPackets))
            {
              NS_ASSERT_MSG(qi->packet->FindFirstMatchingByteTag(deadline),"Did not find TimestampTag in packet!");
              if (deadline.GetTimestamp() < earliestDeadline)
                { //update selected STA for service
                  qiServed = qi;
                  earliestDeadline = deadline.GetTimestamp();
                  if (earliestDeadline <= targetDeadline)
                    {
                      found = true;
                    }
                }
            }
        }
      if (found)
        {
          it = qiServed;
#ifdef SVA_DEBUG_DETAIL
          std::cout << Simulator::Now().GetSeconds() << " PeekEdfRoundRobin: STA "
              << it->hdr.GetAddr1() << " selected with deadline " << earliestDeadline.GetSeconds() << "\n";
#endif
          return true;
        }
    }
  //if perStaQInfo not supported then resort to FCFS scheduling
  else
    {
      found = PeekFcfs(qiServed,blockedPackets);
      if (found)
        {
          it = qiServed;
          return true;
        }
    }
  //otherwise if no packet found then don't service anything.
  //TODO: I am not sure if this will cause a problem.
  return false;
}

bool
PerStaWifiMacQueue::PeekMaxRemainingTimeAllowance (PacketQueueI &it, const QosBlockedDestinations *blockedPackets)
{
  PerStaQInfoContainer::Iterator sta;
  PacketQueueI qi;
  PacketQueueI qiServed; //the one that will eventually be served
  Time maxRta = Seconds(0); //Max Remaining Time Allowance
  Time rta;
  bool found=false;

  if (m_perStaQInfo)//only if PerStaQInfo is supported on this queue
    {
      for (sta = m_perStaQInfo->Begin(); sta != m_perStaQInfo->End(); ++sta)
        {
          if (GetStaHol(qi,(*sta)->GetTid(),(*sta)->GetMac(),blockedPackets))
            {
              rta = (*sta)->GetRemainingTimeAllowance();//TODO: support for time allowance should be moved from PerStaQInfo to the specific type of aggregator or just put in its own separate class
              if (rta > maxRta)
                { //update selected STA for service
                  qiServed = qi;
                  maxRta = rta;
                  found = true;
                }
            }
        }
      if (found)
        {
          it = qiServed;
#ifdef SVA_DEBUG_DETAIL
          std::cout << Simulator::Now().GetSeconds() << " PeekMaxRemainingTimeAllowance: STA "
              << it->hdr.GetAddr1() << " selected with RTA " << maxRta.GetSeconds()*1000 << " msec \n";
#endif
          return true;
        }
    }
  //if perStaQInfo not supported then resort to FCFS scheduling
  else
    {
      found = PeekFcfs(qiServed,blockedPackets);
      if (found)
        {
          it = qiServed;
          return true;
        }
    }
  //otherwise if no packet found then don't service anything.
  //TODO: This causes a problem as it will sometimes result in an unbounded pending service interval. Should call pending service interval if this happens
  if (m_mpduAggregator)
    {
      if (m_mpduAggregator->IsPendingServiceInterval())
        {
          Simulator::ScheduleNow(&MpduUniversalAggregator::PendingServiceInterval, m_mpduAggregator); //make sure current line of execution is finished

          #ifdef SVA_DEBUG_DETAIL
          std::cout << Simulator::Now().GetSeconds() << " PerStaWifiMacQueue::PeekMaxRemainingTimeAllowance pending service interval, CAN NOT PEEK QUEUE so re-schedule PendingServiceInterval \n";
          #endif
        }
    }
  return false;
}


bool
PerStaWifiMacQueue::PeekPerBitrateTimeAllowanceRoundRobin (PacketQueueI &it, const QosBlockedDestinations *blockedPackets)
{//TODO
  PerStaQInfoContainer::Iterator sta;
  PacketQueueI qi;
  PacketQueueI qiServed; //the one that will eventually be served
  Time rta;
  bool found=false;
  Ptr<PerBitrateTimeAllowance> ta;
  uint64_t bitrate=0;

  if (m_perStaQInfo)//only if PerStaQInfo is supported on this queue
    {
      ++m_lastServed;
      if (m_lastServed == m_perStaQInfo->End())
        m_lastServed = m_perStaQInfo->Begin();
      for (sta = m_lastServed; sta != m_perStaQInfo->End(); ++sta)
        {
          ta = (*sta)->GetObject<PerBitrateTimeAllowance>();
          if (GetStaHol(qi,(*sta)->GetTid(),(*sta)->GetMac(),blockedPackets))
            {
              WifiTxVector dataTxVector = m_low->GetDataTxVector (qi->packet, &(qi->hdr));
              bitrate = dataTxVector.GetMode().GetDataRate();
              rta = ta->GetRemainingTimeAllowance(bitrate);
              if (rta > Seconds(0))
                { //update selected STA for service
                  qiServed = qi;
                  m_lastServed = sta;
                  found = true;
                  break;
                }
            }
        }
      if (!found) //continue round robin search
        {
          for (sta = m_perStaQInfo->Begin(); sta != m_lastServed; ++sta)
            {
              ta = (*sta)->GetObject<PerBitrateTimeAllowance>();
              if (GetStaHol(qi,(*sta)->GetTid(),(*sta)->GetMac(),blockedPackets))
                {
                  WifiTxVector dataTxVector = m_low->GetDataTxVector (qi->packet, &(qi->hdr));
                  bitrate = dataTxVector.GetMode().GetDataRate();
                  rta = ta->GetRemainingTimeAllowance(bitrate);
                  if (rta > Seconds(0))
                    { //update selected STA for service
                      qiServed = qi;
                      m_lastServed = sta;
                      found = true;
                      break;
                    }
                }
            }
        }
      if (found)
        {
          it = qiServed;
#ifdef SVA_DEBUG_DETAIL
          std::cout << Simulator::Now().GetSeconds() << " PeekPerBitrateTimeAllowance: STA "
              << it->hdr.GetAddr1() << " selected with RTA " << rta.GetSeconds()*1000 <<
              " msec, bitrate " << (double)bitrate/1e6 << " Mbps\n";
#endif
          return true;
        }
    }
  //if perStaQInfo not supported then resort to FCFS scheduling
  else
    {
      found = PeekFcfs(qiServed,blockedPackets);
      if (found)
        {
          it = qiServed;
          return true;
        }
    }
  //otherwise if no packet found then don't service anything.
  //TODO: This causes a problem as it will sometimes result in an unbounded pending service interval. Should call pending service interval if this happens
  if (m_mpduAggregator)
    {
      if (m_mpduAggregator->IsPendingServiceInterval())
        {//this condition should never become true!!
          Simulator::ScheduleNow(&MpduUniversalAggregator::PendingServiceInterval, m_mpduAggregator); //make sure current line of execution is finished

          #ifdef SVA_DEBUG_DETAIL
          std::cout << Simulator::Now().GetSeconds() << " PerStaWifiMacQueue::PeekPerBitrateTimeAllowance pending service interval, CAN NOT PEEK QUEUE so re-schedule PendingServiceInterval \n";
          #endif
        }
    }
  return false;
}

bool
PerStaWifiMacQueue::PeekMaxQueueSurplus (PacketQueueI &it, const QosBlockedDestinations *blockedPackets)
{
  PerStaQInfoContainer::Iterator sta;
  PacketQueueI qi;
  PacketQueueI qiServed; //the one that will eventually be served
  double maxQSurplus = 1600; //sva: was =0; //Max queue surplus
  double qSurplus;
  bool found=false;

  if (m_perStaQInfo)//only if PerStaQInfo is supported on this queue
    {
      for (sta = m_perStaQInfo->Begin(); sta != m_perStaQInfo->End(); ++sta)
        {
          if (GetStaHol(qi,(*sta)->GetTid(),(*sta)->GetMac(),blockedPackets))
            {
              Ptr<SimpleController> simpleCtrl = (m_mpduAggregator->GetAggregationController()->GetObject<QueueSurplusAggregationController>())->GetController((*sta)->GetMac())->GetObject<SimpleController>();
              simpleCtrl->SetInputSignal(SimpleController::InSigType ((double)(*sta)->GetSizeBytes(), (*sta)->GetAvgSizeBytes(), (*sta)->GetPrEmpty()));
              qSurplus = simpleCtrl->ComputeOutput();

              if (qSurplus > maxQSurplus)
                { //update selected STA for service
                  qiServed = qi;
                  maxQSurplus = qSurplus;
                  found = true;
                }
            }
        }
      if (found)
        {
          it = qiServed;
#ifdef SVA_DEBUG
          std::cout << Simulator::Now().GetSeconds() << " PeekMaxQueueSurplus: STA "
              << it->hdr.GetAddr1() << " selected with surplus " << maxQSurplus/1e6 << " MBytes \n";
#endif
          return true;
        }
    }
  //if perStaQInfo not supported then resort to FCFS scheduling
  else
    {
      found = PeekFcfs(qiServed,blockedPackets);
      if (found)
        {
          it = qiServed;
          return true;
        }
    }
  //otherwise if no packet found then don't service anything.
  //TODO: This causes a problem as it will sometimes result in an unbounded pending service interval. Should call pending service interval if this happens
  if (m_mpduAggregator)
    {
      if (m_mpduAggregator->IsPendingServiceInterval())
        {
          Simulator::ScheduleNow(&MpduUniversalAggregator::PendingServiceInterval, m_mpduAggregator); //make sure current line of execution is finished

          #ifdef SVA_DEBUG
          std::cout << Simulator::Now().GetSeconds() << " PerStaWifiMacQueue::PeekMaxQueueSurplus pending service interval, CAN NOT PEEK QUEUE so re-schedule PendingServiceInterval \n";
          #endif
        }
    }
  return false;
}


} // namespace ns3
