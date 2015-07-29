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

#include "mpdu-aggregator.h"
#include "mpdu-universal-aggregator.h"
#include "ns3/enum.h"
#include "wifi-mac-queue.h"
#include "ns3/per-sta-q-info-container.h"


namespace ns3 {

class PerStaWifiMacQueue;
/**
 * \ingroup wifi
 * Aggregation Controllers
 *
 */


typedef enum
{
  NO_CONTROL,
  PID
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

private:

  //Ptr<MpduUniversalAggregator> m_aggregator; //!< Pointer to MpduUniversalAggregator
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
  //service parameters
  double m_targetDvp; //!< Target delay violation probability
  double m_maxDelay; //!< maximum delay requirement in seconds
  double m_serviceInterval; //!< service interval in seconds

  //controller parameters
  ControllerType m_type; //!< Type of controller, PID, etc.
  PidParametersType m_pidParams; //!< PID controller parameters

  Ptr<PerStaWifiMacQueue> m_queue; //!< Pointer to queue over which this aggregation controller is applied
};

}  // namespace ns3

#endif /* AGGREGATION_CONTROLLERS_H */
