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


#include "nr-v2x-utils.h"
#include <ns3/object-factory.h>
#include <ns3/log.h>
#include <cfloat>
#include <cmath>
#include <ns3/simulator.h>
#include <ns3/double.h>
#include "nist-sl-pool.h"
#include "nist-lte-common.h"
#include <map>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("NrV2XUtils");

uint32_t
SubtractFrames (uint16_t frameAhead, uint16_t frame, uint16_t subframeAhead, uint16_t subframe)
{
  int diff;
  NS_ASSERT_MSG((frameAhead <= 1024) && (frame <= 1024), "Frame number must be smaller than 1024");
  NS_ASSERT_MSG((subframeAhead <= 10) && (subframe <= 10), "Subframe number must be smaller than 10");
//  NS_LOG_UNCOND("SF_ahead(" << frameAhead << "," << subframeAhead << ") SF(" << frame << "," << subframe << ")");
  if (frameAhead == frame) 
  {
    NS_ASSERT_MSG(subframeAhead >= subframe, "The ahead frame is not actually ahead");
    diff = (frameAhead*10 + subframeAhead) - (frame*10 + subframe);
  }
  else if (frameAhead < frame)
  {
    frameAhead = frameAhead + 1024;
    diff = (frameAhead*10 + subframeAhead) - (frame*10 + subframe);
  } 
  else
  {
    diff = (frameAhead*10 + subframeAhead) - (frame*10 + subframe);
  }
//  NS_LOG_UNCOND("Difference is " << diff);
  NS_ASSERT_MSG(diff >= 0, "SubtractFrames must return a non-negative value");
  // The returned difference is expressed in terms of "slots", not milliseconds
  return (uint32_t) diff;

}


SidelinkCommResourcePool::SubframeInfo
SimulatorTimeToSubframe (Time time, double slotDuration)
{
//   uint64_t milliseconds = time.GetSeconds () * 1000 + 15;
//   uint64_t milliseconds = time.GetMilliSeconds () + 15/slotDuration;
   uint64_t milliseconds;
   uint64_t microseconds = time.GetMicroSeconds () + 11000*slotDuration + UL_PUSCH_TTIS_DELAY*slotDuration*1000;
//   NS_LOG_DEBUG("MilliSeconds: " << milliseconds << " MicroSeconds: " << microseconds);
   milliseconds = microseconds / (1000*slotDuration);

   SidelinkCommResourcePool::SubframeInfo SF;
   SF.subframeNo = (uint32_t) (milliseconds % 10);
   SF.frameNo = (uint32_t) ((milliseconds / 10) % 1024);
//   SF.subframeNo = (uint32_t) (microseconds % 10);
//   SF.frameNo = (uint32_t) ((microseconds / 10) % 1024);
   if (SF.subframeNo == 0)
   {
     SF.subframeNo = 10;
     if (SF.frameNo == 0)
       SF.frameNo = 1023;
     else
       SF.frameNo--;
   }
   if (SF.frameNo == 0)
     SF.frameNo = 1024;  
   return SF;
}


/*
* Maximum transmission bandwidth in RBs, taken from 3GPP TS 38.101
* Table 5.3.2-1
*/

