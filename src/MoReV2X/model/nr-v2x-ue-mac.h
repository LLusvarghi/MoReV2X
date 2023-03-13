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
 * Author: Nicola Baldo  <nbaldo@cttc.es>
 * Author: Marco Miozzo <mmiozzo@cttc.es>
 * Modified by: NIST
 * Modified by: Luca Lusvarghi <luca.lusvarghi5@unimore.it>
 */

#ifndef NR_V2X_UE_MAC_ENTITY_H
#define NR_V2X_UE_MAC_ENTITY_H




#include <map>

#include <ns3/nist-lte-mac-sap.h>
#include <ns3/nist-lte-ue-cmac-sap.h>
#include <ns3/nist-lte-ue-cphy-sap.h>
#include <ns3/nist-lte-ue-phy-sap.h>
#include <ns3/nist-lte-amc.h>
#include <ns3/nr-v2x-amc.h>
#include <ns3/nstime.h>
#include <ns3/event-id.h>
#include <vector>
#include <ns3/packet.h>
#include <ns3/packet-burst.h>
#include "ns3/traced-value.h"
#include "ns3/trace-source-accessor.h"


#include <cctype>
namespace ns3 {

class UniformRandomVariable;

class NrV2XUeMac :   public Object
{
  friend class NistUeMemberLteUeCmacSapProvider;
  friend class NistUeMemberLteMacSapProvider;
  friend class NistUeMemberLteUePhySapUser;

public:
  static TypeId GetTypeId (void);

  NrV2XUeMac ();
  virtual ~NrV2XUeMac ();
  virtual void DoDispose (void);

  NistLteMacSapProvider*  GetNistLteMacSapProvider (void);
  void  SetNistLteUeCmacSapUser (NistLteUeCmacSapUser* s);
  NistLteUeCmacSapProvider*  GetNistLteUeCmacSapProvider (void);

  
  /**
   * set the CPHY SAP this MAC should use to interact with the PHY
   *
   * \param s the CPHY SAP Provider
   */
  void SetNistLteUeCphySapProvider (NistLteUeCphySapProvider * s);
  //

  /**
  * \brief Get the PHY SAP user
  * \return a pointer to the SAP user of the PHY
  */
  NistLteUePhySapUser* GetNistLteUePhySapUser ();

  /**
  * \brief Set the PHY SAP Provider
  * \param s a pointer to the PHY SAP Provider
  */
  void SetNistLteUePhySapProvider (NistLteUePhySapProvider* s);
  

  
  

  /**
  * \brief Forwarded from NistLteUePhySapUser: trigger the start from a new frame
  *
  * \param frameNo frame number
  * \param subframeNo subframe number
  */
  void DoSubframeIndication (uint32_t frameNo, uint32_t subframeNo);

  /**
   * \return the UE selected resource mapping type
   *
   * This allows you to get access to the Sl UE selected resource value.
   */
  std::string GetNistSlUeSelResMapping (void) const;

  /**
   * \set the UE selected resource mapping type
   *
   * \param mapping is the UE selected mapping to set in the UE side
   */
  void SetNistSlUeSelResMapping (std::string mapping);
 
  /**
   * TracedCallback signature for transmission of discovery message.
   *
   * \param [in] rnti
   * \param [in] proSeAppCode
   */
  typedef void (* DiscoveryAnnouncementTracedCallback) 
    (const uint16_t rnti, const uint32_t proSeAppCode);

  /**
   *
   * \return discovery reception pools
   */
  std::list< Ptr<SidelinkRxDiscResourcePool> > GetDiscRxPools ();
 
  /**
   *
   * \return discovery transmission pools
   */
  Ptr<SidelinkTxDiscResourcePool> GetDiscTxPool ();


  /**
   *  TracedCallback signature for SL UL scheduling events.
   *
   * \param [in] frame Frame number
   * \param [in] subframe Subframe number
   * \param [in] rnti The C-RNTI identifying the UE
   * \param [in] mcs  The MCS for transport block
   * \param [in] pscch_ri The resource index in the PSCCH
   * \param [in] pssch_rb RB start in the PSSCH
   * \param [in] pssch_lenght Number of RBs in sub-channel in the PSSCH
   * \param [in] pssch_itrp TRP index used in PSSCH
   */
/*  typedef void (* SlUeSchedulingTracedCallback)
    (const uint32_t frame, const uint32_t subframe, const uint16_t rnti,     const uint8_t mcs, const uint16_t pscch_ri, const uint16_t pssch_rb, const uint16_t pssch_length, const uint16_t pssch_itrp);
*/

