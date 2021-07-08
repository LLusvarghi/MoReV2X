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
#include "nr-v2x-spectrum-phy.h"
#include "nist-lte-spectrum-signal-parameters.h"
#include "nist-lte-net-device.h"
#include "nist-lte-radio-bearer-tag.h"
#include "nist-lte-chunk-processor.h"
#include "nist-lte-sl-chunk-processor.h"
#include <ns3/nist-lte-radio-bearer-tag.h>
#include <ns3/boolean.h>
#include <ns3/double.h>
#include <ns3/config.h>
#include <ns3/node.h>
#include <ns3/string.h>
#include "ns3/enum.h"
#include <ns3/pointer.h>
#include <ns3/object-vector.h>
#include <ns3/node-container.h>
#include <iostream>
#include <fstream>
#include <algorithm>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("NrV2XSpectrumPhy");

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

  
NS_OBJECT_ENSURE_REGISTERED (NrV2XSpectrumPhy);

NrV2XSpectrumPhy::NrV2XSpectrumPhy ()
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


NrV2XSpectrumPhy::~NrV2XSpectrumPhy ()
{
  NS_LOG_FUNCTION (this);
  m_expectedTbs.clear ();
  m_expectedSlTbs.clear ();
  m_expectedSlTbSNR.clear();
  m_txModeGain.clear ();
}

void NrV2XSpectrumPhy::DoDispose ()
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
  m_RssiCallback = MakeNullCallback < void, double, std::vector <int>, uint16_t>();  // TODO FIXME New for V2V

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

std::ostream& operator<< (std::ostream& os, NrV2XSpectrumPhy::State s)
{
  switch (s)
    {
    case NrV2XSpectrumPhy::IDLE:
      os << "IDLE";
      break;
    case NrV2XSpectrumPhy::RX_DATA:
      os << "RX_DATA";
      break;
    case NrV2XSpectrumPhy::RX_CTRL:
      os << "RX_CTRL";
      break;
    case NrV2XSpectrumPhy::TX:
      os << "TX";
      break;
    default:
      os << "UNKNOWN";
      break;
    }
  return os;
}

