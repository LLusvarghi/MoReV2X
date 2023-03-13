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
#include <ns3/random-variable-stream.h>


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("NrV2XUtils");

uint32_t
SubtractFrames (uint16_t frameAhead, uint16_t frame, uint16_t subframeAhead, uint16_t subframe)
{
  int diff;
//  NS_LOG_UNCOND("SF_ahead(" << frameAhead << "," << subframeAhead << ") SF(" << frame << "," << subframe << ")");
  NS_ASSERT_MSG((frameAhead <= 1024) && (frame <= 1024), "Frame number must be smaller than 1024");
  NS_ASSERT_MSG((subframeAhead <= 10) && (subframe <= 10), "Subframe number must be smaller than 10");
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


uint32_t EvaluateSlotsDifference(SidelinkCommResourcePool::SubframeInfo SF1, SidelinkCommResourcePool::SubframeInfo SF2, uint32_t maxDifference)
{
   uint32_t SlotsDiff;

   if ((uint32_t) abs((int)SF1.frameNo - (int)SF2.frameNo) > maxDifference)
   {
     if (SF1.frameNo > SF2.frameNo)
       SlotsDiff = abs((int)((SF1.frameNo)*10 + SF1.subframeNo) - (int)((SF2.frameNo+1024)*10 + SF2.subframeNo) );
     else
       SlotsDiff = abs((int)((SF1.frameNo+1024)*10 + SF1.subframeNo) - (int)((SF2.frameNo)*10 + SF2.subframeNo) );
   }
   else
     SlotsDiff = abs((int)((SF1.frameNo)*10 +  SF1.subframeNo) - (int)((SF2.frameNo)*10 + SF2.subframeNo) );

   return SlotsDiff;
}

uint32_t 
ComputeResidualCSRs (std::map<uint16_t,std::list<SidelinkCommResourcePool::SubframeInfo> > L1)
{
   uint32_t nCSR = 0;
   std::map<uint16_t,std::list<SidelinkCommResourcePool::SubframeInfo> >::iterator mapIt;
   for (mapIt = L1.begin (); mapIt != L1.end (); mapIt++)
      {
          nCSR += (*mapIt).second.size ();
      }  
   return nCSR;
}


uint16_t
GetTproc0 (uint16_t numerologyIndex)
{
   uint16_t T_proc_0; //Defined in slots
   switch (numerologyIndex)
   {
     case 0:
       T_proc_0 = 1;
       break;
     case 1:
       T_proc_0 = 1;
       break;
     case 2:
       T_proc_0 = 2;
       break;
     case 3:
       T_proc_0 = 4;
       break;
   }

   return T_proc_0;
}


uint16_t
GetTproc1 (uint16_t numerologyIndex)
{
   uint16_t T_proc_1; //Defined in slots
   switch (numerologyIndex)
   {
     case 0:
       T_proc_1 = 3;
       break;
     case 1:
       T_proc_1 = 5;
       break;
     case 2:
       T_proc_1 = 9;
       break;
     case 3:
       T_proc_1 = 17;
       break;
   }

   return T_proc_1;
}


uint32_t
GetCresel (double RRI)
{
   Ptr<UniformRandomVariable> uniformCresel = CreateObject<UniformRandomVariable> ();
   uint32_t Cresel;
   if (RRI >= 100)
   {
     Cresel =  uniformCresel ->  GetInteger (5,15); //Get a random integer in the interval [min, max]
     //V2XGrant.m_Cresel = 10;
   }      
   else
   {  //TODO check if it works
     uint16_t Cresel_bound;
     Cresel_bound = 100 / std::max(20, (int)RRI);
     Cresel = uniformCresel ->  GetInteger (5*Cresel_bound,15*Cresel_bound); //Get a random integer in the interval [min, max]
     NS_LOG_DEBUG("RRI " << RRI << ", bound: " << Cresel_bound << ", Cresel: " << Cresel);
   }       
   return Cresel;
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

/*
Reference sensitivity of NR-V2X bands
(Table 7.3E.2.1 from 3GPP TS 38.101-1)
*/

double
GetRefSensitivity (uint16_t SCS, uint32_t BW)
{
   std::map <uint16_t, std::map<uint16_t, double> > RefSens;
   std::map<uint16_t, double> tmp;
   
   // Extended from 40MHz to 50MHz in order to simulate the frequency-reuse scheme
   NS_ASSERT_MSG((BW >= 10) && (BW <= 50), "Bandwidth must be between 10 and 50 MHz");

   tmp = { {15, -92.5}, {30, -92.1}, {60, -92.9} };
   RefSens.insert(std::pair <uint16_t, std::map<uint16_t, double> > (10, tmp));

   tmp = { {15, -89.2}, {30, -89.4}, {60, -89.1} };
   RefSens.insert(std::pair <uint16_t, std::map<uint16_t, double> > (20, tmp));

   tmp = { {15, -87.4}, {30, -87.7}, {60, -87.9} };
   RefSens.insert(std::pair <uint16_t, std::map<uint16_t, double> > (30, tmp));

   tmp = { {15, -86.1}, {30, -86.2}, {60, -86.4} };
   RefSens.insert(std::pair <uint16_t, std::map<uint16_t, double> > (40, tmp));

   tmp = { {15, -86.1}, {30, -86.2}, {60, -86.4} };
   RefSens.insert(std::pair <uint16_t, std::map<uint16_t, double> > (50, tmp));   // Fake value added for the simulation of the frequency-reuse scheme

   //Print the table
   for (std::map <uint16_t, std::map<uint16_t, double> >::iterator IT = RefSens.begin(); IT != RefSens.end(); IT++)
   {
    NS_LOG_UNCOND("BW = " << IT->first << " MHz");
    for (std::map<uint16_t, double>::iterator innerIT = IT->second.begin(); innerIT != IT->second.end(); innerIT++)
      NS_LOG_UNCOND("SCS = " << innerIT->first << " kHz. Sensitivity = " << innerIT->second);
   }

   return RefSens[BW][SCS];

}


/* Return the value immediately larger than the reference sensitivity
see TS 38.331 "sl-ThreshS-RSSI-CBR" parameter */
double
GetRSSIthreshold (double RXsensitivity)
{
   double InitialValue = -112.0, FinalValue;
   for (int i = 0; i < 45; i++)
   {
     NS_LOG_DEBUG(RXsensitivity << ", " << InitialValue + i*2);
     if ((InitialValue + i*2) > RXsensitivity)
     {
       FinalValue = InitialValue + i*2;
       break;
     }
   }

   return FinalValue;
}


double
GetGeoCellSize (uint32_t SubChannelSize, uint16_t BW_RBs, double ReuseDistance, uint32_t *NumberGeoCells)
{
   double GeoCellSize; // in meters

   (*NumberGeoCells) = std::floor(BW_RBs / SubChannelSize);  // Number of geo-cells within a cluster

   GeoCellSize = ReuseDistance / (*NumberGeoCells);

   return GeoCellSize;
}


} //namespace ns3
#endif /* NR_V2X_UTILS_H */
