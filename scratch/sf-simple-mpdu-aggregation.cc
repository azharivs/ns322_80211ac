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

//s
//added lib
#include "ns3/gnuplot-aggregator.h"
#include "ns3/gnuplot.h"
#include "ns3/flow-monitor-module.h"
#include <ns3/flow-monitor-helper.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <fstream>
#include "ns3/netanim-module.h"

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
//   Wifi 192.168.1.0
//
//        AP
//   *    *
//   |    |
//   n1   n2
//
// Packets in this simulation aren't marked with a QosTag so they are considered
// belonging to BestEffort Access Class (AC_BE).

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SimpleMpduAggregation");

//s
uint32_t MacTxDropCount = 0, PhyTxDropCount = 0, PhyRxDropCount = 0;
void MacTxDrop(Ptr<const Packet> p) {
	NS_LOG_INFO("Packet Drop");
	MacTxDropCount++;
}

void PrintDrop() {
	std::cout << "print " << Simulator::Now().GetSeconds() << "\t"
			<< MacTxDropCount << "\t" << PhyTxDropCount << "\t"
			<< PhyRxDropCount << "\n";
	//Simulator::Schedule(Seconds(5.0), &PrintDrop);
}

void PhyTxDrop(Ptr<const Packet> p) {
	NS_LOG_INFO("Packet Drop");
	PhyTxDropCount++;
}
void PhyRxDrop(Ptr<const Packet> p) {
	NS_LOG_INFO("Packet Drop");
	PhyRxDropCount++;
}


