/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 TELEMATICS LAB, DEE - Politecnico di Bari
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
 * Author: Giuseppe Piro  <g.piro@poliba.it>
 * Author: Marco Miozzo <mmiozzo@cttc.es>
 * Modified by: NIST
 * Modified by: Luca Lusvarghi <luca.lusvarghi5@unimore.it>
 */

#ifndef NR_V2X_UE_PHY_H
#define NR_V2X_UE_PHY_H

#include <ns3/nist-lte-phy.h>
#include <ns3/nist-ff-mac-common.h>

#include <ns3/nist-lte-control-messages.h>
#include <ns3/nist-lte-ue-phy-sap.h>
#include <ns3/nist-lte-ue-cphy-sap.h>
#include <ns3/ptr.h>
#include <set>
#include <ns3/nist-lte-ue-power-control.h>


namespace ns3 {

class PacketBurst;
class NistLteHarqPhy;


/**
 * \ingroup lte
 *
 * The NrV2XSpectrumPhy models the physical layer of LTE
 */
class NrV2XUePhy : public NistLtePhy
{

  friend class NistUeMemberLteUePhySapProvider;
  friend class NistMemberLteUeCphySapProvider<NrV2XUePhy>;

public:
  /**
   * \brief The states of the UE PHY entity
   */
  enum State
  {
    CELL_SEARCH = 0,
    SYNCHRONIZED,
    NUM_STATES
  };

  /**
   * @warning the default constructor should not be used
   */
  NrV2XUePhy ();

  /**
   *
   * \param dlPhy the downlink NrV2XSpectrumPhy instance
   * \param ulPhy the uplink NrV2XSpectrumPhy instance
   */
  NrV2XUePhy (Ptr<NrV2XSpectrumPhy> dlPhy, Ptr<NrV2XSpectrumPhy> ulPhy);

  virtual ~NrV2XUePhy ();

  // inherited from Object
  static TypeId GetTypeId (void);
  virtual void DoInitialize (void);
  virtual void DoDispose (void);

  /**
   * \brief Get the PHY SAP provider
   * \return a pointer to the SAP Provider 
   */
  NistLteUePhySapProvider* GetNistLteUePhySapProvider ();

  /**
  * \brief Set the PHY SAP User
  * \param s a pointer to the SAP user
  */
  void SetNistLteUePhySapUser (NistLteUePhySapUser* s);

  /**
   * \brief Get the CPHY SAP provider
   * \return a pointer to the SAP Provider
   */
  NistLteUeCphySapProvider* GetNistLteUeCphySapProvider ();

  /**
  * \brief Set the CPHY SAP User
  * \param s a pointer to the SAP user
  */
  void SetNistLteUeCphySapUser (NistLteUeCphySapUser* s);


  /**
   * \param pow the transmission power in dBm
   */
  void SetTxPower (double pow);

  /**
   * \return the transmission power in dBm
   */
  double GetTxPower () const;

  /**
   * \return ptr to UE Uplink Power Control entity
   */
  Ptr<NistLteUePowerControl> GetUplinkPowerControl () const;

  /**
   * \param nf the noise figure in dB
   */
  void SetNoiseFigure (double nf);

  /**
   * \return the noise figure in dB
   */
  double GetNoiseFigure () const;

  /**
   * \returns the TTI delay between MAC and channel
   */
  uint8_t GetMacChDelay (void) const;

  /**
   * \return a pointer to the NrV2XSpectrumPhy instance relative to the downlink
   */
  Ptr<NrV2XSpectrumPhy> GetDlSpectrumPhy () const;

  /**
   * \return a pointer to the NrV2XSpectrumPhy instance relative to the uplink
   */
  Ptr<NrV2XSpectrumPhy> GetUlSpectrumPhy () const;

  /**
   * set the spectrum phy instance for sidelink reception
   * \param phy the sidelink spectrum phy
   */
  void SetSlSpectrumPhy (Ptr<NrV2XSpectrumPhy> phy);
  
  /**
   * \return a pointer to the NrV2XSpectrumPhy instance relative to the sidelink reception
   */
  Ptr<NrV2XSpectrumPhy> GetSlSpectrumPhy () const;
  
