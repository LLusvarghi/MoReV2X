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
 * Author: Nicola Baldo <nbaldo@cttc.es> (re-wrote from scratch this helper)
 *         Giuseppe Piro <g.piro@poliba.it> (parts of the PHY & channel  creation & configuration copied from the GSoC 2011 code)
 * Modified by: NIST
 */


#include "nist-lte-helper.h"
#include <ns3/string.h>
#include <ns3/log.h>
#include <ns3/abort.h>
#include <ns3/pointer.h>
#include <ns3/nist-epc-ue-nas.h>
#include <ns3/nist-lte-ue-rrc.h>
#include <ns3/nr-v2x-ue-mac.h>
#include <ns3/nr-v2x-ue-phy.h>
#include <ns3/nr-v2x-spectrum-phy.h>
#include <ns3/nist-lte-chunk-processor.h>
#include <ns3/nist-lte-sl-chunk-processor.h>
#include <ns3/multi-model-spectrum-channel.h>
#include <ns3/friis-spectrum-propagation-loss.h>
#include <ns3/isotropic-antenna-model.h>
#include <ns3/nr-v2x-ue-net-device.h>
#include <ns3/nist-lte-rlc.h>
#include <ns3/nr-v2x-rlc-um.h>
#include <ns3/nist-lte-rlc-am.h>
#include <ns3/nist-lte-rrc-protocol-ideal.h>
#include <ns3/nist-epc-helper.h>
#include <iostream>
#include <fstream>
#include <ns3/buildings-propagation-loss-model.h>
#include <ns3/nist-lte-spectrum-value-helper.h>
#include <cfloat>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("NistLteHelper");

NS_OBJECT_ENSURE_REGISTERED (NistLteHelper);

NistLteHelper::NistLteHelper (void)
  : m_imsiCounter (0)
{
  NS_LOG_FUNCTION (this);
  m_ueNetDeviceFactory.SetTypeId (NistLteUeNetDevice::GetTypeId ());
  m_ueAntennaModelFactory.SetTypeId (IsotropicAntennaModel::GetTypeId ());
  m_channelFactory.SetTypeId (MultiModelSpectrumChannel::GetTypeId ());
}

void 
NistLteHelper::DoInitialize (void)
{
  NS_LOG_FUNCTION (this);
  m_downlinkChannel = m_channelFactory.Create<SpectrumChannel> ();
  m_uplinkChannel = m_channelFactory.Create<SpectrumChannel> ();

  m_downlinkPathlossModel = m_dlPathlossModelFactory.Create ();
  Ptr<SpectrumPropagationLossModel> dlSplm = m_downlinkPathlossModel->GetObject<SpectrumPropagationLossModel> ();
  if (dlSplm != 0)
    {
      NS_LOG_LOGIC (this << " using a SpectrumPropagationLossModel in DL");
      m_downlinkChannel->AddSpectrumPropagationLossModel (dlSplm);
    }
  else
    {
      NS_LOG_LOGIC (this << " using a PropagationLossModel in DL");
      Ptr<PropagationLossModel> dlPlm = m_downlinkPathlossModel->GetObject<PropagationLossModel> ();
      NS_ASSERT_MSG (dlPlm != 0, " " << m_downlinkPathlossModel << " is neither PropagationLossModel nor SpectrumPropagationLossModel");
      m_downlinkChannel->AddPropagationLossModel (dlPlm);
    }

  //Nist
  if (m_sameUlDlPropagationCondition)
  {
    m_uplinkPathlossModel = m_downlinkPathlossModel;
  }
  else
  {
    m_uplinkPathlossModel = m_ulPathlossModelFactory.Create ();
  }
  //
  Ptr<SpectrumPropagationLossModel> ulSplm = m_uplinkPathlossModel->GetObject<SpectrumPropagationLossModel> ();
  if (ulSplm != 0)
    {
      NS_LOG_LOGIC (this << " using a SpectrumPropagationLossModel in UL");
      m_uplinkChannel->AddSpectrumPropagationLossModel (ulSplm);
    }
  else
    {
      NS_LOG_LOGIC (this << " using a PropagationLossModel in UL");
      Ptr<PropagationLossModel> ulPlm = m_uplinkPathlossModel->GetObject<PropagationLossModel> ();
      NS_ASSERT_MSG (ulPlm != 0, " " << m_uplinkPathlossModel << " is neither PropagationLossModel nor SpectrumPropagationLossModel");
      m_uplinkChannel->AddPropagationLossModel (ulPlm);
    }
  
  if (!m_fadingModelType.empty ())
    {
      m_fadingModule = m_fadingModelFactory.Create<SpectrumPropagationLossModel> ();
      m_fadingModule->Initialize ();
      m_downlinkChannel->AddSpectrumPropagationLossModel (m_fadingModule);
      m_uplinkChannel->AddSpectrumPropagationLossModel (m_fadingModule);
    }
  Object::DoInitialize ();

}

NistLteHelper::~NistLteHelper (void)
{
  NS_LOG_FUNCTION (this);
}

