/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) Jul 2, 2015 IUST
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
 * Author: SEYED VAHID AZHARI <azharivs@iust.ac.ir>
 */
#ifndef PER_STA_Q_INFO_CONTAINER_H
#define PER_STA_Q_INFO_CONTAINER_H

#include <vector>
//#include <deque>
#include <utility>
#include "ns3/packet.h"
#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/udp-client.h"
#include "ns3/timestamp-tag.h"
//#include "wifi-mac-queue.h"
//#include "ns3/net-device-container.h"
#include "ns3/wifi-net-device.h"
#include "ns3/per-sta-q-info.h"

namespace ns3 {

/**
 * \brief create PerStaQInfo objects from a BSS containing an AP and some stations
 *
 *
 */
class PerStaQInfoContainer
{
public:
  /**
   * \brief create empty container
   *
   *
   */
  PerStaQInfoContainer ();

  /// PerStaQInfo container iterator
  typedef std::vector<Ptr<PerStaQInfo> >::const_iterator Iterator;

  /**
   * \brief Get an iterator which refers to the first PerStaQInfo element in the
   * container.
   *
   * Applications can be retrieved from the container in two ways.  First,
   * directly by the MAC of their associated station NetDevice, and second, using an iterator.
   * This method is used in the iterator method and is typically used in a
   * for-loop to run through PerStaQInfo list
   *
   * \code
   *   PerStaQInfoContainer::Iterator i;
   *   for (i = container.Begin (); i != container.End (); ++i)
   *     {
   *       (*i)->method ();  // some PerStaQInfo method
   *     }
   * \endcode
   *
   * \returns an iterator which refers to the first element in the container.
   */
  Iterator Begin (void) const;

  /**
   * \brief Get an iterator which refers to the first PerStaQInfo element in the
   * container.
   *
   * Applications can be retrieved from the container in two ways.  First,
   * directly by the MAC of their associated station NetDevice, and second, using an iterator.
   * This method is used in the iterator method and is typically used in a
   * for-loop to run through PerStaQInfo list
   *
   * \code
   *   PerStaQInfoContainer::Iterator i;
   *   for (i = container.Begin (); i != container.End (); ++i)
   *     {
   *       (*i)->method ();  // some PerStaQInfo method
   *     }
   * \endcode
   *
   * \returns an iterator which indicates an ending condition for a loop.
   */
  Iterator End (void) const;

  /**
   * \brief Get the number of Ptr<PerStaQInfo> stored in this container.
   *
   * Per station queue information can be retrieved from the container in two ways.  First,
   * directly by an index into the container, and second, using an iterator.
   * This method is used in the direct method and is typically used to
   * define an ending condition in a for-loop that runs through the stored
   * information elements.
   *
   * \code
   *   uint32_t nPerStaQInfo = container.GetN ();
   *   for (uint32_t i = 0 i < nPerStaQInfo; ++i)
   *     {
   *       Ptr<PerStaQInfo> p = container.Get (i)
   *       i->method ();  // some PerStaQInfo method
   *     }
   * \endcode
   *
   * \returns the number of Ptr<PerStaQInfo> stored in this container.
   */
  uint32_t GetN (void) const;

  /**
   * \brief Get the Ptr<PerStaQInfo> stored in this container at a given
   * index.
   *
   * Per station queue information can be retrieved from the container in two ways.  First,
   * directly by an index into the container, and second, using an iterator.
   * This method is used in the direct method and is used to retrieve the
   * indexed Ptr<PerStaQInfo>.
   *
   * \code
   *   uint32_t nPerStaQInfo = container.GetN ();
   *   for (uint32_t i = 0 i < nPerStaQInfo; ++i)
   *     {
   *       Ptr<PerStaQInfo> p = container.Get (i)
   *       i->method ();  // some PerStaQInfo method
   *     }
   * \endcode
   *
   * \param i the index of the requested PerStaQInfo pointer.
   * \returns the requested PerStaQInfo pointer.
   */
  Ptr<PerStaQInfo> Get (uint32_t i) const;

  /**
   * \brief Get the Ptr<PerStaQInfo> stored in this container that corresponds
   * to a certain MAC, TID
   * By default, TID is set to UP_VI corresponding to AC_VI which is the only TID
   * supported at this time.
   * Unexpected behavior if more than one match found
   *
   * \param addr the MAC address of the NetDevice associated to the station of interest
   * \param tid the TID of interest (default = UP_VI)
   * \returns the requested PerStaQInfo pointer.
   */
  Ptr<PerStaQInfo> GetByMac (Mac48Address addr, uint8_t tid=UP_VI) const;

  /**
   * \brief Get the Ptr<PerStaQInfo> stored in this container that corresponds
   * to a certain IPv4, TID
   * By default, TID is set to UP_VI corresponding to AC_VI which is the only TID
   * supported at this time.
   * Unexpected behavior if more than one match found
   *
   * \param addr the IPv4 address of the NetDevice associated to the station of interest
   * \param tid the TID of interest (default = UP_VI)
   * \returns the requested PerStaQInfo pointer.
   */
  Ptr<PerStaQInfo> GetByIpv4 (Ipv4Address addr, uint8_t tid=UP_VI) const;

  /**
   * Create one PerStaQInfo and add it to the container
   *
   * \param sta: station net devices pointer
   * Currently assumes only one NetDevice per STA and AP
   * \returns a pointer to the currently added PerStaQInfo object
   */
  Ptr<PerStaQInfo> Add (Ptr<WifiNetDevice> sta);

  /**
   * Takes care of updating queue statistics for appropriate station
   * upon packet arrival
   * called by WifiMacQueue methods
   *
   * \param packet: the arrived packet used to extract size
   * \param hdr: the packet header used to extract MAC address and TID
   * \param tstamp: packet time stamp used to log arrival time, etc.
   *
   */
  void Arrival(Ptr<const Packet> packet, const WifiMacHeader &hdr, Time tstamp);

  /**
   * Takes care of updating queue statistics for appropriate station
   * upon packet departure
   * called by WifiMacQueue methods
   *
   * \param packet: the departed packet used to extract size
   * \param hdr: the packet header used to extract MAC address and TID
   * \param tstamp: packet time stamp indicating original arrival time
   *
   */
  void Departure(Ptr<const Packet> packet, const WifiMacHeader &hdr, Time tstamp);

  /**
   * Resets queue statistics for all stations and is called
   * by WifiMacQueue::Flush()
   */
  void Reset(void);

  /**
   * Returns one if the container is empty
   *
   * \returns 1 or 0
   */
  bool IsEmpty (void) const;

#ifdef SVA_DEBUG
  uint32_t m_cnt; //for testing
#endif

private:
  std::vector<Ptr<PerStaQInfo> > m_staQInfo; //!< PerStaQInfo smart pointers
};

} //name space ns3


#endif /* PER_STA_Q_INFO_CONTAINER_H */
