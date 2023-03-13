/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011-2013 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
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
 * Author: Jaume Nin <jnin@cttc.es>
 *         Nicola Baldo <nbaldo@cttc.es>
 *         Manuel Requena <manuel.requena@cttc.es>
 * Modified by: NIST
 * Modified by: Luca Lusvarghi <luca.lusvarghi5@unimore.it>
 */

#include <ns3/nist-point-to-point-epc-helper.h>
#include <ns3/log.h>
#include <ns3/inet-socket-address.h>
#include <ns3/mac48-address.h>
#include <ns3/nist-eps-bearer.h>
#include <ns3/ipv4-address.h>
#include <ns3/internet-stack-helper.h>
#include <ns3/point-to-point-helper.h>
#include <ns3/packet-socket-helper.h>
#include <ns3/packet-socket-address.h>
#include <ns3/nist-epc-sgw-pgw-application.h>
#include <ns3/nr-v2x-ue-net-device.h>
#include <ns3/nist-epc-mme.h>
#include <ns3/nist-epc-ue-nas.h>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("NistPointToPointEpcHelper");

NS_OBJECT_ENSURE_REGISTERED (NistPointToPointEpcHelper);


NistPointToPointEpcHelper::NistPointToPointEpcHelper () 
  : m_gtpuUdpPort (2152)  // fixed by the standard
{
  NS_LOG_FUNCTION (this);

  // since we use point-to-point links for all S1-U links, 
  // we use a /30 subnet which can hold exactly two addresses 
  // (remember that net broadcast and null address are not valid)
  m_s1uIpv4AddressHelper.SetBase ("10.0.0.0", "255.255.255.252");


  // we use a /8 net for all UEs
  m_ueAddressHelper.SetBase ("7.0.0.0", "255.0.0.0");
  
  // create SgwPgwNode
  m_sgwPgw = CreateObject<Node> ();
  InternetStackHelper internet;
  internet.Install (m_sgwPgw);
  
  // create S1-U socket
  Ptr<Socket> sgwPgwS1uSocket = Socket::CreateSocket (m_sgwPgw, TypeId::LookupByName ("ns3::UdpSocketFactory"));
  int retval = sgwPgwS1uSocket->Bind (InetSocketAddress (Ipv4Address::GetAny (), m_gtpuUdpPort));
  NS_ASSERT (retval == 0);

  // create TUN device implementing tunneling of user data over GTP-U/UDP/IP 
  m_tunDevice = CreateObject<VirtualNetDevice> ();
  // allow jumbo packets
  m_tunDevice->SetAttribute ("Mtu", UintegerValue (30000));

  // yes we need this
  m_tunDevice->SetAddress (Mac48Address::Allocate ()); 

  m_sgwPgw->AddDevice (m_tunDevice);
  NetDeviceContainer tunDeviceContainer;
  tunDeviceContainer.Add (m_tunDevice);
  
  // the TUN device is on the same subnet as the UEs, so when a packet
  // addressed to an UE arrives at the intenet to the WAN interface of
  // the PGW it will be forwarded to the TUN device. 
  Ipv4InterfaceContainer tunDeviceIpv4IfContainer = m_ueAddressHelper.Assign (tunDeviceContainer);  

  // create NistEpcSgwPgwApplication
  m_sgwPgwApp = CreateObject<NistEpcSgwPgwApplication> (m_tunDevice, sgwPgwS1uSocket);
  m_sgwPgw->AddApplication (m_sgwPgwApp);
  
  // connect SgwPgwApplication and virtual net device for tunneling
  m_tunDevice->SetSendCallback (MakeCallback (&NistEpcSgwPgwApplication::RecvFromTunDevice, m_sgwPgwApp));

  // Create MME and connect with SGW via S11 interface
  m_mme = CreateObject<NistEpcMme> ();
  m_mme->SetS11SapSgw (m_sgwPgwApp->GetS11SapSgw ());
  m_sgwPgwApp->SetS11SapMme (m_mme->GetS11SapMme ());
}

NistPointToPointEpcHelper::~NistPointToPointEpcHelper ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
NistPointToPointEpcHelper::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::NistPointToPointEpcHelper")
    .SetParent<NistEpcHelper> ()
    .AddConstructor<NistPointToPointEpcHelper> ()
    .AddAttribute ("S1uLinkDataRate", 
                   "The data rate to be used for the next S1-U link to be created",
                   DataRateValue (DataRate ("10Gb/s")),
                   MakeDataRateAccessor (&NistPointToPointEpcHelper::m_s1uLinkDataRate),
                   MakeDataRateChecker ())
    .AddAttribute ("S1uLinkDelay", 
                   "The delay to be used for the next S1-U link to be created",
                   TimeValue (Seconds (0)),
                   MakeTimeAccessor (&NistPointToPointEpcHelper::m_s1uLinkDelay),
                   MakeTimeChecker ())
    .AddAttribute ("S1uLinkMtu", 
                   "The MTU of the next S1-U link to be created. Note that, because of the additional GTP/UDP/IP tunneling overhead, you need a MTU larger than the end-to-end MTU that you want to support.",
                   UintegerValue (2000),
                   MakeUintegerAccessor (&NistPointToPointEpcHelper::m_s1uLinkMtu),
                   MakeUintegerChecker<uint16_t> ())
  ;
  return tid;
}