TypeId NistLteHelper::GetTypeId (void)
{
  static TypeId
    tid =
    TypeId ("ns3::NistLteHelper")
    .SetParent<Object> ()
    .AddConstructor<NistLteHelper> ()
    .AddAttribute ("PathlossModel",
                   "The type of pathloss model to be used. "
                   "The allowed values for this attributes are the type names "
                   "of any class inheriting from ns3::PropagationLossModel.",
                   StringValue ("ns3::FriisPropagationLossModel"),
                   MakeStringAccessor (&NistLteHelper::SetPathlossModelType),
                   MakeStringChecker ())
    .AddAttribute ("FadingModel",
                   "The type of fading model to be used."
                   "The allowed values for this attributes are the type names "
                   "of any class inheriting from ns3::SpectrumPropagationLossModel."
                   "If the type is set to an empty string, no fading model is used.",
                   StringValue (""),
                   MakeStringAccessor (&NistLteHelper::SetFadingModel),
                   MakeStringChecker ())
    .AddAttribute ("UseIdealRrc",
                   "If true, LteRrcProtocolIdeal will be used for RRC signaling. "
                   "If false, LteRrcProtocolReal will be used.",
                   BooleanValue (true), 
                   MakeBooleanAccessor (&NistLteHelper::m_useIdealRrc),
                   MakeBooleanChecker ())
    .AddAttribute ("AnrEnabled",
                   "Activate or deactivate Automatic Neighbour Relation function",
                   BooleanValue (true),
                   MakeBooleanAccessor (&NistLteHelper::m_isAnrEnabled),
                   MakeBooleanChecker ())
    .AddAttribute ("UsePdschForCqiGeneration",
                   "If true, DL-CQI will be calculated from PDCCH as signal and PDSCH as interference "
                   "If false, DL-CQI will be calculated from PDCCH as signal and PDCCH as interference  ",
                   BooleanValue (true),
                   MakeBooleanAccessor (&NistLteHelper::m_usePdschForCqiGeneration),
                   MakeBooleanChecker ())
    .AddAttribute ("UseSidelink",
                   "If true, UEs will be able to receive sidelink communication. "
                   "If false, sidelink communication will not be possible.",
                   BooleanValue (false), 
                   MakeBooleanAccessor (&NistLteHelper::m_useSidelink),
                   MakeBooleanChecker ())
    .AddAttribute ("UseV2XSidelink",
                   "If true, UEs will use sidelink V2X communications as per Rel'14. "
                   "If false, V2X sidelink communication will not be possible.",
                   BooleanValue (true), 
                   MakeBooleanAccessor (&NistLteHelper::m_useV2XSidelink),
                   MakeBooleanChecker ())
    .AddAttribute ("UseDiscovery",
                   "If true, UEs will be able to do discovery. "
                   "If false, discovery will not be possible.",
                   BooleanValue (false), 
                   MakeBooleanAccessor (&NistLteHelper::m_useDiscovery),
                   MakeBooleanChecker ())
    .AddAttribute ("UseSameUlDlPropagationCondition",
                   "If true, same conditions for both UL and DL"
                   "If false, different instances of the pathloss model",
                   BooleanValue (false),
                   MakeBooleanAccessor (&NistLteHelper::m_sameUlDlPropagationCondition),
                   MakeBooleanChecker ())
  ;
  return tid;
}

void
NistLteHelper::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_downlinkChannel = 0;
  m_uplinkChannel = 0;
  Object::DoDispose ();
}


void 
NistLteHelper::SetEpcHelper (Ptr<NistEpcHelper> h)
{
  NS_LOG_FUNCTION (this << h);
  m_epcHelper = h;
}


void 
NistLteHelper::SetPathlossModelType (std::string type) 
{
  NS_LOG_FUNCTION (this << type);
  m_dlPathlossModelFactory = ObjectFactory ();
  m_dlPathlossModelFactory.SetTypeId (type);
  m_ulPathlossModelFactory = ObjectFactory ();
  m_ulPathlossModelFactory.SetTypeId (type);
}

void
NistLteHelper::SetSlPathlossModelType (std::string type) 
{
  NS_LOG_FUNCTION (this << type);
  m_slPathlossModelFactory = ObjectFactory ();
  m_slPathlossModelFactory.SetTypeId (type);
}


void 
NistLteHelper::SetPathlossModelAttribute (std::string n, const AttributeValue &v)
{
  NS_LOG_FUNCTION (this << n);
  m_dlPathlossModelFactory.Set (n, v);
  m_ulPathlossModelFactory.Set (n, v);
}


void
NistLteHelper::SetUeDeviceAttribute (std::string n, const AttributeValue &v)
{
  NS_LOG_FUNCTION (this);
  m_ueNetDeviceFactory.Set (n, v);
}

void 
NistLteHelper::SetUeAntennaModelType (std::string type)
{
  NS_LOG_FUNCTION (this);
  m_ueAntennaModelFactory.SetTypeId (type);
}

void 
NistLteHelper::SetUeAntennaModelAttribute (std::string n, const AttributeValue &v)
{
  NS_LOG_FUNCTION (this);
  m_ueAntennaModelFactory.Set (n, v);
}

