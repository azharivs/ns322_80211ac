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
#include <list>
#include <deque>
#include <utility>
#include "ns3/packet.h"
#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/uinteger.h"
//#include "wifi-mac-header.h"
//#include "wifi-mac-queue.h"
#include "per-sta-q-info.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (PerStaQInfo);

TypeId
PerStaQInfo::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PerStaQInfo")
    .SetParent<Object> ()
    .AddConstructor<PerStaQInfo> ()
    .AddAttribute ("HistorySize", "Number of Samples Kept for Calculating Statistics.",
                   UintegerValue (100),
                   MakeUintegerAccessor (&PerStaQInfo::m_histSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("TID", "Traffic Indication Map of Interest.",
                   UintegerValue (UP_VI),
                   MakeUintegerAccessor (&PerStaQInfo::m_tid),
                   MakeUintegerChecker<uint32_t> ())

  ;
  return tid;
}

PerStaQInfo::PerStaQInfo()
  : m_addrs (NULL),
    m_queueSize (0), m_queueBytes (0), m_avgQueueSize (0.0), m_avgQueueBytes (0.0),
    m_avgQueueWait (0.0), m_avgArrivalRate (0.0), m_avgArrivalRateBytes (0.0)

{
}

PerStaQInfo::~PerStaQInfo()
{
//TODO: where do the deques get deleted? Do I have to deal with that here?
  m_queueSizeHistory.clear();
  m_queueBytesHistory.clear();
  m_queueWaitHistory.clear();
}

void
PerStaQInfo::SetMac (const Mac48Address &addrs)
{
  uint8_t buff[6];
  addrs.CopyTo(buff);
  m_addrs.CopyFrom(buff);
}

void
PerStaQInfo::SetTid(uint8_t tid)
{
  m_tid = tid;
}

Mac48Address&
PerStaQInfo::GetMac (void)
{
  return m_addrs;
}

uint8_t
PerStaQInfo::GetTid (void)
{
  return m_tid;
}

uint32_t
PerStaQInfo::GetSize (void)
{
  return m_queueSize;
}

uint32_t
PerStaQInfo::GetSizeBytes (void)
{
  return m_queueBytes;
}

double
PerStaQInfo::GetAvgSize (void)
{
  return m_avgQueueSize;
}

double
PerStaQInfo::GetAvgSizeBytes (void)
{
  return m_avgQueueBytes;
}

double
PerStaQInfo::GetAvgWait (void)
{
  return m_avgQueueWait;
}

double
PerStaQInfo::GetAvgArrivalRate (void)
{
  return m_avgArrivalRate;
}

double
PerStaQInfo::GetAvgArrivalRateBytes (void)
{
  return m_avgArrivalRateBytes;
}

void
PerStaQInfo::Arrival (uint32_t bytes, Time tstamp)
{
}

void
PerStaQInfo::Departure (uint32_t bytes, Time tstamp)
{
}

bool
PerStaQInfo::IsEmpty (void)
{
  return (m_queueSize == 0);
}

void
PerStaQInfo::Reset (void)
{
  m_queueSize = 0;
  m_queueBytes = 0;
  m_avgQueueSize = 0;
  m_avgQueueBytes = 0;
  m_avgQueueWait = 0;
  m_avgArrivalRate = 0;
  m_avgArrivalRateBytes = 0;

  m_queueSizeHistory.clear();
  m_queueBytesHistory.clear();
  m_queueWaitHistory.clear();

}

/*
 * still need to calculate average arrival rate. This requires to store
 * the arrival instances in another deque. This deque will have a depth which
 * is specified in terms of time rather than number of samples.
 * May be the other sample histories should also be made according to a time depth??
 *
 * TODO:
 * I think it's better to leave the implementation of arrival rates to later after this
 * whole class is tested.
 */
void
PerStaQInfo::Update(void)
{
  double tmp=0;

  if (!m_queueSizeHistory.empty())
    {
      for (std::deque<uint32_t>::iterator it=m_queueSizeHistory.begin(); it != m_queueSizeHistory.end(); ++it)
        {
          tmp += *it;
        }
      m_avgQueueSize = tmp / (double) m_queueSizeHistory.size();
    }

  tmp = 0;
  if (!m_queueBytesHistory.empty())
    {
      for (std::deque<uint32_t>::iterator it=m_queueBytesHistory.begin(); it != m_queueBytesHistory.end(); ++it)
        {
          tmp += *it;
        }
      m_avgQueueBytes = tmp / (double) m_queueBytesHistory.size();
    }

  tmp = 0;
  if (!m_queueWaitHistory.empty())
    {
      for (std::deque<double>::iterator dit=m_queueWaitHistory.begin(); dit != m_queueWaitHistory.end(); ++dit)
        {
          tmp += *dit;
        }
      m_avgQueueWait = tmp / (double) m_queueWaitHistory.size();
    }

}



} // namespace ns3
