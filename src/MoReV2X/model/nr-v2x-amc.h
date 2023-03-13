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
 * Modified by:     Nicola Baldo   <nbaldo@cttc.es>
 * Modified by:     Marco Miozzo   <mmiozzo@cttc.es>
 * Modified by:     NIST
 */

#ifndef NR_V2X_AMCMODULE_H
#define NR_V2X_AMCMODULE_H

#include <vector>
#include <ns3/ptr.h>
#include <ns3/object.h>
#include <map>

namespace ns3 {

class SpectrumValue;

/**
 * \ingroup lte
 * Implements the Adaptive Modulation And Coding Scheme. As proposed in 3GPP
 * TSG-RAN WG1 [R1-081483 Conveying MCS and TB size via PDCCH]
 * (http://www.3gpp.org/ftp/tsg_ran/WG1_RL1/TSGR1_52b/Docs/R1-081483.zip).
 *
 * \note All the methods of this class are static, so you'll never
 *       need to create and manage instances of this class.
 */
class NrV2XAmc : public Object
{

public:
  static TypeId GetTypeId (void);
  
  NrV2XAmc ();
  virtual ~NrV2XAmc();
  
  /**
   * \brief Get the Transport Block Size for a selected MCS and number of PRB (table 7.1.7.2.1-1 of 36.213)
   * \param mcs the mcs index
   * \param nprb the no. of PRB
   * \return the Transport Block Size in bits
   */
  /*static*/ int GetSlSubchAndTbSizeFromMcs (uint32_t PDUsize, int mcs, uint16_t subchannelSize, uint16_t Nsubchannels, uint16_t *TBlen_subChannels, uint16_t *TBlen_RBs);

private:
 
  int GetSecondStageSCI (double R, uint16_t Qm);

  uint16_t m_PRBsubcarriers;

  uint16_t m_psschSymbols;

  uint16_t m_psfchSymbols;
  uint16_t m_overheadSymbols;

  uint16_t m_firstStageSciSymbols;

  std::string m_DMRSpattern;  
}; // end of `class NrV2XAmc`


}

#endif /* NR_V2X_AMCMODULE_H */
