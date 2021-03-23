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
#include <ns3/nist-ff-mac-common.h>
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


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("NistLteUeMac");

NS_OBJECT_ENSURE_REGISTERED (NistLteUeMac);

//std::map<int, CountersReservations> UnimoreReservations = { {0,{0,0,0,0}} };
//double prevPrintTimeMAC = 0;
////////////////////
// SAP forwarders //
////////////////////


class NistUeMemberLteUeCmacSapProvider : public NistLteUeCmacSapProvider
{
public:
	NistUeMemberLteUeCmacSapProvider (NistLteUeMac* mac);

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
	NistLteUeMac* m_mac;
};


NistUeMemberLteUeCmacSapProvider::NistUeMemberLteUeCmacSapProvider (NistLteUeMac* mac)
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
	NistUeMemberLteMacSapProvider (NistLteUeMac* mac);

	// inherited from NistLteMacSapProvider
	virtual void TransmitPdu (NistTransmitPduParameters params);
	virtual void ReportBufferNistStatus (NistReportBufferNistStatusParameters params);

private:
	NistLteUeMac* m_mac;
};


NistUeMemberLteMacSapProvider::NistUeMemberLteMacSapProvider (NistLteUeMac* mac)
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
	NistUeMemberLteUePhySapUser (NistLteUeMac* mac);

	// inherited from NistLtePhySapUser
	virtual void ReceivePhyPdu (Ptr<Packet> p);
	virtual void SubframeIndication (uint32_t frameNo, uint32_t subframeNo);
	virtual void ReceiveNistLteControlMessage (Ptr<NistLteControlMessage> msg);
	virtual void NotifyChangeOfTiming (uint32_t frameNo, uint32_t subframeNo);

        virtual void ReportPsschRsrp (Time time, uint16_t rbStart, uint16_t rbLen, double rsrpDb);
        virtual void ReportPsschRsrpReservation (Time time, uint16_t rbStart, uint16_t rbLen, double rsrpDb, SidelinkCommResourcePool::SubframeInfo reservedSubframe, uint32_t CreselRx, uint32_t nodeId, uint16_t RRI);
        virtual void StoreTxInfo (SidelinkCommResourcePool::SubframeInfo subframe, uint16_t rbStart, uint16_t rbLen);

private:
	NistLteUeMac* m_mac;
};

NistUeMemberLteUePhySapUser::NistUeMemberLteUePhySapUser (NistLteUeMac* mac) : m_mac (mac)
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
NistUeMemberLteUePhySapUser::ReportPsschRsrpReservation (Time time, uint16_t rbStart, uint16_t rbLen, double rsrpDb, SidelinkCommResourcePool::SubframeInfo reservedSubframe, uint32_t CreselRx, uint32_t nodeId, uint16_t RRI)
{
	m_mac->DoReportPsschRsrpReservation (time, rbStart, rbLen, rsrpDb, reservedSubframe, CreselRx, nodeId, RRI);
}

void
NistUeMemberLteUePhySapUser::StoreTxInfo (SidelinkCommResourcePool::SubframeInfo subframe, uint16_t rbStart, uint16_t rbLen)
{
       m_mac->DoStoreTxInfo (subframe, rbStart, rbLen);
}



//////////////////////////////////////////////////////////
// NistLteUeMac methods
/////////////////////////////////////////////////////////


TypeId
NistLteUeMac::GetTypeId (void)
{
	static TypeId tid = TypeId ("ns3::NistLteUeMac")
		.SetParent<Object> ()
		.AddConstructor<NistLteUeMac> ()
		.AddAttribute ("NistUlScheduler",
					"Type of Scheduler in the UE",
					StringValue ("ns3::NistRrFfMacScheduler"),
					MakeStringAccessor (&NistLteUeMac::SetNistUlScheduler,
					&NistLteUeMac::GetNistUlScheduler),
					MakeStringChecker ())
		.AddAttribute ("Ktrp",
					"The repetition for PSSCH. Default = 0",
					UintegerValue (0),
					MakeUintegerAccessor (&NistLteUeMac::m_slKtrp),
					MakeUintegerChecker<uint8_t> ())
		.AddAttribute ("SlGrantMcs",
					"The MCS of the SL grant, must be [0..15] (default 0)",
					UintegerValue (0),
					MakeUintegerAccessor (&NistLteUeMac::m_slGrantMcs),
					MakeUintegerChecker<uint8_t> ())
		.AddAttribute ("SlGrantSize",
					"The number of RBs allocated per UE for sidelink (default 1)",
					UintegerValue (1),
					MakeUintegerAccessor (&NistLteUeMac::m_slGrantSize),
					MakeUintegerChecker<uint8_t> ())
		.AddAttribute ("PucchSize",
					"Number of RBs reserved for PUCCH (default 0)",
					UintegerValue (0),
					MakeUintegerAccessor (&NistLteUeMac::m_pucchSize),
					MakeUintegerChecker<uint8_t> ())
                .AddAttribute ("RandomV2VSelection",
                                        "Whether the resources for V2V transmissions are randomly selected in UE_SELECTED mode (default false)",
                                        BooleanValue (false),
                                        MakeBooleanAccessor (&NistLteUeMac::m_randomSelection),
                                        MakeBooleanChecker ())
		.AddTraceSource ("SlUeScheduling",
					"Information regarding SL UE scheduling",
					MakeTraceSourceAccessor (&NistLteUeMac::m_slUeScheduling),
					"ns3::NistSlUeMacStatParameters::TracedCallback")
		.AddTraceSource ("SlSharedChUeScheduling",
					"Information regarding SL Shared Channel UE scheduling",
					MakeTraceSourceAccessor (&NistLteUeMac::m_slSharedChUeScheduling),
					"ns3::NistSlUeMacStatParameters::TracedCallback")
   		 // Added to trace the transmission of discovery message
                .AddTraceSource ("DiscoveryAnnouncement",
                                        "trace to track the announcement of discovery messages",
                                        MakeTraceSourceAccessor (&NistLteUeMac::m_discoveryAnnouncementTrace),
                                        "ns3::NistLteUeMac::DiscoveryAnnouncementTracedCallback")
                .AddAttribute ("ListL2Enabled",
                                        "Enable List L2 of Mode 4 SSPS",
                                        BooleanValue (true),
                                        MakeBooleanAccessor (&NistLteUeMac::m_List2Enabled),
                                        MakeBooleanChecker ())
                .AddAttribute ("UseTxCresel",
                                        "Use the transmitter reselection counter",
                                        BooleanValue (false),
                                        MakeBooleanAccessor (&NistLteUeMac::m_useTxCresel),
                                        MakeBooleanChecker ())
                .AddAttribute ("UseRxCresel",
                                        "Use the received SCI reselection counter",
                                        BooleanValue (false),
                                        MakeBooleanAccessor (&NistLteUeMac::m_useRxCresel),
                                        MakeBooleanChecker ())
                .AddAttribute ("AggressiveMode4",
                                        "Use Aggressive Mode 4 strategy when PDB is violated",
                                        BooleanValue (false),
                                        MakeBooleanAccessor (&NistLteUeMac::m_aggressive),
                                        MakeBooleanChecker ())
                .AddAttribute ("StandardSSPS",
                                        "Use one-shot SSPS reservation when PDB is violated",
                                        BooleanValue (false),
                                        MakeBooleanAccessor (&NistLteUeMac::m_standardSSPS),
                                        MakeBooleanChecker ())
                .AddAttribute ("SubmissiveMode4",
                                        "Use Submissive Mode 4 strategy when PDB is violated",
                                        BooleanValue (false),
                                        MakeBooleanAccessor (&NistLteUeMac::m_submissive),
                                        MakeBooleanChecker ())

																									;
	return tid;
}


NistLteUeMac::NistLteUeMac ()
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
   m_nsubCHsize (10),
   m_L_SubCh (1),
   m_UnutilizedReservations (0),
   m_Reservations (0),
   m_LatencyReselections (0),
   m_CounterReselections (0),
   m_TotalTransmissions (0),
   m_prevPrintTime (0.0),
   
   m_NistUlScheduler ("ns3::NistRrFfMacScheduler")    //UE scheduler initialization
{
	NS_LOG_FUNCTION (this);
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
	m_ueSelectedUniformVariable = CreateObject<UniformRandomVariable> ();
	//m_slDiversity.status = SlDiversity::disabled;//enabled should be default!
  
  m_p1UniformVariable = CreateObject<UniformRandomVariable> ();
  m_resUniformVariable = CreateObject<UniformRandomVariable> ();
}


void
NistLteUeMac::SetNistUlScheduler (std::string UeSched)
{
	m_NistUlScheduler = UeSched;
}

std::string 
NistLteUeMac::GetNistUlScheduler (void) const
{
	return m_NistUlScheduler;
}

NistLteUeMac::~NistLteUeMac ()
{
	NS_LOG_FUNCTION (this);
}

void
NistLteUeMac::DoDispose ()
{
	NS_LOG_FUNCTION (this);
	m_miUlHarqProcessesPacket.clear ();
	delete m_macSapProvider;
	delete m_cmacSapProvider;
	delete m_uePhySapUser;
	Object::DoDispose ();
}

void
NistLteUeMac::SetNistLteUeCphySapProvider (NistLteUeCphySapProvider * s)
{
  NS_LOG_FUNCTION (this << s);
  m_cphySapProvider = s;
}

NistLteUePhySapUser*
NistLteUeMac::GetNistLteUePhySapUser (void)
{
	return m_uePhySapUser;
}

void
NistLteUeMac::SetNistLteUePhySapProvider (NistLteUePhySapProvider* s)
{
	m_uePhySapProvider = s;
}


NistLteMacSapProvider*
NistLteUeMac::GetNistLteMacSapProvider (void)
{
	return m_macSapProvider;
}

void
NistLteUeMac::SetNistLteUeCmacSapUser (NistLteUeCmacSapUser* s)
{
	m_cmacSapUser = s;
}

NistLteUeCmacSapProvider*
NistLteUeMac::GetNistLteUeCmacSapProvider (void)
{
	return m_cmacSapProvider;
}


void
NistLteUeMac::DoTransmitPdu (NistLteMacSapProvider::NistTransmitPduParameters params)
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
NistLteUeMac::DoReportBufferNistStatus (NistLteMacSapProvider::NistReportBufferNistStatusParameters params)
{
	NS_LOG_FUNCTION (this << (uint32_t) params.lcid);

	if (params.srcL2Id == 0) {
		NS_ASSERT (params.dstL2Id == 0);
		NS_LOG_INFO ("Reporting for uplink");
		//regular uplink BSR
		if ( ( m_NistUlScheduler =="ns3::NistPriorityFfMacScheduler") || ( m_NistUlScheduler == "ns3::NistPfFfMacScheduler") || ( m_NistUlScheduler == "ns3::NistMtFfMacScheduler" ) || ( m_NistUlScheduler == "ns3::NistRrSlFfMacScheduler"))
		{
			//NIST new iterator since Nist_m_ulBsrReceived is modified
			std::map <uint8_t, std::map <uint8_t, NistLteMacSapProvider::NistReportBufferNistStatusParameters> >::iterator itNist;
			std::map <uint8_t, NistLteMacSapProvider::NistReportBufferNistStatusParameters>::iterator itTempMap;
			std::map <uint8_t, NistLteMacSapProvider::NistReportBufferNistStatusParameters> TempMap;

			itNist = Nist_m_ulBsrReceived.find (m_rnti);
			if (itNist!=Nist_m_ulBsrReceived.end ())
			{
				// update entry

				TempMap=itNist->second;
				itTempMap=TempMap.find ((uint8_t)params.lcid);
				if (itTempMap == TempMap.end ())
				{
					itNist->second.insert (std::pair<uint8_t, NistLteMacSapProvider::NistReportBufferNistStatusParameters> (params.lcid, params));
					m_freshUlBsr = true;
					return;
				}
				else
				{
					(*itTempMap).second = params;
					itNist->second = TempMap;
					m_freshUlBsr = true;
					return;
				}
			}

			else
			{

				std::map<uint8_t, NistLteMacSapProvider::NistReportBufferNistStatusParameters> tempMap;
				tempMap.insert (std::pair<uint8_t, NistLteMacSapProvider::NistReportBufferNistStatusParameters> ((uint8_t)params.lcid, params));
				Nist_m_ulBsrReceived.insert (std::pair <uint8_t, std::map<uint8_t, NistLteMacSapProvider::NistReportBufferNistStatusParameters > > (m_rnti, tempMap));
				m_freshUlBsr = true;
				return;
			}
		}

		else if ( ( m_NistUlScheduler =="ns3::NistRrFfMacScheduler") ||  ( m_NistUlScheduler =="ns3::Nist3GPPcalMacScheduler"))
		{
			std::map <uint8_t, NistLteMacSapProvider::NistReportBufferNistStatusParameters>::iterator it;
			it = m_ulBsrReceived.find (params.lcid);
			if (it!=m_ulBsrReceived.end ())
			{
				// update entry
				(*it).second = params;
			}
			else
			{
				m_ulBsrReceived.insert (std::pair<uint8_t, NistLteMacSapProvider::NistReportBufferNistStatusParameters> (params.lcid, params));
			}
			m_freshUlBsr = true;
		}
	} else {
		NS_LOG_INFO ("Reporting for sidelink");
		//sidelink BSR
		std::map <SidelinkLcIdentifier, NistLteMacSapProvider::NistReportBufferNistStatusParameters>::iterator it;
                // m_slBsrReceived is the last BSR received from the RLC layer. I have to update it with the content of the "params" container coming from above
//		std::printf ("Message type: %" PRIu32 ", PDB: %" PRIu32 " and Traffic type: %" PRIu32 "\n", params.V2XMessageType, params.V2XPdb, params.V2XTrafficType);
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
NistLteUeMac::SendReportBufferNistStatus (void)
{
	NS_LOG_FUNCTION (this);

	if (m_rnti == 0)
	{
		NS_LOG_INFO ("MAC not initialized, BSR deferred");
		return;
	}

	if (Nist_m_ulBsrReceived.size () == 0)
	{
		NS_LOG_INFO ("No NIST BSR report to transmit");

		if (m_ulBsrReceived.size () == 0)
		{
			NS_LOG_INFO ("No BSR report to transmit");
			return;
		}
		NistMacCeListElement_s bsr;
		bsr.m_rnti = m_rnti;
		bsr.m_macCeType = NistMacCeListElement_s::BSR;

		// BSR is reported for each LCG
		std::map <uint8_t, NistLteMacSapProvider::NistReportBufferNistStatusParameters>::iterator it;
		std::vector<uint32_t> queue (4, 0); // one value per each of the 4 LCGs, initialized to 0
		for (it = m_ulBsrReceived.begin (); it != m_ulBsrReceived.end (); it++)
		{
			uint8_t lcid = it->first;
			std::map <uint8_t, NistLcInfo>::iterator lcInfoMapIt;
			lcInfoMapIt = m_lcInfoMap.find (lcid);
			NS_ASSERT (lcInfoMapIt !=  m_lcInfoMap.end ());
			NS_ASSERT_MSG ((lcid != 0) || (((*it).second.txQueueSize == 0)
					&& ((*it).second.retxQueueSize == 0)
					&& ((*it).second.statusPduSize == 0)),
					"BSR should not be used for LCID 0");
			uint8_t lcg = lcInfoMapIt->second.lcConfig.logicalChannelGroup;
			queue.at (lcg) += ((*it).second.txQueueSize + (*it).second.retxQueueSize + (*it).second.statusPduSize);
		}

		// FF API says that all 4 LCGs are always present
		bsr.m_macCeValue.m_bufferNistStatus.push_back (NistBufferSizeLevelBsr::BufferSize2BsrId (queue.at (0)));
		bsr.m_macCeValue.m_bufferNistStatus.push_back (NistBufferSizeLevelBsr::BufferSize2BsrId (queue.at (1)));
		bsr.m_macCeValue.m_bufferNistStatus.push_back (NistBufferSizeLevelBsr::BufferSize2BsrId (queue.at (2)));
		bsr.m_macCeValue.m_bufferNistStatus.push_back (NistBufferSizeLevelBsr::BufferSize2BsrId (queue.at (3)));

		// create the feedback to eNB
		Ptr<NistBsrLteControlMessage> msg = Create<NistBsrLteControlMessage> ();
		msg->SetBsr (bsr);
		m_uePhySapProvider->SendNistLteControlMessage (msg);
	}

	NistMacCeListElement_s bsr;
	bsr.m_rnti = m_rnti;
	bsr.m_macCeType = NistMacCeListElement_s::BSR;

	// BSR is reported for each LCG

	// NIST new iterator for m_ulBsrReceived
	std::map <uint8_t, std::map <uint8_t, NistLteMacSapProvider::NistReportBufferNistStatusParameters > >:: iterator it;
	it=Nist_m_ulBsrReceived.find(m_rnti);

	if (it!=Nist_m_ulBsrReceived.end())
	{
		std::map <uint8_t, NistLteMacSapProvider::NistReportBufferNistStatusParameters > mapLC=it->second;
		std::map <uint8_t, NistLteMacSapProvider::NistReportBufferNistStatusParameters >::iterator it1=mapLC.begin();

		std::vector<uint32_t> queue (4, 0); // one value per each of the 4 LCGs, initialized to 0
		std::vector<uint32_t> queueLCG (4, 0);                                                                                  // this queue is used to fill in the ns3 structure
		for (it1= mapLC.begin (); it1 != mapLC.end (); it1++)
		{
			uint8_t lcid = it1->first;
			std::map <uint8_t, NistLcInfo>::iterator lcInfoMapIt;
			lcInfoMapIt = m_lcInfoMap.find (lcid);
			NS_ASSERT (lcInfoMapIt !=  m_lcInfoMap.end ());
			NS_ASSERT_MSG ((lcid != 0) || (((*it1).second.txQueueSize == 0)
					&& ((*it1).second.retxQueueSize == 0)
					&& ((*it1).second.statusPduSize == 0)),
					"BSR should not be used for LCID 0");
			uint8_t lcg = lcInfoMapIt->second.lcConfig.logicalChannelGroup;
			queue.at (lcg) = ((*it1).second.txQueueSize + (*it1).second.retxQueueSize + (*it1).second.statusPduSize);
			queueLCG.at (lcg) += ((*it1).second.txQueueSize + (*it1).second.retxQueueSize + (*it1).second.statusPduSize);       // this queue is used to fill in the ns3 structure
			std::vector <uint8_t>  bufferStatus (4,0);
			bufferStatus[lcg]=NistBufferSizeLevelBsr::BufferSize2BsrId (queue.at (lcg));
			bsr.m_macCeValue.Nist_m_bufferNistStatus.insert (std::pair <uint8_t, std::vector <uint8_t> > (lcid, bufferStatus));
		}


		//filling in the structure of bsr buffer implemented by ns3 beacause if UE scheduler is RR , it will check this structure
		bsr.m_macCeValue.m_bufferNistStatus.push_back (NistBufferSizeLevelBsr::BufferSize2BsrId (queueLCG.at (0)));
		bsr.m_macCeValue.m_bufferNistStatus.push_back (NistBufferSizeLevelBsr::BufferSize2BsrId (queueLCG.at (1)));
		bsr.m_macCeValue.m_bufferNistStatus.push_back (NistBufferSizeLevelBsr::BufferSize2BsrId (queueLCG.at (2)));
		bsr.m_macCeValue.m_bufferNistStatus.push_back (NistBufferSizeLevelBsr::BufferSize2BsrId (queueLCG.at (3)));
		// end added

		// create the feedback to eNB
		Ptr<NistBsrLteControlMessage> msg = Create<NistBsrLteControlMessage> ();
		msg->SetBsr (bsr);
		m_uePhySapProvider->SendNistLteControlMessage (msg);
	}

}

void
NistLteUeMac::SendSidelinkReportBufferStatus (void)
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
NistLteUeMac::RandomlySelectAndSendRaPreamble ()
{
	NS_LOG_FUNCTION (this);
	// 3GPP 36.321 5.1.1
	NS_ASSERT_MSG (m_rachConfigured, "RACH not configured");
	// assume that there is no Random Access Preambles group B
	m_raPreambleId = m_raPreambleUniformVariable->GetInteger (0, m_rachConfig.numberOfRaPreambles - 1);
	bool contention = true;
	SendRaPreamble (contention);
}

void
NistLteUeMac::SendRaPreamble (bool contention)
{
	NS_LOG_FUNCTION (this << (uint32_t) m_raPreambleId << contention);
	// Since regular UL NistLteControlMessages need m_ulConfigured = true in
	// order to be sent by the UE, the rach preamble needs to be sent
	// with a dedicated primitive (not
	// m_uePhySapProvider->SendNistLteControlMessage (msg)) so that it can
	// bypass the m_ulConfigured flag. This is reasonable, since In fact
	// the RACH preamble is sent on 6RB bandwidth so the uplink
	// bandwidth does not need to be configured.
	NS_ASSERT (m_subframeNo > 0); // sanity check for subframe starting at 1
	m_raRnti = m_subframeNo - 1;
	m_uePhySapProvider->SendRachPreamble (m_raPreambleId, m_raRnti);
	NS_LOG_INFO (this << " sent preamble id " << (uint32_t) m_raPreambleId << ", RA-RNTI " << (uint32_t) m_raRnti);
	// 3GPP 36.321 5.1.4
	Time raWindowBegin = MilliSeconds (3);
	Time raWindowEnd = MilliSeconds (3 + m_rachConfig.raResponseWindowSize);
	Simulator::Schedule (raWindowBegin, &NistLteUeMac::StartWaitingForRaResponse, this);
	m_noRaResponseReceivedEvent = Simulator::Schedule (raWindowEnd, &NistLteUeMac::RaResponseTimeout, this, contention);
}

void 
NistLteUeMac::StartWaitingForRaResponse ()
{
	NS_LOG_FUNCTION (this);
	m_waitingForRaResponse = true;
}

void 
NistLteUeMac::RecvRaResponse (NistBuildNistRarListElement_s raResponse)
{
	NS_LOG_FUNCTION (this);

	m_waitingForRaResponse = false;
	m_noRaResponseReceivedEvent.Cancel ();
	NS_LOG_INFO ("got RAR for RAPID " << (uint32_t) m_raPreambleId << ", setting T-C-RNTI = " << raResponse.m_rnti);
	m_rnti = raResponse.m_rnti;
	m_cmacSapUser->SetTemporaryCellRnti (m_rnti);
	// in principle we should wait for contention resolution,
	// but in the current LTE model when two or more identical
	// preambles are sent no one is received, so there is no need
	// for contention resolution
	m_cmacSapUser->NotifyRandomAccessSuccessful ();
	// trigger tx opportunity for Message 3 over LC 0
	// this is needed since Message 3's UL GRANT is in the RAR, not in UL-DCIs
	const uint8_t lc0Lcid = 0;
	std::map <uint8_t, NistLcInfo>::iterator lc0InfoIt = m_lcInfoMap.find (lc0Lcid);
	NS_ASSERT (lc0InfoIt != m_lcInfoMap.end ());

	//added
	std::map <uint8_t, std::map <uint8_t, NistLteMacSapProvider::NistReportBufferNistStatusParameters > >:: iterator it;
	it=Nist_m_ulBsrReceived.find(m_rnti);
	if (it!=Nist_m_ulBsrReceived.end())
	{
		std::map <uint8_t, NistLteMacSapProvider::NistReportBufferNistStatusParameters > mapLC=it->second;
		std::map <uint8_t, NistLteMacSapProvider::NistReportBufferNistStatusParameters >::iterator lc0BsrIt;
		lc0BsrIt=mapLC.find(lc0Lcid);

		if ((lc0BsrIt != mapLC.end ())
				&& (lc0BsrIt->second.txQueueSize > 0))
		{
			NS_ASSERT_MSG (raResponse.m_grant.m_tbSize > lc0BsrIt->second.txQueueSize,
					"segmentation of Message 3 is not allowed");
			lc0InfoIt->second.macSapUser->NotifyTxOpportunity (raResponse.m_grant.m_tbSize, 0, 0);
			lc0BsrIt->second.txQueueSize = 0;
		}
	}

	else
	{

		std::map <uint8_t, NistLteMacSapProvider::NistReportBufferNistStatusParameters>::iterator lc0BsrIt= m_ulBsrReceived.find (lc0Lcid);
		if ((lc0BsrIt != m_ulBsrReceived.end ())
				&& (lc0BsrIt->second.txQueueSize > 0))
		{
			NS_ASSERT_MSG (raResponse.m_grant.m_tbSize > lc0BsrIt->second.txQueueSize,
					"segmentation of Message 3 is not allowed");
			lc0InfoIt->second.macSapUser->NotifyTxOpportunity (raResponse.m_grant.m_tbSize, 0, 0);
			lc0BsrIt->second.txQueueSize = 0;
		}
	}
}

void 
NistLteUeMac::RaResponseTimeout (bool contention)
{
	NS_LOG_FUNCTION (this << contention);
	m_waitingForRaResponse = false;
	// 3GPP 36.321 5.1.4
	++m_preambleTransmissionCounter;
	if (m_preambleTransmissionCounter == m_rachConfig.preambleTransMax + 1)
	{
		NS_LOG_INFO ("RAR timeout, preambleTransMax reached => giving up");
		m_cmacSapUser->NotifyRandomAccessFailed ();
	}
	else
	{
		NS_LOG_INFO ("RAR timeout, re-send preamble");
		if (contention)
		{
			RandomlySelectAndSendRaPreamble ();
		}
		else
		{
			SendRaPreamble (contention);
		}
	}
}

void 
NistLteUeMac::DoConfigureRach (NistLteUeCmacSapProvider::NistRachConfig rc)
{
	NS_LOG_FUNCTION (this);
	m_rachConfig = rc;
	m_rachConfigured = true;
}

void 
NistLteUeMac::DoStartContentionBasedRandomAccessProcedure ()
{
	NS_LOG_FUNCTION (this);

	// 3GPP 36.321 5.1.1
	NS_ASSERT_MSG (m_rachConfigured, "RACH not configured");
	m_preambleTransmissionCounter = 0;
	m_backoffParameter = 0;
	RandomlySelectAndSendRaPreamble ();
}

void 
NistLteUeMac::DoStartNonContentionBasedRandomAccessProcedure (uint16_t rnti, uint8_t preambleId, uint8_t prachMask)
{
	NS_LOG_FUNCTION (this << " rnti" << rnti);
	NS_ASSERT_MSG (prachMask == 0, "requested PRACH MASK = " << (uint32_t) prachMask << ", but only PRACH MASK = 0 is supported");
	m_rnti = rnti;
	m_raPreambleId = preambleId;
	bool contention = false;
	SendRaPreamble (contention);
}

void
NistLteUeMac::DoAddLc (uint8_t lcId,  NistLteUeCmacSapProvider::NistLogicalChannelConfig lcConfig, NistLteMacSapUser* msu)
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
NistLteUeMac::DoAddLCPriority ( uint8_t rnti, uint8_t lcid, uint8_t priority)
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
NistLteUeMac::DoAddLc (uint8_t lcId, uint32_t srcL2Id, uint32_t dstL2Id, NistLteUeCmacSapProvider::NistLogicalChannelConfig lcConfig, NistLteMacSapUser* msu)
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
NistLteUeMac::DoRemoveLc (uint8_t lcId)
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
NistLteUeMac::DoRemoveLc (uint8_t lcId, uint32_t srcL2Id, uint32_t dstL2Id)
{
	NS_LOG_FUNCTION (this << " lcId" << lcId << ", srcL2Id=" << srcL2Id << ", dstL2Id" << dstL2Id);
	//    NS_ASSERT_MSG (m_lcInfoMap.find (lcId) != m_lcInfoMap.end (), "could not find LCID " << lcId);
	//    m_lcInfoMap.erase (lcId);
}

void
NistLteUeMac::DoReset ()
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
NistLteUeMac::DoAddSlTxPool (Ptr<SidelinkTxDiscResourcePool> pool)
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
NistLteUeMac::DoRemoveSlTxPool ()
{
  m_discTxPools.m_pool = NULL;
}

void
NistLteUeMac::DoSetSlRxPools (std::list<Ptr<SidelinkRxDiscResourcePool> > pools)
{
  m_discRxPools = pools;
}

void 
NistLteUeMac::DoModifyDiscTxApps (std::list<uint32_t> apps)
{
  m_discTxApps = apps;
  m_cphySapProvider->AddDiscTxApps (apps);
}

void 
NistLteUeMac::DoModifyDiscRxApps (std::list<uint32_t> apps)
{
  m_discRxApps = apps;
  m_cphySapProvider->AddDiscRxApps (apps);
}

void
NistLteUeMac::DoAddSlTxPool (uint32_t dstL2Id, Ptr<SidelinkTxCommResourcePool> pool)
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
NistLteUeMac::DoRemoveSlTxPool (uint32_t dstL2Id)
{
	std::map <uint32_t, PoolInfo >::iterator it;
	it = m_sidelinkTxPoolsMap.find (dstL2Id);
	NS_ASSERT_MSG (it != m_sidelinkTxPoolsMap.end (), "Cannot remove sidelink transmission pool for " << dstL2Id << ". Unknown destination");
	m_sidelinkTxPoolsMap.erase (dstL2Id);
}


void
NistLteUeMac::DoSetSlRxPools (std::list<Ptr<SidelinkRxCommResourcePool> > pools)
{
	m_sidelinkRxPools = pools;
}

void
NistLteUeMac::DoSetRnti (uint16_t rnti)
{
	NS_LOG_FUNCTION (this << rnti);
	NS_ASSERT_MSG (m_rnti == 0, "Cannot manually set RNTI if already configured");
	m_rnti = rnti;
}

void
NistLteUeMac::DoReceivePhyPdu (Ptr<Packet> p)
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
                      errorFile.open("results/sidelink/tbRxFile.csv", std::ios_base::app);
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
}


void
NistLteUeMac::DoReceiveNistLteControlMessage (Ptr<NistLteControlMessage> msg)
{
	if ( m_NistUlScheduler == "ns3::NistPfFfMacScheduler")
	{
		DoReceiveNistPFLteControlMessage (msg);
	}
	else if ( m_NistUlScheduler == "ns3::NistMtFfMacScheduler" )
	{
		DoReceiveNistMTLteControlMessage (msg);
	}
	else if ( m_NistUlScheduler == "ns3::NistPriorityFfMacScheduler" )
	{
		DoReceiveNistPrLteControlMessage (msg);
	}
	else if (m_NistUlScheduler == "ns3::NistRrSlFfMacScheduler")
	{
		DoReceiveNistRrLteControlMessage (msg);
	}
	else
	{
		DoReceiveNistRrLteControlMessage (msg);
		//std::cout<<" UE RR SCHEDULER "<<std::endl;
	}
}

void
NistLteUeMac::DoReceiveNistRrLteControlMessage (Ptr<NistLteControlMessage> msg)
{
	//std::cout<<" ENTER DoReceiveNistRrLteControlMessage "<<std::endl;
	NS_LOG_FUNCTION (this);
	if (msg->GetMessageType () == NistLteControlMessage::UL_DCI)
	{
		Ptr<NistUlDciLteControlMessage> msg2 = DynamicCast<NistUlDciLteControlMessage> (msg);
		NistUlDciListElement_s dci = msg2->GetDci ();
		if (dci.m_ndi == 1)
		{
			// New transmission -> emtpy pkt buffer queue (for deleting eventual pkts not acked )
			Ptr<PacketBurst> pb = CreateObject <PacketBurst> ();
			m_miUlHarqProcessesPacket.at (m_harqProcessId) = pb;
			// Retrieve data from RLC
			std::map <uint8_t, NistLteMacSapProvider::NistReportBufferNistStatusParameters>::iterator itBsr;
			uint16_t activeLcs = 0;
			uint32_t statusPduMinSize = 0;
			//added code
			if (m_ulBsrReceived.size()!=0)
			{
				for (itBsr = m_ulBsrReceived.begin (); itBsr != m_ulBsrReceived.end (); itBsr++)
				{
					if (((*itBsr).second.statusPduSize > 0) || ((*itBsr).second.retxQueueSize > 0) || ((*itBsr).second.txQueueSize > 0))
					{
						activeLcs++;
						if (((*itBsr).second.statusPduSize != 0)&&((*itBsr).second.statusPduSize < statusPduMinSize))
						{
							statusPduMinSize = (*itBsr).second.statusPduSize;
						}
						if (((*itBsr).second.statusPduSize != 0)&&(statusPduMinSize == 0))
						{
							statusPduMinSize = (*itBsr).second.statusPduSize;
						}
					}
				}
				if (activeLcs == 0)
				{
					NS_LOG_ERROR (this << " No active flows for this UL-DCI");
					return;
				}
				std::map <uint8_t, NistLcInfo>::iterator it;
				uint32_t bytesPerActiveLc = dci.m_tbSize / activeLcs;
				bool statusPduPriority = false;
				if ((statusPduMinSize != 0)&&(bytesPerActiveLc < statusPduMinSize))
				{
					// send only the status PDU which has highest priority
					statusPduPriority = true;
					NS_LOG_DEBUG (this << " Reduced resource -> send only NistStatus, b ytes " << statusPduMinSize);
					if (dci.m_tbSize < statusPduMinSize)
					{
						NS_FATAL_ERROR ("Insufficient Tx Opportunity for sending a status message");
					}
				}
				NS_LOG_LOGIC (this << " UE " << m_rnti << ": UL-CQI notified TxOpportunity of " << dci.m_tbSize << " => " << bytesPerActiveLc << " bytes per active LC" << " statusPduMinSize " << statusPduMinSize);
				for (it = m_lcInfoMap.begin (); it != m_lcInfoMap.end (); it++)
				{
					itBsr = m_ulBsrReceived.find ((*it).first);
					NS_LOG_DEBUG (this << " Processing LC " << (uint32_t)(*it).first << " bytesPerActiveLc " << bytesPerActiveLc);
					if ( (itBsr != m_ulBsrReceived.end ())
							&& ( ((*itBsr).second.statusPduSize > 0)
									|| ((*itBsr).second.retxQueueSize > 0)
									|| ((*itBsr).second.txQueueSize > 0)) )
					{
						if ((statusPduPriority) && ((*itBsr).second.statusPduSize == statusPduMinSize))
						{
							(*it).second.macSapUser->NotifyTxOpportunity ((*itBsr).second.statusPduSize, 0, 0);
							NS_LOG_LOGIC (this << "\t" << bytesPerActiveLc << " send  " << (*itBsr).second.statusPduSize << " status bytes to LC " << (uint32_t)(*it).first << " statusQueue " << (*itBsr).second.statusPduSize << " retxQueue" << (*itBsr).second.retxQueueSize << " txQueue" <<  (*itBsr).second.txQueueSize);
							(*itBsr).second.statusPduSize = 0;
							break;
						}
						else
						{
							uint32_t bytesForThisLc = bytesPerActiveLc;
							NS_LOG_LOGIC (this << "\t" << bytesPerActiveLc << " bytes to LC " << (uint32_t)(*it).first << " statusQueue " << (*itBsr).second.statusPduSize << " retxQueue" << (*itBsr).second.retxQueueSize << " txQueue" <<  (*itBsr).second.txQueueSize);
							if (((*itBsr).second.statusPduSize > 0) && (bytesForThisLc > (*itBsr).second.statusPduSize))
							{
								(*it).second.macSapUser->NotifyTxOpportunity ((*itBsr).second.statusPduSize, 0, 0);
								bytesForThisLc -= (*itBsr).second.statusPduSize;
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
									(*it).second.macSapUser->NotifyTxOpportunity (bytesForThisLc, 0, 0);
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
									uint16_t lcid = (*it).first;
									uint32_t rlcOverhead;
									if (lcid == 1)
									{
										// for SRB1 (using RLC AM) it's better to
										// overestimate RLC overhead rather than
										// underestimate it and risk unneeded
										// segmentation which increases delay
										rlcOverhead = 4;
									}
									else
									{
										// minimum RLC overhead due to header
										rlcOverhead = 2;
									}
									NS_LOG_DEBUG (this << " serve tx DATA, bytes " << bytesForThisLc << ", RLC overhead " << rlcOverhead);
									(*it).second.macSapUser->NotifyTxOpportunity (bytesForThisLc, 0, 0);
									if ((*itBsr).second.txQueueSize >= bytesForThisLc - rlcOverhead)
									{
										(*itBsr).second.txQueueSize -= bytesForThisLc - rlcOverhead;
									}
									else
									{
										(*itBsr).second.txQueueSize = 0;
									}
								}
							}
							else
							{
								if ( ((*itBsr).second.retxQueueSize > 0) || ((*itBsr).second.txQueueSize > 0))
								{
									// resend BSR info for updating eNB peer MAC
									m_freshUlBsr = true;
								}
							}
							NS_LOG_LOGIC (this << "\t" << bytesPerActiveLc << "\t new queues " << (uint32_t)(*it).first << " statusQueue " << (*itBsr).second.statusPduSize << " retxQueue" << (*itBsr).second.retxQueueSize << " txQueue" <<  (*itBsr).second.txQueueSize);
						}

					}
				}
			} // end if m_ulBsrReceived.size()!=0
			else
			{
				// NIST IMPLEMENTATION OF Nist_m_ulBsrReceived
				uint16_t activeLcs = 0;
				uint32_t statusPduMinSize = 0;
				std::map <uint8_t, std::map <uint8_t, NistLteMacSapProvider::NistReportBufferNistStatusParameters > >:: iterator it;
				it=Nist_m_ulBsrReceived.find(m_rnti);
				if (it!=Nist_m_ulBsrReceived.end())
				{
					// Retrieve data from RLC
					std::map <uint8_t, NistLteMacSapProvider::NistReportBufferNistStatusParameters > mapLC=it->second;
					std::map <uint8_t, NistLteMacSapProvider::NistReportBufferNistStatusParameters>::iterator itBsr;

					for (itBsr = mapLC.begin (); itBsr != mapLC.end (); itBsr++)
					{
						if (((*itBsr).second.statusPduSize > 0) || ((*itBsr).second.retxQueueSize > 0) || ((*itBsr).second.txQueueSize > 0))
						{
							activeLcs++;
							if (((*itBsr).second.statusPduSize!=0)&&((*itBsr).second.statusPduSize < statusPduMinSize))
							{
								statusPduMinSize = (*itBsr).second.statusPduSize;
							}
							if (((*itBsr).second.statusPduSize!=0)&&(statusPduMinSize == 0))
							{
								statusPduMinSize = (*itBsr).second.statusPduSize;
							}
						}// end if
					}// end for

					if (activeLcs == 0)
					{
						NS_LOG_ERROR (this << " No active flows for this UL-DCI");
						return;
					}
					for (itBsr = mapLC.begin (); itBsr != mapLC.end (); itBsr++)
					{
						// compute tb size for this lc
						uint32_t bytesPerActiveLc= dci.m_tbSize / activeLcs ;
						std::map <uint8_t, NistLcInfo>::iterator itLcInfo;
						bool statusPduPriority = false;
						if ((statusPduMinSize != 0)&&(bytesPerActiveLc < statusPduMinSize))
						{
							// send only the status PDU which has highest priority
							statusPduPriority = true;
							NS_LOG_DEBUG (this << " Reduced resource -> send only Status, bytes " << statusPduMinSize);
							if (dci.m_tbSize < statusPduMinSize)
							{
								NS_FATAL_ERROR ("Insufficient Tx Opportunity for sending a status message");
							}
						}
						NS_LOG_LOGIC (this << " UE " << m_rnti << ": UL-CQI notified TxOpportunity of " << dci.m_tbSize << " => " << bytesPerActiveLc << " bytes per active LC" << " statusPduMinSize " << statusPduMinSize);

						if ( (itBsr!=mapLC.end ()) &&
								( ((*itBsr).second.statusPduSize > 0) ||
										((*itBsr).second.retxQueueSize > 0) ||
										((*itBsr).second.txQueueSize > 0)) )
						{
							itLcInfo=m_lcInfoMap.find((*itBsr).first);

							if ((statusPduPriority) && ((*itBsr).second.statusPduSize == statusPduMinSize))
							{

								(*itLcInfo).second.macSapUser->NotifyTxOpportunity ((*itBsr).second.statusPduSize, 0, 0);

								(*itBsr).second.statusPduSize = 0;
							}
							else
							{

								uint32_t bytesForThisLc = bytesPerActiveLc;
								NS_LOG_LOGIC (this << "\t" << bytesPerActiveLc << " bytes to LC " << (uint32_t)(*itLcInfo).first << " statusQueue " << (*itBsr).second.statusPduSize << " retxQueue" << (*itBsr).second.retxQueueSize << " txQueue" <<  (*itBsr).second.txQueueSize);
								if (((*itBsr).second.statusPduSize > 0) && (bytesForThisLc > (*itBsr).second.statusPduSize))
								{
									(*itLcInfo).second.macSapUser->NotifyTxOpportunity ((*itBsr).second.statusPduSize, 0, 0);
									bytesForThisLc -= (*itBsr).second.statusPduSize;
									NS_LOG_DEBUG (this << " serve STATUS " << (*itBsr).second.statusPduSize);
									(*itBsr).second.statusPduSize = 0;

								}
								else
								{
									if ((*itBsr).second.statusPduSize>bytesForThisLc)
									{
										NS_FATAL_ERROR ("Insufficient Tx Opportunity for sending a status message");
									}
								}

								if ((bytesForThisLc > 7) && // 7 is the min TxOpportunity useful for Rlc
										(((*itBsr).second.retxQueueSize > 0) ||
												((*itBsr).second.txQueueSize > 0)))
								{
									if ((*itBsr).second.retxQueueSize > 0)
									{
										NS_LOG_DEBUG (this << " serve retx DATA, bytes " << bytesForThisLc);

										(*itLcInfo).second.macSapUser->NotifyTxOpportunity (bytesForThisLc, 0, 0);
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
										uint16_t lcid = (*itLcInfo).first;
										uint32_t rlcOverhead;
										if (lcid == 1)
										{
											// for SRB1 (using RLC AM) it's better to
											// overestimate RLC overhead rather than
											// underestimate it and risk unneeded
											// segmentation which increases delay
											rlcOverhead = 4;
										}
										else
										{
											// minimum RLC overhead due to header
											rlcOverhead = 2;
										}
										NS_LOG_DEBUG (this << " serve tx DATA, bytes " << bytesForThisLc << ", RLC overhead " << rlcOverhead);
										(*itLcInfo).second.macSapUser->NotifyTxOpportunity (bytesForThisLc, 0, 0);
										if ((*itBsr).second.txQueueSize >= bytesForThisLc - rlcOverhead)
										{
											(*itBsr).second.txQueueSize -= bytesForThisLc - rlcOverhead;

										}
										else
										{
											(*itBsr).second.txQueueSize = 0;

										}
									}
								}
								else
								{
									if ( ((*itBsr).second.retxQueueSize > 0) || ((*itBsr).second.txQueueSize > 0))
									{
										// resend BSR info for updating eNB peer MAC
										m_freshUlBsr = true;
									}
								}
								NS_LOG_LOGIC (this << "\t" << bytesPerActiveLc << "\t new queues " << (uint32_t)(*itLcInfo).first << " statusQueue " << (*itBsr).second.statusPduSize << " retxQueue" << (*itBsr).second.retxQueueSize << " txQueue" <<  (*itBsr).second.txQueueSize);
							}

						}
					}
				}// end for p
			} // end it!=Nist_m_ulBsrReceived.end()

		}
		else
		{
			// HARQ retransmission -> retrieve data from HARQ buffer
			NS_LOG_DEBUG (this << " UE MAC RETX HARQ " << (uint16_t)m_harqProcessId);
			Ptr<PacketBurst> pb = m_miUlHarqProcessesPacket.at (m_harqProcessId);
			for (std::list<Ptr<Packet> >::const_iterator j = pb->Begin (); j != pb->End (); ++j)
			{
				Ptr<Packet> pkt = (*j)->Copy ();
				m_uePhySapProvider->SendMacPdu (pkt);
			}
			m_miUlHarqProcessesPacketTimer.at (m_harqProcessId) = HARQ_PERIOD;
		}

	}
	else if (msg->GetMessageType () == NistLteControlMessage::RAR)
	{
		if (m_waitingForRaResponse)
		{
			Ptr<NistRarLteControlMessage> rarMsg = DynamicCast<NistRarLteControlMessage> (msg);
			uint16_t raRnti = rarMsg->GetRaRnti ();
			NS_LOG_LOGIC (this << "got RAR with RA-RNTI " << (uint32_t) raRnti << ", expecting " << (uint32_t) m_raRnti);
			if (raRnti == m_raRnti) // RAR corresponds to TX subframe of preamble
			{
				for (std::list<NistRarLteControlMessage::NistRar>::const_iterator it = rarMsg->NistRarListBegin ();
						it != rarMsg->NistRarListEnd ();
						++it)
				{
					if (it->rapId == m_raPreambleId) // RAR is for me
					{
						RecvRaResponse (it->rarPayload);
						/// \todo RRC generates the RecvRaResponse messaged
						/// for avoiding holes in transmission at PHY layer
						/// (which produce erroneous UL CQI evaluation)
					}
				}
			}
		}
	}
	else if (msg->GetMessageType () == NistLteControlMessage::SL_DCI)
	{
		Ptr<NistSlDciLteControlMessage> msg2 = DynamicCast<NistSlDciLteControlMessage> (msg);
		NistSlDciListElement_s dci = msg2->GetDci ();

		//store the grant for the next SC period
		//TODO: distinguish SL grants for different pools. Right now just assume there is only one pool
		Ptr<SidelinkTxCommResourcePool> pool = DynamicCast<SidelinkTxCommResourcePool> (m_sidelinkTxPoolsMap.begin()->second.m_pool);
		NS_ASSERT (pool->GetSchedulingType() == SidelinkCommResourcePool::SCHEDULED);

		SidelinkGrant grant;
		grant.m_resPscch = dci.m_resPscch;
		grant.m_tpc = dci.m_tpc;
		grant.m_hopping = dci.m_hopping;
		grant.m_rbStart = dci.m_rbStart;
		grant.m_rbLen = dci.m_rbLen;
		grant.m_trp = dci.m_trp;
		grant.m_mcs = pool->GetMcs();
		grant.m_tbSize = 0; //computed later
		m_sidelinkTxPoolsMap.begin()->second.m_nextGrant = grant;
		m_sidelinkTxPoolsMap.begin()->second.m_grant_received = true;

                // trace File 
                std::ofstream peekFile;
                peekFile.open("results/sidelink/mode1.unimo", std::ios_base::app);
                peekFile << "resPscch=" << dci.m_resPscch <<", tpc" << dci.m_tpc << "\r\n";
                peekFile.close();

		NS_LOG_INFO (this << "Received SL_DCI message rnti=" << m_rnti << " res=" << (uint16_t) dci.m_resPscch);
	}
  else if (msg->GetMessageType () == NistLteControlMessage::SL_DISC_MSG)
  {
    NS_LOG_INFO (this << " Received discovery message");
    //notify RRC (pass msg to RRC where we can filter)
    m_cmacSapUser->NotifyDiscoveryReception (msg);
  }

	else
	{
		NS_LOG_WARN (this << " NistLteControlMessage not recognized");
	}
}