void 
NistLteHelper::SetFadingModel (std::string type) 
{
  NS_LOG_FUNCTION (this << type);
  m_fadingModelType = type;
  if (!type.empty ())
    {
      m_fadingModelFactory = ObjectFactory ();
      m_fadingModelFactory.SetTypeId (type);
    }
}

void 
NistLteHelper::SetFadingModelAttribute (std::string n, const AttributeValue &v)
{
  m_fadingModelFactory.Set (n, v);
}

void 
NistLteHelper::SetSpectrumChannelType (std::string type) 
{
  NS_LOG_FUNCTION (this << type);
  m_channelFactory.SetTypeId (type);
}

void 
NistLteHelper::SetSpectrumChannelAttribute (std::string n, const AttributeValue &v)
{
  m_channelFactory.Set (n, v);
}


NetDeviceContainer
NistLteHelper::InstallUeDevice (NodeContainer c)
{
  NS_LOG_FUNCTION (this);
  NetDeviceContainer devices;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      Ptr<Node> node = *i;
      Ptr<NetDevice> device = InstallSingleUeDevice (node);
      devices.Add (device);
    }
  return devices;
}


Ptr<NetDevice>
NistLteHelper::InstallSingleUeDevice (Ptr<Node> n)
{
  NS_LOG_FUNCTION (this);
  Ptr<NrV2XSpectrumPhy> dlPhy = CreateObject<NrV2XSpectrumPhy> ();
  Ptr<NrV2XSpectrumPhy> ulPhy = CreateObject<NrV2XSpectrumPhy> ();
  Ptr<NrV2XSpectrumPhy> slPhy;
  if (m_useSidelink || m_useDiscovery) {  //We use Sidelink for C-V2X Mode 4 operations
    slPhy = CreateObject<NrV2XSpectrumPhy> ();
    slPhy->SetAttribute ("HalfDuplexPhy", PointerValue (ulPhy));  
   }

  Ptr<NrV2XUePhy> phy = CreateObject<NrV2XUePhy> (dlPhy, ulPhy);
  if (m_useSidelink || m_useDiscovery) {
    phy->SetSlSpectrumPhy (slPhy);
  }

  Ptr<NistLteHarqPhy> harq = Create<NistLteHarqPhy> ();
  dlPhy->SetHarqPhyModule (harq);
  ulPhy->SetHarqPhyModule (harq);
  if (m_useSidelink || m_useDiscovery) {
    slPhy->SetHarqPhyModule (harq);
  }
  phy->SetHarqPhyModule (harq);

  Ptr<NistLteChunkProcessor> pRs = Create<NistLteChunkProcessor> ();
  pRs->AddCallback (MakeCallback (&NrV2XUePhy::ReportRsReceivedPower, phy));
  dlPhy->AddRsPowerChunkProcessor (pRs);

  Ptr<NistLteChunkProcessor> pInterf = Create<NistLteChunkProcessor> ();
  pInterf->AddCallback (MakeCallback (&NrV2XUePhy::ReportInterference, phy));
  dlPhy->AddInterferenceCtrlChunkProcessor (pInterf); // for RSRQ evaluation of UE Measurements

  Ptr<NistLteChunkProcessor> pCtrl = Create<NistLteChunkProcessor> ();
  pCtrl->AddCallback (MakeCallback (&NrV2XSpectrumPhy::UpdateSinrPerceived, dlPhy));
  dlPhy->AddCtrlSinrChunkProcessor (pCtrl);

  Ptr<NistLteChunkProcessor> pData = Create<NistLteChunkProcessor> ();
  pData->AddCallback (MakeCallback (&NrV2XSpectrumPhy::UpdateSinrPerceived, dlPhy));
  dlPhy->AddDataSinrChunkProcessor (pData);

  if (m_useSidelink || m_useDiscovery) {  // for Sidelink
    Ptr<NistLteSlChunkProcessor> pSlSinr = Create<NistLteSlChunkProcessor> ();
    pSlSinr->AddCallback (MakeCallback (&NrV2XSpectrumPhy::UpdateSlSinrPerceived, slPhy));
    slPhy->AddSlSinrChunkProcessor (pSlSinr);
    
    Ptr<NistLteSlChunkProcessor> pSlSignal = Create<NistLteSlChunkProcessor> ();
    pSlSignal->AddCallback (MakeCallback (&NrV2XSpectrumPhy::UpdateSlSigPerceived, slPhy));
    slPhy->AddSlSignalChunkProcessor (pSlSignal);

    Ptr<NistLteSlChunkProcessor> pSlInterference = Create<NistLteSlChunkProcessor> ();
    pSlInterference->AddCallback (MakeCallback (&NrV2XSpectrumPhy::UpdateSlIntPerceived, slPhy));
    slPhy->AddSlInterferenceChunkProcessor (pSlInterference);
    /*
    Ptr<NistLteChunkProcessor> pData = Create<NistLteChunkProcessor> ();
    pData->AddCallback (MakeCallback (&NrV2XSpectrumPhy::UpdateSinrPerceived, slPhy));
    slPhy->AddDataSinrChunkProcessor (pData);
    */
  }
  
  if (m_usePdschForCqiGeneration)
    {
      // CQI calculation based on PDCCH for signal and PDSCH for interference
      pCtrl->AddCallback (MakeCallback (&NrV2XUePhy::GenerateMixedCqiReport, phy));
      Ptr<NistLteChunkProcessor> pDataInterf = Create<NistLteChunkProcessor> ();      
      pDataInterf->AddCallback (MakeCallback (&NrV2XUePhy::ReportDataInterference, phy));
      dlPhy->AddInterferenceDataChunkProcessor (pDataInterf);
    }
  else
    {
      // CQI calculation based on PDCCH for both signal and interference
      pCtrl->AddCallback (MakeCallback (&NrV2XUePhy::GenerateCtrlCqiReport, phy));
    }



  dlPhy->SetChannel (m_downlinkChannel);
  ulPhy->SetChannel (m_uplinkChannel);
  if (m_useSidelink || m_useDiscovery) {
    slPhy->SetChannel (m_uplinkChannel); //the sidelink channel is actually modelled as an uplink channel from the PHY Layer point of view
  }


  Ptr<MobilityModel> mm = n->GetObject<MobilityModel> ();  // mm is a DUMB name 
  NS_ASSERT_MSG (mm, "MobilityModel needs to be set on node before calling NistLteHelper::InstallUeDevice ()");
  dlPhy->SetMobility (mm);
  ulPhy->SetMobility (mm);
  if (m_useSidelink || m_useDiscovery) {
    slPhy->SetMobility (mm);
  }

  Ptr<AntennaModel> antenna = (m_ueAntennaModelFactory.Create ())->GetObject<AntennaModel> ();
  NS_ASSERT_MSG (antenna, "error in creating the AntennaModel object");
  dlPhy->SetAntenna (antenna);
  ulPhy->SetAntenna (antenna);
  if (m_useSidelink || m_useDiscovery) {
    slPhy->SetAntenna (antenna);
  }

  // create UE mac
  Ptr<NrV2XUeMac> mac = CreateObjectWithAttributes<NrV2XUeMac> ();
  // create UE rrc
  Ptr<NistLteUeRrc> rrc = CreateObject<NistLteUeRrc> ();

  if (m_useIdealRrc)  // default value = true
    {
      Ptr<NistLteUeRrcProtocolIdeal> rrcProtocol = CreateObject<NistLteUeRrcProtocolIdeal> ();  // Models the transmission of RRC messages from the UE to the eNB in an ideal fashion, without errors and without consuming any radio resources
      rrcProtocol->SetUeRrc (rrc);
      rrc->AggregateObject (rrcProtocol);
      rrcProtocol->SetNistLteUeRrcSapProvider (rrc->GetNistLteUeRrcSapProvider ());
      rrc->SetNistLteUeRrcSapUser (rrcProtocol->GetNistLteUeRrcSapUser ());
    }
  else
    {
       NS_FATAL_ERROR("Only ideal RRC is available");
    }
  
  if (m_epcHelper != 0)
    {
      rrc->SetUseRlcSm (false);
    }
  Ptr<NistEpcUeNas> nas = CreateObject<NistEpcUeNas> ();
 
  nas->SetAsSapProvider (rrc->GetAsSapProvider ());
  rrc->SetAsSapUser (nas->GetAsSapUser ());

  rrc->SetNistLteUeCmacSapProvider (mac->GetNistLteUeCmacSapProvider ());
  mac->SetNistLteUeCmacSapUser (rrc->GetNistLteUeCmacSapUser ());
  rrc->SetNistLteMacSapProvider (mac->GetNistLteMacSapProvider ());

  phy->SetNistLteUePhySapUser (mac->GetNistLteUePhySapUser ());
  mac->SetNistLteUePhySapProvider (phy->GetNistLteUePhySapProvider ());

  phy->SetNistLteUeCphySapUser (rrc->GetNistLteUeCphySapUser ());
  rrc->SetNistLteUeCphySapProvider (phy->GetNistLteUeCphySapProvider ());
  mac->SetNistLteUeCphySapProvider (phy->GetNistLteUeCphySapProvider ());
  
  NS_ABORT_MSG_IF (m_imsiCounter >= 0xFFFFFFFF, "max num UEs exceeded");
  uint64_t imsi = ++m_imsiCounter;

  //Initialize sidelink configuration
  Ptr<LteUeRrcSl> ueSidelinkConfiguration = CreateObject<LteUeRrcSl> ();
  ueSidelinkConfiguration->SetSourceL2Id ((uint32_t) (imsi & 0xFFFFFF)); //use lower 24 bits of IMSI as source
  rrc->SetAttribute ("SidelinkConfiguration", PointerValue (ueSidelinkConfiguration));
  
  Ptr<NistLteUeNetDevice> dev = m_ueNetDeviceFactory.Create<NistLteUeNetDevice> ();
  dev->SetNode (n);
  dev->SetAttribute ("Imsi", UintegerValue (imsi));
  dev->SetAttribute ("NrV2XUePhy", PointerValue (phy));
  dev->SetAttribute ("NrV2XUeMac", PointerValue (mac));
  dev->SetAttribute ("NistLteUeRrc", PointerValue (rrc));
  dev->SetAttribute ("NistEpcUeNas", PointerValue (nas));

  phy->SetDevice (dev);
  dlPhy->SetDevice (dev);
  ulPhy->SetDevice (dev);
  if (m_useSidelink || m_useDiscovery) {
    slPhy->SetDevice (dev);
  }
  nas->SetDevice (dev);

  n->AddDevice (dev);
  dlPhy->SetNistLtePhyRxDataEndOkCallback (MakeCallback (&NrV2XUePhy::PhyPduReceived, phy));
  dlPhy->SetNistLtePhyRxCtrlEndOkCallback (MakeCallback (&NrV2XUePhy::ReceiveNistLteControlMessageList, phy));
  dlPhy->SetNistLtePhyRxPssCallback (MakeCallback (&NrV2XUePhy::ReceivePss, phy));
  dlPhy->SetNistLtePhyDlHarqFeedbackCallback (MakeCallback (&NrV2XUePhy::ReceiveLteDlHarqFeedback, phy));
  nas->SetForwardUpCallback (MakeCallback (&NistLteUeNetDevice::Receive, dev));

  if (m_useSidelink || m_useDiscovery) {  // Sidelink configuration for Mode 4 operations :)
    slPhy->SetUnimoreReportRssiCallback (MakeCallback (&NrV2XUePhy::UnimoreReceivedRssi, phy));
    slPhy->SetNistLtePhyRxDataEndOkCallback (MakeCallback (&NrV2XUePhy::PhyPduReceived, phy));
    slPhy->SetNistLtePhyRxCtrlEndOkCallback (MakeCallback (&NrV2XUePhy::ReceiveNistLteControlMessageList, phy));
    slPhy->SetNistLtePhyRxSlssCallback (MakeCallback (&NrV2XUePhy::ReceiveSlss, phy));

  // TODO FIXME NEW for V2X 

    slPhy->SetNistLtePhyRxDataStartCallback (MakeCallback (&NrV2XUePhy::MeasurePsschRsrp, phy));
  }
  
  if (m_epcHelper != 0)
    {
      m_epcHelper->AddUe (dev, dev->GetImsi ());
    }

  dev->Initialize ();

  return dev;
}


