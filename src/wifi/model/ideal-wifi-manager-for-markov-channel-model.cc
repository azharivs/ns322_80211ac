/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2006 INRIA
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "ideal-wifi-manager-for-markov-channel-model.h"
#include "wifi-phy.h"
#include "ns3/assert.h"
#include "ns3/double.h"
#include <cmath>
#include "ns3/wifi-mac.h"

#define Min(a,b) ((a < b) ? a : b)

namespace ns3 {

/**
 * \brief hold per-remote-station state for Ideal Wifi manager.
 *
 * This struct extends from WifiRemoteStation struct to hold additional
 * information required by the Ideal Wifi manager
 */
//sf for markov model, i do not need m_lastSnr. maybe in the future I use it.
struct IdealWifiRemoteStationForMarkovChannelModel : public WifiRemoteStation
{
  double m_lastSnr;  //!< SNR of last packet sent to the remote station
};

NS_OBJECT_ENSURE_REGISTERED (IdealWifiManagerForMarkovChannelModel);

TypeId
IdealWifiManagerForMarkovChannelModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::IdealWifiManagerForMarkovChannelModel")
    .SetParent<WifiRemoteStationManager> ()
    .AddConstructor<IdealWifiManagerForMarkovChannelModel> ()

  ;
  return tid;
}