void
NistLteUeMac::DoReceiveNistPFLteControlMessage (Ptr<NistLteControlMessage> msg)
{

	NS_LOG_FUNCTION (this);
	if (msg->GetMessageType () == NistLteControlMessage::UL_DCI)
	{
		Ptr<NistUlDciLteControlMessage> msg2 = DynamicCast<NistUlDciLteControlMessage> (msg);
		NistUlDciListElement_s dci = msg2->GetDci ();
		if (dci.m_ndi==1)
		{
			// New transmission -> emtpy pkt buffer queue (for deleting eventual pkts not acked )
			Ptr<PacketBurst> pb = CreateObject <PacketBurst> ();
			m_miUlHarqProcessesPacket.at (m_harqProcessId) = pb;
			// Retrieve data from RLC
			std::map <uint8_t, NistLteMacSapProvider::NistReportBufferNistStatusParameters>::iterator itBsr;
			uint16_t activeLcs = 0;
			uint32_t statusPduMinSize = 0;
			//added code
			if (m_ulBsrReceived.size()!=0)
			{
				for (itBsr = m_ulBsrReceived.begin (); itBsr != m_ulBsrReceived.end (); itBsr++)
				{
					if (((*itBsr).second.statusPduSize > 0) || ((*itBsr).second.retxQueueSize > 0) || ((*itBsr).second.txQueueSize > 0))
					{
						activeLcs++;
						if (((*itBsr).second.statusPduSize!=0)&&((*itBsr).second.statusPduSize < statusPduMinSize))
						{
							statusPduMinSize = (*itBsr).second.statusPduSize;
						}
						if (((*itBsr).second.statusPduSize!=0)&&(statusPduMinSize == 0))
						{
							statusPduMinSize = (*itBsr).second.statusPduSize;
						}
					}
				}
				if (activeLcs == 0)
				{
					NS_LOG_ERROR (this << " No active flows for this UL-DCI");
					return;
				}
				std::map <uint8_t, NistLcInfo>::iterator it;
				uint32_t bytesPerActiveLc = dci.m_tbSize / activeLcs;
				bool statusPduPriority = false;
				if ((statusPduMinSize != 0)&&(bytesPerActiveLc < statusPduMinSize))
				{
					// send only the status PDU which has highest priority
					statusPduPriority = true;
					NS_LOG_DEBUG (this << " Reduced resource -> send only Status, b ytes " << statusPduMinSize);
					if (dci.m_tbSize < statusPduMinSize)
					{
						NS_FATAL_ERROR ("Insufficient Tx Opportunity for sending a status message");
					}
				}
				NS_LOG_LOGIC (this << " UE " << m_rnti << ": UL-CQI notified TxOpportunity of " << dci.m_tbSize << " => " << bytesPerActiveLc << " bytes per active LC" << " statusPduMinSize " << statusPduMinSize);
				for (it = m_lcInfoMap.begin (); it!=m_lcInfoMap.end (); it++)
				{
					itBsr = m_ulBsrReceived.find ((*it).first);
					NS_LOG_DEBUG (this << " Processing LC " << (uint32_t)(*it).first << " bytesPerActiveLc " << bytesPerActiveLc);
					if ( (itBsr!=m_ulBsrReceived.end ()) &&
							( ((*itBsr).second.statusPduSize > 0) ||
									((*itBsr).second.retxQueueSize > 0) ||
									((*itBsr).second.txQueueSize > 0)) )
					{
						if ((statusPduPriority) && ((*itBsr).second.statusPduSize == statusPduMinSize))
						{
							(*it).second.macSapUser->NotifyTxOpportunity ((*itBsr).second.statusPduSize, 0, 0);
							NS_LOG_LOGIC (this << "\t" << bytesPerActiveLc << " send  " << (*itBsr).second.statusPduSize << " status bytes to LC " << (uint32_t)(*it).first << " statusQueue " << (*itBsr).second.statusPduSize << " retxQueue" << (*itBsr).second.retxQueueSize << " txQueue" <<  (*itBsr).second.txQueueSize);
							(*itBsr).second.statusPduSize = 0;
							break;
						}
						else
						{
							uint32_t bytesForThisLc = bytesPerActiveLc;
							NS_LOG_LOGIC (this << "\t" << bytesPerActiveLc << " bytes to LC " << (uint32_t)(*it).first << " statusQueue " << (*itBsr).second.statusPduSize << " retxQueue" << (*itBsr).second.retxQueueSize << " txQueue" <<  (*itBsr).second.txQueueSize);
							if (((*itBsr).second.statusPduSize > 0) && (bytesForThisLc > (*itBsr).second.statusPduSize))
							{
								(*it).second.macSapUser->NotifyTxOpportunity ((*itBsr).second.statusPduSize, 0, 0);
								bytesForThisLc -= (*itBsr).second.statusPduSize;
								NS_LOG_DEBUG (this << " serve STATUS " << (*itBsr).second.statusPduSize);
								(*itBsr).second.statusPduSize = 0;
							}
							else
							{
								if ((*itBsr).second.statusPduSize>bytesForThisLc)
								{
									NS_FATAL_ERROR ("Insufficient Tx Opportunity for sending a status message");
								}
							}

							if ((bytesForThisLc > 7) && // 7 is the min TxOpportunity useful for Rlc
									(((*itBsr).second.retxQueueSize > 0) ||
											((*itBsr).second.txQueueSize > 0)))
							{
								if ((*itBsr).second.retxQueueSize > 0)
								{
									NS_LOG_DEBUG (this << " serve retx DATA, bytes " << bytesForThisLc);
									(*it).second.macSapUser->NotifyTxOpportunity (bytesForThisLc, 0, 0);
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
									uint16_t lcid = (*it).first;
									uint32_t rlcOverhead;
									if (lcid == 1)
									{
										// for SRB1 (using RLC AM) it's better to
										// overestimate RLC overhead rather than
										// underestimate it and risk unneeded
										// segmentation which increases delay
										rlcOverhead = 4;
									}
									else
									{
										// minimum RLC overhead due to header
										rlcOverhead = 2;
									}
									NS_LOG_DEBUG (this << " serve tx DATA, bytes " << bytesForThisLc << ", RLC overhead " << rlcOverhead);
									(*it).second.macSapUser->NotifyTxOpportunity (bytesForThisLc, 0, 0);
									if ((*itBsr).second.txQueueSize >= bytesForThisLc - rlcOverhead)
									{
										(*itBsr).second.txQueueSize -= bytesForThisLc - rlcOverhead;
									}
									else
									{
										(*itBsr).second.txQueueSize = 0;
									}
								}
							}
							else
							{
								if ( ((*itBsr).second.retxQueueSize > 0) || ((*itBsr).second.txQueueSize > 0))
								{
									// resend BSR info for updating eNB peer MAC
									m_freshUlBsr = true;
								}
							}
							NS_LOG_LOGIC (this << "\t" << bytesPerActiveLc << "\t new queues " << (uint32_t)(*it).first << " statusQueue " << (*itBsr).second.statusPduSize << " retxQueue" << (*itBsr).second.retxQueueSize << " txQueue" <<  (*itBsr).second.txQueueSize);
						}

					}
				}
			} // end if m_ulBsrReceived.size()!=0
			else
			{
				// NIST IMPLEMENTATION OF Nist_m_ulBsrReceived
				uint16_t activeLcs = 0;
				uint32_t statusPduMinSize = 0;
				std::map <uint8_t, std::map <uint8_t, NistLteMacSapProvider::NistReportBufferNistStatusParameters > >:: iterator it;
				it=Nist_m_ulBsrReceived.find(m_rnti);
				if (it!=Nist_m_ulBsrReceived.end())
				{
					// Retrieve data from RLC
					std::map <uint8_t, NistLteMacSapProvider::NistReportBufferNistStatusParameters > mapLC=it->second;
					std::map <uint8_t, NistLteMacSapProvider::NistReportBufferNistStatusParameters>::iterator itBsr;

					for (itBsr = mapLC.begin (); itBsr != mapLC.end (); itBsr++)
					{
						if (((*itBsr).second.statusPduSize > 0) || ((*itBsr).second.retxQueueSize > 0) || ((*itBsr).second.txQueueSize > 0))
						{
							activeLcs++;
							if (((*itBsr).second.statusPduSize!=0)&&((*itBsr).second.statusPduSize < statusPduMinSize))
							{
								statusPduMinSize = (*itBsr).second.statusPduSize;
							}
							if (((*itBsr).second.statusPduSize!=0)&&(statusPduMinSize == 0))
							{
								statusPduMinSize = (*itBsr).second.statusPduSize;
							}
						}// end if
					}// end for

					if (activeLcs == 0)
					{
						NS_LOG_ERROR (this << " No active flows for this UL-DCI");
						return;
					}

					uint32_t totalQueue =0;
					for (itBsr = mapLC.begin (); itBsr != mapLC.end (); itBsr++)
					{
						totalQueue=totalQueue + (*itBsr).second.txQueueSize;
					}
					for (itBsr = mapLC.begin (); itBsr != mapLC.end (); itBsr++)
					{
						// compute tb size for this lc
						uint32_t bytesPerActiveLc= dci.m_tbSize / activeLcs ;
						if (totalQueue > dci.m_tbSize)
						{
							double pfCoef= (double) (*itBsr).second.txQueueSize / (double)totalQueue;
							bytesPerActiveLc = floor (pfCoef * (double)dci.m_tbSize);
						}
						std::map <uint8_t, NistLcInfo>::iterator itLcInfo;
						bool statusPduPriority = false;
						if ((statusPduMinSize != 0)&&(bytesPerActiveLc < statusPduMinSize))
						{
							// send only the status PDU which has highest priority
							statusPduPriority = true;
							NS_LOG_DEBUG (this << " Reduced resource -> send only Status, b ytes " << statusPduMinSize);
							if (dci.m_tbSize < statusPduMinSize)
							{
								NS_FATAL_ERROR ("Insufficient Tx Opportunity for sending a status message");
							}
						}
						NS_LOG_LOGIC (this << " UE " << m_rnti << ": UL-CQI notified TxOpportunity of " << dci.m_tbSize << " => " << bytesPerActiveLc << " bytes per active LC" << " statusPduMinSize " << statusPduMinSize);

						if ( (itBsr!=mapLC.end ()) &&
								( ((*itBsr).second.statusPduSize > 0) ||
										((*itBsr).second.retxQueueSize > 0) ||
										((*itBsr).second.txQueueSize > 0)) )
						{
							itLcInfo=m_lcInfoMap.find((*itBsr).first);

							if ((statusPduPriority) && ((*itBsr).second.statusPduSize == statusPduMinSize))
							{

								(*itLcInfo).second.macSapUser->NotifyTxOpportunity ((*itBsr).second.statusPduSize, 0, 0);

								(*itBsr).second.statusPduSize = 0;
							}
							else
							{

								uint32_t bytesForThisLc = bytesPerActiveLc;
								NS_LOG_LOGIC (this << "\t" << bytesPerActiveLc << " bytes to LC " << (uint32_t)(*itLcInfo).first << " statusQueue " << (*itBsr).second.statusPduSize << " retxQueue" << (*itBsr).second.retxQueueSize << " txQueue" <<  (*itBsr).second.txQueueSize);
								if (((*itBsr).second.statusPduSize > 0) && (bytesForThisLc > (*itBsr).second.statusPduSize))
								{
									(*itLcInfo).second.macSapUser->NotifyTxOpportunity ((*itBsr).second.statusPduSize, 0, 0);
									bytesForThisLc -= (*itBsr).second.statusPduSize;
									NS_LOG_DEBUG (this << " serve STATUS " << (*itBsr).second.statusPduSize);
									(*itBsr).second.statusPduSize = 0;

								}
								else
								{
									if ((*itBsr).second.statusPduSize>bytesForThisLc)
									{
										NS_FATAL_ERROR ("Insufficient Tx Opportunity for sending a status message");
									}
								}

								if ((bytesForThisLc > 7) && // 7 is the min TxOpportunity useful for Rlc
										(((*itBsr).second.retxQueueSize > 0) ||
												((*itBsr).second.txQueueSize > 0)))
								{
									if ((*itBsr).second.retxQueueSize > 0)
									{
										NS_LOG_DEBUG (this << " serve retx DATA, bytes " << bytesForThisLc);

										(*itLcInfo).second.macSapUser->NotifyTxOpportunity (bytesForThisLc, 0, 0);
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
										uint16_t lcid = (*itLcInfo).first;
										uint32_t rlcOverhead;
										if (lcid == 1)
										{
											// for SRB1 (using RLC AM) it's better to
											// overestimate RLC overhead rather than
											// underestimate it and risk unneeded
											// segmentation which increases delay
											rlcOverhead = 4;
										}
										else
										{
											// minimum RLC overhead due to header
											rlcOverhead = 2;
										}
										NS_LOG_DEBUG (this << " serve tx DATA, bytes " << bytesForThisLc << ", RLC overhead " << rlcOverhead);
										(*itLcInfo).second.macSapUser->NotifyTxOpportunity (bytesForThisLc, 0, 0);
										if ((*itBsr).second.txQueueSize >= bytesForThisLc - rlcOverhead)
										{
											(*itBsr).second.txQueueSize -= bytesForThisLc - rlcOverhead;

										}
										else
										{
											(*itBsr).second.txQueueSize = 0;

										}
									}
								}
								else
								{
									if ( ((*itBsr).second.retxQueueSize > 0) || ((*itBsr).second.txQueueSize > 0))
									{
										// resend BSR info for updating eNB peer MAC
										m_freshUlBsr = true;
									}
								}
								NS_LOG_LOGIC (this << "\t" << bytesPerActiveLc << "\t new queues " << (uint32_t)(*itLcInfo).first << " statusQueue " << (*itBsr).second.statusPduSize << " retxQueue" << (*itBsr).second.retxQueueSize << " txQueue" <<  (*itBsr).second.txQueueSize);
							}

						}
					}
				}// end for p
			} // end it!=Nist_m_ulBsrReceived.end()
		} //end if ndi
		else  //else ndi
		{
			// HARQ retransmission -> retrieve data from HARQ buffer
			NS_LOG_DEBUG (this << " UE MAC RETX HARQ " << (uint16_t)m_harqProcessId);
			Ptr<PacketBurst> pb = m_miUlHarqProcessesPacket.at (m_harqProcessId);
			for (std::list<Ptr<Packet> >::const_iterator j = pb->Begin (); j != pb->End (); ++j)
			{
				Ptr<Packet> pkt = (*j)->Copy ();
				m_uePhySapProvider->SendMacPdu (pkt);
			}
			m_miUlHarqProcessesPacketTimer.at (m_harqProcessId) = HARQ_PERIOD;
		}

	}
	else if (msg->GetMessageType () == NistLteControlMessage::RAR)
	{
		if (m_waitingForRaResponse)
		{
			Ptr<NistRarLteControlMessage> rarMsg = DynamicCast<NistRarLteControlMessage> (msg);
			uint16_t raRnti = rarMsg->GetRaRnti ();
			NS_LOG_LOGIC (this << "got RAR with RA-RNTI " << (uint32_t) raRnti << ", expecting " << (uint32_t) m_raRnti);
			if (raRnti == m_raRnti) // RAR corresponds to TX subframe of preamble
			{
				for (std::list<NistRarLteControlMessage::NistRar>::const_iterator it = rarMsg->NistRarListBegin ();
						it != rarMsg->NistRarListEnd ();
						++it)
				{
					if (it->rapId == m_raPreambleId) // RAR is for me
					{
						RecvRaResponse (it->rarPayload);
						/// \todo RRC generates the RecvRaResponse messaged
						/// for avoiding holes in transmission at PHY layer
						/// (which produce erroneous UL CQI evaluation)
					}
				}
			}
		}
	}
	else if (msg->GetMessageType () == NistLteControlMessage::SL_DCI)
	{
		Ptr<NistSlDciLteControlMessage> msg2 = DynamicCast<NistSlDciLteControlMessage> (msg);
		NistSlDciListElement_s dci = msg2->GetDci ();

		//store the grant for the next SC period
		//TODO: distinguish SL grants for different pools. Right now just assume there is only one pool
		Ptr<SidelinkTxCommResourcePool> pool = DynamicCast<SidelinkTxCommResourcePool> (m_sidelinkTxPoolsMap.begin()->second.m_pool);
		NS_ASSERT (pool->GetSchedulingType() == SidelinkCommResourcePool::SCHEDULED);

		SidelinkGrant grant;
		grant.m_resPscch = dci.m_resPscch;
		grant.m_tpc = dci.m_tpc;
		grant.m_hopping = dci.m_hopping;
		grant.m_rbStart = dci.m_rbStart;
		grant.m_rbLen = dci.m_rbLen;
		grant.m_trp = dci.m_trp;
		grant.m_mcs = pool->GetMcs();
		grant.m_tbSize = 0; //computed later
		m_sidelinkTxPoolsMap.begin()->second.m_nextGrant = grant;
		m_sidelinkTxPoolsMap.begin()->second.m_grant_received = true;

		NS_LOG_INFO (this << "Received SL_DCI message rnti=" << m_rnti << " res=" << (uint16_t) dci.m_resPscch);
	}

 else if (msg->GetMessageType () == NistLteControlMessage::SL_DISC_MSG)
  {
    NS_LOG_INFO (this << " Received discovery message");
    //notify RRC (pass msg to RRC where we can filter)
    m_cmacSapUser->NotifyDiscoveryReception (msg);
  }

  else
	{
		NS_LOG_WARN (this << " LteControlMessage not recognized");
	}
}

