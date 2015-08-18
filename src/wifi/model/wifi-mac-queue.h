/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005, 2009 INRIA
 * Copyright (c) 2009 MIRKO BANCHI
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 * Author: Mirko Banchi <mk.banchi@gmail.com>
 * Added support for PerStaWifiMacQueue by,
 * Author: SEYED VAHID AZHARI <azharivs@iust.ac.ir>
 * Iran University of Science & Technology
 */
#ifndef WIFI_MAC_QUEUE_H
#define WIFI_MAC_QUEUE_H

#include <list>
#include <utility>
#include "ns3/packet.h"
#include "ns3/nstime.h"
#include "ns3/object.h"
#include "wifi-mac-header.h"
#include "ns3/per-sta-q-info-container.h"
#include "ns3/enum.h"
#include "mpdu-universal-aggregator.h"
#include "mac-low.h"

namespace ns3 {
class QosBlockedDestinations;
class MpduUniversalAggregator;
class MacLow;
//sva: added support for multiple q per station. Currently not using call backs
//sva: how do I get a pointer to PerStaQInfoContainer initialized?
//sva: May be I can add a new method to WifiMacQueue to do this and if not called
//sva: the pointer will remain NULL meaning that PerStaQInfo is not supported
//sva: such as the case for stations.
//class PerStaQInfoContainer;

/**
 * \ingroup wifi
 *
 * This queue implements the timeout procedure described in
 * (Section 9.19.2.6 "Retransmit procedures" paragraph 6; IEEE 802.11-2012).
 *
 * When a packet is received by the MAC, to be sent to the PHY,
 * it is queued in the internal queue after being tagged by the
 * current time.
 *
 * When a packet is dequeued, the queue checks its timestamp
 * to verify whether or not it should be dropped. If
 * dot11EDCATableMSDULifetime has elapsed, it is dropped.
 * Otherwise, it is returned to the caller.
 */
class WifiMacQueue : public Object
{
public:
  static TypeId GetTypeId (void);
  WifiMacQueue ();
  ~WifiMacQueue ();

  /**
   * Set the maximum queue size.
   *
   * \param maxSize the maximum queue size
   */
  void SetMaxSize (uint32_t maxSize);
  /**
   * Set the maximum delay before the packet is discarded.
   *
   * \param delay the maximum delay
   */
  void SetMaxDelay (Time delay);
  /**
   * Return the maximum queue size.
   *
   * \return the maximum queue size
   */
  uint32_t GetMaxSize (void) const;
  /**
   * Return the maximum delay before the packet is discarded.
   *
   * \return the maximum delay
   */
  Time GetMaxDelay (void) const;

