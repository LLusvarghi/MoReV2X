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



#include <ns3/log.h>
#include <ns3/pointer.h>
#include <ns3/packet.h>
#include <ns3/packet-burst.h>
#include <ns3/random-variable-stream.h>

#include "nr-v2x-ue-mac.h"
#include "nr-v2x-ue-net-device.h"
#include "nist-lte-radio-bearer-tag.h"
#include <ns3/nist-lte-control-messages.h>
#include <ns3/simulator.h>
#include <ns3/nist-lte-common.h>
#include <ns3/string.h>
#include <ns3/enum.h>
#include <inttypes.h>  
#include <ns3/boolean.h>
#include <bitset>
#include <cmath>
#include <algorithm>

#include <iostream>
#include <fstream>

#include "nr-v2x-utils.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("NrV2XUeMac");

NS_OBJECT_ENSURE_REGISTERED (NrV2XUeMac);

////////////////////
// SAP forwarders //
////////////////////


class NistUeMemberLteUeCmacSapProvider : public NistLteUeCmacSapProvider
{
public:
	NistUeMemberLteUeCmacSapProvider (NrV2XUeMac* mac);

	// inherited from NistLteUeCmacSapProvider
	virtual void ConfigureRach (NistRachConfig rc);
	virtual void StartContentionBasedRandomAccessProcedure ();
	virtual void StartNonContentionBasedRandomAccessProcedure (uint16_t rnti, uint8_t preambleId, uint8_t prachMask);
	virtual void AddLc (uint8_t lcId, NistLteUeCmacSapProvider::NistLogicalChannelConfig lcConfig, NistLteMacSapUser* msu);
	virtual void AddLc (uint8_t lcId, uint32_t srcL2Id, uint32_t dstL2Id, NistLteUeCmacSapProvider::NistLogicalChannelConfig lcConfig, NistLteMacSapUser* msu);
	virtual void RemoveLc (uint8_t lcId);
	virtual void RemoveLc (uint8_t lcId, uint32_t srcL2Id, uint32_t dstL2Id);
	virtual void Reset ();
        //Communication
	virtual void AddSlTxPool (uint32_t dstL2Id, Ptr<SidelinkTxCommResourcePool> pool);
	virtual void RemoveSlTxPool (uint32_t dstL2Id);
	virtual void SetSlRxPools (std::list<Ptr<SidelinkRxCommResourcePool> > pools);
	virtual void AddSlDestination (uint32_t destination);
	virtual void RemoveSlDestination (uint32_t destination);
        //Discovery
        virtual void AddSlTxPool (Ptr<SidelinkTxDiscResourcePool> pool);
        virtual void RemoveSlTxPool ();
        virtual void SetSlRxPools (std::list<Ptr<SidelinkRxDiscResourcePool> > pools);
        virtual void ModifyDiscTxApps (std::list<uint32_t> apps);
        virtual void ModifyDiscRxApps (std::list<uint32_t> apps);
  
	virtual void SetRnti (uint16_t rnti);
	// added to handle LC priority
	virtual void AddLCPriority ( uint8_t rnti, uint8_t lcid, uint8_t priority);

private:
	NrV2XUeMac* m_mac;
};


NistUeMemberLteUeCmacSapProvider::NistUeMemberLteUeCmacSapProvider (NrV2XUeMac* mac)
: m_mac (mac)
{
}

void 
NistUeMemberLteUeCmacSapProvider::ConfigureRach (NistRachConfig rc)
{
	m_mac->DoConfigureRach (rc);
}

void
NistUeMemberLteUeCmacSapProvider::StartContentionBasedRandomAccessProcedure ()
{
	m_mac->DoStartContentionBasedRandomAccessProcedure ();
}

void
NistUeMemberLteUeCmacSapProvider::StartNonContentionBasedRandomAccessProcedure (uint16_t rnti, uint8_t preambleId, uint8_t prachMask)
{
	m_mac->DoStartNonContentionBasedRandomAccessProcedure (rnti, preambleId, prachMask);
}


void
NistUeMemberLteUeCmacSapProvider::AddLc (uint8_t lcId, NistLogicalChannelConfig lcConfig, NistLteMacSapUser* msu)
{
	m_mac->DoAddLc (lcId, lcConfig, msu);
}

void
NistUeMemberLteUeCmacSapProvider::AddLc (uint8_t lcId, uint32_t srcL2Id, uint32_t dstL2Id, NistLogicalChannelConfig lcConfig, NistLteMacSapUser* msu)
{
	m_mac->DoAddLc (lcId, srcL2Id, dstL2Id, lcConfig, msu);
}

void
NistUeMemberLteUeCmacSapProvider::RemoveLc (uint8_t lcid)
{
	m_mac->DoRemoveLc (lcid);
}

void
NistUeMemberLteUeCmacSapProvider::RemoveLc (uint8_t lcid, uint32_t srcL2Id, uint32_t dstL2Id)
{
	m_mac->DoRemoveLc (lcid, srcL2Id, dstL2Id);
}

void
NistUeMemberLteUeCmacSapProvider::Reset ()
{
	m_mac->DoReset ();
}

void
NistUeMemberLteUeCmacSapProvider::AddSlTxPool (Ptr<SidelinkTxDiscResourcePool> pool)
{
  m_mac->DoAddSlTxPool (pool);
}

void
NistUeMemberLteUeCmacSapProvider::RemoveSlTxPool ()
{
  m_mac->DoRemoveSlTxPool ();
}
  
void
NistUeMemberLteUeCmacSapProvider::SetSlRxPools (std::list<Ptr<SidelinkRxDiscResourcePool> > pools)
{
  m_mac->DoSetSlRxPools (pools);
}

void
NistUeMemberLteUeCmacSapProvider::AddSlTxPool (uint32_t dstL2Id, Ptr<SidelinkTxCommResourcePool> pool)
{
	m_mac->DoAddSlTxPool (dstL2Id, pool);
}

void
NistUeMemberLteUeCmacSapProvider::RemoveSlTxPool (uint32_t dstL2Id)
{
	m_mac->DoRemoveSlTxPool (dstL2Id);
}

void
NistUeMemberLteUeCmacSapProvider::SetSlRxPools (std::list<Ptr<SidelinkRxCommResourcePool> > pools)
{
	m_mac->DoSetSlRxPools (pools);
}

void
NistUeMemberLteUeCmacSapProvider::AddSlDestination (uint32_t destination)
{
	m_mac->DoAddSlDestination (destination);
}

void
NistUeMemberLteUeCmacSapProvider::RemoveSlDestination (uint32_t destination)
{
	m_mac->DoRemoveSlDestination (destination);
}

void
NistUeMemberLteUeCmacSapProvider::SetRnti (uint16_t rnti)
{
	m_mac->DoSetRnti (rnti);
}

void
NistUeMemberLteUeCmacSapProvider::ModifyDiscTxApps (std::list<uint32_t> apps)
{
  m_mac->DoModifyDiscTxApps (apps);
}

void
NistUeMemberLteUeCmacSapProvider::ModifyDiscRxApps (std::list<uint32_t> apps)
{
  m_mac->DoModifyDiscRxApps (apps);
}

// added function to handle priority for LC 
void 
NistUeMemberLteUeCmacSapProvider::AddLCPriority ( uint8_t rnti, uint8_t lcid, uint8_t priority)
{
	m_mac->DoAddLCPriority (rnti, lcid, priority);
}  

class NistUeMemberLteMacSapProvider : public NistLteMacSapProvider
{
public:
	NistUeMemberLteMacSapProvider (NrV2XUeMac* mac);

	// inherited from NistLteMacSapProvider
	virtual void TransmitPdu (NistTransmitPduParameters params);
	virtual void ReportBufferNistStatus (NistReportBufferNistStatusParameters params);

private:
	NrV2XUeMac* m_mac;
};


NistUeMemberLteMacSapProvider::NistUeMemberLteMacSapProvider (NrV2XUeMac* mac)
: m_mac (mac)
{
}

void
NistUeMemberLteMacSapProvider::TransmitPdu (NistTransmitPduParameters params)
{
	m_mac->DoTransmitPdu (params);
}


void
NistUeMemberLteMacSapProvider::ReportBufferNistStatus (NistReportBufferNistStatusParameters params)
{
	m_mac->DoReportBufferNistStatus (params);
}




class NistUeMemberLteUePhySapUser : public NistLteUePhySapUser
{
public:
	NistUeMemberLteUePhySapUser (NrV2XUeMac* mac);

	// inherited from NistLtePhySapUser
	virtual void ReceivePhyPdu (Ptr<Packet> p);
	virtual void SubframeIndication (uint32_t frameNo, uint32_t subframeNo);
	virtual void ReceiveNistLteControlMessage (Ptr<NistLteControlMessage> msg);
	virtual void NotifyChangeOfTiming (uint32_t frameNo, uint32_t subframeNo);

        virtual void ReportPsschRsrp (Time time, uint16_t rbStart, uint16_t rbLen, double rsrpDb);
        virtual void ReportPsschRsrpReservation (Time time, uint16_t rbStart, uint16_t rbLen, double rsrpDb, SidelinkCommResourcePool::SubframeInfo receivedSubframe, SidelinkCommResourcePool::SubframeInfo reservedSubframe, uint32_t CreselRx, uint32_t nodeId, uint16_t RRI);
        virtual void StoreTxInfo (SidelinkCommResourcePool::SubframeInfo subframe, uint16_t rbStart, uint16_t rbLen);

private:
	NrV2XUeMac* m_mac;
};

NistUeMemberLteUePhySapUser::NistUeMemberLteUePhySapUser (NrV2XUeMac* mac) : m_mac (mac)
{

}

void
NistUeMemberLteUePhySapUser::ReceivePhyPdu (Ptr<Packet> p)
{
	m_mac->DoReceivePhyPdu (p);
}


void
NistUeMemberLteUePhySapUser::SubframeIndication (uint32_t frameNo, uint32_t subframeNo)
{
	m_mac->DoSubframeIndication (frameNo, subframeNo);
}

void
NistUeMemberLteUePhySapUser::ReceiveNistLteControlMessage (Ptr<NistLteControlMessage> msg)
{
	m_mac->DoReceiveNistLteControlMessage (msg);
}

void
NistUeMemberLteUePhySapUser::NotifyChangeOfTiming (uint32_t frameNo, uint32_t subframeNo)
{
	m_mac->DoNotifyChangeOfTiming (frameNo, subframeNo);
}


void
NistUeMemberLteUePhySapUser::ReportPsschRsrp (Time time, uint16_t rbStart, uint16_t rbLen, double rsrpDb)
{
	m_mac->DoReportPsschRsrp (time, rbStart, rbLen, rsrpDb);

}

void
NistUeMemberLteUePhySapUser::ReportPsschRsrpReservation (Time time, uint16_t rbStart, uint16_t rbLen, double rsrpDb, SidelinkCommResourcePool::SubframeInfo receivedSubframe, SidelinkCommResourcePool::SubframeInfo reservedSubframe, uint32_t CreselRx, uint32_t nodeId, uint16_t RRI)
{
	m_mac->DoReportPsschRsrpReservation (time, rbStart, rbLen, rsrpDb, receivedSubframe, reservedSubframe, CreselRx, nodeId, RRI);
}

void
NistUeMemberLteUePhySapUser::StoreTxInfo (SidelinkCommResourcePool::SubframeInfo subframe, uint16_t rbStart, uint16_t rbLen)
{
       m_mac->DoStoreTxInfo (subframe, rbStart, rbLen);
}



//////////////////////////////////////////////////////////
// NrV2XUeMac methods
/////////////////////////////////////////////////////////


std::vector<NrV2XUeMac::UeSelectionInfo> NrV2XUeMac::SelectedGrants;
double NrV2XUeMac::prevPrintTime_selection = 0.0;

std::vector<NrV2XUeMac::UeReEvaluationInfo> NrV2XUeMac::ReEvaluationStats;
double NrV2XUeMac::prevPrintTime_reEvaluation = 0.0;

std::vector<NrV2XUeMac::TxPacketInfo> NrV2XUeMac::TxPacketsStats;
double NrV2XUeMac::prevPrintTime_packetInfo = 0.0;

TypeId
NrV2XUeMac::GetTypeId (void)
{
	static TypeId tid = TypeId ("ns3::NrV2XUeMac")
		.SetParent<Object> ()
		.AddConstructor<NrV2XUeMac> ()
		.AddAttribute ("SlGrantMcs",
					"The MCS of the SL grant, must be [0..15] (default 0)",
					UintegerValue (0),
					MakeUintegerAccessor (&NrV2XUeMac::m_slGrantMcs),
					MakeUintegerChecker<uint8_t> ())
                .AddAttribute ("RandomV2VSelection",
                                        "Whether the resources for V2V transmissions are randomly selected in UE_SELECTED mode (default false)",
                                        BooleanValue (false),
                                        MakeBooleanAccessor (&NrV2XUeMac::m_randomSelection),
                                        MakeBooleanChecker ())
		.AddTraceSource ("SlUeScheduling",
					"Information regarding SL UE scheduling",
					MakeTraceSourceAccessor (&NrV2XUeMac::m_slUeScheduling),
					"ns3::NistSlUeMacStatParameters::TracedCallback")
		.AddTraceSource ("SlSharedChUeScheduling",
					"Information regarding SL Shared Channel UE scheduling",
					MakeTraceSourceAccessor (&NrV2XUeMac::m_slSharedChUeScheduling),
					"ns3::NistSlUeMacStatParameters::TracedCallback")
   		 // Added to trace the transmission of discovery message
                .AddAttribute ("ListL2Enabled",
                                        "Enable List L2 of Mode 4 SSPS",
                                        BooleanValue (false),
                                        MakeBooleanAccessor (&NrV2XUeMac::m_List2Enabled),
                                        MakeBooleanChecker ())
                .AddAttribute ("UseRxCresel",
                                        "Use the received SCI reselection counter",
                                        BooleanValue (false),
                                        MakeBooleanAccessor (&NrV2XUeMac::m_useRxCresel),
                                        MakeBooleanChecker ())
                .AddAttribute ("AggressiveMode4",
                                        "Use Aggressive Mode 4 strategy when PDB is violated",
                                        BooleanValue (false),
                                        MakeBooleanAccessor (&NrV2XUeMac::m_aggressive),
                                        MakeBooleanChecker ())
                .AddAttribute ("OneShot",
                                        "Use one-shot transmission when a reselection is triggered",
                                        BooleanValue (false),
                                        MakeBooleanAccessor (&NrV2XUeMac::m_oneShot),
                                        MakeBooleanChecker ())
                .AddAttribute ("SubmissiveMode4",
                                        "Use Submissive Mode 4 strategy when PDB is violated",
                                        BooleanValue (false),
                                        MakeBooleanAccessor (&NrV2XUeMac::m_submissive),
                                        MakeBooleanChecker ())
		.AddAttribute ("SubchannelSize",
					"The Subchannel size (in RBs)",
					UintegerValue (10),
					MakeUintegerAccessor (&NrV2XUeMac::m_nsubCHsize),
					MakeUintegerChecker<uint16_t> ())
		.AddAttribute ("RBsBandwidth",
					"The bandwidth at MAC sublayer (in RBs)",
					UintegerValue (50),
					MakeUintegerAccessor (&NrV2XUeMac::m_BW_RBs),
					MakeUintegerChecker<uint16_t> ())
		.AddAttribute ("SlotDuration",
					"The NR-V2X time slot duration",
					DoubleValue (1.0),
					MakeDoubleAccessor (&NrV2XUeMac::m_slotDuration),
					MakeDoubleChecker<double> ())
		.AddAttribute ("NumerologyIndex",
					"The NR-V2X numerology index",
					UintegerValue (0),
					MakeUintegerAccessor (&NrV2XUeMac::m_numerologyIndex),
					MakeUintegerChecker<uint16_t> ())
                .AddAttribute ("AllowReEvaluation",
                                        "Enable the re-evaluation mechanism",
                                        BooleanValue (false),
                                        MakeBooleanAccessor (&NrV2XUeMac::m_reEvaluation),
                                        MakeBooleanChecker ())
                .AddAttribute ("AllSlotsReEvaluation",
                                        "Re-evaluation performed according to the all-slots solution",
                                        BooleanValue (false),
                                        MakeBooleanAccessor (&NrV2XUeMac::m_allSlotsReEvaluation),
                                        MakeBooleanChecker ())
                .AddAttribute ("OutputPath",
                                        "Specifiy the output path where to store the results",
                                        StringValue ("results/sidelink/"),
                                        MakeStringAccessor (&NrV2XUeMac::m_outputPath),
                                        MakeStringChecker ())
                .AddAttribute ("SavingPeriod",
                                        "The period used to save data",
                                        DoubleValue (1.0),
                                        MakeDoubleAccessor (&NrV2XUeMac::m_savingPeriod),
                                        MakeDoubleChecker<double> ())

;																									;
	return tid;
}


NrV2XUeMac::NrV2XUeMac ()
:  
   m_cphySapProvider (0),
   m_bsrPeriodicity (MilliSeconds (1)), // ideal behavior
   m_bsrLast (MilliSeconds (0)),
   m_freshUlBsr (false),
   m_harqProcessId (0),
   m_rnti (0),
   m_rachConfigured (false),
   m_waitingForRaResponse (false),
   m_slBsrPeriodicity (MilliSeconds (1)),
   m_slBsrLast (MilliSeconds (0)),
   m_freshSlBsr (false),   
   m_alreadyUeSelectedSlBsr (false), //to check again whether there is a fresh SL BSR in V2V UE selected mode (Mode 4)
   m_absSFN (0), //the absolute SFN cycle number --> incremented every 1024 frames
   m_millisecondsFromLastAbsSFNUpdate (0), 
//   m_nsubCHsize (10),
   m_L_SubCh (1),
//   m_BW_RBs (50),
   m_UnutilizedReservations (0),
   m_UnutilizedSubchannels (0), 
   m_ReservedSubchannels (0),
   m_Reservations (0),
   m_LatencyReselections (0),
   m_SizeReselections (0),
   m_CounterReselections (0),
   m_TotalTransmissions (0),
   m_prevPrintTime (0.0),
//   m_allSlotsReEvaluation (false),
   m_keepProbability (0.0),
   m_sizeThreshold (0.2),
   m_rsrpThreshold (-120),
   m_sensingWindow (1100),
   m_oneShotGrant (false),
//   m_slotDuration (1.0), // Expressed in ms
//   m_RRIvalues ({3, 11, 20, 50, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000})
//   m_RRIvalues ({20, 50, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000})
   m_RRIvalues ({100})
{
   NS_LOG_FUNCTION (this);
   
   NS_ASSERT_MSG(m_RRIvalues.size() < 16, "Maximum size of the RRI list is 16");
   NS_ASSERT_MSG(m_keepProbability >= 0, "Keep probability must be non-negative");

   m_debugNode = 0;

   m_prevListUpdate.frameNo = 0;
   m_prevListUpdate.subframeNo = 0;
   m_miUlHarqProcessesPacket.resize (HARQ_PERIOD);
   for (uint8_t i = 0; i < m_miUlHarqProcessesPacket.size (); i++)
   {
     Ptr<PacketBurst> pb = CreateObject <PacketBurst> ();
     m_miUlHarqProcessesPacket.at (i) = pb;
   }
   m_miUlHarqProcessesPacketTimer.resize (HARQ_PERIOD, 0);

   m_macSapProvider = new NistUeMemberLteMacSapProvider (this);
   m_cmacSapProvider = new NistUeMemberLteUeCmacSapProvider (this);
   m_uePhySapUser = new NistUeMemberLteUePhySapUser (this);
   m_raPreambleUniformVariable = CreateObject<UniformRandomVariable> ();

   m_amc = CreateObject <NistLteAmc> ();
   m_NRamc = CreateObject <NrV2XAmc> ();
   m_ueSelectedUniformVariable = CreateObject<UniformRandomVariable> ();
  //m_slDiversity.status = SlDiversity::disabled;//enabled should be default!
  
   m_p1UniformVariable = CreateObject<UniformRandomVariable> ();
   m_resUniformVariable = CreateObject<UniformRandomVariable> ();
   m_evalKeepProb = CreateObject<UniformRandomVariable> (); // Default range is [0,1)

}


NrV2XUeMac::~NrV2XUeMac ()
{
	NS_LOG_FUNCTION (this);
}

void
NrV2XUeMac::DoDispose ()
{
	NS_LOG_FUNCTION (this);
	m_miUlHarqProcessesPacket.clear ();
	delete m_macSapProvider;
	delete m_cmacSapProvider;
	delete m_uePhySapUser;
	Object::DoDispose ();
}

void
NrV2XUeMac::SetNistLteUeCphySapProvider (NistLteUeCphySapProvider * s)
{
  NS_LOG_FUNCTION (this << s);
  m_cphySapProvider = s;
}

NistLteUePhySapUser*
NrV2XUeMac::GetNistLteUePhySapUser (void)
{
	return m_uePhySapUser;
}

void
NrV2XUeMac::SetNistLteUePhySapProvider (NistLteUePhySapProvider* s)
{
	m_uePhySapProvider = s;
}


NistLteMacSapProvider*
NrV2XUeMac::GetNistLteMacSapProvider (void)
{
	return m_macSapProvider;
}

void
NrV2XUeMac::SetNistLteUeCmacSapUser (NistLteUeCmacSapUser* s)
{
	m_cmacSapUser = s;
}

NistLteUeCmacSapProvider*
NrV2XUeMac::GetNistLteUeCmacSapProvider (void)
{
	return m_cmacSapProvider;
}


void
NrV2XUeMac::DoTransmitPdu (NistLteMacSapProvider::NistTransmitPduParameters params)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT_MSG (m_rnti == params.rnti, "RNTI mismatch between RLC and MAC");
  if (params.srcL2Id == 0)
  {
    NistLteRadioBearerTag tag (params.rnti, params.lcid, 0 /* UE works in SISO mode*/);
    params.pdu->AddPacketTag (tag);
    // store pdu in HARQ buffer
    m_miUlHarqProcessesPacket.at (m_harqProcessId)->AddPacket (params.pdu);
    m_miUlHarqProcessesPacketTimer.at (m_harqProcessId) = HARQ_PERIOD;
    m_uePhySapProvider->SendMacPdu (params.pdu);
  }
  else
  {
    NS_LOG_INFO ("Transmitting sidelink PDU");
    //find transmitting pool
    std::map <uint32_t, PoolInfo>::iterator poolIt = m_sidelinkTxPoolsMap.find (params.dstL2Id);
    NS_ASSERT (poolIt!= m_sidelinkTxPoolsMap.end());
    NistLteRadioBearerTag tag (params.rnti, params.lcid, params.srcL2Id, params.dstL2Id);
    params.pdu->AddPacketTag (tag);
    // store pdu in HARQ buffer
    poolIt->second.m_miSlHarqProcessPacket->AddPacket (params.pdu);
    m_uePhySapProvider->SendMacPdu (params.pdu);
  }
}

void
NrV2XUeMac::DoReportBufferNistStatus (NistLteMacSapProvider::NistReportBufferNistStatusParameters params)
{
  NS_LOG_FUNCTION (this << (uint32_t) params.lcid);
  if (params.srcL2Id == 0)
  {
    NS_FATAL_ERROR("Scheduler not available");
  } 
  else
  {
    NS_LOG_INFO ("Reporting for sidelink");
    //sidelink BSR
    std::map <SidelinkLcIdentifier, NistLteMacSapProvider::NistReportBufferNistStatusParameters>::iterator it;
    // m_slBsrReceived is the last BSR received from the RLC layer. I have to update it with the content of the "params" container coming from above
//  std::printf ("Message type: %" PRIu32 ", PDB: %" PRIu32 " and Traffic type: %" PRIu32 "\n", params.V2XMessageType, params.V2XPdb, params.V2XTrafficType);
    SidelinkLcIdentifier sllcid;
    sllcid.lcId = params.lcid;
    sllcid.srcL2Id = params.srcL2Id;
    sllcid.dstL2Id = params.dstL2Id;
    it = m_slBsrReceived.find (sllcid);
    if (it != m_slBsrReceived.end ())
    {
      // update entry
      (*it).second = params;
    }
    else
    {
      m_slBsrReceived.insert (std::pair<SidelinkLcIdentifier, NistLteMacSapProvider::NistReportBufferNistStatusParameters> (sllcid, params));
    }
    m_freshSlBsr = true;
  }
}


