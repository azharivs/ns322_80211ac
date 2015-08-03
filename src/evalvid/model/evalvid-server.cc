/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 Georgia Institute of Technology
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
 * Author: George F. Riley <riley@ece.gatech.edu>
 */

#include "ns3/log.h"
#include "ns3/address.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/ipv4-address.h"
#include "ns3/inet-socket-address.h"
#include "ns3/string.h"
#include "evalvid-server.h"
#include "ns3/rtp-protocol.h"
#include "ns3/tcp-socket.h"




using namespace std;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("EvalvidServer");
NS_OBJECT_ENSURE_REGISTERED (EvalvidServer);

TypeId
EvalvidServer::GetTypeId (void)
{
 static TypeId tid = TypeId ("ns3::EvalvidServer")
    .SetParent<Application> ()
    .AddConstructor<EvalvidServer> ()
    .AddAttribute ("SenderDumpFilename",
                   "Sender Dump Filename",
                   StringValue(""),
                   MakeStringAccessor(&EvalvidServer::m_senderTraceFileName),
                   MakeStringChecker())
    .AddAttribute ("SendSizeFileName",
                   "Send Size Filename",
                   StringValue(""),
                   MakeStringAccessor(&EvalvidServer:: m_sendsizeFilename),
                   MakeStringChecker())
    .AddAttribute ("SenderTraceFilename",
                   "Sender trace Filename",
                   StringValue(""),
                   MakeStringAccessor(&EvalvidServer::m_videoTraceFileName),
                   MakeStringChecker())
    .AddAttribute ("SendSize", "The amount of data to send each time.",
                   UintegerValue (500),
                   MakeUintegerAccessor (&EvalvidServer::m_sendSize),
                   MakeUintegerChecker<uint32_t> (1))
    .AddAttribute ("Remote", "The address of the destination",
                   AddressValue (),
                   MakeAddressAccessor (&EvalvidServer::m_peer),
                   MakeAddressChecker ())
    .AddAttribute ("MaxBytes",
                   "The total number of bytes to send. "
                   "Once these bytes are sent, "
                   "no data  is sent again. The value zero means "
                   "that there is no limit.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&EvalvidServer::m_maxBytes),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Protocol", "The type of protocol to use.",
                   TypeIdValue (TcpSocketFactory::GetTypeId ()),
                   MakeTypeIdAccessor (&EvalvidServer::m_tid),
                   MakeTypeIdChecker ())
    /*.AddAttribute ("FlowId", "The Flow's Id",
                   UintegerValue (1),
                   MakeUintegerAccessor (&EvalvidServer::m_flowId),
                   MakeUintegerChecker<uint32_t> (1))*/
    .AddTraceSource ("Tx", "A new packet is created and is sent",
                     MakeTraceSourceAccessor (&EvalvidServer::m_txTrace),
                     "ns3::Packet::TracedCallback")
  ;

  return tid;
}


EvalvidServer::EvalvidServer ()
  : m_socket (0),
    m_connected (false),
    m_totBytes (0)
{
   NS_LOG_FUNCTION (this);
   NS_LOG_FUNCTION (this);
   m_numOfFrames = 0;
   m_packetPayload = 0;
   m_packetId = 0;
}

EvalvidServer::~EvalvidServer ()
{
   NS_LOG_FUNCTION (this);
   m_sendFile.close();
}

void
EvalvidServer::SetMaxBytes (uint32_t maxBytes)
{
   NS_LOG_FUNCTION (this << maxBytes);
   m_maxBytes = maxBytes;
}

Ptr<Socket>
EvalvidServer::GetSocket (void) const
{
   NS_LOG_FUNCTION (this);
   return m_socket;
}

void
EvalvidServer::DoDispose (void)
{
   NS_LOG_FUNCTION (this);

   m_socket = 0;
   // chain up
   Application::DoDispose ();
}

