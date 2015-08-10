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
#ifndef AGGREGATION_CONTROLLERS_H
#define AGGREGATION_CONTROLLERS_H

#include <math.h>
#include <algorithm>
#include <map>
#include "mpdu-aggregator.h"
#include "mpdu-universal-aggregator.h"
#include "ns3/enum.h"
#include "wifi-mac-queue.h"
#include "ns3/per-sta-q-info-container.h"
#include "ns3/mac48-address.h"
#include "pid-controller.h"
#include "pid-controller-with-thresholds.h"

namespace ns3 {

class PerStaWifiMacQueue;
class MpduUniversalAggregator;
/**
 * \ingroup wifi
 * Aggregation Controllers
 *
 */


typedef enum
{
  NO_CONTROL,
  PID,
  PID_WITH_THRESHOLDS
} ControllerType;

/*
 *   ____________________________________________________________________
 *   ________________________ Design Approach ___________________________
 *  |____________________________________________________________________|
 *
 */

class AggregationController : public Object
{
public:
  static TypeId GetTypeId (void);

  /**
   */

  virtual void Update (void) ;

  /*
   * Initializes PerStaWifiMacQueue, PerStaQInfoContainer and MpduUniversalAggregator
   * related to this aggregator. Also installs a controller
   * for each PerStaQInfo so the container has to be initialized
   * before calling this function.
   */
  void Initialize(Ptr<PerStaWifiMacQueue> queue, PerStaQInfoContainer &c, Ptr<MpduUniversalAggregator> agg);

protected:

  /*
   * subclass specific initialization procedure
   * to be re-implemented by each subclass
   */
  virtual void DoInitialize (void);
  Ptr<MpduUniversalAggregator> m_aggregator; //!< Pointer to MpduUniversalAggregator
  Ptr<PerStaWifiMacQueue> m_queue; //!< Pointer to PerStaWifiMacQueue
  PerStaQInfoContainer *m_perStaQInfo; //!< Pointer to PerStaQInfoContainer
};

typedef struct{
    double kp;
    double ki;
    double kd;
}PidParametersType;

class TimeAllowanceAggregationController : public AggregationController
{
public:
  static TypeId GetTypeId (void);
  TimeAllowanceAggregationController();
  ~TimeAllowanceAggregationController ();

  /**
   */

  virtual void Update (void);

private:

  void DoInitialize (void);
  /*
   * should be redefined by each subclass to reflect its own specific initialization steps
   */

  void NoControlUpdate (void);
  void DoInitializeNoControl (void);

  void PidControlUpdate (void);
  void DoInitializePidControl (void);

  void PidControlWithThresholdsUpdate (void);
  void DoInitializePidControlWithThresholds (void);

  /*sva-design should be added for every new type of controller ?
  void ?Update (void);
  void DoInitialize? (void);
  sva-design*/

  //service parameters
  double m_targetDvp; //!< Target delay violation probability
  double m_maxDelay; //!< maximum delay requirement in seconds
  double m_serviceInterval; //!< service interval in seconds

  //controller parameters
  typedef std::map<Mac48Address,Ptr<PidController> >::iterator PidIterator;
  std::map<Mac48Address,Ptr<PidController> > m_ctrl; //!< map relating MAC address to controller, assumes there is one PerStaQInfo per STA (TODO: change later)
  Time m_timeAllowance; //!< Fixed time allowance used for NO_CONTROL
  ControllerType m_type; //!< Type of controller, PID, etc.
  PidParametersType m_pidParams; //!< PID controller parameters
  //TODO redundant: is there a way to group these together in the attribute system?
  double m_weightIntegral;
  double m_kp;
  double m_ki;
  double m_kd;
  double m_thrW;
  double m_thrH;
  double m_thrL;

  Ptr<PerStaWifiMacQueue> m_queue; //!< Pointer to queue over which this aggregation controller is applied
};

}  // namespace ns3

#endif /* AGGREGATION_CONTROLLERS_H */