  /**
   * Enqueue the given packet and its corresponding WifiMacHeader at the <i>end</i> of the queue.
   *
   * \param packet the packet to be euqueued at the end
   * \param hdr the header of the given packet
   */
  virtual void Enqueue (Ptr<const Packet> packet, const WifiMacHeader &hdr);
  /**
   * Enqueue the given packet and its corresponding WifiMacHeader at the <i>front</i> of the queue.
   *
   * \param packet the packet to be euqueued at the end
   * \param hdr the header of the given packet
   */
  virtual void PushFront (Ptr<const Packet> packet, const WifiMacHeader &hdr);
  /**
   * Dequeue the packet in the front of the queue.
   *
   * \param hdr the WifiMacHeader of the packet
   * \return the packet
   */
  virtual Ptr<const Packet> Dequeue (WifiMacHeader *hdr);
  /**
   * Peek the packet in the front of the queue. The packet is not removed.
   *
   * \param hdr the WifiMacHeader of the packet
   * \return the packet
   *
   * Apparently never called!!
   */
  virtual Ptr<const Packet> Peek (WifiMacHeader *hdr);
  /**
   * Searchs and returns, if is present in this queue, first packet having
   * address indicated by <i>type</i> equals to <i>addr</i>, and tid
   * equals to <i>tid</i>. This method removes the packet from this queue.
   * Is typically used by ns3::EdcaTxopN in order to perform correct MSDU
   * aggregation (A-MSDU).
   *
   * \param hdr the header of the dequeued packet
   * \param tid the given TID
   * \param type the given address type
   * \param addr the given destination
   * \return packet
   */
  Ptr<const Packet> DequeueByTidAndAddress (WifiMacHeader *hdr,
                                            uint8_t tid,
                                            WifiMacHeader::AddressType type,
                                            Mac48Address addr);
  /**
   * Searchs and returns, if is present in this queue, first packet having
   * address indicated by <i>type</i> equals to <i>addr</i>, and tid
   * equals to <i>tid</i>. This method doesn't remove the packet from this queue.
   * Is typically used by ns3::EdcaTxopN in order to perform correct MSDU
   * aggregation (A-MSDU).
   *
   * \param hdr the header of the dequeued packet
   * \param tid the given TID
   * \param type the given address type
   * \param addr the given destination
   * \param timestamp
   * \return packet
   */
  Ptr<const Packet> PeekByTidAndAddress (WifiMacHeader *hdr,
                                         uint8_t tid,
                                         WifiMacHeader::AddressType type,
                                         Mac48Address addr,
                                         Time *timestamp);
  /**
   * If exists, removes <i>packet</i> from queue and returns true. Otherwise it
   * takes no effects and return false. Deletion of the packet is
   * performed in linear time (O(n)).
   *
   * \param packet the packet to be removed
   * \return true if the packet was removed, false otherwise
   */
  bool Remove (Ptr<const Packet> packet);
  /**
   * Returns number of QoS packets having tid equals to <i>tid</i> and address
   * specified by <i>type</i> equals to <i>addr</i>.
   * sva: can be optimized using PerStaQInfo methods since this information
   * is already available there.
   *
   * \param tid the given TID
   * \param type the given address type
   * \param addr the given destination
   * \return the number of QoS packets
   */
  uint32_t GetNPacketsByTidAndAddress (uint8_t tid,
                                       WifiMacHeader::AddressType type,
                                       Mac48Address addr);
  /**
   * Returns first available packet for transmission. A packet could be no available
   * if it's a QoS packet with a tid and an address1 fields equal to <i>tid</i> and <i>addr</i>
   * respectively that index a pending agreement in the BlockAckManager object.
   * So that packet must not be transmitted until reception of an ADDBA response frame from station
   * addressed by <i>addr</i>. This method removes the packet from queue.
   *
   * \param hdr the header of the dequeued packet
   * \param tStamp
   * \param blockedPackets
   * \return packet
   */
  virtual Ptr<const Packet> DequeueFirstAvailable (WifiMacHeader *hdr,
                                           Time &tStamp,
                                           const QosBlockedDestinations *blockedPackets);
  /**
   * Returns first available packet for transmission. The packet isn't removed from queue.  
   *
   * \param hdr the header of the dequeued packet
   * \param tStamp
   * \param blockedPackets
   * \return packet
   */
  virtual Ptr<const Packet> PeekFirstAvailable (WifiMacHeader *hdr,
                                        Time &tStamp,
                                        const QosBlockedDestinations *blockedPackets);
  /**
   * Flush the queue.
   */
  virtual void Flush (void);

  /**
   * Return if the queue is empty.
   *
   * \return true if the queue is empty, false otherwise
   */
  bool IsEmpty (void);
  /**
   * Return the current queue size.
   *
   * \return the current queue size
   */
  uint32_t GetSize (void);


protected:
  /**
   * Clean up the queue by removing packets that exceeded the maximum delay.
   */
  virtual void Cleanup (void);

  /**
   * A struct that holds information about a packet for putting
   * in a packet queue.
   */
  struct Item
  {
    /**
     * Create a struct with the given parameters.
     *
     * \param packet
     * \param hdr
     * \param tstamp
     */
    Item (Ptr<const Packet> packet,
          const WifiMacHeader &hdr,
          Time tstamp);
    Ptr<const Packet> packet; //!< Actual packet
    WifiMacHeader hdr; //!< Wifi MAC header associated with the packet
    Time tstamp; //!< timestamp when the packet arrived at the queue
  };

  /**
   * typedef for packet (struct Item) queue.
   */
  typedef std::list<struct Item> PacketQueue;
  /**
   * typedef for packet (struct Item) queue reverse iterator.
   */
  typedef std::list<struct Item>::reverse_iterator PacketQueueRI;
  /**
   * typedef for packet (struct Item) queue iterator.
   */
  typedef std::list<struct Item>::iterator PacketQueueI;
  /**
   * Return the appropriate address for the given packet (given by PacketQueue iterator).
   *
   * \param type
   * \param it
   * \return the address
   */
  Mac48Address GetAddressForPacket (enum WifiMacHeader::AddressType type, PacketQueueI it);

