/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011, 2012 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
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
 * Author: Nicola Baldo <nbaldo@cttc.es>,
 *         Marco Miozzo <mmiozzo@cttc.es>
 * Modified by: NIST
 */

#ifndef NIST_LTE_UE_CPHY_SAP_H
#define NIST_LTE_UE_CPHY_SAP_H

#include <stdint.h>
#include <ns3/ptr.h>

#include <ns3/nist-lte-rrc-sap.h>
#include <ns3/nist-sl-pool.h>

namespace ns3 {


class NistLteEnbNetDevice;

/**
 * Service Access Point (SAP) offered by the UE PHY to the UE RRC for control purposes
 *
 * This is the PHY SAP Provider, i.e., the part of the SAP that contains
 * the PHY methods called by the MAC
 */
class NistLteUeCphySapProvider
{
public:

  /** 
   * destructor
   */
  virtual ~NistLteUeCphySapProvider ();

  /** 
   * reset the PHY
   * 
   */
  virtual void Reset () = 0;

  /**
   * \brief Tell the PHY entity to listen to PSS from surrounding cells and
   *        measure the RSRP.
   * \param dlEarfcn the downlink carrier frequency (EARFCN) to listen to
   *
   * This function will instruct this PHY instance to listen to the DL channel
   * over the bandwidth of 6 RB at the frequency associated with the given
   * EARFCN.
   *
   * After this, it will start receiving Primary Synchronization Signal (PSS)
   * and periodically returning measurement reports to RRC via
   * NistLteUeCphySapUser::ReportNistUeMeasurements function.
   */
  virtual void StartCellSearch (uint16_t dlEarfcn) = 0;

  /**
   * \brief Tell the PHY entity to synchronize with a given eNodeB over the
   *        currently active EARFCN for communication purposes.
   * \param cellId the ID of the eNodeB to synchronize with
   *
   * By synchronizing, the PHY will start receiving various information
   * transmitted by the eNodeB. For instance, when receiving system information,
   * the message will be relayed to RRC via
   * NistLteUeCphySapUser::RecvNistMasterInformationBlock and
   * NistLteUeCphySapUser::RecvNistSystemInformationBlockType1 functions.
   *
   * Initially, the PHY will be configured to listen to 6 RBs of BCH.
   * NistLteUeCphySapProvider::SetDlBandwidth can be called afterwards to increase
   * the bandwidth.
   */
  virtual void SynchronizeWithEnb (uint16_t cellId) = 0;

  /**
   * \brief Tell the PHY entity to align to the given EARFCN and synchronize
   *        with a given eNodeB for communication purposes.
   * \param cellId the ID of the eNodeB to synchronize with
   * \param dlEarfcn the downlink carrier frequency (EARFCN)
   *
   * By synchronizing, the PHY will start receiving various information
   * transmitted by the eNodeB. For instance, when receiving system information,
   * the message will be relayed to RRC via
   * NistLteUeCphySapUser::RecvNistMasterInformationBlock and
   * NistLteUeCphySapUser::RecvNistSystemInformationBlockType1 functions.
   *
   * Initially, the PHY will be configured to listen to 6 RBs of BCH.
   * NistLteUeCphySapProvider::SetDlBandwidth can be called afterwards to increase
   * the bandwidth.
   */
  virtual void SynchronizeWithEnb (uint16_t cellId, uint16_t dlEarfcn) = 0;

  /**
   * \param dlBandwidth the DL bandwidth in number of PRBs
   */
  virtual void SetDlBandwidth (uint8_t dlBandwidth) = 0;

  /** 
   * \brief Configure uplink (normally done after reception of SIB2)
   * 
   * \param ulEarfcn the uplink carrier frequency (EARFCN)
   * \param ulBandwidth the UL bandwidth in number of PRBs
   */
  virtual void ConfigureUplink (uint16_t ulEarfcn, uint8_t ulBandwidth) = 0;

