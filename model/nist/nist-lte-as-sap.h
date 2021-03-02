/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2012 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
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

#ifndef NIST_LTE_AS_SAP_H
#define NIST_LTE_AS_SAP_H

#include <stdint.h>
#include <ns3/ptr.h>
#include <ns3/packet.h>

namespace ns3 {

class NistLteEnbNetDevice;

/**
 * This class implements the Access Stratum (AS) Service Access Point
 * (SAP), i.e., the interface between the NistEpcUeNas and the NistLteUeRrc.
 * In particular, this class implements the
 * Provider part of the SAP, i.e., the methods exported by the
 * NistLteUeRrc and called by the NistEpcUeNas.
 * 
 */
class NistLteAsSapProvider
{
public:
  virtual ~NistLteAsSapProvider ();

  /**
   * \brief Set the selected Closed Subscriber Group subscription list to be
   *        used for cell selection.
   *
   * \param csgId identity of the subscribed CSG
   */
  virtual void SetCsgWhiteList (uint32_t csgId) = 0;

  /**
   * \brief Initiate Idle mode cell selection procedure.
   *
   * \param dlEarfcn the downlink carrier frequency (EARFCN)
   */
  virtual void StartCellSelection (uint16_t dlEarfcn) = 0;

  /** 
   * \brief Force the RRC entity to stay camped on a certain eNodeB.
   *
   * \param cellId the cell ID identifying the eNodeB
   * \param dlEarfcn the downlink carrier frequency (EARFCN)
   */
  virtual void ForceCampedOnEnb (uint16_t cellId, uint16_t dlEarfcn) = 0;

  /**
   * \brief Tell the RRC entity to enter Connected mode.
   *
   * If this function is called when the UE is in a situation where connecting
   * is not possible (e.g. before the simulation begin), then the UE will
   * attempt to connect at the earliest possible time (e.g. after it camps to a
   * suitable cell).
   */
  virtual void Connect (void) = 0;

  /** 
   * \brief Send a data packet.
   *
   * \param packet the packet
   * \param bid the EPS bearer ID
   */
  virtual void SendData (Ptr<Packet> packet, uint8_t bid) = 0;

  /** 
   * \brief Send a data packet for sidelink.
   *
   * \param packet the packet
   * \param group The L2 group address
   */
  virtual void SendData (Ptr<Packet> packet, uint32_t group) = 0;


  /** 
   * \brief Tell the RRC entity to release the connection.
   *
   */
  virtual void Disconnect () = 0;

  /**
   * \brief Tell the RRC to activate Sidelink Bearer 
   *
   * \param group The L2 address of interest
   * \param tx Indicates if the interest is to transmit
   * \param rx Indicates if the interest is to receive
   */
  virtual void ActivateSidelinkRadioBearer (uint32_t group, bool tx, bool rx) = 0;

  /**
   * \brief Tell the RRC to tear down Sidelink Bearer 
   *
   * \param group The L2 address of the group the UE is no longer interested
   */
  virtual void DeactivateSidelinkRadioBearer (uint32_t group) = 0;
  
  /**
   * tell RRC to add discovery applications
   * \param apps applications to be added
   * \param rxtx 0 for monitoring and 1 for announcing
   */
  virtual void AddDiscoveryApps (std::list<uint32_t> apps, bool rxtx) = 0;
  
  /**
   * tell RRC to remove discovery applications
   * \param apps applications to be removed
   * \param rxtx 0 for monitoring and 1 for announcing
   */
  virtual void RemoveDiscoveryApps (std::list<uint32_t> apps, bool rxtx) = 0;
  
};


/**
 * This class implements the Access Stratum (AS) Service Access Point
 * (SAP), i.e., the interface between the NistEpcUeNas and the NistLteUeRrc
 * In particular, this class implements the 
 * User part of the SAP, i.e., the methods exported by the
 * NistEpcUeNas and called by the NistLteUeRrc.
 * 
 */
class NistLteAsSapUser
{
public:
  virtual ~NistLteAsSapUser ();

  /** 
   * \brief Notify the NAS that RRC Connection Establishment was successful.
   * 
   */
  virtual void NotifyConnectionSuccessful () = 0;

  /** 
   * \brief Notify the NAS that RRC Connection Establishment failed.
   * 
   */
  virtual void NotifyConnectionFailed () = 0;


  /** 
   * Notify the NAS that RRC Connection was released
   * 
   */
  virtual void NotifyConnectionReleased () = 0;

  /** 
   * receive a data packet
   * 
   * \param packet the packet
   */
  virtual void RecvData (Ptr<Packet> packet) = 0;

  /**
   * Notify the NAS that the sidelink has been setup
   *
   * \param group The group that was setup
   */
  virtual void NotifySidelinkRadioBearerActivated (uint32_t group) = 0;
};




/**
 * Template for the implementation of the NistLteAsSapProvider as a member
 * of an owner class of type C to which all methods are forwarded
 * 
 */
template <class C>
class NistMemberLteAsSapProvider : public NistLteAsSapProvider
{
public:
  NistMemberLteAsSapProvider (C* owner);