  std::vector<uint16_t> m_RRIvalues;

  /* 
    Public function to push new RRI values into the list of allowed RRI values
  */
  void PushNewRRIValue (uint16_t RRI);


  /*
    Map of geo-based subchannel indexes. Needed for the frequency-reuse dynamic scheme
  */
  std::map < uint16_t, std::vector < std::pair <double, double>>> m_subchannelsMap;

  /* 
    Public function to copy the geo-based map with the subchannels index
  */
  void CopySubchannelsMap (std::map < uint16_t, std::vector < std::pair <double, double>>> inputMap);

private:

  /**
   * The `DiscoveryMsgSent` trace source. Track the transmission of discovery message (announce)
   * Exporting RNTI, ProSe App Code.
   */
  TracedCallback<uint16_t, uint32_t> m_discoveryAnnouncementTrace;


  // forwarded from MAC SAP
  void DoTransmitPdu (NistLteMacSapProvider::NistTransmitPduParameters params);
  void DoReportBufferNistStatus (NistLteMacSapProvider::NistReportBufferNistStatusParameters params);

  // forwarded from UE CMAC SAP
  void DoConfigureRach (NistLteUeCmacSapProvider::NistRachConfig rc);
  void DoStartContentionBasedRandomAccessProcedure ();
  void DoStartNonContentionBasedRandomAccessProcedure (uint16_t rnti, uint8_t rapId, uint8_t prachMask);
  void DoAddLc (uint8_t lcId, NistLteUeCmacSapProvider::NistLogicalChannelConfig lcConfig, NistLteMacSapUser* msu);
  void DoAddLc (uint8_t lcId, uint32_t srcL2Id, uint32_t dstL2Id, NistLteUeCmacSapProvider::NistLogicalChannelConfig lcConfig, NistLteMacSapUser* msu);
  void DoRemoveLc (uint8_t lcId);
  void DoRemoveLc (uint8_t lcId, uint32_t srcL2Id, uint32_t dstL2Id);
  void DoReset ();
  //communication
  void DoAddSlTxPool (uint32_t dstL2Id, Ptr<SidelinkTxCommResourcePool> pool);
  void DoRemoveSlTxPool (uint32_t dstL2Id);
  void DoSetSlRxPools (std::list<Ptr<SidelinkRxCommResourcePool> > pools);
  void DoAddSlDestination (uint32_t destination);
  void DoRemoveSlDestination (uint32_t destination);
  //discovery
  void DoAddSlTxPool (Ptr<SidelinkTxDiscResourcePool> pool);
  void DoRemoveSlTxPool ();
  void DoSetSlRxPools (std::list<Ptr<SidelinkRxDiscResourcePool> > pools);
  void DoModifyDiscTxApps (std::list<uint32_t> apps);
  void DoModifyDiscRxApps (std::list<uint32_t> apps);

  void DoSetRnti (uint16_t);
  
  // forwarded from PHY SAP
  void DoReceivePhyPdu (Ptr<Packet> p);
  void DoReceiveNistLteControlMessage (Ptr<NistLteControlMessage> msg);
  
  // internal methods
  void RandomlySelectAndSendRaPreamble ();
  void SendRaPreamble (bool contention);
  void StartWaitingForRaResponse ();
  void RecvRaResponse (NistBuildNistRarListElement_s raResponse);
  void RaResponseTimeout (bool contention);
  void SendReportBufferNistStatus (void);
  void SendSidelinkReportBufferStatus (void);
  void RefreshHarqProcessesPacketBuffer (void);

  // Added function to assign priority to each flow 
  void DoAddLCPriority(uint8_t rnti, uint8_t lcid ,uint8_t  priority);