// Application Methods
void EvalvidServer::StartApplication (void) // Called at time specified by Start
{
   NS_LOG_FUNCTION (this);

   // Create the socket if not already
   if (!m_socket)
   {
       m_socket = Socket::CreateSocket (GetNode (), m_tid);

       // Fatal error if socket type is not NS3_SOCK_STREAM or NS3_SOCK_SEQPACKET
       if (m_socket->GetSocketType () != Socket::NS3_SOCK_STREAM &&
          m_socket->GetSocketType () != Socket::NS3_SOCK_SEQPACKET)
       {
           NS_FATAL_ERROR ("Using BulkSend with an incompatible socket type. "
                           "BulkSend requires SOCK_STREAM or SOCK_SEQPACKET. "
                           "In other words, use TCP instead of UDP.");
       }

       if (Inet6SocketAddress::IsMatchingType (m_peer))
       {
           m_socket->Bind6 ();
       }
       else if (InetSocketAddress::IsMatchingType (m_peer))
       {
           m_socket->Bind ();
       }
 
       m_socket->Connect (m_peer);
       m_socket->ShutdownRecv ();
       m_socket->SetConnectCallback (
       MakeCallback (&EvalvidServer::ConnectionSucceeded, this),
       MakeCallback (&EvalvidServer::ConnectionFailed, this));
       m_socket->SetSendCallback (
       MakeCallback (&EvalvidServer::DataSend, this));
     
    }
  
    Setup();
    if (m_connected)
    {
        SendData ();
    }

}

void EvalvidServer::StopApplication (void) // Called at time specified by Stop
{
    NS_LOG_FUNCTION (this);

    if (m_socket != 0)
    {
        m_socket->Close ();
        m_connected = false;
    }
    else
    {
        NS_LOG_WARN ("EvalvidServer found null socket to close in StopApplication");
    }
}

void 
EvalvidServer::Connect()
{

    m_connected = true;
    m_socket->Connect (m_peer);

}


void
EvalvidServer::Setup()
{
    NS_LOG_FUNCTION_NOARGS();

    m_videoInfoStruct_t *videoInfoStruct;
    uint32_t frameId;
    string frameType;
    uint32_t frameSize;
    uint16_t numOfTcpPackets;
    double sendTime;
    double lastSendTime = 0.0;

    //Open file from mp4trace tool of EvalVid.
    ifstream videoTraceFile(m_videoTraceFileName.c_str(), ios::in);
    if (videoTraceFile.fail())
    {
        NS_FATAL_ERROR(">> EvalvidServer: Error while opening video trace file: " << m_videoTraceFileName.c_str());
        return;
    }


    //Store video trace information on the struct
    while (videoTraceFile >> frameId >> frameType >> frameSize >> numOfTcpPackets >> sendTime)
    {
        videoInfoStruct = new m_videoInfoStruct_t;
        videoInfoStruct->frameType = frameType;
        videoInfoStruct->frameSize = frameSize;
        videoInfoStruct->numOfTcpPackets = numOfTcpPackets;
        videoInfoStruct->packetInterval = Seconds(sendTime - lastSendTime);
        m_videoInfoMap.insert (pair<uint32_t, m_videoInfoStruct_t*>(frameId, videoInfoStruct));
        NS_LOG_LOGIC(">> EvalvidServer: " << frameId << "\t" << frameType << "\t" <<
                                frameSize << "\t" << numOfTcpPackets << "\t" << sendTime);
        lastSendTime = sendTime;
    }

    m_numOfFrames = frameId;
    m_videoInfoMapIt = m_videoInfoMap.begin();


    //Open file to store information of packets transmitted by EvalvidServer.
    m_senderTraceFile.open(m_senderTraceFileName.c_str(), ios::out);
    if (m_senderTraceFile.fail())
    {
        NS_FATAL_ERROR(">> EvalvidServer: Error while opening sender trace file: " << m_senderTraceFileName.c_str());
        return;
    }
 
  
    m_sendsizeFile.open(m_sendsizeFilename.c_str(), ios::out);
    if (m_sendsizeFile.fail())
    {
        NS_FATAL_ERROR(">> EvalvidServer: Error while opening sender sendsize file: " << m_sendsizeFilename.c_str());
        return;
    }


    // calculate size of packets and store them in file
    uint32_t j = 1;
    uint32_t toSend = m_sendSize-12;
    while (m_videoInfoMapIt != m_videoInfoMap.end())
    {
        for(int i = 0; i < m_videoInfoMapIt->second->numOfTcpPackets ; i++)
        {
            m_sendsizeFile << toSend+12 <<std::endl;
            j++;
        }

        uint32_t s = m_videoInfoMapIt->second->frameSize % toSend;
        if (s > 0)
            m_sendsizeFile << s+12 <<std::endl;
        m_videoInfoMapIt++;

        j++;
     }
       
     m_sendsizeFile.close();

     //Open above file for load size of packets
     m_sendFile.open(m_sendsizeFilename.c_str(), ios::in);
     if (m_sendFile.fail())
     {
         NS_FATAL_ERROR(">> EvalvidServer: Error while opening sender sendsize file: " << m_sendsizeFilename.c_str());
         return;
     }

     
     m_videoInfoMapIt = m_videoInfoMap.begin();
}

