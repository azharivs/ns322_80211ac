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
#include "ideal-wifi-manager-for-markov-channel-model-11n.h"
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

NS_OBJECT_ENSURE_REGISTERED (IdealWifiManagerForMarkovChannelModel11n);

TypeId
IdealWifiManagerForMarkovChannelModel11n::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::IdealWifiManagerForMarkovChannelModel11n")
    .SetParent<WifiRemoteStationManager> ()
    .AddConstructor<IdealWifiManagerForMarkovChannelModel11n> ()
	.AddAttribute ("Number Of antennas",
			       "The number of antennas in remote station(destination): " ,
			       UintegerValue (2),
				   MakeUintegerAccessor (&IdealWifiManagerForMarkovChannelModel11n::NumberOfantennas),
				   MakeUintegerChecker<uint32_t> ())
     .AddAttribute ("Initialize State",
			       "The Markov State that we begin to simulate the channel from it " ,
			       UintegerValue (3),
				   MakeUintegerAccessor (&IdealWifiManagerForMarkovChannelModel11n::InitializeState),
				   MakeUintegerChecker<uint32_t> ())
	.AddAttribute ("MinPacketTransmissionToStartChannelSimulationForRemoteStation",
			       "The MinIMUM Packet Transmission to Start Channel simulation For Remote Station " ,
			       UintegerValue (20),
				   MakeUintegerAccessor (&IdealWifiManagerForMarkovChannelModel11n::MinPacketTransmissionToStartChannelSimulationForRemoteStation),
				   MakeUintegerChecker<uint32_t> ())
	 .AddAttribute ("Symbol Duration",
			       "Duration of each symbol " ,
			       UintegerValue (4),
				   MakeUintegerAccessor (&IdealWifiManagerForMarkovChannelModel11n::SymbolDuration),
				   MakeUintegerChecker<uint32_t> ())
	.AddAttribute ("Rho",
			       "expected value of the  received  signal  to  noise  ratio",
				   DoubleValue (5.13),
				   MakeDoubleAccessor (&IdealWifiManagerForMarkovChannelModel11n::Rho),
				   MakeDoubleChecker<double> ())
	.AddAttribute ("fm",
			       "Maximum doppler frequency fm= nu/lambda where  nu is the  speed  of  the  vehicle  and  lambda  is the  wave length",
				   DoubleValue (2),
				   MakeDoubleAccessor (&IdealWifiManagerForMarkovChannelModel11n::fm),
				   MakeDoubleChecker<double> ())
	.AddAttribute ("R",
			       "transmission rate of symbols  per  second",
				   DoubleValue (300),
				   MakeDoubleAccessor (&IdealWifiManagerForMarkovChannelModel11n::R),
				   MakeDoubleChecker<double> ())


  ;

  return tid;
}

IdealWifiManagerForMarkovChannelModel11n::IdealWifiManagerForMarkovChannelModel11n ()
{
  pi=3.1415926;
  MinPacketTransmissionToStartChannelSimulationForRemoteStation=20;
  k=9;
  A =new double  [k];
  A[0]=0;
  A[1]=0.5;
  A[2]=2.5;
  A[3]=3;
  A[4]=4.5;
  A[5]=5.3;
  A[6]= 9.12;
  A[7]=10.5;
  A[8]=11;
  MCS_Datarates = new double  [k];
  MCS_Datarates[0]= 0;
  MCS_Datarates[1]=6.5;
  MCS_Datarates[2]=13;
  MCS_Datarates[3]=19.5;
  MCS_Datarates[4]=26;
  MCS_Datarates[5]=39;
  MCS_Datarates[6]=52;
  MCS_Datarates[7]=58.5;
  MCS_Datarates[8]=65;
}

IdealWifiManagerForMarkovChannelModel11n::~IdealWifiManagerForMarkovChannelModel11n ()
{
}

