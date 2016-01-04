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
 * Author: Seyed Vahid Azhari <azharivs@iust.ac.ir>
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
#include "per-bitrate-bitallowance.h"

namespace ns3 {

  NS_OBJECT_ENSURE_REGISTERED (PerBitrateBitAllowance);

  TypeId
  PerBitrateBitAllowance::GetTypeId (void)
  {
    static TypeId tid = TypeId ("ns3::PerBitrateBitAllowance")
        .SetParent<Object> ()
        .AddConstructor<PerBitrateBitAllowance> ()
        .AddAttribute ("FileName", "Path and Filename Containing Per Bitrate Bit Allowances. Used For Fixed Case: MAC_ADDRESS t1 t2 t3 ...\n",
                       StringValue ("./BitAllowance.txt"),
                       MakeStringAccessor (&PerBitrateBitAllowance::m_filename),
                       MakeStringChecker())
/*        .AddAttribute ("TID", "Traffic Indication Map of Interest.",
                       UintegerValue (UP_VI),
                       MakeUintegerAccessor (&PerBitrateTimeAllowance::m_tid),
                       MakeUintegerChecker<uint64_t> ())
*/
        ;
    return tid;
  }

  PerBitrateBitAllowance::PerBitrateBitAllowance()
  {
  }

  PerBitrateBitAllowance::~PerBitrateBitAllowance()
  {
  }

  bool
  PerBitrateBitAllowance::Init (Ptr<PerStaQInfo> staQInfo, std::string filename)
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
                m_insufficientBitAllowance[index] = false;
                index ++;
                pos = pos1;
              }
            std::cout << "\n"; //sva for debug
          }
        else if (Mac48Address(substr.c_str())==GetMac())
          {//continue to the end of line and extract bit allowances
            std::cout << "MAC=" << substr << ", bit allowances= ";//sva for debug
            index = 0;
            pos=0;
            while (true)
              {
                pos = str.find(' ',pos);//get location of next '.' of next bit allowance value
                if (pos == std::string::npos)
                  break;
               // pos --;//go back one location to point to start of next bit allowance value
               
                pos1 = str.find(' ',pos+1);
                if (pos1 == std::string::npos)
                  pos1 = str.length();
                len = pos1-pos;
                substr = str.substr(pos,len);//extract bit allowance
                m_bitAllowance[m_bitrates[index]] = atof(substr.c_str());
                std::cout << m_bitAllowance[m_bitrates[index]] << ", ";//sva for debug
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
  PerBitrateBitAllowance::GetMac (void)
  {//TODO
    return m_staQInfo->GetMac();
  }

  double
  PerBitrateBitAllowance::GetBitAllowance(uint64_t bitrate)
  {
    return m_bitAllowance[bitrate];
  }

  double
  PerBitrateBitAllowance::GetRemainingBitAllowance(uint64_t bitrate)
  {
    return m_remainingBitAllowance[bitrate];
  }

  bool
  PerBitrateBitAllowance::IsInsufficientBitAllowanceEncountered (uint64_t bitrate)
  {
    if (m_bitAllowance[bitrate] == 0)
      m_insufficientBitAllowance[bitrate] = true;
    return m_insufficientBitAllowance[bitrate];
  }


  void
  PerBitrateBitAllowance::SetInsufficientBitAllowanceEncountered (uint64_t bitrate)
  {
    m_insufficientBitAllowance[bitrate] = true;
  }

  double
  PerBitrateBitAllowance::DeductBitAllowance(double allowance, uint64_t bitrate)
  {
    m_remainingBitAllowance[bitrate] -= allowance;
    if (m_remainingBitAllowance[bitrate] <= 0)
      {
        SetInsufficientBitAllowanceEncountered(bitrate);
      }
    return m_remainingBitAllowance[bitrate];
  }

  void
  PerBitrateBitAllowance::SetRemainingBitAllowance(double allowance, uint64_t bitrate)
  {
    m_remainingBitAllowance[bitrate] = allowance;
  }

  void
  PerBitrateBitAllowance::SetBitAllowance(double allowance, uint64_t bitrate)
  {
    m_bitAllowance[bitrate] = allowance;
    if (m_bitAllowance[bitrate] == 0)
      m_insufficientBitAllowance[bitrate] = true;
    else
      m_insufficientBitAllowance[bitrate] = false;
  }

  void
  PerBitrateBitAllowance::ResetBitAllowance(double allowance, uint64_t bitrate)
  {
    m_bitAllowance[bitrate] = allowance;//set value for per SI allowance
#ifdef SVA_DEBUG_DETAIL
    std::cout << Simulator::Now().GetSeconds() << " " << this->GetMac() << " PerBitrateBitAllowance::ResetBitAllowance last remaining " <<
        m_remainingBitAllowance[bitrate] << " bits, Updated to " <<
        (m_bitAllowance[bitrate]+m_remainingBitAllowance[bitrate]) <<
        " bit, bitrate=" << bitrate <<"\n";
#endif
    //in this version we carry the unused part of the bit allowance to the next service interval
    m_remainingBitAllowance[bitrate] += m_bitAllowance[bitrate]; //update allowance for next SI
    if (m_remainingBitAllowance[bitrate] == 0)
      m_insufficientBitAllowance[bitrate] = true;
    else
      m_insufficientBitAllowance[bitrate] = false;
  }

  void
  PerBitrateBitAllowance::ResetBitAllowance(uint64_t bitrate)
  {
    //in this version we carry the unused part of the bit allowance to the next service interval
#ifdef SVA_DEBUG
    std::cout << Simulator::Now().GetSeconds() << " " << this->GetMac() << " PerBitrateBitAllowance::ResetBitAllowance last remaining " <<
        m_remainingBitAllowance[bitrate] << " bit, Updated to " <<
        (m_bitAllowance[bitrate]+m_remainingBitAllowance[bitrate]) <<
        " bit bitrate= " << (double)bitrate/1e6 <<" Mbps\n";
#endif
    m_remainingBitAllowance[bitrate] += m_bitAllowance[bitrate]; //update allowance for next SI
    if (m_remainingBitAllowance[bitrate] == 0)
      m_insufficientBitAllowance[bitrate] = true;
    else
      m_insufficientBitAllowance[bitrate] = false;
  }

  void
  PerBitrateBitAllowance::ResetAllBitAllowances(void)
  {
    //in this version we carry the unused part of the time allowance to the next service interval
    std::vector<uint64_t>::iterator it;
    for ( it = m_bitrates.begin(); it != m_bitrates.end(); ++it)
      {
        ResetBitAllowance((*it));
      }
  }

  PerBitrateBitAllowanceHelper::PerBitrateBitAllowanceHelper(void)
  {
  }

  PerBitrateBitAllowanceHelper::~PerBitrateBitAllowanceHelper(void)
  {
  }

  void
  PerBitrateBitAllowanceHelper::Install(PerStaQInfoContainer c, std::string filename)
  {
    for (PerStaQInfoContainer::Iterator it = c.Begin(); it != c.End(); ++it)
      {
        Ptr<PerBitrateBitAllowance> baPtr = CreateObject<PerBitrateBitAllowance>();
        baPtr->Init((*it),filename);
        (*it)->AggregateObject(baPtr);
      }
  }

} // namespace ns3
