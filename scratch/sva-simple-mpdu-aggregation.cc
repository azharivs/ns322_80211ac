/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 Seyed Vahid Azhari
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

#include <string>
#include <sstream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/per-sta-q-info-container.h"
#include "ns3/bss-phy-mac-stats.h"
#include "ns3/uinteger.h"
#include "ns3/per-sta-aggregation-helper.h"

// This is a simple example in order to show how 802.11n MPDU aggregation feature works.
// The throughput is obtained for a given number of aggregated MPDUs.
//
// The number of aggregated MPDUs can be chosen by the user through the nMpdus attribute.
// A value of 1 means that no MPDU aggregation is performed.
//
// Example: ./waf --run "simple-mpdu-aggregation --nMpdus=64"
//
// Network topology:
//
//   Wifi 192.168.1.0/24
//   AP will have the first address: 192.168.1.1
//
//        AP
//   *    *
//   |    |
//   n1   n2
//
// sva:
// Seyed Vahid Azhari
// Packets in this simulation will be marked with a QosTag of AC_VI
// This is the first step toward developing my base line scheduler
// The AP acts as the UDP client sending packets and STAs act as servers.
// number of stations can be set using nSta
// Currently there is an unfairness issue if aggregation is set to low values
// I'm not exactly sure why it happens but could be due to buffer overflow
// at the client side.
// Also I am making bad use of application container for the client apps on the AP
// Everything else seems to be working fine
using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SimpleMpduAggregation");

int main (int argc, char *argv[])
{
  //LogComponentEnable ("UdpClient", LOG_LEVEL_DEBUG);
  //LogComponentEnable ("UdpServer", LOG_LEVEL_DEBUG);

  uint32_t payloadSize = 1472; //bytes
  uint64_t simulationTime = 20; //seconds
  uint32_t nMpdus = 64;
  uint32_t nSta = 4;
  double dMax = 1.0;//maximum tolerable delay
  bool enableRts = 0;
    
  CommandLine cmd;
  cmd.AddValue("nSta", "Number of stations", nSta); //sva: number of stations specified by the user
  cmd.AddValue("nMpdus", "Number of aggregated MPDUs", nMpdus); //number of aggregated MPDUs specified by the user
  cmd.AddValue("payloadSize", "Payload size in bytes", payloadSize);
  cmd.AddValue("enableRts", "Enable RTS/CTS", enableRts);
  cmd.AddValue("simulationTime", "Simulation time in seconds", simulationTime);
  cmd.AddValue("dMax", "maximum tolerable end to end delay in seconds", dMax);
  cmd.Parse (argc, argv);
    
  if(!enableRts)
    Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("999999"));
  else
    Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("0"));
  //sva: disable fragmentation
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("990000"));

#ifdef SVA_DEBUG_DETAIL
  Packet::EnablePrinting ();
#endif

  NodeContainer wifiStaNode;
  wifiStaNode.Create (nSta);
  NodeContainer wifiApNode;
  wifiApNode.Create(1);

  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
  phy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);
  phy.SetChannel (channel.Create());

  WifiHelper wifi = WifiHelper::Default ();
  //wifi.EnableLogComponents();//sva: added
  wifi.SetStandard (WIFI_PHY_STANDARD_80211n_5GHZ);
  //wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode", StringValue("OfdmRate65MbpsBW20MHz"), "ControlMode", StringValue("OfdmRate6_5MbpsBW20MHz"));
  wifi.SetRemoteStationManager ("ns3::IdealWifiManagerForMarkovChannelModel11n");
  //wifi.SetRemoteStationManager ("ns3::IdealWifiManager");
  HtWifiMacHelper mac = HtWifiMacHelper::Default ();

  Ssid ssid = Ssid ("universal-mpdu-aggregation");
  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid),
               "ActiveProbing", BooleanValue (false));

  if (nMpdus > 1) mac.SetBlockAckThresholdForAc (AC_VI, 2); //enable Block ACK when A-MPDU is enabled (i.e. nMpdus > 1)

  mac.SetMpduAggregatorForAc (AC_VI,"ns3::MpduUniversalAggregator",
                              "MaxAmpduSize", UintegerValue (nMpdus*(payloadSize+100))); //enable MPDU aggregation for AC_VI with a maximum aggregated size of nMpdus*(payloadSize+100) bytes, i.e. nMpdus aggregated packets in an A-MPDU
  
  NetDeviceContainer staDevice;
  staDevice = wifi.Install (phy, mac, wifiStaNode);

  mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid),
               "BeaconInterval", TimeValue (MicroSeconds(102400)),
               "BeaconGeneration", BooleanValue (true));

  if (nMpdus > 1) mac.SetBlockAckThresholdForAc (AC_VI, 2); //enable Block ACK when A-MPDU is enabled (i.e. nMpdus > 1)
    
  mac.SetMpduAggregatorForAc (AC_VI,"ns3::MpduUniversalAggregator",
                              "MaxAmpduSize", UintegerValue (nMpdus*(payloadSize+100))); //enable MPDU aggregation for AC_VI with a maximum aggregated size of nMpdus*(payloadSize+100) bytes, i.e. nMpdus aggregated packets in an A-MPDU

  NetDeviceContainer apDevice;
  apDevice = wifi.Install (phy, mac, wifiApNode);

  PerStaAggregationHelper bss(nSta);

  //Initialize PerStaQInfo after AP and STA's initialized