IdealWifiManagerForMarkovChannelModel::IdealWifiManagerForMarkovChannelModel ()
{
  // This is for maximum 10 AP and Stations.
  ConvertMacToStationNumber.insert(std::pair<Mac48Address,int>(Mac48Address("00:00:00:00:00:01"),0)); // we start naming station with 0 because array index begins with 0
  ConvertMacToStationNumber.insert(std::pair<Mac48Address,int>(Mac48Address("00:00:00:00:00:02"),1));
  ConvertMacToStationNumber.insert(std::pair<Mac48Address,int>(Mac48Address("00:00:00:00:00:03"),2));
  ConvertMacToStationNumber.insert(std::pair<Mac48Address,int>(Mac48Address("00:00:00:00:00:04"),3));
  ConvertMacToStationNumber.insert(std::pair<Mac48Address,int>(Mac48Address("00:00:00:00:00:05"),4));
  ConvertMacToStationNumber.insert(std::pair<Mac48Address,int>(Mac48Address("00:00:00:00:00:06"),5));
  ConvertMacToStationNumber.insert(std::pair<Mac48Address,int>(Mac48Address("00:00:00:00:00:07"),6));
  ConvertMacToStationNumber.insert(std::pair<Mac48Address,int>(Mac48Address("00:00:00:00:00:08"),7));
  ConvertMacToStationNumber.insert(std::pair<Mac48Address,int>(Mac48Address("00:00:00:00:00:09"),8));
  ConvertMacToStationNumber.insert(std::pair<Mac48Address,int>(Mac48Address("00:00:00:00:00:0A"),9));
  pi=3.1415926;
  //variables for reading and writing
  char comment[255];
  std::ifstream fin("/home/azhari/app/ns-3-allinone/ns-3.22/scratch/input.txt");
  std::ofstream fout("/home/azhari/app/ns-3-allinone/ns-3.22/scratch/output.txt");

  //*********     Reading from input file   *****************
  fin.getline(comment,255);
  fin>>k;
  fin>>NumberOfStations;
  fin>>NumberOfantennas;

  //Now we can make arrays
  int NumberOfAllSourceDestinationPairs=(NumberOfStations * NumberOfStations);
  CurrentStateOfEachAntennaInEcachStation = new int *[NumberOfStations];
  for (int i=0; i<NumberOfStations; i++)
  	CurrentStateOfEachAntennaInEcachStation[i]=new int [ NumberOfantennas];
  for (int i=0; i<NumberOfStations; i++)
    for (int j=0; j<NumberOfantennas; j++)
  	 CurrentStateOfEachAntennaInEcachStation[i][j]=0;
  p= new double *[NumberOfAllSourceDestinationPairs];
  for (int i=0; i<NumberOfAllSourceDestinationPairs; i++)
  p[i]=new double [k];
  N= new double *[NumberOfAllSourceDestinationPairs];
  for (int i=0; i<NumberOfAllSourceDestinationPairs; i++)
    N[i]=new double [k];
  Rt= new double *[NumberOfAllSourceDestinationPairs];
  for (int i=0; i<NumberOfAllSourceDestinationPairs; i++)
    Rt[i]=new double [k];
  A =new double  [k];
  A[0]=0;
  MCS_Datarates = new double  [k];
  MCS_Datarates[0]= 0;
  T= new double **[NumberOfAllSourceDestinationPairs];
  for (int i=0; i<NumberOfAllSourceDestinationPairs; i++)
    T[i]=new double *[k];
  for (int i=0; i<NumberOfAllSourceDestinationPairs; i++)
    for( int j=0; j<k;j++)
      T[i][j]= new double [k];
  Rho =new double  [NumberOfAllSourceDestinationPairs];
  for ( int i =0; i<= (NumberOfAllSourceDestinationPairs-1); i++)
  	fin>>Rho[i];
  fin>>fm;
  fin>>R;
  for ( int i =1; i<= (k-1); i++)
  	fin>>A[i];
  for ( int i =1; i<= (k-1); i++)
  	fin>>MCS_Datarates[i];
  //**************  End of reading   *********************

  // *******************     computation    **********************************
  // equation 34 : calculating p
  for (int i =0; i<NumberOfAllSourceDestinationPairs; i++){
    for ( int j =0; j<= (k-1); j++) {
  	if (j!=k-1)
  	  p[i][j] = exp( -A[j] / Rho[i] )- exp( -A[j+1] / Rho[i] );
  	else
        p[i][j] = exp( -A[j] / Rho[i] ); //
    }
  }

  //calculating Rt(k)
  for (int i =0; i<NumberOfAllSourceDestinationPairs; i++)
    for ( int j =0; j<= (k-1); j++)
      Rt[i][j] = R * p [i][j];

  //calculating N from equation 33
  for (int i =0; i<NumberOfAllSourceDestinationPairs; i++)
    for ( int j =0; j<= (k-1); j++)
  	N[i][j] = sqrt((2*pi*A[j])/Rho[i]) * fm * exp( -A[j]/Rho[i]);


  //initialization of T
  for ( int i=0; i < NumberOfAllSourceDestinationPairs; i++)
    for ( int j=0; j < k; j++)
      for (int l=0; l < k; l++)
  		T[i][j][l]=0;

  // calculating T from equation 39, 40
  for ( int i=0; i < NumberOfAllSourceDestinationPairs; i++)
    for ( int j =0; j<= (k-2); j++)
  	T[i][j][j+1] = N[i][j+1] / Rt[i][j];
  for ( int i=0; i < NumberOfAllSourceDestinationPairs; i++)
    for ( int j =1; j<= (k-1); j++)
  	T[i][j][j-1] = N[i][j] / Rt[i][j];
  for ( int i=0; i < NumberOfAllSourceDestinationPairs; i++){
    T[i][0][0] = 1- T[i][0][1];
    T[i][k-1][k-1] = 1- T[i][k-1][k-2];
  }
  for ( int i=0; i < NumberOfAllSourceDestinationPairs; i++)
    for ( int j =1; j<= (k-2); j++)
  	T[i][j][j] = 1 - T[i][j][j-1]- T[i][j][j+1] ;

  // ***********    End of computation      *************************************



  // ********* Writing in the output.txt **************
  fout<< "*******     State transition  matrix  T     *******"<<std::endl;
  for ( int i=0; i < NumberOfAllSourceDestinationPairs; i++){
    fout<< " T for (Source, Destination) Pair " << (i +1);
    fout<<std::endl;
    for ( int j=0; j < k; j++)
      for (int l=0; l < k; l++){
  		fout <<"T ["<<j<<"] ["<<l<<"] = " << T[i][j][l] <<std::endl;

  		if (l==k-1)
  			fout<<std::endl;
  	}
  }
  fout<<"*******     Steady  state  probability vector p     *******"<<std::endl;
  for ( int i=0; i < NumberOfAllSourceDestinationPairs; i++){
    fout<< " p for (Source, Destination) Pair " << (i +1) << std::endl;
    for ( int j=0; j < k; j++)
  	fout<<p[i][j]<<std::endl;
  }

  //***************   Verification: 42 in page 8 and 2 in page 2  **************
  fout<<std::endl<<"***********************************************************"
      <<std::endl<<"To verify  this  finite-state  Markov channel model," <<std::endl
  	<<"we use  state  equilibrium  equations  for  each  state (42 in page 8)"<<std::endl
  	<<"and equation 2 in page 2"<<std::endl
  	<<"Note that because of floating point operation, the two side of equations may not be exactly equal." <<std::endl
  	<<"***********************************************************"<<std::endl;
  double sum=0;
  for ( int i=0; i <NumberOfAllSourceDestinationPairs; i++){
    sum=0;
    for ( int j=0; j < k; j++){
  	sum+=p[i][j];
  	if(j==k-1)
  	  fout<< "The sum of p[i] for (Source, Destination) Pair " << (i +1) << " is "<< sum <<std::endl<<std::endl;
    }
  }
  for ( int i=0; i < NumberOfAllSourceDestinationPairs; i++){
    fout<<"(Source, Destination) Pair " << (i+1) << " we have" ;
    fout<<std::endl;
    for ( int j=1; j <= (k-2); j++){
  	fout << "p [" << j  <<"] * (T ["<< j <<"]["<<j-1<<"] + T ["<<j<<"]["       <<j+1<<"]) must be equal to "
  	     << "p [" << j-1<<"] * T [" <<j-1<<"]["<<j  <<"] + p ["  <<j+1<<"] * T ["<<j+1<<"]["<<j<<"]"<<std::endl;
  	fout << p[i][j]*(T[i][j][j-1] + T[i][j][j+1])<<" must be equal to "
  	     << p[i][j-1]*T[i][j-1][j]+p[i][j+1]*T[i][j+1][j]<<std::endl<<std::endl;
    }
  }
  //********* End of Writing in the output.txt **********************************





}
IdealWifiManagerForMarkovChannelModel::~IdealWifiManagerForMarkovChannelModel ()
{
}

