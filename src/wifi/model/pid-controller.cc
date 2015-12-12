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

//TODO: need to clean up the feedback signal. At the moment there is no clear distinction between feedback and input signals.
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

PidController::PidStateType::PidStateType(double prevErr, double curErr, double prevOut, double curOut, double integral)
  : prevErr(prevErr), curErr(curErr), prevOut(prevOut), curOut(curOut), integral(integral)
{
}

PidController::PidParamType::PidParamType(double kp, double ki, double kd, double wi)
  : kp(kp), ki(ki), kd(kd), wi(wi)
{
}

PidController::InParamType::InParamType(double dvp, double dMax, double si)
  : dvp(dvp), dMax(dMax), si(si)
{
}

PidController::InSigType::InSigType(double avgQ, double avgQBytes, double prEmpty)
  : avgQ(avgQ), avgQBytes(avgQBytes), prEmpty(prEmpty)
{
}

PidController::FeedbackSigType::FeedbackSigType(double avgServedPacketes, double avgServedBytes)
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

  void PidController::ForceOutput (double output)
  {
    m_state.prevOut = output;
    m_state.curOut = output;
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
  PidController::SetInputParams (const PidController::InParamType &in)
  {
    m_inParam = in;
  }

  void
  PidController::SetInputSignal (const PidController::InSigType sig)
  {
    m_input = sig;
    return ;
  }

  PidController::InSigType
  PidController::GetInputSignal (void)
  {
    return m_input;
  }

  PidController::FeedbackSigType
  PidController::GetFeedbackSignal (void)
  {
    return m_feedback;
  }

  void
  PidController::UpdateFeedbackSignal (void)
  {
    m_feedback.avgServedPacketes = m_staQ->GetAvgArrivalRate();  //sva original: m_staQ->GetAvgServedPackets();
    m_feedback.avgServedBytes = m_staQ->GetAvgArrivalRateBytes(); //sva original: m_staQ->GetAvgServedBytes();
//sva for debug    std::cout << "feedback signal = " << m_feedback.avgServedPacketes << "\n";
  }

  double PidController::ErrorConditioning(double err)
  {
    //return err; //sva: for now we have disabled error conditioning
    if (err >= 0)
      return atan(log(1+fabs(err)));
    else
      return -atan(log(1+fabs(err)));
  }

  double PidController::CtrlConditioning(double ctrl)
  {
    return ctrl;//sva: for now we disable ctrl conditioning
    if (ctrl >= 0)
      return atan(log(1+fabs(ctrl)));
    else
      return -atan(log(1+fabs(ctrl)));
  }

  double
  PidController::ComputeOutput (void)
  {
    double err = ComputeErrorSignal();
    double integral = m_state.integral * (1-m_pidParam.wi) + err * m_pidParam.wi;
    double ctrl = m_pidParam.kp * err + m_pidParam.ki * integral + m_pidParam.kd * (err - m_state.curErr);
    ctrl = CtrlConditioning(ctrl);
    double output = std::max(0.0,m_state.curOut + ctrl);
//sva for debug    std::cout << "err= " << err << " computed output = " << output << "\n";
    return output;
  }

  double
  PidController::UpdateController (double adjustment)
  {
    m_state.prevErr = m_state.curErr;
    m_state.curErr = ComputeErrorSignal();
    m_state.integral = m_state.integral * (1-m_pidParam.wi) + m_state.curErr * m_pidParam.wi;
    m_ctrl.sig = m_pidParam.kp * m_state.curErr + m_pidParam.ki * m_state.integral + m_pidParam.kd * (m_state.curErr - m_state.prevErr);
    m_ctrl.sig = CtrlConditioning(m_ctrl.sig);
    m_output = std::max(0.0,(m_state.curOut + m_ctrl.sig)) / adjustment;
//sva for debug    std::cout << "curErr= " << m_state.curErr << " outputBeforeMax= "<< m_state.curOut + m_ctrl.sig << " actual output = " << m_output << " adjusted by " << adjustment << "\n";
    m_state.prevOut = m_state.curOut;
    m_state.curOut = m_output;
    return m_output;
  }

  PidController::CtrlSigType
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


  double
  PidController::GetDerivative(void)
  {
    return m_state.curErr-m_state.prevErr;
  }


  double
  PidController::GetIntegral(void)
  {
    return m_state.integral;
  }

  double
  PidController::GetReference(void)
  {//for now it returns the reference queue size
    UpdateFeedbackSignal();
    double tmpPrEmpty = 0.999;
    if (m_input.prEmpty != 1) //prevent division by zero
      tmpPrEmpty = m_input.prEmpty;
    double rho = 1-tmpPrEmpty; //sva added later for second form of error signal
    //return -rho*m_feedback.avgServedPacketes*m_inParam.dMax / log(m_inParam.dvp) - rho/2;
    //sva accurate but fluctuating:
    return -rho*m_feedback.avgServedPacketes*m_inParam.dMax / log(m_inParam.dvp/rho) - rho/2;
  }

  double PidController::ComputeErrorSignal(void)
  {
    UpdateFeedbackSignal();
    double tmpPrEmpty = 0.999;
    if (m_input.prEmpty != 1) //prevent division by zero
      tmpPrEmpty = m_input.prEmpty;
    //sva original: double err = -log(m_inParam.dvp)*m_inParam.si * m_input.avgQ/m_inParam.dMax/(1-tmpPrEmpty) - m_feedback.avgServedPacketes;
    double rho = 1-tmpPrEmpty; //sva added later for second form of error signal
    //double err = -rho*m_feedback.avgServedPacketes/(0.5*rho+m_input.avgQ) - log(m_inParam.dvp)/m_inParam.dMax; //sva added later for second form of error signal
    //sva should be this but changed to make it smoother:
    double err = -rho*m_feedback.avgServedPacketes/(0.5*rho+m_input.avgQ) - log(m_inParam.dvp/rho)/m_inParam.dMax; //sva added later for second form of error signal
    err = ErrorConditioning(err);
    return err;
  }

}  // end namespace ns3
