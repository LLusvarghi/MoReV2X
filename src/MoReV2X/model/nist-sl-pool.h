/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * This software was developed at the National Institute of Standards and
 * Technology by employees of the Federal Government in the course of
 * their official duties. Pursuant to titleElement 17 Section 105 of the United
 * States Code this software is not subject to copyright protection and
 * is in the public domain.
 * NIST assumes no responsibility whatsoever for its use by other parties,
 * and makes no guarantees, expressed or implied, about its quality,
 * reliability, or any other characteristic.
 * 
 * We would appreciate acknowledgement if the software is used.
 * 
 * NIST ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS" CONDITION AND
 * DISCLAIM ANY LIABILITY OF ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING
 * FROM THE USE OF THIS SOFTWARE.
 * 
 * Modified by: NIST
 * It was tested under ns-3.22
 */

#ifndef NIST_SL_POOL_H
#define NIST_SL_POOL_H

#include <map>
#include "nist-lte-rrc-sap.h"

namespace ns3 {
    
  /*
   * Class describing a resource pool for sidelink communication
   */
  class SidelinkCommResourcePool : public Object
  {
  public:
    /** types of sidelink pool */
    enum SlPoolType
    {
      UNKNOWN, 
      SCHEDULED,
      UE_SELECTED
    };

    /** Identify the location of a subframe by its frame number and subframe number */
    struct SubframeInfo {
      uint32_t frameNo; //!<The frame number
      uint32_t subframeNo; //!<The subframe number

      /**
       * Adds two subframe locations and return the new location
       * This is used for computing the absolute subframe location from a starting point
       * \param lhs One of the subframe location
       * \param rhs The other subframe location
       * \return The new subframe location
       */
      friend SubframeInfo operator+(const SubframeInfo& lhs, const SubframeInfo& rhs)
      {
        SubframeInfo res;
        uint32_t tmp1 = 10 * (lhs.frameNo % 1024) + lhs.subframeNo % 10;
        tmp1 += 10 * (rhs.frameNo % 1024) + rhs.subframeNo % 10;
        res.frameNo = (tmp1 / 10) % 1024;
        res.subframeNo = tmp1 % 10;
        return res;
      }

     friend SubframeInfo operator-(const SubframeInfo& lhs, const SubframeInfo& rhs)
      {
        SubframeInfo res;
        uint32_t tmp1 = 10 * (lhs.frameNo % 1024) + lhs.subframeNo % 10;
        uint32_t tmp2 = 10 * (rhs.frameNo % 1024) + rhs.subframeNo % 10;
        uint32_t tmpRes = 2;
        if (tmp1 >= tmp2)
          {
             tmpRes = tmp1 - tmp2; // the two SF locations belong to the same SFN cycle

          }
        else 
          {
             tmpRes = tmp1 + (10249 - tmp2);

          }
        res.subframeNo = tmpRes % 10;
        res.frameNo = (tmpRes / 10) % 1024;
        
        return res;
      }
      
      /**
       * Checks if two subframe locations are identical
       * \param lhs One of the subframe location
       * \param rhs The other subframe location
       * \return true if the locations represent the same subframe
       */
      friend bool operator==(const SubframeInfo& lhs, const SubframeInfo& rhs)
      {
        return lhs.frameNo == rhs.frameNo && lhs.subframeNo == rhs.subframeNo;
      }

      /**
       * Checks if a subframe location is smaller that another subframe location
       * \param lhs One of the subframe location
       * \param rhs The other subframe location
       * \return true if the first argument represent a subframe that will happen sooner
       */
      friend bool operator<(const SubframeInfo& lhs, const SubframeInfo& rhs)
      {
        return lhs.frameNo < rhs.frameNo || (lhs.frameNo == rhs.frameNo && lhs.subframeNo < rhs.subframeNo);
      }
    };

