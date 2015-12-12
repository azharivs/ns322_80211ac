/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015, Saleh Fakhrali
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
 * Author: Saleh Fakhrali <fakhrali@iust.ac.ir> and <s.fakhrali@gmail.com>
 */
#ifndef IDEAL_WIFI_MANAGER_FOR_MARKOV_CHANNEL_MODEL_11N_H
#define IDEAL_WIFI_MANAGER_FOR_MARKOV_CHANNEL_MODEL_11N_H

#include <stdint.h>
#include <vector>
#include "wifi-mode.h"
#include "wifi-remote-station-manager.h"
#include "ns3/mac48-address.h"
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include "ns3/random-variable-stream.h"
#include "ns3/uinteger.h"
#include "wifi-phy.h"
#include "ns3/nstime.h"
//If you enable this macro then  the states of the antennas will be calculated by means of steady state probability  not by markov model.
//#define sfmacro_SimulationWithSteadyStateProbability

namespace ns3 {

/**
 * \brief Ideal rate control algorithm
 * \ingroup wifi
 *
 */
struct IdealWifiRemoteStationForMarkovChannelModel11n : public WifiRemoteStation
{
  double     Rho,  // Expected value of the  received  signal  to  noise  ratio at the remote station
             **T,         //T[k][k]:  state transition  matrix  T for each station
              *p;         //p[k]:  steady  state  probability vector for each station
  uint32_t   NumberOfCallToDoGetDataTxVector,//becuase markov model is time consuming then we simulate the channel that at least MinPacketTransmissionToStartChannelSimulationForRemoteStation packet was send to it
  	  	     NumberOfantennas,
			 *CurrentStateOfEachAntenna,
			 **StateFrequency;
			 //CounterForTrackingAntennasState;

  bool IsInitializationAntennas;
};
class IdealWifiManagerForMarkovChannelModel11n : public WifiRemoteStationManager
{
public:
  static TypeId GetTypeId (void);
  IdealWifiManagerForMarkovChannelModel11n ();
  virtual ~IdealWifiManagerForMarkovChannelModel11n ();

private:
  // overriden from base class
  virtual WifiRemoteStation* DoCreateStation (void) const;
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
  virtual WifiTxVector DoGetDataTxVector (WifiRemoteStation *station, uint32_t size);
  virtual WifiTxVector DoGetRtsTxVector (WifiRemoteStation *station);
  virtual bool IsLowLatency (void) const;
  // It returns MCS index from 0 to 31
  uint32_t SpatialMultiplexing (IdealWifiRemoteStationForMarkovChannelModel11n *station);
  /* calculate when The state of each antennas changes and then schedule itself to change the state of them.
  */
  void ChangeTheStateOfAntennas  (IdealWifiRemoteStationForMarkovChannelModel11n *station, uint32_t AntennaNumber ,uint32_t NextState,bool IsInitialization) const;
  #ifdef sfmacro_SimulationWithSteadyStateProbability
  void CalculateTheStatesOfEachAntenna (IdealWifiRemoteStationForMarkovChannelModel11n *station);
  #endif
  // If you want to Track all antennas State then you can use the following function.
   //void TrackAntennasState (IdealWifiRemoteStationForMarkovChannelModel11n *station);

  /* we want to present finite-state markov model from paper "Finite-State Markov  Channel-A     Useful
     Model for Radio Communication Channels", and then simulate channel by means of method that was presented in [Fast simulation of diversity Nakagami fading channels using finite-state Markov models]  */
  /* User must enter these values : k, Rho, fm, R,and A.
     then we calculate p and T. */
  std::map<Mac48Address,uint32_t> ConvertMacToStationNumber;// we need this for accessing T and P of the station
  uint32_t k,NumberOfStations, NumberOfantennas,InitializeState,MinPacketTransmissionToStartChannelSimulationForRemoteStation,
           SymbolDuration ;// it is equall to (1/R)
  double   pi,
           Rho,          //expected value of the  received  signal  to  noise  ratio
           fm,            //Maximum doppler frequency fm= nu/lambda where  nu is the  speed  of  the  vehicle  and  lambda  is the  wavelength
		   R,      	   //transmission rate of symbols  per  second
		   *A,          //A[k]:  thresholds  of  the  received  signals:  User must enter this
		   *MCS_Datarates;  //MCS for 20 MHz,table 3-3 802.11 n: A survival guide.
                           //IMPORTANT: Note that in this program MCS_Datarates[0] refers to no transmission so for example MCS_Datarates[5] refers to MCS4
		  //*Transmission_Rate_based_on_Dof; //Dof 4 is presented with  Transmission_Rate_based_on_Dof[3] because arrayes in c++ begin with index 0

};

} // namespace ns3

#endif /* IDEAL_WIFI_MANAGER_FOR_MARKOV_CHANNEL_MODEL_11N_H */
