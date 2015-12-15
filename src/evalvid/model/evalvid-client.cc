/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright 2007 University of Washington
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
 * Author:  Narges Ferasat Manesh (n.ferasat@gmail.com)
 */


/******************************************************/
/********     By Narges Ferasat Manesh  ***************/
/********         11 Mordad 94          ***************/
/********         Evalvid Client        ***************/
/******************************************************/

#include "ns3/address.h"
#include "ns3/address-utils.h"
#include "ns3/log.h"
#include "ns3/tag.h"
#include "ns3/snr-tag.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/node.h"
#include "ns3/socket.h"
#include "ns3/tcp-socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/ipv4-address.h"
#include "ns3/nstime.h"
#include "ns3/uinteger.h"
#include "evalvid-client.h"
#include <stdlib.h>
#include <stdio.h>
#include "ns3/string.h"
#include "ns3/header.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/udp-socket.h"
#include "ns3/tcp-header.h"
#include "ns3/udp-client.h"
#include "ns3/qos-tag.h"
#include "ns3/double.h"



namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("EvalvidClient");
NS_OBJECT_ENSURE_REGISTERED (EvalvidClient);

TypeId 
EvalvidClient::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::EvalvidClient")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<EvalvidClient> ()
    .AddAttribute ("Local",
                   "The Address on which to Bind the rx socket.",
                   AddressValue (),
                   MakeAddressAccessor (&EvalvidClient::m_local),
                   MakeAddressChecker ())
    .AddAttribute ("Protocol",
                   "The type id of the protocol to use for the rx socket.",
                   TypeIdValue (TcpSocketFactory::GetTypeId ()),
                   MakeTypeIdAccessor (&EvalvidClient::m_tid),
                   MakeTypeIdChecker ())
    .AddAttribute ("ClientId", "The Client's Id",
                   StringValue("1"),
                   MakeStringAccessor(&EvalvidClient::m_clientId),
                   MakeStringChecker())
    .AddAttribute ("PacketWindowSize",
                   "The size of the window used to compute the packet loss. This value should be a multiple of 8.",
                   UintegerValue (32),
                   MakeUintegerAccessor (&EvalvidClient::SetPacketWindowSize),
                   MakeUintegerChecker<uint16_t> (8,256))
    .AddAttribute ("Deadline",
                   "Deadline (Maximum Tolerable Delay) of each packet in seconds.", DoubleValue (1.0),
                    MakeDoubleAccessor (&EvalvidClient::m_deadline),
                    MakeDoubleChecker<double> ())
    .AddTraceSource ("Rx",
                     "A packet has been received",
                     MakeTraceSourceAccessor (&EvalvidClient::m_rxTrace),
                     "ns3::Packet::PacketAddressTracedCallback")
   
  ;
  return tid;
}

EvalvidClient::EvalvidClient ()
: m_lossCounter (0)
{
    NS_LOG_FUNCTION (this);
    m_socket = 0;
    m_totalRx = 0;
    isfragment = false;
    m_received = 0;
    m_connected=false;
}

EvalvidClient::~EvalvidClient()
{
    NS_LOG_FUNCTION (this);
}

uint32_t EvalvidClient::GetTotalRx () const
{
    NS_LOG_FUNCTION (this);
    return m_totalRx;
}

Ptr<Socket> EvalvidClient::GetListeningSocket (void) const
{
    NS_LOG_FUNCTION (this);
    return m_socket;
}

std::list<Ptr<Socket> > EvalvidClient::GetAcceptedSockets (void) const
{
    NS_LOG_FUNCTION (this);
    return m_socketList;
}


uint32_t EvalvidClient::GetReceived (void) const
{
    NS_LOG_FUNCTION (this);
    return m_received;
}

uint32_t EvalvidClient::GetLost (void) const
{
    NS_LOG_FUNCTION (this);
    return m_lossCounter.GetLost ();
}

void EvalvidClient::SetPacketWindowSize (uint16_t size)
{
    NS_LOG_FUNCTION (this << size);
    m_lossCounter.SetBitMapSize (size);
}

