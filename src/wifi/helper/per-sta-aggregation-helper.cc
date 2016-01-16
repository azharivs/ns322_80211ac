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
#include "ns3/simulator.h"
#include "ns3/double.h"

namespace ns3 {

  PerStaAggregationHelper::PerStaAggregationHelper (Ptr<NetDevice> ap, uint8_t nSta, uint8_t ac, AggregationType aggAlg)
  : m_nSta (nSta)
  {
    Ptr<ApWifiMac> apWifiMac = ap->GetObject<WifiNetDevice>()->GetMac()->GetObject<ApWifiMac>();
    NS_ASSERT(apWifiMac);
    Ptr<EdcaTxopN> edca;

    switch (ac)
    {
      case AC_VO:
        edca = apWifiMac->GetVOQueue();//TODO I had to make these methods public in RegularWifiMac. Is there another way to do this?
        break;
      case AC_VI:
        edca = apWifiMac->GetVIQueue();
        break;
      case AC_BE:
        edca = apWifiMac->GetBEQueue();
        break;
      case AC_BK:
        edca = apWifiMac->GetBKQueue();
        break;
      default:
        NS_ASSERT_MSG(0,"Invalid AC type used: " << ac);
    }

    NS_ASSERT(edca);
    m_low = edca->Low();
    m_queue = edca->GetEdcaQueue()->GetObject<PerStaWifiMacQueue>(); //set pointer to queue
    m_aggregator = edca->Low()->GetMpduAggregator()->GetObject<MpduUniversalAggregator>(); //set pointer to aggregator
    switch (aggAlg)
    {
      case TIME_ALLOWANCE:
      case PER_BITRATE_TIME_ALLOWANCE:
        m_aggCtrl = m_aggregator->GetAggregationController()->GetObject<TimeAllowanceAggregationController>(); //set pointer to aggregation controller
        NS_ASSERT(m_aggCtrl);
        std::cout << "PerStaAggregationHelper --> TimeAllowanceAggregationController\n";//sva for debug
        break;
      case QUEUE_SURPLUS:
        m_aggCtrl = m_aggregator->GetAggregationController()->GetObject<QueueSurplusAggregationController>(); //set pointer to aggregation controller
        NS_ASSERT(m_aggCtrl);
        std::cout << "PerStaAggregationHelper --> QueueSurplusAggregationController\n";//sva for debug
        break;
      default:
        break;
    }
    NS_ASSERT(m_queue);
    NS_ASSERT(m_aggregator);
    //NS_ASSERT(m_aggCtrl);
    m_queue->SetMpduAggregator(m_aggregator); //set pointer to aggregator
    m_queue->SetMacLow(edca->Low()); //set pointer to MacLow required for getting current bitrate, etc for aggregation related service policies
  }

  PerStaAggregationHelper::~PerStaAggregationHelper ()
  {
  }

  PerStaQInfoContainer
  PerStaAggregationHelper::InstallPerStaQInfo (const NetDeviceContainer sta, NetDeviceContainer apDevice, uint8_t ac, uint32_t hist, uint32_t largeHist)
  {
    NS_ASSERT_MSG(sta.GetN()!=0,"No Stations Initialized.");
    PerStaQInfoContainer c;
    Ptr<NetDevice> device;
    Ptr<WifiNetDevice> staDevice;
    Ptr<PerStaQInfo> qInfo;
    for (NetDeviceContainer::Iterator i=sta.Begin(); i != sta.End(); ++i)
      {
        device = *i;
        staDevice = device->GetObject<WifiNetDevice>();//sva: safe alternative to dynamic down-casting if aggregation is supported on Object
        qInfo = c.Add(staDevice);
        qInfo->SetAttribute("HistorySize",UintegerValue(hist));
        qInfo->SetAttribute("LargeHistorySize",UintegerValue(largeHist));
      }
    m_queue->EnablePerStaQInfo(c); //simply initializes a member pointer to point to this container
    //sva: old code: apDevice.Get(0)->GetObject<WifiNetDevice>()->GetMac()->GetObject<ApWifiMac>()->SetPerStaQInfo(c,ac);//TODO remove from ApWifiMac and do in helper
    return c;
  }