void
NistLteHelper::Attach (NetDeviceContainer ueDevices)
{
  NS_LOG_FUNCTION (this);
  for (NetDeviceContainer::Iterator i = ueDevices.Begin (); i != ueDevices.End (); ++i)
    {
      Attach (*i);
    }
}

void
NistLteHelper::Attach (Ptr<NetDevice> ueDevice)
{
  NS_LOG_FUNCTION (this);

  if (m_epcHelper == 0)
    {
      NS_FATAL_ERROR ("This function is not valid without properly configured EPC");
    }

  Ptr<NistLteUeNetDevice> ueLteDevice = ueDevice->GetObject<NistLteUeNetDevice> ();
  if (ueLteDevice == 0)
    {
      NS_FATAL_ERROR ("The passed NetDevice must be an NistLteUeNetDevice");
    }

  // initiate cell selection
  Ptr<NistEpcUeNas> ueNas = ueLteDevice->GetNas ();
  NS_ASSERT (ueNas != 0);
  uint16_t dlEarfcn = ueLteDevice->GetDlEarfcn ();
  ueNas->StartCellSelection (dlEarfcn);

  // instruct UE to immediately enter CONNECTED mode after camping
  ueNas->Connect ();

  // activate default EPS bearer
  m_epcHelper->ActivateNistEpsBearer (ueDevice, ueLteDevice->GetImsi (),
                                  NistEpcTft::Default (),
                                  NistEpsBearer (NistEpsBearer::NGBR_VIDEO_TCP_DEFAULT));
}


