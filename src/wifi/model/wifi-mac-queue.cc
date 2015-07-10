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
 */

#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"

#include "wifi-mac-queue.h"
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
                   UintegerValue (400),
                   MakeUintegerAccessor (&WifiMacQueue::m_maxSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("MaxDelay", "If a packet stays longer than this delay in the queue, it is dropped.",
                   TimeValue (Seconds (10.0)),
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
 *
 */

NS_OBJECT_ENSURE_REGISTERED (PerStaWifiMacQueue);

TypeId
PerStaWifiMacQueue::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PerStaWifiMacQueue")
    .SetParent<WifiMacQueue> ()
    .AddConstructor<PerStaWifiMacQueue> ()
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
  if (c.IsEmpty()) return false;
  m_perStaQInfo = &c;
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
  NS_ASSERT_MSG(m_perStaQInfo,"PerStaQInfoContainer not initialized!");
  m_perStaQInfo->Arrival(packet, hdr, now);//sva: deal with PerStaQInfo issues
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
  NS_ASSERT_MSG(m_perStaQInfo,"PerStaQInfoContainer not initialized!");
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
          m_perStaQInfo->Departure(i->packet,i->hdr,now);
          i = m_queue.erase (i);
          n++;
        }
    }
  m_size -= n;
}

Ptr<const Packet>
PerStaWifiMacQueue::Peek (WifiMacHeader *hdr)
{
  NS_ASSERT_MSG(m_perStaQInfo,"PerStaQInfoContainer not initialized!");
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
      NS_ASSERT_MSG(m_perStaQInfo,"PerStaQInfoContainer not initialized!");
      m_perStaQInfo->Departure(i.packet,i.hdr,now);//sva: deal with PerStaQInfo issues
      m_queue.pop_front ();
      m_size--;
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
  NS_ASSERT_MSG(m_perStaQInfo,"PerStaQInfoContainer not initialized!");
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
                  m_perStaQInfo->Departure(it->packet,it->hdr,now);//sva: deal with PerStaQInfo issues
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
  NS_ASSERT_MSG(m_perStaQInfo,"PerStaQInfoContainer not initialized!");
  m_perStaQInfo->Reset();//sva: deal with PerStaQInfo issues
  m_queue.erase (m_queue.begin (), m_queue.end ());
  m_size = 0;
}


bool
PerStaWifiMacQueue::Remove (Ptr<const Packet> packet)
{
  Time now = Simulator::Now();
  NS_ASSERT_MSG(m_perStaQInfo,"PerStaQInfoContainer not initialized!");
  PacketQueueI it = m_queue.begin ();
  for (; it != m_queue.end (); it++)
    {
      if (it->packet == packet)
        {
          m_perStaQInfo->Departure(it->packet,it->hdr,now);//sva: deal with PerStaQInfo issues
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
  NS_ASSERT_MSG(m_perStaQInfo,"PerStaQInfoContainer not initialized!");
  Cleanup ();
  if (m_size == m_maxSize)
    {
      return;
    }
  Time now = Simulator::Now ();
  m_queue.push_front (Item (packet, hdr, now));
  m_size++;
  m_perStaQInfo->Arrival(packet, hdr, now);//sva: deal with PerStaQInfo issues
}


Ptr<const Packet>
PerStaWifiMacQueue::DequeueFirstAvailable (WifiMacHeader *hdr, Time &timestamp,
                                     const QosBlockedDestinations *blockedPackets)
{
  //std::cout << "WifiMacQueue::DequeueFirstAvailable \n";
  Time now = Simulator::Now();
  NS_ASSERT_MSG(m_perStaQInfo,"PerStaQInfoContainer not initialized!");
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
          m_perStaQInfo->Departure(it->packet,it->hdr,now);//sva: deal with PerStaQInfo issues
          m_queue.erase (it);
          m_size--;
          return packet;
        }
    }
  return packet;
}

Ptr<const Packet>
PerStaWifiMacQueue::PeekFirstAvailable (WifiMacHeader *hdr, Time &timestamp,
                                  const QosBlockedDestinations *blockedPackets)
{
  //std::cout << "WifiMacQueue::PeekFirstAvailable \n";
  Cleanup ();
  NS_ASSERT_MSG(m_perStaQInfo,"PerStaQInfoContainer not initialized!");
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

} // namespace ns3
