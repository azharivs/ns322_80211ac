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
#ifndef SOURCE_RATE_ADAPT_H_
#define SOURCE_RATE_ADAPT_H_

/*
 * This will be aggregated to a particular type of client application
 * for which it does rate adaptation. The rate adaptation function will be
 * initialized as a suitable callback. I'm not sure if this is possible!
 * There will be an independent instance of this class responsible for each client application.
 */
#include "ns3/application.h"
#include "ns3/per-sta-q-info-container.h"

namespace ns3 {

  class SourceRateAdapt : public Object
  {
    public:
      static TypeId GetTypeId (void);
      SourceRateAdapt ();
      ~SourceRateAdapt ();

      bool SetApplication (Ptr<Application> app);

      Ptr<Application> GetApplication (void);

      bool SetStaQ (Ptr<PerStaQInfo> sta);

      Ptr<PerStaQInfo> GetStaQ (void);

      void SetInterval(double interval);

      void SetSourceRate (double pps);

      double GetSourceRate (void);

      void UpdateSourceRate (void);

      void DoInit (void);

    protected:
      Ptr<Application> m_app; //!< Pointer to application for which this source rate adaptor is responsible
      Ptr<PerStaQInfo> m_staQ; //!< Pointer to PerStaQInfo responsible for this clients packets. It is assumed that at most one client is related to a PerStaQInfo (TODO make this general)
      double m_interval;

      //virtual function to be re-implemented for each type of client
      virtual void DoSetSourceRate (double pps) = 0;

      //virtual function to be re-implemented for each type of client
      virtual void DoDoInit (void) = 0;
  };

  class CbrRateAdapt : public SourceRateAdapt
  {
    public:
      static TypeId GetTypeId (void);
      CbrRateAdapt ();
      ~CbrRateAdapt();

    private:
      Time m_pktInterval; //!< packet generation interval

      void DoSetSourceRate (double pps);

      void DoDoInit (void);
  };
} //namespace ns3



#endif /* SOURCE_RATE_ADAPT_H_ */