  /**
   * \brief Create the PSD for the TX
   * \return the pointer to the PSD
   */
  virtual Ptr<SpectrumValue> CreateTxPowerSpectralDensity ();

  /**
   * \brief Set a list of sub channels to use in TX
   * \param mask a list of sub channels
   */
  void SetSubChannelsForTransmission (std::vector <int> mask);
  /**
   * \brief Get a list of sub channels to use in TX
   * \return a list of sub channels
   */
  std::vector <int> GetSubChannelsForTransmission (void);

  /**
   * \brief Get a list of sub channels to use in TX
   * \param mask list of sub channels
   */
  void SetSubChannelsForReception (std::vector <int> mask);
  /**
   * \brief Get a list of sub channels to use in RX
   * \return a list of sub channels
   */
  std::vector <int> GetSubChannelsForReception (void);

  /**
  * \brief Create the DL CQI feedback from SINR values perceived at
  * the physical layer with the signal received from eNB
  * \param sinr SINR values vector
  * \return a DL CQI control message containing the CQI feedback
  */
  Ptr<NistDlCqiLteControlMessage> CreateDlCqiFeedbackMessage (const SpectrumValue& sinr);



  // inherited from NistLtePhy
  virtual void GenerateCtrlCqiReport (const SpectrumValue& sinr);
  virtual void GenerateDataCqiReport (const SpectrumValue& sinr);
  virtual void GenerateMixedCqiReport (const SpectrumValue& sinr);
  virtual void ReportInterference (const SpectrumValue& interf);
  virtual void ReportDataInterference (const SpectrumValue& interf);
  virtual void ReportRsReceivedPower (const SpectrumValue& power);

  // callbacks for NrV2XSpectrumPhy
  virtual void ReceiveNistLteControlMessageList (std::list<Ptr<NistLteControlMessage> >);
  virtual void ReceivePss (uint16_t cellId, Ptr<SpectrumValue> p);


  /**
   * \brief PhySpectrum received a new PHY-PDU
   */
  void PhyPduReceived (Ptr<Packet> p);


  /**
  * \brief trigger from eNB the start from a new frame
  *
  * \param frameNo frame number
  * \param subframeNo subframe number
  */
  void SubframeIndication (uint32_t frameNo, uint32_t subframeNo);


  /**
   * \brief Send the SRS signal in the last symbols of the frame
   */
  void SendSrs ();

  /**
   * \brief PhySpectrum generated a new DL HARQ feedback
   */
  virtual void ReceiveLteDlHarqFeedback (NistDlInfoListElement_s mes);

  /**
   * \brief Set the HARQ PHY module
   */
  void SetHarqPhyModule (Ptr<NistLteHarqPhy> harq);

  /**
   * \return The current state
   */
  State GetState () const;

  /**
   * TracedCallback signature for state transition events.
   *
   * \param [in] cellId
   * \param [in] rnti
   * \param [in] oldState
   * \param [in] newState
   */
  typedef void (* StateTracedCallback)
    (const uint16_t cellId, const uint16_t rnti,
     const State oldState, const State newState);

  /**
   * TracedCallback signature for cell RSRP and SINR report.
   *
   * \param [in] cellId
   * \param [in] rnti
   * \param [in] rsrp
   * \param [in] sinr
   */
  typedef void (* RsrpSinrTracedCallback)
    (const uint16_t cellId, const uint16_t rnti,
     const double rsrp, const double sinr);

  /**
   * TracedCallback signature for cell RSRP and RSRQ.
   *
   * \param [in] rnti
   * \param [in] cellId
   * \param [in] rsrp
   * \param [in] rsrq
   * \param [in] isServingCell
   */
  typedef void (* RsrpRsrqTracedCallback)
    (const uint16_t rnti, const uint16_t cellId,
     const double rsrp, const double rsrq, const bool isServingCell);

