#include <fstream>
#include <string.h>

#include "ns3/lte-helper.h"
#include "ns3/epc-helper.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lte-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/config-store.h"
#include "ns3/evalvid-client-server-helper.h"
//#include "ns3/gtk-config-store.h"

using namespace ns3;

/**
 * Sample simulation script for LTE+EPC. It instantiates one eNodeB,
 * attaches one UE per eNodeB starts a video streaming flow from UE to a remote host.
 */
 NS_LOG_COMPONENT_DEFINE ("EpcEvalvidExample");
int
main (int argc, char *argv[])
{
  uint16_t numberOfNodes = 1;
  //double simTime = 5.0;
  double distance = 60.0;
  // Inter packet interval in ms
  //double interPacketInterval = 1;
  double interPacketInterval = 100;

  LogComponentEnable ("EvalvidClient", LOG_LEVEL_INFO);
  LogComponentEnable ("EvalvidServer", LOG_LEVEL_INFO);

  // Command line arguments
  CommandLine cmd;
  cmd.AddValue("numberOfNodes", "Number of eNodeBs + UE pairs", numberOfNodes);
  //cmd.AddValue("simTime", "Total duration of the simulation (in seconds)",simTime);
  cmd.Parse(argc, argv);

  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  Ptr<EpcHelper>  epcHelper = CreateObject<EpcHelper> ();
  lteHelper->SetEpcHelper (epcHelper);
  lteHelper->SetSchedulerType("ns3::PfFfMacScheduler");

  ConfigStore inputConfig;
  inputConfig.ConfigureDefaults();

  // parse again so you can override default values from the command line
  cmd.Parse(argc, argv);

  NS_LOG_INFO ("Create Remote host and PGW Nodes.");
   // Create a single RemoteHost
  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (1);
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);
  InternetStackHelper internet;
  internet.Install (remoteHostContainer);

  // Create the Internet
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.010)));
  Ptr<Node> pgw = epcHelper->GetPgwNode ();
  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
  NS_LOG_INFO("PGW Address: "<<internetIpIfaces.GetAddress(0)<<" RemoteHost Address: "<<internetIpIfaces.GetAddress(1));
  // interface 0 is localhost, 1 is the p2p device
  Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);

  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

  NS_LOG_INFO ("Create LTE Nodes.");
  NodeContainer ueNodes;
  NodeContainer enbNodes;
  enbNodes.Create(numberOfNodes);
  ueNodes.Create(numberOfNodes);

  NS_LOG_INFO ("Install Mobility Model");
  // Install Mobility Model
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  for (uint16_t i = 0; i < numberOfNodes; i++)
    {
      positionAlloc->Add (Vector(distance * i, 0, 0));
    }
  MobilityHelper mobility;
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.SetPositionAllocator(positionAlloc);
  mobility.Install(enbNodes);
  mobility.Install(ueNodes);

  NS_LOG_INFO ("Install LTE Devices to the nodes");
  // Install LTE Devices to the nodes
  NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice (enbNodes);
  NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice (ueNodes);

  NS_LOG_INFO ("Attach one UE per eNodeB");
  // Attach one UE per eNodeB
  for (uint16_t i = 0; i < numberOfNodes; i++)
      {
        lteHelper->Attach (ueLteDevs.Get(i), enbLteDevs.Get(i));
     }

  NS_LOG_INFO ("Install the IP stack on the UEs");
  // Install the IP stack on the UEs
  internet.Install (ueNodes);
  Ipv4InterfaceContainer ueIpIface;
  ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueLteDevs));

  NS_LOG_INFO ("Assign IP address to UEs, and install applications\nNumber of UE nodes are:"<<ueNodes.GetN ());
  // Assign IP address to UEs, and install applications
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
      Ptr<Node> ueNode = ueNodes.Get (u);
      NS_LOG_INFO ("Gateway Address is: "<<epcHelper->GetUeDefaultGatewayAddress ()<<" UE address is: "<<ueIpIface.GetAddress(0));
      // Set the default gateway for the UE
      Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
    }
  lteHelper->ActivateEpsBearer (ueLteDevs, EpsBearer (EpsBearer::NGBR_VIDEO_TCP_DEFAULT), EpcTft::Default ());

  NS_LOG_INFO ("Install and start applications on UEs and remote host");
  // Install and start applications on UEs and remote host
  uint16_t dlPort = 1234;
  uint16_t ulPort = 2000;
  //uint16_t otherPort = 3000;
  
  PacketSinkHelper dlPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), dlPort));
  PacketSinkHelper ulPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), ulPort));
  //PacketSinkHelper packetSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), otherPort));
  
  EvalvidServerHelper server (dlPort); // remote host
  EvalvidClientHelper client (remoteHostAddr,dlPort); // UE
  
  ApplicationContainer clientApps;
  ApplicationContainer serverApps;
  
  server.SetAttribute ("SendTraceFilename", StringValue("st_highway.st"));
  server.SetAttribute ("SendDumpFilename", StringValue("sd_highway"));
  serverApps = server.Install (remoteHostContainer.Get (0));
  //NS_LOG_INFO ("ulClient Address is: "<<remoteHostAddr<<" ulPort is: "<<ulPort);
  UdpClientHelper ulClient (remoteHostAddr, dlPort);
  ulClient.SetAttribute ("Interval", TimeValue (MilliSeconds(interPacketInterval)));
  ulClient.SetAttribute ("MaxPackets", UintegerValue(10000000));
 
  client.SetAttribute ("RecvDumpFilename", StringValue("rd_highway"));
  clientApps = client.Install (ueNodes.Get (0));
  //NS_LOG_INFO ("dlClient Address is: "<<ueIpIface.GetAddress(0)<<" dlPort is: "<<dlPort);
  UdpClientHelper dlClient (ueIpIface.GetAddress (0), dlPort);
  dlClient.SetAttribute ("Interval", TimeValue (MilliSeconds(interPacketInterval)));
  dlClient.SetAttribute ("MaxPackets", UintegerValue(10000000));

  serverApps.Start (Seconds (9));
  
  clientApps.Start (Seconds (10));
  clientApps.Stop (Seconds (59));

  serverApps.Stop (Seconds (60));

  //lteHelper->EnableTraces ();
  // Uncomment to enable PCAP tracing
  p2ph.EnablePcapAll("lena-simple-epc-evalvid");

  NS_LOG_INFO ("Run Simulation.");
  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");
  
  return 0;

}


