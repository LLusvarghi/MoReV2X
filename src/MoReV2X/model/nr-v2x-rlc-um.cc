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
 * Author: Manuel Requena <manuel.requena@cttc.es>
 * Modified by: NIST
 * Modified by: Luca Lusvarghi <luca.lusvarghi5@unimore.it>
 */

#include "ns3/simulator.h"
#include "ns3/log.h"

#include "ns3/nist-lte-rlc-header.h"
#include "ns3/nr-v2x-rlc-um.h"
#include "ns3/nist-lte-rlc-sdu-status-tag.h"
#include "ns3/nist-lte-rlc-tag.h"
#include <ns3/string.h>

#include <iostream>
#include <fstream>
#include <inttypes.h>

#include "ns3/nr-v2x-tag.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("NistLteRlcUm");

NS_OBJECT_ENSURE_REGISTERED (NistLteRlcUm);

NistLteRlcUm::NistLteRlcUm ()
  : m_maxTxBufferSize (10 * 1024),
    m_txBufferSize (0),
    m_sequenceNumber (0),
    m_vrUr (0),
    m_vrUx (0),
    m_vrUh (0),
    m_windowSize (512),
    m_expectedSeqNumber (0)
{
  NS_LOG_FUNCTION (this);
  m_reassemblingState = WAITING_S0_FULL;
}

NistLteRlcUm::~NistLteRlcUm ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
NistLteRlcUm::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::NistLteRlcUm")
    .SetParent<NistLteRlc> ()
    .AddConstructor<NistLteRlcUm> ()
    .AddAttribute ("MaxTxBufferSize",
                   "Maximum Size of the Transmission Buffer (in Bytes)",
                   UintegerValue (10 * 1024),
                   MakeUintegerAccessor (&NistLteRlcUm::m_maxTxBufferSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("OutputPath",
                   "Specifiy the output path where to store the results",
                   StringValue ("results/sidelink/"),
                   MakeStringAccessor (&NistLteRlcUm::m_outputPath),
                   MakeStringChecker ())
    ;
  return tid;
}

void
NistLteRlcUm::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_reorderingTimer.Cancel ();
  m_rbsTimer.Cancel ();

  NistLteRlc::DoDispose ();
}

/**
 * RLC SAP
 */

void
NistLteRlcUm::DoTransmitPdcpPdu (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this << m_rnti << (uint32_t) m_lcid << p->GetSize ());

  if (m_txBufferSize + p->GetSize () <= m_maxTxBufferSize)
    {
      /** Store arrival time */
      NistRlcTag timeTag (Simulator::Now ());
      p->AddPacketTag (timeTag);

      /** Store PDCP PDU */

      NistLteRlcSduNistStatusTag tag;
      tag.SetNistStatus (NistLteRlcSduNistStatusTag::FULL_SDU);
      p->AddPacketTag (tag);

      NS_LOG_LOGIC ("Tx Buffer: New packet added");
      m_txBuffer.push_back (p);
      NS_LOG_LOGIC ("txBufferSize = " << m_txBufferSize);
      m_txBufferSize += p->GetSize ();
      NS_LOG_LOGIC ("NumOfBuffers = " << m_txBuffer.size() );
      NS_LOG_LOGIC ("txBufferSize = " << m_txBufferSize);

       /* LOG the buffer size*/
   /*   std::ofstream RLCbuffer;
      RLCbuffer.open (m_outputPath + "RLCbufferSize.txt", std::ios_base::app);
      RLCbuffer << "UE " << m_rnti << " at time " << Simulator::Now().GetSeconds() << ", NumOfBuffers = " << m_txBuffer.size() << ", txBufferSize = " << m_txBufferSize << std::endl;
      RLCbuffer.close();
      NS_LOG_UNCOND("-------------------Remember to disable this file in NistLteRlcUm");*/

    }
  else
    {
      // Discard full RLC SDU
      NS_LOG_LOGIC ("TxBuffer is full. RLC SDU discarded");
      NS_LOG_LOGIC ("MaxTxBufferSize = " << m_maxTxBufferSize);
      NS_LOG_LOGIC ("txBufferSize    = " << m_txBufferSize);
      NS_LOG_LOGIC ("packet size     = " << p->GetSize ());
    }

  /** Report Buffer NistStatus */
  DoReportBufferNistStatus ();
  m_rbsTimer.Cancel ();

}


/**
 * MAC SAP
 */