    /** Indicates the location of a transmission on the sidelink in time and frequency **/
    struct SidelinkTransmissionInfo {
      SubframeInfo subframe; //!<The time of the transmission
      uint8_t rbStart; //!<The index of the PRB where the transmission starts
      uint8_t nbRb; //!<The number of PRBs used by the transmission 
    };

////TODO FIXME NEW
  /** Indicates the location of a transmission on the V2X sidelink in time and frequency **/
    struct V2XSidelinkTransmissionInfo {
      SubframeInfo subframe; //!<The time of the transmission
      uint16_t rbStartPssch; //!<The index of the PRB where the transmission starts
      uint16_t nbRbPssch; //!<The number of PRBs used by the PSSCH transmission
      uint16_t rbStartPscch; //!<The index of the PRB where the PSCCH transmission starts
      uint16_t nbRbPscch; //!<The number of PRBs used by the PSCCH transmission
      /* The number of PRBs used by the PSCCH transmission in one slot is always 2 */
      bool isThisAReTx; //!<Indicates whether this transmission refers to the retransmission

    };

    SidelinkCommResourcePool (void);
    virtual ~SidelinkCommResourcePool (void);
    static TypeId GetTypeId (void);

    /**
     * Checks if two sidelink pool configurations are identical
     * \param other The configuration of the other resource pool
     * \return true if this configuration is the same as the other one
     */
    bool operator==(const SidelinkCommResourcePool& other);
    
    /**
     * Configure the pool using the content of the RRC message
     * Parsing the message will indicate whether it is a scheduled or UE selected pool
     * \param pool The message containing pool information
     */
    void SetPool (NistLteRrcSap::SlCommResourcePool pool);

    /**
     * Configure the pool using the content of the preconfigured pool
     * Parsing the message will indicate whether it is a scheduled or UE selected pool
     * \param pool The message containing pool information
     */
    void SetPool (NistLteRrcSap::SlPreconfigCommPool pool);

    /**
     * Returns the type of scheduling 
     * \return the type of scheduling    
     */
    SlPoolType GetSchedulingType ();

    /**
     * Determines whether Release 14 V2X operation is enabled 
     * //Added by L. Gibellini //TODO
     * \return true if V2X is enabled, false otherwise    
     */
    bool IsV2XEnabled ();

    /**
     * Determines the start of the current SC period 
     * \param frameNo the frame number
     * \param subframeNo the subframe number
     * \return The start of the current sidelink period where the given subframe is located
     */
    SubframeInfo GetCurrentScPeriod (uint32_t frameNo, uint32_t subframeNo);

    /**
     * Determines the start of the next SC period 
     * \param frameNo the frame number
     * \param subframeNo the subframe number
     * \return The start of the next sidelink period where the given subframe is located
     */
    SubframeInfo GetNextScPeriod (uint32_t frameNo, uint32_t subframeNo);

    std::list<SidelinkCommResourcePool::V2XSidelinkTransmissionInfo> GetV2XSlTransmissions (uint32_t frameNo, uint32_t subframeNo, uint16_t rbStartPscch, uint16_t rbStartPssch, uint16_t nbRbPssch, uint16_t nbRbPscch, uint32_t subframeInitialTx, uint8_t SFGap);

    std::list<SidelinkCommResourcePool::V2XSidelinkTransmissionInfo> GetV2XSlTransmissions (uint32_t frameNo, uint32_t subframeNo, uint16_t rbStartPscch, uint16_t rbStartPssch, uint16_t nbRbPssch, uint16_t nbRbPscch, uint32_t frameNoNextTx, uint32_t subframeNoNextTx, uint8_t SFGap);

    /**
     * Returns the subframe and resource block allocations of the transmissions in 
     * PSCCH for the given resource
     * \param n the selected resource within the pool
     * \return The list of transmission information
     */
    std::list<SidelinkCommResourcePool::SidelinkTransmissionInfo> GetPscchTransmissions (uint32_t n);

    /**
     * Returns the list RBs that contains PSCCH opportunities in the current given subframe 
     * \param frameNo frame number
     * \param subframeNo subframe number
     * \return The list of RBs used by the control channel on the given subframe
     */
    std::list<uint8_t> GetPscchOpportunities (uint32_t frameNo, uint32_t subframeNo);

