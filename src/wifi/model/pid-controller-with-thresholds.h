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

#ifndef PID_CONTROLLER_WITH_THRESHOLDS_H
#define PID_CONTROLLER_WITH_THRESHOLDS_H

#include <math.h>
#include <algorithm>
#include "per-sta-q-info.h"
#include "pid-controller.h"

namespace ns3 {

/**
 * \ingroup wifi
 * PID Controller With Thresholds
 *
 */

class PidControllerWithThresholds : public PidController
{
public:
  static TypeId GetTypeId (void);
  PidControllerWithThresholds (void);
  ~PidControllerWithThresholds (void);

  /*
   * Required struct and type definitions
   */
  struct PidStateType
  {
      double prevErr; //!< previous error signal
      double curErr; //!< current error signal
      double prevOut; //!< previous output (e.g., time allowance)
      double curOut; //!< Current output (e.g., time allowance)
      double integral; //!< Current value of integral term
      double errMean; //!< Mean value of error signal used to calculate thresholds
      double errStdDev; //!< Standard deviation of error signal used to calculate thresholds
      double errCorr; //!< Error signal correlation
      PidStateType(double prevErr=0, double curErr=0, double prevOut=0, double curOut=0, double integral=0, double errMean=0, double errStdDev=0, double errCorr=0);
  };

  struct PidParamType
  {
      double kp; //!< proportional gain
      double ki; //!< integral gain
      double kd; //!< derivative gain
      double wi; //!< recent sample weight for approximating the integral term as a moving average (must be between zero and one)
      double thrW; //!< recent sample weight for approximating the error mean and standard variation as a moving average (must be between zero and one)
      double thrH; //!< standard deviation multiplier for deriving the high threshold (>=0)
      double thrL; //!< standard deviation multiplier for deriving the low threshold (>=0)
      PidParamType(double kp=0, double ki=0, double kd=0, double wi=0, double thrW=0, double thrH=0, double thrL=0);
  };

  /*
   * Initializes controller parameters
   */
  bool Init (void);

  /*
   * Calculates control and output signals
   * uses adjustment parameter to scale output
   * Returns new output signal
   */
  double UpdateController (double adjustment=1);

  /*
   * Calculates control and output signals
   * but does not change controller state variables
   * Returns new output signal
   */
  double ComputeOutput ();

  /*
   * returns the current value of the error signal at the input to the controller
   * that is: target - actual
   */
  double GetErrorSignal(void);
  double GetDerivative(void);
  double GetIntegral(void);

  double GetHighThreshold(void);
  double GetLowThreshold(void);
  double GetErrorCorrelation (void);

protected:

  bool IsThresholdViolated (double err);

  //void DoGetInputSignal(void);


  double m_thrW;
  double m_thrH;
  double m_thrL;
  PidStateType m_state; //!<PID controller state: prevErr,curErr
  PidParamType m_pidParam; //!<PID controller parameters: kp,ki,kd,wi
  //InParamType m_inParam; //!<input parametes: dvp, dMax
  //InSigType m_input; //!<current value of input signal
  //double m_output; //!<current value of output signal
  //FeedbackSigType m_feedback; //!<current value of feedback signal
  //CtrlSigType m_ctrl; //!<current value of control signal
};


}  // namespace ns3

#endif /* PID_CONTROLLER_WITH_THRESHOLDS_H */
