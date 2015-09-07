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
//#include "ns3/point-to-point-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/csma-helper.h"
#include "ns3/evalvid-client-server-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("EvalvidClientServerExample");

int
main (int argc, char *argv[])
{

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
  Config::SetDefault("ns3::TcpSocket::DelAckCount", UintegerValue (0));
  // Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue (524288));
  // Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue (524288));
  // Config::SetDefault("ns3::TcpSocket::SlowStartThreshold", UintegerValue (1000000));
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
/*PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", DataRateValue(linkRate));
  pointToPoint.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  pointToPoint.SetQueue("ns3::DropTailQueue","MaxPackets",UintegerValue(queueSize));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("10ms"));*/




  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", DataRateValue (linkRate));
  csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));
  csma.SetDeviceAttribute ("Mtu", UintegerValue (1500));
csma.SetQueue("ns3::DropTailQueue","MaxPackets",UintegerValue(queueSize));
  NetDeviceContainer d = csma.Install (n);

  Ipv4AddressHelper ipv4;
//
// We've got the "hardware" in place.  Now we need to add IP addresses.
//
  NS_LOG_INFO ("Assign IP Addresses.");
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i = ipv4.Assign (d);

  NS_LOG_INFO ("Create Applications.");



 // Create a BulkSendApplication and install it on node 0
  uint16_t port = 9;  // well-known echo port number

  EvalvidServerHelper source ("ns3::TcpSocketFactory",
                         InetSocketAddress (i.GetAddress (1), port));
  // Set the amount of data to send in bytes.  Zero is unlimited.
  source.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
  // Set the segment size
  source.SetAttribute ("SendSize", UintegerValue (100));//10000
 source.SetAttribute ("SenderTraceFilename", StringValue("st_highway_cif.st"));
  source.SetAttribute ("SenderDumpFilename", StringValue("sd_a01"));
  ApplicationContainer sourceApps = source.Install (n.Get (0));

  sourceApps.Start (Seconds (1.0));
  sourceApps.Stop (Seconds (100.0));

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