    /**
     * Returns the list of RBs to be used in the given subframe for the given opportunity
     * \param frameNo frame number
     * \param subframeNo subframe number
     * \param n resource selected in the pool
     * \return The list of RBs useed by the control channel on the given subframe and resource index
     */
    std::vector<int> GetPscchRbs (uint32_t frameNo, uint32_t subframeNo, uint32_t n);
   
    /**
     * Returns the number of PSCCH resources from the pool
     * \return the number of PSCCH resources from the pool
     */
    uint32_t GetNPscch ();
    
    /**
     * Returns the subframes and RBs associated with the transmission on PSSCH in the current period
     * \param itrp the repetition pattern from the SCI format 0 message
     * \param rbStart the index of the PRB where the transmission occurs
     * \param rbLen the length of the transmission
     * \return the subframes and RBs associated with the transmission on PSSCH
     */
    std::list<SidelinkCommResourcePool::SidelinkTransmissionInfo> GetPsschTransmissions (uint8_t itrp, uint8_t rbStart, uint8_t rbLen);

    /**
     * Returns the subframes and RBs associated with the transmission on PSSCH
     * \param periodStart The first subframe in the sidelink period
     * \param itrp the repetition pattern from the SCI format 0 message
     * \param rbStart the index of the PRB where the transmission occurs
     * \param rbLen the length of the transmission     
     * \return the subframes and RBs associated with the transmission on PSSCH
     */
    std::list<SidelinkCommResourcePool::SidelinkTransmissionInfo> GetPsschTransmissions (SubframeInfo periodStart, uint8_t itrp, uint8_t rbStart, uint8_t rbLen);

  protected:

    void Initialize ();

    /**
     * Translates the filtered subframes to subframe numbers relative to the start
     * of the SC period
     */
    SidelinkCommResourcePool::SidelinkTransmissionInfo TranslatePscch (SidelinkCommResourcePool::SidelinkTransmissionInfo info); 
    
    SlPoolType m_type; //!<The type of pool
    //FIXME NEW
    bool m_V2XEnabled; //!<Whether Release 14 V2X is enabled>
    
    //Common fields for all types of pools
    NistLteRrcSap::SlCpLen m_scCpLen; //!<cyclic prefix length of control channel
    NistLteRrcSap::SlPeriodComm m_scPeriod; //!<duration of the sidelink period
    NistLteRrcSap::SlTfResourceConfig m_scTfResourceConfig; //!<control pool information (subframes and PRBs)
    NistLteRrcSap::SlCpLen m_dataCpLen; //!<cyclic prefix for the shared channel
    NistLteRrcSap::SlHoppingConfigComm m_dataHoppingConfig; //!<frequency hopping parameters
    NistLteRrcSap::SlTrptSubset m_trptSubset; //!<normally only used for UE selected pools because it is assumed that all subframes are available in scheduled pools

    //Fields for UE selected pools
    NistLteRrcSap::SlTfResourceConfig m_dataTfResourceConfig; //!<shared channel pool information

  private:

    /**
     * Compute the frames/RBs that are part of the PSCCH pool
     */
    void ComputeNumberOfPscchResources ();
    /**
     * Compute the frame/RBS that are part of the PSSCH pool
     */
    void ComputeNumberOfPsschResources ();

    uint32_t m_lpscch;
    std::vector <uint32_t> m_lpscchVector; //list of subframes that belong to PSCCH pool
    uint32_t m_rbpscch;
    std::vector <uint32_t> m_rbpscchVector; //list of RBs that belong to PSCCH pool
    uint32_t m_nPscchResources; //number of resources in the PSCCH pools

    uint32_t m_lpssch;
    std::vector <uint32_t> m_lpsschVector; //list of subframes that belong to PSSCH pool
    uint32_t m_rbpssch;
    std::vector <uint32_t> m_rbpsschVector; //list of RBs that belong to PSSCH pool

