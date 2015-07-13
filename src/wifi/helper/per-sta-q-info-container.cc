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
#include "ns3/simulator.h"
//#include "wifi-mac-header.h"
//#include "wifi-mac-queue.h"
#include "per-sta-q-info-container.h"
#include "ns3/qos-tag.h"
#include "ns3/regular-wifi-mac.h"

namespace ns3 {

PerStaQInfoContainer::PerStaQInfoContainer()
{
#ifdef SVA_DEBUG
  m_cnt = 0;
#endif
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
{
  Ptr<PerStaQInfo> qInfo = 0;
  uint8_t flag = 0;
  for (Iterator i = m_staQInfo.begin(); i != m_staQInfo.end(); ++i)
    {
      qInfo = *i;
      if ( qInfo->GetMac() == addr && qInfo->GetTid() == tid)
        {
          flag = 1;
          break;
        }
    }
  if (flag)
    {
      return qInfo;
    }
  else
    {
      return NULL;
    }
}

Ptr<PerStaQInfo>
PerStaQInfoContainer::GetByIpv4 (Ipv4Address addr, uint8_t tid) const
{//TODO : Not implemented
  NS_ASSERT(true);//Not implemented yet: throw exception if execution reaches this point
  return *m_staQInfo.begin();
}

void
PerStaQInfoContainer::Add (Ptr<WifiNetDevice> sta)//, Ptr<WifiNetDevice> ap)
{
  NS_ASSERT_MSG(sta,"Station Pointer NULL!");
  Ptr<PerStaQInfo> qInfo = CreateObject<PerStaQInfo>();
  //Ptr<WifiMac> wifiMac = sta->GetMac();
  //Ptr<RegularWifiMac> mac =  sta->GetMac()->GetObject<RegularWifiMac>();
  Mac48Address addrs = sta->GetMac()->GetObject<RegularWifiMac>()->GetAddress();
  qInfo->SetMac(addrs);
  qInfo->SetTid(UP_VI); //TODO sva: default TID should change later for generalization
  m_staQInfo.push_back(qInfo);
}

void
PerStaQInfoContainer::Arrival (Ptr<const Packet> packet, const WifiMacHeader &hdr, Time tstamp)
{
  if (!hdr.IsData()) //don't count non data packets
    {
#ifdef SVA_DEBUG
      std::cout << " Arrival NON DATA   " << m_cnt << "   NON DATA \n";
#endif
      return ;
    }
  //get which queue to use based on destination MAC and TID
  Ptr<PerStaQInfo> qInfo = GetByMac(hdr.GetAddr1(),hdr.GetQosTid());
  NS_ASSERT(qInfo); //make sure is not NULL
  qInfo->Arrival(packet->GetSize(), tstamp);
#ifdef SVA_DEBUG
  m_cnt ++;
  std::cout << " Arrival XXXXXXXXX   " << m_cnt << "   XXXXXXXXXXX \n";
#endif
}

void
PerStaQInfoContainer::Departure (Ptr<const Packet> packet, const WifiMacHeader &hdr, Time tstamp)
{
  if (!hdr.IsData()) //don't count non data packets
    {
#ifdef SVA_DEBUG
      std::cout << " Departure NON DATA   " << m_cnt << "   NON DATA \n";
#endif
      return ;
    }
  Time now = Simulator::Now();
  //get which queue to use based on destination MAC and TID
  Ptr<PerStaQInfo> qInfo = GetByMac(hdr.GetAddr1(),hdr.GetQosTid());
  NS_ASSERT(qInfo); //make sure is not NULL
  TimestampTag deadline;
  bool ok = packet->FindFirstMatchingByteTag(deadline);
  NS_ASSERT_MSG(ok,"Did not find TimestampTag in packet!");
  qInfo->Departure(packet->GetSize(), now - tstamp, deadline.GetTimestamp());
#ifdef SVA_DEBUG
  m_cnt --;
  std::cout << " Departure XXXXXXXXX   " << m_cnt << "   XXXXXXXXXXX \n";
#endif
}

void
PerStaQInfoContainer::Reset (void)
{
#ifdef SVA_DEBUG
  std::cout << "PerStaQInfoContainer::Reset \n";
#endif
  for (Iterator i = m_staQInfo.begin(); i != m_staQInfo.end(); ++i)
    {
      (*i)->Reset(); //reset i-th PerStaQInfo element
    }
}

bool
PerStaQInfoContainer::IsEmpty (void) const
{
  bool ret=m_staQInfo.empty();
  return (ret);
}

} // namespace ns3