  // added to handle the different schedulers for the UE 
  void DoReceiveNistPFLteControlMessage (Ptr<NistLteControlMessage> msg);
  void DoReceiveNistMTLteControlMessage (Ptr<NistLteControlMessage> msg);
  void DoReceiveNistPrLteControlMessage (Ptr<NistLteControlMessage> msg);
  void DoReceiveNistRrLteControlMessage (Ptr<NistLteControlMessage> msg);

private:

  struct NistLcInfo
  {
    NistLteUeCmacSapProvider::NistLogicalChannelConfig lcConfig;
    NistLteMacSapUser* macSapUser;
  };

  std::map <uint8_t, NistLcInfo> m_lcInfoMap;
  NistLteUeCphySapProvider* m_cphySapProvider;
  NistLteMacSapProvider* m_macSapProvider;

  NistLteUeCmacSapUser* m_cmacSapUser;
  NistLteUeCmacSapProvider* m_cmacSapProvider;

  NistLteUePhySapProvider* m_uePhySapProvider;
  NistLteUePhySapUser* m_uePhySapUser;
  
  std::map <uint8_t, NistLteMacSapProvider::NistReportBufferNistStatusParameters> m_ulBsrReceived; // BSR received from RLC (the last one)
  // NIST: new structure for m_ulBsrReceived to handle UL per flow scheduling 
  std::map <uint8_t, std::map <uint8_t, NistLteMacSapProvider::NistReportBufferNistStatusParameters > > Nist_m_ulBsrReceived;
 
  // added map to handle priority  
  std::map <uint8_t, std::map <uint8_t, uint8_t> > PriorityMap;
  
  
  Time m_bsrPeriodicity;
  Time m_bsrLast;
  
  bool m_freshUlBsr; // true when a BSR has been received in the last TTI

  uint8_t m_harqProcessId;
  std::vector < Ptr<PacketBurst> > m_miUlHarqProcessesPacket; // Packets under trasmission of the UL HARQ processes
  std::vector < uint8_t > m_miUlHarqProcessesPacketTimer; // timer for packet life in the buffer

  uint16_t m_rnti;

  bool m_rachConfigured;
  NistLteUeCmacSapProvider::NistRachConfig m_rachConfig;
  uint8_t m_raPreambleId;
  uint8_t m_preambleTransmissionCounter;
  uint16_t m_backoffParameter;
  EventId m_noRaResponseReceivedEvent;
  Ptr<UniformRandomVariable> m_raPreambleUniformVariable;

  uint32_t m_frameNo;
  uint32_t m_subframeNo;
  uint8_t m_raRnti;
  bool m_waitingForRaResponse;
  
  //sidelink variables
  struct SidelinkLcIdentifier
  {
    uint8_t lcId;
    uint32_t srcL2Id;
    uint32_t dstL2Id;
  };
  
  friend bool operator < (const SidelinkLcIdentifier &l, const SidelinkLcIdentifier &r) { return l.lcId < r.lcId || (l.lcId == r.lcId && l.srcL2Id < r.srcL2Id) || (l.lcId == r.lcId && l.srcL2Id == r.srcL2Id && l.dstL2Id < r.dstL2Id); }
  
  std::map <SidelinkLcIdentifier, NistLcInfo> m_slLcInfoMap;
  Time m_slBsrPeriodicity;
  Time m_slBsrLast;
  bool m_freshSlBsr; // true when a BSR has been received in the last TTI
  bool m_alreadyUeSelectedSlBsr; //added for PSSCH - PSCCH FDM in D2D Mode 4 Rel' 14
  uint64_t m_absSFN;  // the absolute index of the 1024 frame cycle, starting from the origin of time axis
  double m_millisecondsFromLastAbsSFNUpdate;

   /*Subchannelization scheme*/
  uint16_t m_nsubCHsize;
  uint16_t m_L_SubCh;
  uint16_t m_BW_RBs;
  double m_slotDuration;
  uint16_t m_numerologyIndex;

  bool m_randomSelection; 

  std::map <SidelinkLcIdentifier, NistLteMacSapProvider::NistReportBufferNistStatusParameters> m_slBsrReceived; // BSR received from RLC (the last one)

  struct SidelinkGrant
  {
    //fields common with SL_DCI
    uint16_t m_resPscch;
    uint8_t m_tpc;
    uint8_t m_hopping;
    uint8_t m_rbStart; //models rb assignment
    uint8_t m_rbLen;   //models rb assignment
    uint8_t m_trp;

