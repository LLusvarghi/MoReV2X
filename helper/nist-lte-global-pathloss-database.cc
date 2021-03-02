/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011,2012 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
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
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */


#include "nist-lte-global-pathloss-database.h"
#include "ns3/nist-lte-enb-net-device.h"
#include "ns3/nist-lte-ue-net-device.h"
#include "ns3/nist-lte-spectrum-phy.h"

#include <limits>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("NistLteGlobalPathlossDatabase");

NistLteGlobalPathlossDatabase::~NistLteGlobalPathlossDatabase (void)
{
}

void 
NistLteGlobalPathlossDatabase::Print ()
{
  NS_LOG_FUNCTION (this);
  for (std::map<uint16_t, std::map<uint64_t, double> >::const_iterator cellIdIt = m_pathlossMap.begin ();
       cellIdIt != m_pathlossMap.end ();
       ++cellIdIt)
    {
      for (std::map<uint64_t, double>::const_iterator imsiIt = cellIdIt->second.begin ();
           imsiIt != cellIdIt->second.end ();
           ++imsiIt)
        {
          std::cout << "CellId: " << cellIdIt->first << " IMSI: " << imsiIt->first << " pathloss: " << imsiIt->second << " dB" << std::endl;
        }
    }
}


double
NistLteGlobalPathlossDatabase::GetPathloss (uint16_t cellId, uint64_t imsi)
{
  NS_LOG_FUNCTION (this);
  std::map<uint16_t, std::map<uint64_t, double> >::iterator cellIt = m_pathlossMap.find (cellId);
  if (cellIt == m_pathlossMap.end())
    {
      return std::numeric_limits<double>::infinity ();
    }
  std::map<uint64_t, double>::iterator ueIt = cellIt->second.find (imsi);
  if (ueIt ==  cellIt->second.end())
    {
      return std::numeric_limits<double>::infinity ();
    }
  return ueIt->second;
}
 

void
DownlinkNistLteGlobalPathlossDatabase::UpdatePathloss (std::string context, 
                                        Ptr<SpectrumPhy> txPhy, 
                                        Ptr<SpectrumPhy> rxPhy, 
                                        double lossDb)
{
  NS_LOG_FUNCTION (this << lossDb);
  uint16_t cellId = txPhy->GetDevice ()->GetObject<NistLteEnbNetDevice> ()->GetCellId ();
  uint16_t imsi = rxPhy->GetDevice ()->GetObject<NistLteUeNetDevice> ()->GetImsi ();
  m_pathlossMap[cellId][imsi] = lossDb;
}


void
UplinkNistLteGlobalPathlossDatabase::UpdatePathloss (std::string context, 
                                        Ptr<SpectrumPhy> txPhy, 
                                        Ptr<SpectrumPhy> rxPhy, 
                                        double lossDb)
{
  NS_LOG_FUNCTION (this << lossDb);
  uint16_t imsi = txPhy->GetDevice ()->GetObject<NistLteUeNetDevice> ()->GetImsi ();
  uint16_t cellId = rxPhy->GetDevice ()->GetObject<NistLteEnbNetDevice> ()->GetCellId ();
  m_pathlossMap[cellId][imsi] = lossDb;
}



} // namespace ns3
