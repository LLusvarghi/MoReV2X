/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
  */

#include <ns3/applications-module.h>
#include <ns3/config-store.h>
#include <ns3/core-module.h>
#include <ns3/internet-module.h>
#include <ns3/mobility-module.h>
#include <ns3/network-module.h>
#include <ns3/nist-lte-helper.h>
#include <ns3/nist-module.h>
#include "ns3/nist-sl-resource-pool-factory.h"
#include "ns3/nist-sl-preconfig-pool-factory.h"

#include "ns3/delay-jitter-estimation.h"
#include "ns3/packet.h"
#include "ns3/tag.h"
#include "ns3/uinteger.h"
#include "ns3/packet-tag-list.h"
#include <ns3/multi-model-spectrum-channel.h>
#include "ns3/random-variable-stream.h"
//#include "ns3/nist-lte-spectrum-phy.h"
#include <fstream>
#include <iostream>

//#include "ns3/ns2-mobility-helper.h"
//#include "ns3/mobility-model.h"

//#include "ns3/nist-3gpp-propagation-loss-model.h"
//#include "ns3/building-list.h"
//#include "ns3/ns2-mobility-helper.h"
//#include "ns3/nist-lte-enb-rrc.h"
#include <cmath>

#include "ns3/rng-seed-manager.h"
#include "ns3/v2x-lte-tag.h"

#define  vehiclesCount 345

NS_LOG_COMPONENT_DEFINE ("WNS3_2021_Highway");

using namespace ns3;

uint64_t packetID = 0;
double simTime;

Ipv4Address groupAddress; //use multicast address as destination --> broadcast 

int TrepPrint = 100; // (ms). Set the interval for printing the V-UE stats
//int PDBaperiodic = 50;
int Tgen_aperiodic_c, Tgen_periodic;

std::vector<uint16_t> AperiodicPKTs_Size, PeriodicPKTs_Size;
uint16_t LargestAperiodicSize, LargestPeriodicSize, LargestCAMSize;

double PrevX[vehiclesCount], PrevY[vehiclesCount], PrevZ[vehiclesCount], VelX[vehiclesCount], VelY[vehiclesCount], VelZ[vehiclesCount];  
uint8_t VehicleTrafficType[vehiclesCount];
double MeasInterval;
Ptr<ExponentialRandomVariable> RndExp;
bool ExponentialModel, EnableTX[vehiclesCount];

//bool GT_CAMtrace, CAMtraceModel, ML_CAMtrace;
//std::map< uint32_t, std::vector< std::pair<float,int> > > CAMtraces;
std::map< uint32_t, std::vector< std::pair<int,int> > > CAMtraces;

//std::string CAMtraces_path = "/home/luca/Desktop/ML_SUMO/DIEF_nuovo/RUN2/CAMtraces_prediction/";

std::vector<uint16_t> Pattern_index;

bool ETSITraffic;

void LoadCAMtraces (NodeContainer VehicleUEs);

void Print (NodeContainer VehicleUEs);




int Tcheck_CAM;

PosEnabler PositionChecker;

bool UrbanScenario;


void
UdpClient::Send (void)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_sendEvent.IsExpired ());

  Ptr<Node> currentNode = GetNode();
  uint32_t nodeId = currentNode -> GetId();
  double numHops = 0;  
  double T_gen = 0;
  bool insideTX;

  Ptr<UniformRandomVariable> packetSize_index = CreateObject<UniformRandomVariable>();

