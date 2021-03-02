/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009, 2011 CTTC
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
 * Author: Nicola Baldo <nbaldo@cttc.es>
 *         Giuseppe Piro  <g.piro@poliba.it>
 *         Marco Miozzo <marco.miozzo@cttc.es> (add physical error model)
 * Modified by: NIST
 * Modified by: Luca Lusvarghi <luca.lusvarghi5@unimore.it>
 */


#include <ns3/object-factory.h>
#include <ns3/log.h>
#include <cmath>
#include <ns3/simulator.h>
#include <ns3/trace-source-accessor.h>
#include <ns3/antenna-model.h>
#include "nist-lte-spectrum-phy.h"
#include "nist-lte-spectrum-signal-parameters.h"
#include "nist-lte-net-device.h"
#include "nist-lte-radio-bearer-tag.h"
#include "nist-lte-chunk-processor.h"
#include "nist-lte-sl-chunk-processor.h"
#include "nist-lte-phy-tag.h"
#include <ns3/nist-lte-mi-error-model.h>
#include <ns3/nist-lte-radio-bearer-tag.h>
#include <ns3/boolean.h>
#include <ns3/double.h>
#include <ns3/config.h>
#include <ns3/node.h>
#include "ns3/enum.h"
#include <ns3/pointer.h>
#include <ns3/object-vector.h>
#include <ns3/node-container.h>
#include <iostream>
#include <fstream>


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("NistLteSpectrumPhy");

// duration of SRS portion of UL subframe  
// = 1 symbol for SRS -1ns as margin to avoid overlapping simulator events
static const Time UL_SRS_DURATION = NanoSeconds (71429 -1);  

// duration of the control portion of a subframe
// = 0.001 / 14 * 3 (ctrl fixed to 3 symbols) -1ns as margin to avoid overlapping simulator events
static const Time DL_CTRL_DURATION = NanoSeconds (214286 -1);

static const double EffectiveCodingRate[29] = {
  0.08,
  0.1,
  0.11,
  0.15,
  0.19,
  0.24,
  0.3,
  0.37,
  0.44,
  0.51,
  0.3,
  0.33,
  0.37,
  0.42,
  0.48,
  0.54,
  0.6,
  0.43,
  0.45,
  0.5,
  0.55,
  0.6,
  0.65,
  0.7,
  0.75,
  0.8,
  0.85,
  0.89,
  0.92
};



  
NistTbId_t::NistTbId_t ()
{
}

NistTbId_t::NistTbId_t (const uint16_t a, const uint8_t b)
: m_rnti (a),
  m_layer (b)
{
}

bool
operator == (const NistTbId_t &a, const NistTbId_t &b)
{
  return ( (a.m_rnti == b.m_rnti) && (a.m_layer == b.m_layer) );
}

bool
operator < (const NistTbId_t& a, const NistTbId_t& b)
{
  return ( (a.m_rnti < b.m_rnti) || ( (a.m_rnti == b.m_rnti) && (a.m_layer < b.m_layer) ) );
}

NistSlTbId_t::NistSlTbId_t ()
{
}

NistSlTbId_t::NistSlTbId_t (const uint16_t a, const uint8_t b)
: m_rnti (a),
  m_l1dst (b)
{
}

bool
operator == (const NistSlTbId_t &a, const NistSlTbId_t &b)
{
  return ( (a.m_rnti == b.m_rnti) && (a.m_l1dst == b.m_l1dst) );
}

bool
operator < (const NistSlTbId_t& a, const NistSlTbId_t& b)
{
  return ( (a.m_rnti < b.m_rnti) || ( (a.m_rnti == b.m_rnti) && (a.m_l1dst < b.m_l1dst) ) );
}


bool
operator == (const NistSlCtrlPacketInfo_t &a, const NistSlCtrlPacketInfo_t &b)
{
  return (a.sinr == b.sinr);
}

bool
operator < (const NistSlCtrlPacketInfo_t& a, const NistSlCtrlPacketInfo_t& b)
{
  return (a.sinr > b.sinr); //we want by decreasing SINR
}

NistDiscTbId_t::NistDiscTbId_t ()
{
}

NistDiscTbId_t::NistDiscTbId_t (const uint16_t a, const uint8_t b)
: m_rnti (a),
  m_resPsdch (b)
{
}

bool
operator == (const NistDiscTbId_t &a, const NistDiscTbId_t &b)
{
  return ( (a.m_rnti == b.m_rnti) && (a.m_resPsdch == b.m_resPsdch) );
}

bool
operator < (const NistDiscTbId_t& a, const NistDiscTbId_t& b)
{
  return ( (a.m_rnti < b.m_rnti) || ( (a.m_rnti == b.m_rnti) && (a.m_resPsdch < b.m_resPsdch) ) );
}

  
NS_OBJECT_ENSURE_REGISTERED (NistLteSpectrumPhy);

NistLteSpectrumPhy::NistLteSpectrumPhy ()
  : m_state (IDLE),
    m_cellId (0),
  m_transmissionMode (0),
  m_layersNum (1),
  m_ulDataSlCheck (false),
  /* Set the initial sensitivity to a very low value, so as to reproduce the default behaviour 
     when it is not set otherwise */
  m_rxSensitivity (-1000),
  m_slssId(0)
{
  NS_LOG_FUNCTION (this);
  m_random = CreateObject<UniformRandomVariable> ();
  m_random->SetAttribute ("Min", DoubleValue (0.0));
  m_random->SetAttribute ("Max", DoubleValue (1.0));
  m_interferenceData = CreateObject<NistLteInterference> ();
  m_interferenceCtrl = CreateObject<NistLteInterference> ();
  m_interferenceSl = CreateObject<NistLteSlInterference> ();
 
  m_prevPrintTime = 0;
  m_totalReceptions = 0;


  for (uint8_t i = 0; i < 7; i++)
    {
      m_txModeGain.push_back (1.0);
    }
}


NistLteSpectrumPhy::~NistLteSpectrumPhy ()
{
  NS_LOG_FUNCTION (this);
  m_expectedTbs.clear ();
  m_expectedSlTbs.clear ();
  m_expectedSlTbSNR.clear();
  m_txModeGain.clear ();
}

void NistLteSpectrumPhy::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_channel = 0;
  m_mobility = 0;
  m_device = 0;
  m_interferenceData->Dispose ();
  m_interferenceData = 0;
  m_interferenceCtrl->Dispose ();
  m_interferenceCtrl = 0;
  m_interferenceSl->Dispose ();
  m_interferenceSl = 0;
  m_ulDataSlCheck = false;
  m_RssiCallback = MakeNullCallback < void, double, uint16_t, uint16_t>();  // TODO FIXME New for V2V

  m_ltePhyTxEndCallback      = MakeNullCallback< void, Ptr<const Packet> > ();
  m_ltePhyRxDataEndErrorCallback = MakeNullCallback< void > ();
  m_ltePhyRxDataEndOkCallback    = MakeNullCallback< void, Ptr<Packet> >  ();
  m_ltePhyRxCtrlEndOkCallback = MakeNullCallback< void, std::list<Ptr<NistLteControlMessage> > > ();
  m_ltePhyRxCtrlEndErrorCallback = MakeNullCallback< void > ();
  m_ltePhyDlHarqFeedbackCallback = MakeNullCallback< void, NistDlInfoListElement_s > ();
  m_ltePhyUlHarqFeedbackCallback = MakeNullCallback< void, NistUlInfoListElement_s > ();
  m_ltePhyRxPssCallback = MakeNullCallback< void, uint16_t, Ptr<SpectrumValue> > ();
  m_ltePhyRxSlssCallback = MakeNullCallback< void, uint16_t, Ptr<SpectrumValue> > ();

  // TODO FIXME NEW for V2X
  m_ltePhyRxDataStartCallback = MakeNullCallback  < void, Ptr<SpectrumValue > > ();

  SpectrumPhy::DoDispose ();
} 

std::ostream& operator<< (std::ostream& os, NistLteSpectrumPhy::State s)
{
  switch (s)
    {
    case NistLteSpectrumPhy::IDLE:
      os << "IDLE";
      break;
    case NistLteSpectrumPhy::RX_DATA:
      os << "RX_DATA";
      break;
    case NistLteSpectrumPhy::RX_CTRL:
      os << "RX_CTRL";
      break;
    case NistLteSpectrumPhy::TX:
      os << "TX";
      break;
    default:
      os << "UNKNOWN";
      break;
    }
  return os;
}

TypeId
NistLteSpectrumPhy::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::NistLteSpectrumPhy")
    .SetParent<SpectrumPhy> ()
    .AddTraceSource ("TxStart",
                     "Trace fired when a new transmission is started",
                     MakeTraceSourceAccessor (&NistLteSpectrumPhy::m_phyTxStartTrace),
                     "ns3::PacketBurst::TracedCallback")
    .AddTraceSource ("TxEnd",
                     "Trace fired when a previosuly started transmission is finished",
                     MakeTraceSourceAccessor (&NistLteSpectrumPhy::m_phyTxEndTrace),
                     "ns3::PacketBurst::TracedCallback")
    .AddTraceSource ("RxStart",
                     "Trace fired when the start of a signal is detected",
                     MakeTraceSourceAccessor (&NistLteSpectrumPhy::m_phyRxStartTrace),
                     "ns3::PacketBurst::TracedCallback")
    .AddTraceSource ("RxEndOk",
                     "Trace fired when a previosuly started RX terminates successfully",
                     MakeTraceSourceAccessor (&NistLteSpectrumPhy::m_phyRxEndOkTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("RxEndError",
                     "Trace fired when a previosuly started RX terminates with an error",
                     MakeTraceSourceAccessor (&NistLteSpectrumPhy::m_phyRxEndErrorTrace),
                     "ns3::Packet::TracedCallback")
    .AddAttribute ("DropRbOnCollisionEnabled",
                   "Activate/Deactivate the dropping colliding RBs regardless SINR value [by default is deactive].",
                    BooleanValue (false),
                    MakeBooleanAccessor (&NistLteSpectrumPhy::m_dropRbOnCollisionEnabled),
                    MakeBooleanChecker ())
    .AddAttribute ("DataErrorModelEnabled",
                    "Activate/Deactivate the error model of data (TBs of PDSCH and PUSCH) [by default is active].",
                    BooleanValue (true),
                   MakeBooleanAccessor (&NistLteSpectrumPhy::m_dataErrorModelEnabled),
                    MakeBooleanChecker ())
    .AddAttribute ("CtrlErrorModelEnabled",
                    "Activate/Deactivate the error model of control (PCFICH-PDCCH decodification) [by default is active].",
                    BooleanValue (true),
                    MakeBooleanAccessor (&NistLteSpectrumPhy::m_ctrlErrorModelEnabled),
                    MakeBooleanChecker ())
    .AddAttribute ("NistErrorModelEnabled",
                   "Activate/Deactivate the NIST based error model [by default is active].",
                   BooleanValue (true),
                   MakeBooleanAccessor (&NistLteSpectrumPhy::m_nistErrorModelEnabled),
                   MakeBooleanChecker ())
    .AddAttribute ("SlDataBLERModelEnabled",
                   "Activate/Deactivate the PSSCH BLER model [by default is active].",
                   BooleanValue (true),
                   MakeBooleanAccessor (&NistLteSpectrumPhy::m_slBlerEnabled),
                   MakeBooleanChecker ())
    .AddAttribute ("FadingModel",
                   "Fading model",
                   EnumValue (NistLtePhyErrorModel::AWGN),
                   MakeEnumAccessor (&NistLteSpectrumPhy::m_fadingModel),
                   MakeEnumChecker (NistLtePhyErrorModel::AWGN, "AWGN")) 
    .AddTraceSource ("DlPhyReception",
                     "DL reception PHY layer statistics.",
                     MakeTraceSourceAccessor (&NistLteSpectrumPhy::m_dlPhyReception),
                     "ns3::NistPhyReceptionStatParameters::TracedCallback")
    .AddTraceSource ("UlPhyReception",
                     "DL reception PHY layer statistics.",
                     MakeTraceSourceAccessor (&NistLteSpectrumPhy::m_ulPhyReception),
                     "ns3::NistPhyReceptionStatParameters::TracedCallback")
    .AddTraceSource ("SlPhyReception",
                     "SL reception PHY layer statistics.",
                     MakeTraceSourceAccessor (&NistLteSpectrumPhy::m_slPhyReception),
                     "ns3::NistPhyReceptionStatParameters::TracedCallback")
    .AddTraceSource ("SlPscchReception",
                     "SL reception PCCH PHY layer statistics.",
                     MakeTraceSourceAccessor (&NistLteSpectrumPhy::m_slPscchReception),
                     "ns3::NistPhyReceptionStatParameters::TracedCallback")
    .AddAttribute ("HalfDuplexPhy",
                   "a pointer to a spectrum phy object",
                   PointerValue (),
                   MakePointerAccessor (&NistLteSpectrumPhy::m_halfDuplexPhy),
                   MakePointerChecker <NistLteSpectrumPhy> ())
    .AddAttribute ("CtrlFullDuplexEnabled",
                    "Activate/Deactivate the full duplex in the PSCCH [by default is disable].",
                    BooleanValue (false),
                    MakeBooleanAccessor (&NistLteSpectrumPhy::m_ctrlFullDuplexEnabled),
                    MakeBooleanChecker ())
    .AddAttribute ("ErrorModelHarqD2dDiscoveryEnabled",
                   "enable the error model and harq for D2D Discovery",
                   BooleanValue (true),
                   MakeBooleanAccessor (&NistLteSpectrumPhy::m_errorModelHarqD2dDiscoveryEnabled),
                   MakeBooleanChecker ())
    .AddAttribute ("SaveCollisionEnabled",  // DEPRECATED, don't use
                   "Enable collision event saving",
                   BooleanValue (false), 
                   MakeBooleanAccessor (&NistLteSpectrumPhy::m_saveCollisions),
                   MakeBooleanChecker ())
    .AddAttribute ("SaveCollisionLossesUnimore", 
                   "Enable collision and propagation lossess saving",
                   BooleanValue (false), 
                   MakeBooleanAccessor (&NistLteSpectrumPhy::m_saveCollisionsUniMore),
                   MakeBooleanChecker ())
/*    .AddAttribute ("UEsTotal",
	           "Total number of UEs",
		   UintegerValue (3),
		   MakeUintegerAccessor (&NistLteSpectrumPhy::m_UEsTotal),
		   MakeUintegerChecker<uint32_t> ())*/
  ;

  return tid;
}



Ptr<NetDevice>
NistLteSpectrumPhy::GetDevice ()
{
  //NS_LOG_FUNCTION (this);
  return m_device;
}


Ptr<MobilityModel>
NistLteSpectrumPhy::GetMobility ()
{
  //NS_LOG_FUNCTION (this);
  return m_mobility;
}


void
NistLteSpectrumPhy::SetDevice (Ptr<NetDevice> d)
{
  NS_LOG_FUNCTION (this << d);
  m_device = d;
}


void
NistLteSpectrumPhy::SetMobility (Ptr<MobilityModel> m)
{
  NS_LOG_FUNCTION (this << m);
  m_mobility = m;
}


void
NistLteSpectrumPhy::SetChannel (Ptr<SpectrumChannel> c)
{
  NS_LOG_FUNCTION (this << c);
  m_channel = c;
}

Ptr<SpectrumChannel> 
NistLteSpectrumPhy::GetChannel ()
{
  return m_channel;
}

Ptr<const SpectrumModel>
NistLteSpectrumPhy::GetRxSpectrumModel () const
{
  return m_rxSpectrumModel;
}


void
NistLteSpectrumPhy::SetTxPowerSpectralDensity (Ptr<SpectrumValue> txPsd)
{
  NS_LOG_FUNCTION (this << txPsd);
  NS_ASSERT (txPsd);
  m_txPsd = txPsd;
}


void
NistLteSpectrumPhy::SetNoisePowerSpectralDensity (Ptr<const SpectrumValue> noisePsd)
{
  NS_LOG_FUNCTION (this << noisePsd);
  NS_ASSERT (noisePsd);
  m_rxSpectrumModel = noisePsd->GetSpectrumModel ();
  m_interferenceData->SetNoisePowerSpectralDensity (noisePsd);
  m_interferenceCtrl->SetNoisePowerSpectralDensity (noisePsd);
  m_interferenceSl->SetNoisePowerSpectralDensity (noisePsd);
}

  
void 
NistLteSpectrumPhy::Reset ()
{
  NS_LOG_FUNCTION (this);
  m_cellId = 0;
  m_slssId = 0;
  m_state = IDLE;
  m_transmissionMode = 0;
  m_layersNum = 1;
  m_endTxEvent.Cancel ();
  m_endRxDataEvent.Cancel ();
  m_endRxDlCtrlEvent.Cancel ();
  m_endRxUlSrsEvent.Cancel ();
  m_rxControlMessageList.clear ();
  m_expectedTbs.clear ();
  m_txControlMessageList.clear ();
  m_rxPacketBurstList.clear ();
  m_txPacketBurst = 0;
  m_rxSpectrumModel = 0;
  m_halfDuplexPhy = 0;
  m_ulDataSlCheck = false;
}


void 
NistLteSpectrumPhy::ClearExpectedSlTb ()
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG (this << " Expected TBs: " << m_expectedSlTbs.size ());
  m_expectedSlTbs.clear ();
  NS_LOG_DEBUG (this << " After clearing Expected TBs size: " << m_expectedSlTbs.size ());
}


void
NistLteSpectrumPhy::SetNistLtePhyTxEndCallback (NistLtePhyTxEndCallback c)
{
  NS_LOG_FUNCTION (this);
  m_ltePhyTxEndCallback = c;
}


void
NistLteSpectrumPhy::SetNistLtePhyRxDataEndErrorCallback (NistLtePhyRxDataEndErrorCallback c)
{
  NS_LOG_FUNCTION (this);
  m_ltePhyRxDataEndErrorCallback = c;
}


void
NistLteSpectrumPhy::SetNistLtePhyRxDataEndOkCallback (NistLtePhyRxDataEndOkCallback c)
{
  NS_LOG_FUNCTION (this);
  m_ltePhyRxDataEndOkCallback = c;
}

void
NistLteSpectrumPhy::SetNistLtePhyRxCtrlEndOkCallback (NistLtePhyRxCtrlEndOkCallback c)
{
  NS_LOG_FUNCTION (this);
  m_ltePhyRxCtrlEndOkCallback = c;
}

// TODO FIXME NEW for V2X

void
NistLteSpectrumPhy::SetUnimoreReportRssiCallback (UnimoreReportRssiCallback c)
{
  NS_LOG_FUNCTION (this);
  m_RssiCallback = c;
}


void
NistLteSpectrumPhy::SetNistLtePhyRxDataStartCallback (NistLtePhyRxDataStartCallback c)
{
  NS_LOG_FUNCTION (this);
  m_ltePhyRxDataStartCallback = c;
}

void
NistLteSpectrumPhy::SetNistLtePhyRxCtrlEndErrorCallback (NistLtePhyRxCtrlEndErrorCallback c)
{
  NS_LOG_FUNCTION (this);
  m_ltePhyRxCtrlEndErrorCallback = c;
}


void
NistLteSpectrumPhy::SetNistLtePhyRxPssCallback (NistLtePhyRxPssCallback c)
{
  NS_LOG_FUNCTION (this);
  m_ltePhyRxPssCallback = c;
}

void
NistLteSpectrumPhy::SetNistLtePhyDlHarqFeedbackCallback (NistLtePhyDlHarqFeedbackCallback c)
{
  NS_LOG_FUNCTION (this);
  m_ltePhyDlHarqFeedbackCallback = c;
}

void
NistLteSpectrumPhy::SetNistLtePhyUlHarqFeedbackCallback (NistLtePhyUlHarqFeedbackCallback c)
{
  NS_LOG_FUNCTION (this);
  m_ltePhyUlHarqFeedbackCallback = c;
}


Ptr<AntennaModel> NistLteSpectrumPhy::GetRxAntenna ()
{
  return m_antenna;
}

void
NistLteSpectrumPhy::SetAntenna (Ptr<AntennaModel> a)
{
  NS_LOG_FUNCTION (this << a);
  m_antenna = a;
}

void
NistLteSpectrumPhy::SetState (State newState)
{
  ChangeState (newState);
}


void
NistLteSpectrumPhy::ChangeState (State newState)
{
  NS_LOG_LOGIC (this << " state: " << m_state << " -> " << newState);
  m_state = newState;
}


void
NistLteSpectrumPhy::SetHarqPhyModule (Ptr<NistLteHarqPhy> harq)
{
  m_harqPhyModule = harq;
}




bool
NistLteSpectrumPhy::StartTxDataFrame (Ptr<PacketBurst> pb, std::list<Ptr<NistLteControlMessage> > ctrlMsgList, Time duration)
{
  NS_LOG_FUNCTION (this << pb);
  NS_LOG_LOGIC (this << " ID:" << GetDevice()->GetNode()->GetId() << " state: " << m_state);
  
  m_phyTxStartTrace (pb);
  
  switch (m_state)
  {
    case RX_DATA:
    case RX_CTRL:
      NS_FATAL_ERROR ("cannot TX while RX: according to FDD channel acces, the physical layer for transmission cannot be used for reception");
      break;
      
    case TX:
      NS_FATAL_ERROR ("cannot TX while already TX: the MAC should avoid this");
      break;
      
    case IDLE:
    {
      /*
      m_txPsd must be set by the device, according to
      (i) the available subchannel for transmission
      (ii) the power transmission
      */
      NS_ASSERT (m_txPsd);
      m_txPacketBurst = pb;
      
      // we need to convey some PHY meta information to the receiver
      // to be used for simulation purposes (e.g., the CellId). This
      // is done by setting the ctrlMsgList parameter of
      // NistLteSpectrumSignalParametersDataFrame
      ChangeState (TX);
      NS_ASSERT (m_channel);
      Ptr<NistLteSpectrumSignalParametersDataFrame> txParams = Create<NistLteSpectrumSignalParametersDataFrame> ();
      txParams->duration = duration;
      txParams->txPhy = GetObject<SpectrumPhy> ();
      txParams->txAntenna = m_antenna;
      txParams->psd = m_txPsd;
      txParams->packetBurst = pb;
      txParams->ctrlMsgList = ctrlMsgList;
      txParams->cellId = m_cellId;
      if (pb) 
      {
        m_ulDataSlCheck = true;
      }
      m_channel->StartTx (txParams);
      m_endTxEvent = Simulator::Schedule (duration, &NistLteSpectrumPhy::EndTx, this);
    }
    return false;
    break;
    
    default:
      NS_FATAL_ERROR ("unknown state");
      return true;
      break;
  }
}

bool
NistLteSpectrumPhy::StartTxSlDataFrame (Ptr<PacketBurst> pb, std::list<Ptr<NistLteControlMessage> > ctrlMsgList, Time duration, uint8_t groupId)
{
  NS_LOG_FUNCTION (this << pb);
  NS_LOG_LOGIC (this << " ID:" << GetDevice()->GetNode()->GetId() << " state: " << m_state);
  
  m_phyTxStartTrace (pb);
  
  switch (m_state)
  {
    case RX_DATA:
    case RX_CTRL:
      NS_FATAL_ERROR ("cannot TX while RX: according to FDD channel acces, the physical layer for transmission cannot be used for reception");
      break;
      
    case TX:
      NS_FATAL_ERROR ("cannot TX while already TX: the MAC should avoid this");
      break;
      
    case IDLE:
    {
      /*
      m_txPsd must be setted by the device, according to
      (i) the available subchannel for transmission
      (ii) the power transmission
      */
      NS_ASSERT (m_txPsd);
      m_txPacketBurst = pb;
      
      // we need to convey some PHY meta information to the receiver
      // to be used for simulation purposes (e.g., the CellId). This
      // is done by setting the ctrlMsgList parameter of
      // NistLteSpectrumSignalParametersDataFrame
      ChangeState (TX);
      NS_ASSERT (m_channel);
      Ptr<NistLteSpectrumSignalParametersSlFrame> txParams = Create<NistLteSpectrumSignalParametersSlFrame> ();
      txParams->duration = duration;
      txParams->txPhy = GetObject<SpectrumPhy> ();
      txParams->txAntenna = m_antenna;
      txParams->psd = m_txPsd;
      txParams->nodeId = GetDevice()->GetNode()->GetId();
      txParams->groupId = groupId;
      txParams->slssId = m_slssId;
      txParams->packetBurst = pb;
      txParams->ctrlMsgList = ctrlMsgList;
      m_ulDataSlCheck = true;

      m_channel->StartTx (txParams);
 
      //trace  
      
      m_endTxEvent = Simulator::Schedule (duration, &NistLteSpectrumPhy::EndTx, this);
    }
    return false;
    break;
    
    default:
      NS_FATAL_ERROR ("unknown state");
      return true;
      break;
  }
}