  PacketQueue m_queue; //!< Packet (struct Item) queue
  uint32_t m_size; //!< Current queue size
  uint32_t m_maxSize; //!< Queue capacity
  Time m_maxDelay; //!< Time to live for packets in the queue

};


/**
 * \ingroup wifi
 *
 * This is a per station version of the WifiMacQueue and is
 * only to be used for an Access Point
 *
 * When a packet is received by the MAC, to be sent to the PHY,
 * it is queued in the internal queue after being tagged by the
 * current time.
 *
 * When a packet is dequeued, the queue checks its timestamp
 * to verify whether or not it should be dropped. If
 * dot11EDCATableMSDULifetime has elapsed, it is dropped.
 * Otherwise, it is returned to the caller.
 *
 *  _______________________________________________________________
 *  _____________________ Design Approach _________________________
 * |                                                               |
 * | For each arbitration algorithm there will be a new private    |
 * | method for PerStaWifiMacQueue. These methods are then called  |
 * | by DequeueFirstAvailable() and PeekFirstAvailable() that will |
 * | then return the proper packet to be served. The appropriate   |
 * | arbitration algorithm is selected/configured through a new    |
 * | Attribute that is introduced into PerStaMacifiQueue class.    |
 * |_______________________________________________________________|
 */

typedef enum
{
  FCFS,
  EDF,
  EDF_RR,
  MAX_REMAINING_TIME_ALLOWANCE,//to be used in conjunction with TIME_ALLOWANCE aggregation algorithm
  PER_BITRATE_TIME_ALLOWANCE_RR//to be used in conjunction with PER_BITRATE_TIME_ALLOWANCE aggregation algorithm
} ServicePolicyType;

class PerStaWifiMacQueue : public WifiMacQueue
{
public:
  static TypeId GetTypeId (void);
  PerStaWifiMacQueue ();
  ~PerStaWifiMacQueue ();

  /**
   * Enqueue the given packet and its corresponding WifiMacHeader at the <i>end</i> of the queue.
   *
   * \param packet the packet to be euqueued at the end
   * \param hdr the header of the given packet
   */
  void Enqueue (Ptr<const Packet> packet, const WifiMacHeader &hdr);
  /**
   * Enqueue the given packet and its corresponding WifiMacHeader at the <i>front</i> of the queue.
   *
   * \param packet the packet to be euqueued at the end
   * \param hdr the header of the given packet
   */
  void PushFront (Ptr<const Packet> packet, const WifiMacHeader &hdr);
  /**
   * Dequeue the packet in the front of the queue.
   *
   * \param hdr the WifiMacHeader of the packet
   * \return the packet
   *
   * Dequeue() is called by DcaTxop::NotifyAccessGranted() which is for non-11e
   * So we should not be worried about implementing our arbitration in it.
   * Unless we want to define a new way of arbitration for non-11e (DCA) traffic as well.
   *
   * Dequeue() is also called by MacLow::ForwardDown() on its aggregation queue.
   * This is also a FCFS queue so we should not be concerned with it either.
   */
  Ptr<const Packet> Dequeue (WifiMacHeader *hdr);

  /**
   * Peek the packet in the front of the queue. The packet is not removed.
   *
   * \param hdr the WifiMacHeader of the packet
   * \return the packet
   *
   * Apparently not called from anywehere!!
   *
   */
  Ptr<const Packet> Peek (WifiMacHeader *hdr);

  /**
   * Searchs and returns, if is present in this queue, first packet having
   * address indicated by <i>type</i> equals to <i>addr</i>, and tid
   * equals to <i>tid</i>. This method removes the packet from this queue.
   * Is typically used by ns3::EdcaTxopN in order to perform correct MSDU
   * aggregation (A-MSDU).
   *
   * \param hdr the header of the dequeued packet
   * \param tid the given TID
   * \param type the given address type
   * \param addr the given destination
   * \return packet
   *
   * could not find where it was used !!
   */
  Ptr<const Packet> DequeueByTidAndAddress (WifiMacHeader *hdr,
                                            uint8_t tid,
                                            WifiMacHeader::AddressType type,
                                            Mac48Address addr);