  /**
   * \brief Configure referenceSignalPower
   *
   * \param referenceSignalPower received from eNB in SIB2
   */
  virtual void ConfigureReferenceSignalPower (int8_t referenceSignalPower) = 0;

  /** 
   * 
   * \param rnti the cell-specific UE identifier
   */
  virtual void SetRnti (uint16_t rnti) = 0;

  /**
   * \param txMode the transmissionMode of the user
   */
  virtual void SetTransmissionMode (uint8_t txMode) = 0;

  /**
   * \param srcCi the SRS configuration index
   */
  virtual void SetSrsConfigurationIndex (uint16_t srcCi) = 0;

  /**
   * \param pa the P_A value
   */
  virtual void SetPa (double pa) = 0;

  //discovery
  /**
   * set the current discovery transmit pool
   * \param pool the transmission pool
   */
  virtual void SetSlTxPool (Ptr<SidelinkTxDiscResourcePool> pool) = 0;

  /**
   * remove the discovery transmission pool 
   */
  virtual void RemoveSlTxPool (bool disc) = 0;

  /**
   * set the discovery receiving pools
   * \param pools the receiving pools
   */
  virtual void SetSlRxPools (std::list<Ptr<SidelinkRxDiscResourcePool> > pools) = 0;


  /**
   * set discovery annoucement apps
   * \param apps applications we are interested in announcing
   */
   virtual void AddDiscTxApps (std::list<uint32_t> apps) = 0;

  /**
   * set discovery monitoring apps
   * \param apps applications we are interested in monitoring
   */
   virtual void AddDiscRxApps (std::list<uint32_t> apps) = 0;


  /**
   * Set grant for discovery
   */
  virtual void SetDiscGrantInfo (uint8_t resPsdch) = 0;


  //communication
  /**
   * set the current sidelink transmit pool
   * \param pool the transmission pool
   */
  virtual void SetSlTxPool (Ptr<SidelinkTxCommResourcePool> pool) = 0;

  /**
   * remove the sidelink transmission pool 
   * \param dstL2Id the destination
   */
  virtual void RemoveSlTxPool () = 0;

  /**
   * set the sidelink receiving pools
   * \param destinations The list of destinations (group) to monitor
   * \param pools the sidelink receiving pools
   */
  virtual void SetSlRxPools (std::list<Ptr<SidelinkRxCommResourcePool> > pools) = 0;

  /**
   * add a new destination to listen for
   * \param a destination to listen for
   */
  virtual void AddSlDestination (uint32_t destination) = 0;

  /**
   * remove a destination to listen for
   * \param destination the destination that is no longer of interest
   */
  virtual void RemoveSlDestination (uint32_t destination) = 0;
  
  /**
   * Pass to the PHY entity the SLSSID to be set
   * \param slssid the SLSSID
   */
  virtual void SetSlssId (uint64_t slssid) = 0;
  /**
    * Pass to the PHY entity a SLSS to be sent
    * \param mibSl the MIB-SL to send
    */
   virtual void SendSlss (NistLteRrcSap::MasterInformationBlockSL mibSl) = 0;
   /**
    * Notify the PHY entity that a SyncRef has been selected and that it should apply
    * the corresponding change of timing when appropriate
    * \param mibSl the MIB-SL containing the information of the selected SyncRef
    */
   virtual void SynchronizeToSyncRef (NistLteRrcSap::MasterInformationBlockSL mibSl) = 0;

};


/**
 * Service Access Point (SAP) offered by the UE PHY to the UE RRC for control purposes
 *
 * This is the CPHY SAP User, i.e., the part of the SAP that contains the RRC
 * methods called by the PHY
*/
class NistLteUeCphySapUser
{
public:

  /** 
   * destructor
   */
  virtual ~NistLteUeCphySapUser ();


