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
 * Author:  Tom Henderson (tomhend@u.washington.edu)
 */


/******************************************************/
/********     By Narges Ferasat Manesh  ***************/
/********         11 Mordad 94          ***************/
/********         Evalvid Client        ***************/
/******************************************************/

#include "ns3/address.h"
#include "ns3/address-utils.h"
#include "ns3/log.h"
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
#include "ns3/rtp-protocol.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/udp-socket.h"
#include "ns3/tcp-header.h"


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
    
    .AddAttribute ("ReceiverDumpFilename",
                   "Receiver Dump Filename",
                   StringValue(""),
                   MakeStringAccessor(&EvalvidClient::receiverDumpFileName),
                   MakeStringChecker())
    .AddAttribute ("SendSizeFileName",
                   "Send_Size Filename",
                   StringValue(""),
                   MakeStringAccessor(&EvalvidClient::m_sendSizeFileName),
                   MakeStringChecker())
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
    .AddTraceSource ("Rx",
                     "A packet has been received",
                     MakeTraceSourceAccessor (&EvalvidClient::m_rxTrace),
                     "ns3::Packet::PacketAddressTracedCallback")
   
  ;
  return tid;
}

EvalvidClient::EvalvidClient ()
{
    NS_LOG_FUNCTION (this);
    m_socket = 0;
    m_totalRx = 0;
    isfragment = false;
    m_received = 0;
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

Ptr<Socket>
EvalvidClient::GetListeningSocket (void) const
{
    NS_LOG_FUNCTION (this);
    return m_socket;
}

std::list<Ptr<Socket> >
EvalvidClient::GetAcceptedSockets (void) const
{
    NS_LOG_FUNCTION (this);
    return m_socketList;
}


uint32_t
EvalvidClient::GetReceived (void) const
{
  NS_LOG_FUNCTION (this);
  return m_received;
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
   // Create the socket if not already
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

    receiverDumpFile.open(receiverDumpFileName.c_str(), ios::out);
    if (receiverDumpFile.fail())
    {
      NS_FATAL_ERROR(">> EvalvidClient: Error while opening output file: " << receiverDumpFileName.c_str());
      return;
    }
    
   

}

void EvalvidClient::StopApplication ()     // Called at time specified by Stop
{
   NS_LOG_FUNCTION (this);
   while(!m_socketList.empty ()) //these are accepted sockets, close them
   {
      Ptr<Socket> acceptedSocket = m_socketList.front ();
      m_socketList.pop_front ();
      acceptedSocket->Close ();
   }
   if (m_socket) 
   {
      m_socket->Close ();
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
   }
   sendsizeFile.close();
   receiverDumpFile.close();
}

void EvalvidClient::HandleRead (Ptr<Socket> socket)
{
   NS_LOG_FUNCTION (this << socket);
   Ptr<Packet> packet;
   Address from;

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

                   receiverDumpFile << std::fixed << std::setprecision(4) << Simulator::Now().ToDouble(ns3::Time::S)
                                    << std::setfill(' ') << std::setw(16) <<  "id " << packetId
                                    << std::setfill(' ') <<  std::setw(16) <<  "Tcp " << size
                                    << std::endl;
                   std::cout<<Simulator::Now().GetSeconds()<<" r "<<packetId<<" "<< "Data "<<size<<" " << InetSocketAddress::ConvertFrom     (from).GetIpv4 ()<<"\n";
              }
        
              else if (Inet6SocketAddress::IsMatchingType (from))
              {
                  m_totalRx += packet->GetSize ();
                   m_received++;
                   SeqTsHeader seqTs;
                   packet->RemoveHeader (seqTs);
                   uint32_t packetId = seqTs.GetSeq (); 

                   std::cout<<Simulator::Now().GetSeconds()<<" r "<<packetId<<" "<< "Data "<<size<<" " << InetSocketAddress::ConvertFrom     (from).GetIpv4 ()<<"\n";
                   receiverDumpFile << std::fixed << std::setprecision(4) << Simulator::Now().ToDouble(ns3::Time::S)
                               << std::setfill(' ') << std::setw(16) <<  "id " << packetId
                               << std::setfill(' ') <<  std::setw(16) <<  "Tcp " << size
                               << std::endl;
              }
              m_rxTrace (packet, from);
              
        }
     
    }//end while
}


void EvalvidClient::HandlePeerClose (Ptr<Socket> socket)
{
    NS_LOG_FUNCTION (this << socket);
}
 
void EvalvidClient::HandlePeerError (Ptr<Socket> socket)
{
    NS_LOG_FUNCTION (this << socket);
}
 

void EvalvidClient::HandleAccept (Ptr<Socket> s, const Address& from)
{
    NS_LOG_FUNCTION (this << s << from);
    s->SetRecvCallback (MakeCallback (&EvalvidClient::HandleRead, this));
    m_socketList.push_back (s);
 
    //open packet sizes file
    sendsizeFile.open(m_sendSizeFileName.c_str(), ios::in);
    if (sendsizeFile.fail())
    {
        NS_FATAL_ERROR(">> EvalvidServer: Error while opening video sendsize file: " << m_sendSizeFileName.c_str());
        return;
    }
}




} // Namespace ns3toRead
