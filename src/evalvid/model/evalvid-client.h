/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 *
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

#ifndef __EVALVID_CLIENT_H__
#define __EVALVID_CLIENT_H__

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-address.h"
#include "ns3/seq-ts-header.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <fstream>
#include <iostream>
#include <iomanip>
#include "ns3/traced-callback.h"
#include "ns3/address.h"
#include "ns3/packet-loss-counter.h"


using std::ifstream;
using std::ofstream;
using std::ostream;
using std::ios;
using std::endl;
using namespace std;

namespace ns3 {

class Socket;
class Packet;
class Address;
/**
 * \ingroup evalvid
 * \class EvalvidClient
 * \brief A Udp or TCP client. Receive packetet carrying sequence number and time stamp
 *  in their payloads
 *
 */
class EvalvidClient : public Application
{
  public:

     static TypeId GetTypeId (void);

     EvalvidClient ();

     virtual ~EvalvidClient ();

     /**
       * \return the total bytes received in this sink app
       */
     uint32_t GetTotalRx () const;
 
     /**
       * \return the total packet received in this sink app
       */
     uint32_t GetReceived () const;
     /**
       * \brief Returns the number of lost packets
       * \return the number of lost packets
       */
     uint32_t GetLost (void) const;

     /**
       * \return pointer to listening socket
       */
     Ptr<Socket> GetListeningSocket (void) const;
     /**
       * \brief Set the size of the window used for checking loss. This value should
       *  be a multiple of 8
       * \param size the size of the window used for checking loss. This value should
       *  be a multiple of 8
       */
     void SetPacketWindowSize (uint16_t size);

     /**
       * \return list of pointers to accepted sockets
       */
     std::list<Ptr<Socket> > GetAcceptedSockets (void) const;

 protected:
     virtual void DoDispose (void);

 private:
     // inherited from Application base class.
     virtual void StartApplication (void);    // Called at time specified by Start
     virtual void StopApplication (void);     // Called at time specified by Stop
     /**
       * \brief Handle a packet received by the application
       * \param socket the receiving socket
       */
     void HandleRead (Ptr<Socket> socket);
     void HandleReadUdp (Ptr<Socket> socket);
     /**
       * \brief Handle an incoming connection
       * \param socket the incoming connection socket
       * \param from the address the connection is from
       */
     void HandleAccept (Ptr<Socket> socket, const Address& from);
     /**
       * \brief Handle an connection close
       * \param socket the connected socket
       */
     void HandlePeerClose (Ptr<Socket> socket);
     /**
       * \brief Handle an connection error
       * \param socket the connected socket
       */
     void HandlePeerError (Ptr<Socket> socket);

     void OpenSendsizefile();

     // In the case of TCP, each socket accept returns a new socket, so the 
     // listening socket is stored separately from the accepted sockets
     Ptr<Socket>     m_socket;       //!< Listening socket
     std::list<Ptr<Socket> > m_socketList; //!< the accepted sockets

     Address         m_local;        //!< Local address to bind to
     uint32_t        m_totalRx;      //!< Total bytes received
     TypeId          m_tid;          //!< Protocol TypeId
     string          m_clientId;
     double          m_deadline;     //!< Packet deadline in seconds 
     /// Traced Callback: received packets, source address.
     TracedCallback<Ptr<const Packet>, const Address &> m_rxTrace;


     ofstream    receiverDumpFile;       //!< output file 
     string      receiverDumpFileName;   //!< outputFile Name
     ifstream    sendsizeFile;           //!< inputFile (Including the size of packets sent)
     Ptr<Packet> incompletePacket;       //!< incomplete received Packet
     bool        isfragment;             //!< if sink receive an incomplete packet isfragment=true else isfragment=false
     uint32_t    fragment;               //!< The number of packets not received
     uint32_t    m_received;             //!< total packets received
     string      m_sendSizeFileName;
     PacketLossCounter m_lossCounter;    //!< Lost packet counter
     bool        m_connected;
     ofstream    output;
 
};

} // namespace ns3

#endif // __EVALVID_CLIENT_H__
