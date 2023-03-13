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
 *         Marco Miozzo <marco.miozzo@cttc.es>
 *         Nicola Baldo <nbaldo@cttc.es>
 * Modified by: NIST
 * Modified by: Luca Lusvarghi <luca.lusvarghi5@unimore.it>
 */

#include <ns3/object-factory.h>
#include <ns3/log.h>
#include <cfloat>
#include <cmath>
#include <ns3/simulator.h>
#include <ns3/double.h>
#include <list>
#include "nr-v2x-ue-phy.h"
#include "nist-lte-net-device.h"
#include "nr-v2x-ue-net-device.h"
#include "nr-v2x-spectrum-value-helper.h"
#include "nr-v2x-ue-mac.h"
#include "nist-lte-chunk-processor.h"
#include <ns3/nist-lte-common.h>
#include <ns3/pointer.h>
#include <ns3/boolean.h>
#include <ns3/nist-lte-ue-power-control.h>
#include "nist-lte-radio-bearer-tag.h"
#include <ns3/node.h>
#include <fstream>
#include <ns3/string.h>
#include <iostream>
#include <algorithm>
#include <ns3/node-container.h>

#include "nr-v2x-utils.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("NrV2XUePhy");

/**
 * Duration of the data portion of a UL subframe.
 * Equals to "TTI length - 1 symbol length for SRS - margin".
 * The margin is 1 nanosecond and is intended to avoid overlapping simulator
 * events. The duration of one symbol is TTI/14 (rounded). In other words,
 * duration of data portion of UL subframe = 1 ms * (13/14) - 1 ns.
 */
static const Time UL_DATA_DURATION = NanoSeconds (1e6 - 71429 - 1);  //71429 ns is the duration of an OFDM symbol when the subframe duration is 1ms (useful + CP)
//static const Time UL_DATA_DURATION = NanoSeconds (0.5e6 - 35680 - 1); 
/**
 * Delay from subframe start to transmission of SRS.
 * Equals to "TTI length - 1 symbol for SRS".
 */
static const Time UL_SRS_DELAY_FROM_SUBFRAME_START = NanoSeconds (1e6 - 71429); 

////////////////////////////////////////
// member SAP forwarders
////////////////////////////////////////


class NistUeMemberLteUePhySapProvider : public NistLteUePhySapProvider
{
public:
  NistUeMemberLteUePhySapProvider (NrV2XUePhy* phy);

  // inherited from NistLtePhySapProvider
  virtual void SendMacPdu (Ptr<Packet> p);
  virtual void SendNistLteControlMessage (Ptr<NistLteControlMessage> msg);
  virtual void SendRachPreamble (uint32_t prachId, uint32_t raRnti);


private:
  NrV2XUePhy* m_phy;
};

NistUeMemberLteUePhySapProvider::NistUeMemberLteUePhySapProvider (NrV2XUePhy* phy) : m_phy (phy)
{

}

void
NistUeMemberLteUePhySapProvider::SendMacPdu (Ptr<Packet> p)
{
  m_phy->DoSendMacPdu (p);
}

void
NistUeMemberLteUePhySapProvider::SendNistLteControlMessage (Ptr<NistLteControlMessage> msg)
{
  m_phy->DoSendNistLteControlMessage (msg);
}

void
NistUeMemberLteUePhySapProvider::SendRachPreamble (uint32_t prachId, uint32_t raRnti)
{
  m_phy->DoSendRachPreamble (prachId, raRnti);
}
  
////////////////////////////////////////
// NrV2XUePhy methods
////////////////////////////////////////

/// Map each of UE PHY states to its string representation.
static const std::string g_uePhyStateName[NrV2XUePhy::NUM_STATES] =
{
  "CELL_SEARCH",
  "SYNCHRONIZED"
};

/**
 * \param s The UE PHY state.
 * \return The string representation of the given state.
 */
static inline const std::string & ToString (NrV2XUePhy::State s)
{
  return g_uePhyStateName[s];
}


/*struct TxPacketInfo
{
  double txTime;
  uint32_t nodeId;
  uint64_t packetId;
  uint32_t Cresel;
  uint16_t RRI;
  uint16_t psschRbStart;
  uint16_t psschRbLen;
  uint16_t psschRbLenTb;
  uint16_t pscchRbStart;
  uint16_t pscchRbLen;
  SidelinkCommResourcePool::SubframeInfo txFrame;
};

std::vector<TxPacketInfo> txPackets;
double prevPrintTime = 0.0;*/

std::vector<NrV2XUePhy::TxPacketInfo> NrV2XUePhy::txPackets;
double NrV2XUePhy::prevPrintTime = 0.0;

std::vector<NrV2XUePhy::CBRInfo> NrV2XUePhy::CBRValues;
double NrV2XUePhy::CBRSavingInterval = 0.0;

NS_OBJECT_ENSURE_REGISTERED (NrV2XUePhy);


NrV2XUePhy::NrV2XUePhy ()
{
  NS_LOG_FUNCTION (this);
  NS_FATAL_ERROR ("This constructor should not be called");
}

NrV2XUePhy::NrV2XUePhy (Ptr<NrV2XSpectrumPhy> dlPhy, Ptr<NrV2XSpectrumPhy> ulPhy)
  : NistLtePhy (dlPhy, ulPhy),
    m_p10CqiPeriocity (MilliSeconds (1)),  // ideal behavior
    m_a30CqiPeriocity (MilliSeconds (1)),  // ideal behavior
    m_uePhySapUser (0),
    m_ueCphySapUser (0),
    m_state (CELL_SEARCH),
    m_subframeNo (0),
    m_rsReceivedPowerUpdated (false),
    m_rsInterferencePowerUpdated (false),
    m_dataInterferencePowerUpdated (false),
    m_pssReceived (false),
    m_ueMeasurementsFilterPeriod (MilliSeconds (200)),
    m_ueMeasurementsFilterLast (MilliSeconds (0)),
    m_rsrpSinrSampleCounter (0),
    m_tFirstScanning(MilliSeconds (0)),
    m_ueSlssScanningInProgress(false),
    m_ueSlssMeasurementInProgress(false),
    m_currNMeasPeriods(0),
    m_currFrameNo(0),
    m_currSubframeNo(0),
    m_resyncRequested(false),
    m_waitingNextScPeriod(false)
{
  m_powerControl = CreateObject <NistLteUePowerControl> ();
  m_uePhySapProvider = new NistUeMemberLteUePhySapProvider (this);
  m_ueCphySapProvider = new NistMemberLteUeCphySapProvider<NrV2XUePhy> (this);
  m_macChTtiDelay = UL_PUSCH_TTIS_DELAY;

  m_nextScanRdm = CreateObject<UniformRandomVariable> ();

  NS_ASSERT_MSG (Simulator::Now ().GetNanoSeconds () == 0,
                 "Cannot create UE devices after simulation started");

  //Simulator::ScheduleNow (&NrV2XUePhy::SubframeIndication, this, 1, 1); //Now it is done in the function SetInitialSubFrameIndication

  Simulator::Schedule (m_ueMeasurementsFilterPeriod, &NrV2XUePhy::ReportNistUeMeasurements, this);

  //added for sidelink 
  m_slTxPoolInfo.m_pool = NULL;
  m_slTxPoolInfo.m_currentScPeriod.frameNo = 0;
  m_slTxPoolInfo.m_currentScPeriod.subframeNo = 0;
  m_slTxPoolInfo.m_nextScPeriod.frameNo = 0;
  m_slTxPoolInfo.m_nextScPeriod.subframeNo = 0;
  
  m_discTxPools.m_pool = NULL;
  m_discTxPools.m_currentDiscPeriod.frameNo = 0;
  m_discTxPools.m_currentDiscPeriod.subframeNo = 0;
  m_discTxPools.m_nextDiscPeriod.frameNo = 0;
  m_discTxPools.m_nextDiscPeriod.subframeNo = 0;

  m_discRxApps.clear (); 
  m_discTxApps.clear (); 

  //added for V2X
  m_CBRCheckingPeriod = 0.5;
  m_CBRCheckingInterval = 0.0;

  DoReset ();
}

NrV2XUePhy::~NrV2XUePhy ()
{
  m_txModeGain.clear ();
}

void
NrV2XUePhy::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  delete m_uePhySapProvider;
  delete m_ueCphySapProvider;
  if (m_sidelinkSpectrumPhy) {
    m_sidelinkSpectrumPhy->Dispose ();
    m_sidelinkSpectrumPhy = 0;
  }
  NistLtePhy::DoDispose ();
}



