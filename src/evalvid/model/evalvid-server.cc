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
 * Author:  Narges Ferasat Manesh (n.ferasat@gmail.com)
 */

/******************************************************/
/********     By Narges Ferasat Manesh  ***************/
/********         11 Mordad 94          ***************/
/********         Evalvid server        ***************/
/******************************************************/

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
#include "ns3/udp-socket-factory.h"
#include "ns3/ipv4-address.h"
#include "ns3/inet-socket-address.h"
#include "ns3/string.h"
#include "evalvid-server.h"
#include "ns3/double.h"
#include "ns3/tcp-socket.h"
#include "ns3/timestamp-tag.h"
#include "ns3/qos-tag.h"
#include "ns3/seq-ts-header.h"



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
    .AddAttribute ("Interval",
                   "The time to wait between packets", TimeValue (Seconds (1.0)),
                   MakeTimeAccessor (&EvalvidServer::m_interval),
                   MakeTimeChecker ())
    .AddAttribute ("ServerId", "The Server's Id",
                   StringValue("1"),
                   MakeStringAccessor(&EvalvidServer::m_serverId),
                   MakeStringChecker())
    .AddTraceSource ("Tx", "A new packet is created and is sent",
                     MakeTraceSourceAccessor (&EvalvidServer::m_txTrace),
                     "ns3::Packet::TracedCallback")
    .AddAttribute ("Deadline",
                   "Deadline (Maximum Tolerable Delay) of each packet in seconds.", DoubleValue (1.0),
                    MakeDoubleAccessor (&EvalvidServer::m_deadline),
                    MakeDoubleChecker<double> ())
     
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
   m_sendEvent = EventId ();
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

   // Create the UDP socket if not already 
   if (m_tid == UdpSocketFactory::GetTypeId ())
   {
        if (!m_socket)
        { 
            m_socket = Socket::CreateSocket (GetNode (), m_tid);
            m_socket->Bind ();
            m_socket->Connect (m_peer);
         //  m_connected = true;
        }
        m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
        m_sendEvent=Simulator::Schedule (Seconds (0.001), &EvalvidServer::SendData, this);
           
    }
    // Create the TCP socket if not already 
    else 
    {
        if (!m_socket)
        { 
            m_socket = Socket::CreateSocket (GetNode (), m_tid);
            // Fatal error if socket type is not NS3_SOCK_STREAM or NS3_SOCK_SEQPACKET
            if (m_socket->GetSocketType () != Socket::NS3_SOCK_STREAM &&
                m_socket->GetSocketType () != Socket::NS3_SOCK_SEQPACKET)
            {
                   NS_FATAL_ERROR ("Using Evalvid with an incompatible socket type. "
                                   "Evalvid requires SOCK_STREAM or SOCK_SEQPACKET. "
                                   "In other words, use TCP instead of UDP.");
             }

             if (InetSocketAddress::IsMatchingType (m_peer))
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
          }//endif(!m_socket)
             
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
    Simulator::Cancel (m_sendEvent);

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
EvalvidServer::SetDeadline (double deadline)
{
  m_deadline = deadline;
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
        videoInfoStruct->packetInterval = sendTime - lastSendTime;
        m_videoInfoMap.insert (pair<uint32_t, m_videoInfoStruct_t*>(frameId, videoInfoStruct));
        NS_LOG_LOGIC(">> EvalvidServer: " << frameId << "\t" << frameType << "\t" <<
                                frameSize << "\t" << numOfTcpPackets << "\t" << sendTime);
        lastSendTime = sendTime;
    }

    m_numOfFrames = frameId;
    m_videoInfoMapIt = m_videoInfoMap.begin();


    m_senderTraceFileName = "NF-results/output/sender-output";
    m_senderTraceFileName += m_serverId;
    
    //Open file to store information of packets transmitted by EvalvidServer.
    m_senderTraceFile.open(m_senderTraceFileName.c_str(), ios::out);
    if (m_senderTraceFile.fail())
    {
        NS_FATAL_ERROR(">> EvalvidServer: Error while opening sender trace file: " << m_senderTraceFileName.c_str());
        return;
    }
 
  
    //create file for save size of packets
    m_sendsizeFilename = "NF-results/output/send_size";
    m_sendsizeFilename += m_serverId;
    m_sendsizeFile.open(m_sendsizeFilename.c_str(), ios::out);
    if (m_sendsizeFile.fail())
    {
         NS_FATAL_ERROR(">> EvalvidServer: Error while opening sender sendsize file: " << m_sendsizeFilename.c_str());
         return;
    }


    // calculate size of packets and store them in file
    uint32_t toSend = m_sendSize-12;
    while (m_videoInfoMapIt != m_videoInfoMap.end())
    {
         for(int i = 0; i < m_videoInfoMapIt->second->numOfTcpPackets-1; i++)
         {
              m_sendsizeFile << toSend+12  <<"\t"<< 0<<std::endl;
         }

         uint32_t s = m_videoInfoMapIt->second->frameSize % toSend;
        
          m_videoInfoMapIt++;
         m_sendsizeFile << s+12 <<"\t";
         if (m_videoInfoMapIt != m_videoInfoMap.end())
               m_sendsizeFile<<m_videoInfoMapIt->second->packetInterval+0.001 <<std::endl;
         else
               m_sendsizeFile<<"0"<<std::endl;
         
        
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

    /*string filename = "NF-results/output/sender-out";
    filename += m_serverId;
    output.open(filename.c_str(), ios::out);*/

}


void
EvalvidServer::Send ()
{
  /* NS_LOG_FUNCTION( this << Simulator::Now().GetSeconds());
   uint32_t toSend = m_sendSize-12;
   if (m_videoInfoMapIt != m_videoInfoMap.end())
   {
      //Sending the frame in multiples segments
      for(int i=0; i<m_videoInfoMapIt->second->numOfTcpPackets - 1; i++)
      {
          Ptr<Packet> p = Create<Packet> (toSend);
          m_packetId++;

          SeqTsHeader seqTs;
          seqTs.SetSeq (m_packetId);
          p->AddHeader (seqTs);
           
          //add QoS tag to have it treated as AC_VI
          QosTag tag=QosTag(UP_VI);
          p->AddPacketTag(tag);
       
          //add deadline as absolute timestamp
          TimestampTag tsTag;
          tsTag.SetTimestamp(Simulator::Now() + Seconds(m_deadline));
          p->AddByteTag(tsTag);

          m_socket->SendTo(p, 0, m_peer);
             
         //Unsuccess send
        
         {
            

             if (InetSocketAddress::IsMatchingType (m_peer) )
             {
                 // peerAddressStringStream << Ipv4Address::ConvertFrom (m_peer);
                  m_txTrace(p);
               
                  m_senderTraceFile << std::fixed << std::setprecision(4) << Simulator::Now().ToDouble(Time::S)
                                    << std::setfill(' ') << std::setw(16) <<  "id " << m_packetId
                                    << std::setfill(' ') <<  std::setw(16) <<  "udp " << p->GetSize()
                                    << std::endl;
                  //std::cout<< Simulator::Now().GetSeconds() <<" s "<< m_packetId <<" "<< "Data "<< p->GetSize() <<" " << InetSocketAddress::ConvertFrom(m_peer).GetIpv4 ()<< " udp \n";

                //  output<< Simulator::Now().GetSeconds() <<" s "<< m_packetId <<" "<< "Data "<< p->GetSize() <<" " << InetSocketAddress::ConvertFrom(m_peer).GetIpv4 ()<< " udp \n";

             } 

           }//end else 
         
         


      }

      //Sending the rest of the frame
      Ptr<Packet> p = Create<Packet> (m_videoInfoMapIt->second->frameSize % toSend);
      m_packetId++;

     

     

      SeqTsHeader seqTs;
          seqTs.SetSeq (m_packetId);
          p->AddHeader (seqTs);
           
          //add QoS tag to have it treated as AC_VI
          QosTag tag=QosTag(UP_VI);
          p->AddPacketTag(tag);
       
          //add deadline as absolute timestamp
          TimestampTag tsTag;
          tsTag.SetTimestamp(Simulator::Now() + Seconds(m_deadline));
          p->AddByteTag(tsTag);

          m_socket->SendTo(p, 0, m_peer);
           if (InetSocketAddress::IsMatchingType (m_peer) )
           {
                 // peerAddressStringStream << Ipv4Address::ConvertFrom (m_peer);
                  m_txTrace(p);

          m_senderTraceFile << std::fixed << std::setprecision(4) << Simulator::Now().ToDouble(Time::S)
                        << std::setfill(' ') << std::setw(16) <<  "id " << m_packetId
                        << std::setfill(' ') <<  std::setw(16) <<  "udp " << p->GetSize()
                        << std::endl;         
           }
 

      m_videoInfoMapIt++;
      if (m_videoInfoMapIt == m_videoInfoMap.end())
      {
         // NS_LOG_INFO(">> EvalvidServer: Video streaming successfully completed!");
      }
      else
      {
          if (m_videoInfoMapIt->second->packetInterval.GetSeconds() == 0)
          {
              m_sendEvent = Simulator::ScheduleNow (&EvalvidServer::Send, this);
          }
          else
          {
              m_sendEvent = Simulator::Schedule (m_videoInfoMapIt->second->packetInterval,
                                                 &EvalvidServer::Send, this);
          }
        }
    }*/
  
}

/* This is the core method of the class, responsible for creating and
   * sending data packets toward the receiver node.   (for TCP or UDP sockets)*/
void EvalvidServer::SendData (void)
{
    
    uint32_t toSend = 0;
    uint32_t size = 0;
    Time p_Interval ;
   // std::stringstream peerAddressStringStream;
    NS_LOG_FUNCTION (this);
    NS_LOG_FUNCTION( this << Simulator::Now().GetSeconds());


    //TCP send
    if (m_tid == TcpSocketFactory::GetTypeId ())
    {

        //While arrive the end of m_sendFile, read one size for create packet and send it
        while (!m_sendFile.eof())     
        {
            int pos = m_sendFile.tellg();               // save current position
            m_sendFile >> toSend >>p_Interval;          // read size for create new packet
            if (m_sendFile.eof())
               break;
            Ptr<Packet> p = Create<Packet> ((toSend-12)); // 12 is size seqTs (SeqTsHeader)
            m_packetId++;
       
            SeqTsHeader seqTs;
            seqTs.SetSeq (m_packetId);
            p->AddHeader (seqTs);

        
            //add QoS tag to have it treated as AC_VI
            QosTag tag=QosTag(UP_VI);
            p->AddByteTag(tag);
            //add deadline as absolute timestamp
            TimestampTag tsTag;
            tsTag.SetTimestamp(Simulator::Now() + Seconds(m_deadline));
            p->AddByteTag(tsTag);
         
            size = m_socket->Send(p);
      
            // Unsuccess send
            if ((unsigned)size != (toSend))
            {       
                m_sendFile.seekg(pos);
                m_packetId--;
                break;
            }
            m_txTrace(p);

            //if success send,it save information of packets in output file 
            m_senderTraceFile << std::fixed << std::setprecision(4) << Simulator::Now().ToDouble(Time::S)
                              << std::setfill(' ') << std::setw(16) <<  "id " << m_packetId
                              << std::setfill(' ') <<  std::setw(16) <<  "Tcp " << p->GetSize()<<"\t" /*<<(int)tag.GetTid()*/
                              << std::endl;
            std::cout<<Simulator::Now().GetSeconds()<<" s "<<m_packetId<<" "<< "Data "<<p->GetSize()<<" " << InetSocketAddress::ConvertFrom(m_peer).GetIpv4 ()<<" TCP "<<"\n";
       
        }
  
        if (m_sendFile.eof() && m_connected)
        {
            m_socket->Close ();
            m_connected = false;
        
        }
     }   
        
     
     // UDP send
     else if (m_tid == UdpSocketFactory::GetTypeId ())
     {
         int pos = m_sendFile.tellg();   
         //float last_p_Interval=p_Interval;
         m_sendFile >> toSend >>p_Interval;          // read size for create new packet
         if (m_sendFile.eof())
              return;
         Ptr<Packet> p = Create<Packet> ((toSend-12)); // 12 is size seqTs (SeqTsHeader)
         m_packetId++;
         SeqTsHeader seqTs;
         seqTs.SetSeq (m_packetId);
         p->AddHeader (seqTs);
           
         //add QoS tag to have it treated as AC_VI
         QosTag tag=QosTag(UP_VI);
         p->AddPacketTag(tag);
       
         //add deadline as absolute timestamp
         TimestampTag tsTag;
         tsTag.SetTimestamp(Simulator::Now() + Seconds(m_deadline));
         p->AddByteTag(tsTag);
             
         //Unsuccess send
         if (m_socket->Send(p) < 0)
         {    
             std::cout << "\n" << m_packetId << " Send failed !!!!!!!!!\n";  
             m_sendFile.seekg(pos);
             m_packetId--;
             //p_Interval=last_p_Interval;
         }
         else
         {
            

             if (InetSocketAddress::IsMatchingType (m_peer))
             {
                
                  m_txTrace(p);
                 
                  m_senderTraceFile << std::fixed << std::setprecision(4) << Simulator::Now().ToDouble(Time::S)
                                    << std::setfill(' ') << std::setw(16) <<  "id " << m_packetId
                                    << std::setfill(' ') <<  std::setw(16) <<  "udp " << p->GetSize()
                                    << std::endl;

                  //  output<< Simulator::Now().GetSeconds() <<" s "<< m_packetId <<" "<< "Data "<< p->GetSize() <<" " << InetSocketAddress::ConvertFrom(m_peer).GetIpv4 ()<< " udp \n";

             } 

         }//end else 
         
         if (p_Interval == 0)    
               m_sendEvent=Simulator::ScheduleNow (&EvalvidServer::SendData, this);
         else 
              m_sendEvent=Simulator::Schedule (p_Interval, &EvalvidServer::SendData, this);
          // m_sendEvent = Simulator::Schedule (m_interval,&EvalvidServer::SendData, this);
                 
     }//enlse if

 
}

//for TCP socket
void EvalvidServer::ConnectionSucceeded (Ptr<Socket> socket)
{
    NS_LOG_FUNCTION (this << socket);
    NS_LOG_LOGIC ("EvalvidServer Connection succeeded");
    m_connected = true;
    SendData ();
}

//for TCP socket
void EvalvidServer::ConnectionFailed (Ptr<Socket> socket)
{
    NS_LOG_FUNCTION (this << socket);
    NS_LOG_LOGIC ("EvalvidServer, Connection Failed");
}

//for TCP socket
void EvalvidServer::DataSend (Ptr<Socket> socket , uint32_t i)
{
    NS_LOG_FUNCTION (this);

    if (m_connected)
    { // Only send new data if the connection has completed
        Simulator::ScheduleNow (&EvalvidServer::SendData, this);
    }
}



} // Namespace ns3