void
NistLteUeMac::DoReceiveNistMTLteControlMessage (Ptr<NistLteControlMessage> msg)
{
	NS_LOG_FUNCTION (this);
	if (msg->GetMessageType () == NistLteControlMessage::UL_DCI)
	{
		Ptr<NistUlDciLteControlMessage> msg2 = DynamicCast<NistUlDciLteControlMessage> (msg);
		NistUlDciListElement_s dci = msg2->GetDci ();
		if (dci.m_ndi==1)
		{
			// New transmission -> emtpy pkt buffer queue (for deleting eventual pkts not acked )
			Ptr<PacketBurst> pb = CreateObject <PacketBurst> ();
			m_miUlHarqProcessesPacket.at (m_harqProcessId) = pb;
			// Retrieve data from RLC
			std::map <uint8_t, NistLteMacSapProvider::NistReportBufferNistStatusParameters>::iterator itBsr;
			uint16_t activeLcs = 0;
			uint32_t statusPduMinSize = 0;
			//added code
			if (m_ulBsrReceived.size()!=0)
			{
				for (itBsr = m_ulBsrReceived.begin (); itBsr != m_ulBsrReceived.end (); itBsr++)
				{
					if (((*itBsr).second.statusPduSize > 0) || ((*itBsr).second.retxQueueSize > 0) || ((*itBsr).second.txQueueSize > 0))
					{
						activeLcs++;
						if (((*itBsr).second.statusPduSize!=0)&&((*itBsr).second.statusPduSize < statusPduMinSize))
						{
							statusPduMinSize = (*itBsr).second.statusPduSize;
						}
						if (((*itBsr).second.statusPduSize!=0)&&(statusPduMinSize == 0))
						{
							statusPduMinSize = (*itBsr).second.statusPduSize;
						}
					}
				}
				if (activeLcs == 0)
				{
					NS_LOG_ERROR (this << " No active flows for this UL-DCI");
					return;
				}
				std::map <uint8_t, NistLcInfo>::iterator it;
				uint32_t bytesPerActiveLc = dci.m_tbSize / activeLcs;
				bool statusPduPriority = false;
				if ((statusPduMinSize != 0)&&(bytesPerActiveLc < statusPduMinSize))
				{
					// send only the status PDU which has highest priority
					statusPduPriority = true;
					NS_LOG_DEBUG (this << " Reduced resource -> send only Status, b ytes " << statusPduMinSize);
					if (dci.m_tbSize < statusPduMinSize)
					{
						NS_FATAL_ERROR ("Insufficient Tx Opportunity for sending a status message");
					}
				}
				NS_LOG_LOGIC (this << " UE " << m_rnti << ": UL-CQI notified TxOpportunity of " << dci.m_tbSize << " => " << bytesPerActiveLc << " bytes per active LC" << " statusPduMinSize " << statusPduMinSize);
				for (it = m_lcInfoMap.begin (); it!=m_lcInfoMap.end (); it++)
				{
					itBsr = m_ulBsrReceived.find ((*it).first);
					NS_LOG_DEBUG (this << " Processing LC " << (uint32_t)(*it).first << " bytesPerActiveLc " << bytesPerActiveLc);
					if ( (itBsr!=m_ulBsrReceived.end ()) &&
							( ((*itBsr).second.statusPduSize > 0) ||
									((*itBsr).second.retxQueueSize > 0) ||
									((*itBsr).second.txQueueSize > 0)) )
					{
						if ((statusPduPriority) && ((*itBsr).second.statusPduSize == statusPduMinSize))
						{
							(*it).second.macSapUser->NotifyTxOpportunity ((*itBsr).second.statusPduSize, 0, 0);
							NS_LOG_LOGIC (this << "\t" << bytesPerActiveLc << " send  " << (*itBsr).second.statusPduSize << " status bytes to LC " << (uint32_t)(*it).first << " statusQueue " << (*itBsr).second.statusPduSize << " retxQueue" << (*itBsr).second.retxQueueSize << " txQueue" <<  (*itBsr).second.txQueueSize);
							(*itBsr).second.statusPduSize = 0;
							break;
						}
						else
						{
							uint32_t bytesForThisLc = bytesPerActiveLc;
							NS_LOG_LOGIC (this << "\t" << bytesPerActiveLc << " bytes to LC " << (uint32_t)(*it).first << " statusQueue " << (*itBsr).second.statusPduSize << " retxQueue" << (*itBsr).second.retxQueueSize << " txQueue" <<  (*itBsr).second.txQueueSize);
							if (((*itBsr).second.statusPduSize > 0) && (bytesForThisLc > (*itBsr).second.statusPduSize))
							{
								(*it).second.macSapUser->NotifyTxOpportunity ((*itBsr).second.statusPduSize, 0, 0);
								bytesForThisLc -= (*itBsr).second.statusPduSize;
								NS_LOG_DEBUG (this << " serve STATUS " << (*itBsr).second.statusPduSize);
								(*itBsr).second.statusPduSize = 0;
							}
							else
							{
								if ((*itBsr).second.statusPduSize>bytesForThisLc)
								{
									NS_FATAL_ERROR ("Insufficient Tx Opportunity for sending a status message");
								}
							}

							if ((bytesForThisLc > 7) && // 7 is the min TxOpportunity useful for Rlc
									(((*itBsr).second.retxQueueSize > 0) ||
											((*itBsr).second.txQueueSize > 0)))
							{
								if ((*itBsr).second.retxQueueSize > 0)
								{
									NS_LOG_DEBUG (this << " serve retx DATA, bytes " << bytesForThisLc);
									(*it).second.macSapUser->NotifyTxOpportunity (bytesForThisLc, 0, 0);
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
									uint16_t lcid = (*it).first;
									uint32_t rlcOverhead;
									if (lcid == 1)
									{
										// for SRB1 (using RLC AM) it's better to
										// overestimate RLC overhead rather than
										// underestimate it and risk unneeded
										// segmentation which increases delay
										rlcOverhead = 4;
									}
									else
									{
										// minimum RLC overhead due to header
										rlcOverhead = 2;
									}
									NS_LOG_DEBUG (this << " serve tx DATA, bytes " << bytesForThisLc << ", RLC overhead " << rlcOverhead);
									(*it).second.macSapUser->NotifyTxOpportunity (bytesForThisLc, 0, 0);
									if ((*itBsr).second.txQueueSize >= bytesForThisLc - rlcOverhead)
									{
										(*itBsr).second.txQueueSize -= bytesForThisLc - rlcOverhead;
									}
									else
									{
										(*itBsr).second.txQueueSize = 0;
									}
								}
							}
							else
							{
								if ( ((*itBsr).second.retxQueueSize > 0) || ((*itBsr).second.txQueueSize > 0))
								{
									// resend BSR info for updating eNB peer MAC
									m_freshUlBsr = true;
								}
							}
							NS_LOG_LOGIC (this << "\t" << bytesPerActiveLc << "\t new queues " << (uint32_t)(*it).first << " statusQueue " << (*itBsr).second.statusPduSize << " retxQueue" << (*itBsr).second.retxQueueSize << " txQueue" <<  (*itBsr).second.txQueueSize);
						}

					}
				}
			} // end if m_ulBsrReceived.size()!=0
			else
			{
				// NIST IMPLEMENTATION OF Nist_m_ulBsrReceived
				uint16_t activeLcs = 0;
				uint32_t statusPduMinSize = 0;
				std::map <uint8_t, std::map <uint8_t, NistLteMacSapProvider::NistReportBufferNistStatusParameters > >:: iterator it;
				it=Nist_m_ulBsrReceived.find(m_rnti);
				if (it!=Nist_m_ulBsrReceived.end())
				{
					// Retrieve data from RLC
					std::map <uint8_t, NistLteMacSapProvider::NistReportBufferNistStatusParameters > mapLC=it->second;
					std::map <uint8_t, NistLteMacSapProvider::NistReportBufferNistStatusParameters>::iterator itBsr;

					for (itBsr = mapLC.begin (); itBsr != mapLC.end (); itBsr++)
					{
						if (((*itBsr).second.statusPduSize > 0) || ((*itBsr).second.retxQueueSize > 0) || ((*itBsr).second.txQueueSize > 0))
						{
							activeLcs++;
							if (((*itBsr).second.statusPduSize!=0)&&((*itBsr).second.statusPduSize < statusPduMinSize))
							{
								statusPduMinSize = (*itBsr).second.statusPduSize;
							}
							if (((*itBsr).second.statusPduSize!=0)&&(statusPduMinSize == 0))
							{
								statusPduMinSize = (*itBsr).second.statusPduSize;
							}
						}// end if
					}// end for

					if (activeLcs == 0)
					{
						NS_LOG_ERROR (this << " No active flows for this UL-DCI");
						return;
					}

					for (itBsr = mapLC.begin (); itBsr != mapLC.end (); itBsr++)
					{
						// compute tb size for this lc
						uint32_t bytesPerActiveLc= dci.m_tbSize / activeLcs ;
						std::map <uint8_t, NistLcInfo>::iterator itLcInfo;
						bool statusPduPriority = false;
						if ((statusPduMinSize != 0)&&(bytesPerActiveLc < statusPduMinSize))
						{
							// send only the status PDU which has highest priority
							statusPduPriority = true;
							NS_LOG_DEBUG (this << " Reduced resource -> send only Status, b ytes " << statusPduMinSize);
							if (dci.m_tbSize < statusPduMinSize)
							{
								NS_FATAL_ERROR ("Insufficient Tx Opportunity for sending a status message");
							}
						}
						NS_LOG_LOGIC (this << " UE " << m_rnti << ": UL-CQI notified TxOpportunity of " << dci.m_tbSize << " => " << bytesPerActiveLc << " bytes per active LC" << " statusPduMinSize " << statusPduMinSize);

						if ( (itBsr!=mapLC.end ()) &&
								( ((*itBsr).second.statusPduSize > 0) ||
										((*itBsr).second.retxQueueSize > 0) ||
										((*itBsr).second.txQueueSize > 0)) )
						{
							itLcInfo=m_lcInfoMap.find((*itBsr).first);

							if ((statusPduPriority) && ((*itBsr).second.statusPduSize == statusPduMinSize))
							{

								(*itLcInfo).second.macSapUser->NotifyTxOpportunity ((*itBsr).second.statusPduSize, 0, 0);

								(*itBsr).second.statusPduSize = 0;
							}
							else
							{

								uint32_t bytesForThisLc = bytesPerActiveLc;
								NS_LOG_LOGIC (this << "\t" << bytesPerActiveLc << " bytes to LC " << (uint32_t)(*itLcInfo).first << " statusQueue " << (*itBsr).second.statusPduSize << " retxQueue" << (*itBsr).second.retxQueueSize << " txQueue" <<  (*itBsr).second.txQueueSize);
								if (((*itBsr).second.statusPduSize > 0) && (bytesForThisLc > (*itBsr).second.statusPduSize))
								{
									(*itLcInfo).second.macSapUser->NotifyTxOpportunity ((*itBsr).second.statusPduSize, 0, 0);
									bytesForThisLc -= (*itBsr).second.statusPduSize;
									NS_LOG_DEBUG (this << " serve STATUS " << (*itBsr).second.statusPduSize);
									(*itBsr).second.statusPduSize = 0;

								}
								else
								{
									if ((*itBsr).second.statusPduSize>bytesForThisLc)
									{
										NS_FATAL_ERROR ("Insufficient Tx Opportunity for sending a status message");
									}
								}

								if ((bytesForThisLc > 7) && // 7 is the min TxOpportunity useful for Rlc
										(((*itBsr).second.retxQueueSize > 0) ||
												((*itBsr).second.txQueueSize > 0)))
								{
									if ((*itBsr).second.retxQueueSize > 0)
									{
										NS_LOG_DEBUG (this << " serve retx DATA, bytes " << bytesForThisLc);

										(*itLcInfo).second.macSapUser->NotifyTxOpportunity (bytesForThisLc, 0, 0);
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
										uint16_t lcid = (*itLcInfo).first;
										uint32_t rlcOverhead;
										if (lcid == 1)
										{
											// for SRB1 (using RLC AM) it's better to
											// overestimate RLC overhead rather than
											// underestimate it and risk unneeded
											// segmentation which increases delay
											rlcOverhead = 4;
										}
										else
										{
											// minimum RLC overhead due to header
											rlcOverhead = 2;
										}
										NS_LOG_DEBUG (this << " serve tx DATA, bytes " << bytesForThisLc << ", RLC overhead " << rlcOverhead);
										(*itLcInfo).second.macSapUser->NotifyTxOpportunity (bytesForThisLc, 0, 0);
										if ((*itBsr).second.txQueueSize >= bytesForThisLc - rlcOverhead)
										{
											(*itBsr).second.txQueueSize -= bytesForThisLc - rlcOverhead;

										}
										else
										{
											(*itBsr).second.txQueueSize = 0;

										}
									}
								}
								else
								{
									if ( ((*itBsr).second.retxQueueSize > 0) || ((*itBsr).second.txQueueSize > 0))
									{
										// resend BSR info for updating eNB peer MAC
										m_freshUlBsr = true;
									}
								}
								NS_LOG_LOGIC (this << "\t" << bytesPerActiveLc << "\t new queues " << (uint32_t)(*itLcInfo).first << " statusQueue " << (*itBsr).second.statusPduSize << " retxQueue" << (*itBsr).second.retxQueueSize << " txQueue" <<  (*itBsr).second.txQueueSize);
							}

						}
					}
				}// end for p
			} // end it!=Nist_m_ulBsrReceived.end()
		} //end if ndi
		else  //else ndi
		{
			// HARQ retransmission -> retrieve data from HARQ buffer
			NS_LOG_DEBUG (this << " UE MAC RETX HARQ " << (uint16_t)m_harqProcessId);
			Ptr<PacketBurst> pb = m_miUlHarqProcessesPacket.at (m_harqProcessId);
			for (std::list<Ptr<Packet> >::const_iterator j = pb->Begin (); j != pb->End (); ++j)
			{
				Ptr<Packet> pkt = (*j)->Copy ();
				m_uePhySapProvider->SendMacPdu (pkt);
			}
			m_miUlHarqProcessesPacketTimer.at (m_harqProcessId) = HARQ_PERIOD;
		}

	}
	else if (msg->GetMessageType () == NistLteControlMessage::RAR)
	{
		if (m_waitingForRaResponse)
		{
			Ptr<NistRarLteControlMessage> rarMsg = DynamicCast<NistRarLteControlMessage> (msg);
			uint16_t raRnti = rarMsg->GetRaRnti ();
			NS_LOG_LOGIC (this << "got RAR with RA-RNTI " << (uint32_t) raRnti << ", expecting " << (uint32_t) m_raRnti);
			if (raRnti == m_raRnti) // RAR corresponds to TX subframe of preamble
			{
				for (std::list<NistRarLteControlMessage::NistRar>::const_iterator it = rarMsg->NistRarListBegin ();
						it != rarMsg->NistRarListEnd ();
						++it)
				{
					if (it->rapId == m_raPreambleId) // RAR is for me
					{
						RecvRaResponse (it->rarPayload);
						/// \todo RRC generates the RecvRaResponse messaged
						/// for avoiding holes in transmission at PHY layer
						/// (which produce erroneous UL CQI evaluation)
					}
				}
			}
		}
	}
	else if (msg->GetMessageType () == NistLteControlMessage::SL_DCI)
	{
		Ptr<NistSlDciLteControlMessage> msg2 = DynamicCast<NistSlDciLteControlMessage> (msg);
		NistSlDciListElement_s dci = msg2->GetDci ();

		//store the grant for the next SC period
		//TODO: distinguish SL grants for different pools. Right now just assume there is only one pool
		Ptr<SidelinkTxCommResourcePool> pool = DynamicCast<SidelinkTxCommResourcePool> (m_sidelinkTxPoolsMap.begin()->second.m_pool);
		NS_ASSERT (pool->GetSchedulingType() == SidelinkCommResourcePool::SCHEDULED);

		SidelinkGrant grant;
		grant.m_resPscch = dci.m_resPscch;
		grant.m_tpc = dci.m_tpc;
		grant.m_hopping = dci.m_hopping;
		grant.m_rbStart = dci.m_rbStart;
		grant.m_rbLen = dci.m_rbLen;
		grant.m_trp = dci.m_trp;
		grant.m_mcs = pool->GetMcs();
		grant.m_tbSize = 0; //computed later
		m_sidelinkTxPoolsMap.begin()->second.m_nextGrant = grant;
		m_sidelinkTxPoolsMap.begin()->second.m_grant_received = true;

		NS_LOG_INFO (this << "Received SL_DCI message rnti=" << m_rnti << " res=" << (uint16_t) dci.m_resPscch);
	}

  else if (msg->GetMessageType () == NistLteControlMessage::SL_DISC_MSG)
  {
    NS_LOG_INFO (this << " Received discovery message");
    //notify RRC (pass msg to RRC where we can filter)
    m_cmacSapUser->NotifyDiscoveryReception (msg);
  }

  else
	{
		NS_LOG_WARN (this << " LteControlMessage not recognized");
	}

}

