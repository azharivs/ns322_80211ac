/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015, SEYED VAHID AZHARI
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
 * Author: Seyed Vahid Azhari <azharivs@iust.ac.ir>
 */

// TODO: need to bind this to a station NetDevice and to the WifiMacQueue of the AP NetDevice
// Approach 1. Create a container of this type and have it initialized with station NetDevices container
//             so that each station MAC and tid can be used to create one of these classes. The AP should
//             also be provided as a parameter so that this class gets binded to WifiMacQueue of the AP.
//             We need a constructor with the appropriate signature. May be it is better to have these as
//             a container and then write the appropriate container constructor, i.e., PerStaQInfoContainer
#ifndef PER_STA_Q_INFO_H
#define PER_STA_Q_INFO_H

#include <list>
#include <deque>
#include <utility>
#include "ns3/packet.h"
#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/qos-tag.h"
#include "wifi-mac-header.h"
//#include "wifi-mac-queue.h"

namespace ns3 {
//class WiFiMacQueue;

/**
 * \ingroup wifi
 *
 * Note: For now only tid supported is UP_VI equivalent to AC_VI. This means that there will be only one
 * PerStaQInfo for each station.
 *
 */
class PerStaQInfo : public Object
{
public:
  static TypeId GetTypeId (void);
  PerStaQInfo ();
  //sva: not needed PerStaQInfo (Mac48Address addr);
  // for later implementations: PerStaQInfo (Mac48Address addr, uint8_t tid);
  ~PerStaQInfo ();

  /**
   * Set the maximum number of samples kept for calculating statistics.
   * Currently not needed because I'm relying on Attribute base initialization
   * \param histSize the maximum sample history size
   */
  //void SetHistorySize (uint32_t histSize);

  /**
   * Set MAC address associated to this queue
   * \param addrs: reference to address to be set
   */
  void SetMac (const Mac48Address &addrs);

  /**
   * Set Traffic Indication Map associated to this queue
   *
   * \param tid: TID to be set for this queue
   */
  void SetTid (uint8_t tid);

  /**
   * Get MAC address associated to this queue
   *
   * \return reference to MAC address
   */
  Mac48Address& GetMac (void);

  /**
   * Get Traffic Indication Map associated to this queue
   *
   * \return TID
   */
  uint8_t GetTid (void);

  /**
   * Get current number of queued packets belonging to this station MAC and TID
   *
   * \return queue length
   */
  uint32_t GetSize (void);
  /**
   * Get current number of queued bytes belonging to this station MAC and TID
   *
   * \return queue length in bits
   */
  uint32_t GetSizeBytes (void);
  /**
   * Computes and returns average queue length  in packets, over the sample history buffer
   *
   * \return the average queue length in packets
   */
  double GetAvgSize (void);
  /**
   * Computes and returns average queue length in bytes, over the sample history buffer
   *
   * \return the average queue length in bytes
   */
  double GetAvgSizeBytes (void);
  /**
   * Computes and returns average queue waiting time, over the sample history buffer
   * This does not include packet transmission time
   *
   * \return the average queue waiting time (in seconds)
   */
  double GetAvgWait (void);
  /**
   * Computes and returns average packet arrival rate, over ???
   *
   * \return the average arrival rate for the queue (in packets per second)
   */
  double GetAvgArrivalRate (void);
  /**
   * Computes and returns average arrival rate in bps, over ???
   *
   * \return the average arrival rate for the queue (in bits per second)
   */
  double GetAvgArrivalRateBytes (void);

  /**
   * A new packet has arrived: collect statistics and update
   * If successful, should call private member Update() before returning
   *
   * \param bytes : number of incoming bytes
   * \param tstamp: time of packet arrival (should always be set by caller to now())
   */
  void Arrival (uint32_t bytes, Time tstamp);
  /**
   * A packet has departed, collect and update statistics
   * if successful, should call private member Update() before returning
   *
   * \param bytes : number of incoming bytes
   * \param wait: packet waiting time (should always be set by caller to now()- packet queue time stamp)
   */
  void Departure (uint32_t bytes, Time wait);
  /**
   * Return if the queue is empty.
   *
   * \return true if the queue is empty, false otherwise
   */
  bool IsEmpty (void);

  /**
   * Reset all statistics and flush sample history
   * call chain initiated by PerStaWifiMacQueue::Flush()
   */
  void Reset (void);

private:

  /**
   * A struct that holds information about a packet for putting
   * in a packet queue.
   *
  struct Item
  {
    **
     * Create a struct with the given parameters.
     *
     * \param packet
     * \param hdr
     * \param tstamp
     *
    Item (Ptr<const Packet> packet,
          const WifiMacHeader &hdr,
          Time tstamp);
    Ptr<const Packet> packet; //!< Actual packet
    WifiMacHeader hdr; //!< Wifi MAC header associated with the packet
    Time tstamp; //!< timestamp when the packet arrived at the queue
  };*/

  /**
   * typedef for sample history.
   */
  //typedef std::deque<double> History;
  /**
   * typedef for iterator of sample history
   */
  //typedef std::deque<double>::reverse_iterator PacketQueueRI;
  /**
   * typedef for packet (struct Item) queue iterator.
   */
  //typedef std::list<struct Item>::iterator PacketQueueI;
  /**
   * Return the appropriate address for the given packet (given by PacketQueue iterator).
   *
   * \param type
   * \param it
   * \return the address
   */
  //Mac48Address GetAddressForPacket (enum WifiMacHeader::AddressType type, PacketQueueI it);

  //PacketQueue m_queue; //!< Packet (struct Item) queue

  /**
   * Computes all average values from sample history
   * Called by Arrival and Departure methods
   *
   */
  void Update (void);

  std::deque<uint32_t> m_queueSizeHistory; //!< Array of samples of queue length in packets
  std::deque<uint32_t> m_queueBytesHistory; //!< Array of samples of queue length in bytes
  std::deque<double> m_queueWaitHistory; //!< Array of samples of queue waiting time
  Mac48Address m_addrs; //!< MAC address of STA that is represented by this QInfo element
  //Do I need this? Ipv4Address m_ipv4Addrs; //!< IPv4 address of STA that is represented by this QInfo element
  uint8_t m_tid; //!< (Traffic Indication Map) of STA that is represented by this QInfo element
  uint32_t m_queueSize; //!< Current queue size in packets
  uint32_t m_queueBytes; //!< Current queue size in bytes
  uint32_t m_histSize; //!< Sample history size
  double m_avgQueueSize; //!< Last updated average queue size in packets
  double m_avgQueueBytes; //!< Last updated average queue size in bytes
  double m_avgQueueWait; //!< Last updated average queue waiting time in seconds
  double m_avgArrivalRate; //!< Last updated average packet arrival rate in pps
  double m_avgArrivalRateBytes; //!< Last updated average arrival rate in Bytes per second
};

} // namespace ns3

#endif /* PER_STA_Q_INFO_H */
