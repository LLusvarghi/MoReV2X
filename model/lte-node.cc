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


#include "ns3/lte-node.h"
#include <cmath>
#include <fstream>
#include <iostream>

namespace ns3 {

LTENode::LTENode (void)
{
  m_vehicle = true;
}

TypeId 
LTENode::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LTENode")
    .SetParent<Node> ()
    .AddConstructor<LTENode> ()
    /*.AddAttribute ("IsVehicle",
                   "Is the LTE node a vehicle UE?",
                   BooleanValue (true),
                   MakeBooleanAccessor (&MyTag::GetSimpleValue),
                   MakeBooleanChecker ())*/
  ;
  return tid;
}

TypeId 
LTENode::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

bool
LTENode::IsVehicle(void) const
{
  return m_vehicle;
}


} //namespace ns3