  /**
   * Set the time in which the first SyncRef selection will be performed by the UE
   * \param t the time to perform the first SyncRef selection (relative to the
   *          simulation time when the function is called, i.e., relative to 0, if it is called
   *          before the start of the simulation)
   * \note The UE will never perform the SyncRef selection process if this function
   * is not called before the start of the simulation
   */
  void SetFirstScanningTime(Time t);
  /**
   * Get the time in which the first SyncRef selection will be performed
   */
  Time GetFirstScanningTime();
  /**
   * Stores the S-RSRP of the detected SLSSs during the SyncRef selection  process
   * (i.e., during SyncRef scanning or measurement)
   * \param slssid the SLSSID of the SyncRef
   * \param p the received signal
   * \note The S-RSRP is not stored if: it is below the minimum required, or the UE
   *       is not performing the SyncRef selection process
   */
  virtual void ReceiveSlss (uint16_t slssid, Ptr<SpectrumValue> p); //callback for NrV2XSpectrumPhy
  
   // TODO FIXME NEW for V2X
  virtual void MeasurePsschRsrp(Ptr<SpectrumValue> p);
  void UnimoreReceivedRssi (double rssi, std::vector <int> rbMap, uint16_t ID);
  /**
  * Variables needed to store the RSSI measurements in a grid-like fashion
  */

  Time m_SL_DATA_DURATION;

private:

  void SetTxMode1Gain (double gain);
  void SetTxMode2Gain (double gain);
  void SetTxMode3Gain (double gain);
  void SetTxMode4Gain (double gain);
  void SetTxMode5Gain (double gain);
  void SetTxMode6Gain (double gain);
  void SetTxMode7Gain (double gain);
  void SetTxModeGain (uint8_t txMode, double gain);

  void QueueSubChannelsForTransmission (std::vector <int> rbMap);


  /** 
   * internal method that takes care of generating CQI reports,
   * calculating the RSRP and RSRQ metrics, and generating RSRP+SINR traces
   * 
   * \param sinr 
   */
  void GenerateCqiRsrpRsrq (const SpectrumValue& sinr);


  /**
   * \brief Layer-1 filtering of RSRP and RSRQ measurements and reporting to
   *        the RRC entity.
   *
   * Initially executed at +0.200s, and then repeatedly executed with
   * periodicity as indicated by the *NistUeMeasurementsFilterPeriod* attribute.
   */
  void ReportNistUeMeasurements ();

  /**
   * Switch the UE PHY to the given state.
   * \param s the destination state
   */
  void SwitchToState (State s);

  // UE CPHY SAP methods
  void DoReset ();
  void DoStartCellSearch (uint16_t dlEarfcn);
  void DoSynchronizeWithEnb (uint16_t cellId);
  void DoSynchronizeWithEnb (uint16_t cellId, uint16_t dlEarfcn);
  void DoSetDlBandwidth (uint16_t ulBandwidth);
  void DoConfigureUplink (uint16_t ulEarfcn, uint16_t ulBandwidth);
  void DoConfigureReferenceSignalPower (int8_t referenceSignalPower);
  void DoSetRnti (uint16_t rnti);
  void DoSetTransmissionMode (uint8_t txMode);
  void DoSetSrsConfigurationIndex (uint16_t srcCi);
  void DoSetPa (double pa);
 
  //discovery
  void DoSetSlTxPool (Ptr<SidelinkTxDiscResourcePool> pool);
  void DoRemoveSlTxPool (bool disc);
  void DoSetSlRxPools (std::list<Ptr<SidelinkRxDiscResourcePool> > pools);
  void DoSetDiscGrantInfo (uint8_t resPsdch);
  void DoAddDiscTxApps (std::list<uint32_t> apps);
  void DoAddDiscRxApps (std::list<uint32_t> apps);

  //communication
  void DoSetSlTxPool (Ptr<SidelinkTxCommResourcePool> pool);
  void DoRemoveSlTxPool ();
  void DoSetSlRxPools (std::list<Ptr<SidelinkRxCommResourcePool> > pools);
  void DoAddSlDestination (uint32_t destination);
  void DoRemoveSlDestination (uint32_t destination);

  // UE PHY SAP methods 
  virtual void DoSendMacPdu (Ptr<Packet> p);
  virtual void DoSendNistLteControlMessage (Ptr<NistLteControlMessage> msg);
  virtual void DoSendRachPreamble (uint32_t prachId, uint32_t raRnti);