void
NrV2XUeMac::SendReportBufferNistStatus (void)
{
   NS_LOG_FUNCTION (this);
   //Useless
}

void
NrV2XUeMac::SendSidelinkReportBufferStatus (void)
{
	NS_LOG_FUNCTION (this);

	if (m_rnti == 0)
	{
		NS_LOG_INFO ("MAC not initialized, BSR deferred");
		return;
	}

	//check if we have at scheduled pools
	bool scheduled = false;
	for (std::map <uint32_t, PoolInfo >::iterator slTxPoolIt = m_sidelinkTxPoolsMap.begin (); slTxPoolIt != m_sidelinkTxPoolsMap.end () && !scheduled; slTxPoolIt++)
	{
		if (slTxPoolIt->second.m_pool->GetSchedulingType () == SidelinkCommResourcePool::SCHEDULED)
			scheduled = true;
	}



	if (m_slBsrReceived.size () == 0 || !scheduled) 
	{
		NS_LOG_INFO (this << " No SL BSR report to transmit. SL BSR size=" << m_slBsrReceived.size ());
		return;
	}
	NistMacCeListElement_s bsr;
	bsr.m_rnti = m_rnti;
	bsr.m_macCeType = NistMacCeListElement_s::SLBSR;

	// SL BSR is reported for each Group Index

	std::map <SidelinkLcIdentifier, NistLteMacSapProvider::NistReportBufferNistStatusParameters>::iterator it;

	std::vector<uint32_t> queue (4, 0); //TODO: change to create based on the number of destinations, initialized to 0
	for (it = m_slBsrReceived.begin (); it != m_slBsrReceived.end (); it++)
	{
		//uint8_t lcid = it->first.lcId;
		uint32_t dstL2Id = it->first.dstL2Id;

		std::map <SidelinkLcIdentifier, NistLcInfo>::iterator slLcInfoMapIt;
		slLcInfoMapIt = m_slLcInfoMap.find (it->first);
		NS_ASSERT (slLcInfoMapIt !=  m_slLcInfoMap.end ());
		//TODO: find the mapping between the destination and the group index (must be provided by RRC)
		//uint8_t lcg = slLcInfoMapIt->second.lcConfig.logicalChannelGroup;
		//queue.at (lcg) += ((*it).second.txQueueSize + (*it).second.retxQueueSize + (*it).second.statusPduSize);
		std::map <uint32_t, PoolInfo >::iterator slTxPoolIt;
		slTxPoolIt = m_sidelinkTxPoolsMap.find (dstL2Id);
		Ptr<SidelinkTxCommResourcePool> pool = DynamicCast<SidelinkTxCommResourcePool> (slTxPoolIt->second.m_pool);
		NS_ASSERT (slTxPoolIt != m_sidelinkTxPoolsMap.end ());
		if (pool->GetSchedulingType() == SidelinkCommResourcePool::SCHEDULED)
		{
			queue.at (pool->GetIndex()) += ((*it).second.txQueueSize + (*it).second.retxQueueSize + (*it).second.statusPduSize);
		}
	}

	// store
	bsr.m_macCeValue.m_bufferNistStatus.push_back (NistBufferSizeLevelBsr::BufferSize2BsrId (queue.at (0)));
	bsr.m_macCeValue.m_bufferNistStatus.push_back (NistBufferSizeLevelBsr::BufferSize2BsrId (queue.at (1)));
	bsr.m_macCeValue.m_bufferNistStatus.push_back (NistBufferSizeLevelBsr::BufferSize2BsrId (queue.at (2)));
	bsr.m_macCeValue.m_bufferNistStatus.push_back (NistBufferSizeLevelBsr::BufferSize2BsrId (queue.at (3)));

	// create the feedback to eNB
	Ptr<NistBsrLteControlMessage> msg = Create<NistBsrLteControlMessage> ();
	msg->SetBsr (bsr);
	m_uePhySapProvider->SendNistLteControlMessage (msg);

}


void 
NrV2XUeMac::RandomlySelectAndSendRaPreamble ()
{
   NS_LOG_FUNCTION (this);
   //Useless
}

void
NrV2XUeMac::SendRaPreamble (bool contention)
{
   NS_LOG_FUNCTION (this << (uint32_t) m_raPreambleId << contention);
   //Useless
}

void 
NrV2XUeMac::StartWaitingForRaResponse ()
{
	NS_LOG_FUNCTION (this);
	m_waitingForRaResponse = true;
}

void 
NrV2XUeMac::RecvRaResponse (NistBuildNistRarListElement_s raResponse)
{
   NS_LOG_FUNCTION (this);
   //Useless
}

void 
NrV2XUeMac::RaResponseTimeout (bool contention)
{
   NS_LOG_FUNCTION (this << contention);
   //Useless
}

void 
NrV2XUeMac::DoConfigureRach (NistLteUeCmacSapProvider::NistRachConfig rc)
{
   NS_LOG_FUNCTION (this);
   //Useless
}

void 
NrV2XUeMac::DoStartContentionBasedRandomAccessProcedure ()
{
   NS_LOG_FUNCTION (this);
   //Useless
}

void 
NrV2XUeMac::DoStartNonContentionBasedRandomAccessProcedure (uint16_t rnti, uint8_t preambleId, uint8_t prachMask)
{
   NS_LOG_FUNCTION (this << " rnti" << rnti);
   //Useless
}

void
NrV2XUeMac::DoAddLc (uint8_t lcId,  NistLteUeCmacSapProvider::NistLogicalChannelConfig lcConfig, NistLteMacSapUser* msu)
{
	NS_LOG_FUNCTION (this << " lcId" << (uint32_t) lcId);
	NS_ASSERT_MSG (m_lcInfoMap.find (lcId) == m_lcInfoMap.end (), "cannot add channel because LCID " << lcId << " is already present");
       
	NistLcInfo lcInfo;
	lcInfo.lcConfig = lcConfig;
	lcInfo.macSapUser = msu;
	m_lcInfoMap[lcId] = lcInfo;
}

// added function to handle LC priority 
void
NrV2XUeMac::DoAddLCPriority ( uint8_t rnti, uint8_t lcid, uint8_t priority)
{
	std::map <uint8_t, std::map <uint8_t, uint8_t> >::iterator it;
	it = PriorityMap.find(rnti);
	if (it == PriorityMap.end())
	{
		// insert new rnti in the map
		std::map<uint8_t, uint8_t> tempMap;
		tempMap.insert (std::pair<uint8_t, uint8_t> (lcid, priority));
		PriorityMap.insert (std::pair <uint8_t, std::map<uint8_t, uint8_t > > (rnti, tempMap));

	}
	else
	{
		// check if LC exists already or not
		std::map <uint8_t, uint8_t> mapLC=it->second;
		std::map <uint8_t, uint8_t>::iterator itLC;
		itLC=mapLC.find(lcid);
		if (itLC==mapLC.end())
		{
			// LC doesn't exist in the map
			it->second.insert (std::pair<uint8_t, uint8_t> (lcid, priority));
		}
	}
	return;
}

void
NrV2XUeMac::DoAddLc (uint8_t lcId, uint32_t srcL2Id, uint32_t dstL2Id, NistLteUeCmacSapProvider::NistLogicalChannelConfig lcConfig, NistLteMacSapUser* msu)
{
	NS_LOG_FUNCTION (this << (uint32_t) lcId << srcL2Id << dstL2Id);
	SidelinkLcIdentifier sllcid;
	sllcid.lcId = lcId;
	sllcid.srcL2Id = srcL2Id;
	sllcid.dstL2Id = dstL2Id;

	NS_ASSERT_MSG (m_slLcInfoMap.find (sllcid) == m_slLcInfoMap.end (), "cannot add channel because LCID " << lcId << ", srcL2Id " << srcL2Id << ", dstL2Id " << dstL2Id << " is already present");

	NistLcInfo lcInfo;
	lcInfo.lcConfig = lcConfig;
	lcInfo.macSapUser = msu;
	m_slLcInfoMap[sllcid] = lcInfo;
}

void
NrV2XUeMac::DoRemoveLc (uint8_t lcId)
{
	NS_LOG_FUNCTION (this << " lcId" << lcId);
	NS_ASSERT_MSG (m_lcInfoMap.find (lcId) != m_lcInfoMap.end (), "could not find LCID " << lcId);
	m_lcInfoMap.erase (lcId);

	// added code to remove the LC from the LC priority map
	std::map <uint8_t, std::map <uint8_t, uint8_t> >::iterator it;
	it=PriorityMap.find(m_rnti);
	it->second.erase(lcId);
}

void
NrV2XUeMac::DoRemoveLc (uint8_t lcId, uint32_t srcL2Id, uint32_t dstL2Id)
{
	NS_LOG_FUNCTION (this << " lcId" << lcId << ", srcL2Id=" << srcL2Id << ", dstL2Id" << dstL2Id);
	//    NS_ASSERT_MSG (m_lcInfoMap.find (lcId) != m_lcInfoMap.end (), "could not find LCID " << lcId);
	//    m_lcInfoMap.erase (lcId);
}

void
NrV2XUeMac::DoReset ()
{
	NS_LOG_FUNCTION (this);
	std::map <uint8_t, NistLcInfo>::iterator it = m_lcInfoMap.begin ();
	while (it != m_lcInfoMap.end ())
	{
		// don't delete CCCH)
		if (it->first == 0)
		{
			++it;
		}
		else
		{
			// note: use of postfix operator preserves validity of iterator
			m_lcInfoMap.erase (it++);
		}
	}

	m_noRaResponseReceivedEvent.Cancel ();
	m_rachConfigured = false;
	m_freshUlBsr = false;
	m_ulBsrReceived.clear ();
}

void
NrV2XUeMac::DoAddSlTxPool (Ptr<SidelinkTxDiscResourcePool> pool)
{
  //NS_ASSERT_MSG (m_discTxPools.m_pool != NULL, "Cannot add discovery transmission pool for " << m_rnti << ". Pool already exist for destination");
  DiscPoolInfo info;
  info.m_pool = pool;
  info.m_npsdch = info.m_pool->GetNPsdch();
  info.m_currentDiscPeriod.frameNo = 0; //init to 0 to make it invalid
  info.m_currentDiscPeriod.subframeNo = 0; //init to 0 to make it invalid
  info.m_nextDiscPeriod = info.m_pool->GetNextDiscPeriod (m_frameNo, m_subframeNo);
  //adjust because scheduler starts with frame/subframe = 1
  info.m_nextDiscPeriod.frameNo++;
  info.m_nextDiscPeriod.subframeNo++;
  info.m_grant_received = false; 
  m_discTxPools = info;
}

void
NrV2XUeMac::DoRemoveSlTxPool ()
{
  m_discTxPools.m_pool = NULL;
}

void
NrV2XUeMac::DoSetSlRxPools (std::list<Ptr<SidelinkRxDiscResourcePool> > pools)
{
  m_discRxPools = pools;
}

void 
NrV2XUeMac::DoModifyDiscTxApps (std::list<uint32_t> apps)
{
  m_discTxApps = apps;
  m_cphySapProvider->AddDiscTxApps (apps);
}

void 
NrV2XUeMac::DoModifyDiscRxApps (std::list<uint32_t> apps)
{
  m_discRxApps = apps;
  m_cphySapProvider->AddDiscRxApps (apps);
}

void
NrV2XUeMac::DoAddSlTxPool (uint32_t dstL2Id, Ptr<SidelinkTxCommResourcePool> pool)
{
	std::map <uint32_t, PoolInfo >::iterator it;
	it = m_sidelinkTxPoolsMap.find (dstL2Id);
	NS_ASSERT_MSG (it == m_sidelinkTxPoolsMap.end (), "Cannot add sidelink transmission pool for " << dstL2Id << ". Pool already exist for destination");
	PoolInfo info;
	info.m_pool = pool;
	info.m_npscch = info.m_pool->GetNPscch();  // number of PSCCH available in the pool
	info.m_currentScPeriod.frameNo = 0; //init to 0 to make it invalid
	info.m_currentScPeriod.subframeNo = 0; //init to 0 to make it invalid
	info.m_nextScPeriod = info.m_pool->GetNextScPeriod (m_frameNo, m_subframeNo);
	//adjust because scheduler starts with frame/subframe = 1
	info.m_nextScPeriod.frameNo++;
	info.m_nextScPeriod.subframeNo++;
	info.m_grant_received = false;
        
        info.m_V2X_grant_received = false;
        info.m_V2X_grant_fresh = false;
         
	m_sidelinkTxPoolsMap.insert (std::pair<uint32_t, PoolInfo > (dstL2Id, info));
}

void
NrV2XUeMac::DoRemoveSlTxPool (uint32_t dstL2Id)
{
	std::map <uint32_t, PoolInfo >::iterator it;
	it = m_sidelinkTxPoolsMap.find (dstL2Id);
	NS_ASSERT_MSG (it != m_sidelinkTxPoolsMap.end (), "Cannot remove sidelink transmission pool for " << dstL2Id << ". Unknown destination");
	m_sidelinkTxPoolsMap.erase (dstL2Id);
}


void
NrV2XUeMac::DoSetSlRxPools (std::list<Ptr<SidelinkRxCommResourcePool> > pools)
{
	m_sidelinkRxPools = pools;
}

void
NrV2XUeMac::DoSetRnti (uint16_t rnti)
{
	NS_LOG_FUNCTION (this << rnti);
	NS_ASSERT_MSG (m_rnti == 0, "Cannot manually set RNTI if already configured");
	m_rnti = rnti;
}

void
NrV2XUeMac::DoReceivePhyPdu (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this);
  NistLteRadioBearerTag tag;
  p->RemovePacketTag (tag);
  if (tag.GetSourceL2Id () == 0)
  {
    if (tag.GetRnti () == m_rnti)
    {
      NS_LOG_INFO ("Received dowlink packet");
      //regular downlink packet
      // packet is for the current user
      std::map <uint8_t, NistLcInfo>::const_iterator it = m_lcInfoMap.find (tag.GetLcid ());
      if (it != m_lcInfoMap.end ())
      {
        it->second.macSapUser->ReceivePdu (p);
      }
      else
      {
        NS_LOG_WARN ("received packet with unknown lcid " << (uint32_t) tag.GetLcid ());
      }
    }
  }
  else
  {
    //sidelink packet. Perform L2 filtering
    NS_LOG_INFO ("Received sidelink packet");
    std::list <uint32_t>::iterator dstIt;
    bool found = false;
    for (dstIt = m_sidelinkDestinations.begin (); dstIt != m_sidelinkDestinations.end () ; dstIt++)
    {
      if ((*dstIt) == tag.GetDestinationL2Id ())
      {
        //the destination is a group we want to listen to
        SidelinkLcIdentifier identifier;
        identifier.lcId = tag.GetLcid ();
        identifier.srcL2Id = tag.GetSourceL2Id ();
        identifier.dstL2Id = tag.GetDestinationL2Id ();
        std::map <SidelinkLcIdentifier, NistLcInfo>::iterator it = m_slLcInfoMap.find (identifier);
	if (it == m_slLcInfoMap.end ())
	{
	  //notify RRC to setup bearer
	  m_cmacSapUser->NotifySidelinkReception (tag.GetLcid(), tag.GetSourceL2Id (), tag.GetDestinationL2Id ());
          //should be setup now
	  it = m_slLcInfoMap.find (identifier);
	  if (it == m_slLcInfoMap.end ())
	  {
	    NS_LOG_WARN ("Failure to setup sidelink radio bearer");
	  }
	}
        it->second.macSapUser->ReceivePdu (p);
        /* 
        std::ofstream errorFile;
        errorFile.open(m_outputPath + "tbRxFile.csv", std::ios_base::app);
        errorFile << m_rnti << "," << Simulator::Now().GetSeconds() << "\r\n";
        errorFile.close();
        */
	found = true;
	break;
      }
    }
    if (!found)
    {
      NS_LOG_INFO ("received packet with unknown destination " << tag.GetDestinationL2Id ());
    }
  }
//  std::cin.get();
}


void
NrV2XUeMac::DoReceiveNistLteControlMessage (Ptr<NistLteControlMessage> msg)
{
   //Useless
}

void
NrV2XUeMac::DoReceiveNistRrLteControlMessage (Ptr<NistLteControlMessage> msg)
{
   NS_LOG_FUNCTION (this);
   //Useless
}

void
NrV2XUeMac::DoReceiveNistPFLteControlMessage (Ptr<NistLteControlMessage> msg)
{
   NS_LOG_FUNCTION (this);
   //Useless
}

void
NrV2XUeMac::DoReceiveNistMTLteControlMessage (Ptr<NistLteControlMessage> msg)
{
   NS_LOG_FUNCTION (this);
   //Useless
}

void
NrV2XUeMac::DoReceiveNistPrLteControlMessage (Ptr<NistLteControlMessage> msg)
{
   NS_LOG_FUNCTION (this);
   //Useless
}

void
NrV2XUeMac::RefreshHarqProcessesPacketBuffer (void)
{
	NS_LOG_FUNCTION (this);

	for (uint16_t i = 0; i < m_miUlHarqProcessesPacketTimer.size (); i++)
	{
		if (m_miUlHarqProcessesPacketTimer.at (i) == 0)
		{
			if (m_miUlHarqProcessesPacket.at (i)->GetSize () > 0)
			{
				// timer expired: drop packets in buffer for this process
				NS_LOG_INFO (this << " HARQ Proc Id " << i << " packets buffer expired");
				Ptr<PacketBurst> emptyPb = CreateObject <PacketBurst> ();
				m_miUlHarqProcessesPacket.at (i) = emptyPb;
			}
		}
		else
		{
			m_miUlHarqProcessesPacketTimer.at (i)--;
		}
	}
}


uint32_t 
NrV2XUeMac::ComputeResidualCSRs (std::map<uint16_t,std::list<SidelinkCommResourcePool::SubframeInfo> > L1)
{
   
   uint32_t nCSR = 0;
   std::map<uint16_t,std::list<SidelinkCommResourcePool::SubframeInfo> >::iterator mapIt;
   for (mapIt = L1.begin (); mapIt != L1.end (); mapIt++)
      {
          nCSR += (*mapIt).second.size ();
      }  
   return nCSR;
}

bool
NrV2XUeMac::CompareRssi (CandidateCSRl2 first, CandidateCSRl2 second)
{
  return (first.rssi < second.rssi);
}


uint16_t
NrV2XUeMac::GetTproc0 (uint16_t numerologyIndex)
{
   uint16_t T_proc_0; //Defined in slots
   switch (m_numerologyIndex)
   {
     case 0:
       T_proc_0 = 1;
       break;
     case 1:
       T_proc_0 = 1;
       break;
     case 2:
       T_proc_0 = 2;
       break;
     case 3:
       T_proc_0 = 4;
       break;
   }

   return T_proc_0;
}


uint16_t
NrV2XUeMac::GetTproc1 (uint16_t numerologyIndex)
{
   uint16_t T_proc_1; //Defined in slots
   switch (m_numerologyIndex)
   {
     case 0:
       T_proc_1 = 3;
       break;
     case 1:
       T_proc_1 = 5;
       break;
     case 2:
       T_proc_1 = 9;
       break;
     case 3:
       T_proc_1 = 17;
       break;
   }

   return T_proc_1;
}


uint32_t
NrV2XUeMac::GetCresel (uint32_t RRI)
{
   Ptr<UniformRandomVariable> uniformCresel = CreateObject<UniformRandomVariable> ();
   uint32_t Cresel;
   if (RRI >= 100)
   {
     Cresel =  uniformCresel ->  GetInteger (5,15); //Get a random integer in the interval [min, max]
     //V2XGrant.m_Cresel = 10;
   }      
   else
   {  //TODO check if it works
     uint16_t Cresel_bound;
     Cresel_bound = 100 / std::max(20, (int)RRI);
     Cresel = uniformCresel ->  GetInteger (5*Cresel_bound,15*Cresel_bound); //Get a random integer in the interval [min, max]
     NS_LOG_DEBUG("RRI " << RRI << ", bound: " << Cresel_bound << ", Cresel: " << Cresel);
   }       
   return Cresel;
}