void
NistLteRlcUm::DoNotifyTxOpportunity (uint32_t bytes, uint8_t layer, uint8_t harqId)
{
  NS_LOG_FUNCTION (this << m_rnti << (uint32_t) m_lcid << bytes);

  if (bytes <= 2)
    {
      // Stingy MAC: Header fix part is 2 bytes, we need more bytes for the data
      NS_LOG_LOGIC ("TX opportunity too small = " << bytes);
      return;
    }

  Ptr<Packet> packet = Create<Packet> ();
  NistLteRlcHeader rlcHeader;

  // Build Data field
  uint32_t nextSegmentSize = bytes - 2;
  uint32_t nextSegmentId = 1;
  uint32_t dataFieldTotalSize = 0;
  uint32_t dataFieldAddedSize = 0;
  std::vector < Ptr<Packet> > dataField;

  // Remove the first packet from the transmission buffer.
  // If only a segment of the packet is taken, then the remaining is given back later
  if ( m_txBuffer.size () == 0 )
    {
      NS_LOG_LOGIC ("No data pending");
      return;
    }

  NS_LOG_LOGIC ("SDUs in TxBuffer  = " << m_txBuffer.size ());
  NS_LOG_LOGIC ("First SDU buffer  = " << *(m_txBuffer.begin()));
  NS_LOG_LOGIC ("First SDU size    = " << (*(m_txBuffer.begin()))->GetSize ());
  NS_LOG_LOGIC ("Next segment size = " << nextSegmentSize);
  NS_LOG_LOGIC ("Remove SDU from TxBuffer");
  Ptr<Packet> copyPacket = (*(m_txBuffer.begin ()))->Copy ();
  Ptr<Packet> firstSegment = (*(m_txBuffer.begin ()))->Copy ();
  NS_LOG_LOGIC ("txBufferSize      = " << m_txBufferSize );
  m_txBufferSize -= (*(m_txBuffer.begin()))->GetSize ();
  NS_LOG_LOGIC ("txBufferSize      = " << m_txBufferSize );
  m_txBuffer.erase (m_txBuffer.begin ());

  while ( firstSegment && (firstSegment->GetSize () > 0) && (nextSegmentSize > 0) )
    {
      NS_LOG_LOGIC ("WHILE ( firstSegment && firstSegment->GetSize > 0 && nextSegmentSize > 0 )");
      NS_LOG_LOGIC ("    firstSegment size = " << firstSegment->GetSize ());
      NS_LOG_LOGIC ("    nextSegmentSize   = " << nextSegmentSize);
      if ( (firstSegment->GetSize () > nextSegmentSize) ||
           // Segment larger than 2047 octets can only be mapped to the end of the Data field
           (firstSegment->GetSize () > 2047)
         )
        {
          // Take the minimum size, due to the 2047-bytes 3GPP exception
          // This exception is due to the length of the LI field (just 11 bits)
          uint32_t currSegmentSize = std::min (firstSegment->GetSize (), nextSegmentSize);

          NS_LOG_LOGIC ("    IF ( firstSegment > nextSegmentSize ||");
          NS_LOG_LOGIC ("         firstSegment > 2047 )");

          // Segment txBuffer.FirstBuffer and
          // Give back the remaining segment to the transmission buffer
          Ptr<Packet> newSegment = firstSegment->CreateFragment (0, currSegmentSize);
          NS_LOG_LOGIC ("    newSegment size   = " << newSegment->GetSize ());

          // NistStatus tag of the new and remaining segments
          // Note: This is the only place where a PDU is segmented and
          // therefore its status can change
          NistLteRlcSduNistStatusTag oldTag, newTag;
          firstSegment->RemovePacketTag (oldTag);
          newSegment->RemovePacketTag (newTag);
          if (oldTag.GetNistStatus () == NistLteRlcSduNistStatusTag::FULL_SDU)
            {
              newTag.SetNistStatus (NistLteRlcSduNistStatusTag::FIRST_SEGMENT);
              oldTag.SetNistStatus (NistLteRlcSduNistStatusTag::LAST_SEGMENT);
            }
          else if (oldTag.GetNistStatus () == NistLteRlcSduNistStatusTag::LAST_SEGMENT)
            {
              newTag.SetNistStatus (NistLteRlcSduNistStatusTag::MIDDLE_SEGMENT);
              //oldTag.SetNistStatus (NistLteRlcSduNistStatusTag::LAST_SEGMENT);
            }

          // Give back the remaining segment to the transmission buffer
          firstSegment->RemoveAtStart (currSegmentSize);
          NS_LOG_LOGIC ("    firstSegment size (after RemoveAtStart) = " << firstSegment->GetSize ());
          if (firstSegment->GetSize () > 0)
            {
              firstSegment->AddPacketTag (oldTag);

              m_txBuffer.insert (m_txBuffer.begin (), firstSegment);
              m_txBufferSize += (*(m_txBuffer.begin()))->GetSize ();

              NS_LOG_LOGIC ("    TX buffer: Give back the remaining segment");
              NS_LOG_LOGIC ("    TX buffers = " << m_txBuffer.size ());
              NS_LOG_LOGIC ("    Front buffer size = " << (*(m_txBuffer.begin()))->GetSize ());
              NS_LOG_LOGIC ("    txBufferSize = " << m_txBufferSize );
            }
          else
            {
              // Whole segment was taken, so adjust tag
              if (newTag.GetNistStatus () == NistLteRlcSduNistStatusTag::FIRST_SEGMENT)
                {
                  newTag.SetNistStatus (NistLteRlcSduNistStatusTag::FULL_SDU);
                }
              else if (newTag.GetNistStatus () == NistLteRlcSduNistStatusTag::MIDDLE_SEGMENT)
                {
                  newTag.SetNistStatus (NistLteRlcSduNistStatusTag::LAST_SEGMENT);
                }
            }
          // Segment is completely taken or
          // the remaining segment is given back to the transmission buffer
          firstSegment = 0;

          // Put status tag once it has been adjusted
          newSegment->AddPacketTag (newTag);

          // Add Segment to Data field
          dataFieldAddedSize = newSegment->GetSize ();
          dataFieldTotalSize += dataFieldAddedSize;
          dataField.push_back (newSegment);
          newSegment = 0;

          // ExtensionBit (Next_Segment - 1) = 0
          rlcHeader.PushExtensionBit (NistLteRlcHeader::DATA_FIELD_FOLLOWS);

          // no LengthIndicator for the last one

          nextSegmentSize -= dataFieldAddedSize;
          nextSegmentId++;

          // nextSegmentSize MUST be zero (only if segment is smaller or equal to 2047)

          // (NO more segments) → exit
          // break;
        }
      else if ( (nextSegmentSize - firstSegment->GetSize () <= 2) || (m_txBuffer.size () == 0) )
        {
          NS_LOG_LOGIC ("    IF nextSegmentSize - firstSegment->GetSize () <= 2 || txBuffer.size == 0");
          // Add txBuffer.FirstBuffer to DataField
          dataFieldAddedSize = firstSegment->GetSize ();
          dataFieldTotalSize += dataFieldAddedSize;
          dataField.push_back (firstSegment);
          firstSegment = 0;

          // ExtensionBit (Next_Segment - 1) = 0
          rlcHeader.PushExtensionBit (NistLteRlcHeader::DATA_FIELD_FOLLOWS);

          // no LengthIndicator for the last one

          nextSegmentSize -= dataFieldAddedSize;
          nextSegmentId++;

          NS_LOG_LOGIC ("        SDUs in TxBuffer  = " << m_txBuffer.size ());
          if (m_txBuffer.size () > 0)
            {
              NS_LOG_LOGIC ("        First SDU buffer  = " << *(m_txBuffer.begin()));
              NS_LOG_LOGIC ("        First SDU size    = " << (*(m_txBuffer.begin()))->GetSize ());
            }
          NS_LOG_LOGIC ("        Next segment size = " << nextSegmentSize);

          // nextSegmentSize <= 2 (only if txBuffer is not empty)

          // (NO more segments) → exit
          // break;
        }
      else // (firstSegment->GetSize () < m_nextSegmentSize) && (m_txBuffer.size () > 0)
        {
          NS_LOG_LOGIC ("    IF firstSegment < NextSegmentSize && txBuffer.size > 0");
          // Add txBuffer.FirstBuffer to DataField
          dataFieldAddedSize = firstSegment->GetSize ();
          dataFieldTotalSize += dataFieldAddedSize;
          dataField.push_back (firstSegment);

          // ExtensionBit (Next_Segment - 1) = 1
          rlcHeader.PushExtensionBit (NistLteRlcHeader::E_LI_FIELDS_FOLLOWS);

          // LengthIndicator (Next_Segment)  = txBuffer.FirstBuffer.length()
          rlcHeader.PushLengthIndicator (firstSegment->GetSize ());

          nextSegmentSize -= ((nextSegmentId % 2) ? (2) : (1)) + dataFieldAddedSize;
          nextSegmentId++;

          NS_LOG_LOGIC ("        SDUs in TxBuffer  = " << m_txBuffer.size ());
          if (m_txBuffer.size () > 0)
            {
              NS_LOG_LOGIC ("        First SDU buffer  = " << *(m_txBuffer.begin()));
              NS_LOG_LOGIC ("        First SDU size    = " << (*(m_txBuffer.begin()))->GetSize ());
            }
          NS_LOG_LOGIC ("        Next segment size = " << nextSegmentSize);
          NS_LOG_LOGIC ("        Remove SDU from TxBuffer");

          // (more segments)
          firstSegment = (*(m_txBuffer.begin ()))->Copy ();
          m_txBufferSize -= (*(m_txBuffer.begin()))->GetSize ();
          m_txBuffer.erase (m_txBuffer.begin ());
          NS_LOG_LOGIC ("        txBufferSize = " << m_txBufferSize );
        }

    }

  // Build RLC header
  rlcHeader.SetSequenceNumber (m_sequenceNumber++);

  // Build RLC PDU with DataField and Header
  std::vector< Ptr<Packet> >::iterator it;
  it = dataField.begin ();

  uint8_t framingInfo = 0;

  // FIRST SEGMENT
  NistLteRlcSduNistStatusTag tag;
  (*it)->RemovePacketTag (tag);
  if ( (tag.GetNistStatus () == NistLteRlcSduNistStatusTag::FULL_SDU) ||
        (tag.GetNistStatus () == NistLteRlcSduNistStatusTag::FIRST_SEGMENT) )
    {
      framingInfo |= NistLteRlcHeader::FIRST_BYTE;
    }
  else
    {
      framingInfo |= NistLteRlcHeader::NO_FIRST_BYTE;
    }
  (*it)->AddPacketTag (tag);

  while (it < dataField.end ())
    {
      NS_LOG_LOGIC ("Adding SDU/segment to packet, length = " << (*it)->GetSize ());

      packet->AddAtEnd (*it);
      it++;
    }

  // LAST SEGMENT (Note: There could be only one and be the first one)
  it--;
  (*it)->RemovePacketTag (tag);
  if ( (tag.GetNistStatus () == NistLteRlcSduNistStatusTag::FULL_SDU) ||
        (tag.GetNistStatus () == NistLteRlcSduNistStatusTag::LAST_SEGMENT) )
    {
      framingInfo |= NistLteRlcHeader::LAST_BYTE;
    }
  else
    {
      framingInfo |= NistLteRlcHeader::NO_LAST_BYTE;
    }
  (*it)->AddPacketTag (tag);

  rlcHeader.SetFramingInfo (framingInfo);

  NS_LOG_LOGIC ("RLC header: " << rlcHeader);
  packet->AddHeader (rlcHeader);

  // Sender timestamp
  NistRlcTag rlcTag (Simulator::Now ());
  packet->AddByteTag (rlcTag);
  m_txPdu (m_rnti, m_lcid, packet->GetSize ());

  // Send RLC PDU to MAC layer
  NistLteMacSapProvider::NistTransmitPduParameters params;
  params.pdu = packet;
  params.rnti = m_rnti;
  params.lcid = m_lcid;
  params.srcL2Id = m_srcL2Id;
  params.dstL2Id = m_dstL2Id;
  params.layer = layer;
//  params.harqProcessId = harqId;
  params.harqProcessId = 0;
  
  //TODO FIXME NEW for V2X
  NrV2XTag v2xTag;
  if (packet -> FindFirstMatchingByteTag (v2xTag))
    {
   /*   std::ofstream tagFile;
      tagFile.open (m_outputPath + "v2xTag.txt", std::ios_base::app);
      tagFile << v2xTag.GetGenTime () << ", message type: " << (int) v2xTag.GetMessageType () << ", from RLC \r\n";
      tagFile.close ();*/
      
      params.V2XMessageType = v2xTag.GetMessageType ();
    }
  
  m_macSapProvider->TransmitPdu (params);

  if (! m_txBuffer.empty ())
    {
      m_rbsTimer.Cancel ();
      m_rbsTimer = Simulator::Schedule (MilliSeconds (10), &NistLteRlcUm::ExpireRbsTimer, this);
    }

  NS_LOG_LOGIC ("First SDU size    = " << (*(m_txBuffer.begin()))->GetSize ());

  if ((uint8_t) harqId == 1) 
  {
    NS_LOG_LOGIC("Pushing back another packet for blind retransmissions");
    m_txBuffer.push_back(copyPacket);
    m_txBufferSize += copyPacket->GetSize ();
//    std::cin.get();
  }

}

