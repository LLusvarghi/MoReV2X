/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * 2018 Dipartimento di Ingegneria 'Enzo Ferrari' (DIEF),
 *      Universita' degli Studi di Modena e Reggio Emilia (UniMoRe)
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
 * University of Modena and Reggio Emilia 
 * 
 */


#include "ns3/lte-node-state.h"
#include <cmath>
#include <fstream>
#include <iostream>

namespace ns3 {

LTENodeState::LTENodeState (void) // sets the initial (default state)
{
  m_vehicle = true;  // It's always a vehicle
  m_lastRcvPacketId = 0;
}


TypeId 
LTENodeState::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LTENodeState")
    .SetParent<Object> ()
    .AddConstructor<LTENodeState> ()
    /*.AddAttribute ("IsVehicle",
                   "Is the LTE node a vehicle UE?",
                   BooleanValue (true),
                   MakeBooleanAccessor (&MyTag::GetSimpleValue),
                   MakeBooleanChecker ())*/
  ;
  return tid;
}

TypeId 
LTENodeState::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void 
LTENodeState::AddNewReceivedPacket (uint32_t packetID)
{
  m_receivedPacketIds.push_back(packetID);
}

bool
LTENodeState::HasReceivedPacket (uint32_t packetID)
{
  bool received = false;
  for(std::vector<uint32_t>::iterator it = m_receivedPacketIds.begin(); it != m_receivedPacketIds.end(); ++it)
  {
     if(*it == packetID)
     {
        return true;
     }
  }
  
  return received;
}

// Getters
bool
LTENodeState::IsVehicle(void) const
{
  return true;  // It's always a vehicle in my case.
}

uint32_t
LTENodeState::GetLastRcvPacketId (void) const
{
  return m_lastRcvPacketId;
}

Ptr<Node>
LTENodeState::GetNode (void) const
{
  return m_node;
}

// Setters
void 
LTENodeState::SetLastRcvPacketId(uint32_t packetID)
{
  m_lastRcvPacketId = packetID;
}

void 
LTENodeState::SetNode(Ptr<Node> node)
{
  m_node = node;
}

} //namespace ns3