void
NistPointToPointEpcHelper::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_tunDevice->SetSendCallback (MakeNullCallback<bool, Ptr<Packet>, const Address&, const Address&, uint16_t> ());
  m_tunDevice = 0;
  m_sgwPgwApp = 0;  
  m_sgwPgw->Dispose ();
}



void 
NistPointToPointEpcHelper::AddUe (Ptr<NetDevice> ueDevice, uint64_t imsi)
{
  NS_LOG_FUNCTION (this << imsi << ueDevice );
  
  m_mme->AddUe (imsi);
  m_sgwPgwApp->AddUe (imsi);
  

}

uint8_t
NistPointToPointEpcHelper::ActivateNistEpsBearer (Ptr<NetDevice> ueDevice, uint64_t imsi, Ptr<NistEpcTft> tft, NistEpsBearer bearer)
{
  NS_LOG_FUNCTION (this << ueDevice << imsi);

  // we now retrieve the IPv4 address of the UE and notify it to the SGW;
  // we couldn't do it before since address assignment is triggered by
  // the user simulation program, rather than done by the EPC   
  Ptr<Node> ueNode = ueDevice->GetNode (); 
  Ptr<Ipv4> ueIpv4 = ueNode->GetObject<Ipv4> ();
  NS_ASSERT_MSG (ueIpv4 != 0, "UEs need to have IPv4 installed before EPS bearers can be activated");
  int32_t interface =  ueIpv4->GetInterfaceForDevice (ueDevice);
  NS_ASSERT (interface >= 0);
  NS_ASSERT (ueIpv4->GetNAddresses (interface) == 1);
  Ipv4Address ueAddr = ueIpv4->GetAddress (interface, 0).GetLocal ();
  NS_LOG_LOGIC (" UE IP address: " << ueAddr);  m_sgwPgwApp->SetUeAddress (imsi, ueAddr);
  
  uint8_t bearerId = m_mme->AddBearer (imsi, tft, bearer);
  Ptr<NistLteUeNetDevice> ueLteDevice = ueDevice->GetObject<NistLteUeNetDevice> ();
  if (ueLteDevice)
    {
      ueLteDevice->GetNas ()->ActivateNistEpsBearer (bearer, tft);
    }
  return bearerId;
}

void
NistPointToPointEpcHelper::ActivateSidelinkBearer (Ptr<NetDevice> ueDevice, Ptr<NistSlTft> tft)
{
  NS_LOG_FUNCTION (this << ueDevice);

  Ptr<NistLteUeNetDevice> ueLteDevice = ueDevice->GetObject<NistLteUeNetDevice> ();
  NS_ASSERT (ueLteDevice);
  ueLteDevice->GetNas ()->ActivateSidelinkBearer (tft);
}

void
NistPointToPointEpcHelper::DeactivateSidelinkBearer (Ptr<NetDevice> ueDevice, Ptr<NistSlTft> tft)
{
  NS_LOG_FUNCTION (this << ueDevice);

  Ptr<NistLteUeNetDevice> ueLteDevice = ueDevice->GetObject<NistLteUeNetDevice> ();
  NS_ASSERT (ueLteDevice);
  ueLteDevice->GetNas ()->DeactivateSidelinkBearer (tft);
}

void
NistPointToPointEpcHelper::StartDiscovery (Ptr<NetDevice> ueDevice, std::list<uint32_t> apps, bool rxtx)
{
  NS_LOG_FUNCTION (this << ueDevice);
  ueDevice->GetObject<NistLteUeNetDevice> ()->GetNas ()->AddDiscoveryApps (apps, rxtx);
}

void
NistPointToPointEpcHelper::StopDiscovery (Ptr<NetDevice> ueDevice, std::list<uint32_t> apps, bool rxtx)
{
  NS_LOG_FUNCTION (this << ueDevice);
  ueDevice->GetObject<NistLteUeNetDevice> ()->GetNas ()->RemoveDiscoveryApps (apps, rxtx);
}


  
Ptr<Node>
NistPointToPointEpcHelper::GetPgwNode ()
{
  return m_sgwPgw;
}


Ipv4InterfaceContainer 
NistPointToPointEpcHelper::AssignUeIpv4Address (NetDeviceContainer ueDevices)
{
  return m_ueAddressHelper.Assign (ueDevices);
}



Ipv4Address
NistPointToPointEpcHelper::GetUeDefaultGatewayAddress ()
{
  // return the address of the tun device
  return m_sgwPgw->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal ();
}


} // namespace ns3