void
NistLteRlcUm::DoNotifyHarqDeliveryFailure ()
{
  NS_LOG_FUNCTION (this);
}

void
NistLteRlcUm::DoReceivePdu (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this << m_rnti << (uint32_t) m_lcid << p->GetSize ());
  // Receiver timestamp
  NistRlcTag rlcTag;
  Time delay;
  if (p->FindFirstMatchingByteTag (rlcTag))
    {
      delay = Simulator::Now() - rlcTag.GetSenderTimestamp ();
     // NS_LOG_UNCOND("Delay: " << delay.GetSeconds());
    }
  m_rxPdu (m_rnti, m_lcid, p->GetSize (), delay.GetNanoSeconds ());

  /*std::ofstream delayRLC;
  delayRLC.open(m_outputPath + "delayRLC.csv", std::ios_base::app);
  delayRLC << delay.GetSeconds () << " s , Now: " << Simulator::Now ().GetSeconds () << " s" << "\r\n";
  delayRLC.close (); */

  // 5.1.2.2 Receive operations

  // Get RLC header parameters
  NistLteRlcHeader rlcHeader;
  p->PeekHeader (rlcHeader);
  NS_LOG_LOGIC ("RLC header: " << rlcHeader);
  NistSequenceNumber10 seqNumber = rlcHeader.GetSequenceNumber ();

  // 5.1.2.2.1 General
  // The receiving UM RLC entity shall maintain a reordering window according to state variable VR(UH) as follows:
  // - a SN falls within the reordering window if (VR(UH) - UM_Window_Size) <= SN < VR(UH);
  // - a SN falls outside of the reordering window otherwise.
  // When receiving an UMD PDU from lower layer, the receiving UM RLC entity shall:
  // - either discard the received UMD PDU or place it in the reception buffer (see sub clause 5.1.2.2.2);
  // - if the received UMD PDU was placed in the reception buffer:
  // - update state variables, reassemble and deliver RLC SDUs to upper layer and start/stop t-Reordering as needed (see sub clause 5.1.2.2.3);
  // When t-Reordering expires, the receiving UM RLC entity shall:
  // - update state variables, reassemble and deliver RLC SDUs to upper layer and start t-Reordering as needed (see sub clause 5.1.2.2.4).

  // 5.1.2.2.2 Actions when an UMD PDU is received from lower layer
  // When an UMD PDU with SN = x is received from lower layer, the receiving UM RLC entity shall:
  // - if VR(UR) < x < VR(UH) and the UMD PDU with SN = x has been received before; or
  // - if (VR(UH) - UM_Window_Size) <= x < VR(UR):
  //    - discard the received UMD PDU;
  // - else:
  //    - place the received UMD PDU in the reception buffer.

  NS_LOG_LOGIC ("VR(UR) = " << m_vrUr);
  NS_LOG_LOGIC ("VR(UX) = " << m_vrUx);
  NS_LOG_LOGIC ("VR(UH) = " << m_vrUh);
  NS_LOG_LOGIC ("SN = " << seqNumber);

  m_vrUr.SetModulusBase (m_vrUh - m_windowSize);
  m_vrUh.SetModulusBase (m_vrUh - m_windowSize);
  seqNumber.SetModulusBase (m_vrUh - m_windowSize);

  if ( ( (m_vrUr < seqNumber) && (seqNumber < m_vrUh) && (m_rxBuffer.count (seqNumber.GetValue ()) > 0) ) ||
       ( ((m_vrUh - m_windowSize) <= seqNumber) && (seqNumber < m_vrUr) )
     )
    {
      NS_LOG_LOGIC ("PDU discarded");
      p = 0;

    /*
      std::ofstream RxRlcUmFile;
      RxRlcUmFile.open (m_outputPath + "RxRlcUmFileDiscard.csv", std::ios_base::app);
      RxRlcUmFile << m_rnti << "," << Simulator::Now ().GetSeconds () << "," << seqNumber << "\r\n";
      RxRlcUmFile.close ();
   */

      return;
    }
  else
    {
      NS_LOG_INFO ("Place PDU in the reception buffer, Now: " << Simulator::Now().GetSeconds() << "s, SN: " << seqNumber.GetValue ());
      m_rxBuffer[seqNumber.GetValue ()] = p;
      NS_LOG_INFO("m_rxBuffer Size: " << m_rxBuffer.size()); 
     
/*
      std::ofstream RxRlcUmFile;
      RxRlcUmFile.open (m_outputPath + "RxRlcUmFile.csv", std::ios_base::app);
      RxRlcUmFile << Simulator::Now ().GetSeconds () << "\r\n";
      RxRlcUmFile.close ();
*/
    }


  // 5.1.2.2.3 Actions when an UMD PDU is placed in the reception buffer
  // When an UMD PDU with SN = x is placed in the reception buffer, the receiving UM RLC entity shall:

  // - if x falls outside of the reordering window:
  //    - update VR(UH) to x + 1;
  //    - reassemble RLC SDUs from any UMD PDUs with SN that falls outside of the reordering window, remove
  //      RLC headers when doing so and deliver the reassembled RLC SDUs to upper layer in ascending order of the
  //      RLC SN if not delivered before;
  //    - if VR(UR) falls outside of the reordering window:
  //        - set VR(UR) to (VR(UH) - UM_Window_Size);

  if ( ! IsInsideReorderingWindow (seqNumber))
    {
      NS_LOG_LOGIC ("SN " << seqNumber << " is outside the reordering window");

      m_vrUh = seqNumber + 1;
      NS_LOG_LOGIC ("New VR(UH) = " << m_vrUh);

      ReassembleOutsideWindow ();
      NS_LOG_LOGIC ("Current VR(UR) = " << m_vrUr);
      if ( ! IsInsideReorderingWindow (m_vrUr) )
        {
          m_vrUr = m_vrUh - m_windowSize;
          NS_LOG_LOGIC ("VR(UR) is outside the reordering window"); //LOGIC
          NS_LOG_LOGIC ("New VR(UR) = " << m_vrUr);
        }
    }

  // - if the reception buffer contains an UMD PDU with SN = VR(UR):
  //    - update VR(UR) to the SN of the first UMD PDU with SN > current VR(UR) that has not been received;
  //    - reassemble RLC SDUs from any UMD PDUs with SN < updated VR(UR), remove RLC headers when doing
  //      so and deliver the reassembled RLC SDUs to upper layer in ascending order of the RLC SN if not delivered
  //      before;

  NS_LOG_INFO("m_rxBuffer.count (m_vrUr.GetValue ()): " << m_rxBuffer.count (m_vrUr.GetValue ()));
  if ( m_rxBuffer.count (m_vrUr.GetValue ()) > 0 )
    {
      NS_LOG_INFO ("Reception buffer contains SN = " << m_vrUr);

      std::map <uint16_t, Ptr<Packet> >::iterator it;
      uint16_t newVrUr;
      NistSequenceNumber10 oldVrUr = m_vrUr;

      it = m_rxBuffer.find (m_vrUr.GetValue ());
      newVrUr = (it->first) + 1;
      while ( m_rxBuffer.count (newVrUr) > 0 )
        {
          newVrUr++;
        }
      m_vrUr = newVrUr;
      NS_LOG_LOGIC ("New VR(UR) = " << m_vrUr);

      ReassembleSnInterval (oldVrUr, m_vrUr);
    }

  //TODO FIXME NEW for V2X --> this is just a test

    else 
    {
std::map <uint16_t, Ptr<Packet> >::iterator it;
  NistSequenceNumber10 newVrUr = m_vrUx;

  while ( (it = m_rxBuffer.find (newVrUr.GetValue ())) != m_rxBuffer.end () )
    {
      newVrUr++;
    }
  NistSequenceNumber10 oldVrUr = m_vrUr;
  
  m_vrUr = newVrUr;
  oldVrUr = m_vrUr - 1;
  NS_LOG_INFO ("New VR(UR) = " << m_vrUr << " VR(UX) = " << m_vrUx << ", VR(UH) = " << m_vrUh << " , Now: " << Simulator::Now().GetSeconds() << "s");

  ReassembleSnInterval (m_vrUx, m_vrUh);
  }

  // m_vrUh can change previously, set new modulus base
  // for the t-Reordering timer-related comparisons
  m_vrUr.SetModulusBase (m_vrUh - m_windowSize);
  m_vrUx.SetModulusBase (m_vrUh - m_windowSize);
  m_vrUh.SetModulusBase (m_vrUh - m_windowSize);

  // - if t-Reordering is running:
  //    - if VR(UX) <= VR(UR); or
  //    - if VR(UX) falls outside of the reordering window and VR(UX) is not equal to VR(UH)::
  //        - stop and reset t-Reordering;
  if ( m_reorderingTimer.IsRunning () )
    {
      NS_LOG_LOGIC ("Reordering timer is running");

      if ( (m_vrUx <= m_vrUr) ||
           ((! IsInsideReorderingWindow (m_vrUx)) && (m_vrUx != m_vrUh)) )
        {
          NS_LOG_LOGIC ("Stop reordering timer");
          m_reorderingTimer.Cancel ();
        }
    }

  // - if t-Reordering is not running (includes the case when t-Reordering is stopped due to actions above):
  //    - if VR(UH) > VR(UR):
  //        - start t-Reordering;
  //        - set VR(UX) to VR(UH).
  if ( ! m_reorderingTimer.IsRunning () )
    {
      NS_LOG_LOGIC ("Reordering timer is not running");

      if ( m_vrUh > m_vrUr )
        {
          NS_LOG_LOGIC ("VR(UH) > VR(UR)");
          NS_LOG_LOGIC ("Start reordering timer");
          m_reorderingTimer = Simulator::Schedule (Time ("0.1s"),
                                                   &NistLteRlcUm::ExpireReorderingTimer ,this);
          m_vrUx = m_vrUh;
          NS_LOG_LOGIC ("New VR(UX) = " << m_vrUx);
        }
    }

}