//TODO FIXME NEW for V2X
bool
NistLteSpectrumPhy::StartTxV2XSlDataFrame (Ptr<PacketBurst> pb, std::list<Ptr<NistLteControlMessage> > ctrlMsgList, Time duration, uint8_t groupId)
{
  NS_LOG_FUNCTION (this << pb);
  NS_LOG_LOGIC (this << " ID:" << GetDevice()->GetNode()->GetId() << " state: " << m_state);

  m_phyTxStartTrace (pb);
  
  switch (m_state)
  {
    case RX_DATA:
    case RX_CTRL:
      NS_FATAL_ERROR ("cannot TX while RX: according to FDD channel acces, the physical layer for transmission cannot be used for reception");
      break;
      
    case TX:
      NS_FATAL_ERROR ("cannot TX while already TX: the MAC should avoid this");
      break;
      
    case IDLE:
    {
      /*
      m_txPsd must be setted by the device, according to
      (i) the available subchannel for transmission
      (ii) the power transmission
      */
      NS_ASSERT (m_txPsd);
      m_txPacketBurst = pb;
      
      // we need to convey some PHY meta information to the receiver
      // to be used for simulation purposes (e.g., the CellId). This
      // is done by setting the ctrlMsgList parameter of
      // NistLteSpectrumSignalParametersDataFrame
      ChangeState (TX);
      NS_ASSERT (m_channel);
      Ptr<NistLteSpectrumSignalParametersV2XSlFrame> txParams = Create<NistLteSpectrumSignalParametersV2XSlFrame> ();
      txParams->duration = duration;
      txParams->txPhy = GetObject<SpectrumPhy> ();
      txParams->txAntenna = m_antenna;
      txParams->psd = m_txPsd;
      txParams->nodeId = GetDevice()->GetNode()->GetId();
      txParams->groupId = groupId;
      txParams->slssId = m_slssId;
      txParams->packetBurst = pb;
      txParams->ctrlMsgList = ctrlMsgList;
      m_ulDataSlCheck = true;

      m_channel->StartTx (txParams);

      //trace  
      
      m_endTxEvent = Simulator::Schedule (duration, &NistLteSpectrumPhy::EndTx, this);
    }
    return false;
    break;
    
    default:
      NS_FATAL_ERROR ("unknown state");
      return true;
      break;
  }

}
  
bool
NistLteSpectrumPhy::StartTxDlCtrlFrame (std::list<Ptr<NistLteControlMessage> > ctrlMsgList, bool pss)
{
  NS_LOG_FUNCTION (this << " PSS " << (uint16_t)pss);
  NS_LOG_LOGIC (this << " ID:" << GetDevice()->GetNode()->GetId() << " state: " << m_state);
  
//   m_phyTxStartTrace (pb);
  
  switch (m_state)
  {
    case RX_DATA:
    case RX_CTRL:
      NS_FATAL_ERROR ("cannot TX while RX: according to FDD channel acces, the physical layer for transmission cannot be used for reception");
      break;
      
    case TX:
      NS_FATAL_ERROR ("cannot TX while already TX: the MAC should avoid this");
      break;
      
    case IDLE:
    {
      /*
      m_txPsd must be setted by the device, according to
      (i) the available subchannel for transmission
      (ii) the power transmission
      */
      NS_ASSERT (m_txPsd);
      
      // we need to convey some PHY meta information to the receiver
      // to be used for simulation purposes (e.g., the CellId). This
      // is done by setting the cellId parameter of
      // NistLteSpectrumSignalParametersDlCtrlFrame
      ChangeState (TX);
      NS_ASSERT (m_channel);

      Ptr<NistLteSpectrumSignalParametersDlCtrlFrame> txParams = Create<NistLteSpectrumSignalParametersDlCtrlFrame> ();
      txParams->duration = DL_CTRL_DURATION;
      txParams->txPhy = GetObject<SpectrumPhy> ();
      txParams->txAntenna = m_antenna;
      txParams->psd = m_txPsd;
      txParams->cellId = m_cellId;
      txParams->pss = pss;
      txParams->ctrlMsgList = ctrlMsgList;
      m_channel->StartTx (txParams);
      m_endTxEvent = Simulator::Schedule (DL_CTRL_DURATION, &NistLteSpectrumPhy::EndTx, this);
    }
    return false;
    break;
    
    default:
      NS_FATAL_ERROR ("unknown state");
      return true;
      break;
  }
}


bool
NistLteSpectrumPhy::StartTxUlSrsFrame ()
{
  NS_LOG_FUNCTION (this);
  NS_LOG_LOGIC (this << " ID:" << GetDevice()->GetNode()->GetId() << " state: " << m_state);

  //   m_phyTxStartTrace (pb);
  
  switch (m_state)
  {
    case RX_DATA:
    case RX_CTRL:
      NS_FATAL_ERROR ("cannot TX while RX: according to FDD channel acces, the physical layer for transmission cannot be used for reception");
      break;
      
    case TX:
      NS_FATAL_ERROR ("cannot TX while already TX: the MAC should avoid this");
      break;
      
    case IDLE:
    {
      /*
      m_txPsd must be setted by the device, according to
      (i) the available subchannel for transmission
      (ii) the power transmission
      */
      NS_ASSERT (m_txPsd);
      NS_LOG_LOGIC (this << " m_txPsd: " << *m_txPsd);
      
      // we need to convey some PHY meta information to the receiver
      // to be used for simulation purposes (e.g., the CellId). This
      // is done by setting the cellId parameter of 
      // NistLteSpectrumSignalParametersDlCtrlFrame
      ChangeState (TX);
      NS_ASSERT (m_channel);
      Ptr<NistLteSpectrumSignalParametersUlSrsFrame> txParams = Create<NistLteSpectrumSignalParametersUlSrsFrame> ();
      txParams->duration = UL_SRS_DURATION;
      txParams->txPhy = GetObject<SpectrumPhy> ();
      txParams->txAntenna = m_antenna;
      txParams->psd = m_txPsd;
      txParams->cellId = m_cellId;
      m_channel->StartTx (txParams);
      m_endTxEvent = Simulator::Schedule (UL_SRS_DURATION, &NistLteSpectrumPhy::EndTx, this);
    }
    return false;
    break;
    
    default:
      NS_FATAL_ERROR ("unknown state");
      return true;
      break;
  }

}



void
NistLteSpectrumPhy::EndTx ()
{
  NS_LOG_FUNCTION (this);
  NS_LOG_LOGIC (this << " ID:" << GetDevice()->GetNode()->GetId() << " state: " << m_state);

  NS_ASSERT (m_state == TX);

  m_phyTxEndTrace (m_txPacketBurst);

  if (!m_ltePhyTxEndCallback.IsNull ())
    {
      for (std::list<Ptr<Packet> >::const_iterator iter = m_txPacketBurst->Begin (); iter
           != m_txPacketBurst->End (); ++iter)
        {
          Ptr<Packet> packet = (*iter)->Copy ();
          m_ltePhyTxEndCallback (packet);
        }
    }

  m_txPacketBurst = 0;
  m_ulDataSlCheck = false;
  ChangeState (IDLE);
}



void
NistLteSpectrumPhy::StartRx (Ptr<SpectrumSignalParameters> spectrumRxParams)
{
  NS_LOG_FUNCTION (this << spectrumRxParams);
  NS_LOG_LOGIC (this << " ID:" << GetDevice()->GetNode()->GetId() << " state: " << m_state);
    
  Ptr <const SpectrumValue> rxPsd = spectrumRxParams->psd;
  Time duration = spectrumRxParams->duration;
  
  // the device might start RX only if the signal is of a type
  // understood by this device - in this case, an LTE signal.
  Ptr<NistLteSpectrumSignalParametersDataFrame> lteDataRxParams = DynamicCast<NistLteSpectrumSignalParametersDataFrame> (spectrumRxParams);
  Ptr<NistLteSpectrumSignalParametersSlFrame> lteSlRxParams = DynamicCast<NistLteSpectrumSignalParametersSlFrame> (spectrumRxParams);

// TODO FIXME NEW for V2X SL

Ptr<NistLteSpectrumSignalParametersV2XSlFrame> lteV2XSlRxParams = DynamicCast<NistLteSpectrumSignalParametersV2XSlFrame> (spectrumRxParams);  
  
  if (lteDataRxParams != 0)
    {
      m_interferenceData->AddSignal (rxPsd, duration);
      m_interferenceSl->AddSignal (rxPsd, duration); //to compute UL/SL interference
      StartRxData (lteDataRxParams);
    }
  else if (lteSlRxParams)
    {
      m_interferenceSl->AddSignal (rxPsd, duration); 
      m_interferenceData->AddSignal (rxPsd, duration); //to compute UL/SL interference
      if( m_ctrlFullDuplexEnabled && lteSlRxParams->ctrlMsgList.size () > 0) 
      {
        StartRxSlData (lteSlRxParams);
      }
      else if (!m_halfDuplexPhy || m_halfDuplexPhy->GetState () == IDLE || !(m_halfDuplexPhy->m_ulDataSlCheck))
      {
        StartRxSlData (lteSlRxParams);
      }
    }
   else if (lteV2XSlRxParams) 
   {  
      m_interferenceSl->AddSignal (rxPsd, duration); 
      m_interferenceData->AddSignal (rxPsd, duration); //to compute UL/SL interference

      // Initialize the entry if it does not exis
      if ( (GetDevice()->GetNode()->GetId() != lteV2XSlRxParams->nodeId) && (m_lostPKTs.find(lteV2XSlRxParams->nodeId) == m_lostPKTs.end()) ) //else this entry already exists
        m_lostPKTs[lteV2XSlRxParams->nodeId] = {0,0,0};    
   
      SetRxSensitivity (-90.4);


      double totalPowerW = 0;
      double nRB = 0;
                for (Values::const_iterator it=lteV2XSlRxParams->psd->ConstValuesBegin (); it != lteV2XSlRxParams->psd->ConstValuesEnd () ; it++)
                  {
                    if (*it != 0)
                      {



                        double powerRxW = (*it) * 180000.0;
                        totalPowerW += powerRxW;
                        nRB++;
                      }
                  }
                 double totalPowerDbm = 10*std::log10(1000*totalPowerW);
                 double rxSensitivitymWPerRB = (std::pow (10, (m_rxSensitivity)/10) )/50;
                 double rxSensitivitydBmPerTB = 10 * std::log10(nRB * rxSensitivitymWPerRB);
                 rxSensitivitydBmPerTB += 0;
               //  std::cout << "nRB: " << nRB << ", full BW Sens: " << m_rxSensitivity <<  " dBm, rxSensitivitymWPerRB: " << rxSensitivitymWPerRB << "mW, nRB * rxSensitivitymWPerRB: " << nRB * rxSensitivitymWPerRB << "mW, std::log(nRB * rxSensitivitymWPerRB): " << std::log(nRB * rxSensitivitymWPerRB) <<  ", rxSensitivitydBmPerTB:" << rxSensitivitydBmPerTB << "\r\n";
          
                Ptr<MobilityModel> mobility = GetDevice () -> GetNode() -> GetObject<MobilityModel>(); 
                double xPosRx = mobility -> GetPosition().x;
                     
                /*std::ofstream totalPowerSpectrumPhy;
                totalPowerSpectrumPhy.open ("results/sidelink/receivedPower.csv", std::ios_base::app);
                totalPowerSpectrumPhy << xPosRx << "," << totalPowerDbm << "\r\n";
                totalPowerSpectrumPhy.close ();*/

               

      if( (m_ctrlFullDuplexEnabled && lteV2XSlRxParams->ctrlMsgList.size () > 0) && (totalPowerDbm > rxSensitivitydBmPerTB))
      {
        StartRxV2XSlData (lteV2XSlRxParams);
        NS_LOG_INFO("Total power in dBm " << totalPowerDbm);
      }
      else if ((!m_halfDuplexPhy || m_halfDuplexPhy->GetState () == IDLE || !(m_halfDuplexPhy->m_ulDataSlCheck)) && (totalPowerDbm > rxSensitivitydBmPerTB))
      {
        StartRxV2XSlData (lteV2XSlRxParams);
        NS_LOG_INFO ("Total power in dBm " << totalPowerDbm);
      }
      if (totalPowerDbm < rxSensitivitydBmPerTB)
       {
          NS_LOG_INFO ("Signal level below sensitivity");
     //     NS_LOG_UNCOND(GetDevice()->GetNode()->GetId() << " receiving message from " <<lteV2XSlRxParams->nodeId);
     //     std::cin.get();
          ++m_totalReceptions;
          ++m_lostPKTs[lteV2XSlRxParams->nodeId].propagationLosses;
       }
   }
  else
    {
      Ptr<NistLteSpectrumSignalParametersDlCtrlFrame> lteDlCtrlRxParams = DynamicCast<NistLteSpectrumSignalParametersDlCtrlFrame> (spectrumRxParams);
      Ptr<NistLteSpectrumSignalParametersUlSrsFrame> lteUlSrsRxParams = DynamicCast<NistLteSpectrumSignalParametersUlSrsFrame> (spectrumRxParams);
      if ((lteDlCtrlRxParams!=0)||(lteUlSrsRxParams!=0))
        {
          m_interferenceCtrl->AddSignal (rxPsd, duration);
          StartRxCtrl (spectrumRxParams);
        }
      else
        {
          // other type of signal (could be 3G, GSM, whatever) -> interference
          m_interferenceData->AddSignal (rxPsd, duration);
          m_interferenceCtrl->AddSignal (rxPsd, duration);
          m_interferenceSl->AddSignal (rxPsd, duration); 
        }
    }

}

void
NistLteSpectrumPhy::StartRxData (Ptr<NistLteSpectrumSignalParametersDataFrame> params)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_LOGIC (this << " ID:" << GetDevice()->GetNode()->GetId() << " state: " << m_state);
  
  switch (m_state)
    {
      case TX:
        NS_FATAL_ERROR ("cannot RX while TX: according to FDD channel access, the physical layer for transmission cannot be used for reception");
        break;
      case RX_CTRL:
        NS_FATAL_ERROR ("cannot RX Data while receiving control (cellId=" << params->cellId << ")");
        break;
      case IDLE:
      case RX_DATA:
        // the behavior is similar when
        // we're IDLE or RX because we can receive more signals
        // simultaneously (e.g., at the eNB).
        {
          // To check if we're synchronized to this signal, we check
          // for the CellId which is reported in the
          //  NistLteSpectrumSignalParametersDataFrame
          if (params->cellId  == m_cellId)
            {
              NS_LOG_LOGIC (this << " synchronized with this signal (cellId=" << params->cellId << ")");
              if ((m_rxPacketBurstList.empty ())&&(m_rxControlMessageList.empty ()))
                {
                  NS_ASSERT (m_state == IDLE);
                  // first transmission, i.e., we're IDLE and we
                  // start RX
                  m_firstRxStart = Simulator::Now ();
                  m_firstRxDuration = params->duration;
                  NS_LOG_LOGIC (this << " scheduling EndRx with delay " << params->duration.GetSeconds () << "s");
                  m_endRxDataEvent = Simulator::Schedule (params->duration, &NistLteSpectrumPhy::EndRxData, this);
                }
              else
                {
                  NS_ASSERT (m_state == RX_DATA);
                  // sanity check: if there are multiple RX events, they
                  // should occur at the same time and have the same
                  // duration, otherwise the interference calculation
                  // won't be correct
                  NS_ASSERT ((m_firstRxStart == Simulator::Now ()) 
                  && (m_firstRxDuration == params->duration));
                }
              
              ChangeState (RX_DATA);
              if (params->packetBurst)
                {
                  m_rxPacketBurstList.push_back (params->packetBurst);
                  m_interferenceData->StartRx (params->psd);
                  
                  m_phyRxStartTrace (params->packetBurst);
                }
                NS_LOG_DEBUG (this << " insert msgs " << params->ctrlMsgList.size ());
              m_rxControlMessageList.insert (m_rxControlMessageList.end (), params->ctrlMsgList.begin (), params->ctrlMsgList.end ());
              
              NS_LOG_LOGIC (this << " numSimultaneousRxEvents = " << m_rxPacketBurstList.size ());
            }
          else
            {
              NS_LOG_LOGIC (this << " not in sync with this signal (cellId=" 
              << params->cellId  << ", m_cellId=" << m_cellId << ")");
            }
        }
        break;
        
        default:
          NS_FATAL_ERROR ("unknown state");
          break;
      }
      
   NS_LOG_LOGIC (this << " state: " << m_state);
}

void
NistLteSpectrumPhy::StartRxSlData (Ptr<NistLteSpectrumSignalParametersSlFrame> params)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_LOGIC (this << " ID:" << GetDevice()->GetNode()->GetId() << " state: " << m_state);

  switch (m_state)
  {
    case TX:
      NS_FATAL_ERROR ("cannot RX while TX: according to FDD channel access, the physical layer for transmission cannot be used for reception");
      break;
    case RX_CTRL:
      NS_FATAL_ERROR ("cannot RX Data while receiving control");
      break;
    case IDLE:
    case RX_DATA:
      // the behavior is similar when
      // we're IDLE or RX because we can receive more signals
      // simultaneously (e.g., at the eNB).
      {
        
        // check it is not an eNB and not the same sending node (sidelink : discovery & communication )
        if (m_cellId == 0 && params->nodeId != GetDevice()->GetNode()->GetId())
          {
            NS_LOG_LOGIC (this << " the signal is neither from eNodeB nor from this UE ");

            //SLSSs (PSBCH) should be received by all UEs
            //Checking if it is a SLSS, and if it is: measure S-RSRP and receive MIB-SL
            if (params->ctrlMsgList.size () >0)
              {
                std::list<Ptr<NistLteControlMessage> >::iterator ctrlIt;
                for (ctrlIt=params->ctrlMsgList.begin() ; ctrlIt != params->ctrlMsgList.end(); ctrlIt++)
                  {
                    //Detection of a SLSS and callback for measurement of S-RSRP
                    if( (*ctrlIt)->GetMessageType () == NistLteControlMessage::MIB_SL)
                      {
                        NS_LOG_LOGIC (this << " receiving a SLSS");
                        Ptr<MibSLNistLteControlMessage> msg = DynamicCast<MibSLNistLteControlMessage> (*ctrlIt);
                        NistLteRrcSap::MasterInformationBlockSL mibSL = msg->GetMibSL ();
                        //Measure S-RSRP
                        if (!m_ltePhyRxSlssCallback.IsNull ())
                          {
                            m_ltePhyRxSlssCallback (mibSL.slssid, params->psd);
                          }
                        //Receive MIB-SL
                        if (m_rxPacketInfo.empty ())
                          {
                            NS_ASSERT (m_state == IDLE);
                            // first transmission, i.e., we're IDLE and we start RX
                            m_firstRxStart = Simulator::Now ();
                            m_firstRxDuration = params->duration;
                            NS_LOG_LOGIC (this << " scheduling EndRxSl with delay " << params->duration.GetSeconds () << "s");
                              
                            m_endRxDataEvent = Simulator::Schedule (params->duration, &NistLteSpectrumPhy::EndRxSlData, this);
                          }
                        else
                          {
                            NS_ASSERT (m_state == RX_DATA);
                            // sanity check: if there are multiple RX events, they
                            // should occur at the same time and have the same
                            // duration, otherwise the interference calculation
                            // won't be correct
                            NS_ASSERT ((m_firstRxStart == Simulator::Now ())
                                       && (m_firstRxDuration == params->duration));
                          }
                        ChangeState (RX_DATA);
                        m_interferenceSl->StartRx (params->psd);
                        NistSlRxPacketInfo_t packetInfo;
                        packetInfo.m_rxPacketBurst = params->packetBurst;
                        packetInfo.m_rxControlMessage = *ctrlIt;
                        //convert the PSD to RB map so we know which RBs were used to transmit the control message
                        //will be used later to compute error rate
                        std::vector <int> rbMap;
                        int i = 0;
                        for (Values::const_iterator it=params->psd->ConstValuesBegin (); it != params->psd->ConstValuesEnd () ; it++, i++)
                          {
                            if (*it != 0)
                              {
                                NS_LOG_INFO (this << " SL MIB-SL arriving on RB " << i);
                                rbMap.push_back (i);
                              }
                          }
                        packetInfo.rbBitmap = rbMap;
                        m_rxPacketInfo.push_back (packetInfo);
                        params->ctrlMsgList.erase(ctrlIt);
                        break;
                      }
                  }
              }


            //Receive PSCCH, PSSCH and PSDCH only if synchronized to the transmitter (having the same SLSSID)
            //and belonging to the destination group
            if (params->slssId == m_slssId && (params->groupId == 0 || m_l1GroupIds.find (params->groupId) != m_l1GroupIds.end()))
              { 

                if (m_rxPacketInfo.empty ())
                  {
                    NS_ASSERT (m_state == IDLE);
                    // first transmission, i.e., we're IDLE and we start RX
                    m_firstRxStart = Simulator::Now ();
                    m_firstRxDuration = params->duration;
                    NS_LOG_INFO(this << " scheduling EndRxSl with delay " << params->duration.GetSeconds () << "s");
                    m_endRxDataEvent = Simulator::Schedule (params->duration, &NistLteSpectrumPhy::EndRxSlData, this);
                  }
                else
                  {
                    NS_ASSERT (m_state == RX_DATA);
                    // sanity check: if there are multiple RX events, they
                    // should occur at the same time and have the same
                    // duration, otherwise the interference calculation
                    // won't be correct
                    NS_ASSERT ((m_firstRxStart == Simulator::Now ())
                               && (m_firstRxDuration == params->duration));
                  }
                ChangeState (RX_DATA);
                m_interferenceSl->StartRx (params->psd);

                NistSlRxPacketInfo_t packetInfo;
                packetInfo.m_rxPacketBurst = params->packetBurst; 
                NS_LOG_INFO("Ctrlmsg size: " << params->ctrlMsgList.size ());
                if (params->ctrlMsgList.size () >0)
                  {

                    NS_ASSERT (params->ctrlMsgList.size () == 1); //this condition must be true
                    packetInfo.m_rxControlMessage = *(params->ctrlMsgList.begin());
                  }
                //convert the PSD to RB map so we know which RBs were used to transmit the control message
                //will be used later to compute error rate
                std::vector <int> rbMap; //for PSCCH? also PSSCH!!!
                int i = 0;
                for (Values::const_iterator it=params->psd->ConstValuesBegin (); it != params->psd->ConstValuesEnd () ; it++, i++)
                  {
                    if (*it != 0)
                      {
                        NS_LOG_INFO (this << "Now: "<<Simulator::Now()<<" SL Control Message arriving on RB " << i);
                        rbMap.push_back (i);
                      }
                  }
                packetInfo.rbBitmap = rbMap;
                m_rxPacketInfo.push_back (packetInfo);

                if (params->packetBurst)
                  {
                    m_phyRxStartTrace (params->packetBurst);
                    NS_LOG_INFO (this << " RX Burst containing " << params->packetBurst->GetNPackets() << " packets");
                  }
                NS_LOG_DEBUG (this << " insert sidelink ctrl msgs " << params->ctrlMsgList.size ());

                NS_LOG_LOGIC (this << " numSimultaneousRxEvents = " << m_rxPacketInfo.size ());
              }

            else
              {
                NS_LOG_LOGIC (this << " not in sync with this sidelink signal... Ignoring ");
              }
          }
        else
        {
        	NS_LOG_LOGIC (this << " the signal is from eNodeB or from this UE... Ignoring");
        }
      }
      break;

    default:
      NS_FATAL_ERROR ("unknown state");
      break;
  }

  NS_LOG_LOGIC (this << " state: " << m_state);
}