TypeId
NrV2XSpectrumPhy::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::NrV2XSpectrumPhy")
    .SetParent<SpectrumPhy> ()
    .AddTraceSource ("TxStart",
                     "Trace fired when a new transmission is started",
                     MakeTraceSourceAccessor (&NrV2XSpectrumPhy::m_phyTxStartTrace),
                     "ns3::PacketBurst::TracedCallback")
    .AddTraceSource ("TxEnd",
                     "Trace fired when a previosuly started transmission is finished",
                     MakeTraceSourceAccessor (&NrV2XSpectrumPhy::m_phyTxEndTrace),
                     "ns3::PacketBurst::TracedCallback")
    .AddTraceSource ("RxStart",
                     "Trace fired when the start of a signal is detected",
                     MakeTraceSourceAccessor (&NrV2XSpectrumPhy::m_phyRxStartTrace),
                     "ns3::PacketBurst::TracedCallback")
    .AddTraceSource ("RxEndOk",
                     "Trace fired when a previosuly started RX terminates successfully",
                     MakeTraceSourceAccessor (&NrV2XSpectrumPhy::m_phyRxEndOkTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("RxEndError",
                     "Trace fired when a previosuly started RX terminates with an error",
                     MakeTraceSourceAccessor (&NrV2XSpectrumPhy::m_phyRxEndErrorTrace),
                     "ns3::Packet::TracedCallback")
    .AddAttribute ("DataErrorModelEnabled",
                    "Activate/Deactivate the error model of data (TBs of PDSCH and PUSCH) [by default is active].",
                    BooleanValue (true),
                   MakeBooleanAccessor (&NrV2XSpectrumPhy::m_dataErrorModelEnabled),
                    MakeBooleanChecker ())
    .AddAttribute ("CtrlErrorModelEnabled",
                    "Activate/Deactivate the error model of control (PCFICH-PDCCH decodification) [by default is active].",
                    BooleanValue (true),
                    MakeBooleanAccessor (&NrV2XSpectrumPhy::m_ctrlErrorModelEnabled),
                    MakeBooleanChecker ())
    .AddTraceSource ("SlPhyReception",
                     "SL reception PHY layer statistics.",
                     MakeTraceSourceAccessor (&NrV2XSpectrumPhy::m_slPhyReception),
                     "ns3::NistPhyReceptionStatParameters::TracedCallback")
    .AddTraceSource ("SlPscchReception",
                     "SL reception PCCH PHY layer statistics.",
                     MakeTraceSourceAccessor (&NrV2XSpectrumPhy::m_slPscchReception),
                     "ns3::NistPhyReceptionStatParameters::TracedCallback")
    .AddAttribute ("HalfDuplexPhy",
                   "a pointer to a spectrum phy object",
                   PointerValue (),
                   MakePointerAccessor (&NrV2XSpectrumPhy::m_halfDuplexPhy),
                   MakePointerChecker <NrV2XSpectrumPhy> ())
    .AddAttribute ("ChannelMatrix", 
                   "Matrix with 3GPP channel models",
                   PointerValue (),
                   MakePointerAccessor (&NrV2XSpectrumPhy::m_channelModels),
                   MakePointerChecker<NrV2XPropagationLossModel> ())
    .AddAttribute ("CtrlFullDuplexEnabled",
                    "Activate/Deactivate the full duplex in the PSCCH [by default is disable].",
                    BooleanValue (false),
                    MakeBooleanAccessor (&NrV2XSpectrumPhy::m_ctrlFullDuplexEnabled),
                    MakeBooleanChecker ())
    .AddAttribute ("SaveCollisionLossesUnimore", 
                   "Enable collision and propagation lossess saving",
                   BooleanValue (false), 
                   MakeBooleanAccessor (&NrV2XSpectrumPhy::m_saveCollisionsUniMore),
                   MakeBooleanChecker ())
    .AddAttribute ("SubchannelSize",
	           "The Subchannel size (in RBs)",
		   UintegerValue (10),
	           MakeUintegerAccessor (&NrV2XSpectrumPhy::m_subChSize),
		   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("RBsBandwidth",
	           "The bandwidth at Spectrum layer (in RBs)",
		   UintegerValue (50),
		   MakeUintegerAccessor (&NrV2XSpectrumPhy::m_BW_RBs),
		   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("SlotDuration",
	           "The NR-V2X time slot duration",
		   DoubleValue (1.0),
		   MakeDoubleAccessor (&NrV2XSpectrumPhy::m_slotDuration),
		   MakeDoubleChecker<double> ())
    .AddAttribute ("SubCarrierSpacing",
	           "The NR-V2X SubCarrier Spacing (SCS)",
		   UintegerValue (15),
		   MakeUintegerAccessor (&NrV2XSpectrumPhy::m_SCS),
		   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("ReferenceSensitivity",
	           "The Reference Sensitivity",
		   DoubleValue (-92.5),
		   MakeDoubleAccessor (&NrV2XSpectrumPhy::m_rxSensitivity),
		   MakeDoubleChecker<double> ())
    .AddAttribute ("OutputPath",
                   "Specifiy the output path where to store the results",
                   StringValue ("results/sidelink/"),
                   MakeStringAccessor (&NrV2XSpectrumPhy::m_outputPath),
                   MakeStringChecker ())
    .AddAttribute ("SavingPeriod",
                   "The period used to save data",
		   DoubleValue (1.0),
		   MakeDoubleAccessor (&NrV2XSpectrumPhy::m_savingPeriod),
		   MakeDoubleChecker<double> ())

  ;

  return tid;
}



Ptr<NetDevice>
NrV2XSpectrumPhy::GetDevice ()
{
  //NS_LOG_FUNCTION (this);
  return m_device;
}


Ptr<MobilityModel>
NrV2XSpectrumPhy::GetMobility ()
{
  //NS_LOG_FUNCTION (this);
  return m_mobility;
}


void
NrV2XSpectrumPhy::SetDevice (Ptr<NetDevice> d)
{
  NS_LOG_FUNCTION (this << d);
  m_device = d;
}


void
NrV2XSpectrumPhy::SetMobility (Ptr<MobilityModel> m)
{
  NS_LOG_FUNCTION (this << m);
  m_mobility = m;
}


void
NrV2XSpectrumPhy::SetChannel (Ptr<SpectrumChannel> c)
{
  NS_LOG_FUNCTION (this << c);
  m_channel = c;
}

Ptr<SpectrumChannel> 
NrV2XSpectrumPhy::GetChannel ()
{
  return m_channel;
}

Ptr<const SpectrumModel>
NrV2XSpectrumPhy::GetRxSpectrumModel () const
{
  return m_rxSpectrumModel;
}


void
NrV2XSpectrumPhy::SetTxPowerSpectralDensity (Ptr<SpectrumValue> txPsd)
{
  NS_LOG_FUNCTION (this << txPsd);
  NS_ASSERT (txPsd);
  m_txPsd = txPsd;
}


void
NrV2XSpectrumPhy::SetNoisePowerSpectralDensity (Ptr<const SpectrumValue> noisePsd)
{
  NS_LOG_FUNCTION (this << noisePsd);
  NS_ASSERT (noisePsd);
  m_rxSpectrumModel = noisePsd->GetSpectrumModel ();
  m_interferenceData->SetNoisePowerSpectralDensity (noisePsd);
  m_interferenceCtrl->SetNoisePowerSpectralDensity (noisePsd);
  m_interferenceSl->SetNoisePowerSpectralDensity (noisePsd);
}

  
void 
NrV2XSpectrumPhy::Reset ()
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
NrV2XSpectrumPhy::ClearExpectedSlTb ()
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG (this << " Expected TBs: " << m_expectedSlTbs.size ());
  m_expectedSlTbs.clear ();
  NS_LOG_DEBUG (this << " After clearing Expected TBs size: " << m_expectedSlTbs.size ());
}


void
NrV2XSpectrumPhy::SetNistLtePhyTxEndCallback (NistLtePhyTxEndCallback c)
{
  NS_LOG_FUNCTION (this);
  m_ltePhyTxEndCallback = c;
}


void
NrV2XSpectrumPhy::SetNistLtePhyRxDataEndErrorCallback (NistLtePhyRxDataEndErrorCallback c)
{
  NS_LOG_FUNCTION (this);
  m_ltePhyRxDataEndErrorCallback = c;
}


void
NrV2XSpectrumPhy::SetNistLtePhyRxDataEndOkCallback (NistLtePhyRxDataEndOkCallback c)
{
  NS_LOG_FUNCTION (this);
  m_ltePhyRxDataEndOkCallback = c;
}

void
NrV2XSpectrumPhy::SetNistLtePhyRxCtrlEndOkCallback (NistLtePhyRxCtrlEndOkCallback c)
{
  NS_LOG_FUNCTION (this);
  m_ltePhyRxCtrlEndOkCallback = c;
}

// TODO FIXME NEW for V2X

void
NrV2XSpectrumPhy::SetUnimoreReportRssiCallback (UnimoreReportRssiCallback c)
{
  NS_LOG_FUNCTION (this);
  m_RssiCallback = c;
}


void
NrV2XSpectrumPhy::SetNistLtePhyRxDataStartCallback (NistLtePhyRxDataStartCallback c)
{
  NS_LOG_FUNCTION (this);
  m_ltePhyRxDataStartCallback = c;
}

void
NrV2XSpectrumPhy::SetNistLtePhyRxCtrlEndErrorCallback (NistLtePhyRxCtrlEndErrorCallback c)
{
  NS_LOG_FUNCTION (this);
  m_ltePhyRxCtrlEndErrorCallback = c;
}


void
NrV2XSpectrumPhy::SetNistLtePhyRxPssCallback (NistLtePhyRxPssCallback c)
{
  NS_LOG_FUNCTION (this);
  m_ltePhyRxPssCallback = c;
}

void
NrV2XSpectrumPhy::SetNistLtePhyDlHarqFeedbackCallback (NistLtePhyDlHarqFeedbackCallback c)
{
  NS_LOG_FUNCTION (this);
  m_ltePhyDlHarqFeedbackCallback = c;
}

void
NrV2XSpectrumPhy::SetNistLtePhyUlHarqFeedbackCallback (NistLtePhyUlHarqFeedbackCallback c)
{
  NS_LOG_FUNCTION (this);
  m_ltePhyUlHarqFeedbackCallback = c;
}


Ptr<AntennaModel> NrV2XSpectrumPhy::GetRxAntenna ()
{
  return m_antenna;
}

void
NrV2XSpectrumPhy::SetAntenna (Ptr<AntennaModel> a)
{
  NS_LOG_FUNCTION (this << a);
  m_antenna = a;
}

void
NrV2XSpectrumPhy::SetState (State newState)
{
  ChangeState (newState);
}


void
NrV2XSpectrumPhy::ChangeState (State newState)
{
  NS_LOG_LOGIC (this << " state: " << m_state << " -> " << newState);
  m_state = newState;
}


void
NrV2XSpectrumPhy::SetHarqPhyModule (Ptr<NistLteHarqPhy> harq)
{
  m_harqPhyModule = harq;
}




bool
NrV2XSpectrumPhy::StartTxDataFrame (Ptr<PacketBurst> pb, std::list<Ptr<NistLteControlMessage> > ctrlMsgList, Time duration)
{
  NS_LOG_FUNCTION (this << pb);
  return true;
  // Useless             
}

bool
NrV2XSpectrumPhy::StartTxSlDataFrame (Ptr<PacketBurst> pb, std::list<Ptr<NistLteControlMessage> > ctrlMsgList, Time duration, uint8_t groupId)
{
  NS_LOG_FUNCTION (this << pb);
  // Useless          
  return true;   
}


//TODO FIXME NEW for V2X
bool
NrV2XSpectrumPhy::StartTxV2XSlDataFrame (Ptr<PacketBurst> pb, std::list<Ptr<NistLteControlMessage> > ctrlMsgList, Time duration, uint8_t groupId)
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
      m_endTxEvent = Simulator::Schedule (duration, &NrV2XSpectrumPhy::EndTx, this);
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
NrV2XSpectrumPhy::StartTxDlCtrlFrame (std::list<Ptr<NistLteControlMessage> > ctrlMsgList, bool pss)
{
  NS_LOG_FUNCTION (this << " PSS " << (uint16_t)pss);
  //Useless
  return true;
}


bool
NrV2XSpectrumPhy::StartTxUlSrsFrame ()
{
  NS_LOG_FUNCTION (this);
  // Useless  
  return true;           
}



void
NrV2XSpectrumPhy::EndTx ()
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
NrV2XSpectrumPhy::StartRx (Ptr<SpectrumSignalParameters> spectrumRxParams)
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
  
  Ptr<SciV2XLteControlMessage> msg_tmp = DynamicCast<SciV2XLteControlMessage>  ( *(lteV2XSlRxParams->ctrlMsgList.begin()));
  NistV2XSciListElement_s sci_tmp;
  sci_tmp = msg_tmp ->GetSci();

  NS_LOG_INFO("UE " << GetDevice()->GetNode()->GetId() << " receiving packet " << sci_tmp.m_packetID << " from UE " << lteV2XSlRxParams->nodeId);
 
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
   
   //   SetRxSensitivity (-90.4); //expressed in dBm 
      int i = 0;
      double totalPowerW = 0;
      double nRB = 0;
      for (Values::const_iterator it=lteV2XSlRxParams->psd->ConstValuesBegin (); it != lteV2XSlRxParams->psd->ConstValuesEnd () ; it++, i++)
      {
        if ((*it != 0) && (i >= sci_tmp.m_rbStartPssch) && (i < sci_tmp.m_rbStartPssch + sci_tmp.m_rbLenPssch_TB))
//        if (*it != 0)
        {
        //  NS_LOG_INFO ("NrV2XSpectrumPhy::StartRx, Now: " << Simulator::Now() << " V2X SL message arriving on RB " << i << ", PSD = " << (*it) << " W/Hz");  
          double powerRxW = (*it) * 180000.0 / m_slotDuration;
          totalPowerW += powerRxW;
          nRB++;
        }
      }
      double totalPowerDbm = 10*std::log10(1000*totalPowerW);
      double rxSensitivitymWPerRB = (std::pow (10, (m_rxSensitivity)/10) )/m_BW_RBs;
      double rxSensitivitydBmPerTB = 10 * std::log10(nRB * rxSensitivitymWPerRB);
      rxSensitivitydBmPerTB += 0;
      //  std::cout << "nRB: " << nRB << ", full BW Sens: " << m_rxSensitivity <<  " dBm, rxSensitivitymWPerRB: " << rxSensitivitymWPerRB << "mW, nRB * rxSensitivitymWPerRB: " << nRB * rxSensitivitymWPerRB << "mW, std::log(nRB * rxSensitivitymWPerRB): " << std::log(nRB * rxSensitivitymWPerRB) <<  ", rxSensitivitydBmPerTB:" << rxSensitivitydBmPerTB << "\r\n";
      NS_LOG_INFO("Received " << nRB << " RBs with power = " << totalPowerDbm << " dBm. Rx sensitivity = " << m_rxSensitivity << " dBm");
    //  NS_LOG_INFO("Rx sensitivity for this data = " << rxSensitivitydBmPerTB << " dBm");
    //  std::cin.get();
      Ptr<MobilityModel> mobility = GetDevice () -> GetNode() -> GetObject<MobilityModel>(); 
      /*double xPosRx = mobility -> GetPosition().x;
      std::ofstream totalPowerSpectrumPhy;
      totalPowerSpectrumPhy.open (m_outputPath + "receivedPower.csv", std::ios_base::app);
      totalPowerSpectrumPhy << xPosRx << "," << totalPowerDbm << "\r\n";
      totalPowerSpectrumPhy.close ();*/
             

//      if( (m_ctrlFullDuplexEnabled && lteV2XSlRxParams->ctrlMsgList.size () > 0) && (totalPowerDbm > rxSensitivitydBmPerTB))
      if( (m_ctrlFullDuplexEnabled && lteV2XSlRxParams->ctrlMsgList.size () > 0) && (totalPowerDbm >= m_rxSensitivity))
      {
        StartRxV2XSlData (lteV2XSlRxParams);
        NS_LOG_INFO("Total power in dBm " << totalPowerDbm);
      }
//      else if ((!m_halfDuplexPhy || m_halfDuplexPhy->GetState () == IDLE || !(m_halfDuplexPhy->m_ulDataSlCheck)) && (totalPowerDbm > rxSensitivitydBmPerTB))
      else if ((!m_halfDuplexPhy || m_halfDuplexPhy->GetState () == IDLE || !(m_halfDuplexPhy->m_ulDataSlCheck)) && (totalPowerDbm >= m_rxSensitivity))
      {
        StartRxV2XSlData (lteV2XSlRxParams);
        NS_LOG_INFO ("Total power in dBm " << totalPowerDbm);
      }
      else
      {
        NS_LOG_INFO("Cannot receive this packet!");
        Vector posRX;
        Ptr<MobilityModel> mobTX, mobRX; 
        mobRX = GetDevice()->GetNode()->GetObject<MobilityModel>();
        posRX = mobRX->GetPosition();
        NodeContainer GlobalContainer = NodeContainer::GetGlobal(); //Used to evaluate the transmitter-receiver distance at spectrum layer
        if ((posRX.x >= 1500) && (posRX.x <= 3500) && ( lteV2XSlRxParams->nodeId != GetDevice()->GetNode()->GetId()) && (GetDevice()->GetNode()->GetId() < (GlobalContainer.GetN()-3) ))
        {
          Ptr<Node> TxNode;
          for (NodeContainer::Iterator L = GlobalContainer.Begin(); L != GlobalContainer.End(); ++L) 
          {
            TxNode = *L;
            if ( lteV2XSlRxParams->nodeId == TxNode->GetId())
            {
              mobTX = TxNode->GetObject<MobilityModel>();
              break;
            }
          }
          PacketStatus newRx;
          newRx.rxTime = std::floor(Simulator::Now().GetSeconds()*100)/100;
          newRx.TxDistance = mobRX->GetDistanceFrom(mobTX);
          newRx.packetID = sci_tmp.m_packetID;
          newRx.txID = TxNode->GetId();
          newRx.rxID = GetDevice()->GetNode()->GetId();
          newRx.decodingStatus = false;
          if (totalPowerDbm < m_rxSensitivity)
          { 
            NS_LOG_INFO ("Signal level below sensitivity");
            newRx.lossType = 0;
          }
          else if (!(!m_halfDuplexPhy || m_halfDuplexPhy->GetState () == IDLE || !(m_halfDuplexPhy->m_ulDataSlCheck)))
          {
            NS_LOG_INFO("Half duplex loss!");
            newRx.lossType = 1;
          }
          m_receivedPackets.push_back(newRx);
         /* std::ofstream AlePDR; 
          AlePDR.open(m_outputPath + "AlejandroPDR.txt", std::ios_base::app);
          AlePDR << std::floor(Simulator::Now().GetSeconds()*100)/100 << "," << sci_tmp.m_packetID << "," << mobRX->GetDistanceFrom(mobTX) << "," <<  TxNode->GetId() << "," << GetDevice()->GetNode()->GetId() << ",0";
          if (totalPowerDbm < m_rxSensitivity)
          { 
            NS_LOG_INFO ("Signal level below sensitivity");
            AlePDR << ",0" << std::endl;
          }
          else if (!(!m_halfDuplexPhy || m_halfDuplexPhy->GetState () == IDLE || !(m_halfDuplexPhy->m_ulDataSlCheck)))
          {
            NS_LOG_INFO("Half duplex loss!");
            AlePDR << ",1" << std::endl;
          }
          AlePDR.close();*/
     //     if (lteV2XSlRxParams->nodeId != 1)
     //     std::cin.get();  
       }

        if (totalPowerDbm < m_rxSensitivity)
        {
           NS_LOG_INFO ("Signal level below sensitivity");
     //     NS_LOG_UNCOND(GetDevice()->GetNode()->GetId() << " receiving message from " <<lteV2XSlRxParams->nodeId);
       //   std::cin.get();
           ++m_totalReceptions;
           ++m_lostPKTs[lteV2XSlRxParams->nodeId].propagationLosses;
        }
        else if (!(!m_halfDuplexPhy || m_halfDuplexPhy->GetState () == IDLE || !(m_halfDuplexPhy->m_ulDataSlCheck)))
        {
          NS_LOG_INFO("Half duplex loss!");
        }

      }
//      if (totalPowerDbm < rxSensitivitydBmPerTB)

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
NrV2XSpectrumPhy::StartRxData (Ptr<NistLteSpectrumSignalParametersDataFrame> params)
{
  NS_LOG_FUNCTION (this);
  // Useless             
}

void
NrV2XSpectrumPhy::StartRxSlData (Ptr<NistLteSpectrumSignalParametersSlFrame> params)
{
  NS_LOG_FUNCTION (this);
  // Useless             
}

//TODO FIXME NEW for V2X
void
NrV2XSpectrumPhy::StartRxV2XSlData (Ptr<NistLteSpectrumSignalParametersV2XSlFrame> params)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_LOGIC(this << " ID:" << GetDevice()->GetNode()->GetId() << " state: " << m_state << " Starting to receive SL V2X Data");

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
                       m_endRxDataEvent = Simulator::Schedule (params->duration, &NrV2XSpectrumPhy::EndRxV2XSlData, this);
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
                 m_endRxDataEvent = Simulator::Schedule (params->duration, &NrV2XSpectrumPhy::EndRxV2XSlData, this);  
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
             NistV2XSciListElement_s sci, sci_tmp;
             //convert the PSD to RB map so we know which RBs were used to transmit the control message
             //will be used later to compute error rate


             Ptr<SciV2XLteControlMessage> msg_tmp = DynamicCast<SciV2XLteControlMessage>  ( *(params->ctrlMsgList.begin()));
             sci_tmp = msg_tmp ->GetSci();
             std::vector <int> rbMap; // including both PSSCH and PSCCH
             std::map <int, double> psdMap;
             int i = 0;
             double RSRP = 0, RSRP_dBm = 0, avgRSRP_dBm = 0, RSSI = 0, RSSI_dBm;
             int rbLen = 0;

             for (Values::const_iterator it=params->psd->ConstValuesBegin (); it != params->psd->ConstValuesEnd () ; it++, i++)
             {             
                if ((*it != 0) && (i >= sci_tmp.m_rbStartPssch) && (i < sci_tmp.m_rbStartPssch + sci_tmp.m_rbLenPssch_TB))
                {
                    ++rbLen;
                    RSRP = ((*it) * 180000.0 / m_slotDuration) / 12.0; // convert PSD [W/Hz] to linear power [W] per symbol
                    RSSI += RSRP * 12.0;
                    RSRP_dBm = 10*std::log10(1000*RSRP);
 	            avgRSRP_dBm += RSRP_dBm;
                    NS_LOG_INFO ("NrV2XSpectrumPhy::StartRxV2XSlData, Now: "<< Simulator::Now() << " V2X SL message arriving on RB " << i << ", PSD = " << (*it) << " W/Hz, RSRP = " << RSRP_dBm << " dBm");  
                    
                    rbMap.push_back (i);
                    psdMap.insert (std::pair<int,double> (i, (*it)));
                 }
               
             }
             avgRSRP_dBm /= rbLen;
             RSSI_dBm = 10*std::log10(1000*RSSI); 

             NS_LOG_DEBUG("UE ID " << GetDevice()->GetNode()->GetId() << " evaluated RSRP over " << rbLen << " RBs starting from " << rbMap.front() << ". Avg RSRP = " << avgRSRP_dBm << " dBm, RSSI = " << RSSI_dBm << " dBm");
             // 9dB is the noise figure
             NS_LOG_DEBUG("SNR RSRP = " << avgRSRP_dBm - (-174 + 9 + 10*std::log10(180000.0 / m_slotDuration / 12)) << " dB, SNR RSSI = " << RSSI_dBm - (-174 + 9 + 10*std::log10(180000.0 / m_slotDuration * rbLen) ) << " dB");
             NS_LOG_DEBUG("Now: RSSI Callback");
             if (!m_RssiCallback.IsNull ())
	       m_RssiCallback (RSSI_dBm, rbMap, params->nodeId);
       
             // Fill the vector with the SNR values (without interference)
             m_expectedSlTbSNR.push_back(std::pair<uint32_t,double> (params->nodeId,std::pow( 10, (RSSI_dBm - (-174 + 9 + 10*std::log10(180000.0 / m_slotDuration * rbLen) )) / 10 ) ) ); 
                       
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
                for (int i = sci.m_rbStartPssch; i < sci.m_rbStartPssch + sci.m_rbLenPssch_TB; i++)
                {
                   rbMapPssch.push_back (i);
                }
                AddExpectedTb (sci.m_rnti, sci.m_groupDstId, !((int) sci.m_reTxIndex == 1), sci.m_tbSize, sci.m_mcs, rbMapPssch, 1);
                NS_LOG_INFO("Added expected Tb from NrV2XSpectrumPhy");
             }
             packetInfo.rbBitmap = rbMap;
             sci.m_psschRsrpDb = avgRSRP_dBm; //This is the RSRP without interference
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
NrV2XSpectrumPhy::StartRxCtrl (Ptr<SpectrumSignalParameters> params)
{
  NS_LOG_FUNCTION (this);
  // Useless
}

void
NrV2XSpectrumPhy::UpdateSinrPerceived (const SpectrumValue& sinr)
{
  NS_LOG_FUNCTION (this << sinr);
  m_sinrPerceived = sinr;
}

void
NrV2XSpectrumPhy::UpdateSlSinrPerceived (std::vector <SpectrumValue> sinr)
{
  NS_LOG_FUNCTION (this);
  m_slSinrPerceived = sinr;
}

void
NrV2XSpectrumPhy::UpdateSlSigPerceived (std::vector <SpectrumValue> signal)
{
  NS_LOG_FUNCTION (this);
  m_slSignalPerceived = signal;
}

void
NrV2XSpectrumPhy::UpdateSlIntPerceived (std::vector <SpectrumValue> interference)
{
  NS_LOG_FUNCTION (this);
  m_slInterferencePerceived = interference;
  //std::cout << "UpdateSlIntPerceived \r\n";
}

void
NrV2XSpectrumPhy::AddExpectedTb (uint16_t  rnti, uint8_t ndi, uint16_t size, uint8_t mcs, std::vector<int> map, uint8_t layer, uint8_t harqId,uint8_t rv,  bool downlink)
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
NrV2XSpectrumPhy::AddExpectedTb (uint16_t  rnti, uint8_t l1dst, uint8_t ndi, uint16_t size, uint8_t mcs, std::vector<int> map, uint8_t rv)
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
NrV2XSpectrumPhy::AddExpectedTb (uint16_t  rnti, uint8_t resPsdch, uint8_t ndi, std::vector<int> map, uint8_t rv)
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
NrV2XSpectrumPhy::EndRxData ()
{                               
  NS_LOG_FUNCTION (this);
  // Useless
}

  void
NrV2XSpectrumPhy::EndRxSlData ()
{
  NS_LOG_FUNCTION (this);
  // Useless
}


void
NrV2XSpectrumPhy::EndRxV2XSlData ()
{
  NS_LOG_FUNCTION (this);
  bool debugSpectrum = false;
  double Distance;
//  double MIMOGain = 1;
//  double correctionFactor = 6; // Please remove it
//  double correctionFactor = 1; // The correction factor was needed when using the NIST BLER curves. 
  double RelativeSpeed;
  Vector posTX, posRX;
  Ptr<MobilityModel> mobTX;
//  std::map<int,Ptr<Node>>::iterator TxNodeIT;
  std::vector<int>::iterator CtrlCheck;
  Ptr<Node> TxNode;
  bool LOS = true;
  //bool psschCollision = false;
  // bool pscchCollision = true;

  // V2V
  Ptr<NormalRandomVariable> randomNormal =  CreateObject<NormalRandomVariable> ();  // Log-normal random variable for the addition of NLOSv shadowing in the Highway scenario

  // NodeContainer storing all nodes stored in ns3::NodeList
  NodeContainer GlobalContainer = NodeContainer::GetGlobal(); //Used to evaluate the transmitter-receiver distance at spectrum layer

  NS_LOG_LOGIC (this << " ID:" << GetDevice()->GetNode()->GetId() << " state: " << m_state << " Time " << Simulator::Now ().GetSeconds () << " Memory address " << GetDevice()->GetNode());
 
  // Adding position evaluation for the new PHY layer (see 3GPP TR 37.885)
  Ptr<MobilityModel> mobRX = GetDevice()->GetNode()->GetObject<MobilityModel>();
  posRX = mobRX->GetPosition();
//  NS_LOG_LOGIC (this << " Node: " << GetDevice()->GetNode()->GetId() << " Position X = " << posRX.x << " Position Y = " << posRX.y);

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
  
  NS_LOG_DEBUG("-----------SNR MAP-----------");
  for (uint16_t i = 0; i < m_expectedSlTbSNR.size(); i++)
  {
    NS_LOG_DEBUG("TX UE: " << m_expectedSlTbSNR[i].first << " received with SNR = " << 10*std::log10(m_expectedSlTbSNR[i].second) << " dB");
  }
  //Sort the received TBs SCI
//  double tmpSINR, avgSINR; // in linear scale
  double first_SINR, second_SINR; // in linear scale
  expectedSlTbs_t::iterator itTb, itTb_BestSINR;
  std::map <NistSlTbId_t, uint32_t>::iterator itSinr;
 
  expectedSlTbs_t::iterator itTb_2;
  std::map <NistSlTbId_t, uint32_t>::iterator itSinr_2;

  NS_LOG_DEBUG("Evaluating the overlap of SL transmissions"); // Works only with NR-V2X
  // I can decode only one SCI at a time. Sort them and take only the one with the best SINR in case of overlap
  uint16_t N_subCh = std::floor(m_BW_RBs / m_subChSize); 
  for (uint16_t i = 0; i < N_subCh; i++) // Iterate over all the possible subchannels
  {
    NS_LOG_DEBUG("Checking PSCCH/PSSCH starting on subchannel: " << i << ". RBs from: " << i*m_subChSize << " to " << i*m_subChSize+m_subChSize -1);
    itTb = m_expectedSlTbs.begin ();
    while (itTb!=m_expectedSlTbs.end ())  // Iterate over the received packets
    {
      itSinr = expectedTbToSinrIndex.find (itTb->first);
      // Check if the current packet was transmitted over the i-th subchannel
      if (std::find(m_rxPacketInfo[itSinr->second].rbBitmap.begin(), m_rxPacketInfo[itSinr->second].rbBitmap.end(), i*m_subChSize) != m_rxPacketInfo[itSinr->second].rbBitmap.end())
  //    if (m_rxPacketInfo[(*itSinr).second].rbBitmap[0] == i*m_subChSize)  //If the SCI starts in the current subchannel
      {
        NS_LOG_DEBUG("UE " << itTb->first.m_rnti << " transmitted on subchannel " << i);
        itTb_2 = m_expectedSlTbs.begin ();
        while (itTb_2!=m_expectedSlTbs.end ()) //Iterate over other received packets (not the current one) and compare the SINR in case of overlap
        {
          if (itTb != itTb_2)
          { 
            itSinr_2 = expectedTbToSinrIndex.find ((*itTb_2).first);
            // Check if another packet was transmitted over the i-th subchannel
            if (std::find(m_rxPacketInfo[itSinr_2->second].rbBitmap.begin(), m_rxPacketInfo[itSinr_2->second].rbBitmap.end(), i*m_subChSize) != m_rxPacketInfo[itSinr_2->second].rbBitmap.end())
            {
              NS_LOG_DEBUG("Also UE " << itTb_2->first.m_rnti << " transmitted on subchannel " << i);
              debugSpectrum = true;
              std::vector<int> overlappedRBs; //Vector of the overlapped RBs
              for (uint16_t rbId =  i*m_subChSize; rbId < i*m_subChSize + m_subChSize; rbId++)
              { 
                if ( (std::find(m_rxPacketInfo[itSinr_2->second].rbBitmap.begin(), m_rxPacketInfo[itSinr_2->second].rbBitmap.end(), rbId) != m_rxPacketInfo[itSinr_2->second].rbBitmap.end()) &&
                     (std::find(m_rxPacketInfo[itSinr->second].rbBitmap.begin(), m_rxPacketInfo[itSinr->second].rbBitmap.end(), rbId) != m_rxPacketInfo[itSinr->second].rbBitmap.end()) ) 
                overlappedRBs.push_back(rbId);
              }
              first_SINR = GetMeanSinr (m_slSinrPerceived[itSinr->second], overlappedRBs);
              second_SINR = GetMeanSinr (m_slSinrPerceived[itSinr_2->second], overlappedRBs);
              NS_LOG_DEBUG("Mean SINR of UE " << itTb->first.m_rnti << " is " << first_SINR << ", equal to " << 10*std::log10(first_SINR) << " dB");
              NS_LOG_DEBUG("Mean SINR of UE " << itTb_2->first.m_rnti << " is " << second_SINR << ", equal to " << 10*std::log10(second_SINR) << " dB");
              if (first_SINR < second_SINR) //The first user transmission is corrupted
              {
                if (m_rxPacketInfo[itSinr->second].rbBitmap[0] == i*m_subChSize)  //If the SCI starts in the current subchannel
                {
                  NS_LOG_DEBUG("UE " << itTb->first.m_rnti << " lost the fight. Labelling as corrupted both the SCI and the TB");
          //        itTb->second.corrupt = true; //Then the SCI is corrupted
          //        itTb->second.collidedPssch = true; //Also the TB is corrupted in NR-V2X
                }
                else
                {
                  NS_LOG_DEBUG("UE " << itTb->first.m_rnti << " lost the fight. Labelling as corrupted only the TB");
          //        itTb->second.collidedPssch = true; //Otherwise, only the TB is corrupted
                }
              }
        //  cmpOutput = UnimoreCompareSinrPSSCH(m_slSinrPerceived[(*itSinr).second], (*itTb).second.rbBitmap ,m_slSinrPerceived[(*itSinr_2).second], (*itTb_2).second.rbBitmap); 
            }
          }
        itTb_2++;   
        }
      }
    itTb++;
    }
  } 


/*  NS_LOG_DEBUG("Evaluating the PSCCH (SCI)");
  // I can decode only one SCI at a time. Sort them and take only the one with the best SINR in case of overlap
  uint16_t N_subCh = std::floor(m_BW_RBs / m_subChSize); 
  for (uint16_t i = 0; i < N_subCh; i++) // Iterate over all the possible subchannels
  {
    NS_LOG_DEBUG("Checking PSCCH on subchannel " << i);
    tmpSINR = 0; // Initialize
    itTb = m_expectedSlTbs.begin ();
    while (itTb!=m_expectedSlTbs.end ())  // Iterate over the received packets
    {
      itSinr = expectedTbToSinrIndex.find ((*itTb).first);
      if (m_rxPacketInfo[(*itSinr).second].rbBitmap[0] == i*m_subChSize)  //If the SCI starts in the current subchannel
      {
          NS_LOG_DEBUG("Found PSCCH in subchannel " << i);
     //     avgSINR = GetMeanSinrPSCCH(m_slSinrPerceived[(*itSinr).second], (*itTb).second.rbBitmap, 2); //Mode 4
          avgSINR = GetMeanSinrPSCCH(m_slSinrPerceived[(*itSinr).second], (*itTb).second.rbBitmap, m_subChSize); // SCI is accommodated over the first subchannel  
          NS_LOG_DEBUG("Average PSCCH SINR = " << 10*std::log10(avgSINR) << " dB");
         if (avgSINR > tmpSINR) //Check the highest SINR value
         {
            tmpSINR = avgSINR;
            itTb_BestSINR = itTb;
         }
      }
      itTb++;
    } 
    itTb = m_expectedSlTbs.begin ();
    while (itTb!=m_expectedSlTbs.end ()) 
    {
      itSinr = expectedTbToSinrIndex.find ((*itTb).first);
      //If the SCI starts in the current subchannel, but it's not the one with the best SINR, then label the packet as corrupted
      //  Its SCI could not be recovered 
      if ((m_rxPacketInfo[(*itSinr).second].rbBitmap[0] == i*m_subChSize) && (itTb != itTb_BestSINR)) 
      {
         itTb->second.corrupt = true;
      }
      itTb++;
    } 

  } 

  // Now check if there is any PSSCH and PSCCH overlapping
  NS_LOG_DEBUG("Evaluating the PSSCH/PSCCH overlapping");
  expectedSlTbs_t::iterator itTb_2;
  std::map <NistSlTbId_t, uint32_t>::iterator itSinr_2;
  int cmpOutput;
  itTb = m_expectedSlTbs.begin ();
  while (itTb!=m_expectedSlTbs.end ()) 
  {
//    if ((!itTb->second.corrupt) && (!itTb->second.collidedPssch)) //If this PSSCH is not already corrupted
    if (true) //If this PSSCH is not already corrupted
    { 
      itSinr = expectedTbToSinrIndex.find ((*itTb).first); //SINR of the current PSSCH
      itTb_2 = m_expectedSlTbs.begin ();
      while (itTb_2!=m_expectedSlTbs.end ()) //Iterate over other received packets (not the current one) and compare the SINR in case of overlap
      { 
//        if ((itTb != itTb_2) && (!itTb_2->second.corrupt))
        if (itTb != itTb_2)
        { 
          itSinr_2 = expectedTbToSinrIndex.find ((*itTb_2).first);
          NS_LOG_DEBUG("Evaluating UEs " << itTb->first.m_rnti << " and " << itTb_2->first.m_rnti);
          // returns 0 if there is no overlap, returns 1 if the first avg SINR is stronger than the second and returns 2 otherwise
          cmpOutput = UnimoreCompareSinrPSSCH(m_slSinrPerceived[(*itSinr).second], (*itTb).second.rbBitmap ,m_slSinrPerceived[(*itSinr_2).second], (*itTb_2).second.rbBitmap); 
          if (cmpOutput == 0)  
          { 
             NS_LOG_DEBUG("No overlap");
          }
          else if (cmpOutput == 1)  
          {
             NS_LOG_DEBUG("Overlap: SINR of UE " << itTb->first.m_rnti << " is larger");
             itTb_2->second.collidedPssch = true;
             if ((*itTb).second.rbBitmap.size() >= (*itTb_2).second.rbBitmap.size())
             {
                itTb_2->second.corrupt = true;
             }
          }
          else if (cmpOutput == 2)  
          {
             NS_LOG_DEBUG("Overlap: SINR of UE " << itTb_2->first.m_rnti << " is larger");
             itTb->second.collidedPssch = true;
             if ((*itTb).second.rbBitmap.size() <= (*itTb_2).second.rbBitmap.size())
             {
                itTb->second.corrupt = true;
             }
          }
          else
          {
             // Useless
             NS_ASSERT_MSG(false,"Unknown value returned from NrV2XSpectrumPhy::cmpOutput");
          }
        }
        itTb_2++;   
      } // end of while (itTb_2!=m_expectedSlTbs.end ()) 
    } //end of if ((!itTb->second.corrupt) && (!itTb->second.collidedPssch)) 
    itTb++;
  }  //end of while (itTb!=m_expectedSlTbs.end ()) */

  NS_LOG_DEBUG("Rx node: " << GetDevice()->GetNode()->GetId() << " at (X,Y) = (" << posRX.x << "," << posRX.y << "). Starting the SNR/SINR evaluation");
  itTb = m_expectedSlTbs.begin ();
  NS_LOG_DEBUG("Now: evaluating only the SNR for packets that cannot be recovered due to overlapping");
  while (itTb!=m_expectedSlTbs.end ()) 
  {
    //If already corrupted because it's overlapped, check what would have happened without the overlapping
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
      RelativeSpeed = mobTX->GetRelativeSpeed(mobRX)*3.6; //expressed in km/h
      bool LOSstate = m_channelModels->GetLineOfSightState(TxNode->GetId(), GetDevice()->GetNode()->GetId());
      NS_LOG_DEBUG(this << " TX Node: " << TxNode->GetId() << " at (X,Y) = (" << posTX.x << "," << posTX.y << "), Tx-Rx Distance = " << Distance << ", LOS state " << LOSstate << " Tx-Rx relative speed = " << RelativeSpeed);
   //   std::cin.get();
      double SNR;
      for (uint16_t i = 0; i < m_expectedSlTbSNR.size(); i++)
      {
        if (m_expectedSlTbSNR[i].first == (*itTb).first.m_rnti)
        SNR = m_expectedSlTbSNR[i].second;
      }
      NS_LOG_DEBUG("SNR = " << 10*std::log10(SNR) << " dB");
      NistTbErrorStats_t tbStats;
      NistTbErrorStats_t tbStatsPSCCH1;
//      NistTbErrorStats_t tbStatsPSCCH2;
      NS_LOG_DEBUG (this << " Computing the PSSCH BLER in LOS (without Interference) ");
//        tbStats = NrV2XPhyErrorModel::GetV2VPsschBler (itTb->second.mcs, SNR,  harqInfoList, LOS, m_SCS, itTb->second.rbBitmap.size());
      tbStats = NrV2XPhyErrorModel::GetNrV2XPsschBler (itTb->second.mcs, SNR, LOS, m_SCS, RelativeSpeed);
      NS_LOG_DEBUG (this << " Computing the PSCCH BLER (1) ");
      //  tbStatsPSCCH1 = NrV2XPhyErrorModel::GetV2VPscchBler (itTb->second.mcs, SNR,  harqInfoList, LOS, m_SCS, m_subChSize);
      tbStatsPSCCH1 = NrV2XPhyErrorModel::GetNrV2XPscchBler(itTb->second.mcs, SNR, LOS, m_SCS);
  
//      if (m_random->GetValue () > tbStatsPSCCH1.tbler ? true : false || m_random->GetValue () > tbStatsPSCCH2.tbler ? true : false) // Mode 4
      if (m_random->GetValue () > tbStatsPSCCH1.tbler ? true : false) 
      {
            bool tmp_collidedPssch;
	    NS_LOG_INFO("PSCCH Recovered Successfully!");
       	    tmp_collidedPssch = m_random->GetValue () > tbStats.tbler ? false : true;  // m_random is uniformly distributed between 0 and 1
            if (tmp_collidedPssch)
            {
                itTb->second.lossType = 2;
                ++m_lostPKTs[itTb->first.m_rnti].propagationLosses;
                NS_LOG_INFO("PSSCH Not Recovered!");
            }
            else
            {
                NS_LOG_INFO("PSSCH Recovered Successfully!");
                itTb->second.lossType = 3;
                ++m_lostPKTs[itTb->first.m_rnti].collisionLosses;
            }
      }
      else
      {
         NS_LOG_INFO("PSCCH Recovery Failed! Dropping the packet");
         itTb->second.lossType = 2;
         ++m_lostPKTs[itTb->first.m_rnti].propagationLosses;
      }

    }  // end if (itTb->second.corrupt || itTb->second.collidedPssch)
    itTb++;
  } 

  //Compute error for each expected Tb
  //expectedSlTbs_t::iterator itTb = m_expectedSlTbs.begin ();
  itTb = m_expectedSlTbs.begin ();  //Already declared in the previous block
//  std::map <NistSlTbId_t, uint32_t>::iterator itSinr;
  NS_LOG_DEBUG("Now: evaluating the SINR");
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
          RelativeSpeed = mobTX->GetRelativeSpeed(mobRX)*3.6; //expressed in km/h
          bool LOSstate = m_channelModels->GetLineOfSightState(TxNode->GetId(), GetDevice()->GetNode()->GetId());
          NS_LOG_DEBUG(this << " TX Node: " << TxNode->GetId() << " at (X,Y) = (" << posTX.x << "," << posTX.y << "), Tx-Rx Distance = " << Distance << ", LOS state " << LOSstate << " Tx-Rx relative speed = " << RelativeSpeed);
      //    std::cin.get();
          double BLERrandomValue = m_random->GetValue ();
          NS_LOG_DEBUG("BLER random value: " << BLERrandomValue);
          if (!itTb->second.corrupt)//If the SCI is not already corrupted
          {   
              NS_LOG_DEBUG("Valid SCI");
              if (!itTb->second.collidedPssch) //If the TB is not already corrupted
              {
                 NS_LOG_DEBUG("Non-collided PSSCH");
                 double SNR;
                // bool PropagationError = false;
                 for (uint16_t i = 0; i < m_expectedSlTbSNR.size(); i++)
                 {
                    if (m_expectedSlTbSNR[i].first == (*itTb).first.m_rnti)
                       SNR = m_expectedSlTbSNR[i].second;
                 }
                 NS_LOG_DEBUG("SNR = " << 10*std::log10(SNR) << " dB");
                 NistTbErrorStats_t tbStats;
                 NistTbErrorStats_t tbStatsPSCCH1;
//                 NistTbErrorStats_t tbStatsPSCCH2;
                 NS_LOG_DEBUG (this << " Not already collided PSSCH. Computing the PSSCH BLER in LOS (without Interference) ");
                 //  tbStats = NrV2XPhyErrorModel::GetV2VPsschBler (itTb->second.mcs, SNR,  harqInfoList, LOS, m_SCS, itTb->second.rbBitmap.size());
                 tbStats = NrV2XPhyErrorModel::GetNrV2XPsschBler (itTb->second.mcs, SNR, LOS, m_SCS, RelativeSpeed);
                 NS_LOG_DEBUG (this << " Computing the PSCCH BLER (1) ");
                 // tbStatsPSCCH1 = NrV2XPhyErrorModel::GetV2VPscchBler (itTb->second.mcs, SNR,  harqInfoList, LOS, m_SCS, m_subChSize);
                 tbStatsPSCCH1 = NrV2XPhyErrorModel::GetNrV2XPscchBler(itTb->second.mcs, SNR, LOS, m_SCS);                
                 NS_LOG_DEBUG("TBLER PSSCH = " << tbStats.tbler << " TBLER PSCCH (1) = " << tbStatsPSCCH1.tbler);
                //Before looking at the PSSCH I need to check that at least one PSCCH RB has been received successfully!
//		 if (m_random->GetValue () > tbStatsPSCCH1.tbler ? true : false || m_random->GetValue () > tbStatsPSCCH2.tbler ? true : false) // If one out of 2 PSCCH RBs is correct
//		 if (m_random->GetValue () > tbStatsPSCCH1.tbler ? true : false)
		 if (BLERrandomValue > tbStatsPSCCH1.tbler ? true : false)
		 {
		   NS_LOG_INFO("PSCCH Recovered Successfully!");
                  // (*itTb).second.collidedPssch = m_random->GetValue () > tbStats.tbler ? false : true;  // m_random is uniformly distributed between 0 and 1
                   (*itTb).second.collidedPssch = BLERrandomValue > tbStats.tbler ? false : true;  // m_random is uniformly distributed between 0 and 1
                   if ((*itTb).second.collidedPssch)
                   {
                     ++m_lostPKTs[itTb->first.m_rnti].propagationLosses;
                     itTb->second.lossType = 2;
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
                   itTb->second.lossType = 2;
                   NS_LOG_INFO("Propagation losses " << m_lostPKTs[itTb->first.m_rnti].propagationLosses);
                 }
                                    
                 // If it was not corrupted by the propagation (using the original SNR) and if there is some interference to evaluate
                 double SNR_new = 10*std::log10(GetLowestSinr (m_slSinrPerceived[itSinr->second], itTb->second.rbBitmap) );
                 if ( (!itTb->second.collidedPssch) && (!itTb->second.corrupt) && ( abs(SNR_new-10*std::log10(SNR)) > 0.001 ) )
                 {
                   NS_LOG_DEBUG("There is some interference to evaluate and the packet is not already corrupted (either SCI or TB)");

                   NS_LOG_DEBUG (this << " Not already collided PSSCH. Computing the PSSCH BLER in LOS (with Interference) ");
                   //  tbStats = NrV2XPhyErrorModel::GetV2VPsschBler (itTb->second.mcs, GetMeanSinr (m_slSinrPerceived[itSinr->second], itTb->second.rbBitmap),  harqInfoList, LOS, m_SCS, itTb->second.rbBitmap.size());
//                     tbStats = NrV2XPhyErrorModel::GetNrV2XPsschBler (itTb->second.mcs, GetMeanSinr (m_slSinrPerceived[itSinr->second], itTb->second.rbBitmap), LOS, m_SCS, RelativeSpeed);
                   tbStats = NrV2XPhyErrorModel::GetNrV2XPsschBler (itTb->second.mcs, GetLowestSinr (m_slSinrPerceived[itSinr->second], itTb->second.rbBitmap), LOS, m_SCS, RelativeSpeed);
                   NS_LOG_DEBUG (this << " Computing the PSCCH BLER (1) ");
                   //  tbStatsPSCCH1 = NrV2XPhyErrorModel::GetV2VPscchBler (itTb->second.mcs, GetMeanSinrPSCCH (m_slSinrPerceived[(*itSinr).second], (*itTb).second.rbBitmap, m_subChSize),  harqInfoList, LOS, m_SCS, m_subChSize);
//                     tbStatsPSCCH1 = NrV2XPhyErrorModel::GetNrV2XPscchBler(itTb->second.mcs, GetMeanSinrPSCCH (m_slSinrPerceived[(*itSinr).second], (*itTb).second.rbBitmap, m_subChSize), LOS, m_SCS);
                   tbStatsPSCCH1 = NrV2XPhyErrorModel::GetNrV2XPscchBler(itTb->second.mcs, GetLowestSinrPSCCH (m_slSinrPerceived[(*itSinr).second], (*itTb).second.rbBitmap, m_subChSize), LOS, m_SCS);
                   NS_LOG_DEBUG("TBLER PSSCH = " << tbStats.tbler << " TBLER PSCCH (1) = " << tbStatsPSCCH1.tbler);            
                   //Before looking at the PSSCH I need to check that at least one PSCCH RB has been received successfully!
//                   if (m_random->GetValue () > tbStatsPSCCH1.tbler ? true : false || m_random->GetValue () > tbStatsPSCCH2.tbler ? true : false) // If one out of 2 PSCCH RBs is correct
//                   if (m_random->GetValue () > tbStatsPSCCH1.tbler ? true : false )
                   if (BLERrandomValue > tbStatsPSCCH1.tbler ? true : false )
		   {
		     NS_LOG_INFO("PSCCH Recovered Successfully!");
                   //  (*itTb).second.collidedPssch = m_random->GetValue () > tbStats.tbler ? false : true;  // m_random is uniformly distributed between 0 and 1
                     (*itTb).second.collidedPssch = BLERrandomValue > tbStats.tbler ? false : true;  // m_random is uniformly distributed between 0 and 1
                     if ((*itTb).second.collidedPssch)
                     {
                       ++m_lostPKTs[itTb->first.m_rnti].collisionLosses;
                       itTb->second.lossType = 3;
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
                     itTb->second.lossType = 3;
                     ++m_lostPKTs[itTb->first.m_rnti].collisionLosses;
                     NS_LOG_INFO("Collision losses " << m_lostPKTs[itTb->first.m_rnti].collisionLosses);
                   }                                     
                 } // end if ( (!itTb->second.collidedPssch) && (!itTb->second.corrupt) && ((uint16_t)SNR_new*10000 != (uint16_t)SNR*10000) ) //rounded at the 4th decimal place
                 else
                 {
                   NS_LOG_INFO("There is no interference to evaluate for this packet");
                 }
              } // end if (!itTb->second.collidedPssch)
              else
              {
                 // if the TB is already corrupted check if at least the SCI can be recovered
                 NS_LOG_DEBUG("Collided PSSCH, decoding only the SCI");
                 NistTbErrorStats_t tbStatsPSCCH1;
//                 NistTbErrorStats_t tbStatsPSCCH2;
                 NS_LOG_DEBUG (this << " Computing the PSCCH BLER in LOS ");
                 NS_LOG_DEBUG (this << " Computing the PSCCH BLER (1) ");
          //         tbStatsPSCCH1 = NrV2XPhyErrorModel::GetV2VPscchBler (itTb->second.mcs, GetMeanSinrPSCCH (m_slSinrPerceived[(*itSinr).second], (*itTb).second.rbBitmap, m_subChSize),  harqInfoList, LOS, m_SCS, m_subChSize);
//                   tbStatsPSCCH1 =  NrV2XPhyErrorModel::GetNrV2XPscchBler(itTb->second.mcs, GetMeanSinrPSCCH (m_slSinrPerceived[(*itSinr).second], (*itTb).second.rbBitmap, m_subChSize), LOS, m_SCS);
                 tbStatsPSCCH1 =  NrV2XPhyErrorModel::GetNrV2XPscchBler(itTb->second.mcs, GetLowestSinrPSCCH (m_slSinrPerceived[(*itSinr).second], (*itTb).second.rbBitmap, m_subChSize), LOS, m_SCS);
                 NS_LOG_DEBUG("TBLER PSCCH (1) = " << tbStatsPSCCH1.tbler);
                 //Before looking at the PSSCH I need to check that at least one PSCCH RB has been received successfully!
//		 if (m_random->GetValue () > tbStatsPSCCH1.tbler ? true : false || m_random->GetValue () > tbStatsPSCCH2.tbler ? true : false) // If one out of 2 PSCCH RBs is correct
//		 if (m_random->GetValue () > tbStatsPSCCH1.tbler ? true : false)
		 if (BLERrandomValue > tbStatsPSCCH1.tbler ? true : false)
		 {
		   NS_LOG_INFO("PSCCH Recovered Successfully!");
		 }
		 else
		 {
                   NS_LOG_INFO("PSCCH Recovery Failed! Dropping the packet");
                   (*itTb).second.corrupt = true; 
                 }                
                 NS_LOG_INFO (this << " from RNTI " << (*itTb).first.m_rnti << " size " << (*itTb).second.size << " mcs " << (uint32_t)(*itTb).second.mcs << " bitmap " << (*itTb).second.rbBitmap.size () << " corrupted SCI " << (*itTb).second.corrupt << " corrupted TB " << (*itTb).second.collidedPssch);
              } // end else (non-corrupted TB)
          } // end else if (!itTb->second.corrupt)
          else
          {
              NS_LOG_DEBUG("Non-valid SCI");
              NS_ASSERT_MSG(itTb->second.collidedPssch, "In NR-V2X the TB should be corrupted if the SCI cannot be recovered");
              itTb->second.collidedPssch = true; //useless, also the TB is corrupted in NR-V2X if the SCI is corrupted
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
//          NS_LOG_DEBUG("Fired traces on SL reception PHY stats");     

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
                NS_ASSERT (m_rxPacketInfo[i].m_rxControlMessage -> GetMessageType () == NistLteControlMessage::SCI_V2X);
                Ptr<SciV2XLteControlMessage> msg2 = DynamicCast<SciV2XLteControlMessage> (m_rxPacketInfo[i].m_rxControlMessage);
                NistV2XSciListElement_s sci = msg2->GetSci ();

           //     Vector posRX;
           //     Ptr<MobilityModel> mobTX, mobRX; 
                mobRX = GetDevice()->GetNode()->GetObject<MobilityModel>();
                posRX = mobRX->GetPosition();
                for (NodeContainer::Iterator L = GlobalContainer.Begin(); L != GlobalContainer.End(); ++L) 
                {
                  TxNode = *L;
                  if ( tbId.m_rnti == TxNode->GetId())
                  {
                    mobTX = TxNode->GetObject<MobilityModel>();
                    break;
                  }
                }

                if ((posRX.x >= 1500) && (posRX.x <= 3500))
                {              
                  PacketStatus newRx;
                  newRx.rxTime = std::floor(Simulator::Now().GetSeconds()*100)/100;
                  newRx.TxDistance = mobRX->GetDistanceFrom(mobTX);
                  newRx.packetID = sci.m_packetID ;
                  newRx.txID = TxNode->GetId();
                  newRx.rxID = GetDevice()->GetNode()->GetId();
                  if (!itTb->second.corrupt)
                  {
                    if (!itTb->second.collidedPssch)
                    {
                      newRx.decodingStatus = true;
                      newRx.lossType = 9;
                    }
                    else
                    {
                      newRx.decodingStatus = false;
                      newRx.lossType = itTb->second.lossType;
                    }
                  }
                  else
                  {
                    newRx.decodingStatus = false;
                    newRx.lossType = itTb->second.lossType;
                  }
                  m_receivedPackets.push_back(newRx);
                /*  std::ofstream AlePDR; 
                  AlePDR.open(m_outputPath + "AlejandroPDR.txt", std::ios_base::app);
                  AlePDR << std::floor(Simulator::Now().GetSeconds()*100)/100 << "," << sci.m_packetID << "," << mobRX->GetDistanceFrom(mobTX) << "," <<  TxNode->GetId() << "," << GetDevice()->GetNode()->GetId() << ",";
                  if (!itTb->second.corrupt)
                  {
                    if (!itTb->second.collidedPssch)
                       AlePDR << "1,9" << std::endl;
                    else
                       AlePDR << "0," << itTb->second.lossType << std::endl;
                  }
                  else
                  {
                    AlePDR << "0," << itTb->second.lossType << std::endl;
                  }
                  AlePDR.close();*/

                }

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
                    /*if (!m_nistErrorModelEnabled)
                    {
                      m_harqPhyModule->UpdateSlHarqProcessNistStatus (tbId.m_rnti, tbId.m_l1dst, (*itTb).second.mi, (*itTb).second.size, (*itTb).second.size / EffectiveCodingRate [(*itTb).second.mcs]);
                    }
                    else
                    {*/
                      m_harqPhyModule->UpdateSlHarqProcessNistStatus (tbId.m_rnti, tbId.m_l1dst, (*itTb).second.sinr);
                    //}
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
            NS_LOG_DEBUG("Adding PSCCH trace"); //Not used 
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
       NS_LOG_DEBUG (this << " PSCCH OK, size = " << rxControlMessageOkList.size()); 
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

  if ((m_saveCollisionsUniMore) && (Simulator::Now ().GetSeconds () - m_prevPrintTime > m_savingPeriod))
  {

    std::ofstream AlePDR; 
    AlePDR.open(m_outputPath + "AlejandroPDR.txt", std::ios_base::app);
    for (std::vector<PacketStatus>::iterator iit = m_receivedPackets.begin(); iit != m_receivedPackets.end(); iit++)
    {
      AlePDR << iit->rxTime << "," << iit->packetID << "," << iit->TxDistance << "," << iit->txID << "," << iit->rxID << "," << (uint16_t) iit->decodingStatus << "," << iit->lossType << std::endl;
    }
    m_receivedPackets.clear();
     //std::floor(Simulator::Now().GetSeconds()*100)/100 << "," << sci.m_packetID << "," << mobRX->GetDistanceFrom(mobTX) << "," <<  TxNode->GetId() << "," << GetDevice()->GetNode()->GetId() << ",";
    AlePDR.close();

    m_prevPrintTime = Simulator::Now ().GetSeconds ();
    std::ofstream collisionCounters; 
    collisionCounters.open(m_outputPath + "collisionCounters.txt", std::ios_base::app);
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

//  if ((m_expectedSlTbs.size()>1) && (debugSpectrum))
//   std::cin.get();

  //done with sidelink data, control and discovery
  ChangeState (IDLE);
  m_rxPacketBurstList.clear ();
  m_rxControlMessageList.clear ();
  //m_rxControlMessageRbMap.clear ();
  m_rxPacketInfo.clear ();
  m_expectedSlTbs.clear ();
  m_expectedDiscTbs.clear ();
  m_expectedSlTbSNR.clear();

 // std::cin.get();
}

void
NrV2XSpectrumPhy::EndRxDlCtrl ()
{
  NS_LOG_FUNCTION (this);
  // Useless
}
  
void
NrV2XSpectrumPhy::EndRxUlSrs ()
{
  NS_ASSERT (m_state == RX_CTRL);
  ChangeState (IDLE);
  m_interferenceCtrl->EndRx ();
  // nothing to do (used only for SRS at this stage)
}

void 
NrV2XSpectrumPhy::SetCellId (uint16_t cellId)
{
  m_cellId = cellId;
}

void 
NrV2XSpectrumPhy::AddL1GroupId (uint8_t groupId)
{
  NS_LOG_FUNCTION (this << (uint16_t) groupId);
  m_l1GroupIds.insert(groupId);
}

void 
NrV2XSpectrumPhy::RemoveL1GroupId (uint8_t groupId)
{
  m_l1GroupIds.erase (groupId);
}

void
NrV2XSpectrumPhy::AddRsPowerChunkProcessor (Ptr<NistLteChunkProcessor> p)
{
  m_interferenceCtrl->AddRsPowerChunkProcessor (p);
}

void
NrV2XSpectrumPhy::AddDataPowerChunkProcessor (Ptr<NistLteChunkProcessor> p)
{
  m_interferenceData->AddRsPowerChunkProcessor (p);
}

void
NrV2XSpectrumPhy::AddDataSinrChunkProcessor (Ptr<NistLteChunkProcessor> p)
{
  m_interferenceData->AddSinrChunkProcessor (p);
}

void
NrV2XSpectrumPhy::AddInterferenceCtrlChunkProcessor (Ptr<NistLteChunkProcessor> p)
{
  m_interferenceCtrl->AddInterferenceChunkProcessor (p);
}

void
NrV2XSpectrumPhy::AddInterferenceDataChunkProcessor (Ptr<NistLteChunkProcessor> p)
{
  m_interferenceData->AddInterferenceChunkProcessor (p);
}

void
NrV2XSpectrumPhy::AddCtrlSinrChunkProcessor (Ptr<NistLteChunkProcessor> p)
{
  m_interferenceCtrl->AddSinrChunkProcessor (p);
}

void
NrV2XSpectrumPhy::AddSlSinrChunkProcessor (Ptr<NistLteSlChunkProcessor> p)
{
  m_interferenceSl->AddSinrChunkProcessor (p);
}

void
NrV2XSpectrumPhy::AddSlSignalChunkProcessor (Ptr<NistLteSlChunkProcessor> p)
{
  m_interferenceSl->AddRsPowerChunkProcessor (p);
}

void
NrV2XSpectrumPhy::AddSlInterferenceChunkProcessor (Ptr<NistLteSlChunkProcessor> p)
{
  m_interferenceSl->AddInterferenceChunkProcessor (p);
}

void 
NrV2XSpectrumPhy::SetTransmissionMode (uint8_t txMode)
{
  NS_LOG_FUNCTION (this << (uint16_t) txMode);
  NS_ASSERT_MSG (txMode < m_txModeGain.size (), "TransmissionMode not available: 1.." << m_txModeGain.size ());
  m_transmissionMode = txMode;
  m_layersNum = NistTransmissionModesLayers::TxMode2LayerNum (txMode);
}


void 
NrV2XSpectrumPhy::SetTxModeGain (uint8_t txMode, double gain)
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
NrV2XSpectrumPhy::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  m_random->SetStream (stream);
  return 1;
}

double 
NrV2XSpectrumPhy::GetLowestSinr (const SpectrumValue& sinr, const std::vector<int>& map)
{
  NS_LOG_FUNCTION(this << " Evaluating lowest SINR");
  SpectrumValue sinrCopy = sinr;
  double sinrMin = sinrCopy[map.at (0)];
  NS_LOG_DEBUG("On RB " << map.at (0) << " SINR is " << 10*std::log10(sinrCopy[map.at (0)] ) << " dB");
  for (uint32_t i = 1; i < map.size (); i++)
    { 
      NS_LOG_DEBUG("On RB " << map.at (i) << " SINR is " << 10*std::log10(sinrCopy[map.at (i)] ) << " dB");
//      NS_LOG_DEBUG("Sinr value: " << sinrCopy[map.at (i)] << " at " << i);
      if (sinrCopy[map.at (i)] < sinrMin)
         sinrMin = sinrCopy[map.at (i)];
    }
  NS_LOG_DEBUG("Lowest SINR is " << sinrMin << ", in dB = " << 10*std::log10(sinrMin));

  return sinrMin;
}

double 
NrV2XSpectrumPhy::GetLowestSinrPSCCH (const SpectrumValue& sinr, std::vector<int>& map, uint16_t LenPSCCH)
{
  NS_LOG_FUNCTION(this << " Evaluating PSCCH lowest SINR");
  SpectrumValue sinrCopy = sinr;
  double sinrLin = sinrCopy[map.at (0)];
  for (uint32_t i = 0; i < LenPSCCH; i++)
  { 
//    NS_LOG_DEBUG("Sinr value: " << sinrCopy[map.at (i)] << " at " << i);
    if (sinrCopy[map.at (i)] < sinrLin)
       sinrLin = sinrCopy[map.at (i)];
  }

//  NS_LOG_DEBUG("Lowest PSCCH SINR is " << sinrLin);

  return sinrLin;
}

double 
NrV2XSpectrumPhy::GetMeanSinr (const SpectrumValue& sinr, const std::vector<int>& map)
{
  NS_LOG_FUNCTION(this);
  SpectrumValue sinrCopy = sinr;
  double sinrLin = 0;
  for (uint32_t i = 0; i < map.size (); i++)
  { 
    sinrLin += sinrCopy[map.at (i)];
//    NS_LOG_DEBUG("On RB " << map.at (i) << " SINR is " << 10*std::log10(sinrCopy[map.at (i)] ) << " dB");
  }
//  NS_LOG_DEBUG(this << ": Average SINR = " << 10*std::log10(sinrLin / map.size()) << " dB");
//  NS_LOG_FUNCTION(this << 10*std::log10(sinrLin / map.size()));
  return sinrLin / map.size();
}


double 
NrV2XSpectrumPhy::GetMeanSinrPSCCH (const SpectrumValue& sinr, std::vector<int>& map, uint16_t LenPSCCH)
{
  NS_LOG_FUNCTION(this << " Evaluating PSCCH mean SINR");
  double sinrLin = 0;
  std::vector<int>::iterator bitmapIT = map.begin();
  for (uint32_t i = 0; i < LenPSCCH; i++)
  { 
   //   NS_LOG_DEBUG("Sinr value = " << 10*std::log10(sinr[map.at (i)]) << " at " << *bitmapIT);
      sinrLin += sinr[map.at (i)];
      bitmapIT++;
  }

  sinrLin /= LenPSCCH;
    
//  NS_LOG_DEBUG("SINR = " << sinrLin << " AVG SINR = " << sinrLin << " AVG SINR in dB = " << 10*std::log10(sinrLin));

  return sinrLin;
}


int
NrV2XSpectrumPhy::UnimoreCompareSinrPSSCH (const SpectrumValue& first_sinr, const std::vector<int>& first_map,const SpectrumValue& second_sinr, const std::vector<int>& second_map)
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


NrV2XSpectrumPhy::State
NrV2XSpectrumPhy::GetState ()
{
  return m_state;
}

void
NrV2XSpectrumPhy::SetSlssid (uint64_t slssid)
{
  NS_LOG_FUNCTION (this);
  m_slssId = slssid;
}

void
NrV2XSpectrumPhy::SetNistLtePhyRxSlssCallback (NistLtePhyRxSlssCallback c)
{
  NS_LOG_FUNCTION (this);
  m_ltePhyRxSlssCallback = c;
}

void 
NrV2XSpectrumPhy::SetRxPool (Ptr<SidelinkDiscResourcePool> newpool)
{
  m_discRxPools.push_back (newpool);
}

// NEW: Set the receiver sensitivity

void 
NrV2XSpectrumPhy::SetRxSensitivity (double sensitivity)
{
  m_rxSensitivity = sensitivity;
}

void 
NrV2XSpectrumPhy::AddDiscTxApps (std::list<uint32_t> apps)
{
    m_discTxApps = apps;
}

void 
NrV2XSpectrumPhy::AddDiscRxApps (std::list<uint32_t> apps)
{
  m_discRxApps = apps;
}

bool 
NrV2XSpectrumPhy::FilterRxApps (NistSlDiscMsg disc)
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
NrV2XSpectrumPhy::SetDiscNumRetx (uint8_t retx)
{
  NS_LOG_FUNCTION (this << retx);
  m_harqPhyModule->SetDiscNumRetx (retx);
}

} // namespace ns3
