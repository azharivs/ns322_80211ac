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

#ifndef PID_CONTROLLER_H
#define PID_CONTROLLER_H

#include <math.h>
#include <algorithm>

namespace ns3 {

/**
 * \ingroup wifi
 * PID Controller
 *
 */

typedef struct
{
    double prevErr; //!< previous error signal
    double curErr; //!< current error signal
}PidStateType;

typedef struct
{
    double kp; //!< proportional gain
    double ki; //!< integral gain
    double kd; //!< derivative gain
    double wi; //!< recent sample weight for approximating the integral term as a moving average (must be between zero and one)
}PidParamType;

typedef struct
{
    double dvp; //!< Target delay violation probability
    double dMax; //!< Maximum tolerable delay
    double si; //!< Length of service interval in seconds
}InParamType;

typedef struct
{
    double avgQ; //!<average queue length in packets
    double avgQBytes; //!<average queue length in bytes
    double prEmpty; //!<probability of empty queue
}InSigType;

typedef struct
{
    double avgServedPacketes; //!<average served packets in a service interval
    double avgServedBytes; //!<average served bytes in a service interval
}FeedbackSigType;

typedef struct
{
    double deltaTimeAllowance; //!<delta that should be applied to current time allowance
}OutSigType;

typedef struct
{
    double sig; //!<delta that should be applied to current time allowance
}CtrlSigType;

class PidController : public Object
{
public:
  static TypeId GetTypeId (void);
  PidController (void);
  ~PidController (void);

  /*
   * Initializes controller parameters
   */
  bool Init (void);

  /*
   * returns the current value of the input signal to the controller
   */
  void SetInputSignal (const InSigType &sig);

  /*
   * returns the current value of the input signal to the controller
   */
  InSigType GetInputSignal (void);

  /*
   * returns current value of the feedback signal
   */
  FeedbackSigType GetFeedbackSignal (void);

  /*
   * Calculates control and output signals
   * Returns new output signal
   */
  OutSigType UpdateController (void);

  /*
   * returns the most recent value of the control signal
   */
  CtrlSigType GetControlSignal (void);

  /*
   * returns the current value of the controller output signal
   */
  OutSigType GetOutputSignal (void);

protected:

  /*
   * returns the current value of the error signal at the input to the controller
   * that is: target - actual
   *
  double GetErrorSignal(void);

  void DoGetInputSignal(void);
  */

  double m_weightIntegral; //!<recent sample weight for approximating the integral term as a moving average. Will initialize m_pidParam.wi because I didn't know how to directly access that from the attribute system
  double m_kp; //!< proportional coefficient. Will initialize m_pidParam.kp because I didn't know how to directly access that from the attribute system
  double m_ki; //!< integral coefficient. Will initialize m_pidParam.ki because I didn't know how to directly access that from the attribute system
  double m_kd; //!< derivative coefficient. Will initialize m_pidParam.kd because I didn't know how to directly access that from the attribute system
  PidStateType m_state; //!<PID controller state: prevErr,curErr
  PidParamType m_pidParam; //!<PID controller parameters: kp,ki,kd,wi
  InParamType m_inParam; //!<input parametes: dvp, dMax
  InSigType m_input; //!<current value of input signal
  OutSigType m_output; //!<current value of output signal
  FeedbackSigType m_feedback; //!<current value of feedback signal
  CtrlSigType m_ctrl; //!<current value of control signal

};


}  // namespace ns3

#endif /* PID_CONTROLLER_H */
