/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * This software was developed at the National Institute of Standards and
 * Technology by employees of the Federal Government in the course of
 * their official duties. Pursuant to titleElement 17 Section 105 of the United
 * States Code this software is not subject to copyright protection and
 * is in the public domain.
 * NIST assumes no responsibility whatsoever for its use by other parties,
 * and makes no guarantees, expressed or implied, about its quality,
 * reliability, or any other characteristic.
 * 
 * We would appreciate acknowledgement if the software is used.
 * 
 * NIST ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS" CONDITION AND
 * DISCLAIM ANY LIABILITY OF ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING
 * FROM THE USE OF THIS SOFTWARE.
 * 
 * Modified by: Luca Lusvarghi <luca.lusvarghi5@unimore.it>
 */

#include "ns3/mobility-model.h"
#include "ns3/double.h"
#include "ns3/mobility-building-info.h"
#include "ns3/enum.h"
#include "ns3/nr-v2x-propagation-loss-model.h"
#include "ns3/building-list.h"
#include <ns3/nr-v2x-ue-net-device.h>
#include "ns3/node-container.h"
#include <fstream>
#include <iostream>

NS_LOG_COMPONENT_DEFINE ("NrV2XPropagationLossModel");

namespace ns3 {

std::map<uint32_t, std::map<uint32_t , NrV2XPropagationLossModel::ChannelModel> > NrV2XPropagationLossModel::ChannelMatrix;

NS_OBJECT_ENSURE_REGISTERED (NrV2XPropagationLossModel);


NrV2XPropagationLossModel::NrV2XPropagationLossModel ()
{
  m_randomUniform->SetAttribute ("Min", DoubleValue (0.0));
  m_randomUniform->SetAttribute ("Max", DoubleValue (1.0));
}

NrV2XPropagationLossModel::~NrV2XPropagationLossModel ()
{
}

TypeId
NrV2XPropagationLossModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::NrV2XPropagationLossModel")
    
    .SetParent<BuildingsPropagationLossModel> ()
    
    .AddConstructor<NrV2XPropagationLossModel> ()
    
    .AddAttribute ("Frequency",
                   "The propagation frequency [in GHz]",
                   DoubleValue (2.4),
                   MakeDoubleAccessor (&NrV2XPropagationLossModel::m_frequency),
                   MakeDoubleChecker<double> ()) 
    .AddAttribute ("Sigma",
                   "The log-normal shadowing standard deviation",
                   DoubleValue (3.0),
                   MakeDoubleAccessor (&NrV2XPropagationLossModel::m_sigma),
                   MakeDoubleChecker<double> ())   
    .AddAttribute ("SigmaNLOSv",
                   "The log-normal NLOSv shadowing standard deviation",
                   DoubleValue (3.0),
                   MakeDoubleAccessor (&NrV2XPropagationLossModel::m_sigmaNLOSv),
                   MakeDoubleChecker<double> ())  
    .AddAttribute ("DecorrDistance",
                   "The shadowing decorrelation distance",
                   DoubleValue (10.0),
                   MakeDoubleAccessor (&NrV2XPropagationLossModel::m_decorrDistance),
                   MakeDoubleChecker<double> ())   
  ;
  return tid;
}


