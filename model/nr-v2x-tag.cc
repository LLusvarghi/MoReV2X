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
 * Author: Lorenzo Gibellini, <lorenzo.gibellini@gmail.com> 
 * Author: Luca Lusvarghi <luca.lusvarghi5@unimore.it>
 * University of Modena and Reggio Emilia 
 * 
 */

#include "nr-v2x-tag.h"
#include "ns3/tag.h"
#include "ns3/uinteger.h"

#include <fstream>
#include <iostream>
namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (NrV2xTag);

TypeId 
NrV2xTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::NrV2xTag")
    .SetParent<Tag> ()
    .AddConstructor<NrV2xTag> ()
    .AddAttribute ("SimpleValue",
                   "A simple value",
                   EmptyAttributeValue (),
                   MakeUintegerAccessor (&NrV2xTag::GetSimpleValue),
                   MakeUintegerChecker<uint8_t> ())
  ;
  return tid;
}
TypeId 
NrV2xTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}
uint32_t 
NrV2xTag::GetSerializedSize (void) const
{
   return sizeof m_doubleValue + sizeof m_intValue + sizeof m_simpleValue + sizeof m_genTime + sizeof m_packetId + sizeof m_genPosX + sizeof m_genPosY + sizeof m_nodeId + sizeof m_numHops + sizeof m_messageType + sizeof m_trafficType + sizeof m_Cresel + sizeof m_PacketSize + sizeof m_ReservationSize + sizeof m_pppp + sizeof m_pdb + sizeof m_p_rsvp + sizeof m_mcs;
}

void 
NrV2xTag::Serialize (TagBuffer i) const
{
  i.WriteU8 (m_simpleValue);
  i.WriteU64 (m_intValue);
  i.WriteDouble(m_doubleValue);

  i.WriteDouble(m_genTime);
  i.WriteDouble(m_genPosX);
  i.WriteDouble(m_genPosY);
  i.WriteU64 (m_packetId);
  i.WriteU32 (m_nodeId);
  i.WriteU32 (m_numHops);

  i.WriteU8 (m_messageType);
  i.WriteU8 (m_trafficType);
  i.WriteU16 (m_Cresel);
  i.WriteU16 (m_PacketSize);
  i.WriteU16 (m_ReservationSize);

  i.WriteU8 (m_pppp);
  i.WriteU32 (m_pdb);
  i.WriteU32 (m_p_rsvp);
  i.WriteU32 (m_mcs);

}
void 
NrV2xTag::Deserialize (TagBuffer i)
{
  m_simpleValue = i.ReadU8 ();
  m_intValue = i.ReadU64 ();
  m_doubleValue = i.ReadDouble();

  m_genTime = i.ReadDouble();
  m_genPosX = i.ReadDouble();
  m_genPosY = i.ReadDouble();
  m_packetId = i.ReadU64();
  m_nodeId = i.ReadU32();
  m_numHops = i.ReadU32();
  m_messageType = i.ReadU8();
  m_trafficType = i.ReadU8();
  m_Cresel = i.ReadU16();
  m_PacketSize = i.ReadU16();
  m_ReservationSize = i.ReadU16();
  m_pppp = i.ReadU8();
  m_pdb = i.ReadU32 ();
  m_p_rsvp = i.ReadU32 ();
  m_mcs = i.ReadU32 ();
  
  
}
void 
NrV2xTag::Print (std::ostream &os) const
{
  os << "int64 = " << (uint64_t)m_intValue << "\nint8 = " << (uint8_t)m_simpleValue;

  std::ofstream outfile;
  outfile.open("tagFile.txt", std::ios_base::app);
  outfile << "int64 = " << (uint64_t)m_intValue << "\nint8 = " << (uint8_t)m_simpleValue;
  outfile.close();
}

void
NrV2xTag::SetSimpleValue (uint8_t value)
{
  m_simpleValue = value;
}


void
NrV2xTag::SetIntValue (uint64_t value)
{
  m_intValue = value;
}

void
NrV2xTag::SetDoubleValue (double value)
{
  m_doubleValue = value;
}

void
NrV2xTag::SetGenTime (double value)
{
  m_genTime = value;
}

void
NrV2xTag::SetGenPosX (double value)
{
  m_genPosX = value;
}

void
NrV2xTag::SetGenPosY (double value)
{
  m_genPosY = value;
}

void
NrV2xTag::SetPacketId (uint64_t value)
{
  m_packetId = value;
}

void
NrV2xTag::SetNodeId (uint32_t value)
{
  m_nodeId = value;
}

void
NrV2xTag::SetNumHops (uint32_t value)
{
  m_numHops = value;
}

void
NrV2xTag::SetMessageType (uint8_t value)
{
  m_messageType = value;
}

void
NrV2xTag::SetTrafficType (uint8_t value)
{
  m_trafficType = value;
}

void
NrV2xTag::SetReselectionCounter (uint16_t value)
{
  m_Cresel = value;
}

void
NrV2xTag::SetPacketSize (uint16_t value)
{
  m_PacketSize = value;
}

void
NrV2xTag::SetReservationSize (uint16_t value)
{
  m_ReservationSize = value;
}

void
NrV2xTag::SetPPPP (uint8_t value)
{
  m_pppp = value;
}

void
NrV2xTag::SetPdb (uint32_t value)
{
  m_pdb = value;
}

void
NrV2xTag::SetPrsvp (uint32_t value)
{
  m_p_rsvp = value;
}

void
NrV2xTag::SetMcs (uint32_t value)
{
  m_mcs = value;
}

//Getters
uint8_t 
NrV2xTag::GetSimpleValue (void) const
{
  return m_simpleValue;
}

uint64_t 
NrV2xTag::GetIntValue (void) const
{
  return m_intValue;
}

double
NrV2xTag::GetDoubleValue (void) const
{
  return m_doubleValue;
}

double
NrV2xTag::GetGenTime (void) const
{
  return m_genTime;
}

double
NrV2xTag::GetGenPosX (void) const
{
  return m_genPosX;
}

double
NrV2xTag::GetGenPosY (void) const
{
  return m_genPosY;
}

uint64_t
NrV2xTag::GetPacketId (void) const
{
  return m_packetId;
}

uint32_t
NrV2xTag::GetNodeId (void) const
{
  return m_nodeId;
}

uint32_t
NrV2xTag::GetNumHops (void) const
{
  return m_numHops;
}

uint8_t 
NrV2xTag::GetMessageType (void) const
{
  return m_messageType;
}

uint8_t 
NrV2xTag::GetTrafficType (void) const
{
  return m_trafficType;
}

uint16_t 
NrV2xTag::GetReselectionCounter (void) const
{
  return m_Cresel;
}

uint16_t 
NrV2xTag::GetPacketSize (void) const
{
  return m_PacketSize;
}

uint16_t
NrV2xTag::GetReservationSize (void) const
{
  return m_ReservationSize;
}

uint8_t 
NrV2xTag::GetPPPP (void) const
{
  return m_pppp;
}

uint32_t 
NrV2xTag::GetPdb (void) const
{
  return m_pdb;
}

uint32_t 
NrV2xTag::GetPrsvp (void) const
{
  return m_p_rsvp;
}

uint32_t 
NrV2xTag::GetMcs (void) const
{
  return m_mcs;
}

}// namespace ns3