    bool m_preconfigured; //indicates if the pool is preconfigured
  };

  /**
   * Class describing a resource pool for receiving sidelink communication
   */
  class SidelinkRxCommResourcePool :  public SidelinkCommResourcePool
  {
  public:
    static TypeId GetTypeId (void);

  protected:
    

  };

  /**
   * Class describing a resource pool for transmitting sidelink communication
   */
  class SidelinkTxCommResourcePool :  public SidelinkCommResourcePool
  {
  public:

    static TypeId GetTypeId (void);
   
    
    /**
     * Configure the pool using the content of the RRC message
     * \param pool The message containing the pool information
     */
    void SetPool (NistLteRrcSap::SlCommResourcePool pool);

    /**
     * Configure the pool using the content of the preconfigured pool
     * \param pool The message containing the pool information
     */
    void SetPool (NistLteRrcSap::SlPreconfigCommPool pool);

    /**
     * Set parameters for a scheduled pool
     * \param slrnti The sidelink RNTI assigned by the eNodeB
     * \param macMainConfig MAC layer configuration
     * \param commTxConfig The pool configuration
     * \param index associated with the resource pool 
     */
    void SetScheduledTxParameters (uint16_t slrnti, NistLteRrcSap::SlMacMainConfigSl macMainConfig, NistLteRrcSap::SlCommResourcePool commTxConfig, uint8_t index);

    /**
     * Set parameters for a scheduled pool
     * \param slrnti The sidelink RNTI assigned by the eNodeB
     * \param macMainConfig MAC layer configuration
     * \param commTxConfig The pool configuration
     * \param index associated with the resource pool 
     * \param mcs The MCS to be used for transmitting data
     */
    void SetScheduledTxParameters (uint16_t slrnti, NistLteRrcSap::SlMacMainConfigSl macMainConfig, NistLteRrcSap::SlCommResourcePool commTxConfig, uint8_t index, uint8_t mcs);

    /**
     * Returns the index of the resource pool
     * \return the index of the resource pool
     */
    uint8_t GetIndex ();

    /**
     * Returns the MCS to use
     * \return The MCS to use
     */
    uint8_t GetMcs ();
  protected:
    
    //Fields for UE selected pools
    NistLteRrcSap::SlTxParameters m_scTxParameters; //!<configuration of the control channel
    NistLteRrcSap::SlTxParameters m_dataTxParameters; //!<configuration of the shared channel

    //Fields for scheduled pools
    uint16_t m_slrnti; //!<sidelink RNTI 
    NistLteRrcSap::SlMacMainConfigSl m_macMainConfig; //!<MAC layer configuration
    NistLteRrcSap::SlCommResourcePool m_commTxConfig; //!<resource pool configuration
    bool m_haveMcs; //!<indicates if MCS is being set
    uint8_t m_mcs; //!<the MCS value to use if set (0..28)
    uint8_t m_index; //!<index to be used in BSR
    
  };
  
  /**
   * Class describing a resource pool for sidelink discovery
   */
  class SidelinkDiscResourcePool : public Object
  {
  public:
    
    /** types of sidelink pool */
    enum SlPoolType
    {
      UNKNOWN, 
      SCHEDULED,
      UE_SELECTED
    };
    
    /** Identify the location of a subframe by its frame number and subframe number */
    struct SubframeInfo {
      uint32_t frameNo; //!<The frame number
      uint32_t subframeNo; //!<The subframe number

      /**
       * Adds two subframe locations and return the new location
       * This is used for computing the absolute subframe location from a starting point
       * \param lhs One of the subframe location
       * \param rhs The other subframe location
       * \return The new subframe location
       */
      friend SubframeInfo operator+(const SubframeInfo& lhs, const SubframeInfo& rhs)
      {
        SubframeInfo res;
        uint32_t tmp1 = 10 * (lhs.frameNo % 1024) + lhs.subframeNo % 10;
        tmp1 += 10 * (rhs.frameNo % 1024) + rhs.subframeNo % 10;
        res.frameNo = tmp1 / 10;
        res.subframeNo = tmp1 % 10;
        return res;
      }