int main (int argc, char *argv[])
{
  uint32_t payloadSize = 1472; //bytes
  //For standard MTU of 1500 bytes, the maximum data size is 1472 bytes
  //(MTU minus 20 bytes IP header and 8 bytes for the ICMP header)

  uint64_t simulationTime =1; //seconds
  uint32_t nSta = 3;
  uint32_t    Ideal11n = 1; // 1 means true
  uint32_t   nApp=3; //number of application ; maximum of 3 application
  uint32_t  StaToAp = 0; //if it is 1 then we have one application , sta send to ap
  //uint32_t nMpdus = 1;
  bool enableRts = 0;// 0 is false


  //s for creating figs: open the Terminal, go to ns-3.22 directory then write these two commands
  // rm all.sh ; cat *.sh > all.sh
  //. all.sh
  Ptr<GnuplotAggregator> delayA = CreateObject<GnuplotAggregator>("Delay");
  delayA->SetTerminal("png");
  delayA->SetTitle("Delay");
  delayA->SetLegend("AMPDU Size", "Delay");
  delayA->Enable();

  Ptr<GnuplotAggregator> troughA = CreateObject<GnuplotAggregator>("Throughput");
  troughA->SetTerminal("png");
  troughA->SetTitle("Throughput");
  troughA->SetLegend("AMPDU Size", "Throughput (Mb/s)");
  troughA->Enable();

  Ptr<GnuplotAggregator> jitterA = CreateObject<GnuplotAggregator>("Jitter");
  jitterA->SetTerminal("png");
  jitterA->SetTitle("Jitter");
  jitterA->SetLegend("AMPDU Size", "Jitter");
  jitterA->Enable();

  Ptr<GnuplotAggregator> pdrA = CreateObject<GnuplotAggregator>("PDR");
  pdrA->SetTerminal("png");
  pdrA->SetTitle("PDR");
  pdrA->SetLegend("AMPDU Size", "PDR");
  pdrA->Enable();
    
  for (int inter =1000; inter <=1000; inter += 250) {
    std::string context;
	std::stringstream ss;
	std::stringstream ss2;
	std::stringstream ss3;
	ss << "DataSet/Context/"<<inter*1472;
    ss2 << "Data Rate "<<inter*11.776;//kbit 1472 * 8 /1000=11.776
	ss3 << std::fixed << std::setw( 9 )<<std::setprecision(7 )<<std::setfill( '0' )<<1.0/inter;
    std::cout<<"ss "<<ss.str()<<" ss2 "<<ss2.str()<<" ss3 "<<ss3.str()<<std::endl;

	delayA->Add2dDataset(ss.str(),ss2.str());
	troughA->Add2dDataset(ss.str(),ss2.str());
	jitterA->Add2dDataset(ss.str(),ss2.str());
    pdrA->Add2dDataset(ss.str(),ss2.str());

	// this 'for' is used to see how some parameters such as throughput change by increasing the size of Mpdu aggregation
    for (int nMpdus =7; nMpdus <=7 ; nMpdus += 1) {
	  CommandLine cmd;
	  cmd.AddValue("nSta", "Number of stations", nSta); //sva: number of stations specified by the user
	  //cmd.AddValue("nMpdus", "Number of aggregated MPDUs", nMpdus); //number of aggregated MPDUs specified by the user
	  cmd.AddValue("payloadSize", "Payload size in bytes", payloadSize);
	  cmd.AddValue("enableRts", "Enable RTS/CTS", enableRts);
	  cmd.AddValue("simulationTime", "Simulation time in seconds", simulationTime);
	  cmd.AddValue("IsIdeal", "Is Ideal WifiManager For MarkovChannel Model", Ideal11n);
	  cmd.AddValue("nApp", "Number of application", nApp);
	  cmd.AddValue("StaToAp", "If it is 1 then sta 0 sends to Ap", StaToAp);
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
      wifi.SetStandard (WIFI_PHY_STANDARD_80211n_5GHZ);
	  //wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode", StringValue("OfdmRate65MbpsBW20MHz"), "ControlMode", StringValue("OfdmRate6_5MbpsBW20MHz"));
      if(Ideal11n==1)
          wifi.SetRemoteStationManager ("ns3::IdealWifiManagerForMarkovChannelModel11n");
      else
    	  wifi.SetRemoteStationManager ("ns3::IdealWifiManagerForMarkovChannelModel");
	  HtWifiMacHelper mac = HtWifiMacHelper::Default ();

	  Ssid ssid = Ssid ("simple-mpdu-aggregation");

	  mac.SetType ("ns3::ApWifiMac",
	              "Ssid", SsidValue (ssid),
	              "BeaconInterval", TimeValue (MicroSeconds(102400)),
	              "BeaconGeneration", BooleanValue (true));

	 if (nMpdus > 1) mac.SetBlockAckThresholdForAc (AC_BE, 2); //enable Block ACK when A-MPDU is enabled (i.e. nMpdus > 1)

	  mac.SetMpduAggregatorForAc (AC_BE,"ns3::MpduStandardAggregator",
		                         "MaxAmpduSize", UintegerValue (nMpdus*(payloadSize+100))); //enable MPDU aggregation for AC_BE with a maximum aggregated size of nMpdus*(payloadSize+100) bytes, i.e. nMpdus aggregated packets in an A-MPDU

	  NetDeviceContainer apDevice;
	  //sf  mac addresses begin with "00.00.00.01"
	  apDevice = wifi.Install (phy, mac, wifiApNode); //sf AP will get the first mac address ("00:00:00:00:00:01")
	   	   	   	   	   	   	   	   	   	   	   	   	  // we use this address for distinguishing between Ap and Sta in ideal-wifi-manager-for-markov-channel-model.cc that located in /src/wifi/model

	  mac.SetType ("ns3::StaWifiMac",
	               "Ssid", SsidValue (ssid),
	               "ActiveProbing", BooleanValue (false));

	  if (nMpdus > 1) mac.SetBlockAckThresholdForAc (AC_BE, 2); //enable Block ACK when A-MPDU is enabled (i.e. nMpdus > 1)

	  mac.SetMpduAggregatorForAc (AC_BE,"ns3::MpduStandardAggregator",
		                          "MaxAmpduSize", UintegerValue (nMpdus*(payloadSize+100))); //enable MPDU aggregation for AC_BE with a maximum aggregated size of nMpdus*(payloadSize+100) bytes, i.e. nMpdus aggregated packets in an A-MPDU

	  NetDeviceContainer staDevice;
	  staDevice = wifi.Install (phy, mac, wifiStaNode);


	  /* Setting mobility model */
	  MobilityHelper mobility;
	  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();

	  //sva  push positions on a stack
	  positionAlloc->Add (Vector (1.0, 0.0, 0.0));
	  positionAlloc->Add (Vector (0.0, 1.0, 0.0));
	  positionAlloc->Add (Vector (0.0, 0.0, 0.0));
	  positionAlloc->Add (Vector (0.0, 0.0, 2.0));
	  mobility.SetPositionAllocator (positionAlloc);

	  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

	  //sva  AP gets position at top of the stack and other STAs iterate through the position stack with looping
	  mobility.Install (wifiStaNode);
	  mobility.Install (wifiApNode);


	  /* Internet stack*/
	  InternetStackHelper stack;
	  stack.Install (wifiStaNode);
	  stack.Install (wifiApNode);


	  Ipv4AddressHelper address;

	  address.SetBase ("192.168.1.0", "255.255.255.0");
	  Ipv4InterfaceContainer ApInterface;
	  ApInterface = address.Assign (apDevice);//sva AP will get the first address 192.168.1.1
	  Ipv4InterfaceContainer StaInterface;
	  StaInterface = address.Assign (staDevice);//sva: allocates addresses in an increasing order


	  //AP sends to STAs
	  // Setting applications
	  //client sends to server : in this AP sends to a Sta
	  //First application
	  UdpServerHelper myServer (9);
	  ApplicationContainer serverApp = myServer.Install (wifiStaNode.Get(0));
	  serverApp.Start (Seconds (0.0));
	  serverApp.Stop (Seconds (simulationTime+1));

	  UdpClientHelper myClient (StaInterface.GetAddress (0), 9);
	  myClient.SetAttribute ("MaxPackets", UintegerValue (4294967295u));//4294967295u
	  myClient.SetAttribute ("Interval", TimeValue(Time(ss3.str()))); //packets/s
	  myClient.SetAttribute ("PacketSize", UintegerValue (payloadSize));

	  ApplicationContainer clientApp = myClient.Install (wifiApNode.Get (0));
	  clientApp.Start (Seconds (1.0));
	  clientApp.Stop (Seconds (simulationTime+1));

	  if(nApp>=2){
	  //Second application
	  UdpServerHelper myServer2 (10);
	  ApplicationContainer serverApp2 = myServer2.Install (wifiStaNode.Get(1));
	  serverApp2.Start (Seconds (0.0));
      serverApp2.Stop (Seconds (simulationTime+1));

	  UdpClientHelper myClient2 (StaInterface.GetAddress (1), 10);
	  myClient2.SetAttribute ("MaxPackets", UintegerValue (4294967295u));//4294967295u
	  myClient2.SetAttribute ("Interval", TimeValue(Time(ss3.str()))); //packets/s
	  myClient2.SetAttribute ("PacketSize", UintegerValue (payloadSize));

	  ApplicationContainer clientApp2 = myClient2.Install (wifiApNode.Get (0));
	  clientApp2.Start (Seconds (1.0));
	  clientApp2.Stop (Seconds (simulationTime+1));
	  }

	  if(nApp>=3){

	  //Third application
	  UdpServerHelper myServer3 (11);
	  ApplicationContainer serverApp3 = myServer3.Install (wifiStaNode.Get(2));
	  serverApp3.Start (Seconds (0.0));
	  serverApp3.Stop (Seconds (simulationTime+1));

	  UdpClientHelper myClient3 (StaInterface.GetAddress (2), 11);
	  myClient3.SetAttribute ("MaxPackets", UintegerValue (4294967295u));//4294967295u
	  myClient3.SetAttribute ("Interval", TimeValue(Time(ss3.str()))); //packets/s
	  myClient3.SetAttribute ("PacketSize", UintegerValue (payloadSize));

	  ApplicationContainer clientApp3 = myClient3.Install (wifiApNode.Get (0));
	  clientApp3.Start (Seconds (1.0));
	  clientApp3.Stop (Seconds (simulationTime+1));
	  }

	  if(StaToAp==1){
	  // STAs send to AP
	  //first application
	  UdpServerHelper myServer4 (12);
	  ApplicationContainer serverApp4 = myServer4.Install (wifiApNode.Get(0));
	  serverApp4.Start (Seconds (0.0));
	  serverApp4.Stop (Seconds (simulationTime+1));

	  UdpClientHelper myClient4 (ApInterface.GetAddress (0), 12);
	  myClient4.SetAttribute ("MaxPackets", UintegerValue (4294967295u));
	  //myClient4.SetAttribute ("Interval",TimeValue(Time("0.005"))); //packets/s
	  myClient4.SetAttribute ("Interval", TimeValue(Time(ss3.str()))); //packets/s
	  myClient4.SetAttribute ("PacketSize", UintegerValue (payloadSize));

	  ApplicationContainer clientApp4 = myClient4.Install (wifiStaNode.Get (0));
	  clientApp4.Start (Seconds (1.0));
	  clientApp4.Stop (Seconds (simulationTime+1));
	  }
       /*
	  //second aplication
	  UdpServerHelper myServer2 (10);
	  ApplicationContainer serverApp2 = myServer2.Install (wifiApNode.Get(0));
	  serverApp2.Start (Seconds (0.0));
	  serverApp2.Stop (Seconds (simulationTime+1));

	  UdpClientHelper myClient2 (ApInterface.GetAddress (0), 10);
	  myClient2.SetAttribute ("MaxPackets", UintegerValue (4294967295u));
	  //myClient2.SetAttribute ("Interval",TimeValue(Time("0.0005"))); //packets/s
	  myClient2.SetAttribute ("Interval", TimeValue(Time(ss3.str()))); //packets/s
	  myClient2.SetAttribute ("PacketSize", UintegerValue (payloadSize));

	  ApplicationContainer clientApp2 = myClient2.Install (wifiStaNode.Get (1));
	  clientApp2.Start (Seconds (1.0));
	  clientApp2.Stop (Seconds (simulationTime+1));*/

	  Simulator::Stop (Seconds (simulationTime+1));

	  //s
	  FlowMonitorHelper flowmon;
	  Ptr<FlowMonitor> monitor = flowmon.InstallAll();

	  // Trace Collisions
	  Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/MacTxDrop",
		                            MakeCallback(&MacTxDrop));
	  Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyRxDrop",
	     	                    	  MakeCallback(&PhyRxDrop));
	  Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyTxDrop",
		  			                MakeCallback(&PhyTxDrop));

	  Simulator::Run ();
	  //s
	 monitor->CheckForLostPackets();
	 Ptr<Ipv4FlowClassifier> classifier =DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
	 std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();
	 uint64_t bytesTotal = 0;
	 double lastRxTime = -1;
	 double firstRxTime = -1;
	 int flowid=0;
	 for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin(); i != stats.end(); ++i) {
	   flowid++;
	   if (firstRxTime < 0)
		  firstRxTime = i->second.timeFirstRxPacket.GetSeconds();
	   else if (firstRxTime > i->second.timeFirstRxPacket.GetSeconds())
		  firstRxTime = i->second.timeFirstRxPacket.GetSeconds();
	   if (lastRxTime < i->second.timeLastRxPacket.GetSeconds())
		  lastRxTime = i->second.timeLastRxPacket.GetSeconds();
		  bytesTotal = bytesTotal + i->second.rxBytes;

	   std::cout <<"th for flowid "<<flowid << "="<< (i->second.rxBytes)*8/(i->second.timeLastRxPacket.GetSeconds()-i->second.timeFirstRxPacket.GetSeconds())/1000 << std::endl;

      }

	  std::cout << "Traffic volume = " << bytesTotal << " bytes"
				<< std::endl;
	  std::cout << "Avg throughput = "
				<< bytesTotal * 8 / (lastRxTime - firstRxTime) / 1000
				<< " kbits/sec" << std::endl;
      troughA->Write2d( ss.str(),nMpdus,bytesTotal * 8 / (lastRxTime - firstRxTime) / 1000);
	  double delaysum = 0;
	  double rxPkt = 0;
	  double sum = 0;
      double num = 0;
	  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i =
		    stats.begin(); i != stats.end(); ++i) {
	    delaysum = i->second.delaySum.GetSeconds();
		rxPkt = i->second.rxPackets;
		if (rxPkt != 0) {
		  sum += delaysum / rxPkt;
		  std::cout << "delayPerFlow = " << delaysum / rxPkt
					<< " kbits/sec" << std::endl;
		}
		num++;

	  }

	  std::cout << "Mean delay = " << sum / num << " sec" << std::endl;
	  delayA->Write2d(ss.str(),nMpdus,sum / num);
	  double txPkt = 0;
	  rxPkt = 0;
	  double sumr = 0;
	  double sumt = 0;
	  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i =
		   stats.begin(); i != stats.end(); ++i) {
     	txPkt = i->second.txPackets;
		rxPkt = i->second.rxPackets;
		sumt += txPkt;
		sumr += rxPkt;
	  }

	  double pdr = sumr / sumt;
	  std::cout << "PDR = " << pdr << " persent" << std::endl;
	  pdrA->Write2d(ss.str(),nMpdus,pdr);
	  delaysum = 0;
	  rxPkt = 0;
	  sum = 0;
	  num = 0;
			for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i =
					stats.begin(); i != stats.end(); ++i) {
				delaysum = i->second.jitterSum.GetSeconds();
				rxPkt = i->second.rxPackets;
				sum += delaysum / (rxPkt - 1);
				//std::cout << "delayPerFlow = " << delaysum / rxPkt << " kbits/sec" << std::endl;
				num++;
			}
			std::cout << "Mean jitter = " << sum / num << " sec" << std::endl;
			jitterA->Write2d(ss.str(),nMpdus,sum / num);
			sum = 0;
			num = 0;
			double delaysum1 = 0;
			sumr = 0;
			rxPkt = 0;
			txPkt = 0;
			delaysum = 0;
			for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i =
					stats.begin(); i != stats.end(); ++i) {
				delaysum = i->second.txBytes;
				delaysum1 = i->second.rxBytes;

				rxPkt = i->second.timeLastTxPacket.GetSeconds();
				txPkt = i->second.timeFirstTxPacket.GetSeconds();

				sum += delaysum / (rxPkt - txPkt);
				sumr += delaysum1 / (rxPkt - txPkt);
				//std::cout << "delayPerFlow = " << delaysum / rxPkt << " kbits/sec" << std::endl;
				num++;
			}
			std::cout << "mean transmit bitrate = " << (sum / num) / (125)<< " kbit/s"
					<< std::endl;
			std::cout << "mean received bitrate = " << (sumr / num)/(125)
					<< " kbit/s" << std::endl;

			sum = 0;
			num = 0;
			rxPkt = 0;
			txPkt = 0;
			delaysum = 0;
			for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i =
					stats.begin(); i != stats.end(); ++i) {
				if (i->second.rxPackets != 0) {
					delaysum = (i->second.timesForwarded / i->second.rxPackets)
							+ 1;
					sum += delaysum;
				}
				num++;
			}
			std::cout << "mean hop count = " << sum / num << std::endl;

			sum = 0;
			num = 0;
			rxPkt = 0;
			txPkt = 0;
			delaysum = 0;
			for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i =
					stats.begin(); i != stats.end(); ++i) {
				delaysum = i->second.lostPackets
						/ (i->second.rxPackets + i->second.lostPackets);
				sum += delaysum;
				num++;
			}
			std::cout << "packet loss ratio = " << sum / num << std::endl;

			sum = 0;
			num = 0;
			rxPkt = 0;
			txPkt = 0;
			delaysum = 0;
			for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i =
					stats.begin(); i != stats.end(); ++i) {
				delaysum = i->second.lostPackets;
				sum += delaysum;
				num++;
			}
			std::cout << "packet loss = " << sum / num << std::endl;

			sum = 0;
			num = 0;
			rxPkt = 0;
			txPkt = 0;
			delaysum = 0;
			for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i =
					stats.begin(); i != stats.end(); ++i) {
				delaysum = i->second.txBytes;
				sum += delaysum;
				num++;
			}
			std::cout << "txBytes = " << sum / num << std::endl;

			sum = 0;
			num = 0;
			rxPkt = 0;
			txPkt = 0;
			delaysum = 0;
			for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i =
					stats.begin(); i != stats.end(); ++i) {
				std::cout << "------------------" << std::endl;
				std::cout << "timeFirstRxPacket " << i->second.timeFirstRxPacket
						<< std::endl;
				std::cout << "timeLastRxPacket " << i->second.timeLastRxPacket
						<< std::endl;
				std::cout << "timeFirstTxPacket " << i->second.timeFirstTxPacket
						<< std::endl;
				std::cout << "timeLastTxPacket " << i->second.timeLastTxPacket
						<< std::endl;
			}

		  Simulator::Destroy ();

		  //s
		  monitor->SerializeToXmlFile("flowmon.xml", true, true);

		  //sf uint32_t totalPacketsThrough = DynamicCast<UdpServer>(serverApp.Get (0))->GetReceived ();
		  //sf double throughput = totalPacketsThrough*payloadSize*8/(simulationTime*1000000.0);
		  // sf std::cout << "Throughput: " << throughput << " Mbit/s" << '\n';
	  }//for (int nMpdus = ...) {

  }//for (int inter = ...) {

  pdrA->Disable();
  jitterA->Disable();
  troughA->Disable();
  delayA->Disable();

    
  return 0;
}
