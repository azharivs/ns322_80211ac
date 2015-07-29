/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013
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
#include "aggregation-controllers.h"
#include "ns3/enum.h"
#include "ns3/double.h"
#include "ns3/nstime.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (AggregationController);

TypeId
AggregationController::GetTypeId (void)
{//TODO
  static TypeId tid = TypeId ("ns3::AggregationController")
    .SetParent<Object> ()
    .AddConstructor<AggregationController> ()
;
  return tid;
}

void
AggregationController::SetQueue(Ptr<PerStaWifiMacQueue> queue, PerStaQInfoContainer &c)
{
  m_queue = queue;
  m_perStaQInfo = &c;
}

void
AggregationController::SetAggregator(Ptr<MpduUniversalAggregator> agg)
{
  m_aggregator = agg;
}

void
AggregationController::Update (void)
{
  return ;
}

NS_OBJECT_ENSURE_REGISTERED (TimeAllowanceAggregationController);

TypeId
TimeAllowanceAggregationController::GetTypeId (void)
{//TODO
  static TypeId tid = TypeId ("ns3::TimeAllowanceAggregationController")
    .SetParent<AggregationController> ()
    .AddConstructor<TimeAllowanceAggregationController> ()
    .AddAttribute ("DVP", "Tolerable Delay Violation Probability",
                   DoubleValue (0.1),
                   MakeDoubleAccessor (&TimeAllowanceAggregationController::m_targetDvp),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("ServiceInterval", "Periodicity with which queues are guaranteed to be serviced (in seconds).",
                   DoubleValue (0.1), //sva: the default value should be later changed to beacon interval
                   MakeDoubleAccessor (&TimeAllowanceAggregationController::m_serviceInterval),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("MaxDelay", "Maximum tolerable single hop delay in seconds",
                   DoubleValue (1.0), //sva: the default value should be later changed to beacon interval
                   MakeDoubleAccessor (&TimeAllowanceAggregationController::m_maxDelay),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("TimeAllowance", "Fixed Time Allowance (in sec) when NO_CONTROL controller is selected",
                   TimeValue (MilliSeconds (1.0) ), //sva: the default value should be later changed to beacon interval
                   MakeTimeAccessor (&TimeAllowanceAggregationController::m_timeAllowance),
                   MakeTimeChecker ())
    .AddAttribute ("Controller", "The aggregation controller used for adjusting parameters.",
                   EnumValue (NO_CONTROL),
                   MakeEnumAccessor (&TimeAllowanceAggregationController::m_type),
                   MakeEnumChecker (ns3::NO_CONTROL, "ns3::NO_CONTROL",
                                    /*sva-design: add for new aggregation controller AGG_CRTL
                                    ns3::AGG_CTRL, "ns3::AGG_CRTL",
                                    sva-design*/
                                    ns3::PID, "ns3::PID"))
;
  return tid;
}

TimeAllowanceAggregationController::TimeAllowanceAggregationController ()
{
}

TimeAllowanceAggregationController::~TimeAllowanceAggregationController ()
{
}

void
TimeAllowanceAggregationController::Update (void)
{
  switch (m_type)
  {
    case NO_CONTROL:
      NoControlUpdate();
      break;
    case PID:
      break;
      /*sva-design: add for new aggregation controller AGG_CTRL
    case AGG_CTRL:
      break;
      sva-design*/
    default:
      NS_FATAL_ERROR("Unspecified Aggregation Controller" << m_type);
  }
  return ;
}

void
TimeAllowanceAggregationController::NoControlUpdate (void)
{
  if (!m_perStaQInfo)//not supported
    {
      return ;
    }
  PerStaQInfoContainer::Iterator it;
  for (it = m_perStaQInfo->Begin(); it != m_perStaQInfo->End(); ++it)
    {
      (*it)->SetTimeAllowance(m_timeAllowance);
    }
}


}  // namespace ns3