      /**
       * Checks if two subframe locations are identical
       * \param lhs One of the subframe location
       * \param rhs The other subframe location
       * \return true if the locations represent the same subframe
       */
      friend bool operator==(const SubframeInfo& lhs, const SubframeInfo& rhs)
      {
        return lhs.frameNo == rhs.frameNo && lhs.subframeNo == rhs.subframeNo;
      }

      /**
       * Checks if a subframe location is smaller that another subframe location
       * \param lhs One of the subframe location
       * \param rhs The other subframe location
       * \return true if the first argument represent a subframe that will happen sooner
       */
      friend bool operator<(const SubframeInfo& lhs, const SubframeInfo& rhs)
      {
        return lhs.frameNo < rhs.frameNo || (lhs.frameNo == rhs.frameNo && lhs.subframeNo < rhs.subframeNo);
      }
    };

    /** indicates the location of a discovery transmission on the sidelink */
    struct SidelinkTransmissionInfo {
      SubframeInfo subframe; //!<The time of the transmission
      uint8_t rbStart; //!<The index of the PRB where the transmission occurs
      uint8_t nbRb; //!<The number of PRBs used by the transmission
    };

    SidelinkDiscResourcePool (void);
    virtual ~SidelinkDiscResourcePool (void);
    static TypeId GetTypeId (void);
    
    /**
     * Configure the pool using the content of the RRC message
     * Parsing the message will indicate whether it is a scheduled or UE selected pool
     * \param pool discovery pool
     */
    void SetPool (NistLteRrcSap::SlDiscResourcePool pool);

    /**
     * Configure the pool using the content of the preconfigured pool
     * Parsing the message will indicate 
     * \param pool preconfigured discovery pool
     */
    void SetPool (NistLteRrcSap::SlPreconfigDiscPool pool);


    /**
     * Returns the type of scheduling 
     * \return the type of scheduling    
     */
    SlPoolType GetSchedulingType ();

    /**
     * Determines the start of the current discovery period 
     * \param frameNo the frame number
     * \param subframeNo the subframe number
     */
    SubframeInfo GetCurrentDiscPeriod (uint32_t frameNo, uint32_t subframeNo);

    /**
     * Determines the start of the next discovery period 
     * \param frameNo the frame number
     * \param subframeNo the subframe number
     */
    SubframeInfo GetNextDiscPeriod (uint32_t frameNo, uint32_t subframeNo);
    
    /**
     * Returns the number of PSDCH resources from the pool
     * \return the number of PSDCH resources from the pool
     */
    uint32_t GetNPsdch ();

    /**
     * Returns the number of subframes reserved to discovery
     * \return the number of subframes reserved to discovery
     */
    uint32_t GetNSubframes ();

    /**
     * Returns the number of resource block pairs
     * \return the number of resource block pairs
     */
    uint32_t GetNRbPairs ();

    /**
     * Returns the maximum number of retransmission
     * \return the maximum number of retransmission
     */
    uint8_t GetNumRetx ();

    /**
     * Returns discovery period
     * \return discovery period
     */
    int32_t GetDiscPeriod ();

    
    /**
     * Returns the subframes and RBs associated with the transmission on PSDCH
     * \param npsdch The index of the resource within the pool
     *
     */
    std::list<SidelinkDiscResourcePool::SidelinkTransmissionInfo> GetPsdchTransmissions (uint32_t npsdch);
   
    /**
     * Returns the list RBs that contains PSDCH opportunities in the current given subframe 
     * \param frameNo frame number
     * \param subframeNo subframe number
     * \return the list RBs that contains PSDCH opportunities in the current given subframe
     */
    std::list<uint8_t> GetPsdchOpportunities (uint32_t frameNo, uint32_t subframeNo);

  protected:

    void Initialize ();
   
    /**
     * Translates the filtered subframes to subframe numbers relative to the start discovery period
     */
    SidelinkDiscResourcePool::SidelinkTransmissionInfo TranslatePsdch (SidelinkDiscResourcePool::SidelinkTransmissionInfo info); 
    