//TODO FIXME NEW for V2X
void
NistLteSpectrumPhy::StartRxV2XSlData (Ptr<NistLteSpectrumSignalParametersV2XSlFrame> params)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_LOGIC(this << " ID:" << GetDevice()->GetNode()->GetId() << " state: " << m_state << "Starting to receive SL V2X Data");

  switch (m_state)
  {
    case TX:
      NS_FATAL_ERROR ("cannot RX while TX: according to FDD channel access, the physical layer for transmission cannot be used for reception");
      break;
    case RX_CTRL:
      NS_FATAL_ERROR ("cannot RX Data while receiving control");
      break;
    case IDLE:
    case RX_DATA:
      // the behavior is similar when
      // we're IDLE or RX because we can receive more signals
      // simultaneously (e.g., at the eNB).
    {
      // check it is not an eNB and not the same sending node (sidelink : discovery & communication )
      if (m_cellId == 0 && params->nodeId != GetDevice()->GetNode()->GetId())
      {
         NS_LOG_DEBUG (this << " Signal from UE " << params->nodeId << ", the signal is neither from eNodeB nor from this UE ");
         //SLSSs (PSBCH) should be received by all UEs
         //Checking if it is a SLSS, and if it is: measure S-RSRP and receive MIB-SL
         if (params->ctrlMsgList.size () >0)
         {
            std::list<Ptr<NistLteControlMessage> >::iterator ctrlIt;
            for (ctrlIt=params->ctrlMsgList.begin() ; ctrlIt != params->ctrlMsgList.end(); ctrlIt++)
            {
                //Detection of a SLSS and callback for measurement of S-RSRP
                if( (*ctrlIt)->GetMessageType () == NistLteControlMessage::MIB_SL)
                {
                    NS_LOG_LOGIC (this << " receiving a SLSS");
                    Ptr<MibSLNistLteControlMessage> msg = DynamicCast<MibSLNistLteControlMessage> (*ctrlIt);
                    NistLteRrcSap::MasterInformationBlockSL mibSL = msg->GetMibSL ();
                    //Measure S-RSRP
                    if (!m_ltePhyRxSlssCallback.IsNull ())
                    {
                      m_ltePhyRxSlssCallback (mibSL.slssid, params->psd); // ReceiveSlss
                    }
                    //Receive MIB-SL
                    if (m_rxPacketInfo.empty ())
                    {
                       NS_ASSERT (m_state == IDLE);
                       // first transmission, i.e., we're IDLE and we start RX
                       m_firstRxStart = Simulator::Now ();
                       m_firstRxDuration = params->duration;
                       NS_LOG_LOGIC (this << " scheduling EndRxSl with delay " << params->duration.GetSeconds () << "s");
                       m_endRxDataEvent = Simulator::Schedule (params->duration, &NistLteSpectrumPhy::EndRxV2XSlData, this);
                    }
                    else
                    {
                       NS_ASSERT (m_state == RX_DATA);
                       // sanity check: if there are multiple RX events, they
                       // should occur at the same time and have the same
                       // duration, otherwise the interference calculation
                       // won't be correct
                       NS_ASSERT ((m_firstRxStart == Simulator::Now ())
                               && (m_firstRxDuration == params->duration));
                    }
                    ChangeState (RX_DATA);
                    m_interferenceSl->StartRx (params->psd);
                    NistSlRxPacketInfo_t packetInfo;
                    packetInfo.m_rxPacketBurst = params->packetBurst;
                    packetInfo.m_rxControlMessage = *ctrlIt;
                    //convert the PSD to RB map so we know which RBs were used to transmit the control message
                    //will be used later to compute error rate
                    std::vector <int> rbMap;
                    int i = 0;
                    for (Values::const_iterator it=params->psd->ConstValuesBegin (); it != params->psd->ConstValuesEnd () ; it++, i++)
                    {
                        if (*it != 0)
                        {
                           NS_LOG_INFO (this << " SL MIB-SL arriving on RB " << i);
                           rbMap.push_back (i);
                        }
                    }
                    packetInfo.rbBitmap = rbMap;
                    m_rxPacketInfo.push_back (packetInfo);
                    params->ctrlMsgList.erase(ctrlIt);
                    break;
                } // end if( (*ctrlIt)->GetMessageType () == NistLteControlMessage::MIB_SL)
            } // end for (ctrlIt=params->ctrlMsgList.begin() ; ctrlIt != params->ctrlMsgList.end(); ctrlIt++)
         }
 
         //Receive PSCCH, PSSCH and PSDCH only if synchronized to the transmitter (having the same SLSSID)
         //and belonging to the destination group
         if (params->slssId == m_slssId && (params->groupId == 0 || m_l1GroupIds.find (params->groupId) != m_l1GroupIds.end()))
         { 
             if (m_rxPacketInfo.empty ())
             {
                 NS_ASSERT (m_state == IDLE);
                 // first transmission, i.e., we're IDLE and we start RX
                 m_firstRxStart = Simulator::Now ();
                 m_firstRxDuration = params->duration;
                 NS_LOG_LOGIC(this << " scheduling EndRxSl with delay " << params->duration.GetSeconds () << "s");
                 m_endRxDataEvent = Simulator::Schedule (params->duration, &NistLteSpectrumPhy::EndRxV2XSlData, this);  
             }
             else
             {
                NS_ASSERT (m_state == RX_DATA);
                // sanity check: if there are multiple RX events, they
                // should occur at the same time and have the same
                // duration, otherwise the interference calculation
                // won't be correct
                NS_ASSERT ((m_firstRxStart == Simulator::Now ())
                            && (m_firstRxDuration == params->duration));
             }
             ChangeState (RX_DATA);
             m_interferenceSl->StartRx (params->psd);
             NistSlRxPacketInfo_t packetInfo;
             packetInfo.m_rxPacketBurst = params->packetBurst; 
             NS_LOG_INFO("Ctrlmsg size: " << params->ctrlMsgList.size ());
             NistV2XSciListElement_s sci;
             //convert the PSD to RB map so we know which RBs were used to transmit the control message
             //will be used later to compute error rate
             std::vector <int> rbMap; // including both PSSCH and PSCCH
             std::map <int, double> psdMap;
             int i = 0;
             double rsrpDb = 0, AVGrsrpDb = 0, rssi = 0;
             double totalPowerW = 0;
             int rbLen = 0;
             for (Values::const_iterator it=params->psd->ConstValuesBegin (); it != params->psd->ConstValuesEnd () ; it++, i++)
             {
                if (*it != 0)
                {
                    ++rbLen;

                    rbMap.push_back (i);
                    psdMap.insert (std::pair<int,double> (i, (*it)));
                    double powerRxW = ((*it) * 180000.0) / 12.0; // convert PSD [W/Hz] to linear power [W]
                    rsrpDb = 10*std::log10(1000*powerRxW); // then, we will use the last one, since they are all equal
                                                           // in principle, we should take the average value over the RBs
                    rssi += powerRxW;
 	            AVGrsrpDb += rsrpDb;
                    NS_LOG_INFO (this << "Now: "<< Simulator::Now() << " V2X SL  Message arriving on RB " << i << ", Power is: " << (*it) << ", RSRP is " << rsrpDb);                    	
                    powerRxW = (*it) * 180000.0;
                    totalPowerW += powerRxW;
                 }
               
             }
             rssi *= 12;
             rssi = 10*std::log10(1000*rssi); // in dBm
             AVGrsrpDb /= rbLen;
             double totalPowerDbm = 10*std::log10(1000*totalPowerW);
             uint16_t CSRindex, iterations;
             iterations = rbLen/12;
             NS_LOG_DEBUG("UE ID " << GetDevice()->GetNode()->GetId());
             for (uint16_t j = 0; j < iterations; ++j)
             {
                 CSRindex = rbMap.front()/12 + j;
                 NS_LOG_DEBUG("Callback! CSR index " << CSRindex);
                 if (!m_RssiCallback.IsNull ())
			m_RssiCallback (rssi, CSRindex, GetDevice()->GetNode()->GetId());

       
             }
             NS_LOG_DEBUG("The total power in Dbm is " << totalPowerDbm << ", RB start " << rbMap.front() << ", number of RBs " << rbLen << ", Average RSRP " << AVGrsrpDb << ", RSSI " << rssi);
             // Fill the vector with the SNR values (without interference)
             m_expectedSlTbSNR.push_back(std::pair<uint32_t,double> (params->nodeId,std::pow(10,(AVGrsrpDb-(-123.2391))/10) )); //123.3291 is the noise power level (checked)
             //std::cin.get();
                       
             if (! m_ltePhyRxDataStartCallback.IsNull ())
             {
                     // m_ltePhyRxDataStartCallback (params -> psd); //to store PSSCH-RSRP values
             }
             if (params->ctrlMsgList.size () >0)
             {
                NS_ASSERT (params->ctrlMsgList.size () == 1); //this condition must be true
                packetInfo.m_rxControlMessage = *(params->ctrlMsgList.begin());
                NS_ASSERT (packetInfo.m_rxControlMessage -> GetMessageType () == NistLteControlMessage::SCI_V2X);
                //TODO FIXME : Add here the expected Tb
                Ptr<SciV2XLteControlMessage> msg2 = DynamicCast<SciV2XLteControlMessage> (packetInfo.m_rxControlMessage);
                sci = msg2->GetSci ();
                std::vector <int> rbMapPssch;
                for (int i = sci.m_rbStart; i < sci.m_rbStart + sci.m_rbLen_TB; i++)
                {
                   rbMapPssch.push_back (i);
                }
                AddExpectedTb (sci.m_rnti, sci.m_groupDstId, !((int) sci.m_reTxIndex == 1), sci.m_tbSize, sci.m_mcs, rbMapPssch, 1);
                NS_LOG_INFO("Added expected Tb from NistLteSpectrumPhy");
             }
             packetInfo.rbBitmap = rbMap;
             sci.m_psschRsrpDb = rsrpDb;
             Ptr<SciV2XLteControlMessage> msg = Create<SciV2XLteControlMessage> (); //reconvert to Control Message
	     msg->SetSci (sci);
             packetInfo.m_rxControlMessage = msg;
             m_rxPacketInfo.push_back (packetInfo);
             if (params->packetBurst)
             {
                 m_phyRxStartTrace (params->packetBurst);
                 NS_LOG_INFO (this << " RX Burst containing " << params->packetBurst->GetNPackets() << " packets");
             }
             NS_LOG_DEBUG (this << " insert sidelink ctrl msgs " << params->ctrlMsgList.size ());
             NS_LOG_LOGIC (this << " numSimultaneousRxEvents = " << m_rxPacketInfo.size ());
         }
         else
         {
               NS_LOG_LOGIC (this << " not in sync with this sidelink signal... Ignoring ");
         }
      } //end if (m_cellId == 0 && params->nodeId != GetDevice()->GetNode()->GetId())
      else
      {
               NS_LOG_LOGIC (this << " the signal is from eNodeB or from this UE... Ignoring");
      }
     } //end case RX_DATA
     break;
  
     default:
        NS_FATAL_ERROR ("unknown state");
        break;
  } //end switch
 
//  std::cin.get();
  NS_LOG_LOGIC (this << " state: " << m_state);
}

void
NistLteSpectrumPhy::StartRxCtrl (Ptr<SpectrumSignalParameters> params)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_LOGIC (this << " ID:" << GetDevice()->GetNode()->GetId() << " state: " << m_state);
  
  switch (m_state)
  {
    case TX:
      NS_FATAL_ERROR ("cannot RX while TX: according to FDD channel access, the physical layer for transmission cannot be used for reception");
      break;
    case RX_DATA:
      NS_FATAL_ERROR ("cannot RX data while receing control");
      break;
    case IDLE:
    case RX_CTRL:
      // the behavior is similar when
      // we're IDLE or RX because we can receive more signals
      // simultaneously (e.g., at the eNB).
      {
        // To check if we're synchronized to this signal, we check
        // for the CellId which is reported in the
        // NistLteSpectrumSignalParametersDlCtrlFrame
        uint16_t cellId;
        bool dl;
        Ptr<NistLteSpectrumSignalParametersDlCtrlFrame> lteDlCtrlRxParams = DynamicCast<NistLteSpectrumSignalParametersDlCtrlFrame> (params);
        if (lteDlCtrlRxParams!=0)
          {
            cellId = lteDlCtrlRxParams->cellId;
            dl = true;
          }
        else
          {
            Ptr<NistLteSpectrumSignalParametersUlSrsFrame> lteUlSrsRxParams = DynamicCast<NistLteSpectrumSignalParametersUlSrsFrame> (params);
            cellId = lteUlSrsRxParams->cellId;
            dl = false;
          }
        if (dl)
            {
              // check presence of PSS for UE measuerements
              if (lteDlCtrlRxParams->pss == true)
                {
                  SpectrumValue pssPsd = *params->psd;
                  if (!m_ltePhyRxPssCallback.IsNull ())
                    {
                      m_ltePhyRxPssCallback (cellId, params->psd);
                    }
                }
            }
        if (cellId  == m_cellId)
        {
          NS_LOG_LOGIC (this << " synchronized with this signal (cellId=" << cellId << ")");
          if (m_state == IDLE)
          {
            // first transmission, i.e., we're IDLE and we
            // start RX
            NS_ASSERT (m_rxControlMessageList.empty ());
            m_firstRxStart = Simulator::Now ();
            m_firstRxDuration = params->duration;
            NS_LOG_LOGIC (this << " scheduling EndRx with delay " << params->duration);
            if (dl==true)
              {
                // store the DCIs
                m_rxControlMessageList = lteDlCtrlRxParams->ctrlMsgList;
                m_endRxDlCtrlEvent = Simulator::Schedule (params->duration, &NistLteSpectrumPhy::EndRxDlCtrl, this);
              }
            else
              {
                m_endRxUlSrsEvent = Simulator::Schedule (params->duration, &NistLteSpectrumPhy::EndRxUlSrs, this);
              }
          }
          else if (m_state == RX_CTRL)
          {
            // sanity check: if there are multiple RX events, they
            // should occur at the same time and have the same
            // duration, otherwise the interference calculation
            // won't be correct
            NS_ASSERT ((m_firstRxStart == Simulator::Now ()) 
            && (m_firstRxDuration == params->duration));
          }
          
          ChangeState (RX_CTRL);
          m_interferenceCtrl->StartRx (params->psd);
          
//           NS_LOG_LOGIC (this << " numSimultaneousRxEvents = " << m_rxPacketBurstList.size ());
        }
        else
        {
          NS_LOG_LOGIC (this << " not in sync with this signal (cellId=" 
          << cellId  << ", m_cellId=" << m_cellId << ")");          
        }
      }
      break;
      
      default:
        NS_FATAL_ERROR ("unknown state");
        break;
  }
  
  NS_LOG_LOGIC (this << " state: " << m_state);
}

void
NistLteSpectrumPhy::UpdateSinrPerceived (const SpectrumValue& sinr)
{
  NS_LOG_FUNCTION (this << sinr);
  m_sinrPerceived = sinr;
}

void
NistLteSpectrumPhy::UpdateSlSinrPerceived (std::vector <SpectrumValue> sinr)
{
  NS_LOG_FUNCTION (this);
  m_slSinrPerceived = sinr;
}

void
NistLteSpectrumPhy::UpdateSlSigPerceived (std::vector <SpectrumValue> signal)
{
  NS_LOG_FUNCTION (this);
  m_slSignalPerceived = signal;
}

void
NistLteSpectrumPhy::UpdateSlIntPerceived (std::vector <SpectrumValue> interference)
{
  NS_LOG_FUNCTION (this);
  m_slInterferencePerceived = interference;
  //std::cout << "UpdateSlIntPerceived \r\n";
}

void
NistLteSpectrumPhy::AddExpectedTb (uint16_t  rnti, uint8_t ndi, uint16_t size, uint8_t mcs, std::vector<int> map, uint8_t layer, uint8_t harqId,uint8_t rv,  bool downlink)
{
  NS_LOG_FUNCTION (this << " rnti: " << rnti << " NDI " << (uint16_t)ndi << " size " << size << " mcs " << (uint16_t)mcs << " layer " << (uint16_t)layer << " rv " << (uint16_t)rv);
  NistTbId_t tbId;
  tbId.m_rnti = rnti;
  tbId.m_layer = layer;
  expectedTbs_t::iterator it;
  it = m_expectedTbs.find (tbId);
  if (it != m_expectedTbs.end ())
    {
      // migth be a TB of an unreceived packet (due to high progpalosses)
      m_expectedTbs.erase (it);
    }
  // insert new entry
  NisttbInfo_t tbInfo = {ndi, size, mcs, map, harqId, rv, 0.0, downlink, false, false};
  m_expectedTbs.insert (std::pair<NistTbId_t, NisttbInfo_t> (tbId,tbInfo));
}

void
NistLteSpectrumPhy::AddExpectedTb (uint16_t  rnti, uint8_t l1dst, uint8_t ndi, uint16_t size, uint8_t mcs, std::vector<int> map, uint8_t rv)
{
  NS_LOG_INFO (this << " rnti: " << rnti << " group " << (uint16_t) l1dst << " NDI " << (uint16_t)ndi << " size " << size << " mcs " << (uint16_t)mcs << " rv " << (uint16_t)rv);
  NistSlTbId_t tbId;
  tbId.m_rnti = rnti;
  tbId.m_l1dst = l1dst;
  expectedSlTbs_t::iterator it;
  it = m_expectedSlTbs.find (tbId);
  if (it != m_expectedSlTbs.end ())
    {
      // migth be a TB of an unreceived packet (due to high progpalosses)
      m_expectedSlTbs.erase (it);
    }
  // insert new entry
  NistSltbInfo_t tbInfo = {ndi, size, mcs, map, rv, 0.0, false, false}; //'map' is the RB map

  
  m_expectedSlTbs.insert (std::pair<NistSlTbId_t, NistSltbInfo_t> (tbId,tbInfo));

  // if it is for new data, reset the HARQ process
  if (ndi)
    {
      m_harqPhyModule->ResetSlHarqProcessNistStatus (rnti, l1dst);
    }
}

void
NistLteSpectrumPhy::AddExpectedTb (uint16_t  rnti, uint8_t resPsdch, uint8_t ndi, std::vector<int> map, uint8_t rv)
{
  NS_LOG_FUNCTION (this << " rnti: " << rnti << " resPsdch " << resPsdch << " NDI " << (uint16_t)ndi << " rv " << (uint16_t)rv);
  NistDiscTbId_t tbId;
  tbId.m_rnti = rnti;
  tbId.m_resPsdch = resPsdch;
  expectedDiscTbs_t::iterator it;
  it = m_expectedDiscTbs.find (tbId);
  if (it != m_expectedDiscTbs.end ())
    {
      // migth be a TB of an unreceived packet (due to high progpalosses)
      m_expectedDiscTbs.erase (it);
    }
  // insert new entry
  NistDisctbInfo_t tbInfo = {ndi, resPsdch, map, rv, 0.0, false, false};

  m_expectedDiscTbs.insert (std::pair<NistDiscTbId_t, NistDisctbInfo_t> (tbId,tbInfo));

  // if it is for new data, reset the HARQ process
  if (ndi)
    {
      m_harqPhyModule->ResetDiscHarqProcessNistStatus (rnti, resPsdch);
    }
}

void
NistLteSpectrumPhy::EndRxData ()
{                               
  NS_LOG_FUNCTION (this);
  NS_LOG_LOGIC (this << " ID:" << GetDevice()->GetNode()->GetId() << " state: " << m_state);

  NS_ASSERT (m_state == RX_DATA);

  // this will trigger CQI calculation and Error Model evaluation
  // as a side effect, the error model should update the error status of all TBs
  m_interferenceData->EndRx ();
  NS_LOG_DEBUG (this << " No. of burts " << m_rxPacketBurstList.size ());
  NS_LOG_DEBUG (this << " Expected TBs " << m_expectedTbs.size ());
  expectedTbs_t::iterator itTb = m_expectedTbs.begin ();
  
  // apply transmission mode gain
  NS_LOG_DEBUG (this << " txMode " << (uint16_t)m_transmissionMode << " gain " << m_txModeGain.at (m_transmissionMode));
  NS_ASSERT (m_transmissionMode < m_txModeGain.size ());
  m_sinrPerceived *= m_txModeGain.at (m_transmissionMode);
  
  while (itTb!=m_expectedTbs.end ())
    {
      if ((m_dataErrorModelEnabled)&&(m_rxPacketBurstList.size ()>0)) // avoid to check for errors when there is no actual data transmitted
        {
          // retrieve HARQ info
          HarqProcessInfoList_t harqInfoList;
          if ((*itTb).second.ndi == 0)
            {
              // TB retxed: retrieve HARQ history
              uint16_t ulHarqId = 0;
              if ((*itTb).second.downlink)
                {
                  harqInfoList = m_harqPhyModule->GetHarqProcessInfoDl ((*itTb).second.harqProcessId, (*itTb).first.m_layer);
                }
              else
                {
                  harqInfoList = m_harqPhyModule->GetHarqProcessInfoUl ((*itTb).first.m_rnti, ulHarqId);
                }
            }
          NistTbStats_t tbStats = NistLteMiErrorModel::GetTbDecodificationStats (m_sinrPerceived, (*itTb).second.rbBitmap, (*itTb).second.size, (*itTb).second.mcs, harqInfoList);
          (*itTb).second.mi = tbStats.mi;
          (*itTb).second.corrupt = m_random->GetValue () > tbStats.tbler ? false : true;
          NS_LOG_DEBUG (this << "RNTI " << (*itTb).first.m_rnti << " size " << (*itTb).second.size << " mcs " << (uint32_t)(*itTb).second.mcs << " bitmap " << (*itTb).second.rbBitmap.size () << " layer " << (uint16_t)(*itTb).first.m_layer << " TBLER " << tbStats.tbler << " corrupted " << (*itTb).second.corrupt);
          // fire traces on DL/UL reception PHY stats
          NistPhyReceptionStatParameters params;
          params.m_timestamp = Simulator::Now ().GetMilliSeconds ();
          params.m_cellId = m_cellId;
          params.m_imsi = 0; // it will be set by DlPhyTransmissionCallback in NistLteHelper
          params.m_rnti = (*itTb).first.m_rnti;
          params.m_txMode = m_transmissionMode;
          params.m_layer =  (*itTb).first.m_layer;
          params.m_mcs = (*itTb).second.mcs;
          params.m_size = (*itTb).second.size;
          params.m_rv = (*itTb).second.rv;
          params.m_ndi = (*itTb).second.ndi;
          params.m_correctness = (uint8_t)!(*itTb).second.corrupt;

          //Aziza
          //SpectrumValue& m_sinrPerceived : the perceived sinrs in the whole bandwidth
          //std::vector<int>& (*itTb).second.rbBitmap : the actives RBs for the TB

          SpectrumValue sinrCopy = m_sinrPerceived;
          std::vector<int> map = (*itTb).second.rbBitmap;
          double sum = 0.0;
          for (uint32_t i = 0; i < map.size (); ++i)
          {
            double sinrLin = sinrCopy[map.at (i)];
            sum = sum + sinrLin;
            //std::cout << "RB " << map.at (i) << "\tSINR " << 10 * std::log10 (sinrLin) << " dB" << std::endl;
          }
          params.m_sinrPerRb = sum / map.size ();
          //std::cout << "average sinr " << 10 * std::log10 (params.m_sinrPerRb) << " dB" <<  std::endl;
          //

          if ((*itTb).second.downlink)
            {
              // DL
              m_dlPhyReception (params);
            }
          else
            {
              // UL
              params.m_rv = harqInfoList.size ();
              m_ulPhyReception (params);
            }
       }
      
      itTb++;
    }
    std::map <uint16_t, NistDlInfoListElement_s> harqDlInfoMap;
    for (std::list<Ptr<PacketBurst> >::const_iterator i = m_rxPacketBurstList.begin (); 
    i != m_rxPacketBurstList.end (); ++i)
      {
        for (std::list<Ptr<Packet> >::const_iterator j = (*i)->Begin (); j != (*i)->End (); ++j)
          {
            // retrieve TB info of this packet 
            NistLteRadioBearerTag tag;
            (*j)->PeekPacketTag (tag);
            NistTbId_t tbId;
            tbId.m_rnti = tag.GetRnti ();
            tbId.m_layer = tag.GetLayer ();
            itTb = m_expectedTbs.find (tbId);
            NS_LOG_INFO (this << " Packet of " << tbId.m_rnti << " layer " <<  (uint16_t) tag.GetLayer ());
            if (itTb!=m_expectedTbs.end ())
              {
                if (!(*itTb).second.corrupt)
                  {
                    m_phyRxEndOkTrace (*j);
                
                    if (!m_ltePhyRxDataEndOkCallback.IsNull ())
                      {
                        m_ltePhyRxDataEndOkCallback (*j);
                      }
                  }
                else
                  {
                    // TB received with errors
                    m_phyRxEndErrorTrace (*j);
                  }

                // send HARQ feedback (if not already done for this TB)
                if (!(*itTb).second.harqFeedbackSent)
                  {
                    (*itTb).second.harqFeedbackSent = true;
                    if (!(*itTb).second.downlink)
                      {
                        NistUlInfoListElement_s harqUlInfo;
                        harqUlInfo.m_rnti = tbId.m_rnti;
                        harqUlInfo.m_tpc = 0;
                        if ((*itTb).second.corrupt)
                          {
                            harqUlInfo.m_receptionNistStatus = NistUlInfoListElement_s::NotOk;
                            NS_LOG_DEBUG (this << " RNTI " << tbId.m_rnti << " send UL-HARQ-NACK");
                            m_harqPhyModule->UpdateUlHarqProcessNistStatus (tbId.m_rnti, (*itTb).second.mi, (*itTb).second.size, (*itTb).second.size / EffectiveCodingRate [(*itTb).second.mcs]);
                          }
                        else
                          {
                            harqUlInfo.m_receptionNistStatus = NistUlInfoListElement_s::Ok;
                            NS_LOG_DEBUG (this << " RNTI " << tbId.m_rnti << " send UL-HARQ-ACK");
                            m_harqPhyModule->ResetUlHarqProcessNistStatus (tbId.m_rnti, (*itTb).second.harqProcessId);
                          }
                          if (!m_ltePhyUlHarqFeedbackCallback.IsNull ())
                            {
                              m_ltePhyUlHarqFeedbackCallback (harqUlInfo);
                            }
                      }
                    else
                      {
                        std::map <uint16_t, NistDlInfoListElement_s>::iterator itHarq = harqDlInfoMap.find (tbId.m_rnti);
                        if (itHarq==harqDlInfoMap.end ())
                          {
                            NistDlInfoListElement_s harqDlInfo;
                            harqDlInfo.m_harqNistStatus.resize (m_layersNum, NistDlInfoListElement_s::NACK);
                            harqDlInfo.m_rnti = tbId.m_rnti;
                            harqDlInfo.m_harqProcessId = (*itTb).second.harqProcessId;
                            if ((*itTb).second.corrupt)
                              {
                                harqDlInfo.m_harqNistStatus.at (tbId.m_layer) = NistDlInfoListElement_s::NACK;
                                NS_LOG_DEBUG (this << " RNTI " << tbId.m_rnti << " harqId " << (uint16_t)(*itTb).second.harqProcessId << " layer " <<(uint16_t)tbId.m_layer << " send DL-HARQ-NACK");
                                m_harqPhyModule->UpdateDlHarqProcessNistStatus ((*itTb).second.harqProcessId, tbId.m_layer, (*itTb).second.mi, (*itTb).second.size, (*itTb).second.size / EffectiveCodingRate [(*itTb).second.mcs]);
                              }
                            else
                              {

                                harqDlInfo.m_harqNistStatus.at (tbId.m_layer) = NistDlInfoListElement_s::ACK;
                                NS_LOG_DEBUG (this << " RNTI " << tbId.m_rnti << " harqId " << (uint16_t)(*itTb).second.harqProcessId << " layer " <<(uint16_t)tbId.m_layer << " size " << (*itTb).second.size << " send DL-HARQ-ACK");
                                m_harqPhyModule->ResetDlHarqProcessNistStatus ((*itTb).second.harqProcessId);
                              }
                            harqDlInfoMap.insert (std::pair <uint16_t, NistDlInfoListElement_s> (tbId.m_rnti, harqDlInfo));
                          }
                        else
                        {
                          if ((*itTb).second.corrupt)
                            {
                              (*itHarq).second.m_harqNistStatus.at (tbId.m_layer) = NistDlInfoListElement_s::NACK;
                              NS_LOG_DEBUG (this << " RNTI " << tbId.m_rnti << " harqId " << (uint16_t)(*itTb).second.harqProcessId << " layer " <<(uint16_t)tbId.m_layer << " size " << (*itHarq).second.m_harqNistStatus.size () << " send DL-HARQ-NACK");
                              m_harqPhyModule->UpdateDlHarqProcessNistStatus ((*itTb).second.harqProcessId, tbId.m_layer, (*itTb).second.mi, (*itTb).second.size, (*itTb).second.size / EffectiveCodingRate [(*itTb).second.mcs]);
                            }
                          else
                            {
                              NS_ASSERT_MSG (tbId.m_layer < (*itHarq).second.m_harqNistStatus.size (), " layer " << (uint16_t)tbId.m_layer);
                              (*itHarq).second.m_harqNistStatus.at (tbId.m_layer) = NistDlInfoListElement_s::ACK;
                              NS_LOG_DEBUG (this << " RNTI " << tbId.m_rnti << " harqId " << (uint16_t)(*itTb).second.harqProcessId << " layer " << (uint16_t)tbId.m_layer << " size " << (*itHarq).second.m_harqNistStatus.size () << " send DL-HARQ-ACK");
                              m_harqPhyModule->ResetDlHarqProcessNistStatus ((*itTb).second.harqProcessId);
                            }
                        }
                      } // end if ((*itTb).second.downlink) HARQ
                  } // end if (!(*itTb).second.harqFeedbackSent)
              }
          }
      }

  // send DL HARQ feedback to NistLtePhy
  std::map <uint16_t, NistDlInfoListElement_s>::iterator itHarq;
  for (itHarq = harqDlInfoMap.begin (); itHarq != harqDlInfoMap.end (); itHarq++)
    {
      if (!m_ltePhyDlHarqFeedbackCallback.IsNull ())
        {
          m_ltePhyDlHarqFeedbackCallback ((*itHarq).second);
        }
    }
  // forward control messages of this frame to NistLtePhy
  if (!m_rxControlMessageList.empty ())
    {
      if (!m_ltePhyRxCtrlEndOkCallback.IsNull ())
        {
          m_ltePhyRxCtrlEndOkCallback (m_rxControlMessageList);
        }
    }
  ChangeState (IDLE);
  m_rxPacketBurstList.clear ();
  m_rxControlMessageList.clear ();
  m_expectedTbs.clear ();
}

  void
