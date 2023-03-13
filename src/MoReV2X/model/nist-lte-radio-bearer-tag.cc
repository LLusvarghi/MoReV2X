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
 * Author: Marco Miozzo <marco.miozzo@cttc.es>
 * Modified by: NIST
 */


#include "nist-lte-radio-bearer-tag.h"
#include "ns3/tag.h"
#include "ns3/uinteger.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (NistLteRadioBearerTag);

TypeId
NistLteRadioBearerTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::NistLteRadioBearerTag")
    .SetParent<Tag> ()
    .AddConstructor<NistLteRadioBearerTag> ()
    .AddAttribute ("rnti", "The rnti that indicates the UE to which packet belongs",
                   UintegerValue (0),
                   MakeUintegerAccessor (&NistLteRadioBearerTag::GetRnti),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("lcid", "The id whithin the UE identifying the logical channel to which the packet belongs",
                   UintegerValue (0),
                   MakeUintegerAccessor (&NistLteRadioBearerTag::GetLcid),
                   MakeUintegerChecker<uint8_t> ())
  .AddAttribute ("srcL2Id", "For sidelink communication, the source L2 identifier",
                 UintegerValue (0),
                 MakeUintegerAccessor (&NistLteRadioBearerTag::GetSourceL2Id),
                 MakeUintegerChecker<uint32_t> ())
  .AddAttribute ("dstL2Id", "For sidelink communication, the destination L2 identifier",
                 UintegerValue (0),
                 MakeUintegerAccessor (&NistLteRadioBearerTag::GetDestinationL2Id),
                 MakeUintegerChecker<uint32_t> ())
  ;
  return tid;
}

TypeId
NistLteRadioBearerTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

NistLteRadioBearerTag::NistLteRadioBearerTag ()
  : m_rnti (0),
    m_lcid (0),
    m_layer (0),
    m_srcL2Id (0),
    m_dstL2Id (0)
{
}
NistLteRadioBearerTag::NistLteRadioBearerTag (uint16_t rnti, uint8_t lcid)
  : m_rnti (rnti),
    m_lcid (lcid),
    m_layer (0),
    m_srcL2Id (0),
    m_dstL2Id (0)
{
}
  
NistLteRadioBearerTag::NistLteRadioBearerTag (uint16_t rnti, uint8_t lcid, uint32_t srcL2Id, uint32_t dstL2Id)
  : m_rnti (rnti),
    m_lcid (lcid),
    m_layer (0),
    m_srcL2Id (srcL2Id),
    m_dstL2Id (dstL2Id)
{
}
  
NistLteRadioBearerTag::NistLteRadioBearerTag (uint16_t rnti, uint8_t lcid, uint8_t layer)
  : m_rnti (rnti),
    m_lcid (lcid),
    m_layer (layer),
    m_srcL2Id (0),
    m_dstL2Id (0)
{
}

void
NistLteRadioBearerTag::SetRnti (uint16_t rnti)
{
  m_rnti = rnti;
}

void
NistLteRadioBearerTag::SetLcid (uint8_t lcid)
{
  m_lcid = lcid;
}

void
NistLteRadioBearerTag::SetLayer (uint8_t layer)
{
  m_layer = layer;
}

void
NistLteRadioBearerTag::SetSourceL2Id (uint32_t src)
{
  m_srcL2Id = src;
}
  
void
NistLteRadioBearerTag::SetDestinationL2Id (uint32_t dst)
{
  m_dstL2Id = dst;
}
  
  
uint32_t
NistLteRadioBearerTag::GetSerializedSize (void) const
{
  return 12;
}

void
NistLteRadioBearerTag::Serialize (TagBuffer i) const
{
  i.WriteU16 (m_rnti);
  i.WriteU8 (m_lcid);
  i.WriteU8 (m_layer);
  i.WriteU32 (m_srcL2Id);
  i.WriteU32 (m_dstL2Id);
}

void
NistLteRadioBearerTag::Deserialize (TagBuffer i)
{
  m_rnti = (uint16_t) i.ReadU16 ();
  m_lcid = (uint8_t) i.ReadU8 ();
  m_layer = (uint8_t) i.ReadU8 ();
  m_srcL2Id = (uint32_t) i.ReadU32 ();
  m_dstL2Id = (uint32_t) i.ReadU32 ();
}

uint16_t
NistLteRadioBearerTag::GetRnti () const
{
  return m_rnti;
}

uint8_t
NistLteRadioBearerTag::GetLcid () const
{
  return m_lcid;
}

uint8_t
NistLteRadioBearerTag::GetLayer () const
{
  return m_layer;
}
  
uint32_t
NistLteRadioBearerTag::GetSourceL2Id () const
{
  return m_srcL2Id;
}

uint32_t
NistLteRadioBearerTag::GetDestinationL2Id () const
{
  return m_dstL2Id;
}

void
NistLteRadioBearerTag::Print (std::ostream &os) const
{
  if (m_srcL2Id == 0) {
    os << "rnti=" << m_rnti << ", lcid=" << (uint16_t) m_lcid << ", layer=" << (uint16_t)m_layer;
  } else {
    os << "rnti=" << m_rnti << ", lcid=" << (uint16_t) m_lcid << ", srcL2Id=" << m_srcL2Id << ", dstL2Id=" << m_dstL2Id << ", layer=" << (uint16_t)m_layer;
  }
}

} // namespace ns3