NrV2XUeMac::V2XSidelinkGrant 
NrV2XUeMac::V2XSelectResources (uint32_t frameNo, uint32_t subframeNo, double pdb, uint32_t p_rsvp, uint8_t v2xMessageType, uint8_t v2xTrafficType, uint16_t ReselectionCounter, uint16_t PacketSize, uint16_t ReservationSize, reselectionTrigger V2Xtrigger)
{        
   NS_LOG_FUNCTION(this);
         
   V2XSidelinkGrant V2XGrant;

   frameNo --;
   subframeNo --; 

   std::vector<uint16_t>::iterator RRIit;
   RRIit = find(m_RRIvalues.begin(), m_RRIvalues.end(), p_rsvp);
   NS_ASSERT_MSG(RRIit != m_RRIvalues.end(), "RRI not included in the list of allowed RRI values");
//   NS_LOG_UNCOND(*RRIit);

//   NS_ASSERT_MSG(ReservationSize >= PacketSize, "Reservation size must be larger than the packet size (in bytes)"); //Remove for size reselections
   NS_ASSERT_MSG(pdb <= p_rsvp, "Packet Delay Budget (PDB) must be lower or equal than the reservation period");

   V2XGrant.m_mcs = m_slGrantMcs;
   V2XGrant.m_RRI = p_rsvp;


           
   if (ReselectionCounter == 0)
   {
     NS_ASSERT_MSG(false, "Reselection counter = 0, check the CAM trace! Node ID " << m_rnti);
   }
   if (p_rsvp == 0)
   {
     NS_ASSERT_MSG(false, "Reservation period = 0, check the CAM trace! Node ID " << m_rnti);
   }

   
   // Assigning the reselection counter value        
   if (ReselectionCounter == 10000)
   { 
     V2XGrant.m_Cresel = GetCresel (p_rsvp);                    
   }
   else
     V2XGrant.m_Cresel = ReselectionCounter;


   SidelinkCommResourcePool::SubframeInfo currentSF;
   currentSF.frameNo = frameNo;
   currentSF.subframeNo = subframeNo;

   NS_LOG_INFO("Resource Reselection Requested Now: SF(" <<  currentSF.frameNo << "," << currentSF.subframeNo <<  "), Time: " << Simulator::Now ().GetSeconds () << "s, estimated SF(" 
   << SimulatorTimeToSubframe (Simulator::Now (), m_slotDuration).frameNo << "," << SimulatorTimeToSubframe (Simulator::Now (), m_slotDuration).subframeNo << ")");

   uint16_t nsubCHsize = m_nsubCHsize; // [RB]
   //uint16_t startRBSubchannel = 0;
   uint16_t NSubCh; //the total number of subchannels
   NSubCh = std::floor(m_BW_RBs / nsubCHsize); // 50/10 
   uint16_t L_SubCh = m_L_SubCh, L_RBs; // the number of subchannels for the reservation
    	 
   uint32_t AdjustedPacketSize, AdjustedReservationSize;  
   AdjustedPacketSize = m_NRamc->GetSlSubchAndTbSizeFromMcs (PacketSize, V2XGrant.m_mcs , m_nsubCHsize, m_BW_RBs, &L_SubCh, &L_RBs) / 8;
   L_SubCh/= m_nsubCHsize;
   NS_LOG_DEBUG("Reserving resources for packet size: " << PacketSize << " B, adjusted to " << AdjustedPacketSize << " B, required RBs = " << L_RBs << ", required subchannels " << L_SubCh);

   AdjustedReservationSize = m_NRamc->GetSlSubchAndTbSizeFromMcs (ReservationSize, V2XGrant.m_mcs , m_nsubCHsize, m_BW_RBs, &L_SubCh, &L_RBs) / 8;
   L_SubCh/= m_nsubCHsize;
   NS_LOG_DEBUG("Actual reservation size is: " << ReservationSize << " B, adjusted to " << AdjustedReservationSize << " B, required RBs = " << L_RBs << ", required subchannels " << L_SubCh);

//   uint16_t N_CSR_per_SF = NSubCh - L_SubCh + 1;
   NS_LOG_INFO("N_CSR_per_SF: " << (int) NSubCh - L_SubCh + 1);
 //  uint16_t T_1 = 4; // should be T1 = 4 (see 3GPP)
   double T_2 = pdb - 1; 
//   uint16_t T_1_slots = T_1/m_slotDuration;
   uint32_t T_2_slots = T_2/m_slotDuration;

   // these values should be converted in slot values, now they rely on the assumption: subframe length = 1 ms
 //  uint16_t nbRb_Pssch = L_SubCh*nsubCHsize - 2; //FIXME: this applies to the adjacent situation only (Mode 4)
   uint16_t nbRb_Pssch = L_SubCh*nsubCHsize ; //FIXME: Mode 2 PSSCH
   uint16_t nbRb_Pscch = nsubCHsize ; //FIXME: Mode 2 PSCCH (Occupy only the first subchannel)

   V2XGrant.m_tpc = 0;
   V2XGrant.m_SFGap = 0; // for now, no retransmission
   V2XGrant.m_adjacency = true;
 
   uint32_t subframeInitialTx;
   Ptr<UniformRandomVariable> uniformRnd = CreateObject<UniformRandomVariable> ();
                      
   // Second Option to build the list
   std::map<uint16_t,std::list<SidelinkCommResourcePool::SubframeInfo> > Sa, L1;
   std::vector<CandidateCSRl2> finalL2; 
   uint32_t iterationsCounter = 0;
   double psschThresh = m_rsrpThreshold;



   Sa = SelectionWindow (currentSF, T_2_slots, NSubCh - L_SubCh + 1);

   NS_LOG_DEBUG("Initial list Sa size: " << ComputeResidualCSRs (Sa) );

   NS_LOG_DEBUG("Saving initial Sa");
   if (m_rnti == m_debugNode)
   {
     std::ofstream SaFileAlert;
     SaFileAlert.open (m_outputPath + "SafileAlert.txt", std::ios_base::app);
     SaFileAlert << "-----Initial Sa------ At time " << Simulator::Now().GetSeconds() << ", UE " << m_rnti << " at SF(" << currentSF.frameNo << "," << currentSF.subframeNo << ") Residual resources " << ComputeResidualCSRs (Sa) << " ----------" << std::endl;
     for(std::map<uint16_t,std::list<SidelinkCommResourcePool::SubframeInfo> >::iterator SaIt = Sa.begin(); SaIt != Sa.end(); SaIt++)
     {
       for(std::list<SidelinkCommResourcePool::SubframeInfo>::iterator FrameIT = SaIt->second.begin(); FrameIT != SaIt->second.end(); FrameIT++)
         SaFileAlert << "CSR index " << SaIt->first << " Frame " << FrameIT->frameNo << " subframe " << FrameIT->subframeNo << std::endl;
     }
     SaFileAlert.close ();
   }

   uint32_t nCSRinitial = ComputeResidualCSRs (Sa);
   uint32_t nCSRpastTx, nCSRfinal;

   // Print the list of CSRs (Sa)
   /*NS_LOG_DEBUG("Printing the initial list Sa of candidate resources");
     UnimorePrintCSR(Sa);*/

//   if (m_rnti == m_debugNode)
//     std::cin.get();

   if (!m_randomSelection)
   {    
     L1 = Mode2Step1 (Sa, currentSF, V2XGrant, T_2, NSubCh, L_SubCh, &iterationsCounter, &psschThresh, &nCSRpastTx, true);
   }
   else
   {
     //In case of random selection, L1 is filled with all CSRs
     NS_LOG_DEBUG("Random selection!");
  //   L1 = Sa; // Already assigned
   }
//   std::cin.get();
   nCSRfinal = ComputeResidualCSRs (L1);

   NS_LOG_INFO("Initial list size = " << nCSRinitial << ". After removing past Tx = " << nCSRpastTx << ". After removing reservations = " << nCSRfinal);

   if (m_List2Enabled)
   {
     NS_ASSERT_MSG(false, "List L2 is temporarily not available");
   }
   else
   {
     std::vector<CandidateCSRl2> L2EquivalentVector;
     CandidateCSRl2 FinalL2tmpItem;
     for(std::map<uint16_t,std::list<SidelinkCommResourcePool::SubframeInfo> >::iterator L1it = L1.begin(); L1it != L1.end(); L1it++)
     {
       //  NS_LOG_DEBUG("CSR index " << L1it->first);
       for(std::list<SidelinkCommResourcePool::SubframeInfo>::iterator FrameIT = L1it->second.begin(); FrameIT != L1it->second.end(); FrameIT++)
       {
         FinalL2tmpItem.CSRIndex = L1it->first;
         FinalL2tmpItem.subframe = (*FrameIT);
         FinalL2tmpItem.rssi = 0;
         finalL2.push_back (FinalL2tmpItem);
       }
       //   NS_LOG_DEBUG("Frame " << FrameIT->frameNo << " subframe " << FrameIT->subframeNo);
     }

     if (m_rnti == m_debugNode)
     {
       std::ofstream L2fileAlert;
       L2fileAlert.open (m_outputPath + "L2fileAlert.txt", std::ios_base::app);
       L2fileAlert << "At " << Simulator::Now ().GetSeconds () << " SF(" << currentSF.frameNo << "," << currentSF.subframeNo << ") Final L2 size: " << finalL2.size () << ", L1 size: " << nCSRfinal << ", target L2 size: " << nCSRfinal << "\r\n" << "\r\n";
       std::vector<CandidateCSRl2>::iterator L2ItDebug;
       for (L2ItDebug = finalL2.begin (); L2ItDebug != finalL2.end (); L2ItDebug++)
       {
         L2fileAlert << "CSRindex: " << (int) (*L2ItDebug).CSRIndex << ", SF(" <<  (*L2ItDebug).subframe.frameNo << "," << (*L2ItDebug).subframe.subframeNo << "), RSSI: " <<  (*L2ItDebug).rssi << " mW" << "\r\n";
      //       NS_LOG_DEBUG("CSRindex: " << (int) (*L2ItDebug).CSRIndex << ", SF(" <<  (*L2ItDebug).subframe.frameNo << "," << (*L2ItDebug).subframe.subframeNo << "), RSSI: " <<  (*L2ItDebug).rssi << " mW");
       }
       L2fileAlert.close ();
    //      std::cin.get();
     }  

   }

  
   uint16_t selectedCSR;
   SidelinkCommResourcePool::SubframeInfo selectedSF;
   if (m_randomSelection)  
   { 
     subframeInitialTx = uniformRnd -> GetInteger (3, pdb);
   }
   else // if list L2 Enabled
   {
     Ptr<UniformRandomVariable> selectFromL2 = CreateObject<UniformRandomVariable> ();
     CandidateCSRl2 selectedResourceFromL2 = finalL2[selectFromL2 -> GetInteger (0, finalL2.size () - 1)];
     selectedCSR = selectedResourceFromL2.CSRIndex;
     selectedSF = selectedResourceFromL2.subframe;
     NS_LOG_DEBUG("Now: UE " << m_rnti << " at SF(" << currentSF.frameNo << "," << currentSF.subframeNo << ") Selected CSR index " << selectedCSR << " at SF(" << selectedSF.frameNo << "," << selectedSF.subframeNo << "). Adjusted to SF(" << selectedSF.frameNo+1 << "," << selectedSF.subframeNo+1 << ")");

     //Fake grants to force collisions
    /* if (m_rnti == 1)
     {
       selectedCSR = 0;
       selectedSF = {110,7};
     }     */
    /* if ((m_rnti == 2) && (frameNo <130))
     {
       selectedCSR = 0;
       selectedSF = {130,8};
     }
     if ((m_rnti == 3) && (frameNo <130))
     {
       selectedCSR = 0;
       selectedSF = {130,8};
     } */
    /* if (m_rnti == 4)
     {
       selectedCSR = 2;
       selectedSF = {20,5};
     }*/

  //   SidelinkCommResourcePool::SubframeInfo SFoffset = selectedSF - currentSF;
     SidelinkCommResourcePool::SubframeInfo SFoffset;
     if (selectedSF.frameNo < currentSF.frameNo)
        SFoffset.frameNo = selectedSF.frameNo + 1024 - currentSF.frameNo;
     else
        SFoffset.frameNo = selectedSF.frameNo - currentSF.frameNo;

     if (selectedSF.subframeNo < currentSF.subframeNo)
     {
        SFoffset.subframeNo = selectedSF.subframeNo + 10 -  currentSF.subframeNo;
        SFoffset.frameNo--;
     }
     else
        SFoffset.subframeNo = selectedSF.subframeNo -  currentSF.subframeNo;

     if (m_rnti == m_debugNode)
     {
       std::ofstream SFoffsetFile;
       SFoffsetFile.open(m_outputPath + "SFOffset.csv", std::ios_base::app);
       SFoffsetFile << "Now: " << Simulator::Now ().GetSeconds () << " s, " << "Selected: (" << selectedSF.frameNo << "," << selectedSF.subframeNo << "), current: (" << currentSF.frameNo << "," << currentSF.subframeNo << "), Offset: (" << SFoffset.frameNo << "," << SFoffset.subframeNo << ")"<< "\r\n";
       SFoffsetFile.close ();
     }
     subframeInitialTx = 10 * SFoffset.frameNo + SFoffset.subframeNo;
    // NS_LOG_DEBUG("Subframe Initial Tx " << subframeInitialTx);
     NS_ASSERT (( subframeInitialTx % 10 == SFoffset.subframeNo) &&  (subframeInitialTx / 10) % 1024 == SFoffset.frameNo );
   } 

   if (m_rnti == m_debugNode)
   {
     std::ofstream initialTxFile;
     initialTxFile.open(m_outputPath + "initialTx.csv", std::ios_base::app);
     initialTxFile << m_rnti << subframeInitialTx << ","<< (int) selectedCSR << "\r\n";
     initialTxFile.close ();
   }

   V2XGrant.m_subframeInitialTx = subframeInitialTx;

   V2XGrant.m_tbSize = 0; //computed later

   V2XGrant.m_nextReservedSubframe = selectedSF.subframeNo+1;
   V2XGrant.m_nextReservedFrame = selectedSF.frameNo+1;
   V2XGrant.m_rbLenPssch = nbRb_Pssch;
   V2XGrant.m_rbLenPscch = nbRb_Pscch;
   /* std::ofstream selectedFile;
   selectedFile.open(m_outputPath + "selectedFile.txt", std::ios_base::app);
   selectedFile << "Now true: SF(" << currentSF.frameNo+1 << "," << currentSF.subframeNo+1 << ", Selected true: SF(" << V2XGrant.m_nextReservedFrame << "," << V2XGrant.m_nextReservedSubframe << ")" << "\r\n";
   selectedFile.close (); */
   if (m_randomSelection)
   { 
     NS_FATAL_ERROR("Random selection not yet implemented");
   } 
   else 
   {
     V2XGrant.m_resPscch = selectedCSR;
     V2XGrant.m_resPssch = V2XGrant.m_resPscch;
 //    V2XGrant.m_rbLenPscch = 2; //fixed and standardized (Mode 4)
 //    V2XGrant.m_rbStartPscch = V2XGrant.m_resPscch * nsubCHsize; // (Mode 4)
     V2XGrant.m_rbStartPscch = selectedCSR * nsubCHsize; // (Mode 2)
     V2XGrant.m_rbStartPssch = selectedCSR * nsubCHsize; // (Mode 2)
   }

 /*  if (m_rnti == 1 && false)
   {
      sensingDebug.open (m_outputPath + "sensingDebug.txt", std::ios_base::app);
      sensingDebug << "CSR Index " << (int) selectedCSR << ", SF(" << V2XGrant.m_nextReservedFrame << "," << V2XGrant.m_nextReservedSubframe << ")\r\n \r\n";
      sensingDebug.close ();
   }    */
   // dummy: retrieve rsrp information
   std::map <Time, PsschRsrp>::iterator itRsrp;
   for (itRsrp = m_PsschRsrpMap.begin (); itRsrp != m_PsschRsrpMap.end (); itRsrp++)
   {
     if (itRsrp != m_PsschRsrpMap.end ())
     {
           NS_LOG_INFO("Rnti " << m_rnti << " - Now: " << Simulator::Now().GetSeconds() << "s, SF:(" << frameNo << "," << subframeNo << "), Cycle no." << m_absSFN << ", Meas. Time: " << (*itRsrp).first.GetSeconds() << "s, PSSCH-RSRP: " <<  (*itRsrp).second.psschRsrpDb << "dB" );
     }
   }


   // Re-evaluation mechanism
   uint16_t T_3_slots = GetTproc1 (m_numerologyIndex);

   if (V2XGrant.m_nextReservedSubframe <= (T_3_slots % 10) )
   {
     V2XGrant.m_ReEvaluationSubframe = 10 + V2XGrant.m_nextReservedSubframe - (T_3_slots % 10);
     if (V2XGrant.m_nextReservedFrame <= (T_3_slots/10 +1))                                       
    // if (V2XGrant.m_nextReservedFrame == 1)
       V2XGrant.m_ReEvaluationFrame = 1024 + V2XGrant.m_nextReservedFrame - T_3_slots/10-1;
     else
       V2XGrant.m_ReEvaluationFrame = V2XGrant.m_nextReservedFrame - T_3_slots/10 -1;
   }
   else
   {
     V2XGrant.m_ReEvaluationSubframe = V2XGrant.m_nextReservedSubframe - (T_3_slots % 10);
     V2XGrant.m_ReEvaluationFrame = V2XGrant.m_nextReservedFrame - T_3_slots/10;
   }

   V2XGrant.m_EnableReEvaluation = true;
   V2XGrant.m_SelectionTrigger = V2Xtrigger;

/*   if (SubtractFrames(V2XGrant.m_nextReservedFrame, currentSF.frameNo+1 , V2XGrant.m_nextReservedSubframe, currentSF.subframeNo+1) < GetTproc1 (m_numerologyIndex))
   {
     NS_LOG_UNCOND("Not re-evaluating!");
     std::cin.get();
   }*/
 /*  if (V2Xtrigger == ReEVALUATION)
   {
//     V2XGrant.m_ReEvaluationSubframe = 99999;
//     V2XGrant.m_ReEvaluationFrame = 99999;
     V2XGrant.m_EnableReEvaluation = false;
   }*/

   NS_LOG_DEBUG(Simulator::Now ().GetSeconds () << " UE " << m_rnti << " selected SF(" << V2XGrant.m_nextReservedFrame << "," 
   << V2XGrant.m_nextReservedSubframe << "), rbStart (PSSCH) " << V2XGrant.m_rbStartPssch << ", rbLen (PSSCH) " << V2XGrant.m_rbLenPssch 
   << ", rbStart (PSCCH) " << V2XGrant.m_rbStartPscch << ", rbLen (PSCCH) " << V2XGrant.m_rbLenPscch << ", Cresel " << V2XGrant.m_Cresel);

   UeSelectionInfo tmp;
   tmp.selGrant = V2XGrant;
   tmp.RSRPthresh = psschThresh-3;
   tmp.iterations = iterationsCounter;
   tmp.time = Simulator::Now ().GetSeconds();
   tmp.selFrame.frameNo = currentSF.frameNo;
   tmp.selFrame.subframeNo = currentSF.subframeNo;
   tmp.nodeId = m_rnti;
   tmp.nCSRfinal = nCSRfinal;
   tmp.nCSRpastTx = nCSRpastTx;
   tmp.nCSRinitial = nCSRinitial;
   tmp.pdb = pdb;

   NrV2XUeMac::SelectedGrants.push_back(tmp);

   if (Simulator::Now ().GetSeconds() - NrV2XUeMac::prevPrintTime_selection > m_savingPeriod)
   {
     NrV2XUeMac::prevPrintTime_selection = Simulator::Now ().GetSeconds();
     std::ofstream SSPSlog;
     SSPSlog.open (m_outputPath + "SSPSlog.txt", std::ios_base::app);
     std::ofstream SSPSlogEXT;
     SSPSlogEXT.open (m_outputPath + "SSPSlog_EXT.txt", std::ios_base::app);
     for(std::vector<UeSelectionInfo>::iterator selIT = NrV2XUeMac::SelectedGrants.begin(); selIT != NrV2XUeMac::SelectedGrants.end(); selIT++)
     {
       SSPSlog << selIT->selGrant.m_SelectionTrigger << "," << selIT->time << "," << selIT->selFrame.frameNo+1 << "," << selIT->selFrame.subframeNo+1 << "," << selIT->nodeId << "," << selIT->iterations 
       << "," << selIT->RSRPthresh << "," << selIT->nCSRfinal << "," << selIT->nCSRpastTx << "," << selIT->nCSRinitial << "," << selIT->selGrant.m_nextReservedFrame << "," << selIT->selGrant.m_nextReservedSubframe << "," 
       << selIT->selGrant.m_ReEvaluationFrame << "," << selIT->selGrant.m_ReEvaluationSubframe << "," << selIT->selGrant.m_rbStartPssch << "," << selIT->selGrant.m_rbLenPssch << "," 
       << selIT->selGrant.m_rbStartPscch << "," << selIT->selGrant.m_rbLenPscch << "," << selIT->selGrant.m_Cresel << "," << selIT->selGrant.m_RRI << "," << (int) selIT->selGrant.m_RRI/m_slotDuration << "," << selIT->pdb << std::endl;

       if (selIT->selGrant.m_SelectionTrigger == COUNTER)
         SSPSlogEXT << "Counter reselection";
       else if (selIT->selGrant.m_SelectionTrigger == LATENCYandSIZE)
         SSPSlogEXT << "Latency and size reselection";
       else if (selIT->selGrant.m_SelectionTrigger == LATENCY)
         SSPSlogEXT << "Latency reselection";
       else if (selIT->selGrant.m_SelectionTrigger == SIZE)
         SSPSlogEXT << "Size reselection";
       else
         SSPSlogEXT << "Re-evaluation";
       SSPSlogEXT << " at time: " << selIT->time << ", SF(" << selIT->selFrame.frameNo+1 << "," <<  selIT->selFrame.subframeNo+1 << ") Node " << selIT->nodeId 
       << ", Number of iterations: " << selIT->iterations << ", PSSCH Threshold = " << selIT->RSRPthresh << " dBm, final L1 size = " << selIT->nCSRfinal << ", partial L1 size = " << selIT->nCSRpastTx 
       << ", total CSRs: " << selIT->nCSRinitial << ", Next: SF(" << selIT->selGrant.m_nextReservedFrame << "," << selIT->selGrant.m_nextReservedSubframe << "), rbStartPSSCH: " 
       << selIT->selGrant.m_rbStartPssch << ", rbLenPSSCH: " << selIT->selGrant.m_rbLenPssch << ", rbStartPSCCH: " << selIT->selGrant.m_rbStartPscch << ", rbLenPSCCH: " 
       << selIT->selGrant.m_rbLenPscch<< ", Cresel: " << selIT->selGrant.m_Cresel << ", RRI[ms]: " << selIT->selGrant.m_RRI << ", RRI[slots] " << (int)selIT->selGrant.m_RRI/m_slotDuration << ", ReEvaluation at SF(" 
       << selIT->selGrant.m_ReEvaluationFrame << "," << selIT->selGrant.m_ReEvaluationSubframe << ") PDB = " << selIT->pdb << " ms" << std::endl;
     }
     SSPSlog.close();
     SSPSlogEXT.close();
     NrV2XUeMac::SelectedGrants.clear();
   }
 
   NS_LOG_DEBUG("Re-evaluation at SF(" << V2XGrant.m_ReEvaluationFrame << "," << V2XGrant.m_ReEvaluationSubframe << ")");


//   if (m_rnti == m_debugNode)
//     std::cin.get();

   return V2XGrant;
}


std::map<uint16_t,std::list<SidelinkCommResourcePool::SubframeInfo>>
NrV2XUeMac::SelectionWindow (SidelinkCommResourcePool::SubframeInfo currentSF, uint32_t T_2_slots, uint16_t N_CSR_per_SF)
{
   NS_LOG_FUNCTION(this);
 
   std::map<uint16_t,std::list<SidelinkCommResourcePool::SubframeInfo> > Sa;
   // Build list Sa, containing all candidate resources

  // uint32_t T_1_slots = GetTproc1 (m_numerologyIndex);
   uint32_t T_1_slots = 2;
  
   NS_LOG_DEBUG("Building list of all candidate resources Sa at frame " << currentSF.frameNo << ", subframe " << currentSF.subframeNo);

   for (uint16_t j=0; j<  N_CSR_per_SF; j++)
   {
 //    NS_LOG_DEBUG("CSR index " << j);
     std::list<SidelinkCommResourcePool::SubframeInfo> SFvector;
     for (uint32_t i = T_1_slots; i <= T_2_slots; i++)
     {
       SidelinkCommResourcePool::SubframeInfo candidateSF;
       candidateSF.subframeNo = (currentSF.subframeNo + i) % 10; // valid subframe index between 0 and 9
       candidateSF.frameNo = (currentSF.frameNo + (currentSF.subframeNo + i) / 10) % 1024;  // valid frame index between 0 and 1023
       SFvector.push_back (candidateSF);           
     }
     Sa.insert (std::pair<uint16_t,std::list<SidelinkCommResourcePool::SubframeInfo> > (j,SFvector)); 
   }

   return Sa;

}