uint8_t
NistLteHelper::ActivateDedicatedNistEpsBearer (NetDeviceContainer ueDevices, NistEpsBearer bearer, Ptr<NistEpcTft> tft)
{
  NS_LOG_FUNCTION (this);
  for (NetDeviceContainer::Iterator i = ueDevices.Begin (); i != ueDevices.End (); ++i)
    {
      uint8_t bearerId = ActivateDedicatedNistEpsBearer (*i, bearer, tft);
      return bearerId;
    }
  return 0;
}


uint8_t
NistLteHelper::ActivateDedicatedNistEpsBearer (Ptr<NetDevice> ueDevice, NistEpsBearer bearer, Ptr<NistEpcTft> tft)
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT_MSG (m_epcHelper != 0, "dedicated EPS bearers cannot be set up when the EPC is not used");

  uint64_t imsi = ueDevice->GetObject<NistLteUeNetDevice> ()->GetImsi ();
  uint8_t bearerId = m_epcHelper->ActivateNistEpsBearer (ueDevice, imsi, tft, bearer);
  return bearerId;
}

void
NistLteHelper::ActivateSidelinkBearer (NetDeviceContainer ueDevices, Ptr<NistSlTft> tft)
{
  NS_LOG_FUNCTION (this);
  for (NetDeviceContainer::Iterator i = ueDevices.Begin (); i != ueDevices.End (); ++i)
    {
      ActivateSidelinkBearer (*i, tft->Copy());
    }
}