    SlPoolType m_type; //!<The type of pool 
    
    //Common fields for all types of pools
    NistLteRrcSap::SlCpLen m_discCpLen; //!<cyclic prefix length
    NistLteRrcSap::SlPeriodDisc m_discPeriod; //!<duration of the discovery period
    uint8_t m_numRetx;  //!<number of retransmission
    uint32_t m_numRepetition; //!<number of repetition
    NistLteRrcSap::SlTfResourceConfig m_discTfResourceConfig; //!<resource pool information
    bool m_haveTxParameters; //!<indicates if the Tx parameters are present or not
    NistLteRrcSap::SlTxParameters m_txParametersGeneral; //!<Tx parameters to use if defined
    
  private:
    /**
     * Compute the frames/RBs that are part of the PSDCH pool
     * for ue-selected and scheduled cases
     */
    void ComputeNumberOfPsdchResources ();

    uint32_t m_lpsdch;
    std::vector <uint32_t> m_lpsdchVector; //list of subframes that belong to PSDCH pool
    uint32_t m_rbpsdch;
    std::vector <uint32_t> m_rbpsdchVector; //list of RBs that belong to PSDCH pool
    uint32_t m_nPsdchResources; //number of resources in the PSDCH pools
    bool m_preconfigured; //indicates if the pool is preconfigured
  };

  /**
   * Class describing a resource pool for receiving sidelink discovery (Monitor)
   */
  class SidelinkRxDiscResourcePool :  public SidelinkDiscResourcePool
  {
  public:
    static TypeId GetTypeId (void);

  protected:
    

  };

  /**
   * Class describing a resource pool for transmitting sidelink discovery (Announce)
   */
  class SidelinkTxDiscResourcePool :  public SidelinkDiscResourcePool
  {
  public:

    static TypeId GetTypeId (void);
   
    
    /**
     * Configure the pool 
     * \param pool discovery pool to be configured
     */
    void SetPool (NistLteRrcSap::SlDiscResourcePool pool);

    /**
     * Configure the pool using the content of the preconfigured pool
     * \param pool The preconfigured pool
     */
    void SetPool (NistLteRrcSap::SlPreconfigDiscPool pool);

    /**
     * Set parameters for scheduled config
     * \param discPool discovery resource pool
     * \param discResources resources in terms of resource blocks and subframes
     * \param discHopping frequency hopping configuration
     */
    void SetScheduledTxParameters (NistLteRrcSap::SlDiscResourcePool discPool, NistLteRrcSap::SlTfIndexPairList discResources, NistLteRrcSap::SlHoppingConfigDisc discHopping);

    /**
     * Returns the transmission probability
     * \return The transmission probability
     */
    uint32_t GetTxProbability ();

    /**
     * Sets the transmission probability
     * \param theta The transmission prbability (0.25, 0.5, 0.75, or 1)
     */
    void SetTxProbability ( uint32_t theta);

  protected:
    
    //Fields for UE selected pools
    NistLteRrcSap::PoolSelection m_poolSelection; //!<method for selecting the pool
    bool m_havePoolSelectionRsrpBased; //!<indicates if the pool selection is RSRP based
    NistLteRrcSap::PoolSelectionRsrpBased m_poolSelectionRsrpBased; //!<parameters for the RSRP based selection
    NistLteRrcSap::TxProbability m_txProbability;  //!<transmission probability
    bool m_txProbChanged; //!<indicates if the transmission probability has changed

    //Fields for scheduled pools
    NistLteRrcSap::SlDiscResourcePool m_discTxConfig; //!<resource configuration
    NistLteRrcSap::SlTfIndexPairList m_discTfIndexList; //!<index of the resource pool
    NistLteRrcSap::SlHoppingConfigDisc m_discHoppingConfigDisc; //!<frequency hopping configuration

  };

 

} // namespace ns3

#endif //NIST_SL_POOL_H
