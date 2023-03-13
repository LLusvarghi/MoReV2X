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
 * Author: Nicola Baldo <nbaldo@cttc.es>
 * Modified by: NIST
 */

#ifndef NIST_LTE_RADIO_BEARER_INFO_H
#define NIST_LTE_RADIO_BEARER_INFO_H

#include <ns3/object.h>
#include <ns3/pointer.h>
#include <ns3/nist-eps-bearer.h>
#include <ns3/nist-lte-rrc-sap.h>
#include <ns3/ipv4-address.h>

namespace ns3 {

class NistLteRlc;
class NistLtePdcp;

/**
 * store information on active radio bearer instance
 * 
 */
class NistLteRadioBearerInfo : public Object
{

public:
  NistLteRadioBearerInfo (void);
  virtual ~NistLteRadioBearerInfo (void);
  static TypeId GetTypeId (void);

  Ptr<NistLteRlc> m_rlc;
  Ptr<NistLtePdcp> m_pdcp;  
};


/**
 * store information on active signaling radio bearer instance
 * 
 */
class LteSignalingRadioNistBearerInfo : public NistLteRadioBearerInfo
{

public:
  static TypeId GetTypeId (void);

  uint8_t m_srbIdentity;   
  NistLteRrcSap::NistLogicalChannelConfig m_logicalChannelConfig;  
};


/**
 * store information on active data radio bearer instance
 * 
 */
class LteDataRadioNistBearerInfo : public NistLteRadioBearerInfo
{

public:
  static TypeId GetTypeId (void);

  NistEpsBearer m_epsBearer;
  uint8_t m_epsBearerIdentity;
  uint8_t m_drbIdentity;
  NistLteRrcSap::NistRlcConfig m_rlcConfig;
  uint8_t m_logicalChannelIdentity;
  NistLteRrcSap::NistLogicalChannelConfig m_logicalChannelConfig;
  uint32_t m_gtpTeid; /**< S1-bearer GTP tunnel endpoint identifier, see 36.423 9.2.1 */
  Ipv4Address m_transportLayerAddress; /**< IP Address of the SGW, see 36.423 9.2.1 */
};

/**
 * store information on active sidelink data radio bearer instance
 * 
 */
class LteSidelinkRadioBearerInfo : public NistLteRadioBearerInfo
{

public:
  static TypeId GetTypeId (void);

  //NistLteRrcSap::NistRlcConfig m_rlcConfig;
  uint8_t m_logicalChannelIdentity;
  NistLteRrcSap::NistLogicalChannelConfig m_logicalChannelConfig;
  uint32_t m_sourceL2Id;
  uint32_t m_destinationL2Id;
  //bool m_tx;
  //bool m_rx;
};




} // namespace ns3


#endif // NIST_LTE_RADIO_BEARER_INFO_H