  /**
   * Parameters of the ReportNistUeMeasurements primitive: RSRP [dBm] and RSRQ [dB]
   * See section 5.1.1 and 5.1.3 of TS 36.214
   */
  struct NistUeMeasurementsElement
  {
    uint16_t m_cellId;  ///< Cell ID of the measured cell
    double m_rsrp;  ///< Measured RSRP [dBm]
    double m_rsrq;  ///< Measured RSRQ [dB]
  };

  /**
   * List of PSS measurements to be reported to the RRC
   */
  struct NistUeMeasurementsParameters
  {
    std::vector <struct NistUeMeasurementsElement> m_ueMeasurementsList; ///< List of PSS measurements to be reported to the RRC
  };

   /**
    * Parameters for reporting S-RSRP measurements to the RRC by the PHY
    */
    struct NistUeSlssMeasurementReportElement
     {
       uint16_t m_slssid; ///< SLSSID of the measured SyncRef
       double m_srsrp;  ///< Measured S-RSRP [dBm]
       uint16_t m_offset; ///< Reception offset
     };
    /**
     * List of SLSS measurements to be reported to the RRC by the PHY
     */
    struct NistUeSlssMeasurementsParameters
    {
      std::vector <struct NistUeSlssMeasurementReportElement> m_ueSlssMeasurementsList; ///< List of SLSS measurements to be reported to the RRC by the PHY
    };

  /**
   * \brief Relay an MIB message from the PHY entity to the RRC layer.
   * \param cellId the ID of the eNodeB where the message originates from
   * \param mib the Master Information Block message
   * 
   * This function is typically called after PHY receives an MIB message over
   * the BCH.
   */
  virtual void RecvNistMasterInformationBlock (uint16_t cellId,
                                           NistLteRrcSap::NistMasterInformationBlock mib) = 0;

  /**
   * \brief Relay an SIB1 message from the PHY entity to the RRC layer.
   * \param cellId the ID of the eNodeB where the message originates from
   * \param sib1 the System Information Block Type 1 message
   *
   * This function is typically called after PHY receives an SIB1 message over
   * the BCH.
   */
  virtual void RecvNistSystemInformationBlockType1 (uint16_t cellId,
                                                NistLteRrcSap::NistSystemInformationBlockType1 sib1) = 0;

  /**
   * \brief Send a report of RSRP and RSRQ values perceived from PSS by the PHY
   *        entity (after applying layer-1 filtering) to the RRC layer.
   * \param params the structure containing a vector of cellId, RSRP and RSRQ
   */
  virtual void ReportNistUeMeasurements (NistUeMeasurementsParameters params) = 0;

   /**
    * \brief Send a report of S-RSRP values perceived from SLSSs by the PHY
    *        entity (after applying layer-1 filtering) to the RRC layer.
    * \param params the structure containing a list of
    *        (SyncRef SLSSID, SyncRef offset and S-RSRP value)
    * \param slssid the SLSSID of the evaluated SyncRef if corresponding
    * \param offset the offset of the evaluated SyncRef if corresponding
    */
   virtual void ReportSlssMeasurements (NistLteUeCphySapUser::NistUeSlssMeasurementsParameters params,  uint64_t slssid, uint16_t offset) = 0;
   /**
    * The PHY indicated to the RRC the current subframe indication
    * \param frameNo the current frameNo
    * \param subFrameNo the current subframeNo
    */
   virtual void ReportSubframeIndication (uint16_t frameNo, uint16_t subFrameNo) = 0;
   /**
    * The PHY pass a received MIB-SL to the RRC
    * \param mibSl the received MIB-SL
    */
   virtual void ReceiveMibSL (NistLteRrcSap::MasterInformationBlockSL mibSl) = 0;
   /**
    * Notify the successful change of timing/SyncRef, and store the selected
    * (current) SyncRef information
    * \param mibSl the SyncRef MIB-SL containing its information
    * \param frameNo the current frameNo
    * \param subFrameNo the current subframeNo
    */
   virtual void ReportChangeOfSyncRef (NistLteRrcSap::MasterInformationBlockSL mibSl, uint16_t frameNo, uint16_t subFrameNo) = 0;
};




/**
 * Template for the implementation of the NistLteUeCphySapProvider as a member
 * of an owner class of type C to which all methods are forwarded
 * 
 */
template <class C>
class NistMemberLteUeCphySapProvider : public NistLteUeCphySapProvider
{
public:
  NistMemberLteUeCphySapProvider (C* owner);