bool
NistLteRlcUm::IsInsideReorderingWindow (NistSequenceNumber10 seqNumber)
{
  NS_LOG_FUNCTION (this << seqNumber);
  NS_LOG_LOGIC ("Reordering Window: " <<
                m_vrUh << " - " << m_windowSize << " <= " << seqNumber << " < " << m_vrUh);

  m_vrUh.SetModulusBase (m_vrUh - m_windowSize);
  seqNumber.SetModulusBase (m_vrUh - m_windowSize);

  if ( ((m_vrUh - m_windowSize) <= seqNumber) && (seqNumber < m_vrUh))
    {
      NS_LOG_LOGIC (seqNumber << " is INSIDE the reordering window");
      return true;
    }
  else
    {
      NS_LOG_LOGIC (seqNumber << " is OUTSIDE the reordering window");
      return false;
    }
}


void
NistLteRlcUm::ReassembleAndDeliver (Ptr<Packet> packet)
{
  NistLteRlcHeader rlcHeader;
  packet->RemoveHeader (rlcHeader);
  uint8_t framingInfo = rlcHeader.GetFramingInfo ();
  NistSequenceNumber10 currSeqNumber = rlcHeader.GetSequenceNumber ();
  bool expectedSnLost;

  if ( currSeqNumber != m_expectedSeqNumber )
    {
      expectedSnLost = true;
      NS_LOG_LOGIC ("There are losses. Expected SN = " << m_expectedSeqNumber << ". Current SN = " << currSeqNumber);
      m_expectedSeqNumber = currSeqNumber + 1;
    }
  else
    {
      expectedSnLost = false;
      NS_LOG_LOGIC ("No losses. Expected SN = " << m_expectedSeqNumber << ". Current SN = " << currSeqNumber);
      m_expectedSeqNumber++;
    }

  // Build list of SDUs
  uint8_t extensionBit;
  uint16_t lengthIndicator;
  do
    {
      extensionBit = rlcHeader.PopExtensionBit ();
      NS_LOG_LOGIC ("E = " << (uint16_t)extensionBit);

      if ( extensionBit == 0 )
        {
          m_sdusBuffer.push_back (packet);
        }
      else // extensionBit == 1
        {
          lengthIndicator = rlcHeader.PopLengthIndicator ();
          NS_LOG_LOGIC ("LI = " << lengthIndicator);

          // Check if there is enough data in the packet
          if ( lengthIndicator >= packet->GetSize () )
            {
              NS_LOG_LOGIC ("INTERNAL ERROR: Not enough data in the packet (" << packet->GetSize () << "). Needed LI=" << lengthIndicator);
            }

          // Split packet in two fragments
          Ptr<Packet> data_field = packet->CreateFragment (0, lengthIndicator);
          packet->RemoveAtStart (lengthIndicator);

          m_sdusBuffer.push_back (data_field);
        }
    }
  while ( extensionBit == 1 );

  std::list < Ptr<Packet> >::iterator it;

  // Current reassembling state
  if      (m_reassemblingState == WAITING_S0_FULL)  NS_LOG_LOGIC ("Reassembling State = 'WAITING_S0_FULL'");
  else if (m_reassemblingState == WAITING_SI_SF)    NS_LOG_LOGIC ("Reassembling State = 'WAITING_SI_SF'");
  else                                              NS_LOG_LOGIC ("Reassembling State = Unknown state");

  // Received framing Info
  NS_LOG_LOGIC ("Framing Info = " << (uint16_t)framingInfo);

  // Reassemble the list of SDUs (when there is no losses)
  if (!expectedSnLost)
    {
      switch (m_reassemblingState)
        {
          case WAITING_S0_FULL:
                  switch (framingInfo)
                    {
                      case (NistLteRlcHeader::FIRST_BYTE | NistLteRlcHeader::LAST_BYTE):
                              m_reassemblingState = WAITING_S0_FULL;

                              /**
                              * Deliver one or multiple PDUs
                              */
                              for ( it = m_sdusBuffer.begin () ; it != m_sdusBuffer.end () ; it++ )
                                {
                                  m_rlcSapUser->ReceivePdcpPdu (*it);
                                }
                              m_sdusBuffer.clear ();
                      break;

                      case (NistLteRlcHeader::FIRST_BYTE | NistLteRlcHeader::NO_LAST_BYTE):
                              m_reassemblingState = WAITING_SI_SF;

                              /**
                              * Deliver full PDUs
                              */
                              while ( m_sdusBuffer.size () > 1 )
                                {
                                  m_rlcSapUser->ReceivePdcpPdu (m_sdusBuffer.front ());
                                  m_sdusBuffer.pop_front ();
                                }

                              /**
                              * Keep S0
                              */
                              m_keepS0 = m_sdusBuffer.front ();
                              m_sdusBuffer.pop_front ();
                      break;

                      case (NistLteRlcHeader::NO_FIRST_BYTE | NistLteRlcHeader::LAST_BYTE):
                              m_reassemblingState = WAITING_S0_FULL;

                              /**
                               * Discard SI or SN
                               */
                              m_sdusBuffer.pop_front ();

                              /**
                               * Deliver zero, one or multiple PDUs
                               */
                              while ( ! m_sdusBuffer.empty () )
                                {
                                  m_rlcSapUser->ReceivePdcpPdu (m_sdusBuffer.front ());
                                  m_sdusBuffer.pop_front ();
                                }
                      break;

                      case (NistLteRlcHeader::NO_FIRST_BYTE | NistLteRlcHeader::NO_LAST_BYTE):
                              if ( m_sdusBuffer.size () == 1 )
                                {
                                  m_reassemblingState = WAITING_S0_FULL;
                                }
                              else
                                {
                                  m_reassemblingState = WAITING_SI_SF;
                                }

                              /**
                               * Discard SI or SN
                               */
                              m_sdusBuffer.pop_front ();

                              if ( m_sdusBuffer.size () > 0 )
                                {
                                  /**
                                   * Deliver zero, one or multiple PDUs
                                   */
                                  while ( m_sdusBuffer.size () > 1 )
                                    {
                                      m_rlcSapUser->ReceivePdcpPdu (m_sdusBuffer.front ());
                                      m_sdusBuffer.pop_front ();
                                    }

                                  /**
                                   * Keep S0
                                   */
                                  m_keepS0 = m_sdusBuffer.front ();
                                  m_sdusBuffer.pop_front ();
                                }
                      break;

                      default:
                              /**
                              * ERROR: Transition not possible
                              */
                              NS_LOG_LOGIC ("INTERNAL ERROR: Transition not possible. FI = " << (uint32_t) framingInfo);
                      break;
                    }
          break;

          case WAITING_SI_SF:
                  switch (framingInfo)
                    {
                      case (NistLteRlcHeader::NO_FIRST_BYTE | NistLteRlcHeader::LAST_BYTE):
                              m_reassemblingState = WAITING_S0_FULL;

                              /**
                              * Deliver (Kept)S0 + SN
                              */
                              m_keepS0->AddAtEnd (m_sdusBuffer.front ());
                              m_sdusBuffer.pop_front ();
                              m_rlcSapUser->ReceivePdcpPdu (m_keepS0);

                              /**
                                * Deliver zero, one or multiple PDUs
                                */
                              while ( ! m_sdusBuffer.empty () )
                                {
                                  m_rlcSapUser->ReceivePdcpPdu (m_sdusBuffer.front ());
                                  m_sdusBuffer.pop_front ();
                                }
                      break;

                      case (NistLteRlcHeader::NO_FIRST_BYTE | NistLteRlcHeader::NO_LAST_BYTE):
                              m_reassemblingState = WAITING_SI_SF;

                              /**
                              * Keep SI
                              */
                              if ( m_sdusBuffer.size () == 1 )
                                {
                                  m_keepS0->AddAtEnd (m_sdusBuffer.front ());
                                  m_sdusBuffer.pop_front ();
                                }
                              else // m_sdusBuffer.size () > 1
                                {
                                  /**
                                  * Deliver (Kept)S0 + SN
                                  */
                                  m_keepS0->AddAtEnd (m_sdusBuffer.front ());
                                  m_sdusBuffer.pop_front ();
                                  m_rlcSapUser->ReceivePdcpPdu (m_keepS0);

                                  /**
                                  * Deliver zero, one or multiple PDUs
                                  */
                                  while ( m_sdusBuffer.size () > 1 )
                                    {
                                      m_rlcSapUser->ReceivePdcpPdu (m_sdusBuffer.front ());
                                      m_sdusBuffer.pop_front ();
                                    }

                                  /**
                                  * Keep S0
                                  */
                                  m_keepS0 = m_sdusBuffer.front ();
                                  m_sdusBuffer.pop_front ();
                                }
                      break;

                      case (NistLteRlcHeader::FIRST_BYTE | NistLteRlcHeader::LAST_BYTE):
                      case (NistLteRlcHeader::FIRST_BYTE | NistLteRlcHeader::NO_LAST_BYTE):
                      default:
                              /**
                                * ERROR: Transition not possible
                                */
                              NS_LOG_LOGIC ("INTERNAL ERROR: Transition not possible. FI = " << (uint32_t) framingInfo);
                      break;
                    }
          break;

          default:
                NS_LOG_LOGIC ("INTERNAL ERROR: Wrong reassembling state = " << (uint32_t) m_reassemblingState);
          break;
        }
    }
  else // Reassemble the list of SDUs (when there are losses, i.e. the received SN is not the expected one)
    {
      switch (m_reassemblingState)
        {
          case WAITING_S0_FULL:
                  switch (framingInfo)
                    {
                      case (NistLteRlcHeader::FIRST_BYTE | NistLteRlcHeader::LAST_BYTE):
                              m_reassemblingState = WAITING_S0_FULL;

                              /**
                               * Deliver one or multiple PDUs
                               */
                              for ( it = m_sdusBuffer.begin () ; it != m_sdusBuffer.end () ; it++ )
                                {
                                  m_rlcSapUser->ReceivePdcpPdu (*it);
                                }
                              m_sdusBuffer.clear ();
                      break;

                      case (NistLteRlcHeader::FIRST_BYTE | NistLteRlcHeader::NO_LAST_BYTE):
                              m_reassemblingState = WAITING_SI_SF;

                              /**
                               * Deliver full PDUs
                               */
                              while ( m_sdusBuffer.size () > 1 )
                                {
                                  m_rlcSapUser->ReceivePdcpPdu (m_sdusBuffer.front ());
                                  m_sdusBuffer.pop_front ();
                                }

                              /**
                               * Keep S0
                               */
                              m_keepS0 = m_sdusBuffer.front ();
                              m_sdusBuffer.pop_front ();
                      break;

                      case (NistLteRlcHeader::NO_FIRST_BYTE | NistLteRlcHeader::LAST_BYTE):
                              m_reassemblingState = WAITING_S0_FULL;

                              /**
                               * Discard SN
                               */
                              m_sdusBuffer.pop_front ();

                              /**
                               * Deliver zero, one or multiple PDUs
                               */
                              while ( ! m_sdusBuffer.empty () )
                                {
                                  m_rlcSapUser->ReceivePdcpPdu (m_sdusBuffer.front ());
                                  m_sdusBuffer.pop_front ();
                                }
                      break;

                      case (NistLteRlcHeader::NO_FIRST_BYTE | NistLteRlcHeader::NO_LAST_BYTE):
                              if ( m_sdusBuffer.size () == 1 )
                                {
                                  m_reassemblingState = WAITING_S0_FULL;
                                }
                              else
                                {
                                  m_reassemblingState = WAITING_SI_SF;
                                }

                              /**
                               * Discard SI or SN
                               */
                              m_sdusBuffer.pop_front ();

                              if ( m_sdusBuffer.size () > 0 )
                                {
                                  /**
                                  * Deliver zero, one or multiple PDUs
                                  */
                                  while ( m_sdusBuffer.size () > 1 )
                                    {
                                      m_rlcSapUser->ReceivePdcpPdu (m_sdusBuffer.front ());
                                      m_sdusBuffer.pop_front ();
                                    }

                                  /**
                                  * Keep S0
                                  */
                                  m_keepS0 = m_sdusBuffer.front ();
                                  m_sdusBuffer.pop_front ();
                                }
                      break;

                      default:
                              /**
                               * ERROR: Transition not possible
                               */
                              NS_LOG_LOGIC ("INTERNAL ERROR: Transition not possible. FI = " << (uint32_t) framingInfo);
                      break;
                    }
          break;

          case WAITING_SI_SF:
                  switch (framingInfo)
                    {
                      case (NistLteRlcHeader::FIRST_BYTE | NistLteRlcHeader::LAST_BYTE):
                              m_reassemblingState = WAITING_S0_FULL;

                              /**
                               * Discard S0
                               */
                              m_keepS0 = 0;

                              /**
                               * Deliver one or multiple PDUs
                               */
                              while ( ! m_sdusBuffer.empty () )
                                {
                                  m_rlcSapUser->ReceivePdcpPdu (m_sdusBuffer.front ());
                                  m_sdusBuffer.pop_front ();
                                }
                      break;

                      case (NistLteRlcHeader::FIRST_BYTE | NistLteRlcHeader::NO_LAST_BYTE):
                              m_reassemblingState = WAITING_SI_SF;

                              /**
                               * Discard S0
                               */
                              m_keepS0 = 0;

                              /**
                               * Deliver zero, one or multiple PDUs
                               */
                              while ( m_sdusBuffer.size () > 1 )
                                {
                                  m_rlcSapUser->ReceivePdcpPdu (m_sdusBuffer.front ());
                                  m_sdusBuffer.pop_front ();
                                }

                              /**
                               * Keep S0
                               */
                              m_keepS0 = m_sdusBuffer.front ();
                              m_sdusBuffer.pop_front ();

                      break;

                      case (NistLteRlcHeader::NO_FIRST_BYTE | NistLteRlcHeader::LAST_BYTE):
                              m_reassemblingState = WAITING_S0_FULL;

                              /**
                               * Discard S0
                               */
                              m_keepS0 = 0;

                              /**
                               * Discard SI or SN
                               */
                              m_sdusBuffer.pop_front ();

                              /**
                               * Deliver zero, one or multiple PDUs
                               */
                              while ( ! m_sdusBuffer.empty () )
                                {
                                  m_rlcSapUser->ReceivePdcpPdu (m_sdusBuffer.front ());
                                  m_sdusBuffer.pop_front ();
                                }
                      break;

                      case (NistLteRlcHeader::NO_FIRST_BYTE | NistLteRlcHeader::NO_LAST_BYTE):
                              if ( m_sdusBuffer.size () == 1 )
                                {
                                  m_reassemblingState = WAITING_S0_FULL;
                                }
                              else
                                {
                                  m_reassemblingState = WAITING_SI_SF;
                                }

                              /**
                               * Discard S0
                               */
                              m_keepS0 = 0;

                              /**
                               * Discard SI or SN
                               */
                              m_sdusBuffer.pop_front ();

                              if ( m_sdusBuffer.size () > 0 )
                                {
                                  /**
                                   * Deliver zero, one or multiple PDUs
                                   */
                                  while ( m_sdusBuffer.size () > 1 )
                                    {
                                      m_rlcSapUser->ReceivePdcpPdu (m_sdusBuffer.front ());
                                      m_sdusBuffer.pop_front ();
                                    }

                                  /**
                                   * Keep S0
                                   */
                                  m_keepS0 = m_sdusBuffer.front ();
                                  m_sdusBuffer.pop_front ();
                                }
                      break;

                      default:
                              /**
                                * ERROR: Transition not possible
                                */
                              NS_LOG_LOGIC ("INTERNAL ERROR: Transition not possible. FI = " << (uint32_t) framingInfo);
                      break;
                    }
          break;

          default:
                NS_LOG_LOGIC ("INTERNAL ERROR: Wrong reassembling state = " << (uint32_t) m_reassemblingState);
          break;
        }
    }

}


