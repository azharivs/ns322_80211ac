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
 * Author: Billy Pinheiro <haquiticos@gmail.com>
 *         Saulo da Mata <damata.saulo@gmail.com>
 *
 */

#ifndef __EVALVID_SERVER_H__
#define __EVALVID_SERVER_H__

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/address.h"
#include "ns3/ipv4-address.h"
#include "ns3/seq-ts-header.h"
#include "ns3/socket.h"
#include <stdio.h>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <vector>
#include <map>
#include "ns3/traced-callback.h"


using std::ifstream;
using std::ofstream;
using std::ostream;
using std::ios;
using std::endl;


using namespace std;

namespace ns3 {
class Address;
class Socket;

/**
 * \ingroup applications
 * \defgroup Evalvid EvalvidServerApplication
 *
 * This traffic generator simply sends data
 * as fast as possible up to MaxBytes or until
 * the application is stopped (if MaxBytes is
 * zero). Once the lower layer send buffer is
 * filled, it waits until space is free to
 * send more data, essentially keeping a
 * constant flow of data. Only SOCK_STREAM 
 * and SOCK_SEQPACKET sockets are supported. 
 * For example, TCP sockets can be used, but 
 * UDP sockets can not be used.
 */


class EvalvidServer : public Application
{
public:
   /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
     static TypeId GetTypeId (void);

     EvalvidServer ();

     virtual ~EvalvidServer ();

     /**
       * brief Set the upper bound for the total number of bytes to send.
       */
     void SetMaxBytes (uint32_t maxBytes);

     /**
       * \brief Get the socket this application is attached to.
       * \return pointer to associated socket
       */
     Ptr<Socket> GetSocket (void) const;

     void Connect ();

protected:
     virtual void DoDispose (void);

private:
     // inherited from Application base class.
     virtual void StartApplication (void);    // Called at time specified by Start
     virtual void StopApplication (void);     // Called at time specified by Stop

     /**
       * \brief Send data until the L4 transmission buffer is full.
       */
     void SendData (); 

     /**
       * \brief Connection Succeeded (called by Socket through a callback)
       * \param socket the connected socket
       */
     void ConnectionSucceeded (Ptr<Socket> socket);

     /**
       * \brief Connection Failed (called by Socket through a callback)
       * \param socket the connected socket
       */
     void ConnectionFailed (Ptr<Socket> socket);

     /**
       * \brief Send more data as soon as some has been transmitted.
       */
     void DataSend (Ptr<Socket>, uint32_t); // for socket's SetSendCallback

     /*
      * \ etelalat ra az file vorodi k shamele etelaat video ast daryaft karde va dar yek sakhtar be nam m_videoInfoMap mirizad
      *  sepas size packet hai k sender mikhad ersal konad ra ba tavajoh be in etelaat mohasebe karde 
      *  va dar file m_sendsizeFile zakhire mikonad
      */
     void Setup (void);
 
     /// Traced Callback: sent packets
     TracedCallback<Ptr<const Packet> > m_txTrace;

  

     Ptr<Socket>     m_socket;       //!< Associated socket
     Address         m_peer;         //!< Peer address
     bool            m_connected;    //!< True if connected
     uint32_t        m_sendSize;     //!< Size of data to send each time
     uint32_t        m_maxBytes;     //!< Limit total number of bytes sent
     uint32_t        m_totBytes;     //!< Total bytes sent so far
     TypeId          m_tid;          //!< The type of protocol to use.

     string      m_videoTraceFileName;	        //!< File from mp4trace tool of Evalvid.
     string      m_senderTraceFileName;		//!< File with information of packets transmitted by EvalvidServer.
     fstream     m_videoTraceFile;
     uint32_t    m_numOfFrames;
     uint16_t    m_packetPayload;
     ofstream    m_senderTraceFile;             
     ofstream    m_sendsizeFile;                // !< File with size of packets transmitted by EvalvidServer.
     ifstream    m_sendFile;                    // !< Open above file for input 
                                                
     uint32_t    m_packetId;
     string      m_sendsizeFilename;
 


     struct m_videoInfoStruct_t
     {
        string   frameType;
        uint32_t frameSize;
        uint16_t numOfTcpPackets;  //uint16_t numOfTcpPackets;
        Time     packetInterval;
     };
     Time m_interval;    //!< Packet inter-send time
     map<uint32_t, m_videoInfoStruct_t*> m_videoInfoMap;
     map<uint32_t, m_videoInfoStruct_t*>::iterator m_videoInfoMapIt;
};

} // namespace ns3

#endif // __EVALVID_SERVER_H__
