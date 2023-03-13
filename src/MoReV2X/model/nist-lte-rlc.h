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

#ifndef NIST_LTE_RLC_H
#define NIST_LTE_RLC_H

#include <ns3/simple-ref-count.h>
#include <ns3/packet.h>
#include "ns3/uinteger.h"
#include "ns3/traced-value.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/nstime.h"

#include "ns3/object.h"

#include "ns3/nist-lte-rlc-sap.h"
#include "ns3/nist-lte-mac-sap.h"

namespace ns3 {


// class NistLteRlcSapProvider;
// class NistLteRlcSapUser;
// 
// class NistLteMacSapProvider;
// class NistLteMacSapUser;

/**
 * This abstract base class defines the API to interact with the Radio Link Control
 * (LTE_RLC) in LTE, see 3GPP TS 36.322
 *
 */
class NistLteRlc : public Object // SimpleRefCount<NistLteRlc>
{
  friend class NistLteRlcSpecificLteMacSapUser;
  friend class NistLteRlcSpecificLteRlcSapProvider<NistLteRlc>;
public:
  NistLteRlc ();
  virtual ~NistLteRlc ();
  static TypeId GetTypeId (void);
  virtual void DoDispose ();

  /**
   *
   *
   * \param rnti
   */
  void SetRnti (uint16_t rnti);

  /**
   *
   *
   * \param lcId
   */
  void SetLcId (uint8_t lcId);

  /**
   * Sets the source L2 Id for sidelink identification of the PDCP entity
   * \param src
   */
  void SetSourceL2Id (uint32_t src);
  
  /**
   * Sets the destination L2 Id for sidelink identification of the PDCP entity
   * \param src
   */
  void SetDestinationL2Id (uint32_t dst);
  
  /**
   *
   *
   * \param s the RLC SAP user to be used by this LTE_RLC
   */
  void SetNistLteRlcSapUser (NistLteRlcSapUser * s);

  /**
   *
   *
   * \return the RLC SAP Provider interface offered to the PDCP by this LTE_RLC
   */
  NistLteRlcSapProvider* GetNistLteRlcSapProvider ();

  /**
   *
   *
   * \param s the MAC SAP Provider to be used by this LTE_RLC
   */
  void SetNistLteMacSapProvider (NistLteMacSapProvider * s);

  /**
   *
   *
   * \return the MAC SAP User interface offered to the MAC by this LTE_RLC
   */
  NistLteMacSapUser* GetNistLteMacSapUser ();


  /**
   * TracedCallback signature for NotifyTxOpportunity events.
   *
   * \param [in] rnti C-RNTI scheduled.
   * \param [in] lcid The logical channel id corresponding to
   *             the sending RLC instance.
   * \param [in] bytes The number of bytes to transmit
   */
  typedef void (* NotifyTxTracedCallback)
    (const uint16_t rnti, const uint8_t lcid, const uint32_t bytes);

  /**
   * TracedCallback signature for
   *
   * \param [in] rnti C-RNTI scheduled.
   * \param [in] lcid The logical channel id corresponding to
   *             the sending RLC instance.
   * \param [in] bytes The packet size.
   * \param [in] delay Delay since sender timestamp, in ns.
   */
  typedef void (* ReceiveTracedCallback)
    (const uint16_t rnti, const uint8_t lcid,
     const uint32_t bytes, const uint64_t delay);

  /// \todo MRE What is the sense to duplicate all the interfaces here???
  // NB to avoid the use of multiple inheritance
  
protected:
  // Interface forwarded by NistLteRlcSapProvider
  virtual void DoTransmitPdcpPdu (Ptr<Packet> p) = 0;

  NistLteRlcSapUser* m_rlcSapUser;
  NistLteRlcSapProvider* m_rlcSapProvider;

  // Interface forwarded by NistLteMacSapUser
  virtual void DoNotifyTxOpportunity (uint32_t bytes, uint8_t layer, uint8_t harqId) = 0;
  virtual void DoNotifyHarqDeliveryFailure () = 0;
  virtual void DoReceivePdu (Ptr<Packet> p) = 0;

  NistLteMacSapUser* m_macSapUser;
  NistLteMacSapProvider* m_macSapProvider;

  uint16_t m_rnti;
  uint8_t m_lcid;
  /* Additional identifier for sidelink */
  uint32_t m_srcL2Id;  /**< Source L2 ID (24 bits) */
  uint32_t m_dstL2Id;  /**< Destination L2 ID (24 bits) */

  /**
   * Used to inform of a PDU delivery to the MAC SAP provider
   */
  TracedCallback<uint16_t, uint8_t, uint32_t> m_txPdu;
  /**
   * Used to inform of a PDU reception from the MAC SAP user
   */
  TracedCallback<uint16_t, uint8_t, uint32_t, uint64_t> m_rxPdu;

};



/**
 * LTE_RLC Saturation Mode (SM): simulation-specific mode used for
 * experiments that do not need to consider the layers above the LTE_RLC.
 * The LTE_RLC SM, unlike the standard LTE_RLC modes, it does not provide
 * data delivery services to upper layers; rather, it just generates a
 * new LTE_RLC PDU whenever the MAC notifies a transmission opportunity.
 *
 */
class NistLteRlcSm : public NistLteRlc
{
public:
  NistLteRlcSm ();
  virtual ~NistLteRlcSm ();
  static TypeId GetTypeId (void);
  virtual void DoInitialize ();
  virtual void DoDispose ();

  virtual void DoTransmitPdcpPdu (Ptr<Packet> p);
  virtual void DoNotifyTxOpportunity (uint32_t bytes, uint8_t layer, uint8_t harqId);
  virtual void DoNotifyHarqDeliveryFailure ();
  virtual void DoReceivePdu (Ptr<Packet> p);



private:
  void ReportBufferNistStatus ();

};




// /**
//  * Implements LTE_RLC Transparent Mode (TM), see  3GPP TS 36.322
//  *
//  */
// class NistLteRlcTm : public NistLteRlc
// {
// public:
//   virtual ~NistLteRlcTm ();

// };


// /**
//  * Implements LTE_RLC Unacknowledged Mode (UM), see  3GPP TS 36.322
//  *
//  */
// class NistLteRlcUm : public NistLteRlc
// {
// public:
//   virtual ~NistLteRlcUm ();

// };

// /**
//  * Implements LTE_RLC Acknowledged Mode (AM), see  3GPP TS 36.322
//  *
//  */

// class NistLteRlcAm : public NistLteRlc
// {
// public:
//   virtual ~NistLteRlcAm ();
// };





} // namespace ns3

#endif // NIST_LTE_RLC_H