  /// A list of sub channels to use in TX.
  std::vector <int> m_subChannelsForTransmission;
  /// A list of sub channels to use in RX.
  std::vector <int> m_subChannelsForReception;

  std::vector< std::vector <int> > m_subChannelsForTransmissionQueue;


  /**
   * The `EnableUplinkPowerControl` attribute. If true, Uplink Power Control
   * will be enabled.
   */
  bool m_enableUplinkPowerControl;
  /// Pointer to UE Uplink Power Control entity.
  Ptr<NistLteUePowerControl> m_powerControl;

  /// Wideband Periodic CQI. 2, 5, 10, 16, 20, 32, 40, 64, 80 or 160 ms.
  Time m_p10CqiPeriocity;
  Time m_p10CqiLast;

  /**
   * SubBand Aperiodic CQI. Activated by DCI format 0 or Random Access Response
   * Grant.
   * \note Defines a periodicity for academic studies.
   */
  Time m_a30CqiPeriocity;
  Time m_a30CqiLast;

  NistLteUePhySapProvider* m_uePhySapProvider;
  NistLteUePhySapUser* m_uePhySapUser;

  NistLteUeCphySapProvider* m_ueCphySapProvider;
  NistLteUeCphySapUser* m_ueCphySapUser;

  uint16_t  m_rnti;
 
  uint8_t m_transmissionMode;
  std::vector <double> m_txModeGain;

  uint16_t m_srsPeriodicity;
  uint16_t m_srsSubframeOffset;
  uint16_t m_srsConfigured;
  Time     m_srsStartTime;

  double m_paLinear;

  bool m_dlConfigured;
  bool m_ulConfigured;

  /// The current UE PHY state.
  State m_state;
  /**
   * The `StateTransition` trace source. Fired upon every UE PHY state
   * transition. Exporting the serving cell ID, RNTI, old state, and new state.
   */
  TracedCallback<uint16_t, uint16_t, State, State> m_stateTransitionTrace;
  
  /**
   * The `DiscoveryMsgSent` trace source. Track the transmission of discovery message (announce)
   * Exporting cellId, RNTI, ProSe App Code.
   */
  TracedCallback<uint16_t, uint16_t, uint32_t> m_discoveryAnnouncementTrace;

  /// \todo Can be removed.
  uint8_t m_subframeNo;

  bool m_rsReceivedPowerUpdated;
  SpectrumValue m_rsReceivedPower;

  bool m_rsInterferencePowerUpdated;
  SpectrumValue m_rsInterferencePower;

  bool m_dataInterferencePowerUpdated;
  SpectrumValue m_dataInterferencePower;

  bool m_pssReceived;
  struct NistPssElement
  {
    uint16_t cellId;
    double pssPsdSum;
    uint16_t nRB;
  };
  std::list <NistPssElement> m_pssList;

  /**
   * The `RsrqUeMeasThreshold` attribute. Receive threshold for PSS on RSRQ
   * in dB.
   */
  double m_pssReceptionThreshold;

  /**
   * The `RsrpUeMeasThreshold` attribute. Receive threshold for RSRP
   * in dB.
   */
  double m_rsrpReceptionThreshold;
  

  /// Summary results of measuring a specific cell. Used for layer-1 filtering.
  struct NistUeMeasurementsElement
  {
    double rsrpSum;   ///< Sum of RSRP sample values in linear unit.
    uint8_t rsrpNum;  ///< Number of RSRP samples.
    double rsrqSum;   ///< Sum of RSRQ sample values in linear unit.
    uint8_t rsrqNum;  ///< Number of RSRQ samples.
  };

  /**
   * Store measurement results during the last layer-1 filtering period.
   * Indexed by the cell ID where the measurements come from.
   */
  std::map <uint16_t, NistUeMeasurementsElement> m_ueMeasurementsMap;
  /**
   * The `NistUeMeasurementsFilterPeriod` attribute. Time period for reporting UE
   * measurements, i.e., the length of layer-1 filtering (default 200 ms).
   */
  Time m_ueMeasurementsFilterPeriod;
  /// \todo Can be removed.
  Time m_ueMeasurementsFilterLast;

  Ptr<NistLteHarqPhy> m_harqPhyModule;

  uint32_t m_raPreambleId;
  uint32_t m_raRnti;