//  NS_LOG_UNCOND("UdpClient ID " << nodeId << " pointer " << this);
//  std::cin.get();
  if (EnableTX[nodeId-1]) //Not all nodes are enabled to transmit (see SUMO simulation details)
  {
    SeqTsHeader seqTs;  // Packet header for UDP Client/Server application
    seqTs.SetSeq (m_sent); // The packet header must be sequentially increased. Use the packet number m_sent
    Ptr<Packet> p = Create<Packet> (m_size-(8+4)); // 8+4 : the size of the seqTs header
    p->AddHeader (seqTs);  // Add the header to the packet
    
    V2xLteTag v2xTag;
    v2xTag.SetGenTime (Simulator::Now ().GetSeconds ());
    v2xTag.SetMessageType (0x00); //CAM, @LUCA just want to work with CAMs
    //---------------TRAFFIC TYPE SELECTION-------------------
    //  v2xTag.SetTrafficType (0x00); // Periodic traffic
//  v2xTag.SetTrafficType (0x01); // Aperiodic traffic
    v2xTag.SetTrafficType (VehicleTrafficType[nodeId-1]); // Coexistence of periodic and aperiodic traffic
    v2xTag.SetPPPP (0x00); // PPPP for CAM
    v2xTag.SetPrsvp ((uint32_t) (1000 * m_interval.GetSeconds ())); // the required PHY reservation interval. For periodic traffic is ok
    v2xTag.SetNodeId ((uint32_t) nodeId); // Encapsulate the nodeId. It will be used at MAC layer
    v2xTag.SetReselectionCounter((uint16_t)10000); //safe value for indicating non-ETSI traffic
    /*if (VehicleTrafficType[nodeId-1] == 0x01)
    {
      if (CAMtraceModel)
      {
        if ((GT_CAMtrace) || (ML_CAMtrace))
        { 
          v2xTag.SetReselectionCounter((uint16_t) CAMtraces[nodeId][Pattern_index[nodeId-1]][3] + 1);
          v2xTag.SetPrsvp ((uint32_t) CAMtraces[nodeId][Pattern_index[nodeId-1]][2]); // the required PHY reservation interval          
       //   NS_LOG_UNCOND(" Controlla nodo " << nodeId << " RSVP " << (uint16_t) CAMtraces[nodeId][Pattern_index[nodeId-1]][2] << " Cresel " << (uint16_t) CAMtraces[nodeId][Pattern_index[nodeId-1]][3] + 1);
      //    std::cin.get();
        }
        else
        {
          v2xTag.SetPrsvp ((uint32_t) (200)); 
        }
      }
    }*/
    if (VehicleTrafficType[nodeId-1] == 0x01)
    {
      if (ETSITraffic) 
      {
        v2xTag.SetPdb ((uint32_t)100); // @LUCA modified later 
        m_size = CAMtraces[nodeId][Pattern_index[nodeId-1]].second;
        NS_LOG_UNCOND("Udp node " << nodeId << ": transmitting packet with size: " << m_size << " but reserving resources using " << LargestCAMSize);
        v2xTag.SetPrsvp ((uint32_t)100); // the required PHY reservation interval 
        v2xTag.SetReservationSize((uint16_t) LargestCAMSize);

      }
      else //Exponential model
      {
        v2xTag.SetPdb ((uint32_t)Tgen_aperiodic_c); // @LUCA modified later
        m_size = AperiodicPKTs_Size[packetSize_index->GetInteger(0,AperiodicPKTs_Size.size()-1)];
        NS_LOG_UNCOND("Udp: transmitting packet with size: " << m_size << " but reserving resources using " << LargestAperiodicSize);
        v2xTag.SetPrsvp ((uint32_t) Tgen_aperiodic_c); // the required PHY reservation interval 
        v2xTag.SetReservationSize((uint16_t) LargestAperiodicSize);
      }
  //    std::cin.get();
      v2xTag.SetPacketSize((uint16_t) m_size);
    }
    else
    {
      v2xTag.SetPrsvp ((uint32_t) Tgen_periodic);
      v2xTag.SetPdb (v2xTag.GetPrsvp ()); // @LUCA modified later
      m_size = PeriodicPKTs_Size[Pattern_index[nodeId-1]%5];
      NS_LOG_UNCOND("Udp: transmitting packet with size: " << m_size << " but reserving resources using " << LargestPeriodicSize);
    //  std::cin.get();
      v2xTag.SetPacketSize((uint16_t) m_size);
      v2xTag.SetReservationSize((uint16_t) LargestPeriodicSize);
    }
    packetID = packetID + 1; // increment the packet ID number 
    v2xTag.SetIntValue(packetID); 
    Time time = Simulator::Now();
    double timeSec = time.GetSeconds();
    v2xTag.SetDoubleValue(timeSec);
    Ptr<MobilityModel> mobility = GetNode()->GetObject<MobilityModel>();
    Vector currentPos = mobility -> GetPosition();
    double xPosition = currentPos.x;
    double yPosition = currentPos.y;
    // Set generation position in tag
    v2xTag.SetGenPosX(xPosition);
    v2xTag.SetGenPosY(yPosition);  
    v2xTag.SetNumHops(numHops); // 0 by default
    //uint64_t packetID = p->GetUid();
    //uint64_t tagValue;
    //tagValue += 0;
    p->AddByteTag (v2xTag); // Attach the tag
    if ((VehicleTrafficType[nodeId-1] == 0x01) && (ETSITraffic)) 
    {
      std::ofstream CAMdebug;
      CAMdebug.open("results/sidelink/CAMdebugFile.txt", std::ios_base::app);
      CAMdebug << packetID << "," << timeSec << "," << nodeId << "," <<  Pattern_index[nodeId-1] << "," <<  CAMtraces[nodeId][Pattern_index[nodeId-1]].first << "," <<  CAMtraces[nodeId][Pattern_index[nodeId-1]].second << "\r\n" ;
      CAMdebug.close();
    }
    Point point = {(int)xPosition, (int)yPosition}; 
    insideTX = PositionChecker.isInsidePoly("TX", point);
 //   if ((xPosition >= 1500) && (xPosition <= 3500)){
    if (insideTX){
    std::ofstream filetest;
    filetest.open("results/sidelink/TxFile.txt", std::ios_base::app);
    filetest << packetID << "," << timeSec << "," << nodeId << "," << xPosition << "," << yPosition << "," << (int)v2xTag.GetMessageType () << "," << (int)v2xTag.GetTrafficType () << "," << m_size << "\r\n" ;
    filetest.close();
    }
    std::stringstream peerAddressStringStream;
    if (Ipv4Address::IsMatchingType (m_peerAddress))
    {
      peerAddressStringStream << Ipv4Address::ConvertFrom (m_peerAddress);
    }
    else if (Ipv6Address::IsMatchingType (m_peerAddress))
    {
      peerAddressStringStream << Ipv6Address::ConvertFrom (m_peerAddress);
    }
    // my custom socket to send data
    TypeId UDPtid = TypeId::LookupByName ("ns3::UdpSocketFactory");
    // This method wraps the creation of sockets that is performed on a given node by a SocketFactory specified by TypeId
    Ptr<Socket> sendSocket = Socket::CreateSocket(currentNode, UDPtid);
    sendSocket -> SetAllowBroadcast(true);  // Configure whether broadcast transmissions are allowed
    Ipv4Address destAddress = groupAddress; // Important! The SlTft will check for the groupAddress and forward the packet on the Sidelink bearer
    //Ipv4Address destAddress ("225.255.255.255");
    uint16_t destPort = 8000;

    if (Ipv4Address::IsMatchingType(destAddress) == true)
    {
      sendSocket->Bind ();
      sendSocket->Connect (InetSocketAddress (Ipv4Address::ConvertFrom(destAddress), destPort));
          //sendSocket->ShutdownRecv();
    }
    else if (Ipv6Address::IsMatchingType(destAddress) == true)
    {
      sendSocket->Bind6 ();
      sendSocket->Connect (Inet6SocketAddress (Ipv6Address::ConvertFrom(destAddress), destPort));
    } 
    // Send data on the socket and check if returns an error or not (if -> Send fails it returns -1)
    if ((sendSocket->Send (p)) >= 0)
    {
      ++m_sent;
      NS_LOG_INFO ("TraceDelay TX " << m_size << " bytes to "
                                    << peerAddressStringStream.str () << " Uid: "
                                    << p->GetUid () << " Time: "
                                    << (Simulator::Now ()).GetSeconds ());
    }
    else
    {
      NS_LOG_INFO ("Error while sending " << m_size << " bytes to "
                                          << peerAddressStringStream.str ());
    }

    Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();

    if (m_sent < m_count)
    {
       if (VehicleTrafficType[nodeId-1] == 0x00)  //--- If it's PERIODIC traffic ---
       {
         Pattern_index[nodeId-1]++;
         m_sendEvent = Simulator::Schedule (m_interval, &UdpClient::Send, this); 
       }
       else //--- If it's APERIODIC traffic ---
       {  
         if (ETSITraffic)
         {
           T_gen = CAMtraces[nodeId][Pattern_index[nodeId-1]].first;

           Pattern_index[nodeId-1]++;
           m_sendEvent = Simulator::Schedule (MilliSeconds(T_gen), &UdpClient::Send, this); 

         }
         else
         {
           T_gen = Tgen_aperiodic_c + RndExp->GetValue (); 
       //    NS_LOG_UNCOND("Node ID " << nodeId << " Tgen " << T_gen);
       //    std::cin.get();
           m_sendEvent = Simulator::Schedule (MilliSeconds(T_gen), &UdpClient::Send, this); 
         }
       }  
//	m_sendEvent = Simulator::Schedule (MilliSeconds(T_gen), &UdpClient::Send, this); 
//    	m_sendEvent = Simulator::Schedule (m_interval, &UdpClient::Send, this); 
    }
  } // end if Enable
  else
  {
    m_sendEvent = Simulator::Schedule (MilliSeconds(TrepPrint), &UdpClient::Send, this); 
  }

}


