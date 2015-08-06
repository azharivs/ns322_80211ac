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

//TODO: For now all PerStaQ's have the same DVP and MaxDelay. To be changed later.

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
                   TimeValue (MilliSeconds (7.0) ), //sva: the default value should be later changed to beacon interval
                   MakeTimeAccessor (&TimeAllowanceAggregationController::m_timeAllowance),
                   MakeTimeChecker ())
    .AddAttribute ("KP", "Proportional coefficient used with the PID controller",
                   DoubleValue (1.0),
                   MakeDoubleAccessor (&TimeAllowanceAggregationController::kp),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("KI", "Integral coefficient used with the PID controller",
                   DoubleValue (1.0),
                   MakeDoubleAccessor (&TimeAllowanceAggregationController::ki),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("KD", "Derivative coefficient used with the PID controller",
                   DoubleValue (1.0),
                   MakeDoubleAccessor (&TimeAllowanceAggregationController::kd),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("Controller", "The aggregation controller used for adjusting parameters.",
                   EnumValue (PID),
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
      PidControlUpdate();
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


void
TimeAllowanceAggregationController::PidControlUpdate (void)
{//TODO: define perSta targetDVP and Dmax
  if (!m_perStaQInfo)//not supported
    {
      return ;
    }
  double err = 0;
  double ctrlSignal = 0;
  double totalTimeAllowance = 0;
  double tmpTimeAllowance = 0;
  double tmpPrEmpty = 0;
  PerStaQInfoContainer::Iterator it;
  for (it = m_perStaQInfo->Begin(); it != m_perStaQInfo->End(); ++it)
    {
      tmpPrEmpty = 0.999;
      if ((*it)->GetPrEmpty() != 1) //prevent division by zero
        tmpPrEmpty = (*it)->GetPrEmpty();
      err = (*it)->GetAvgServedPackets() + log(m_targetDvp)*m_serviceInterval * (*it)->GetAvgSize()/m_maxDelay/(1-tmpPrEmpty);
      ctrlSignal = -kp*err; //TODO: for now just proportional controller
      tmpTimeAllowance = std::max((double)0,(const double)(*it)->GetTimeAllowance().GetSeconds() + ctrlSignal);

      #ifdef SVA_DEBUG
      std::cout << Simulator::Now().GetSeconds() << " AggregationController (PID) " << (*it)->GetMac()
          << " err= " << err << " ctrlSignal= " << ctrlSignal
          << " curTimeAllowance= " << (*it)->GetTimeAllowance().GetSeconds()*1000 << " msec"
          << " newTimeAllowance= " << tmpTimeAllowance*1000 << " msec"
          << " avgServed= " << (*it)->GetAvgServedPackets()
          << " avgQueue= " << (*it)->GetAvgSize()
          << " const= " << log(m_targetDvp)*m_serviceInterval/m_maxDelay << "\n";
      #endif

      (*it)->SetTimeAllowance(Seconds(tmpTimeAllowance));
      totalTimeAllowance += tmpTimeAllowance;
    }
  //adjust time allowance to not exceed service interval
  if (totalTimeAllowance > m_serviceInterval)
    {
      totalTimeAllowance = totalTimeAllowance / m_serviceInterval;
      for (it = m_perStaQInfo->Begin(); it != m_perStaQInfo->End(); ++it)
        {
          (*it)->SetTimeAllowance((*it)->GetTimeAllowance()/totalTimeAllowance);
        }
    }
}

}  // namespace ns3

