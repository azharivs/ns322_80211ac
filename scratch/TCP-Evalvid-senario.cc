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
 */


//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//this program use 80.11n
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/evalvid-client-server-helper.h"
#include "ns3/netanim-module.h"

// Default Network Topology
//
// Wifi(802.11n) 10.1.3.0
//                 AP
//  *    *    *    *
//  |    |    |    |    10.1.1.0
// n5   n6   n7   n0 -------------- n1   n2   n3   n4
//                   point-to-point  |    |    |    |
//                                   ================
//                                     LAN 10.1.2.0

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TCP-Evalvid-Senario");

int 
main (int argc, char *argv[])
{
    uint32_t payloadSize = 360; //bytes
    uint64_t simulationTime = 10; //seconds
    bool enableRts = 0;// 0 is false
    uint32_t maxBytes = 0;
    uint32_t nCsma = 3;
    uint32_t nWifi = 3;
    int nMpdus = 6;
    DataRate linkRate("1Gbps");
    std::string protocol = "TcpNewReno";

    // Enable logging for EvalvidClient and
    //
    LogComponentEnable ("EvalvidClient", LOG_LEVEL_INFO);
    LogComponentEnable ("EvalvidServer", LOG_LEVEL_INFO);


    CommandLine cmd;
    cmd.AddValue ("nCsma", "Number of \"extra\" CSMA nodes/devices", nCsma);
    cmd.AddValue ("nWifi", "Number of wifi STA devices", nWifi);
    cmd.Parse (argc,argv);

    if (nWifi > 18)
    {
      std::cout << "Number of wifi nodes " << nWifi << 
                   " specified exceeds the mobility bounding box" << std::endl;
      exit (1);
    }

    uint32_t mtu_bytes = 400;
    Header* temp_header = new Ipv4Header ();
    uint32_t ip_header = temp_header->GetSerializedSize ();
    NS_LOG_LOGIC ("IP Header size is: " << ip_header);
    delete temp_header;
    temp_header = new TcpHeader ();
    uint32_t tcp_header = temp_header->GetSerializedSize ();
    NS_LOG_LOGIC ("TCP Header size is: " << tcp_header);
    delete temp_header;
    uint32_t tcp_adu_size = mtu_bytes - (ip_header + tcp_header);
    NS_LOG_LOGIC ("TCP ADU size is: " << tcp_adu_size);

 

    // Set TCP defaults
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue (tcp_adu_size));
    Config::SetDefault("ns3::TcpSocket::DelAckCount", UintegerValue (0));



    if (protocol == "TcpTahoe")
         Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpTahoe"));
    else
         Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpNewReno"));

  
    if(!enableRts)
         Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("999999"));
    else
         Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("0"));
     
    Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("990000"));

 
     NodeContainer p2pNodes;
     p2pNodes.Create (2);

     PointToPointHelper pointToPoint;
     pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("100Gbps"));
     pointToPoint.SetChannelAttribute ("Delay", StringValue ("1ms"));

     NetDeviceContainer p2pDevices;
     p2pDevices = pointToPoint.Install (p2pNodes);

     NodeContainer csmaNodes;
     csmaNodes.Add (p2pNodes.Get (1));
     csmaNodes.Create (nCsma);

     CsmaHelper csma;
     csma.SetChannelAttribute ("DataRate", StringValue ("100Gbps"));
     csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (1)));


     NetDeviceContainer csmaDevices;
     csmaDevices = csma.Install (csmaNodes);

     NodeContainer wifiStaNodes;
     wifiStaNodes.Create (nWifi);
     NodeContainer wifiApNode = p2pNodes.Get (0);



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
     staDevice = wifi.Install (phy, mac, wifiStaNodes);

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
       mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                     "MinX", DoubleValue (0.0),
                                     "MinY", DoubleValue (0.0),
                                     "DeltaX", DoubleValue (5.0),
                                     "DeltaY", DoubleValue (10.0),
                                     "GridWidth", UintegerValue (3),
                                     "LayoutType", StringValue ("RowFirst"));

        mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                                   "Bounds", RectangleValue (Rectangle (-70, 70, -70, 70)));
        mobility.Install (wifiStaNodes);

        mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
        mobility.Install (wifiApNode);
		  
        //Csmanodes mobility
        Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
        positionAlloc->Add (Vector (30.0, 20.0, 0.0));
        positionAlloc->Add (Vector (25.0, 28.0, 0.0));
        positionAlloc->Add (Vector (33.0, 33.0, 0.0));
		 
        mobility.SetPositionAllocator (positionAlloc);
        mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

        mobility.Install (csmaNodes);
        //mobility.Install (wifiApNode);


        InternetStackHelper stack;
        stack.Install (csmaNodes);
        stack.Install (wifiApNode);
        stack.Install (wifiStaNodes);

        Ipv4AddressHelper address;

        address.SetBase ("10.1.1.0", "255.255.255.0");
        Ipv4InterfaceContainer p2pInterfaces;
        p2pInterfaces = address.Assign (p2pDevices);

        address.SetBase ("10.1.2.0", "255.255.255.0");
        Ipv4InterfaceContainer csmaInterfaces;
        csmaInterfaces = address.Assign (csmaDevices);

        address.SetBase ("10.1.3.0", "255.255.255.0");
        Ipv4InterfaceContainer StaInterface;
        StaInterface = address.Assign (staDevice);
        Ipv4InterfaceContainer ApInterface;
        ApInterface = address.Assign (apDevice);


        // and setup ip routing tables to get total ip-level connectivity.
        Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

        NS_LOG_INFO ("Create Applications.");


        // Create a EvalvidServer and install it on node csmaNodes(nCsma)
        //EvalvidServer Send video 
        uint16_t port = 9;  // well-known echo port number

        EvalvidServerHelper source ("ns3::TcpSocketFactory",
                                 InetSocketAddress (StaInterface.GetAddress (nWifi-1), port));
        // Set the amount of data to send in bytes.  Zero is unlimited.
        source.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
        source.SetAttribute ("SendSize", UintegerValue (tcp_adu_size));
        source.SetAttribute ("SenderTraceFilename", StringValue("st_highway_cif.st"));
        source.SetAttribute ("SenderDumpFilename", StringValue("sender-output"));
        ApplicationContainer sourceApps = source.Install (csmaNodes.Get (nCsma));

        sourceApps.Start (Seconds (0.0));
        sourceApps.Stop (Seconds (simulationTime));


        // Create a EvalvidClient and install it on node wifiStaNodes(nWifi-1)

        Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue (tcp_adu_size));
        EvalvidClientHelper sink ("ns3::TcpSocketFactory",
                                 InetSocketAddress (Ipv4Address::GetAny (), port));
        sink.SetAttribute ("ReceiverDumpFilename", StringValue("receiver-output"));
          
        ApplicationContainer sinkApps = sink.Install (wifiStaNodes.Get (nWifi-1));

        sinkApps.Start (Seconds (0.0));
        sinkApps.Stop (Seconds (simulationTime));
        AsciiTraceHelper ascii;
        //pointToPoint.EnableAsciiAll (ascii.CreateFileStream ("evalvid-client-server.tr"));
           

    
        // Now, do the actual simulation.
        //
        NS_LOG_INFO ("Run Simulation.");
        Simulator::Stop (Seconds (simulationTime));
        Simulator::Run ();
        Simulator::Destroy ();
        NS_LOG_INFO ("Done.");
        return 0;
}
