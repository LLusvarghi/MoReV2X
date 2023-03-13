/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This software was developed at the National Institute of Standards and
 * Technology by employees of the Federal Government in the course of
 * their official duties. Pursuant to titleElement 17 Section 105 of the United
 * States Code this software is not subject to copyright protection and
 * is in the public domain.
 * NIST assumes no responsibility whatsoever for its use by other parties,
 * and makes no guarantees, expressed or implied, about its quality,
 * reliability, or any other characteristic.

 * We would appreciate acknowledgement if the software is used.

 * NIST ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS" CONDITION AND
 * DISCLAIM ANY LIABILITY OF ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING
 * FROM THE USE OF THIS SOFTWARE.
 *
 * Modified by: NIST
 * Modified by: Luca Lusvarghi <luca.lusvarghi5@unimore.it>
 */

#ifndef NR_V2X_PHY_ERROR_MODEL_H
#define NR_V2X_PHY_ERROR_MODEL_H
#include <stdint.h>
#include <ns3/nist-lte-harq-phy.h>
#include "ns3/random-variable-stream.h"
namespace ns3 {

  const uint16_t XTABLE_SIZE = 3; //!<number of element in the X tables
  const uint16_t PUSCH_AWGN_SIZE = 33; //!<number of BLER values per HARQ transmission
  const uint16_t PSDCH_AWGN_SIZE = 23; //!<number of BLER values per HARQ transmission
  const uint16_t PSCCH_AWGN_SIZE = 38; //!<number of BLER values 
  const uint16_t PSBCH_AWGN_SIZE = 43; //!<number of BLER values
  const uint16_t PSSCH_CDL_SIZE = 46; //!<number of BLER values
  const uint16_t PSCCH_CDL_SIZE = 41; //!<number of BLER values

  const uint16_t Ericsson_PSSCH_CDL_SIZE = 16; //!<number of BLER values
  const uint16_t Ericsson_PSCCH_CDL_SIZE = 31; //!<number of BLER values

  const uint16_t Huawei_PSSCH_CDL_SIZE = 8; //!<number of BLER values

  const uint16_t Alejandro_TB_SIZE = 26; //!<number of BLER values
  const uint16_t Alejandro_SCI_SIZE = 28; //!<number of BLER values

  /**
   * Structure to report transport block error rate  value and computed SINR
   */
  struct NistTbErrorStats_t
  {
    double tbler; //!< block error rate
    double sinr; //!< sinr value
  };

/**
  * This class contains functions to access the BLER model
  */
class NrV2XPhyErrorModel
{

public:
  
  /**
   * List of possible channels
   */
  enum NistLtePhyChannel {
    PDCCH,
    PDSCH,
    PUCCH,
    PUSCH,
    PSCCH,
    PSSCH,
    PSDCH
  };

  /**
   * List of channel models. Note that not all models are available for each physical channel
   */
  enum NistLteFadingModel {
    AWGN
  };

  /**
   * List of transmission mode. Note that not all transmission modes are available for each physical channel or fading model
   */
  enum NistLteTxMode {
    SISO,
    SIMO,
    TxDiversity,
    SpatMultplex
  };

/**
   * \brief Lookup the BLER for the given SINR
   * \param fadingChannel The channel to use
   * \param txmode The Transmission mode used
   * \param mcs The MCS of the TB
   * \param sinr The mean sinr of the TB
   * \param harqHistory The HARQ information
   * \param SCS the OFDM subcarrier spacing
   * \param NPRB number of occupied physical resource blocks
   */
  // V2V
  static NistTbErrorStats_t GetV2VPsschBler (uint16_t mcs, double sinr, HarqProcessInfoList_t harqHistory, bool LOS, uint16_t SCS, uint32_t NPRB);
  static NistTbErrorStats_t GetV2VPscchBler (uint16_t mcs, double sinr, HarqProcessInfoList_t harqHistory, bool LOS, uint16_t SCS, uint32_t NPRB);

  static NistTbErrorStats_t GetNrV2XPsschBler (uint16_t mcs, double sinr, bool LOS, uint16_t SCS, double RelativeSpeed);
  static NistTbErrorStats_t GetNrV2XPscchBler (uint16_t mcs, double sinr, bool LOS, uint16_t SCS);




}; //end class
} // namespace ns3
#endif /* NR_V2X_PHY_ERROR_MODEL_H */