void
NistLteHelper::ActivateSidelinkBearer (Ptr<NetDevice> ueDevice, Ptr<NistSlTft> tft)
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT_MSG (m_epcHelper != 0, "sidelink bearers cannot be set up when the EPC is not used");

  m_epcHelper->ActivateSidelinkBearer (ueDevice, tft);
}  

void
NistLteHelper::DeactivateSidelinkBearer (NetDeviceContainer ueDevices, Ptr<NistSlTft> tft)
{
  NS_LOG_FUNCTION (this);
  for (NetDeviceContainer::Iterator i = ueDevices.Begin (); i != ueDevices.End (); ++i)
    {
      DeactivateSidelinkBearer (*i, tft);
    }
}


void
NistLteHelper::DeactivateSidelinkBearer (Ptr<NetDevice> ueDevice, Ptr<NistSlTft> tft)
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT_MSG (m_epcHelper != 0, "sidelink bearers cannot be set up when the EPC is not used");

  m_epcHelper->DeactivateSidelinkBearer (ueDevice, tft);
}  
void
NistLteHelper::StartDiscovery (NetDeviceContainer ueDevices, std::list<uint32_t> apps, bool rxtx) // Rel' 12
{
  NS_LOG_FUNCTION (this);
  for (NetDeviceContainer::Iterator i = ueDevices.Begin (); i != ueDevices.End (); ++i)
    {
      StartDiscovery (*i, apps, rxtx);
    }
}

void
NistLteHelper::StartDiscovery (Ptr<NetDevice> ueDevice, std::list<uint32_t> apps, bool rxtx) // Rel' 12
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT_MSG (m_epcHelper != 0, "discovery can't start when the EPC is not used");

  m_epcHelper->StartDiscovery (ueDevice, apps, rxtx);
}  

void
NistLteHelper::StopDiscovery (NetDeviceContainer ueDevices, std::list<uint32_t> apps, bool rxtx) // Rel' 12
{
  NS_LOG_FUNCTION (this);
  for (NetDeviceContainer::Iterator i = ueDevices.Begin (); i != ueDevices.End (); ++i)
    {
      StopDiscovery (*i, apps, rxtx);
    }
}

void
NistLteHelper::StopDiscovery (Ptr<NetDevice> ueDevice, std::list<uint32_t> apps, bool rxtx) // Rel' 12
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT_MSG (m_epcHelper != 0, "no EPC is used");

  m_epcHelper->StopDiscovery (ueDevice, apps, rxtx);
}



void
NistLteHelper::EnableLogComponents (void)
{
  LogComponentEnable ("NistLteHelper", LOG_LEVEL_ALL);
  LogComponentEnable ("NistLteUeRrc", LOG_LEVEL_ALL);
  LogComponentEnable ("NrV2XUeMac", LOG_LEVEL_ALL);
  LogComponentEnable ("NistLteRlc", LOG_LEVEL_ALL);
  LogComponentEnable ("NistLteRlcUm", LOG_LEVEL_ALL);
  LogComponentEnable ("NistLteRlcAm", LOG_LEVEL_ALL);

  LogComponentEnable ("NistLtePhy", LOG_LEVEL_ALL);
  LogComponentEnable ("NrV2XUePhy", LOG_LEVEL_ALL);
  LogComponentEnable ("NistLteSpectrumValueHelper", LOG_LEVEL_ALL);
  LogComponentEnable ("NrV2XSpectrumPhy", LOG_LEVEL_ALL);
  LogComponentEnable ("NistLteInterference", LOG_LEVEL_ALL);
  LogComponentEnable ("NistLteChunkProcessor", LOG_LEVEL_ALL);

  std::string propModelStr = m_dlPathlossModelFactory.GetTypeId ().GetName ().erase (0,5).c_str ();
  LogComponentEnable ("NistLteNetDevice", LOG_LEVEL_ALL);
  LogComponentEnable ("NistLteUeNetDevice", LOG_LEVEL_ALL);

}


  /**
   * Deploys the Sidelink configuration to the UEs
   * \param ueDevices List of devices where to configure sidelink
   * \param slConfiguration Sidelink configuration
   */
void
NistLteHelper::InstallSidelinkConfiguration (NetDeviceContainer ueDevices, Ptr<LteUeRrcSl> slConfiguration)
{
  //for each device, install a copy of the configuration
  NS_LOG_FUNCTION (this);
  for (NetDeviceContainer::Iterator i = ueDevices.Begin (); i != ueDevices.End (); ++i)
    {
      InstallSidelinkConfiguration (*i, slConfiguration);
    }
}

  /**
   * Deploys the Sidelink configuration to the UE
   * \param ueDevice The UE to configure
   * \param slConfiguration Sidelink configuration
   */
