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
#ifndef PER_STA_AGGREGATION_HELPER_H
#define PER_STA_AGGREGATION_HELPER_H

#include <vector>
#include <utility>
#include "ns3/packet.h"
#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/udp-client.h"
#include "ns3/wifi-net-device.h"
#include "ns3/per-sta-q-info-container.h"
#include "ns3/net-device-container.h"
#include "ns3/bss-phy-mac-stats.h"
#include "ns3/wifi-mac-queue.h"
#include "ns3/mpdu-universal-aggregator.h"
#include "ns3/aggregation-controllers.h"

namespace ns3 {

class PerStaAggregationHelper
{
public:
  /**
   * Helper class for creating and configuring a single BSS with
   * particular service policy and aggregation algorithm at the
   * access point. Uses the PerStaQInfo class.
   */
    PerStaAggregationHelper (Ptr<NetDevice> ap, uint8_t nSta, uint8_t ac);
    ~PerStaAggregationHelper ();

    PerStaQInfoContainer InstallPerStaQInfo (const NetDeviceContainer sta, NetDeviceContainer apDevice, uint8_t ac);

    Ptr<BssPhyMacStats> InstallBssPhyMacStats (uint32_t hist, PerStaQInfoContainer &c);

    void FinalizeSetup (PerStaQInfoContainer &c);

    void SetPerStaWifiMacQueue (std::string n0 = "", const AttributeValue &v0 = EmptyAttributeValue (),
                                std::string n1 = "", const AttributeValue &v1 = EmptyAttributeValue (),
                                std::string n2 = "", const AttributeValue &v2 = EmptyAttributeValue ());

    void SetMpduUniversalAggregator (std::string n0 = "", const AttributeValue &v0 = EmptyAttributeValue (),
                                     std::string n1 = "", const AttributeValue &v1 = EmptyAttributeValue (),
                                     std::string n2 = "", const AttributeValue &v2 = EmptyAttributeValue ());

    void SetAggregationController (std::string n0 = "", const AttributeValue &v0 = EmptyAttributeValue (),
                                   std::string n1 = "", const AttributeValue &v1 = EmptyAttributeValue (),
                                   std::string n2 = "", const AttributeValue &v2 = EmptyAttributeValue (),
                                   std::string n3 = "", const AttributeValue &v3 = EmptyAttributeValue (),
                                   std::string n4 = "", const AttributeValue &v4 = EmptyAttributeValue (),
                                   std::string n5 = "", const AttributeValue &v5 = EmptyAttributeValue (),
                                   std::string n6 = "", const AttributeValue &v6 = EmptyAttributeValue (),
                                   std::string n7 = "", const AttributeValue &v7 = EmptyAttributeValue (),
                                   std::string n8 = "", const AttributeValue &v8 = EmptyAttributeValue (),
                                   std::string n9 = "", const AttributeValue &v9 = EmptyAttributeValue (),
                                   std::string n10 = "", const AttributeValue &v10 = EmptyAttributeValue ());

protected:

    uint8_t m_nSta; //!< Number of Stations
    Ptr<MacLow> m_low; //!< Pointer to MacLow related to the AP
    Ptr<PerStaWifiMacQueue> m_queue; //!< Pointer to PerStaWifiMacQueue related to the AP
    Ptr<MpduUniversalAggregator> m_aggregator; //!< Pointer to MpduUniversalAggregator related to the AP
    Ptr<TimeAllowanceAggregationController> m_aggCtrl; //!< Pointer to TimeAllowanceAggregationController related to the AP //TODO make it generic <AggregationController>
};

} //name space ns3


#endif /* PER_STA_AGGREGATION_HELPER_H */