double
NrV2XPropagationLossModel::GetLoss (Ptr<MobilityModel> a, Ptr<MobilityModel> b) const
{
  NS_LOG_FUNCTION(this);
  uint32_t nodeIdA = a->GetObject<Node>()->GetId();
  uint32_t nodeIdB = b->GetObject<Node>()->GetId();
  double distance = 0.0, pathloss = 0.0;


/*  NS_LOG_INFO("Printing channel models matrix");
  //Print matrix
  for (std::map<uint32_t, std::map<uint32_t , ChannelModel> >::iterator II = NrV2XPropagationLossModel::ChannelMatrix.begin(); II != NrV2XPropagationLossModel::ChannelMatrix.end(); ++II)
    for (std::map<uint32_t , ChannelModel>::iterator IIinner = II->second.begin(); IIinner != II->second.end(); ++IIinner)
      NS_LOG_INFO("(" << II->first << "," << IIinner->first << "): distance = " << IIinner->second.Distance << " m, pathloss = " << IIinner->second.Pathloss << " dB, shadowing = " << IIinner->second.Shadowing << " dB, shadowing NLOSv = " << IIinner->second.ShadowingNLOSv << " dB, LOS = " << IIinner->second.LOS);
  */    
  std::map<uint32_t, std::map<uint32_t , ChannelModel> >::iterator txIT = NrV2XPropagationLossModel::ChannelMatrix.find(nodeIdA);
  if (txIT != NrV2XPropagationLossModel::ChannelMatrix.end())
  {
    std::map<uint32_t , ChannelModel>::iterator rxIT = txIT->second.find(nodeIdB);
    if (rxIT != txIT->second.end())
    {
      distance = rxIT->second.Distance;
      pathloss = rxIT->second.Pathloss;
    }
  }

  NS_LOG_INFO ("Tx node: " << nodeIdA << " Rx node: " << nodeIdB << " Distance: " << distance << " Pathloss " << pathloss << " dB");

  return pathloss;
}


double
NrV2XPropagationLossModel::GetShadowing (Ptr<MobilityModel> a, Ptr<MobilityModel> b) const
{
  NS_LOG_FUNCTION(this);
  uint32_t nodeIdA = a->GetObject<Node>()->GetId();
  uint32_t nodeIdB = b->GetObject<Node>()->GetId();
  double distance = 0.0, shadowing = 0.0, shadowingNLOSv = 0.0, totalShadowing;
  bool LOS = false;

  std::map<uint32_t, std::map<uint32_t , ChannelModel> >::iterator txIT = NrV2XPropagationLossModel::ChannelMatrix.find(nodeIdA);
  if (txIT != NrV2XPropagationLossModel::ChannelMatrix.end())
  {
    std::map<uint32_t , ChannelModel>::iterator rxIT = txIT->second.find(nodeIdB);
    if (rxIT != txIT->second.end())
    {
       distance = rxIT->second.Distance;
       shadowing = rxIT->second.Shadowing;
       shadowingNLOSv = rxIT->second.ShadowingNLOSv;
       LOS = rxIT->second.LOS;
    }
  }
  totalShadowing = shadowing + shadowingNLOSv;

  NS_LOG_INFO ("Tx node: " << nodeIdA << " Rx node: " << nodeIdB << " Distance: " << distance << " LOS " << LOS << " Shadowing " << shadowing << " Shadowing NLOSv " << shadowingNLOSv << " Total " << totalShadowing);

  return totalShadowing;
}