WifiRemoteStation *
IdealWifiManagerForMarkovChannelModel::DoCreateStation (void) const
{
  IdealWifiRemoteStationForMarkovChannelModel *station = new IdealWifiRemoteStationForMarkovChannelModel ();
  station->m_lastSnr = 0.0;
  return station;
}


void
IdealWifiManagerForMarkovChannelModel::DoReportRxOk (WifiRemoteStation *station,
                                double rxSnr, WifiMode txMode)
{
}
void
IdealWifiManagerForMarkovChannelModel::DoReportRtsFailed (WifiRemoteStation *station)
{
}
void
IdealWifiManagerForMarkovChannelModel::DoReportDataFailed (WifiRemoteStation *station)
{
}
void
IdealWifiManagerForMarkovChannelModel::DoReportRtsOk (WifiRemoteStation *st,
                                 double ctsSnr, WifiMode ctsMode, double rtsSnr)
{
  IdealWifiRemoteStationForMarkovChannelModel *station = (IdealWifiRemoteStationForMarkovChannelModel *)st;
  station->m_lastSnr = rtsSnr;
}
void
IdealWifiManagerForMarkovChannelModel::DoReportDataOk (WifiRemoteStation *st,
                                  double ackSnr, WifiMode ackMode, double dataSnr)
{
  IdealWifiRemoteStationForMarkovChannelModel *station = (IdealWifiRemoteStationForMarkovChannelModel *)st;
  station->m_lastSnr = dataSnr;
}
void
IdealWifiManagerForMarkovChannelModel::DoReportFinalRtsFailed (WifiRemoteStation *station)
{
}
void
IdealWifiManagerForMarkovChannelModel::DoReportFinalDataFailed (WifiRemoteStation *station)
{
}

