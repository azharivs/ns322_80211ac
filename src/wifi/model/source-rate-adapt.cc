/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) Sep 2, 2015 IUST
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
 * Iran University of Science & Technology
 */

#include <math.h>
#include "source-rate-adapt.h"

namespace ns3{

  NS_OBJECT_ENSURE_REGISTERED (SourceRateAdapt);

  TypeId
  SourceRateAdapt::GetTypeId (void)
  {
    static TypeId tid = TypeId ("ns3::SourceRateAdapt")
        .SetParent<Object> ()
//        .AddConstructor<SourceRateAdapt> ()
/*        .AddAttribute ("HistorySize", "Number of Samples Kept for Calculating Statistics.",
                       UintegerValue (25),
                       MakeUintegerAccessor (&PerStaQInfo::m_histSize),
                       MakeUintegerChecker<uint32_t> ())
        .AddAttribute ("ObservationInterval", "Length of Interval in Seconds Over Which Some Parameters Such as Arrival Rate Deficit/Excess Are Monitored.",
                       DoubleValue (2.0),
                       MakeDoubleAccessor (&PerStaQInfo::m_observationInterval),
                       MakeDoubleChecker<double> ())
        .AddAttribute ("TID", "Traffic Indication Map of Interest.",
                       UintegerValue (UP_VI),
                       MakeUintegerAccessor (&PerStaQInfo::m_tid),
                       MakeUintegerChecker<uint32_t> ())
*/
        ;
    return tid;
  }

  SourceRateAdapt::SourceRateAdapt()
  {
  }

  SourceRateAdapt::~SourceRateAdapt()
  {
  }

  bool
  SourceRateAdapt::SetApplication (Ptr<Application> app)
  {
    if (app)
      {
        m_app = app;
        return true;
      }
    return false;
  }

  Ptr<Application>
  SourceRateAdapt::GetApplication (void)
  {
    return m_app;
  }

  bool
  SourceRateAdapt::SetStaQ (Ptr<PerStaQInfo> sta)
  {
    if (sta)
      {
        m_staQ = sta;
        return true;
      }
    return false;
  }

  Ptr<PerStaQInfo>
  SourceRateAdapt::GetStaQ (void)
  {
    return m_staQ;
  }

  void
  SourceRateAdapt::SetInterval (double interval)
  {
    m_interval = interval;
  }

  void
  SourceRateAdapt::SetSourceRate (double pps)
  {
    DoSetSourceRate(pps); //virtual function to be re-implemented for each type of client
  }

  double
  SourceRateAdapt::GetSourceRate (void)
  {
    return 0; //virtual function to be re-implemented for each type of client
  }

  void
  SourceRateAdapt::UpdateSourceRate (void)
  {
    double surplus = m_staQ->GetArrivalRateSurplus();
    double curArrivalRate = m_staQ->GetAvgArrivalRate();
    SetSourceRate(std::max(50.0,curArrivalRate-surplus)); //never go below 10 pps!!
    Simulator::Schedule(Seconds(m_interval), &SourceRateAdapt::UpdateSourceRate, this);
  }

  void
  SourceRateAdapt::DoInit (void)
  {
    DoDoInit(); //re-implemented by subclass
  }

  NS_OBJECT_ENSURE_REGISTERED (CbrRateAdapt);

  TypeId
  CbrRateAdapt::GetTypeId (void)
  {
    static TypeId tid = TypeId ("ns3::CbrRateAdapt")
        .SetParent<SourceRateAdapt> ()
        .AddConstructor<CbrRateAdapt> ()
/*        .AddAttribute ("HistorySize", "Number of Samples Kept for Calculating Statistics.",
                       UintegerValue (25),
                       MakeUintegerAccessor (&PerStaQInfo::m_histSize),
                       MakeUintegerChecker<uint32_t> ())
        .AddAttribute ("ObservationInterval", "Length of Interval in Seconds Over Which Some Parameters Such as Arrival Rate Deficit/Excess Are Monitored.",
                       DoubleValue (2.0),
                       MakeDoubleAccessor (&PerStaQInfo::m_observationInterval),
                       MakeDoubleChecker<double> ())
        .AddAttribute ("TID", "Traffic Indication Map of Interest.",
                       UintegerValue (UP_VI),
                       MakeUintegerAccessor (&PerStaQInfo::m_tid),
                       MakeUintegerChecker<uint32_t> ())
*/
        ;
    return tid;
  }

  CbrRateAdapt::CbrRateAdapt()
  {
  }

  CbrRateAdapt::~CbrRateAdapt()
  {
  }

  void
  CbrRateAdapt::DoDoInit (void)
  {//TODO could be redundant
    TimeValue pktGenInterval;
    m_app->GetObject<UdpClient>()->GetAttribute("Interval",pktGenInterval);
  }

  void
  CbrRateAdapt::DoSetSourceRate (double pps)
  {//TODO could be redundant
    if (Simulator::Now() < Seconds(10.0)) //allow for warm up
      return;
    Time pktGenInterval = Seconds(1.0/pps);
    m_app->GetObject<UdpClient>()->SetAttribute("Interval", TimeValue (pktGenInterval));
  }
} //namespace ns3

