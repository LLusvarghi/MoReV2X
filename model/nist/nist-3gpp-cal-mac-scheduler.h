/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
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
 *         Nicola Baldo <nbaldo@cttc.es>
 */

#ifndef NIST_3GPP_CAL_MAC_SCHEDULER_H
#define NIST_3GPP_CAL_MAC_SCHEDULER_H

#include <ns3/nist-ff-mac-csched-sap.h>
#include <ns3/nist-ff-mac-sched-sap.h>
#include <ns3/nist-ff-mac-scheduler.h>
#include <vector>
#include <map>
#include <ns3/nist-lte-common.h>
#include <ns3/nist-lte-amc.h>
#include <ns3/nist-lte-ffr-sap.h>

#define HARQ_PROC_NUM 8
#define HARQ_DL_TIMEOUT 11

namespace ns3 {


  typedef std::vector < uint8_t > DlHarqProcessesNistStatus_t; //!< status of the downlink HARQ processes
typedef std::vector < uint8_t > DlHarqProcessesTimer_t;
typedef std::vector < NistDlDciListElement_s > DlHarqProcessesDciBuffer_t;
typedef std::vector < std::vector <struct NistRlcPduListElement_s> > RlcPduList_t; // vector of the LCs and layers per UE
typedef std::vector < RlcPduList_t > DlHarqRlcPduListBuffer_t; // vector of the 8 HARQ processes per UE

typedef std::vector < NistUlDciListElement_s > UlHarqProcessesDciBuffer_t;
  typedef std::vector < uint8_t > UlHarqProcessesNistStatus_t; //!< status of the uplink HARQ processes




/**
 * \ingroup ff-api
 * \brief Implements the SCHED SAP and CSCHED SAP for a Round Robin scheduler
 *
 * This class implements the interface defined by the NistFfMacScheduler abstract class
 */

class Nist3GPPcalMacScheduler : public NistFfMacScheduler
{
public:
  /**
   * \brief Constructor
   *
   * Creates the MAC Scheduler interface implementation
   */
  Nist3GPPcalMacScheduler ();

  /**
   * Destructor
   */
  virtual ~Nist3GPPcalMacScheduler ();

  // inherited from Object
  virtual void DoDispose (void);
  static TypeId GetTypeId (void);

  // inherited from NistFfMacScheduler
  virtual void SetNistFfMacCschedSapUser (NistFfMacCschedSapUser* s);
  virtual void SetNistFfMacSchedSapUser (NistFfMacSchedSapUser* s);
  virtual NistFfMacCschedSapProvider* GetNistFfMacCschedSapProvider ();
  virtual NistFfMacSchedSapProvider* GetNistFfMacSchedSapProvider ();

  // FFR SAPs
  virtual void SetNistLteFfrSapProvider (NistLteFfrSapProvider* s);
  virtual NistLteFfrSapUser* GetNistLteFfrSapUser ();

  friend class NistCalSchedulerMemberCschedSapProvider;
  friend class NistCalSchedulerMemberSchedSapProvider;

  /**
   * Updates the UE transmission mode
   * \param rnti The UE RNTI value
   * \param txMode The new transmission mode
   */
  void TransmissionModeConfigurationUpdate (uint16_t rnti, uint8_t txMode);

private:
  //
  // Implementation of the CSCHED API primitives
  // (See 4.1 for description of the primitives)
  //

  void DoCschedCellConfigReq (const struct NistFfMacCschedSapProvider::NistCschedCellConfigReqParameters& params);

  void DoCschedNistUeConfigReq (const struct NistFfMacCschedSapProvider::NistCschedNistUeConfigReqParameters& params);

  void DoCschedLcConfigReq (const struct NistFfMacCschedSapProvider::NistCschedLcConfigReqParameters& params);

  void DoCschedLcReleaseReq (const struct NistFfMacCschedSapProvider::NistCschedLcReleaseReqParameters& params);

  void DoCschedUeReleaseReq (const struct NistFfMacCschedSapProvider::NistCschedUeReleaseReqParameters& params);

  //
  // Implementation of the SCHED API primitives
  // (See 4.2 for description of the primitives)
  //

  void DoSchedDlRlcBufferReq (const struct NistFfMacSchedSapProvider::NistSchedDlRlcBufferReqParameters& params);

  void DoSchedDlPagingBufferReq (const struct NistFfMacSchedSapProvider::NistSchedDlPagingBufferReqParameters& params);

  void DoSchedDlMacBufferReq (const struct NistFfMacSchedSapProvider::NistSchedDlMacBufferReqParameters& params);

  void DoSchedDlTriggerReq (const struct NistFfMacSchedSapProvider::NistSchedDlTriggerReqParameters& params);

  void DoSchedDlRachInfoReq (const struct NistFfMacSchedSapProvider::NistSchedDlRachInfoReqParameters& params);

  void DoSchedDlCqiInfoReq (const struct NistFfMacSchedSapProvider::NistSchedDlCqiInfoReqParameters& params);

  void DoSchedUlTriggerReq (const struct NistFfMacSchedSapProvider::NistSchedUlTriggerReqParameters& params);

  void DoSchedUlNoiseInterferenceReq (const struct NistFfMacSchedSapProvider::NistSchedUlNoiseInterferenceReqParameters& params);