void PacketSink::HandleRead (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;
  Address from,localAddress;
  Ptr<Node> currentNode = GetNode();
  uint32_t nodeId = currentNode -> GetId();
//  uint32_t TXnodeId;
  Time time = Simulator::Now();
  bool multihop = true;
  double timeSec = time.GetSeconds();
  double tGenSec;
  double genPosX;
  double genPosY;
  double rxPosX;
  double rxPosY;
  double TxRxDistance = 0;
  uint64_t rxPacketID = 0;
  uint32_t numHops;
  uint32_t lastReceivedPacketId = 0;
//  bool rebroadcast = true;
  uint8_t messageType = 0x00;
  uint8_t alreadyReceived = 0;
  bool insideRX;

  V2xLteTag rxV2xTag;

  std::ofstream filetest;
  std::ofstream fileTx;
  while ((packet = socket->RecvFrom (from)))
  {
    if (packet->GetSize () == 0)
    { 
      break;
    }
    m_totalRx += packet->GetSize ();
    if(packet->FindFirstMatchingByteTag(rxV2xTag)) // If the packet tag exists, inspect its content
    {
      rxPacketID = rxV2xTag.GetIntValue();
      messageType = rxV2xTag.GetMessageType ();       
      // Retrieve node state
      Ptr<LTENodeState> nodeState = Create<LTENodeState> ();
      nodeState = currentNode -> GetObject<LTENodeState> ();
      lastReceivedPacketId = nodeState -> GetLastRcvPacketId();
      lastReceivedPacketId+=0;
      //Update the last received packet ID
      nodeState -> SetLastRcvPacketId(rxPacketID);

      //NS_LOG_UNCOND("\nOk: " << rxPacketID << ", rebroadcast: " << rebroadcast);
      NS_LOG_UNCOND("\nOk: " << rxPacketID);
      tGenSec = rxV2xTag.GetDoubleValue();
      //TXnodeId = rxV2xTag.GetNodeId();
      genPosX = rxV2xTag.GetGenPosX();
      genPosY = rxV2xTag.GetGenPosY();
      Ptr<MobilityModel> mobility = GetNode()->GetObject<MobilityModel>();
      Vector currentPos = mobility -> GetPosition(); 
      rxPosX = currentPos.x;
      rxPosY = currentPos.y;      
      TxRxDistance = std::sqrt(std::pow(genPosX - rxPosX, 2) + std::pow(genPosY - rxPosY, 2));       
      numHops = rxV2xTag.GetNumHops();  
    }
      
    SeqTsHeader seqTs;
    packet->RemoveHeader (seqTs);
    uint32_t currentSequenceNumber = seqTs.GetSeq ();
    currentSequenceNumber+=0;
    Point p = {(int)rxPosX, (int)rxPosY}; 
    insideRX = PositionChecker.isInsidePoly("RX", p);
    if (insideRX)
    {
      filetest.open("results/sidelink/RxFile.txt",std::ios_base::app);
      filetest << rxPacketID << "," << tGenSec << "," <<  timeSec << "," << timeSec - tGenSec <<"," << nodeId << "," << packet->GetSize () << "," << TxRxDistance << "," << numHops << "," << (int) messageType << "," << (int)rxV2xTag.GetTrafficType () << "," << (int) alreadyReceived << "\r\n" ;
      filetest.close();
    }
  }
}

double
BuildingsPropagationLossModel::DoCalcRxPower (double txPowerDbm, Ptr<MobilityModel> a, Ptr<MobilityModel> b) const
{
   
   double fc = 5.9;  // Center frequency in GHz (used for pathloss calculations)
   double hbs1 = 1.5;
   double hms1 = 1.5;

   double txAntHeight = hbs1;
   double rxAntHeight = hms1;
   double frequency = 5.9e9;
   double c = 299792458.0;
   double lambda = c / frequency;
   double m_systemLoss = 1.0;
   
   double Pathloss;  
   double dist = a -> GetDistanceFrom (b);

   double sigma;
   double shadowingValue;
   if (UrbanScenario)
   {     
      Ptr<MobilityModel> mob_a = a->GetObject<Node>()->GetObject<MobilityModel> ();
      Ptr<MobilityModel> mob_b = b->GetObject<Node>()->GetObject<MobilityModel> ();
     
      Vector pos_a = mob_a->GetPosition ();
      Vector pos_b = mob_b->GetPosition ();

      if ( (abs(pos_a.x-pos_b.x) < 5) || (abs(pos_a.y-pos_b.y) < 5) ) //Then it's LOS/NLOSv
      {
        NS_LOG_UNCOND("LOS");
        sigma = 3; //dB, for LOS and NLOSv Urban scenario
        Pathloss = 38.77 + 16.7 * std::log10(dist) + 18.2 * std::log10(fc);     
      }
      else //It's NLOS
      {
        NS_LOG_UNCOND("NLOS");
        sigma = 4; //dB, for LOS and NLOSv Urban scenario
        Pathloss = 36.85 + 30 * std::log10(dist) + 18.9 * std::log10(fc);    
      }
   }
   else
   {
     sigma = 3; //dB, for LOS and NLOSv Highway scenario
     shadowingValue = m_randVariable->GetValue (0.0, (sigma*sigma));
     Pathloss = 32.4 + 20 * std::log10(dist) + 20 * std::log10(fc);
   }
   double totalLoss = std::max(Pathloss + shadowingValue, 0.0);

  int nodeIdA = a->GetObject<Node>()->GetId();
  int nodeIdB = b->GetObject<Node>()->GetId();

  NS_LOG_INFO (this << " Distance: " << dist << " Shadowing " << shadowingValue << " Total loss = " << totalLoss << " Tx node: " << nodeIdA << " Rx node: " << nodeIdB << " Tx power = " << txPowerDbm << " TX - LOSS = " << txPowerDbm - totalLoss);

  return txPowerDbm - totalLoss;
}


void LoadCAMtraces (NodeContainer VehicleUEs)
{
    uint32_t ID;

    for (NodeContainer::Iterator L = VehicleUEs.Begin(); L != VehicleUEs.End(); ++L)
    {
      Ptr<Node> node = *L;
      ID = node->GetId ();     
  //    Pattern_index[ID-1] = 0;
      std::string line,word;
      std::vector<std::string> row; 
      std::vector<int> row_int; 
      std::ifstream CAMtraceFile;
      CAMtraceFile.open("/home/luca/Desktop/C-V2V/CAM-tools/CAM-model/CAMtraces/CAMtrace_" + std::to_string(ID) + ".csv");
      if (CAMtraceFile.is_open())
      {  
        while ( getline (CAMtraceFile,line) )
        {
          row.clear();
          row_int.clear();
          std::stringstream s(line); 
          while (getline(s, word, ',')) 
          { 
             row.push_back(word);
             row_int.push_back(std::stoi(word));
          }
     //     NS_LOG_UNCOND("Node ID " << ID << " row " << row_int[1]);
   //       CAMtraces[ID].push_back(row_int);
          CAMtraces[ID].push_back(std::make_pair(row_int[1],row_int[2]));
        }
      }
      CAMtraceFile.close();   
    }
//  std::cin.get();
}


