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
#include "ideal-wifi-manager-for-markov-channel-model.h"
#include "wifi-phy.h"
#include "ns3/assert.h"
#include "ns3/double.h"
#include <cmath>
#include "ns3/wifi-mac.h"
#include "ns3/simulator.h"

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
  IsInitializationAntennasState=true;
  InitializeState = 0;
  SymbolDuration = 4 ; // unit is microsecond

  //variables for reading and writing
  char comment[255];
  std::ifstream fin("InputParametersForMarkovModel.txt");
  std::ofstream fout("SomeOutputsForMarkovModel.txt");

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
  	 CurrentStateOfEachAntennaInEcachStation[i][j]=InitializeState ;
  CounterForTrackingAntennasState=0;
  StateFrequency = new uint64_t **[NumberOfStations] ;
  for (int i=0; i<NumberOfStations; i++)
	  StateFrequency[i]=new uint64_t *[NumberOfantennas];
  for (int i=0; i<NumberOfStations; i++)
    for( int j=0; j<NumberOfantennas;j++)
      StateFrequency[i][j]= new uint64_t [k];
  for (int i=0; i<NumberOfStations; i++)
    for( int j=0; j<NumberOfantennas;j++)
	  for( int l=0; l< k;l++)
		  StateFrequency[i][j][l]= 0;
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
  Ptr<WifiMac> WifiMacOfThisWifiRemoteStationManager = GetMac() ;
  Mac48Address MacAddressOfThisWifiRemoteStationManager= WifiMacOfThisWifiRemoteStationManager->GetAddress();
  int SourceStationIndex = ConvertMacToStationNumber.find(MacAddressOfThisWifiRemoteStationManager)->second;

  #ifdef sfmacro_SimulationWithSteadyStateProbability
  CalculateTheStatesOfEachAntenna (DestinationStationIndex,SourceStationIndex);
  #else
  // We want to initialize all antennas state  not the antennas in destination 0 that has index 0
  //When a Source want to send to a Destination then all markov chains in all destination will be simulated.
  if(IsInitializationAntennasState) {
	  //Debug
	  //std::cout<< "Source Number = " << SourceStationIndex<<" Destination Number = "<<DestinationStationIndex<<std::endl;
	  ChangeTheStateOfAntennas (0, SourceStationIndex, 0,InitializeState, true);
  	  IsInitializationAntennasState=false;
  }
  #endif

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

  #ifdef sfmacro_SimulationWithSteadyStateProbability
  Ptr<WifiMac> WifiMacOfThisWifiRemoteStationManager = GetMac() ;
  Mac48Address MacAddressOfThisWifiRemoteStationManager= WifiMacOfThisWifiRemoteStationManager->GetAddress();
  int SourceStationIndex = ConvertMacToStationNumber.find(MacAddressOfThisWifiRemoteStationManager)->second;
  CalculateTheStatesOfEachAntenna (DestinationStationIndex,SourceStationIndex);
  #endif

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

