/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
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
 * Author: Luca Lusvarghi <luca.lusvarghi5@unimore.it>
 *
 */

#ifndef NR_V2X_UTILS_H
#define NR_V2X_UTILS_H

#include <ns3/nstime.h>
#include "nist-sl-pool.h"

namespace ns3 {

uint32_t SubtractFrames (uint16_t frameAhead, uint16_t frame, uint16_t subframeAhead, uint16_t subframe);

SidelinkCommResourcePool::SubframeInfo SimulatorTimeToSubframe (Time time, double slotDuration);

uint32_t EvaluateSlotsDifference(SidelinkCommResourcePool::SubframeInfo SF1, SidelinkCommResourcePool::SubframeInfo SF2, uint32_t maxDifference);

/**
* Method to count the residual CSRs
*/

uint32_t ComputeResidualCSRs (std::map<uint16_t,std::list<SidelinkCommResourcePool::SubframeInfo> > L1);

uint16_t GetTproc0 (uint16_t numerologyIndex);

uint16_t GetTproc1 (uint16_t numerologyIndex);

uint32_t GetCresel (double RRI);

uint32_t GetRbsFromBW (uint16_t SCS, uint32_t BW);

double GetRefSensitivity (uint16_t SCS, uint32_t BW);

double GetRSSIthreshold (double RXsensitivity);

double GetGeoCellSize (uint32_t SubChannelSize, uint16_t BW_RBs, double ReuseDistance, uint32_t *NumberGeoCells);

} //namespace ns3
#endif /* NR_V2X_UTILS_H */