    //other fields
    uint8_t m_mcs;
    uint32_t m_tbSize;
  };

  struct V2XSchedulingInfo
  {
    //Lenght of the reservation should be a common parameter. 
    uint16_t m_rbStartPssch; //models rb assignment
    uint16_t m_rbLenPssch;   //models rb assignment: reservation size

    uint16_t m_rbStartPscch; //models rb assignment
    uint16_t m_rbLenPscch;   //models rb assignment

    uint32_t m_nextReservedFrame;
    uint32_t m_nextReservedSubframe;

    uint32_t m_ReEvaluationFrame;
    uint32_t m_ReEvaluationSubframe;

    bool m_EnableReEvaluation;

    uint16_t m_SelectionTrigger;

    bool m_announced;
  };

  struct V2XSidelinkGrant
  {
 //   uint16_t m_rbStartPssch; //models rb assignment
 //   uint16_t m_rbLenPssch;   //models rb assignment: reservation size

 //   uint16_t m_rbStartPscch; //models rb assignment
 //   uint16_t m_rbLenPscch;   //models rb assignment

    //other fields
    uint8_t m_mcs;
    uint32_t m_tbSize;
    
    uint16_t m_TxNumber; //Number of transmissions using this grant (1 or 2 for the moment)
    uint16_t m_TxIndex; //Indicate whether it is the first or second transmission (i.e., initial transmission or retransmission)

    std::map<uint16_t, V2XSchedulingInfo> m_grantTransmissions;

    /*Resource Reservation Fields*/
    double m_RRI; // Reservation period: offset from next expected Tx [ms]

//    uint16_t m_reservation;  
    uint32_t m_Cresel; // the Relelection Counter
 //   uint32_t m_nextReservedFrame;
 //   uint32_t m_nextReservedSubframe;

  //  uint32_t m_ReEvaluationFrame;
  //  uint32_t m_ReEvaluationSubframe;

//    bool m_EnableReEvaluation;
//    uint16_t m_SelectionTrigger;
  };

  struct UeSelectionInfo
  {
    V2XSidelinkGrant selGrant;
    double RSRPthresh;
    uint32_t iterations;
    double time;
    SidelinkCommResourcePool::SubframeInfo selFrame;
    uint32_t nodeId;
    uint32_t nCSRfinal;
    uint32_t nCSRpastTx;
    uint32_t nCSRinitial;
    double pdb;
  };

  // Variables to store output data
  static std::vector<UeSelectionInfo> SelectedGrants;
  static double prevPrintTime_selection;

  struct UeReEvaluationInfo
  {
    uint32_t nodeId;
    double time;
    uint16_t checkedTxIndex;
    SidelinkCommResourcePool::SubframeInfo ReEvalSF;
    SidelinkCommResourcePool::SubframeInfo LastReEvalSF;
    SidelinkCommResourcePool::SubframeInfo CheckSF;
    uint32_t packetID;
    uint16_t CheckCSR;
    bool freshGrant;
    bool reSelection;
    uint16_t reSelectionType;
  };

  // Variables to store output data
  static std::vector<UeReEvaluationInfo> ReEvaluationStats;
  static double prevPrintTime_reEvaluation;

  struct TxPacketInfo
  {
    uint32_t packetID;
    double txTime;
    uint16_t selTrigger;
    bool announced;
  };

  // Variables to store output data
  static std::vector<TxPacketInfo> TxPacketsStats;
  static double prevPrintTime_packetInfo;

  struct PoolInfo
  {
    Ptr<SidelinkCommResourcePool> m_pool; //the pool
    SidelinkCommResourcePool::SubframeInfo m_currentScPeriod; //start of current period
    SidelinkGrant m_currentGrant; //grant for the next SC period

    V2XSidelinkGrant m_currentV2XGrant; //current V2X grant
    
    SidelinkCommResourcePool::SubframeInfo m_nextScPeriod; //start of next period

    uint32_t m_npscch; // number of PSCCH available in the pool

    bool m_grant_received;
    bool m_V2X_grant_received;
    bool m_V2X_grant_fresh;

