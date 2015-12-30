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
#ifndef SIMPLE_CONTROLLER_H
#define SIMPLE_CONTROLLER_H

#include <math.h>
#include <algorithm>
//#include "per-sta-q-info.h"
#include "pid-controller.h"

namespace ns3 {
  //class PerStaQInfo;
  //class Controller;

/**
 * \ingroup wifi
 * Simple Controller
 *
 */

class SimpleController : public Controller
{
public:
  static TypeId GetTypeId (void);
  SimpleController (void);
  ~SimpleController (void);

  /*
   * Required type and struct definitions
   */

  struct SimpleStateType
  {
      double prevErr; //!< previous error signal
      double curErr; //!< current error signal
      double prevOut; //!< previous output (e.g., time allowance)
      double curOut; //!< Current output (e.g., time allowance)
      SimpleStateType(double prevErr=0, double curErr=0, double prevOut=0, double curOut=0);
  };

  struct InParamType
  {
      double dvp; //!< Target delay violation probability
      double dMax; //!< Maximum tolerable delay
      double si; //!< Length of service interval in seconds
      InParamType(double dvp=0, double dMax=0, double si=0);
  };

  struct InSigType
  {
      double avgQ; //!<average queue length in packets
      double avgQBytes; //!<average queue length in bytes
      double prEmpty; //!<probability of empty queue
      InSigType(double avgQ=0, double avgQBytes=0, double prEmpty=0);
  };

  struct FeedbackSigType
  {
      double avgServedPacketes; //!<average served packets in a service interval
      double avgServedBytes; //!<average served bytes in a service interval
      FeedbackSigType(double avgServedPacketes=0, double avgServedBytes=0);
  };

  struct CtrlSigType
  {
      double sig; //!<delta that should be applied to current output
  };


  /*
   * Initializes controller parameters
   */
  virtual bool Init (void);

  /*
   * forces output and previous output to a certain value.
   * can be used for initialization as well.
   */
  virtual void ForceOutput (double output);


  virtual void SetStaQInfo (const Ptr<PerStaQInfo> sta);
  /*
   * sets the input parameters of the controller: dvp,dMax,SI
   */
  virtual void SetInputParams (const InParamType &in);

  /*
   * sets the current value of the input signal to the controller
   */
  virtual void SetInputSignal (const InSigType sig);

  /*
   * returns the current value of the input signal to the controller
   */
  virtual InSigType GetInputSignal (void);

  /*
   * returns current value of the feedback signal
   */
  virtual FeedbackSigType GetFeedbackSignal (void);

  /*
   * Calculates control and output signals
   * uses adjustment parameter to scale output
   * Returns new output signal
   */
  virtual double UpdateController (double adjustment=1);

  /*
   * Calculates control and output signals
   * but does not change controller state variables
   * Returns new output signal
   */
  virtual double ComputeOutput ();

  /*
   * returns the most recent value of the control signal
   */
  virtual CtrlSigType GetControlSignal (void);

  /*
   * returns the current value of the controller output signal
   */
  virtual double GetOutputSignal (void);

  /*
   * returns the current value of the error signal at the input to the controller
   * that is: target - actual
   */
  virtual double GetErrorSignal(void);
  virtual double GetReference(void);

protected:

  virtual double ComputeErrorSignal(void);
  virtual double ErrorConditioning(double err);
  virtual double CtrlConditioning(double ctrl);
  virtual void UpdateFeedbackSignal(void);

  //void DoGetInputSignal(void);


  SimpleStateType m_state; //!<PID controller state: prevErr,curErr
  InParamType m_inParam; //!<input parametes: dvp, dMax
  InSigType m_input; //!<current value of input signal
  double m_output; //!<current value of output signal
  FeedbackSigType m_feedback; //!<current value of feedback signal
  CtrlSigType m_ctrl; //!<current value of control signal
  Ptr<PerStaQInfo> m_staQ; //!<Pointer to PerStaQInfo element being used by this controller
};


}  // namespace ns3




#endif /* SIMPLE_CONTROLLER_H */
