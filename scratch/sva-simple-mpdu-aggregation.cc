/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 Sébastien Deronne
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
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"

// This is a simple example in order to show how 802.11n MPDU aggregation feature works.
// The throughput is obtained for a given number of aggregated MPDUs.
//
// The number of aggregated MPDUs can be chosen by the user through the nMpdus attibute.
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
// number of stations canbe set using nSta
// Currently there is an unfairness issue if aggregation is set to low values
// I'm not exactly sure why it happens but could be due to buffer overflow
// at the client side.
// Also I am making bad use of application container for the client apps on the AP
// Everything else seems to be working fine
// TODO: Extract delay and delay violation probability for each traffic flow
// TODO: Assign traffic flows to AC_VI
using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SimpleMpduAggregation");

int main (int argc, char *argv[])
{
  //LogComponentEnable ("UdpClient", LOG_LEVEL_DEBUG);
  //LogComponentEnable ("UdpServer", LOG_LEVEL_DEBUG);

  uint32_t payloadSize = 1472; //bytes
  uint64_t simulationTime = 2; //seconds
  uint32_t nMpdus = 1;
  uint32_t nSta = 1;
  bool enableRts = 0;
    
  CommandLine cmd;
  cmd.AddValue("nSta", "Number of stations", nSta); //sva: number of stations specified by the user
  cmd.AddValue("nMpdus", "Number of aggregated MPDUs", nMpdus); //number of aggregated MPDUs specified by the user
  cmd.AddValue("payloadSize", "Payload size in bytes", payloadSize);
  cmd.AddValue("enableRts", "Enable RTS/CTS", enableRts);
  cmd.AddValue("simulationTime", "Simulation time in seconds", simulationTime);
  cmd.Parse (argc, argv);
    
  if(!enableRts)
    Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("999999"));
  else
    Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("0"));
  //sva: disable fragmentation
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("990000"));

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
  myClient.SetAttribute ("Interval", TimeValue (Time ("0.0002"))); //packets/s
  myClient.SetAttribute ("PacketSize", UintegerValue (payloadSize));

  ApplicationContainer clientApp;//sva: my empty application container for UDP clients
  uint32_t j; //sva: figuring out how to do the iterations.
  for ( j = 0 ; j < nSta ; j ++)
    {
	  //sva: initialize correct remote address for next client instantiation
	  myClient.SetAttribute ("RemoteAddress", AddressValue (StaInterface.GetAddress (j)));
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
  Simulator::Destroy ();

  uint32_t totalPacketsThrough=0;
  uint32_t totalPacketsLost=0;
  double throughput=0;
  double pdr=0;
  for (j = 0; j < nSta; j ++)
    {
	  totalPacketsThrough = DynamicCast<UdpServer>(serverApp.Get (j))->GetReceived ();
	  totalPacketsLost = DynamicCast<UdpServer>(serverApp.Get (j))->GetLost ();
	  throughput = totalPacketsThrough*payloadSize*8/(simulationTime*1000000.0);
	  pdr = (double) totalPacketsThrough/(double)(totalPacketsThrough+totalPacketsLost);
      std::cout << "STA(" << j << ") Throughput: " << throughput << " Mbit/s, App layer PDR: " << pdr << '\n';
    }
    
  return 0;
}
