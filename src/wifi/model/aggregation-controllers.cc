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
#include "pid-controller.h"
#include "pid-controller-with-thresholds.h"

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
AggregationController::Initialize(Ptr<PerStaWifiMacQueue> queue, PerStaQInfoContainer &c, Ptr<MpduUniversalAggregator> agg)
{//TODO: initialize aggregator here as well and remove next method
  m_queue = queue;
  m_perStaQInfo = &c;
  m_aggregator = agg;
  NS_ASSERT(m_perStaQInfo);
  NS_ASSERT(m_queue);
  NS_ASSERT(m_aggregator);
  //perform initialization specific to the particular type of AggragationController subclass
  DoInitialize();
}

/*
 * should be redefined by each subclass to reflect its own specific initialization steps
 */
void
AggregationController::DoInitialize(void)
{
  return ;
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
                   DoubleValue (0.02),
                   MakeDoubleAccessor (&TimeAllowanceAggregationController::m_targetDvp),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("ServiceInterval", "Periodicity with which queues are guaranteed to be serviced (in seconds).",
                   DoubleValue (0.1), //sva: the default value should be later changed to beacon interval
                   MakeDoubleAccessor (&TimeAllowanceAggregationController::m_serviceInterval),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("MaxDelay", "Maximum tolerable single hop delay in seconds",
                   DoubleValue (3.0), //sva: the default value should be later changed to beacon interval
                   MakeDoubleAccessor (&TimeAllowanceAggregationController::m_maxDelay),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("TimeAllowance", "Fixed Time Allowance (in sec) when NO_CONTROL controller is selected",
                   TimeValue (MilliSeconds (12.0) ), //sva: the default value should be later changed to beacon interval
                   MakeTimeAccessor (&TimeAllowanceAggregationController::m_timeAllowance),
                   MakeTimeChecker ())
    .AddAttribute ("MovingIntegralWeight", "Recent sample moving average weight for approximating the integral term of the PID controllers",
                   DoubleValue (0.05),
                   MakeDoubleAccessor (&TimeAllowanceAggregationController::m_weightIntegral),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("KP", "Proportional coefficient used with the PID controllers",
                   DoubleValue (0.01),
                   MakeDoubleAccessor (&TimeAllowanceAggregationController::m_kp),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("KI", "Integral coefficient used with the PID controllers",
                   DoubleValue (0.02),
                   MakeDoubleAccessor (&TimeAllowanceAggregationController::m_ki),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("KD", "Derivative coefficient used with the PID controllers",
                   DoubleValue (0.05),
                   MakeDoubleAccessor (&TimeAllowanceAggregationController::m_kd),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("ThrWeight", "Recent sample moving average weight for approximating standard deviation of error signal of the threshold based PID controller",
                   DoubleValue (0.5),
                   MakeDoubleAccessor (&TimeAllowanceAggregationController::m_thrW),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("ThrHighCoef", "Mean error multiplier for defining high threshold of the threshold based PID controller",
                   DoubleValue (2.5),
                   MakeDoubleAccessor (&TimeAllowanceAggregationController::m_thrH),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("ThrLowCoef", "Mean error multiplier for defining high threshold of the threshold based PID controller",
                   DoubleValue (2.5),
                   MakeDoubleAccessor (&TimeAllowanceAggregationController::m_thrL),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("Controller", "The aggregation controller used for adjusting parameters.",
                   EnumValue (NO_CONTROL),
                   MakeEnumAccessor (&TimeAllowanceAggregationController::m_type),
                   MakeEnumChecker (ns3::NO_CONTROL, "ns3::NO_CONTROL",
                                    ns3::PID_WITH_THRESHOLDS, "ns3::PID_WITH_THRESHOLDS",
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
TimeAllowanceAggregationController::DoInitialize (void)
{
  switch (m_type)
  {
    case ns3::NO_CONTROL:
      DoInitializeNoControl();
      break;
    case ns3::PID:
      DoInitializePidControl();
      break;
    case ns3::PID_WITH_THRESHOLDS:
      DoInitializePidControlWithThresholds();
      break;
      /*sva design: add following lines for each new type of controller ?
    case ns3::?:
      DoInitialize?();
      break;
      sva-design*/
    default:
      NS_FATAL_ERROR("Undefined Controller Associated to AggregationController " << m_type);
  }
}

/*sva design: add following lines for each new type of controller ?
void
TimeAllowanceAggregationController::DoInitialize? (void)
{
  //TODO Implementation of initialization code for controller of type ?
}
sva-design*/

void
TimeAllowanceAggregationController::DoInitializeNoControl (void)
{
  return ;
}


void
TimeAllowanceAggregationController::DoInitializePidControl (void)
{
  //allocate a controller for each PerStaQInfo
  for (PerStaQInfoContainer::Iterator it = m_perStaQInfo->Begin(); it != m_perStaQInfo->End(); ++it)
    {
      Ptr<PidController> pid;
      pid = CreateObject<PidController>();
      //initialize pid controller if necessary
      pid->SetAttribute("MovingAverageWeight",DoubleValue(m_weightIntegral));
      pid->SetAttribute("KP",DoubleValue(m_kp));
      pid->SetAttribute("KI",DoubleValue(m_ki));
      pid->SetAttribute("KD",DoubleValue(m_kd));
      pid->SetStaQInfo ( (*it) );
      pid->SetInputParams(PidController::InParamType(m_targetDvp, m_maxDelay, m_serviceInterval));
      pid->Init();
      m_ctrl[(*it)->GetMac()] = pid;
    }
}

void
TimeAllowanceAggregationController::DoInitializePidControlWithThresholds (void)
{
  //allocate a controller for each PerStaQInfo
  for (PerStaQInfoContainer::Iterator it = m_perStaQInfo->Begin(); it != m_perStaQInfo->End(); ++it)
    {
      Ptr<PidControllerWithThresholds> pid;
      pid = CreateObject<PidControllerWithThresholds>();
      //initialize pid controller if necessary
      pid->SetAttribute("MovingAverageWeight",DoubleValue(m_weightIntegral));
      pid->SetAttribute("KP",DoubleValue(m_kp));
      pid->SetAttribute("KI",DoubleValue(m_ki));
      pid->SetAttribute("KD",DoubleValue(m_kd));
      pid->SetAttribute("ThrWeight",DoubleValue(m_thrW));
      pid->SetAttribute("ThrHighCoef",DoubleValue(m_thrH));
      pid->SetAttribute("ThrLowCoef",DoubleValue(m_thrL));
      pid->SetStaQInfo ( (*it) );
      pid->SetInputParams(PidControllerWithThresholds::InParamType(m_targetDvp, m_maxDelay, m_serviceInterval));
      pid->Init();
      m_ctrl[(*it)->GetMac()] = pid;
    }
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
    case PID_WITH_THRESHOLDS:
      PidControlWithThresholdsUpdate();
      break;
      /*sva-design: add for new aggregation controller AGG_CTRL
    case AGG_CTRL:
      break;
      sva-design*/
    default:
      NS_FATAL_ERROR("Unspecified Aggregation Controller " << m_type);
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
  double totalTimeAllowance = 0;
  double tmpTimeAllowance = 0;
  PidIterator it;
  Ptr<PerStaQInfo> sta;
  for (it = m_ctrl.begin(); it != m_ctrl.end(); ++it)
    {
      sta = m_perStaQInfo->GetByMac(it->first); //tid = UP_VI by default, TODO: this should be made generic
      NS_ASSERT(sta);

      //apply input signal to controller and update controller
      it->second->SetInputSignal(PidController::InSigType (sta->GetAvgSize(), sta->GetAvgSizeBytes(), sta->GetPrEmpty()));
      tmpTimeAllowance = it->second->ComputeOutput();
      totalTimeAllowance += tmpTimeAllowance;
    }

  //adjust time allowance to not exceed service interval
  double adjustment = 1;
  if (totalTimeAllowance > m_serviceInterval)
      adjustment = totalTimeAllowance/m_serviceInterval;
  //start adjusting output signals to fit in one service interval
  for (it = m_ctrl.begin(); it != m_ctrl.end(); ++it)
    {
      tmpTimeAllowance = it->second->UpdateController(adjustment);
      sta = m_perStaQInfo->GetByMac(it->first);
      sta->SetTimeAllowance(Seconds(tmpTimeAllowance));
      //sva: sta->SetTimeAllowance(m_timeAllowance);


#ifdef SVA_DEBUG
std::cout << Simulator::Now().GetSeconds() << " AggregationController (PidController) " << sta->GetMac()
    << " err= " << it->second->GetErrorSignal() << " ctrlSignal= " << m_ctrl[sta->GetMac()]->GetControlSignal().sig
    << " curTimeAllowance= " << sta->GetTimeAllowance().GetSeconds()*1000 << " msec"
    << " newTimeAllowance= " << tmpTimeAllowance*1000 << " msec"
    << " avgServed= " << sta->GetAvgServedPackets()
    << " avgQueue= " << sta->GetAvgSize()
    << " derivative= " << m_ctrl[sta->GetMac()]->GetDerivative()
    << " integral= " << m_ctrl[sta->GetMac()]->GetIntegral()
    << " reference= " << m_ctrl[sta->GetMac()]->GetReference()
    << " totalAllowance= " << totalTimeAllowance
    << " adjust= " << adjustment
    << "\n";
#endif

    }
}


void
TimeAllowanceAggregationController::PidControlWithThresholdsUpdate (void)
{//TODO: define perSta targetDVP and Dmax
  if (!m_perStaQInfo)//not supported
    {
      return ;
    }
  double totalTimeAllowance = 0;
  double tmpTimeAllowance = 0;
  PidIterator it;
  Ptr<PidControllerWithThresholds> controller;
  Ptr<PerStaQInfo> sta;
  for (it = m_ctrl.begin(); it != m_ctrl.end(); ++it)
    {
      controller = it->second->GetObject<PidControllerWithThresholds>();
      sta = m_perStaQInfo->GetByMac(it->first); //tid = UP_VI by default, TODO: this should be made generic
      NS_ASSERT(sta);

      //apply input signal to controller and update controller
      controller->SetInputSignal(PidControllerWithThresholds::InSigType (sta->GetAvgSize(), sta->GetAvgSizeBytes(), sta->GetPrEmpty()));
      tmpTimeAllowance = controller->ComputeOutput();
      totalTimeAllowance += tmpTimeAllowance;
    }

  //adjust time allowance to not exceed service interval
  double adjustment = 1;
  if (totalTimeAllowance > m_serviceInterval)
      adjustment = totalTimeAllowance/m_serviceInterval;
  //start adjusting output signals to fit in one service interval
  for (it = m_ctrl.begin(); it != m_ctrl.end(); ++it)
    {
      controller = it->second->GetObject<PidControllerWithThresholds>();
      tmpTimeAllowance = controller->UpdateController(adjustment);
      sta = m_perStaQInfo->GetByMac(it->first);
      //sva: actual line of code
      sta->SetTimeAllowance(Seconds(tmpTimeAllowance));
      //sva: for fixed time allowance      sta->SetTimeAllowance(m_timeAllowance);

#ifdef SVA_DEBUG
std::cout << Simulator::Now().GetSeconds() << " AggregationController (PidControllerWithThresholds) " << sta->GetMac()
    << " err= " << controller->GetErrorSignal() << " ctrlSignal= " << controller->GetControlSignal().sig
    << " curTimeAllowance= " << sta->GetTimeAllowance().GetSeconds()*1000 << " msec"
    << " newTimeAllowance= " << tmpTimeAllowance*1000 << " msec"
    << " avgServed= " << sta->GetAvgServedPackets()
    << " avgQueue= " << sta->GetAvgSize()
    << " derivative= " << controller->GetDerivative()
    << " integral= " << controller->GetIntegral()
    << " reference= " << controller->GetReference()
    << " totalAllowance= " << totalTimeAllowance
    << " errCorr= " << controller->GetErrorCorrelation()
    << " thrHi= " << controller->GetHighThreshold()
    << " thrLo= " << controller->GetLowThreshold()
    << "\n";
#endif

    }
}


}  // namespace ns3