WifiRemoteStation *
IdealWifiManagerForMarkovChannelModel11n::DoCreateStation (void) const
{
  IdealWifiRemoteStationForMarkovChannelModel11n *station = new IdealWifiRemoteStationForMarkovChannelModel11n ();
  double *N, *Rt;
  station->Rho =Rho;
  station->NumberOfantennas = NumberOfantennas;
  station->IsInitializationAntennas =true;
  //station->CounterForTrackingAntennasState =0;
  station->NumberOfCallToDoGetDataTxVector =0;
  station->CurrentStateOfEachAntenna= new uint32_t [station->NumberOfantennas];
  for (uint32_t i=0; i< (station->NumberOfantennas); i++)
	  station->CurrentStateOfEachAntenna[i]=InitializeState;
  station->p =new double  [k];
  N =new double  [k];
  Rt=new double  [k];
  station->T =new double *[k];
  for (uint32_t i=0; i<k; i++)
	  station->T[i]= new double [k];
  station->StateFrequency = new uint32_t *[station->NumberOfantennas];
  for (uint32_t i=0; i<station->NumberOfantennas; i++)
	  station->StateFrequency[i]= new uint32_t [k];
   for ( uint32_t i =0; i< station->NumberOfantennas; i++)
	 for ( uint32_t j =0; j<= (k-1); j++)
	  	station->StateFrequency[i][j]=0;
  // *******************     computation    **********************************
  // equation 34 : calculating p
  for ( uint32_t i =0; i<= (k-1); i++) {
	  if (i!=k-1)
		station->p[i] = exp( -A[i] / (station->Rho) )- exp( -A[i+1] / (station->Rho) );
	else
		station->p[i] = exp( -A[i] / (station->Rho) ); //
  }

  //calculating Rt(k)
  for ( uint32_t i =0; i<= (k-1); i++)
	  Rt[i] = R * station->p [i];

  //calculating N from equation 33
  for ( uint32_t  i =1; i<= (k-1); i++)
	 N[i] = sqrt((2*pi*A[i])/(station->Rho)) * fm * exp( -A[i]/(station->Rho));


  //initialization of T
  for ( uint32_t  i=0; i < k; i++)
	  for ( uint32_t  j=0; j < k; j++)
		  station->T[i][j]=0;

  // calculating T from equation 39, 40
  for ( uint32_t  i =0; i<= (k-2); i++)
	station->T[i][i+1] = N[i+1] / Rt[i];
  for ( uint32_t  i =1; i<= (k-1); i++)
	station->T[i][i-1] = N[i] / Rt[i];
  station->T[0][0] = 1- station->T[0][1];
  station->T[k-1][k-1] = 1- station->T[k-1][k-2];
  for ( uint32_t  i =1; i<= (k-2); i++)
	station->T[i][i] = 1 - station->T[i][i-1]- station->T[i][i+1] ;

   //variables for reading and writing

  std::ofstream fout("Outputdocreation.txt");

  //********* Writing in the output.txt **************
  fout<< "*******     State transition  matrix  T     *******"<<std::endl;
  for ( uint32_t i=0; i < k; i++)
	for ( uint32_t j=0; j < k; j++){
	  NS_ASSERT ( station->T[i][j] >=0 &&  station->T[i][j] <= 1);
	  fout <<"T ["<<i<<"] ["<<j<<"] = " << station->T[i][j] <<std::endl;
	  if (j==k-1)
		fout<<std::endl;
	}
  fout<<"*******     Steady  state  probability vector p     *******"<<std::endl;
  for ( uint32_t i=0; i < k; i++) {
	NS_ASSERT ( station->p[i] >=0  &&   station->p[i] <= 1);
	fout<<station->p[i]<<std::endl;
  }

  //***************   Verification: 42 in page 8 and 2 in page 2  **************
  fout<<std::endl<<"***********************************************************"
      <<std::endl<<"To verify  this  finite-state  Markov channel model," <<std::endl
	  <<"we use  state  equilibrium  equations  for  each  state (42 in page 8)"<<std::endl
	  <<"and equation 2 in page 2"<<std::endl
	  <<"Note that because of floating point operation, the two side of equations may not be exactly equal." <<std::endl
	  <<"***********************************************************"<<std::endl;

  double sum=0;
  for ( uint32_t i=0; i < k; i++)
	sum+=station->p[i];
  fout<< "The sum of p[i] is " << sum <<std::endl<<std::endl;
  for ( uint32_t i=1; i <= (k-2); i++){
	fout << "p [" << i  <<"] * (T ["<< i <<"]["<<i-1<<"] + T ["<<i<<"]["       <<i+1<<"]) must be equal to "
	     << "p [" << i-1<<"] * T [" <<i-1<<"]["<<i  <<"] + p ["  <<i+1<<"] * T ["<<i+1<<"]["<<i<<"]"<<std::endl;
	fout << station->p[i]*(station->T[i][i-1] + station->T[i][i+1])<<" must be equal to "
	     << station->p[i-1]*station->T[i-1][i]+station->p[i+1]*station->T[i+1][i]<<std::endl<<std::endl;
  }

  return station;
}


