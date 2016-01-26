/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) Dec 25, 2015 IUST
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

#include <math.h>
#include <algorithm>
#include "ns3/object.h"
#include "ns3/double.h"
#include "simple-controller.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (SimpleController);
/**
 * \ingroup wifi
 * Simple Controller
 *
 */

SimpleController::SimpleStateType::SimpleStateType(double prevErr, double curErr, double prevOut, double curOut)
  : prevErr(prevErr), curErr(curErr), prevOut(prevOut), curOut(curOut)
{
}

SimpleController::InParamType::InParamType(double dvp, double dMax, double si)
  : dvp(dvp), dMax(dMax), si(si)
{
}

SimpleController::InSigType::InSigType(double avgQ, double avgQBytes, double prEmpty)
  : avgQ(avgQ), avgQBytes(avgQBytes), prEmpty(prEmpty)
{
}

SimpleController::FeedbackSigType::FeedbackSigType(double avgServedPacketes, double avgServedBytes)
  : avgServedPacketes(avgServedPacketes), avgServedBytes(avgServedBytes)
{
}

  TypeId
  SimpleController::GetTypeId (void)
  {
    static TypeId tid = TypeId ("ns3::SimpleController")
        .SetParent<Controller> ()
        .AddConstructor<SimpleController> ()
        ;
    return tid;
  }

  bool
  SimpleController::Init (void)
  {//TODO
    return true;
  }

  void SimpleController::ForceOutput (double output)
  {
    m_state.prevOut = output;
    m_state.curOut = output;
  }

  void
  SimpleController::SetStaQInfo (const Ptr<PerStaQInfo> sta)
  {
    m_staQ = sta;
  }

  SimpleController::SimpleController ()
  {//TODO
    return ;
  }

  SimpleController::~SimpleController ()
  {//TODO
    return ;
  }

  void
  SimpleController::SetInputParams (const SimpleController::InParamType &in)
  {
    m_inParam = in;
  }

  void
  SimpleController::SetInputSignal (const SimpleController::InSigType sig)
  {
    m_input = sig;
    return ;
  }

  SimpleController::InSigType
  SimpleController::GetInputSignal (void)
  {
    return m_input;
  }

  SimpleController::FeedbackSigType
  SimpleController::GetFeedbackSignal (void)
  {
    return m_feedback;
  }

  void
  SimpleController::UpdateFeedbackSignal (void)
  {
    m_feedback.avgServedPacketes = m_staQ->GetAvgArrivalRate();  //sva original: m_staQ->GetAvgServedPackets();
    m_feedback.avgServedBytes = m_staQ->GetAvgArrivalRateBytes(); //sva original: m_staQ->GetAvgServedBytes();
    //std::cout << "feedback signal = " << m_feedback.avgServedBytes << "\n";
  }

  double SimpleController::ErrorConditioning(double err)
  {
    return err; //sva: for now we have disabled error conditioning
    if (err >= 0)
      return atan(log(1+fabs(err)));
    else
      return -atan(log(1+fabs(err)));
  }

  double SimpleController::CtrlConditioning(double ctrl)
  {
    return ctrl;//sva: for now we disable ctrl conditioning
    if (ctrl >= 0)
      return atan(log(1+fabs(ctrl)));
    else
      return -atan(log(1+fabs(ctrl)));
  }

  double
  SimpleController::ComputeOutput (void)
  {
    double err = ComputeErrorSignal();
    double output = std::max(0.0,err);
    //std::cout << "err= " << err << " computed output = " << output << "\n";
    return output;
  }

  double
  SimpleController::UpdateController (double adjustment)
  {
    m_state.prevErr = m_state.curErr;
    m_state.curErr = ComputeErrorSignal();
    m_ctrl.sig = m_state.curErr;//m_pidParam.kp * m_state.curErr + m_pidParam.ki * m_state.integral + m_pidParam.kd * (m_state.curErr - m_state.prevErr);
    m_ctrl.sig = CtrlConditioning(m_ctrl.sig);
    m_output = std::max(0.0,m_ctrl.sig) / adjustment;
//sva for debug    std::cout << "curErr= " << m_state.curErr << " outputBeforeMax= "<< m_state.curOut + m_ctrl.sig << " actual output = " << m_output << " adjusted by " << adjustment << "\n";
    m_state.prevOut = m_state.curOut;
    m_state.curOut = m_output;
    return m_output;
  }

  SimpleController::CtrlSigType
  SimpleController::GetControlSignal (void)
  {
    return m_ctrl;
  }

  double
  SimpleController::GetOutputSignal (void)
  {
    return m_output;
  }

  double
  SimpleController::GetErrorSignal(void)
  {
    return m_state.curErr;
  }

  double
  SimpleController::GetReference(void)
  {//for now it returns the reference queue size
    UpdateFeedbackSignal();
    double tmpPrEmpty = 0.999;
    if (m_input.prEmpty != 1) //prevent division by zero
      tmpPrEmpty = m_input.prEmpty;
    double rho = 1-tmpPrEmpty; //sva added later for second form of error signal
    //return -rho*m_feedback.avgServedPacketes*m_inParam.dMax / log(m_inParam.dvp) - rho/2;
    //sva accurate but fluctuating:
    return -rho*m_feedback.avgServedBytes*m_inParam.dMax / log(m_inParam.dvp/rho) - rho/2;
  }

  double SimpleController::ComputeErrorSignal(void)
  {
    UpdateFeedbackSignal();
    /* sva: put here for applying queue length as reference signal. to be removed
    double tmpPrEmpty = 0.999;
    if (m_input.prEmpty != 1) //prevent division by zero
      tmpPrEmpty = m_input.prEmpty;
    //sva original: double err = -log(m_inParam.dvp)*m_inParam.si * m_input.avgQ/m_inParam.dMax/(1-tmpPrEmpty) - m_feedback.avgServedPacketes;
    double rho = 1-tmpPrEmpty; //sva added later for second form of error signal
    to be removed*/
    //double err = -rho*m_feedback.avgServedPacketes/(0.5*rho+m_input.avgQ) - log(m_inParam.dvp)/m_inParam.dMax; //sva added later for second form of error signal
    //sva should be this but changed to make it smoother:
    //double err = -rho*m_feedback.avgServedBytes/(0.5*rho+m_input.avgQBytes) - log(m_inParam.dvp/rho)/m_inParam.dMax; //sva added later for second form of error signal
    //older code uses average q size:
    //double err = m_input.avgQBytes - GetReference(); //sva added later for second form of error signal
    //m_input.avgQ uses a bad naming. It represents current queue size in bytes
    double err = m_input.avgQ - GetReference(); //sva added later for second form of error signal
    err = ErrorConditioning(err);

    return err;
  }

}  // end namespace ns3