WifiTxVector
IdealWifiManagerForMarkovChannelModel::DoGetDataTxVector (WifiRemoteStation *st, uint32_t size)
{
  IdealWifiRemoteStationForMarkovChannelModel *station = (IdealWifiRemoteStationForMarkovChannelModel *)st;

  int DestinationStationIndex=ConvertMacToStationNumber.find(station->m_state->m_address)->second;
  CalculateTheStatesOfEachAntenna (DestinationStationIndex);

  // because  m_deviceRateSet have OfdmRate6Mbps,OfdmRate12Mbps, and OfdmRate24Mbps by default, we must add mcs index with 3.
  uint32_t i = SpatialMultiplexing(DestinationStationIndex) + 3;
  //sf I added below "if" because at first mcs was not added to Supported rates
  if (i>=(GetNSupported (station)))
	  i= 0;
  WifiMode maxMode =  GetSupported (station, i);
  return WifiTxVector (maxMode, GetDefaultTxPowerLevel (), GetLongRetryCount (station), GetShortGuardInterval (station), Min (GetNumberOfReceiveAntennas (station),GetNumberOfTransmitAntennas()), GetNess (station), GetStbc (station));
}
WifiTxVector
IdealWifiManagerForMarkovChannelModel::DoGetRtsTxVector (WifiRemoteStation *st)
{
  IdealWifiRemoteStationForMarkovChannelModel *station = (IdealWifiRemoteStationForMarkovChannelModel *)st;

  int DestinationStationIndex=ConvertMacToStationNumber.find(station->m_state->m_address)->second;
  CalculateTheStatesOfEachAntenna (DestinationStationIndex);

  // because  m_deviceRateSet have OfdmRate6Mbps,OfdmRate12Mbps, and OfdmRate24Mbps by default, we must add mcs index with 3.
  uint32_t i = SpatialMultiplexing(DestinationStationIndex) + 3;
  //sf I added below "if" because at first mcs was not added to Supported rates
  if (i>=(GetNSupported (station)))
	  i= 0;
  WifiMode maxMode =  GetSupported (station, i);
  return WifiTxVector (maxMode, GetDefaultTxPowerLevel (), GetShortRetryCount (station), GetShortGuardInterval (station), Min (GetNumberOfReceiveAntennas (station),GetNumberOfTransmitAntennas()), GetNess (station), GetStbc (station));
}

bool
IdealWifiManagerForMarkovChannelModel::IsLowLatency (void) const
{
  return true;
}

void IdealWifiManagerForMarkovChannelModel::CalculateTheStatesOfEachAntenna (int DestinationStationNumber)
{
  Ptr<WifiMac> WifiMacOfThisWifiRemoteStationManager = GetMac() ;
  Mac48Address MacAddressOfThisWifiRemoteStationManager= WifiMacOfThisWifiRemoteStationManager->GetAddress();
  int SourceStationNumber = ConvertMacToStationNumber.find(MacAddressOfThisWifiRemoteStationManager)->second;
  for (int i=0; i<NumberOfantennas; i++){
	double random_probability = 0, mm=10000,
		   SumOfP=0; //we use this to determine in which states the antenna is
		   random_probability= ( rand() % 10000)/mm;
	for ( int j =0; j < k; j++) {
	  if (j!=k-1){
	    if( (SumOfP <= random_probability) && (random_probability < (SumOfP + p[(DestinationStationNumber) + (NumberOfStations*SourceStationNumber)][j])) ){
	      CurrentStateOfEachAntennaInEcachStation[DestinationStationNumber][i]= j;
	      //sf debug
	      //std::cout<< " j is " << j <<std::endl;
	    }
	  } else{
		if( (SumOfP <= random_probability) && (random_probability < 1) ) {
		  CurrentStateOfEachAntennaInEcachStation[DestinationStationNumber][i]= j;
		  //sf debug
		  //std::cout<< " j is " << j <<std::endl;
		}
	  }
      SumOfP += p[(DestinationStationNumber) + (NumberOfStations*SourceStationNumber)][j];
	}
  }
}