void
NistLteHelper::InstallSidelinkConfiguration (Ptr<NetDevice> ueDevice, Ptr<LteUeRrcSl> slConfiguration)
{
  Ptr<NistLteUeRrc> rrc = ueDevice->GetObject<NistLteUeNetDevice> ()->GetRrc();
  NS_ASSERT_MSG (rrc != 0, "RRC layer not found");
  PointerValue ptr;
  rrc->GetAttribute ("SidelinkConfiguration", ptr);
  Ptr<LteUeRrcSl> ueConfig = ptr.Get<LteUeRrcSl> ();
  ueConfig->SetSlPreconfiguration (slConfiguration->GetSlPreconfiguration());
  ueConfig->SetSlEnabled (slConfiguration->IsSlEnabled());
  ueConfig->SetDiscEnabled (slConfiguration->IsDiscEnabled());
  ueConfig->SetDiscTxResources (slConfiguration->GetDiscTxResources ());
  ueConfig->SetDiscInterFreq (slConfiguration->GetDiscInterFreq ());
}

/**
 * Compute the RSRP between the given nodes for the given propagation loss model
 * This code is derived from the multi-model-spectrum-channel class
 * \param propagationLoss The loss model
 * \param psd The power spectral density of the transmitter
 * \param txPhy The transmitter
 * \param rxPhy The receiver
 * \return The RSRP 
 */
double
NistLteHelper::DoCalcRsrp (Ptr<PropagationLossModel> propagationLoss, Ptr<SpectrumValue> psd, Ptr<SpectrumPhy> txPhy, Ptr<SpectrumPhy> rxPhy)
{
  Ptr<MobilityModel> txMobility = txPhy->GetMobility ();
  Ptr<MobilityModel> rxMobility = rxPhy->GetMobility ();

  double pathLossDb = 0;
  if (txPhy->GetRxAntenna() != 0)
    {
      Angles txAngles (rxMobility->GetPosition (), txMobility->GetPosition ());
      double txAntennaGain = txPhy->GetRxAntenna()->GetGainDb (txAngles);
      NS_LOG_DEBUG ("txAntennaGain = " << txAntennaGain << " dB");
      pathLossDb -= txAntennaGain;
    }
  Ptr<AntennaModel> rxAntenna = rxPhy->GetRxAntenna ();
  if (rxAntenna != 0)
    {
      Angles rxAngles (txMobility->GetPosition (), rxMobility->GetPosition ());
      double rxAntennaGain = rxAntenna->GetGainDb (rxAngles);
      NS_LOG_DEBUG ("rxAntennaGain = " << rxAntennaGain << " dB");
      pathLossDb -= rxAntennaGain;
    }
  if (propagationLoss)
    {
      double propagationGainDb = propagationLoss->CalcRxPower (0, txMobility, rxMobility);
      NS_LOG_DEBUG ("propagationGainDb = " << propagationGainDb << " dB");
      pathLossDb -= propagationGainDb;
    }                    
  NS_LOG_DEBUG ("total pathLoss = " << pathLossDb << " dB");  

  double pathGainLinear = std::pow (10.0, (-pathLossDb) / 10.0);
  Ptr<SpectrumValue> rxPsd = Copy<SpectrumValue> (psd);
  *rxPsd *= pathGainLinear;

  // RSRP evaluated as averaged received power among RBs
  double sum = 0.0;
  uint8_t rbNum = 0;
  Values::const_iterator it;
  for (it = (*rxPsd).ValuesBegin (); it != (*rxPsd).ValuesEnd (); it++)
    {
      //The non active RB will be set to -inf
      //We count only the active
      if((*it))
        {          
          // convert PSD [W/Hz] to linear power [W] for the single RE
          // we consider only one RE for the RS since the channel is 
          // flat within the same RB 
          double powerTxW = ((*it) * 180000.0) / 12.0;
          sum += powerTxW;
          rbNum++;
        }
    }
  double rsrp = (rbNum > 0) ? (sum / rbNum) : DBL_MAX;

  NS_LOG_INFO ("RSRP linear=" << rsrp << " (" << 10 * std::log10 (rsrp) + 30 << "dBm)");
   
  std::ofstream testfile;
  testfile.open("pathloss.log", std::ios_base::app);
  testfile << 10 * std::log10 (rsrp) + 30 << "\r\n";
  testfile.close();

  return 10 * std::log10 (rsrp) + 30;
}

/**
 * Compute the RSRP between the given nodes for the given propagation loss model
 * This code is derived from the multi-model-spectrum-channel class
 * \param propagationLoss The loss model
 * \param txPower The transmit power
 * \param txPhy The transmitter
 * \param rxPhy The receiver
 * \return The RSRP 
 */