TypeId
NrV2XUePhy::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::NrV2XUePhy")
    .SetParent<NistLtePhy> ()
    .AddConstructor<NrV2XUePhy> ()
    .AddAttribute ("TxPower",
                   "Transmission power in dBm",
                   DoubleValue (10.0),
                   MakeDoubleAccessor (&NrV2XUePhy::SetTxPower, 
                                       &NrV2XUePhy::GetTxPower),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("NoiseFigure",
                   "Loss (dB) in the Signal-to-Noise-Ratio due to non-idealities in the receiver."
                   " According to Wikipedia (http://en.wikipedia.org/wiki/Noise_figure), this is "
                   "\"the difference in decibels (dB) between"
                   " the noise output of the actual receiver to the noise output of an "
                   " ideal receiver with the same overall gain and bandwidth when the receivers "
                   " are connected to sources at the standard noise temperature T0.\" "
                   "In this model, we consider T0 = 290K.",
                   DoubleValue (9.0),
                   MakeDoubleAccessor (&NrV2XUePhy::SetNoiseFigure, 
                                       &NrV2XUePhy::GetNoiseFigure),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("TxMode1Gain",
                   "Transmission mode 1 gain in dB",
                   DoubleValue (0.0),
                   MakeDoubleAccessor (&NrV2XUePhy::SetTxMode1Gain),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("TxMode2Gain",
                   "Transmission mode 2 gain in dB",
                   DoubleValue (4.2),
                   MakeDoubleAccessor (&NrV2XUePhy::SetTxMode2Gain),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("TxMode3Gain",
                   "Transmission mode 3 gain in dB",
                   DoubleValue (-2.8),
                   MakeDoubleAccessor (&NrV2XUePhy::SetTxMode3Gain),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("TxMode4Gain",
                   "Transmission mode 4 gain in dB",
                   DoubleValue (0.0),
                   MakeDoubleAccessor (&NrV2XUePhy::SetTxMode4Gain),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("TxMode5Gain",
                   "Transmission mode 5 gain in dB",
                   DoubleValue (0.0),
                   MakeDoubleAccessor (&NrV2XUePhy::SetTxMode5Gain),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("TxMode6Gain",
                   "Transmission mode 6 gain in dB",
                   DoubleValue (0.0),
                   MakeDoubleAccessor (&NrV2XUePhy::SetTxMode6Gain),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("TxMode7Gain",
                   "Transmission mode 7 gain in dB",
                   DoubleValue (0.0),
                   MakeDoubleAccessor (&NrV2XUePhy::SetTxMode7Gain),
                   MakeDoubleChecker<double> ())
    .AddTraceSource ("UlPhyTransmission",
                     "DL transmission PHY layer statistics.",
                     MakeTraceSourceAccessor (&NrV2XUePhy::m_ulPhyTransmission),
                     "ns3::NistPhyTransmissionStatParameters::TracedCallback")
    .AddAttribute ("DlSpectrumPhy",
                   "The downlink NrV2XSpectrumPhy associated to this NistLtePhy",
                   TypeId::ATTR_GET,
                   PointerValue (),
                   MakePointerAccessor (&NrV2XUePhy::GetDlSpectrumPhy),
                   MakePointerChecker <NrV2XSpectrumPhy> ())
    .AddAttribute ("UlSpectrumPhy",
                   "The uplink NrV2XSpectrumPhy associated to this NistLtePhy",
                   TypeId::ATTR_GET,
                   PointerValue (),
                   MakePointerAccessor (&NrV2XUePhy::GetUlSpectrumPhy),
                   MakePointerChecker <NrV2XSpectrumPhy> ())
    .AddAttribute ("SlSpectrumPhy",
                   "The uplink NrV2XSpectrumPhy associated to this NistLtePhy",
                   TypeId::ATTR_GET,
                   PointerValue (),
                   MakePointerAccessor (&NrV2XUePhy::GetSlSpectrumPhy),
                   MakePointerChecker <NrV2XSpectrumPhy> ())
    .AddAttribute ("RsrpUeMeasThreshold",
                   "Receive threshold for RSRP [dB]",
                   DoubleValue (-1000.0), //to avoid changing the default behavior, make it low so that it acts as if it was not used
                   MakeDoubleAccessor (&NrV2XUePhy::m_rsrpReceptionThreshold),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("NistUeMeasurementsFilterPeriod",
                   "Time period for reporting UE measurements, i.e., the"
                   "length of layer-1 filtering.",
                   TimeValue (MilliSeconds (200)),
                   MakeTimeAccessor (&NrV2XUePhy::m_ueMeasurementsFilterPeriod),
                   MakeTimeChecker ())
    .AddTraceSource ("ReportNistUeMeasurements",
                     "Report UE measurements RSRP (dBm) and RSRQ (dB).",
                     MakeTraceSourceAccessor (&NrV2XUePhy::m_reportNistUeMeasurements),
                     "ns3::NrV2XUePhy::RsrpRsrqTracedCallback")
    .AddTraceSource ("StateTransition",
                     "Trace fired upon every UE PHY state transition",
                     MakeTraceSourceAccessor (&NrV2XUePhy::m_stateTransitionTrace),
                     "ns3::NrV2XUePhy::StateTracedCallback")
    .AddAttribute ("EnableUplinkPowerControl",
                   "If true, Uplink Power Control will be enabled.",
                   BooleanValue (true),
                   MakeBooleanAccessor (&NrV2XUePhy::m_enableUplinkPowerControl),
                   MakeBooleanChecker ())
    .AddAttribute ("UeSlssInterScanningPeriodMax",
                  "The upper bound of the uniform random variable for the interval between SyncRef selection processes",
                  TimeValue(MilliSeconds(2000)),
                  MakeTimeAccessor(&NrV2XUePhy::SetUeSlssInterScanningPeriodMax),
                  MakeTimeChecker())
    .AddAttribute ("UeSlssInterScanningPeriodMin",
                  "The lower bound of the uniform random variable for the interval between SyncRef selection processes",
                  TimeValue(MilliSeconds(2000)),
                  MakeTimeAccessor(&NrV2XUePhy::SetUeSlssInterScanningPeriodMin),
                  MakeTimeChecker())
    .AddAttribute("UeRandomInitialSubframeIndication",
                 "If True, the first frame and subframe values (beginning of the simulation) are chosen randomly, if False they are fixed to 1,1 respectively",
                 BooleanValue(false),
                 MakeBooleanAccessor(&NrV2XUePhy::SetInitialSubFrameIndication),
                 MakeBooleanChecker())
    .AddAttribute ("MinSrsrp",
                  "The minimum S-RSRP required to consider a SyncRef detectable",
                  DoubleValue(-125),
                  MakeDoubleAccessor (&NrV2XUePhy::m_minSrsrp),
                  MakeDoubleChecker<double>())
    .AddAttribute ("SidelinkDataDuration",
                  "The duration of data transmission on the Sidelink channel",
                  TimeValue(NanoSeconds (1e6 - 71350 - 1)),
                  MakeTimeAccessor(&NrV2XUePhy::m_SL_DATA_DURATION),
                  MakeTimeChecker())
    .AddAttribute ("SlotDuration",
		   "The NR-V2X time slot duration",
		   DoubleValue (1.0),
		   MakeDoubleAccessor (&NrV2XUePhy::m_slotDuration),
		   MakeDoubleChecker<double> ())
    .AddAttribute ("SubchannelSize",
	           "The Subchannel size (in RBs)",
		   UintegerValue (10),
		   MakeUintegerAccessor (&NrV2XUePhy::m_nsubCHsize),
		   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("RBsBandwidth",
	           "The bandwidth at PHY layer (in RBs)",
		   UintegerValue (50),
		   MakeUintegerAccessor (&NrV2XUePhy::m_BW_RBs),
		   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("SubCarrierSpacing",
	           "The NR-V2X SubCarrier Spacing (SCS)",
		   UintegerValue (15),
		   MakeUintegerAccessor (&NrV2XUePhy::m_SCS),
		   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("IBE",
	           "Enable In-Band Emissions (IBE)",
                   BooleanValue(false),
                   MakeBooleanAccessor(&NrV2XUePhy::m_IBE),
                   MakeBooleanChecker())
    .AddAttribute ("ReferenceSensitivity",
	           "The Reference Sensitivity",
		   DoubleValue (-92.5),
		   MakeDoubleAccessor (&NrV2XUePhy::m_rxSensitivity),
		   MakeDoubleChecker<double> ())
    .AddAttribute ("RSSIthreshold",
	           "The CBR S-RSSI threshold",
		   DoubleValue (-90.0),
		   MakeDoubleAccessor (&NrV2XUePhy::m_RSSIthresh),
		   MakeDoubleChecker<double> ())
    .AddAttribute ("OutputPath",
                   "Specifiy the output path where to store the results",
                   StringValue ("results/sidelink/"),
                   MakeStringAccessor (&NrV2XUePhy::m_outputPath),
                   MakeStringChecker ())
    .AddAttribute ("SavingPeriod",
                   "The period used to save data",
		   DoubleValue (1.0),
		   MakeDoubleAccessor (&NrV2XUePhy::m_savingPeriod),
		   MakeDoubleChecker<double> ())



    ;
  return tid;
}

void
NrV2XUePhy::DoInitialize ()
{
  NS_LOG_FUNCTION (this);
  NistLtePhy::DoInitialize ();
}

void
NrV2XUePhy::SetNistLteUePhySapUser (NistLteUePhySapUser* s)
{
  NS_LOG_FUNCTION (this);
  m_uePhySapUser = s;
}

NistLteUePhySapProvider*
NrV2XUePhy::GetNistLteUePhySapProvider ()
{
  NS_LOG_FUNCTION (this);
  return (m_uePhySapProvider);
}


void
NrV2XUePhy::SetNistLteUeCphySapUser (NistLteUeCphySapUser* s)
{
  NS_LOG_FUNCTION (this);
  m_ueCphySapUser = s;
}

NistLteUeCphySapProvider*
NrV2XUePhy::GetNistLteUeCphySapProvider ()
{
  NS_LOG_FUNCTION (this);
  return (m_ueCphySapProvider);
}

void
NrV2XUePhy::SetNoiseFigure (double nf)
{
  NS_LOG_FUNCTION (this << nf);
  m_noiseFigure = nf;
}

double
NrV2XUePhy::GetNoiseFigure () const
{
  NS_LOG_FUNCTION (this);
  return m_noiseFigure;
}

void
NrV2XUePhy::SetTxPower (double pow)
{
  NS_LOG_FUNCTION (this << pow);
  m_txPower = pow;
  m_powerControl->SetTxPower (pow);
}

double
NrV2XUePhy::GetTxPower () const
{
  NS_LOG_FUNCTION (this);
  return m_txPower;
}

Ptr<NistLteUePowerControl>
NrV2XUePhy::GetUplinkPowerControl () const
{
  NS_LOG_FUNCTION (this);
  return m_powerControl;
}

uint8_t
NrV2XUePhy::GetMacChDelay (void) const
{
  return (m_macChTtiDelay);
}

Ptr<NrV2XSpectrumPhy>
NrV2XUePhy::GetDlSpectrumPhy () const
{
  return m_downlinkSpectrumPhy;
}

Ptr<NrV2XSpectrumPhy>
NrV2XUePhy::GetUlSpectrumPhy () const
{
  return m_uplinkSpectrumPhy;
}

//@NEW: Creation of a dedicated Sidelink channel
void
NrV2XUePhy::SetSlSpectrumPhy (Ptr<NrV2XSpectrumPhy> phy) 
{
  m_sidelinkSpectrumPhy = phy;
}

Ptr<NrV2XSpectrumPhy> NrV2XUePhy::GetSlSpectrumPhy () const
{
  return m_sidelinkSpectrumPhy;
}

  
void
NrV2XUePhy::DoSendMacPdu (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this);

  SetMacPdu (p);
}


void
NrV2XUePhy::PhyPduReceived (Ptr<Packet> p)
{
  m_uePhySapUser->ReceivePhyPdu (p);
}

void
NrV2XUePhy::SetSubChannelsForTransmission (std::vector <int> mask)
{
  NS_LOG_FUNCTION (this);

  m_subChannelsForTransmission = mask;

  Ptr<SpectrumValue> txPsd = CreateTxPowerSpectralDensity ();
  m_uplinkSpectrumPhy->SetTxPowerSpectralDensity (txPsd);
}


void
NrV2XUePhy::SetSubChannelsForReception (std::vector <int> mask)
{
  NS_LOG_FUNCTION (this);
  m_subChannelsForReception = mask;
}


std::vector <int>
NrV2XUePhy::GetSubChannelsForTransmission ()
{
  NS_LOG_FUNCTION (this);
  return m_subChannelsForTransmission;
}


std::vector <int>
NrV2XUePhy::GetSubChannelsForReception ()
{
  NS_LOG_FUNCTION (this);
  return m_subChannelsForReception;
}


Ptr<SpectrumValue> NrV2XUePhy::CreateTxPowerSpectralDensity () //FIXME The values employed here are only valid for uplink
{
  NS_LOG_FUNCTION (this);

  NrV2XSpectrumValueHelper psdHelper;
//  Ptr<SpectrumValue> psd = psdHelper.CreateUlTxPowerSpectralDensity (m_ulEarfcn, m_ulBandwidth, m_txPower, GetSubChannelsForTransmission (), m_slotDuration);
  Ptr<SpectrumValue> psd = psdHelper.CreateUlTxPowerSpectralDensity (m_ulEarfcn, m_BW_RBs, m_txPower, GetSubChannelsForTransmission (), m_slotDuration, m_SCS, m_currentMCS, m_IBE);

//  NS_LOG_UNCOND("Configured TX Power: " << m_txPower << " and slot duration: " << m_slotDuration);
//  std::cin.get();
  return psd;
}

void
NrV2XUePhy::GenerateCtrlCqiReport (const SpectrumValue& sinr)
{
  NS_LOG_FUNCTION (this);
  
  GenerateCqiRsrpRsrq (sinr);
}

void
NrV2XUePhy::GenerateCqiRsrpRsrq (const SpectrumValue& sinr)
{
  NS_LOG_FUNCTION (this << sinr);
  //Useless
} // end of void NrV2XUePhy::GenerateCtrlCqiReport (const SpectrumValue& sinr)

void
NrV2XUePhy::GenerateDataCqiReport (const SpectrumValue& sinr)
{
  // Not used by UE, CQI are based only on RS
}

void
NrV2XUePhy::GenerateMixedCqiReport (const SpectrumValue& sinr)
{
  NS_LOG_FUNCTION (this);
  //Useless
}

void
NrV2XUePhy::ReportInterference (const SpectrumValue& interf)
{
  NS_LOG_FUNCTION (this << interf);
  m_rsInterferencePowerUpdated = true;
  m_rsInterferencePower = interf;
}

void
NrV2XUePhy::ReportDataInterference (const SpectrumValue& interf)
{
  NS_LOG_FUNCTION (this << interf);

  m_dataInterferencePowerUpdated = true;
  m_dataInterferencePower = interf;
}

void
NrV2XUePhy::ReportRsReceivedPower (const SpectrumValue& power)
{
  NS_LOG_FUNCTION (this << power);
  //Useless
}

Ptr<NistDlCqiLteControlMessage>
NrV2XUePhy::CreateDlCqiFeedbackMessage (const SpectrumValue& sinr)
{
  NS_LOG_FUNCTION (this);
  //Useless
  Ptr<NistDlCqiLteControlMessage> msg;
  return msg;
}


void
NrV2XUePhy::ReportNistUeMeasurements ()
{
  NS_LOG_FUNCTION (this << Simulator::Now ());
  NS_LOG_DEBUG (this << " Report UE Measurements ");

  NistLteUeCphySapUser::NistUeMeasurementsParameters ret;

  std::map <uint16_t, NistUeMeasurementsElement>::iterator it;
  for (it = m_ueMeasurementsMap.begin (); it != m_ueMeasurementsMap.end (); it++)
    {
      double avg_rsrp = (*it).second.rsrpSum / (double)(*it).second.rsrpNum;
      double avg_rsrq = (*it).second.rsrqSum / (double)(*it).second.rsrqNum;
      /*
       * In CELL_SEARCH state, this may result in avg_rsrq = 0/0 = -nan.
       * UE RRC must take this into account when receiving measurement reports.
       * TODO remove this shortcoming by calculating RSRQ during CELL_SEARCH
       */
      NS_LOG_DEBUG (this << " CellId " << (*it).first
                         << " RSRP " << avg_rsrp
                         << " (nSamples " << (uint16_t)(*it).second.rsrpNum << ")"
                         << " RSRQ " << avg_rsrq
                         << " (nSamples " << (uint16_t)(*it).second.rsrqNum << ")");

      if (avg_rsrp >= m_rsrpReceptionThreshold) {
        NistLteUeCphySapUser::NistUeMeasurementsElement newEl;
        newEl.m_cellId = (*it).first;
        newEl.m_rsrp = avg_rsrp;
        newEl.m_rsrq = avg_rsrq;
        ret.m_ueMeasurementsList.push_back (newEl);
      }
      
      // report to UE measurements trace
      m_reportNistUeMeasurements (m_rnti, (*it).first, avg_rsrp, avg_rsrq, ((*it).first == m_cellId ? 1 : 0));
    }

  // report to RRC
  m_ueCphySapUser->ReportNistUeMeasurements (ret);

  m_ueMeasurementsMap.clear ();
  Simulator::Schedule (m_ueMeasurementsFilterPeriod, &NrV2XUePhy::ReportNistUeMeasurements, this);
}

void
NrV2XUePhy::DoSendNistLteControlMessage (Ptr<NistLteControlMessage> msg)
{
  NS_LOG_FUNCTION (this << msg);

  SetControlMessages (msg);
}

void 
NrV2XUePhy::DoSendRachPreamble (uint32_t raPreambleId, uint32_t raRnti)
{
  NS_LOG_FUNCTION (this << raPreambleId);

  // unlike other control messages, RACH preamble is sent ASAP
  Ptr<NistRachPreambleLteControlMessage> msg = Create<NistRachPreambleLteControlMessage> ();
  msg->SetRapId (raPreambleId);
  m_raPreambleId = raPreambleId;
  m_raRnti = raRnti;
  m_controlMessagesQueue.at (0).push_back (msg);
}



void
NrV2XUePhy::UnimoreReceivedRssi (double rssi, std::vector <int> rbMap, uint16_t ID)
{
  NS_LOG_FUNCTION (this);

  uint16_t NSubCh = std::floor(m_BW_RBs / m_nsubCHsize); 

  SidelinkCommResourcePool::SubframeInfo SF;
  SF = SimulatorTimeToSubframe(Simulator::Now(), m_slotDuration);
//  SF.frameNo--;
//  SF.subframeNo--;

  NS_LOG_DEBUG("Rx UE: " << m_rnti << ", Tx UE: " << ID << ". RSSI = " << rssi << " dBm measured over " << rbMap.size() << " RBs starting from " <<
  rbMap.front() << " at SF(" << SF.frameNo << "," << SF.subframeNo << ") Number of subchannels is " << NSubCh);

  std::vector <int> RBindex;

  for (uint16_t j = rbMap.front(); j < rbMap.front()+rbMap.size(); j++)
    RBindex.push_back(j);

  std::map <uint16_t,std::map<SidelinkCommResourcePool::SubframeInfo, RSSImeas> >::iterator RSSIit;
  NS_LOG_DEBUG("Filling the RSSI list");
  for (uint16_t subChannelIndex = 0; subChannelIndex < NSubCh; subChannelIndex++)
  {
    double meanRSSI = 0;
    uint16_t lowestRB = subChannelIndex*m_nsubCHsize;
    uint16_t highestRB = (subChannelIndex+1)*m_nsubCHsize;
    NS_LOG_INFO("Subchannel index: " << subChannelIndex << ", lowest RB: " << lowestRB <<  " highest RB: " << highestRB); 
    for (std::vector <int>::iterator RBindexIT = RBindex.begin(); RBindexIT != RBindex.end(); RBindexIT++)
    {
       if ((*RBindexIT) >= lowestRB && (*RBindexIT) < highestRB)
       {
         NS_LOG_DEBUG("Adding RB " << *RBindexIT << " to subchannel " << subChannelIndex);
         meanRSSI += std::pow(10,rssi/10);
       }
    }
    meanRSSI /= m_nsubCHsize;
    if (meanRSSI > 0)
    {
      NS_LOG_INFO("Mean RSSI for subchannel " << subChannelIndex << " is " << 10*std::log10(meanRSSI) << " dBm");
      if ( m_receivedRSSI.find(subChannelIndex) == m_receivedRSSI.end() )
      {
        NS_LOG_DEBUG("Subchannel not found, creating new map entry");
        RSSImeas tmp;
        tmp.m_IDs.push_back(ID);
        tmp.m_rssi = 10*std::log10(meanRSSI);
        std::map<SidelinkCommResourcePool::SubframeInfo, RSSImeas> tmp_elem;
        tmp_elem.insert (std::pair<SidelinkCommResourcePool::SubframeInfo, RSSImeas> (SF, tmp));
        m_receivedRSSI.insert (std::pair<uint16_t,std::map<SidelinkCommResourcePool::SubframeInfo, RSSImeas> > (subChannelIndex,tmp_elem));    
      }
      else
      {
        NS_LOG_DEBUG("Subchannel found");
        std::map<SidelinkCommResourcePool::SubframeInfo, RSSImeas> tmp_elem = m_receivedRSSI[subChannelIndex];
        std::map<SidelinkCommResourcePool::SubframeInfo, RSSImeas>::iterator tmpIT = tmp_elem.find(SF);
        if (tmpIT != tmp_elem.end())
        {
          double RSSIsum = std::pow(10,rssi/10) +  std::pow(10,tmpIT->second.m_rssi/10);
          NS_LOG_DEBUG("This entry already exists. New RSSI = " << rssi << " dBm. Old RSSI = " << tmpIT->second.m_rssi << " dBm. Sum = " << 10*std::log10(RSSIsum));
          // Collision on this subchannel, need to keep only the highest RSSI value and add the new ID node
          //if (meanRSSI > tmpIT->second.m_rssi)
          tmpIT->second.m_rssi = 10*std::log10(RSSIsum); 
          tmpIT->second.m_IDs.push_back(ID);
          m_receivedRSSI[subChannelIndex] = tmp_elem;
        }  
        else
        {
          NS_LOG_DEBUG("SF not found");
          RSSImeas tmp;
          tmp.m_IDs.push_back(ID);
          tmp.m_rssi = 10*std::log10(meanRSSI);
          tmp_elem.insert (std::pair<SidelinkCommResourcePool::SubframeInfo, RSSImeas> (SF, tmp));
          m_receivedRSSI[subChannelIndex] = tmp_elem;
        }   
      }
    }
  }


  NS_LOG_DEBUG("Cleaning the new RSSI list");
  for (RSSIit = m_receivedRSSI.begin(); RSSIit != m_receivedRSSI.end(); RSSIit++)
  {
  //  NS_LOG_DEBUG("CSR index " << RSSIit->first);
    std::map<SidelinkCommResourcePool::SubframeInfo, RSSImeas>::iterator subframeIt;
    for (subframeIt = RSSIit->second.begin(); subframeIt != RSSIit->second.end(); /*no increment*/)
    {
      if (SubtractFrames( SF.frameNo, subframeIt->first.frameNo, SF.subframeNo, subframeIt->first.subframeNo) > 100/m_slotDuration)
      {
     //   std::cout << "CSR index " << RSSIit->first << " SF(" << std::to_string(subframeIt->first.frameNo) << "," << std::to_string(subframeIt->first.subframeNo) << ") with IDs: ";
     //   for (std::vector<uint16_t>::iterator vectIT = subframeIt->second.m_IDs.begin(); vectIT != subframeIt->second.m_IDs.end(); vectIT++)
     //   {
     //     std::cout << (*vectIT) << ", ";
     //   }
     //   std::cout << "and RSSI " << subframeIt->second.m_rssi << " -> Erasing (too old)" << std::endl;
        subframeIt = RSSIit->second.erase(subframeIt);  
      }
      else
      {
        subframeIt++;
      }
    //  NS_LOG_DEBUG("SF(" << subframeIt->first.frameNo << "," << subframeIt->first.subframeNo << ")");
    }
  }

/*  NS_LOG_DEBUG("Printing the new RSSI list");
  for (RSSIit = m_receivedRSSI.begin(); RSSIit != m_receivedRSSI.end(); RSSIit++)
  {
    NS_LOG_DEBUG("Subchannel index " << RSSIit->first);
    std::map<SidelinkCommResourcePool::SubframeInfo, RSSImeas>::iterator subframeIt;
    for (subframeIt = RSSIit->second.begin(); subframeIt != RSSIit->second.end(); subframeIt++)
    {
      std::cout << "SF(" << std::to_string(subframeIt->first.frameNo) << "," << std::to_string(subframeIt->first.subframeNo) << ") with IDs: ";
      for (std::vector<uint16_t>::iterator vectIT = subframeIt->second.m_IDs.begin(); vectIT != subframeIt->second.m_IDs.end(); vectIT++)
      {
        std::cout << (*vectIT) << ", ";
      }
      std::cout << "and RSSI " << subframeIt->second.m_rssi << std::endl;
      //NS_LOG_DEBUG("SF(" << subframeIt->first.frameNo << "," << subframeIt->first.subframeNo << ")");
    }
  } */

  if (Simulator::Now ().GetSeconds () - m_CBRCheckingInterval > m_CBRCheckingPeriod)
  {
    NS_LOG_INFO("UE " << m_rnti << " evaluating CBR now " << Simulator::Now ().GetSeconds ());
    m_CBRCheckingInterval = Simulator::Now ().GetSeconds ();
    NodeContainer GlobalContainer = NodeContainer::GetGlobal(); 
    for (NodeContainer::Iterator L = GlobalContainer.Begin(); L != GlobalContainer.End(); ++L) 
    {
      Ptr<Node> RxNode;
      Ptr<MobilityModel> mobRX;
      Vector posRX;
      RxNode = *L;
      if (RxNode->GetId() == m_rnti)
      {
        mobRX = RxNode->GetObject<MobilityModel>();
        posRX = mobRX->GetPosition();
        if (posRX.x >= 1500 && posRX.x <= 3500)
        {
          NrV2XUePhy::UnimoreEvaluateCBR(SF.frameNo, SF.subframeNo);
        }
        else
        {
          NS_LOG_INFO("Rx UE outside of the central section");
        }
      }
    }
  }

  if (Simulator::Now ().GetSeconds () - NrV2XUePhy::CBRSavingInterval >= m_savingPeriod)
  {
    NS_LOG_INFO("Saving CBR now " << Simulator::Now ().GetSeconds ());
    NrV2XUePhy::CBRSavingInterval = Simulator::Now ().GetSeconds ();
    std::ofstream CBRfile;
    CBRfile.open (m_outputPath + "CBR_OutputFile.txt", std::ios_base::app);
    for (std::vector<CBRInfo>::iterator CBRit = NrV2XUePhy::CBRValues.begin(); CBRit != NrV2XUePhy::CBRValues.end(); CBRit++)
    {
      CBRfile << CBRit->nodeId << "," << CBRit->CBRvalue << "," << CBRit->time << std::endl;
    }
    CBRfile.close();
    NrV2XUePhy::CBRValues.clear();

//    std::cin.get();
  }
//  std::cin.get();
}

void
NrV2XUePhy::UnimoreEvaluateCBR (uint16_t frameNo, uint16_t subframeNo)
{
  NS_LOG_FUNCTION(this);
 
  uint16_t N_subCh = std::floor(m_BW_RBs / m_nsubCHsize); // Consider the subchannels according to 3GPP 38.215
 // uint16_t N_totalRBs = m_BW_RBs;
  uint16_t CBR_window_slots = 100/m_slotDuration;
  uint16_t N_Resources = CBR_window_slots*N_subCh, counter = 0;

//  double RSSIthresh = -88.0; // in dBm
//  NS_LOG_UNCOND("Ref sensitivity = " << m_rxSensitivity << ", RSSIthresh = " << m_RSSIthresh);
//  std::cin.get();

  NS_LOG_DEBUG("Evaluating CBR for UE " << m_rnti << " now: SF(" << frameNo << "," << subframeNo << "). Total number of Subchannels = " << N_Resources);

  // No need to update the list. It's already up-to-date
  std::map <uint16_t,std::map<SidelinkCommResourcePool::SubframeInfo, RSSImeas> >::iterator RSSIit;
  for (RSSIit = m_receivedRSSI.begin(); RSSIit != m_receivedRSSI.end(); RSSIit++)
  {
    NS_LOG_DEBUG("RB index " << RSSIit->first);
    std::map<SidelinkCommResourcePool::SubframeInfo, RSSImeas>::iterator subframeIt;
    for (subframeIt = RSSIit->second.begin(); subframeIt != RSSIit->second.end(); subframeIt++)
    {
   /*   std::cout << "SF(" << std::to_string(subframeIt->first.frameNo) << "," << std::to_string(subframeIt->first.subframeNo) << ") with IDs: ";
      for (std::vector<uint16_t>::iterator vectIT = subframeIt->second.m_IDs.begin(); vectIT != subframeIt->second.m_IDs.end(); vectIT++)
      {
        std::cout << (*vectIT) << ", ";
      }
      std::cout << "and RSSI " << subframeIt->second.m_rssi << std::endl;*/
      if( (! ( (subframeIt->first.frameNo == frameNo) && (subframeIt->first.subframeNo == subframeNo) ) ) && (subframeIt->second.m_rssi >= m_RSSIthresh) )
      {
        NS_LOG_DEBUG("Valid CBR entry: SF(" << subframeIt->first.frameNo << "," << subframeIt->first.subframeNo << ") and RSSI = " << subframeIt->second.m_rssi);
        counter++;
      }
      else
        NS_LOG_DEBUG("Non-valid CBR entry: SF(" << subframeIt->first.frameNo << "," << subframeIt->first.subframeNo << ") and RSSI = " << subframeIt->second.m_rssi);
      //NS_LOG_DEBUG("SF(" << subframeIt->first.frameNo << "," << subframeIt->first.subframeNo << ")");
    }
  }

 /* if (m_rnti == 3 && false)
  { 
    NS_LOG_DEBUG("Saving the new RSSI list");
    std::ofstream CBRlist_UE1;
    CBRlist_UE1.open (m_outputPath + "CBRlist_1.txt", std::ios_base::app);
    CBRlist_UE1 << "-------- UE " << m_rnti << " at SF(" << std::to_string(frameNo) << "," << std::to_string(subframeNo) << ")" << std::endl;
    for (RSSIit = m_receivedRSSI.begin(); RSSIit != m_receivedRSSI.end(); RSSIit++)
    {
      CBRlist_UE1 << "CSR index " << RSSIit->first << ":\r\n";
      std::map<SidelinkCommResourcePool::SubframeInfo, RSSImeas>::iterator subframeIt;
      for (subframeIt = RSSIit->second.begin(); subframeIt != RSSIit->second.end(); subframeIt++)
      {
        if( (! ( (subframeIt->first.frameNo == frameNo) && (subframeIt->first.subframeNo == subframeNo) ) ) && (subframeIt->second.m_rssi >= m_RSSIthresh) )
        {
          CBRlist_UE1 << "SF(" << std::to_string(subframeIt->first.frameNo) << "," << std::to_string(subframeIt->first.subframeNo) << ") with IDs: ";
          for (std::vector<uint16_t>::iterator vectIT = subframeIt->second.m_IDs.begin(); vectIT != subframeIt->second.m_IDs.end(); vectIT++)
          {
            CBRlist_UE1 << (*vectIT) << ", ";
          }
          CBRlist_UE1 << "and RSSI " << subframeIt->second.m_rssi << std::endl;
          //NS_LOG_DEBUG("SF(" << subframeIt->first.frameNo << "," << subframeIt->first.subframeNo << ")");
        }
      }
    }
    CBRlist_UE1 << std::endl;
    CBRlist_UE1.close();
  }*/

  NS_LOG_DEBUG("Number of occupied Subchannels = " << counter << ". Total number of Subchannels = " << N_Resources );

  CBRInfo tmp;
  tmp.nodeId = m_rnti;
  tmp.CBRvalue = (float)counter/N_Resources;
  tmp.time = Simulator::Now ().GetSeconds ();
  
  NrV2XUePhy::CBRValues.push_back(tmp);

}


void
NrV2XUePhy::ReceiveNistLteControlMessageList (std::list<Ptr<NistLteControlMessage> > msgList)
{
  NS_LOG_FUNCTION (this);
  std::list<Ptr<NistLteControlMessage> >::iterator it;
  // Iterate over the list of SCI messages received
  for (it = msgList.begin (); it != msgList.end (); it++)  
  {
    Ptr<NistLteControlMessage> msg = (*it);
    NS_LOG_INFO ("Message type=" << msg->GetMessageType ());
    if (msg->GetMessageType() == NistLteControlMessage::SCI_V2X)
    {  //TODO FIXME V2X V2X V2X V2X V2X V2X V2X V2X V2X V2X V2X V2X 
      Ptr<SciV2XLteControlMessage> msg2 = DynamicCast<SciV2XLteControlMessage> (msg);
      NistV2XSciListElement_s sci = msg2->GetSci ();
      NS_LOG_INFO("Received one SCI_V2X, Now: " << Simulator::Now().GetSeconds() << "s. From " << sci.m_rnti);
      // Sensing - based SPS: 
      SidelinkCommResourcePool::SubframeInfo reservedSF;
      SidelinkCommResourcePool::SubframeInfo receivedSF = sci.m_receivedSubframe; // the subframe at which the reservation (the SCI) is received;
      uint16_t rbStartPssch;
      uint16_t rbLenPssch;
      double psschRsrpDb = sci.m_psschRsrpDb;
      uint32_t CreselRx = std::min (sci.m_CreselRx, uint32_t(1000));
      if ((int) sci.m_reservation != 0)
      {
        NS_LOG_INFO ("sci.m_reservation != 0");
        reservedSF = sci.m_reservedSubframe; // the reserved subframe
        rbStartPssch = sci.m_rbStartPssch;
        rbLenPssch = sci.m_rbLenPssch;
   //     m_uePhySapUser -> ReportPsschRsrpReservation (Simulator::Now(), rbStartPssch-2, rbLenPssch+2, psschRsrpDb, receivedSF, reservedSF, CreselRx, sci.m_rnti, (uint16_t) sci.m_reservation); //Mode 4
        m_uePhySapUser -> ReportPsschRsrpReservation (Simulator::Now(), rbStartPssch, rbLenPssch, psschRsrpDb, receivedSF, reservedSF, CreselRx, sci.m_rnti, sci.m_reservation,0,0); 
        NS_LOG_INFO("Received SCI_V2X with reservation at SF(" << receivedSF.frameNo << "," << receivedSF.subframeNo << ") with reserved SF(" << reservedSF.frameNo << "," << reservedSF.subframeNo << "), RBstart: " 
        << rbStartPssch << ", RBLen: " << rbLenPssch << ", PSSCH-RSRP: " << psschRsrpDb << "dB" );
        if (sci.m_announceNextTxion)
        {
          rbStartPssch = sci.m_secondRbStartPssch;
          rbLenPssch = sci.m_secondRbLenPssch;
          reservedSF = sci.m_secondSubframe;
          m_uePhySapUser -> ReportPsschRsrpReservation (Simulator::Now(), rbStartPssch, rbLenPssch, psschRsrpDb, receivedSF, reservedSF, CreselRx, sci.m_rnti, sci.m_timeDiff,1,1); 
          NS_LOG_INFO("Received SCI_V2X with reservation at SF(" << receivedSF.frameNo << "," << receivedSF.subframeNo << ") with reserved SF(" << reservedSF.frameNo << "," << reservedSF.subframeNo << "), RBstart: " 
          << rbStartPssch << ", RBLen: " << rbLenPssch << ", PSSCH-RSRP: " << psschRsrpDb << "dB" );
          reservedSF = sci.m_secondReservedSubframe;
          m_uePhySapUser -> ReportPsschRsrpReservation (Simulator::Now(), rbStartPssch, rbLenPssch, psschRsrpDb, receivedSF, reservedSF, CreselRx, sci.m_rnti, sci.m_reservation + sci.m_timeDiff,1,0); 
          NS_LOG_INFO("Received SCI_V2X with reservation at SF(" << receivedSF.frameNo << "," << receivedSF.subframeNo << ") with reserved SF(" << reservedSF.frameNo << "," << reservedSF.subframeNo << "), RBstart: " 
          << rbStartPssch << ", RBLen: " << rbLenPssch << ", PSSCH-RSRP: " << psschRsrpDb << "dB" );
        }
      }
      else if (sci.m_announceNextTxion) 
      {
        rbStartPssch = sci.m_secondRbStartPssch;
        rbLenPssch = sci.m_secondRbLenPssch;
        reservedSF = sci.m_secondSubframe;
        m_uePhySapUser -> ReportPsschRsrpReservation (Simulator::Now(), rbStartPssch, rbLenPssch, psschRsrpDb, receivedSF, reservedSF, CreselRx, sci.m_rnti, sci.m_timeDiff,1,1); 
        NS_LOG_INFO("Received SCI_V2X with reservation at SF(" << receivedSF.frameNo << "," << receivedSF.subframeNo << ") with reserved SF(" << reservedSF.frameNo << "," << reservedSF.subframeNo << "), RBstart: " 
        << rbStartPssch << ", RBLen: " << rbLenPssch << ", PSSCH-RSRP: " << psschRsrpDb << "dB" );
      }
//      std::cin.get();
      //must check if the destination is one to monitor
      std::list <uint32_t>::iterator it;
      bool for_me = false; //dummy
      for (it = m_destinations.begin (); it != m_destinations.end () && !for_me; it++)
      {
        if (sci.m_groupDstId == ((*it) & 0xFF))
        {
          NS_LOG_INFO("received SCI_V2X for group " << (uint32_t)((*it) & 0xFF) << " from rnti " << sci.m_rnti);
          //TODO, how to find the pool among the available ones?
          //right now just use the first one
          std::list <PoolInfo>::iterator poolIt = m_sidelinkRxPools.begin();
          if (poolIt == m_sidelinkRxPools.end())
          {
            NS_LOG_INFO (this << " No Rx pool configured");
          }
          else
          {
            std::map<uint16_t, V2XSidelinkGrantInfo>::iterator grantIt = poolIt->m_currentV2XGrants.find (sci.m_rnti);
            if (grantIt == poolIt->m_currentV2XGrants.end())
            {
              V2XSidelinkGrantInfo v2xgrantInfo;
              v2xgrantInfo.m_v2x_grant_received = true;
              v2xgrantInfo.m_grant.m_rnti = sci.m_rnti;
              v2xgrantInfo.m_grant.m_rbStartPscch = sci.m_rbStartPscch;
              v2xgrantInfo.m_grant.m_rbLenPscch = sci.m_rbLenPscch;
              v2xgrantInfo.m_grant.m_rbStartPssch = sci.m_rbStartPssch;
              v2xgrantInfo.m_grant.m_rbLenPssch = sci.m_rbLenPssch;
              v2xgrantInfo.m_grant.m_groupDstId = sci.m_groupDstId;
              v2xgrantInfo.m_grant.m_mcs = sci.m_mcs;
              v2xgrantInfo.m_grant.m_tbSize = sci.m_tbSize;
              // v2xgrantInfo.m_grant.frameNo = frameNo;
              // v2xgrantInfo.m_grant.subframeNo = subframeNo;
              NS_LOG_LOGIC("Adding v2x sl rx grant at phy layer at time: " << Simulator::Now().GetSeconds()*1000 << "ms");
              //insert grant
              poolIt->m_currentV2XGrants.insert (std::pair <uint16_t, V2XSidelinkGrantInfo> (sci.m_rnti, v2xgrantInfo));
              /*   std::vector <int> rbMapPssch;
              for (int i = v2xgrantInfo.m_grant.m_rbStartPssch; i < v2xgrantInfo.m_grant.m_rbStartPssch + v2xgrantInfo.m_grant.m_rbLenPssch; i++)
              {
                rbMapPssch.push_back (i);
              }*/
              //m_sidelinkSpectrumPhy->AddExpectedTb (v2xgrantInfo.m_grant.m_rnti, v2xgrantInfo.m_grant.m_groupDstId, 1, v2xgrantInfo.m_grant.m_tbSize, v2xgrantInfo.m_grant.m_mcs, rbMapPssch, 1);
            } //else it should be the retransmission and the data should be the same...add check
            else
            {
              NS_LOG_DEBUG ("SCI_V2X Grant already present");
            }
          }
          //m_uePhySapUser->ReceiveNistLteControlMessage (msg);
        }             
      } //END OF for cycle
    }
    else
    {
      // pass the message to UE-MAC
      NS_ASSERT_MSG(false, "NON-SCI_V2X control message");
      m_uePhySapUser->ReceiveNistLteControlMessage (msg);
    }
  }
}


void
NrV2XUePhy::ReceivePss (uint16_t cellId, Ptr<SpectrumValue> p)
{
  NS_LOG_FUNCTION (this << cellId << (*p));
  //Useless
} // end of void NrV2XUePhy::ReceivePss (uint16_t cellId, Ptr<SpectrumValue> p)


void
NrV2XUePhy::QueueSubChannelsForTransmission (std::vector <int> rbMap)
{
 // m_subChannelsForTransmissionQueue.at (m_macChTtiDelay - 1) = rbMap;
  //Useless
}



void
NrV2XUePhy::SubframeIndication (uint32_t frameNo, uint32_t subframeNo)
{
  NS_LOG_FUNCTION (this << " frame " << frameNo << " subframe " << subframeNo << " rnti " << m_rnti);
  bool TransmissionEnabled;
  m_uePhySapUser->SubframeIndication (frameNo, subframeNo);
  NS_ASSERT_MSG (frameNo > 0, "the SRS index check code assumes that frameNo starts at 1");
  //NS_LOG_UNCOND("Grant Size:" << m_sidelinkRxPools.front().m_currentGrants.size());
  // refresh internal variables
  m_rsReceivedPowerUpdated = false;
  m_rsInterferencePowerUpdated = false;
  m_pssReceived = false;
  // Clear expected TB not received in previous subframes
  m_sidelinkSpectrumPhy->ClearExpectedSlTb();
  //Notify RRC about the current Subframe indication
  m_ueCphySapUser->ReportSubframeIndication(frameNo, subframeNo);
  //If a change of timing (resynchronization) was requested before, do the change of frameNo and subframeNo if possible
  // Do it here for avoiding  misalignments of subframe indications
  if (m_resyncRequested)
  {
    NS_LOG_LOGIC(this <<" (re)synchronization requested ");
    if(ChangeOfTiming(frameNo, subframeNo) )
    {
      frameNo = m_currFrameNo;
      subframeNo = m_currSubframeNo;
      NS_LOG_LOGIC(this << " (re)synchronization successfully performed ");
    }
    else
    {
      NS_LOG_LOGIC(this <<" (re)synchronization postponed ");
    }
  }

  if (m_ulConfigured)
  {
    if (m_slTxPoolInfo.m_pool)
    {
      //Check if we need to initialize the Tx pool
      if (m_slTxPoolInfo.m_nextScPeriod.frameNo == 0)
      {
        //pool not initialized yet
        m_slTxPoolInfo.m_nextScPeriod = m_slTxPoolInfo.m_pool->GetNextScPeriod (frameNo, subframeNo);
        //adjust because scheduler starts with frame/subframe = 1
        m_slTxPoolInfo.m_nextScPeriod.frameNo++;
        m_slTxPoolInfo.m_nextScPeriod.subframeNo++;
        NS_LOG_INFO (this << " Tx Pool initialized: SF(" << m_slTxPoolInfo.m_nextScPeriod.frameNo << "," << m_slTxPoolInfo.m_nextScPeriod.subframeNo << ")");
      }
      //Check if this is a new SC period
      //NS_LOG_DEBUG (this << "Checking if beginning of next period " << m_slTxPoolInfo.m_nextScPeriod.frameNo << "/" << m_slTxPoolInfo.m_nextScPeriod.subframeNo);
      if (frameNo == m_slTxPoolInfo.m_nextScPeriod.frameNo && subframeNo == m_slTxPoolInfo.m_nextScPeriod.subframeNo)
      {
        m_slTxPoolInfo.m_currentScPeriod = m_slTxPoolInfo.m_nextScPeriod;
        m_slTxPoolInfo.m_nextScPeriod = m_slTxPoolInfo.m_pool->GetNextScPeriod (frameNo, subframeNo);
        //adjust because scheduler starts with frame/subframe = 1
        m_slTxPoolInfo.m_nextScPeriod.frameNo++;
        m_slTxPoolInfo.m_nextScPeriod.subframeNo++;
        NS_LOG_INFO (this << " Starting new SC period for TX pool " << ". Next period at " << m_slTxPoolInfo.m_nextScPeriod.frameNo << "/" << m_slTxPoolInfo.m_nextScPeriod.subframeNo);
        if (m_waitingNextScPeriod)
        {
          NS_LOG_LOGIC (this << " the UE was waiting for next SC period and it just started");
          m_waitingNextScPeriod = false;
        }
        //clear any previous grant
        m_slTxPoolInfo.m_currentGrants.clear();
      }
    }  //END OF if (m_slTxPoolInfo.m_pool)

    //check if we received grants for sidelink
    //compute the reception slots for the PSSCH. Do this here because
    //we did not have access to the frame/subframe number at the reception
    std::list <PoolInfo>::iterator it; //PoolInfo already keeps v2x info 
    // --------------- RECEPTION STUFF --------------------------
    for (it = m_sidelinkRxPools.begin() ; it != m_sidelinkRxPools.end () ; it++)  
    {
      //TODO FIXME V2X STUFF FOR RECEPTION!! --------------RECEPTION---------------
      if (true)  // if V2X Operation
      {    
        std::map <uint16_t, V2XSidelinkGrantInfo>::iterator grantIt = it->m_currentV2XGrants.begin();
        while (grantIt != it->m_currentV2XGrants.end())
        {   //This is ok :)
          //NS_LOG_UNCOND("Found rx phy V2X pool at Frame " << frameNo << ", subframe " << subframeNo);
          std::list<SidelinkCommResourcePool::V2XSidelinkTransmissionInfo>::iterator rxIt;
          if (grantIt->second.m_v2x_grant_received)
          {
            NS_LOG_DEBUG (this << " New V2X grant received, SF:(" << frameNo << "," << subframeNo << ")");
            //TODO: how to identify pool if multiple are presents? 
            /*SidelinkCommResourcePool::SubframeInfo tmp = it->m_pool->GetCurrentScPeriod(frameNo, subframeNo);
            grantIt->second.m_psschTx = it->m_pool->GetPsschTransmissions (tmp, grantIt->second.m_grant.m_trp, grantIt->second.m_grant.m_rbStart, grantIt->second.m_grant.m_rbLen);
            for (rxIt = grantIt->second.m_psschTx.begin (); rxIt != grantIt->second.m_psschTx.end (); rxIt++)
            {   
              rxIt->subframe.frameNo++;
              rxIt->subframe.subframeNo++;
              NS_LOG_INFO (this << " Subframe Rx" << rxIt->subframe.frameNo << "/" << rxIt->subframe.subframeNo << ": rbStart=" << (uint32_t) rxIt->rbStart << ", rbLen=" << (uint32_t) rxIt->nbRb);
            }*/
            grantIt->second.m_v2xTx = it->m_pool->GetV2XSlTransmissions (frameNo, subframeNo, grantIt->second.m_grant.m_rbStartPscch, grantIt->second.m_grant.m_rbStartPssch, grantIt->second.m_grant.m_rbLenPssch,  grantIt->second.m_grant.m_rbLenPscch, 0, 0);
            for (rxIt = grantIt->second.m_v2xTx.begin (); rxIt != grantIt->second.m_v2xTx.end (); rxIt++)
            {
              rxIt->subframe.frameNo++;
              rxIt->subframe.subframeNo++;
              //NS_LOG_INFO (this << " Subframe Rx" << rxIt->subframe.frameNo << "/" << rxIt->subframe.subframeNo << ": rbStart=" << (uint32_t) rxIt->rbStart << ", rbLen=" << (uint32_t) rxIt->nbRb);
            } 
            grantIt->second.m_v2x_grant_received =false;
            //NS_LOG_UNCOND("Now: SF: (" << frameNo << "," << subframeNo << "), grantIt->second.m_v2xTx.subframe: SF: (" << (*grantIt->second.m_v2xTx.begin()).subframe.frameNo << "," << (*grantIt->second.m_v2xTx.begin()).subframe.subframeNo << ")");
          } //END OF if(grant received)
          //now check if there is any grant for the current subframe and pass them to lower layer
          rxIt = grantIt->second.m_v2xTx.begin();
          if (rxIt != grantIt->second.m_v2xTx.end())
          {
            NS_LOG_DEBUG (frameNo << "/" << subframeNo << " RNTI=" << m_rnti << " next pssch at " << (*rxIt).subframe.frameNo << "/" << (*rxIt).subframe.subframeNo);
          }
          if (rxIt != grantIt->second.m_v2xTx.end() && (*rxIt).subframe.frameNo == frameNo && (*rxIt).subframe.subframeNo == subframeNo)
          { 
            //NS_LOG_UNCOND("Now: SF: (" << frameNo << "," << subframeNo << "), grantIt->second.m_v2xTx.subframe: SF: (" << (*rxIt).subframe.frameNo << "," << (*rxIt).subframe.subframeNo << ")");
            //reception
            // NS_LOG_INFO (this << " Expecting PSSCH + PSCCH reception RB " << (uint16_t) grantIt->second.m_grant.m_rbStart << " to " << (uint16_t) (grantIt->second.m_grant.m_rbStart + grantIt->second.m_grant.m_rbLen - 1));
            std::vector <int> rbMapPssch;
            for (int i = grantIt->second.m_grant.m_rbStartPssch; i < grantIt->second.m_grant.m_rbStartPssch + grantIt->second.m_grant.m_rbLenPssch; i++)
            {
              rbMapPssch.push_back (i);
            }
            //remove reception information
            grantIt->second.m_v2xTx.erase (rxIt);
          }
          if (grantIt->second.m_v2xTx.size() == 0)
          {
            //no more PSSCH transmission, clear the grant
            it->m_currentV2XGrants.erase (grantIt++);
          }
          else 
          {
            grantIt++;
          } //END if (reception in this subframe)
        }
      }// END if (V2X Operation)
    } // END for (it = m_sidelinkRxPools.begin() ; it != m_sidelinkRxPools.end () ; it++)

    // ---------- SWITCHING TO TRANSMISSION ----------------
    // update uplink transmission mask according to previous UL-CQIs
    std::vector <int> rbMask;
//    std::vector <int> rbMask = m_subChannelsForTransmissionQueue.at (0);
//    SetSubChannelsForTransmission (m_subChannelsForTransmissionQueue.at (0), 1);
//    std::cin.get();
    // shift the queue
/*    for (uint8_t i = 1; i < m_macChTtiDelay; i++)
    {
      m_subChannelsForTransmissionQueue.at (i-1) = m_subChannelsForTransmissionQueue.at (i);
    }*/
//    m_subChannelsForTransmissionQueue.at (m_macChTtiDelay-1).clear ();
    m_subChannelsForTransmissionQueue.at (m_macChTtiDelay).clear ();
    if (m_srsConfigured && (m_srsStartTime <= Simulator::Now ()))
    {
      NS_ASSERT_MSG (subframeNo > 0 && subframeNo <= 10, "the SRS index check code assumes that subframeNo starts at 1");
      if ((((frameNo-1)*10 + (subframeNo-1)) % m_srsPeriodicity) == m_srsSubframeOffset)
      {
        NS_LOG_INFO (this << " frame " << frameNo << " subframe " << subframeNo << " sending SRS (offset=" << m_srsSubframeOffset << ", period=" << m_srsPeriodicity << ")");
        m_sendSrsEvent = Simulator::Schedule (UL_SRS_DELAY_FROM_SUBFRAME_START, &NrV2XUePhy::SendSrs, this);
      }
    }
    //If rbMask has non empty RBs, it means we are expected to send messages in the uplink
    //otherwise we check if there are sidelink transmissions
    //Is this true if there are only control messages?
    std::list<Ptr<NistLteControlMessage> > ctrlMsg = GetControlMessages ();
    // retrieve the current bursts of packets ready for transmission
    Ptr<PacketBurst> pb = GetPacketBurst ();
    bool sciDiscfound = false;
    bool mibslfound = false;
    bool sciv2xfound = false;
    if (rbMask.size () == 0)
    {
      //we do not have uplink data to send. Normally, uplink has priority over sidelink but
      //since we send UL CQI messages all the time, we can remove them if we have a sidelink
      //transmission
      std::list<Ptr<NistLteControlMessage> >::iterator ctrlIt;
      for (ctrlIt=ctrlMsg.begin() ; ctrlIt != ctrlMsg.end() && !sciDiscfound; ctrlIt++) 
      {
        sciDiscfound = (*ctrlIt)->GetMessageType () == NistLteControlMessage::SCI || (*ctrlIt)->GetMessageType () == NistLteControlMessage::SL_DISC_MSG || (*ctrlIt)->GetMessageType () == NistLteControlMessage::SCI_V2X;
        mibslfound = (*ctrlIt)->GetMessageType() == NistLteControlMessage::MIB_SL;
        sciv2xfound = (*ctrlIt)->GetMessageType () == NistLteControlMessage::SCI_V2X;  // Change to true the sciv2xfound variable when it's a v2x transmission
      }
      if (pb || sciDiscfound || mibslfound)
      { 
        if(pb){ NS_LOG_INFO("Packet Burst"); }
        if(sciDiscfound){ NS_LOG_INFO("SCIDisc Found"); }
        if(mibslfound){ NS_LOG_INFO("MIB_SL Found"); }
        if(sciv2xfound){ NS_LOG_INFO("SCI_V2X Found"); } 
        //we have sidelink to send, purge the control messages
        ctrlIt=ctrlMsg.begin();
        while (ctrlIt != ctrlMsg.end())
        {
          NS_LOG_INFO ("Message type = " << (*ctrlIt)->GetMessageType ());
          if ((*ctrlIt)->GetMessageType () == NistLteControlMessage::DL_CQI || (*ctrlIt)->GetMessageType () == NistLteControlMessage::BSR)
          {
            ctrlIt = ctrlMsg.erase (ctrlIt);
          }
          else
          {
            NS_ASSERT ((*ctrlIt)->GetMessageType () == NistLteControlMessage::SCI
            || (*ctrlIt)->GetMessageType () == NistLteControlMessage::SCI_V2X
            || (*ctrlIt)->GetMessageType() == NistLteControlMessage::MIB_SL
            || (*ctrlIt)->GetMessageType () == NistLteControlMessage::SL_DISC_MSG);
            ctrlIt++;
          }
        } //END while
      } //END if( pb || sciDiscFound || mibslfound)
    } //END if (rbMask.size () == 0)
    if (rbMask.size() != 0 || (ctrlMsg.size () > 0 && (*ctrlMsg.begin())->GetMessageType () != NistLteControlMessage::SCI
       && (*ctrlMsg.begin())->GetMessageType () != NistLteControlMessage::SCI_V2X
       && (*ctrlMsg.begin())->GetMessageType () != NistLteControlMessage::MIB_SL
       && (*ctrlMsg.begin())->GetMessageType () != NistLteControlMessage::SL_DISC_MSG))
    {
      // send packets in queue
      NS_LOG_LOGIC (this << " Not sidelink control messages");
    } // END if (not sidelink control messages)
    else
    {
      //check sidelink
      //check if there is a SLSS message to be transmitted
      bool mibSLfound = false;
      std::list<Ptr<NistLteControlMessage> >::iterator ctrlIt;
      for (ctrlIt = ctrlMsg.begin(); ctrlIt != ctrlMsg.end(); ctrlIt++)
      {
        if ((*ctrlIt)->GetMessageType() == NistLteControlMessage::MIB_SL)
        {
          mibSLfound = true;
        }
      }
      //TODO FIXME Transmit V2X Sidelink data  -- > Not tested yet!!!!!
      if (true) //if (V2X Operation)
      {   //clear any previous grant
        //NS_LOG_UNCOND("Clearing previous PHY tx grants...");
        m_slTxPoolInfo.m_currentV2XGrants.clear();
        if (pb)
        {   
          //NS_ASSERT(sciv2xfound, "Sidelink Mode 3 and 4 mandate that PSSCH and PSCCH are sent in the same subframe")
          NS_LOG_LOGIC (this << " UE - start TX V2X PSSCH + PSCCH");
          NS_LOG_DEBUG (this << " V2X TX Burst containing " << pb->GetNPackets() << " V2X sidelink packets");
          /*V2X PSCCH: retrieve information and build the phy layer tx grant*/
          std::list<Ptr<NistLteControlMessage> >::iterator msgIt = ctrlMsg.begin();
          //skipping the MIB-SL if it is the first in the list
          if((*msgIt)->GetMessageType () != NistLteControlMessage::SCI_V2X && (*msgIt)->GetMessageType () != NistLteControlMessage::SL_DISC_MSG)
          {
            msgIt++;  // advance the iterator
          }
          else if ((*msgIt)->GetMessageType () == NistLteControlMessage::SCI_V2X)
          {
            NS_LOG_INFO (this << " UE - start TX V2X PSCCH");
            //access the control message to store the PSSCH grant and be able to
            //determine the subframes/RBs for PSSCH transmissions
            NS_ASSERT_MSG ((*msgIt)->GetMessageType () == NistLteControlMessage::SCI_V2X, "Received " << (*msgIt)->GetMessageType ());
            Ptr<SciV2XLteControlMessage> msg2 = DynamicCast<SciV2XLteControlMessage> (*msgIt);
            NistV2XSciListElement_s sci = msg2->GetSci ();
            std::map<uint16_t, V2XSidelinkGrantInfo>::iterator grantIt = m_slTxPoolInfo.m_currentV2XGrants.find (sci.m_rnti);
            if (grantIt == m_slTxPoolInfo.m_currentV2XGrants.end ()) // If no grants are present yet
            {
              if (sci.m_hopping == 0x00)
                TransmissionEnabled = false;
 	      else
                TransmissionEnabled = true;
              NS_LOG_DEBUG("Transmission is enabled? " <<TransmissionEnabled);
              V2XSidelinkGrantInfo v2xgrantInfo;
              //this is the first transmission of PSCCH
              v2xgrantInfo.m_v2x_grant_received = true;
              v2xgrantInfo.m_grant.m_rnti = sci.m_rnti;
              v2xgrantInfo.m_grant.m_rbStartPscch = sci.m_rbStartPscch;
              v2xgrantInfo.m_grant.m_rbLenPscch = sci.m_rbLenPscch;
              v2xgrantInfo.m_grant.m_rbStartPssch = sci.m_rbStartPssch;
              v2xgrantInfo.m_grant.m_rbLenPssch = sci.m_rbLenPssch;
              NS_LOG_INFO("Reserved RBs " << (int)sci.m_rbLenPssch << " occupied " << (int)sci.m_rbLenPssch_TB);
              //grantInfo.m_grant.m_trp = sci.m_trp;
              v2xgrantInfo.m_grant.m_groupDstId = sci.m_groupDstId;
              v2xgrantInfo.m_grant.m_mcs = sci.m_mcs;
              v2xgrantInfo.m_grant.m_tbSize = sci.m_tbSize;
              v2xgrantInfo.m_grant.frameNo = frameNo;
              v2xgrantInfo.m_grant.subframeNo = subframeNo;

              v2xgrantInfo.m_v2xTx = m_slTxPoolInfo.m_pool->GetV2XSlTransmissions (frameNo, subframeNo, sci.m_rbStartPscch, sci.m_rbStartPssch, sci.m_rbLenPssch_TB, sci.m_rbLenPscch, 0, 0);

              // Saving the PHY layer information for every node
              if (TransmissionEnabled)
	      {
                TxPacketInfo tmp;
                tmp.txTime = Simulator::Now ().GetSeconds ();
                tmp.genTime = sci.m_genTime;
                tmp.nodeId = sci.m_rnti;
                tmp.packetId = sci.m_packetID;
                tmp.txIndex = sci.m_TxIndex;
                tmp.txFrame.frameNo = frameNo;
                tmp.txFrame.subframeNo = subframeNo;
                tmp.Cresel = sci.m_CreselRx;
                tmp.RRI = sci.m_reservation;
                tmp.psschRbStart = sci.m_rbStartPssch ;
                tmp.psschRbLen = sci.m_rbLenPssch;
                tmp.psschRbLenTb = sci.m_rbLenPssch_TB;
                tmp.pscchRbStart = sci.m_rbStartPscch;
                tmp.pscchRbLen = sci.m_rbLenPscch;

                NrV2XUePhy::txPackets.push_back(tmp);
 
                NS_LOG_INFO("NODE " << (int)sci.m_rnti  << " tx packet " <<  sci.m_packetID << " at time: " << Simulator::Now ().GetSeconds () << ". Tx SF(" << frameNo << "," << subframeNo << "), Reselection Counter = " << sci.m_CreselRx << ", RRI = " << (int)sci.m_reservation 
                << ", (PSSCH) RBs start " << sci.m_rbStartPssch << ", (PSSCH) reservation length = " << sci.m_rbLenPssch << " RBs, (PSSCH) occupied length = " << sci.m_rbLenPssch_TB << " RBs, (PSCCH) RBs start " << sci.m_rbStartPscch << ", (PSCCH) reservation length = " << sci.m_rbLenPscch << " RBs");

              }
              if (Simulator::Now ().GetSeconds () - NrV2XUePhy::prevPrintTime >= m_savingPeriod)
              {
                NrV2XUePhy::prevPrintTime = Simulator::Now ().GetSeconds (); 

                std::ofstream phyDebugShort;
                phyDebugShort.open (m_outputPath + "phyDebugShort.txt", std::ios_base::app);
                for (std::vector<TxPacketInfo>::iterator txIT = NrV2XUePhy::txPackets.begin(); txIT != NrV2XUePhy::txPackets.end(); txIT++)
                {
                  phyDebugShort << txIT->nodeId << "," <<  txIT->packetId << "," << txIT->genTime << "," << txIT->txIndex << "," << txIT->txTime << "," << txIT->txFrame.frameNo << "," << txIT->txFrame.subframeNo << "," << txIT->Cresel << "," << txIT->RRI
                  << "," << txIT->psschRbStart << "," << txIT->psschRbLen << "," << txIT->psschRbLenTb << "," << txIT->pscchRbStart << "," << txIT->pscchRbLen << std::endl;
                }
                phyDebugShort.close ();

             /*   std::ofstream phyDebugALL;
                phyDebugALL.open (m_outputPath + "phyDebugALL.txt", std::ios_base::app);
                for (std::vector<TxPacketInfo>::iterator txIT = NrV2XUePhy::txPackets.begin(); txIT != NrV2XUePhy::txPackets.end(); txIT++)
                {
                  phyDebugALL << "NODE " << txIT->nodeId << ", packet " <<  txIT->packetId << " (" << txIT->genTime << ") @ tx index: " <<  txIT->txIndex << " at time: " << txIT->txTime << ". Tx SF(" << txIT->txFrame.frameNo << "," << txIT->txFrame.subframeNo << "), Reselection Counter = " << txIT->Cresel << ", RRI = " << txIT->RRI
                  << ", (PSSCH) RBs start " << txIT->psschRbStart << ", (PSSCH) reservation length = " << txIT->psschRbLen << " RBs, (PSSCH) occupied length = " << txIT->psschRbLenTb << " RBs, (PSCCH) RBs start " << txIT->pscchRbStart << ", (PSCCH) reservation length = " << txIT->pscchRbLen << " RBs\r\n";
                }
                phyDebugALL.close (); */

                NrV2XUePhy::txPackets.clear();
              }

              std::list<SidelinkCommResourcePool::V2XSidelinkTransmissionInfo>::iterator txIt;
              for (txIt = v2xgrantInfo.m_v2xTx.begin (); txIt != v2xgrantInfo.m_v2xTx.end (); txIt++)
              {
                /*
                if (txIt->subframe.frameNo == 0)
                {
                  txIt->subframe.frameNo++;
                }
                if (txIt->subframe.subframeNo == 0)
                {
                  txIt->subframe.subframeNo++;
                }
                */
                txIt->subframe.frameNo++;
                txIt->subframe.subframeNo++;
                //NS_LOG_INFO (this << " Subframe Tx" << txIt->subframe.frameNo << "/" << txIt->subframe.subframeNo << ": rbStart=" << (uint32_t) txIt->rbStart << ", rbLen=" << (uint32_t) txIt->nbRb);
              } 
              //insert TX grant
              m_slTxPoolInfo.m_currentV2XGrants.insert (std::pair <uint16_t, V2XSidelinkGrantInfo> (sci.m_rnti, v2xgrantInfo));
              NS_LOG_INFO (this <<  " Creating grant at SF(" << v2xgrantInfo.m_grant.frameNo << "," << v2xgrantInfo.m_grant.subframeNo << ")");
            }
            else
            {
              NS_LOG_INFO (this <<  " Grant created at " << grantIt->second.m_grant.frameNo << "/" << grantIt->second.m_grant.subframeNo);
            }
          } //END if((*msgIt)->GetMessageType () == NistLteControlMessage::SCI_V2X)
          std::list<SidelinkCommResourcePool::V2XSidelinkTransmissionInfo>::iterator txIt = m_slTxPoolInfo.m_currentV2XGrants.begin()->second.m_v2xTx.begin ();
          /*  txIt -> subframe.frameNo++; 
              txIt -> subframe.subframeNo++;
          */ 
          NS_LOG_INFO(m_slTxPoolInfo.m_currentV2XGrants.begin()->second.m_v2xTx.size());
          while ((txIt->subframe.frameNo < frameNo || (txIt->subframe.frameNo == frameNo && txIt->subframe.subframeNo < subframeNo)) && txIt->nbRbPssch!=0)
          {
            txIt = m_slTxPoolInfo.m_currentV2XGrants.begin()->second.m_v2xTx.erase (txIt);
            if (txIt == m_slTxPoolInfo.m_currentV2XGrants.begin()->second.m_v2xTx.end()) 
            {
              NS_LOG_ERROR ("Reached end of transmission list");
            }
          }
          NS_ASSERT (txIt != m_slTxPoolInfo.m_currentV2XGrants.begin()->second.m_v2xTx.end()); //must be at least one element
          NS_LOG_INFO ("txIt->subframe.frameNo = " << txIt->subframe.frameNo << ", txIt->subframe.subframeNo = " << txIt->subframe.subframeNo);
          NS_ASSERT_MSG (txIt->subframe.frameNo == frameNo && txIt->subframe.subframeNo == subframeNo, "Found " << txIt->subframe.frameNo << "/" << txIt->subframe.subframeNo); //there must be an opportunity in this subframe
          NS_ASSERT (rbMask.size() == 0);
          // Occupy RBs also for PSCCH, in the same subframe!!!
          std::vector<int> slRb;
       //   for (int jc = (int) txIt->rbStartPscch ; jc < (int) txIt->rbStartPscch + 2 ; jc++)
          for (uint16_t jc = txIt->rbStartPscch ; jc < txIt->rbStartPscch + txIt->nbRbPscch ; jc++)
          {
            NS_LOG_INFO (this << " Transmitting PSCCH on RB " << jc);
            slRb.push_back (jc);
          }
          NS_LOG_INFO("Occupied RBs for SCI at SF:(" << frameNo << "," << subframeNo << ") :[" << slRb.front() << "," << slRb.back() << "]");
          /*Now consider the PSSCH*/
          NS_LOG_INFO("txIt->rbStartPssch: " << txIt->rbStartPssch << ", txIt->nbRbPssch: " << txIt->nbRbPssch);
          for (uint16_t i = txIt->rbStartPssch ; i < txIt->rbStartPssch + txIt->nbRbPssch ; i++)
          {
            NS_LOG_INFO (this << " Transmitting PSSCH on RB " << i);
            rbMask.push_back (i);
          }
          NS_LOG_INFO("Occupied RBs for TB at SF:(" << frameNo << "," << subframeNo << ") :[" << rbMask.front() << "," << rbMask.back() << "]");
        //  std::cin.get();
          m_slTxPoolInfo.m_currentV2XGrants.begin()->second.m_v2xTx.erase (txIt); 
          if (m_enableUplinkPowerControl)
          {
            m_txPower = m_powerControl->GetPsschTxPower (rbMask); //this should be done separately for PSCCH and for PSCCH
          }
          if (!mibSLfound)
          {
            if(m_ueSlssScanningInProgress)
            {
              NS_LOG_LOGIC(this <<" trying to do a PSSCH transmission while there is a scanning in progress... Ignoring transmission");
            }
            else if(m_ueSlssMeasurementsSched.find(Simulator::Now().GetMilliSeconds()) != m_ueSlssMeasurementsSched.end())
            {
              NS_LOG_LOGIC(this << " trying to do a PSSCH transmission while measuring S-RSRP in the same subframe... Ignoring transmission");
            }
            else if (TransmissionEnabled)
            {  
	      NS_LOG_INFO("I'm starting to transmit...");
//              std::cin.get();
              std::vector<int> v2xRbMask;
//              std::merge(slRb.begin(),slRb.end(),rbMask.begin(),rbMask.end(),std::back_inserter (v2xRbMask)); // Mode 4
              v2xRbMask = rbMask; //Mode 2 (SCI and TB are on the same PRBs)
//              for (std::vector<int>::iterator iitt = v2xRbMask.begin(); iitt != v2xRbMask.end(); iitt++)
//                NS_LOG_UNCOND("PRB mask: " << *iitt);
              m_currentMCS = m_slTxPoolInfo.m_currentV2XGrants.begin()->second.m_grant.m_mcs; //Update the MCS variable needed to create the tx PSD
              SetSubChannelsForTransmission (v2xRbMask); // MERGE THE PSCCH and PSSCH rb mask
            //  m_uplinkSpectrumPhy->StartTxV2XSlDataFrame (pb, ctrlMsg, UL_DATA_DURATION, m_slTxPoolInfo.m_currentV2XGrants.begin()->second.m_grant.m_groupDstId); //TODO: built new method for v2x
              m_uplinkSpectrumPhy->StartTxV2XSlDataFrame (pb, ctrlMsg, m_SL_DATA_DURATION, m_slTxPoolInfo.m_currentV2XGrants.begin()->second.m_grant.m_groupDstId); //TODO: built new method for v2x
             // std::cin.get();
              // store Tx info
              SidelinkCommResourcePool::SubframeInfo currentSFInfo;
              currentSFInfo.frameNo = frameNo; 
              currentSFInfo.subframeNo = subframeNo; 
            //  m_uePhySapUser -> StoreTxInfo (currentSFInfo, (uint16_t) txIt->rbStartPscch, (uint16_t) (2 + txIt->nbRbPssch)); //Mode 4
              m_uePhySapUser -> StoreTxInfo (currentSFInfo, txIt->rbStartPssch, txIt->nbRbPssch); 
            //  std::cin.get();
            }
          }  // END if (!mibSLfound)
        } // END if(pb)
      } // END if(V2X operation)
      else
      {
        NS_ASSERT_MSG(false, "NON-V2X operation");
      }
    }  // m_configured
  }
  std::ofstream simTick;  
  // trigger the MAC
//  m_uePhySapUser->SubframeIndication (frameNo, subframeNo);
  m_subframeNo = subframeNo;
  ++subframeNo;
  if (subframeNo > 10)
  {
    if (m_rnti == 1)
    {
      std::cout << Simulator::Now ().GetSeconds () << std::endl;
     /* simTick.open (m_outputPath + "simTick.csv");
      simTick << Simulator::Now ().GetSeconds () << "\r\n";
      simTick.close ();*/
    }
    ++frameNo;
    if (frameNo > 1024) { // Reset the frame number 
      frameNo = 1;
    }
    subframeNo = 1;
  }
  // NS_LOG_DEBUG("TTI " << GetTti() << " SL duration " << m_SL_DATA_DURATION.GetNanoSeconds());
  // Schedule next subframe indication (GetTti() is inherited from the nist-lte-phy class)
  Simulator::Schedule (Seconds (GetTti ()), &NrV2XUePhy::SubframeIndication, this, frameNo, subframeNo);

}


void
NrV2XUePhy::SendSrs ()
{
  NS_LOG_FUNCTION (this << " UE " << m_rnti << " start tx SRS, cell Id " << (uint32_t) m_cellId);
  //Useless
}


void
NrV2XUePhy::DoReset ()
{
  NS_LOG_FUNCTION (this);

  m_rnti = 0;
  m_transmissionMode = 0;
  m_srsPeriodicity = 0;
  m_srsConfigured = false;
  m_dlConfigured = false;
  m_ulConfigured = false;
  m_raPreambleId = 255; // value out of range
  m_raRnti = 11; // value out of range
  m_rsrpSinrSampleCounter = 0;
  m_p10CqiLast = Simulator::Now ();
  m_a30CqiLast = Simulator::Now ();
  m_paLinear = 1;

  m_packetBurstQueue.clear ();
  m_controlMessagesQueue.clear ();
  m_subChannelsForTransmissionQueue.clear ();
/*  for (int i = 0; i <= m_macChTtiDelay; i++) //Old implementation
    {
      Ptr<PacketBurst> pb = CreateObject <PacketBurst> ();
      m_packetBurstQueue.push_back (pb);
      std::list<Ptr<NistLteControlMessage> > l;
      m_controlMessagesQueue.push_back (l);
    }*/
  Ptr<PacketBurst> pb = CreateObject <PacketBurst> ();
  m_packetBurstQueue.push_back (pb);
  std::list<Ptr<NistLteControlMessage> > l;
  m_controlMessagesQueue.push_back (l);

  std::vector <int> ulRb;
//  m_subChannelsForTransmissionQueue.resize (m_macChTtiDelay, ulRb);
  m_subChannelsForTransmissionQueue.resize (m_macChTtiDelay+1, ulRb);

  m_sendSrsEvent.Cancel ();
  m_downlinkSpectrumPhy->Reset ();
  m_uplinkSpectrumPhy->Reset ();

} // end of void NrV2XUePhy::DoReset ()

void
NrV2XUePhy::DoStartCellSearch (uint16_t dlEarfcn)
{
  NS_LOG_FUNCTION (this << dlEarfcn);
  m_dlEarfcn = dlEarfcn;
  DoSetDlBandwidth (6); // configure DL for receiving PSS
  SwitchToState (CELL_SEARCH);
}

void
NrV2XUePhy::DoSynchronizeWithEnb (uint16_t cellId, uint16_t dlEarfcn)
{
  NS_LOG_FUNCTION (this << cellId << dlEarfcn);
  m_dlEarfcn = dlEarfcn;
  DoSynchronizeWithEnb (cellId);
}

void
NrV2XUePhy::DoSynchronizeWithEnb (uint16_t cellId)
{
  NS_LOG_FUNCTION (this << cellId);
  //Useless
}

void
NrV2XUePhy::DoSetDlBandwidth (uint16_t dlBandwidth)
{
  NS_LOG_FUNCTION (this << (uint32_t) dlBandwidth);
  if (m_dlBandwidth != dlBandwidth or !m_dlConfigured)
    {
      m_dlBandwidth = dlBandwidth;

      static const int Type0AllocationRbg[4] = {
        10,     // RGB size 1
        26,     // RGB size 2
        63,     // RGB size 3
        110     // RGB size 4
      };  // see table 7.1.6.1-1 of 36.213
      for (int i = 0; i < 4; i++)
        {
          if (dlBandwidth < Type0AllocationRbg[i])
            {
              m_rbgSize = i + 1;
              break;
            }
        }

      m_noisePsd = NrV2XSpectrumValueHelper::CreateNoisePowerSpectralDensity (m_dlEarfcn, m_dlBandwidth, m_noiseFigure);
      m_downlinkSpectrumPhy->SetNoisePowerSpectralDensity (m_noisePsd);
      m_downlinkSpectrumPhy->GetChannel ()->AddRx (m_downlinkSpectrumPhy);
    }
  m_dlConfigured = true;
}


void 
NrV2XUePhy::DoConfigureUplink (uint16_t ulEarfcn, uint16_t ulBandwidth)
{
  NS_LOG_FUNCTION (this << ulEarfcn << (uint16_t) ulBandwidth);
  m_ulEarfcn = ulEarfcn;
  m_ulBandwidth = ulBandwidth;
  m_ulConfigured = true;

  //configure sidelink with UL
  if (m_sidelinkSpectrumPhy) {
    m_slNoisePsd = NrV2XSpectrumValueHelper::CreateNoisePowerSpectralDensity (m_ulEarfcn, m_ulBandwidth, m_noiseFigure);
    m_sidelinkSpectrumPhy->SetNoisePowerSpectralDensity (m_slNoisePsd);
    m_sidelinkSpectrumPhy->GetChannel ()->AddRx (m_sidelinkSpectrumPhy);
  }
}

void
NrV2XUePhy::DoConfigureReferenceSignalPower (int8_t referenceSignalPower)
{
  NS_LOG_FUNCTION (this);
  m_powerControl->ConfigureReferenceSignalPower (referenceSignalPower);
}
 
void
NrV2XUePhy::DoSetRnti (uint16_t rnti)
{
  NS_LOG_FUNCTION (this << " ID:" << m_uplinkSpectrumPhy->GetDevice()->GetNode()->GetId() << " RNTI: " << rnti);
  m_rnti = rnti;

  m_powerControl->SetCellId (m_cellId);
  m_powerControl->SetRnti (m_rnti);
}
 
void
NrV2XUePhy::DoSetTransmissionMode (uint8_t txMode)
{
  NS_LOG_FUNCTION (this << (uint16_t)txMode);
  m_transmissionMode = txMode;
  m_downlinkSpectrumPhy->SetTransmissionMode (txMode);
}

void
NrV2XUePhy::DoSetSrsConfigurationIndex (uint16_t srcCi)
{
  NS_LOG_FUNCTION (this << srcCi);
  //Useless
}

void
NrV2XUePhy::DoSetPa (double pa)
{
  NS_LOG_FUNCTION (this << pa);
  m_paLinear = pow (10,(pa/10));
}
  
void
NrV2XUePhy::DoSetSlTxPool (Ptr<SidelinkTxDiscResourcePool> pool)
{
  m_discTxPools.m_pool = pool;
  m_discTxPools.m_npsdch = pool->GetNPsdch ();
  m_discTxPools.m_currentGrants.clear ();  
  m_discTxPools.m_nextDiscPeriod.frameNo = 0;
  m_discTxPools.m_nextDiscPeriod.subframeNo = 0;

}


void
NrV2XUePhy::DoRemoveSlTxPool (bool disc)
{
  m_discTxPools.m_pool = NULL;
  m_discTxPools.m_npsdch = 0;
  m_discTxPools.m_currentGrants.clear ();
}


void
NrV2XUePhy::DoSetSlRxPools (std::list<Ptr<SidelinkRxDiscResourcePool> > pools)
{
  
  std::list<Ptr<SidelinkRxDiscResourcePool> >::iterator poolIt;
  for (poolIt = pools.begin (); poolIt != pools.end(); poolIt++)
    {
      bool found = false;
      std::list<DiscPoolInfo >::iterator currentPoolIt;
      for (currentPoolIt = m_discRxPools.begin (); currentPoolIt != m_discRxPools.end() && !found; currentPoolIt++)
        {
          if (*poolIt == currentPoolIt->m_pool)
            {
              found = true;
            }
        }
      if (!found)
        {
          DiscPoolInfo newpool;
          newpool.m_pool = *poolIt;
          newpool.m_npsdch = (*poolIt)->GetNPsdch ();
          newpool.m_currentGrants.clear ();
          m_discRxPools.push_back (newpool);
          //
          m_sidelinkSpectrumPhy->SetRxPool (newpool.m_pool);
          //
        }
    }
}

void
NrV2XUePhy::DoSetDiscGrantInfo (uint8_t resPsdch)
{
  m_discResPsdch = resPsdch;
}

void 
NrV2XUePhy::DoAddDiscTxApps (std::list<uint32_t> apps)
{
  m_discTxApps = apps;
  m_sidelinkSpectrumPhy->AddDiscTxApps (apps);
}

void 
NrV2XUePhy::DoAddDiscRxApps (std::list<uint32_t> apps)
{
  m_discRxApps = apps;
  m_sidelinkSpectrumPhy->AddDiscRxApps (apps);
}

void
NrV2XUePhy::DoSetSlTxPool (Ptr<SidelinkTxCommResourcePool> pool)
{
  m_slTxPoolInfo.m_pool = pool;
  m_slTxPoolInfo.m_npscch = pool->GetNPscch();
  m_slTxPoolInfo.m_currentGrants.clear();
  m_slTxPoolInfo.m_nextScPeriod.frameNo = 0; //init to 0 to make it invalid
  m_slTxPoolInfo.m_nextScPeriod.subframeNo = 0; //init to 0 to make it invalid
}

void
NrV2XUePhy::DoRemoveSlTxPool ()
{
  m_slTxPoolInfo.m_pool = NULL;
  m_slTxPoolInfo.m_npscch = 0;
  m_slTxPoolInfo.m_currentGrants.clear();
}
  
void
NrV2XUePhy::DoSetSlRxPools (std::list<Ptr<SidelinkRxCommResourcePool> > pools)
{ 
  //update the pools that have changed
  std::list<Ptr<SidelinkRxCommResourcePool> >::iterator poolIt;
  for (poolIt = pools.begin (); poolIt != pools.end(); poolIt++)
    {
      bool found = false;
      std::list<PoolInfo >::iterator currentPoolIt;
      for (currentPoolIt = m_sidelinkRxPools.begin (); currentPoolIt != m_sidelinkRxPools.end() && !found; currentPoolIt++)
        {
          if (*poolIt == currentPoolIt->m_pool)
            {
              found = true;
            }
        }
      if (!found)
        {
          PoolInfo newpool;
          newpool.m_pool = *poolIt;
          newpool.m_npscch = (*poolIt)->GetNPscch();
          newpool.m_currentGrants.clear();
          newpool.m_currentV2XGrants.clear();
          m_sidelinkRxPools.push_back (newpool);
        }
    }
  //TODO: should remove the ones no longer needed.
  //Find a clean way to handle updates
  //m_sidelinkRxPools.clear ();
  
}

void
NrV2XUePhy::DoAddSlDestination (uint32_t destination)
{
  std::list <uint32_t>::iterator it;
  for (it = m_destinations.begin (); it != m_destinations.end ();it++) {
    if ((*it) == destination) {
      break;
    }
  }
  if (it == m_destinations.end ()) {
    //did not find it, so insert
    m_destinations.push_back (destination);

    if (m_sidelinkSpectrumPhy) {
      m_sidelinkSpectrumPhy->AddL1GroupId (destination);
    }
  }
}

  
void
NrV2XUePhy::DoRemoveSlDestination (uint32_t destination)
{
  std::list <uint32_t>::iterator it = m_destinations.begin ();
  while (it != m_destinations.end ()) {
    if ((*it) == destination) {
      m_destinations.erase (it);
      if (m_sidelinkSpectrumPhy) {
        m_sidelinkSpectrumPhy->RemoveL1GroupId (destination);
      }
      break;//leave the loop
    }
    it++;
  }
}
  

void 
NrV2XUePhy::SetTxMode1Gain (double gain)
{
  SetTxModeGain (1, gain);
}

void 
NrV2XUePhy::SetTxMode2Gain (double gain)
{
  SetTxModeGain (2, gain);
}

void 
NrV2XUePhy::SetTxMode3Gain (double gain)
{
  SetTxModeGain (3, gain);
}

void 
NrV2XUePhy::SetTxMode4Gain (double gain)
{
  SetTxModeGain (4, gain);
}

void 
NrV2XUePhy::SetTxMode5Gain (double gain)
{
  SetTxModeGain (5, gain);
}

void 
NrV2XUePhy::SetTxMode6Gain (double gain)
{
  SetTxModeGain (6, gain);
}

void 
NrV2XUePhy::SetTxMode7Gain (double gain)
{
  SetTxModeGain (7, gain);
}


void
NrV2XUePhy::SetTxModeGain (uint8_t txMode, double gain)
{
  NS_LOG_FUNCTION (this << gain);
  // convert to linear
  double gainLin = std::pow (10.0, (gain / 10.0));
  if (m_txModeGain.size () < txMode)
    {
      m_txModeGain.resize (txMode);
    }
  std::vector <double> temp;
  temp = m_txModeGain;
  m_txModeGain.clear ();
  for (uint8_t i = 0; i < temp.size (); i++)
    {
      if (i==txMode-1)
        {
          m_txModeGain.push_back (gainLin);
        }
      else
        {
          m_txModeGain.push_back (temp.at (i));
        }
    }
  // forward the info to DL NrV2XSpectrumPhy
  m_downlinkSpectrumPhy->SetTxModeGain (txMode, gain);
}



void
NrV2XUePhy::ReceiveLteDlHarqFeedback (NistDlInfoListElement_s m)
{
  NS_LOG_FUNCTION (this);
  // generate feedback to eNB and send it through ideal PUCCH
  Ptr<NistDlHarqFeedbackLteControlMessage> msg = Create<NistDlHarqFeedbackLteControlMessage> ();
  msg->SetDlHarqFeedback (m);
  SetControlMessages (msg);
}

void
NrV2XUePhy::SetHarqPhyModule (Ptr<NistLteHarqPhy> harq)
{
  m_harqPhyModule = harq;
}


NrV2XUePhy::State
NrV2XUePhy::GetState () const
{
  NS_LOG_FUNCTION (this);
  return m_state;
}


void
NrV2XUePhy::SwitchToState (State newState)
{
  NS_LOG_FUNCTION (this << newState);
  State oldState = m_state;
  m_state = newState;
  NS_LOG_INFO (this << " cellId=" << m_cellId << " rnti=" << m_rnti
                    << " UePhy " << ToString (oldState)
                    << " --> " << ToString (newState));
  m_stateTransitionTrace (m_cellId, m_rnti, oldState, newState);
}

void NrV2XUePhy::SetFirstScanningTime(Time t){
  NS_LOG_FUNCTION (this);
  m_tFirstScanning = t;
  Simulator::Schedule(m_tFirstScanning,&NrV2XUePhy::StartSlssScanning, this);
}

Time NrV2XUePhy::GetFirstScanningTime(){
  NS_LOG_FUNCTION (this);
  return m_tFirstScanning;
}

void NrV2XUePhy::ReceiveSlss(uint16_t slssid, Ptr<SpectrumValue> p)
{
  NS_LOG_FUNCTION(this << slssid);
  //Useless
}

// TODO FIXME NEW for V2X
void 
NrV2XUePhy::MeasurePsschRsrp (Ptr<SpectrumValue> p) // Does not work, useless
{
   NS_LOG_FUNCTION(this);
   //Useless
}

void NrV2XUePhy::SetInitialSubFrameIndication(bool rdm)
{
  NS_LOG_FUNCTION (this << rdm);

  if (rdm)
    {
      NS_LOG_LOGIC (this << " Random initial frame/subframe indication");

      Ptr<UniformRandomVariable> frameRdm = CreateObject<UniformRandomVariable> ();
      Ptr<UniformRandomVariable> subframeRdm = CreateObject<UniformRandomVariable> ();
      frameRdm->SetAttribute("Min", DoubleValue(1));
      frameRdm->SetAttribute("Max", DoubleValue(1024));
      subframeRdm->SetAttribute("Min", DoubleValue(1));
      subframeRdm->SetAttribute("Max", DoubleValue(10));
      Simulator::ScheduleNow(&NrV2XUePhy::SubframeIndication, this,
                             frameRdm->GetInteger(), subframeRdm->GetInteger());
    }
  else
    {
      NS_LOG_LOGIC (this << " Standard initial frame/subframe indication (frameNo=1, subframeNo=1");
      Simulator::ScheduleNow(&NrV2XUePhy::SubframeIndication, this, 1, 1);
    }
}

void NrV2XUePhy::SetUeSlssInterScanningPeriodMax(Time t)
{
  NS_LOG_FUNCTION (this);
  m_nextScanRdm->SetAttribute("Max",DoubleValue(t.GetMilliSeconds()) );
}

void NrV2XUePhy::SetUeSlssInterScanningPeriodMin(Time t)
{
  NS_LOG_FUNCTION (this);
  m_nextScanRdm->SetAttribute("Min",DoubleValue(t.GetMilliSeconds()) );
}

void NrV2XUePhy::StartSlssScanning()
{
  NS_LOG_FUNCTION (this);
  //Useless
}

void NrV2XUePhy::EndSlssScanning()
{
  NS_LOG_FUNCTION (this);
  //Useless
}

void NrV2XUePhy::StartSlssMeasurements(uint64_t slssid, uint16_t offset)
{
  NS_LOG_FUNCTION (this);
}


void NrV2XUePhy::ReportSlssMeasurements(uint64_t slssid, uint16_t offset)
{
  NS_LOG_FUNCTION (this);
  //Useless
}

void NrV2XUePhy::ScheduleNextSyncRefReselection(uint16_t endOfPrevious ){
  NS_LOG_FUNCTION (this);
  //Useless
}

bool
NrV2XUePhy::ChangeOfTiming(uint32_t frameNo, uint32_t subframeNo)
{
  NS_LOG_FUNCTION (this);
  //Useless
  return true;
}

void NrV2XUePhy::DoSetSlssId(uint64_t slssid)
{
  NS_LOG_FUNCTION (this);
  m_uplinkSpectrumPhy->SetSlssid(slssid);
  m_sidelinkSpectrumPhy->SetSlssid(slssid);
}

void NrV2XUePhy::DoSendSlss(NistLteRrcSap::MasterInformationBlockSL mibSl)
{
  NS_LOG_FUNCTION (this);
  //Useless
}

void NrV2XUePhy::DoSynchronizeToSyncRef(NistLteRrcSap::MasterInformationBlockSL mibSl)
{
  NS_LOG_FUNCTION (this);
  //Useless
}

} // namespace ns3