uint32_t IdealWifiManagerForMarkovChannelModel::SpatialMultiplexing (int DestinationStationNumber)
{
  // If all of antenna are in the state 0 that we send with MCS0
  //check if all of them are in state 0?
  bool IsAllAntennasInState0 = true;
  for (int i=0; i<NumberOfantennas; i++) {
	if( ( CurrentStateOfEachAntennaInEcachStation[DestinationStationNumber][i]) != 0 )
		IsAllAntennasInState0 =	false;
  }
  // when all Antenna is in state 0 then we send with dof1 with lowest rate that means MCS0
  if (IsAllAntennasInState0)
	  return 0;
  //first we copy CurrentStateOfEachAntennaInEcachStation[DestinationStationNumber][i] to CopyOfCurrentStateOfEachAntenna[i]
  int *CopyOfCurrentStateOfEachAntenna, *McsIndexBasedOnDof;
  double *TransmissionRateBasedOnDof;

  CopyOfCurrentStateOfEachAntenna=new int [ NumberOfantennas];
  TransmissionRateBasedOnDof=new double [ NumberOfantennas]; //Dof 4 is presented with  Transmission_Rate_based_on_Dof[3]
  McsIndexBasedOnDof = new int [ NumberOfantennas];

  for (int i=0; i<NumberOfantennas; i++) {
	  CopyOfCurrentStateOfEachAntenna[i]= CurrentStateOfEachAntennaInEcachStation[DestinationStationNumber][i];
  }

  //sort CopyOfCurrentStateOfEachAntenna by means of bubble sort
  // therefore the worst SNR is in CopyOfCurrentStateOfEachAntenna[0]
  int temp;
  for ( int pass=0; pass < ( NumberOfantennas-1); pass++){
	for ( int i = 0; i<( NumberOfantennas-1-pass); i ++)
	  if ( CopyOfCurrentStateOfEachAntenna[i] > CopyOfCurrentStateOfEachAntenna[i+1] ){
		temp =  CopyOfCurrentStateOfEachAntenna[i];
		CopyOfCurrentStateOfEachAntenna[i] = CopyOfCurrentStateOfEachAntenna[i+1];
		CopyOfCurrentStateOfEachAntenna[i+1] = temp;
      }
  }

  TransmissionRateBasedOnDof[( NumberOfantennas-1)] = MCS_Datarates[CopyOfCurrentStateOfEachAntenna[0]] ; // with Dof 4 non of antenna is deleted so with minimum rate we should send.
  McsIndexBasedOnDof[( NumberOfantennas-1)] = CopyOfCurrentStateOfEachAntenna[0];


  //because 802.11 n uses the worst SNR, we just need to recalculate CopyOfCurrentStateOfEachAntenna[i]
  // we start with ignoring the worst SNR i.e CopyOfCurrentStateOfEachAntenna[i-1] , SNR in each chain will increase
  // because 802.11 n uses the worst SNR between antennas, we just need to recalculate CopyOfCurrentStateOfEachAntenna[i]

  for (int i=1; i< NumberOfantennas; i ++) {
    int counter = k-1-CopyOfCurrentStateOfEachAntenna[i];
    TransmissionRateBasedOnDof[(NumberOfantennas-(i+1))] = MCS_Datarates[CopyOfCurrentStateOfEachAntenna[i]] ;
    McsIndexBasedOnDof[(NumberOfantennas-(i+1))] = CopyOfCurrentStateOfEachAntenna[i] ;
    while(counter!=0){
	  if (( (NumberOfantennas) * A[CopyOfCurrentStateOfEachAntenna[i]] / (NumberOfantennas-i) ) >= (A [CopyOfCurrentStateOfEachAntenna[i] + counter]) ){
	    TransmissionRateBasedOnDof[(NumberOfantennas-(i+1))] = MCS_Datarates[CopyOfCurrentStateOfEachAntenna[i] +counter] ;
	    McsIndexBasedOnDof[(NumberOfantennas-(i+1))] = CopyOfCurrentStateOfEachAntenna[i] +counter ;
	    break;
	  }
	  counter--;
    }
  }

  double rate = TransmissionRateBasedOnDof[(NumberOfantennas-1)] ;
  int Dof=NumberOfantennas;
  // McsIndex is between 0 to (k-2) ,
  int McsIndex = McsIndexBasedOnDof[(NumberOfantennas-1)] -1  ;

  // calculate the maximum rate that can be obtained
  for (int i=1; i< NumberOfantennas; i ++) {
    if(( (NumberOfantennas-i)*TransmissionRateBasedOnDof[(NumberOfantennas-(i+1))] ) > (Dof*rate)){
	  Dof=(NumberOfantennas-i);
	  rate = TransmissionRateBasedOnDof[(NumberOfantennas-(i+1))];
	  McsIndex = McsIndexBasedOnDof[(NumberOfantennas-(i+1))] -1;
    }
  }
  //sf debug
  //if ( (McsIndex<0) || (McsIndex>k-2) )
	// std::cout<< "******There is an error *******";

 // I merge Dof and Mcs(0 to k-2) to obtain Mcs with spatial multiplexing
 // for example mcs2 with Dof 3 means Mcs18.
 uint32_t McsIndexWithDof= McsIndex + ( (Dof-1)* 8);

 //sf debug
 //std::cout<<"mcs index is" << McsIndexWithDof <<std::endl;

 return McsIndexWithDof;
}
} // namespace ns3