/* This is the core method of the class, responsible for creating and
   * sending data packets toward the receiver node.  */
void EvalvidServer::SendData (void)
{
    uint32_t toSend = 0;
    uint32_t size = 0;
   
    NS_LOG_FUNCTION (this);
    NS_LOG_FUNCTION( this << Simulator::Now().GetSeconds());

    if (m_connected == false)
       return;

    //While arrive the end of m_sendFile, read one size for create packet and send it
    while (!m_sendFile.eof())     
    {
        int pos = m_sendFile.tellg();   // save current position
        m_sendFile >> toSend ;          // read size for create new packet
        if (m_sendFile.eof())
           break;
        Ptr<Packet> p = Create<Packet> ((toSend-12)); // 12 is size seqTs (SeqTsHeader)
        m_packetId++;
       
        SeqTsHeader seqTs;
        seqTs.SetSeq (m_packetId);
        p->AddHeader (seqTs);
         
        size = m_socket->Send(p);

        // Unsuccess send
        if ((unsigned)size != (toSend))
        {       
             m_sendFile.seekg(pos);
             m_packetId--;
             break;
        }

        //if success send,it save information of packets in output file 
        m_senderTraceFile << std::fixed << std::setprecision(4) << Simulator::Now().ToDouble(Time::S)
                            << std::setfill(' ') << std::setw(16) <<  "id " << m_packetId
                            << std::setfill(' ') <<  std::setw(16) <<  "Tcp " << p->GetSize()
                            << std::endl;
        std::cout<<Simulator::Now().GetSeconds()<<" s "<<m_packetId<<" "<< "Data "<<p->GetSize()<<" " << InetSocketAddress::ConvertFrom(m_peer).GetIpv4 ()<<"\n";
       
    }  

    if (m_sendFile.eof() && m_connected)
    {
         m_socket->Close ();
         m_connected = false;
        //NS_FATAL_ERROR(">> EvalvidServer: Frame does not exist!");
    }

}

void EvalvidServer::ConnectionSucceeded (Ptr<Socket> socket)
{
    NS_LOG_FUNCTION (this << socket);
    NS_LOG_LOGIC ("EvalvidServer Connection succeeded");
    m_connected = true;
    SendData ();
}

void EvalvidServer::ConnectionFailed (Ptr<Socket> socket)
{
    NS_LOG_FUNCTION (this << socket);
    NS_LOG_LOGIC ("EvalvidServer, Connection Failed");
}

void EvalvidServer::DataSend (Ptr<Socket>, uint32_t)
{
    NS_LOG_FUNCTION (this);

    if (m_connected)
    { // Only send new data if the connection has completed
        Simulator::ScheduleNow (&EvalvidServer::SendData, this);
    }
}


} // Namespace ns3