void
NistLteRlcUm::ReassembleOutsideWindow (void)
{
  NS_LOG_LOGIC ("Reassemble Outside Window, Now: " << Simulator::Now().GetSeconds() << "s"); //LOGIC

  std::map <uint16_t, Ptr<Packet> >::iterator it;
  it = m_rxBuffer.begin ();
  //if(IsInsideReorderingWindow (NistSequenceNumber10 (it->first))){
  //NS_LOG_UNCOND("NistSequenceNumber10 (it->first): " << NistSequenceNumber10 (it->first));}
  while ( (it != m_rxBuffer.end ()) && ! IsInsideReorderingWindow (NistSequenceNumber10 (it->first)) )
    {
      NS_LOG_LOGIC ("SN = " << it->first);

      // Reassemble RLC SDUs and deliver the PDCP PDU to upper layer
      ReassembleAndDeliver (it->second);

      std::map <uint16_t, Ptr<Packet> >::iterator it_tmp = it;
      ++it;
      m_rxBuffer.erase (it_tmp);
    }

  if (it != m_rxBuffer.end ())
    {
      NS_LOG_LOGIC ("(SN = " << it->first << ") is inside the reordering window");
    }
}

void
NistLteRlcUm::ReassembleSnInterval (NistSequenceNumber10 lowSeqNumber, NistSequenceNumber10 highSeqNumber)
{
  NS_LOG_LOGIC ("Reassemble SN between " << lowSeqNumber << " and " << highSeqNumber << ", Now: " << Simulator::Now().GetSeconds() << "s");

  std::map <uint16_t, Ptr<Packet> >::iterator it;

  NistSequenceNumber10 reassembleSn = lowSeqNumber;
  NS_LOG_LOGIC ("reassembleSN = " << reassembleSn);
  NS_LOG_LOGIC ("highSeqNumber = " << highSeqNumber);
  while (reassembleSn.GetValue() < highSeqNumber.GetValue())
    {
        NS_LOG_LOGIC ("reassembleSN = " << reassembleSn);
        NS_LOG_LOGIC ("highSeqNumber = " << highSeqNumber);

      it = m_rxBuffer.find (reassembleSn.GetValue ());
      NS_LOG_LOGIC ("it->first  = " << it->first);
      NS_LOG_LOGIC ("it->second = " << it->second);
      if (it != m_rxBuffer.end () )
        {
          NS_LOG_LOGIC ("SN = " << it->first);

          // Reassemble RLC SDUs and deliver the PDCP PDU to upper layer
          ReassembleAndDeliver (it->second);

          m_rxBuffer.erase (it);
        }
        
      reassembleSn++;
      NS_LOG_DEBUG("Incremented reassembleSn, now reassembleSn = " << reassembleSn.GetValue () << " highseq = " << highSeqNumber.GetValue () );
    }
    NS_LOG_DEBUG("Out of the while");
}