void 
NrV2XPropagationLossModel::InitChannelMatrix (NodeContainer VehicleUEs)
{
  NS_LOG_FUNCTION(this);
  double shadowingValue, shadowingNLOSv;
  bool LOS;
  double Plos;
  // Save the NodeContainer
  m_UEsContainer = VehicleUEs;
  NS_LOG_INFO("Frequency " << m_frequency << " sigma = " << m_sigma << " sigma NLOSv = " << m_sigmaNLOSv << " Decor. distance = " << m_decorrDistance);
  NS_LOG_INFO("Creating channel models matrix...");

  //Init the channel models
  for (NodeContainer::Iterator L = m_UEsContainer.Begin(); L != m_UEsContainer.End(); ++L)
  {
    Ptr<Node> TxNode = *L;
    uint32_t txID;
    txID = TxNode->GetId ();
    std::map<uint32_t , ChannelModel> tmpColumns;
    for (NodeContainer::Iterator K = m_UEsContainer.Begin(); K != m_UEsContainer.End(); ++K)
    {
      Ptr<Node> RxNode = *K;
      uint32_t rxID;     
      rxID = RxNode->GetId ();
      ChannelModel tmp;
      tmp.Distance = 0.0;
      tmp.Pathloss = 0.0;
      tmp.Shadowing = 0.0;
      tmp.ShadowingNLOSv = 0.0;
      tmp.LOS = false;
      tmpColumns.insert(std::pair<uint32_t , ChannelModel> (rxID,tmp));
    }
    NrV2XPropagationLossModel::ChannelMatrix.insert(std::pair<uint32_t, std::map<uint32_t , ChannelModel> > (txID, tmpColumns));
  }
  NS_LOG_INFO("Done.");

  NS_LOG_INFO("Initializing channel models matrix...");
  for (NodeContainer::Iterator L = m_UEsContainer.Begin(); L != m_UEsContainer.End(); ++L)
  {
    Ptr<Node> TxNode = *L;
    uint32_t txID;
    txID = TxNode->GetId ();
    Ptr<MobilityModel> mobTX = TxNode->GetObject<MobilityModel> ();
    for (NodeContainer::Iterator K = m_UEsContainer.Begin(); K != m_UEsContainer.End(); ++K)
    {
      Ptr<Node> RxNode = *K;
      uint32_t rxID;     
      rxID = RxNode->GetId ();
      if (rxID < txID)
      {      
        Ptr<MobilityModel> mobRX = RxNode->GetObject<MobilityModel> ();
        double TxRxDistance = mobTX->GetDistanceFrom(mobRX);
//        NS_LOG_INFO("Tx ID " << txID << ", Rx ID " << rxID << ", Tx-Rx distance " << TxRxDistance << ", " << mobTX->GetPosition().x << ", " << mobRX->GetPosition().x);
        NrV2XPropagationLossModel::ChannelMatrix[txID][rxID].Distance = TxRxDistance;
        NrV2XPropagationLossModel::ChannelMatrix[txID][rxID].Pathloss = 32.4 + 20 * std::log10(TxRxDistance) + 20 * std::log10(m_frequency); 
        shadowingValue = m_shadowing->GetValue (0.0, (m_sigma*m_sigma));
        NrV2XPropagationLossModel::ChannelMatrix[txID][rxID].Shadowing = shadowingValue;
        if (TxRxDistance <= 475)
        { 
          Plos = std::min(1.0,2.1013e-6*TxRxDistance*TxRxDistance - 0.002*TxRxDistance + 1.0193);  // Probability of being in LOS in the Highway scenario (see 3GPP TR 37.885)
        }
        else
        {
          Plos = std::max(0.0,0.54 - 0.001*(TxRxDistance-475));
        }
//        Plos = 0;         //TODO COMMENT
        LOS = m_randomUniform->GetValue () > Plos ? false : true;
        NrV2XPropagationLossModel::ChannelMatrix[txID][rxID].LOS = LOS;
        if (LOS)
        {
          NS_LOG_INFO("LOS link");
          NrV2XPropagationLossModel::ChannelMatrix[txID][rxID].ShadowingNLOSv = 0;
        }
        else
        {
          Ptr<NormalRandomVariable> RandomShadowingNLOSv =  CreateObject<NormalRandomVariable> ();
          shadowingNLOSv = RandomShadowingNLOSv->GetValue (5 + std::max(0.0,(15*std::log10(TxRxDistance))-41), (m_sigmaNLOSv*m_sigmaNLOSv));// Due to the presence of other vehicles
//          shadowingNLOSv = m_shadowingNLOSv->GetValue (5 + std::max(0.0,(15*std::log10(TxRxDistance))-41), (m_sigmaNLOSv*m_sigmaNLOSv));// Due to the presence of other vehicles
          shadowingNLOSv = std::max(0.0,shadowingNLOSv);
          NS_LOG_INFO("NLOSv link. Additional shadowing = " << shadowingNLOSv << " dB");
          NrV2XPropagationLossModel::ChannelMatrix[txID][rxID].ShadowingNLOSv = shadowingNLOSv;
        }
      }
    }
  }
  NS_LOG_INFO("Done.");

  NS_LOG_INFO("Creating symmetric matrix...");
  for (std::map<uint32_t, std::map<uint32_t , ChannelModel> >::iterator II = NrV2XPropagationLossModel::ChannelMatrix.begin(); II != NrV2XPropagationLossModel::ChannelMatrix.end(); ++II)    
    for (std::map<uint32_t , ChannelModel>::iterator IIinner = II->second.begin(); IIinner != II->second.end(); ++IIinner)       
      if (IIinner->first > II->first)         
        NrV2XPropagationLossModel::ChannelMatrix[II->first][IIinner->first] = NrV2XPropagationLossModel::ChannelMatrix[IIinner->first][II->first];
  NS_LOG_INFO("Done.");

  NS_LOG_INFO("Printing channel models matrix");
  //Print matrix
  for (std::map<uint32_t, std::map<uint32_t , ChannelModel> >::iterator II = NrV2XPropagationLossModel::ChannelMatrix.begin(); II != NrV2XPropagationLossModel::ChannelMatrix.end(); ++II)
    for (std::map<uint32_t , ChannelModel>::iterator IIinner = II->second.begin(); IIinner != II->second.end(); ++IIinner)
      NS_LOG_INFO("(" << II->first << "," << IIinner->first << "): distance = " << IIinner->second.Distance << " m, pathloss = " << IIinner->second.Pathloss << " dB, shadowing = " << IIinner->second.Shadowing << " dB, shadowing NLOSv = " << IIinner->second.ShadowingNLOSv << " dB, LOS = " << IIinner->second.LOS);
  
//  std::cin.get();    
  Simulator::Schedule (MilliSeconds (100), &NrV2XPropagationLossModel::UpdateChannelMatrix, this);      
}