void Print (NodeContainer VehicleUEs) {
        uint32_t ID;
        bool inside;
        std::ofstream positFile;  
     //   std::ofstream positFile_poly;
     //   positFile_poly.open("results/sidelink/posFile_poly.txt",std::ofstream::app);
        positFile.open("results/sidelink/posFile.txt",std::ofstream::app);
        for (NodeContainer::Iterator L = VehicleUEs.Begin(); L != VehicleUEs.End(); ++L)
        {
            Ptr<Node> node = *L;
            ID = node->GetId ();
                
            Ptr<MobilityModel> mob = node->GetObject<MobilityModel> ();
            if (! mob) continue; // Strange -- node has no mobility model installed. Skip.
            Vector pos = mob->GetPosition ();
          //  Vector vel = mob->GetVelocity ();
            Vector vel;
            VelX[ID-1] = 0;
            VelY[ID-1] = 0;
            VelZ[ID-1] = 0;

            VelX[ID-1] = (pos.x-PrevX[ID-1])/MeasInterval;
            VelY[ID-1] = (pos.y-PrevY[ID-1])/MeasInterval;
            VelZ[ID-1] = (pos.z-PrevZ[ID-1])/MeasInterval;

       //     vel.x = VelX[ID-1];
       //     vel.y = VelY[ID-1];
       //     vel.z = VelZ[ID-1];

            Point p = {(int)pos.x, (int)pos.y}; 
            inside = PositionChecker.isInsidePoly("RX", p);

            if (inside){
              positFile << Simulator::Now().GetSeconds() << "," << ID << "," << pos.x << "," << pos.y << "," << pos.z << "," << VelX[ID-1] << "," << VelY[ID-1] << "," << VelZ[ID-1] << "," << (int)VehicleTrafficType[ID-1] << "," << "1" << "\r\n";
            }
            else
              positFile << Simulator::Now().GetSeconds() << "," << ID << "," << pos.x << "," << pos.y << "," << pos.z << "," << VelX[ID-1] << "," << VelY[ID-1] << "," << VelZ[ID-1] << "," << (int)VehicleTrafficType[ID-1] << "," << "0" << "\r\n";
       //    if ((pos.x >= 1500) && (pos.x <= 3500)){
       //     positFile << Simulator::Now().GetSeconds() << "," << ID << "," << pos.x << "," << pos.y << "," << pos.z << "," << VelX[ID-1] << "," << VelY[ID-1] << "," << VelZ[ID-1] << "," << (int)VehicleTrafficType[ID-1] << "\r\n";
       //    }

            EnableTX[ID-1] = true;
            if (!PositionChecker.isEnabled(pos))
              EnableTX[ID-1] = false;


           PrevX[ID-1] = pos.x;
           PrevY[ID-1] = pos.y;
           PrevZ[ID-1] = pos.z;
        }         
     //   Simulator::Schedule (MilliSeconds (TrepPrint), &Print);
        Simulator::Schedule (MilliSeconds (TrepPrint), &Print, VehicleUEs);      
        positFile.close();
     //   positFile_poly.close();
}



