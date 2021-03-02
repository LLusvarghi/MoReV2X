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
 * Author: Marco Miozzo  <marco.miozzo@cttc.es>
 *         Nicola Baldo  <nbaldo@cttc.es>
 */
#ifndef NIST_LTE_PHY_TAG_H
#define NIST_LTE_PHY_TAG_H

#include "ns3/tag.h"

namespace ns3 {

/**
 * Tag used to define PHY parameters 
 */
class NistLtePhyTag : public Tag
{
public:
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;

  /**
   * Create an empty NistLtePhyTag
   */
  NistLtePhyTag ();

  /**
   * Create a NistLtePhyTag with the given RNTI and LC id
   */
  NistLtePhyTag (uint16_t cellId);


  virtual ~NistLtePhyTag ();

  virtual void Serialize (TagBuffer i) const;
  virtual void Deserialize (TagBuffer i);
  virtual uint32_t GetSerializedSize () const;
  virtual void Print (std::ostream &os) const;

  uint16_t GetCellId () const;

private:
  uint16_t m_cellId;

};



} // namespace ns3

#endif /* NIST_LTE_PHY_TAG_H */