    SidelinkGrant m_nextGrant; //grant received for the next SC period
    V2XSidelinkGrant m_nextV2XGrant; //grant received for the next.... what?

    std::list<SidelinkCommResourcePool::SidelinkTransmissionInfo> m_pscchTx;//list of PSCCH transmissions within the pool
    std::list<SidelinkCommResourcePool::SidelinkTransmissionInfo> m_psschTx; //list of PSSCH transmissions within the pool
    
    std::list<SidelinkCommResourcePool::V2XSidelinkTransmissionInfo> m_v2xTx; //list of V2X PSSCH and PSCCH transmissions within the pool
  
    Ptr<PacketBurst> m_miSlHarqProcessPacket; // Packets under trasmission of the SL HARQ process

   //fields for V2X communication
   
  };

  
  std::map <uint32_t, PoolInfo > m_sidelinkTxPoolsMap;
  std::list <Ptr<SidelinkRxCommResourcePool> > m_sidelinkRxPools;
  std::list <uint32_t> m_sidelinkDestinations;

  Ptr<NistLteAmc> m_amc; //needed now since UE is doing scheduling
  Ptr<NrV2XAmc> m_NRamc; // New Radio (NR) Adaptive Modulation and Coding (AMC)
  Ptr<UniformRandomVariable> m_ueSelectedUniformVariable;

  //fields for fixed UE_SELECTED pools
  uint8_t m_slGrantMcs;


  //TODO FIXME NEW
  //fields for V2X Communication V2X V2X V2X V2X V2X V2X V2X V2X V2X
  uint8_t m_SFGap; //Time gap [subframes] between initial transmission and retransmission
  
  enum reselectionTrigger
  {
    COUNTER = 0,
    LATENCYandSIZE = 1,
    LATENCY = 2,
    SIZE = 3,
    ReEVALUATION = 4
  };

  SidelinkCommResourcePool::SubframeInfo m_prevListUpdate; //Clean past tx and sensed subframe list removing subframes older than the selection window
  void UpdatePastTxInfo (uint16_t current_frameNo, uint16_t current_subframeNo);
  void UpdateSensedCSR (uint16_t current_frameNo, uint16_t current_subframeNo);


  struct ReservationsInfo
  {
    std::vector<double> UnutilizedSubchannelsRatio;
    uint32_t UnutilizedReservations;
    uint32_t Reservations;
    uint32_t LatencyReselections;
    uint32_t SizeReselections;
    uint32_t CounterReselections;
    uint32_t TotalTransmissions;
  };


  static std::map<uint32_t, ReservationsInfo> ReservationsStats;
  static double prevPrintTime_reservations;

  bool m_List2Enabled;
  bool m_EnableReTx;   
  bool m_useRxCresel;

  uint16_t m_debugNode;

  bool m_dynamicScheduling;
  bool m_mixedTraffic;

  bool m_aggressive;
  bool m_oneShot;
  bool m_submissive;
//  bool m_useMode4;
//  bool m_useMode2;

  double m_maxPDB;

  bool m_allSlotsReEvaluation;
  bool m_UMHvariant;
  bool m_FreqReuse;
  bool m_AdaptiveScheduling;

  double m_keepProbability;
  Ptr<UniformRandomVariable> m_evalKeepProb;

  double m_sizeThreshold;
  double m_rsrpThreshold;
  uint16_t m_sensingWindow;

  std::string m_outputPath;
  double m_savingPeriod;

  bool m_oneShotGrant;
//  uint16_t SubtractFrames (uint16_t frameAhead, uint16_t frame, uint16_t subframeAhead, uint16_t subframe);
 
//  void ReEvaluateResources (V2XSidelinkGrant currentV2Xgrant, NistLteMacSapProvider::NistReportBufferNistStatusParameters pktParams);
  void ReEvaluateResources (SidelinkCommResourcePool::SubframeInfo currentSF, std::map <uint32_t, PoolInfo>::iterator IT, NistLteMacSapProvider::NistReportBufferNistStatusParameters pktParams);

  SidelinkCommResourcePool::SubframeInfo UnimoreUpdateReservation (uint32_t frameNo, uint32_t subframeNo, uint16_t RRI_slots);

