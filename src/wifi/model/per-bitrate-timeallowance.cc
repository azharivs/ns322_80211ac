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
#include <math.h>
#include <stdlib.h>
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
                       StringValue ("./TimeAllowance.txt"),
                       MakeStringAccessor (&PerBitrateTimeAllowance::m_filename),
                       MakeStringChecker())
/*        .AddAttribute ("TID", "Traffic Indication Map of Interest.",
                       UintegerValue (UP_VI),
                       MakeUintegerAccessor (&PerBitrateTimeAllowance::m_tid),
                       MakeUintegerChecker<uint64_t> ())
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
  PerBitrateTimeAllowance::Init (Ptr<PerStaQInfo> staQInfo, std::string filename)
  {//TODO improve implementation using strtod()'s way for iteratively reading doubles
    NS_ASSERT(staQInfo);
    m_staQInfo = staQInfo;//should be initialized by now
    m_filename = std::string(filename);
    std::fstream ifs(m_filename.c_str(), std::fstream::in);
    NS_ASSERT(ifs.is_open());
    char line[500];
    std::string str;
    std::string substr;
    size_t pos,pos1,len;
    uint8_t index=0;
    bool flag = true;
    while (ifs.good())
      {
        ifs.getline(line,500); //MAC_ADDRESS t1 t2 t3 ... \n
        str = line;
        pos = str.find(' ');//find first blank right after keyword
        if (pos == std::string::npos)
          {
            flag = false;
            break;
          }
        substr = str.substr(0,pos);
        //add code for reading set of bitrates from the first line of the file
        if (substr == "bitrates")
          {//continue to the end of line and extract supported bitrates
            std::cout << "bitrates= ";//sva for debug
            while (true)
              {
                pos = str.find('.',pos);//get location of next '.' of next entry
                if (pos == std::string::npos)
                  break;
                pos --;//go back one location to point to start of next entry
                pos1 = str.find(' ',pos);
                if (pos1 == std::string::npos)
                  pos1 = str.length();
                len = pos1-pos;
                substr = str.substr(pos,len);//extract entry
                m_bitrates.push_back(lrint(strtod(substr.c_str(), NULL)));
                std::cout << m_bitrates[index] << ", ";//sva for debug
                m_insufficientTimeAllowance[index] = false;
                index ++;
                pos = pos1;
              }
            std::cout << "\n"; //sva for debug
          }
        else if (Mac48Address(substr.c_str())==GetMac())
          {//continue to the end of line and extract time allowances
            std::cout << "MAC=" << substr << ", time allowances= ";//sva for debug
            index = 0;
            while (true)
              {
                pos = str.find('.',pos);//get location of next '.' of next time allowance value
                if (pos == std::string::npos)
                  break;
                pos --;//go back one location to point to start of next time allowance value
                pos1 = str.find(' ',pos);
                if (pos1 == std::string::npos)
                  pos1 = str.length();
                len = pos1-pos;
                substr = str.substr(pos,len);//extract time allowance
                m_timeAllowance[m_bitrates[index]] = Seconds(atof(substr.c_str()));
                std::cout << m_timeAllowance[m_bitrates[index]] << ", ";//sva for debug
                index ++;
                pos = pos1;
              }
            std::cout << "\n"; //sva for debug
          }
      }
    ifs.close();
    return flag;
  }

  Mac48Address&
  PerBitrateTimeAllowance::GetMac (void)
  {//TODO
    return m_staQInfo->GetMac();
  }

  Time
  PerBitrateTimeAllowance::GetTimeAllowance(uint64_t bitrate)
  {
    return m_timeAllowance[bitrate];
  }

  Time
  PerBitrateTimeAllowance::GetRemainingTimeAllowance(uint64_t bitrate)
  {
    return m_remainingTimeAllowance[bitrate];
  }

  bool
  PerBitrateTimeAllowance::IsInsufficientTimeAllowanceEncountered (uint64_t bitrate)
  {
    if (m_timeAllowance[bitrate] == 0)
      m_insufficientTimeAllowance[bitrate] = true;
    return m_insufficientTimeAllowance[bitrate];
  }


  void
  PerBitrateTimeAllowance::SetInsufficientTimeAllowanceEncountered (uint64_t bitrate)
  {
    m_insufficientTimeAllowance[bitrate] = true;
  }

  Time
  PerBitrateTimeAllowance::DeductTimeAllowance(Time allowance, uint64_t bitrate)
  {
    m_remainingTimeAllowance[bitrate] -= allowance;
    if (m_remainingTimeAllowance[bitrate] <= 0)
      {
        SetInsufficientTimeAllowanceEncountered(bitrate);
      }
    return m_remainingTimeAllowance[bitrate];
  }

  void
  PerBitrateTimeAllowance::SetRemainingTimeAllowance(Time allowance, uint64_t bitrate)
  {
    m_remainingTimeAllowance[bitrate] = allowance;
  }

  void
  PerBitrateTimeAllowance::SetTimeAllowance(Time allowance, uint64_t bitrate)
  {
    m_timeAllowance[bitrate] = allowance;
    if (m_timeAllowance[bitrate] == 0)
      m_insufficientTimeAllowance[bitrate] = true;
    else
      m_insufficientTimeAllowance[bitrate] = false;
  }

  void
  PerBitrateTimeAllowance::ResetTimeAllowance(Time allowance, uint64_t bitrate)
  {
    m_timeAllowance[bitrate] = allowance;//set value for per SI allowance
#ifdef SVA_DEBUG_DETAIL
    std::cout << Simulator::Now().GetSeconds() << " " << this->GetMac() << " PerBitrateTimeAllowance::ResetTimeAllowance last remaining " <<
        m_remainingTimeAllowance[bitrate].GetSeconds()*1000 << " msec, Updated to " <<
        (m_timeAllowance[bitrate]+m_remainingTimeAllowance[bitrate]).GetSeconds()*1000 <<
        " msec bitrate=" << bitrate <<"\n";
#endif
    //in this version we carry the unused part of the time allowance to the next service interval
    m_remainingTimeAllowance[bitrate] += m_timeAllowance[bitrate]; //update allowance for next SI
    if (m_remainingTimeAllowance[bitrate] == 0)
      m_insufficientTimeAllowance[bitrate] = true;
    else
      m_insufficientTimeAllowance[bitrate] = false;
  }

  void
  PerBitrateTimeAllowance::ResetTimeAllowance(uint64_t bitrate)
  {
    //in this version we carry the unused part of the time allowance to the next service interval
#ifdef SVA_DEBUG
    std::cout << Simulator::Now().GetSeconds() << " " << this->GetMac() << " PerBitrateTimeAllowance::ResetTimeAllowance last remaining " <<
        m_remainingTimeAllowance[bitrate].GetSeconds()*1000 << " msec, Updated to " <<
        (m_timeAllowance[bitrate]+m_remainingTimeAllowance[bitrate]).GetSeconds()*1000 <<
        " msec bitrate= " << (double)bitrate/1e6 <<" Mbps\n";
#endif
    m_remainingTimeAllowance[bitrate] += m_timeAllowance[bitrate]; //update allowance for next SI
    if (m_remainingTimeAllowance[bitrate] == 0)
      m_insufficientTimeAllowance[bitrate] = true;
    else
      m_insufficientTimeAllowance[bitrate] = false;
  }

  void
  PerBitrateTimeAllowance::ResetAllTimeAllowances(void)
  {
    //in this version we carry the unused part of the time allowance to the next service interval
    std::vector<uint64_t>::iterator it;
    for ( it = m_bitrates.begin(); it != m_bitrates.end(); ++it)
      {
        ResetTimeAllowance((*it));
      }
  }

  PerBitrateTimeAllowanceHelper::PerBitrateTimeAllowanceHelper(void)
  {
  }

  PerBitrateTimeAllowanceHelper::~PerBitrateTimeAllowanceHelper(void)
  {
  }

  void
  PerBitrateTimeAllowanceHelper::Install(PerStaQInfoContainer c, std::string filename)
  {
    for (PerStaQInfoContainer::Iterator it = c.Begin(); it != c.End(); ++it)
      {
        Ptr<PerBitrateTimeAllowance> taPtr = CreateObject<PerBitrateTimeAllowance>();
        taPtr->Init((*it),filename);
        (*it)->AggregateObject(taPtr);
      }
  }

} // namespace ns3