double
NistLteHelper::DoCalcRsrp (Ptr<PropagationLossModel> propagationLoss, double txPower, Ptr<SpectrumPhy> txPhy, Ptr<SpectrumPhy> rxPhy)
{
  Ptr<MobilityModel> txMobility = txPhy->GetMobility ();
  Ptr<MobilityModel> rxMobility = rxPhy->GetMobility ();

  double pathLossDb = 0;
  if (txPhy->GetRxAntenna() != 0)
    {
      Angles txAngles (rxMobility->GetPosition (), txMobility->GetPosition ());
      double txAntennaGain = txPhy->GetRxAntenna()->GetGainDb (txAngles);
      NS_LOG_DEBUG ("txAntennaGain = " << txAntennaGain << " dB");
      pathLossDb -= txAntennaGain;
    }
  Ptr<AntennaModel> rxAntenna = rxPhy->GetRxAntenna ();
  if (rxAntenna != 0)
    {
      Angles rxAngles (txMobility->GetPosition (), rxMobility->GetPosition ());
      double rxAntennaGain = rxAntenna->GetGainDb (rxAngles);
      NS_LOG_DEBUG ("rxAntennaGain = " << rxAntennaGain << " dB");
      pathLossDb -= rxAntennaGain;
    }
  if (propagationLoss)
    {
      double propagationGainDb = propagationLoss->CalcRxPower (0, txMobility, rxMobility);
      NS_LOG_DEBUG ("propagationGainDb = " << propagationGainDb << " dB");
      pathLossDb -= propagationGainDb;
    }                    
  NS_LOG_DEBUG ("total pathLoss = " << pathLossDb << " dB");  
  
  double rsrp = txPower - pathLossDb;

  NS_LOG_INFO ("RSRP=" << rsrp << " dBm");

  return rsrp;
}


/**
 * Computes the S-RSRP between 2 UEs. Information about the uplink frequency and band is necessary  to be able to call the function before the simulation starts.
 * \param txPower Transmit power for the reference signal
 * \param ulEarfcn Uplink frequency
 * \param ulBandwidth Uplink bandwidth
 * \param txDevice Transmitter UE
 * \param rxDevice Receiver UE
 * \return RSRP value
 */
double
NistLteHelper::CalcSidelinkRsrp (double txPower, double ulEarfcn, double ulBandwidth, Ptr<NetDevice> txDevice, Ptr<NetDevice> rxDevice)
{
  Ptr<PropagationLossModel> lossModel = m_uplinkPathlossModel->GetObject<PropagationLossModel> ();
  NS_ASSERT_MSG (lossModel != 0, " " << m_uplinkPathlossModel << " is not a PropagationLossModel");

  /*
    Sidelink Reference Signal Received Power (S-RSRP) is defined as the linear average over the
    power contributions (in [W]) of the resource elements that carry demodulation reference signals
    associated with PSBCH, within the central 6 PRBs of the applicable subframes. 
  */
  //This method returned very low values of RSRP 
  std::vector <int> rbMask;
  for (int i = 22; i < 28 ; i++)
    {
      rbMask.push_back (i);
    }
  NistLteSpectrumValueHelper psdHelper;
  Ptr<SpectrumValue> psd = psdHelper.CreateUlTxPowerSpectralDensity (ulEarfcn, ulBandwidth, txPower, rbMask, 1.0, 15, 4, false);
  
  double rsrp= DoCalcRsrp (lossModel, psd, txDevice->GetObject<NistLteUeNetDevice> ()->GetPhy()->GetUlSpectrumPhy (), rxDevice->GetObject<NistLteUeNetDevice> ()->GetPhy()->GetUlSpectrumPhy ());
  
  NS_LOG_INFO ("S-RSRP=" << rsrp);

  return rsrp;
}

/**
 * Computes the RSRP between a transmitter UE and a receiver UE as defined in TR 36.843. Information about the uplink frequency and band is necessary  to be able to call the function before the simulation starts. 
 * \param txPower Transmit power for the reference signal
 * \param ulEarfcn Uplink frequency
 * \param txDevice Transmitter UE
 * \param rxDevice Receiver UE
 * \return RSRP value
 */
double
NistLteHelper::CalcSidelinkRsrpEval (double txPower, double ulEarfcn, Ptr<NetDevice> txDevice, Ptr<NetDevice> rxDevice)
{
  Ptr<PropagationLossModel> lossModel = m_uplinkPathlossModel->GetObject<PropagationLossModel> ();
  NS_ASSERT_MSG (lossModel != 0, " " << m_uplinkPathlossModel << " is not a PropagationLossModel");

  /*
    36.843: RSRP is calculated for transmit power of 23dBm by the transmitter UE and is the received power at the receiver UE calculated after accounting for large scale path loss and shadowing. Additionally note that wrap around is used for path loss calculations except for the case of partial -coverage.
  */
  double rsrp= DoCalcRsrp (lossModel, txPower, txDevice->GetObject<NistLteUeNetDevice> ()->GetPhy()->GetUlSpectrumPhy (), rxDevice->GetObject<NistLteUeNetDevice> ()->GetPhy()->GetUlSpectrumPhy ());
  
  NS_LOG_INFO ("RSRP=" << rsrp);

  return rsrp;
}


} // namespace ns3