  /**
   * The `ReportCurrentCellRsrpSinr` trace source. Trace information regarding
   * RSRP and average SINR (see TS 36.214). Exporting cell ID, RNTI, RSRP, and
   * SINR.
   */
  TracedCallback<uint16_t, uint16_t, double, double> m_reportCurrentCellRsrpSinrTrace;
  /**
   * The `RsrpSinrSamplePeriod` attribute. The sampling period for reporting
   * RSRP-SINR stats.
   */
  uint16_t m_rsrpSinrSamplePeriod;
  uint16_t m_rsrpSinrSampleCounter;

  /**
   * The `ReportNistUeMeasurements` trace source. Contains trace information
   * regarding RSRP and RSRQ measured from a specific cell (see TS 36.214).
   * Exporting RNTI, the ID of the measured cell, RSRP (in dBm), RSRQ (in dB),
   * and whether the cell is the serving cell.
   */
  TracedCallback<uint16_t, uint16_t, double, double, bool> m_reportNistUeMeasurements;

  EventId m_sendSrsEvent;

  /**
   * The `UlPhyTransmission` trace source. Contains trace information regarding
   * PHY stats from UL Tx perspective. Exporting a structure with type
   * NistPhyTransmissionStatParameters.
   */
  TracedCallback<NistPhyTransmissionStatParameters> m_ulPhyTransmission;

  
  Ptr<SpectrumValue> m_noisePsd; ///< Noise power spectral density for
                                 ///the configured bandwidth 

  /**
   * The sidelink NrV2XSpectrumPhy associated to this NrV2XUePhy. 
   */
  Ptr<NrV2XSpectrumPhy> m_sidelinkSpectrumPhy;

  Ptr<SpectrumValue> m_slNoisePsd; ///< Noise power spectral density for
                                 ///the configured bandwidth 

  /**
   * The V2X sidelink NrV2XSpectrumPhy associated to this NrV2XUePhy. 
   */
  Ptr<NrV2XSpectrumPhy> m_V2XSidelinkSpectrumPhy;

  Ptr<SpectrumValue> m_V2XSlNoisePsd; ///< Noise power spectral density for
                                 ///the configured bandwidth 


  struct SidelinkGrant
  {
    //fields common with SL_DCI
    uint16_t m_rnti;
    uint16_t m_resPscch;
    uint8_t m_tpc;
    uint8_t m_hopping;
    uint8_t m_rbStart; //models rb assignment
    uint8_t m_rbLen;   //models rb assignment
    uint8_t m_trp;
    uint8_t m_groupDstId;

    //other fields
    uint8_t m_mcs;
    uint32_t m_tbSize;

    uint32_t frameNo;
    uint32_t subframeNo;
  };


//TODO FIXME NEW for V2X V2X V2X V2X V2X V2X V2X V2X V2X V2X V2X V2X V2X V2X V2X V2X V2X V2X V2X V2X V2X V2X V2X V2X V2X V2X V2X V2X V2X
 /*Subchannelization scheme*/
 uint16_t m_nsubCHsize;
 uint16_t m_BW_RBs;
 double m_slotDuration;
 uint16_t m_SCS;
 double m_rxSensitivity;
 double m_RSSIthresh;
 bool m_IBE;

 uint16_t m_currentMCS;

 struct V2XSidelinkGrant
  {
    //fields common with SL_DCI
    uint16_t m_rnti;

    uint32_t frameNo;
    uint32_t subframeNo;
    
    uint8_t m_rbStartPssch; //models rb assignment
    uint8_t m_rbLenPssch;   //models rb assignment

    uint8_t m_rbStartPscch; //models rb assignment
    uint8_t m_rbLenPscch;   //models rb assignment
    //uint8_t m_trp;
    uint8_t m_groupDstId; //FIXME this should not be present for V2X sidelink

    //other fields
    uint8_t m_mcs;
    uint32_t m_tbSize;
  };