uint32_t
GetRbsFromBW (uint16_t SCS, uint32_t BW)
{
   std::map <uint16_t, std::map<uint16_t, uint16_t> > BWtoRBs;
   std::map<uint16_t, uint16_t> tmp;
   
   tmp = { {15, 25}, {30, 11}, {60, 0} };
   BWtoRBs.insert(std::pair <uint16_t, std::map<uint16_t, uint16_t> > (5, tmp));

   tmp = { {15, 52}, {30, 24}, {60, 11} };
   BWtoRBs.insert(std::pair <uint16_t, std::map<uint16_t, uint16_t> > (10, tmp));

   tmp = { {15, 79}, {30, 38}, {60, 18} };
   BWtoRBs.insert(std::pair <uint16_t, std::map<uint16_t, uint16_t> > (15, tmp));

   tmp = { {15, 106}, {30, 51}, {60, 24} };
   BWtoRBs.insert(std::pair <uint16_t, std::map<uint16_t, uint16_t> > (20, tmp));

   tmp = { {15, 133}, {30, 65}, {60, 31} };
   BWtoRBs.insert(std::pair <uint16_t, std::map<uint16_t, uint16_t> > (25, tmp));

   tmp = { {15, 160}, {30, 78}, {60, 38} };
   BWtoRBs.insert(std::pair <uint16_t, std::map<uint16_t, uint16_t> > (30, tmp));

   tmp = { {15, 216}, {30, 106}, {60, 51} };
   BWtoRBs.insert(std::pair <uint16_t, std::map<uint16_t, uint16_t> > (40, tmp));

   tmp = { {15, 270}, {30, 133}, {60, 65} };
   BWtoRBs.insert(std::pair <uint16_t, std::map<uint16_t, uint16_t> > (50, tmp));

   tmp = { {15, 0}, {30, 162}, {60, 79} };
   BWtoRBs.insert(std::pair <uint16_t, std::map<uint16_t, uint16_t> > (60, tmp));

   tmp = { {15, 0}, {30, 189}, {60, 93} };
   BWtoRBs.insert(std::pair <uint16_t, std::map<uint16_t, uint16_t> > (70, tmp));

   tmp = { {15, 0}, {30, 217}, {60, 107} };
   BWtoRBs.insert(std::pair <uint16_t, std::map<uint16_t, uint16_t> > (80, tmp));

   tmp = { {15, 0}, {30, 245}, {60, 121} };
   BWtoRBs.insert(std::pair <uint16_t, std::map<uint16_t, uint16_t> > (90, tmp));

   tmp = { {15, 0}, {30, 273}, {60, 135} };
   BWtoRBs.insert(std::pair <uint16_t, std::map<uint16_t, uint16_t> > (100, tmp));

   //Print the table
  /* for (std::map <uint16_t, std::map<uint16_t, uint16_t> >::iterator IT = BWtoRBs.begin(); IT != BWtoRBs.end(); IT++)
   {
    NS_LOG_UNCOND("BW = " << IT->first << " MHz");
    for (std::map<uint16_t, uint16_t>::iterator innerIT = IT->second.begin(); innerIT != IT->second.end(); innerIT++)
      NS_LOG_UNCOND("SCS = " << innerIT->first << " kHz. Nrb = " << innerIT->second);
   }*/

   return BWtoRBs[BW][SCS];

}


double
GetRefSensitivity (uint16_t SCS, uint32_t BW)
{
   std::map <uint16_t, std::map<uint16_t, double> > RefSens;
   std::map<uint16_t, double> tmp;

   NS_ASSERT_MSG((BW >= 10) && (BW <= 40), "Bandwidth must be between 10 and 40 MHz");

   tmp = { {15, -92.5}, {30, -92.1}, {60, -92.9} };
   RefSens.insert(std::pair <uint16_t, std::map<uint16_t, double> > (10, tmp));

   tmp = { {15, -89.2}, {30, -89.4}, {60, -89.1} };
   RefSens.insert(std::pair <uint16_t, std::map<uint16_t, double> > (20, tmp));

   tmp = { {15, -87.4}, {30, -87.7}, {60, -87.9} };
   RefSens.insert(std::pair <uint16_t, std::map<uint16_t, double> > (30, tmp));

   tmp = { {15, -86.1}, {30, -86.2}, {60, -86.4} };
   RefSens.insert(std::pair <uint16_t, std::map<uint16_t, double> > (40, tmp));

   //Print the table
   for (std::map <uint16_t, std::map<uint16_t, double> >::iterator IT = RefSens.begin(); IT != RefSens.end(); IT++)
   {
    NS_LOG_UNCOND("BW = " << IT->first << " MHz");
    for (std::map<uint16_t, double>::iterator innerIT = IT->second.begin(); innerIT != IT->second.end(); innerIT++)
      NS_LOG_UNCOND("SCS = " << innerIT->first << " kHz. Sensitivity = " << innerIT->second);
   }

   return RefSens[BW][SCS];

}



} //namespace ns3
#endif /* NR_V2X_UTILS_H */
