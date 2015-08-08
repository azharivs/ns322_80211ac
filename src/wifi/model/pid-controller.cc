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
#include "pid-controller.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (PidController);
/**
 * \ingroup wifi
 * PID Controller
 *
 */

PidStateType::PidStateType(double prevErr, double curErr, double prevOut, double curOut, double integral)
  : prevErr(prevErr), curErr(curErr), prevOut(prevOut), curOut(curOut), integral(integral)
{
}

PidParamType::PidParamType(double kp, double ki, double kd, double wi)
  : kp(kp), ki(ki), kd(kd), wi(wi)
{
}

InParamType::InParamType(double dvp, double dMax, double si)
  : dvp(dvp), dMax(dMax), si(si)
{
}

InSigType::InSigType(double avgQ, double avgQBytes, double prEmpty)
  : avgQ(avgQ), avgQBytes(avgQBytes), prEmpty(prEmpty)
{
}

FeedbackSigType::FeedbackSigType(double avgServedPacketes, double avgServedBytes)
  : avgServedPacketes(avgServedPacketes), avgServedBytes(avgServedBytes)
{
}
  TypeId
  PidController::GetTypeId (void)
  {
    static TypeId tid = TypeId ("ns3::PidController")
        .SetParent<Object> ()
        .AddConstructor<PidController> ()
        .AddAttribute ("MovingAverageWeight", "Recent sample moving average weight for approximating the integral term of the PID controller",
                       DoubleValue (0.1),
                       MakeDoubleAccessor (&PidController::m_weightIntegral),
                       MakeDoubleChecker<double> ())
        .AddAttribute ("KP", "Proportional coefficient used with the PID controller",
                       DoubleValue (1.0),
                       MakeDoubleAccessor (&PidController::m_kp),
                       MakeDoubleChecker<double> ())
        .AddAttribute ("KI", "Integral coefficient used with the PID controller",
                      DoubleValue (1.0),
                      MakeDoubleAccessor (&PidController::m_ki),
                      MakeDoubleChecker<double> ())
       .AddAttribute ("KD", "Derivative coefficient used with the PID controller",
                      DoubleValue (1.0),
                      MakeDoubleAccessor (&PidController::m_kd),
                      MakeDoubleChecker<double> ())
        ;
    return tid;
  }

  bool
  PidController::Init (void)
  {//TODO
    m_pidParam.kp = m_kp;
    m_pidParam.ki = m_ki;
    m_pidParam.kd = m_kd;
    m_pidParam.wi = m_weightIntegral;
    return true;
  }

  void
  PidController::SetStaQInfo (const Ptr<PerStaQInfo> sta)
  {
    m_staQ = sta;
  }

  PidController::PidController ()
  {//TODO
    return ;
  }

  PidController::~PidController ()
  {//TODO
    return ;
  }

  void
  PidController::SetInputParams (const InParamType &in)
  {
    m_inParam = in;
  }

  void
  PidController::SetInputSignal (const InSigType &sig)
  {
    m_input = sig;
    return ;
  }

  InSigType
  PidController::GetInputSignal (void)
  {
    return m_input;
  }

  FeedbackSigType
  PidController::GetFeedbackSignal (void)
  {
    return m_feedback;
  }

  void
  PidController::UpdateFeedbackSignal (void)
  {
    m_feedback.avgServedPacketes = m_staQ->GetAvgServedPackets();
    m_feedback.avgServedBytes = m_staQ->GetAvgServedBytes();
  }

  double
  PidController::ComputeOutput (void)
  {
    double err = ComputeErrorSignal();
    double integral = m_state.integral * (1-m_pidParam.wi) + err * m_pidParam.wi;
    double ctrl = m_pidParam.kp * err + m_pidParam.ki * integral + m_pidParam.kd * (err - m_state.curErr);
    double output = std::max(0.0,m_state.curOut + ctrl);
    return output;
  }

  double
  PidController::UpdateController (double adjustment)
  {
    m_state.prevErr = m_state.curErr;
    m_state.curErr = ComputeErrorSignal();
    m_state.integral = m_state.integral * (1-m_pidParam.wi) + m_state.curErr * m_pidParam.wi;
    m_ctrl.sig = m_pidParam.kp * m_state.curErr + m_pidParam.ki * m_state.integral + m_pidParam.kd * (m_state.curErr - m_state.prevErr);
    m_output = std::max(0.0,(m_state.prevOut + m_ctrl.sig)) / adjustment;
    std::cout << "PidController::UpdateController (KP,KI,KD)= (" << m_pidParam.kp << "," << m_pidParam.ki << "," << m_pidParam.kd
        << ") error= " << m_state.curErr << " control= " << m_ctrl.sig << " output= " << m_output << "\n";
    m_state.prevOut = m_state.curOut;
    m_state.curOut = m_output;
    return m_output;
  }

  CtrlSigType
  PidController::GetControlSignal (void)
  {
    return m_ctrl;
  }

  double
  PidController::GetOutputSignal (void)
  {
    return m_output;
  }

  double
  PidController::GetErrorSignal(void)
  {
    return m_state.curErr;
  }

  double PidController::ComputeErrorSignal(void)
  {
    double tmpPrEmpty = 0.999;
    if (m_input.prEmpty != 1) //prevent division by zero
      tmpPrEmpty = m_input.prEmpty;
    double err = -log(m_inParam.dvp)*m_inParam.si * m_input.avgQ/m_inParam.dMax/(1-tmpPrEmpty) - m_feedback.avgServedPacketes;
    return err;
  }

}  // end namespace ns3