  // inherited from NistLteAsSapProvider
  virtual void SetCsgWhiteList (uint32_t csgId);
  virtual void StartCellSelection (uint16_t dlEarfcn);
  virtual void ForceCampedOnEnb (uint16_t cellId, uint16_t dlEarfcn);
  virtual void Connect (void);
  virtual void SendData (Ptr<Packet> packet, uint8_t bid);
  virtual void SendData (Ptr<Packet> packet, uint32_t group);
  virtual void Disconnect ();
  //communication
  virtual void ActivateSidelinkRadioBearer (uint32_t group, bool tx, bool rx);
  virtual void DeactivateSidelinkRadioBearer (uint32_t group);
  //Discovery
  virtual void AddDiscoveryApps (std::list<uint32_t> apps, bool rxtx);
  virtual void RemoveDiscoveryApps (std::list<uint32_t> apps, bool rxtx);

private:
  NistMemberLteAsSapProvider ();
  C* m_owner;
};

template <class C>
NistMemberLteAsSapProvider<C>::NistMemberLteAsSapProvider (C* owner)
  : m_owner (owner)
{
}

template <class C>
NistMemberLteAsSapProvider<C>::NistMemberLteAsSapProvider ()
{
}

template <class C>
void
NistMemberLteAsSapProvider<C>::SetCsgWhiteList (uint32_t csgId)
{
  m_owner->DoSetCsgWhiteList (csgId);
}

template <class C>
void
NistMemberLteAsSapProvider<C>::StartCellSelection (uint16_t dlEarfcn)
{
  m_owner->DoStartCellSelection (dlEarfcn);
}

template <class C>
void
NistMemberLteAsSapProvider<C>::ForceCampedOnEnb (uint16_t cellId, uint16_t dlEarfcn)
{
  m_owner->DoForceCampedOnEnb (cellId, dlEarfcn);
}

template <class C>
void 
NistMemberLteAsSapProvider<C>::Connect ()
{
  m_owner->DoConnect ();
}

template <class C>
void
NistMemberLteAsSapProvider<C>::SendData (Ptr<Packet> packet, uint8_t bid)
{
  m_owner->DoSendData (packet, bid);
}

template <class C>
void
NistMemberLteAsSapProvider<C>::SendData (Ptr<Packet> packet, uint32_t group)
{
  m_owner->DoSendData (packet, group);
}

template <class C>
void 
NistMemberLteAsSapProvider<C>::Disconnect ()
{
  m_owner->DoDisconnect ();
}

template <class C>
void
NistMemberLteAsSapProvider<C>::ActivateSidelinkRadioBearer (uint32_t group, bool tx, bool rx)
{
  m_owner->DoActivateSidelinkRadioBearer (group, tx, rx);
}

template <class C>
void
NistMemberLteAsSapProvider<C>::DeactivateSidelinkRadioBearer (uint32_t group)
{
  m_owner->DoDeactivateSidelinkRadioBearer (group);
}

template <class C>
void
NistMemberLteAsSapProvider<C>::AddDiscoveryApps (std::list<uint32_t> apps, bool rxtx)
{
  m_owner->DoAddDiscoveryApps (apps, rxtx);
}

template <class C>
void
NistMemberLteAsSapProvider<C>::RemoveDiscoveryApps (std::list<uint32_t> apps, bool rxtx)
{
  m_owner->DoRemoveDiscoveryApps (apps, rxtx);
}

/**
 * Template for the implementation of the NistLteAsSapUser as a member
 * of an owner class of type C to which all methods are forwarded
 * 
 */
template <class C>
class NistMemberLteAsSapUser : public NistLteAsSapUser
{
public:
  NistMemberLteAsSapUser (C* owner);

  // inherited from NistLteAsSapUser
  virtual void NotifyConnectionSuccessful ();
  virtual void NotifyConnectionFailed ();
  virtual void RecvData (Ptr<Packet> packet);
  virtual void NotifyConnectionReleased ();
  virtual void NotifySidelinkRadioBearerActivated (uint32_t group);

private:
  NistMemberLteAsSapUser ();
  C* m_owner;
};

template <class C>
NistMemberLteAsSapUser<C>::NistMemberLteAsSapUser (C* owner)
  : m_owner (owner)
{
}

template <class C>
NistMemberLteAsSapUser<C>::NistMemberLteAsSapUser ()
{
}

template <class C>
void 
NistMemberLteAsSapUser<C>::NotifyConnectionSuccessful ()
{
  m_owner->DoNotifyConnectionSuccessful ();
}

template <class C>
void 
NistMemberLteAsSapUser<C>::NotifyConnectionFailed ()
{
  m_owner->DoNotifyConnectionFailed ();
}

template <class C>
void 
NistMemberLteAsSapUser<C>::RecvData (Ptr<Packet> packet)
{
  m_owner->DoRecvData (packet);
}

template <class C>
void 
NistMemberLteAsSapUser<C>::NotifyConnectionReleased ()
{
  m_owner->DoNotifyConnectionReleased ();
}

template <class C>
void 
NistMemberLteAsSapUser<C>::NotifySidelinkRadioBearerActivated (uint32_t group)
{
  m_owner->DoNotifySidelinkRadioBearerActivated (group);
}

} // namespace ns3

#endif // NIST_LTE_AS_SAP_H
