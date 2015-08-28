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
 * Iran University of Science & Technology
 */

#include <vector>
//#include <deque>
#include <utility>
#include "ns3/packet.h"
#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/udp-client.h"
#include "ns3/wifi-net-device.h"
#include "ns3/per-sta-q-info.h"
#include "per-sta-aggregation-helper.h"
#include "ns3/ap-wifi-mac.h"

namespace ns3 {

  PerStaAggregationHelper::PerStaAggregationHelper (uint8_t nSta)
  : m_nSta (nSta)
  {
  }

  PerStaAggregationHelper::~PerStaAggregationHelper ()
  {
  }

  PerStaQInfoContainer
  PerStaAggregationHelper::InstallPerStaQInfo (const NetDeviceContainer sta, NetDeviceContainer apDevice, uint8_t ac)
  {
    NS_ASSERT_MSG(sta.GetN()!=0,"No Stations Initialized.");
    PerStaQInfoContainer c;
    Ptr<NetDevice> device;
    Ptr<WifiNetDevice> staDevice;
    for (NetDeviceContainer::Iterator i=sta.Begin(); i != sta.End(); ++i)
      {
        device = *i;
        staDevice = device->GetObject<WifiNetDevice>();//sva: safe alternative to dynamic down-casting if aggregation is supported on Object
        c.Add(staDevice);
      }
    apDevice.Get(0)->GetObject<WifiNetDevice>()->GetMac()->GetObject<ApWifiMac>()->SetPerStaQInfo(c,ac);
    return c;
  }

  Ptr<BssPhyMacStats>
  PerStaAggregationHelper::InstallBssPhyMacStats (uint32_t hist, PerStaQInfoContainer &c)
  {
    std::ostringstream path;
    path << "/NodeList/"<< m_nSta << "/DeviceList/0/$ns3::WifiNetDevice/Phy/State";
    Ptr<BssPhyMacStats> bssPhyMacStats = CreateObject<BssPhyMacStats> (path.str());
    bssPhyMacStats->SetAttribute("HistorySize",UintegerValue(hist)); //keep history of the last hist beacons (i.e. one seconds)
    NS_ASSERT(bssPhyMacStats->SetPerStaQInfo(&c));
    return bssPhyMacStats;
  }

  void
  PerStaAggregationHelper::SetPerStaWifiMacQueue (std::string n0, const AttributeValue &v0,
                                                  std::string n1, const AttributeValue &v1,
                                                  std::string n2, const AttributeValue &v2,
                                                  std::string n3, const AttributeValue &v3)
  {
    m_queue->SetAttribute (n0, v0);
    m_queue->SetAttribute (n1, v1);
    m_queue->SetAttribute (n2, v2);
    m_queue->SetAttribute (n3, v3);
  }

  void
  PerStaAggregationHelper::SetMpduUniversalAggregator (std::string n0, const AttributeValue &v0,
                                                       std::string n1, const AttributeValue &v1,
                                                       std::string n2, const AttributeValue &v2)
  {
    m_aggregator->SetAttribute (n0, v0);
    m_aggregator->SetAttribute (n1, v1);
    m_aggregator->SetAttribute (n2, v2);
  }

  void
  PerStaAggregationHelper::SetAggregationController (std::string n0, const AttributeValue &v0,
                                                     std::string n1, const AttributeValue &v1,
                                                     std::string n2, const AttributeValue &v2,
                                                     std::string n3, const AttributeValue &v3,
                                                     std::string n4, const AttributeValue &v4,
                                                     std::string n5, const AttributeValue &v5,
                                                     std::string n6, const AttributeValue &v6,
                                                     std::string n7, const AttributeValue &v7,
                                                     std::string n8, const AttributeValue &v8,
                                                     std::string n9, const AttributeValue &v9,
                                                     std::string n10, const AttributeValue &v10)
  {
    m_aggCtrl->SetAttribute (n1, v1);
    m_aggCtrl->SetAttribute (n2, v2);
    m_aggCtrl->SetAttribute (n3, v3);
    m_aggCtrl->SetAttribute (n4, v4);
    m_aggCtrl->SetAttribute (n5, v5);
    m_aggCtrl->SetAttribute (n6, v6);
    m_aggCtrl->SetAttribute (n7, v7);
    m_aggCtrl->SetAttribute (n8, v8);
    m_aggCtrl->SetAttribute (n9, v9);
    m_aggCtrl->SetAttribute (n10, v10);
  }


} //name space ns3