//  PerStaQInfoContainer perStaQueue = wifi.InitPerStaQInfo(staDevice, AC_VI);//TODO take this out of the WifiHelper and put it in mine
//  apDevice.Get(0)->GetObject<WifiNetDevice>()->GetMac()->GetObject<ApWifiMac>()->
//      SetPerStaQInfo(perStaQueue,AC_VI);

  PerStaQInfoContainer perStaQueue = bss.InstallPerStaQInfo(staDevice, apDevice, AC_VI);

  //Initialize per station time allowances
  //PerBitrateTimeAllowanceHelper taHelper;
  //taHelper.Install(perStaQueue,"./TimeAllowance.txt");

  //Initialize BssPhyMacStats for statistic collection on the medium

//  std::ostringstream path;
//  path << "/NodeList/"<< nSta << "/DeviceList/0/$ns3::WifiNetDevice/Phy/State";
//  Ptr<BssPhyMacStats> bssPhyMacStats = CreateObject<BssPhyMacStats> (path.str());
//  bssPhyMacStats->SetAttribute("HistorySize",UintegerValue(10)); //keep history of the last 10 beacons (i.e. one seconds)
//  NS_ASSERT(bssPhyMacStats->SetPerStaQInfo(&perStaQueue));

  Ptr<BssPhyMacStats> bssPhyMacStats = bss.InstallBssPhyMacStats(10,perStaQueue);

  //Initialize PerStaWifiMacQueue. Only need to care about the AP

  Ptr<PerStaWifiMacQueue> macQueue;
  ServicePolicyType QueueServicePolicy = EDF;
  uint32_t MaxPacketNumber=30000;
  double ServiceInterval = 0.1; //seconds
  double MaxDelay = 10.0; //seconds
  macQueue->SetAttribute("ServicePolicy",EnumValue(QueueServicePolicy));
  macQueue->SetAttribute("MaxPacketNumber",UintegerValue(MaxPacketNumber));
  macQueue->SetAttribute("MaxDelay",DoubleValue(MaxDelay));
  macQueue->SetAttribute("ServiceInterval",DoubleValue(ServiceInterval));

  //Initialize MpduUniversalAggregator.  Only need to care about the AP

  Ptr<MpduUniversalAggregator> aggregator;
  AggregationType AggregationAlgorithm = STANDARD;
  uint32_t MaxAmpduSize = 65535;//TODO allow larger values. May require changes to the aggregator class
  aggregator->SetAttribute("Algorithm",EnumValue(AggregationAlgorithm));
  aggregator->SetAttribute("ServiceInterval",DoubleValue(ServiceInterval));
  aggregator->SetAttribute("MaxAmpduSize",UintegerValue(MaxAmpduSize));

  //Initialize AggregationController. Only need to care about the AP
  Ptr<TimeAllowanceAggregationController> aggCtrl;
  double dvp = 0.02;
  double MovingIntegralWeight = 0.05;
  double kp = 0.01;
  double ki = 0.02;
  double kd = 0.05;
  double thrW = 0.5;
  double thrH = 2.5;
  double thrL = 2.5;
  ControllerType controller = PID;
  aggCtrl->SetAttribute("DVP",DoubleValue(dvp));
  aggCtrl->SetAttribute("ServiceInterval",DoubleValue());
  aggCtrl->SetAttribute("MaxDelay",DoubleValue(dMax));
  aggCtrl->SetAttribute("MovingIntegralWeight",DoubleValue(MovingIntegralWeight));
  aggCtrl->SetAttribute("KP",DoubleValue(kp));
  aggCtrl->SetAttribute("KI",DoubleValue(ki));
  aggCtrl->SetAttribute("KD",DoubleValue(kd));
  aggCtrl->SetAttribute("ThrWeight",DoubleValue(thrW));
  aggCtrl->SetAttribute("ThrHighCoef",DoubleValue(thrH));
  aggCtrl->SetAttribute("ThrLowCoef",DoubleValue(thrL));
  aggCtrl->SetAttribute("Controller",EnumValue(controller));

  /* Setting mobility model */
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();

  //sva: push positions on a stack
  positionAlloc->Add (Vector (1.0, 0.0, 0.0));
  positionAlloc->Add (Vector (-1.0, 0.0, 0.0));
  positionAlloc->Add (Vector (0.0, 1.0, 0.0));
  positionAlloc->Add (Vector (0.0, -1.0, 0.0));
  positionAlloc->Add (Vector (0.0, 0.0, 0.0));
  mobility.SetPositionAllocator (positionAlloc);

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

  mobility.Install (wifiApNode);//sva: AP gets position at top of the stack
  mobility.Install (wifiStaNode);//sva: other STAs iterate through the position stack with looping

  /* Internet stack*/
  InternetStackHelper stack;
  stack.Install (wifiApNode);
  stack.Install (wifiStaNode);

  Ipv4AddressHelper address;

  address.SetBase ("192.168.1.0", "255.255.255.0");
  Ipv4InterfaceContainer ApInterface;
  ApInterface = address.Assign (apDevice);//AP will get the first address 192.168.1.1
  Ipv4InterfaceContainer StaInterface;
  StaInterface = address.Assign (staDevice);//sva: allocates addresses in an increasing order
 
  /* Setting applications */
  //sva: STA is server receiving packets

  UdpServerHelper myServer (9);
  ApplicationContainer serverApp = myServer.Install (wifiStaNode);
  serverApp.Start (Seconds (0.0));
  serverApp.Stop (Seconds (simulationTime+1));
      
  //sva: AP is client sending packets
  //sva: pre-initialize all clients to pick the first station as their remote server
  UdpClientHelper myClient (StaInterface.GetAddress (0), 9);
  myClient.SetAttribute ("MaxPackets", UintegerValue (4294967295u));
  myClient.SetAttribute ("Interval", TimeValue (Time ("0.002"))); //packets/s
  myClient.SetAttribute ("PacketSize", UintegerValue (payloadSize));

  ApplicationContainer clientApp;//sva: my empty application container for UDP clients
  uint32_t j;
  //sva: figuring out how to do the iterations.
  for ( j = 0 ; j < nSta ; j ++)
    {
	  //sva: initialize correct remote address for next client instantiation
	  myClient.SetAttribute ("RemoteAddress", AddressValue (StaInterface.GetAddress (j)));
	  //sva: set dealine for each stations traffic
	  //sva: 0.5 sec, 0.7sec, 0.9sec, 1.2 sec
	  myClient.SetAttribute("Deadline",DoubleValue(dMax)); //set deadline
	  //sva: this is not the correct way of doing it.
	  //sva: I am creating a single dangling application container for each client
	  //sva: May have to change the upd client helper to fix this
	  //sva: needs a new Install method
	  clientApp = myClient.Install (wifiApNode.Get (0));
	  //myClient.setStartTime(Seconds (1.0));
	  //myClient.setStopTime(Seconds (simulatioTime+1));
	  clientApp.Start (Seconds (1.0));
	  clientApp.Stop (Seconds (simulationTime+1));
    }


      
  Simulator::Stop (Seconds (simulationTime+1));

  Simulator::Run ();

  uint32_t totalPacketsThrough=0;
  uint32_t totalPacketsLost=0;
  double throughput=0;
  double pdr=0;
  PerStaStatType stats;
  Ptr<PerStaQInfo> staPtr;
  for (j = 0; j < nSta; j ++)
    {
      Ptr<NetDevice> netDevice = staDevice.Get(j);
      Ptr<WifiNetDevice> device = netDevice->GetObject<WifiNetDevice>();
      Ptr<RegularWifiMac> wifiMac = device->GetMac()->GetObject<RegularWifiMac>();
      staPtr=perStaQueue.GetByMac( wifiMac->GetAddress() );
      stats = staPtr->GetAllStats();
	    totalPacketsThrough = DynamicCast<UdpServer>(serverApp.Get (j))->GetReceived ();
	    totalPacketsLost = DynamicCast<UdpServer>(serverApp.Get (j))->GetLost ();
	    throughput = totalPacketsThrough*payloadSize*8/(simulationTime*1000000.0);
	    pdr = (double) totalPacketsThrough/(double)(totalPacketsThrough+totalPacketsLost);
      std::cout << "STA(" << j << ") [" << staPtr->GetMac() << "] Throughput: " << throughput << " Mbps, PDR: " << pdr
          << " Arrival Rate = " << stats.avgArrival << " pps(" << stats.avgArrivalBytes/1000000*8 << " Mbps), Average Q Size = "
          << stats.avgQueue << " Pkts(" << stats.avgBytes/1000000 << " MBytes), Average Q Wait = "<< stats.avgWait*1000
          << " msec, DVP = " << stats.dvp << "; \n";
    }

  Simulator::Destroy ();

  return 0;
}