void
NistLteUeMac::DoReceiveNistPrLteControlMessage (Ptr<NistLteControlMessage> msg)
{
	NS_LOG_FUNCTION (this);
	if (msg->GetMessageType () == NistLteControlMessage::UL_DCI)
	{
		Ptr<NistUlDciLteControlMessage> msg2 = DynamicCast<NistUlDciLteControlMessage> (msg);
		NistUlDciListElement_s dci = msg2->GetDci ();
		if (dci.m_ndi==1)
		{
			// New transmission -> emtpy pkt buffer queue (for deleting eventual pkts not acked )
			Ptr<PacketBurst> pb = CreateObject <PacketBurst> ();
			m_miUlHarqProcessesPacket.at (m_harqProcessId) = pb;
			// Retrieve data from RLC
			std::map <uint8_t, NistLteMacSapProvider::NistReportBufferNistStatusParameters>::iterator itBsr;
			uint16_t activeLcs = 0;
			uint32_t statusPduMinSize = 0;

			if (m_ulBsrReceived.size()!=0)
			{
				for (itBsr = m_ulBsrReceived.begin (); itBsr != m_ulBsrReceived.end (); itBsr++)
				{
					if (((*itBsr).second.statusPduSize > 0) || ((*itBsr).second.retxQueueSize > 0) || ((*itBsr).second.txQueueSize > 0))
					{
						activeLcs++;
						if (((*itBsr).second.statusPduSize!=0)&&((*itBsr).second.statusPduSize < statusPduMinSize))
						{
							statusPduMinSize = (*itBsr).second.statusPduSize;
						}
						if (((*itBsr).second.statusPduSize!=0)&&(statusPduMinSize == 0))
						{
							statusPduMinSize = (*itBsr).second.statusPduSize;
						}
					}
				}
				if (activeLcs == 0)
				{
					NS_LOG_ERROR (this << " No active flows for this UL-DCI");
					return;
				}
				std::map <uint8_t, NistLcInfo>::iterator it;
				uint32_t bytesPerActiveLc = dci.m_tbSize / activeLcs;
				bool statusPduPriority = false;
				if ((statusPduMinSize != 0)&&(bytesPerActiveLc < statusPduMinSize))
				{
					// send only the status PDU which has highest priority
					statusPduPriority = true;
					NS_LOG_DEBUG (this << " Reduced resource -> send only Status, b ytes " << statusPduMinSize);
					if (dci.m_tbSize < statusPduMinSize)
					{
						NS_FATAL_ERROR ("Insufficient Tx Opportunity for sending a status message");
					}
				}
				NS_LOG_LOGIC (this << " UE " << m_rnti << ": UL-CQI notified TxOpportunity of " << dci.m_tbSize << " => " << bytesPerActiveLc << " bytes per active LC" << " statusPduMinSize " << statusPduMinSize);
				for (it = m_lcInfoMap.begin (); it!=m_lcInfoMap.end (); it++)
				{
					itBsr = m_ulBsrReceived.find ((*it).first);
					NS_LOG_DEBUG (this << " Processing LC " << (uint32_t)(*it).first << " bytesPerActiveLc " << bytesPerActiveLc);
					if ( (itBsr!=m_ulBsrReceived.end ()) &&
							( ((*itBsr).second.statusPduSize > 0) ||
									((*itBsr).second.retxQueueSize > 0) ||
									((*itBsr).second.txQueueSize > 0)) )
					{
						if ((statusPduPriority) && ((*itBsr).second.statusPduSize == statusPduMinSize))
						{
							(*it).second.macSapUser->NotifyTxOpportunity ((*itBsr).second.statusPduSize, 0, 0);
							NS_LOG_LOGIC (this << "\t" << bytesPerActiveLc << " send  " << (*itBsr).second.statusPduSize << " status bytes to LC " << (uint32_t)(*it).first << " statusQueue " << (*itBsr).second.statusPduSize << " retxQueue" << (*itBsr).second.retxQueueSize << " txQueue" <<  (*itBsr).second.txQueueSize);
							(*itBsr).second.statusPduSize = 0;
							break;
						}
						else
						{
							uint32_t bytesForThisLc = bytesPerActiveLc;
							NS_LOG_LOGIC (this << "\t" << bytesPerActiveLc << " bytes to LC " << (uint32_t)(*it).first << " statusQueue " << (*itBsr).second.statusPduSize << " retxQueue" << (*itBsr).second.retxQueueSize << " txQueue" <<  (*itBsr).second.txQueueSize);
							if (((*itBsr).second.statusPduSize > 0) && (bytesForThisLc > (*itBsr).second.statusPduSize))
							{
								(*it).second.macSapUser->NotifyTxOpportunity ((*itBsr).second.statusPduSize, 0, 0);
								bytesForThisLc -= (*itBsr).second.statusPduSize;
								NS_LOG_DEBUG (this << " serve STATUS " << (*itBsr).second.statusPduSize);
								(*itBsr).second.statusPduSize = 0;
							}
							else
							{
								if ((*itBsr).second.statusPduSize>bytesForThisLc)
								{
									NS_FATAL_ERROR ("Insufficient Tx Opportunity for sending a status message");
								}
							}

							if ((bytesForThisLc > 7) && // 7 is the min TxOpportunity useful for Rlc
									(((*itBsr).second.retxQueueSize > 0) ||
											((*itBsr).second.txQueueSize > 0)))
							{
								if ((*itBsr).second.retxQueueSize > 0)
								{
									NS_LOG_DEBUG (this << " serve retx DATA, bytes " << bytesForThisLc);
									(*it).second.macSapUser->NotifyTxOpportunity (bytesForThisLc, 0, 0);
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
									uint16_t lcid = (*it).first;
									uint32_t rlcOverhead;
									if (lcid == 1)
									{
										// for SRB1 (using RLC AM) it's better to
										// overestimate RLC overhead rather than
										// underestimate it and risk unneeded
										// segmentation which increases delay
										rlcOverhead = 4;
									}
									else
									{
										// minimum RLC overhead due to header
										rlcOverhead = 2;
									}
									NS_LOG_DEBUG (this << " serve tx DATA, bytes " << bytesForThisLc << ", RLC overhead " << rlcOverhead);
									(*it).second.macSapUser->NotifyTxOpportunity (bytesForThisLc, 0, 0);
									if ((*itBsr).second.txQueueSize >= bytesForThisLc - rlcOverhead)
									{
										(*itBsr).second.txQueueSize -= bytesForThisLc - rlcOverhead;
									}
									else
									{
										(*itBsr).second.txQueueSize = 0;
									}
								}
							}
							else
							{
								if ( ((*itBsr).second.retxQueueSize > 0) || ((*itBsr).second.txQueueSize > 0))
								{
									// resend BSR info for updating eNB peer MAC
									m_freshUlBsr = true;
								}
							}
							NS_LOG_LOGIC (this << "\t" << bytesPerActiveLc << "\t new queues " << (uint32_t)(*it).first << " statusQueue " << (*itBsr).second.statusPduSize << " retxQueue" << (*itBsr).second.retxQueueSize << " txQueue" <<  (*itBsr).second.txQueueSize);
						}

					}
				}
			} // end if m_ulBsrReceived.size()!=0
			else
			{
				// NIST IMPLEMENTATION OF Nist_m_ulBsrReceived
				uint16_t activeLcs = 0;
				uint32_t statusPduMinSize = 0;
				std::map <uint8_t, std::map <uint8_t, NistLteMacSapProvider::NistReportBufferNistStatusParameters > >:: iterator it;
				it=Nist_m_ulBsrReceived.find(m_rnti);
				if (it!=Nist_m_ulBsrReceived.end())
				{
					// Retrieve data from RLC
					std::map <uint8_t, NistLteMacSapProvider::NistReportBufferNistStatusParameters > mapLC=it->second;
					std::map <uint8_t, NistLteMacSapProvider::NistReportBufferNistStatusParameters>::iterator itBsr;

					for (itBsr = mapLC.begin (); itBsr != mapLC.end (); itBsr++)
					{
						if (((*itBsr).second.statusPduSize > 0) || ((*itBsr).second.retxQueueSize > 0) || ((*itBsr).second.txQueueSize > 0))
						{
							activeLcs++;
							if (((*itBsr).second.statusPduSize!=0)&&((*itBsr).second.statusPduSize < statusPduMinSize))
							{
								statusPduMinSize = (*itBsr).second.statusPduSize;
							}
							if (((*itBsr).second.statusPduSize!=0)&&(statusPduMinSize == 0))
							{
								statusPduMinSize = (*itBsr).second.statusPduSize;
							}
						}// end if
					}// end for

					if (activeLcs == 0)
					{
						NS_LOG_ERROR (this << " No active flows for this UL-DCI");
						return;
					}

					std::vector<uint8_t> TreatedLCs;

					std::map <uint8_t, std::map <uint8_t, uint8_t> >::iterator itP;
					std::map <uint8_t, uint8_t> mapLCP;
					std::map <uint8_t, uint8_t>::iterator itLCP;
					itP= PriorityMap.find(m_rnti);
					mapLCP=itP->second;
					uint16_t TbTemp=dci.m_tbSize;
					for (uint16_t p=0; p < activeLcs; p++)
					{
						uint8_t MinPriority=10;
						uint8_t lcidMin=0;

						for (itLCP= mapLCP.begin (); itLCP != mapLCP.end (); itLCP++)
						{
							//CHECK IF THIS LCID IS ALREADY TREATED
							std::vector <uint8_t>::iterator Ft;
							bool findF=false;
							for(Ft=TreatedLCs.begin();Ft!=TreatedLCs.end();Ft++)
							{
								if((*Ft)==(*itLCP).first)
								{
									findF=true;

									break;
								}
							}
							if (findF==true)
							{
								MinPriority=10;
								continue;
							}
							if (findF==false)
							{
								//this LC doesn't exist in Treated LC
								if ((*itLCP).second < MinPriority )
								{
									lcidMin=(*itLCP).first;

									MinPriority=(*itLCP).second;

								}
							}
						}//end for

						TreatedLCs.push_back (lcidMin);
						// compute tb size for this lc
						itBsr=mapLC.find (lcidMin);
						uint32_t bytesPerActiveLc;
						if ( TbTemp >= (*itBsr).second.txQueueSize )
						{
							bytesPerActiveLc=(*itBsr).second.txQueueSize;
							TbTemp = TbTemp - (*itBsr).second.txQueueSize;
						}
						else
						{
							bytesPerActiveLc = TbTemp;
							TbTemp=0;
						}
						std::map <uint8_t, NistLcInfo>::iterator itLcInfo;
						bool statusPduPriority = false;
						if ((statusPduMinSize != 0)&&(bytesPerActiveLc < statusPduMinSize))
						{
							// send only the status PDU which has highest priority
							statusPduPriority = true;
							NS_LOG_DEBUG (this << " Reduced resource -> send only Status, b ytes " << statusPduMinSize);
							if (dci.m_tbSize < statusPduMinSize)
							{
								NS_FATAL_ERROR ("Insufficient Tx Opportunity for sending a status message");
							}
						}
						NS_LOG_LOGIC (this << " UE " << m_rnti << ": UL-CQI notified TxOpportunity of " << dci.m_tbSize << " => " << bytesPerActiveLc << " bytes per active LC" << " statusPduMinSize " << statusPduMinSize);

						itBsr=mapLC.begin();
						itBsr = mapLC.find (lcidMin);

						if ( (itBsr!=mapLC.end ()) &&
								( ((*itBsr).second.statusPduSize > 0) ||
										((*itBsr).second.retxQueueSize > 0) ||
										((*itBsr).second.txQueueSize > 0)) )
						{
							itLcInfo=m_lcInfoMap.find((*itBsr).first);

							if ((statusPduPriority) && ((*itBsr).second.statusPduSize == statusPduMinSize))
							{

								(*itLcInfo).second.macSapUser->NotifyTxOpportunity ((*itBsr).second.statusPduSize, 0, 0);

								(*itBsr).second.statusPduSize = 0;
							}
							else
							{

								uint32_t bytesForThisLc = bytesPerActiveLc;
								NS_LOG_LOGIC (this << "\t" << bytesPerActiveLc << " bytes to LC " << (uint32_t)(*itLcInfo).first << " statusQueue " << (*itBsr).second.statusPduSize << " retxQueue" << (*itBsr).second.retxQueueSize << " txQueue" <<  (*itBsr).second.txQueueSize);
								if (((*itBsr).second.statusPduSize > 0) && (bytesForThisLc > (*itBsr).second.statusPduSize))
								{
									(*itLcInfo).second.macSapUser->NotifyTxOpportunity ((*itBsr).second.statusPduSize, 0, 0);
									bytesForThisLc -= (*itBsr).second.statusPduSize;
									NS_LOG_DEBUG (this << " serve STATUS " << (*itBsr).second.statusPduSize);
									(*itBsr).second.statusPduSize = 0;

								}
								else
								{
									if ((*itBsr).second.statusPduSize>bytesForThisLc)
									{
										NS_FATAL_ERROR ("Insufficient Tx Opportunity for sending a status message");
									}
								}

								if ((bytesForThisLc > 7) && // 7 is the min TxOpportunity useful for Rlc
										(((*itBsr).second.retxQueueSize > 0) ||
												((*itBsr).second.txQueueSize > 0)))
								{
									if ((*itBsr).second.retxQueueSize > 0)
									{
										NS_LOG_DEBUG (this << " serve retx DATA, bytes " << bytesForThisLc);

										(*itLcInfo).second.macSapUser->NotifyTxOpportunity (bytesForThisLc, 0, 0);
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
										uint16_t lcid = (*itLcInfo).first;
										uint32_t rlcOverhead;
										if (lcid == 1)
										{
											// for SRB1 (using RLC AM) it's better to
											// overestimate RLC overhead rather than
											// underestimate it and risk unneeded
											// segmentation which increases delay
											rlcOverhead = 4;
										}
										else
										{
											// minimum RLC overhead due to header
											rlcOverhead = 2;
										}
										NS_LOG_DEBUG (this << " serve tx DATA, bytes " << bytesForThisLc << ", RLC overhead " << rlcOverhead);
										(*itLcInfo).second.macSapUser->NotifyTxOpportunity (bytesForThisLc, 0, 0);
										if ((*itBsr).second.txQueueSize >= bytesForThisLc - rlcOverhead)
										{
											(*itBsr).second.txQueueSize -= bytesForThisLc - rlcOverhead;

										}
										else
										{
											(*itBsr).second.txQueueSize = 0;

										}
									}
								}
								else
								{
									if ( ((*itBsr).second.retxQueueSize > 0) || ((*itBsr).second.txQueueSize > 0))
									{
										// resend BSR info for updating eNB peer MAC
										m_freshUlBsr = true;
									}
								}
								NS_LOG_LOGIC (this << "\t" << bytesPerActiveLc << "\t new queues " << (uint32_t)(*itLcInfo).first << " statusQueue " << (*itBsr).second.statusPduSize << " retxQueue" << (*itBsr).second.retxQueueSize << " txQueue" <<  (*itBsr).second.txQueueSize);
							}

						}
					}
				}// end for p
			} // end it!=Nist_m_ulBsrReceived.end()
		} //end if ndi
		else  //else ndi
		{
			// HARQ retransmission -> retrieve data from HARQ buffer
			NS_LOG_DEBUG (this << " UE MAC RETX HARQ " << (uint16_t)m_harqProcessId);
			Ptr<PacketBurst> pb = m_miUlHarqProcessesPacket.at (m_harqProcessId);
			for (std::list<Ptr<Packet> >::const_iterator j = pb->Begin (); j != pb->End (); ++j)
			{
				Ptr<Packet> pkt = (*j)->Copy ();
				m_uePhySapProvider->SendMacPdu (pkt);
			}
			m_miUlHarqProcessesPacketTimer.at (m_harqProcessId) = HARQ_PERIOD;
		}

	}
	else if (msg->GetMessageType () == NistLteControlMessage::RAR)
	{
		if (m_waitingForRaResponse)
		{
			Ptr<NistRarLteControlMessage> rarMsg = DynamicCast<NistRarLteControlMessage> (msg);
			uint16_t raRnti = rarMsg->GetRaRnti ();
			NS_LOG_LOGIC (this << "got RAR with RA-RNTI " << (uint32_t) raRnti << ", expecting " << (uint32_t) m_raRnti);
			if (raRnti == m_raRnti) // RAR corresponds to TX subframe of preamble
			{
				for (std::list<NistRarLteControlMessage::NistRar>::const_iterator it = rarMsg->NistRarListBegin ();
						it != rarMsg->NistRarListEnd ();
						++it)
				{
					if (it->rapId == m_raPreambleId) // RAR is for me
					{
						RecvRaResponse (it->rarPayload);
						/// \todo RRC generates the RecvRaResponse messaged
						/// for avoiding holes in transmission at PHY layer
						/// (which produce erroneous UL CQI evaluation)
					}
				}
			}
		}
	}
	else if (msg->GetMessageType () == NistLteControlMessage::SL_DCI)
	{
		Ptr<NistSlDciLteControlMessage> msg2 = DynamicCast<NistSlDciLteControlMessage> (msg);
		NistSlDciListElement_s dci = msg2->GetDci ();

		//store the grant for the next SC period
		//TODO: distinguish SL grants for different pools. Right now just assume there is only one pool
		Ptr<SidelinkTxCommResourcePool> pool = DynamicCast<SidelinkTxCommResourcePool> (m_sidelinkTxPoolsMap.begin()->second.m_pool);
		NS_ASSERT (pool->GetSchedulingType() == SidelinkCommResourcePool::SCHEDULED);

		SidelinkGrant grant;
		grant.m_resPscch = dci.m_resPscch;
		grant.m_tpc = dci.m_tpc;
		grant.m_hopping = dci.m_hopping;
		grant.m_rbStart = dci.m_rbStart;
		grant.m_rbLen = dci.m_rbLen;
		grant.m_trp = dci.m_trp;
		grant.m_mcs = pool->GetMcs();
		grant.m_tbSize = 0; //computed later
		m_sidelinkTxPoolsMap.begin()->second.m_nextGrant = grant;
		m_sidelinkTxPoolsMap.begin()->second.m_grant_received = true;

		NS_LOG_INFO (this << "Received SL_DCI message rnti=" << m_rnti << " res=" << (uint16_t) dci.m_resPscch);
	}

  else if (msg->GetMessageType () == NistLteControlMessage::SL_DISC_MSG)
  {
    NS_LOG_INFO (this << " Received discovery message");
    //notify RRC (pass msg to RRC where we can filter)
    m_cmacSapUser->NotifyDiscoveryReception (msg);
  }
  
  else
	{
		NS_LOG_WARN (this << " LteControlMessage not recognized");
	}

}

void
NistLteUeMac::RefreshHarqProcessesPacketBuffer (void)
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

//TODO FIXME NEW for V2X

SidelinkCommResourcePool::SubframeInfo
NistLteUeMac::SimulatorTimeToSubframe (Time time)
{
   uint64_t milliseconds = time.GetSeconds () * 1000 + 15;
   SidelinkCommResourcePool::SubframeInfo SF;
   SF.subframeNo = (uint32_t) (milliseconds % 10);
   SF.frameNo = (uint32_t) ((milliseconds / 10) % 1024);
  
   return SF;
}

uint32_t 
NistLteUeMac::ComputeResidualCSRs (std::map<uint16_t,std::list<SidelinkCommResourcePool::SubframeInfo> > L1)
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
NistLteUeMac::CompareRssi (CandidateCSRl2 first, CandidateCSRl2 second)
{
  return (first.rssi < second.rssi);
}