std::map<uint16_t,std::list<SidelinkCommResourcePool::SubframeInfo>> 
NrV2XUeMac::Mode2Step1_RemovePastTx (std::map<uint16_t,std::list<SidelinkCommResourcePool::SubframeInfo> > Sa, SidelinkCommResourcePool::SubframeInfo currentSF, V2XSidelinkGrant V2XGrant, double T_2)
{
   NS_LOG_FUNCTION(this);
   std::map<uint16_t,std::list<SidelinkCommResourcePool::SubframeInfo> > Sa_noPastTx;
   Sa_noPastTx = Sa;

   // Remove old frames used for transmission
   m_prevListUpdate.frameNo = currentSF.frameNo+1;
   m_prevListUpdate.subframeNo = currentSF.subframeNo+1;
   UpdatePastTxInfo(currentSF.frameNo+1, currentSF.subframeNo+1);

   // Print the list of CSRs (Sa)
 /*  NS_LOG_DEBUG("Printing the initial list Sa of candidate resources, after removing past transmissions");
   UnimorePrintCSR(Sa);*/

   std::list<std::pair<Time,SidelinkCommResourcePool::SubframeInfo>>::iterator pastTxIt;

   // Print the frames used for past transmissions 
 /*  NS_LOG_DEBUG("Print the frames used for past transmissions, UE " << m_rnti);
   for (pastTxIt = m_pastTxUnimore.begin (); pastTxIt != m_pastTxUnimore.end (); pastTxIt++)
   {
     NS_LOG_DEBUG("Current time: " << Simulator::Now().GetSeconds() << " Tx Time: " << pastTxIt->first.GetSeconds() << " Frame: " << pastTxIt->second.frameNo << " subframe: " << pastTxIt->second.subframeNo);
   }*/
//   if (m_rnti == m_debugNode)
//   std::cin.get();

   // Create a list of the subframes to be removed from the selection window
   std::vector<uint16_t>::iterator RRIit;
   std::list<SidelinkCommResourcePool::SubframeInfo> rm_pastTx_frames;
   for(RRIit = m_RRIvalues.begin(); RRIit != m_RRIvalues.end(); RRIit++)
   {
     uint16_t RRI_to_slot = *RRIit/m_slotDuration;
//     NS_LOG_DEBUG("Working with RRI: " << *RRIit << " ms, and " << RRI_to_slot << " slots at SF(" << currentSF.frameNo << ", " << currentSF.subframeNo << ")");
     for (pastTxIt = m_pastTxUnimore.begin (); pastTxIt != m_pastTxUnimore.end (); pastTxIt++)
     {
     //  NS_LOG_DEBUG("Past transmission at SF(" << pastTxIt->second.frameNo << ", " << pastTxIt->second.subframeNo << ")");
       uint16_t Q;
       SidelinkCommResourcePool::SubframeInfo insert_pastTx;
       if ((SubtractFrames( currentSF.frameNo, pastTxIt->second.frameNo, currentSF.subframeNo, pastTxIt->second.subframeNo) <= *RRIit) && (*RRIit < (T_2 + 1)) )
       {
//         Q = std::ceil( (float) (T_2 + 1)/ *RRIit );
         Q = std::ceil( (float) T_2/ *RRIit );
//         NS_LOG_DEBUG("IF clause, Q= " << Q)  ;
         for(uint16_t q = 1; q <= Q; q++)
         {
           insert_pastTx.subframeNo = (pastTxIt->second.subframeNo + q*RRI_to_slot)%10; // valid subframe index between 0 and 9
           insert_pastTx.frameNo = (pastTxIt->second.frameNo  + (pastTxIt->second.subframeNo + q*RRI_to_slot) / 10) % 1024;  // valid frame index between 0 and 1023 
//           NS_LOG_DEBUG("q= " << q << ", q*RRI= " << q*RRI_to_slot << " eliminate frame SF(" << insert_pastTx.frameNo << ", " << insert_pastTx.subframeNo << ")");
           if ( find(rm_pastTx_frames.begin(), rm_pastTx_frames.end(), insert_pastTx) ==  rm_pastTx_frames.end())
             rm_pastTx_frames.push_back(insert_pastTx);
         }
       }
       else
       { 
         Q = 1;
//         NS_LOG_DEBUG("ELSE clause, Q= " << Q)  ;
         insert_pastTx.subframeNo = (pastTxIt->second.subframeNo + Q*RRI_to_slot) % 10; // valid subframe index between 0 and 9
         insert_pastTx.frameNo = (pastTxIt->second.frameNo  + (pastTxIt->second.subframeNo + Q*RRI_to_slot) / 10) % 1024;  // valid frame index between 0 and 1023 
//         NS_LOG_DEBUG("q=Q= " << Q << ", Q*RRI= " << Q*RRI_to_slot << " eliminate frame SF(" << insert_pastTx.frameNo << ", " << insert_pastTx.subframeNo << ")");
         if ( find(rm_pastTx_frames.begin(), rm_pastTx_frames.end(), insert_pastTx) ==  rm_pastTx_frames.end())
           rm_pastTx_frames.push_back(insert_pastTx);
       }
     }
   
   }

 /*  NS_LOG_DEBUG("Print the list of frames to be removed");
   for(std::list<SidelinkCommResourcePool::SubframeInfo>::iterator rm_pastTxIt = rm_pastTx_frames.begin(); rm_pastTxIt != rm_pastTx_frames.end(); rm_pastTxIt++)
   {
     NS_LOG_DEBUG(rm_pastTxIt->frameNo << "," << rm_pastTxIt->subframeNo);
   }*/
//   if (m_rnti == m_debugNode)
//     std::cin.get();

   // Now remove the frames, considering my future transmissions as well (reselection counter + RRI)
   for(std::map<uint16_t,std::list<SidelinkCommResourcePool::SubframeInfo> >::iterator SaIt = Sa_noPastTx.begin(); SaIt != Sa_noPastTx.end(); SaIt++)
   { 
     NS_LOG_DEBUG("CSR index " << SaIt->first);
     for(std::list<SidelinkCommResourcePool::SubframeInfo>::iterator FrameIT = SaIt->second.begin(); FrameIT != SaIt->second.end(); FrameIT++)
     {
  //     NS_LOG_DEBUG("Frame " << FrameIT->frameNo << " subframe " << FrameIT->subframeNo << ", Cresel " << V2XGrant.m_Cresel);
       SidelinkCommResourcePool::SubframeInfo futureTxFrame;
       uint16_t RRI_slots = V2XGrant.m_RRI/m_slotDuration;
       for(uint16_t Cresel = 0; Cresel < V2XGrant.m_Cresel; Cresel++)
//       for(uint16_t Cresel = 0; Cresel <= (V2XGrant.m_Cresel*10-1); Cresel++)
       {
         futureTxFrame.subframeNo = (FrameIT->subframeNo + Cresel*RRI_slots) % 10;  // valid subframe index between 0 and 9
         futureTxFrame.frameNo = (FrameIT->frameNo + (FrameIT->subframeNo + Cresel*RRI_slots) / 10) % 1024; // valid frame index between 0 and 1023
         std::list<SidelinkCommResourcePool::SubframeInfo>::iterator rm_pastTxIt;
       //  NS_LOG_DEBUG("Future Frame " << futureTxFrame.frameNo << " subframe " << futureTxFrame.subframeNo << ", Cresel " << Cresel);
         rm_pastTxIt = find(rm_pastTx_frames.begin(), rm_pastTx_frames.end(), futureTxFrame);
         if (rm_pastTxIt != rm_pastTx_frames.end())
         {
     //    if ( find(rm_pastTx_frames.begin(), rm_pastTx_frames.end(), futureTxFrame) !=  rm_pastTx_frames.end())
           NS_LOG_DEBUG("Erasing: list SF(" << FrameIT->frameNo << ", " << FrameIT->subframeNo << ") Future Tx SF(" << futureTxFrame.frameNo << ", " << futureTxFrame.subframeNo << ") past tx frame (" << rm_pastTxIt->frameNo << ", " << rm_pastTxIt->subframeNo << ")");
           FrameIT = SaIt->second.erase(FrameIT);
           FrameIT--;
     //      NS_LOG_DEBUG("Current frame (" << FrameIT->frameNo << ", " << FrameIT->subframeNo << ")");
           break;
         }
       }
     }
 //  if (m_rnti == m_debugNode)
 //    std::cin.get();
   }

 
   // Print the list of CSRs (Sa)
 //  NS_LOG_DEBUG("Printing the initial list Sa of candidate resources, after removing past transmissions");
   //  UnimorePrintCSR(Sa);

//   if (m_rnti == m_debugNode)
//   std::cin.get();
   return Sa_noPastTx;

}


std::map<uint16_t,std::list<SidelinkCommResourcePool::SubframeInfo>> 
NrV2XUeMac::Mode2Step1 (std::map<uint16_t,std::list<SidelinkCommResourcePool::SubframeInfo> > Sa, SidelinkCommResourcePool::SubframeInfo currentSF,  
V2XSidelinkGrant V2XGrant, double T_2, uint16_t NSubCh,  uint16_t L_SubCh, uint32_t *iterationsCounter, double *psschThresh, uint32_t *nCSRpartial, bool saveData)
{
   NS_LOG_FUNCTION(this);
   std::map<uint16_t,std::list<SidelinkCommResourcePool::SubframeInfo> > Sa_pastTx, L1;

   Sa_pastTx = Sa; 

   uint16_t N_CSR_per_SF = NSubCh - L_SubCh + 1;

   // Remove old frames used for transmission
   m_prevListUpdate.frameNo = currentSF.frameNo+1;
   m_prevListUpdate.subframeNo = currentSF.subframeNo+1;
   UpdatePastTxInfo(currentSF.frameNo+1, currentSF.subframeNo+1);

   // Print the list of CSRs (Sa)
 /*  NS_LOG_DEBUG("Printing the initial list Sa of candidate resources, after removing past transmissions");
   UnimorePrintCSR(Sa);*/

   std::list<std::pair<Time,SidelinkCommResourcePool::SubframeInfo>>::iterator pastTxIt;

   // Print the frames used for past transmissions 
 /*  NS_LOG_DEBUG("Print the frames used for past transmissions, UE " << m_rnti);
   for (pastTxIt = m_pastTxUnimore.begin (); pastTxIt != m_pastTxUnimore.end (); pastTxIt++)
   {
     NS_LOG_DEBUG("Current time: " << Simulator::Now().GetSeconds() << " Tx Time: " << pastTxIt->first.GetSeconds() << " Frame: " << pastTxIt->second.frameNo << " subframe: " << pastTxIt->second.subframeNo);
   }*/
//   if (m_rnti == m_debugNode)
//   std::cin.get();

   // Create a list of the subframes to be removed from the selection window
   std::vector<uint16_t>::iterator RRIit;
   std::list<SidelinkCommResourcePool::SubframeInfo> rm_pastTx_frames;
   for(RRIit = m_RRIvalues.begin(); RRIit != m_RRIvalues.end(); RRIit++)
   {
     uint16_t RRI_to_slot = *RRIit/m_slotDuration;
//     NS_LOG_DEBUG("Working with RRI: " << *RRIit << " ms, and " << RRI_to_slot << " slots at SF(" << currentSF.frameNo << ", " << currentSF.subframeNo << ")");
     for (pastTxIt = m_pastTxUnimore.begin (); pastTxIt != m_pastTxUnimore.end (); pastTxIt++)
     {
     //  NS_LOG_DEBUG("Past transmission at SF(" << pastTxIt->second.frameNo << ", " << pastTxIt->second.subframeNo << ")");
       uint16_t Q;
       SidelinkCommResourcePool::SubframeInfo insert_pastTx;
       if ((SubtractFrames( currentSF.frameNo, pastTxIt->second.frameNo, currentSF.subframeNo, pastTxIt->second.subframeNo) <= *RRIit) && (*RRIit < (T_2 + 1)) )
       {
//         Q = std::ceil( (float) (T_2 + 1)/ *RRIit );
         Q = std::ceil( (float) T_2/ *RRIit );
//         NS_LOG_DEBUG("IF clause, Q= " << Q)  ;
         for(uint16_t q = 1; q <= Q; q++)
         {
           insert_pastTx.subframeNo = (pastTxIt->second.subframeNo + q*RRI_to_slot)%10; // valid subframe index between 0 and 9
           insert_pastTx.frameNo = (pastTxIt->second.frameNo  + (pastTxIt->second.subframeNo + q*RRI_to_slot) / 10) % 1024;  // valid frame index between 0 and 1023 
//           NS_LOG_DEBUG("q= " << q << ", q*RRI= " << q*RRI_to_slot << " eliminate frame SF(" << insert_pastTx.frameNo << ", " << insert_pastTx.subframeNo << ")");
           if ( find(rm_pastTx_frames.begin(), rm_pastTx_frames.end(), insert_pastTx) ==  rm_pastTx_frames.end())
             rm_pastTx_frames.push_back(insert_pastTx);
         }
       }
       else
       { 
         Q = 1;
//         NS_LOG_DEBUG("ELSE clause, Q= " << Q)  ;
         insert_pastTx.subframeNo = (pastTxIt->second.subframeNo + Q*RRI_to_slot) % 10; // valid subframe index between 0 and 9
         insert_pastTx.frameNo = (pastTxIt->second.frameNo  + (pastTxIt->second.subframeNo + Q*RRI_to_slot) / 10) % 1024;  // valid frame index between 0 and 1023 
//         NS_LOG_DEBUG("q=Q= " << Q << ", Q*RRI= " << Q*RRI_to_slot << " eliminate frame SF(" << insert_pastTx.frameNo << ", " << insert_pastTx.subframeNo << ")");
         if ( find(rm_pastTx_frames.begin(), rm_pastTx_frames.end(), insert_pastTx) ==  rm_pastTx_frames.end())
           rm_pastTx_frames.push_back(insert_pastTx);
       }
     }
   
   }

 /*  NS_LOG_DEBUG("Print the list of frames to be removed");
   for(std::list<SidelinkCommResourcePool::SubframeInfo>::iterator rm_pastTxIt = rm_pastTx_frames.begin(); rm_pastTxIt != rm_pastTx_frames.end(); rm_pastTxIt++)
   {
     NS_LOG_DEBUG(rm_pastTxIt->frameNo << "," << rm_pastTxIt->subframeNo);
   }*/
//   if (m_rnti == m_debugNode)
//     std::cin.get();

   // Now remove the frames, considering my future transmissions as well (reselection counter + RRI)
   for(std::map<uint16_t,std::list<SidelinkCommResourcePool::SubframeInfo> >::iterator SaIt = Sa_pastTx.begin(); SaIt != Sa_pastTx.end(); SaIt++)
   { 
     NS_LOG_DEBUG("CSR index " << SaIt->first);
     for(std::list<SidelinkCommResourcePool::SubframeInfo>::iterator FrameIT = SaIt->second.begin(); FrameIT != SaIt->second.end(); FrameIT++)
     {
  //     NS_LOG_DEBUG("Frame " << FrameIT->frameNo << " subframe " << FrameIT->subframeNo << ", Cresel " << V2XGrant.m_Cresel);
       SidelinkCommResourcePool::SubframeInfo futureTxFrame;
       uint16_t RRI_slots = V2XGrant.m_RRI/m_slotDuration;
       for(uint16_t Cresel = 0; Cresel < V2XGrant.m_Cresel; Cresel++)
//       for(uint16_t Cresel = 0; Cresel <= (V2XGrant.m_Cresel*10-1); Cresel++)
       {
         futureTxFrame.subframeNo = (FrameIT->subframeNo + Cresel*RRI_slots) % 10;  // valid subframe index between 0 and 9
         futureTxFrame.frameNo = (FrameIT->frameNo + (FrameIT->subframeNo + Cresel*RRI_slots) / 10) % 1024; // valid frame index between 0 and 1023
         std::list<SidelinkCommResourcePool::SubframeInfo>::iterator rm_pastTxIt;
       //  NS_LOG_DEBUG("Future Frame " << futureTxFrame.frameNo << " subframe " << futureTxFrame.subframeNo << ", Cresel " << Cresel);
         rm_pastTxIt = find(rm_pastTx_frames.begin(), rm_pastTx_frames.end(), futureTxFrame);
         if (rm_pastTxIt != rm_pastTx_frames.end())
         {
     //    if ( find(rm_pastTx_frames.begin(), rm_pastTx_frames.end(), futureTxFrame) !=  rm_pastTx_frames.end())
           NS_LOG_DEBUG("Erasing: list SF(" << FrameIT->frameNo << ", " << FrameIT->subframeNo << ") Future Tx SF(" << futureTxFrame.frameNo << ", " << futureTxFrame.subframeNo << ") past tx frame (" << rm_pastTxIt->frameNo << ", " << rm_pastTxIt->subframeNo << ")");
           FrameIT = SaIt->second.erase(FrameIT);
           FrameIT--;
     //      NS_LOG_DEBUG("Current frame (" << FrameIT->frameNo << ", " << FrameIT->subframeNo << ")");
           break;
         }
       }
     }
 //  if (m_rnti == m_debugNode)
 //    std::cin.get();
   }

 
   // Print the list of CSRs (Sa)
/*   NS_LOG_DEBUG("Printing the initial list Sa of candidate resources, after removing past transmissions");
     UnimorePrintCSR(Sa_pastTx);*/

//   if (m_rnti == m_debugNode)
//   std::cin.get();

   uint32_t nCSRtot = 1; //ComputeResidualCSRs (L1)
   uint32_t nCSRresidual = 0;

   NS_LOG_DEBUG("List Sa size after removing past transmissions: " << ComputeResidualCSRs (Sa_pastTx) );
//   if (ComputeResidualCSRs (Sa_pastTx) == 0)
//     Sa_pastTx = Sa;
   
   // Now put Sa into L1 and start excluding reserved resources
   L1 = Sa_pastTx;
   nCSRtot = ComputeResidualCSRs (L1);
   *nCSRpartial = nCSRtot;

  /* NS_LOG_DEBUG("Printing the initial list L1 of candidate resources, after removing past transmissions");
   UnimorePrintCSR(L1);*/

 //  if (m_rnti == m_debugNode)
 //  std::cin.get();


   NS_LOG_DEBUG("Saving initial L1");
   if (m_rnti == m_debugNode && saveData)
   {
     std::ofstream L1fileAlert;
     L1fileAlert.open (m_outputPath + "L1fileAlert.txt", std::ios_base::app);
     L1fileAlert << "-----Initial L1 (after past Tx) ------ At time " << Simulator::Now().GetSeconds() << ", UE " << m_rnti << " at SF(" << currentSF.frameNo << "," << currentSF.subframeNo << ") Residual resources " << nCSRtot << " ----------" << std::endl;
     for(std::map<uint16_t,std::list<SidelinkCommResourcePool::SubframeInfo> >::iterator L1it = L1.begin(); L1it != L1.end(); L1it++)
     {
       for(std::list<SidelinkCommResourcePool::SubframeInfo>::iterator FrameIT = L1it->second.begin(); FrameIT != L1it->second.end(); FrameIT++)
         L1fileAlert << "CSR index " << L1it->first << " Frame " << FrameIT->frameNo << " subframe " << FrameIT->subframeNo << std::endl;
     }
     L1fileAlert.close ();
   }

   std::map < uint16_t, std::map < SidelinkCommResourcePool::SubframeInfo, std::vector<ReservedCSR> > >::iterator sensedIt;
   double L1targetSize = m_sizeThreshold;
 //  double L1targetSize = 0.2;
   while (nCSRresidual < L1targetSize * nCSRtot)
   {
     L1.clear();
     L1 = Sa_pastTx;
     NS_LOG_DEBUG("Initializing list L1. RSRP Threshold = " << *psschThresh << " dBm, L1 size " << ComputeResidualCSRs (L1)  << ", L1 Target size = " << L1targetSize);
  
     // Save the list of sensed resources
     // Remove old sensed resources (outside of the selection window)
     //   m_prevListUpdate.frameNo = frameNo+1; // Already assigned when UpdatePastTxInfo is invoked
     //   m_prevListUpdate.subframeNo = subframeNo+1;
     UpdateSensedCSR(currentSF.frameNo+1, currentSF.subframeNo+1);

     if (m_rnti == m_debugNode && saveData)
       NrV2XUeMac::UnimorePrintSensedCSR(m_sensedReservedCSRMap, currentSF, true);

  //   if (m_rnti == m_debugNode)
  //   std::cin.get();

     NS_LOG_DEBUG("UE " << m_rnti << " Now L1: remove reserved resources");
     NS_LOG_DEBUG("First: initialize the map of CSRs to be removed");
    /* std::map<uint16_t,std::list<SidelinkCommResourcePool::SubframeInfo> > L1_out = Sa;
     for(std::map<uint16_t,std::list<SidelinkCommResourcePool::SubframeInfo> >::iterator IT = L1_out.begin(); IT != L1_out.end(); IT++)
     {
       IT->second.clear();
     }*/
     std::map<uint16_t,std::list<SidelinkCommResourcePool::SubframeInfo> > L1_out, L1_out_full;
     for (uint16_t subCH_index = 0; subCH_index < NSubCh; subCH_index++)
     {
       std::list<SidelinkCommResourcePool::SubframeInfo> SFvector;
       SidelinkCommResourcePool::SubframeInfo fakeSF;
       fakeSF.subframeNo = 0;
       fakeSF.frameNo = 0;
       SFvector.push_back (fakeSF);           
       L1_out.insert (std::pair<uint16_t,std::list<SidelinkCommResourcePool::SubframeInfo> > (subCH_index,SFvector)); 
       L1_out_full.insert (std::pair<uint16_t,std::list<SidelinkCommResourcePool::SubframeInfo> > (subCH_index,SFvector)); 
     }
     for(std::map<uint16_t,std::list<SidelinkCommResourcePool::SubframeInfo> >::iterator IT = L1_out.begin(); IT != L1_out.end(); IT++)
     {
       IT->second.clear();
     }
     for(std::map<uint16_t,std::list<SidelinkCommResourcePool::SubframeInfo> >::iterator IT = L1_out_full.begin(); IT != L1_out_full.end(); IT++)
     {
       IT->second.clear();
     }

     // Print the list of CSRs (L1)
   /*  NS_LOG_DEBUG("Printing L1_out (initialized)");
     UnimorePrintCSR(L1_out);*/

     uint16_t Tproc0 = GetTproc0 (m_numerologyIndex);

     NS_LOG_DEBUG("Now adding the resources to be removed. The RSRP threshold is " << *psschThresh << " dBm");
     for (sensedIt = m_sensedReservedCSRMap.begin (); sensedIt != m_sensedReservedCSRMap.end (); sensedIt++)
     {
    //     NS_LOG_DEBUG("CSR index " << sensedIt->first);
       std::map<SidelinkCommResourcePool::SubframeInfo, std::vector<ReservedCSR> >::iterator sensedSFIt; 
       for (sensedSFIt = sensedIt->second.begin(); sensedSFIt != sensedIt->second.end(); sensedSFIt++)
       {
      //   NS_LOG_DEBUG("Diff: " << SubtractFrames(currentSF.frameNo, sensedSFIt->first.frameNo, currentSF.subframeNo, sensedSFIt->first.subframeNo));
         if (SubtractFrames(currentSF.frameNo, sensedSFIt->first.frameNo, currentSF.subframeNo, sensedSFIt->first.subframeNo) > Tproc0)
         {
           for (std::vector<ReservedCSR>::iterator resIt = sensedSFIt->second.begin(); resIt != sensedSFIt->second.end(); resIt++)
           {
     //        NS_LOG_DEBUG("Reservation received at SF(" << sensedSFIt->first.frameNo << "," << sensedSFIt->first.subframeNo << ") with RRI = " << resIt->RRI << ", RSRP = "
     //        << resIt->psschRsrpDb << ", Cresel = " << resIt->CreselRx << " from UE " << resIt->nodeId);
             uint16_t RRI_to_slot = resIt->RRI/m_slotDuration;
             uint16_t Q;
             SidelinkCommResourcePool::SubframeInfo toRemoveSF;
             if (resIt->psschRsrpDb >= *psschThresh)
             {
               if ((SubtractFrames( currentSF.frameNo, sensedSFIt->first.frameNo, currentSF.subframeNo, sensedSFIt->first.subframeNo) <= (resIt->RRI /m_slotDuration)) && (resIt->RRI  < T_2) )
               {
                 Q = std::ceil( (float) T_2/ resIt->RRI );
                // NS_LOG_DEBUG("-------IF clause, Q= " << Q)  ;
                 for(uint16_t q = 1; q <= Q; q++)
                 {
                   toRemoveSF.subframeNo = (sensedSFIt->first.subframeNo + q*RRI_to_slot)%10; // valid subframe index between 0 and 9
                   toRemoveSF.frameNo = (sensedSFIt->first.frameNo  + (sensedSFIt->first.subframeNo + q*RRI_to_slot) / 10) % 1024;  // valid frame index between 0 and 1023 
       //            NS_LOG_DEBUG("q=" << q << ", q*RRI= " << q*RRI_to_slot << " eliminate frame SF(" << toRemoveSF.frameNo << ", " << toRemoveSF.subframeNo << ")");
                   if ( find(L1_out[sensedIt->first].begin(), L1_out[sensedIt->first].end(), toRemoveSF) ==  L1_out[sensedIt->first].end()) 
                    L1_out[sensedIt->first].push_back(toRemoveSF);
                 }
               }
               else
               {
                 Q = 1;
            //     NS_LOG_DEBUG("-------ELSE clause, Q= " << Q)  ;
                 toRemoveSF.subframeNo = (sensedSFIt->first.subframeNo + Q*RRI_to_slot)%10; // valid subframe index between 0 and 9
                 toRemoveSF.frameNo = (sensedSFIt->first.frameNo  + (sensedSFIt->first.subframeNo + Q*RRI_to_slot) / 10) % 1024;  // valid frame index between 0 and 1023
          //       NS_LOG_DEBUG("q=Q= " << Q << ", Q*RRI= " << Q*RRI_to_slot << " eliminate frame SF(" << toRemoveSF.frameNo << ", " << toRemoveSF.subframeNo << ")");
                 if ( find(L1_out[sensedIt->first].begin(), L1_out[sensedIt->first].end(), toRemoveSF) ==  L1_out[sensedIt->first].end())
                   L1_out[sensedIt->first].push_back(toRemoveSF);
               }
             }
             else
              NS_LOG_DEBUG("Reservation received with RSRP level below the threshold");
           } //end for (std::vector<ReservedCSR>::iterator resIt = sensedSFIt->second.begin(); resIt != sensedSFIt->second.end(); resIt++)
         }
         else
         {
           NS_LOG_INFO("Reservation received too late");
         }
       } //end for (sensedSFIt = sensedIt->second.begin(); sensedSFIt != sensedIt->second.end(); sensedSFIt++)

     } //end for (sensedIt = m_sensedReservedCSRMap.begin (); sensedIt != m_sensedReservedCSRMap.end (); sensedIt++)

    /* NS_LOG_DEBUG("Printing L1_out");
     UnimorePrintCSR(L1_out);*/

     NS_LOG_DEBUG("Extending L1_out. Number of occupied subchannels: " << L_SubCh);
     L1_out_full = L1_out;
     for (uint16_t subCH_index = 0; subCH_index < NSubCh; subCH_index++) // Iterating over the CSRs of L1_out
     {
      // NS_LOG_DEBUG("Now CSR " << subCH_index);
       std::vector<uint16_t> extraCSR;
       for (uint16_t j = 1; j < L_SubCh; j++) // Subtract
       {
         if ((subCH_index - j) >= 0)
         {
     //      NS_LOG_DEBUG("Subtract " << subCH_index - j);
           extraCSR.push_back(subCH_index - j);
         }
       }
   //    extraCSR.push_back(subCH_index);
    /*   for (uint16_t j = 1; j < L_SubCh; j++) // Add
       {
         if ((subCH_index + j) < N_CSR_per_SF)
         {
           NS_LOG_DEBUG("Add " << subCH_index + j);
           extraCSR.push_back(subCH_index + j);         
         }
       }*/
       for (std::vector<uint16_t>::iterator extraCSR_it = extraCSR.begin(); extraCSR_it != extraCSR.end(); extraCSR_it++)
       {
     //    NS_LOG_DEBUG("Adding frames of CSR " << subCH_index << " to CSR " << *extraCSR_it);
         std::list<SidelinkCommResourcePool::SubframeInfo> FrameIT_extra = L1_out_full[*extraCSR_it];
         for(std::list<SidelinkCommResourcePool::SubframeInfo>::iterator FrameIT_subCH = L1_out[subCH_index].begin(); FrameIT_subCH != L1_out[subCH_index].end(); FrameIT_subCH++)
         {
      //     NS_LOG_DEBUG("SF(" << FrameIT_subCH->frameNo << "," << FrameIT_subCH->subframeNo << ")");
           if (std::find(FrameIT_extra.begin(), FrameIT_extra.end(), *FrameIT_subCH) == FrameIT_extra.end())
             L1_out_full[*extraCSR_it].push_back(*FrameIT_subCH);              
         }
       }
     }
   
   /*  NS_LOG_DEBUG("Printing L1_out_full");
     UnimorePrintCSR(L1_out_full);*/

  //   if (m_rnti == m_debugNode)
  //   std::cin.get();

     NS_LOG_DEBUG("Now removing");
     // Now remove the frames, considering my future transmissions as well (reselection counter + RRI)
     for(std::map<uint16_t,std::list<SidelinkCommResourcePool::SubframeInfo> >::iterator L1it = L1.begin(); L1it != L1.end(); L1it++)
     { 
       NS_LOG_DEBUG("CSR index " << L1it->first);
       for(std::list<SidelinkCommResourcePool::SubframeInfo>::iterator FrameIT = L1it->second.begin(); FrameIT != L1it->second.end(); FrameIT++)
       {
     //    NS_LOG_DEBUG("Frame " << FrameIT->frameNo << " subframe " << FrameIT->subframeNo << ", Cresel " << V2XGrant.m_Cresel);
         SidelinkCommResourcePool::SubframeInfo futureTxFrame;
         uint16_t RRI_slots = V2XGrant.m_RRI/m_slotDuration;
         for(uint16_t Cresel = 0; Cresel < V2XGrant.m_Cresel; Cresel++)
  //       for(uint16_t Cresel = 0; Cresel <= (V2XGrant.m_Cresel*10-1); Cresel++)
         {
           futureTxFrame.subframeNo = (FrameIT->subframeNo + Cresel*RRI_slots) % 10;  // valid subframe index between 0 and 9
           futureTxFrame.frameNo = (FrameIT->frameNo + (FrameIT->subframeNo + Cresel*RRI_slots) / 10) % 1024; // valid frame index between 0 and 1023
           std::list<SidelinkCommResourcePool::SubframeInfo>::iterator rm_reservedCSRit;
           rm_reservedCSRit = find(L1_out_full[L1it->first].begin(), L1_out_full[L1it->first].end(), futureTxFrame);
           if (rm_reservedCSRit != L1_out_full[L1it->first].end())
           {
     //    if ( find(rm_pastTx_frames.begin(), rm_pastTx_frames.end(), futureTxFrame) !=  rm_pastTx_frames.end())
             NS_LOG_DEBUG("Erasing: current frame (" << FrameIT->frameNo << ", " << FrameIT->subframeNo << ") Future frame (" << futureTxFrame.frameNo << ", " << futureTxFrame.subframeNo << ") reserved frame (" << rm_reservedCSRit->frameNo << ", " << rm_reservedCSRit->subframeNo << ")");
             FrameIT = L1it->second.erase(FrameIT);
             FrameIT--;
       //      NS_LOG_DEBUG("Current frame (" << FrameIT->frameNo << ", " << FrameIT->subframeNo << ")");
             break;
           }
         }
       }
     }

     nCSRresidual = ComputeResidualCSRs (L1);
     *psschThresh += 3;
     *iterationsCounter += 1;

   } //end while
        
   NS_LOG_DEBUG("Saving final L1");
   if (m_rnti == m_debugNode && saveData)
   {
     std::ofstream L1fileAlert;
     L1fileAlert.open (m_outputPath + "L1fileAlert.txt", std::ios_base::app);
     L1fileAlert << "-----Final L1------ At time " << Simulator::Now().GetSeconds() << ", UE " << m_rnti << " at SF(" << currentSF.frameNo << "," << currentSF.subframeNo << ") Residual resources " << nCSRresidual << " ----------" << std::endl;
     for(std::map<uint16_t,std::list<SidelinkCommResourcePool::SubframeInfo> >::iterator L1it = L1.begin(); L1it != L1.end(); L1it++)
     {
       for(std::list<SidelinkCommResourcePool::SubframeInfo>::iterator FrameIT = L1it->second.begin(); FrameIT != L1it->second.end(); FrameIT++)
         L1fileAlert << "CSR index " << L1it->first << " Frame " << FrameIT->frameNo << " subframe " << FrameIT->subframeNo << std::endl;
     }
     L1fileAlert.close ();
   }

   return L1;

}