void IdealWifiManagerForMarkovChannelModel::ChangeTheStateOfAntennas ( int DestinationStationNumber, int SourceStationNumber, int AntennaNumber ,int NextState,bool IsInitialization)
{
  double RandomProbability,UniformRandomVariableBetween0and1,
         ProbabilityOfRemainingInThisState,
		 NumberOfClocksItTakesToExit; //It is geometric distributed random variable and take 0,1,2, 3, ...
                                     //  0 means that the antenna exist from current state at the next clock
                                     // we use the way that described in "Lewis, P. W. A., and Ed McKenzie. Simulation methodology for statisticians, operations analysts, and engineers. Vol. 1. CRC press, 1988" for generating geometrically distributed random variable.
  int  NextStateOfCurrentNextState;
  Ptr<UniformRandomVariable>  RandomVariable =   CreateObject<UniformRandomVariable> ();

  if(IsInitialization){
	  // If you want to Track all antennas State then you can remove the // in the next line
	  //TrackAntennasState(SourceStationNumber);

	 // Initialize all antennas in all destinations
     for (int i=0; i<NumberOfStations; i++){
 	  for (int j=0; j<NumberOfantennas; j++){
 		if(i!=SourceStationNumber){ //It is not necessary to simulate source chain!
 		  CurrentStateOfEachAntennaInEcachStation[i][j]= NextState;
 		  //calculate the next state that will be schedule
 		  RandomProbability = (RandomVariable->GetValue(0,1));
 		  ProbabilityOfRemainingInThisState = T [i+(NumberOfStations*SourceStationNumber)] [NextState][NextState];
 		  if (NextState ==0) {
 			  NextStateOfCurrentNextState = NextState+1;
 		  }else if (NextState ==(k-1)){
 		      NextStateOfCurrentNextState = NextState -1;
 	      }else{
 			  if(RandomProbability <= ((T [i+(NumberOfStations*SourceStationNumber)] [NextState][NextState -1])/(1-ProbabilityOfRemainingInThisState)))
 				  NextStateOfCurrentNextState = NextState -1;
 			  else
 				  NextStateOfCurrentNextState = NextState +1;
 		  }
 		  //Schedule the next transition
 		  UniformRandomVariableBetween0and1 = RandomVariable->GetValue(0,1);
 		  NumberOfClocksItTakesToExit = static_cast<uint64_t> (floor ((log(1 - UniformRandomVariableBetween0and1))/(log (ProbabilityOfRemainingInThisState))));
 		  //sf Debug
 		  //std::cout<<NextStateOfCurrentNextState << "   " << NumberOfClocksItTakesToExit<<std::endl;
 		  //We Add 1 with NumberOfClocksItTakesToExit because for example 0 means that at the Next Clock Transition must happens.
 		  Simulator::Schedule (MilliSeconds((NumberOfClocksItTakesToExit+1)*SymbolDuration),
 				               &IdealWifiManagerForMarkovChannelModel::ChangeTheStateOfAntennas, this,
 		                       i,
							   SourceStationNumber,
 		                       j,
							   NextStateOfCurrentNextState,
 		                       false);
       }
 	  }
 	}
   }else{
	 //Debug
	 //std::cout<< "After Initialization in ChangeTheStateOfAntennas :Source Number = " << SourceStationNumber<<" Destination Number = "<<DestinationStationNumber<<std::endl;
	 // It is not initialization, so we must change the desired antenna state
     CurrentStateOfEachAntennaInEcachStation[DestinationStationNumber][AntennaNumber]= NextState;
     //calculate the next state that will be schedule
	 RandomProbability = (RandomVariable->GetValue(0,1));
     ProbabilityOfRemainingInThisState = T [DestinationStationNumber+(NumberOfStations*SourceStationNumber)] [NextState][NextState];
	 if (NextState ==0) {
	 	NextStateOfCurrentNextState = NextState+1;
	 }else if (NextState ==(k-1)){
	 	NextStateOfCurrentNextState = NextState -1;
	 }else{
	 	if(RandomProbability <= ((T [DestinationStationNumber+(NumberOfStations*SourceStationNumber)] [NextState][NextState -1])/(1-ProbabilityOfRemainingInThisState)))
	 		NextStateOfCurrentNextState = NextState -1;
	 	else
	 		NextStateOfCurrentNextState = NextState +1;
	 }
	 //Schedule the next transition
	 UniformRandomVariableBetween0and1 = RandomVariable->GetValue(0,1);
	 NumberOfClocksItTakesToExit = static_cast<uint64_t> (floor ((log(1 - UniformRandomVariableBetween0and1))/(log (ProbabilityOfRemainingInThisState))));
	 //sf Debug
	 //std::cout<<NextStateOfCurrentNextState << "   " << NumberOfClocksItTakesToExit<<std::endl;
	 //We Add 1 with NumberOfClocksItTakesToExit because for example 0 means that at the Next Clock Transition must happens.
	 Simulator::Schedule (MilliSeconds((NumberOfClocksItTakesToExit+1)*SymbolDuration),
	 		              &IdealWifiManagerForMarkovChannelModel::ChangeTheStateOfAntennas, this,
						  DestinationStationNumber,
						  SourceStationNumber,
						  AntennaNumber,
	 					  NextStateOfCurrentNextState,
	                      false);
   }
}

#ifdef sfmacro_SimulationWithSteadyStateProbability
void IdealWifiManagerForMarkovChannelModel::CalculateTheStatesOfEachAntenna (int DestinationStationNumber, int SourceStationNumber)
{
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
#endif
/*
void IdealWifiManagerForMarkovChannelModel::TrackAntennasState(int SourceStationNumber)
{
  CounterForTrackingAntennasState++;
  for (int i=0; i<NumberOfStations; i++)
	for (int j=0; j<NumberOfantennas; j++)
		StateFrequency[i][j][CurrentStateOfEachAntennaInEcachStation[i][j]]++;

  //Validating the Markov chain viewed by Source Station with index 0 i.e. AP
  //Note that You can Validate all source and destination pairs.
  if( (CounterForTrackingAntennasState==50000) && (SourceStationNumber==0)){
	  std::ofstream fout("ValidationOfChannelSimulation.txt");
	  fout<<"*******   Validating the markov chain viewed by  AP   ******* " <<std::endl;
	  fout<<"We compare steady state probability of each State to the probability that comes from the Channel Simulation. " <<std::endl;
	  fout<<"These two Probabilities must be almost equal." <<std::endl<<std::endl;
	  for (int i=0; i<NumberOfStations; i++)
		  if(i!=SourceStationNumber){
			  fout << std::endl<<"*** Validating for Destination with index " << i << "  ***"<<std::endl;
			  for( int j=0; j<NumberOfantennas;j++)
				  for( int l=0; l< k;l++)
					  fout<< "Probability Of being The Antenna "<< j << "in State "<<l << " is "
	  	    		   << (static_cast<double>(StateFrequency[i][j][l])/ CounterForTrackingAntennasState) <<std::endl
	  	    		   << " and steady state Probability of This State is  "
					   << p[(i) + (NumberOfStations*SourceStationNumber)][l]<<std::endl;


		  }
  }else{
	  Simulator::Schedule(MicroSeconds(SymbolDuration),&IdealWifiManagerForMarkovChannelModel::TrackAntennasState,this, SourceStationNumber);
  }
}*/

} // namespace ns3