void
NistLteRlcUm::DoReportBufferNistStatus (void)
{
  Time holDelay (0);
  uint32_t queueSize = 0;
 // uint8_t V2XMessageType = 0x02; //fake 
 // uint8_t V2XPPPP = 0x00;

  NistLteMacSapProvider::NistReportBufferNistStatusParameters r;

  if (! m_txBuffer.empty ())
    {
      NistRlcTag holTimeTag;
      m_txBuffer.front ()->PeekPacketTag (holTimeTag);
      holDelay = Simulator::Now () - holTimeTag.GetSenderTimestamp ();

      queueSize = m_txBufferSize + 2 * m_txBuffer.size (); // Data in tx queue + estimated headers size
        
      NrV2XTag v2xTag;
      if (m_txBuffer.front () -> FindFirstMatchingByteTag (v2xTag))
        {
            r.V2XMessageType = v2xTag.GetMessageType ();
            r.V2XTrafficType = v2xTag.GetTrafficType ();
            r.V2XPPPP = v2xTag.GetPPPP ();
            r.V2XPdb = v2xTag.GetPdb ();
            r.V2XPrsvp = v2xTag.GetPrsvp ();
            r.V2XReselectionCounter = v2xTag.GetReselectionCounter();
            r.V2XPacketSize = v2xTag.GetPacketSize();
            r.V2XReservationSize = v2xTag.GetReservationSize();
            r.V2XNodeId = v2xTag.GetNodeId ();
            r.isNewV2X = true;
            r.V2XGenTime = v2xTag.GetGenTime();
            r.V2XPacketID = v2xTag.GetPacketId();
        }
    }



  
  r.rnti = m_rnti;
  r.lcid = m_lcid;
  r.srcL2Id = m_srcL2Id;
  r.dstL2Id = m_dstL2Id;
  r.txQueueSize = queueSize;
  r.txQueueHolDelay = holDelay.GetMilliSeconds () ;
  r.retxQueueSize = 0;
  r.retxQueueHolDelay = 0;
  r.statusPduSize = 0;

 //TODO FIXME NEW for V2X       ~ maybe wrong
  r.alreadyUESelected = false;

  NS_LOG_UNCOND ("Send ReportBufferNistStatus = " << r.txQueueSize << ", " << r.txQueueHolDelay );
 // std::cin.get(); // Pause the program and check the buffer size
  /*if (Simulator::Now ().GetSeconds() == 0.298701)
    {
      std::ofstream alert;
      alert.open(m_outputPath + "RLCAlert.txt", std::ios_base::app);
      alert << "Send ReportBufferNistStatus = " << r.txQueueSize << ", " << r.txQueueHolDelay << "\r\n";
      alert.close();
    }*/
  m_macSapProvider->ReportBufferNistStatus (r);
}