  // inherited from NistLteUeCphySapProvider
  virtual void Reset ();
  virtual void StartCellSearch (uint16_t dlEarfcn);
  virtual void SynchronizeWithEnb (uint16_t cellId);
  virtual void SynchronizeWithEnb (uint16_t cellId, uint16_t dlEarfcn);
  virtual void SetDlBandwidth (uint8_t dlBandwidth);
  virtual void ConfigureUplink (uint16_t ulEarfcn, uint8_t ulBandwidth);
  virtual void ConfigureReferenceSignalPower (int8_t referenceSignalPower);
  virtual void SetRnti (uint16_t rnti);
  virtual void SetTransmissionMode (uint8_t txMode);
  virtual void SetSrsConfigurationIndex (uint16_t srcCi);
  virtual void SetPa (double pa);
  //discovery
  virtual void SetSlTxPool (Ptr<SidelinkTxDiscResourcePool> pool);
  virtual void RemoveSlTxPool (bool disc);
  virtual void SetSlRxPools (std::list<Ptr<SidelinkRxDiscResourcePool> > pools);
  virtual void SetDiscGrantInfo (uint8_t resPsdch);
  virtual void AddDiscTxApps (std::list<uint32_t> apps);
  virtual void AddDiscRxApps (std::list<uint32_t> apps);
  //communication
  virtual void SetSlTxPool (Ptr<SidelinkTxCommResourcePool> pool);
  virtual void RemoveSlTxPool ();
  virtual void SetSlRxPools (std::list<Ptr<SidelinkRxCommResourcePool> > pools);
  virtual void AddSlDestination (uint32_t destination);
  virtual void RemoveSlDestination (uint32_t destination);
  virtual void SetSlssId (uint64_t slssid);
  virtual void SendSlss (NistLteRrcSap::MasterInformationBlockSL mibSl);
  virtual void SynchronizeToSyncRef (NistLteRrcSap::MasterInformationBlockSL mibSl);


private:
  NistMemberLteUeCphySapProvider ();
  C* m_owner;
};

template <class C>
NistMemberLteUeCphySapProvider<C>::NistMemberLteUeCphySapProvider (C* owner)
  : m_owner (owner)
{
}

template <class C>
NistMemberLteUeCphySapProvider<C>::NistMemberLteUeCphySapProvider ()
{
}

template <class C>
void 
NistMemberLteUeCphySapProvider<C>::Reset ()
{
  m_owner->DoReset ();
}

template <class C>
void
NistMemberLteUeCphySapProvider<C>::StartCellSearch (uint16_t dlEarfcn)
{
  m_owner->DoStartCellSearch (dlEarfcn);
}

template <class C>
void
NistMemberLteUeCphySapProvider<C>::SynchronizeWithEnb (uint16_t cellId)
{
  m_owner->DoSynchronizeWithEnb (cellId);
}

template <class C>
void
NistMemberLteUeCphySapProvider<C>::SynchronizeWithEnb (uint16_t cellId, uint16_t dlEarfcn)
{
  m_owner->DoSynchronizeWithEnb (cellId, dlEarfcn);
}

template <class C>
void
NistMemberLteUeCphySapProvider<C>::SetDlBandwidth (uint8_t dlBandwidth)
{
  m_owner->DoSetDlBandwidth (dlBandwidth);
}

template <class C>
void 
NistMemberLteUeCphySapProvider<C>::ConfigureUplink (uint16_t ulEarfcn, uint8_t ulBandwidth)
{
  m_owner->DoConfigureUplink (ulEarfcn, ulBandwidth);
}

template <class C>
void 
NistMemberLteUeCphySapProvider<C>::ConfigureReferenceSignalPower (int8_t referenceSignalPower)
{
  m_owner->DoConfigureReferenceSignalPower (referenceSignalPower);
}

template <class C>
void
NistMemberLteUeCphySapProvider<C>::SetRnti (uint16_t rnti)
{
  m_owner->DoSetRnti (rnti);
}

template <class C>
void 
NistMemberLteUeCphySapProvider<C>::SetTransmissionMode (uint8_t txMode)
{
  m_owner->DoSetTransmissionMode (txMode);
}

template <class C>
void 
NistMemberLteUeCphySapProvider<C>::SetSrsConfigurationIndex (uint16_t srcCi)
{
  m_owner->DoSetSrsConfigurationIndex (srcCi);
}

template <class C>
void
NistMemberLteUeCphySapProvider<C>::SetPa (double pa)
{
  m_owner->DoSetPa (pa);
}

//discovery
template <class C>
void NistMemberLteUeCphySapProvider<C>::SetSlTxPool (Ptr<SidelinkTxDiscResourcePool> pool)
{
  m_owner->DoSetSlTxPool (pool);
}

template <class C>
void NistMemberLteUeCphySapProvider<C>::RemoveSlTxPool (bool disc)
{
  m_owner->DoRemoveSlTxPool (disc);
}

template <class C>
void NistMemberLteUeCphySapProvider<C>::SetSlRxPools (std::list<Ptr<SidelinkRxDiscResourcePool> > pools)
{
  m_owner->DoSetSlRxPools (pools);
}

template <class C>
void NistMemberLteUeCphySapProvider<C>::SetDiscGrantInfo (uint8_t resPsdch) 
{
  m_owner->DoSetDiscGrantInfo (resPsdch);
}

template <class C>
void NistMemberLteUeCphySapProvider<C>::AddDiscTxApps (std::list<uint32_t> apps)
{
  m_owner->DoAddDiscTxApps (apps);
}

template <class C>
void NistMemberLteUeCphySapProvider<C>::AddDiscRxApps (std::list<uint32_t> apps)
{
  m_owner->DoAddDiscRxApps (apps);
}


//communication
template <class C>
void
NistMemberLteUeCphySapProvider<C>::SetSlTxPool (Ptr<SidelinkTxCommResourcePool> pool)
{
  m_owner->DoSetSlTxPool (pool);
}

template <class C>
void NistMemberLteUeCphySapProvider<C>::RemoveSlTxPool ()
{
  m_owner->DoRemoveSlTxPool ();
}

template <class C>
void NistMemberLteUeCphySapProvider<C>::SetSlRxPools (std::list<Ptr<SidelinkRxCommResourcePool> > pools)
{
  m_owner->DoSetSlRxPools (pools);
}

template <class C>
void NistMemberLteUeCphySapProvider<C>::AddSlDestination (uint32_t destination)
{
  m_owner->DoAddSlDestination (destination);
}

template <class C>
void NistMemberLteUeCphySapProvider<C>::RemoveSlDestination (uint32_t destination)
{
  m_owner->DoRemoveSlDestination (destination);
}

template <class C>
void
NistMemberLteUeCphySapProvider<C>::SetSlssId (uint64_t slssid)
{
  m_owner->DoSetSlssId (slssid);
}

template <class C>
void
NistMemberLteUeCphySapProvider<C>::SendSlss (NistLteRrcSap::MasterInformationBlockSL mibSl)
{
  m_owner->DoSendSlss (mibSl);
}

template <class C>
void
NistMemberLteUeCphySapProvider<C>::SynchronizeToSyncRef (NistLteRrcSap::MasterInformationBlockSL mibSl)
{
  m_owner->DoSynchronizeToSyncRef (mibSl);
}

/**
 * Template for the implementation of the NistLteUeCphySapUser as a member
 * of an owner class of type C to which all methods are forwarded
 * 
 */
template <class C>
class NistMemberLteUeCphySapUser : public NistLteUeCphySapUser
{
public:
  NistMemberLteUeCphySapUser (C* owner);