NistLteSpectrumPhy::EndRxSlData ()
{
  NS_LOG_FUNCTION (this);
  NS_LOG_LOGIC (this << " ID:" << GetDevice()->GetNode()->GetId() << " state: " << m_state);

  NS_ASSERT (m_state == RX_DATA);

  // this will trigger CQI calculation and Error Model evaluation
  // as a side effect, the error model should update the error status of all TBs
  m_interferenceSl->EndRx ();
  NS_LOG_DEBUG (this << " No. of SL bursts " << m_rxPacketInfo.size ());
  NS_LOG_DEBUG (this << " Expected TBs (communication) " << m_expectedSlTbs.size ());
  NS_LOG_DEBUG (this << " Expected TBs (discovery) " << m_expectedDiscTbs.size ());
  NS_LOG_DEBUG (this << " No. Ctrl messages " << m_rxControlMessageList.size());

  // apply transmission mode gain
  // TODO: Check what is the mode for D2D (SIMO?)
  //       should it be done to each SINR reported?  
  //NS_LOG_DEBUG (this << " txMode " << (uint16_t)m_transmissionMode << " gain " << m_txModeGain.at (m_transmissionMode));
  NS_ASSERT (m_transmissionMode < m_txModeGain.size ());  
  //m_sinrPerceived *= m_txModeGain.at (m_transmissionMode);
  
  //Compute error on PSSCH
  //Create a mapping between the packet tag and the index of the packet bursts. We need this information to access the right SINR measurement.
  std::map <NistSlTbId_t, uint32_t> expectedTbToSinrIndex;
  for (uint32_t i = 0 ; i < m_rxPacketInfo.size() ; i++)
    {
      //even though there may be multiple packets, they all have
      //the same tag
      if (m_rxPacketInfo[i].m_rxPacketBurst) //if data packet
        { 
          std::list<Ptr<Packet> >::const_iterator j = m_rxPacketInfo[i].m_rxPacketBurst->Begin (); 
          // retrieve TB info of this packet 
          NistLteRadioBearerTag tag;
          (*j)->PeekPacketTag (tag);
          NistSlTbId_t tbId;
          tbId.m_rnti = tag.GetRnti ();
          tbId.m_l1dst = tag.GetDestinationL2Id () & 0xFF; //This is for ProSe, whereas V2X does not require L1 filtering, i.e., all the 24 bits are passed up to the MAC layer
          expectedTbToSinrIndex.insert (std::pair<NistSlTbId_t, uint32_t> (tbId, i));
        }
    }

  std::set<int> rbDecodedBitmap;
  if (m_dropRbOnCollisionEnabled)
    {
      NS_LOG_INFO (this << " PSSCH DropOnCollisionEnabled: Identifying RB Collisions");
      //Add new loop to make one pass and identify which RB have collisions
      std::set<int> rbDecodedBitmapTemp;
      NS_LOG_INFO ("No. of expected TBs: " << m_expectedSlTbs.size());
      for (expectedSlTbs_t::iterator itTb = m_expectedSlTbs.begin (); itTb != m_expectedSlTbs.end (); itTb++ )
        {
          for (std::vector<int>::iterator rbIt =  (*itTb).second.rbBitmap.begin (); rbIt != (*itTb).second.rbBitmap.end(); rbIt++)
            {
              if (rbDecodedBitmapTemp.find (*rbIt) != rbDecodedBitmapTemp.end()) 
                {
		          //collision, update the bitmap: this bitmap stores the collided RBs
                  rbDecodedBitmap.insert (*rbIt);  
                }
              else
                {
                  //store resources used by the packet to detect collision
                  rbDecodedBitmapTemp.insert (*rbIt);  
                }
            }
        }
      
    }



  //Compute error for each expected Tb
  expectedSlTbs_t::iterator itTb = m_expectedSlTbs.begin ();
  std::map <NistSlTbId_t, uint32_t>::iterator itSinr;
  while (itTb!=m_expectedSlTbs.end ())
    {
      itSinr = expectedTbToSinrIndex.find ((*itTb).first);
      if ((m_dataErrorModelEnabled)&&(m_rxPacketInfo.size ()>0)&&(itSinr != expectedTbToSinrIndex.end())) // avoid to check for errors when there is no actual data transmitted
        {
          // retrieve HARQ info
          HarqProcessInfoList_t harqInfoList;
          if ((*itTb).second.ndi == 0)
            {
              harqInfoList = m_harqPhyModule->GetHarqProcessInfoSl ((*itTb).first.m_rnti, (*itTb).first.m_l1dst);
              NS_LOG_DEBUG (this << " Nb Retx=" << harqInfoList.size());
              //std::cout << this << " Nb Retx=" << harqInfoList.size() << std::endl;
            }
          
          NS_LOG_DEBUG(this << "\t" << Simulator::Now ().GetMilliSeconds () << "\tFrom: " << (*itTb).first.m_rnti << "\tCorrupt: " << (*itTb).second.corrupt);

          bool rbDecoded = false;
          if (m_dropRbOnCollisionEnabled)
            {
              NS_LOG_DEBUG (this << " PSSCH DropOnCollisionEnabled: Labeling Corrupted TB");
              //Check if any of the RBs have been decoded
              for (std::vector<int>::iterator rbIt =  (*itTb).second.rbBitmap.begin (); rbIt != (*itTb).second.rbBitmap.end(); rbIt++)
                 {
                   if (rbDecodedBitmap.find (*rbIt) != rbDecodedBitmap.end ())
                     {
                        NS_LOG_DEBUG( this << "\t" << *rbIt << " decoded, labeled as corrupted!");
                        rbDecoded = true;
                        (*itTb).second.corrupt = true;

                     std::ofstream collisionFile; 
                     collisionFile.open("results/sidelink/collisionFile.unimo", std::ios_base::app);
                     collisionFile << Simulator::Now().GetSeconds() << "," << *rbIt << "\r\n";
                     collisionFile.close();
                        break;
                     }
                  }
            }

          if (!m_nistErrorModelEnabled)
            {
              NistTbStats_t tbStats = NistLteMiErrorModel::GetTbDecodificationStats (m_slSinrPerceived[(*itSinr).second]*4 /* Average gain for SIMO based on [CatreuxMIMO] */, (*itTb).second.rbBitmap, (*itTb).second.size, (*itTb).second.mcs, harqInfoList);
              (*itTb).second.mi = tbStats.mi;
                if(m_slBlerEnabled)
                  {
                    if(!rbDecoded)
                      {
                        (*itTb).second.corrupt = m_random->GetValue () > tbStats.tbler ? false : true;
                      }
                  }
              NS_LOG_DEBUG (this << " from RNTI " << (*itTb).first.m_rnti << " size " << (*itTb).second.size << " mcs " << (uint32_t)(*itTb).second.mcs << " bitmap " << (*itTb).second.rbBitmap.size () << " TBLER " << tbStats.tbler << " corrupted " << (*itTb).second.corrupt);
		//std::cout << this << " from RNTI " << (*itTb).first.m_rnti << " size " << (*itTb).second.size << " mcs " << (uint32_t)(*itTb).second.mcs << " bitmap " << (*itTb).second.rbBitmap.size () << " TBLER " << tbStats.tbler << " corrupted " << (*itTb).second.corrupt << " mean SINR " << GetMeanSinr (m_slSinrPerceived[(*itSinr).second]*4, (*itTb).second.rbBitmap) << std::endl;
            } 
          else // if nist Error Model Enabled
            {
              NistTbErrorStats_t tbStats = NistLtePhyErrorModel::GetPsschBler (m_fadingModel,NistLtePhyErrorModel::SISO, (*itTb).second.mcs, GetMeanSinr (m_slSinrPerceived[(*itSinr).second]*4 /* Average gain for SIMO based on [CatreuxMIMO] */, (*itTb).second.rbBitmap),  harqInfoList);
              (*itTb).second.sinr = tbStats.sinr;
              if(m_slBlerEnabled)
                {
                  if (!rbDecoded)
                    {
                      uint32_t nodeId = Simulator::GetContext();
                      (*itTb).second.corrupt = m_random->GetValue () > tbStats.tbler ? false : true;
                     if((*itTb).second.corrupt == 0){
                      /*std::ofstream errorFile;
                      errorFile.open("results/sidelink/tbRxFile.csv", std::ios_base::app);
                      errorFile << Simulator::Now().GetSeconds() << "," << tbStats.tbler << ", " << tbStats.sinr << ", " << (*itTb).second.corrupt << ", " << nodeId << "," << harqInfoList.size() << "\r\n";*/
                        }
                      }
                }
              NS_LOG_INFO (this << " from RNTI " << (*itTb).first.m_rnti << " size " << (*itTb).second.size << " mcs " << (uint32_t)(*itTb).second.mcs << " bitmap " << (*itTb).second.rbBitmap.size () << " TBLER " << tbStats.tbler << " corrupted " << (*itTb).second.corrupt);
              //std::cout << this << " from RNTI " << (*itTb).first.m_rnti << " size " << (*itTb).second.size << " mcs " << (uint32_t)(*itTb).second.mcs << " bitmap " << (*itTb).second.rbBitmap.size () << " TBLER " << tbStats.tbler << " corrupted " << (*itTb).second.corrupt << " mean SINR " << GetMeanSinr (m_slSinrPerceived[(*itSinr).second]*4, (*itTb).second.rbBitmap) << std::endl;

            }
          // std::cout << "Expected transmission on RBs= ";
          // for (uint32_t i = 0; i < (*itTb).second.rbBitmap.size (); i++)
          //   {
          //     std::cout << (*itTb).second.rbBitmap.at(i) << ",";
          //   }
          // std::cout << std::endl;
          // fire traces on SL reception PHY stats
          NistPhyReceptionStatParameters params;
          params.m_timestamp = Simulator::Now ().GetMilliSeconds ();
          params.m_cellId = m_cellId;
          params.m_imsi = 0; // it will be set by DlPhyTransmissionCallback in NistLteHelper
          params.m_rnti = (*itTb).first.m_rnti;
          params.m_txMode = m_transmissionMode;
          params.m_layer =  0;
          params.m_mcs = (*itTb).second.mcs;
          params.m_size = (*itTb).second.size;
          params.m_rv = (*itTb).second.rv;
          params.m_ndi = (*itTb).second.ndi;
          params.m_correctness = (uint8_t)!(*itTb).second.corrupt;
          params.m_sinrPerRb = GetMeanSinr (m_slSinrPerceived[(*itSinr).second]*4 /* Average gain for SIMO based on [CatreuxMIMO] */, (*itTb).second.rbBitmap);

          params.m_rv = harqInfoList.size ();
          m_slPhyReception (params);          
        }
      
      itTb++;
    }


  for (uint32_t i = 0 ; i < m_rxPacketInfo.size() ; i++)
    {
      //even though there may be multiple packets, they all have
      //the same tag
      if (m_rxPacketInfo[i].m_rxPacketBurst) //if data packet
        {
          for (std::list<Ptr<Packet> >::const_iterator j = m_rxPacketInfo[i].m_rxPacketBurst->Begin (); j != m_rxPacketInfo[i].m_rxPacketBurst->End (); ++j)
            {
              // retrieve TB info of this packet 
              NistLteRadioBearerTag tag;
              (*j)->PeekPacketTag (tag);
              NistSlTbId_t tbId;
              tbId.m_rnti = tag.GetRnti ();
              tbId.m_l1dst = tag.GetDestinationL2Id () & 0xFF;
              itTb = m_expectedSlTbs.find (tbId);//find this among the expected transport blocks
              NS_LOG_INFO (this << " Packet of " << tbId.m_rnti << " group " <<  (uint16_t) tbId.m_l1dst);
              //NS_ASSERT (itTb!=m_expectedSlTbs.end ());
              if (itTb!=m_expectedSlTbs.end ())
                {
                  if (!(*itTb).second.corrupt)
                    {
                      m_phyRxEndOkTrace (*j);
                
                      if (!m_ltePhyRxDataEndOkCallback.IsNull ())
                        { 
                          m_ltePhyRxDataEndOkCallback (*j);
                        }
                    }
                  else
                    {
                      // TB received with errors
                      m_phyRxEndErrorTrace (*j);
                    }

                  //store HARQ information
                  if (!(*itTb).second.harqFeedbackSent)
                    {
                      (*itTb).second.harqFeedbackSent = true;
                      //because we do not have feedbacks we do not reset HARQ now.
                      //we will do it when we expect a new data
                      if ((*itTb).second.corrupt)
                        {
                          if (!m_nistErrorModelEnabled)
                            {
                              m_harqPhyModule->UpdateSlHarqProcessNistStatus (tbId.m_rnti, tbId.m_l1dst, (*itTb).second.mi, (*itTb).second.size, (*itTb).second.size / EffectiveCodingRate [(*itTb).second.mcs]);
                            }
                          else
                            {
                              m_harqPhyModule->UpdateSlHarqProcessNistStatus (tbId.m_rnti, tbId.m_l1dst, (*itTb).second.sinr);
                            }
                        }
                      else
                        {
                          //m_harqPhyModule->ResetSlHarqProcessNistStatus (tbId.m_rnti, tbId.m_l1dst);
                        }
                      /*
                        if (!m_ltePhySlHarqFeedbackCallback.IsNull ())
                        {
                        m_ltePhySlHarqFeedbackCallback (harqSlInfo);
                        }
                      */
                    }
                }
            }
        }
    }


  /* Currently the MIB-SL is treated as a control message. Thus, the following logic applies also to the MIB-SL
   * The differences: calculation of BLER */
  // When control messages collide in the PSCCH, the receiver cannot know how many transmissions occured
  // we sort the messages by SINR and try to decode the ones with highest average SINR per RB first
  // only one message per RB can be decoded
  std::list<Ptr<NistLteControlMessage> > rxControlMessageOkList;
  bool error = true; 
  bool ctrlMessageFound = false;
  std::multiset<NistSlCtrlPacketInfo_t> sortedControlMessages;
  rbDecodedBitmap.clear ();

  

  for (uint32_t i = 0 ; i < m_rxPacketInfo.size() ; i++)
    {
      if (m_rxPacketInfo[i].m_rxControlMessage && m_rxPacketInfo[i].m_rxControlMessage->GetMessageType () != NistLteControlMessage::SL_DISC_MSG) //if control packet               
        { 
          /*if(m_rxPacketInfo[i].m_rxControlMessage->GetMessageType () == NistLteControlMessage::SCI_V2X )
           {
        NS_LOG_UNCOND ("SCI V2X");
            }*/
          double meanSinr = GetMeanSinr (m_slSinrPerceived[i], m_rxPacketInfo[i].rbBitmap);
          NistSlCtrlPacketInfo_t pInfo;
          pInfo.sinr = meanSinr;          
          pInfo.index = i;
          sortedControlMessages.insert (pInfo);

          //ctrl_msg_count++;
        }
    }
  
  if (m_dropRbOnCollisionEnabled)
    {
      NS_LOG_DEBUG (this << "Ctrl DropOnCollisionEnabled");
      //Add new loop to make one pass and identify which RB have collisions ???? What does it mean?
      std::set<int> rbDecodedBitmapTemp;
      //DEBUG:
      //std::cout<<Simulator::Now ().GetMilliSeconds () << " DEBUG sortedControlMessages RB:";

      for (std::multiset<NistSlCtrlPacketInfo_t>::iterator it =sortedControlMessages.begin(); it != sortedControlMessages.end(); it++ )
        {
          int i = (*it).index;
          
          
          //DEBUG:
          //for (std::vector<int>::iterator rbIt =  m_rxPacketInfo[i].rbBitmap.begin (); rbIt != m_rxPacketInfo[i].rbBitmap.end(); rbIt++)
          //  {
          //    std::cout<< " " << *rbIt;
          //  }
          ///////

          for (std::vector<int>::iterator rbIt =  m_rxPacketInfo[i].rbBitmap.begin (); rbIt != m_rxPacketInfo[i].rbBitmap.end(); rbIt++)
            {   //NS_LOG_UNCOND("Rb: " << *rbIt);
              if (rbDecodedBitmapTemp.find (*rbIt) != rbDecodedBitmapTemp.end()) 
                {
                   NS_LOG_DEBUG("collision, update the bitmap");
		          //collision, update the bitmap
                 // rbDecodedBitmap.insert ( m_rxPacketInfo[i].rbBitmap.begin(), m_rxPacketInfo[i].rbBitmap.end());
                 rbDecodedBitmapTemp.insert ( *rbIt);  
                  break;
                }
              else
                {
                 NS_LOG_DEBUG("store resources used by the packet to detect collision");
                  //store resources used by the packet to detect collision
                 // TODO FIXME BUG: check if this is SCI V2X, then change the way RBs are stored.
                 // For now, just change the code here: insert just one RB at a time

                 // FIXME rbDecodedBitmapTemp.insert ( m_rxPacketInfo[i].rbBitmap.begin(), m_rxPacketInfo[i].rbBitmap.end());
                 rbDecodedBitmapTemp.insert ( *rbIt);
                //DEBUG   
               //for (std::set<int>::iterator bittempit = rbDecodedBitmapTemp.begin(); bittempit != rbDecodedBitmapTemp.end(); ++bittempit)  {
                 //       NS_LOG_UNCOND("rbDecodedBitmap: "<< *bittempit);
                //  }

                }
            }
        }
      
      /////// DEBUG: /////////
      //std::cout<<std::endl;
      //std::cout<< this << " DEBUG rbDecodedBitmapTemp:";
      //for(std::set<int>::iterator it = rbDecodedBitmapTemp.begin(); it!= rbDecodedBitmapTemp.end(); it++)
      //  {
      //    std::cout<< " " << *it;
      //  }
      //  std::cout<<std::endl;
      ///////////////////////
    }
  /////// DEBUG: /////////
  //std::cout<< this << " DEBUG rbDecodedBitmap:";
  //for(std::set<int>::iterator it = rbDecodedBitmap.begin(); it!= rbDecodedBitmap.end(); it++)
  //  {
  //    std::cout<< " " << *it;
  //  }
  //  std::cout<<std::endl;
  ///////////////////////

  for (std::multiset<NistSlCtrlPacketInfo_t>::iterator it = sortedControlMessages.begin(); it != sortedControlMessages.end() ; it++ )
    {
      int i = (*it).index;

      bool ctrlError = false;
      ctrlMessageFound = true;

      
      if (m_ctrlErrorModelEnabled)
        {
          bool rbDecoded = false;
          for (std::vector<int>::iterator rbIt =  m_rxPacketInfo[i].rbBitmap.begin ();  rbIt != m_rxPacketInfo[i].rbBitmap.end() ; rbIt++)
            {
              if (rbDecodedBitmap.find (*rbIt) != rbDecodedBitmap.end()) 
                {
                  rbDecoded = true;
                  ctrlError = true;
                  //NS_LOG_UNCOND (this << " RB " << *rbIt << " has already been decoded ");
                  break;
                }
              else{
                  //NS_LOG_UNCOND (this << " RB " << *rbIt << " has not been decoded yet :) ");

              }
            }

          if (!rbDecoded)
            {
              //if (!m_dropRbOnCollisionEnabled)//uncomment IF_CONDITION if PSCCH BLER is not considered in this mode!
                //{
                  double  errorRate;
                  if ( m_rxPacketInfo[i].m_rxControlMessage->GetMessageType() == NistLteControlMessage::SCI)
                    {
                      errorRate = NistLtePhyErrorModel::GetPscchBler (m_fadingModel,NistLtePhyErrorModel::SISO, GetMeanSinr (m_slSinrPerceived[i]*4 /* Average gain for SIMO based on [CatreuxMIMO] */, m_rxPacketInfo[i].rbBitmap)).tbler;
                      ctrlError = m_random->GetValue () > errorRate ? false : true;
                      NS_LOG_DEBUG (this << " PSCCH Decoding, errorRate " << errorRate << " error " << ctrlError);
                    }
                  else if (m_rxPacketInfo[i].m_rxControlMessage->GetMessageType() == NistLteControlMessage::MIB_SL)
                    {
                      errorRate = NistLtePhyErrorModel::GetPsbchBler (m_fadingModel,NistLtePhyErrorModel::SISO, GetMeanSinr (m_slSinrPerceived[i]*4 /* Average gain for SIMO based on [CatreuxMIMO] */, m_rxPacketInfo[i].rbBitmap)).tbler;
                      ctrlError = m_random->GetValue () > errorRate ? false : true;
                      NS_LOG_DEBUG (this << " PSBCH Decoding, errorRate " << errorRate << " error " << ctrlError);
                    }
                  else
                    {
                      NS_LOG_DEBUG (this << " Unknown SL control message ");
                    }
                //}
            }
        }

      if (!ctrlError)
        {
          error = false; //at least one control packet is OK
          rxControlMessageOkList.push_back (m_rxPacketInfo[i].m_rxControlMessage);
          rbDecodedBitmap.insert ( m_rxPacketInfo[i].rbBitmap.begin(), m_rxPacketInfo[i].rbBitmap.end());
        }

      if(m_rxPacketInfo[i].m_rxControlMessage->GetMessageType () == NistLteControlMessage::SCI)
        {

          // Add PSCCH trace.
          NS_ASSERT(m_rxPacketInfo[i].m_rxControlMessage->GetMessageType () == NistLteControlMessage::SCI);
          Ptr<SciNistLteControlMessage> msg2 = DynamicCast<SciNistLteControlMessage> (m_rxPacketInfo[i].m_rxControlMessage);
          NistSciListElement_s sci = msg2->GetSci ();

          NistPhyReceptionStatParameters params;
          params.m_timestamp = Simulator::Now ().GetMilliSeconds ();
          params.m_cellId = m_cellId;
          params.m_imsi = 0; // it will be set by DlPhyTransmissionCallback in NistLteHelper
          params.m_rnti = sci.m_rnti;
          params.m_layer =  0;
          params.m_mcs = sci.m_mcs;
          params.m_size = sci.m_tbSize;
          params.m_rv = sci.m_rbStart;    // Using m_rv to store the RB start
          params.m_ndi = sci.m_rbLen;     // Using m_ndi to store the number of RBs used
          params.m_correctness = (uint8_t)!ctrlError;
          params.m_sinrPerRb = 0; // NOT USED, JUST INITIALIZED TO AVOID COMPILATION WARNING!
          params.m_txMode = m_transmissionMode; // NOT USED, JUST INITIALIZED TO AVOID COMPILATION WARNING!
          // Call trace
          m_slPscchReception (params);
        }
    }

  if (ctrlMessageFound)
    { 
      if (!error)
        { 
          if (!m_ltePhyRxCtrlEndOkCallback.IsNull ())
            {
              NS_LOG_DEBUG (this << " PSCCH OK"); 

              m_ltePhyRxCtrlEndOkCallback (rxControlMessageOkList);
            }
        }
      else
        {
          if (!m_ltePhyRxCtrlEndErrorCallback.IsNull ())
            {
              NS_LOG_DEBUG (this << " PSCCH Error");
              m_ltePhyRxCtrlEndErrorCallback ();
            }
        }
    }

    //discovery
    
    // error model and harq enabled for d2d discovery 
    if (m_errorModelHarqD2dDiscoveryEnabled)
    {
      std::map<NistDiscTbId_t, uint32_t> expectedTbToSinrDiscIndex;
      for (uint32_t i = 0 ; i < m_rxPacketInfo.size() ; i++)
        {
          //data isn't included and control is discovery message
          if (m_rxPacketInfo[i].m_rxControlMessage)
          {
            if (!m_rxPacketInfo[i].m_rxPacketBurst && m_rxPacketInfo[i].m_rxControlMessage->GetMessageType () == NistLteControlMessage::SL_DISC_MSG)
              {
                Ptr<NistLteControlMessage> rxCtrlMsg= m_rxPacketInfo[i].m_rxControlMessage;
                Ptr<NistSlDiscMessage> msg = DynamicCast<NistSlDiscMessage> (rxCtrlMsg);
                NistSlDiscMsg disc = msg->GetSlDiscMessage ();
                bool exist = FilterRxApps (disc);
                if (exist)
                {
                  // retrieve TB info of this packet
                  NistDiscTbId_t tbId;
                  tbId.m_rnti = disc.m_rnti;
                  tbId.m_resPsdch = disc.m_resPsdch;
                  expectedTbToSinrDiscIndex.insert (std::pair<NistDiscTbId_t, uint8_t> (tbId, i));

                  std::list<Ptr<SidelinkDiscResourcePool> >::iterator discIt;
                  for (discIt = m_discRxPools.begin (); discIt != m_discRxPools.end(); discIt++)
                    {
                      std::list<SidelinkDiscResourcePool::SidelinkTransmissionInfo> m_psdchTx = (*discIt)->GetPsdchTransmissions (disc.m_resPsdch);
                      std::list<SidelinkDiscResourcePool::SidelinkTransmissionInfo>::iterator rxIt = m_psdchTx.begin ();
                      if (rxIt != m_psdchTx.end())
                        {
                          //reception
                          NS_LOG_INFO (this << " Expecting PSDCH reception RB " << (uint16_t) (disc.m_resPsdch));
                          std::list<SidelinkDiscResourcePool::SidelinkTransmissionInfo>::iterator txIt = m_psdchTx.begin ();
                          std::vector <int> rbMap;
                          for (int i = txIt->rbStart ; i < txIt->rbStart + txIt->nbRb ; i++)
                          {
                            NS_LOG_LOGIC (this << " Receiving PSDCH on RB " << i);
                            rbMap.push_back (i);
                          }
                      
                          AddExpectedTb (disc.m_rnti, disc.m_resPsdch, m_psdchTx.size() % 4 == 0, rbMap, (4 - m_psdchTx.size () % 4));
                        }
                    }
                }
              }
          }
        }

      //           
      expectedDiscTbs_t::iterator itTbDisc = m_expectedDiscTbs.begin ();
      std::map<NistDiscTbId_t, uint32_t>::iterator itSinrDisc;
      while (itTbDisc!= m_expectedDiscTbs.end ())
        {
          itSinrDisc = expectedTbToSinrDiscIndex.find ((*itTbDisc).first);
          //to check: m_ctrlErrorModelEnabled or should we have m_dataErrorModelEnabled enabled?
          if ((m_ctrlErrorModelEnabled)&&(m_rxPacketInfo.size ()>0)&&(itSinrDisc != expectedTbToSinrDiscIndex.end())) // avoid to check for errors when there is no actual discovery transmitted
            {   
              // retrieve HARQ info
              HarqProcessInfoList_t harqInfoList;
              if ((*itTbDisc).second.ndi == 0)
                {
                  harqInfoList = m_harqPhyModule->GetHarqProcessInfoDisc ((*itTbDisc).first.m_rnti,(*itTbDisc).first.m_resPsdch);
                  NS_LOG_DEBUG (this << " Nb Retx=" << harqInfoList.size());
                  //std::cout << this << " Nb Retx=" << harqInfoList.size() << std::endl;
                }
                           
              if (!m_nistErrorModelEnabled)
                {
                  NS_LOG_ERROR ("Any error model other than the NistErrorModel is not supported");
                } 
              else 
                {
                  NistTbErrorStats_t tbStats = NistLtePhyErrorModel::GetPsdchBler (m_fadingModel,NistLtePhyErrorModel::SISO, GetMeanSinr (m_slSinrPerceived[(*itSinrDisc).second]*4 /* Average gain for SIMO based on [CatreuxMIMO] */, (*itTbDisc).second.rbBitmap),  harqInfoList);
                  (*itTbDisc).second.sinr = tbStats.sinr;
                  (*itTbDisc).second.corrupt = m_random->GetValue () > tbStats.tbler ? false : true;
                  NS_LOG_DEBUG (this << " from RNTI " << (*itTbDisc).first.m_rnti << " TBLER " << tbStats.tbler << " corrupted " << (*itTbDisc).second.corrupt);
                  //std::cout << this << " from RNTI " << (*itTbDisc).first.m_rnti << " TBLER " << tbStats.tbler << " corrupted " << (*itTbDisc).second.corrupt << " mean SINR " << GetMeanSinr (m_slSinrPerceived[(*itSinrDisc).second]*4, (*itTbDisc).second.rbBitmap) << std::endl;
                }
                  
              //traces for discovery rx
              //we would know it is discovery mcs=0 and size=232
              NistPhyReceptionStatParameters params;
              params.m_timestamp = Simulator::Now ().GetMilliSeconds ();
              params.m_cellId = m_cellId;
              params.m_imsi = 0; // it will be set by DlPhyTransmissionCallback in NistLteHelper
              params.m_rnti = (*itTbDisc).first.m_rnti;
              params.m_txMode = m_transmissionMode;
              params.m_layer =  0;
              params.m_mcs = 0; //for discovery, we use a fixed modulation (no mcs defined), use 0 to identify discovery
              params.m_size = 232; // discovery message has a static size
              params.m_rv = (*itTbDisc).second.rv;
              params.m_ndi = (*itTbDisc).second.ndi;
              params.m_correctness = (uint8_t)!(*itTbDisc).second.corrupt;
              params.m_sinrPerRb = GetMeanSinr (m_slSinrPerceived[(*itSinrDisc).second]*4 /* Average gain for SIMO based on [CatreuxMIMO] */, (*itTbDisc).second.rbBitmap);
              params.m_rv = harqInfoList.size ();
              m_slPhyReception (params);  
            }
          itTbDisc++;
        }

      

      // handling collison for discovery
      // same as sidelink control but taking into account HARQ
      std::list<Ptr<NistLteControlMessage> > rxDiscMessageOkList;
      
      bool discError = true; 
      bool discMessageFound = false;
      std::set<NistSlCtrlPacketInfo_t> sortedDiscMessages;
      std::set<int> discDecodedBitmap;

      for (uint32_t i = 0 ; i < m_rxPacketInfo.size() ; i++)
        {
          Ptr<NistLteControlMessage> rxCtrlMsg= m_rxPacketInfo[i].m_rxControlMessage;
          Ptr<NistSlDiscMessage> msg = DynamicCast<NistSlDiscMessage> (rxCtrlMsg);
          if (msg)
          {
            NistSlDiscMsg disc = msg->GetSlDiscMessage ();
            bool exist = FilterRxApps (disc);
            if (exist)
            {
              if (m_rxPacketInfo[i].m_rxControlMessage->GetMessageType () == NistLteControlMessage::SL_DISC_MSG) //if discovery message     
                {
                  double meanSinr = GetMeanSinr (m_slSinrPerceived[i], m_rxPacketInfo[i].rbBitmap);
                  NistSlCtrlPacketInfo_t pInfo;
                  pInfo.sinr = meanSinr;          
                  pInfo.index = i;
                  sortedDiscMessages.insert (pInfo);
                }
            }
          }
        }
     
      // check all sorted discovery messages
      for (std::set<NistSlCtrlPacketInfo_t>::iterator it =sortedDiscMessages.begin(); it != sortedDiscMessages.end() ; it++ )
        {
          discMessageFound = true;
          // retrieve TB info of this packet
          int i = (*it).index;
          Ptr<NistLteControlMessage> rxCtrlMsg= m_rxPacketInfo[i].m_rxControlMessage;
          Ptr<NistSlDiscMessage> msg = DynamicCast<NistSlDiscMessage> (rxCtrlMsg);
          NistSlDiscMsg disc = msg->GetSlDiscMessage ();
          NistDiscTbId_t tbId;
          tbId.m_rnti = disc.m_rnti;
          tbId.m_resPsdch = disc.m_resPsdch;
          
          itTbDisc =  m_expectedDiscTbs.find (tbId);
          NS_LOG_INFO (this << " Packet of " << tbId.m_rnti << " resPsdch " <<  (uint16_t) tbId.m_resPsdch); 

          bool rbDecoded = false;
          for (std::vector<int>::iterator rbIt =  m_rxPacketInfo[i].rbBitmap.begin ();  rbIt != m_rxPacketInfo[i].rbBitmap.end() ; rbIt++)
            {
              if (discDecodedBitmap.find (*rbIt) != discDecodedBitmap.end()) 
              {
                rbDecoded = true;
                NS_LOG_DEBUG (this << " RB " << *rbIt << " has already been decoded ");
                break;
              }
            }

          if (itTbDisc!=m_expectedDiscTbs.end () && !rbDecoded)
          {
            if (!(*itTbDisc).second.corrupt)
            {
              discError = false; 
              rxDiscMessageOkList.push_back (m_rxPacketInfo[i].m_rxControlMessage);
              discDecodedBitmap.insert ( m_rxPacketInfo[i].rbBitmap.begin(), m_rxPacketInfo[i].rbBitmap.end());
                        
              //to add theta and sinr to NistDiscTxProbabilityModule
              //assuming we have only one pool
              Ptr<NistSlDiscMessage> msg = DynamicCast<NistSlDiscMessage> (m_rxPacketInfo[i].m_rxControlMessage);
              NistSlDiscMsg disc = msg->GetSlDiscMessage ();
              NS_LOG_DEBUG (this << " from RNTI " << disc.m_rnti << " ProSeAppCode " << disc.m_proSeAppCode.to_ulong () << " SINR " << (*it).sinr);
            }

            //store HARQ information
            if (!(*itTbDisc).second.harqFeedbackSent)
            {
              (*itTbDisc).second.harqFeedbackSent = true;
              //because we do not have feedbacks we do not reset HARQ now.
              //we will do it when we expect a new data
              if ((*itTbDisc).second.corrupt)
              {
                if (!m_nistErrorModelEnabled)
                {
                  NS_LOG_ERROR ("Any error model other than the NistErrorModel is not supported");
                }
                else
                {
                  m_harqPhyModule->UpdateDiscHarqProcessNistStatus (tbId.m_rnti, tbId.m_resPsdch, (*itTbDisc).second.sinr);
                }
              }
            }
          }     
        }

      if (discMessageFound)
      {        
        if (!discError)
        {
          if (!m_ltePhyRxCtrlEndOkCallback.IsNull ())
          {
            NS_LOG_DEBUG (this << " Discovery OK");
            m_ltePhyRxCtrlEndOkCallback (rxDiscMessageOkList);
          }
        }
        else
        {
          if (!m_ltePhyRxCtrlEndErrorCallback.IsNull ())
          {
            NS_LOG_DEBUG (this << " Discovery Error");
            m_ltePhyRxCtrlEndErrorCallback ();
          }
        }
      }
    }
      
    //error model and harq not enabled for d2d discovery
    //discard colliding packets for discovery
    else
    {
      for (uint32_t i = 0 ; i < m_rxPacketInfo.size() ; i++)
        { 
          if (m_rxPacketInfo[i].m_rxControlMessage) //if control packet               
            {
              if (!m_rxPacketInfo[i].m_rxPacketBurst && m_rxPacketInfo[i].m_rxControlMessage->GetMessageType () == NistLteControlMessage::SL_DISC_MSG)
              {
                Ptr<NistLteControlMessage> rxCtrlMsg= m_rxPacketInfo[i].m_rxControlMessage;
                Ptr<NistSlDiscMessage> msg = DynamicCast<NistSlDiscMessage> (rxCtrlMsg);
                NistSlDiscMsg disc = msg->GetSlDiscMessage ();
                bool exist = FilterRxApps (disc);
                if (exist)
                {
                  double meanSinr = GetMeanSinr (m_slSinrPerceived[i], m_rxPacketInfo[i].rbBitmap);
                  NistSlCtrlPacketInfo_t pInfo;
                  pInfo.sinr = meanSinr;          
                  pInfo.index = i;
                  sortedControlMessages.insert (pInfo);
                }
              }
            }
        }
       
      NS_ASSERT (m_slSinrPerceived.size ()>0);
      uint32_t countRb=0;
      for (Values::iterator vit = m_slSinrPerceived[0].ValuesBegin (); vit != m_slSinrPerceived[0].ValuesEnd (); ++vit)
      {
        countRb++;
      }

      std::vector<uint32_t> rbsUsed (countRb, 0);
      for (uint32_t i = 0; i < m_rxPacketInfo.size (); i++) 
        {
          for (std::vector<int>::iterator rbIt =  m_rxPacketInfo[i].rbBitmap.begin ();  rbIt != m_rxPacketInfo[i].rbBitmap.end() ; rbIt++)
            {
              rbsUsed[*rbIt]++;
            }
        }

     
      for (std::set<NistSlCtrlPacketInfo_t>::iterator it =sortedControlMessages.begin(); it != sortedControlMessages.end() ; it++ )
        {
          int i = (*it).index;

          bool ctrlError = false;
          ctrlMessageFound = true;
          
          if (m_ctrlErrorModelEnabled)
            {          
              bool rbDecoded = false;
              for (std::vector<int>::iterator rbIt =  m_rxPacketInfo[i].rbBitmap.begin ();  rbIt != m_rxPacketInfo[i].rbBitmap.end() ; rbIt++)
                {
                  if (rbDecodedBitmap.find (*rbIt) != rbDecodedBitmap.end()) 
                    {
                      rbDecoded = true;
                      NS_LOG_DEBUG (this << " RB " << *rbIt << " has already been decoded ");
                      break;
                    }
                }
              if (!rbDecoded)
                {
                  bool ok = true;
                  for (std::vector<int>::iterator rbIt =  m_rxPacketInfo[i].rbBitmap.begin ();  rbIt != m_rxPacketInfo[i].rbBitmap.end() ; rbIt++)
                    {
                      if (rbsUsed [*rbIt] > 1) 
                        {
                          ok = false;
                          break;
                        }
                    }
                  if (ok)
                    {
                      double  errorRate = NistLtePhyErrorModel::GetPscchBler (m_fadingModel,NistLtePhyErrorModel::SISO, GetMeanSinr (m_slSinrPerceived[i]*4 /* Average gain for SIMO based on [CatreuxMIMO] */, m_rxPacketInfo[i].rbBitmap)).tbler;
                      ctrlError = m_random->GetValue () > errorRate ? false : true;
                      NS_LOG_DEBUG (this << " Discovery Decodification, errorRate " << errorRate << " error " << ctrlError);
                    } 
                  else
                    {
                      ctrlError = true;
                    }
                }
              else {
                ctrlError = true;
              }
            }
          
          if (!ctrlError)
            {
              error = false; //at least one control packet is OK
              rxControlMessageOkList.push_back (m_rxPacketInfo[i].m_rxControlMessage);
              rbDecodedBitmap.insert ( m_rxPacketInfo[i].rbBitmap.begin(), m_rxPacketInfo[i].rbBitmap.end());

              Ptr<NistSlDiscMessage> msg = DynamicCast<NistSlDiscMessage> (m_rxPacketInfo[i].m_rxControlMessage);
              NistSlDiscMsg disc = msg->GetSlDiscMessage ();
              NS_LOG_DEBUG (this << " from RNTI " << disc.m_rnti << " ProSeAppCode " << disc.m_proSeAppCode.to_ulong () << " SINR " << (*it).sinr);
            } 
        }

        if (ctrlMessageFound)
        {
          if (!error)
            {
              if (!m_ltePhyRxCtrlEndOkCallback.IsNull ())
                {
                  NS_LOG_DEBUG (this << "Discovery OK");
                  m_ltePhyRxCtrlEndOkCallback (rxControlMessageOkList);
                }
            }
          else
            {
              if (!m_ltePhyRxCtrlEndErrorCallback.IsNull ())
                {
                  NS_LOG_DEBUG (this << "Discovery Error");
                  m_ltePhyRxCtrlEndErrorCallback ();
                }
            }
        }    
    }

  //done with sidelink data, control and discovery
  ChangeState (IDLE);
  m_rxPacketBurstList.clear ();
  m_rxControlMessageList.clear ();
  //m_rxControlMessageRbMap.clear ();
  m_rxPacketInfo.clear ();
  m_expectedSlTbs.clear ();
  m_expectedDiscTbs.clear ();
}