void
IdealWifiManagerForMarkovChannelModel11n::DoReportRxOk (WifiRemoteStation *station,
                                double rxSnr, WifiMode txMode)
{
}
void
IdealWifiManagerForMarkovChannelModel11n::DoReportRtsFailed (WifiRemoteStation *station)
{
}
void
IdealWifiManagerForMarkovChannelModel11n::DoReportDataFailed (WifiRemoteStation *station)
{
}
void
IdealWifiManagerForMarkovChannelModel11n::DoReportRtsOk (WifiRemoteStation *st,
                                 double ctsSnr, WifiMode ctsMode, double rtsSnr)
{

}
void
IdealWifiManagerForMarkovChannelModel11n::DoReportDataOk (WifiRemoteStation *st,
                                  double ackSnr, WifiMode ackMode, double dataSnr)
{

}
void
IdealWifiManagerForMarkovChannelModel11n::DoReportFinalRtsFailed (WifiRemoteStation *station)
{
}
void
IdealWifiManagerForMarkovChannelModel11n::DoReportFinalDataFailed (WifiRemoteStation *station)
{
}

WifiTxVector
IdealWifiManagerForMarkovChannelModel11n::DoGetDataTxVector (WifiRemoteStation *st, uint32_t size)
{
  IdealWifiRemoteStationForMarkovChannelModel11n *station = (IdealWifiRemoteStationForMarkovChannelModel11n *)st;

  #ifdef sfmacro_SimulationWithSteadyStateProbability
  CalculateTheStatesOfEachAntenna (station);
  #else

  // We want to initialize all antennas state  not the antennas  that has index 0
  if( station->IsInitializationAntennas) {
	  station->NumberOfCallToDoGetDataTxVector ++;
	  if(station->NumberOfCallToDoGetDataTxVector>=MinPacketTransmissionToStartChannelSimulationForRemoteStation){
	      ChangeTheStateOfAntennas (station, 0,InitializeState, true);
	      //TrackAntennasState(station);
	      station->IsInitializationAntennas=false;
	  }
  }
  #endif

  // because  m_deviceRateSet have OfdmRate6Mbps,OfdmRate12Mbps, and OfdmRate24Mbps by default, we must add mcs index with 3.
  uint32_t i = SpatialMultiplexing(station) + 3;
  //i = rand()% (GetNSupported (station));
  //sf I added below "if" because at first mcs was not added to Supported rates
  if (i>=(GetNSupported (station)))
	  i= 0;
  WifiMode maxMode =  GetSupported (station, i);
  return WifiTxVector (maxMode, GetDefaultTxPowerLevel (), GetLongRetryCount (station), GetShortGuardInterval (station), Min (GetNumberOfReceiveAntennas (station),GetNumberOfTransmitAntennas()), GetNess (station), GetStbc (station));
}
WifiTxVector
IdealWifiManagerForMarkovChannelModel11n::DoGetRtsTxVector (WifiRemoteStation *st)
{
  IdealWifiRemoteStationForMarkovChannelModel11n *station = (IdealWifiRemoteStationForMarkovChannelModel11n *)st;

  #ifdef sfmacro_SimulationWithSteadyStateProbability
  CalculateTheStatesOfEachAntenna (station);
  #endif

  // because  m_deviceRateSet have OfdmRate6Mbps,OfdmRate12Mbps, and OfdmRate24Mbps by default, we must add mcs index with 3.
  uint32_t i = SpatialMultiplexing(station) + 3;
  //sf I added below "if" because at first mcs was not added to Supported rates
  if (i>=(GetNSupported (station)))
  	  i= 0;
  WifiMode maxMode =  GetSupported (station, i);
  return WifiTxVector (maxMode, GetDefaultTxPowerLevel (), GetShortRetryCount (station), GetShortGuardInterval (station), Min (GetNumberOfReceiveAntennas (station),GetNumberOfTransmitAntennas()), GetNess (station), GetStbc (station));
}