void
NrV2XUeMac::UnimorePrintCSR (std::map<uint16_t,std::list<SidelinkCommResourcePool::SubframeInfo> > Map)
{
  NS_LOG_FUNCTION(this);

  for(std::map<uint16_t,std::list<SidelinkCommResourcePool::SubframeInfo> >::iterator MapIT = Map.begin(); MapIT != Map.end(); MapIT++)
  {
    NS_LOG_DEBUG("CSR index " << MapIT->first);
    for(std::list<SidelinkCommResourcePool::SubframeInfo>::iterator FrameIT = MapIT->second.begin(); FrameIT != MapIT->second.end(); FrameIT++)
      NS_LOG_DEBUG("Frame " << FrameIT->frameNo << " subframe " << FrameIT->subframeNo);
  }

}


void
NrV2XUeMac::UnimorePrintSensedCSR (std::map <uint16_t,std::map<SidelinkCommResourcePool::SubframeInfo, std::vector<ReservedCSR>> > SensedResources_Map, SidelinkCommResourcePool::SubframeInfo currentSF, bool save)
{
  NS_LOG_FUNCTION(this);
//  NS_LOG_DEBUG("Printing the list of sensed CSRs, at time: " << Simulator::Now ().GetSeconds ());
  std::map <uint16_t,std::map<SidelinkCommResourcePool::SubframeInfo, std::vector<ReservedCSR>> >::iterator sensedIt;

 /* NS_LOG_DEBUG("Print the list of sensed resources at SF(" << currentSF.frameNo << ", " << currentSF.subframeNo << ")");
  for (sensedIt = SensedResources_Map.begin (); sensedIt != SensedResources_Map.end (); sensedIt++)
  {
    std::map<SidelinkCommResourcePool::SubframeInfo, std::vector<ReservedCSR> >::iterator sensedSFIt; 
    NS_LOG_DEBUG("CSR index " << sensedIt->first);
    for (sensedSFIt = sensedIt->second.begin(); sensedSFIt != sensedIt->second.end(); sensedSFIt++)
    {
      for (std::vector<ReservedCSR>::iterator resIt = sensedSFIt->second.begin(); resIt != sensedSFIt->second.end(); resIt++)
        NS_LOG_DEBUG("SF(" << sensedSFIt->first.frameNo << "," << sensedSFIt->first.subframeNo << ") RRI = " << resIt->RRI << ", RSRP = " << resIt->psschRsrpDb << ", Cresel = " << resIt->CreselRx << " from UE " << resIt->nodeId);
    }
  }*/

  std::ofstream sensingDebug;
  sensingDebug.open (m_outputPath + "UnimoreSensingDebug.txt", std::ios_base::app);
  sensingDebug << "--------------------------------------------------\r\n \r\n";
  sensingDebug << "Sensed Reservation List at RNTI " << m_rnti << " at time " << Simulator::Now ().GetSeconds () << ", SF(" << currentSF.frameNo+1 << "," << currentSF.subframeNo+1 << ")\r\n";
  for (sensedIt = SensedResources_Map.begin (); sensedIt != SensedResources_Map.end (); sensedIt++)
  {
    sensingDebug << " CSR Index " <<  (int) (*sensedIt).first << " :\r\n";
    std::map<SidelinkCommResourcePool::SubframeInfo, std::vector<ReservedCSR>>::iterator sensedSFIt;
    for (sensedSFIt = sensedIt->second.begin (); sensedSFIt != sensedIt->second.end (); sensedSFIt++)
    {
      for (std::vector<ReservedCSR>::iterator resIt = sensedSFIt->second.begin(); resIt != sensedSFIt->second.end(); resIt++)
      {
        sensingDebug << "      SF(" << sensedSFIt->first.frameNo+1 << "," << sensedSFIt->first.subframeNo+1 << "), reception time: " << resIt->reservationTime 
        << ", RRI = " << resIt->RRI << ", RSRP = " << resIt->psschRsrpDb << ", Cresel = " << resIt->CreselRx << " from UE " << resIt->nodeId << std::endl; 
      }
    }
  }
  sensingDebug.close();

}


void 
NrV2XUeMac::UpdatePastTxInfo (uint16_t current_frameNo, uint16_t current_subframeNo)
{
   NS_LOG_FUNCTION(this);
   uint16_t sensingWindow_slots = m_sensingWindow/m_slotDuration;
   current_frameNo--;
   current_subframeNo--;

   std::list<std::pair<Time,SidelinkCommResourcePool::SubframeInfo>>::iterator pastTxIterator;
   //Remove resources already used for transmission
   NS_LOG_DEBUG("Clean the frames used for past transmissions, UE " << m_rnti << " at SF(" << current_frameNo << ", " << current_subframeNo << "), Sensing window = " << sensingWindow_slots << " slots");
   // Remove old frames used for transmission
//   std::list<std::pair<Time,SidelinkCommResourcePool::SubframeInfo>>::iterator pastTxIt;
   for (pastTxIterator = m_pastTxUnimore.begin (); pastTxIterator != m_pastTxUnimore.end (); pastTxIterator++)
   {
//     NS_LOG_DEBUG("Past TX at SF(" << pastTxIterator->second.frameNo << "," << pastTxIterator->second.subframeNo << ")");
     if (SubtractFrames( current_frameNo, pastTxIterator->second.frameNo, current_subframeNo, pastTxIterator->second.subframeNo) > sensingWindow_slots)
     {
//       NS_LOG_DEBUG("Time: " << Simulator::Now().GetSeconds()-pastTxIterator->first.GetSeconds()  << " Frame: " << pastTxIterator->second.frameNo << " subframe: " << pastTxIterator->second.subframeNo << " -> Erasing (outside of S)");
       pastTxIterator = m_pastTxUnimore.erase(pastTxIterator);
       pastTxIterator--;
     }         
   }

}


void
NrV2XUeMac::UpdateSensedCSR (uint16_t current_frameNo, uint16_t current_subframeNo)
{
   NS_LOG_FUNCTION(this);
   uint16_t sensingWindow_slots = m_sensingWindow/m_slotDuration;
   current_frameNo--;
   current_subframeNo--;

   std::map < uint16_t, std::map < SidelinkCommResourcePool::SubframeInfo, std::vector<ReservedCSR> > >::iterator sensedIt;
//   NS_LOG_INFO("Remove sensed resources outside of the selection window, UE " << m_rnti << " at SF(" << current_frameNo << ", " << current_subframeNo << "), Sensing window = " << sensingWindow_slots << " slots");
   for (sensedIt = m_sensedReservedCSRMap.begin (); sensedIt != m_sensedReservedCSRMap.end (); sensedIt++)
   {
     std::map<SidelinkCommResourcePool::SubframeInfo, std::vector<ReservedCSR>>::iterator sensedSFIt; 
     for (sensedSFIt = sensedIt->second.begin(); sensedSFIt != sensedIt->second.end(); /*no increment*/)
     {
       if (SubtractFrames( current_frameNo, sensedSFIt->first.frameNo, current_subframeNo, sensedSFIt->first.subframeNo) > sensingWindow_slots)
       {
//         NS_LOG_DEBUG("CSR index " << sensedIt->first << " SF(" << sensedSFIt->first.frameNo << "," << sensedSFIt->first.subframeNo << ") -> Erasing (outside of S)" );
         sensedSFIt = sensedIt->second.erase(sensedSFIt);  
       }
       else
       {
         sensedSFIt++;
       }
     }
   } // end for (sensedIt = m_sensedReservedCSRMap.begin (); sensedIt != m_sensedReservedCSRMap.end (); sensedIt++)
 

}


void
NrV2XUeMac::ReEvaluateResources (SidelinkCommResourcePool::SubframeInfo currentSF, std::map <uint32_t, PoolInfo>::iterator IT, NistLteMacSapProvider::NistReportBufferNistStatusParameters pktParams)
{
   NS_LOG_FUNCTION(this);
   V2XSidelinkGrant currentV2Xgrant = IT->second.m_currentV2XGrant;

   SidelinkCommResourcePool::SubframeInfo checkSF;
   checkSF.frameNo = currentV2Xgrant.m_nextReservedFrame - 1;
   checkSF.subframeNo = currentV2Xgrant.m_nextReservedSubframe - 1;

//   SidelinkCommResourcePool::SubframeInfo currentSF;
//   currentSF.frameNo = currentV2Xgrant.m_ReEvaluationFrame - 1;
//   currentSF.subframeNo = currentV2Xgrant.m_ReEvaluationSubframe - 1;
   currentSF.frameNo--;
   currentSF.subframeNo--;

   NS_LOG_DEBUG("Now: SF(" << currentSF.frameNo + 1 << "," << currentSF.subframeNo + 1 << "). Last Re-evaluation at SF(" << currentV2Xgrant.m_ReEvaluationFrame << "," << currentV2Xgrant.m_ReEvaluationSubframe << "): UE " << m_rnti << " checking SF(" 
   << currentV2Xgrant.m_nextReservedFrame << "," << currentV2Xgrant.m_nextReservedSubframe << "), adjusted to SF(" << checkSF.frameNo << "," << checkSF.subframeNo << ")");   
  
//  uint16_t totalLength = (uint32_t)( currentV2Xgrant.m_rbLenPscch + currentV2Xgrant.m_rbLenPssch); //Mode 4
   uint16_t L_SubCh = (uint32_t) currentV2Xgrant.m_rbLenPssch/m_nsubCHsize;
   uint16_t NSubCh = std::floor(m_BW_RBs / m_nsubCHsize);
   uint16_t CSRindex = ((uint32_t) currentV2Xgrant.m_rbStartPssch) / m_nsubCHsize;
   
   SidelinkCommResourcePool::SubframeInfo genTimeSF;
   genTimeSF = SimulatorTimeToSubframe(Seconds(pktParams.V2XGenTime), m_slotDuration);
   double ElapsedTime;
   ElapsedTime = SubtractFrames(currentSF.frameNo+1, genTimeSF.frameNo, currentSF.subframeNo+1, genTimeSF.subframeNo)*m_slotDuration; //Expressed in ms
   double newPDB = pktParams.V2XPdb - ElapsedTime;
   NS_LOG_DEBUG("Elapsed time = " << ElapsedTime << " ms. PDB = " << pktParams.V2XPdb << " ms. New PDB = " << newPDB << " ms");
   NS_LOG_DEBUG("Time left = " << newPDB << " ms, checking CSR index " << CSRindex << ". RBs from " << (uint32_t) currentV2Xgrant.m_rbStartPscch << " to " <<  currentV2Xgrant.m_rbStartPscch + L_SubCh*m_nsubCHsize);
//   std::cin.get();
  // uint32_t T_2_slots = (newPDB-1)/m_slotDuration;

   std::map<uint16_t,std::list<SidelinkCommResourcePool::SubframeInfo> > Sa, L1, Sa_noPastTx;
 
//   Sa = SelectionWindow (currentSF, (newPDB-1)/m_slotDuration, NSubCh - L_SubCh + 1);
   Sa = SelectionWindow (currentSF, (uint32_t)((newPDB-1)/m_slotDuration +1), NSubCh - L_SubCh + 1);
   if (ComputeResidualCSRs(Sa) == 0)
   {
     NS_LOG_UNCOND("Selection window is empty");
     NS_FATAL_ERROR("Selection window is empty");
     std::cin.get();
   }
   // Print the list of CSRs (Sa)
   NS_LOG_DEBUG("Printing the initial list Sa of candidate resources");
   for(std::map<uint16_t,std::list<SidelinkCommResourcePool::SubframeInfo> >::iterator SaIt = Sa.begin(); SaIt != Sa.end(); SaIt++)
   {
     NS_LOG_DEBUG("CSR index " << SaIt->first);
     for(std::list<SidelinkCommResourcePool::SubframeInfo>::iterator FrameIT = SaIt->second.begin(); FrameIT != SaIt->second.end(); FrameIT++)
       NS_LOG_DEBUG("Frame " << FrameIT->frameNo << " subframe " << FrameIT->subframeNo);
   }

//   if (m_rnti == m_debugNode)
//     std::cin.get();

   uint32_t iterationsCounter = 0, nCSR;
   double psschThresh = m_rsrpThreshold;

   bool changeSelectedResources = false, pastTxError = false, SelWindowError = false;

   for(std::map<uint16_t,std::list<SidelinkCommResourcePool::SubframeInfo> >::iterator L1it = Sa.begin(); L1it != Sa.end(); L1it++)
   {
   //  NS_LOG_DEBUG("CSR index " << L1it->first);
     if (L1it->first == CSRindex)
     {
      /* for(std::list<SidelinkCommResourcePool::SubframeInfo>::iterator FrameIT = L1it->second.begin(); FrameIT != L1it->second.end(); FrameIT++)
         NS_LOG_DEBUG("---Frame " << FrameIT->frameNo << " subframe " << FrameIT->subframeNo);*/
       std::list<SidelinkCommResourcePool::SubframeInfo>::iterator collisionIT = std::find(L1it->second.begin(), L1it->second.end(), checkSF);
       if (collisionIT != L1it->second.end())
         NS_LOG_DEBUG("Re-evaluation of resources did not detect any imminent collision");
       else
       {
         NS_LOG_UNCOND("UE " << m_rnti << ": re-evaluation of resources detected an imminent collision at CSR " << L1it->first << " SF(" << checkSF.frameNo << "," << checkSF.subframeNo << ")");
         changeSelectedResources = true;
         SelWindowError = true;
//         std::cin.get();
         IT->second.m_currentV2XGrant = V2XSelectResources (currentSF.frameNo+1, currentSF.subframeNo+1, newPDB+m_slotDuration, pktParams.V2XPrsvp, pktParams.V2XMessageType, pktParams.V2XTrafficType, currentV2Xgrant.m_Cresel, pktParams.V2XPacketSize, pktParams.V2XReservationSize, ReEVALUATION); 
//         IT->second.m_V2X_grant_fresh = true;
       }
     }
    /* else
     {
       for(std::list<SidelinkCommResourcePool::SubframeInfo>::iterator FrameIT = L1it->second.begin(); FrameIT != L1it->second.end(); FrameIT++)
         NS_LOG_DEBUG("Frame " << FrameIT->frameNo << " subframe " << FrameIT->subframeNo);  
     }*/
   }

   if (!SelWindowError)
   {
     Sa_noPastTx = Mode2Step1_RemovePastTx (Sa, currentSF, currentV2Xgrant, newPDB);
     if (ComputeResidualCSRs(Sa_noPastTx) == 0)
     {
       NS_LOG_UNCOND("Selection window (without past transmissions) is empty");
       std::cin.get();
     }

     for(std::map<uint16_t,std::list<SidelinkCommResourcePool::SubframeInfo> >::iterator L1it = Sa_noPastTx.begin(); L1it != Sa_noPastTx.end(); L1it++)
     {
     //  NS_LOG_DEBUG("CSR index " << L1it->first);
       if (L1it->first == CSRindex)
       {
        /* for(std::list<SidelinkCommResourcePool::SubframeInfo>::iterator FrameIT = L1it->second.begin(); FrameIT != L1it->second.end(); FrameIT++)
           NS_LOG_DEBUG("---Frame " << FrameIT->frameNo << " subframe " << FrameIT->subframeNo);*/
         std::list<SidelinkCommResourcePool::SubframeInfo>::iterator collisionIT = std::find(L1it->second.begin(), L1it->second.end(), checkSF);
         if (collisionIT != L1it->second.end())
           NS_LOG_DEBUG("Re-evaluation of resources did not detect any imminent collision");
         else
         {
           NS_LOG_UNCOND("UE " << m_rnti << ": re-evaluation of resources detected an imminent collision at CSR " << L1it->first << " SF(" << checkSF.frameNo << "," << checkSF.subframeNo << ")");
           changeSelectedResources = true;
           pastTxError = true;
//         std::cin.get();
           IT->second.m_currentV2XGrant = V2XSelectResources (currentSF.frameNo+1, currentSF.subframeNo+1, newPDB+m_slotDuration, pktParams.V2XPrsvp, pktParams.V2XMessageType, pktParams.V2XTrafficType, currentV2Xgrant.m_Cresel, pktParams.V2XPacketSize, pktParams.V2XReservationSize, ReEVALUATION); 
//           IT->second.m_V2X_grant_fresh = true;
         }
       }
      /* else
       {
         for(std::list<SidelinkCommResourcePool::SubframeInfo>::iterator FrameIT = L1it->second.begin(); FrameIT != L1it->second.end(); FrameIT++)
           NS_LOG_DEBUG("Frame " << FrameIT->frameNo << " subframe " << FrameIT->subframeNo);  
       }*/
     }
   }
   if ( (!SelWindowError) && (!pastTxError) )
   {
     L1 = Mode2Step1 (Sa, currentSF, currentV2Xgrant, newPDB, NSubCh, L_SubCh, &iterationsCounter, &psschThresh, &nCSR, false);
  //   std::cin.get();
     // Print the list of CSRs (L1)
     for(std::map<uint16_t,std::list<SidelinkCommResourcePool::SubframeInfo> >::iterator L1it = L1.begin(); L1it != L1.end(); L1it++)
     {
     //  NS_LOG_DEBUG("CSR index " << L1it->first);
       if (L1it->first == CSRindex)
       {
        /* for(std::list<SidelinkCommResourcePool::SubframeInfo>::iterator FrameIT = L1it->second.begin(); FrameIT != L1it->second.end(); FrameIT++)
           NS_LOG_DEBUG("---Frame " << FrameIT->frameNo << " subframe " << FrameIT->subframeNo);*/
         std::list<SidelinkCommResourcePool::SubframeInfo>::iterator collisionIT = std::find(L1it->second.begin(), L1it->second.end(), checkSF);
         if (collisionIT != L1it->second.end())
           NS_LOG_DEBUG("Re-evaluation of resources did not detect any imminent collision");
         else
         {
           NS_LOG_UNCOND("UE " << m_rnti << ": re-evaluation of resources detected an imminent collision at CSR " << L1it->first << " SF(" << checkSF.frameNo << "," << checkSF.subframeNo << ")");
           changeSelectedResources = true;
  //         std::cin.get();
           IT->second.m_currentV2XGrant = V2XSelectResources (currentSF.frameNo+1, currentSF.subframeNo+1, newPDB+m_slotDuration, pktParams.V2XPrsvp, pktParams.V2XMessageType, pktParams.V2XTrafficType, currentV2Xgrant.m_Cresel, pktParams.V2XPacketSize, pktParams.V2XReservationSize, ReEVALUATION); 
//           IT->second.m_V2X_grant_fresh = true;
         }
       }
      /* else
       {
         for(std::list<SidelinkCommResourcePool::SubframeInfo>::iterator FrameIT = L1it->second.begin(); FrameIT != L1it->second.end(); FrameIT++)
           NS_LOG_DEBUG("Frame " << FrameIT->frameNo << " subframe " << FrameIT->subframeNo);  
       }*/
     }
   }

   UeReEvaluationInfo tmpStorage;
   tmpStorage.nodeId = m_rnti;
   tmpStorage.time = Simulator::Now ().GetSeconds ();
   tmpStorage.ReEvalSF.frameNo = currentSF.frameNo;
   tmpStorage.ReEvalSF.subframeNo = currentSF.subframeNo;
   tmpStorage.LastReEvalSF.frameNo = currentV2Xgrant.m_ReEvaluationFrame;
   tmpStorage.LastReEvalSF.subframeNo = currentV2Xgrant.m_ReEvaluationSubframe;
   tmpStorage.CheckSF.frameNo = checkSF.frameNo;
   tmpStorage.CheckSF.subframeNo = checkSF.subframeNo;
   tmpStorage.CheckCSR = CSRindex;
   tmpStorage.freshGrant = IT->second.m_V2X_grant_fresh;
   if (changeSelectedResources)
   {
     tmpStorage.reSelection = true;  
     if (SelWindowError)
       tmpStorage.reSelectionType = 1;  
     else if (pastTxError)
       tmpStorage.reSelectionType = 2;  
     else
       tmpStorage.reSelectionType = 3;  
   }
   else
   {
     tmpStorage.reSelection = false;  
     tmpStorage.reSelectionType = 0;  
   }

   NrV2XUeMac::ReEvaluationStats.push_back(tmpStorage);

   if (changeSelectedResources)
     IT->second.m_V2X_grant_fresh = true;

   if (Simulator::Now ().GetSeconds() - NrV2XUeMac::prevPrintTime_reEvaluation > m_savingPeriod)
   {
     NrV2XUeMac::prevPrintTime_reEvaluation = Simulator::Now ().GetSeconds();
     //Check if the UE is within the central 2km
     std::ofstream ReEvalFileEXT;
     ReEvalFileEXT.open (m_outputPath + "ReEvaluations_EXT.txt", std::ios_base::app);
     std::ofstream ReEvalFile;
     ReEvalFile.open (m_outputPath + "ReEvaluations.txt", std::ios_base::app);
     for (std::vector<UeReEvaluationInfo>::iterator reEvalIT = NrV2XUeMac::ReEvaluationStats.begin(); reEvalIT != NrV2XUeMac::ReEvaluationStats.end(); reEvalIT++)
     {
       if (reEvalIT->freshGrant)
         ReEvalFileEXT << "Fresh grant, ";
       else
         ReEvalFileEXT << "Non-fresh grant, "; 
       ReEvalFileEXT << "UE " << reEvalIT->nodeId << ", Re-evaluation now: " << reEvalIT->time << ", at SF(" << reEvalIT->ReEvalSF.frameNo+1 << "," << reEvalIT->ReEvalSF.subframeNo+1 << "). Last re-evaluation at SF(" 
       << reEvalIT->LastReEvalSF.frameNo  << "," << reEvalIT->LastReEvalSF.subframeNo  << "). Checking CSR index " << reEvalIT->CheckCSR << " at SF(" << reEvalIT->CheckSF.frameNo+1 << "," << reEvalIT->CheckSF.subframeNo+1 << "): ";
       if (reEvalIT->reSelection)
       {
         ReEvalFileEXT << "Re-selection: ";
         if (reEvalIT->reSelectionType == 1)
           ReEvalFileEXT << "selection window" << std::endl;
         else if (reEvalIT->reSelectionType == 2)
           ReEvalFileEXT << "past-tx" << std::endl;
         else
           ReEvalFileEXT << "collision" << std::endl;  
       }
       else
         ReEvalFileEXT << "Don't change" << std::endl;

       ReEvalFile << (int) reEvalIT->freshGrant << "," << reEvalIT->nodeId << "," << reEvalIT->time << "," << reEvalIT->ReEvalSF.frameNo+1 << "," << reEvalIT->ReEvalSF.subframeNo+1 << "," << reEvalIT->LastReEvalSF.frameNo << "," 
       << reEvalIT->LastReEvalSF.subframeNo  << "," << reEvalIT->CheckCSR << "," << reEvalIT->CheckSF.frameNo+1 << "," << reEvalIT->CheckSF.subframeNo+1 << "," << (int) reEvalIT->reSelection << "," << reEvalIT->reSelectionType << std::endl;
     }
     ReEvalFile.close();
     ReEvalFileEXT.close();
     NrV2XUeMac::ReEvaluationStats.clear();
   }

//   if (m_rnti == m_debugNode)
//     std::cin.get();

}