void
NistLteSpectrumPhy::EndRxV2XSlData ()
{
  NS_LOG_FUNCTION (this);
  float Plos;
  double MIMOGain = 1, Distance;
//  double correctionFactor = 6; // Please remove it
  double correctionFactor = 1; // The correction factor was needed when using the NIST BLER curves. 
  double sigmaNLOSv = 4; //dB, NLOSv Highway scenario
  double shadowingNLOSv;
  int nCollidedRBs = 0, PSCCHlen = 2; // PSCCH is always 2 RBs long in Release 14 C-V2X 
//int PSCCHstart;
  Vector posTX, posRX;
  Ptr<MobilityModel> mobTX;
//  std::map<int,Ptr<Node>>::iterator TxNodeIT;
  std::vector<int>::iterator CtrlCheck;
  Ptr<Node> TxNode;
  bool saveCollisions = m_saveCollisions;
  bool LOS;
  //bool psschCollision = false;
  // bool pscchCollision = true;

  // V2V
  Ptr<NormalRandomVariable> randomNormal =  CreateObject<NormalRandomVariable> ();  // Log-normal random variable for the addition of NLOSv shadowing in the Highway scenario

  // NodeContainer storing all nodes stored in ns3::NodeList
  NodeContainer GlobalContainer = NodeContainer::GetGlobal(); //Used to evaluate the transmitter-receiver distance at spectrum layer

  NS_LOG_LOGIC (this << " ID:" << GetDevice()->GetNode()->GetId() << " state: " << m_state << " Time " << Simulator::Now ().GetSeconds () << " Memory address " << GetDevice()->GetNode());
 
  // Adding position evaluation for the new PHY layer (see 3GPP TR 37.885)
  Ptr<MobilityModel> mob = GetDevice()->GetNode()->GetObject<MobilityModel>();
  posRX = mob->GetPosition();
  NS_LOG_LOGIC (this << " Node: " << GetDevice()->GetNode()->GetId() << " Position X = " << posRX.x << " Position Y = " << posRX.y);

/*  for(std::map<uint32_t,CountersLosses>::iterator ITT = m_lostPKTs.begin(); ITT != m_lostPKTs.end(); ITT++)
  {
     NS_LOG_UNCOND("ID RX " << GetDevice()->GetNode()->GetId() << " ID Tx " << ITT->first << " collisions " << ITT->second.collisionLosses << " prop " << ITT->second.propagationLosses << " OK " << ITT->second.totalOK);
  }*/
 // std::cin.get();

  NS_ASSERT (m_state == RX_DATA);
  // this will trigger CQI calculation and Error Model evaluation
  // as a side effect, the error model should update the error status of all TBs
  m_interferenceSl->EndRx ();
  NS_LOG_DEBUG (this << " No. of SL bursts " << m_rxPacketInfo.size ());
  NS_LOG_DEBUG (this << " Expected TBs (communication) " << m_expectedSlTbs.size ());
  NS_LOG_DEBUG (this << " Expected TBs (discovery) " << m_expectedDiscTbs.size ());
  NS_LOG_DEBUG (this << " No. Ctrl messages " << m_rxControlMessageList.size());

  m_totalReceptions += m_expectedSlTbs.size ();

  // apply transmission mode gain
  // TODO: Check what is the mode for D2D (SIMO?)
  //       should it be done to each SINR reported?  
  //NS_LOG_DEBUG (this << " txMode " << (uint16_t)m_transmissionMode << " gain " << m_txModeGain.at (m_transmissionMode));
  NS_ASSERT (m_transmissionMode < m_txModeGain.size ());  
  //m_sinrPerceived *= m_txModeGain.at (m_transmissionMode);

  //Compute error on PSSCH
  //Create a mapping between the packet tag and the index of the packet bursts. We need this information to access the right SINR measurement.
  std::map <NistSlTbId_t, uint32_t> expectedTbToSinrIndex;
  for (uint32_t i = 0 ; i < m_rxPacketInfo.size() ; i++)
  {
     //even though there may be multiple packets, they all have the same tag
     if (m_rxPacketInfo[i].m_rxPacketBurst) //if data packet
     { 
/*   	  NistV2XSciListElement_s sciT; // Set of instructions to retrieve the first RB used for control message
          Ptr<SciV2XLteControlMessage> msg2 = DynamicCast<SciV2XLteControlMessage> (m_rxPacketInfo[i].m_rxControlMessage);
          sciT = msg2->GetSci ();
	  PSCCHstart = (int)sciT.m_rbStartPscch;
	  NS_LOG_DEBUG("PSSCH RBs " << (int)sciT.m_rbStartPscch << " of Length " << (int)sciT.m_rbLenPscch);
  	  std::cin.get();*/
          std::list<Ptr<Packet> >::const_iterator j = m_rxPacketInfo[i].m_rxPacketBurst->Begin (); 
          // retrieve TB info of this packet 
          NistLteRadioBearerTag tag;
          (*j)->PeekPacketTag (tag);
          NistSlTbId_t tbId;
          tbId.m_rnti = tag.GetRnti ();
          tbId.m_l1dst = tag.GetDestinationL2Id () & 0xFF; //This is for ProSe, whereas V2X does not require L1 filtering, i.e., all the 24 bits are passed up to the MAC layer
          expectedTbToSinrIndex.insert (std::pair<NistSlTbId_t, uint32_t> (tbId, i));
     }
  }
  
  std::set<int> rbDecodedBitmap;
  if (m_dropRbOnCollisionEnabled) //useless
  {
      NS_LOG_INFO (this << " PSSCH DropOnCollisionEnabled: Identifying RB Collisions");
      //Add new loop to make one pass and identify which RB have collisions
      std::set<int> rbDecodedBitmapTemp;
      NS_LOG_INFO ("No. of expected TBs: " << m_expectedSlTbs.size());
      for (expectedSlTbs_t::iterator itTb = m_expectedSlTbs.begin (); itTb != m_expectedSlTbs.end (); itTb++ )
      {
          for (std::vector<int>::iterator rbIt =  (*itTb).second.rbBitmap.begin (); rbIt != (*itTb).second.rbBitmap.end(); rbIt++)
          {
              if (rbDecodedBitmapTemp.find (*rbIt) != rbDecodedBitmapTemp.end()) 
              {
		   //collision, update the bitmap: this bitmap stores the collided RBs
                   rbDecodedBitmap.insert (*rbIt);  
              }
              else
              {
                  //store resources used by the packet to detect collision
                  rbDecodedBitmapTemp.insert (*rbIt);  
              }
          }
      }    
  } // end if (m_dropRbOnCollisionEnabled)

  NS_LOG_DEBUG("-----------SNR MAP-----------");
  for (uint16_t i = 0; i < m_expectedSlTbSNR.size(); i++)
  {
    NS_LOG_DEBUG(m_expectedSlTbSNR[i].first << "," << m_expectedSlTbSNR[i].second);
  }
  //Sort the received TBs SCI
  double tmpSINR, newSINR; // in linear scale
  expectedSlTbs_t::iterator itTb_BestSINR;
  expectedSlTbs_t::iterator itTb;
  std::map <NistSlTbId_t, uint32_t>::iterator itSinr;
  NS_LOG_DEBUG("Evaluating the SCI");
  // I can decode only one SCI at a time. Sort them and take only the one with the best SINR
  uint16_t N_subCh = 4;
  uint16_t SubCh_Len = 12;
  for (uint16_t i = 0; i < N_subCh; i++) 
  {
    tmpSINR = 0;
    itTb = m_expectedSlTbs.begin ();
    while (itTb!=m_expectedSlTbs.end ()) 
    {
      itSinr = expectedTbToSinrIndex.find ((*itTb).first);
      if (m_rxPacketInfo[(*itSinr).second].rbBitmap[0] == i*SubCh_Len)
      {
        // NS_LOG_DEBUG("SCI in the " << i << "-th space");
          newSINR = GetMeanSinrPSCCH(m_slSinrPerceived[(*itSinr).second], (*itTb).second.rbBitmap, PSCCHlen);
         if (newSINR > tmpSINR) 
         {
            tmpSINR = newSINR;
            itTb_BestSINR = itTb;
         }

      }
      itTb++;
    } 
    itTb = m_expectedSlTbs.begin ();
    while (itTb!=m_expectedSlTbs.end ()) 
    {
      itSinr = expectedTbToSinrIndex.find ((*itTb).first);
      if ((m_rxPacketInfo[(*itSinr).second].rbBitmap[0] == i*SubCh_Len) && (itTb != itTb_BestSINR))
      {
         itTb->second.corrupt = true;
      }
      itTb++;
    } 

  } 
      /* //Print the PSSCH RBs
      for (std::vector<int>::iterator rbIT = itTb->second.rbBitmap.begin (); rbIT != itTb->second.rbBitmap.end (); rbIT++)
      {
          NS_LOG_DEBUG("RB " << *rbIT);
       
      }*/
  NS_LOG_DEBUG("Evaluating the PSSCH");
  // Now check if there is any PSSCH overlapping among the valid SCIs
  expectedSlTbs_t::iterator itTb_2;
  std::map <NistSlTbId_t, uint32_t>::iterator itSinr_2;
  int cmpOutput;
  itTb = m_expectedSlTbs.begin ();
  while (itTb!=m_expectedSlTbs.end ()) 
  {
    if ((!itTb->second.corrupt) && (!itTb->second.collidedPssch))
    { 
      itSinr = expectedTbToSinrIndex.find ((*itTb).first);
      itTb_2 = m_expectedSlTbs.begin ();
      while (itTb_2!=m_expectedSlTbs.end ()) 
      { 
        if ((itTb != itTb_2) && (!itTb_2->second.corrupt))
        { 
          itSinr_2 = expectedTbToSinrIndex.find ((*itTb_2).first);
          NS_LOG_DEBUG("Evaluating UEs # " << itTb->first.m_rnti << " and # " << itTb_2->first.m_rnti);
          // returns 0 if there is no overlap, returns 1 if the first avg SINR is stronger than the second and returns 2 otherwise
          cmpOutput = UnimoreCompareSinrPSSCH(m_slSinrPerceived[(*itSinr).second], (*itTb).second.rbBitmap,m_slSinrPerceived[(*itSinr_2).second], (*itTb_2).second.rbBitmap); 
          if (cmpOutput == 0)  
          { 
             NS_LOG_DEBUG("No overlap in the PSSCH");
          }
          else if (cmpOutput == 1)  
          {
             NS_LOG_DEBUG("First SINR is larger");
             itTb_2->second.collidedPssch = true;
             if ((*itTb).second.rbBitmap.size() > (*itTb_2).second.rbBitmap.size())
             {
                itTb_2->second.corrupt = true;
             }
          }
          else if (cmpOutput == 2)  
          {
             NS_LOG_DEBUG("Second SINR is larger");
             itTb->second.collidedPssch = true;
             if ((*itTb).second.rbBitmap.size() < (*itTb_2).second.rbBitmap.size())
             {
                itTb->second.corrupt = true;
             }
          }
          else
          {
             // Useless
             NS_LOG_DEBUG("Unknown value " << cmpOutput);
          }
            
        }
        itTb_2++;   
      }
    }
    itTb++;
  } 


  itTb = m_expectedSlTbs.begin ();
  while (itTb!=m_expectedSlTbs.end ()) 
  {
    NS_LOG_DEBUG("Tx node ID " << itTb->first.m_rnti << " Corrupted? " <<  itTb->second.corrupt << " Collided PSSCH? " << itTb->second.collidedPssch);
    if (itTb->second.corrupt || itTb->second.collidedPssch)
    {
      HarqProcessInfoList_t harqInfoList;
      harqInfoList = m_harqPhyModule->GetHarqProcessInfoSl ((*itTb).first.m_rnti, (*itTb).first.m_l1dst);
      // Search the transmitter ID within the global container
      for (NodeContainer::Iterator L = GlobalContainer.Begin(); L != GlobalContainer.End(); ++L) 
      {
        TxNode = *L;
        if ( (*itTb).first.m_rnti == TxNode->GetId())
        {
          mobTX = TxNode->GetObject<MobilityModel>();
          posTX = mobTX->GetPosition(); 
          break;
        }
      }

      Distance = std::sqrt(std::pow(posTX.x-posRX.x,2) + std::pow(posTX.y-posRX.y,2));
      shadowingNLOSv = randomNormal->GetValue (5 + std::max(0.0,(15*std::log10(Distance))-41), (sigmaNLOSv*sigmaNLOSv));// Due to the presence of other vehicles
      shadowingNLOSv = std::max(0.0,shadowingNLOSv);
      if (Distance <= 475)
      {
	 Plos = std::min(1.0,2.1013e-6*Distance*Distance - 0.002*Distance + 1.0193);  // Probability of being in LOS in the Highway scenario (see 3GPP TR 37.885)
      }
      else
      {
	 Plos = std::max(0.0,0.54 - 0.001*(Distance-475));
      }
      LOS = m_random->GetValue () > Plos ? false : true;
      NS_LOG_DEBUG(this << " TX Node: " << TxNode->GetId() << " Position X = " << posTX.x << " Position Y = " << posTX.y << " Tx-Rx Distance " << Distance << " P(LOS) " << Plos << " LOS " << LOS);
      double SNR;
      for (uint16_t i = 0; i < m_expectedSlTbSNR.size(); i++)
      {
        if (m_expectedSlTbSNR[i].first == (*itTb).first.m_rnti)
        SNR = m_expectedSlTbSNR[i].second;
      }
      NS_LOG_DEBUG("SNR value is " << SNR);
      NistTbErrorStats_t tbStats;
      NistTbErrorStats_t tbStatsPSCCH1;
      NistTbErrorStats_t tbStatsPSCCH2;
      if (LOS)
      {
        NS_LOG_DEBUG (this << " Not already collided PSSCH. Computing the PSSCH BLER in LOS (without Interference) ");
        tbStats = NistLtePhyErrorModel::GetV2VPsschBler (m_fadingModel,NistLtePhyErrorModel::SISO, (*itTb).second.mcs, SNR,  harqInfoList, LOS, Distance, (*itTb).second.rbBitmap.size());
        NS_LOG_DEBUG (this << " Computing the PSCCH BLER (1) ");
        tbStatsPSCCH1 = NistLtePhyErrorModel::GetV2VPscchBler (m_fadingModel,NistLtePhyErrorModel::SISO, (*itTb).second.mcs, SNR,  harqInfoList, LOS, Distance);
        NS_LOG_DEBUG (this << " Computing the PSCCH BLER (2) ");
        tbStatsPSCCH2 = NistLtePhyErrorModel::GetV2VPscchBler (m_fadingModel,NistLtePhyErrorModel::SISO, (*itTb).second.mcs, SNR,  harqInfoList, LOS, Distance);
      }
      else
      {
        NS_LOG_DEBUG (this << " Not already collided PSSCH. Computing the PSSCH BLER in NLOSv (without Interference) ");
        tbStats = NistLtePhyErrorModel::GetV2VPsschBler (m_fadingModel,NistLtePhyErrorModel::SISO, (*itTb).second.mcs, SNR/std::pow(10,shadowingNLOSv/10),  harqInfoList, LOS, Distance, (*itTb).second.rbBitmap.size());
        NS_LOG_DEBUG (this << " Computing the PSCCH BLER (1) ");
        tbStatsPSCCH1 = NistLtePhyErrorModel::GetV2VPscchBler (m_fadingModel,NistLtePhyErrorModel::SISO, (*itTb).second.mcs, SNR/std::pow(10,shadowingNLOSv/10),  harqInfoList, LOS, Distance);
        NS_LOG_DEBUG (this << " Computing the PSCCH BLER (2) ");
        tbStatsPSCCH2 = NistLtePhyErrorModel::GetV2VPscchBler (m_fadingModel,NistLtePhyErrorModel::SISO, (*itTb).second.mcs, SNR/std::pow(10,shadowingNLOSv/10),  harqInfoList, LOS, Distance);
      }
      if (m_random->GetValue () > tbStatsPSCCH1.tbler ? true : false || m_random->GetValue () > tbStatsPSCCH2.tbler ? true : false) // If one out of 2 PSCCH RBs is correct
      {
            bool tmp_collidedPssch;
	    NS_LOG_INFO("PSCCH Recovered Successfully!");
       	    tmp_collidedPssch = m_random->GetValue () > tbStats.tbler ? false : true;  // m_random is uniformly distributed between 0 and 1
            if (tmp_collidedPssch)
            {
                ++m_lostPKTs[itTb->first.m_rnti].propagationLosses;
                NS_LOG_INFO("PSSCH Not Recovered!");
            }
            else
            {
                NS_LOG_INFO("PSSCH Recovered Successfully!");
                ++m_lostPKTs[itTb->first.m_rnti].collisionLosses;
            }
      }
      else
      {
         NS_LOG_INFO("PSCCH Recovery Failed! Dropping the packet");
         ++m_lostPKTs[itTb->first.m_rnti].propagationLosses;
      }

    }
    itTb++;
  } 

  //Compute error for each expected Tb
  //expectedSlTbs_t::iterator itTb = m_expectedSlTbs.begin ();
  itTb = m_expectedSlTbs.begin ();  //Already declared in the previous block
//  std::map <NistSlTbId_t, uint32_t>::iterator itSinr;
  while (itTb!=m_expectedSlTbs.end ())
  {
      itSinr = expectedTbToSinrIndex.find ((*itTb).first);
      if ((m_dataErrorModelEnabled) && (m_rxPacketInfo.size () > 0) && (itSinr != expectedTbToSinrIndex.end())) // avoid to check for errors when there is no actual data transmitted
      {
          // retrieve HARQ info
          HarqProcessInfoList_t harqInfoList;
          if ((*itTb).second.ndi == 0)
          {
              harqInfoList = m_harqPhyModule->GetHarqProcessInfoSl ((*itTb).first.m_rnti, (*itTb).first.m_l1dst);
              NS_LOG_DEBUG (this << " Nb Retx=" << harqInfoList.size());
          }


          NS_LOG_DEBUG(this << " Time " << Simulator::Now ().GetSeconds () << "\tFrom: " << (*itTb).first.m_rnti << "\tCorrupt: " << (*itTb).second.corrupt);

          for (NodeContainer::Iterator L = GlobalContainer.Begin(); L != GlobalContainer.End(); ++L) 
          {
            TxNode = *L;
            if ( (*itTb).first.m_rnti == TxNode->GetId())
            {
              mobTX = TxNode->GetObject<MobilityModel>();
              posTX = mobTX->GetPosition(); 
              break;
            }
          }

	  Distance = std::sqrt(std::pow(posTX.x-posRX.x,2) + std::pow(posTX.y-posRX.y,2));
          shadowingNLOSv = randomNormal->GetValue (5 + std::max(0.0,(15*std::log10(Distance))-41), (sigmaNLOSv*sigmaNLOSv));// Due to the presence of other vehicles
          shadowingNLOSv = std::max(0.0,shadowingNLOSv);
	  if (Distance <= 475)
	  {
		Plos = std::min(1.0,2.1013e-6*Distance*Distance - 0.002*Distance + 1.0193);  // Probability of being in LOS in the Highway scenario (see 3GPP TR 37.885)
	  }
	  else
	  {
		Plos = std::max(0.0,0.54 - 0.001*(Distance-475));
	  }
	  LOS = m_random->GetValue () > Plos ? false : true;
          NS_LOG_DEBUG(this << " TX Node: " << TxNode->GetId() << " Position X = " << posTX.x << " Position Y = " << posTX.y << " Tx-Rx Distance " << Distance << " P(LOS) " << Plos << " LOS " << LOS);

	//  std::cin.get();
          bool rbDecoded = false;

          //------------- not active ------------------------------------------------------------------------------------------------------------------------------------------
          if (m_dropRbOnCollisionEnabled) // By default is not active
          {
              NS_LOG_DEBUG (this << " PSSCH DropOnCollisionEnabled: Labeling Corrupted TB");

              //Check if any of the RBs have been decoded
              nCollidedRBs = 0;
              uint32_t startCollidedRB = 0;
              for (std::vector<int>::iterator rbIt =  (*itTb).second.rbBitmap.begin (); rbIt != (*itTb).second.rbBitmap.end(); rbIt++)
              {   // NS_LOG_UNCOND("RB check: " << (*rbIt));

                   if (rbIt ==  (*itTb).second.rbBitmap.begin ())
                   {
                        startCollidedRB = (uint32_t) (*rbIt);
                        std::ofstream provaFile; 
                        provaFile.open("results/sidelink/collisions.txt", std::ios_base::app);
                        provaFile << Simulator::Now().GetSeconds() << "," << m_device->GetNode()->GetId() << "," << startCollidedRB << "\r\n";
                        provaFile.close();
                   }
                   if (rbDecodedBitmap.find (*rbIt) != rbDecodedBitmap.end ())
                   {
                        nCollidedRBs += 1;
                        NS_LOG_DEBUG( this << "\t" << *rbIt << " decoded, labeled as corrupted!");
		//	std::cin.get();
                        rbDecoded = true;
                        (*itTb).second.corrupt = true;
                        (*itTb).second.collidedPssch = true;
                        if (saveCollisions)
                        {
                     		std::ofstream collisionFile; 
                     		collisionFile.open("results/sidelink/collisionFile.unimo", std::ios_base::app);
		                collisionFile << Simulator::Now().GetSeconds() << "," << m_device->GetNode()->GetId() << "," << (*rbIt) << "\r\n";
                		collisionFile.close();
                        }
                        //break;
                   }
                   if (saveCollisions)
                   {
	                    std::ofstream detectedRBFile; 
      		            detectedRBFile.open("results/sidelink/detectedRBFile.unimo", std::ios_base::app);
                	    detectedRBFile << Simulator::Now().GetSeconds() << "," << m_device->GetNode()->GetId() << "," << (*rbIt) << "\r\n";
	                    detectedRBFile.close();
                   }

              } //end for (std::vector<int>::iterator rbIt =  (*itTb).second.rbBitmap.begin (); rbIt != (*itTb).second.rbBitmap.end(); rbIt++)
              
              if ((*itTb).second.collidedPssch)
              {
		  std::ofstream collisions;
                  collisions.open("results/sidelink/collisions.csv", std::ios_base::app);
                  collisions << Simulator::Now ().GetSeconds () << "," << m_device->GetNode()->GetId() << "\r\n";
                  collisions.close ();
              }
          } // if (m_dropRbOnCollisionEnabled)
          //----------- not active --------------------------------------------------------------------------------------------------------------------------------------------

          if (!m_nistErrorModelEnabled)
          {
               NistTbStats_t tbStats = NistLteMiErrorModel::GetTbDecodificationStats (m_slSinrPerceived[(*itSinr).second]*MIMOGain /* Average gain for SIMO based on [CatreuxMIMO] */, (*itTb).second.rbBitmap, (*itTb).second.size, (*itTb).second.mcs, harqInfoList);
               (*itTb).second.mi = tbStats.mi;
               if(m_slBlerEnabled)
               {
                    if(!rbDecoded)
                    {
                        (*itTb).second.corrupt = m_random->GetValue () > tbStats.tbler ? false : true;
                    }
               }
               NS_LOG_DEBUG (this << " from RNTI " << (*itTb).first.m_rnti << " size " << (*itTb).second.size << " mcs " << (uint32_t)(*itTb).second.mcs << " bitmap " << (*itTb).second.rbBitmap.size () << " TBLER " << tbStats.tbler << " corrupted " << (*itTb).second.corrupt << " corrupted TB " << (*itTb).second.collidedPssch);
          } 
          else if (!itTb->second.corrupt)// if NIST Error Model Enabled and the SCI is not already corrupted
          {   
              NS_LOG_DEBUG("Valid SCI");
              if (!itTb->second.collidedPssch)
              {
                 NS_LOG_DEBUG("Non-collided PSSCH");
                 double SNR;
                // bool PropagationError = false;
                 for (uint16_t i = 0; i < m_expectedSlTbSNR.size(); i++)
                 {
                    if (m_expectedSlTbSNR[i].first == (*itTb).first.m_rnti)
                       SNR = m_expectedSlTbSNR[i].second;
                 }
                 NS_LOG_DEBUG("SNR value is " << SNR);
                 NistTbErrorStats_t tbStats;
                 NistTbErrorStats_t tbStatsPSCCH1;
                 NistTbErrorStats_t tbStatsPSCCH2;
                 if (LOS)
	         {
                   NS_LOG_DEBUG (this << " Not already collided PSSCH. Computing the PSSCH BLER in LOS (without Interference) ");
              	   tbStats = NistLtePhyErrorModel::GetV2VPsschBler (m_fadingModel,NistLtePhyErrorModel::SISO, (*itTb).second.mcs, SNR,  harqInfoList, LOS, Distance, (*itTb).second.rbBitmap.size());
                   NS_LOG_DEBUG (this << " Computing the PSCCH BLER (1) ");
                   tbStatsPSCCH1 = NistLtePhyErrorModel::GetV2VPscchBler (m_fadingModel,NistLtePhyErrorModel::SISO, (*itTb).second.mcs, SNR,  harqInfoList, LOS, Distance);
                   NS_LOG_DEBUG (this << " Computing the PSCCH BLER (2) ");
                   tbStatsPSCCH2 = NistLtePhyErrorModel::GetV2VPscchBler (m_fadingModel,NistLtePhyErrorModel::SISO, (*itTb).second.mcs, SNR,  harqInfoList, LOS, Distance);
                 }
                 else
                 {
                   NS_LOG_DEBUG (this << " Not already collided PSSCH. Computing the PSSCH BLER in NLOSv (without Interference) ");
             	   tbStats = NistLtePhyErrorModel::GetV2VPsschBler (m_fadingModel,NistLtePhyErrorModel::SISO, (*itTb).second.mcs, SNR/std::pow(10,shadowingNLOSv/10),  harqInfoList, LOS, Distance, (*itTb).second.rbBitmap.size());
                   NS_LOG_DEBUG (this << " Computing the PSCCH BLER (1) ");
                   tbStatsPSCCH1 = NistLtePhyErrorModel::GetV2VPscchBler (m_fadingModel,NistLtePhyErrorModel::SISO, (*itTb).second.mcs, SNR/std::pow(10,shadowingNLOSv/10),  harqInfoList, LOS, Distance);
                   NS_LOG_DEBUG (this << " Computing the PSCCH BLER (2) ");
                   tbStatsPSCCH2 = NistLtePhyErrorModel::GetV2VPscchBler (m_fadingModel,NistLtePhyErrorModel::SISO, (*itTb).second.mcs, SNR/std::pow(10,shadowingNLOSv/10),  harqInfoList, LOS, Distance);
                 }
                 NS_LOG_DEBUG("TBLER PSSCH = " << tbStats.tbler << " TBLER PSCCH (1) = " << tbStatsPSCCH1.tbler << " TBLER PSCCH (2) = " << tbStatsPSCCH2.tbler);
                 if(m_slBlerEnabled) //By default is true
                 {
                   if (!rbDecoded) 
                   { //Before looking at the PSSCH I need to check that at least one PSCCH RB has been received successfully!
		      if (m_random->GetValue () > tbStatsPSCCH1.tbler ? true : false || m_random->GetValue () > tbStatsPSCCH2.tbler ? true : false) // If one out of 2 PSCCH RBs is correct
		      {
			    NS_LOG_INFO("PSCCH Recovered Successfully!");
                     	    (*itTb).second.collidedPssch = m_random->GetValue () > tbStats.tbler ? false : true;  // m_random is uniformly distributed between 0 and 1
                            if ((*itTb).second.collidedPssch)
                            {
                               ++m_lostPKTs[itTb->first.m_rnti].propagationLosses;
                               NS_LOG_INFO("PSSCH Not Recovered!");
                               NS_LOG_INFO("Propagation losses " << m_lostPKTs[itTb->first.m_rnti].propagationLosses);

                         //      std::cin.get();
                            }
                            else
                               NS_LOG_INFO("PSSCH Recovered Successfully!");
		      }
		      else
		      {
			    NS_LOG_INFO("PSCCH Recovery Failed! Dropping the packet");
                     	    (*itTb).second.corrupt = true; 
                            ++m_lostPKTs[itTb->first.m_rnti].propagationLosses;
                            NS_LOG_INFO("Propagation losses " << m_lostPKTs[itTb->first.m_rnti].propagationLosses);
                      }
                   }
                 }
                 // If it was not corrupted by the propagation (using the original SNR) and if there is some interference to evaluate
                 double SNR_new = 10*std::log10(GetLowestSinr (m_slSinrPerceived[(*itSinr).second], (*itTb).second.rbBitmap) );

                 if ( (!itTb->second.collidedPssch) && (!itTb->second.corrupt) && ( abs(SNR_new-10*std::log10(SNR)) > 0.001 ) )
                 {
                   if (LOS)
	           {
                     NS_LOG_DEBUG (this << " Not already collided PSSCH. Computing the PSSCH BLER in LOS (with Interference) ");
              	     tbStats = NistLtePhyErrorModel::GetV2VPsschBler (m_fadingModel,NistLtePhyErrorModel::SISO, (*itTb).second.mcs, GetLowestSinr (m_slSinrPerceived[(*itSinr).second], (*itTb).second.rbBitmap),  harqInfoList, LOS, Distance, (*itTb).second.rbBitmap.size());
                     NS_LOG_DEBUG (this << " Computing the PSCCH BLER (1) ");
                     tbStatsPSCCH1 = NistLtePhyErrorModel::GetV2VPscchBler (m_fadingModel,NistLtePhyErrorModel::SISO, (*itTb).second.mcs, GetMeanSinrPSCCH (m_slSinrPerceived[(*itSinr).second], (*itTb).second.rbBitmap, PSCCHlen),  harqInfoList, LOS, Distance);
                     NS_LOG_DEBUG (this << " Computing the PSCCH BLER (2) ");
                     tbStatsPSCCH2 = NistLtePhyErrorModel::GetV2VPscchBler (m_fadingModel,NistLtePhyErrorModel::SISO, (*itTb).second.mcs, GetMeanSinrPSCCH (m_slSinrPerceived[(*itSinr).second], (*itTb).second.rbBitmap, PSCCHlen),  harqInfoList, LOS, Distance);
                   }
                   else
                   {
                     NS_LOG_DEBUG (this << " Not already collided PSSCH. Computing the PSSCH BLER in NLOSv (with Interference) ");
             	     tbStats = NistLtePhyErrorModel::GetV2VPsschBler (m_fadingModel,NistLtePhyErrorModel::SISO, (*itTb).second.mcs, GetLowestSinr (m_slSinrPerceived[(*itSinr).second], (*itTb).second.rbBitmap)/std::pow(10,shadowingNLOSv/10),  harqInfoList, LOS, Distance, (*itTb).second.rbBitmap.size());
                     NS_LOG_DEBUG (this << " Computing the PSCCH BLER (1) ");
                     tbStatsPSCCH1 = NistLtePhyErrorModel::GetV2VPscchBler (m_fadingModel,NistLtePhyErrorModel::SISO, (*itTb).second.mcs, GetMeanSinrPSCCH (m_slSinrPerceived[(*itSinr).second], (*itTb).second.rbBitmap, PSCCHlen)/std::pow(10,shadowingNLOSv/10),  harqInfoList, LOS, Distance);
                     NS_LOG_DEBUG (this << " Computing the PSCCH BLER (2) ");
                     tbStatsPSCCH2 = NistLtePhyErrorModel::GetV2VPscchBler (m_fadingModel,NistLtePhyErrorModel::SISO, (*itTb).second.mcs, GetMeanSinrPSCCH (m_slSinrPerceived[(*itSinr).second], (*itTb).second.rbBitmap, PSCCHlen)/std::pow(10,shadowingNLOSv/10),  harqInfoList, LOS, Distance);
                   }
                   NS_LOG_DEBUG("TBLER PSSCH = " << tbStats.tbler << " TBLER PSCCH (1) = " << tbStatsPSCCH1.tbler << " TBLER PSCCH (2) = " << tbStatsPSCCH2.tbler);
                   if(m_slBlerEnabled) //By default is true
                   {
                     if (!rbDecoded) 
                     {   //Before looking at the PSSCH I need to check that at least one PSCCH RB has been received successfully!
		        if (m_random->GetValue () > tbStatsPSCCH1.tbler ? true : false || m_random->GetValue () > tbStatsPSCCH2.tbler ? true : false) // If one out of 2 PSCCH RBs is correct
		        {
			      NS_LOG_INFO("PSCCH Recovered Successfully!");
                     	      (*itTb).second.collidedPssch = m_random->GetValue () > tbStats.tbler ? false : true;  // m_random is uniformly distributed between 0 and 1
                              if ((*itTb).second.collidedPssch)
                              {
                                 ++m_lostPKTs[itTb->first.m_rnti].collisionLosses;
                                 NS_LOG_INFO("PSSCH Not Recovered!");
                                 NS_LOG_INFO("Collision losses " << m_lostPKTs[itTb->first.m_rnti].collisionLosses);
                              }
                              else
                                 NS_LOG_INFO("PSSCH Recovered Successfully!");
		        }
		        else
	        	{
			      NS_LOG_INFO("PSCCH Recovery Failed! Dropping the packet");
                     	      (*itTb).second.corrupt = true; 
                              ++m_lostPKTs[itTb->first.m_rnti].collisionLosses;
                              NS_LOG_INFO("Collision losses " << m_lostPKTs[itTb->first.m_rnti].collisionLosses);
                        }
                     }
                   }
                 } // end if ( (!itTb->second.collidedPssch) && (!itTb->second.corrupt) && ((uint16_t)SNR_new*10000 != (uint16_t)SNR*10000) ) //rounded at the 4th decimal place
              } // end if (!itTb->second.collidedPssch)
              else
              {
                 // if the TB is already corrupted check if at least the SCI can be recovered
                 NS_LOG_DEBUG("Collided PSSCH, decoding only the SCI");
                 NistTbErrorStats_t tbStatsPSCCH1;
                 NistTbErrorStats_t tbStatsPSCCH2;
                 if (LOS)
	         {
                   NS_LOG_DEBUG (this << " Computing the PSCCH BLER in LOS ");
                   NS_LOG_DEBUG (this << " Computing the PSCCH BLER (1) ");
                   tbStatsPSCCH1 = NistLtePhyErrorModel::GetV2VPscchBler (m_fadingModel,NistLtePhyErrorModel::SISO, (*itTb).second.mcs, GetMeanSinrPSCCH (m_slSinrPerceived[(*itSinr).second], (*itTb).second.rbBitmap, PSCCHlen),  harqInfoList, LOS, Distance);
                   NS_LOG_DEBUG (this << " Computing the PSCCH BLER (2) ");
                   tbStatsPSCCH2 = NistLtePhyErrorModel::GetV2VPscchBler (m_fadingModel,NistLtePhyErrorModel::SISO, (*itTb).second.mcs, GetMeanSinrPSCCH (m_slSinrPerceived[(*itSinr).second], (*itTb).second.rbBitmap, PSCCHlen),  harqInfoList, LOS, Distance);  
                   NS_LOG_DEBUG("TBLER PSCCH (1) = " << tbStatsPSCCH1.tbler << " TBLER PSCCH (2) = " << tbStatsPSCCH2.tbler);
                 }
                 else
                 {
                   NS_LOG_DEBUG (this << " Computing the PSCCH BLER in NLOSv ");
                   NS_LOG_DEBUG (this << " Computing the PSCCH BLER (1) ");
                   tbStatsPSCCH1 = NistLtePhyErrorModel::GetV2VPscchBler (m_fadingModel,NistLtePhyErrorModel::SISO, (*itTb).second.mcs, GetMeanSinrPSCCH (m_slSinrPerceived[(*itSinr).second], (*itTb).second.rbBitmap, PSCCHlen)/std::pow(10,shadowingNLOSv/10),  harqInfoList, LOS, Distance);
                   NS_LOG_DEBUG (this << " Computing the PSCCH BLER (2) ");
                   tbStatsPSCCH2 = NistLtePhyErrorModel::GetV2VPscchBler (m_fadingModel,NistLtePhyErrorModel::SISO, (*itTb).second.mcs, GetMeanSinrPSCCH (m_slSinrPerceived[(*itSinr).second], (*itTb).second.rbBitmap, PSCCHlen)/std::pow(10,shadowingNLOSv/10),  harqInfoList, LOS, Distance);
                   NS_LOG_DEBUG("TBLER PSCCH (1) = " << tbStatsPSCCH1.tbler << " TBLER PSCCH (2) = " << tbStatsPSCCH2.tbler);
                 }
                 if(m_slBlerEnabled) //By default is true
                 {
                   if (!rbDecoded) 
                   { //Before looking at the PSSCH I need to check that at least one PSCCH RB has been received successfully!
		      if (m_random->GetValue () > tbStatsPSCCH1.tbler ? true : false || m_random->GetValue () > tbStatsPSCCH2.tbler ? true : false) // If one out of 2 PSCCH RBs is correct
		      {
			    NS_LOG_INFO("PSCCH Recovered Successfully!");
		      }
		      else
		      {
			    NS_LOG_INFO("PSCCH Recovery Failed! Dropping the packet");
                     	    (*itTb).second.corrupt = true; 
                      }
                   }
                 }
                 NS_LOG_INFO (this << " from RNTI " << (*itTb).first.m_rnti << " size " << (*itTb).second.size << " mcs " << (uint32_t)(*itTb).second.mcs << " bitmap " << (*itTb).second.rbBitmap.size () << " corrupted SCI " << (*itTb).second.corrupt << " corrupted TB " << (*itTb).second.collidedPssch);
              } // end else (non-corrupted TB)
          } // end else if (!itTb->second.corrupt)
          else
          {
              NS_LOG_DEBUG("Non-valid SCI");
              (*itTb).second.collidedPssch = true; //then label as corrupted also the TB
          }

          // std::cin.get();
          //fire traces on SL reception PHY stats
          NistPhyReceptionStatParameters params;
          params.m_timestamp = Simulator::Now ().GetMilliSeconds ();
          params.m_cellId = m_cellId;
          params.m_imsi = 0; // it will be set by DlPhyTransmissionCallback in NistLteHelper
          params.m_rnti = (*itTb).first.m_rnti;
          params.m_txMode = m_transmissionMode;
          params.m_layer =  0;
          params.m_mcs = (*itTb).second.mcs;
          params.m_size = (*itTb).second.size;
          params.m_rv = (*itTb).second.rv;
          params.m_ndi = (*itTb).second.ndi;
     //     params.m_correctness = (uint8_t)!(*itTb).second.corrupt;
          params.m_correctness = (uint8_t)!(*itTb).second.collidedPssch;
          params.m_sinrPerRb = GetMeanSinr (m_slSinrPerceived[(*itSinr).second], (*itTb).second.rbBitmap); // Average only on the RBs used for data

          params.m_rv = harqInfoList.size ();
          m_slPhyReception (params);     
          NS_LOG_DEBUG("Fired traces on SL reception PHY stats");     
      } //end if ((m_dataErrorModelEnabled) && (m_rxPacketInfo.size () > 0) && (itSinr != expectedTbToSinrIndex.end())) 
      itTb++;
  } //end   while (itTb!=m_expectedSlTbs.end ())
//  std::cin.get();
  std::list<Ptr<NistLteControlMessage> > rxControlMessageOkList;
  for (uint32_t i = 0 ; i < m_rxPacketInfo.size() ; i++)
  {
      //even though there may be multiple packets, they all have
      //the same tag
      if (m_rxPacketInfo[i].m_rxPacketBurst) //if data packet
      {
          for (std::list<Ptr<Packet> >::const_iterator j = m_rxPacketInfo[i].m_rxPacketBurst->Begin (); j != m_rxPacketInfo[i].m_rxPacketBurst->End (); ++j)
          {
              // retrieve TB info of this packet 
              NistLteRadioBearerTag tag;
              (*j)->PeekPacketTag (tag);
              NistSlTbId_t tbId;
              tbId.m_rnti = tag.GetRnti ();
              tbId.m_l1dst = tag.GetDestinationL2Id () & 0xFF;
              itTb = m_expectedSlTbs.find (tbId);//find this among the expected transport blocks
              NS_LOG_INFO (this << " Packet of " << tbId.m_rnti << " group " <<  (uint16_t) tbId.m_l1dst);
              //NS_ASSERT (itTb!=m_expectedSlTbs.end ());
              if (itTb!=m_expectedSlTbs.end ())
              {
                  if (!(*itTb).second.corrupt)  
                  {
                      NS_LOG_DEBUG("SCI from " << tbId.m_rnti << " received successfully");
                      rxControlMessageOkList.push_back(m_rxPacketInfo[i].m_rxControlMessage);
                      if (!(*itTb).second.collidedPssch)
                      {  
                         m_phyRxEndOkTrace (*j);
                         if (!m_ltePhyRxDataEndOkCallback.IsNull ())
                         { 
                           m_ltePhyRxDataEndOkCallback (*j);
                           NS_LOG_DEBUG("TB from " << tbId.m_rnti << " received successfully");
                           ++m_lostPKTs[itTb->first.m_rnti].totalOK;
                         }
                      }
                      else
                      {
                       /*  if (!m_ltePhyRxDataEndOkCallback.IsNull ())
                         { 
                           m_ltePhyRxDataEndOkCallback (*j);
                         }*/
                         // TB received with errors or not decoded at all 
                         m_phyRxEndErrorTrace (*j);
                         NS_LOG_DEBUG("TB from " << tbId.m_rnti << " received with errors/not decoded");
                      }
                  }
                  else
                  {
                   /* if (!m_ltePhyRxDataEndOkCallback.IsNull ())
                    { 
                      m_ltePhyRxDataEndOkCallback (*j);
                    }*/
                    NS_LOG_DEBUG("SCI from " << tbId.m_rnti << " received with errors/not decoded");
                  }
                  //store HARQ information
                  if (!(*itTb).second.harqFeedbackSent)
                  {
                      (*itTb).second.harqFeedbackSent = true;
                      //because we do not have feedbacks we do not reset HARQ now.
                      //we will do it when we expect a new data
                      if ((*itTb).second.corrupt)
                      {
                          if (!m_nistErrorModelEnabled)
                          {
                              m_harqPhyModule->UpdateSlHarqProcessNistStatus (tbId.m_rnti, tbId.m_l1dst, (*itTb).second.mi, (*itTb).second.size, (*itTb).second.size / EffectiveCodingRate [(*itTb).second.mcs]);
                          }
                          else
                          {
                              m_harqPhyModule->UpdateSlHarqProcessNistStatus (tbId.m_rnti, tbId.m_l1dst, (*itTb).second.sinr);
                          }
                      }
                      else
                      {
                         //m_harqPhyModule->ResetSlHarqProcessNistStatus (tbId.m_rnti, tbId.m_l1dst);
                      }
                  } // end if (!(*itTb).second.harqFeedbackSent)
              } // end if (itTb!=m_expectedSlTbs.end ())
          } // end for (std::list<Ptr<Packet> >::const_iterator j = m_rxPacketInfo[i].m_rxPacketBurst->Begin (); j != m_rxPacketInfo[i].m_rxPacketBurst->End (); ++j)

          if(m_rxPacketInfo[i].m_rxControlMessage->GetMessageType () == NistLteControlMessage::SCI)
          {
            NS_LOG_DEBUG("Adding PSCCH trace");
            // Add PSCCH trace.
            NS_ASSERT(m_rxPacketInfo[i].m_rxControlMessage->GetMessageType () == NistLteControlMessage::SCI);
            Ptr<SciNistLteControlMessage> msg2 = DynamicCast<SciNistLteControlMessage> (m_rxPacketInfo[i].m_rxControlMessage);
            NistSciListElement_s sci = msg2->GetSci ();
            NistPhyReceptionStatParameters params;
            params.m_timestamp = Simulator::Now ().GetMilliSeconds ();
            params.m_cellId = m_cellId;
            params.m_imsi = 0; // it will be set by DlPhyTransmissionCallback in NistLteHelper
            params.m_rnti = sci.m_rnti;
            params.m_layer =  0;
            params.m_mcs = sci.m_mcs;
            params.m_size = sci.m_tbSize;
            params.m_rv = sci.m_rbStart;    // Using m_rv to store the RB start
            params.m_ndi = sci.m_rbLen;     // Using m_ndi to store the number of RBs used
            params.m_correctness = (uint8_t)(*itTb).second.corrupt;
            params.m_sinrPerRb = 0; // NOT USED, JUST INITIALIZED TO AVOID COMPILATION WARNING!
            params.m_txMode = m_transmissionMode; // NOT USED, JUST INITIALIZED TO AVOID COMPILATION WARNING!
            // Call trace
            m_slPscchReception (params);
          }
      } // end if (m_rxPacketInfo[i].m_rxPacketBurst) //if data packet
  } //end for (uint32_t i = 0 ; i < m_rxPacketInfo.size() ; i++)

//  NS_LOG_DEBUG("rxControlMessageOkList size " << rxControlMessageOkList_prova.size());
  if (rxControlMessageOkList.size() > 0)
  {
    if (!m_ltePhyRxCtrlEndOkCallback.IsNull ())
    {
       NS_LOG_DEBUG (this << " PSCCH OK"); 
       m_ltePhyRxCtrlEndOkCallback (rxControlMessageOkList); // Very important, do not delete
    }
  }
 /*  if (!m_ltePhyRxCtrlEndErrorCallback.IsNull ()) // Actually this callback is not set
  {
     NS_LOG_DEBUG (this << " PSCCH Error");
     m_ltePhyRxCtrlEndErrorCallback (); // Very important, do not delete
  }*/
 
  //print the collisions counters file
  uint32_t pkt_SUM = 0;
  for (std::map<uint32_t,CountersLosses>::iterator ITT = m_lostPKTs.begin(); ITT != m_lostPKTs.end(); ITT++)
  {
    pkt_SUM += ITT->second.totalOK;
    pkt_SUM += ITT->second.collisionLosses;
    pkt_SUM += ITT->second.propagationLosses;
  }
  NS_ASSERT_MSG(m_totalReceptions == pkt_SUM,"The counters sum does not match the number of total receptions!");

  if ((m_saveCollisionsUniMore) && (Simulator::Now ().GetSeconds () - m_prevPrintTime > 0.25))
  {
    m_prevPrintTime = Simulator::Now ().GetSeconds ();
    std::ofstream collisionCounters; 
    collisionCounters.open("results/sidelink/collisionCounters.txt", std::ios_base::app);
    for (std::map<uint32_t,CountersLosses>::iterator ITT = m_lostPKTs.begin(); ITT != m_lostPKTs.end(); ITT++)
    {
      for (NodeContainer::Iterator L = GlobalContainer.Begin(); L != GlobalContainer.End(); ++L) 
      {
        TxNode = *L;
        if ( ITT->first == TxNode->GetId())
        {
          mobTX = TxNode->GetObject<MobilityModel>();
          posTX = mobTX->GetPosition(); 
          break;
        }
      }
      collisionCounters << GetDevice()->GetNode()->GetId() /*RX node ID*/ << "," << Simulator::Now().GetSeconds() << "," << m_totalReceptions << "," << ITT->first /*TX node ID*/ << "," << ITT->second.totalOK << "," << ITT->second.collisionLosses << "," << ITT->second.propagationLosses << "," << posRX.x << "," << posRX.y << "," << posTX.x << "," << posTX.y << std::endl;
    }
    
    collisionCounters.close();

    // Reset the counters. This is mandatory for non-stationary UEs
    m_totalReceptions = 0;
    for (std::map<uint32_t,CountersLosses>::iterator ITT = m_lostPKTs.begin(); ITT != m_lostPKTs.end(); ITT++)
    {
      ITT->second.totalOK = 0;
      ITT->second.collisionLosses = 0;
      ITT->second.propagationLosses = 0;
    }

//     std::cin.get();
  }

//-------------------------- OLD IMPLEMENTATION ---------------------------------------------------

  /* Currently the MIB-SL is treated as a control message. Thus, the following logic applies also to the MIB-SL
   * The differences: calculation of BLER */
  // When control messages collide in the PSCCH, the receiver cannot know how many transmissions occured
  // we sort the messages by SINR and try to decode the ones with highest average SINR per RB first
  // only one message per RB can be decoded
  rxControlMessageOkList.clear();
  bool error = true; 
  bool ctrlMessageFound = false;
  std::multiset<NistSlCtrlPacketInfo_t> sortedControlMessages;
  rbDecodedBitmap.clear ();
  for (uint32_t i = 0 ; i < m_rxPacketInfo.size() ; i++)
  {
      NS_LOG_DEBUG("Sorting the received SCI");
      if (m_rxPacketInfo[i].m_rxControlMessage && m_rxPacketInfo[i].m_rxControlMessage->GetMessageType () != NistLteControlMessage::SL_DISC_MSG) //if control packet               
      { 
          NS_LOG_DEBUG("Sorting...");
          double meanSinr = GetMeanSinr (m_slSinrPerceived[i], m_rxPacketInfo[i].rbBitmap);  // Averages over all the used RBs
          NistSlCtrlPacketInfo_t pInfo;
          pInfo.sinr = meanSinr;          
          pInfo.index = i;
          sortedControlMessages.insert (pInfo);
      }
  }
  if (m_dropRbOnCollisionEnabled)
  {
      NS_LOG_DEBUG (this << "Ctrl DropOnCollisionEnabled");
      std::set<int> rbDecodedBitmapTemp;

      for (std::multiset<NistSlCtrlPacketInfo_t>::iterator it =sortedControlMessages.begin(); it != sortedControlMessages.end(); it++ )
      {
          int i = (*it).index;
          for (std::vector<int>::iterator rbIt =  m_rxPacketInfo[i].rbBitmap.begin (); rbIt != m_rxPacketInfo[i].rbBitmap.end(); rbIt++)
          { 
              if (rbDecodedBitmapTemp.find (*rbIt) != rbDecodedBitmapTemp.end()) 
              {
                  NS_LOG_DEBUG("collision, update the bitmap");
                  rbDecodedBitmapTemp.insert ( *rbIt);  
                  break;
              }
              else
              {
                  NS_LOG_DEBUG("store resources used by the packet to detect collision");
                  rbDecodedBitmapTemp.insert ( *rbIt);
              }
         }
      }
  }

  for (std::multiset<NistSlCtrlPacketInfo_t>::iterator it = sortedControlMessages.begin(); it != sortedControlMessages.end() ; it++ )
  {
      int i = (*it).index;
      bool ctrlError = false;
      ctrlMessageFound = true;
      if (m_ctrlErrorModelEnabled)
      {
          bool rbDecoded = false;
          for (std::vector<int>::iterator rbIt =  m_rxPacketInfo[i].rbBitmap.begin ();  rbIt != m_rxPacketInfo[i].rbBitmap.end() ; rbIt++)
          {
               if (rbDecodedBitmap.find (*rbIt) != rbDecodedBitmap.end()) 
               {
                   rbDecoded = true;
                   ctrlError = true;
                   if (saveCollisions)
                   {
                         std::ofstream collisionFile; 
                         collisionFile.open("results/sidelink/collisionFile.unimo", std::ios_base::app);
                         collisionFile << Simulator::Now().GetSeconds() << "," << m_device->GetNode()->GetId() << "," << (*rbIt) << "\r\n";
                         collisionFile.close();
                   }
               }
               else
               {

               }
               if (saveCollisions)
               {
                     std::ofstream detectedRBFile; 
                     detectedRBFile.open("results/sidelink/detectedRBFile.unimo", std::ios_base::app);
                     detectedRBFile << Simulator::Now().GetSeconds() << "," << m_device->GetNode()->GetId() << "," << (*rbIt) << "\r\n";
                     detectedRBFile.close();
               }
          }
          if (!rbDecoded)
          {
                double  errorRate;
                if ( m_rxPacketInfo[i].m_rxControlMessage->GetMessageType() == NistLteControlMessage::SCI)
                {
                      errorRate = NistLtePhyErrorModel::GetPscchBler (m_fadingModel,NistLtePhyErrorModel::SISO, GetMeanSinr (m_slSinrPerceived[i]*MIMOGain/correctionFactor, m_rxPacketInfo[i].rbBitmap)).tbler;
                      ctrlError = m_random->GetValue () > errorRate ? false : true;
                      NS_LOG_DEBUG (this << " PSCCH Decoding, errorRate " << errorRate << " error " << ctrlError);
                }
                else if (m_rxPacketInfo[i].m_rxControlMessage->GetMessageType() == NistLteControlMessage::MIB_SL)
                {
                      errorRate = NistLtePhyErrorModel::GetPsbchBler (m_fadingModel,NistLtePhyErrorModel::SISO, GetMeanSinr (m_slSinrPerceived[i]*MIMOGain/correctionFactor, m_rxPacketInfo[i].rbBitmap)).tbler;
                      ctrlError = m_random->GetValue () > errorRate ? false : true;
                      NS_LOG_DEBUG (this << " PSBCH Decoding, errorRate " << errorRate << " error " << ctrlError);
                }
                else
                {
                      NS_LOG_DEBUG (this << " Unknown SL control message");
                }
          }
      }
      if (!ctrlError)
      {
           error = false; 
           rxControlMessageOkList.push_back (m_rxPacketInfo[i].m_rxControlMessage);
           rbDecodedBitmap.insert ( m_rxPacketInfo[i].rbBitmap.begin(), m_rxPacketInfo[i].rbBitmap.end());
      }
      if(m_rxPacketInfo[i].m_rxControlMessage->GetMessageType () == NistLteControlMessage::SCI)
      {
          NS_LOG_DEBUG("Adding PSCCH trace");
          // Add PSCCH trace.
          NS_ASSERT(m_rxPacketInfo[i].m_rxControlMessage->GetMessageType () == NistLteControlMessage::SCI);
          Ptr<SciNistLteControlMessage> msg2 = DynamicCast<SciNistLteControlMessage> (m_rxPacketInfo[i].m_rxControlMessage);
          NistSciListElement_s sci = msg2->GetSci ();
          NistPhyReceptionStatParameters params;
          params.m_timestamp = Simulator::Now ().GetMilliSeconds ();
          params.m_cellId = m_cellId;
          params.m_imsi = 0; 
          params.m_rnti = sci.m_rnti;
          params.m_layer =  0;
          params.m_mcs = sci.m_mcs;
          params.m_size = sci.m_tbSize;
          params.m_rv = sci.m_rbStart;   
          params.m_ndi = sci.m_rbLen;    
          params.m_correctness = (uint8_t)!ctrlError;
          params.m_sinrPerRb = 0; 
          params.m_txMode = m_transmissionMode; 
          m_slPscchReception (params);
      }
  }
  if (ctrlMessageFound)
  { 
      if (!error)
      { 
          if (!m_ltePhyRxCtrlEndOkCallback.IsNull ())
          {
              NS_LOG_DEBUG (this << " PSCCH OK"); 
      //        m_ltePhyRxCtrlEndOkCallback (rxControlMessageOkList); 
          }
      }
      else
      {   
          if (!m_ltePhyRxCtrlEndErrorCallback.IsNull ()) 
          {
              NS_LOG_DEBUG (this << " PSCCH Error");
        //      m_ltePhyRxCtrlEndErrorCallback (); 
          }
      }
  }
//-------------------------- OLD IMPLEMENTATION ---------------------------------------------------

//  std::cin.get();


  //-------------------------------Discovery---------------------------------------------------------
  // error model and harq enabled for d2d discovery 
  if (m_errorModelHarqD2dDiscoveryEnabled)
  {
      std::map<NistDiscTbId_t, uint32_t> expectedTbToSinrDiscIndex;
      for (uint32_t i = 0 ; i < m_rxPacketInfo.size() ; i++)
      {
          //data isn't included and control is discovery message
          if (m_rxPacketInfo[i].m_rxControlMessage)
          {
              if (!m_rxPacketInfo[i].m_rxPacketBurst && m_rxPacketInfo[i].m_rxControlMessage->GetMessageType () == NistLteControlMessage::SL_DISC_MSG)
              {
                   Ptr<NistLteControlMessage> rxCtrlMsg= m_rxPacketInfo[i].m_rxControlMessage;
                   Ptr<NistSlDiscMessage> msg = DynamicCast<NistSlDiscMessage> (rxCtrlMsg);
                   NistSlDiscMsg disc = msg->GetSlDiscMessage ();
                   bool exist = FilterRxApps (disc);
                   if (exist)
                   {  
                       // retrieve TB info of this packet
                       NistDiscTbId_t tbId;
                       tbId.m_rnti = disc.m_rnti;
                       tbId.m_resPsdch = disc.m_resPsdch;
                       expectedTbToSinrDiscIndex.insert (std::pair<NistDiscTbId_t, uint8_t> (tbId, i));
                       std::list<Ptr<SidelinkDiscResourcePool> >::iterator discIt;
                       for (discIt = m_discRxPools.begin (); discIt != m_discRxPools.end(); discIt++)
                       {
                           std::list<SidelinkDiscResourcePool::SidelinkTransmissionInfo> m_psdchTx = (*discIt)->GetPsdchTransmissions (disc.m_resPsdch);
                           std::list<SidelinkDiscResourcePool::SidelinkTransmissionInfo>::iterator rxIt = m_psdchTx.begin ();
                           if (rxIt != m_psdchTx.end())
                           {
                                //reception
                                NS_LOG_INFO (this << " Expecting PSDCH reception RB " << (uint16_t) (disc.m_resPsdch));
                                std::list<SidelinkDiscResourcePool::SidelinkTransmissionInfo>::iterator txIt = m_psdchTx.begin ();
                                std::vector <int> rbMap;
                                for (int i = txIt->rbStart ; i < txIt->rbStart + txIt->nbRb ; i++)
                                {
                                       NS_LOG_LOGIC (this << " Receiving PSDCH on RB " << i);
                                       rbMap.push_back (i);
                                }
                                AddExpectedTb (disc.m_rnti, disc.m_resPsdch, m_psdchTx.size() % 4 == 0, rbMap, (4 - m_psdchTx.size () % 4));
                           }
                       }
                   }
              }
          }
      }
      //           
      expectedDiscTbs_t::iterator itTbDisc = m_expectedDiscTbs.begin ();
      std::map<NistDiscTbId_t, uint32_t>::iterator itSinrDisc;
      while (itTbDisc!= m_expectedDiscTbs.end ())
      {
          itSinrDisc = expectedTbToSinrDiscIndex.find ((*itTbDisc).first);
          //to check: m_ctrlErrorModelEnabled or should we have m_dataErrorModelEnabled enabled?
          if ((m_ctrlErrorModelEnabled)&&(m_rxPacketInfo.size ()>0)&&(itSinrDisc != expectedTbToSinrDiscIndex.end())) // avoid to check for errors when there is no actual discovery transmitted
            {   
              // retrieve HARQ info
              HarqProcessInfoList_t harqInfoList;
              if ((*itTbDisc).second.ndi == 0)
                {
                  harqInfoList = m_harqPhyModule->GetHarqProcessInfoDisc ((*itTbDisc).first.m_rnti,(*itTbDisc).first.m_resPsdch);
                  NS_LOG_DEBUG (this << " Nb Retx=" << harqInfoList.size());
                  //std::cout << this << " Nb Retx=" << harqInfoList.size() << std::endl;
                }
                           
              if (!m_nistErrorModelEnabled)
                {
                  NS_LOG_ERROR ("Any error model other than the NistErrorModel is not supported");
                } 
              else 
                {
                  NistTbErrorStats_t tbStats = NistLtePhyErrorModel::GetPsdchBler (m_fadingModel,NistLtePhyErrorModel::SISO, GetMeanSinr (m_slSinrPerceived[(*itSinrDisc).second]*MIMOGain /* Average gain for SIMO based on [CatreuxMIMO] */, (*itTbDisc).second.rbBitmap),  harqInfoList);
                  (*itTbDisc).second.sinr = tbStats.sinr;
                  (*itTbDisc).second.corrupt = m_random->GetValue () > tbStats.tbler ? false : true;
                  NS_LOG_DEBUG (this << " from RNTI " << (*itTbDisc).first.m_rnti << " TBLER " << tbStats.tbler << " corrupted " << (*itTbDisc).second.corrupt);
                  //std::cout << this << " from RNTI " << (*itTbDisc).first.m_rnti << " TBLER " << tbStats.tbler << " corrupted " << (*itTbDisc).second.corrupt << " mean SINR " << GetMeanSinr (m_slSinrPerceived[(*itSinrDisc).second]*4, (*itTbDisc).second.rbBitmap) << std::endl;
                }
                  
              //traces for discovery rx
              //we would know it is discovery mcs=0 and size=232
              NistPhyReceptionStatParameters params;
              params.m_timestamp = Simulator::Now ().GetMilliSeconds ();
              params.m_cellId = m_cellId;
              params.m_imsi = 0; // it will be set by DlPhyTransmissionCallback in NistLteHelper
              params.m_rnti = (*itTbDisc).first.m_rnti;
              params.m_txMode = m_transmissionMode;
              params.m_layer =  0;
              params.m_mcs = 0; //for discovery, we use a fixed modulation (no mcs defined), use 0 to identify discovery
              params.m_size = 232; // discovery message has a static size
              params.m_rv = (*itTbDisc).second.rv;
              params.m_ndi = (*itTbDisc).second.ndi;
              params.m_correctness = (uint8_t)!(*itTbDisc).second.corrupt;
              params.m_sinrPerRb = GetMeanSinr (m_slSinrPerceived[(*itSinrDisc).second]*4 /* Average gain for SIMO based on [CatreuxMIMO] */, (*itTbDisc).second.rbBitmap);
              params.m_rv = harqInfoList.size ();
              m_slPhyReception (params);  
            }
          itTbDisc++;
        }

      

      // handling collison for discovery
      // same as sidelink control but taking into account HARQ
      std::list<Ptr<NistLteControlMessage> > rxDiscMessageOkList;
      
      bool discError = true; 
      bool discMessageFound = false;
      std::set<NistSlCtrlPacketInfo_t> sortedDiscMessages;
      std::set<int> discDecodedBitmap;

      for (uint32_t i = 0 ; i < m_rxPacketInfo.size() ; i++)
        {
          Ptr<NistLteControlMessage> rxCtrlMsg= m_rxPacketInfo[i].m_rxControlMessage;
          Ptr<NistSlDiscMessage> msg = DynamicCast<NistSlDiscMessage> (rxCtrlMsg);
          if (msg)
          {
            NistSlDiscMsg disc = msg->GetSlDiscMessage ();
            bool exist = FilterRxApps (disc);
            if (exist)
            {
              if (m_rxPacketInfo[i].m_rxControlMessage->GetMessageType () == NistLteControlMessage::SL_DISC_MSG) //if discovery message     
                {
                  double meanSinr = GetMeanSinr (m_slSinrPerceived[i], m_rxPacketInfo[i].rbBitmap);
                  NistSlCtrlPacketInfo_t pInfo;
                  pInfo.sinr = meanSinr;          
                  pInfo.index = i;
                  sortedDiscMessages.insert (pInfo);
                }
            }
          }
        }
     
      // check all sorted discovery messages
      for (std::set<NistSlCtrlPacketInfo_t>::iterator it =sortedDiscMessages.begin(); it != sortedDiscMessages.end() ; it++ )
        {
          discMessageFound = true;
          // retrieve TB info of this packet
          int i = (*it).index;
          Ptr<NistLteControlMessage> rxCtrlMsg= m_rxPacketInfo[i].m_rxControlMessage;
          Ptr<NistSlDiscMessage> msg = DynamicCast<NistSlDiscMessage> (rxCtrlMsg);
          NistSlDiscMsg disc = msg->GetSlDiscMessage ();
          NistDiscTbId_t tbId;
          tbId.m_rnti = disc.m_rnti;
          tbId.m_resPsdch = disc.m_resPsdch;
          
          itTbDisc =  m_expectedDiscTbs.find (tbId);
          NS_LOG_INFO (this << " Packet of " << tbId.m_rnti << " resPsdch " <<  (uint16_t) tbId.m_resPsdch); 

          bool rbDecoded = false;
          for (std::vector<int>::iterator rbIt =  m_rxPacketInfo[i].rbBitmap.begin ();  rbIt != m_rxPacketInfo[i].rbBitmap.end() ; rbIt++)
            {
              if (discDecodedBitmap.find (*rbIt) != discDecodedBitmap.end()) 
              {
                rbDecoded = true;
                NS_LOG_DEBUG (this << " RB " << *rbIt << " has already been decoded ");
                break;
              }
            }

          if (itTbDisc!=m_expectedDiscTbs.end () && !rbDecoded)
          {
            if (!(*itTbDisc).second.corrupt)
            {
              discError = false; 
              rxDiscMessageOkList.push_back (m_rxPacketInfo[i].m_rxControlMessage);
              discDecodedBitmap.insert ( m_rxPacketInfo[i].rbBitmap.begin(), m_rxPacketInfo[i].rbBitmap.end());
                        
              //to add theta and sinr to NistDiscTxProbabilityModule
              //assuming we have only one pool
              Ptr<NistSlDiscMessage> msg = DynamicCast<NistSlDiscMessage> (m_rxPacketInfo[i].m_rxControlMessage);
              NistSlDiscMsg disc = msg->GetSlDiscMessage ();
              NS_LOG_DEBUG (this << " from RNTI " << disc.m_rnti << " ProSeAppCode " << disc.m_proSeAppCode.to_ulong () << " SINR " << (*it).sinr);
            }

            //store HARQ information
            if (!(*itTbDisc).second.harqFeedbackSent)
            {
              (*itTbDisc).second.harqFeedbackSent = true;
              //because we do not have feedbacks we do not reset HARQ now.
              //we will do it when we expect a new data
              if ((*itTbDisc).second.corrupt)
              {
                if (!m_nistErrorModelEnabled)
                {
                  NS_LOG_ERROR ("Any error model other than the NistErrorModel is not supported");
                }
                else
                {
                  m_harqPhyModule->UpdateDiscHarqProcessNistStatus (tbId.m_rnti, tbId.m_resPsdch, (*itTbDisc).second.sinr);
                }
              }
            }
          }     
        }

      if (discMessageFound)
      {        
        if (!discError)
        {
          if (!m_ltePhyRxCtrlEndOkCallback.IsNull ())
          {
            NS_LOG_DEBUG (this << " Discovery OK");
            m_ltePhyRxCtrlEndOkCallback (rxDiscMessageOkList);
          }
        }
        else
        {
          if (!m_ltePhyRxCtrlEndErrorCallback.IsNull ())
          {
            NS_LOG_DEBUG (this << " Discovery Error");
            m_ltePhyRxCtrlEndErrorCallback ();
          }
        }
      }
    }
      
    //error model and harq not enabled for d2d discovery
    //discard colliding packets for discovery
    else
    {
      for (uint32_t i = 0 ; i < m_rxPacketInfo.size() ; i++)
        { 
          if (m_rxPacketInfo[i].m_rxControlMessage) //if control packet               
            {
              if (!m_rxPacketInfo[i].m_rxPacketBurst && m_rxPacketInfo[i].m_rxControlMessage->GetMessageType () == NistLteControlMessage::SL_DISC_MSG)
              {
                Ptr<NistLteControlMessage> rxCtrlMsg= m_rxPacketInfo[i].m_rxControlMessage;
                Ptr<NistSlDiscMessage> msg = DynamicCast<NistSlDiscMessage> (rxCtrlMsg);
                NistSlDiscMsg disc = msg->GetSlDiscMessage ();
                bool exist = FilterRxApps (disc);
                if (exist)
                {
                  double meanSinr = GetMeanSinr (m_slSinrPerceived[i], m_rxPacketInfo[i].rbBitmap);
                  NistSlCtrlPacketInfo_t pInfo;
                  pInfo.sinr = meanSinr;          
                  pInfo.index = i;
                  sortedControlMessages.insert (pInfo);
                }
              }
            }
        }
       
      NS_ASSERT (m_slSinrPerceived.size ()>0);
      uint32_t countRb=0;
      for (Values::iterator vit = m_slSinrPerceived[0].ValuesBegin (); vit != m_slSinrPerceived[0].ValuesEnd (); ++vit)
      {
        countRb++;
      }

      std::vector<uint32_t> rbsUsed (countRb, 0);
      for (uint32_t i = 0; i < m_rxPacketInfo.size (); i++) 
        {
          for (std::vector<int>::iterator rbIt =  m_rxPacketInfo[i].rbBitmap.begin ();  rbIt != m_rxPacketInfo[i].rbBitmap.end() ; rbIt++)
            {
              rbsUsed[*rbIt]++;
            }
        }

     
      for (std::set<NistSlCtrlPacketInfo_t>::iterator it =sortedControlMessages.begin(); it != sortedControlMessages.end() ; it++ )
        {
          int i = (*it).index;

          bool ctrlError = false;
          ctrlMessageFound = true;
          
          if (m_ctrlErrorModelEnabled)
            {          
              bool rbDecoded = false;
              for (std::vector<int>::iterator rbIt =  m_rxPacketInfo[i].rbBitmap.begin ();  rbIt != m_rxPacketInfo[i].rbBitmap.end() ; rbIt++)
                {
                  if (rbDecodedBitmap.find (*rbIt) != rbDecodedBitmap.end()) 
                    {
                      rbDecoded = true;
                      NS_LOG_DEBUG (this << " RB " << *rbIt << " has already been decoded ");
                      break;
                    }
                }
              if (!rbDecoded)
                {
                  bool ok = true;
                  for (std::vector<int>::iterator rbIt =  m_rxPacketInfo[i].rbBitmap.begin ();  rbIt != m_rxPacketInfo[i].rbBitmap.end() ; rbIt++)
                    {
                      if (rbsUsed [*rbIt] > 1) 
                        {
                          ok = false;
                          break;
                        }
                    }
                  if (ok)
                    {
                      double  errorRate = NistLtePhyErrorModel::GetPscchBler (m_fadingModel,NistLtePhyErrorModel::SISO, GetMeanSinr (m_slSinrPerceived[i]*MIMOGain /* Average gain for SIMO based on [CatreuxMIMO] */, m_rxPacketInfo[i].rbBitmap)).tbler;
                      ctrlError = m_random->GetValue () > errorRate ? false : true;
                      NS_LOG_DEBUG (this << " Discovery Decodification, errorRate " << errorRate << " error " << ctrlError);
                    } 
                  else
                    {
                      ctrlError = true;
                    }
                }
              else {
                ctrlError = true;
              }
            }
          
          if (!ctrlError)
            {
              error = false; //at least one control packet is OK
              rxControlMessageOkList.push_back (m_rxPacketInfo[i].m_rxControlMessage);
              rbDecodedBitmap.insert ( m_rxPacketInfo[i].rbBitmap.begin(), m_rxPacketInfo[i].rbBitmap.end());

              Ptr<NistSlDiscMessage> msg = DynamicCast<NistSlDiscMessage> (m_rxPacketInfo[i].m_rxControlMessage);
              NistSlDiscMsg disc = msg->GetSlDiscMessage ();
              NS_LOG_DEBUG (this << " from RNTI " << disc.m_rnti << " ProSeAppCode " << disc.m_proSeAppCode.to_ulong () << " SINR " << (*it).sinr);
            } 
        }

        if (ctrlMessageFound)
        {
          if (!error)
            {
              if (!m_ltePhyRxCtrlEndOkCallback.IsNull ())
                {
                  NS_LOG_DEBUG (this << "Discovery OK");
                  m_ltePhyRxCtrlEndOkCallback (rxControlMessageOkList);
                }
            }
          else
            {
              if (!m_ltePhyRxCtrlEndErrorCallback.IsNull ())
                {
                  NS_LOG_DEBUG (this << "Discovery Error");
                  m_ltePhyRxCtrlEndErrorCallback ();
                }
            }
        }    
    }



  //done with sidelink data, control and discovery
  ChangeState (IDLE);
  m_rxPacketBurstList.clear ();
  m_rxControlMessageList.clear ();
  //m_rxControlMessageRbMap.clear ();
  m_rxPacketInfo.clear ();
  m_expectedSlTbs.clear ();
  m_expectedDiscTbs.clear ();
  m_expectedSlTbSNR.clear();

//  std::cin.get();
}