  //discovery
  struct DiscGrant
  {
    uint16_t m_rnti;
    uint8_t m_resPsdch;
    
    //uint32_t m_tbSize; //232 bits (fixed) 
  };

  struct DiscPoolInfo
  {
    Ptr<SidelinkTxDiscResourcePool> m_pool; //the pool
    SidelinkDiscResourcePool::SubframeInfo m_currentDiscPeriod; //start of current period
    DiscGrant m_currentGrant; //grant for the next discovery period
    SidelinkDiscResourcePool::SubframeInfo m_nextDiscPeriod; //start of next period

    uint32_t m_npsdch; // number of PSDCH available in the pool

    bool m_grant_received;
    DiscGrant m_nextGrant; //grant received for the next discovery period

    std::list<SidelinkDiscResourcePool::SidelinkTransmissionInfo> m_psdchTx;//list of PSDCH transmissions within the pool

    //HARQ is not considered for now
    //Ptr<PacketBurst> m_miSlHarqProcessPacket; // Packets under trasmission of the SL HARQ process
  };

  DiscPoolInfo m_discTxPools;
  std::list <Ptr<SidelinkRxDiscResourcePool> > m_discRxPools;

  std::list<uint32_t> m_discTxApps;
  std::list<uint32_t> m_discRxApps;

  Ptr<UniformRandomVariable> m_p1UniformVariable;
  Ptr<UniformRandomVariable> m_resUniformVariable;


  /**
   * Trace information regarding SL UE scheduling.
   * Frame number, Subframe number, RNTI, MCS, PSCCH resource index, PSSCH RB start, PSSCH lenght in RBs, PSSCH TRP index
   */
  TracedCallback<NistSlUeMacStatParameters> m_slUeScheduling;
 
  TracedCallback<NistSlUeMacStatParameters> m_slSharedChUeScheduling;

  /**
   * True if there is data to transmit in the PSSCH
   */
  bool m_slHasDataToTx;
  //The PHY notifies the change of timing as consequence of a change of SyncRef, the MAC adjust its timing
  void DoNotifyChangeOfTiming (uint32_t frameNo, uint32_t subframeNo);
 



  //TODO FIXME NEW for V2X

  struct PsschRsrp
  {
    uint16_t rbStart;
    uint16_t rbLen;
    double psschRsrpDb;
  };

  std::map <Time,PsschRsrp> m_PsschRsrpMap;
  std::list <std::pair<Time,SidelinkCommResourcePool::SubframeInfo>> m_pastTxUnimore;

  bool m_validReservation;
  bool m_updateReservation;
  uint32_t m_tmpFrameNo;
  uint32_t m_tmpSubframeNo;

  bool m_reEvaluation;

  V2XSidelinkGrant m_aperiodicV2XGrant; //aperiodic V2X grant
  V2XSidelinkGrant m_tmpV2XGrant; //aperiodic V2X grant


  //TODO FIXME NEW for V2X Sensing-Based SPS
  struct ReservedCSR
  {
    uint16_t rbStart;
    uint16_t rbLen;
    double psschRsrpDb;
    Time reservationTime; //when the reservation was received
    SidelinkCommResourcePool::SubframeInfo reservedSF;
    uint32_t CreselRx; // the number of times the given resource is expected to be repeated in the future
    uint32_t nodeId;
    double RRI;
    bool isReTx;
    bool isSameTB;
  };

  //std::map <SidelinkCommResourcePool::SubframeInfo,ReservedCSR> m_sensedReservedCSRMap;
  void UnimorePrintCSR (std::map<uint16_t,std::list<SidelinkCommResourcePool::SubframeInfo> > Map);
  void UnimorePrintSensedCSR (std::map <uint16_t,std::map<SidelinkCommResourcePool::SubframeInfo, std::vector<ReservedCSR>> > SensedResources_Map, SidelinkCommResourcePool::SubframeInfo currentSF, bool save);  // print the sensed CSRs
 // void UnimoreSaveSensedCSR (std::map <uint16_t,std::map<SidelinkCommResourcePool::SubframeInfo, std::vector<ReservedCSR>> > SensedResources_Map, SidelinkCommResourcePool::SubframeInfo currentSF);  // save the sensed CSRs

