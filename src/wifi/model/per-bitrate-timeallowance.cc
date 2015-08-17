/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) Jul 2, 2015 IUST
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
#include <list>
#include <deque>
#include <utility>
#include <fstream>
#include "ns3/packet.h"
#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/uinteger.h"
#include "ns3/string.h"
#include "per-bitrate-timeallowance.h"

namespace ns3 {

  NS_OBJECT_ENSURE_REGISTERED (PerBitrateTimeAllowance);

  TypeId
  PerBitrateTimeAllowance::GetTypeId (void)
  {
    static TypeId tid = TypeId ("ns3::PerBitrateTimeAllowance")
        .SetParent<Object> ()
        .AddConstructor<PerBitrateTimeAllowance> ()
        .AddAttribute ("FileName", "Path and Filename Containing Per Bitrate Time Allowances. Used For Fixed Case: MAC_ADDRESS t1 t2 t3 ...\n",
                       StringValue ("./T.txt"),
                       MakeStringAccessor (&PerBitrateTimeAllowance::m_filename),
                       MakeStringChecker())
/*        .AddAttribute ("TID", "Traffic Indication Map of Interest.",
                       UintegerValue (UP_VI),
                       MakeUintegerAccessor (&PerBitrateTimeAllowance::m_tid),
                       MakeUintegerChecker<uint32_t> ())
*/
        ;
    return tid;
  }

  PerBitrateTimeAllowance::PerBitrateTimeAllowance()
  {
  }

  PerBitrateTimeAllowance::~PerBitrateTimeAllowance()
  {
  }

  bool
  PerBitrateTimeAllowance::Init (Ptr<PerStaQInfo> staQInfo)
  {
    NS_ASSERT(staQInfo);
    m_staQInfo = staQInfo;//should be initialized by now
    std::ifstream ifs(m_filename.c_str(), std::ifstream::in);
    NS_ASSERT(ifs.is_open());
    char line[500];
    std::string str;
    std::string substr;
    size_t pos,pos1;
    while (ifs.good())
      {
        ifs.getline(line,500); //MAC_ADDRESS t1 t2 t3 ... \n
        str = line;
        pos = str.find(' ');//find first blank right after MAC address
        if (pos == std::string::npos)
          return false;
        pos --; //point to end of MAC address
        substr = str.substr(0,pos);
        //TODO add code for reading set of bitrates from the first line of the file
        if (Mac48Address(substr.c_str())==GetMac())
          {
            pos ++;
            bool endOfLine = false;
            while (!endOfLine)
              {
                pos = str.find('.',pos);//get location of next '.' of next time allowance value
                if (pos == std::string::npos)
                  return false;
                pos --;//go back one location to point to start of next time allowance value
                pos1 = str.find(' ',pos);
                if (pos1 == std::string::npos)
                  return false;
                pos1 --;
                substr = str.substr(pos,pos1);//extract time allowance
                //TODO add code for associating the time allowance to a bitrate
              }
          }
      }
    //TODO: m_insufficientTimeAllowance[] = false;
    return true;
  }

  Mac48Address&
  PerBitrateTimeAllowance::GetMac (void)
  {//TODO
    return m_staQInfo->GetMac();
  }

  Time
  PerBitrateTimeAllowance::GetTimeAllowance(uint32_t bitrate)
  {
    return m_timeAllowance[bitrate];
  }

  Time
  PerBitrateTimeAllowance::GetRemainingTimeAllowance(uint32_t bitrate)
  {
    return m_remainingTimeAllowance[bitrate];
  }

  bool
  PerBitrateTimeAllowance::IsInsufficientTimeAllowanceEncountered (uint32_t bitrate)
  {
    if (m_timeAllowance[bitrate] == 0)
      m_insufficientTimeAllowance[bitrate] = true;
    return m_insufficientTimeAllowance[bitrate];
  }


  void
  PerBitrateTimeAllowance::SetInsufficientTimeAllowanceEncountered (uint32_t bitrate)
  {
    m_insufficientTimeAllowance[bitrate] = true;
  }

  Time
  PerBitrateTimeAllowance::DeductTimeAllowance(Time allowance, uint32_t bitrate)
  {
    m_remainingTimeAllowance[bitrate] -= allowance;
    if (m_remainingTimeAllowance[bitrate] <= 0)
      {
        SetInsufficientTimeAllowanceEncountered(bitrate);
      }
    return m_remainingTimeAllowance[bitrate];
  }

  void
  PerBitrateTimeAllowance::SetRemainingTimeAllowance(Time allowance, uint32_t bitrate)
  {
    m_remainingTimeAllowance[bitrate] = allowance;
  }

  void
  PerBitrateTimeAllowance::SetTimeAllowance(Time allowance, uint32_t bitrate)
  {
    m_timeAllowance[bitrate] = allowance;
    if (m_timeAllowance[bitrate] == 0)
      m_insufficientTimeAllowance[bitrate] = true;
    else
      m_insufficientTimeAllowance[bitrate] = false;
  }

  void
  PerBitrateTimeAllowance::ResetTimeAllowance(Time allowance, uint32_t bitrate)
  {
    m_timeAllowance[bitrate] = allowance;
    //in this version we carry the unused part of the time allowance to the next service interval
    if (m_remainingTimeAllowance[bitrate] > 0)
      {
        m_timeAllowance[bitrate] += m_remainingTimeAllowance[bitrate];
      }
#ifdef SVA_DEBUG_DETAIL
    std::cout << Simulator::Now().GetSeconds() << " " << this->GetMac() << " PerBitrateTimeAllowance::ResetTimeAllowance last remaining " <<
        m_remainingTimeAllowance[bitrate].GetSeconds()*1000 << " msec, Reset to " << m_timeAllowance[bitrate].GetSeconds()*1000 <<
        " msec bitrate=" << bitrate <<"\n";
#endif
    m_remainingTimeAllowance[bitrate] = m_timeAllowance[bitrate];
    if (m_timeAllowance[bitrate] == 0)
      m_insufficientTimeAllowance[bitrate] = true;
    else
      m_insufficientTimeAllowance[bitrate] = false;
  }

  void
  PerBitrateTimeAllowance::ResetTimeAllowance(uint32_t bitrate)
  {
    //in this version we carry the unused part of the time allowance to the next service interval
    Time leftOver(Seconds(0));
    if (m_remainingTimeAllowance[bitrate] > 0)
      {
        leftOver = m_remainingTimeAllowance[bitrate];
      }
#ifdef SVA_DEBUG_DETAIL
    std::cout << Simulator::Now().GetSeconds() << " " << this->GetMac() << " PerBitrateTimeAllowance::ResetTimeAllowance last remaining " <<
        m_remainingTimeAllowance[bitrate].GetSeconds()*1000 << " msec, Reset to " << (m_timeAllowance[bitrate] + leftOver).GetSeconds()*1000 <<
        " msec bitrate=" << bitrate <<"\n";
#endif
    m_remainingTimeAllowance[bitrate] = m_timeAllowance[bitrate] + leftOver;
    m_insufficientTimeAllowance[bitrate] = false;
  }


} // namespace ns3