  /**
   * Returns first available packet for transmission. A packet could be no available
   * if it's a QoS packet with a tid and an address1 fields equal to <i>tid</i> and <i>addr</i>
   * respectively that index a pending agreement in the BlockAckManager object.
   * So that packet must not be transmitted until reception of an ADDBA response frame from station
   * addressed by <i>addr</i>. This method removes the packet from queue.
   *
   * \param hdr the header of the dequeued packet
   * \param tStamp
   * \param blockedPackets
   * \return packet
   *
   * should reflect our queue arbitration algorithm
   */
  Ptr<const Packet> DequeueFirstAvailable (WifiMacHeader *hdr,
                                           Time &tStamp,
                                           const QosBlockedDestinations *blockedPackets);
  /**
   * Returns number of QoS packets having tid equals to <i>tid</i> and address
   * specified by <i>type</i> equals to <i>addr</i>.
   * sva: can be optimized using PerStaQInfo methods since this information
   * is already available there.
   *
   * \param tid the given TID
   * \param type the given address type
   * \param addr the given destination
   * \return the number of QoS packets
   *
   * not a concern. Don't change
   */
  uint32_t GetNPacketsByTidAndAddress (uint8_t tid,
                                       WifiMacHeader::AddressType type,
                                       Mac48Address addr);
  /**
   * Returns first available packet for transmission. The packet isn't removed from queue.
   *
   * \param hdr the header of the dequeued packet
   * \param tStamp
   * \param blockedPackets
   * \return packet
   * should reflect our queue arbitration algorithm
   */
  Ptr<const Packet> PeekFirstAvailable (WifiMacHeader *hdr,
                                        Time &tStamp,
                                       const QosBlockedDestinations *blockedPackets);
  /**
   * If exists, removes <i>packet</i> from queue and returns true. Otherwise it
   * takes no effects and return false. Deletion of the packet is
   * performed in linear time (O(n)).
   *
   * \param packet the packet to be removed
   * \return true if the packet was removed, false otherwise
   */
  bool Remove (Ptr<const Packet> packet);

  /**
   * Flush the queue.
   */
  void Flush (void);

  /**
   * Return if the queue is empty.
   *
   * \return true if the queue is empty, false otherwise
   */
  bool IsEmpty (void);

 /**
   * Initialize pointer to PerStaQInfoContainer if support is required
   * TODO: combine EnablePerStaQInfo and SetMpduAggregator
   * \param c: Pointer to the container
   * \returns TRUE if successful and FALSE if container was NULL
   */
  bool EnablePerStaQInfo (PerStaQInfoContainer &c);

  /*
   * Sets pointer to MpduUniversalAggregator
   */
  bool SetMpduAggregator (Ptr<MpduUniversalAggregator> agg);

  /*
   * Sets pointer to MacLow
   */
  bool SetMacLow (Ptr<MacLow> low);

  /*
   * Event handler that is called at the beginning of a service interval
   */
  void PendingServiceInterval (void);

  /*
   * called by MpduUniversalAggregator whenever new service interval is
   * actually started
   */
  void BeginServiceInterval (void);

private:

  ServicePolicyType m_servicePolicy; //!< type of service policy
  double m_serviceInterval; //<! service interval period in seconds used for EDF_RR and MAX_REMAINING_TIME_ALLOWANCE

  /*
   * Called to implement the FCFS service policy
   * This function is only called by DequeueFirstAvailable() and PeekFirstAvailable()
   *
   * \param it: reference to the queue iterator pointing to the HoL queue according to service policy
   * \param blockedPackets: exactly passed by caller
   *
   * Returns true if a packet was found, false if no packet was found
   * also assigns it to the correct queue placeholder containing packet to be served
   */
  bool PeekFcfs (PacketQueueI &it, const QosBlockedDestinations *blockedPackets);

