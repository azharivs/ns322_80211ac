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

//TODO: For now all control and input signals are of type double
//TODO: Later, c++ templates can be used to make it support generic
//TODO: signals such as Time or even a complex struct

#include <math.h>
#include <algorithm>
#include "ns3/object.h"
#include "ns3/double.h"
#include "pid-controller-with-thresholds.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (PidControllerWithThresholds);
/**
 * \ingroup wifi
 * PID Controller
 *
 */

PidControllerWithThresholds::PidStateType::PidStateType(double prevErr, double curErr, double prevOut, double curOut, double integral, double errMean, double errStdDev, double errCorr)
  : prevErr(prevErr), curErr(curErr), prevOut(prevOut), curOut(curOut), integral(integral), errMean(errMean), errStdDev(errStdDev), errCorr(errCorr)
{
}

PidControllerWithThresholds::PidParamType::PidParamType(double kp, double ki, double kd, double wi, double thrW, double thrH, double thrL)
  : kp(kp), ki(ki), kd(kd), wi(wi), thrW(thrW), thrH(thrH), thrL(thrL)
{
}


  TypeId
  PidControllerWithThresholds::GetTypeId (void)
  {
    static TypeId tid = TypeId ("ns3::PidControllerWithThresholds")
        .SetParent<PidController> ()
        .AddConstructor<PidControllerWithThresholds> ()
        .AddAttribute ("ThrWeight", "Recent sample moving average weight for approximating standard deviation of error signal of the threshold based PID controller",
                       DoubleValue (0.05),
                       MakeDoubleAccessor (&PidControllerWithThresholds::m_thrW),
                       MakeDoubleChecker<double> ())
        .AddAttribute ("ThrHighCoef", "Mean error multiplier for defining high threshold of the threshold based PID controller",
                       DoubleValue (1.0),
                       MakeDoubleAccessor (&PidControllerWithThresholds::m_thrH),
                       MakeDoubleChecker<double> ())
        .AddAttribute ("ThrLowCoef", "Mean error multiplier for defining high threshold of the threshold based PID controller",
                      DoubleValue (1.0),
                      MakeDoubleAccessor (&PidControllerWithThresholds::m_thrL),
                      MakeDoubleChecker<double> ())
        ;
    return tid;
  }

  bool
  PidControllerWithThresholds::Init (void)
  {//TODO
    m_pidParam.kp = m_kp;
    m_pidParam.ki = m_ki;
    m_pidParam.kd = m_kd;
    m_pidParam.wi = m_weightIntegral;
    m_pidParam.thrW = m_thrW;
    m_pidParam.thrL = m_thrL;
    m_pidParam.thrH = m_thrH;
    return true;
  }

  PidControllerWithThresholds::PidControllerWithThresholds ()
  {//TODO
    return ;
  }

  PidControllerWithThresholds::~PidControllerWithThresholds ()
  {//TODO
    return ;
  }


  double
  PidControllerWithThresholds::GetErrorSignal(void)
  {
    return m_state.curErr;
  }


  double
  PidControllerWithThresholds::GetDerivative(void)
  {
    return m_state.curErr-m_state.prevErr;
  }


  double
  PidControllerWithThresholds::GetIntegral(void)
  {
    return m_state.integral;
  }

  double
  PidControllerWithThresholds::ComputeOutput (void)
  {
    double err = ComputeErrorSignal();
    double integral = m_state.integral * (1-m_pidParam.wi) + err * m_pidParam.wi;
    double ctrl = m_pidParam.kp * err + m_pidParam.ki * integral + m_pidParam.kd * (err - m_state.curErr);
    ctrl = CtrlConditioning(ctrl);
    if (!IsThresholdViolated(err))
      ctrl = 0;
    double output = std::max(0.0,m_state.curOut + ctrl);
//sva for debug    std::cout << "err= " << err << " computed output = " << output << "\n";
    return output;
  }

  double
  PidControllerWithThresholds::UpdateController (double adjustment)
  {
    m_state.prevErr = m_state.curErr;
    m_state.curErr = ComputeErrorSignal();
    m_state.integral = m_state.integral * (1-m_pidParam.wi) + m_state.curErr * m_pidParam.wi;
    m_ctrl.sig = m_pidParam.kp * m_state.curErr + m_pidParam.ki * m_state.integral + m_pidParam.kd * (m_state.curErr - m_state.prevErr);
    m_ctrl.sig = CtrlConditioning(m_ctrl.sig);
    if (!IsThresholdViolated(m_state.curErr))
      m_ctrl.sig = 0.0;
    m_output = std::max(0.0,(m_state.curOut + m_ctrl.sig)) / adjustment;
//sva for debug    std::cout << "curErr= " << m_state.curErr << " outputBeforeMax= "<< m_state.curOut + m_ctrl.sig << " actual output = " << m_output << " adjusted by " << adjustment << "\n";

    //update mean and standard deviation of error using moving average
    m_state.errMean = m_state.errMean * (1-m_pidParam.thrW) + m_state.curErr * m_pidParam.thrW;
    m_state.errStdDev = m_state.errStdDev * (1-m_pidParam.thrW) + fabs(m_state.curErr - m_state.errMean) * m_pidParam.thrW;

    m_state.prevOut = m_state.curOut;
    m_state.curOut = m_output;
    m_state.errCorr = m_state.errCorr * (1-m_pidParam.thrW) + m_state.prevErr*m_state.curErr* m_pidParam.thrW;
    return m_output;
  }


  bool
  PidControllerWithThresholds::IsThresholdViolated (double err)
  {
    //if (abs(m_state.errCorr) < 200) //sva: to be enabled for error correlation based triggering
    //  return false;  //sva: to be enabled for ...
    double thrH = GetHighThreshold();
    double thrL = GetLowThreshold();
    if (thrH < 0 || thrL > 0)
      return true;
    else
      return false;
  }

  double
  PidControllerWithThresholds::GetHighThreshold (void)
  {
    return m_state.errMean + m_state.errStdDev * m_pidParam.thrH;
  }

  double
  PidControllerWithThresholds::GetLowThreshold (void)
  {
    return m_state.errMean - m_state.errStdDev * m_pidParam.thrL;
  }

  double
  PidControllerWithThresholds::GetErrorCorrelation (void)
  {
    return m_state.errCorr;// /m_state.errStdDev/m_state.errStdDev;
  }

}  // end namespace ns3
