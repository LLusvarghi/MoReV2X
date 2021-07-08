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
 * Original Author: Giuseppe Piro  <g.piro@poliba.it>
 * Modified by:     Marco Miozzo   <mmiozzo@cttc.es>
 *                  Nicola Baldo   <nbaldo@cttc.es>
 *                  NIST
 * Modified by:     Luca Lusvarghi   <luca.lusvarghi5@unimore.it>
 */


#include <ns3/nr-v2x-amc.h>
#include <ns3/log.h>
#include <ns3/assert.h>
#include <ns3/math.h>
#include <vector>
#include <algorithm>
#include <ns3/spectrum-value.h>
#include <ns3/uinteger.h>
#include <ns3/double.h>
#include <ns3/string.h>


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("NrV2XAmc");

NS_OBJECT_ENSURE_REGISTERED (NrV2XAmc);

/**
 * Table of MCS index and its spectral efficiency.
 * Taken from 3GPP TS 38.214 Table 5.1.3.1-1.
 */
static const double SpectralEfficiencyForMcs_1[28] = {
  0.2344, 0.3066, 0.3770, 0.4902, 0.6016, 0.7402, 0.877, 1.0273, 1.1758, 1.3262, 1.3281, 1.4766, 1.6953, 1.9141, 2.1602, 2.4063, 2.5703,
  2.5664, 2.7305, 3.0293, 3.3223, 3.6094, 3.9023, 4.2129, 4.5234, 4.8164, 5.1152, 5.332
};

/**
 * Table of MCS index and Modulation Order (Qm)
 * Taken from 3GPP TS 38.214 Table 5.1.3.1-1.
 */
static const uint16_t ModulationOrderForMcs_1[28] = {
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 4, 4, 4, 4, 4, 4, 4, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6 
};

/**
 * Table of MCS index and Target Code Rate R x [1024]
 * Taken from 3GPP TS 38.214 Table 5.1.3.1-1.
 */
static const double TargetCodeRateForMcs_1[28] = {
  120, 157, 193, 251, 308, 379, 449, 526, 602, 679, 340, 378, 434, 490, 553, 616, 658, 438, 466, 517, 567, 616, 666, 719, 772, 822, 873, 910
};


/**
 * Table of MCS index and its spectral efficiency.
 * Taken from 3GPP TS 38.214 Table 5.1.3.1-2.
 */
static const double SpectralEfficiencyForMcs_2[28] = {
  0.2344, 0.3770, 0.6016, 0.877, 1.1758, 1.4766, 1.6953, 1.9141, 2.1602, 2.4063, 2.5703,
  2.7305, 3.0293, 3.3223, 3.6094, 3.9023, 4.2129, 4.5234,
  4.8164, 5.1152, 5.332, 5.5547, 5.8906, 6.2266, 6.5703, 6.9141, 7.1602, 7.4063 
};

/**
 * Table of MCS index and Modulation Order (Qm)
 * Taken from 3GPP TS 38.214 Table 5.1.3.1-2.
 */
static const uint16_t ModulationOrderForMcs_2[28] = {
  2, 2, 2, 2, 2, 4, 4, 4, 4, 4, 4, 6, 6, 6, 6, 6, 6, 6, 6, 6, 8, 8, 8, 8, 8, 8, 8, 8 
};

/**
 * Table of MCS index and Target Code Rate R x [1024]
 * Taken from 3GPP TS 38.214 Table 5.1.3.1-2.
 */
static const double TargetCodeRateForMcs_2[28] = {
  120, 193, 308, 449, 602, 378, 434, 490, 553, 616, 658, 466, 517, 567, 616, 666, 719, 772, 
  822, 873, 682.5, 711, 754, 797, 841, 885, 916.5, 948 
};

/**
 * Table with the allowed TBS values when Ninfo <= 3824
 * Taken from Table 5.1.3.2-1 in 3GPP TS 38.214
 */
