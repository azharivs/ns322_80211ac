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
        ;
    return tid;
  }

  bool
  PidController::Init (void)
  {//TODO
    return true;
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
  PidController::SetInputSignal (const InSigType &sig)
  {//TODO
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

  OutSigType
  PidController::UpdateController (void)
  {//TODO
    return m_output;
  }

  CtrlSigType
  PidController::GetControlSignal (void)
  {
    return m_ctrl;
  }

  OutSigType
  PidController::GetOutputSignal (void)
  {
    return m_output;
  }

}  // end namespace ns3