void
NistLteSpectrumPhy::EndRxDlCtrl ()
{
  NS_LOG_FUNCTION (this);
  NS_LOG_LOGIC (this << " state: " << m_state);
  
  NS_ASSERT (m_state == RX_CTRL);
  
  // this will trigger CQI calculation and Error Model evaluation
  // as a side effect, the error model should update the error status of all TBs
  m_interferenceCtrl->EndRx ();
  // apply transmission mode gain
  NS_LOG_DEBUG (this << " txMode " << (uint16_t)m_transmissionMode << " gain " << m_txModeGain.at (m_transmissionMode));
  NS_ASSERT (m_transmissionMode < m_txModeGain.size ());
  if (m_transmissionMode>0)
    {
      // in case of MIMO, ctrl is always txed as TX diversity
      m_sinrPerceived *= m_txModeGain.at (1);
    }
//   m_sinrPerceived *= m_txModeGain.at (m_transmissionMode);
  bool error = false;
  if (m_ctrlErrorModelEnabled)
    {
      double  errorRate = NistLteMiErrorModel::GetPcfichPdcchError (m_sinrPerceived);
      errorRate = NistLteMiErrorModel::GetPcfichPdcchError (m_sinrPerceived);
      error = m_random->GetValue () > errorRate ? false : true;
      NS_LOG_DEBUG (this << " PCFICH-PDCCH Decodification, errorRate " << errorRate << " error " << error);
    }

  if (!error)
    {
      if (!m_ltePhyRxCtrlEndOkCallback.IsNull ())
        {
          NS_LOG_DEBUG (this << " PCFICH-PDCCH Rxed OK");
          m_ltePhyRxCtrlEndOkCallback (m_rxControlMessageList);
        }
    }
  else
    {
      if (!m_ltePhyRxCtrlEndErrorCallback.IsNull ())
        {
          NS_LOG_DEBUG (this << " PCFICH-PDCCH Error");
          m_ltePhyRxCtrlEndErrorCallback ();
        }
    }
  ChangeState (IDLE);
  m_rxControlMessageList.clear ();
}
  