static const uint16_t TransportBlockSizeTable[93] = {
  24, 32, 40, 48, 56, 64, 72, 80, 88, 96, 104, 112, 120, 128, 136, 144, 152, 160, 168, 176, 184, 192, 208, 
  224, 240, 256, 272, 288, 304, 320, 336, 352, 368, 384, 408, 432, 456, 480, 504, 528, 552, 576, 608, 640, 
  672, 704, 736, 768, 808, 848, 888, 928, 984, 1032, 1064, 1128, 1160, 1192, 1224, 1256, 1288, 1320, 
  1352, 1416, 1480, 1544, 1608, 1672, 1736, 1800, 1864, 1928, 2024, 2088, 2152, 2216, 2280, 2408, 2472, 
  2536, 2600, 2664, 2728, 2792, 2856, 2976, 3104, 3240, 3368, 3496, 3624, 3752, 3824 
};

/**
 * Numer of DMRS Resource Elements (REs) depending on the DMRS time pattern
 * Taken from Table 8.1.3.2-1 in 3GPP TS 38.214
 */
static const std::map <std::string, uint16_t> PSSCH_DMRStimePattern = {
  {"{2}", 12}, {"{3}", 18}, {"{4}", 24}, {"{2,3}", 15}, {"{2,4}", 18}, {"{3,4}", 21}, {"{2,3,4}", 18}
};

NrV2XAmc::NrV2XAmc ()
:
  m_PRBsubcarriers(12),
  m_psfchSymbols(0),
  m_overheadSymbols(0)
{

}


NrV2XAmc::~NrV2XAmc ()
{ 
}

TypeId
NrV2XAmc::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::NrV2XAmc")
  .SetParent<Object> ()
  .AddConstructor<NrV2XAmc> ()
  .AddAttribute ("PSSCHsidelinkSymbols",
                 "Number of PSSCH symbols within a slot",
		 UintegerValue (14),
		 MakeUintegerAccessor (&NrV2XAmc::m_psschSymbols),
		 MakeUintegerChecker<uint16_t> ())
  .AddAttribute ("FirstStageSciSymbols",
                 "Number of OFDM symbols assigned to the 1st stage SCI",
		 UintegerValue (2),
		 MakeUintegerAccessor (&NrV2XAmc::m_firstStageSciSymbols),
		 MakeUintegerChecker<uint16_t> ())
  .AddAttribute ("PSSCH_DMRStimePattern",
                 "Type of DMRS time pattern for the PSSCH",
		 StringValue ("{2,4}"),
		 MakeStringAccessor (&NrV2XAmc::m_DMRSpattern),
                 MakeStringChecker ())
;
  return tid;
}


