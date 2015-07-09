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
//#include <deque>
#include <utility>
#include "ns3/packet.h"
#include "ns3/nstime.h"
#include "ns3/object.h"
//#include "wifi-mac-header.h"
//#include "wifi-mac-queue.h"
#include "per-sta-q-info-container.h"

namespace ns3 {

PerStaQInfoContainer::PerStaQInfoContainer()
{
}

PerStaQInfoContainer::Iterator
PerStaQInfoContainer::Begin() const
{
  return m_staQInfo.begin();
}

PerStaQInfoContainer::Iterator
PerStaQInfoContainer::End() const
{
  return m_staQInfo.end();
}

uint32_t
PerStaQInfoContainer::GetN (void) const
{
  return m_staQInfo.size();
}

Ptr<PerStaQInfo>
PerStaQInfoContainer::Get (uint32_t i) const
{
  return m_staQInfo.at(i);
}


Ptr<PerStaQInfo>
PerStaQInfoContainer::GetByMac (Mac48Address addr, uint8_t tid) const
{//TODO
  return *m_staQInfo.begin();
}

Ptr<PerStaQInfo>
PerStaQInfoContainer::GetByIpv4 (Ipv4Address addr, uint8_t tid) const
{//TODO
  return *m_staQInfo.begin();
}

PerStaQInfoContainer
PerStaQInfoContainer::Add (Ptr<WifiNetDevice> sta, Ptr<WifiNetDevice> ap)
{//TODO
  return *this;
}

void
PerStaQInfoContainer::Arrival (Ptr<const Packet> packet, const WifiMacHeader &hdr, Time tstamp)
{//TODO
  return;
}

void
PerStaQInfoContainer::Departure (Ptr<const Packet> packet, const WifiMacHeader &hdr, Time tstamp)
{//TODO
  return;
}

void
PerStaQInfoContainer::Reset (void)
{//TODO
  return;
}

bool
PerStaQInfoContainer::IsEmpty (void) const
{
  return (m_staQInfo.empty());
}

} // namespace ns3