void
NrV2XUeMac::UnimoreUpdateReservation (std::map <uint32_t, PoolInfo>::iterator poolIt)
{
  NS_LOG_FUNCTION(this);

  poolIt->second.m_currentV2XGrant.m_Cresel--;
  NS_LOG_INFO("Cresel decremented to: " << poolIt->second.m_currentV2XGrant.m_Cresel);

  uint32_t SFtemp = poolIt->second.m_currentV2XGrant.m_nextReservedSubframe;
  uint16_t RRI_slots = poolIt->second.m_currentV2XGrant.m_RRI/m_slotDuration;
  SFtemp--;
  poolIt->second.m_currentV2XGrant.m_nextReservedSubframe = (SFtemp + RRI_slots) % 10 + 1;
  poolIt->second.m_currentV2XGrant.m_nextReservedFrame --;
  poolIt->second.m_currentV2XGrant.m_nextReservedFrame = (poolIt->second.m_currentV2XGrant.m_nextReservedFrame + (SFtemp + RRI_slots) / 10) % 1024 +1;
  SFtemp++;

  NS_LOG_UNCOND("Just (manually) updated reservation without data to Tx: SF(" << poolIt->second.m_currentV2XGrant.m_nextReservedFrame << "," << poolIt->second.m_currentV2XGrant.m_nextReservedSubframe << ")");
  m_UnutilizedReservations++;
  //std::cin.get();

}