  // methods inherited from NistLteUeCphySapUser go here
  virtual void RecvNistMasterInformationBlock (uint16_t cellId,
                                           NistLteRrcSap::NistMasterInformationBlock mib);
  virtual void RecvNistSystemInformationBlockType1 (uint16_t cellId,
                                                NistLteRrcSap::NistSystemInformationBlockType1 sib1);
  virtual void ReportNistUeMeasurements (NistLteUeCphySapUser::NistUeMeasurementsParameters params);

  virtual void ReportSlssMeasurements (NistLteUeCphySapUser::NistUeSlssMeasurementsParameters params,  uint64_t slssid, uint16_t offset);
  virtual void ReportSubframeIndication (uint16_t frameNo, uint16_t subFrameNo);
  virtual void ReceiveMibSL (NistLteRrcSap::MasterInformationBlockSL mibSL);
  virtual void ReportChangeOfSyncRef (NistLteRrcSap::MasterInformationBlockSL mibSL, uint16_t frameNo, uint16_t subFrameNo);

private:
  NistMemberLteUeCphySapUser ();
  C* m_owner;
};

template <class C>
NistMemberLteUeCphySapUser<C>::NistMemberLteUeCphySapUser (C* owner)
  : m_owner (owner)
{
}

template <class C>
NistMemberLteUeCphySapUser<C>::NistMemberLteUeCphySapUser ()
{
}

template <class C> 
void 
NistMemberLteUeCphySapUser<C>::RecvNistMasterInformationBlock (uint16_t cellId,
                                                       NistLteRrcSap::NistMasterInformationBlock mib)
{
  m_owner->DoRecvNistMasterInformationBlock (cellId, mib);
}

template <class C>
void
NistMemberLteUeCphySapUser<C>::RecvNistSystemInformationBlockType1 (uint16_t cellId,
                                                            NistLteRrcSap::NistSystemInformationBlockType1 sib1)
{
  m_owner->DoRecvNistSystemInformationBlockType1 (cellId, sib1);
}

template <class C>
void
NistMemberLteUeCphySapUser<C>::ReportNistUeMeasurements (NistLteUeCphySapUser::NistUeMeasurementsParameters params)
{
  m_owner->DoReportNistUeMeasurements (params);
}

template <class C>
void
NistMemberLteUeCphySapUser<C>::ReportSlssMeasurements (NistLteUeCphySapUser::NistUeSlssMeasurementsParameters params,  uint64_t slssid, uint16_t offset)
{
  m_owner->DoReportSlssMeasurements (params,  slssid, offset);
}


template <class C>
void
NistMemberLteUeCphySapUser<C>::ReportSubframeIndication (uint16_t frameNo, uint16_t subFrameNo)
{
  m_owner->DoReportSubframeIndication (frameNo, subFrameNo);
}

template <class C>
void
NistMemberLteUeCphySapUser<C>::ReceiveMibSL (NistLteRrcSap::MasterInformationBlockSL mibSL)
{
  m_owner->DoReceiveMibSL (mibSL);
}

template <class C>
void
NistMemberLteUeCphySapUser<C>::ReportChangeOfSyncRef (NistLteRrcSap::MasterInformationBlockSL mibSL, uint16_t frameNo, uint16_t subFrameNo)
{
  m_owner->DoReportChangeOfSyncRef (mibSL, frameNo, subFrameNo );
}


} // namespace ns3


#endif // NIST_LTE_UE_CPHY_SAP_H
