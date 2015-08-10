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

PidControllerWithThresholds::PidStateType::PidStateType(double prevErr, double curErr, double prevOut, double curOut, double integral, double errMean, double errStdDev)
  : prevErr(prevErr), curErr(curErr), prevOut(prevOut), curOut(curOut), integral(integral), errMean(errMean), errStdDev(errStdDev)
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

  void
  PidControllerWithThresholds::SetStaQInfo (const Ptr<PerStaQInfo> sta)
  {
    m_staQ = sta;
  }

  PidControllerWithThresholds::PidControllerWithThresholds ()
  {//TODO
    return ;
  }

  PidControllerWithThresholds::~PidControllerWithThresholds ()
  {//TODO
    return ;
  }

  void
  PidControllerWithThresholds::SetInputParams (const InParamType &in)
  {
    m_inParam = in;
  }

  void
  PidControllerWithThresholds::SetInputSignal (const InSigType sig)
  {
    m_input = sig;
    return ;
  }

  PidController::InSigType
  PidControllerWithThresholds::GetInputSignal (void)
  {
    return m_input;
  }

  PidController::FeedbackSigType
  PidControllerWithThresholds::GetFeedbackSignal (void)
  {
    return m_feedback;
  }

  void
  PidControllerWithThresholds::UpdateFeedbackSignal (void)
  {
    m_feedback.avgServedPacketes = m_staQ->GetAvgServedPackets();
    m_feedback.avgServedBytes = m_staQ->GetAvgServedBytes();
//sva for debug    std::cout << "feedback signal = " << m_feedback.avgServedPacketes << "\n";
  }

  double
  PidControllerWithThresholds::ComputeOutput (void)
  {
    double err = ComputeErrorSignal();
    double integral = m_state.integral * (1-m_pidParam.wi) + err * m_pidParam.wi;
    double ctrl = m_pidParam.kp * err + m_pidParam.ki * integral + m_pidParam.kd * (err - m_state.curErr);
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
    if (!IsThresholdViolated(m_state.curErr))
      m_ctrl.sig = 0.0;
    m_output = std::max(0.0,(m_state.curOut + m_ctrl.sig)) / adjustment;
//sva for debug    std::cout << "curErr= " << m_state.curErr << " outputBeforeMax= "<< m_state.curOut + m_ctrl.sig << " actual output = " << m_output << " adjusted by " << adjustment << "\n";

    //update mean and standard deviation of error using moving average
    m_state.errMean = m_state.errMean * (1-m_pidParam.thrW) + m_state.curErr * m_pidParam.thrW;
    m_state.errStdDev = m_state.errStdDev * (1-m_pidParam.thrW) + std::abs(m_state.curErr - m_state.errMean) * m_pidParam.thrW;

    m_state.prevOut = m_state.curOut;
    m_state.curOut = m_output;
    return m_output;
  }

  PidController::CtrlSigType
  PidControllerWithThresholds::GetControlSignal (void)
  {
    return m_ctrl;
  }

  double
  PidControllerWithThresholds::GetOutputSignal (void)
  {
    return m_output;
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
  PidControllerWithThresholds::GetReference(void)
  {
    return ComputeErrorSignal() + m_feedback.avgServedPacketes;
  }

  double PidControllerWithThresholds::ComputeErrorSignal(void)
  {
    UpdateFeedbackSignal();
    double tmpPrEmpty = 0.999;
    if (m_input.prEmpty != 1) //prevent division by zero
      tmpPrEmpty = m_input.prEmpty;
    double err = -log(m_inParam.dvp)*m_inParam.si * m_input.avgQ/m_inParam.dMax/(1-tmpPrEmpty) - m_feedback.avgServedPacketes;
    return err;
  }

  bool
  PidControllerWithThresholds::IsThresholdViolated (double err)
  {
    double thrH = GetHighThreshold();
    double thrL = GetLowThreshold();
    if (thrH < 0 || thrL > 0)
      return true;
    else
      return false;
  }

  double PidControllerWithThresholds::GetHighThreshold (void)
  {
    return m_state.errMean + m_state.errStdDev * m_pidParam.thrH;
  }

  double PidControllerWithThresholds::GetLowThreshold (void)
  {
    return m_state.errMean - m_state.errStdDev * m_pidParam.thrL;
  }

}  // end namespace ns3