bool
IdealWifiManagerForMarkovChannelModel11n::IsLowLatency (void) const
{
  return true;
}

uint32_t IdealWifiManagerForMarkovChannelModel11n::SpatialMultiplexing (IdealWifiRemoteStationForMarkovChannelModel11n *station)
{
  // If all of antenna are in the state 0 that we send with MCS0
  //check if all of them are in state 0?
  bool IsAllAntennasInState0 = true;
  for (uint32_t i=0; i<(station->NumberOfantennas); i++) {
	if( ( station->CurrentStateOfEachAntenna[i]) != 0 )
		IsAllAntennasInState0 =	false;
  }
  // when all Antenna is in state 0 then we send with dof1 with lowest rate that means MCS0
  if (IsAllAntennasInState0)
	  return 0;
  //first we copy CurrentStateOfEachAntenna[i] to CopyOfCurrentStateOfEachAntenna[i]
  uint32_t *CopyOfCurrentStateOfEachAntenna, *McsIndexBasedOnDof;
  double *TransmissionRateBasedOnDof;

  CopyOfCurrentStateOfEachAntenna=new uint32_t [ (station->NumberOfantennas)];
  TransmissionRateBasedOnDof=new double [ (station->NumberOfantennas)]; //Dof 4 is presented with  Transmission_Rate_based_on_Dof[3]
  McsIndexBasedOnDof = new uint32_t [ (station->NumberOfantennas)];

  for (uint32_t i=0; i<(station->NumberOfantennas); i++) {
	  CopyOfCurrentStateOfEachAntenna[i]= station->CurrentStateOfEachAntenna[i];
  }

  //sort CopyOfCurrentStateOfEachAntenna by means of bubble sort
  // therefore the worst SNR is in CopyOfCurrentStateOfEachAntenna[0]
  uint32_t temp;
  for ( uint32_t pass=0; pass < ( (station->NumberOfantennas)-1); pass++){
	for ( uint32_t i = 0; i<( (station->NumberOfantennas)-1-pass); i ++)
	  if ( CopyOfCurrentStateOfEachAntenna[i] > CopyOfCurrentStateOfEachAntenna[i+1] ){
		temp =  CopyOfCurrentStateOfEachAntenna[i];
		CopyOfCurrentStateOfEachAntenna[i] = CopyOfCurrentStateOfEachAntenna[i+1];
		CopyOfCurrentStateOfEachAntenna[i+1] = temp;
      }
  }

  TransmissionRateBasedOnDof[( (station->NumberOfantennas)-1)] = MCS_Datarates[CopyOfCurrentStateOfEachAntenna[0]] ; // with Dof 4 non of antenna is deleted so with minimum rate we should send.
  McsIndexBasedOnDof[( (station->NumberOfantennas)-1)] = CopyOfCurrentStateOfEachAntenna[0];


  //because 802.11 n uses the worst SNR, we just need to recalculate CopyOfCurrentStateOfEachAntenna[i]
  // we start with ignoring the worst SNR i.e CopyOfCurrentStateOfEachAntenna[i-1] , SNR in each chain will increase
  // because 802.11 n uses the worst SNR between antennas, we just need to recalculate CopyOfCurrentStateOfEachAntenna[i]

  for (uint32_t i=1; i< (station->NumberOfantennas); i ++) {
	uint32_t counter = k-1-CopyOfCurrentStateOfEachAntenna[i];
    TransmissionRateBasedOnDof[((station->NumberOfantennas)-(i+1))] = MCS_Datarates[CopyOfCurrentStateOfEachAntenna[i]] ;
    McsIndexBasedOnDof[((station->NumberOfantennas)-(i+1))] = CopyOfCurrentStateOfEachAntenna[i] ;
    while(counter!=0){
	  if (( ((station->NumberOfantennas)) * A[CopyOfCurrentStateOfEachAntenna[i]] / ((station->NumberOfantennas)-i) ) >= (A [CopyOfCurrentStateOfEachAntenna[i] + counter]) ){
	    TransmissionRateBasedOnDof[((station->NumberOfantennas)-(i+1))] = MCS_Datarates[CopyOfCurrentStateOfEachAntenna[i] +counter] ;
	    McsIndexBasedOnDof[((station->NumberOfantennas)-(i+1))] = CopyOfCurrentStateOfEachAntenna[i] +counter ;
	    break;
	  }
	  counter--;
    }
  }

  double rate = TransmissionRateBasedOnDof[((station->NumberOfantennas)-1)] ;
  uint32_t Dof=(station->NumberOfantennas);
  // McsIndex is between 0 to (k-2) ,
  uint32_t McsIndex = McsIndexBasedOnDof[((station->NumberOfantennas)-1)] -1  ;

  // calculate the maximum rate that can be obtained
  for (uint32_t i=1; i< (station->NumberOfantennas); i ++) {
    if(( ((station->NumberOfantennas)-i)*TransmissionRateBasedOnDof[((station->NumberOfantennas)-(i+1))] ) > (Dof*rate)){
	  Dof=((station->NumberOfantennas)-i);
	  rate = TransmissionRateBasedOnDof[((station->NumberOfantennas)-(i+1))];
	  McsIndex = McsIndexBasedOnDof[((station->NumberOfantennas)-(i+1))] -1;
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

void IdealWifiManagerForMarkovChannelModel11n::ChangeTheStateOfAntennas (  IdealWifiRemoteStationForMarkovChannelModel11n *station, uint32_t AntennaNumber ,uint32_t NextState,bool IsInitialization) const
{
  double RandomProbability,UniformRandomVariableBetween0and1,
         ProbabilityOfRemainingInThisState,
		 NumberOfClocksItTakesToExit; //It is geometric distributed random variable and take 0,1,2, 3, ...
                                     //  0 means that the antenna exist from current state at the next clock
                                     // we use the way that described in "Lewis, P. W. A., and Ed McKenzie. Simulation methodology for statisticians, operations analysts, and engineers. Vol. 1. CRC press, 1988" for generating geometrically distributed random variable.
  uint32_t  NextStateOfCurrentNextState;
  Ptr<UniformRandomVariable>  RandomVariable =   CreateObject<UniformRandomVariable> ();

  if(IsInitialization){
	  //debug
	  //std::cout<<"destination is"<<(station->m_state->m_address)<<std::endl;
	 // Initialize all antennas
     for (uint32_t j=0; j<station->NumberOfantennas; j++){
    	 station->CurrentStateOfEachAntenna[j]= NextState;
 		  //calculate the next state that will be schedule
 		  RandomProbability = (RandomVariable->GetValue(0,1));
 		  ProbabilityOfRemainingInThisState = station->T  [NextState][NextState];
 		  if (NextState ==0) {
 			  NextStateOfCurrentNextState = NextState+1;
 		  }else if (NextState ==(k-1)){
 		      NextStateOfCurrentNextState = NextState -1;
 	      }else{
 			  if(RandomProbability <= ((station->T  [NextState][NextState -1])/(1-ProbabilityOfRemainingInThisState)))
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
 				               &IdealWifiManagerForMarkovChannelModel11n::ChangeTheStateOfAntennas, this,
 		                       station,
							   j,
							   NextStateOfCurrentNextState,
 		                       false);

 	}
   }else{
	 //Debug
	 //std::cout<< "After Initialization in ChangeTheStateOfAntennas :Source Number = " << SourceStationNumber<<" Destination Number = "<<DestinationStationNumber<<std::endl;
	 // It is not initialization, so we must change the desired antenna state
	 station->CurrentStateOfEachAntenna[AntennaNumber]= NextState;
     //calculate the next state that will be schedule
	 RandomProbability = (RandomVariable->GetValue(0,1));
     ProbabilityOfRemainingInThisState = station->T  [NextState][NextState];
	 if (NextState ==0) {
	 	NextStateOfCurrentNextState = NextState+1;
	 }else if (NextState ==(k-1)){
	 	NextStateOfCurrentNextState = NextState -1;
	 }else{
	 	if(RandomProbability <= ((station->T  [NextState][NextState -1])/(1-ProbabilityOfRemainingInThisState)))
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
	 		              &IdealWifiManagerForMarkovChannelModel11n::ChangeTheStateOfAntennas, this,
						  station,
						  AntennaNumber,
	 					  NextStateOfCurrentNextState,
	                      false);
   }
}

#ifdef sfmacro_SimulationWithSteadyStateProbability
void IdealWifiManagerForMarkovChannelModel11n::CalculateTheStatesOfEachAntenna (IdealWifiRemoteStationForMarkovChannelModel11n *station)
{
  for (uint32_t i=0; i< station->NumberOfantennas; i++){
	double random_probability = 0, mm=10000,
		   SumOfP=0; //we use this to determine in which states the antenna is
		   random_probability= ( rand() % 10000)/mm;
	for ( uint32_t j =0; j < k; j++) {
	  if (j!=k-1){
	    if( (SumOfP <= random_probability) && (random_probability < (SumOfP + station->p[j])) ){
	      station->CurrentStateOfEachAntenna[i]= j;
	      //sf debug
	      //std::cout<< " j is " << j <<std::endl;
	    }
	  } else{
		if( (SumOfP <= random_probability) && (random_probability < 1) ) {
		  station->CurrentStateOfEachAntenna[i]= j;
		  //sf debug
		  //std::cout<< " j is " << j <<std::endl;
		}
	  }
      SumOfP += station->p[j];
	}
  }
}
#endif
/*
void IdealWifiManagerForMarkovChannelModel11n::TrackAntennasState(IdealWifiRemoteStationForMarkovChannelModel11n *station)
{
	station->CounterForTrackingAntennasState++;

	for (uint32_t i=0; i<station->NumberOfantennas; i++)
		station->StateFrequency[i][station->CurrentStateOfEachAntenna[i]]++;

	if( (station->CounterForTrackingAntennasState==10000) ){
	  std::ofstream fout("ValidationOfChannelSimulation.txt");
	  fout<<"We compare steady state probability of each State to the probability that comes from the Channel Simulation. " <<std::endl;
	  fout<<"These two Probabilities must be almost equal." <<std::endl<<std::endl;
	  for( uint32_t i=0; i<station->NumberOfantennas;i++)
		 for( uint32_t j=0; j< k;j++)
			       fout<< "Probability Of being The Antenna "<< i << "in State "<< j << " is "
	  	    		   << (static_cast<double>(station->StateFrequency[i][j])/ station->CounterForTrackingAntennasState) <<std::endl
	  	    		   << " and steady state Probability of This State is  "
					   << station->p[j]<<std::endl;



  }else{
	  Simulator::Schedule(MilliSeconds(SymbolDuration),&IdealWifiManagerForMarkovChannelModel11n::TrackAntennasState,this, station);
  }
}
*/

} // namespace ns3