void EvalvidClient::DoDispose (void)
{
    NS_LOG_FUNCTION (this);
    m_socket = 0;
    m_socketList.clear ();

    // chain up
    Application::DoDispose ();
}


// Application Methods
void EvalvidClient::StartApplication ()    // Called at time specified by Start
{
    NS_LOG_FUNCTION (this);
    
    // Create the UDP socket if not already    
    if (m_tid == UdpSocketFactory::GetTypeId ())
    {
        if (!m_socket)
        {
             m_socket = Socket::CreateSocket (GetNode (), m_tid);
             m_socket->Bind (m_local);
        }
        m_socket->SetRecvCallback (MakeCallback (&EvalvidClient::HandleRead, this)); 
     }

    // Create the TCP socket if not already 
    else 
    {
         if (!m_socket)
         {
             m_socket = Socket::CreateSocket (GetNode (), m_tid);
             m_socket->Bind (m_local);
             m_socket->Listen ();
             m_socket->ShutdownSend ();
             if (addressUtils::IsMulticast (m_local))
             {
                 Ptr<UdpSocket> udpSocket = DynamicCast<UdpSocket> (m_socket);
                 if (udpSocket)
                 {
                      // equivalent to setsockopt (MCAST_JOIN_GROUP)
                      udpSocket->MulticastJoinGroup (0, m_local);
                  }
                  else
                  {
                       NS_FATAL_ERROR ("Error: joining multicast on a non-UDP socket");
                  }
              }
          }
          m_socket->SetRecvCallback (MakeCallback (&EvalvidClient::HandleRead, this));
          m_socket->SetAcceptCallback (
          MakeNullCallback<bool, Ptr<Socket>, const Address &> (),
          MakeCallback (&EvalvidClient::HandleAccept, this));
          m_socket->SetCloseCallbacks (
          MakeCallback (&EvalvidClient::HandlePeerClose, this),
          MakeCallback (&EvalvidClient::HandlePeerError, this));
      }

   
      receiverDumpFileName = "NF-results/output/receiver-output";
      receiverDumpFileName += m_clientId;
      receiverDumpFile.open(receiverDumpFileName.c_str(), ios::out);
      if (receiverDumpFile.fail())
      {
          NS_FATAL_ERROR(">> EvalvidClient: Error while opening output file: " << receiverDumpFileName.c_str());
          return;
      }

    /*  string filename = "NF-results/output/receiver-out";
      filename += m_clientId;   
      output.open(filename.c_str(), ios::out);*/
     
      //OpenSendsizefile();

}

void EvalvidClient::OpenSendsizefile()
{
      m_sendSizeFileName = "NF-results/output/send_size";
      m_sendSizeFileName += m_clientId;
      //open packet sizes file
      sendsizeFile.open(m_sendSizeFileName.c_str(), ios::in);
      if (sendsizeFile.fail())
      {
          m_connected=false;
      }
      else
          m_connected=true;
}

void EvalvidClient::StopApplication ()     // Called at time specified by Stop
{
    NS_LOG_FUNCTION (this);

    if (m_socket) 
    {
       m_socket->Close ();
       m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    }
    while(!m_socketList.empty ()) //these are accepted sockets, close them
    {
       Ptr<Socket> acceptedSocket = m_socketList.front ();
       m_socketList.pop_front ();
       acceptedSocket->Close ();
    }
   
    sendsizeFile.close();
    receiverDumpFile.close();
    output.close();
}