void
NistLteSpectrumPhy::EndRxUlSrs ()
{
  NS_ASSERT (m_state == RX_CTRL);
  ChangeState (IDLE);
  m_interferenceCtrl->EndRx ();
  // nothing to do (used only for SRS at this stage)
}

void 
NistLteSpectrumPhy::SetCellId (uint16_t cellId)
{
  m_cellId = cellId;
}

void 
NistLteSpectrumPhy::AddL1GroupId (uint8_t groupId)
{
  NS_LOG_FUNCTION (this << (uint16_t) groupId);
  m_l1GroupIds.insert(groupId);
}

void 
NistLteSpectrumPhy::RemoveL1GroupId (uint8_t groupId)
{
  m_l1GroupIds.erase (groupId);
}

void
NistLteSpectrumPhy::AddRsPowerChunkProcessor (Ptr<NistLteChunkProcessor> p)
{
  m_interferenceCtrl->AddRsPowerChunkProcessor (p);
}

void
NistLteSpectrumPhy::AddDataPowerChunkProcessor (Ptr<NistLteChunkProcessor> p)
{
  m_interferenceData->AddRsPowerChunkProcessor (p);
}

void
NistLteSpectrumPhy::AddDataSinrChunkProcessor (Ptr<NistLteChunkProcessor> p)
{
  m_interferenceData->AddSinrChunkProcessor (p);
}