int
main (int argc, char *argv[])
{

//-----DEBUGGING TOOLS------------------------------------------------------------------------
//  LogComponentEnable("NistLtePhyErrorModel", LOG_LEVEL_ALL);
//  LogComponentEnable("NistLteUeMac", LOG_LEVEL_ALL);
//  LogComponentEnable("NistLteUePhy", LOG_LEVEL_ALL);
//  LogComponentEnable("NistLteRlcUm", LOG_LEVEL_ALL);
//  LogComponentEnable("NistLteSpectrumPhy", LOG_LEVEL_ALL);
//    LogComponentEnable("WNS3_2021_Highway",LOG_LEVEL_ALL);
  //LogComponentEnable ("UdpClient", LOG_LEVEL_INFO);
//--------------------------------------------------------------------------------------------
  
  // Provides uniform random variables.
  Ptr<UniformRandomVariable> random = CreateObject<UniformRandomVariable>(); 
 
  // Initialize some values
  uint32_t mcs = 10; // The Modulation and Coding Scheme
  uint32_t rbSize = 8;  // Useless but needed to compile
  uint32_t ktrp = 1;    // number of allocated subframes within the the TRP. It's NIST D2D Mode 2 Parameter: Useless for V2V, but needed to compile 
  uint32_t pscchLength = 8;
  std::string period="sf40";
  simTime = 28;
  double ueTxPower = 23.0; // [dBm]
  uint32_t ueCount = vehiclesCount; // Number of V-UEs 
  bool verbose = true;
  double trep = 0.1; // [s] the CAM repetition period
  uint32_t packetSize = 100;  // [Bytes] The packet size can be either 135, 350, 580, 800 or 1020 Bytes

//  AperiodicPKTs_Size = packetSize;  // Can be set with a different dimension too.

  uint32_t pucchSize = 0;  // PUCCH size in RBs = zero because we do not use the uplink; sidelink channel is located over a distinct frequency band (5.9GHz)


  // For Infrastructure (needed for setting up the simulation, but not used)
  uint32_t nbRings = 1;            // Number of rings in hexagon cell topology 
  double isd = 100;                // Inter Site Distance
  double minCenterDist = 1;        // Minimum deploy distance to center of cell site

  bool PeriodicTraffic = false;
  bool AperiodicTraffic = false;
  bool MixedTraffic = false;
  int PeriodicPercentage = 0;
  ETSITraffic = false;

  bool CtrlErrorModelEnabled = true; // Enable error model in the PSCCH


  bool randomV2VSelection = false; // If true, transmission resources are randomly selected
 //uint32_t numPackets = 1;

  Tcheck_CAM = 100; // 100ms is the deafult value


  bool TxCresel = false;
  bool RxCresel = false;

// Change the random run  
  uint32_t seed = 867; // this is the default seed;
  uint32_t runNumber = 1; // this is the default run --> this will be overridden shortly...
  uint32_t maxDENMPackets = 0; // the maximum allowable number of generated DENM packets --> if 0, only CAMs are generated
  
  double offsetStartTime = 0; // [s] Time at which V-UEs different from V-UE number 1 can start to generate traffic

  UrbanScenario = false;  // Enable the urban scenario channel models

  // Arrays of SUMO coordinates needed for checking the UEs position
  std::vector<BoundingBox> BBs = {{-39, 203, -17, 31}, {4837, 5033, -17, 31}};

  Point polygonTX[] = {{1500, 31}, {1500, -17}, {3500, -17}, {3500, 31}};
  Point polygonRX[] = {{1450, 31}, {1450, -17}, {3550, -17}, {3550, 31}};


  //global variables ruling the aperiodic traffic behaviour down at MAC level
  // if both are false then submissive strategy is applied
  bool Aggressive = false;  
  bool StandardSSPS = true; 
  bool Submissive = false;



 CommandLine cmd;
  cmd.AddValue ("period", "Sidelink period", period);
  cmd.AddValue ("pscchLength", "Length of PSCCH.", pscchLength);
  cmd.AddValue ("ktrp", "Repetition.", ktrp);
  cmd.AddValue ("mcs", "MCS.", mcs);
  cmd.AddValue ("rbSize", "Allocation size.", rbSize);
  cmd.AddValue ("verbose", "Print time progress.", verbose);
  cmd.AddValue ("trep", "The Application-layer message repetition", trep);
  cmd.AddValue ("ueTxPower", "The UE TX Power", ueTxPower);
  cmd.AddValue ("packetSize", "The Application-layer Packet Size in Byte", packetSize);
  cmd.AddValue ("simTime", "The simulated time in seconds", simTime);
  cmd.AddValue ("seed", "The random seed", seed);
  cmd.AddValue ("runNo", "The run number", runNumber);
  cmd.AddValue ("offsetStartTime", "The time offset [s] when UEs 2:end start to generate packets", offsetStartTime);
  cmd.AddValue ("randomV2VSelection", "Whether V2V resources are randomly selected in autonomous scheduling mode", randomV2VSelection);
  cmd.AddValue ("maxDENMPackets", "The maximum number of DENM packets to be generated", maxDENMPackets); // Da rimuovere per WNS3??
  cmd.AddValue ("useTxCresel", "Mode 2: use the transmitter reselection counter", TxCresel);  // Da rimuovere per WNS3??
  cmd.AddValue ("useRxCresel", "Mode 2: use the receiver SCI reselection counter", RxCresel); // Da rimuovere per WNS3??
//  cmd.AddValue ("GT", "Mode 2: use Ground-Truth reservations", GT_CAMtrace); // Da rimuovere per WNS3??
//  cmd.AddValue ("ML", "Mode 2: use predictive reservations", ML_CAMtrace); // Da rimuovere per WNS3??
//  cmd.AddValue ("PKTsize", "Size of aperiodic traffic packets", AperiodicPKTs_Size); //Assolutamente da rimuovere, ora è un vettore

  cmd.AddValue ("Periodic", "Enable periodic traffic generation", PeriodicTraffic);
  cmd.AddValue ("Aperiodic", "Enable aperiodic traffic generation", AperiodicTraffic);
  cmd.AddValue ("Mixed", "Enable the mixed periodic/aperiodic traffic generation", MixedTraffic);
  cmd.AddValue ("Percentage", "In mixed mode, the percentage of periodic UEs", PeriodicPercentage);
  cmd.AddValue ("ETSI", "Enable the ETSI-Algorithm for the CAMs generation", ETSITraffic);

  cmd.Parse(argc, argv);

  //Check the traffic configuration
  if (ETSITraffic)
  {
    if ((MixedTraffic) || (AperiodicTraffic) || (PeriodicTraffic))
      NS_ASSERT_MSG (false, "Enable only one traffic model");      
  }
  else if (MixedTraffic)
  {
    if ((ETSITraffic) || (AperiodicTraffic) || (PeriodicTraffic))
      NS_ASSERT_MSG (false, "Enable only one traffic model");      
    if (PeriodicPercentage == 0)
      NS_ASSERT_MSG (false, "Set a valid periodic traffic percentage. Allowed values: 10, 50 and 90 %");
    if ((PeriodicPercentage != 10) and (PeriodicPercentage != 50) and (PeriodicPercentage != 90))
      NS_ASSERT_MSG (false, "Set a valid periodic traffic percentage. Allowed values: 10, 50 and 90 %");
  }
  else if (AperiodicTraffic)
  {
    if ((ETSITraffic) || (MixedTraffic) || (PeriodicTraffic))
      NS_ASSERT_MSG (false, "Enable only one traffic model");      
  }
  else if (PeriodicTraffic)
  {
    if ((ETSITraffic) || (MixedTraffic) || (AperiodicTraffic))
      NS_ASSERT_MSG (false, "Enable only one traffic model"); 
  }
  else     
    NS_ASSERT_MSG (false, "Enable at least one traffic model"); 

  //Roba da togliere per WNS3--------------------------------------
  NS_ASSERT_MSG (!(Aggressive and StandardSSPS), "Choose only one strategy for serving aperiodic traffic");
  NS_ASSERT_MSG (!(Aggressive and Submissive), "Choose only one strategy for serving aperiodic traffic");
  NS_ASSERT_MSG (!(StandardSSPS and Submissive), "Choose only one strategy for serving aperiodic traffic");

//  NS_ASSERT_MSG (!(ExponentialModel and CAMtraceModel), "Choose only one aperiodic traffic model");
//  NS_ASSERT_MSG (!(ML_CAMtrace and GT_CAMtrace), "Choose the Machine Learning algorithm OR the Ground-Truth predictions"); // Da rimuovere per WNS3??

  if (RxCresel)
    if (!TxCresel)
      NS_ASSERT_MSG(false, "Can not use Rx Cresel without using Tx Cresel");
  //----------------------------------------------------------------------------

   if (offsetStartTime > 0)
     {
       simTime = simTime * 5;

     }  
 
  //Clear the results folder 
  std::system("rm -r results/sidelink/");
  std::system("mkdir results/sidelink/");

  std::ofstream readme;
  readme.open ("results/sidelink/simREADME.txt");
  readme << "----------------------" << std::endl;
  readme << "Simulation:" << std::endl;
  readme << " - seed = " << seed << std::endl; 
  readme << " - run = " << runNumber << std::endl;
  readme << " - RandomV2VSelection = " <<  (int)randomV2VSelection << std::endl;
  readme << " - UDP packet size = " << packetSize << " B" << std::endl; 
  readme << " - T_rep = " << trep << " s" << std::endl; 
  readme << " - UE count = " << ueCount << std::endl;
  readme << " - MCS = " << mcs << std::endl;
  readme << " - Aperiodic traffic configuration:" << std::endl;
  readme << " --- Aggressive strategy: " << Aggressive << std::endl;
  readme << " --- Standard SSPS strategy: " << StandardSSPS << std::endl;
  readme << " --- Submissive strategy: " << Submissive << std::endl;
  readme << " --- Exponential Model: " << ExponentialModel << std::endl;
//  readme << " --- CAM trace Model: " << CAMtraceModel << ", Ground truth? " << GT_CAMtrace << ", Machine Learning? " << ML_CAMtrace << std::endl;
  readme << " - SSPS configuration:" << std::endl;

  readme.close ();

// Set the random seed and run
  RngSeedManager::SetSeed (seed);
  RngSeedManager::SetRun (runNumber);


  //Initialize the position checker
  PositionChecker.initPolygon(polygonTX, (int)sizeof(polygonTX)/sizeof(polygonTX[0]), "TX"); //Filter TX users
  PositionChecker.initPolygon(polygonRX, (int)sizeof(polygonRX)/sizeof(polygonRX[0]), "RX"); //Filter TX users
  PositionChecker.initBBs(BBs); 
//  PositionChecker.DisableChecker(); // Disable the UEs position checker


// Using the ns3::Config::* Functions to set up the simulations
  NS_LOG_INFO ("Configuring UE settings...");

  // Configuring MAC sublayer
  Config::SetDefault ("ns3::NistLteUeMac::SlGrantMcs", UintegerValue (mcs)); //The MCS of the SL grant, must be [0..15] (default 0)
  Config::SetDefault ("ns3::NistLteUeMac::SlGrantSize", UintegerValue (rbSize)); //The number of RBs allocated per UE for sidelink (default 1)
  Config::SetDefault ("ns3::NistLteUeMac::Ktrp", UintegerValue (ktrp)); //The repetition for PSSCH. Default = 0
  Config::SetDefault ("ns3::NistLteUeMac::PucchSize", UintegerValue (pucchSize)); 
  Config::SetDefault ("ns3::NistLteUeMac::ListL2Enabled", BooleanValue (false)); 
  Config::SetDefault ("ns3::NistLteUeMac::AggressiveMode4", BooleanValue (Aggressive)); 
  Config::SetDefault ("ns3::NistLteUeMac::StandardSSPS", BooleanValue (StandardSSPS)); 
  Config::SetDefault ("ns3::NistLteUeMac::SubmissiveMode4", BooleanValue (Submissive)); 


  // Configure Power Control and Phy layer
  Config::SetDefault ("ns3::NistLteUePhy::TxPower", DoubleValue (ueTxPower));
  Config::SetDefault ("ns3::NistLteUePowerControl::Pcmax", DoubleValue (ueTxPower));
  Config::SetDefault ("ns3::NistLteUePowerControl::PoNominalPusch", IntegerValue (-106));
  Config::SetDefault ("ns3::NistLteUePowerControl::PscchTxPower", DoubleValue (ueTxPower));  // Setting the Transmission Power for the control channel
  Config::SetDefault ("ns3::NistLteUePowerControl::PsschTxPower", DoubleValue (ueTxPower));  // Setting the Transmission Power for the sidelink channel
  Config::SetDefault ("ns3::NistLteUePhy::RsrpUeMeasThreshold", DoubleValue (-10.0));        // Setting the RSRP threshold

  // Configure spectrum layer
  Config::SetDefault ("ns3::NistLteSpectrumPhy::CtrlErrorModelEnabled", BooleanValue (CtrlErrorModelEnabled));  // Set but not used
  Config::SetDefault ("ns3::NistLteSpectrumPhy::CtrlFullDuplexEnabled", BooleanValue (!CtrlErrorModelEnabled)); // Set but not used
  Config::SetDefault ("ns3::NistLteSpectrumPhy::SaveCollisionLossesUnimore", BooleanValue (true)); //fare var apposta   // Enable the collision and propagation loss event saving
  // Output file path: "results/sidelink/collisionCounters.txt"
  // Output file format: RX node ID, Timestamp, total # rx events, TX node ID, # decoded packets, # collision losses, # propagation losses, RX node X pos, RX node Y pos, TX node X pos, TX node Y pos


  //@DOC: Not important, just needed to compile; the operating frequency is set later and at a lower layer (PHY)
  // Set the frequency to use for the Public Safety case (band 14 : 788 - 798 MHz for Uplink)
  // See TS36.101 
  Config::SetDefault ("ns3::NistLteEnbNetDevice::UlEarfcn", StringValue ("23330"));
  Config::SetDefault ("ns3::NistLteEnbNetDevice::UlBandwidth", StringValue ("50"));
  // Used for 
  Config::SetDefault ("ns3::NistLteUeMac::RandomV2VSelection", BooleanValue (randomV2VSelection));

  NS_LOG_INFO ("Starting network configuration...");

  Config::SetDefault ("ns3::NistLteRlcUm::MaxTxBufferSize", StringValue ("100000"));
  Config::SetDefault ("ns3::DropTailQueue::MaxPackets", UintegerValue (10000000)); // In order not to run out of packets ;)
 

  // These two lines are not used. Needed only to configure the simulation using a stored input configuration file
  ConfigStore inputConfig;
  inputConfig.ConfigureDefaults();  


  NS_LOG_INFO ("Creating helpers...");
//@DOC Not really needed: ns3 must set up the LTE infrastructure to run the simulation (at least the PDN-GW or PGW to assign IP addresses)
  Ptr<NistPointToPointEpcHelper>  epcHelper = CreateObject<NistPointToPointEpcHelper> (); // NIST EPC helper, used later
  Ptr<Node> pgw = epcHelper->GetPgwNode ();

  Ptr<NistLteHelper> lteHelper = CreateObject<NistLteHelper> ();  // Important! It's the NIST modified version of a lteHelper, which is needed to 
								  // create and configure LTE network entities (see online Doc)

  //@NEW
  //@DOC: now setting the operating frequency
  // Set pathloss model
  Config::SetDefault ("ns3::Nist3gppPropagationLossModel::CacheLoss", BooleanValue (false));
  Config::SetDefault ("ns3::Nist3gppPropagationLossModel::Frequency", DoubleValue (5.9e9));

  lteHelper->SetPathlossModelType ("ns3::Nist3gppPropagationLossModel"); 



  lteHelper->SetAttribute ("UseSidelink", BooleanValue (true));  // ObjectBase::SetAttribute() sets a single attribute, raising fatal errors if unsuccessful
  lteHelper->SetEpcHelper (epcHelper);  // Set the NIST ECP helper as lteHelper

  lteHelper->Initialize (); // Invoke DoInitialize() on all Objects aggregated to this one

  Ptr<NistLteProseHelper> proseHelper = CreateObject<NistLteProseHelper> ();
  proseHelper->SetLteHelper (lteHelper);

  NS_LOG_INFO ("Deploying UE's...");
  

//@DOC: Now the UEs (actually, V-UEs are created)
  NodeContainer ueResponders;
 
   
  for (uint32_t t=0; t<ueCount; ++t)
  {
    Ptr<Node> lteNode = CreateObject<Node> ();
    // LTENodeState is a new class developed from scratch. Defined before the main()
    Ptr<LTENodeState> nodeState = CreateObject<LTENodeState> ();
    nodeState -> SetNode(lteNode);
    lteNode -> AggregateObject(nodeState);  // Aggregate two objects together. Now it's possible to call GetObject() on one to get the other and vice-versa.
    ueResponders.Add(lteNode);
  }


  Ns2MobilityHelper ns2 = Ns2MobilityHelper("/home/luca/Desktop/ML_SUMO/Highway/60Veh_km/RUN1_300/RUN1.tcl");
  ns2.Install(ueResponders.Begin(), ueResponders.End());

/*  MobilityHelper mobilityUE;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject <ListPositionAllocator>();
  positionAlloc ->Add(Vector(0, 0, 0)); // node1
  positionAlloc ->Add(Vector(200, 50, 0)); // node2
  positionAlloc ->Add(Vector(400, 0, 0)); // node3
  positionAlloc ->Add(Vector(1000, 0, 0)); // node4
  mobilityUE.SetPositionAllocator(positionAlloc);
  mobilityUE.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobilityUE.Install (ueResponders);*/


  for (NodeContainer::Iterator L = ueResponders.Begin(); L != ueResponders.End(); ++L)
  {  
     int ID;
     Ptr<Node> node = *L;
     ID = node->GetId ();
  //   std::cin.get();
     Ptr<MobilityModel> mob = node->GetObject<MobilityModel> ();
     if (! mob) continue; // Strange -- node has no mobility model installed. Skip.
     Vector pos = mob->GetPosition ();
     PrevX[ID-1] = pos.x;
     PrevY[ID-1] = pos.y;
     PrevZ[ID-1] = pos.z;

     if ((AperiodicTraffic) || (ETSITraffic))
       VehicleTrafficType[ID-1] = 0x01;  // 0x00 for periodic traffic. 0x01 for aperiodic traffic
     else if (PeriodicTraffic)
       VehicleTrafficType[ID-1] = 0x00;
     else if (PeriodicPercentage == 10)
     {
       VehicleTrafficType[ID-1] = 0x01;
       if (ID % 10 == 0) 
           VehicleTrafficType[ID-1] = 0x00;
     }
     else if (PeriodicPercentage == 50)
     {
       VehicleTrafficType[ID-1] = 0x01;
       if (ID % 2 == 0) 
           VehicleTrafficType[ID-1] = 0x00;
     }
     else if (PeriodicPercentage == 90)
     {
       VehicleTrafficType[ID-1] = 0x00;
       if (ID % 10 == 0) 
           VehicleTrafficType[ID-1] = 0x01;
     }

     EnableTX[ID-1] = true; //Enable a UE to transmit
    
   }

   // Always initialize the ETSI algorithm runner, it is embedded within the "Print" function
 //  ETSI_CAM ETSI_Algorithm(vehiclesCount, Simulator::Now ().GetSeconds ());
   Print(ueResponders);  // Print the initial position of the nodes in the output file
   
//LargestAperiodicSize, LargestPeriodicSize;
   // Define the packets inter-arrival time and size
   if ((MixedTraffic) || (AperiodicTraffic) || (PeriodicTraffic))
   {
     //@LUCA setting the random variable generator for working with aperiodic traffic
     Tgen_aperiodic_c = 50;
     RndExp = CreateObject<ExponentialRandomVariable> ();
     RndExp->SetAttribute ("Mean", DoubleValue(Tgen_aperiodic_c));


     LargestAperiodicSize = 1000; // 1000 bytes is the largest packet size for aperiodic traffic
     for(uint16_t k = 1; k <= LargestAperiodicSize/100; k++)
     {
        AperiodicPKTs_Size.push_back(k*100);  // Valid packet sizes from 100 to 1000 bytes with 100 bytes quantization step
     }

     Ptr<UniformRandomVariable> random_index = CreateObject<UniformRandomVariable>();

     Tgen_periodic = 100;

//     PeriodicPKTs_Size = {300, 190, 190, 190 ,190};
     PeriodicPKTs_Size = {300, 135, 135, 135 ,135};
     LargestPeriodicSize = PeriodicPKTs_Size[0];  // 300 bytes is the largest packet size for aperiodic traffic
     for (NodeContainer::Iterator NN = ueResponders.Begin(); NN != ueResponders.End(); ++NN)  // UEs are indexed starting from 1
     {  
       Pattern_index.push_back(random_index->GetInteger(0,PeriodicPKTs_Size.size()-1) );
     }
   }
   else 
   {
     for (NodeContainer::Iterator NN = ueResponders.Begin(); NN != ueResponders.End(); ++NN)
     {  
       Pattern_index.push_back(0);
     }
     LargestCAMSize = 500;
//     system("CAM-tools/test.sh");
     //std::string veh = std::to_string(vehiclesCount);
     system(("cd CAM-tools/CAM-model/ && python3 NS3_traces_generation.py -p CAMtraces --model Complete --scenario Highway --profile Volkswagen -m 5 -n " + std::to_string(vehiclesCount) + " -t 30 ").c_str());
//     std::cin.get();
     LoadCAMtraces(ueResponders);
   }
   

   MeasInterval = ((double)TrepPrint)/1000;

//   if (ETSITraffic) 
 //  LoadCAMtraces(ueResponders);

  //mobility.SetPositionAllocator (positionAlloc);

  NS_LOG_INFO ("Installing UE network devices...");
  NetDeviceContainer ueDevs = lteHelper->InstallUeDevice (ueResponders);

   // NetDeviceContainer ueSendersDevs = lteHelper->InstallUeDevice (ueResponders);
   // ueDevs.Add (ueSendersDevs);

  // Add eNBs. Not actually useful for the simulation, but needed from the simulator
  // Topology (Hex Grid)(useless)
  Ptr<NistLteHexGridEnbTopologyHelper> topoHelper = CreateObject<NistLteHexGridEnbTopologyHelper> ();
  topoHelper->SetLteHelper (lteHelper);
  topoHelper->SetNbRings (nbRings);
  topoHelper->SetInterSiteDistance (isd);
  topoHelper->SetMinimumDistance (minCenterDist);

  // Configure eNBs' antenna parameters before deploying them (useless)
  lteHelper->SetEnbAntennaModelType ("ns3::NistParabolic3dAntennaModel");

  // Create eNbs(useless)
  NodeContainer sectorNodes;
  sectorNodes.Create (topoHelper->GetNumberOfNodes ());
  std::cout << "eNb IDs=[";
  for (NodeContainer::Iterator it=sectorNodes.Begin (); it != sectorNodes.End (); it++)
    {
      if(it+1 != sectorNodes.End ())
        std::cout << (*it)->GetId () << ",";
      else
        std::cout << (*it)->GetId () << "]" << std::endl;
    }

  // Install mobility (eNBs are fixed)(useless)
  MobilityHelper mobilityeNodeB;
  mobilityeNodeB.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobilityeNodeB.Install (sectorNodes);

 

  // Compute the position of each site and antenna orientation (useless)
  NetDeviceContainer enbDevs = topoHelper->SetPositionAndInstallEnbDevice(sectorNodes);
  for(uint32_t j=0; j< sectorNodes.GetN(); ++j)
  {
     std::cout << "eNB " << sectorNodes.Get(j) -> GetId() << " pos = (" << sectorNodes.Get(j)->GetObject<MobilityModel> ()->GetPosition ().x << "," << sectorNodes.Get(j)->GetObject<MobilityModel> ()->GetPosition ().x << ")\r\n";
  }


 //@OLD: Save UE Positions

/* std::ofstream positFile;
 positFile.open("results/sidelink/posFile.txt",std::ofstream::out);

 for(uint32_t i = 0; i < ueResponders.GetN (); ++i)
 {
   positFile << Simulator::Now().GetSeconds() << "," << ueResponders.Get(i) -> GetId() << "," << ueResponders.Get(i)->GetObject<MobilityModel> ()->GetPosition ().x << "," << ueResponders.Get(i)->GetObject<MobilityModel> ()->GetPosition ().y << "\r\n";
 }

 positFile.close();*/

  NS_LOG_INFO ("Installing IP stack...");
  InternetStackHelper internet;
  internet.Install (ueResponders);
  
  NS_LOG_INFO ("Allocating IP addresses and setting up network route...");

  /*Ipv4InterfaceContainer ueIpIface;
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  ueIpIface = epcHelper->AssignUeIpv4Address (ueDevs);*/

  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ipv4AddressHelper ipv4;
  NS_LOG_INFO ("Assign IP Addresses.");

  ipv4.SetBase ("10.1.0.0", "255.255.0.0");
  Ipv4InterfaceContainer intcont = ipv4.Assign (ueDevs);
    //Ipv4InterfaceContainer intcont2 = ipv4.Assign (ueSendersDevs); 

  // Set up the IP network seen from the eNB
  for (uint32_t u = 0; u < ueResponders.GetN (); ++u) // Could have used an iterator also here. Anyway, the result is the same
    {
      Ptr<Node> ueNode = ueResponders.Get (u);
//      std::cout << ueNode -> GetObject<Ipv4> () -> GetAddress(1,0).GetLocal() << std::endl;
      // Set the default gateway for the UE
      Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
     // ueStaticRouting->SetDefaultRoute (Ipv4Address::GetAny(), 1);
      //ueStaticRouting->SetDefaultMulticastRoute(1);
    }

 // std::cin.get();  // PAUSE
  
  NS_LOG_INFO ("Attaching UE's to LTE network...");
  lteHelper->Attach (ueDevs);



  // Required to use NIST 3GPP Propagation model
  BuildingsHelper::Install (sectorNodes);

  BuildingsHelper::Install (ueResponders);
  BuildingsHelper::MakeMobilityModelConsistent ();
  
  NS_LOG_INFO ("Installing applications...");
  // UDP application: in our case, this is the application that generates CAMs and DENMs



 TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");

  // groupAddress is a Ipv4Address variable
//  Ipv4AddressGenerator::Init(Ipv4Address ("225.0.0.0"), Ipv4Mask ("255.0.0.0"));
//  groupAddress = Ipv4AddressGenerator::NextAddress (Ipv4Mask ("255.0.0.0"));
  groupAddress = "225.0.0.1";
  std::cout << "Group address " << groupAddress << std::endl;
  UdpClientHelper udpClient (groupAddress , 8000); //set destination IP address and UDP port (8000 in this case). The group address is used to set the Sidelink Bearers

  udpClient.SetAttribute ("MaxPackets", UintegerValue (100000));
  udpClient.SetAttribute ("Interval", TimeValue (Seconds (trep)));  //If non-periodic this value is changed every time a UdpClient::Send is scheduled
  udpClient.SetAttribute ("PacketSize", UintegerValue (packetSize));  // Can be changed modifiying the UdpClient class
  udpClient.SetAttribute ("MaxDENMPackets", UintegerValue (maxDENMPackets));

  ApplicationContainer clientApps = udpClient.Install(ueResponders);
//  ApplicationContainer clientApps = udpClient.Install(ueResponders.Get(0));  // Similar to NodeContainer, but now is a vector of smart pointers pointing to applications
//  clientApps.Add(udpClient.Install(ueResponders.Get(1)));	
  Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();
 
//@OLD  clientApps.Get(0) -> SetStartTime(Seconds (rand -> GetValue(0.001, 1.0)));
  for(uint32_t i = 0; i < ueResponders.GetN(); i++)
  {
      //@DOC: Randomize the generation time of CAM messages
      clientApps.Get(i) -> SetStartTime(Seconds (rand -> GetValue(0.001, 1.0)));
      clientApps.Get(i) -> SetStopTime (Seconds (simTime + 0.5)); // The simulation ends at simTime + 1, so we have a buffer of 0.5s 
  }
  //clientApps.Get(0) -> SetStartTime(Seconds (rand -> GetValue(0.001, 0.1)));
//  clientApps.Get(0) -> SetStartTime(Seconds (rand -> GetValue(0.001, 1.0)));
//  clientApps.Get(0) -> SetStopTime (Seconds (simTime + 0.5)); // The simulation ends at simTime + 1, so we have a buffer of 0.5s 
//  clientApps.Get(1) -> SetStartTime(Seconds (rand -> GetValue(0.001, 1.0)));
//  clientApps.Get(1) -> SetStopTime (Seconds (simTime + 0.5));
//@OLD  clientApps.Get(0) -> SetStopTime (Seconds (simTime + 1));
 
  // Application to receive traffic
  PacketSinkHelper clientPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 8000));  // Ipv4Address::GetAny() returns the dummy 0.0.0.0 address. The port number must be the correct one
  ApplicationContainer clientRespondersSrvApps = clientPacketSinkHelper.Install(ueResponders);
  clientRespondersSrvApps.Start (Seconds (0.001));  // Start ASAP
  clientRespondersSrvApps.Stop (Seconds (simTime+0.9));


  // At this point, each V-UE mounts both a Client and a Server (Sink) application
 



  
  
  NS_LOG_INFO ("Creating sidelink configuration...");
  uint32_t groupL2Address = 0x00;
  


 Ptr<NistSlTft> tftRx = Create<NistSlTft> (NistSlTft::BIDIRECTIONAL, groupAddress, groupL2Address);
  proseHelper->ActivateSidelinkBearer (Seconds(0.1), ueDevs, tftRx); // Set accordingly with clientApps StartTime! 0.1 Seconds is the ActivationTime of the Sidelink bearer


  Ptr<LteUeRrcSl> ueSidelinkConfiguration = CreateObject<LteUeRrcSl> ();
  ueSidelinkConfiguration->SetSlEnabled (true);

  NistLteRrcSap::SlPreconfiguration preconfiguration;
  preconfiguration.preconfigGeneral.carrierFreq = 54900; //not important
  preconfiguration.preconfigGeneral.slBandwidth = 50;    //original: 50 PRBs = 10MHz
  preconfiguration.preconfigComm.nbPools = 1; // the number of pools, not relevant for V2V


  NistSlPreconfigPoolFactory pfactory;
  NistSlResourcePoolFactory commfactory;
 

  //build PSCCH bitmap value
  uint64_t pscchBitmapValue = 0x0;
  for (uint32_t i = 0 ; i < pscchLength; i++) {
    pscchBitmapValue = pscchBitmapValue >> 1 | 0x8000000000;
  }
  std::cout << "bitmap=" << std::hex << pscchBitmapValue << '\n'; // this is the PSCCH subframe pool bitmap, from NIST D2D implementation


  pfactory.SetControlBitmap (pscchBitmapValue);
  pfactory.SetControlPeriod (period);
  pfactory.SetDataOffset (pscchLength);

  commfactory.SetControlBitmap (pscchBitmapValue);

  pfactory.SetHaveUeSelectedResourceConfig(false); // Mode1 --> done in this way, it does not work

  preconfiguration.preconfigComm.pools[0] = pfactory.CreatePool ();

  ueSidelinkConfiguration->SetSlPreconfiguration (preconfiguration);


 
  NS_LOG_INFO ("Installing sidelink configuration...");
  lteHelper->InstallSidelinkConfiguration (ueDevs, ueSidelinkConfiguration);
  //lteHelper->InstallSidelinkConfiguration (ueSendersDevs, ueSidelinkConfiguration);

  NS_LOG_INFO ("Enabling LTE traces...");
  lteHelper->EnableTraces ();
  

  NS_LOG_INFO ("Starting simulation...");
  Simulator::Stop (Seconds (simTime+1)); 
  Simulator::Run ();
  Simulator::Destroy ();

  NS_LOG_INFO ("Done.");

  return 0;

}