  struct SidelinkGrantInfo
  {
    SidelinkGrant m_grant;
    std::list<SidelinkCommResourcePool::SidelinkTransmissionInfo> m_pscchTx;//list of PSCCH transmissions within the pool
    std::list<SidelinkCommResourcePool::SidelinkTransmissionInfo> m_psschTx; //list of PSSCH transmissions within the pool
    bool m_grant_received;
  };

 
//TODO FIXME NEW for V2X
  struct V2XSidelinkGrantInfo
  {
    V2XSidelinkGrant m_grant;
    std::list<SidelinkCommResourcePool::V2XSidelinkTransmissionInfo> m_v2xTx; //list of V2X PSSCH and PSCCH transmissions within the pool
    bool m_v2x_grant_received;
  };

  
  struct TxPacketInfo
  {
    double txTime;
    double genTime;
    uint32_t nodeId;
    uint64_t packetId;
    uint16_t txIndex;
    uint32_t Cresel;
    uint16_t RRI;
    uint16_t psschRbStart;
    uint16_t psschRbLen;
    uint16_t psschRbLenTb;
    uint16_t pscchRbStart;
    uint16_t pscchRbLen;
    SidelinkCommResourcePool::SubframeInfo txFrame;
  };

  static std::vector<TxPacketInfo> txPackets;
  static double prevPrintTime;

  struct PoolInfo
  {
    Ptr<SidelinkCommResourcePool> m_pool; //the pool
    SidelinkCommResourcePool::SubframeInfo m_currentScPeriod; //start of current period
    SidelinkCommResourcePool::SubframeInfo m_nextScPeriod; //start of next period

    uint32_t m_npscch; // number of PSCCH available in the pool

    //SidelinkGrant m_currentGrant; //grant for the current SC period
    std::map<uint16_t, SidelinkGrantInfo> m_currentGrants;
    std::map<uint16_t, V2XSidelinkGrantInfo> m_currentV2XGrants;
  };

  PoolInfo m_slTxPoolInfo;
  std::list <PoolInfo> m_sidelinkRxPools;
  std::list <uint32_t> m_destinations;
  
  //discovery
  struct DiscGrant
  {
    uint16_t m_rnti;
    uint8_t m_resPsdch;
  };

  struct DiscGrantInfo
  {
    DiscGrant m_grant;
    std::list<SidelinkDiscResourcePool::SidelinkTransmissionInfo> m_psdchTx;//list of PSDCH transmissions within the pool
    bool m_grant_received;
  };

  struct DiscPoolInfo
  {
    Ptr<SidelinkDiscResourcePool> m_pool; //the pool
    SidelinkDiscResourcePool::SubframeInfo m_currentDiscPeriod; //start of current period 
    SidelinkDiscResourcePool::SubframeInfo m_nextDiscPeriod; //start of next period
    uint32_t m_npsdch; // number of PSDCH available in the pool
    std::map<uint16_t, DiscGrantInfo> m_currentGrants;
  };

  DiscPoolInfo m_discTxPools;
  std::list <DiscPoolInfo> m_discRxPools;
  
  uint8_t m_discResPsdch ;

  std::list<uint32_t> m_discTxApps;
  std::list<uint32_t> m_discRxApps;

 //TODO FIXME NEW for V2X V2X V2X V2X V2X V2X V2X V2X V2X V2X V2X V2X V2X V2X 


  struct RSSImeas
   {
     std::vector<uint16_t> m_IDs;
     double m_rssi;
   };


  double m_CBRCheckingInterval;
  double m_CBRCheckingPeriod;

  struct CBRInfo
  {
    uint32_t nodeId;
    double CBRvalue;
    double time;
  };

  static std::vector<CBRInfo> CBRValues;
  static double CBRSavingInterval;

  std::map <uint16_t,std::map<SidelinkCommResourcePool::SubframeInfo, RSSImeas> > m_receivedRSSI;

  void UnimoreEvaluateCBR (uint16_t frameNo, uint16_t subframeNo);

  std::string m_outputPath;
  double m_savingPeriod;