void
NistLteSpectrumPhy::AddInterferenceCtrlChunkProcessor (Ptr<NistLteChunkProcessor> p)
{
  m_interferenceCtrl->AddInterferenceChunkProcessor (p);
}

void
NistLteSpectrumPhy::AddInterferenceDataChunkProcessor (Ptr<NistLteChunkProcessor> p)
{
  m_interferenceData->AddInterferenceChunkProcessor (p);
}

void
NistLteSpectrumPhy::AddCtrlSinrChunkProcessor (Ptr<NistLteChunkProcessor> p)
{
  m_interferenceCtrl->AddSinrChunkProcessor (p);
}

void
NistLteSpectrumPhy::AddSlSinrChunkProcessor (Ptr<NistLteSlChunkProcessor> p)
{
  m_interferenceSl->AddSinrChunkProcessor (p);
}

void
NistLteSpectrumPhy::AddSlSignalChunkProcessor (Ptr<NistLteSlChunkProcessor> p)
{
  m_interferenceSl->AddRsPowerChunkProcessor (p);
}

void
NistLteSpectrumPhy::AddSlInterferenceChunkProcessor (Ptr<NistLteSlChunkProcessor> p)
{
  m_interferenceSl->AddInterferenceChunkProcessor (p);
}

void 
NistLteSpectrumPhy::SetTransmissionMode (uint8_t txMode)
{
  NS_LOG_FUNCTION (this << (uint16_t) txMode);
  NS_ASSERT_MSG (txMode < m_txModeGain.size (), "TransmissionMode not available: 1.." << m_txModeGain.size ());
  m_transmissionMode = txMode;
  m_layersNum = NistTransmissionModesLayers::TxMode2LayerNum (txMode);
}


void 
NistLteSpectrumPhy::SetTxModeGain (uint8_t txMode, double gain)
{
  NS_LOG_FUNCTION (this << " txmode " << (uint16_t)txMode << " gain " << gain);
  // convert to linear
  gain = std::pow (10.0, (gain / 10.0));
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
      m_txModeGain.push_back (gain);
    }
    else
    {
      m_txModeGain.push_back (temp.at (i));
    }
  }
}

int64_t
NistLteSpectrumPhy::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  m_random->SetStream (stream);
  return 1;
}

double 
NistLteSpectrumPhy::GetLowestSinr (const SpectrumValue& sinr, const std::vector<int>& map)
{
  SpectrumValue sinrCopy = sinr;
  double sinrLin = sinrCopy[map.at (0)];
  for (uint32_t i = 1; i < map.size (); i++)
    { 
   //   NS_LOG_DEBUG("Sinr value: " << sinrCopy[map.at (i)] << " at " << i);
      if (sinrCopy[map.at (i)] < sinrLin)
         sinrLin = sinrCopy[map.at (i)];
    }
  NS_LOG_DEBUG("Lowest SINR is " << sinrLin);

  return sinrLin;
}

double 
NistLteSpectrumPhy::GetMeanSinr (const SpectrumValue& sinr, const std::vector<int>& map)
{
  SpectrumValue sinrCopy = sinr;
  double sinrLin = 0;
  for (uint32_t i = 0; i < map.size (); i++)
    { 
      sinrLin += sinrCopy[map.at (i)];
    }
  NS_LOG_DEBUG("SINR = " << sinrLin << " AVG SINR " << sinrLin / map.size());
  return sinrLin / map.size();
}

double 
NistLteSpectrumPhy::GetMeanSinrPSCCH (const SpectrumValue& sinr, const std::vector<int>& map, int LenPSCCH)
{
  SpectrumValue sinrCopy = sinr;
//  NS_LOG_DEBUG("Sinr = " << sinr << " Length = " << LenPSCCH);
  double sinrLin = 0;
  uint32_t i = 0;
  sinrLin += sinrCopy[map.at (i) - LenPSCCH];
  sinrLin += sinrCopy[map.at (i) - LenPSCCH + 1];
  sinrLin /= LenPSCCH;
    
  NS_LOG_DEBUG("SINR = " << sinrLin << " AVG SINR " << sinrLin << " AVG SINR in dB " << 10*std::log10(sinrLin));

  return sinrLin;
}


int
NistLteSpectrumPhy::UnimoreCompareSinrPSSCH (const SpectrumValue& first_sinr, const std::vector<int>& first_map,const SpectrumValue& second_sinr, const std::vector<int>& second_map)
{
  SpectrumValue sinrCopy_first = first_sinr;
  SpectrumValue sinrCopy_second = second_sinr;
  std::vector<int> overlappedRBs;
  int checkRB;

  for (uint16_t i = 0; i < first_map.size (); i++)
  {
    checkRB = first_map.at(i);
    for (uint16_t j = 0; j < second_map.size (); j++)
    {
       if (second_map.at(j) == checkRB)
         overlappedRBs.push_back(checkRB);
    } 
  }

  if (overlappedRBs.size() == 0)
    return 0;
  else if (GetMeanSinr(sinrCopy_first, overlappedRBs) > GetMeanSinr(sinrCopy_second, overlappedRBs))
    return 1;
  else
    return 2;

  //Print the PSSCH RBs
/*  NS_LOG_DEBUG("First UE, startRB = " << first_startRB << ", stopRB = " << first_stopRB);
  for (uint16_t i = 0; i < first_map.size (); i++)
  {
     NS_LOG_DEBUG("RB " << first_map.at(i) << " SINR " << sinrCopy_first[first_map.at(i)]);
  }
  NS_LOG_DEBUG("Second UE, startRB = " << second_startRB << ", stopRB = " << second_stopRB);
  for (uint16_t i = 0; i < second_map.size (); i++)
  {
     NS_LOG_DEBUG("RB " << second_map.at(i) << " SINR " << sinrCopy_second[second_map.at(i)]);
  } */

}


NistLteSpectrumPhy::State
NistLteSpectrumPhy::GetState ()
{
  return m_state;
}

void
NistLteSpectrumPhy::SetSlssid (uint64_t slssid)
{
  NS_LOG_FUNCTION (this);
  m_slssId = slssid;
}

void
NistLteSpectrumPhy::SetNistLtePhyRxSlssCallback (NistLtePhyRxSlssCallback c)
{
  NS_LOG_FUNCTION (this);
  m_ltePhyRxSlssCallback = c;
}

void 
NistLteSpectrumPhy::SetRxPool (Ptr<SidelinkDiscResourcePool> newpool)
{
  m_discRxPools.push_back (newpool);
}

// NEW: Set the receiver sensitivity

void 
NistLteSpectrumPhy::SetRxSensitivity (double sensitivity)
{
  m_rxSensitivity = sensitivity;
}

void 
NistLteSpectrumPhy::AddDiscTxApps (std::list<uint32_t> apps)
{
    m_discTxApps = apps;
}

void 
NistLteSpectrumPhy::AddDiscRxApps (std::list<uint32_t> apps)
{
  m_discRxApps = apps;
}

bool 
NistLteSpectrumPhy::FilterRxApps (NistSlDiscMsg disc)
{
  NS_LOG_FUNCTION (this << disc.m_proSeAppCode);
  bool exist = false;
  for (std::list<uint32_t>::iterator it = m_discRxApps.begin (); it != m_discRxApps.end (); ++it)
  {
    //std::cout << "app=" << *it  << std::endl;
    if ((std::bitset <184>)*it == disc.m_proSeAppCode)
    {
      exist = true;
    }
  }

  return exist;
}

void
NistLteSpectrumPhy::SetDiscNumRetx (uint8_t retx)
{
  NS_LOG_FUNCTION (this << retx);
  m_harqPhyModule->SetDiscNumRetx (retx);
}

} // namespace ns3
