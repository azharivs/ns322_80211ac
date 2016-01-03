/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015, SEYED VAHID AZHARI
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


#ifndef PER_BITRATE_BITALLOWANCE_H
#define PER_BITRATE_BITALLOWANCE_H

#include <list>
#include <deque>
#include <utility>
#include <map>
#include <string>
#include <vector>
#include "ns3/packet.h"
#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/qos-tag.h"
#include "wifi-mac-header.h"
#include "ns3/simulator.h"
#include "per-sta-q-info.h"
#include "ns3/per-sta-q-info-container.h"

namespace ns3 {

/**
 * \ingroup wifi
 *
 */

class PerBitrateBitAllowance : public Object
{
public:
  static TypeId GetTypeId (void);
  PerBitrateBitAllowance ();
  ~PerBitrateBitAllowance ();

  /*
   * Initializes the class run once after perStaQInfo is initialized
   * returns true upon success
   */
  bool Init (Ptr<PerStaQInfo> staQInfo, std::string bitrate);

  /**
   * Get MAC address associated to this bit allowance
   *
   * \return reference to MAC address
   */
  Mac48Address& GetMac (void);

  /*
   * returns the nominal amount of bit allowance for this service interval for a particular bitrate
   */
  double GetBitAllowance(uint64_t bitrate);

  /*
   * returns the remaining bit allowance up to now for a particular bitrate
   */
  double GetRemainingBitAllowance(uint64_t bitrate);

  /*
   *
   */
  bool IsInsufficientBitAllowanceEncountered (uint64_t bitrate);

  /*
   *
   */
  void SetInsufficientBitAllowanceEncountered (uint64_t bitrate);

  /*
   * deduct input parameter from remaining bit allowance to new value
   */
  double DeductBitAllowance(double allowance, uint64_t bitrate);

  /*
   * set remaining bit allowance to new value
   */
  void SetRemainingBitAllowance(double allowance, uint64_t bitrate);

  /*
   * set bit allowance and do not touch remaining bit allowance
   */
  void SetBitAllowance(double allowance, uint64_t bitrate);

  /*
   * re-initializes the amount of remaining bit allowance to m_bitAllowance
   * This is called at the beginning of a new service interval
   */
  void ResetBitAllowance (uint64_t bitrate);

  /*
   * Sets m_bitAllowance to the provided parameter and
   * re-initializes the amount of remaining bit allowance to m_bitAllowance
   * This is called at the beginning of a new service interval
   */
  void ResetBitAllowance(double allowance, uint64_t bitrate);

  /*
   * re-initializes the amount of remaining bit allowance to m_bitAllowance
   * for all bitrates
   * This is called at the beginning of a new service interval
   */
  void ResetAllBitAllowances (void);


private:

  std::map<uint64_t,double> m_bitAllowance; //!< Nominal amount of bit allowance for the current service interval per bitrate. Used by PER_BITRATE_BIT_ALLOWANCE aggregation algorithm.
  std::map<uint64_t,double> m_remainingBitAllowance; //!< Amount of remaining bit allowance for the current service interval per bitrate. Used by PER_BITRATE_BIT_ALLOWANCE aggregation algorithm.
  std::map<uint64_t,bool> m_insufficientBitAllowance; //!< Insufficient amount of bit allowance encountered at last access for this bitrate
  Ptr<PerStaQInfo> m_staQInfo; //!< Pointer to PerStaQInfo instance corresponding to this object
  std::string m_filename; //!< String containing filename for fixed bit allowance values per bitrate for all stations MAC_ADDRESS t1 t2 t3 ... \n
  std::vector<uint64_t> m_bitrates; //!< List of supported bitrates
};


class PerBitrateBitAllowanceHelper
{
  public:
    PerBitrateBitAllowanceHelper (void);
    ~PerBitrateBitAllowanceHelper (void);
    void Install (PerStaQInfoContainer c, std::string filename);
};

} // namespace ns3

#endif /* PER_BITRATE_BITALLOWANCE_H */