  /**
   * Summary results of measuring a specific SyncRef. Used for layer-1 filtering.
   */
  struct NistUeSlssMeasurementsElement
  {
    double srsrpSum;   ///< Sum of S-RSRP sample values in linear unit.
    uint8_t srsrpNum;  ///< Number of S-RSRP samples.
  };
  /**
   * Stores the S-RSRP information of the SyncRefs detected during the scanning process,
   * indexed by the SLSSID of the SyncRef and the offset it uses for transmitting the SLSSs
   */
  std::map <std::pair<uint16_t,uint16_t>, NistUeSlssMeasurementsElement> m_ueSlssDetectionMap;
  /**
   * Represents the S-RSRP measurement schedule for the current measurement process.
   * It is used for knowing when the UE needs to take samples of the detected SyncRefs S-RSRP.
   * The index is the simulation time and the elements are the SyncRef identifiers (SLSSID and offset)
   */
  std::map <int64_t, std::pair<uint16_t,uint16_t> > m_ueSlssMeasurementsSched;
  /**
   * Stores the S-RSRP information of the SyncRefs in measurement during the measurement process
   * indexed by the SLSSID of the SyncRef and the offset it uses for transmitting the SLSSs
   */
  std::map <std::pair<uint16_t,uint16_t>, NistUeSlssMeasurementsElement> m_ueSlssMeasurementsMap;
  /**
   * Time period for searching/scanning to detect available SyncRefs (supporting SyncRef selection)
   */
  Time m_ueSlssScanningPeriod;
  /**
   * Time period for taking S-RSRP samples of the detected SyncRefs (supporting SyncRef selection)
   */
  Time m_ueSlssMeasurementPeriod;
  /**
   * Time period for taking S-RSRP samples of the selected SyncRef (supporting SLSS transmission decision)
   */
  Time m_ueSlssEvaluationPeriod; //How long the UE will evaluate the selected SyncRef for determine cease/initiation of SLSS tx
  /**
   *  The number of samples the UE takes during the measurement and evaluation periods
   */
  uint16_t m_nSamplesSrsrpMeas;
  /**
   * Time for the first SyncRef selection period
   */
  Time m_tFirstScanning;
  /**
   * Random number generator used for determining the time between SyncRef selection processes
   */
  Ptr<UniformRandomVariable> m_nextScanRdm;
  /**
   * True if a SyncRef selection is in progress and the UE is performing the SyncRef search/scanning
   */
  bool m_ueSlssScanningInProgress;
  /**
   * True if a SyncRef selection is in progress and the UE is performing the S-RSRP measurements,
   * either of the detected SyncRefs (measurement sub-process) or the selected one (evaluation sub-process)
   */
  bool m_ueSlssMeasurementInProgress;
  /**
   * Number of S-RSRP measurement periods already performed in SyncRef selection process in progress
   * (1 = measurement, 2 = measurement + evaluation)
   */
  uint16_t m_currNMeasPeriods;
  /**
   * The minimum S-RSRP value for considering a SyncRef S-RSRP sample valid
   */
  double m_minSrsrp;
  /**
   * Stores the received MIB-SL during the SyncRef search/scanning.
   * Used to determine the detected SyncRefs, as SyncRefs with valid S-RSRP but without
   * successfully decoded/received MIB-SL are not considered detected
   */
  std::map <std::pair<uint16_t,uint16_t>, NistLteRrcSap::MasterInformationBlockSL> m_detectedMibSl;
  /**
   * Initial frame number
   */
  uint32_t m_initFrameNo;
  /**
   * Initial subframe number
   */
  uint32_t m_initSubframeNo;
  /**
   * Current frame number
   */
  uint32_t m_currFrameNo;
  /**
   * Current subframe number
   */
  uint32_t m_currSubframeNo;
  /**
   * Configuration needed for the timely change of subframe indication upon synchronization to
   * a different SyncRef
   */
  struct ResyncParams
  {
    uint16_t newSubframeNo;
    uint16_t newFrameNo;
    NistLteRrcSap::MasterInformationBlockSL syncRefMib;
    uint16_t offset;
  };
  /**
   * Parameters to be used for the change of subframe indication upon synchronization to
   * a different SyncRef
   */
  ResyncParams m_resyncParams;
  /**
   * True if the RRC instructed to synchronize to a different SyncRef
   */
  bool m_resyncRequested;
  /**
   * True if the UE changed of timing (synchronized to a different SyncRef) and have to
   * wait the start of a new sideliink communication period for transmitting the data
   * (the subframe indication changed and the data indication in the SCI of the current
   * period is not valid anymore)
   */
  bool m_waitingNextScPeriod;