NistLteUeMac::V2XSidelinkGrant 
NistLteUeMac::V2XSelectResources (uint32_t frameNo, uint32_t subframeNo, NistLteUeMac::V2XSidelinkGrant V2XGrant, uint32_t pdb, uint32_t p_rsvp, uint8_t v2xMessageType, uint8_t v2xTrafficType, uint16_t ReselectionCounter, uint16_t PacketSize, uint16_t ReservationSize)
{                               
   frameNo --;
   subframeNo --; 

   NS_ASSERT_MSG(ReservationSize >= PacketSize, "Packet size must be larger than the reservation size (in bytes)");

  /* std::ofstream sensingDebug;
   if (m_rnti == 1 && false)
   {
     sensingDebug.open ("results/sidelink/sensingDebug.txt", std::ios_base::app);
     sensingDebug << "--------------------------------------------------\r\n \r\n";
     sensingDebug << "Sensed Reservation List at RNTI " << m_rnti << " at time " << Simulator::Now ().GetSeconds () << ":\r\n";
     std::map <uint16_t,std::map<SidelinkCommResourcePool::SubframeInfo, ReservedCSR> >::iterator sensedIt;
     for (sensedIt = m_sensedReservedCSRMap.begin (); sensedIt != m_sensedReservedCSRMap.end (); sensedIt++)
     {
       sensingDebug << " CSR Index " <<  (int) (*sensedIt).first << " :\r\n";
       std::map<SidelinkCommResourcePool::SubframeInfo, ReservedCSR>::iterator sensedSFIt;
       for (sensedSFIt = (*sensedIt).second.begin (); sensedSFIt != (*sensedIt).second.end (); sensedSFIt++)
       {
         sensingDebug << "      SF(" << (*sensedSFIt).first.frameNo << "," << (*sensedSFIt).first.subframeNo << "), reception time: " << (*sensedSFIt).second.reservationTime << ", RSRP: " << (*sensedSFIt).second.psschRsrpDb << "dBm" << " :\r\n";
       }
     }
     sensingDebug << "\r\n \r\n Choice : --------------------------------------\r\n \r\n";
     sensingDebug.close ();
   }*/

   // Reservation
   if (p_rsvp >= 100)
   { 
      V2XGrant.m_reservation = (uint8_t) (p_rsvp/100); // TS 36.213, cl. 14.2.1	
   //   NS_LOG_UNCOND("Received p_rsvp " << p_rsvp << " Translated in " << V2XGrant.m_reservation);
   }
   else if (p_rsvp == 50)
   {
      V2XGrant.m_reservation = 0x0b;  
   }
   else if (p_rsvp == 20)
   {
      V2XGrant.m_reservation = 0x0c;  
   }
			
   // Update the reselection counter
   // TS 36.321, cl.5.14.1.1 Rel'14
   Ptr<UniformRandomVariable> uniformCresel = CreateObject<UniformRandomVariable> ();
           
   if (ReselectionCounter == 0)
   {
     NS_ASSERT_MSG(false, "Reselection counter = 0, check the CAM trace! Node ID " << m_rnti);
   }
   if (p_rsvp == 0)
   {
     NS_ASSERT_MSG(false, "Reservation period = 0, check the CAM trace! Node ID " << m_rnti);
   }

   
              
   if (p_rsvp >= 100)
   {
      if (ReselectionCounter == 10000 || ReselectionCounter > 17)  // Means non-GT allocation
      { 
         V2XGrant.m_Cresel =  uniformCresel ->  GetInteger (5,15);
         //V2XGrant.m_Cresel = 10;
      }                                   
      else
      {
         V2XGrant.m_Cresel = ReselectionCounter;
      }
   }
   else if (p_rsvp == 50)
   {
      V2XGrant.m_Cresel =  uniformCresel ->  GetInteger (10,30);
   }
   else if (p_rsvp == 20)
   {
      V2XGrant.m_Cresel =  uniformCresel -> GetInteger (25,75);
   }
   else
   {
      V2XGrant.m_Cresel = 0;
   }
 //  if (v2xTrafficType == 0x01) // -------------APERIODIC---------------- (not TESI-like)
 //  {
 //    V2XGrant.m_Cresel = 1; // For the moment, I want the reselection counter to be one for the aperiodic traffic
     //     pdb = 5; // [ms]
 //  }

   if (m_randomSelection)
   {
      V2XGrant.m_Cresel =  uniformCresel ->  GetInteger (5,15);
      V2XGrant.m_Cresel = 0;
   }

 //  UnimoreReservations[m_rnti].Reservations += V2XGrant.m_Cresel;

   SidelinkCommResourcePool::SubframeInfo currentSF;
   currentSF.frameNo = frameNo;
   currentSF.subframeNo = subframeNo;

   NS_LOG_INFO("Resource Reselection Requested Now: SF(" <<  currentSF.frameNo << "," << currentSF.subframeNo <<  "), Time: " << Simulator::Now ().GetSeconds () << "s, estimated SF(" << SimulatorTimeToSubframe (Simulator::Now ()).frameNo << "," << SimulatorTimeToSubframe (Simulator::Now ()).subframeNo << ")");
   double psschThresh = -140;
   uint16_t nsubCHsize = m_nsubCHsize; // [RB]
   //uint16_t startRBSubchannel = 0;
   uint16_t NSubCh; //the total number of subchannels
   NSubCh = std::floor(50 / nsubCHsize); // 50/10 = 5
   uint16_t L_SubCh = m_L_SubCh; // the number of subchannels for the reservation

   /*if (v2xMessageType == 0x01) //DENM
   {
      L_SubCh = 4; 
   }*/
   /*
   if (v2xTrafficType == 0x01) // -------------APERIODIC----------------
   {    // Aperiodic traffic always occupies the whole frequency axis
     L_SubCh = 4;
   }*/
   // For the moment, just four dimensions statically mapped into their respective number of subchannels
  /* if ((ReservationSize <= 1020) && (ReservationSize > 800))
     L_SubCh = 5;
   else if ((ReservationSize <= 800) && (ReservationSize > 580))
     L_SubCh = 4;
   else if ((ReservationSize <= 580) && (ReservationSize > 350))
     L_SubCh = 3;
   else if ((ReservationSize <= 350) && (ReservationSize > 135))
     L_SubCh = 2;
   else if (ReservationSize <= 135)
     L_SubCh = 1;
   else
   {
     NS_ASSERT_MSG(false, "Invalid reservation size. Choose a value below 1020 Bytes");
   }*/
   L_SubCh = GetSubchannelsNumber(ReservationSize);

   NS_LOG_DEBUG("Reserving resources for packet size: " << PacketSize << ", reservation size " << ReservationSize << ", L_SubCh " << L_SubCh);

   // the number of Candidate Single-Subframe Resources (CSRs) per subframe. 
   // TS 36.213, cl. 14.1.1.6, Rel'14:
   /*
      A candidate single-subframe resource for PSSCH transmission R_xy is defined as a set of
      L_SubCh contiguous subchannels with subchannel x + j in subframe t_y where j=0,1,...,L_Subch-1.
      The UE shall assume that any contiguous set of L_SubCh contiguous subchannels included in the 
      corresponding PSSCH resource block pool within the time interval [n+T_1 , n+T_2] corresponds to
      one CSR.
   */
   uint16_t N_CSR_per_SF = NSubCh - L_SubCh + 1;
   NS_LOG_INFO("N_CSR_per_SF: " << (int) N_CSR_per_SF);
   uint32_t T_1 = 4; // should be T1 = 4 (see 3GPP)
   uint32_t T_2 = pdb-1;

   uint16_t nbRbPssch = L_SubCh*nsubCHsize - 2; //FIXME: this applies to the adjacent situation only

   // build the first CSR list
   std::map<uint32_t,CandidateCSR> Sa;
                               
   SidelinkCommResourcePool::SubframeInfo candidateSF;
   CandidateCSR csr;
   uint32_t csrId = 0;
   for (uint32_t i = T_1; i <= T_2; i++)
   {
     candidateSF.subframeNo = (currentSF.subframeNo + i) % 10;
     candidateSF.frameNo = (currentSF.frameNo + (currentSF.subframeNo + i) / 10) % 1024;
     for (uint32_t j=0; j< (uint32_t) N_CSR_per_SF; j++)
     {
       csr.nSubCHStart = (uint16_t) j;
       csr.rbLen = nbRbPssch;
       csr.rbStart = csr.nSubCHStart * nsubCHsize;
       csr.subframe = candidateSF;
       Sa.insert (std::pair<uint32_t,CandidateCSR> (csrId, csr));
       csrId ++;
     }
     // NS_LOG_UNCOND (i);
   }

   V2XGrant.m_tpc = 0;
   V2XGrant.m_SFGap = 0; // for now, no retransmission
   V2XGrant.m_adjacency = true;
 
   uint32_t subframeInitialTx;
   Ptr<UniformRandomVariable> uniformRnd = CreateObject<UniformRandomVariable> ();
                      
   // Second Option to build the list
   std::map<uint16_t,std::list<SidelinkCommResourcePool::SubframeInfo> > L1;
   std::vector<CandidateCSRl2> finalL2; 
 //  bool List2Enabled = false;  
   uint32_t DebugNode = 2; // 0 to disable
   uint32_t iterationsCounter = 0;
 //  bool SelWindow_OUT = true;
   /*Determine the set of subchannels to be allocated*/
   // For now, just consider adjacent allocation
   // For now, select it randomly; they just need to be contiguous owing to SC-FDMA operation
   //uint16_t startSubChannel = m_ueSelectedUniformVariable->GetInteger (0, N_SubCh-L_SubCh);
 
   // ------------------------- NEW IMPLEMENTATION -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
   uint32_t nCSRtot = 1; //ComputeResidualCSRs (L1)
   uint32_t nCSRresidual = 0;
   if(!m_randomSelection)
   {
      std::map <uint16_t,std::map<SidelinkCommResourcePool::SubframeInfo, ReservedCSR>>::iterator sensedIt;
      double L1targetSize = 0.2;
      while (nCSRresidual < L1targetSize * nCSRtot)
      {
         NS_LOG_DEBUG("Building list L1");
         L1.clear();
         // build list L1
   //      std::map<uint16_t,std::list<SidelinkCommResourcePool::SubframeInfo> > L1tmp;
         for (uint16_t j=0; j<  N_CSR_per_SF; j++)
         {    
           std::list<SidelinkCommResourcePool::SubframeInfo> SFvector;
           for (uint32_t i = T_1; i <= T_2; i++)
           {
             SidelinkCommResourcePool::SubframeInfo candidateSF;
             candidateSF.subframeNo = (currentSF.subframeNo + i) % 10;
             candidateSF.frameNo = (currentSF.frameNo + (currentSF.subframeNo + i) / 10) % 1024;
             SFvector.push_back (candidateSF);
           } 
           L1.insert (std::pair<uint16_t,std::list<SidelinkCommResourcePool::SubframeInfo> > (j,SFvector)); 
         } 
         // initial L1 size
         nCSRtot = ComputeResidualCSRs (L1);
         NS_LOG_DEBUG(Simulator::Now ().GetSeconds () << " Node " << m_rnti << " Initial L1 Size " << nCSRtot);
         // Print the list of CSRs (L1)

         // Print the list of CSRs (L1)
         /*for(std::map<uint16_t,std::list<SidelinkCommResourcePool::SubframeInfo> >::iterator L1it = L1.begin(); L1it != L1.end(); L1it++)
         {
            NS_LOG_DEBUG("CSR index " << L1it->first);
            for(std::list<SidelinkCommResourcePool::SubframeInfo>::iterator FrameIT = L1it->second.begin(); FrameIT != L1it->second.end(); FrameIT++)
             NS_LOG_DEBUG("Frame " << FrameIT->frameNo << " subframe " << FrameIT->subframeNo);
         }*/

        if (m_rnti == DebugNode)
        {
          std::ofstream L1fileAlert;
          L1fileAlert.open ("results/sidelink/L1fileAlert.txt", std::ios_base::app);
          L1fileAlert << "-----Initial L1------At time " << Simulator::Now().GetSeconds() << " Frame " << frameNo << " Subframe " << subframeNo << " Number of resources: " << nCSRtot << " Target: " << (L1targetSize * nCSRtot) << " ----------" << std::endl;
          for(std::map<uint16_t,std::list<SidelinkCommResourcePool::SubframeInfo> >::iterator L1it = L1.begin(); L1it != L1.end(); L1it++)
          {
            for(std::list<SidelinkCommResourcePool::SubframeInfo>::iterator FrameIT = L1it->second.begin(); FrameIT != L1it->second.end(); FrameIT++)
             L1fileAlert << "CSR index " << L1it->first << " Frame " << FrameIT->frameNo << " subframe " << FrameIT->subframeNo << std::endl;
           }
          L1fileAlert.close ();
        }

         // ------------------ Now remove resources ------------------------------
         NS_LOG_DEBUG("UE " << m_rnti << " Now L1: remove reserved resources");
         NS_LOG_INFO("Remove sensed resources outside of the selection window. Sensed resources map size: " <<  m_sensedReservedCSRMap.size());
         // 1) Clean the list of reserved resources: keep only valid the ones within the selection window
         for (sensedIt = m_sensedReservedCSRMap.begin (); sensedIt != m_sensedReservedCSRMap.end (); sensedIt++)
         {
           std::map<SidelinkCommResourcePool::SubframeInfo, ReservedCSR>::iterator sensedSFIt; 
//           for (sensedSFIt = sensedIt->second.begin(); sensedSFIt != sensedIt->second.end(); sensedSFIt++)   
           for (sensedSFIt = sensedIt->second.begin(); sensedSFIt != sensedIt->second.end(); /*no increment*/)
           {
             if (Simulator::Now().GetSeconds() - (sensedSFIt->second.reservationTime.GetSeconds() + 0.1) > 1.0) // 0.1 is the actual difference between reservation time and frame/subframe number
             {
                NS_LOG_DEBUG("Reservation outside of the 1s Sensing Window");
                NS_LOG_DEBUG("Erase this resource: CSR index " << sensedIt->first << ", frame: " << sensedSFIt->first.frameNo << ", subframe " << sensedSFIt->first.subframeNo << " Now: " << Simulator::Now().GetSeconds() << " Time: " << sensedSFIt->second.reservationTime.GetSeconds());

                sensedSFIt = sensedIt->second.erase(sensedSFIt);  

           //     NS_LOG_DEBUG("Next pointer: CSR index " << sensedIt->first << ", frame: " << sensedSFIt->first.frameNo << ", subframe " << sensedSFIt->first.subframeNo << " Now: " << Simulator::Now().GetSeconds() << " Time: " << sensedSFIt->second.reservationTime.GetSeconds());

             }
             else
             {
                NS_LOG_DEBUG("Keep this resource: CSR index " << sensedIt->first << ", frame: " << sensedSFIt->first.frameNo << ", subframe " << sensedSFIt->first.subframeNo);
                sensedSFIt++;
             }
           }
         } // end for (sensedIt = m_sensedReservedCSRMap.begin (); sensedIt != m_sensedReservedCSRMap.end (); sensedIt++)

         // Print the list of sensed reserved resources
      /*   NS_LOG_DEBUG("Print the list of sensed resources (after removing old ones)");
         for (sensedIt = m_sensedReservedCSRMap.begin (); sensedIt != m_sensedReservedCSRMap.end (); sensedIt++)
         {
           NS_LOG_DEBUG("CSR index: " << sensedIt->first);
           std::map<SidelinkCommResourcePool::SubframeInfo, ReservedCSR>::iterator sensedSFIt;    
           for (sensedSFIt = sensedIt->second.begin(); sensedSFIt != sensedIt->second.end(); sensedSFIt++)
             NS_LOG_DEBUG("Frame " << sensedSFIt->first.frameNo << " subframe " << sensedSFIt->first.subframeNo );
             
         }*/

     /*    NS_LOG_DEBUG("Print L1");
         for(std::map<uint16_t,std::list<SidelinkCommResourcePool::SubframeInfo> >::iterator L1it = L1.begin(); L1it != L1.end(); L1it++)
         {
            NS_LOG_DEBUG("CSR index " << L1it->first);
            for(std::list<SidelinkCommResourcePool::SubframeInfo>::iterator FrameIT = L1it->second.begin(); FrameIT != L1it->second.end(); FrameIT++)
             NS_LOG_DEBUG("Frame " << FrameIT->frameNo << " subframe " << FrameIT->subframeNo);
         }*/

      //   if (m_rnti == DebugNode)
      //   std::cin.get();

         NS_LOG_INFO("Remove resources from L1"); // Only those inside the selection window

         if (!(m_useTxCresel) && !(m_useRxCresel)) 
         {
           // Save the list of sensed resources
           if (m_rnti == DebugNode)
           NistLteUeMac::UnimorePrintSensedCSR(m_sensedReservedCSRMap);

           NS_LOG_INFO("Remove resources inside the selection window from L1: dont' use Cresel");
           std::map<SidelinkCommResourcePool::SubframeInfo, ReservedCSR>::iterator candIt;
           for (std::map<uint16_t,std::list<SidelinkCommResourcePool::SubframeInfo>>::iterator L1it = L1.begin(); L1it != L1.end(); L1it++)
           {
      //       NS_LOG_INFO("L1 at CSR index " << L1it->first);
             uint16_t CurrentCSRindex = L1it->first;
             for(std::list<SidelinkCommResourcePool::SubframeInfo>::iterator FrameIT = L1it->second.begin(); FrameIT != L1it->second.end(); FrameIT++)
             {
           //     NS_LOG_DEBUG("Evaluating L1 frame " << FrameIT->frameNo << " subframe " << FrameIT->subframeNo << " P_rsvp " << p_rsvp);
                SidelinkCommResourcePool::SubframeInfo tmp;
                tmp.frameNo = FrameIT->frameNo;
                tmp.subframeNo = FrameIT->subframeNo;
                std::vector<double> RSRP_values; // posso farlo anche aggiungendo valori e poi dividendo per L_subch (faccio la media in automatico)(aggiungo zero se non c'è)
                double RSRPmin = 10000000.0; //huge dB value
                uint16_t PositiveValues = 0;
                // Fill the vector of average RSRP
                for (uint16_t j = 0; j < L_SubCh; j++)
                {
                  RSRP_values.push_back(0.0); // in mW
                }

                for (uint16_t CSRindex = CurrentCSRindex; CSRindex < CurrentCSRindex + L_SubCh; CSRindex++)
                {
                  candIt = m_sensedReservedCSRMap[CSRindex].find(tmp);
                  if (candIt != m_sensedReservedCSRMap[CSRindex].end() )
                  {
                    RSRP_values[CSRindex-CurrentCSRindex] = std::pow(10, candIt->second.psschRsrpDb/10);
                  }
                 // else it's a zero
                } 

                for (uint16_t CSRindex = CurrentCSRindex; CSRindex < CurrentCSRindex + L_SubCh; CSRindex++)
                {
              //  NS_LOG_DEBUG("CSR index " << CSRindex-CurrentCSRindex << " RSRP value " << RSRP_values[CSRindex-CurrentCSRindex]);
                  if (RSRP_values[CSRindex-CurrentCSRindex] > 0.0)
                  {
                    ++PositiveValues;
                    if (10*std::log10(RSRP_values[CSRindex-CurrentCSRindex]) < RSRPmin)
                     RSRPmin = 10*std::log10(RSRP_values[CSRindex-CurrentCSRindex]);
                  }
                }  
                if (PositiveValues > 0)
                {
             //     NS_LOG_DEBUG("The smallest RSRP is " << RSRPmin);  
                  if (RSRPmin >= psschThresh)
                  {
                   // NS_LOG_DEBUG("Removed from L1, CSR index: " << CurrentCSRindex << " Frame: " << FrameIT->frameNo << " subframe: " << FrameIT->subframeNo);
                    FrameIT = L1it->second.erase(FrameIT);
                    FrameIT--;
        //            if (m_rnti == DebugNode)
        //            std::cin.get();
                  }
                }          
             } // end for(std::list<SidelinkCommResourcePool::SubframeInfo>::iterator FrameIT = L1it->second.begin(); FrameIT != L1it->second.end(); FrameIT++)
           } // end for (std::map<uint16_t,std::list<SidelinkCommResourcePool::SubframeInfo>>::iterator L1it = L1.begin(); L1it != L1.end(); L1++)
         } // end if (!(m_useTxCresel) && !(m_useRxCresel))  
         else if ((m_useTxCresel) && !(m_useRxCresel)) 
         {
           // NEW ETSI NEW ETSI NEW ETSI NEW ETSI NEW ETSI NEW ETSI NEW ETSI NEW ETSI NEW ETSI NEW ETSI NEW ETSI NEW ETSI NEW ETSI
           // Now remove also future frames (outside of the selection window)
           NS_LOG_INFO("Remove also resources outside of the selection window from L1: use Tx Cresel");

        /*   NS_LOG_DEBUG("Print the list of sensed resources (after removing old ones)");
           for (sensedIt = m_sensedReservedCSRMap.begin (); sensedIt != m_sensedReservedCSRMap.end (); sensedIt++)
           {
             NS_LOG_DEBUG("CSR index: " << sensedIt->first);
             std::map<SidelinkCommResourcePool::SubframeInfo, ReservedCSR>::iterator sensedSFIt;    
             for (sensedSFIt = sensedIt->second.begin(); sensedSFIt != sensedIt->second.end(); sensedSFIt++)
               NS_LOG_DEBUG("Frame " << sensedSFIt->first.frameNo << " subframe " << sensedSFIt->first.subframeNo << " Cresel " << sensedSFIt->second.CreselRx << " RRI " << sensedSFIt->second.RRI << " Node ID " << sensedSFIt->second.nodeId);            
           }*/
      //     if (m_rnti == DebugNode)
      //     std::cin.get();

           if (m_rnti == DebugNode)
           NistLteUeMac::UnimorePrintSensedCSR(m_sensedReservedCSRMap);

     //      std::ofstream removedFrames;
     //      removedFrames.open ("results/sidelink/removedFrames.txt", std::ios_base::app);

           // Now remove the resources from L1
           std::map<SidelinkCommResourcePool::SubframeInfo, ReservedCSR>::iterator candIt;
           for (std::map<uint16_t,std::list<SidelinkCommResourcePool::SubframeInfo>>::iterator L1it = L1.begin(); L1it != L1.end(); L1it++)
           {
      //       NS_LOG_INFO("L1 at CSR index " << L1it->first);
             uint16_t CurrentCSRindex = L1it->first;
             for(std::list<SidelinkCommResourcePool::SubframeInfo>::iterator FrameIT = L1it->second.begin(); FrameIT != L1it->second.end(); FrameIT++)
             {
        //       NS_LOG_DEBUG("Evaluating L1 frame " << FrameIT->frameNo << " subframe " << FrameIT->subframeNo << " P_rsvp " << p_rsvp);
               std::vector<SidelinkCommResourcePool::SubframeInfo> TxFrames;
               for (uint16_t i = 0; i < V2XGrant.m_Cresel; i++)
               {
                 SidelinkCommResourcePool::SubframeInfo tmp;
                 tmp.frameNo = (FrameIT->frameNo + (i*p_rsvp/10) )%1024;
                 tmp.subframeNo = FrameIT->subframeNo; 
         //        NS_LOG_DEBUG("Next frame " << tmp.frameNo << " subframe " << tmp.subframeNo);
                 std::vector<double> RSRP_values; // posso farlo anche aggiungendo valori e poi dividendo per L_subch (faccio la media in automatico)(aggiungo zero se non c'è)
                 double RSRPmin = 10000000.0; //huge dB value
                 uint16_t PositiveValues = 0;
                 // Fill the vector of average RSRP
                 for (uint16_t j = 0; j < L_SubCh; j++)
                 {
                   RSRP_values.push_back(0.0); // in mW
                 }
                 for (uint16_t CSRindex = CurrentCSRindex; CSRindex < CurrentCSRindex + L_SubCh; CSRindex++)
                 {
                   candIt = m_sensedReservedCSRMap[CSRindex].find(tmp);
                   if (candIt != m_sensedReservedCSRMap[CSRindex].end() )
                   {
                     RSRP_values[CSRindex-CurrentCSRindex] = std::pow(10, candIt->second.psschRsrpDb/10);
                   }
                 // else it's a zero
                 } 
                 for (uint16_t CSRindex = CurrentCSRindex; CSRindex < CurrentCSRindex + L_SubCh; CSRindex++)
                 {
              //   NS_LOG_DEBUG("CSR index " << CSRindex-CurrentCSRindex << " RSRP value " << RSRP_values[CSRindex-CurrentCSRindex]);
              /*   if ((RSRP_values[CSRindex-CurrentCSRindex] > 0.0) && (10*std::log10(RSRP_values[CSRindex-CurrentCSRindex]) < RSRPmin))
                   {
                     RSRPmin = 10*std::log10(RSRP_values[CSRindex-CurrentCSRindex]);
                   }*/
                   if (RSRP_values[CSRindex-CurrentCSRindex] > 0.0)
                   {
                     ++PositiveValues;
                     if (10*std::log10(RSRP_values[CSRindex-CurrentCSRindex]) < RSRPmin)
                      RSRPmin = 10*std::log10(RSRP_values[CSRindex-CurrentCSRindex]);
                   }
                 } 
                 if (PositiveValues > 0)
                 {
           //      NS_LOG_DEBUG("The smallest RSRP is " << RSRPmin);
                   if (RSRPmin >= psschThresh)
                   {
              //       NS_LOG_DEBUG("Removed from L1, CSR index: " << CurrentCSRindex << " Frame: " << FrameIT->frameNo << " subframe: " << FrameIT->subframeNo);
               //      if (m_rnti == DebugNode)
              //        removedFrames << Simulator::Now().GetSeconds() << "," << m_rnti << "," << CurrentCSRindex << "," << FrameIT->frameNo << "," << FrameIT->subframeNo << std::endl;
                     FrameIT = L1it->second.erase(FrameIT);
                     FrameIT--;
          //           if (m_rnti == DebugNode)
          //          std::cin.get();
                     break;
                   }
             //   std::cin.get();
                 }          
               }
             } // end for(std::list<SidelinkCommResourcePool::SubframeInfo>::iterator FrameIT = L1it->second.begin(); FrameIT != L1it->second.end(); FrameIT++)
           } // end for (std::map<uint16_t,std::list<SidelinkCommResourcePool::SubframeInfo>>::iterator L1it = L1.begin(); L1it != L1.end(); L1++)
          // removedFrames.close();

     //      if (m_rnti == DebugNode)
     //      std::cin.get();
         } // end else if ((m_useTxCresel) && !(m_useRxCresel)) 
         else 
         {
           // NEW ETSI NEW ETSI NEW ETSI NEW ETSI NEW ETSI NEW ETSI NEW ETSI NEW ETSI NEW ETSI NEW ETSI NEW ETSI NEW ETSI NEW ETSI
           // Now remove also future frames (outside of the selection window)
           NS_LOG_INFO("Remove also resources outside of the selection window from L1: use Rx Cresel and Tx Cresel");

         /*  NS_LOG_DEBUG("Print the list of sensed resources (after removing old ones)");
           for (sensedIt = m_sensedReservedCSRMap.begin (); sensedIt != m_sensedReservedCSRMap.end (); sensedIt++)
           {
             NS_LOG_DEBUG("CSR index: " << sensedIt->first);
             std::map<SidelinkCommResourcePool::SubframeInfo, ReservedCSR>::iterator sensedSFIt;    
             for (sensedSFIt = sensedIt->second.begin(); sensedSFIt != sensedIt->second.end(); sensedSFIt++)
               NS_LOG_DEBUG("Frame " << sensedSFIt->first.frameNo << " subframe " << sensedSFIt->first.subframeNo << " Cresel " << sensedSFIt->second.CreselRx << " RRI " << sensedSFIt->second.RRI << " Node ID " << sensedSFIt->second.nodeId);            
           }*/

       //    if (m_rnti == DebugNode)
       //      std::cin.get();

           std::map <uint16_t,std::map<SidelinkCommResourcePool::SubframeInfo, ReservedCSR> > SensedMap_EXT;

           NS_LOG_DEBUG("Extending the list of sensed resources"); // assume to know the reselection counter value, can be changed

           for (sensedIt = m_sensedReservedCSRMap.begin (); sensedIt != m_sensedReservedCSRMap.end (); sensedIt++)
           {
      //       NS_LOG_DEBUG("CSR index: " << sensedIt->first);
             std::map<SidelinkCommResourcePool::SubframeInfo, ReservedCSR>::iterator sensedSFIt;    
             for (sensedSFIt = sensedIt->second.begin(); sensedSFIt != sensedIt->second.end(); sensedSFIt++)
             {
       //         NS_LOG_DEBUG("Frame " << sensedSFIt->first.frameNo << " subframe " << sensedSFIt->first.subframeNo << " Cresel " << sensedSFIt->second.CreselRx << " RRI " << sensedSFIt->second.RRI << " Node ID " << sensedSFIt->second.nodeId);
                for (uint16_t j = 0; j < sensedSFIt->second.CreselRx; j++)
                {
                   SidelinkCommResourcePool::SubframeInfo tmp;
                   tmp.frameNo = (sensedSFIt->first.frameNo + (j*sensedSFIt->second.RRI/10) ) % 1024;
                   tmp.subframeNo = sensedSFIt->first.subframeNo;
             //      NS_LOG_DEBUG("Evaluating frame " << tmp.frameNo << " subframe " << tmp.subframeNo);
                   std::map <uint16_t,std::map<SidelinkCommResourcePool::SubframeInfo, ReservedCSR> >::iterator EXTmap_it = SensedMap_EXT.find(sensedIt->first);
                   if (EXTmap_it != SensedMap_EXT.end())  // if the CSR index does not exist
                   {
                      std::map<SidelinkCommResourcePool::SubframeInfo, ReservedCSR>::iterator checkIT = SensedMap_EXT[sensedIt->first].find(tmp);
                      if ((checkIT != (*EXTmap_it).second.end()) && (checkIT->second.nodeId == sensedSFIt->second.nodeId))
                      {
                //         NS_LOG_DEBUG("Found " << checkIT->first.frameNo << " subframe " << checkIT->first.subframeNo << " Cresel " << sensedSFIt->second.CreselRx << " RRI " << sensedSFIt->second.RRI << " Node ID " << sensedSFIt->second.nodeId);
                //         NS_LOG_DEBUG("This entry already exists!");
                 //        if (m_rnti == DebugNode)
                 //        std::cin.get();
                      }
                      else if ((checkIT != (*EXTmap_it).second.end()) && (checkIT->second.nodeId != sensedSFIt->second.nodeId))
                      {
                //         NS_LOG_DEBUG("Big Mistake! Collision incoming");
                  //       if (m_rnti == DebugNode)
                         checkIT->second.psschRsrpDb = 10*std::log10( std::pow (10.0,(checkIT->second.psschRsrpDb/10.0)) + std::pow(10.0,(sensedSFIt->second.psschRsrpDb/10.0)) );
                         checkIT->second.nodeId = 0; // Label collision
                     //    std::cin.get();
                      }
                      else 
                      {
                      //   NS_LOG_DEBUG("Inserting");
                         ReservedCSR newEntry = sensedSFIt->second;
                         newEntry.CreselRx -= j;
                         (*EXTmap_it).second.insert (std::pair<SidelinkCommResourcePool::SubframeInfo, ReservedCSR> (tmp, newEntry));
                   //      if (m_rnti == DebugNode)
                   //      std::cin.get();
                      }
                   }
                   else
                   {
                  //    NS_LOG_DEBUG("Inserting");
                      std::map<SidelinkCommResourcePool::SubframeInfo, ReservedCSR> value;
                      ReservedCSR newEntry = sensedSFIt->second;
                      newEntry.CreselRx -= j;
                      value.insert (std::pair<SidelinkCommResourcePool::SubframeInfo, ReservedCSR> (tmp, newEntry));
                      SensedMap_EXT.insert (std::pair<uint16_t,std::map<SidelinkCommResourcePool::SubframeInfo, ReservedCSR> > (sensedIt->first,value));   
                   }
                }
             } 
           } // end for (sensedIt = m_sensedReservedCSRMap.begin (); sensedIt != m_sensedReservedCSRMap.end (); sensedIt++)
       //    if (m_rnti == DebugNode)
       //    std::cin.get();

  
          
           if (m_rnti == DebugNode)
           NistLteUeMac::UnimorePrintSensedCSR(SensedMap_EXT);
 
       /*    NS_LOG_DEBUG("Print the extended list of sensed resources"); 
           for (sensedIt = SensedMap_EXT.begin (); sensedIt != SensedMap_EXT.end (); sensedIt++)
           {
             NS_LOG_DEBUG("CSR index: " << sensedIt->first);
             std::map<SidelinkCommResourcePool::SubframeInfo, ReservedCSR>::iterator sensedSFIt;    
             for (sensedSFIt = sensedIt->second.begin(); sensedSFIt != sensedIt->second.end(); sensedSFIt++)
               NS_LOG_DEBUG("Frame " << sensedSFIt->first.frameNo << " subframe " << sensedSFIt->first.subframeNo << " Cresel " << sensedSFIt->second.CreselRx << " RRI " << sensedSFIt->second.RRI );
             
           } */

       //    if (m_rnti == DebugNode)
       //      std::cin.get();

      //     std::ofstream removedFrames;
      //     removedFrames.open ("results/sidelink/removedFrames.txt", std::ios_base::app);

           // Now remove the resources from L1
           std::map<SidelinkCommResourcePool::SubframeInfo, ReservedCSR>::iterator candIt;
           for (std::map<uint16_t,std::list<SidelinkCommResourcePool::SubframeInfo>>::iterator L1it = L1.begin(); L1it != L1.end(); L1it++)
           {
       //      NS_LOG_INFO("L1 at CSR index " << L1it->first);
             uint16_t CurrentCSRindex = L1it->first;
             for(std::list<SidelinkCommResourcePool::SubframeInfo>::iterator FrameIT = L1it->second.begin(); FrameIT != L1it->second.end(); FrameIT++)
             {
               NS_LOG_DEBUG("Evaluating L1 frame " << FrameIT->frameNo << " subframe " << FrameIT->subframeNo << " P_rsvp " << p_rsvp);
               std::vector<SidelinkCommResourcePool::SubframeInfo> TxFrames;
               for (uint16_t i = 0; i < V2XGrant.m_Cresel; i++)
               {
                 SidelinkCommResourcePool::SubframeInfo tmp;
                 tmp.frameNo = (FrameIT->frameNo + (i*p_rsvp/10) )%1024;
                 tmp.subframeNo = FrameIT->subframeNo; 
                 NS_LOG_DEBUG("Next frame " << tmp.frameNo << " subframe " << tmp.subframeNo);
                 std::vector<double> RSRP_values; // posso farlo anche aggiungendo valori e poi dividendo per L_subch (faccio la media in automatico)(aggiungo zero se non c'è)
                 double RSRPmin = 10000000.0; //huge dB value
                 uint16_t PositiveValues = 0;
                 // Fill the vector of average RSRP
                 for (uint16_t j = 0; j < L_SubCh; j++)
                 {
                   RSRP_values.push_back(0.0); // in mW
                 }
                 for (uint16_t CSRindex = CurrentCSRindex; CSRindex < CurrentCSRindex + L_SubCh; CSRindex++)
                 {
                   candIt = SensedMap_EXT[CSRindex].find(tmp);
                   if (candIt != SensedMap_EXT[CSRindex].end() )
                   {
                     RSRP_values[CSRindex-CurrentCSRindex] = std::pow(10, candIt->second.psschRsrpDb/10);
                   }
                 // else it's a zero
                 } 
                 for (uint16_t CSRindex = CurrentCSRindex; CSRindex < CurrentCSRindex + L_SubCh; CSRindex++)
                 {
              //   NS_LOG_DEBUG("CSR index " << CSRindex-CurrentCSRindex << " RSRP value " << RSRP_values[CSRindex-CurrentCSRindex]);
              /*   if ((RSRP_values[CSRindex-CurrentCSRindex] > 0.0) && (10*std::log10(RSRP_values[CSRindex-CurrentCSRindex]) < RSRPmin))
                   {
                     RSRPmin = 10*std::log10(RSRP_values[CSRindex-CurrentCSRindex]);
                   }*/
                   if (RSRP_values[CSRindex-CurrentCSRindex] > 0.0)
                   {
                     ++PositiveValues;
                     if (10*std::log10(RSRP_values[CSRindex-CurrentCSRindex]) < RSRPmin)
                      RSRPmin = 10*std::log10(RSRP_values[CSRindex-CurrentCSRindex]);
                   }
                 } 
                 if (PositiveValues > 0)
                 {
           //      NS_LOG_DEBUG("The smallest RSRP is " << RSRPmin);
                   if (RSRPmin >= psschThresh)
                   {
              //       NS_LOG_DEBUG("Removed from L1, CSR index: " << CurrentCSRindex << " Frame: " << FrameIT->frameNo << " subframe: " << FrameIT->subframeNo);
               //      if (m_rnti == DebugNode)
                //      removedFrames << Simulator::Now().GetSeconds() << "," << m_rnti << "," << CurrentCSRindex << "," << FrameIT->frameNo << "," << FrameIT->subframeNo << std::endl;
                     FrameIT = L1it->second.erase(FrameIT);
                     FrameIT--;
           //          if (m_rnti == DebugNode)
           //         std::cin.get();
                     break;
                   }
             //   std::cin.get();
                 }          
               }
             } // end for(std::list<SidelinkCommResourcePool::SubframeInfo>::iterator FrameIT = L1it->second.begin(); FrameIT != L1it->second.end(); FrameIT++)
           } // end for (std::map<uint16_t,std::list<SidelinkCommResourcePool::SubframeInfo>>::iterator L1it = L1.begin(); L1it != L1.end(); L1++)
      //     removedFrames.close();
  //         if (m_rnti == DebugNode) 
  //         std::cin.get();

         } // end else
         

         // Print the list of CSRs (L1)
      /*   NS_LOG_DEBUG("Print the residual L1");
         for(std::map<uint16_t,std::list<SidelinkCommResourcePool::SubframeInfo> >::iterator L1it = L1.begin(); L1it != L1.end(); L1it++)
         {
            NS_LOG_DEBUG("CSR index " << L1it->first);
            for(std::list<SidelinkCommResourcePool::SubframeInfo>::iterator FrameIT = L1it->second.begin(); FrameIT != L1it->second.end(); FrameIT++)
             NS_LOG_DEBUG("Frame " << FrameIT->frameNo << " subframe " << FrameIT->subframeNo);
         }*/

     //    std::cin.get();
         NS_LOG_DEBUG("Clean the frames used for transmissions");
         // Remove old frames used for transmission
         std::list<std::pair<Time,SidelinkCommResourcePool::SubframeInfo>>::iterator pastTxIt;
         for (pastTxIt = m_pastTxUnimore.begin (); pastTxIt != m_pastTxUnimore.end (); pastTxIt++)
         {
           if (Simulator::Now().GetSeconds() - pastTxIt->first.GetSeconds() > 1.0 ) 
           {
             pastTxIt = m_pastTxUnimore.erase(pastTxIt);
             pastTxIt--;
           }         
         }

         // Print the frames used for past transmissions 
     /*  for (pastTxIt = m_pastTxUnimore.begin (); pastTxIt != m_pastTxUnimore.end (); pastTxIt++)
         {
           NS_LOG_DEBUG("Current time: " << Simulator::Now().GetSeconds() << " Tx Time: " << pastTxIt->first.GetSeconds() << " Frame: " << pastTxIt->second.frameNo << " subframe: " << pastTxIt->second.subframeNo);
         }
*/
         // Remove the frame used for past transmissions
         for (pastTxIt = m_pastTxUnimore.begin (); pastTxIt != m_pastTxUnimore.end (); pastTxIt++)
         {
            SidelinkCommResourcePool::SubframeInfo pastTxOffset;
            for (uint16_t j = 1; j <= 10; j++) 
            {
              pastTxOffset.subframeNo = pastTxIt->second.subframeNo + 0; // the subframe number is the same 
              pastTxOffset.frameNo = (pastTxIt->second.frameNo + (100 * j / 10)) % 1024;
//              NS_LOG_DEBUG("Checking frame: " << pastTxOffset.frameNo << " subframe " << pastTxOffset.subframeNo);
              for (std::map<uint16_t,std::list<SidelinkCommResourcePool::SubframeInfo>>::iterator L1it = L1.begin(); L1it != L1.end(); L1it++)
              { 
             //   NS_LOG_INFO("L1 at CSR index " << L1it->first);
                std::list<SidelinkCommResourcePool::SubframeInfo>::iterator FrameFinder;
                FrameFinder = std::find(L1it->second.begin(),L1it->second.end(),pastTxOffset);
                if (FrameFinder !=  L1it->second.end())
                {
              //     NS_LOG_DEBUG("Found! Removing Frame: " << FrameFinder->frameNo << " subframe: " << FrameFinder->subframeNo);
                   L1it->second.erase(FrameFinder);
                }

              } //end for (std::map<uint16_t,std::list<SidelinkCommResourcePool::SubframeInfo>>::iterator L1it = L1.begin(); L1it != L1.end(); L1it++)
            } // end for (uint16_t j = 1; j <= 10; j++) 

         }

        // std::cin.get();

   /*      NS_LOG_DEBUG("Print the residual L1");
         for(std::map<uint16_t,std::list<SidelinkCommResourcePool::SubframeInfo> >::iterator L1it = L1.begin(); L1it != L1.end(); L1it++)
         {
            NS_LOG_DEBUG("CSR index " << L1it->first);
            for(std::list<SidelinkCommResourcePool::SubframeInfo>::iterator FrameIT = L1it->second.begin(); FrameIT != L1it->second.end(); FrameIT++)
             NS_LOG_DEBUG("Frame " << FrameIT->frameNo << " subframe " << FrameIT->subframeNo);
         }*/

         nCSRresidual = ComputeResidualCSRs (L1);
         NS_LOG_INFO ("Initial L1 size " << nCSRtot << " Now, nCSRresidual: " << nCSRresidual);
         psschThresh += 3;
         iterationsCounter ++;
   //      std::cin.get();

      } // end while (nCSRresidual < L1targetSize * nCSRtot)

      // Save L1 in a text file
      if (m_rnti == DebugNode)
      {
        std::ofstream L1fileAlert;
        L1fileAlert.open ("results/sidelink/L1fileAlert.txt", std::ios_base::app);
        L1fileAlert << "-----Final L1------At time " << Simulator::Now().GetSeconds() << " Size " << nCSRresidual << "----------" << std::endl;
        for(std::map<uint16_t,std::list<SidelinkCommResourcePool::SubframeInfo> >::iterator L1it = L1.begin(); L1it != L1.end(); L1it++)
        {
            for(std::list<SidelinkCommResourcePool::SubframeInfo>::iterator FrameIT = L1it->second.begin(); FrameIT != L1it->second.end(); FrameIT++)
             L1fileAlert << "CSR index " << L1it->first << " Frame " << FrameIT->frameNo << " subframe " << FrameIT->subframeNo << std::endl;
        }
        L1fileAlert.close ();
      }

      if (m_List2Enabled)
      {
        std::map<uint32_t, CandidateCSRl2> L2;
        uint32_t L2EntryId = 0; // the unique identifier of each entry in list L2
        std::map<uint16_t,std::list<SidelinkCommResourcePool::SubframeInfo> >::iterator L1it; 
        std::list<SidelinkCommResourcePool::SubframeInfo>::iterator candSFIt;
        for (L1it = L1.begin (); L1it != L1.end (); L1it++)
        {
          uint16_t CurrentCSRindex = L1it->first;
       //   NS_LOG_DEBUG("Current L1 CSR index " << CurrentCSRindex);
          for (candSFIt = (*L1it).second.begin (); candSFIt != (*L1it).second.end (); candSFIt++)
          {
            // Evaluate the average RSSI observed during the past subframes
    //        NS_LOG_DEBUG("Evaluating frame: " << candSFIt->frameNo << " Subframe " << candSFIt->subframeNo);
    //        double sumRssi = 0;
    //        double avgRssi = 0;
            std::vector<double> RSRP_AVGs;
            for (uint16_t j = 0; j < L_SubCh; j++)
            {
               RSRP_AVGs.push_back(0.0); // in mW
            }
            for (uint16_t CSRindex = CurrentCSRindex; CSRindex < CurrentCSRindex + L_SubCh; CSRindex++)
            {
       //       NS_LOG_DEBUG("Evaluating CSR index " << CSRindex);
              std::map <uint16_t,std::map<SidelinkCommResourcePool::SubframeInfo, ReservedCSR> >::iterator sensedIt = m_sensedReservedCSRMap.find (CSRindex);
              if (sensedIt != m_sensedReservedCSRMap.end ())
              {
                for (uint32_t j = 1; j <= 10; j++)
                { 
                  double newContribution = 0.0;
                  SidelinkCommResourcePool::SubframeInfo offsetPastMeasured;
                  offsetPastMeasured.subframeNo = (j * 100) % 10;
                  offsetPastMeasured.frameNo = ((j * 100) / 10) % 1024;
             //    NS_LOG_DEBUG("Frame " << ((*candSFIt) - offsetPastMeasured).frameNo << " subframe " << ((*candSFIt) - offsetPastMeasured).subframeNo);
                  std::map<SidelinkCommResourcePool::SubframeInfo, ReservedCSR>::iterator sensedSFIt = (*sensedIt).second.find ((*candSFIt) - offsetPastMeasured);
                  if (sensedSFIt != (*sensedIt).second.end ()) 
                  {
                    double measuredRsrp =  (*sensedSFIt).second.psschRsrpDb;
                 //   NS_LOG_DEBUG("RSRP " << measuredRsrp);
                    double linearRsrpmW = std::pow (10, measuredRsrp / 10);
                    newContribution = linearRsrpmW;
                  }
                  RSRP_AVGs[CSRindex-CurrentCSRindex] += newContribution;
                } //end for (uint32_t j = 1; j <= 10; j++)
              } //end if (sensedIt != m_sensedReservedCSRMap.end ())
              else
              {
                // At the beginning of the simulation, the list of sensed resources is empty
                RSRP_AVGs[CSRindex-CurrentCSRindex] = 0;
              }
              RSRP_AVGs[CSRindex-CurrentCSRindex] /= 10;
            }
            double SUM = 0;
            for (uint16_t j = 0; j < L_SubCh; j++)
            {
               SUM += RSRP_AVGs[j]; // in mW
            }
            SUM /= L_SubCh;
         //   avgRssi = sumRssi / 10; // always assuming a length of 10 RBs for the TB
         //   avgRssi += 0;
            CandidateCSRl2 CSRL2;
            CSRL2.CSRIndex = (*L1it).first;
            CSRL2.rssi = SUM;  
            CSRL2.subframe = (*candSFIt);
            L2EntryId += 1;
            L2.insert (std::pair<uint32_t, CandidateCSRl2> (L2EntryId,CSRL2));
          } //end for (candSFIt = (*L1it).second.begin (); candSFIt != (*L1it).second.end (); candSFIt++)
        } //end for (L1it = L1.begin (); L1it != L1.end (); L1it++)
      //  NS_LOG_DEBUG("Finita L2");
        /*Now, the first version of L2 has been built
        We need to retain a number of entries equal to the 20% of the
        L1 size*/
        uint32_t targetL2Size = (uint32_t) (0.2 *  nCSRresidual);
    //    uint32_t targetL2Size = (uint32_t) (nCSRresidual);
        if (m_rnti == DebugNode)
        {
          std::ofstream L2fileAlert;
          L2fileAlert.open ("results/sidelink/L2fileAlert.txt", std::ios_base::app);
          L2fileAlert << "At " << Simulator::Now ().GetSeconds () << " L2 size: " << L2.size () << ", L1 size: " << nCSRresidual << ", target L2 size: " << targetL2Size << "\r\n" << "\r\n";
          std::map<uint32_t, CandidateCSRl2>::iterator L2ItDebug;
          for (L2ItDebug = L2.begin (); L2ItDebug != L2.end (); L2ItDebug++)
          {
             L2fileAlert << "- res. " << (*L2ItDebug).first << ": CSRindex: " << (int) (*L2ItDebug).second.CSRIndex << ", SF(" <<  (*L2ItDebug).second.subframe.frameNo << "," << (*L2ItDebug).second.subframe.subframeNo << "), RSSI: " <<  (*L2ItDebug).second.rssi << " mW" << "\r\n";
        //     NS_LOG_DEBUG("- res. " << (*L2ItDebug).first << ": CSRindex: " << (int) (*L2ItDebug).second.CSRIndex << ", SF(" <<  (*L2ItDebug).second.subframe.frameNo << "," << (*L2ItDebug).second.subframe.subframeNo << "), RSSI: " <<  (*L2ItDebug).second.rssi << " mW");
          }
          L2fileAlert.close ();
        //  std::cin.get();
        }
        //std::list<CandidateCSRl2> rssiCSRList;
        std::list<double> rssiList;
        std::map<uint32_t, CandidateCSRl2>::iterator L2It;
        for (L2It = L2.begin (); L2It != L2.end (); L2It++)
        {
          rssiList.push_back ((*L2It).second.rssi);
        }
        rssiList.sort ();
        rssiList.unique ();
        std::list<double>::iterator rssiIt;
        std::vector<CandidateCSRl2> L2EquivalentVector;
        for (rssiIt = rssiList.begin (); rssiIt != rssiList.end (); rssiIt++)
        {
          for (L2It = L2.begin (); L2It != L2.end (); L2It++)
          {
            if ((*rssiIt) == (*L2It).second.rssi)
            {
              L2EquivalentVector.push_back ((*L2It).second);
               //L2.erase (L2It);
            }
          }
        }
 
        std::vector<CandidateCSRl2>::iterator L2vecIt = L2EquivalentVector.begin ();
        double previousRssi = (*L2vecIt).rssi;
        for (L2vecIt = L2EquivalentVector.begin (); L2vecIt != L2EquivalentVector.end (); L2vecIt++)
        {
          if (finalL2.size () < targetL2Size || (finalL2.size () >= targetL2Size && (*L2vecIt).rssi == previousRssi))
          {
             previousRssi = (*L2vecIt).rssi;
             finalL2.push_back ((*L2vecIt));
          }
        } 
        if (m_rnti == DebugNode)
        {
          std::ofstream L2fileAlert;
          L2fileAlert.open ("results/sidelink/L2fileAlert.txt", std::ios_base::app);
          L2fileAlert << "At " << Simulator::Now ().GetSeconds () << " Final L2 size: " << finalL2.size () << ", L1 size: " << nCSRresidual << ", target L2 size: " << targetL2Size << "\r\n" << "\r\n";
          std::vector<CandidateCSRl2>::iterator L2ItDebug;
          for (L2ItDebug = finalL2.begin (); L2ItDebug != finalL2.end (); L2ItDebug++)
          {
             L2fileAlert << "CSRindex: " << (int) (*L2ItDebug).CSRIndex << ", SF(" <<  (*L2ItDebug).subframe.frameNo << "," << (*L2ItDebug).subframe.subframeNo << "), RSSI: " <<  (*L2ItDebug).rssi << " mW" << "\r\n";
      //       NS_LOG_DEBUG("CSRindex: " << (int) (*L2ItDebug).CSRIndex << ", SF(" <<  (*L2ItDebug).subframe.frameNo << "," << (*L2ItDebug).subframe.subframeNo << "), RSSI: " <<  (*L2ItDebug).rssi << " mW");
          }
          L2fileAlert.close ();
    //      std::cin.get();
        }  

      } // end if (List2Enabled)
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

        if (m_rnti == DebugNode)
        {
          std::ofstream L2fileAlert;
          L2fileAlert.open ("results/sidelink/L2fileAlert.txt", std::ios_base::app);
          L2fileAlert << "At " << Simulator::Now ().GetSeconds () << " Final L2 size: " << finalL2.size () << ", L1 size: " << nCSRresidual << ", target L2 size: " << nCSRresidual << "\r\n" << "\r\n";
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

   } // end if random selection


   // ------------------------- NEW IMPLEMENTATION (END)-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

   uint16_t selectedCSR;
   SidelinkCommResourcePool::SubframeInfo selectedSF;
   if (m_randomSelection)  
   { 
     subframeInitialTx = uniformRnd -> GetInteger (3, pdb);
   }
/*   else if (!List2Enabled)
   {
     std::vector<uint16_t> CSRVector;
     std::map<uint16_t,std::list<SidelinkCommResourcePool::SubframeInfo> >::iterator L1it;
     for (L1it = L1.begin (); L1it != L1.end (); L1it++)
     {
       if (L1it != L1.end ())
       {
          if ((*L1it).second.size () > 0)   //if there are residual subframes for this CSR index
          {
              CSRVector.push_back ((*L1it).first);
          }
       }
     }
     NS_LOG_DEBUG ("CSRVector.size () = " << CSRVector.size ());
     if (CSRVector.size () > 0)
     {
       selectedCSR = CSRVector[uniformRnd -> GetInteger (0, CSRVector.size () - 1)];
     }
     else
     { 
       std::ofstream alertFile;
       alertFile.open ("results/sidelink/alert.txt", std::ios_base::app);
       alertFile << Simulator::Now ().GetSeconds () << "s, " << "CSRVector.size () = " << CSRVector.size ();
       alertFile.close ();
       selectedCSR = 0;
     }
     L1it = L1.find (selectedCSR);
     if (L1it != L1.end ())
     {
       Ptr<UniformRandomVariable> uniformSF = CreateObject<UniformRandomVariable> ();
       //TODO create SF vector here
       std::list<SidelinkCommResourcePool::SubframeInfo>::iterator listIt;
       std::vector<SidelinkCommResourcePool::SubframeInfo> SFvec;
       for (listIt = (*L1it).second.begin (); listIt != (*L1it).second.end (); listIt++)
       {
             SFvec.push_back ((*listIt));  
       }
       selectedSF = SFvec [uniformSF -> GetInteger (0, SFvec.size () -1)]; 
     }
     SidelinkCommResourcePool::SubframeInfo SFoffset = selectedSF - currentSF;
     std::ofstream SFoffsetFile;
     SFoffsetFile.open("results/sidelink/SFOffset.csv", std::ios_base::app);
     SFoffsetFile << "Now: " << Simulator::Now ().GetSeconds () << " s, " << "Selected: (" << selectedSF.frameNo << "," << selectedSF.subframeNo << "), current: (" << currentSF.frameNo << "," << currentSF.subframeNo << "), Offset: (" << SFoffset.frameNo << "," << SFoffset.subframeNo << ")"<< "\r\n";
     SFoffsetFile.close ();
     subframeInitialTx = 10 * SFoffset.frameNo + SFoffset.subframeNo;
     NS_ASSERT (( subframeInitialTx % 10 == SFoffset.subframeNo) &&  (subframeInitialTx/10)%1024 == SFoffset.frameNo );
   } */
   else // if list L2 Enabled
   {
     Ptr<UniformRandomVariable> selectFromL2 = CreateObject<UniformRandomVariable> ();
     CandidateCSRl2 selectedResourceFromL2 = finalL2[selectFromL2 -> GetInteger (0, finalL2.size () - 1)];
     selectedCSR = selectedResourceFromL2.CSRIndex;
     selectedSF = selectedResourceFromL2.subframe;
     NS_LOG_DEBUG ("Selected Frame Number: " << selectedSF.frameNo << ", Subframe Number: " << selectedSF.subframeNo);
     //  std::cin.get();
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

     std::ofstream SFoffsetFile;
     SFoffsetFile.open("results/sidelink/SFOffset.csv", std::ios_base::app);
     SFoffsetFile << "Now: " << Simulator::Now ().GetSeconds () << " s, " << "Selected: (" << selectedSF.frameNo << "," << selectedSF.subframeNo << "), current: (" << currentSF.frameNo << "," << currentSF.subframeNo << "), Offset: (" << SFoffset.frameNo << "," << SFoffset.subframeNo << ")"<< "\r\n";
     SFoffsetFile.close ();
     subframeInitialTx = 10 * SFoffset.frameNo + SFoffset.subframeNo;
     NS_LOG_DEBUG("Subframe Initial Tx " << subframeInitialTx);
     NS_ASSERT (( subframeInitialTx % 10 == SFoffset.subframeNo) &&  (subframeInitialTx / 10) % 1024 == SFoffset.frameNo );
   } 

   std::ofstream initialTxFile;
   initialTxFile.open("results/sidelink/initialTx.csv", std::ios_base::app);
   initialTxFile << m_rnti << subframeInitialTx << ","<< (int) selectedCSR << "\r\n";
   initialTxFile.close ();
   V2XGrant.m_subframeInitialTx = subframeInitialTx;
   //FIXME adapt MCS based on the current needs
   /*
   uint32_t maxRbNumPerTb = 12;
   FIXME an indication of the required TB size (in bytes) must be provided by 
   a higher-layer parameter conveyed within the BSR and then passed to this function
   */
   V2XGrant.m_mcs = m_slGrantMcs;
   V2XGrant.m_tbSize = 0; //computed later
   V2XGrant.m_rbLenPssch = nbRbPssch;
   // compute subframe for next reserved transmission
   /*  V2XGrant.m_nextReservedSubframe = (subframeNo + p_rsvp) % 10;
   V2XGrant.m_nextReservedFrame = frameNo + (subframeNo + p_rsvp) / 10; */
   V2XGrant.m_nextReservedSubframe = (subframeNo + V2XGrant.m_subframeInitialTx) % 10 + 1;
   /*  if (V2XGrant.m_nextReservedSubframe == 0)
   {
     V2XGrant.m_nextReservedSubframe ++;
   }
   */

   V2XGrant.m_nextReservedFrame = (frameNo + (subframeNo + V2XGrant.m_subframeInitialTx) / 10) % 1024 + 1; 
   /*   if (m_rnti == 1)
   {
     std::cout << V2XGrant.m_nextReservedFrame << "\r\n";
   }*/
   /*  if (V2XGrant.m_nextReservedFrame == 0)
   {
     V2XGrant.m_nextReservedFrame ++;
   }
   */
   V2XGrant.m_pRsvpTx = p_rsvp;
   /* std::ofstream selectedFile;
   selectedFile.open("results/sidelink/selectedFile.txt", std::ios_base::app);
   selectedFile << "Now true: SF(" << currentSF.frameNo+1 << "," << currentSF.subframeNo+1 << ", Selected true: SF(" << V2XGrant.m_nextReservedFrame << "," << V2XGrant.m_nextReservedSubframe << ")" << "\r\n";
   selectedFile.close (); */
   if (m_randomSelection)
   { 
     V2XGrant.m_resPscch = m_ueSelectedUniformVariable->GetInteger (0, N_CSR_per_SF - 1); // m in TS 36.213; this can be also viewed as a CSR index                    
     V2XGrant.m_rbLenPscch = 2; //fixed and standardized
     V2XGrant.m_rbStartPscch =   V2XGrant.m_resPscch * nsubCHsize;
     if (V2XGrant.m_adjacency)
     {
        V2XGrant.m_resPssch = V2XGrant.m_resPscch;
        V2XGrant.m_rbStartPssch = V2XGrant.m_rbStartPscch + V2XGrant.m_rbLenPscch;
     }
     else
     {
        V2XGrant.m_resPssch = m_ueSelectedUniformVariable->GetInteger (0, N_CSR_per_SF - 1);
     }
   } 
   else 
   {
     V2XGrant.m_resPscch = selectedCSR;
     V2XGrant.m_rbLenPscch = 2; //fixed and standardized
     V2XGrant.m_rbStartPscch = V2XGrant.m_resPscch * nsubCHsize;
     if (V2XGrant.m_adjacency)
     {
        V2XGrant.m_resPssch = V2XGrant.m_resPscch;
        V2XGrant.m_rbStartPssch = V2XGrant.m_rbStartPscch + V2XGrant.m_rbLenPscch;
     }
     else  // NEVER USED: for now, just assume adjacency
     {
        V2XGrant.m_resPssch = V2XGrant.m_resPscch;
        V2XGrant.m_rbStartPssch = V2XGrant.m_rbStartPscch + V2XGrant.m_rbLenPscch; 
     }
   }
 /*  if (m_rnti == 1 && false)
   {
      sensingDebug.open ("results/sidelink/sensingDebug.txt", std::ios_base::app);
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
  // Fake GRANTs to force collisions
/*   if (m_rnti == 1)
   {
      V2XGrant.m_nextReservedFrame = 120;
      V2XGrant.m_nextReservedSubframe = 4;
      V2XGrant.m_rbStartPscch = 0;
      V2XGrant.m_rbStartPssch = V2XGrant.m_rbStartPscch + V2XGrant.m_rbLenPscch;
   }
   if (m_rnti == 2)
   {
      V2XGrant.m_nextReservedFrame = 110;
      V2XGrant.m_nextReservedSubframe = 4;
      V2XGrant.m_rbStartPscch = 0;
      V2XGrant.m_rbStartPssch = V2XGrant.m_rbStartPscch + V2XGrant.m_rbLenPscch;
   } 
   if (m_rnti == 3)
   {
      V2XGrant.m_nextReservedFrame = 120;
      V2XGrant.m_nextReservedSubframe = 4;
      V2XGrant.m_rbStartPscch = 24;
      V2XGrant.m_rbStartPssch = V2XGrant.m_rbStartPscch + V2XGrant.m_rbLenPscch;
   } 
   if (m_rnti == 4)
   {
      V2XGrant.m_nextReservedFrame = 120;
      V2XGrant.m_nextReservedSubframe = 4;
      V2XGrant.m_rbStartPscch = 12;
      V2XGrant.m_rbStartPssch = V2XGrant.m_rbStartPscch + V2XGrant.m_rbLenPscch;
   } 
   if (m_rnti == 5)
   {
      V2XGrant.m_nextReservedFrame = 110;
      V2XGrant.m_nextReservedSubframe = 4;
      V2XGrant.m_rbStartPscch = 24;
      V2XGrant.m_rbStartPssch = V2XGrant.m_rbStartPscch + V2XGrant.m_rbLenPscch;
   } 
   if (m_rnti == 6)
   {
      V2XGrant.m_nextReservedFrame = 110;
      V2XGrant.m_nextReservedSubframe = 4;
      V2XGrant.m_rbStartPscch = 36;
      V2XGrant.m_rbStartPssch = V2XGrant.m_rbStartPscch + V2XGrant.m_rbLenPscch;
   }*/


   NS_LOG_DEBUG(Simulator::Now ().GetSeconds () << " UE " << m_rnti << " Next reserved frame: " << V2XGrant.m_nextReservedFrame << ", next reserved subframe: " << V2XGrant.m_nextReservedSubframe << ", rbStartPSSCH " << (uint32_t)V2XGrant.m_rbStartPssch << ", rbLenPSSCH " << (uint32_t)V2XGrant.m_rbLenPssch);

   std::ofstream SSPSlogEXT;
   SSPSlogEXT.open ("results/sidelink/SSPSlog_EXT.txt", std::ios_base::app);
   SSPSlogEXT << " At time: " << Simulator::Now ().GetSeconds () << ", Node " << m_rnti << ", Number of iterations: " << iterationsCounter << ", PSSCH Threshold = " << psschThresh-3 << " dBm, L1 size = " << nCSRresidual << ", total CSRs: " << nCSRtot << ", Next reserved frame: " << V2XGrant.m_nextReservedFrame << ", next reserved subframe: " << V2XGrant.m_nextReservedSubframe << ", rbStartPSSCH: " << (uint32_t)V2XGrant.m_rbStartPssch << ", rbLenPSSCH: " << (uint32_t)V2XGrant.m_rbLenPssch << ", Cresel: " << V2XGrant.m_Cresel << ", RRI: " << (uint16_t)(V2XGrant.m_reservation*100) << std::endl;
   SSPSlogEXT.close();

   std::ofstream SSPSlog;
   SSPSlog.open ("results/sidelink/SSPSlog.txt", std::ios_base::app);
   SSPSlog << Simulator::Now ().GetSeconds () << "," << m_rnti << "," << iterationsCounter << "," << psschThresh-3 << "," << nCSRresidual << "," << nCSRtot << "," << V2XGrant.m_nextReservedFrame << "," << V2XGrant.m_nextReservedSubframe << "," << (uint32_t)V2XGrant.m_rbStartPssch << "," << (uint32_t)V2XGrant.m_rbLenPssch << "," << V2XGrant.m_Cresel << "," << (uint16_t)(V2XGrant.m_reservation*100) << std::endl;
   SSPSlog.close();

//   if (m_rnti == DebugNode)
//   {
//     std::cin.get();
 //    Simulator::ScheduleNow (&NistLteUeMac::UnimorePrintSensedCSR, this);
//   }

//    std::cin.get();
   return V2XGrant;
}


void
NistLteUeMac::UnimorePrintSensedCSR (std::map <uint16_t,std::map<SidelinkCommResourcePool::SubframeInfo, ReservedCSR> > SensedResources_Map)
{
  NS_LOG_FUNCTION(this);
  NS_LOG_DEBUG("Printing the list of sensed CSRs, at time: " << Simulator::Now ().GetSeconds ());
   
  std::ofstream sensingDebug;
  sensingDebug.open ("results/sidelink/UnimoreSensingDebug.txt", std::ios_base::app);
  sensingDebug << "--------------------------------------------------\r\n \r\n";
  sensingDebug << "Sensed Reservation List at RNTI " << m_rnti << " at time " << Simulator::Now ().GetSeconds () << ":\r\n";
  std::map <uint16_t,std::map<SidelinkCommResourcePool::SubframeInfo, ReservedCSR> >::iterator sensedIt;
  for (sensedIt = SensedResources_Map.begin (); sensedIt != SensedResources_Map.end (); sensedIt++)
  {
    sensingDebug << " CSR Index " <<  (int) (*sensedIt).first << " :\r\n";
    std::map<SidelinkCommResourcePool::SubframeInfo, ReservedCSR>::iterator sensedSFIt;
    for (sensedSFIt = (*sensedIt).second.begin (); sensedSFIt != (*sensedIt).second.end (); sensedSFIt++)
    {
      sensingDebug << "      SF(" << (*sensedSFIt).first.frameNo << "," << (*sensedSFIt).first.subframeNo << "), reception time: " << (*sensedSFIt).second.reservationTime << ", RSRP: " << (*sensedSFIt).second.psschRsrpDb << " dBm, RRI =  " << (*sensedSFIt).second.RRI << " ms, Cresel: " << (*sensedSFIt).second.CreselRx << ", Node ID: " << (*sensedSFIt).second.nodeId << std::endl;
   //   sensingDebug << "      SF(" << (*sensedSFIt).first.frameNo << "," << (*sensedSFIt).first.subframeNo << "), reception time: " << (*sensedSFIt).second.reservationTime << ", RSRP: " << (*sensedSFIt).second.psschRsrpDb << " dBm, rbStart: " << (*sensedSFIt).second.rbStart << ", rbLen " << (*sensedSFIt).second.rbLen <<  " :\r\n";
    }
  }
  sensingDebug.close();

 // Simulator::Schedule (Seconds(0.1), &NistLteUeMac::UnimorePrintSensedCSR, this);

}


void
NistLteUeMac::UnimoreUpdateReservation (V2XSidelinkGrant *V2Xgrant)
{
   V2Xgrant->m_Cresel--;
   NS_LOG_INFO("Cresel decremented to: " << V2Xgrant->m_Cresel);
   uint32_t SFtemp_new = V2Xgrant->m_nextReservedSubframe;
   SFtemp_new--;
   V2Xgrant->m_nextReservedSubframe = (SFtemp_new + V2Xgrant->m_pRsvpTx) % 10 + 1;
   V2Xgrant->m_nextReservedFrame --;
   V2Xgrant->m_nextReservedFrame = (V2Xgrant->m_nextReservedFrame + (SFtemp_new + V2Xgrant->m_pRsvpTx) / 10) % 1024 +1;
   SFtemp_new++;
   //Adjust for having frames and subframes starting from index 1
   if (V2Xgrant->m_nextReservedSubframe == 0)
   {
     V2Xgrant->m_nextReservedSubframe ++;
   }
   if (V2Xgrant->m_nextReservedFrame == 0)
   {
     V2Xgrant->m_nextReservedFrame ++;
   }
}


uint16_t
NistLteUeMac::GetSubchannelsNumber (uint16_t TBsize)
{
   uint16_t L_SubCh_TB = m_L_SubCh;
//m_nsubCHsize * m_L_SubCh
   if ((TBsize <= 1020) && (TBsize > 800))
     L_SubCh_TB = 5;
   else if ((TBsize <= 800) && (TBsize > 580))
     L_SubCh_TB = 4;
   else if ((TBsize <= 580) && (TBsize > 350))
     L_SubCh_TB = 3;
   else if ((TBsize <= 350) && (TBsize > 135))
     L_SubCh_TB = 2;
   else if (TBsize <= 135)
     L_SubCh_TB = 1;
   else
   {
     NS_ASSERT_MSG(false, "Invalid reservation size. Choose a value below 1020 Bytes");
   }
 
   return L_SubCh_TB;

}

void
NistLteUeMac::DoSubframeIndication (uint32_t frameNo, uint32_t subframeNo)
{
   NS_LOG_FUNCTION (this << " Frame no. " << frameNo << " subframe no. " << subframeNo);
   m_frameNo = frameNo;
   m_subframeNo = subframeNo;

   // TODO FIXME New for V2V

   bool aperiodicTraffic = false;
   uint32_t nodeId = 0;
   uint16_t ReselectionCounter, PacketSize;
   int ReservationDelay;
//   uint32_t tmpFrameNo, tmpSubframeNo;
//   uint32_t tmpFrameNo, tmpSubframeNo;
//   tmpFrameNo = 0;
//   tmpSubframeNo = 0;
   uint32_t RNGframe, RNGsubframe, tmpNextReservedFrame;

   Ptr<UniformRandomVariable> randomUniFrame = CreateObject<UniformRandomVariable> ();
   Ptr<UniformRandomVariable> randomUniSubframe = CreateObject<UniformRandomVariable> ();

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
   subframeNo += 4;
   if (subframeNo > 10)
   {
     ++frameNo;
     if (frameNo > 1024)
     {
       frameNo = 1;
       if (Simulator::Now ().GetSeconds () * 1000 > m_millisecondsFromLastAbsSFNUpdate + 1000.0) 
       //check whether the SFN cycle has not just been updated
       {
	 m_absSFN ++;
         m_millisecondsFromLastAbsSFNUpdate = Simulator::Now ().GetSeconds () * 1000;
       }
     }     
     subframeNo -= 10;
   }
   NS_LOG_INFO (this << " Adjusted Frame no. " << frameNo << " subframe no. " << subframeNo);
   
   //Discovery 
   //Check if this is a new Discovery Period
   if (frameNo == m_discTxPools.m_nextDiscPeriod.frameNo && subframeNo == m_discTxPools.m_nextDiscPeriod.subframeNo)
   {
     //define periods and frames
     m_discTxPools.m_currentDiscPeriod = m_discTxPools.m_nextDiscPeriod;
     m_discTxPools.m_nextDiscPeriod = m_discTxPools.m_pool->GetNextDiscPeriod (frameNo, subframeNo);
     m_discTxPools.m_nextDiscPeriod.frameNo++;
     m_discTxPools.m_nextDiscPeriod.subframeNo++;
     NS_LOG_INFO (this << " starting new discovery period " << ". Next period at " << m_discTxPools.m_nextDiscPeriod.frameNo << "/" << m_discTxPools.m_nextDiscPeriod.subframeNo);
   
     if (m_discTxPools.m_pool->GetSchedulingType() == SidelinkDiscResourcePool::UE_SELECTED) 
     {
       //use txProbability
       DiscGrant grant;
       double p1 = m_p1UniformVariable->GetValue (0, 1);
       double txProbability = m_discTxPools.m_pool->GetTxProbability (); //calculate txProbability
       if (p1 <= txProbability/100)
       {
         grant.m_resPsdch = m_resUniformVariable->GetInteger (0, m_discTxPools.m_npsdch-1);
         grant.m_rnti = m_rnti;
         m_discTxPools.m_nextGrant = grant;
         m_discTxPools.m_grant_received = true;
         NS_LOG_INFO (this << " UE selected grant: resource=" << (uint16_t) grant.m_resPsdch << "/" << m_discTxPools.m_npsdch);
       }
     }
     else //Scheduled
     {
      //TODO
      //use defined grant : SL-TF-IndexPair
     } 

     //If we received a grant
     if (m_discTxPools.m_grant_received)
     {
       m_discTxPools.m_currentGrant = m_discTxPools.m_nextGrant;
       NS_LOG_INFO (this << " Discovery grant received resource " << (uint32_t) m_discTxPools.m_currentGrant.m_resPsdch);  
       SidelinkDiscResourcePool::SubframeInfo tmp;
       tmp.frameNo = m_discTxPools.m_currentDiscPeriod.frameNo-1;
       tmp.subframeNo = m_discTxPools.m_currentDiscPeriod.subframeNo-1;
       m_discTxPools.m_psdchTx = m_discTxPools.m_pool->GetPsdchTransmissions (m_discTxPools.m_currentGrant.m_resPsdch);
       for (std::list<SidelinkDiscResourcePool::SidelinkTransmissionInfo>::iterator txIt = m_discTxPools.m_psdchTx.begin (); txIt != m_discTxPools.m_psdchTx.end (); txIt++)
       {
         txIt->subframe = txIt->subframe + tmp;
         //adjust for index starting at 1
         txIt->subframe.frameNo++;
         txIt->subframe.subframeNo++;
         NS_LOG_INFO (this << " PSDCH: Subframe " << txIt->subframe.frameNo << "/" << txIt->subframe.subframeNo << ": rbStart=" << (uint32_t) txIt->rbStart << ", rbLen=" << (uint32_t) txIt->nbRb);
         //std::cout <<  " PSDCH: Subframe " << txIt->subframe.frameNo << "/" << txIt->subframe.subframeNo << ": rbStart=" << (uint32_t) txIt->rbStart << ", rbLen=" << (uint32_t) txIt->nbRb << std::endl;
       }
       //Inform PHY: find a way to inform the PHY layer of the resources
       m_cphySapProvider->SetDiscGrantInfo (m_discTxPools.m_currentGrant.m_resPsdch);   
       //clear the grant
       m_discTxPools.m_grant_received = false;
     }
   } //END IF checking if it is a Discovery period

   std::list<SidelinkDiscResourcePool::SidelinkTransmissionInfo>::iterator allocIt;

   //Check if we need to transmit PSDCH (still Discovery operation)
   allocIt = m_discTxPools.m_psdchTx.begin();
   if (allocIt != m_discTxPools.m_psdchTx.end() && (*allocIt).subframe.frameNo == frameNo && (*allocIt).subframe.subframeNo == subframeNo)
   {
      NS_LOG_INFO (this << "PSDCH transmission");
      for (std::list<uint32_t>::iterator txApp = m_discTxApps.begin (); txApp != m_discTxApps.end (); ++txApp)
      {
        //Create Discovery message for each discovery application announcing
        NistSlDiscMsg discMsg;
        discMsg.m_rnti = m_rnti;
        discMsg.m_resPsdch = m_discTxPools.m_currentGrant.m_resPsdch;
        discMsg.m_proSeAppCode =  (std::bitset <184>)*txApp;
        Ptr<NistSlDiscMessage> msg = Create<NistSlDiscMessage> ();
        msg->SetSlDiscMessage (discMsg);
        NS_LOG_INFO ("discovery message sent by " << m_rnti << ", proSeAppCode = " << *txApp);
        m_discoveryAnnouncementTrace (m_rnti, *txApp);
        m_uePhySapProvider->SendNistLteControlMessage (msg);
     
      }
      m_discTxPools.m_psdchTx.erase (allocIt);
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
           NS_LOG_DEBUG("Node ID = " << nodeId);
           ReselectionCounter = itBsr->second.V2XReselectionCounter;
	   PacketSize = itBsr->second.V2XPacketSize;
	//   NS_LOG_DEBUG ("Reselection counter is " << ReselectionCounter);

	   break;
	 }//end if (itBsr->first.dstL2Id == poolIt->first)
       }// end for (itBsr = m_slBsrReceived.begin () ;  itBsr != m_slBsrReceived.end () ; itBsr++)
    
       if (itBsr == m_slBsrReceived.end () || (*itBsr).second.txQueueSize == 0)  // If there is no data to transmit
       {
	 NS_LOG_DEBUG (this << " no BSR received. Assume no data to transfer");
         if (poolIt->second.m_V2X_grant_received) // If the UE has a valid reservation
         {
           //NS_LOG_DEBUG("m_nextReservedFrame: " << poolIt->second.m_currentV2XGrant.m_nextReservedFrame);
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
             SFtemp--;
             poolIt->second.m_currentV2XGrant.m_nextReservedSubframe = (SFtemp + poolIt->second.m_currentV2XGrant.m_pRsvpTx) % 10 + 1;
             poolIt->second.m_currentV2XGrant.m_nextReservedFrame --;
             poolIt->second.m_currentV2XGrant.m_nextReservedFrame = (poolIt->second.m_currentV2XGrant.m_nextReservedFrame + (SFtemp + poolIt->second.m_currentV2XGrant.m_pRsvpTx) / 10) % 1024 +1;
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
             NS_LOG_UNCOND("Just updated reservation without data to Tx SF(" << poolIt->second.m_currentV2XGrant.m_nextReservedFrame << "," << poolIt->second.m_currentV2XGrant.m_nextReservedSubframe << ")");
             m_UnutilizedReservations++;
           //  std::cin.get();
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
	     V2XSidelinkGrant V2Xgrant; // check if we need to modify SidelinkGrant
             uint32_t pdb = (*itBsr).second.V2XPdb; // [ms]
             uint32_t p_rsvp = (*itBsr).second.V2XPrsvp; // the reservation period [ms]
             V2XSidelinkGrant processedV2Xgrant;
             processedV2Xgrant = V2XSelectResources (frameNo, subframeNo, V2Xgrant, pdb, p_rsvp, (*itBsr).second.V2XMessageType, itBsr->second.V2XTrafficType, ReselectionCounter, PacketSize, itBsr->second.V2XReservationSize); 
             if ((*itBsr).second.V2XMessageType == 0x01)
             {
               processedV2Xgrant.m_Cresel = 0; 
             }
             // Now assign the subframe!
             poolIt->second.m_nextV2XGrant = processedV2Xgrant;
             poolIt->second.m_V2X_grant_received = true;
             poolIt->second.m_V2X_grant_fresh = true;            
             if (m_rnti == 1 && false)
             {
               std::ofstream MAClog;
               MAClog.open ("results/sidelink/MACDebug.txt", std::ios_base::app);   
               MAClog << " V2X UE selected grant: PSCCH resource=" << (uint16_t) processedV2Xgrant.m_resPscch << ", rbStart=" << (uint16_t) processedV2Xgrant.m_rbStartPscch << ", rbLen=" << (uint16_t) processedV2Xgrant.m_rbLenPscch << ", mcs=" << (uint16_t) processedV2Xgrant.m_mcs << ", Cresel: " << processedV2Xgrant.m_Cresel << ", Next Reserved Tx: SF(" << processedV2Xgrant.m_nextReservedFrame << "," << processedV2Xgrant.m_nextReservedSubframe << ")" << "\r\n";
               MAClog.close ();
             }                       
             poolIt->second.m_nextV2XGrant.m_tbSize = m_amc->GetUlTbSizeFromMcs ((int) poolIt->second.m_nextV2XGrant.m_mcs, (int) poolIt->second.m_nextV2XGrant.m_rbLenPssch) / 8;
             NS_LOG_UNCOND("UE MAC: just made UE selection. Now: F:" << frameNo << ", SF:" << subframeNo);
           } //end if (!poolIt->second.m_V2X_grant_received || ((*itBsr).second.V2XMessageType == 0x01 && (*itBsr).second.isNewV2X))
         } //END OF if (!(*itBsr).second.alreadyUESelected && !(itBsr == m_slBsrReceived.end () || (*itBsr).second.txQueueSize == 0))
       } //END OF if (poolIt->second.m_pool->GetSchedulingType() == SidelinkCommResourcePool::UE_SELECTED)

       // Section for the aperiodic traffic  ------------------ APERIODIC ------------------------

       if (poolIt->second.m_V2X_grant_received && !(*itBsr).second.alreadyUESelected && !(itBsr == m_slBsrReceived.end () || (*itBsr).second.txQueueSize == 0) && aperiodicTraffic) // without frame boundary, now not needed
       {
	 NS_LOG_INFO("APERIODIC TRAFFIC: SF(" << frameNo << ", " << subframeNo << ") Node ID = " << nodeId);
	 // Change also the fresh grant
         if (poolIt->second.m_V2X_grant_fresh)
         { 
	   poolIt->second.m_currentV2XGrant = poolIt->second.m_nextV2XGrant;
	   m_aperiodicV2XGrant =  poolIt->second.m_currentV2XGrant;
	   // Need to make sure that no other transmission occurs before the first
         }
	 else
	 {
	   m_aperiodicV2XGrant = poolIt->second.m_currentV2XGrant;			
	 }
	 NS_LOG_DEBUG("Current Frame = " << frameNo << " Current Subframe = " << subframeNo);
	 NS_LOG_DEBUG("Send the packet: Reserved Frame = " << poolIt->second.m_currentV2XGrant.m_nextReservedFrame << " Reserved Subframe = " << poolIt->second.m_currentV2XGrant.m_nextReservedSubframe);
         ReservationDelay = (poolIt->second.m_currentV2XGrant.m_nextReservedFrame*10 + poolIt->second.m_currentV2XGrant.m_nextReservedSubframe-1) - (frameNo*10 + subframeNo-1);
         if (ReservationDelay < 0)  // handle the 1024 shift in the frame number
         {
          tmpNextReservedFrame = poolIt->second.m_currentV2XGrant.m_nextReservedFrame;
          poolIt->second.m_currentV2XGrant.m_nextReservedFrame += 1024;
          ReservationDelay = (poolIt->second.m_currentV2XGrant.m_nextReservedFrame*10 + poolIt->second.m_currentV2XGrant.m_nextReservedSubframe-1) - (frameNo*10 + subframeNo-1);
          poolIt->second.m_currentV2XGrant.m_nextReservedFrame = tmpNextReservedFrame;
         }
         NS_LOG_DEBUG("Reservation delay = " << ReservationDelay);
         //   std::cin.get();
         //poolIt->second.m_currentV2XGrant.m_nextReservedFrame = frameNo + 5ms
         //poolIt->second.m_currentV2XGrant.m_nextReservedSubframe = subframeNo + 5ms
	 (*itBsr).second.alreadyUESelected = true; //FIXME please change the name of this member
         //Compute the TB size
 	 poolIt->second.m_currentV2XGrant.m_tbSize = m_amc->GetUlTbSizeFromMcs ((int) poolIt->second.m_currentV2XGrant.m_mcs, (int) poolIt->second.m_currentV2XGrant.m_rbLenPssch) / 8;
         NS_LOG_DEBUG("TB size is "<<poolIt->second.m_currentV2XGrant.m_tbSize);

	 if (poolIt->second.m_V2X_grant_fresh)	
	 {	
           m_updateReservation = true;
	   NS_LOG_INFO("Fresh Grant");
	   //	poolIt->second.m_currentV2XGrant.m_nextReservedFrame = ++frameNo;  // Setting the Packet Delay Budget of the Aperiodic packet
	   //	poolIt->second.m_currentV2XGrant.m_nextReservedSubframe = subframeNo;
	   // poolIt->second.m_v2xTx = poolIt->second.m_pool->GetV2XSlTransmissions (frameNo, subframeNo, poolIt->second.m_currentV2XGrant.m_rbStartPscch, poolIt->second.m_currentV2XGrant.m_rbStartPssch, poolIt->second.m_currentV2XGrant.m_rbLenPssch, poolIt->second.m_currentV2XGrant.m_subframeInitialTx, poolIt->second.m_currentV2XGrant.m_SFGap);
           poolIt->second.m_v2xTx = poolIt->second.m_pool->GetV2XSlTransmissions (frameNo, subframeNo, poolIt->second.m_currentV2XGrant.m_rbStartPscch, poolIt->second.m_currentV2XGrant.m_rbStartPssch, poolIt->second.m_currentV2XGrant.m_rbLenPssch, poolIt->second.m_currentV2XGrant.m_nextReservedFrame, poolIt->second.m_currentV2XGrant.m_nextReservedSubframe, poolIt->second.m_currentV2XGrant.m_SFGap);
	   poolIt->second.m_V2X_grant_fresh = false; //we have just used this grant for the first time 
           m_validReservation = true; 
           m_Reservations++;
	 }
    //     else if (ReservationDelay > itBsr->second.V2XPdb || ReservationDelay < 4) // If the reservation does not respect the aperiodic traffic PDB
         else if ((uint16_t) ReservationDelay > itBsr->second.V2XPdb) // If the reservation does not respect the aperiodic traffic PDB
         {       
           m_updateReservation = false;  // Violated the PDB, this reservation will not be used

          /* if (ReservationDelay < 4) // Manually update the original grant
           { 
             UnimoreUpdateReservation(&m_aperiodicV2XGrant);
             if (m_aperiodicV2XGrant.m_Cresel == 0 ) //TODO FIXME 
             { 
               poolIt->second.m_V2X_grant_received = false; //GRANT HAS EXPIRED, next time will be recomputed
             } 
           }*/

           if (m_standardSSPS) // Select another resource using SSPS
           {
             NS_LOG_UNCOND("PDB violated, invoking SSPS for the selection of a new SSR!"); 
             //std::cin.get();
             m_LatencyReselections++;
             m_validReservation = true; 
 	     V2XSidelinkGrant tmpV2Xgrant_1; // check if we need to modify SidelinkGrant
             V2XSidelinkGrant tmpV2Xgrant;
             tmpV2Xgrant = V2XSelectResources (frameNo, subframeNo, tmpV2Xgrant_1, (uint32_t) (*itBsr).second.V2XPdb, (uint32_t)100, (*itBsr).second.V2XMessageType, itBsr->second.V2XTrafficType, 1 /*Reselection counter*/, itBsr->second.V2XPacketSize, itBsr->second.V2XPacketSize);  // Reserve resources only for this packet (one-shot transmission)
             NS_LOG_DEBUG("New resource selection needed!");
             NS_LOG_DEBUG("Next reserved frame " << tmpV2Xgrant.m_nextReservedFrame << " next subframe " << tmpV2Xgrant.m_nextReservedSubframe << " RbstartPSCCH " << (uint32_t) tmpV2Xgrant.m_rbStartPscch << " RbstartPSSCH " << (uint32_t) tmpV2Xgrant.m_rbStartPssch << " rbLenPSSCH " << (uint32_t) tmpV2Xgrant.m_rbLenPssch << " SFgap " << (uint8_t) tmpV2Xgrant.m_SFGap << "");
             poolIt->second.m_v2xTx = poolIt->second.m_pool->GetV2XSlTransmissions (frameNo, subframeNo, tmpV2Xgrant.m_rbStartPscch, tmpV2Xgrant.m_rbStartPssch, tmpV2Xgrant.m_rbLenPssch, tmpV2Xgrant.m_nextReservedFrame, tmpV2Xgrant.m_nextReservedSubframe, tmpV2Xgrant.m_SFGap);

           }
           else if (m_aggressive)  // choose another random resource within the PDB
           {
             NS_LOG_DEBUG("Aggressive scheduling for aperiodic traffic. Selecting a random SSR!");
             m_LatencyReselections++;
             m_validReservation = true; 
	     //     poolIt->second.m_currentV2XGrant.m_nextReservedFrame = ++frameNo % 1024;  // Setting the Packet Delay Budget of the Aperiodic packet
	     randomUniFrame->SetAttribute ("Min", DoubleValue (frameNo));
	     randomUniFrame->SetAttribute ("Max", DoubleValue ((itBsr->second.V2XPdb/10) + frameNo));
	     RNGframe = randomUniFrame->GetValue();
	     RNGframe = std::round(RNGframe);
             if (RNGframe == frameNo)
             {
               NS_LOG_DEBUG("Current frame number selected");
               if (subframeNo > 6)
	       {
	         RNGframe += 1;
                 randomUniSubframe->SetAttribute ("Min", DoubleValue (1));
	         randomUniSubframe->SetAttribute ("Max", DoubleValue (10)); 
               }
               else
	       {
                 randomUniSubframe->SetAttribute ("Min", DoubleValue (subframeNo+3));
	         randomUniSubframe->SetAttribute ("Max", DoubleValue (10));
               }
             }
             else
	     {
	       randomUniSubframe->SetAttribute ("Min", DoubleValue (1));
	       randomUniSubframe->SetAttribute ("Max", DoubleValue (10));
             }
	     RNGsubframe = randomUniSubframe->GetValue();
	     RNGsubframe = std::round(RNGsubframe);
	     NS_LOG_DEBUG("Aperiodic frame selection: Random frame number = " << RNGframe << " Random subframe number " << RNGsubframe);
	     // std::cin.get();
	     poolIt->second.m_currentV2XGrant.m_nextReservedFrame = RNGframe % 1024;  // Setting the Packet Delay Budget of the Aperiodic packet
	     // poolIt->second.m_currentV2XGrant.m_nextReservedFrame = ((itBsr->second.V2XPdb/10) + frameNo) % 1024;  // Setting the Packet Delay Budget of the Aperiodic packet
	     poolIt->second.m_currentV2XGrant.m_nextReservedSubframe = RNGsubframe;
             NS_LOG_INFO("Reused Grant, now: SF(" << frameNo << "," << subframeNo << ")");
             poolIt->second.m_v2xTx = poolIt->second.m_pool->GetV2XSlTransmissions (frameNo, subframeNo, poolIt->second.m_currentV2XGrant.m_rbStartPscch, poolIt->second.m_currentV2XGrant.m_rbStartPssch, poolIt->second.m_currentV2XGrant.m_rbLenPssch, poolIt->second.m_currentV2XGrant.m_nextReservedFrame, poolIt->second.m_currentV2XGrant.m_nextReservedSubframe, poolIt->second.m_currentV2XGrant.m_SFGap);
           } //end if (Aggressive)
           else if (m_submissive)   // drop the packet (submissive)
	   {
             NS_LOG_DEBUG("Submissive scheduling for aperiodic traffic. Dropping the packet!");
	     m_validReservation = false; 
	     poolIt->second.m_currentV2XGrant.m_nextReservedFrame = (frameNo + 1) % 1024;  
	     poolIt->second.m_currentV2XGrant.m_nextReservedSubframe = subframeNo;
             poolIt->second.m_v2xTx = poolIt->second.m_pool->GetV2XSlTransmissions (frameNo, subframeNo, poolIt->second.m_currentV2XGrant.m_rbStartPscch, poolIt->second.m_currentV2XGrant.m_rbStartPssch, poolIt->second.m_currentV2XGrant.m_rbLenPssch, poolIt->second.m_currentV2XGrant.m_nextReservedFrame, poolIt->second.m_currentV2XGrant.m_nextReservedSubframe, poolIt->second.m_currentV2XGrant.m_SFGap);
	   }
           else
           {
             NS_ASSERT_MSG(false,"No valid option for handling non-PDB compliant traffic");
           }
           
         }//end else if (ReservationDelay > itBsr->second.V2XPdb || ReservationDelay < 3)
         else // If the reservation respects the aperiodic traffic PDB
	 {
           NS_LOG_DEBUG("The reservation respects the aperiodic traffic PDB");
           m_updateReservation = true;
	   m_validReservation = true; 
           m_Reservations++;
           poolIt->second.m_v2xTx = poolIt->second.m_pool->GetV2XSlTransmissions (frameNo, subframeNo, poolIt->second.m_currentV2XGrant.m_rbStartPscch, poolIt->second.m_currentV2XGrant.m_rbStartPssch, poolIt->second.m_currentV2XGrant.m_rbLenPssch, poolIt->second.m_currentV2XGrant.m_nextReservedFrame, poolIt->second.m_currentV2XGrant.m_nextReservedSubframe, poolIt->second.m_currentV2XGrant.m_SFGap);
	 }
	 //	(*itBsr).second.txQueueSize = 0;
	 poolIt->second.m_currentV2XGrant = m_aperiodicV2XGrant; // backup

         if (m_updateReservation)
         {
    	   if (poolIt->second.m_currentV2XGrant.m_Cresel > 0)
           {
             poolIt->second.m_currentV2XGrant.m_Cresel--;
             //NS_LOG_DEBUG("Decreasing the reselection counter. Node " << m_rnti);
         //    std::cin.get();              
           }
	   uint32_t SFtemp = poolIt->second.m_currentV2XGrant.m_nextReservedSubframe;
           SFtemp--;
           poolIt->second.m_currentV2XGrant.m_nextReservedSubframe = (SFtemp + poolIt->second.m_currentV2XGrant.m_pRsvpTx) % 10 + 1;
	   poolIt->second.m_currentV2XGrant.m_nextReservedFrame --;
	   poolIt->second.m_currentV2XGrant.m_nextReservedFrame = (poolIt->second.m_currentV2XGrant.m_nextReservedFrame + (SFtemp + poolIt->second.m_currentV2XGrant.m_pRsvpTx) / 10) % 1024 + 1;
	   SFtemp++;
         }

         NS_LOG_UNCOND("Just updated reservation SF(" << poolIt->second.m_currentV2XGrant.m_nextReservedFrame << "," << poolIt->second.m_currentV2XGrant.m_nextReservedSubframe << ")");
	 m_tmpFrameNo = poolIt->second.m_currentV2XGrant.m_nextReservedFrame;
	 m_tmpSubframeNo = poolIt->second.m_currentV2XGrant.m_nextReservedSubframe;
       //  NS_LOG_UNCOND("tmpFrameNo " << m_tmpFrameNo << " tmpSubframeNo " << m_tmpSubframeNo);
	 for (std::list<SidelinkCommResourcePool::V2XSidelinkTransmissionInfo>::iterator txIt = poolIt->second.m_v2xTx.begin (); txIt != poolIt->second.m_v2xTx.end (); txIt++) 
	 {		
           txIt->subframe.frameNo++;
	   txIt->subframe.subframeNo++;
         }
         NS_LOG_INFO("poolIt->second.m_currentV2XGrant.m_Cresel: " << poolIt->second.m_currentV2XGrant.m_Cresel);
         if (poolIt->second.m_currentV2XGrant.m_Cresel == 0 ) //TODO FIXME 
         {  
           // Next time higher layers request TB transmission, a new reservation or Scheduling Request will be needed
           poolIt->second.m_V2X_grant_received = false; //grant has expired
           NS_LOG_INFO("Grant has expired");
         }
//	 std::cin.get();  //Pause the program for debugging purposes
       }
       //------------------IF NOT APERIODIC------------------------------------------
       else if (!aperiodicTraffic && poolIt->second.m_V2X_grant_received && !(*itBsr).second.alreadyUESelected && !(itBsr == m_slBsrReceived.end () || (*itBsr).second.txQueueSize == 0) && ((poolIt->second.m_currentV2XGrant.m_nextReservedFrame == frameNo && poolIt->second.m_currentV2XGrant.m_nextReservedSubframe == subframeNo) || (poolIt->second.m_V2X_grant_fresh && poolIt->second.m_nextV2XGrant.m_nextReservedFrame == frameNo && poolIt->second.m_nextV2XGrant.m_nextReservedSubframe == subframeNo))) // if I have new pending data and I have an active grant
       {    
         NS_LOG_INFO("Received grant for this subframe: SF(" << frameNo << ", " << subframeNo << ")");
         NS_LOG_DEBUG("PERIODIC TRAFFIC: Next reserved Frame = " << poolIt->second.m_currentV2XGrant.m_nextReservedFrame << " Next reserved Subframe = " << poolIt->second.m_currentV2XGrant.m_nextReservedSubframe);
         // Periodic traffic always works with valid reservations
         m_updateReservation = true;
	 m_validReservation = true; 
         m_Reservations++;
         if (m_rnti == 1 && false)
         {
           std::ofstream macGrant;
           macGrant.open ("results/sidelink/MACDebug.txt", std::ios_base::app);
           macGrant << "Received grant for this subframe: SF(" << frameNo << ", " << subframeNo << ")" << "\r\n";
           macGrant.close ();
         }
	 //Make the fresh grant our current grant. If it is not fresh, it means that the already assigned grant is ok
         if (poolIt->second.m_V2X_grant_fresh)
         { 
	   poolIt->second.m_currentV2XGrant = poolIt->second.m_nextV2XGrant;
         }
         (*itBsr).second.alreadyUESelected = true; //FIXME please change the name of this member
         //Compute the TB size
 	 poolIt->second.m_currentV2XGrant.m_tbSize = m_amc->GetUlTbSizeFromMcs ((int) poolIt->second.m_currentV2XGrant.m_mcs, (int) poolIt->second.m_currentV2XGrant.m_rbLenPssch) / 8;
      //   NS_LOG_UNCOND(" Size :" << poolIt->second.m_currentV2XGrant.m_tbSize);
      //   std::cin.get();
         if (poolIt->second.m_V2X_grant_fresh)
         {
           NS_LOG_INFO("Fresh Grant");
           // poolIt->second.m_v2xTx = poolIt->second.m_pool->GetV2XSlTransmissions (frameNo, subframeNo, poolIt->second.m_currentV2XGrant.m_rbStartPscch, poolIt->second.m_currentV2XGrant.m_rbStartPssch, poolIt->second.m_currentV2XGrant.m_rbLenPssch, poolIt->second.m_currentV2XGrant.m_subframeInitialTx, poolIt->second.m_currentV2XGrant.m_SFGap);
           poolIt->second.m_v2xTx = poolIt->second.m_pool->GetV2XSlTransmissions (frameNo, subframeNo, poolIt->second.m_currentV2XGrant.m_rbStartPscch, poolIt->second.m_currentV2XGrant.m_rbStartPssch, poolIt->second.m_currentV2XGrant.m_rbLenPssch, poolIt->second.m_currentV2XGrant.m_nextReservedFrame, poolIt->second.m_currentV2XGrant.m_nextReservedSubframe, poolIt->second.m_currentV2XGrant.m_SFGap);
           poolIt->second.m_V2X_grant_fresh = false; //we have just used this grant for the first time 
         }
         else
         { 
           NS_LOG_INFO("Reused Grant, now: SF(" << frameNo << "," << subframeNo << ")");
           poolIt->second.m_v2xTx = poolIt->second.m_pool->GetV2XSlTransmissions (frameNo, subframeNo, poolIt->second.m_currentV2XGrant.m_rbStartPscch, poolIt->second.m_currentV2XGrant.m_rbStartPssch, poolIt->second.m_currentV2XGrant.m_rbLenPssch, poolIt->second.m_currentV2XGrant.m_nextReservedFrame, poolIt->second.m_currentV2XGrant.m_nextReservedSubframe, poolIt->second.m_currentV2XGrant.m_SFGap);
         }
         //Update grant parameters
         // Try this: update only if the subframe corresponds to the previously reserved one
         if (poolIt->second.m_currentV2XGrant.m_Cresel > 0)
         {
           poolIt->second.m_currentV2XGrant.m_Cresel--;
         }
         uint32_t SFtemp = poolIt->second.m_currentV2XGrant.m_nextReservedSubframe;
         SFtemp--;
         poolIt->second.m_currentV2XGrant.m_nextReservedSubframe = (SFtemp + poolIt->second.m_currentV2XGrant.m_pRsvpTx) % 10 + 1;
         poolIt->second.m_currentV2XGrant.m_nextReservedFrame --;
         poolIt->second.m_currentV2XGrant.m_nextReservedFrame = (poolIt->second.m_currentV2XGrant.m_nextReservedFrame + (SFtemp + poolIt->second.m_currentV2XGrant.m_pRsvpTx) / 10) % 1024 + 1;
         SFtemp++;
         /*if (poolIt->second.m_currentV2XGrant.m_nextReservedSubframe == 0)
           {
             poolIt->second.m_currentV2XGrant.m_nextReservedSubframe ++;
           }
           if (poolIt->second.m_currentV2XGrant.m_nextReservedFrame == 0)
           {
             poolIt->second.m_currentV2XGrant.m_nextReservedFrame ++;
           }
         */
         NS_LOG_UNCOND("Just updated reservation SF(" << poolIt->second.m_currentV2XGrant.m_nextReservedFrame << "," << poolIt->second.m_currentV2XGrant.m_nextReservedSubframe << ")");
	 m_tmpFrameNo = poolIt->second.m_currentV2XGrant.m_nextReservedFrame;
	 m_tmpSubframeNo = poolIt->second.m_currentV2XGrant.m_nextReservedSubframe;
         // std::cin.get();
         for (std::list<SidelinkCommResourcePool::V2XSidelinkTransmissionInfo>::iterator txIt = poolIt->second.m_v2xTx.begin (); txIt != poolIt->second.m_v2xTx.end (); txIt++) 
	 {		
	   txIt->subframe.frameNo++;
	   txIt->subframe.subframeNo++;
         }
         NS_LOG_INFO("poolIt->second.m_currentV2XGrant.m_Cresel: " << poolIt->second.m_currentV2XGrant.m_Cresel);
         if (poolIt->second.m_currentV2XGrant.m_Cresel == 0 ) //TODO FIXME 
         {  
           // Next time higher layers request TB transmission, a new reservation or Scheduling Request will be needed
           poolIt->second.m_V2X_grant_received = false; //grant has expired
           NS_LOG_INFO("Grant has expired");
         }
       } //end else if (!aperiodicTraffic && poolIt->second.m_V2X_grant_received && !(*itBsr).second.alreadyUESelected && !(itBsr == m_slBsrReceived.end () || (*itBsr).second.txQueueSize == 0) && ((poolIt->second.m_currentV2XGrant.m_nextReservedFrame == frameNo && poolIt->second.m_currentV2XGrant.m_nextReservedSubframe == subframeNo) || (poolIt->second.m_V2X_grant_fresh && poolIt->second.m_nextV2XGrant.m_nextReservedFrame == frameNo && poolIt->second.m_nextV2XGrant.m_nextReservedSubframe == subframeNo)))

       std::list<SidelinkCommResourcePool::V2XSidelinkTransmissionInfo>::iterator allocIter;
       //check if we need to transmit PSCCH and PSSCH
       allocIter = poolIt->second.m_v2xTx.begin();
       //NS_LOG_UNCOND("Frame: " << frameNo << ", SF: " << subframeNo << ", AllocFrame: " << (*allocIter).subframe.frameNo << ", allocSubframe: " << (*allocIter).subframe.subframeNo);
       /*if (allocIter != poolIt->second.m_v2xTx.end())
         {
           NS_LOG_UNCOND("Now: "<<Simulator::Now().GetSeconds()*1000<<"ms: Ok, Should I Transmit Data?, Frame no. " <<frameNo<<", Subframe no. " << subframeNo << ", Allocated: SF:(" << (*allocIter).subframe.frameNo << "," << (*allocIter).subframe.subframeNo << ")");
         }*/
       // NS_LOG_UNCOND("(*allocIter).subframe.frameNo: " << (*allocIter).subframe.frameNo);
       if (allocIter != poolIt->second.m_v2xTx.end() && (*allocIter).subframe.frameNo == frameNo && (*allocIter).subframe.subframeNo  == subframeNo)
       {
         NS_LOG_UNCOND("Now: "<<Simulator::Now().GetSeconds()*1000 << " ms: Ok, now I should transmit data, Frame no. " <<frameNo<<", Subframe no. " << subframeNo);
         if (m_rnti == 1 && false)
         { 
           std::cout << "RNTI 1 is about to transmit\r\n";
           std::ofstream MACDebug; 
           MACDebug.open ("results/sidelink/MACDebug.txt", std::ios_base::app);
           MACDebug << "Allocated SF(" << ((*allocIter).subframe.frameNo)%1024 << "," << ((*allocIter).subframe.subframeNo)%10 << "), Current SF(" << frameNo << "," << subframeNo << ")" << "\r\n";
           MACDebug.close ();
         }
         //create SCI message
	 NistV2XSciListElement_s sci1;
	 sci1.m_rnti = m_rnti;
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
         sci1.m_rbStart = poolIt->second.m_currentV2XGrant.m_rbStartPssch;
         sci1.m_rbLen = poolIt->second.m_currentV2XGrant.m_rbLenPssch;
	 sci1.m_rbLen_TB =  (uint8_t)(GetSubchannelsNumber(itBsr->second.V2XPacketSize)*m_nsubCHsize) - 2 ;
 
         /*Maybe not really needed*/
         sci1.m_rbStartPscch = poolIt->second.m_currentV2XGrant.m_rbStartPscch;
         sci1.m_rbLenPscch = poolIt->second.m_currentV2XGrant.m_rbLenPscch;
 	 sci1.m_mcs = poolIt->second.m_currentV2XGrant.m_mcs;
	 sci1.m_tbSize = poolIt->second.m_currentV2XGrant.m_tbSize;
	 sci1.m_groupDstId = (poolIt->first & 0xFF);
         sci1.m_SFGap = poolIt->second.m_currentV2XGrant.m_SFGap;
         if (poolIt->second.m_currentV2XGrant.m_Cresel > 0)
         {
           if (poolIt->second.m_currentV2XGrant.m_pRsvpTx >= 100)
           {
             sci1.m_reservation = (uint8_t) (poolIt->second.m_currentV2XGrant.m_pRsvpTx * 0.01);
           }
           else if (poolIt->second.m_currentV2XGrant.m_pRsvpTx == 50)
           {
             sci1.m_reservation = 0x0b;
           }
           else if (poolIt->second.m_currentV2XGrant.m_pRsvpTx == 20)
           {
             sci1.m_reservation = 0x0c;
           }
         }
         else
         {
           sci1.m_reservation = 0; // Won't reserve resources
         }
         SidelinkCommResourcePool::SubframeInfo currentSF;
         SidelinkCommResourcePool::SubframeInfo reservationSF;
         currentSF.frameNo = frameNo -1;  //Current time is always 1 frame and 1 subframe ahead
         currentSF.subframeNo = subframeNo -1;
         // The reserved resources are computed twice??
         reservationSF.subframeNo = poolIt->second.m_currentV2XGrant.m_pRsvpTx % 10 ;
         reservationSF.frameNo = (poolIt->second.m_currentV2XGrant.m_pRsvpTx / 10) % 1024 ;
         sci1.m_reservedSubframe = currentSF + reservationSF;
         // sci1.m_reservedSubframe.frameNo ++;
         // sci1.m_reservedSubframe.subframeNo ++;
	 sci1.m_reservedSubframe.frameNo = m_tmpFrameNo;
         sci1.m_reservedSubframe.subframeNo = m_tmpSubframeNo;
         currentSF.frameNo ++;
         currentSF.subframeNo ++;
         sci1.m_reTxIndex = (*allocIter).isThisAReTx;
         sci1.m_CreselRx = poolIt->second.m_currentV2XGrant.m_Cresel;
         Ptr<SciV2XLteControlMessage> msg = Create<SciV2XLteControlMessage> ();

         // Print the MAC debugger counters (for ETSI traffic)
         if (Simulator::Now ().GetSeconds() - m_prevPrintTime > 0.25)
         {
           m_prevPrintTime = Simulator::Now ().GetSeconds();
           std::ofstream DebugTMP; 
           DebugTMP.open ("results/sidelink/ReservationsDebug.txt", std::ios_base::app);
           DebugTMP << m_rnti << "," << Simulator::Now ().GetSeconds() << "," << m_UnutilizedReservations << "," << m_Reservations << "," << m_LatencyReselections << "," << m_CounterReselections << "," << m_TotalTransmissions << std::endl;
           DebugTMP.close ();
           m_UnutilizedReservations = 0;
           m_Reservations = 0;
           m_LatencyReselections = 0;
           m_CounterReselections = 0;
           m_TotalTransmissions = 0;
//	   std::cin.get();
         }

         if (!m_updateReservation)
         {
           sci1.m_reservation = 0; // Notify other neighbors that these resource won't be reserved
         }
	 NS_LOG_DEBUG("SCI >> Node ID: " << nodeId << " Cresel: " << sci1.m_CreselRx << " rbStart PSSCH: " << (int)sci1.m_rbStart << " rbLen PSSCH (reserved): " << (int)sci1.m_rbLen << " rbLen PSSCH (used): " << (int)sci1.m_rbLen_TB << " rbStart PSCCH: " << (int)sci1.m_rbStartPscch << " rbLen PSCCH: " << (int)sci1.m_rbLenPscch << " Next Frame: " <<  sci1.m_reservedSubframe.frameNo << " Next subframe: " << sci1.m_reservedSubframe.subframeNo << " Reservation " << (uint16_t)sci1.m_reservation);

         //   if (m_rnti==1)
//         std::cin.get();

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
		 uint32_t bytesForThisLc = poolIt->second.m_currentV2XGrant.m_tbSize;
		 NS_LOG_LOGIC (this << " RNTI " << m_rnti << " Sidelink Tx " << bytesForThisLc << " bytes to LC " << (uint32_t)(*itBsr).first.lcId << " statusQueue " << (*itBsr).second.statusPduSize << " retxQueue" << (*itBsr).second.retxQueueSize << " txQueue" <<  (*itBsr).second.txQueueSize);
                 //  std::cin.get();
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
       //Check if this is a new SC period
       if (frameNo == poolIt->second.m_nextScPeriod.frameNo && subframeNo == poolIt->second.m_nextScPeriod.subframeNo)
       {
	 poolIt->second.m_currentScPeriod = poolIt->second.m_nextScPeriod;
	 poolIt->second.m_nextScPeriod = poolIt->second.m_pool->GetNextScPeriod (frameNo, subframeNo);
	 //adjust because scheduler starts with frame/subframe = 1
	 poolIt->second.m_nextScPeriod.frameNo++;
	 poolIt->second.m_nextScPeriod.subframeNo++;
	 NS_LOG_INFO (this << " Starting new SC period for pool of group " << poolIt->first << ". Next period at " << poolIt->second.m_nextScPeriod.frameNo << "/" << poolIt->second.m_nextScPeriod.subframeNo);
         Ptr<PacketBurst> emptyPb = CreateObject <PacketBurst> ();
	 poolIt->second.m_miSlHarqProcessPacket = emptyPb;
         if (poolIt->second.m_pool->GetSchedulingType() == SidelinkCommResourcePool::UE_SELECTED)
	 {    
           //If m_slHasDataToTx is False here (at the beginning of the period), it means
	   //that no transmissions in the PSSCH occurred in the previous SC period.
	   //Notify the RRC for stopping SLSS transmissions if appropriate
	   if (!m_slHasDataToTx)
           {
	     m_cmacSapUser->NotifyMacHasNotSlDataToSend();
	   }
	   //Make m_slHasDataToTx = false here (beginning of the period) to detect if transmissions
	   //in the PSSCH are performed in this period
	   m_slHasDataToTx=false;
           //get the BSR for this pool
	   //if we have data in the queue
	   //find the BSR for that pool (will also give the SidleinkLcIdentifier)
	   std::map <SidelinkLcIdentifier, NistLteMacSapProvider::NistReportBufferNistStatusParameters>::iterator itBsr;
	   for (itBsr = m_slBsrReceived.begin () ;  itBsr != m_slBsrReceived.end () ; itBsr++)
	   {
	     if (itBsr->first.dstL2Id == poolIt->first)
	     {
	       //this is the BSR for the pool
	       break;
	     }
           }
           if (itBsr == m_slBsrReceived.end () || (*itBsr).second.txQueueSize == 0)
	   {
	     NS_LOG_INFO (this << " no BSR received. Assume no data to transfer");
           }
	   else  //that is, if UE needs to transmit data during this SC Period,
	   {   //NS_LOG_UNCOND("txQueueSize: " << (*itBsr).second.txQueueSize);
	     //we need to pick a random resource from the pool (D2D Mode 2)
	     //NS_ASSERT_MSG (0, "UE_SELECTED pools not implemented");
	     NS_LOG_DEBUG (this << "SL BSR size=" << m_slBsrReceived.size ());
	     SidelinkGrant grant;
	     //in order to pick a resource that is valid, we compute the number of subchannels
	     //on the PSSCH
	     NS_ASSERT_MSG (m_pucchSize % 2 == 0, "Number of RBs for PUCCH must be multiple of 2");
	     //TODO: add function so the RRC tells the MAC what the UL bandwidth is.
	     //currently only the phy has it
	     uint16_t nbSubchannels = std::floor ((50 - m_pucchSize) / m_slGrantSize);
	     uint16_t nbTxOpt = poolIt->second.m_npscch;//before was (-1)
             grant.m_resPscch = m_ueSelectedUniformVariable->GetInteger (0, nbTxOpt-1); //Randomly selected Resource in PSCCH.
	     grant.m_tpc = 0;
	     grant.m_hopping = 0; //Assume no frequency hopping
	     uint16_t subCh = 0;
	     subCh = m_ueSelectedUniformVariable->GetInteger (0, nbSubchannels-1);
	     switch (m_slKtrp)
	     {
	       case 1:
		grant.m_trp = m_ueSelectedUniformVariable->GetInteger (0, 7);
		break;
	       case 2:
		grant.m_trp = m_ueSelectedUniformVariable->GetInteger (8, 35);
		break;
	       case 4:
		grant.m_trp = m_ueSelectedUniformVariable->GetInteger (36, 105);
		break;
	       case 8:
		grant.m_trp = 106;
		break;
	       default:
		NS_FATAL_ERROR ("Invalid KTRP value " << (uint16_t) m_slKtrp << ". Supported values: [1, 2, 4, 8]");
	     }
             grant.m_rbStart = m_pucchSize / 2 + m_slGrantSize * subCh;
	     grant.m_rbLen = m_slGrantSize;
             //grant.m_trp = (uint16_t) std::floor (grant.m_resPscch / nbSubchannels)/*m_slItrp*/;
	     grant.m_mcs = m_slGrantMcs;
	     grant.m_tbSize = 0; //computed later
	     poolIt->second.m_nextGrant = grant;
	     poolIt->second.m_grant_received = true;
	     NS_LOG_INFO (this << " UE selected grant: resource=" << (uint16_t) grant.m_resPscch << "/" << poolIt->second.m_npscch << ", rbStart=" << (uint16_t) grant.m_rbStart << ", rbLen=" << (uint16_t) grant.m_rbLen << ", mcs=" << (uint16_t) grant.m_mcs << ", ch=" << subCh << ",itrp=" << (uint16_t) grant.m_trp);
             //std::cout << this << " UE selected grant: resource=" << (uint16_t) grant.m_resPscch << "/" << poolIt->second.m_npscch << ", rbStart=" << (uint16_t) grant.m_rbStart << ", rbLen=" << (uint16_t) grant.m_rbLen << ", mcs=" << (uint16_t) grant.m_mcs << ", ch=" << subCh << ",itrp=" << (uint16_t) grant.m_trp << "\r\n";
             /* // Trace SL UE mac scheduling
	     NistSlUeMacStatParameters stats_params;
	     stats_params.m_timestamp = Simulator::Now ().GetMilliSeconds ();
	     stats_params.m_frameNo = frameNo;
	     stats_params.m_subframeNo = subframeNo;
	     stats_params.m_rnti = m_rnti;
	     stats_params.m_mcs = grant.m_mcs;
	     stats_params.m_pscchRi = grant.m_resPscch;
	     stats_params.m_pscchTx1 = 1; //NEED to obtain SF of first Tx in PSCCH!!!!!!!!!!!!!!
	     stats_params.m_pscchTx2 = 2; //NEED to obtain SF of second Tx in PSCCH!!!!!!!!!!!!!!
	     stats_params.m_psschTxStartRB = grant.m_rbStart;
	     stats_params.m_psschTxLengthRB = grant.m_rbLen;
	     stats_params.m_psschItrp = grant.m_trp;
             m_slUeScheduling (stats_params);
             */
             // trace File 
             std::ofstream peekFile;
             peekFile.open("results/sidelink/mode2.unimo", std::ios_base::app);
             peekFile << this << " UE selected grant: resource=" << (uint16_t) grant.m_resPscch << "/" << poolIt->second.m_npscch << ", rbStart=" << (uint16_t) grant.m_rbStart << ", rbLen=" << (uint16_t) grant.m_rbLen << ", mcs=" << (uint16_t) grant.m_mcs << ", ch=" << subCh << ",itrp=" << (uint16_t) grant.m_trp << ", pool type: " << poolIt->second.m_pool->GetSchedulingType() << "\r\n";
             peekFile.close();
           }
         } //end if (poolIt->second.m_pool->GetSchedulingType() == SidelinkCommResourcePool::UE_SELECTED)

         //if we received a grant, compute the transmission opportunities for PSCCH and PSSCH
	 if (poolIt->second.m_grant_received) 
         {
	   //make the grant our current grant
	   poolIt->second.m_currentGrant = poolIt->second.m_nextGrant;
           NS_LOG_INFO (this << " Sidelink grant received resource " << (uint32_t) poolIt->second.m_currentGrant.m_resPscch);
           SidelinkCommResourcePool::SubframeInfo tmp;
	   tmp.frameNo = poolIt->second.m_currentScPeriod.frameNo-1;
	   tmp.subframeNo = poolIt->second.m_currentScPeriod.subframeNo-1;
           // Collect statistics for SL UE mac scheduling trace
	   NistSlUeMacStatParameters stats_params;
	   stats_params.m_frameNo = tmp.frameNo+1;
	   stats_params.m_subframeNo = tmp.subframeNo+1;
	   stats_params.m_pscchRi = poolIt->second.m_currentGrant.m_resPscch;
	   stats_params.m_cellId = 0;
	   stats_params.m_imsi = 0 ;
	   stats_params.m_pscchFrame1 = 0;
	   stats_params.m_pscchSubframe1 = 0;
	   stats_params.m_pscchFrame2 = 0;
	   stats_params.m_pscchSubframe2 = 0;
	   stats_params.m_psschFrame = 0;
	   stats_params.m_psschSubframeStart = 0;
           poolIt->second.m_pscchTx = poolIt->second.m_pool->GetPscchTransmissions (poolIt->second.m_currentGrant.m_resPscch);
	   uint16_t tx_counter = 1;
	   for (std::list<SidelinkCommResourcePool::SidelinkTransmissionInfo>::iterator txIt = poolIt->second.m_pscchTx.begin (); txIt != poolIt->second.m_pscchTx.end (); txIt++) // 'first' and 'second' transmission
	   {
	     txIt->subframe = txIt->subframe + tmp;
	     //adjust for index starting at 1
	     txIt->subframe.frameNo++;
	     txIt->subframe.subframeNo++;
	     NS_LOG_INFO (this << " PSCCH: Subframe " << txIt->subframe.frameNo << "/" << txIt->subframe.subframeNo << ": rbStart=" << (uint32_t) txIt->rbStart << ", rbLen=" << (uint32_t) txIt->nbRb);
	     switch (tx_counter)
             {
	      case 1:
		stats_params.m_pscchFrame1 = txIt->subframe.frameNo;
		stats_params.m_pscchSubframe1 = txIt->subframe.subframeNo;
		break;
	      case 2:
		stats_params.m_pscchFrame2 = txIt->subframe.frameNo;
		stats_params.m_pscchSubframe2 = txIt->subframe.subframeNo;
		break;
	      default:
		NS_FATAL_ERROR(this << "PSCCH ONLY SUPPORTS 2 TRANSMISSIONS PER UE GRANT!");
	     }
	     tx_counter++; 
           }
           poolIt->second.m_psschTx = poolIt->second.m_pool->GetPsschTransmissions (tmp, poolIt->second.m_currentGrant.m_trp, poolIt->second.m_currentGrant.m_rbStart, poolIt->second.m_currentGrant.m_rbLen);
	   //adjust PSSCH frame to next period
	   for (std::list<SidelinkCommResourcePool::SidelinkTransmissionInfo>::iterator txIt = poolIt->second.m_psschTx.begin (); txIt != poolIt->second.m_psschTx.end (); txIt++)
	   {
	     //txIt->subframe = txIt->subframe + tmp;
	     //adjust for index starting at 1
	     txIt->subframe.frameNo++;
	     txIt->subframe.subframeNo++;
	     NS_LOG_INFO (this << " PSSCH: Subframe " << txIt->subframe.frameNo << "/" << txIt->subframe.subframeNo << ": rbStart=" << (uint32_t) txIt->rbStart << ", rbLen=" << (uint32_t) txIt->nbRb);
           }

	   //compute the tb size
	   poolIt->second.m_currentGrant.m_tbSize = m_amc->GetUlTbSizeFromMcs (poolIt->second.m_currentGrant.m_mcs, poolIt->second.m_currentGrant.m_rbLen) / 8;
	   NS_LOG_INFO ("Sidelink Tb size = " << poolIt->second.m_currentGrant.m_tbSize << " bytes (mcs=" << (uint32_t) poolIt->second.m_currentGrant.m_mcs << ")");
           stats_params.m_rnti = m_rnti;
	   stats_params.m_mcs = poolIt->second.m_currentGrant.m_mcs;
	   stats_params.m_tbSize = poolIt->second.m_currentGrant.m_tbSize;
	   stats_params.m_psschTxStartRB = poolIt->second.m_currentGrant.m_rbStart;
	   stats_params.m_psschTxLengthRB = poolIt->second.m_currentGrant.m_rbLen;
	   stats_params.m_psschItrp = poolIt->second.m_currentGrant.m_trp;
	   stats_params.m_timestamp = Simulator::Now ().GetMilliSeconds ();
           // Call trace
	   m_slUeScheduling (stats_params);
           //clear the grant
	   poolIt->second.m_grant_received = false;
           //NS_LOG_UNCOND("SF n. " << subframeNo << ", MCS: " << (int) poolIt->second.m_currentGrant.m_mcs);
	 } //end if(poolIt->second.m_grant_received)
       } //end if (this is the beginning of a new SC Period)
 
       std::list<SidelinkCommResourcePool::SidelinkTransmissionInfo>::iterator allocIt;
       //check if we need to transmit PSCCH
       allocIt = poolIt->second.m_pscchTx.begin();
       if (allocIt != poolIt->second.m_pscchTx.end() && (*allocIt).subframe.frameNo == frameNo && (*allocIt).subframe.subframeNo == subframeNo)
       {
	 //transmission of PSCCH, no need for HARQ
	 if (poolIt->second.m_pscchTx.size () == 2)
         {
	   NS_LOG_INFO (this << " First PSCCH transmission");
	 }
         else 
         {
	   NS_LOG_INFO (this << " Second PSCCH transmission");
	 }
	 //create SCI message
	 NistSciListElement_s sci;
	 sci.m_rnti = m_rnti;
	 sci.m_resPscch = poolIt->second.m_currentGrant.m_resPscch;
	 sci.m_rbStart = poolIt->second.m_currentGrant.m_rbStart;
	 sci.m_rbLen = poolIt->second.m_currentGrant.m_rbLen;
	 sci.m_trp = poolIt->second.m_currentGrant.m_trp;
	 sci.m_mcs = poolIt->second.m_currentGrant.m_mcs;
	 sci.m_tbSize = poolIt->second.m_currentGrant.m_tbSize;
	 sci.m_groupDstId = (poolIt->first & 0xFF);
         Ptr<SciNistLteControlMessage> msg = Create<SciNistLteControlMessage> ();
	 msg->SetSci (sci);
	 m_uePhySapProvider->SendNistLteControlMessage (msg);
         poolIt->second.m_pscchTx.erase (allocIt);
       } //end if (allocIt != poolIt->second.m_pscchTx.end() && (*allocIt).subframe.frameNo == frameNo && (*allocIt).subframe.subframeNo == subframeNo)

       //check if we need to transmit PSSCH
       allocIt = poolIt->second.m_psschTx.begin();
       if (allocIt != poolIt->second.m_psschTx.end() && (*allocIt).subframe.frameNo == frameNo && (*allocIt).subframe.subframeNo == subframeNo)
       {
	 // Collect statistics for SL shared channel UE mac scheduling trace
	 NistSlUeMacStatParameters stats_sch_params;
	 stats_sch_params.m_frameNo = poolIt->second.m_currentScPeriod.frameNo;
	 stats_sch_params.m_subframeNo = poolIt->second.m_currentScPeriod.subframeNo;
	 stats_sch_params.m_psschFrame = frameNo;
	 stats_sch_params.m_psschSubframe = subframeNo;
	 stats_sch_params.m_cellId = 0;
	 stats_sch_params.m_imsi = 0 ;
	 stats_sch_params.m_pscchRi = 0 ;
	 stats_sch_params.m_pscchFrame1 = 0;
	 stats_sch_params.m_pscchSubframe1 = 0;
	 stats_sch_params.m_pscchFrame2 = 0;
	 stats_sch_params.m_pscchSubframe2 = 0;
	 stats_sch_params.m_psschItrp = 0;
	 stats_sch_params.m_psschFrameStart = 0;
	 stats_sch_params.m_psschSubframeStart = 0;
         //Get first subframe of PSSCH
	 SidelinkCommResourcePool::SubframeInfo currScPeriod;
	 currScPeriod.frameNo = poolIt->second.m_currentScPeriod.frameNo-1;
	 currScPeriod.subframeNo = poolIt->second.m_currentScPeriod.subframeNo-1;
         std::list<SidelinkCommResourcePool::SidelinkTransmissionInfo> psschTx = poolIt->second.m_pool->GetPsschTransmissions (currScPeriod, 0, poolIt->second.m_currentGrant.m_rbStart, poolIt->second.m_currentGrant.m_rbLen);
	 for (std::list<SidelinkCommResourcePool::SidelinkTransmissionInfo>::iterator txIt = psschTx.begin (); txIt != psschTx.end (); txIt++)
	 {
	   //adjust for index starting at 1
	   txIt->subframe.frameNo++;
	   txIt->subframe.subframeNo++;
	   stats_sch_params.m_psschFrameStart = txIt->subframe.frameNo;
	   stats_sch_params.m_psschSubframeStart = txIt->subframe.subframeNo;
	   break; //Just need the first one!
	 }
         stats_sch_params.m_rnti = m_rnti;
	 stats_sch_params.m_mcs = poolIt->second.m_currentGrant.m_mcs;
	 stats_sch_params.m_tbSize = poolIt->second.m_currentGrant.m_tbSize;
	 stats_sch_params.m_psschTxStartRB = poolIt->second.m_currentGrant.m_rbStart;
	 stats_sch_params.m_psschTxLengthRB = poolIt->second.m_currentGrant.m_rbLen;
	 stats_sch_params.m_timestamp = Simulator::Now ().GetMilliSeconds ();
         // Call trace
	 m_slSharedChUeScheduling (stats_sch_params);
        if (poolIt->second.m_psschTx.size () % 4 == 0)
	{
	  NS_LOG_INFO (this << " New PSSCH transmission");
	  Ptr<PacketBurst> emptyPb = CreateObject <PacketBurst> ();
	  poolIt->second.m_miSlHarqProcessPacket = emptyPb;
          //get the BSR for this pool
	  //if we have data in the queue
	  //find the BSR for that pool (will also give the SidleinkLcIdentifier)
	  std::map <SidelinkLcIdentifier, NistLteMacSapProvider::NistReportBufferNistStatusParameters>::iterator itBsr;
	  for (itBsr = m_slBsrReceived.begin () ;  itBsr != m_slBsrReceived.end () ; itBsr++)
	  {
	    if (itBsr->first.dstL2Id == poolIt->first)
	    {
	      //this is the BSR for the pool
	      std::map <SidelinkLcIdentifier, NistLcInfo>::iterator it = m_slLcInfoMap.find (itBsr->first);
	      //for sidelink we should never have retxQueueSize since it is unacknowledged mode
	      //we still keep the process similar to uplink to be more generic (and maybe handle
	      //future modifications)
	      if ( ((*itBsr).second.statusPduSize > 0)
		 || ((*itBsr).second.retxQueueSize > 0)
		 || ((*itBsr).second.txQueueSize > 0))
	      {
                 //We have data to send in the PSSCH, notify the RRC to start/continue sending SLSS if appropriate
		 m_slHasDataToTx = true;
		 m_cmacSapUser->NotifyMacHasSlDataToSend();
                 NS_ASSERT ((*itBsr).second.statusPduSize == 0 && (*itBsr).second.retxQueueSize == 0);
		 //similar code as uplink transmission
		 uint32_t bytesForThisLc = poolIt->second.m_currentGrant.m_tbSize;
		 NS_LOG_LOGIC (this << " RNTI " << m_rnti << " Sidelink Tx " << bytesForThisLc << " bytes to LC " << (uint32_t)(*itBsr).first.lcId << " statusQueue " << (*itBsr).second.statusPduSize << " retxQueue" << (*itBsr).second.retxQueueSize << " txQueue" <<  (*itBsr).second.txQueueSize);
		 if (((*itBsr).second.statusPduSize > 0) && (bytesForThisLc > (*itBsr).second.statusPduSize))
		 {
		   (*it).second.macSapUser->NotifyTxOpportunity ((*itBsr).second.statusPduSize, 0, 0);
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
		     (*it).second.macSapUser->NotifyTxOpportunity (bytesForThisLc, 0, 0);
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
		     if ((*itBsr).second.txQueueSize >= bytesForThisLc - rlcOverhead)
		     {
			(*itBsr).second.txQueueSize -= bytesForThisLc - rlcOverhead;
		     }
		     else
		     {
			(*itBsr).second.txQueueSize = 0;
		     }
		   }
		 }// end if ((bytesForThisLc > 7)..
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
              }//end  if ( ((*itBsr).second.statusPduSize > 0)
              break;
  	    }
          }
 	}
	else
	{
	  NS_LOG_INFO (this << " PSSCH retransmission " << (4 - poolIt->second.m_psschTx.size () % 4));
	  Ptr<PacketBurst> pb = poolIt->second.m_miSlHarqProcessPacket;
	  for (std::list<Ptr<Packet> >::const_iterator j = pb->Begin (); j != pb->End (); ++j)
	  {
	    Ptr<Packet> pkt = (*j)->Copy ();
	    m_uePhySapProvider->SendMacPdu (pkt);
	  }
	}
	poolIt->second.m_psschTx.erase (allocIt);
      }
    }
  }

}