void 
NrV2XPropagationLossModel::UpdateChannelMatrix (void)
{
  NS_LOG_FUNCTION(this);
  double shadowingValue, shadowingNLOSv;
  bool LOS;
  double Plos;
  NS_LOG_INFO("Frequency " << m_frequency << " sigma = " << m_sigma << " sigma NLOSv = " << m_sigmaNLOSv << " Decor. distance = " << m_decorrDistance);
  NS_LOG_INFO("Updating channel matrix at " << Simulator::Now().GetSeconds() << " s...");

  for (NodeContainer::Iterator L = m_UEsContainer.Begin(); L != m_UEsContainer.End(); ++L)
  {
    Ptr<Node> TxNode = *L;
    uint32_t txID;
    txID = TxNode->GetId ();
    Ptr<MobilityModel> mobTX = TxNode->GetObject<MobilityModel> ();
    for (NodeContainer::Iterator K = m_UEsContainer.Begin(); K != m_UEsContainer.End(); ++K)
    {
      Ptr<Node> RxNode = *K;
      uint32_t rxID;     
      rxID = RxNode->GetId ();
      if (rxID < txID)
      {      
        Ptr<MobilityModel> mobRX = RxNode->GetObject<MobilityModel> ();
        double TxRxDistance = mobTX->GetDistanceFrom(mobRX);
        double UpdateDistance = abs(TxRxDistance - NrV2XPropagationLossModel::ChannelMatrix[txID][rxID].Distance);
  //     NS_LOG_INFO("Update distance for Tx " << txID << " and Rx " << rxID << " is " << UpdateDistance << " m");
        shadowingValue = m_shadowing->GetValue (0.0, (m_sigma*m_sigma));
        NS_LOG_INFO("Tx Node " << txID << " Rx Node " << rxID << " Update distance " << UpdateDistance << " Previous shadowing value " << NrV2XPropagationLossModel::ChannelMatrix[txID][rxID].Shadowing << " current shadowing value " << shadowingValue);
        //TODO UNCOMMENT
        shadowingValue = exp(-UpdateDistance/m_decorrDistance)*NrV2XPropagationLossModel::ChannelMatrix[txID][rxID].Shadowing + sqrt( 1-exp(-2*UpdateDistance/m_decorrDistance) )*shadowingValue; 
        NS_LOG_INFO("Shadowing value after decorrelation " << shadowingValue);
        NrV2XPropagationLossModel::ChannelMatrix[txID][rxID].Distance = TxRxDistance;
        NrV2XPropagationLossModel::ChannelMatrix[txID][rxID].Pathloss = 32.4 + 20 * std::log10(TxRxDistance) + 20 * std::log10(m_frequency); 
        NrV2XPropagationLossModel::ChannelMatrix[txID][rxID].Shadowing = shadowingValue;
        if (TxRxDistance <= 475)
        { 
          Plos = std::min(1.0,2.1013e-6*TxRxDistance*TxRxDistance - 0.002*TxRxDistance + 1.0193);  // Probability of being in LOS in the Highway scenario (see 3GPP TR 37.885)
        }
        else
        {
          Plos = std::max(0.0,0.54 - 0.001*(TxRxDistance-475));
        }
//        Plos = 0;         //TODO COMMENT
        LOS = m_randomUniform->GetValue () > Plos ? false : true;
        NrV2XPropagationLossModel::ChannelMatrix[txID][rxID].LOS = LOS;
        if (LOS)
        {
          NS_LOG_INFO("LOS link");
          NrV2XPropagationLossModel::ChannelMatrix[txID][rxID].ShadowingNLOSv = 0;
        }
        else
        {
          Ptr<NormalRandomVariable> RandomShadowingNLOSv =  CreateObject<NormalRandomVariable> ();
          shadowingNLOSv = RandomShadowingNLOSv->GetValue (5 + std::max(0.0,(15*std::log10(TxRxDistance))-41), (m_sigmaNLOSv*m_sigmaNLOSv));// Due to the presence of other vehicles
//          shadowingNLOSv = m_shadowingNLOSv->GetValue (5 + std::max(0.0,(15*std::log10(TxRxDistance))-41), (m_sigmaNLOSv*m_sigmaNLOSv));// Due to the presence of other vehicles
          shadowingNLOSv = std::max(0.0,shadowingNLOSv);
          NS_LOG_INFO("NLOSv link. Additional shadowing = " << shadowingNLOSv << " dB");
          NrV2XPropagationLossModel::ChannelMatrix[txID][rxID].ShadowingNLOSv = shadowingNLOSv;
        }
      }
    }
  }  
  NS_LOG_INFO("Done.");

  NS_LOG_INFO("Creating symmetric matrix...");
  for (std::map<uint32_t, std::map<uint32_t , ChannelModel> >::iterator II = NrV2XPropagationLossModel::ChannelMatrix.begin(); II != NrV2XPropagationLossModel::ChannelMatrix.end(); ++II)    
    for (std::map<uint32_t , ChannelModel>::iterator IIinner = II->second.begin(); IIinner != II->second.end(); ++IIinner)       
      if (IIinner->first > II->first)         
        NrV2XPropagationLossModel::ChannelMatrix[II->first][IIinner->first] = NrV2XPropagationLossModel::ChannelMatrix[IIinner->first][II->first];       
  NS_LOG_INFO("Done.");

  NS_LOG_INFO("Printing channel models matrix");
  //Print matrix
  for (std::map<uint32_t, std::map<uint32_t , ChannelModel> >::iterator II = NrV2XPropagationLossModel::ChannelMatrix.begin(); II != NrV2XPropagationLossModel::ChannelMatrix.end(); ++II)
    for (std::map<uint32_t , ChannelModel>::iterator IIinner = II->second.begin(); IIinner != II->second.end(); ++IIinner)
      NS_LOG_INFO("(" << II->first << "," << IIinner->first << "): distance = " << IIinner->second.Distance << " m, pathloss = " << IIinner->second.Pathloss << " dB, shadowing = " << IIinner->second.Shadowing << " dB, shadowing NLOSv = " << IIinner->second.ShadowingNLOSv << " dB, LOS = " << IIinner->second.LOS);

//  std::cin.get();
  Simulator::Schedule (MilliSeconds (100), &NrV2XPropagationLossModel::UpdateChannelMatrix, this);      
}

bool
NrV2XPropagationLossModel::GetLineOfSightState (uint32_t txID, uint32_t rxID)
{
  NS_LOG_FUNCTION(this);
  double distance = 0.0;
  bool LOS = false;

  std::map<uint32_t, std::map<uint32_t , ChannelModel> >::iterator txIT = NrV2XPropagationLossModel::ChannelMatrix.find(txID);
  if (txIT != NrV2XPropagationLossModel::ChannelMatrix.end())
  {
    std::map<uint32_t , ChannelModel>::iterator rxIT = txIT->second.find(rxID);
    if (rxIT != txIT->second.end())
    {
       distance = rxIT->second.Distance;
       LOS = rxIT->second.LOS;
    }
  }

  NS_LOG_INFO ("Tx node: " << txID << " Rx node: " << rxID << " Distance: " << distance << " LOS " << LOS);

  return LOS;
}



} // namespace ns3