//for TCP or UDP packets
void EvalvidClient::HandleRead (Ptr<Socket> socket)
{
   

    NS_LOG_FUNCTION (this << socket);
    Ptr<Packet> packet;
    Address from;

    
    //Read TCP packets
    if (m_tid == TcpSocketFactory::GetTypeId ())
    {   
        while ( m_connected==false)
            OpenSendsizefile();

        while (socket->GetRxAvailable () > 0) 
        {
            uint32_t toRead = 0;
     
            if (isfragment == false)  //Since there is not incompatible packet, receive a new packet
            {
                sendsizeFile >> toRead;
                if (sendsizeFile.eof())
                    break;
                packet = socket->RecvFrom (toRead ,0 ,from);
                if (packet->GetSize () < toRead)
                {
                    isfragment = true;
                    incompletePacket = packet;
                    fragment = toRead - packet->GetSize ();
                    continue;
                }
            }
            else 
            {
                isfragment = false; 
                packet = socket->RecvFrom (fragment ,0 ,from);
                incompletePacket->AddAtEnd (packet);
                packet = incompletePacket;
            }
             
     
            if (packet->GetSize () == 0)
            { //EOF
                break;
            }

  
            if (packet->GetSize () > 0)
            {
                uint32_t size=packet->GetSize();
                if (InetSocketAddress::IsMatchingType (from))
                {
                    m_totalRx += packet->GetSize ();
                    m_received++;
                    SeqTsHeader seqTs;
                    packet->RemoveHeader (seqTs);
                    uint32_t packetId = seqTs.GetSeq (); 

                    /*TimestampTag tsTag;
                    packet->RemovePacketTag (tsTag);
                    QosTag qosTag;
                    packet->RemovePacketTag (qosTag);
                    QosTag qosTag;
                    packet->FindFirstMatchingByteTag(qosTag);*/
                   

                    receiverDumpFile << std::fixed << std::setprecision(4) << Simulator::Now().ToDouble(ns3::Time::S)
                                     << std::setfill(' ') << std::setw(16) <<  "id " << packetId
                                     << std::setfill(' ') <<  std::setw(16) <<  "Tcp " << size+12 //<<"\t"<< (int)qosTag.GetTid()
                                     << std::endl;
                    
                }
        
                 m_rxTrace (packet, from);
              
             }
      
         }//end while
     }//end if(m_tid ==...)


     // Read UDP packets
     else if (m_tid == UdpSocketFactory::GetTypeId ())
     {
         // Address from;
         while ((packet = socket->RecvFrom (from)))
         {
              if (InetSocketAddress::IsMatchingType (from))
              {
                  if (packet->GetSize () > 0)
                   {
                        m_received++;
                        SeqTsHeader seqTs;
                        packet->RemoveHeader (seqTs);
                        uint32_t packetId = seqTs.GetSeq ();
                        uint32_t currentSequenceNumber = seqTs.GetSeq ();
                        TimestampTag tsTag;
                        packet->FindFirstMatchingByteTag(tsTag);
                        QosTag qosTag;
                        packet->RemovePacketTag (qosTag);
                     
                 
                   //   double delay=Simulator::Now().ToDouble(ns3::Time::S) - seqTs.GetTs ().ToDouble(ns3::Time::S);
                  
                    //  if (delay <= m_deadline)
                        if (packetId > 0)
                        {

                              receiverDumpFile << std::fixed << std::setprecision(4) << Simulator::Now().ToDouble(ns3::Time::S)
                                               << std::setfill(' ') << std::setw(16) <<  "id " << packetId
                                               << std::setfill(' ') <<  std::setw(16) <<  "udp " << (packet->GetSize()+12)//<<"\t"<<packet->GetUid()
                                               << std::endl;
                         }
                     
              
                         m_lossCounter.NotifyReceived (currentSequenceNumber);
                         m_rxTrace (packet, from);
                       
                   }
               }
          }//end while
     }//end if (m_tid=...

}


//for TCP
void EvalvidClient::HandlePeerClose (Ptr<Socket> socket)
{
    NS_LOG_FUNCTION (this << socket);
}

//for TCP 
void EvalvidClient::HandlePeerError (Ptr<Socket> socket)
{
    NS_LOG_FUNCTION (this << socket);
}
 
//for TCP
void EvalvidClient::HandleAccept (Ptr<Socket> s, const Address& from)
{
    NS_LOG_FUNCTION (this << s << from);
    s->SetRecvCallback (MakeCallback (&EvalvidClient::HandleRead, this));
    m_socketList.push_back (s); 
}

} // Namespace ns3toRead