int64_t
NistLteUeMac::AssignStreams (int64_t stream)
{
	NS_LOG_FUNCTION (this << stream);
	m_raPreambleUniformVariable->SetStream (stream);
	return 1;
}

void
NistLteUeMac::DoAddSlDestination (uint32_t destination)
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
NistLteUeMac::DoRemoveSlDestination (uint32_t destination)
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
NistLteUeMac::DoNotifyChangeOfTiming(uint32_t frameNo, uint32_t subframeNo)
{
	NS_LOG_FUNCTION (this);

	//there is a delay between the MAC scheduling and the transmission so we assume that we are ahead
	subframeNo += 4;
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
NistLteUeMac::DoReportPsschRsrp (Time time, uint16_t rbStart, uint16_t rbLen, double rsrpDb)
{
  

  PsschRsrp rsrpStruct;
  rsrpStruct.rbStart = rbStart;
  rsrpStruct.rbStart = rbLen;
  rsrpStruct.psschRsrpDb = rsrpDb;

  m_PsschRsrpMap.insert (std::pair<Time,PsschRsrp> (time, rsrpStruct)); 
  
  NS_LOG_INFO("NistLteUeMac::DoReportPsschRsrp at time: " << time.GetSeconds () << " s, rbStart PSSCH: " << (int)rbStart << ", rbLen PSSCH: " << rbLen << ", PSSCH-RSRP: " << rsrpDb << "dB, List length: " << m_PsschRsrpMap.size ());   

}

void
NistLteUeMac::DoReportPsschRsrpReservation (Time time, uint16_t rbStart, uint16_t rbLen, double rsrpDb, SidelinkCommResourcePool::SubframeInfo reservedSubframe, uint32_t CreselRx, uint32_t nodeId, uint16_t RRI)
{

 reservedSubframe.frameNo = reservedSubframe.frameNo -1;
 reservedSubframe.subframeNo = reservedSubframe.subframeNo -1;
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
  
    //Calculate the CSR index
    uint16_t RBperCSR = m_nsubCHsize * m_L_SubCh;
    uint16_t CSRindex = rbStart / RBperCSR;
    NS_LOG_DEBUG("RBs start: " << rbStart << " Number of RBs: " << rbLen);
    if (rbLen > m_nsubCHsize)
    {
       for (uint16_t j=CSRindex; j < CSRindex + uint16_t(rbLen/m_nsubCHsize); j++)
       {
     //    NS_LOG_DEBUG("CSR index " << j);
         std::map <uint16_t,std::map<SidelinkCommResourcePool::SubframeInfo, ReservedCSR> >::iterator mapIt = m_sensedReservedCSRMap.find (j);

         newSensedReservedCSR.rbStart = rbStart + j*m_nsubCHsize;
         newSensedReservedCSR.rbLen = m_nsubCHsize;

         if (mapIt != m_sensedReservedCSRMap.end ())
         {
            // this is the right CSR index
            (*mapIt).second.insert (std::pair<SidelinkCommResourcePool::SubframeInfo, ReservedCSR> (reservedSubframe, newSensedReservedCSR));
         }
         else 
         {
            // CSR index not found: create it
            std::map<SidelinkCommResourcePool::SubframeInfo, ReservedCSR> value;
            value.insert (std::pair<SidelinkCommResourcePool::SubframeInfo, ReservedCSR> (reservedSubframe, newSensedReservedCSR));
            m_sensedReservedCSRMap.insert (std::pair<uint16_t,std::map<SidelinkCommResourcePool::SubframeInfo, ReservedCSR> > (j,value));    
         }
       }

    }
    else
    {
       newSensedReservedCSR.rbStart = rbStart;
       newSensedReservedCSR.rbLen = m_nsubCHsize;
       CSRindex = (uint16_t) rbStart / m_nsubCHsize; //this is the starting subchannel index
    //   NS_LOG_DEBUG("CSR index " << CSRindex);
       std::map <uint16_t,std::map<SidelinkCommResourcePool::SubframeInfo, ReservedCSR> >::iterator mapIt = m_sensedReservedCSRMap.find (CSRindex);
       if (mapIt != m_sensedReservedCSRMap.end ())
       {
          // this is the right CSR index
          (*mapIt).second.insert (std::pair<SidelinkCommResourcePool::SubframeInfo, ReservedCSR> (reservedSubframe, newSensedReservedCSR));
       }
       else 
       {
          // CSR index not found: create it
          std::map<SidelinkCommResourcePool::SubframeInfo, ReservedCSR> value;
          value.insert (std::pair<SidelinkCommResourcePool::SubframeInfo, ReservedCSR> (reservedSubframe, newSensedReservedCSR));
          m_sensedReservedCSRMap.insert (std::pair<uint16_t,std::map<SidelinkCommResourcePool::SubframeInfo, ReservedCSR> > (CSRindex,value));    
       }
    }

 } // end if (!m_randomselection)
}

void
NistLteUeMac::DoStoreTxInfo (SidelinkCommResourcePool::SubframeInfo subframe, uint16_t rbStart, uint16_t rbLen)
{

  NS_LOG_DEBUG("Storing transmission information");
//  std::cin.get();
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
NistLteUeMac::GetDiscRxPools ()
{
  NS_LOG_FUNCTION (this);
  return m_discRxPools;
}

Ptr<SidelinkTxDiscResourcePool>
NistLteUeMac::GetDiscTxPool ()
{
  NS_LOG_FUNCTION (this);
  return m_discTxPools.m_pool; 
}

} // namespace ns3
