/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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

 Author: Billy Pinheiro <haquiticos@gmail.com>

 */



// Network topology
//
//       n0    n1
//       |     |
//       =======
//         LAN
//
// - UDP flows from n0 to n1

#include <fstream>
#include <string.h>
#include "ns3/point-to-point-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/csma-helper.h"
#include "ns3/evalvid-client-server-helper.h"

#include <string>
#include <fstream>
#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"

#include "ns3/simulator.h"

#include <ctype.h>
#include <iostream>
#include <fstream>
#include <string>
#include <cassert>



using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("EvalvidClientServerExample");

int
main (int argc, char *argv[])
{
 uint32_t payloadSize = 1460; //bytes
 uint32_t nMpdus = 1;
  bool enableRts = 0;
uint32_t maxBytes = 0;
  uint32_t queueSize = 100000;
  DataRate linkRate("1000Mbps");
  std::string protocol = "TcpNewReno";
//
// Enable logging for EvalvidClient and
//
  LogComponentEnable ("EvalvidClient", LOG_LEVEL_INFO);
  LogComponentEnable ("EvalvidServer", LOG_LEVEL_INFO);



  // Set TCP defaults
  Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue (1500));
 // Config::SetDefault("ns3::TcpSocket::DelAckCount", UintegerValue (0));
   Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue (524288));
   Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue (52428800));
 //  Config::SetDefault("ns3::TcpSocket::SlowStartThreshold", UintegerValue (1000000));
  if (protocol == "TcpTahoe")
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpTahoe"));
  else
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpNewReno"));


  //
  // But since this is a realtime script, don't allow the user to mess with
  // that.
  //
  //GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::RealtimeSimulatorImpl"));

//
// Explicitly create the nodes required by the topology (shown above).
//
  NS_LOG_INFO ("Create nodes.");
  NodeContainer n;
  n.Create (2);

  InternetStackHelper internet;
  internet.Install (n);

  NS_LOG_INFO ("Create channels.");
//
// Explicitly create the channels required by the topology (shown above).
//
 PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", DataRateValue(linkRate));//DataRate(10000000)
  pointToPoint.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  //pointToPoint.SetQueue("ns3::DropTailQueue","MaxPackets",UintegerValue(queueSize));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("4ms"));//TimeValue (MilliSeconds(10))




  
  NetDeviceContainer d = pointToPoint.Install (n);

  Ipv4AddressHelper ipv4;
//
// We've got the "hardware" in place.  Now we need to add IP addresses.
//
  NS_LOG_INFO ("Assign IP Addresses.");
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i = ipv4.Assign (d);

//Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

 /*if(!enableRts)
    Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("999999"));
  else
    Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("0"));
     
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("990000"));

  NodeContainer wifiStaNode;
  wifiStaNode.Create (1);
  NodeContainer wifiApNode;
  wifiApNode.Create(1);

  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
  phy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);
  phy.SetChannel (channel.Create());

  WifiHelper wifi = WifiHelper::Default ();
  wifi.SetStandard (WIFI_PHY_STANDARD_80211n_5GHZ);
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode", StringValue("OfdmRate65MbpsBW20MHz"), "ControlMode", StringValue("OfdmRate6_5MbpsBW20MHz"));
  HtWifiMacHelper mac = HtWifiMacHelper::Default ();

  Ssid ssid = Ssid ("simple-mpdu-aggregation");
  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid),
               "ActiveProbing", BooleanValue (false));

  if (nMpdus > 1) mac.SetBlockAckThresholdForAc (AC_BE, 2); //enable Block ACK when A-MPDU is enabled (i.e. nMpdus > 1)

  mac.SetMpduAggregatorForAc (AC_BE,"ns3::MpduStandardAggregator",
                              "MaxAmpduSize", UintegerValue (nMpdus*(payloadSize+100))); //enable MPDU aggregation for AC_BE with a maximum aggregated size of nMpdus*(payloadSize+100) bytes, i.e. nMpdus aggregated packets in an A-MPDU
  
  NetDeviceContainer staDevice;
  staDevice = wifi.Install (phy, mac, wifiStaNode);

  mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid),
               "BeaconInterval", TimeValue (MicroSeconds(102400)),
               "BeaconGeneration", BooleanValue (true));

  if (nMpdus > 1) mac.SetBlockAckThresholdForAc (AC_BE, 2); //enable Block ACK when A-MPDU is enabled (i.e. nMpdus > 1)
    
  mac.SetMpduAggregatorForAc (AC_BE,"ns3::MpduStandardAggregator",
                              "MaxAmpduSize", UintegerValue (nMpdus*(payloadSize+100))); //enable MPDU aggregation for AC_BE with a maximum aggregated size of nMpdus*(payloadSize+100) bytes, i.e. nMpdus aggregated packets in an A-MPDU

  NetDeviceContainer apDevice;
  apDevice = wifi.Install (phy, mac, wifiApNode);

 
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();

  positionAlloc->Add (Vector (0.0, 0.0, 0.0));
  positionAlloc->Add (Vector (1.0, 0.0, 0.0));
  mobility.SetPositionAllocator (positionAlloc);

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

  mobility.Install (wifiApNode);
  mobility.Install (wifiStaNode);

 
  InternetStackHelper stack;
  stack.Install (wifiApNode);
  stack.Install (wifiStaNode);

  Ipv4AddressHelper address;

  address.SetBase ("192.168.1.0", "255.255.255.0");
  Ipv4InterfaceContainer StaInterface;
  StaInterface = address.Assign (staDevice);
  Ipv4InterfaceContainer ApInterface;
  ApInterface = address.Assign (apDevice);


















  NS_LOG_INFO ("Create Applications.");



 // Create a BulkSendApplication and install it on node 0
  uint16_t port = 9;  // well-known echo port number

  EvalvidServerHelper source ("ns3::TcpSocketFactory",
                         InetSocketAddress (ApInterface.GetAddress (0), port));
  // Set the amount of data to send in bytes.  Zero is unlimited.
  source.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
  // Set the segment size
  source.SetAttribute ("SendSize", UintegerValue (500));//10000
 source.SetAttribute ("SenderTraceFilename", StringValue("st_highway_cif.st"));
  source.SetAttribute ("SenderDumpFilename", StringValue("sd_a01"));
  ApplicationContainer sourceApps = source.Install (wifiStaNode.Get (0));

  sourceApps.Start (Seconds (1.0));
  sourceApps.Stop (Seconds (100.0));

  // Create a PacketSinkApplication and install it on node 1
  EvalvidClientHelper sink ("ns3::TcpSocketFactory",
                         InetSocketAddress (Ipv4Address::GetAny (), port));
sink.SetAttribute ("ReceiverDumpFilename", StringValue("rd_a01"));
  ApplicationContainer sinkApps = sink.Install (wifiApNode.Get (0));

  sinkApps.Start (Seconds (0.0));
  sinkApps.Stop (Seconds (101.0));*/


// Create a BulkSendApplication and install it on node 0
  // Create a BulkSendApplication and install it on node 0
  uint16_t port = 9;  // well-known echo port number

  EvalvidServerHelper source ("ns3::TcpSocketFactory",
                         InetSocketAddress (i.GetAddress (1), port));
  // Set the amount of data to send in bytes.  Zero is unlimited.
  source.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
  // Set the segment size
  source.SetAttribute ("SendSize", UintegerValue (100));//10000
  //source.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  source.SetAttribute ("SenderTraceFilename", StringValue("st_highway_cif.st"));
  source.SetAttribute ("SenderDumpFilename", StringValue("sd_a01"));
  ApplicationContainer sourceApps = source.Install (n.Get (0));

  sourceApps.Start (Seconds (1.0));
  sourceApps.Stop (Seconds (100.0));



/*  Address sinkAddress (InetSocketAddress (i.GetAddress (1), port));
  PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), port));
  ApplicationContainer sinkApps = packetSinkHelper.Install (n.Get (1));
  sinkApps.Start (Seconds (0.));
  sinkApps.Stop (Seconds (101));*/



  // Create a PacketSinkApplication and install it on node 1
  EvalvidClientHelper sink ("ns3::TcpSocketFactory",
                         InetSocketAddress (Ipv4Address::GetAny (), port));
sink.SetAttribute ("ReceiverDumpFilename", StringValue("rd_a01"));

  ApplicationContainer sinkApps = sink.Install (n.Get (1));

sinkApps.Start (Seconds (0.0));
  sinkApps.Stop (Seconds (101.0));

  

  //
// Now, do the actual simulation.
//
  NS_LOG_INFO ("Run Simulation.");
  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");

}