  Ptr<BssPhyMacStats>
  PerStaAggregationHelper::InstallBssPhyMacStats (uint32_t hist, PerStaQInfoContainer &c)
  {
    std::ostringstream path;
    path << "/NodeList/"<< (int) m_nSta << "/DeviceList/0/$ns3::WifiNetDevice/Phy/State";
    Ptr<BssPhyMacStats> bssPhyMacStats = CreateObject<BssPhyMacStats> (path.str());
    bssPhyMacStats->SetAttribute("HistorySize",UintegerValue(hist)); //keep history of the last hist beacons (i.e. one seconds)
    NS_ASSERT(bssPhyMacStats->SetPerStaQInfo(&c));
    return bssPhyMacStats;
  }

  void
  PerStaAggregationHelper::InstallSourceRateAdaptor (const PerStaQInfoContainer &staQInfo, const ApplicationContainer &clientApps, double interval)
  {//TODO
    NS_ASSERT_MSG(staQInfo.GetN()!=0,"No PerStaQInfo Initialized.");
    NS_ASSERT_MSG(staQInfo.GetN() == clientApps.GetN(),"Mismatch between number of PerStaQInfo and Application Container Entries.");
    NS_ASSERT(m_aggCtrl);

    ApplicationContainer::Iterator cit = clientApps.Begin();
    for (PerStaQInfoContainer::Iterator it = staQInfo.Begin(); it != staQInfo.End(); ++it)
      {//assuming order of stations and clients in both containers correspond to each other.
        Ptr<CbrRateAdapt> sra = CreateObject<CbrRateAdapt>();
        sra->SetApplication(*cit);
        sra->SetStaQ(*it);
        sra->SetInterval(interval);
        sra->DoInit();
        (*cit)->AggregateObject(sra);
        (*it)->AggregateObject(sra);
        (*it)->SetController(m_aggCtrl->GetController((*it)->GetMac()) );//TODO bad way of relating controller to per sta q info
        (*it)->SetAttribute("ObservationInterval",DoubleValue(interval));
        Simulator::Schedule(Seconds(interval), &CbrRateAdapt::UpdateSourceRate, sra);
        ++cit;
      }
  }

  void
  PerStaAggregationHelper::FinalizeSetup (PerStaQInfoContainer &c)
  {
    m_aggregator->EnablePerStaQInfo (c,m_queue,m_low,m_low->GetPhy());
  }

  void
  PerStaAggregationHelper::SetPerStaWifiMacQueue (std::string n0, const AttributeValue &v0,
                                                  std::string n1, const AttributeValue &v1,
                                                  std::string n2, const AttributeValue &v2,
                                                  std::string n3, const AttributeValue &v3,
                                                  std::string n4, const AttributeValue &v4)
  {
    if (n0 != "") m_queue->SetAttribute (n0, v0);
    if (n1 != "") m_queue->SetAttribute (n1, v1);
    if (n2 != "") m_queue->SetAttribute (n2, v2);
    if (n3 != "") m_queue->SetAttribute (n3, v3);
    if (n4 != "") m_queue->SetAttribute (n4, v4);
  }

  void
  PerStaAggregationHelper::SetMpduUniversalAggregator (std::string n0, const AttributeValue &v0,
                                                       std::string n1, const AttributeValue &v1,
                                                       std::string n2, const AttributeValue &v2)
  {
    if (n0 != "") m_aggregator->SetAttribute (n0, v0);
    if (n1 != "") m_aggregator->SetAttribute (n1, v1);
    if (n2 != "") m_aggregator->SetAttribute (n2, v2);
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
                                                     std::string n10, const AttributeValue &v10,
                                                     std::string n11, const AttributeValue &v11)
  {
    if (n0 != "") m_aggCtrl->SetAttribute (n0, v0);
    if (n1 != "") m_aggCtrl->SetAttribute (n1, v1);
    if (n2 != "") m_aggCtrl->SetAttribute (n2, v2);
    if (n3 != "") m_aggCtrl->SetAttribute (n3, v3);
    if (n4 != "") m_aggCtrl->SetAttribute (n4, v4);
    if (n5 != "") m_aggCtrl->SetAttribute (n5, v5);
    if (n6 != "") m_aggCtrl->SetAttribute (n6, v6);
    if (n7 != "") m_aggCtrl->SetAttribute (n7, v7);
    if (n8 != "") m_aggCtrl->SetAttribute (n8, v8);
    if (n9 != "") m_aggCtrl->SetAttribute (n9, v9);
    if (n10 != "") m_aggCtrl->SetAttribute (n10, v10);
    if (n11 != "") m_aggCtrl->SetAttribute (n11, v11);
  }


} //name space ns3


