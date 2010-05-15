/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/* 
 * Copyright (c) 2009 Duy Nguyen 
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
 * Author: Duy Nguyen <duy@soe.ucsc.edu> 
 */



#ifndef MINSTREL_WIFI_MANAGER_H
#define MINSTREL_WIFI_MANAGER_H

#include "wifi-remote-station-manager.h"
#include "wifi-mode.h"
#include "ns3/nstime.h"
#include <vector>

/**
 * \author Duy Nguyen
 * \brief Implementation of Minstrel Rate Control Algorithm 
 *
 * Porting Minstrel from Madwifi and Linux Kernel 
 * http://linuxwireless.org/en/developers/Documentation/mac80211/RateControl/minstrel
 */


namespace ns3 {

class MinstrelWifiRemoteStation;


class MinstrelWifiManager : public WifiRemoteStationManager
{

public:
  static TypeId GetTypeId (void);
  MinstrelWifiManager ();
  virtual ~MinstrelWifiManager();

  virtual void SetupPhy (Ptr<WifiPhy> phy);

private:
  // overriden from base class
  virtual class WifiRemoteStation *DoCreateStation (void) const;
  virtual void DoReportRxOk (WifiRemoteStation *station, 
                             double rxSnr, WifiMode txMode);
  virtual void DoReportRtsFailed (WifiRemoteStation *station);
  virtual void DoReportDataFailed (WifiRemoteStation *station);
  virtual void DoReportRtsOk (WifiRemoteStation *station,
                              double ctsSnr, WifiMode ctsMode, double rtsSnr);
  virtual void DoReportDataOk (WifiRemoteStation *station,
                               double ackSnr, WifiMode ackMode, double dataSnr);
  virtual void DoReportFinalRtsFailed (WifiRemoteStation *station);
  virtual void DoReportFinalDataFailed (WifiRemoteStation *station);
  virtual WifiMode DoGetDataMode (WifiRemoteStation *station, uint32_t size);
  virtual WifiMode DoGetRtsMode (WifiRemoteStation *station);
  virtual bool IsLowLatency (void) const;

  /// for estimating the TxTime of a packet with a given mode  
  Time GetCalcTxTime (WifiMode mode) const;
  void AddCalcTxTime (WifiMode mode, Time t);

  /// update the number of retries and reset accordingly
  void UpdateRetry (MinstrelWifiRemoteStation *station);

  /// getting the next sample from Sample Table
  uint32_t GetNextSample (MinstrelWifiRemoteStation *station);

  /// find a rate to use from Minstrel Table
  uint32_t FindRate (MinstrelWifiRemoteStation *station);

  /// updating the Minstrel Table every 1/10 seconds
  void UpdateStats (MinstrelWifiRemoteStation *station);

  /// initialize Minstrel Table
  void RateInit (MinstrelWifiRemoteStation *station);

  /// initialize Sample Table
  void InitSampleTable (MinstrelWifiRemoteStation *station);

  /// printing Sample Table
  void PrintSampleTable (MinstrelWifiRemoteStation *station);

  /// printing Minstrel Table
  void PrintTable (MinstrelWifiRemoteStation *station);

  void CheckInit (MinstrelWifiRemoteStation *station);  ///< check for initializations


  typedef std::vector<std::pair<Time,WifiMode> > TxTime;

  TxTime m_calcTxTime;  ///< to hold all the calculated TxTime for all modes
  Time m_updateStats;  ///< how frequent do we calculate the stats(1/10 seconds)
  double m_lookAroundRate;  ///< the % to try other rates than our current rate 
  double m_ewmaLevel;  ///< exponential weighted moving average
  uint32_t m_segmentSize;  ///< largest allowable segment size
  uint32_t m_sampleCol;  ///< number of sample columns
  uint32_t m_pktLen;  ///< packet length used  for calculate mode TxTime  
};

}// namespace ns3

#endif /* MINSTREL_WIFI_MANAGER_H */