  /**
   * Schedules the first call of the function SubframeIndication with the appropriate
   * values for its parameters: frameNo and subframeNo
   * \param rdm when true the values of frameNo and subframeNo are selected randomly,
   *            when false, frameNo=1 and subframeNo=1
   */
  void SetInitialSubFrameIndication(bool rdm);
  /**
   * Set the upper limit for the random values generated by m_nextScanRdm
   * \param t the upper limit for m_nextScanRdm
   */
  void SetUeSlssInterScanningPeriodMax(Time t);
  /**
   * Set the lower limit for the random values generated by m_nextScanRdm
   * \param the lower limit for m_nextScanRdm
   */
  void SetUeSlssInterScanningPeriodMin(Time t);
  /**
   * Notify the start of a new SyncRef selection process, starting with the
   * SyncRef search/scanning
   */
  void StartSlssScanning ();
  /**
   * Notify the end of the SyncRef search/scanning,
   * keep only the six detected SyncRef (with received MIB-SL) with highest S-RSRP,
   * create the S-RSRP measurement schedule for each of them and start the measurement
   * process
   */
  void EndSlssScanning ();

  /**
   * Notify the start of a S-RSRP measurement process.
   * The S-RSRP measurement is used for two sub-processes:
   * 1. Measurement: collect the S-RSRP samples of the detected SyncRefs for determining
   *    the suitable SyncRef to select and synchronize with, and
   * 2. Evaluation: collect the S-RSRP of the selected SyncRef (if any) to determine if the UE needs
   *    to become itself a SyncRef and start transmitting SLSS
   * \param slssid the SLSSID of the selected SyncRef if the function is called for Evaluation,
   *               0 if it is called for Measurement
   * \param offset the offset in which the selected SyncRef sends SLSS if the function is called
   *               for Evaluation, 0 if it is called for Measurement
   */
  void StartSlssMeasurements (uint64_t slssid, uint16_t offset);
  /**
   * Perform L1 filtering of the S-RSRP samples collected during the measurement process
   * for each SyncRef, and report them to the RRC
   * \param slssid the SLSSID of the selected SyncRef if the measurement process was called for
   *               Evaluation, 0 if was is called for Measurement
   * \param offset the offset in which the selected SyncRef sends SLSS if the process was called
   *               for Evaluation, 0 if it was called for Measurement
   */
  void ReportSlssMeasurements (uint64_t slssid,uint16_t offset);
  /**
   * Schedule the next SyncRef selection process.
   * The function is called at the end of the SyncRef selection process in progress
   * \param endOfPrevious indicates after which sub-process the SyncRef selection process ended:
   *                      0 if it ended after scanning, 1 if it ended after measurement,
   *                      or 2 if it ends after evaluation
   */
  void ScheduleNextSyncRefReselection(uint16_t endOfPrevious);
  /**
   * Apply the change of timing (change of frame/subframe indication) when appropriate.
   * The change of timing is instructed when the UE selected and wants to synchronize to a given
   * SyncRef. The change is applied immediately upon resynchronization request if the UE is
   * not transmitting sidelink communication at the moment. Otherwise, the change is delayed until
   * the end of the current sidelink communication period to avoid the loss of already scheduled
   * transmissions (the subframe indication will change, and the data indication in the SCI
   * of the current period will not be valid anymore).
   * \param frameNo the current frame number
   * \param subframeNo the current subframe number
   */
  bool ChangeOfTiming(uint32_t frameNo, uint32_t subframeNo);

  // UE CPHY SAP methods related to synchronization
  // The RRC set the SLSSID value for lower layers
  void DoSetSlssId(uint64_t slssid);
  // The RRC instructs the PHY to send a MIB-SL in the PSBCH
  void DoSendSlss (NistLteRrcSap::MasterInformationBlockSL mibSl);
  // The RRC instructs the PHY to synchronize to a given SyncRef and apply the corresponding change of timing
  void DoSynchronizeToSyncRef (NistLteRrcSap::MasterInformationBlockSL mibSl);

}; // end of `class NrV2XUePhy`


}

#endif /* NR_V2X_UE_PHY_H */