void
NrV2XUeMac::DoSubframeIndication (uint32_t frameNo, uint32_t subframeNo)
{
   NS_LOG_FUNCTION (this << " Frame no. " << frameNo << " subframe no. " << subframeNo << " UE " << m_rnti);
   m_frameNo = frameNo;
   m_subframeNo = subframeNo;

   // TODO FIXME New for V2V

   bool aperiodicTraffic = false;
   uint32_t nodeId = 0;
//   uint32_t RNGframe, RNGsubframe;
   uint16_t TB_RBs = 0;

   RefreshHarqProcessesPacketBuffer ();
   if ((Simulator::Now () >= m_bsrLast + m_bsrPeriodicity) && (m_freshUlBsr == true))
   {
	SendReportBufferNistStatus ();
	m_bsrLast = Simulator::Now ();
	m_freshUlBsr = false;
	m_harqProcessId = (m_harqProcessId + 1) % HARQ_PERIOD; // HARQ_PERIOD = 7
   }

   //Sidelink Processes
   //There is a delay between the MAC scheduling and the transmission so we assume that we are ahead
   subframeNo += UL_PUSCH_TTIS_DELAY;
   if (subframeNo > 10)
   {
     ++frameNo;
     if (frameNo > 1024)
     {
       frameNo = 1;
       if (Simulator::Now ().GetSeconds () * 1000 > m_millisecondsFromLastAbsSFNUpdate + 1000.0)  //Useless
       //check whether the SFN cycle has not just been updated
       {
	 m_absSFN ++;
         m_millisecondsFromLastAbsSFNUpdate = Simulator::Now ().GetSeconds () * 1000;
       }
     }     
     subframeNo -= 10;
   }

//   NS_LOG_INFO (this << " Adjusted Frame no. " << frameNo << " subframe no. " << subframeNo);
   SidelinkCommResourcePool::SubframeInfo currentSF, estSF;
   estSF = SimulatorTimeToSubframe(Simulator::Now (), m_slotDuration);
   currentSF.frameNo = frameNo;
   currentSF.subframeNo = subframeNo;

   NS_LOG_INFO (this << " Adjusted Frame no. " << frameNo << " subframe no. " << subframeNo << ". Estimated: SF(" << estSF.frameNo << "," << estSF.subframeNo << ")");

 //  NS_LOG_DEBUG("Previous list update at SF(" << m_prevListUpdate.frameNo << "," << m_prevListUpdate.subframeNo << ")");

   if (SubtractFrames(frameNo, m_prevListUpdate.frameNo, subframeNo, m_prevListUpdate.subframeNo) >= 1000 )
   {
   //  NS_LOG_DEBUG("----------------------------");
     m_prevListUpdate.frameNo = frameNo;
     m_prevListUpdate.subframeNo = subframeNo;
     UpdatePastTxInfo(frameNo, subframeNo);
     UpdateSensedCSR(frameNo, subframeNo);
    // std::cin.get();
   }


   // V2X Stuff
   //Communication (D2D)
   if ((Simulator::Now () >= m_slBsrLast + m_slBsrPeriodicity) && (m_freshSlBsr == true))
   {
     SendSidelinkReportBufferStatus ();
     m_slBsrLast = Simulator::Now ();
     m_freshSlBsr = false; 
     //m_harqProcessId = (m_harqProcessId + 1) % HARQ_PERIOD; //is this true?
   }

   std::map <uint32_t, PoolInfo>::iterator poolIt;

   for (poolIt = m_sidelinkTxPoolsMap.begin() ; poolIt != m_sidelinkTxPoolsMap.end() ; poolIt++) // Actually there is only one pool
   {  
     if ((poolIt -> second.m_pool -> IsV2XEnabled() || true) && m_slGrantMcs != 0)  // If the pool is V2X enabled
     { 
       //TODO V2X stuff
       Ptr<PacketBurst> emptyPburst = CreateObject <PacketBurst> ();
       poolIt->second.m_miSlHarqProcessPacket = emptyPburst;
       //Get the BSR for this pool
       //If we have data in the queue
       //find the BSR for that pool (will also give the SidelinkLcIdentifier)
       std::map <SidelinkLcIdentifier, NistLteMacSapProvider::NistReportBufferNistStatusParameters>::iterator itBsr;
       for (itBsr = m_slBsrReceived.begin () ;  itBsr != m_slBsrReceived.end () ; itBsr++)
       {

	 if (itBsr->first.dstL2Id == poolIt->first)
	 {
	   //This is the BSR for the pool 
	   if (itBsr->second.V2XTrafficType == 0X00)
           {
	     NS_LOG_DEBUG (this << " Periodic traffic");
           }
	   else if (itBsr->second.V2XTrafficType == 0X01)
	   {
	     NS_LOG_DEBUG (this << " Aperiodic traffic");
	     aperiodicTraffic = true;
	   }
	   nodeId = itBsr->second.V2XNodeId;
         //  NS_LOG_DEBUG("Node ID = " << nodeId);

	   break;
	 }//end if (itBsr->first.dstL2Id == poolIt->first)
       }// end for (itBsr = m_slBsrReceived.begin () ;  itBsr != m_slBsrReceived.end () ; itBsr++)
    
       if (itBsr == m_slBsrReceived.end () || (*itBsr).second.txQueueSize == 0)  // If there is no data to transmit
       {
	 NS_LOG_DEBUG (this << " no BSR received. Assume no data to transfer. Valid grant? " << poolIt->second.m_V2X_grant_received);
         if (poolIt->second.m_V2X_grant_received) // If the UE has a valid reservation
         {
           //NS_LOG_DEBUG("m_nextReservedFrame: " << poolIt->second.m_currentV2XGrant.m_nextReservedFrame);
           /*if (poolIt->second.m_currentV2XGrant.m_Cresel > 0 && poolIt->second.m_currentV2XGrant.m_ReEvaluationFrame == frameNo && poolIt->second.m_currentV2XGrant.m_ReEvaluationSubframe == subframeNo)
           {
             uint32_t SFtemp_ReEval = poolIt->second.m_currentV2XGrant.m_ReEvaluationSubframe;
             uint16_t RRI_slots = poolIt->second.m_currentV2XGrant.m_RRI/m_slotDuration;
             SFtemp_ReEval--;
//           poolIt->second.m_currentV2XGrant.m_nextReservedSubframe = (SFtemp + poolIt->second.m_currentV2XGrant.m_RRI) % 10 + 1;
             poolIt->second.m_currentV2XGrant.m_ReEvaluationSubframe = (SFtemp_ReEval + RRI_slots) % 10 + 1;
             poolIt->second.m_currentV2XGrant.m_ReEvaluationFrame --;
//           poolIt->second.m_currentV2XGrant.m_nextReservedFrame = (poolIt->second.m_currentV2XGrant.m_nextReservedFrame + (SFtemp + poolIt->second.m_currentV2XGrant.m_RRI) / 10) % 1024 + 1;
             poolIt->second.m_currentV2XGrant.m_ReEvaluationFrame = (poolIt->second.m_currentV2XGrant.m_ReEvaluationFrame + (SFtemp_ReEval + RRI_slots) / 10) % 1024 + 1;       
             SFtemp_ReEval++;
             NS_LOG_INFO("Just updated re-evaluation without data to Tx: SF(" << poolIt->second.m_currentV2XGrant.m_ReEvaluationFrame << "," << poolIt->second.m_currentV2XGrant.m_ReEvaluationSubframe << ")"); 
             std::cin.get();
           }*/
           if (poolIt->second.m_currentV2XGrant.m_Cresel > 0 && poolIt->second.m_currentV2XGrant.m_nextReservedFrame == frameNo && poolIt->second.m_currentV2XGrant.m_nextReservedSubframe == subframeNo)
           {
             // Update grant reselection parameters anyway, even if higher layers did not request Tx
             poolIt->second.m_currentV2XGrant.m_Cresel--;
             NS_LOG_INFO("Cresel decremented to: " << poolIt->second.m_currentV2XGrant.m_Cresel);
             if (poolIt->second.m_currentV2XGrant.m_Cresel == 0 ) //TODO FIXME 
             { 
               poolIt->second.m_V2X_grant_received = false; //GRANT HAS EXPIRED, next time will be recomputed
             }
             uint32_t SFtemp = poolIt->second.m_currentV2XGrant.m_nextReservedSubframe;
             uint16_t RRI_slots = poolIt->second.m_currentV2XGrant.m_RRI/m_slotDuration;
             SFtemp--;
     //        poolIt->second.m_currentV2XGrant.m_nextReservedSubframe = (SFtemp + poolIt->second.m_currentV2XGrant.m_RRI) % 10 + 1;
             poolIt->second.m_currentV2XGrant.m_nextReservedSubframe = (SFtemp + RRI_slots) % 10 + 1;
             poolIt->second.m_currentV2XGrant.m_nextReservedFrame --;
//             poolIt->second.m_currentV2XGrant.m_nextReservedFrame = (poolIt->second.m_currentV2XGrant.m_nextReservedFrame + (SFtemp + poolIt->second.m_currentV2XGrant.m_RRI) / 10) % 1024 +1;
             poolIt->second.m_currentV2XGrant.m_nextReservedFrame = (poolIt->second.m_currentV2XGrant.m_nextReservedFrame + (SFtemp + RRI_slots) / 10) % 1024 +1;
             SFtemp++;

             //Adjust for having frames and subframes starting from index 1
             if (poolIt->second.m_currentV2XGrant.m_nextReservedSubframe == 0)
             {
               poolIt->second.m_currentV2XGrant.m_nextReservedSubframe ++;
             }
             if (poolIt->second.m_currentV2XGrant.m_nextReservedFrame == 0)
             {
               poolIt->second.m_currentV2XGrant.m_nextReservedFrame ++;
             }

             uint32_t SFtemp_ReEval = poolIt->second.m_currentV2XGrant.m_ReEvaluationSubframe;
         //    uint16_t RRI_slots = poolIt->second.m_currentV2XGrant.m_RRI/m_slotDuration;
             SFtemp_ReEval--;
//           poolIt->second.m_currentV2XGrant.m_nextReservedSubframe = (SFtemp + poolIt->second.m_currentV2XGrant.m_RRI) % 10 + 1;
             poolIt->second.m_currentV2XGrant.m_ReEvaluationSubframe = (SFtemp_ReEval + RRI_slots) % 10 + 1;
             poolIt->second.m_currentV2XGrant.m_ReEvaluationFrame --;
//           poolIt->second.m_currentV2XGrant.m_nextReservedFrame = (poolIt->second.m_currentV2XGrant.m_nextReservedFrame + (SFtemp + poolIt->second.m_currentV2XGrant.m_RRI) / 10) % 1024 + 1;
             poolIt->second.m_currentV2XGrant.m_ReEvaluationFrame = (poolIt->second.m_currentV2XGrant.m_ReEvaluationFrame + (SFtemp_ReEval + RRI_slots) / 10) % 1024 + 1;       
             SFtemp_ReEval++;
             NS_LOG_INFO("Just updated re-evaluation without data to Tx: SF(" << poolIt->second.m_currentV2XGrant.m_ReEvaluationFrame << "," << poolIt->second.m_currentV2XGrant.m_ReEvaluationSubframe << ")"); 

             NS_LOG_UNCOND("Now: SF(" << frameNo << "," << subframeNo << "). Just updated reservation without data to Tx: SF(" << poolIt->second.m_currentV2XGrant.m_nextReservedFrame << "," << poolIt->second.m_currentV2XGrant.m_nextReservedSubframe << ")");
             poolIt->second.m_currentV2XGrant.m_EnableReEvaluation = true; // Re-evaluate next selected resource
             m_UnutilizedReservations++;
        //     std::cin.get();
           } //end if (poolIt->second.m_currentV2XGrant.m_Cresel > 0 && poolIt->second.m_currentV2XGrant.m_nextReservedFrame == frameNo && poolIt->second.m_currentV2XGrant.m_nextReservedSubframe == subframeNo)
         } //end if (poolIt->second.m_V2X_grant_received) 
       } //end if (itBsr == m_slBsrReceived.end () || (*itBsr).second.txQueueSize == 0) 

       if (poolIt->second.m_pool->GetSchedulingType() == SidelinkCommResourcePool::UE_SELECTED)
       {
	 if (!(*itBsr).second.alreadyUESelected && !(itBsr == m_slBsrReceived.end () || (*itBsr).second.txQueueSize == 0)) //that is, if UE needs to transmit new data
         {    
           NS_LOG_INFO (this << " SL BSR size=" << m_slBsrReceived.size ());
           if (!poolIt->second.m_V2X_grant_received || ((*itBsr).second.V2XMessageType == 0x01 && (*itBsr).second.isNewV2X))
           {
             m_CounterReselections++;
             (*itBsr).second.isNewV2X = false;
             V2XSidelinkGrant processedV2Xgrant;
             NS_LOG_DEBUG("TxQueue: " << (*itBsr).second.txQueueSize);
             SidelinkCommResourcePool::SubframeInfo SFpkt;
             SFpkt = SimulatorTimeToSubframe(Seconds(itBsr->second.V2XGenTime), m_slotDuration);
             NS_LOG_DEBUG("Packet generated at: " << itBsr->second.V2XGenTime << " = SF(" << SFpkt.frameNo << "," << SFpkt.subframeNo << "), with PDB = " << itBsr->second.V2XPdb << " ms. No valid grant, creating a new one");

//             if (m_rnti == m_debugNode)
//             std::cin.get();

             processedV2Xgrant = V2XSelectResources (frameNo, subframeNo, itBsr->second.V2XPdb, itBsr->second.V2XPrsvp, itBsr->second.V2XMessageType, itBsr->second.V2XTrafficType, itBsr->second.V2XReselectionCounter, itBsr->second.V2XPacketSize, itBsr->second.V2XReservationSize, COUNTER); 
             if ((*itBsr).second.V2XMessageType == 0x01)
             {
               processedV2Xgrant.m_Cresel = 0; 
             }
             // Now assign the subframe!
             poolIt->second.m_currentV2XGrant = processedV2Xgrant;
             poolIt->second.m_V2X_grant_received = true;
             poolIt->second.m_V2X_grant_fresh = true;            
             if (m_rnti == 1 && false)
             {
               std::ofstream MAClog;
               MAClog.open (m_outputPath + "MACDebug.txt", std::ios_base::app);   
               MAClog << " V2X UE selected grant: PSCCH resource=" << (uint16_t) processedV2Xgrant.m_resPscch << ", rbStart=" << (uint16_t) processedV2Xgrant.m_rbStartPscch << ", rbLen=" << (uint16_t) processedV2Xgrant.m_rbLenPscch << ", mcs=" << (uint16_t) processedV2Xgrant.m_mcs << ", Cresel: " << processedV2Xgrant.m_Cresel << ", Next Reserved Tx: SF(" << processedV2Xgrant.m_nextReservedFrame << "," << processedV2Xgrant.m_nextReservedSubframe << ")" << "\r\n";
               MAClog.close ();
             }                       
         //    poolIt->second.m_currentV2XGrant.m_tbSize = m_amc->GetUlTbSizeFromMcs ((int) poolIt->second.m_currentV2XGrant.m_mcs, (int) poolIt->second.m_currentV2XGrant.m_rbLenPssch) / 8;
             NS_LOG_UNCOND("UE MAC: just made UE selection. Now: F:" << frameNo << ", SF:" << subframeNo);
           } //end if (!poolIt->second.m_V2X_grant_received || ((*itBsr).second.V2XMessageType == 0x01 && (*itBsr).second.isNewV2X))
         } //END OF if (!(*itBsr).second.alreadyUESelected && !(itBsr == m_slBsrReceived.end () || (*itBsr).second.txQueueSize == 0))
       } //END OF if (poolIt->second.m_pool->GetSchedulingType() == SidelinkCommResourcePool::UE_SELECTED)

//       if (poolIt->second.m_V2X_grant_received && !(*itBsr).second.alreadyUESelected && !(itBsr == m_slBsrReceived.end () || (*itBsr).second.txQueueSize == 0) && aperiodicTraffic) // without frame boundary, now not needed
       if (poolIt->second.m_V2X_grant_received && !(*itBsr).second.alreadyUESelected && !(itBsr == m_slBsrReceived.end () || (*itBsr).second.txQueueSize == 0)) // without frame boundary, now not needed
       {
//         bool latencyReselection = false, sizeReselection = false;
         SidelinkCommResourcePool::SubframeInfo SFpkt;
         SFpkt = SimulatorTimeToSubframe(Seconds(itBsr->second.V2XGenTime), m_slotDuration);
         NS_LOG_DEBUG("Packet generated at: " << itBsr->second.V2XGenTime << " = SF(" << SFpkt.frameNo << "," << SFpkt.subframeNo << "), next reservation at SF(" << poolIt->second.m_currentV2XGrant.m_nextReservedFrame << "," << poolIt->second.m_currentV2XGrant.m_nextReservedSubframe << ")");

         double ReservationDelay = SubtractFrames(poolIt->second.m_currentV2XGrant.m_nextReservedFrame, SFpkt.frameNo, poolIt->second.m_currentV2XGrant.m_nextReservedSubframe, SFpkt.subframeNo)*m_slotDuration; //Expressed in ms
         NS_LOG_DEBUG("Reservation delay = " << ReservationDelay << " ms, PDB = " << itBsr->second.V2XPdb << " ms");

         uint16_t TBlen_subCH, TBLen_RBs;
    	 poolIt->second.m_currentV2XGrant.m_tbSize = m_NRamc->GetSlSubchAndTbSizeFromMcs ((*itBsr).second.txQueueSize, poolIt->second.m_currentV2XGrant.m_mcs, m_nsubCHsize, m_BW_RBs, &TBlen_subCH, &TBLen_RBs) / 8;
         TB_RBs  = TBLen_RBs;
         NS_LOG_DEBUG("Allocated " << poolIt->second.m_currentV2XGrant.m_tbSize << " B, with " <<  (*itBsr).second.txQueueSize << " B in the queue. PSSCH length = " << TBLen_RBs << " RBs, reservation is " << poolIt->second.m_currentV2XGrant.m_rbLenPssch << " RBs");

//         double NewPDB = std::floor( itBsr->second.V2XGenTime*1000 + itBsr->second.V2XPdb -Simulator::Now().GetMilliSeconds() );

//        std::cin.get();

         if (ReservationDelay >itBsr->second.V2XPdb)
         {
           NS_ASSERT_MSG(aperiodicTraffic, "Periodic traffic should not generate latency reselections");
           m_LatencyReselections++;
           if (poolIt->second.m_currentV2XGrant.m_rbLenPssch < TBLen_RBs)
           {
             m_SizeReselections++; 
             NS_LOG_DEBUG("Now: Latency and Size Reselection");
           }
           else
             NS_LOG_DEBUG("Now: Latency Reselection");
        //   std::cin.get();
           V2XSidelinkGrant newV2Xgrant;

           if (m_oneShot)
           {
             if (poolIt->second.m_currentV2XGrant.m_rbLenPssch < TBLen_RBs)
               newV2Xgrant = V2XSelectResources (frameNo, subframeNo, itBsr->second.V2XPdb, itBsr->second.V2XPrsvp, itBsr->second.V2XMessageType, itBsr->second.V2XTrafficType, 1, itBsr->second.V2XPacketSize, itBsr->second.V2XReservationSize, LATENCYandSIZE); 
             else
               newV2Xgrant = V2XSelectResources (frameNo, subframeNo, itBsr->second.V2XPdb, itBsr->second.V2XPrsvp, itBsr->second.V2XMessageType, itBsr->second.V2XTrafficType, 1, itBsr->second.V2XPacketSize, itBsr->second.V2XReservationSize, LATENCY); 
             m_tmpV2XGrant = poolIt->second.m_currentV2XGrant;
             m_oneShotGrant = true;
           }
           else if (m_aggressive)
           {
             NS_FATAL_ERROR("Aggressive strategy has not been implemented yet");
           }
           else if (m_submissive)
           {
             NS_FATAL_ERROR("Submissive strategy has not been implemented yet");
           }
           else 
           {
            if (poolIt->second.m_currentV2XGrant.m_rbLenPssch < TBLen_RBs)
               newV2Xgrant = V2XSelectResources (frameNo, subframeNo, itBsr->second.V2XPdb, itBsr->second.V2XPrsvp, itBsr->second.V2XMessageType, itBsr->second.V2XTrafficType, itBsr->second.V2XReselectionCounter, itBsr->second.V2XPacketSize, itBsr->second.V2XReservationSize, LATENCYandSIZE); 
             else
               newV2Xgrant = V2XSelectResources (frameNo, subframeNo, itBsr->second.V2XPdb, itBsr->second.V2XPrsvp, itBsr->second.V2XMessageType, itBsr->second.V2XTrafficType, itBsr->second.V2XReselectionCounter, itBsr->second.V2XPacketSize, itBsr->second.V2XReservationSize, LATENCY); 
           }
           poolIt->second.m_V2X_grant_fresh = true;
           poolIt->second.m_currentV2XGrant = newV2Xgrant;
         }
         else if (poolIt->second.m_currentV2XGrant.m_rbLenPssch < TBLen_RBs)
         {
         //  NS_ASSERT_MSG(aperiodicTraffic, "Periodic traffic should not generate size reselections");
           NS_LOG_DEBUG("Now: Size Reselection");
         //  std::cin.get();
           m_SizeReselections++; 
           V2XSidelinkGrant newV2Xgrant;
           SidelinkCommResourcePool::SubframeInfo nextReservedSF; 
           nextReservedSF.frameNo = poolIt->second.m_currentV2XGrant.m_nextReservedFrame;
           nextReservedSF.subframeNo = poolIt->second.m_currentV2XGrant.m_nextReservedSubframe;

           if (m_oneShot)
           {
             newV2Xgrant = V2XSelectResources (frameNo, subframeNo, itBsr->second.V2XPdb, itBsr->second.V2XPrsvp, itBsr->second.V2XMessageType, itBsr->second.V2XTrafficType, 1, itBsr->second.V2XPacketSize, itBsr->second.V2XReservationSize, SIZE); 
           
             NS_LOG_DEBUG("New reservation at SF(" << newV2Xgrant.m_nextReservedFrame << "," << newV2Xgrant.m_nextReservedSubframe 
             << "). Old reservation at SF(" << nextReservedSF.frameNo << "," << nextReservedSF.subframeNo << ")");
             NS_LOG_DEBUG("New diff = " << SubtractFrames(newV2Xgrant.m_nextReservedFrame, frameNo, newV2Xgrant.m_nextReservedSubframe, subframeNo) << ", old diff = " << SubtractFrames(nextReservedSF.frameNo, frameNo, nextReservedSF.subframeNo, subframeNo));

             if ( SubtractFrames(newV2Xgrant.m_nextReservedFrame, frameNo, newV2Xgrant.m_nextReservedSubframe, subframeNo) >= SubtractFrames(nextReservedSF.frameNo, frameNo, nextReservedSF.subframeNo, subframeNo) )
             {
               NS_LOG_INFO("Manually update the current grant");
               UnimoreUpdateReservation (poolIt);

               nextReservedSF.frameNo = poolIt->second.m_currentV2XGrant.m_nextReservedFrame;
               nextReservedSF.subframeNo = poolIt->second.m_currentV2XGrant.m_nextReservedSubframe;

               NS_LOG_DEBUG("New reservation at SF(" << newV2Xgrant.m_nextReservedFrame << "," << newV2Xgrant.m_nextReservedSubframe << "). Old reservation at SF(" << nextReservedSF.frameNo << "," << nextReservedSF.subframeNo << ")");
               NS_LOG_DEBUG("New diff = " << SubtractFrames(newV2Xgrant.m_nextReservedFrame, frameNo, newV2Xgrant.m_nextReservedSubframe, subframeNo) << ", old diff = " << SubtractFrames(nextReservedSF.frameNo, frameNo, nextReservedSF.subframeNo, subframeNo));
          //     std::cin.get(); 
             }

             m_tmpV2XGrant = poolIt->second.m_currentV2XGrant;
             m_oneShotGrant = true;
           }
           else 
           {
             newV2Xgrant = V2XSelectResources (frameNo, subframeNo, itBsr->second.V2XPdb, itBsr->second.V2XPrsvp, itBsr->second.V2XMessageType, itBsr->second.V2XTrafficType, itBsr->second.V2XReselectionCounter, itBsr->second.V2XPacketSize, itBsr->second.V2XReservationSize, SIZE); 
           }

           poolIt->second.m_V2X_grant_fresh = true;
           poolIt->second.m_currentV2XGrant = newV2Xgrant;

         }
         else
         {
           NS_LOG_DEBUG("Packet respects the current reservation");
         }


         //std::cin.get();

         NS_LOG_DEBUG("Re-eval next selected resource? " << poolIt->second.m_currentV2XGrant.m_EnableReEvaluation << ". Re-eval at SF(" << poolIt->second.m_currentV2XGrant.m_ReEvaluationFrame << "," << poolIt->second.m_currentV2XGrant.m_ReEvaluationSubframe << "). Selected at SF(" << poolIt->second.m_currentV2XGrant.m_nextReservedFrame << "," << poolIt->second.m_currentV2XGrant.m_nextReservedSubframe << ")");
      //   std::cin.get();
         if (m_reEvaluation && SubtractFrames(poolIt->second.m_currentV2XGrant.m_nextReservedFrame, frameNo, poolIt->second.m_currentV2XGrant.m_nextReservedSubframe, subframeNo) >= GetTproc1 (m_numerologyIndex))
         {
           if (poolIt->second.m_currentV2XGrant.m_ReEvaluationFrame == frameNo && poolIt->second.m_currentV2XGrant.m_ReEvaluationSubframe == subframeNo)
           {
             NS_LOG_DEBUG("Re-evaluation now SF(" << frameNo << "," << subframeNo << ")");
//            if (m_reEvaluation && poolIt->second.m_V2X_grant_fresh)
             if (poolIt->second.m_currentV2XGrant.m_EnableReEvaluation)
             {
//               std::cin.get();
//               ReEvaluateResources(poolIt->second.m_currentV2XGrant, itBsr->second);
               ReEvaluateResources(currentSF, poolIt, itBsr->second);
             }

           }
           else if (SubtractFrames(poolIt->second.m_currentV2XGrant.m_ReEvaluationFrame, frameNo, poolIt->second.m_currentV2XGrant.m_ReEvaluationSubframe, subframeNo) >= 0 && m_allSlotsReEvaluation)
           {
             NS_LOG_DEBUG("Re-evaluation now SF(" << frameNo << "," << subframeNo << ")");
//            if (m_reEvaluation && poolIt->second.m_V2X_grant_fresh)
             if (poolIt->second.m_currentV2XGrant.m_EnableReEvaluation)
             {
//               std::cin.get();
//               ReEvaluateResources(poolIt->second.m_currentV2XGrant, itBsr->second);
               ReEvaluateResources(currentSF, poolIt, itBsr->second);
             }
           }
         }
         else
         {
           if(SubtractFrames(poolIt->second.m_currentV2XGrant.m_nextReservedFrame, frameNo, poolIt->second.m_currentV2XGrant.m_nextReservedSubframe, subframeNo) < GetTproc1 (m_numerologyIndex))
           {
             NS_LOG_DEBUG("Packet arrived too late for re-evaluation");
       //      if (poolIt->second.m_currentV2XGrant.m_EnableReEvaluation)
       //      {
       //        NS_LOG_UNCOND("Eccolo");
       //        std::cin.get();
       //      }
           }

         }
         if (poolIt->second.m_currentV2XGrant.m_nextReservedFrame == frameNo && poolIt->second.m_currentV2XGrant.m_nextReservedSubframe == subframeNo)
         {
           NS_LOG_INFO("Received grant for SF(" << frameNo << ", " << subframeNo << ")");

           m_updateReservation = true;
           m_validReservation = true; 
           m_Reservations++;

           if (m_oneShotGrant)
             m_updateReservation = false;

	   // Change also the fresh grant
       /*    if (poolIt->second.m_V2X_grant_fresh)
           { 
	    // poolIt->second.m_currentV2XGrant = poolIt->second.m_currentV2XGrant;
	     m_aperiodicV2XGrant =  poolIt->second.m_currentV2XGrant;
	     // Need to make sure that no other transmission occurs before the first
           }
	   else
	   {
	     m_aperiodicV2XGrant = poolIt->second.m_currentV2XGrant;			
	   }*/

	   NS_LOG_DEBUG("Current SF(" << frameNo << "," << subframeNo << ")");
	   NS_LOG_DEBUG("Send the packet at SF(" << poolIt->second.m_currentV2XGrant.m_nextReservedFrame << "," << poolIt->second.m_currentV2XGrant.m_nextReservedSubframe << ")");

	   (*itBsr).second.alreadyUESelected = true; //FIXME please change the name of this member
           //Compute the TB size
           uint16_t TBlen_subCH, TBLen_RBs;
    	   poolIt->second.m_currentV2XGrant.m_tbSize = m_NRamc->GetSlSubchAndTbSizeFromMcs ((*itBsr).second.txQueueSize, poolIt->second.m_currentV2XGrant.m_mcs, m_nsubCHsize, m_BW_RBs, &TBlen_subCH, &TBLen_RBs) / 8;
 	//   poolIt->second.m_currentV2XGrant.m_tbSize = m_amc->GetUlTbSizeFromMcs ((int) poolIt->second.m_currentV2XGrant.m_mcs, (int) poolIt->second.m_currentV2XGrant.m_rbLenPssch) / 8;
           NS_LOG_DEBUG("Allocated " << poolIt->second.m_currentV2XGrant.m_tbSize << " B, with " <<  (*itBsr).second.txQueueSize << " B in the queue. PSSCH length = " << TBLen_RBs << " RBs, reservation is " << poolIt->second.m_currentV2XGrant.m_rbLenPssch << " RBs");
            
           TxPacketInfo tmpInfo;
           tmpInfo.packetID = itBsr->second.V2XPacketID;
           tmpInfo.txTime = Simulator::Now().GetSeconds();
           tmpInfo.selTrigger = poolIt->second.m_currentV2XGrant.m_SelectionTrigger;

	   if (poolIt->second.m_V2X_grant_fresh)	
   	   {	
	     NS_LOG_INFO("Fresh Grant");
	     // poolIt->second.m_v2xTx = poolIt->second.m_pool->GetV2XSlTransmissions (frameNo, subframeNo, poolIt->second.m_currentV2XGrant.m_rbStartPscch, poolIt->second.m_currentV2XGrant.m_rbStartPssch, poolIt->second.m_currentV2XGrant.m_rbLenPssch, poolIt->second.m_currentV2XGrant.m_subframeInitialTx, poolIt->second.m_currentV2XGrant.m_SFGap);
             poolIt->second.m_v2xTx = poolIt->second.m_pool->GetV2XSlTransmissions (frameNo, subframeNo, poolIt->second.m_currentV2XGrant.m_rbStartPscch, poolIt->second.m_currentV2XGrant.m_rbStartPssch, poolIt->second.m_currentV2XGrant.m_rbLenPssch, poolIt->second.m_currentV2XGrant.m_rbLenPscch, poolIt->second.m_currentV2XGrant.m_nextReservedFrame, poolIt->second.m_currentV2XGrant.m_nextReservedSubframe, poolIt->second.m_currentV2XGrant.m_SFGap);

             if (poolIt->second.m_currentV2XGrant.m_EnableReEvaluation)
             {
               NS_LOG_DEBUG("Packet " << itBsr->second.V2XPacketID << " transmitted without being announced");
               tmpInfo.announced = false;
//               PKTsType << itBsr->second.V2XPacketID << ",0," << poolIt->second.m_currentV2XGrant.m_SelectionTrigger << "," << Simulator::Now().GetSeconds() << std::endl;
             }
             else
             {
               NS_LOG_DEBUG("Packet " << itBsr->second.V2XPacketID << " transmitted after being announced");
               tmpInfo.announced = true;
//               PKTsType << itBsr->second.V2XPacketID << ",1," << poolIt->second.m_currentV2XGrant.m_SelectionTrigger << "," << Simulator::Now().GetSeconds() << std::endl;
               NS_FATAL_ERROR("Newly selected packets should always be re-evaluated");
             }
//             std::cin.get();
	     poolIt->second.m_V2X_grant_fresh = false; //we have just used this grant for the first time 
             poolIt->second.m_currentV2XGrant.m_EnableReEvaluation = false; 
	   }
           else // If the packet respects the reservation
	   {
             NS_LOG_INFO("Reused Grant, now: SF(" << frameNo << "," << subframeNo << ")");
             poolIt->second.m_v2xTx = poolIt->second.m_pool->GetV2XSlTransmissions (frameNo, subframeNo, poolIt->second.m_currentV2XGrant.m_rbStartPscch, poolIt->second.m_currentV2XGrant.m_rbStartPssch, poolIt->second.m_currentV2XGrant.m_rbLenPssch, poolIt->second.m_currentV2XGrant.m_rbLenPscch, poolIt->second.m_currentV2XGrant.m_nextReservedFrame, poolIt->second.m_currentV2XGrant.m_nextReservedSubframe, poolIt->second.m_currentV2XGrant.m_SFGap);
             if (poolIt->second.m_currentV2XGrant.m_EnableReEvaluation)
             {
               NS_LOG_DEBUG("Packet " << itBsr->second.V2XPacketID << " transmitted without being announced");
               tmpInfo.announced = false;
//               PKTsType << itBsr->second.V2XPacketID << ",0," << poolIt->second.m_currentV2XGrant.m_SelectionTrigger << "," << Simulator::Now().GetSeconds() << std::endl;
             }
             else
             {
               NS_LOG_DEBUG("Packet " << itBsr->second.V2XPacketID << " transmitted after being announced");
               tmpInfo.announced = true;
//               PKTsType << itBsr->second.V2XPacketID << ",1," << poolIt->second.m_currentV2XGrant.m_SelectionTrigger << "," << Simulator::Now().GetSeconds() << std::endl;
             }
 //            std::cin.get();
             poolIt->second.m_currentV2XGrant.m_EnableReEvaluation = false; 
	   }

       /*    NrV2XUeMac::TxPacketsStats.push_back(tmpInfo);

           if (Simulator::Now ().GetSeconds() - NrV2XUeMac::prevPrintTime_packetInfo > m_savingPeriod)
           {
             NrV2XUeMac::prevPrintTime_packetInfo = Simulator::Now ().GetSeconds();
             std::ofstream PKTsType;
             PKTsType.open (m_outputPath + "PacketsType.txt", std::ios_base::app);
             for(std::vector<TxPacketInfo>::iterator iiT = NrV2XUeMac::TxPacketsStats.begin(); iiT != NrV2XUeMac::TxPacketsStats.end(); iiT++)
             {
               PKTsType << iiT->packetID << "," << (int) iiT->announced << "," << iiT->selTrigger << "," << iiT->txTime << std::endl;
             }
             PKTsType.close();
             NrV2XUeMac::TxPacketsStats.clear();
           }*/

           if (m_updateReservation)
           {
    	     if (poolIt->second.m_currentV2XGrant.m_Cresel > 0)
             {
               poolIt->second.m_currentV2XGrant.m_Cresel--;
               //NS_LOG_DEBUG("Decreasing the reselection counter. Node " << m_rnti);
           //    std::cin.get();              
             }
	     uint32_t SFtemp = poolIt->second.m_currentV2XGrant.m_nextReservedSubframe;
             uint16_t RRI_slots = poolIt->second.m_currentV2XGrant.m_RRI/m_slotDuration;
             SFtemp--;
//             poolIt->second.m_currentV2XGrant.m_nextReservedSubframe = (SFtemp + poolIt->second.m_currentV2XGrant.m_RRI) % 10 + 1;
             poolIt->second.m_currentV2XGrant.m_nextReservedSubframe = (SFtemp + RRI_slots) % 10 + 1;
	     poolIt->second.m_currentV2XGrant.m_nextReservedFrame --;
//	     poolIt->second.m_currentV2XGrant.m_nextReservedFrame = (poolIt->second.m_currentV2XGrant.m_nextReservedFrame + (SFtemp + poolIt->second.m_currentV2XGrant.m_RRI) / 10) % 1024 + 1;
	     poolIt->second.m_currentV2XGrant.m_nextReservedFrame = (poolIt->second.m_currentV2XGrant.m_nextReservedFrame + (SFtemp + RRI_slots) / 10) % 1024 + 1;
	     SFtemp++;

             uint32_t SFtemp_ReEval = poolIt->second.m_currentV2XGrant.m_ReEvaluationSubframe;
          //   uint16_t RRI_slots = poolIt->second.m_currentV2XGrant.m_RRI/m_slotDuration;
             SFtemp_ReEval--;
//           poolIt->second.m_currentV2XGrant.m_nextReservedSubframe = (SFtemp + poolIt->second.m_currentV2XGrant.m_RRI) % 10 + 1;
             poolIt->second.m_currentV2XGrant.m_ReEvaluationSubframe = (SFtemp_ReEval + RRI_slots) % 10 + 1;
             poolIt->second.m_currentV2XGrant.m_ReEvaluationFrame --;
//           poolIt->second.m_currentV2XGrant.m_nextReservedFrame = (poolIt->second.m_currentV2XGrant.m_nextReservedFrame + (SFtemp + poolIt->second.m_currentV2XGrant.m_RRI) / 10) % 1024 + 1;
             poolIt->second.m_currentV2XGrant.m_ReEvaluationFrame = (poolIt->second.m_currentV2XGrant.m_ReEvaluationFrame + (SFtemp_ReEval + RRI_slots) / 10) % 1024 + 1;       
             SFtemp_ReEval++;
             poolIt->second.m_currentV2XGrant.m_EnableReEvaluation = false;
             NS_LOG_INFO("Just updated re-evaluation: SF(" << poolIt->second.m_currentV2XGrant.m_ReEvaluationFrame << "," << poolIt->second.m_currentV2XGrant.m_ReEvaluationSubframe << ")"); 
             //Re-evaluation should not be updated since it is performed only on selected resources
           }

           NS_LOG_UNCOND("Just updated reservation SF(" << poolIt->second.m_currentV2XGrant.m_nextReservedFrame << "," << poolIt->second.m_currentV2XGrant.m_nextReservedSubframe << ")");

	   for (std::list<SidelinkCommResourcePool::V2XSidelinkTransmissionInfo>::iterator txIt = poolIt->second.m_v2xTx.begin (); txIt != poolIt->second.m_v2xTx.end (); txIt++) 
	   {		
             txIt->subframe.frameNo++;
	     txIt->subframe.subframeNo++;
           }
           NS_LOG_INFO("poolIt->second.m_currentV2XGrant.m_Cresel: " << poolIt->second.m_currentV2XGrant.m_Cresel);
           if (poolIt->second.m_currentV2XGrant.m_Cresel == 0 ) //TODO FIXME 
           {  
             if (m_evalKeepProb->GetValue() > (1-m_keepProbability))
             {
               NS_LOG_UNCOND("Keep the same resources");
               std::cin.get();
               poolIt->second.m_currentV2XGrant.m_Cresel = GetCresel(poolIt->second.m_currentV2XGrant.m_RRI);
             }
             else
             {
               // Next time higher layers request TB transmission, a new reservation or Scheduling Request will be needed
               poolIt->second.m_V2X_grant_received = false; //grant has expired
               NS_LOG_INFO("Grant has expired");
             }
           }
      //     std::cin.get();  //Pause the program for debugging purposes
         } // else if (poolIt->second.m_currentV2XGrant.m_nextReservedFrame == frameNo && poolIt->second.m_currentV2XGrant.m_nextReservedSubframe == subframeNo)
         else
         {
           NS_LOG_DEBUG("Not yet ready to transmit: SF(" << frameNo << "," << subframeNo << ")");
         }
       } // if (poolIt->second.m_V2X_grant_received && !(*itBsr).second.alreadyUESelected && !(itBsr == m_slBsrReceived.end () || (*itBsr).second.txQueueSize == 0) && aperiodicTraffic) 
       std::list<SidelinkCommResourcePool::V2XSidelinkTransmissionInfo>::iterator allocIter;
       //check if we need to transmit PSCCH and PSSCH
       allocIter = poolIt->second.m_v2xTx.begin();
     
       if (allocIter != poolIt->second.m_v2xTx.end() && allocIter->subframe.frameNo == frameNo && allocIter->subframe.subframeNo  == subframeNo)
       {
         NS_LOG_UNCOND("Now: " << Simulator::Now().GetSeconds()*1000 << " ms: Ok, now I should transmit data, Frame no. " << frameNo << ", Subframe no. " << subframeNo);
         if (m_rnti == 1 && false)
         { 
           std::cout << "RNTI 1 is about to transmit\r\n";
           std::ofstream MACDebug; 
           MACDebug.open (m_outputPath + "MACDebug.txt", std::ios_base::app);
           MACDebug << "Allocated SF(" << ((*allocIter).subframe.frameNo)%1024 << "," << ((*allocIter).subframe.subframeNo)%10 << "), Current SF(" << frameNo << "," << subframeNo << ")" << "\r\n";
           MACDebug.close ();
         }
         //create SCI message
	 NistV2XSciListElement_s sci1;
	 sci1.m_rnti = m_rnti;
         sci1.m_packetID = itBsr->second.V2XPacketID;
         // The m_validReservation flag triggers the packet drop down at PHY layer. its content is conveyed at PHY layer by the SCI hopping field
         // The hopping field is unused. TODO create a new dedicated SCI field.
         if (m_validReservation)
         {
           sci1.m_hopping = 0x01; 
           m_TotalTransmissions++;
         }
         else
         {
           sci1.m_hopping = 0x00;
         }
	 //sci1.m_resPscch = poolIt->second.m_currentV2XGrant.m_resPscch;
         sci1.m_rbStartPssch = poolIt->second.m_currentV2XGrant.m_rbStartPssch;
         sci1.m_rbLenPssch = poolIt->second.m_currentV2XGrant.m_rbLenPssch;
 	 sci1.m_rbLenPssch_TB = TB_RBs;

         NS_ASSERT_MSG(poolIt->second.m_currentV2XGrant.m_rbLenPssch >= TB_RBs, "Reservation length must be greater than the TB before the transmission");

//         m_UnutilizedSubchannels += (poolIt->second.m_currentV2XGrant.m_rbLenPssch - TB_RBs)/m_nsubCHsize;
//         m_ReservedSubchannels += poolIt->second.m_currentV2XGrant.m_rbLenPssch/m_nsubCHsize;

         m_UnutilizedSubchannels += (poolIt->second.m_currentV2XGrant.m_rbLenPssch - TB_RBs); //Unused RBs actually FIXME
         m_ReservedSubchannels += poolIt->second.m_currentV2XGrant.m_rbLenPssch;

         /*Maybe not really needed*/
         sci1.m_rbStartPscch = poolIt->second.m_currentV2XGrant.m_rbStartPscch;
         sci1.m_rbLenPscch = poolIt->second.m_currentV2XGrant.m_rbLenPscch;
 	 sci1.m_mcs = poolIt->second.m_currentV2XGrant.m_mcs;
	 sci1.m_tbSize = poolIt->second.m_currentV2XGrant.m_tbSize;
	 sci1.m_groupDstId = (poolIt->first & 0xFF);
         sci1.m_SFGap = poolIt->second.m_currentV2XGrant.m_SFGap;
         if (poolIt->second.m_currentV2XGrant.m_Cresel > 0)
         {
            sci1.m_reservation = poolIt->second.m_currentV2XGrant.m_RRI;
         }
         else
         {
           sci1.m_reservation = 0; // Won't reserve resources
         }
         SidelinkCommResourcePool::SubframeInfo currentSF;
//         SidelinkCommResourcePool::SubframeInfo reservationSF;
         currentSF.frameNo = frameNo -1;  //Current time is always 1 frame and 1 subframe ahead
         currentSF.subframeNo = subframeNo -1;

	 sci1.m_reservedSubframe.frameNo = poolIt->second.m_currentV2XGrant.m_nextReservedFrame;
         sci1.m_reservedSubframe.subframeNo = poolIt->second.m_currentV2XGrant.m_nextReservedSubframe;


         currentSF.frameNo ++;
         currentSF.subframeNo ++;
         sci1.m_receivedSubframe.frameNo = currentSF.frameNo;
         sci1.m_receivedSubframe.subframeNo = currentSF.subframeNo;

         sci1.m_reTxIndex = (*allocIter).isThisAReTx;
         sci1.m_CreselRx = poolIt->second.m_currentV2XGrant.m_Cresel;
         Ptr<SciV2XLteControlMessage> msg = Create<SciV2XLteControlMessage> ();

         // Print the MAC debugger counters (for ETSI traffic)
         if (Simulator::Now ().GetSeconds() - m_prevPrintTime > m_savingPeriod)
         {
           m_prevPrintTime = Simulator::Now ().GetSeconds();
           std::ofstream DebugTMP; 
           DebugTMP.open (m_outputPath + "ReservationsDebug.txt", std::ios_base::app);
           DebugTMP << m_rnti << "," << Simulator::Now ().GetSeconds() << "," <<  m_UnutilizedSubchannels << "," << m_ReservedSubchannels << "," << m_UnutilizedReservations << "," << m_Reservations << "," << m_LatencyReselections << ","
           << m_SizeReselections << "," << m_CounterReselections << "," << m_TotalTransmissions << std::endl;
           DebugTMP.close ();
           m_UnutilizedSubchannels = 0;
           m_ReservedSubchannels = 0;
           m_UnutilizedReservations = 0;
           m_Reservations = 0;
           m_LatencyReselections = 0;
           m_SizeReselections = 0;
           m_CounterReselections = 0;
           m_TotalTransmissions = 0;
         }


         if (!m_updateReservation)
         {
           sci1.m_reservation = 0; // Notify other neighbors that these resource won't be reserved
         }
	 NS_LOG_DEBUG("SCI >> Node ID: " << nodeId << " transmitting packet " << sci1.m_packetID  << " at SF(" << sci1.m_receivedSubframe.frameNo << ", " <<  sci1.m_receivedSubframe.subframeNo << ") Cresel: " << sci1.m_CreselRx << " rbStart PSSCH: " << sci1.m_rbStartPssch << " rbLen PSSCH (reserved): " << sci1.m_rbLenPssch << " rbLen PSSCH (used): " 
         << sci1.m_rbLenPssch_TB << " rbStart PSCCH: " << sci1.m_rbStartPscch << " rbLen PSCCH: " << sci1.m_rbLenPscch << " Reserved SF(" <<  sci1.m_reservedSubframe.frameNo << ", " 
         << sci1.m_reservedSubframe.subframeNo << ") Reservation: " << sci1.m_reservation);

      //   if (m_rnti==m_debugNode)
      //   std::cin.get();

   	 msg->SetSci (sci1);
         m_uePhySapProvider->SendNistLteControlMessage (msg);   
         // Transmit data
         if (true) //FIXME check if this is the first transmission
         {
           NS_LOG_INFO (this << " New PSSCH transmission");
	   Ptr<PacketBurst> emptyPb = CreateObject <PacketBurst> ();
	   poolIt->second.m_miSlHarqProcessPacket = emptyPb;
           //get the BSR for this pool
	   //if we have data in the queue
	   //find the BSR for that pool (will also give the SidleinkLcIdentifier)
	   std::map <SidelinkLcIdentifier, NistLteMacSapProvider::NistReportBufferNistStatusParameters>::iterator itBsr;
           //  NS_ASSERT(m_slBsrReceived.size () < 2);
	   for (itBsr = m_slBsrReceived.begin () ;  itBsr != m_slBsrReceived.end () ; itBsr++)
	   {  
	     if (itBsr->first.dstL2Id == poolIt->first)
	     {   
	       //this is the BSR for the pool 
	       std::map <SidelinkLcIdentifier, NistLcInfo>::iterator it = m_slLcInfoMap.find (itBsr->first);
	       //for sidelink we should never have retxQueueSize since it is unacknowledged mode
	       //we still keep the process similar to uplink to be more generic (and maybe handle
	       //future modifications)
	       if ( ((*itBsr).second.statusPduSize > 0) || ((*itBsr).second.retxQueueSize > 0) || ((*itBsr).second.txQueueSize > 0))
	       {
		 //We have data to send in the PSSCH, notify the RRC to start/continue sending SLSS if appropriate
                 //   if (m_validReservation){
              	 m_slHasDataToTx = true;
		 m_cmacSapUser->NotifyMacHasSlDataToSend();
                 //   }
                 // else
              	 //   m_slHasDataToTx = false;
		 NS_ASSERT ((*itBsr).second.statusPduSize == 0 && (*itBsr).second.retxQueueSize == 0);
 		 //similar code as uplink transmission
		 uint32_t bytesForThisLc = poolIt->second.m_currentV2XGrant.m_tbSize;  // Notifying the number of bytes that can be transmitted
		 NS_LOG_LOGIC (this << " RNTI " << m_rnti << " Sidelink Tx " << bytesForThisLc << " bytes to LC " << (uint32_t)(*itBsr).first.lcId << " statusQueue " << (*itBsr).second.statusPduSize << " retxQueue" << (*itBsr).second.retxQueueSize << " txQueue" <<  (*itBsr).second.txQueueSize);
                 if (m_oneShotGrant)
                 {
                   m_oneShotGrant = false; 
                   poolIt->second.m_currentV2XGrant = m_tmpV2XGrant;
                   if (poolIt->second.m_currentV2XGrant.m_Cresel == 0 )
                   { 
                     if (m_evalKeepProb->GetValue() > (1-m_keepProbability))
                     {
                       NS_LOG_UNCOND("Keep the same resources");
                       std::cin.get();
                       poolIt->second.m_currentV2XGrant.m_Cresel = GetCresel(poolIt->second.m_currentV2XGrant.m_RRI);
                     }
                     else
                     {
                       // Next time higher layers request TB transmission, a new reservation or Scheduling Request will be needed
                       poolIt->second.m_V2X_grant_received = false; //grant has expired
                       NS_LOG_INFO("Grant has expired");
                     }
                   }
                   NS_LOG_DEBUG("Next reservation: SF(" << poolIt->second.m_currentV2XGrant.m_nextReservedFrame << "," << poolIt->second.m_currentV2XGrant.m_nextReservedSubframe << ") with Cresel = " << poolIt->second.m_currentV2XGrant.m_Cresel << ". Valid grant? " << poolIt->second.m_V2X_grant_received);
 
                 }
             //    std::cin.get();
		 if (((*itBsr).second.statusPduSize > 0) && (bytesForThisLc > (*itBsr).second.statusPduSize))
		 {
 		   (*it).second.macSapUser->NotifyTxOpportunity ((*itBsr).second.statusPduSize, 0, 0); 
                   if ( (*itBsr).second.alreadyUESelected)
                   { 
                     (*itBsr).second.alreadyUESelected = false;
                   }
  		   bytesForThisLc -= (*itBsr).second.statusPduSize; //decrement size available for data
		   NS_LOG_DEBUG (this << " serve STATUS " << (*itBsr).second.statusPduSize);
		   (*itBsr).second.statusPduSize = 0;
		 }
		 else
		 {      
		   if ((*itBsr).second.statusPduSize > bytesForThisLc)
		   { 
		     NS_FATAL_ERROR ("Insufficient Tx Opportunity for sending a status message");
 		   }
		 }
		 if ((bytesForThisLc > 7)    // 7 is the min TxOpportunity useful for Rlc
		    && (((*itBsr).second.retxQueueSize > 0)
	            || ((*itBsr).second.txQueueSize > 0)))
		 {
		   if ((*itBsr).second.retxQueueSize > 0)
		   {
		     NS_LOG_DEBUG (this << " serve retx DATA, bytes " << bytesForThisLc);
		     (*it).second.macSapUser->NotifyTxOpportunity(bytesForThisLc, 0, 0);
		     if ((*itBsr).second.retxQueueSize >= bytesForThisLc)
		     {
		       (*itBsr).second.retxQueueSize -= bytesForThisLc;
		     }
		     else
		     {
		       (*itBsr).second.retxQueueSize = 0;
		     }
		   }
		   else if ((*itBsr).second.txQueueSize > 0)
		   {
		     // minimum RLC overhead due to header
		     uint32_t rlcOverhead = 2;
		     NS_LOG_DEBUG (this << " serve tx DATA, bytes " << bytesForThisLc << ", RLC overhead " << rlcOverhead);
		     (*it).second.macSapUser->NotifyTxOpportunity (bytesForThisLc, 0, 0);     
                     // added for V2X
                     if ((*itBsr).second.alreadyUESelected)
                     {  
		       NS_LOG_LOGIC("already UE SELECTED");
	               (*itBsr).second.alreadyUESelected = false;
                     }
		     if ((*itBsr).second.txQueueSize >= bytesForThisLc - rlcOverhead)
		     {
		       (*itBsr).second.txQueueSize -= bytesForThisLc - rlcOverhead;
		     }
		     else
		     {
		       (*itBsr).second.txQueueSize = 0;
		     }
		   }
		 } //end if ((bytesForThisLc > 7..
		 else
		 {
		   if ( ((*itBsr).second.retxQueueSize > 0) || ((*itBsr).second.txQueueSize > 0))
		   {
		     if (poolIt->second.m_pool->GetSchedulingType() == SidelinkCommResourcePool::SCHEDULED)
		     {
		       // resend BSR info for updating eNB peer MAC
		       m_freshSlBsr = true;
		     }
		   }
		 }
		 NS_LOG_LOGIC (this << " RNTI " << m_rnti << " Sidelink Tx " << bytesForThisLc << "\t new queues " << (uint32_t)(*it).first.lcId << " statusQueue " << (*itBsr).second.statusPduSize << " retxQueue" << (*itBsr).second.retxQueueSize << " txQueue" <<  (*itBsr).second.txQueueSize);
	       } // end if ( ((*itBsr).second.statusPduSize > 0) || ((*itBsr).second.retxQueueSize > 0) || ((*itBsr).second.txQueueSize > 0))
	       break;
	     } // end if (itBsr->first.dstL2Id == poolIt->first)
           } // end for (itBsr = m_slBsrReceived.begin () ;  itBsr != m_slBsrReceived.end () ; itBsr++)
         //  std::cin.get();
	 } //end if (true)
	 else
	 {
	   NS_LOG_INFO (this << " PSSCH retransmission " << (2 - poolIt->second.m_v2xTx.size () % 2));
	   Ptr<PacketBurst> pb = poolIt->second.m_miSlHarqProcessPacket;
	   for (std::list<Ptr<Packet> >::const_iterator j = pb->Begin (); j != pb->End (); ++j)
	   {
	     Ptr<Packet> pkt = (*j)->Copy ();
	     m_uePhySapProvider->SendMacPdu (pkt);
	   }
	 }
	 poolIt->second.m_v2xTx.erase (allocIter); //clear the transmission
       }// end if (allocIter != poolIt->second.m_v2xTx.end() && (*allocIter).subframe.frameNo == frameNo && (*allocIter).subframe.subframeNo  == subframeNo)

     } // end if ( IsV2XEnabled() )
     else
     {   
       NS_ASSERT_MSG(false,"Non-V2X enabled pool!"); 
     }
  }

}