  void DoSchedUlSrInfoReq (const struct NistFfMacSchedSapProvider::NistSchedUlSrInfoReqParameters& params);

  void DoSchedUlMacCtrlInfoReq (const struct NistFfMacSchedSapProvider::NistSchedUlMacCtrlInfoReqParameters& params);

  void DoSchedUlCqiInfoReq (const struct NistFfMacSchedSapProvider::NistSchedUlCqiInfoReqParameters& params);


  int GetRbgSize (int dlbandwidth);

  static bool SortRlcBufferReq (NistFfMacSchedSapProvider::NistSchedDlRlcBufferReqParameters i,NistFfMacSchedSapProvider::NistSchedDlRlcBufferReqParameters j);

  void RefreshDlCqiMaps (void);
  void RefreshUlCqiMaps (void);

  void UpdateDlRlcBufferInfo (uint16_t rnti, uint8_t lcid, uint16_t size);
  void UpdateUlRlcBufferInfo (uint16_t rnti, uint16_t size);

  /**
  * \brief Update and return a new process Id for the RNTI specified
  *
  * \param rnti the RNTI of the UE to be updated
  * \return the process id  value
  */
  uint8_t UpdateHarqProcessId (uint16_t rnti);

  /**
  * \brief Return the availability of free process for the RNTI specified
  *
  * \param rnti the RNTI of the UE to be updated
  * \return the process id  value
  */
  uint8_t HarqProcessAvailability (uint16_t rnti);

  /**
  * \brief Refresh HARQ processes according to the timers
  *
  */
  void RefreshHarqProcesses ();

  Ptr<NistLteAmc> m_amc;

  /*
   * Vectors of UE's RLC info
  */
  std::list <NistFfMacSchedSapProvider::NistSchedDlRlcBufferReqParameters> m_rlcBufferReq;

  /*
  * Map of UE's DL CQI P01 received
  */
  std::map <uint16_t,uint8_t> m_p10CqiRxed;
  /*
  * Map of UE's timers on DL CQI P01 received
  */
  std::map <uint16_t,uint32_t> m_p10CqiTimers;

  /*
  * Map of previous allocated UE per RBG
  * (used to retrieve info from UL-CQI)
  */
  std::map <uint16_t, std::vector <uint16_t> > m_allocationMaps;

  /*
  * Map of UEs' UL-CQI per RBG
  */
  std::map <uint16_t, std::vector <double> > m_ueCqi;
  /*
  * Map of UEs' timers on UL-CQI per RBG
  */
  std::map <uint16_t, uint32_t> m_ueCqiTimers;



  /*
  * Map of UE's buffer status reports received
  */
  std::map <uint16_t,uint32_t> m_ceBsrRxed;

  // MAC SAPs
  NistFfMacCschedSapUser* m_cschedSapUser;
  NistFfMacSchedSapUser* m_schedSapUser;
  NistFfMacCschedSapProvider* m_cschedSapProvider;
  NistFfMacSchedSapProvider* m_schedSapProvider;

  // FFR SAPs
  NistLteFfrSapUser* m_ffrSapUser;
  NistLteFfrSapProvider* m_ffrSapProvider;

  // Internal parameters
  NistFfMacCschedSapProvider::NistCschedCellConfigReqParameters m_cschedCellConfig;

  uint16_t m_nextRntiDl; // RNTI of the next user to be served next scheduling in DL
  uint16_t m_nextRntiUl; // RNTI of the next user to be served next scheduling in UL

  uint32_t m_cqiTimersThreshold; // # of TTIs for which a CQI canbe considered valid

  std::map <uint16_t,uint8_t> m_uesTxMode; // txMode of the UEs
  


  // HARQ attributes
  /**
  * m_harqOn when false inhibit te HARQ mechanisms (by default active)
  */
  bool m_harqOn;
  std::map <uint16_t, uint8_t> m_dlHarqCurrentProcessId;
  //HARQ status
  // 0: process Id available
  // x>0: process Id equal to `x` trasmission count
  std::map <uint16_t, DlHarqProcessesNistStatus_t> m_dlHarqProcessesNistStatus;
  std::map <uint16_t, DlHarqProcessesTimer_t> m_dlHarqProcessesTimer;
  std::map <uint16_t, DlHarqProcessesDciBuffer_t> m_dlHarqProcessesDciBuffer;
  std::map <uint16_t, DlHarqRlcPduListBuffer_t> m_dlHarqProcessesRlcPduListBuffer;
  std::vector <NistDlInfoListElement_s> m_dlInfoListBuffered; // HARQ retx buffered

  std::map <uint16_t, uint8_t> m_ulHarqCurrentProcessId;
  //HARQ status
  // 0: process Id available
  // x>0: process Id equal to `x` trasmission count
  std::map <uint16_t, UlHarqProcessesNistStatus_t> m_ulHarqProcessesNistStatus;
  std::map <uint16_t, UlHarqProcessesDciBuffer_t> m_ulHarqProcessesDciBuffer;


  // RACH attributes
  std::vector <struct NistRachListElement_s> m_rachList;
  std::vector <uint16_t> m_rachAllocationMap;
  uint8_t m_ulGrantMcs; // MCS for UL grant (default 0)
};

} // namespace ns3

#endif // NIST_3GPP_CAL_MAC_SCHEDULER_H