int
NrV2XAmc::GetSlSubchAndTbSizeFromMcs (uint32_t PDUsize, int mcs, uint16_t subchannelSize, uint16_t channelBW_RBs, uint16_t *TBlen_subChannels, uint16_t *TBlen_RBs)
{
  NS_LOG_FUNCTION (this);

  uint32_t PDUsize_bits = PDUsize * 8;

  uint16_t Nsubchannels = std::floor(channelBW_RBs/subchannelSize);

  NS_ASSERT_MSG (mcs <= 27, "MCS index must be lower than 28");
  NS_ASSERT_MSG(PSSCH_DMRStimePattern.find(m_DMRSpattern) != PSSCH_DMRStimePattern.end(), "Non-valid DMRS pattern");

  NS_LOG_DEBUG("Computing TB size for PDU size = " << PDUsize << " B, subchannel size = " << subchannelSize << " RBs, number of subchannels = " 
  << Nsubchannels << ", and MCS index " << mcs);
  uint16_t Nre_DMRS = PSSCH_DMRStimePattern.find(m_DMRSpattern)->second;
  uint16_t actual_psschSymbols = m_psschSymbols - 2;

  NS_LOG_DEBUG(this << " configuration parameters:");
  NS_LOG_DEBUG("Subcarriers in PRB = " << m_PRBsubcarriers << " PSFCH symbols = " << m_psfchSymbols << " number of DMRS REs = " << Nre_DMRS);
  NS_LOG_DEBUG("Sidelink symbols within the slot = " << actual_psschSymbols << " number of overhead symbols = " << m_overheadSymbols);

  // Start with computing the number of Resource Elements (REs) allocated for PSSCH within a PRB
  uint16_t Nre_pssch_PRB = m_PRBsubcarriers*(actual_psschSymbols - m_psfchSymbols) - m_overheadSymbols - Nre_DMRS;

  NS_LOG_DEBUG("Number of REs allocated for the PSSCH within a PRB = " << Nre_pssch_PRB);

  // Then, determine the total number of REs allocated for PSSCH
  uint16_t Nre_pssch, TBStemp;
  uint16_t Nre_pscch = m_firstStageSciSymbols * m_PRBsubcarriers * subchannelSize; //SCI is allocated over one subchannel (fixed)

  NS_LOG_DEBUG("Number of REs allocated for the PSCCH = " << Nre_pscch << " Target code rate = " << TargetCodeRateForMcs_1[mcs] << ", modulation order Qm = " << ModulationOrderForMcs_1[mcs]);

  // Now look for the number of lowest number of subchannels that fits the PDU
  uint32_t TBS = 0;
  uint16_t alloc_subchannels, alloc_RBs;
  uint16_t SecondStageSCI = GetSecondStageSCI(TargetCodeRateForMcs_1[mcs]/1024,  2);
  NS_LOG_DEBUG("Evaluating the number of required subchannels. Second stage SCI REs = " << SecondStageSCI);
  for (uint16_t j = 1; j <= Nsubchannels; j++)
  {
    alloc_subchannels = j;
    Nre_pssch = (Nre_pssch_PRB * j * subchannelSize) - Nre_pscch - SecondStageSCI; 
    TBStemp = std::round(Nre_pssch * (TargetCodeRateForMcs_1[mcs]/1024) * ModulationOrderForMcs_1[mcs]);
    //NS_LOG_DEBUG("TBStemp = " << TBStemp);
    uint16_t n;
    uint32_t Ninfo_prime;
    if (TBStemp <= 3824)
    {
      n = std::max(3, (int) std::floor( std::log2(TBStemp) ) - 6);
      Ninfo_prime = std::max(24, (int) ( std::pow(2,n) * std::floor(TBStemp/std::pow(2,n)) ));
      for (uint16_t i = 0; i < sizeof(TransportBlockSizeTable)/sizeof(TransportBlockSizeTable[0]); i++)
      {
        //NS_LOG_DEBUG("i = " << i << ", TBS = " << TransportBlockSizeTable[i]);
        if (TransportBlockSizeTable[i] >= Ninfo_prime)
        {
          TBS = TransportBlockSizeTable[i];
          break;
        }
      }
    }
    else
    {
      n = std::floor( std::log2(TBStemp-24) ) - 5;
      Ninfo_prime = std::max(3840, (int) (std::pow(2,n) * std::round((TBStemp-24)/std::pow(2,n)) ));
      uint16_t C;
      if ((TargetCodeRateForMcs_1[mcs]/1024) <= (1/4))
      {
        C = std::ceil( (Ninfo_prime+24)/3816 );
        TBS = (8 * C * std::ceil( (Ninfo_prime+24)/(8*C) ) )- 24;
      }
      else
      {
        if (Ninfo_prime > 8424)
        {
          C = std::ceil( (Ninfo_prime+24)/8424 );
          TBS = (8 * C * std::ceil( (Ninfo_prime+24)/(8*C) ) )- 24;
        }
        else
        {
          TBS = (8 * std::ceil( (Ninfo_prime+24)/8 ) )- 24;
        }
      }
    }
//    NS_LOG_DEBUG("n = " << n << ", Ninfo' = " << Ninfo_prime << ", TBS = " << TBS);
    NS_LOG_DEBUG("TBS = " << TBS/8 << ", with " << j << " subchannels");
    if (TBS >= PDUsize_bits)
      break;
   
  }
  NS_ASSERT_MSG(TBS != 0, "Could not match any TB size value");
  NS_ASSERT_MSG(PDUsize <= TBS/8, "PDU size is too large for this MCS");
  NS_LOG_DEBUG("Over: PDU " << PDUsize << " B, TBS = " << TBS/8 << " B with " << alloc_subchannels << " subchannel(s)");
//  *TBlen_RBs = alloc_subchannels*subchannelSize;
  *TBlen_subChannels = alloc_subchannels*subchannelSize;
//  std::cin.get();

  TBS = 0;
  NS_LOG_DEBUG("Evaluating the number of required RBs. Second stage SCI REs = " << SecondStageSCI);
  for (uint16_t j = subchannelSize; j <= channelBW_RBs; j++)
  {
    alloc_RBs = j;
    Nre_pssch = (Nre_pssch_PRB * j) - Nre_pscch - SecondStageSCI; 
    TBStemp = std::round(Nre_pssch * (TargetCodeRateForMcs_1[mcs]/1024) * ModulationOrderForMcs_1[mcs]);
    //NS_LOG_DEBUG("TBStemp = " << TBStemp);
    uint16_t n;
    uint32_t Ninfo_prime;
    if (TBStemp <= 3824)
    {
      n = std::max(3, (int) std::floor( std::log2(TBStemp) ) - 6);
      Ninfo_prime = std::max(24, (int) ( std::pow(2,n) * std::floor(TBStemp/std::pow(2,n)) ));
      for (uint16_t i = 0; i < sizeof(TransportBlockSizeTable)/sizeof(TransportBlockSizeTable[0]); i++)
      {
        //NS_LOG_DEBUG("i = " << i << ", TBS = " << TransportBlockSizeTable[i]);
        if (TransportBlockSizeTable[i] >= Ninfo_prime)
        {
          TBS = TransportBlockSizeTable[i];
          break;
        }
      }
    }
    else
    {
      n = std::floor( std::log2(TBStemp-24) ) - 5;
      Ninfo_prime = std::max(3840, (int) (std::pow(2,n) * std::round((TBStemp-24)/std::pow(2,n)) ));
      uint16_t C;
      if ((TargetCodeRateForMcs_1[mcs]/1024) <= (1/4))
      {
        C = std::ceil( (Ninfo_prime+24)/3816 );
        TBS = (8 * C * std::ceil( (Ninfo_prime+24)/(8*C) ) )- 24;
      }
      else
      {
        if (Ninfo_prime > 8424)
        {
          C = std::ceil( (Ninfo_prime+24)/8424 );
          TBS = (8 * C * std::ceil( (Ninfo_prime+24)/(8*C) ) )- 24;
        }
        else
        {
          TBS = (8 * std::ceil( (Ninfo_prime+24)/8 ) )- 24;
        }
      }
    }
//    NS_LOG_DEBUG("n = " << n << ", Ninfo' = " << Ninfo_prime << ", TBS = " << TBS);
    NS_LOG_DEBUG("TBS = " << TBS/8 << ", with " << j << " RBs");
    if (TBS >= PDUsize_bits)
      break;
   
  }
  NS_LOG_DEBUG("Over: PDU " << PDUsize << " B, TBS = " << TBS/8 << " B with " << alloc_RBs << " RBs");
  *TBlen_RBs = alloc_RBs;


//  std::cin.get();

  return TBS;
}


/*Implemented according to TS 38.212 and R1-2005796*/

int
NrV2XAmc::GetSecondStageSCI (double R, uint16_t Qm)
{
  NS_LOG_FUNCTION(this);
  uint16_t SCIbits = 48; //Format 2-B
  uint16_t CRCbits = 24; //Format 2-B
  double beta_offset = 3.5;
  
  return std::ceil( (SCIbits+CRCbits) * beta_offset / (R*Qm) );

}


} // namespace ns3