void
NrV2XUeMac::DoAddSlDestination (uint32_t destination)
{
	std::list <uint32_t>::iterator it;
	for (it = m_sidelinkDestinations.begin (); it != m_sidelinkDestinations.end ();it++) {
		if ((*it) == destination) {
			break;
		}
	}
	if (it == m_sidelinkDestinations.end ()) {
		//did not find it, so insert
		m_sidelinkDestinations.push_back (destination);
	}
}


void
NrV2XUeMac::DoRemoveSlDestination (uint32_t destination)
{
	std::list <uint32_t>::iterator it = m_sidelinkDestinations.begin ();
	while (it != m_sidelinkDestinations.end ()) {
		if ((*it) == destination) {
			m_sidelinkDestinations.erase (it);
			break;//leave the loop
		}
		it++;
	}
}

void
NrV2XUeMac::DoNotifyChangeOfTiming(uint32_t frameNo, uint32_t subframeNo)
{
	NS_LOG_FUNCTION (this);

	//there is a delay between the MAC scheduling and the transmission so we assume that we are ahead
	subframeNo += UL_PUSCH_TTIS_DELAY;
	if (subframeNo > 10)
	{
		++frameNo;
		if (frameNo > 1024)
			frameNo = 1;
		subframeNo -= 10;
	}

	std::map <uint32_t, PoolInfo>::iterator poolIt;
	for (poolIt = m_sidelinkTxPoolsMap.begin() ; poolIt != m_sidelinkTxPoolsMap.end() ; poolIt++)
	{
		poolIt->second.m_currentScPeriod = poolIt->second.m_pool->GetCurrentScPeriod (frameNo, subframeNo);
		poolIt->second.m_nextScPeriod = poolIt->second.m_pool->GetNextScPeriod (poolIt->second.m_currentScPeriod.frameNo, poolIt->second.m_currentScPeriod.subframeNo);
		//adjust because scheduler starts with frame/subframe = 1
		poolIt->second.m_nextScPeriod.frameNo++;
		poolIt->second.m_nextScPeriod.subframeNo++;
		NS_LOG_INFO (this << " Adapting the period for pool of group " << poolIt->first << ". Next period at " << poolIt->second.m_nextScPeriod.frameNo << "/" << poolIt->second.m_nextScPeriod.subframeNo);
	}
}

void
NrV2XUeMac::DoReportPsschRsrp (Time time, uint16_t rbStart, uint16_t rbLen, double rsrpDb)
{
  

  PsschRsrp rsrpStruct;
  rsrpStruct.rbStart = rbStart;
  rsrpStruct.rbStart = rbLen;
  rsrpStruct.psschRsrpDb = rsrpDb;

  m_PsschRsrpMap.insert (std::pair<Time,PsschRsrp> (time, rsrpStruct)); 
  
  NS_LOG_INFO("NrV2XUeMac::DoReportPsschRsrp at time: " << time.GetSeconds () << " s, rbStart PSSCH: " << (int)rbStart << ", rbLen PSSCH: " << rbLen << ", PSSCH-RSRP: " << rsrpDb << "dB, List length: " << m_PsschRsrpMap.size ());   

}

void
NrV2XUeMac::DoReportPsschRsrpReservation (Time time, uint16_t rbStart, uint16_t rbLen, double rsrpDb, SidelinkCommResourcePool::SubframeInfo receivedSubframe, SidelinkCommResourcePool::SubframeInfo reservedSubframe, uint32_t CreselRx, uint32_t nodeId, uint16_t RRI)
{

 reservedSubframe.frameNo = reservedSubframe.frameNo -1;
 reservedSubframe.subframeNo = reservedSubframe.subframeNo -1;

 receivedSubframe.frameNo = receivedSubframe.frameNo -1;
 receivedSubframe.subframeNo = receivedSubframe.subframeNo -1;

 NS_LOG_DEBUG("DoReportPsschRsrpReservation");
 if (!m_randomSelection)
 { 
//    bool debug = false;
    bool useCreselRx = true;

    ReservedCSR newSensedReservedCSR;
    newSensedReservedCSR.psschRsrpDb = rsrpDb;
    newSensedReservedCSR.reservationTime = time;
    newSensedReservedCSR.nodeId = nodeId;
    newSensedReservedCSR.RRI = RRI;
    newSensedReservedCSR.reservedSF.frameNo = reservedSubframe.frameNo;
    newSensedReservedCSR.reservedSF.subframeNo = reservedSubframe.subframeNo;
    
    if (useCreselRx)
    {
   //    newSensedReservedCSR.CreselRx = CreselRx + 1;
        newSensedReservedCSR.CreselRx = CreselRx;
       //newSensedReservedCSR.CreselRx = 10;
    }
    else 
    {
       newSensedReservedCSR.CreselRx = 1;     
    }
  

    NS_LOG_DEBUG("Reserved SF(" <<  reservedSubframe.frameNo << "," <<  reservedSubframe.subframeNo << "), RBs from " << rbStart << " to " << rbStart+rbLen-1);

    std::vector <int> CSRindex;
    for (uint16_t j = 0; j < rbLen/m_nsubCHsize; j++)
      CSRindex.push_back(rbStart/m_nsubCHsize + j);

    NS_LOG_DEBUG("Received SF(" <<  receivedSubframe.frameNo << "," <<  receivedSubframe.subframeNo << "), RBs from " << rbStart << " to " << rbStart+rbLen-1);
    for (std::vector <int>::iterator CSRindexIT = CSRindex.begin(); CSRindexIT != CSRindex.end(); CSRindexIT++)
    {
      std::map <uint16_t, std::map<SidelinkCommResourcePool::SubframeInfo, std::vector<ReservedCSR> > >::iterator mapIt = m_sensedReservedCSRMap.find (*CSRindexIT);
      newSensedReservedCSR.rbStart = *CSRindexIT * m_nsubCHsize;
      newSensedReservedCSR.rbLen = m_nsubCHsize;
      if (mapIt != m_sensedReservedCSRMap.end ())
      {
        //CSR index already exists
        std::map<SidelinkCommResourcePool::SubframeInfo, std::vector<ReservedCSR> >::iterator frameInfoIt = mapIt->second.find(receivedSubframe);
        if (frameInfoIt != mapIt->second.end())
        {
          NS_LOG_INFO("This SF already exists. Push back the new reservation");
          frameInfoIt->second.push_back(newSensedReservedCSR);
       //   std::cin.get();
        }
        else
          (*mapIt).second.insert (std::pair<SidelinkCommResourcePool::SubframeInfo, std::vector<ReservedCSR> > (receivedSubframe, {newSensedReservedCSR}));
      }
      else 
      {
        // CSR index not found: create it
        std::map<SidelinkCommResourcePool::SubframeInfo, std::vector<ReservedCSR>> value;
        value.insert (std::pair<SidelinkCommResourcePool::SubframeInfo, std::vector<ReservedCSR>> (receivedSubframe, {newSensedReservedCSR}));
        m_sensedReservedCSRMap.insert (std::pair<uint16_t,std::map<SidelinkCommResourcePool::SubframeInfo, std::vector<ReservedCSR>> > (*CSRindexIT,value));    
      }
    }




 } // end if (!m_randomselection)


}

void
NrV2XUeMac::DoStoreTxInfo (SidelinkCommResourcePool::SubframeInfo subframe, uint16_t rbStart, uint16_t rbLen)
{

  NS_LOG_DEBUG("Storing transmission information");

  subframe.subframeNo = subframe.subframeNo -1 ;
  subframe.frameNo = subframe.frameNo -1;

   //Calculate the CSR index
  uint16_t RBperCSR = m_nsubCHsize * m_L_SubCh;
  uint16_t CSRindex = rbStart / RBperCSR;
  CSRindex = (uint16_t) rbStart / m_nsubCHsize;

  m_pastTxUnimore.push_back(std::pair<Time,SidelinkCommResourcePool::SubframeInfo> (Simulator::Now(),subframe));

  std::map<uint16_t,std::list<SidelinkCommResourcePool::SubframeInfo> >::iterator mapIt = m_pastTxMap.find (CSRindex);
  if (mapIt != m_pastTxMap.end ())
          {
             
             // this is the right CSR index
             (*mapIt).second.push_back (subframe);
          }
        else 
         {
              // CSR index not found: create it
              std::list<SidelinkCommResourcePool::SubframeInfo> value;
              value.push_back (subframe);
              m_pastTxMap.insert (std::pair<uint16_t,std::list<SidelinkCommResourcePool::SubframeInfo> > (CSRindex,value));
              
         }
}

std::list< Ptr<SidelinkRxDiscResourcePool> > 
NrV2XUeMac::GetDiscRxPools ()
{
  NS_LOG_FUNCTION (this);
  return m_discRxPools;
}

Ptr<SidelinkTxDiscResourcePool>
NrV2XUeMac::GetDiscTxPool ()
{
  NS_LOG_FUNCTION (this);
  return m_discTxPools.m_pool; 
}

} // namespace ns3