  std::map <uint16_t, std::map<SidelinkCommResourcePool::SubframeInfo, std::vector<ReservedCSR> > > m_sensedReservedCSRMap;

  /*Map to store the past transmission information*/
  std::map<uint16_t,std::list<SidelinkCommResourcePool::SubframeInfo> > m_pastTxMap;
 
  struct CandidateCSR
  {
    uint16_t rbStart; //maybe redundant
    uint16_t rbLen;   //maybe redundant
    uint16_t nSubCHStart;
    SidelinkCommResourcePool::SubframeInfo subframe;   
  }; 

  struct CandidateCSRl2
  {
    uint16_t CSRIndex;
    SidelinkCommResourcePool::SubframeInfo subframe;   
    double rssi;
  };


  /**
  * Method to store PSSCH-RSRP measurement results coming from PHY layer 
  */
  void DoReportPsschRsrp (Time time, uint16_t rbStart, uint16_t rbLen, double rsrpDb);

  void DoReportPsschRsrpReservation (Time time, uint16_t rbStart, uint16_t rbLen, double rsrpDb, SidelinkCommResourcePool::SubframeInfo receivedSubframe, SidelinkCommResourcePool::SubframeInfo reservedSubframe, uint32_t CreselRx, uint32_t nodeId, double RRI, bool isReTx, bool isSameTB);
  

  SidelinkCommResourcePool::SubframeInfo ComputeReEvaluationFrame(uint32_t frameNo, uint32_t subframeNo);

 // uint32_t EvaluateSlotsDifference(SidelinkCommResourcePool::SubframeInfo SF1, SidelinkCommResourcePool::SubframeInfo SF2);

  std::map<uint16_t, V2XSchedulingInfo> UnimoreSortSelections (V2XSchedulingInfo Selection1, V2XSchedulingInfo Selection2, uint32_t maxDiffSlots);

  /**
  * Method to select resources in LTE-V2X UE_SELECTED Mode 4 
  * Method to select resources in NR-V2X UE_SELECTED Mode 2 
  */
  //V2XSidelinkGrant V2XSelectResources (uint32_t frameNo, uint32_t subframeNo, V2XSidelinkGrant V2XGrant, uint32_t pdb, uint32_t p_rsvp, uint8_t v2xMessageType, uint8_t v2xTrafficType, uint16_t ReselectionCounter, uint16_t PacketSize, uint16_t ReservationSize);
  V2XSidelinkGrant V2XSelectResources (uint32_t frameNo, uint32_t subframeNo, double pdb, double p_rsvp, uint8_t v2xMessageType, uint8_t v2xTrafficType, uint16_t ReselectionCounter, uint16_t PacketSize, uint16_t ReservationSize, reselectionTrigger V2Xtrigger);

  V2XSidelinkGrant V2XChangeResources (V2XSidelinkGrant OriginalGrant, std::vector<uint16_t> GrantsToChangeIndex, std::vector<uint16_t> OkGrantsIndex, uint32_t frameNo, uint32_t subframeNo, double pdb, uint16_t PacketSize, uint16_t ReservationSize, reselectionTrigger V2Xtrigger);

  std::map<uint16_t,std::list<SidelinkCommResourcePool::SubframeInfo>> SelectionWindow (SidelinkCommResourcePool::SubframeInfo currentSF, uint32_t T_2_slots, uint16_t N_CSR_per_SF);

  std::map<uint16_t,std::list<SidelinkCommResourcePool::SubframeInfo>> Mode2Step1 (std::map<uint16_t,std::list<SidelinkCommResourcePool::SubframeInfo> > Sa, SidelinkCommResourcePool::SubframeInfo currentSF, V2XSidelinkGrant V2XGrant, double T_2, uint16_t NSubCh,  uint16_t L_SubCh, uint32_t *iterationsCounter, double *psschThresh, uint32_t *nCSRpartial, bool OnlyReTxions);

  /**
  * Method to store Tx events for scheduling assistance in LTE-V2V UE_SELECTED Mode 4 
  */
  void DoStoreTxInfo (SidelinkCommResourcePool::SubframeInfo subframe, uint16_t rbStart, uint16_t rbLen);

};

} // namespace ns3

#endif // NR_V2X_UE_MAC_ENTITY