void
NistLteRlcUm::ExpireReorderingTimer (void)
{
  NS_LOG_FUNCTION (this << m_rnti << (uint32_t) m_lcid);
  NS_LOG_LOGIC ("Reordering timer has expired");

  // 5.1.2.2.4 Actions when t-Reordering expires
  // When t-Reordering expires, the receiving UM RLC entity shall:
  // - update VR(UR) to the SN of the first UMD PDU with SN >= VR(UX) that has not been received;
  // - reassemble RLC SDUs from any UMD PDUs with SN < updated VR(UR), remove RLC headers when doing so
  //   and deliver the reassembled RLC SDUs to upper layer in ascending order of the RLC SN if not delivered before;
  // - if VR(UH) > VR(UR):
  //    - start t-Reordering;
  //    - set VR(UX) to VR(UH).

  std::map <uint16_t, Ptr<Packet> >::iterator it;
  NistSequenceNumber10 newVrUr = m_vrUx;

  while ( (it = m_rxBuffer.find (newVrUr.GetValue ())) != m_rxBuffer.end () )
    {
      newVrUr++;
    }
  NistSequenceNumber10 oldVrUr = m_vrUr;
  m_vrUr = newVrUr;
  NS_LOG_LOGIC ("New VR(UR) = " << m_vrUr << ", Now: " << Simulator::Now().GetSeconds() << "s");

  ReassembleSnInterval (oldVrUr, m_vrUr);

  if ( m_vrUh > m_vrUr)
    {
      NS_LOG_LOGIC ("Start reordering timer");
      m_reorderingTimer = Simulator::Schedule (Time ("0.1s"),
                                               &NistLteRlcUm::ExpireReorderingTimer, this);
      m_vrUx = m_vrUh;
      NS_LOG_LOGIC ("New VR(UX) = " << m_vrUx);
    }
}


void
NistLteRlcUm::ExpireRbsTimer (void)
{
  NS_LOG_LOGIC ("RBS Timer expires");

  if (! m_txBuffer.empty ())
    {
      DoReportBufferNistStatus ();
      m_rbsTimer = Simulator::Schedule (MilliSeconds (10), &NistLteRlcUm::ExpireRbsTimer, this);
    }
}

} // namespace ns3