  /*
   * Called to implement the Earliest Deadline First (EDF) service policy
   * This function is only called by DequeueFirstAvailable() and PeekFirstAvailable()
   *
   * \param it: reference to the queue iterator pointing to the HoL queue according to service policy
   * \param blockedPackets: exactly passed by caller
   *
   * Returns true if a packet was found, false if no packet was found
   * also assigns it to the correct queue placeholder containing packet to be served
   */
  bool PeekEdf (PacketQueueI &it, const QosBlockedDestinations *blockedPackets);

  /*
   * Called to implement the Earliest Deadline First (EDF+RR) service policy
   * Serves the queue with HoL packet that will earliest deadline that
   * also exceeds its deadline by the next service interval.
   * Could cause problems. Also at this point I have not implemented a guaranteed
   * way so that the queue will get service periodically every m_serviceInterval.
   * This feature is going to require modifications to MacLow as it requires the AP
   * to coordinate access times in a round robin manner.
   * This function is only called by DequeueFirstAvailable() and PeekFirstAvailable()
   *
   * \param it: reference to the queue iterator pointing to the HoL queue according to service policy
   * \param blockedPackets: exactly passed by caller
   *
   * Returns true if a packet was found, false if no packet was found
   * also assigns it to the correct queue placeholder containing packet to be served
   */
  bool PeekEdfRoundRobin (PacketQueueI &it, const QosBlockedDestinations *blockedPackets);

  /*
   * Called to implement the Maximum Remaining Time Allowance service policy
   * Serves the queue which has largest amount of remaining time allowance
   * for the current service interval.
   * At this point I have not implemented a guaranteed
   * way so that the queue will get service periodically every m_serviceInterval.
   * This feature is going to require modifications to MacLow as it requires the AP
   * to coordinate access times in a round robin manner.
   * This function is only called by DequeueFirstAvailable() and PeekFirstAvailable()
   *
   * \param it: reference to the queue iterator pointing to the HoL queue according to service policy
   * \param blockedPackets: exactly passed by caller
   *
   * Returns true if a packet was found, false if no packet was found
   * also assigns it to the correct queue placeholder containing packet to be served
   */
  bool PeekMaxRemainingTimeAllowance (PacketQueueI &it, const QosBlockedDestinations *blockedPackets);

  /*
   * Called to implement the Per Bitrate Time Allowance Round Robin service policy
   * Iterates over stations in a round robin fashion and selects a station if
   * it has enough remaining time allowance for its current bitrate.
   * Needs to check current bitrate.
   * Should be optimized to choose station with largest time allowance and queue
   * so that channel is efficiently used.
   */
  bool PeekPerBitrateTimeAllowanceRoundRobin (PacketQueueI &it, const QosBlockedDestinations *blockedPackets);

  /*
   * Return iterator pointing to queue location holding packet with
   * appropriate tid destined to certain dest STA which is ready to be sent.
   *
   * \param it: PacketQueue Iterator (reference)
   * \param tid: tid we are looking for
   * \param dest: MAC destinaton address (i.e. of station)
   * \param blockedPackets: passed directly by caller
   *
   */
  bool GetStaHol (PacketQueueI &it, uint8_t tid, Mac48Address dest,
                                 const QosBlockedDestinations *blockedPackets);

  /**
   * Clean up the queue by removing packets that exceeded the maximum delay.
   */
  virtual void Cleanup (void);

  /*
   * sva: TODO maybe I need to remove the reference to PerStaQInfoContainer and use call backs
   *      In this case I will have to define call backs from here to the container.
   *      However, I'm first going to get this compiled the way it is!
   */
  //sva: should be set to appropriate value by EnablePerStaQInfo () method if support is required
  PerStaQInfoContainer *m_perStaQInfo; //!< pointer to PerStaQInfoContainer NULL if not supported
  Ptr<MpduUniversalAggregator> m_mpduAggregator; //!< Pointer to aggregator operating on this queue
  Ptr<MacLow> m_low; //!< Pointer to MacLow instance
  Time m_currentServiceIntervalStart; //!< Actual beginning of current service interval
  Time m_pendingServiceIntervalStart; //!< Pending beginning of current service interval
  bool m_serviceIntervalPending; //!< flag that shows the status of current service interval as pending or actually started. m_currentServiceInterval will be valid only if this flag is false.
  PerStaQInfoContainer::Iterator m_lastServed; //!< used for RR service policy
};


} // namespace ns3

#endif /* WIFI_MAC_QUEUE_H */
