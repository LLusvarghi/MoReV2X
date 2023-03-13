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
#include "ns3/nist-lte-spectrum-phy.h"
#include <fstream>
#include <iostream>

#include "ns3/ns2-mobility-helper.h"
#include "ns3/mobility-model.h"

#include "ns3/nist-3gpp-propagation-loss-model.h"
#include "ns3/building-list.h"
#include "ns3/ns2-mobility-helper.h"
#include "ns3/nist-lte-enb-rrc.h"
#include <cmath>

#include "ns3/rng-seed-manager.h"
#include "ns3/v2x-lte-tag.h"

#include "/home/luca/Desktop/C-V2V/src/nist/model/UNIMORE-Gvar.h"

#define  vehiclesCount 240 // Change also in the nist-lte-ue-mac.cc file

NS_LOG_COMPONENT_DEFINE ("wns3_2017_pssch");


using namespace ns3;

uint64_t packetID = 0;
uint64_t nTransportBlocks = 0;
uint64_t nTotPackets = 0;
Ipv4Address groupAddress; //use multicast address as destination --> broadcast 
NodeContainer::Iterator i,k,L;
int TrepPrint = 10, T_Check = 10; // (ms). Set the interval for printing the V-UE stats
int PDBaperiodic = 50;
double PrevX[vehiclesCount], PrevY[vehiclesCount], PrevZ[vehiclesCount], VelX[vehiclesCount], VelY[vehiclesCount], VelZ[vehiclesCount];  
uint8_t VehicleTraffic[vehiclesCount];
double MeasInterval;
Ptr<ExponentialRandomVariable> RndExp;
double T_GenCam[vehiclesCount];
int T_Check_Counter[vehiclesCount], GeneratorCounter[vehiclesCount];
double OriginPos[vehiclesCount], OriginVel[vehiclesCount];
double PosX[vehiclesCount], PosY[vehiclesCount], PosZ[vehiclesCount];
Ptr<UdpClient> THIS[vehiclesCount]; 
EventId CONTEXT[vehiclesCount];
int N_GenCam = 3;
int CHECK[vehiclesCount], FIRST[vehiclesCount];

// Class to hold node state
//@NEW
class LTENodeState : public Object
{
  public:
      
      LTENodeState ();
      static TypeId GetTypeId (void);
      virtual TypeId GetInstanceTypeId (void) const; 
   
      void AddNewReceivedPacket (uint32_t packetID);
      bool HasReceivedPacket (uint32_t packetID);
      
      // Getters
      bool IsVehicle(void) const;
      uint32_t GetLastRcvPacketId (void) const;
      Ptr<Node> GetNode (void) const;
      
      
      
      // Setters
      void SetLastRcvPacketId(uint32_t packetID);
      void SetNode(Ptr<Node> node);
      
 private:
      bool m_vehicle;
      uint32_t m_lastRcvPacketId;
      Ptr<Node> m_node;
      std::vector<uint32_t> m_receivedPacketIds;
      
};

LTENodeState::LTENodeState (void) // sets the initial (default state)
{
  m_vehicle = true;  // It's always a vehicle
  m_lastRcvPacketId = 0;
}


TypeId 
LTENodeState::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LTENodeState")
    .SetParent<Object> ()
    .AddConstructor<LTENodeState> ()
    /*.AddAttribute ("IsVehicle",
                   "Is the LTE node a vehicle UE?",

                   BooleanValue (true),

                   MakeBooleanAccessor (&MyTag::GetSimpleValue),

                   MakeBooleanChecker ())*/
  ;
  return tid;
}

TypeId 
LTENodeState::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void 
LTENodeState::AddNewReceivedPacket (uint32_t packetID)
{
  m_receivedPacketIds.push_back(packetID);
}

bool
LTENodeState::HasReceivedPacket (uint32_t packetID)
{
  bool received = false;
  for(std::vector<uint32_t>::iterator it = m_receivedPacketIds.begin(); it != m_receivedPacketIds.end(); ++it)
  {
     if(*it == packetID)
     {
        return true;
     }
  }
  
  return received;
}

// Getters
bool
LTENodeState::IsVehicle(void) const
{
  return true;  // It's always a vehicle in my case.
}

uint32_t
LTENodeState::GetLastRcvPacketId (void) const
{
  return m_lastRcvPacketId;
}

Ptr<Node>
LTENodeState::GetNode (void) const
{
  return m_node;
}

// Setters
void 
LTENodeState::SetLastRcvPacketId(uint32_t packetID)
{
  m_lastRcvPacketId = packetID;
}

void 
LTENodeState::SetNode(Ptr<Node> node)
{
  m_node = node;
}



// Another newly defined class
class LTENode : public Node
{
  public:
      LTENode ();
      static TypeId GetTypeId (void);
      virtual TypeId GetInstanceTypeId (void) const; 
      
      //Getters
      bool IsVehicle(void) const;
  private:
      bool m_vehicle;
};

LTENode::LTENode (void)
{
  m_vehicle = true;
}

TypeId 
LTENode::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LTENode")
    .SetParent<Node> ()
    .AddConstructor<LTENode> ()
    /*.AddAttribute ("IsVehicle",
                   "Is the LTE node a vehicle UE?",
                   BooleanValue (true),
                   MakeBooleanAccessor (&MyTag::GetSimpleValue),
                   MakeBooleanChecker ())*/
  ;
  return tid;
}

TypeId 
LTENode::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

bool
LTENode::IsVehicle(void) const
{
  return m_vehicle;
}



void
UdpClient::Send (void)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_sendEvent.IsExpired ());

  Ptr<Node> currentNode = GetNode();
  uint32_t nodeId = currentNode -> GetId();
  double numHops = 0;  
//  double T_gen = 0;  //@LUCA Trying to generate aperiodic UPD traffic
//  T_gen = 50 + RndExp->GetValue ();
  std::ofstream DEBFile;
  DEBFile.open("results/sidelink/DEBFile.txt",std::ofstream::app);

  SeqTsHeader seqTs;  // Packet header for UDP Client/Server application
  seqTs.SetSeq (m_sent); // The packet header must be sequentially increased. Use the packet number m_sent
  Ptr<Packet> p = Create<Packet> (m_size-(8+4)); // 8+4 : the size of the seqTs header
  p->AddHeader (seqTs);  // Add the header to the packet
  
   // create a tag.
  //MyTag newTag;
  
  V2xLteTag v2xTag;
  v2xTag.SetGenTime (Simulator::Now ().GetSeconds ());


/*
  @LUCA: This is old stuff for the generation of DENM
  if (GetNode() -> GetId () == 1 && m_sentDENM < m_MaxSentDENM ) // @DOC: Only vehicle number 1 is authorized to send DENMs
     {
        v2xTag.SetMessageType (0x01); //DENM
        v2xTag.SetPPPP (0x08); // Pro-Se Per-Packet Priority value (PPPP) for DENM
        m_size = 960; // UDP Packet size in Bytes
     }
  else
     {
        v2xTag.SetMessageType (0x00); //CAM
        v2xTag.SetPPPP (0x00); // PPPP for CAM ; size has already been set
     }*/

  v2xTag.SetMessageType (0x00); //CAM, @LUCA just want to work with CAMs
  
  //---------------TRAFFIC TYPE SELECTION-------------------

//  v2xTag.SetTrafficType (0x00); // Periodic traffic
//  v2xTag.SetTrafficType (0x01); // Aperiodic traffic
  v2xTag.SetTrafficType (VehicleTraffic[nodeId-1]); // Coexistence of periodic and aperiodic traffic
  v2xTag.SetPPPP (0x00); // PPPP for CAM
  v2xTag.SetPrsvp ((uint32_t) (1000 * m_interval.GetSeconds ())); // the required PHY reservation interval 
  v2xTag.SetNodeId ((uint32_t) nodeId); // Encapsulate the nodeId. It will be used at MAC layer

/*
  if (v2xTag.GetMessageType () == 0x01)
    {
        v2xTag.SetPdb (100);
    }
  else
    {
        v2xTag.SetPdb (v2xTag.GetPrsvp ()); // the Packet Delay Budget
    }*/


  if (VehicleTraffic[nodeId-1] == 0x01)
  {
	v2xTag.SetPdb ((uint32_t)PDBaperiodic); // @LUCA modified later
  }
  else
  {
	v2xTag.SetPdb (v2xTag.GetPrsvp ()); // @LUCA modified later
  }
  v2xTag.SetSimpleValue (0x57); // Used for testing
  
/* @DOC: The Packet Tag is an element which links a kind of data structure (actually an object) to a packet.	
	 It is extremely useful for delivering information along with the packet, which ns3 would otherwise
	 use as a bunch of useless bytes.
	 The class v2xTag is actually the one which creates and manages the packet tag content (position, node Id, timestamp and so on)
*/


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
  uint64_t tagValue;
  tagValue += 0;


   p->AddByteTag (v2xTag); // Attach the tag

  

  THIS[nodeId-1] = this;
  if (FIRST[nodeId-1] == 1)
  {
      OriginPos[nodeId-1] = PosX[nodeId-1];  
      OriginVel[nodeId-1] = 0;
      FIRST[nodeId-1] = 0;
      CHECK[nodeId-1] = 1;
  }

  
/*@DOC: salviamo nel file TxFile.txt l'evento di generazione del pacchetto
  Struttura del file:

  packetID, generation Time [s], Node ID, Pos X [m], Pos Y [m], Message Type [0->CAM, 1->DENM]

*/
  if ((xPosition >= 1000) && (xPosition <= 3000)){
  std::ofstream filetest;
  filetest.open("results/sidelink/TxFile.txt", std::ios_base::app);
  filetest << packetID << "," << timeSec << "," << nodeId << "," << xPosition << "," << yPosition << "," << (int)v2xTag.GetMessageType () << "," << (int)v2xTag.GetTrafficType () << "\r\n" ;
  filetest.close();
  }
  
/*@DOC: Qui comincia una serie di magheggi sugli indirizzi IP che hanno occupato -da soli- circa 15 giorni di tesi

  Non sono sicuro di aver capito perchè funzioni ma a noi poco interessa: l'importante è che si faccia il broadcast (non ci interesserebbe
  nemmeno avere il livello IP in realtà)*/

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
     /* if (v2xTag.GetMessageType () == 0x01)
        {
            ++m_sentDENM;
        }*/
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
     if (VehicleTraffic[nodeId-1] == 0x00) //If it's periodic traffic
     {
    	m_sendEvent = Simulator::Schedule (m_interval, &UdpClient::Send, this); 
     }
     else //If it's aperiodic traffic
     {
         if (T_GenCam[nodeId-1] != 1000)
         {
            GeneratorCounter[nodeId-1]++;
            CHECK[nodeId-1] = 0;
         }
         if (GeneratorCounter[nodeId-1] == N_GenCam+1)
         {
            T_GenCam[nodeId-1] = 1000;
            OriginPos[nodeId-1] = PosX[nodeId-1];  
            OriginVel[nodeId-1] = VelX[nodeId-1];
            GeneratorCounter[nodeId-1] = 0;
            CHECK[nodeId-1] = 1;
            T_Check_Counter[nodeId-1] = -1;
            DEBFile << Simulator::Now ().GetSeconds () << " Node ID " << nodeId << "\r\n";
         }
         m_sendEvent = Simulator::Schedule (MilliSeconds (T_GenCam[nodeId-1]), &UdpClient::Send, this); 
         CONTEXT[nodeId-1] = m_sendEvent;
	 //m_sendEvent = Simulator::Schedule (MilliSeconds(T_gen), &UdpClient::Send, this); 
     }  
//	m_sendEvent = Simulator::Schedule (MilliSeconds(T_gen), &UdpClient::Send, this); 
//    	m_sendEvent = Simulator::Schedule (m_interval, &UdpClient::Send, this); 
  }

  DEBFile.close();
}


ApplicationContainer
UdpClientHelper::Install(NodeContainer c)
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      Ptr<Node> node = *i;
      Ptr<UdpClient> client = m_factory.Create<UdpClient> ();
      node->AddApplication (client);
      apps.Add (client);
    }
  return apps;
}



void PacketSink::HandleRead (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);



  Ptr<Packet> packet;
  Address from,localAddress;

  Ptr<Node> currentNode = GetNode();
  uint32_t nodeId = currentNode -> GetId();

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
  
  bool rebroadcast = true;
  
  uint8_t messageType = 0x00;

  uint8_t alreadyReceived = 0;
  
  //MyTag rxTag;

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

      
      


  if(packet->FindFirstMatchingByteTag(rxV2xTag)){

       rxPacketID = rxV2xTag.GetIntValue();
       messageType = rxV2xTag.GetMessageType ();       

       // Retrieve node state
       Ptr<LTENodeState> nodeState = Create<LTENodeState> ();
       nodeState = currentNode -> GetObject<LTENodeState> ();
       lastReceivedPacketId = nodeState -> GetLastRcvPacketId();
       lastReceivedPacketId+=0;

       //Update the last received packet ID
       nodeState -> SetLastRcvPacketId(rxPacketID);
  
       
       
       bool hasReceivedAlready = nodeState -> HasReceivedPacket(rxPacketID);
       

       if(!hasReceivedAlready)
       {
           //Update Reception list
           nodeState -> AddNewReceivedPacket (rxPacketID);
           alreadyReceived = 0;
       }
       else
       {
           alreadyReceived = 1;

       }
       
          


       NS_LOG_UNCOND("\nOk: " << rxPacketID << ", rebroadcast: " << rebroadcast);
       tGenSec = rxV2xTag.GetDoubleValue();

       genPosX = rxV2xTag.GetGenPosX();
       genPosY = rxV2xTag.GetGenPosY();
       
       Ptr<MobilityModel> mobility = GetNode()->GetObject<MobilityModel>();
       Vector currentPos = mobility -> GetPosition();
  
       rxPosX = currentPos.x;
       rxPosY = currentPos.y;
      
       TxRxDistance = std::sqrt(std::pow(genPosX - rxPosX, 2) + std::pow(genPosY - rxPosY, 2));
       
       numHops = rxV2xTag.GetNumHops();

       bool isBehindSource = (rxPosX > genPosX);
       rebroadcast = ((!hasReceivedAlready) && isBehindSource);   
  }
  else{
       std::cout << "\nNo\n";
       filetest.open("results/sidelink/sentPacket.txt", std::ios_base::app);
           filetest << "No" << "\r\n";
           filetest.close();
  }

      
 if(multihop && messageType == 0x01) // only DENMs can be rebroadcast
    {    
       // Create socket for multihop relay 
      


         TypeId UDPtid = TypeId::LookupByName ("ns3::UdpSocketFactory");
         Ptr<Socket> sendSocket = Socket::CreateSocket(currentNode, UDPtid);
         sendSocket -> SetAllowBroadcast(true);

         Ipv4Address destAddress = groupAddress;
         uint16_t destPort = 8000;

   if (Ipv4Address::IsMatchingType(destAddress) == true)
        {
           sendSocket->Bind ();
           sendSocket->Connect (InetSocketAddress (Ipv4Address::ConvertFrom(destAddress), destPort));
          sendSocket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
           //sendSocket->Connect (from);
        }
      else if (Ipv6Address::IsMatchingType(destAddress) == true)
        {
          sendSocket->Bind6 ();
          sendSocket->Connect (Inet6SocketAddress (Ipv6Address::ConvertFrom(destAddress), destPort));
        }

    if ( ! sendSocket -> GetAllowBroadcast()){
       filetest.open("results/sidelink/BC.txt", std::ios_base::app);
           filetest << Simulator::Now().GetSeconds() << "," << nodeId << "\r\n";
           filetest.close();
       
    }


    //Address completeDestAddress = InetSocketAddress (Ipv4Address::ConvertFrom(destAddress), destPort);
  
  //sendPcktOverSocket(pCopy, sendSocket, currentNode);
 
/* if(rebroadcast)  // Rebroadcasting Policy for DENM messages
  {
      Ptr<Packet> pCopy = packet->Copy ();

  Simulator::ScheduleWithContext(sendSocket -> GetNode() -> GetId(), Seconds(0.003), &sendPcktOverSocket, pCopy, sendSocket);

 }*/


//EventId evento = Simulator::Schedule(Seconds(0.001), UdpClient::Send, this);

    
} //end if(multihop)
 
  SeqTsHeader seqTs;
          packet->RemoveHeader (seqTs);
          uint32_t currentSequenceNumber = seqTs.GetSeq ();

         currentSequenceNumber+=0;
 if ((rxPosX >= 950) && (rxPosX <= 3050)){
       filetest.open("results/sidelink/RxFile.txt",std::ios_base::app);
          filetest << rxPacketID << "," << tGenSec << "," <<  timeSec << "," << timeSec - tGenSec <<"," << nodeId << "," << packet->GetSize () << "," << TxRxDistance << "," << numHops << "," << (int) messageType << "," << (int)rxV2xTag.GetTrafficType () << "," << (int) alreadyReceived << "\r\n" ;
          filetest.close();
}
  // Rebroadcast the packet
   // create a copy of the packet
 // Ptr<Packet> pCopy = packet->Copy ();
  

    }
}

void
PacketSocketServer::HandleRead (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  
  std::ofstream filetest;
  
  
  filetest.open("testfile.txt",std::ios_base::app);
          filetest << Simulator::Now() ;
          filetest.close();
  Ptr<Packet> packet;
  Address from;
  while ((packet = socket->RecvFrom (from)))
    {
      if (PacketSocketAddress::IsMatchingType (from))
        {
          m_pktRx ++;
          m_bytesRx += packet->GetSize ();
          NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
                       << "s packet sink received "
                       << packet->GetSize () << " bytes from "
                       << PacketSocketAddress::ConvertFrom (from)
                       << " total Rx " << m_pktRx << " packets"
                       << " and " << m_bytesRx << " bytes");
          m_rxTrace (packet, from);
          SeqTsHeader seqTs;
          packet->RemoveHeader (seqTs);
          uint32_t currentSequenceNumber = seqTs.GetSeq ();

        filetest.open("testfile.txt",std::ios_base::app);
          filetest << currentSequenceNumber << "," << Simulator::Now() ;
          filetest.close();
        }
    }
}




double
Nist3gppPropagationLossModel::GetLoss (Ptr<MobilityModel> a, Ptr<MobilityModel> b) const
{
  
  NS_ASSERT_MSG ((a->GetPosition ().z >= 0) && (b->GetPosition ().z >= 0), "Nist3gppsPropagationLossModel does not support underground nodes (placed at z < 0)");


  //Check if loss value is cached
  double loss = 0.0;
  MobilityDuo couple;
  couple.a = a;
  couple.b = b;
  std::map<MobilityDuo, double>::iterator it_a = m_lossMap.find (couple);
  if (it_a != m_lossMap.end ())
    {
      loss = it_a->second;
    }
  else
    {
      couple.a = b;
      couple.b = a;
      std::map<MobilityDuo, double>::iterator it_b = m_lossMap.find (couple);
      if (it_b != m_lossMap.end ())
        {
          loss = it_b->second;
        }
      else
        {
          // Get the MobilityBuildingInfo pointers
          Ptr<MobilityBuildingInfo> a1 = a->GetObject<MobilityBuildingInfo> ();
          Ptr<MobilityBuildingInfo> b1 = b->GetObject<MobilityBuildingInfo> ();
          NS_ASSERT_MSG ((a1 != 0) && (b1 != 0), "Nist3gppsPropagationLossModel only works with MobilityBuildingInfo");

          //double loss = 0.0;
          Vector aPos = a->GetPosition ();
          Vector bPos = b->GetPosition ();

          bool aIndoor = false;
          bool bIndoor = false;

          // Go through the different buildings in the simulation 
          // and check if the nodes are outdoor or indoor
          for (BuildingList::Iterator bit = BuildingList::Begin (); bit != BuildingList::End (); ++bit)
            {
              NS_LOG_LOGIC ("checking building " << (*bit)->GetId () << " with boundaries " << (*bit)->GetBoundaries ());
   //std::cout << "checking building " << (*bit)->GetId () << " with boundaries " << (*bit)->GetBoundaries () << "\r\n";
              if ((*bit)->IsInside (aPos))
                {
                  aIndoor = true;
                  uint16_t floor = (*bit)->GetFloor (aPos);
                  uint16_t roomX = (*bit)->GetRoomX (aPos);
                  uint16_t roomY = (*bit)->GetRoomY (aPos);     
                  a1->SetIndoor (*bit, floor, roomX, roomY);       
                }
              if ((*bit)->IsInside (bPos))
                {
                  bIndoor = true;
                  uint16_t floor = (*bit)->GetFloor (bPos);
                  uint16_t roomX = (*bit)->GetRoomX (bPos);
                  uint16_t roomY = (*bit)->GetRoomY (bPos);     
                  b1->SetIndoor (*bit, floor, roomX, roomY);  
                }
            }

          // Verify if it is a D2D communication (UE-UE) or LTE communication (eNB-UE)
          // LTE
          if (((a->GetObject<Node> ()->GetDevice (0)->GetObject<NistLteUeNetDevice> () != 0) and (b->GetObject<Node> ()->GetDevice (0)->GetObject<NistLteEnbNetDevice> () != 0)) or ((b->GetObject<Node> ()->GetDevice (0)->GetObject<NistLteUeNetDevice> () != 0) and (a->GetObject<Node> ()->GetDevice (0)->GetObject<NistLteEnbNetDevice> () != 0)))
            {
              loss = Urbanmacrocell (a,b);
            }
          //D2D
          else if ((a->GetObject<Node> ()->GetDevice (0)->GetObject<NistLteUeNetDevice> () != 0) and (b->GetObject<Node> ()->GetDevice (0)->GetObject<NistLteUeNetDevice> () != 0))
            { 
               //TODO 
               // Added for V2V by Lorenzo Gibellini: V2V is always outdoor
                aIndoor = false; 
                bIndoor = false;
              // Calculate the pathloss based on the position of the nodes (outdoor/indoor)
              // a outdoor
              if (!aIndoor)
                {
                  // b outdoor
                  if (!bIndoor)
                    {
                      // Outdoor tranmission 
                      loss = Outdoor (a, b);
                      //NS_LOG_INFO (this << " Outdoor : " << loss);
                      //std::cout << this << " Outdoor : " << loss << "\r\n";
                    }

                  // b indoor
                  else
                    {
                      loss = Hybrid (a, b);
                      NS_LOG_INFO (this << " Hybrid : " << loss);
                    }
                } 

              // a is indoor
              else
                {
                  // b is indoor
                  if (bIndoor)
                    {
                      loss = Indoor (a, b).first;
                      //m_los = Indoor (a, b).second;
                      NS_LOG_INFO (this << " Inddor : " << loss );  
                    }

                  // b is outdoor
                  else
                    {
                      loss = Hybrid (a, b);
                      NS_LOG_INFO (this << " Hybrid : " << loss);
                    } 
                }
            }
          //Other cases (not nist nodes)
          else 
            {
              NS_FATAL_ERROR ("Non-valid NIST nodes");
            }
          loss = std::max (loss, 0.0);
          if (m_cacheLoss)
            {
              m_lossMap[couple] = loss; //cache value
            }
          m_nistPathlossTrace (loss, a->GetObject<Node> (), b->GetObject<Node> (), a->GetDistanceFrom (b), aIndoor, bIndoor);
        }      
    }

  
  return loss;
}

double 
NistOutdoorPropagationLossModel::DoCalcRxPower (double txPowerDbm,
					       Ptr<MobilityModel> a,
					       Ptr<MobilityModel> b) const
{
  NS_LOG_UNCOND(this << " Tx Power = " << txPowerDbm);
  std::cin.get();
 return (txPowerDbm - GetLoss (a, b));
}

double
BuildingsPropagationLossModel::EvaluateSigma (Ptr<MobilityBuildingInfo> a, Ptr<MobilityBuildingInfo> b)
const
{
  Ptr<LTENodeState> nodeStateA = Create<LTENodeState>();
  Ptr<LTENodeState> nodeStateB = Create<LTENodeState>();

  nodeStateA = a->GetObject<Node>()->GetObject<LTENodeState>();
  nodeStateB = b->GetObject<Node>()->GetObject<LTENodeState>();

  bool isVehicleA = 0;
  bool isVehicleB = 0;

  isVehicleA = nodeStateA -> IsVehicle();
  isVehicleB = nodeStateB -> IsVehicle();

 if(isVehicleA && isVehicleB)
 {
    return 3;
   
 }
 else
  {
    return 8;
  } 
}

double
BuildingsPropagationLossModel::DoCalcRxPower (double txPowerDbm, Ptr<MobilityModel> a, Ptr<MobilityModel> b) const
{
   
  // txPowerDbm = 23;
   double fc = 5.9;  // Center frequency in GHz (used for pathloss calculations)
   double hbs1 = 1.5;
   double hms1 = 1.5;

   double txAntHeight = hbs1;
   double rxAntHeight = hms1;
   double frequency = 5.9e9;
   double c = 299792458.0;
   double lambda = c / frequency;
   double m_systemLoss = 1.0;
   double sigma = 3; //dB, for LOS and NLOSv Highway scenario
   double shadowingValue = m_randVariable->GetValue (0.0, (sigma*sigma));
   double Pathloss;  
   double dist = a -> GetDistanceFrom (b);
          //shadowingValue = GetShadowing (a,b);
   // Fast Fading: Nakagami
/*   double m = 0.75; 

 

      // Schumaker2017 - V2V Highway 
   m = 2.7 * std::exp(-0.01 * (dist - 1)) + 1.0;

   Ptr<GammaRandomVariable> gammaRand = CreateObject<GammaRandomVariable>();
   double fastFading = - 10 * std::log10 (gammaRand -> GetValue (m, 1/m));

   



   double dbp = 4 * fc * 1e9 * (hbs1 - 1) * (hbs1 - 1)  / c;
 

   double plos = std::exp(-dist / 200);
   plos+=0;
   //double expectedLoss = GetLoss (a, b);
   double expectedLossFree = 21.5*std::log10 (dist) + 44.2 + 20*std::log10 (fc / 5.0);
   double expectedLossWinner;

   Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();
   
   if (dist >= dbp){
    expectedLossWinner = 40 * std::log10 (dist) + 9.45 - 17.3 * std::log10 (hbs1) - 17.3 * std::log10 (hms1) + 2.7 * std::log10 (fc/5.0);
    }
   else{
   expectedLossWinner = 22.7 * std::log10 (dist) + 41 + 20 *std::log10 (fc/5.0);
   }

  double expectedLoss = 0;

  expectedLoss += 0;
  double dCross = (4 * M_PI * txAntHeight * rxAntHeight) / lambda;
  double tmp = 0;
  
  if (dist <= dCross)
    {
      // We use Friis
      double numerator = lambda * lambda;
      tmp = M_PI * dist;
      double denominator = 16 * tmp * tmp * m_systemLoss;
      double pr = 10 * std::log10 (numerator / denominator);
      NS_LOG_DEBUG ("Receiver within crossover (" << dCross << "m) for Two_ray path; using Friis");
      NS_LOG_DEBUG ("distance=" << dist << "m, attenuation coefficient=" << pr << "dB");
      expectedLoss = pr;
       
    }
  else   // Use Two-Ray Pathloss
    {
      tmp = txAntHeight * rxAntHeight;
      double rayNumerator = tmp * tmp;
      tmp = dist * dist;
      double rayDenominator = tmp * tmp * m_systemLoss;
      double rayPr = 10 * std::log10 (rayNumerator / rayDenominator);
      NS_LOG_DEBUG ("distance=" << dist << "m, attenuation coefficient=" << rayPr << "dB");

      expectedLoss = rayPr; 

    }*/

//  double expectedLoss = 72.63 + 16*std::log10 (dist / 10) + 50;

   //double totalLoss = std::max(std::max(expectedLossWinner, expectedLossFree) + shadowingValue + fastFading, 0.0);
//   double totalLoss = std::max(expectedLossWinner + shadowingValue + fastFading, 0.0);

   Pathloss = 32.4 + 20 * std::log10(dist) + 20 * std::log10(fc);
   //double totalLoss = std::max(expectedLossWinner + shadowingValue, 0.0);
   double totalLoss = std::max(Pathloss + shadowingValue, 0.0);



   //NS_LOG_UNCOND("Tx Power: " << txPowerDbm << ", Expected Loss: " << expectedLossFree << ", Shadowing Loss: " << shadowingValue << ", Fast Fading: " << fastFading);
  
 /* Ptr<LTENodeState> nodeStateA = Create<LTENodeState>();
  Ptr<LTENodeState> nodeStateB = Create<LTENodeState>();
  bool isVehicleA = 0;
  bool isVehicleB = 0;

  nodeStateA = a->GetObject<Node>()->GetObject<LTENodeState>();
  nodeStateB = b->GetObject<Node>()->GetObject<LTENodeState>();
  */
  int nodeIdA = a->GetObject<Node>()->GetId();
  int nodeIdB = b->GetObject<Node>()->GetId();


  /*isVehicleA = nodeStateA -> IsVehicle();
  isVehicleB = nodeStateB -> IsVehicle();
  
 if(isVehicleA && isVehicleB){

  }*/

//  NS_LOG_DEBUG (this << " Distance: " << dist << " Shadowing " << shadowingValue << " Total loss = " << totalLoss << " Tx node: " << nodeIdA << " Rx node: " << nodeIdB << " Tx power = " << txPowerDbm << " TX - LOSS = " << txPowerDbm - totalLoss);
 // if(nodeIdA < 5) 
   //     std::cin.get();


  return txPowerDbm - totalLoss;
}


void
MyTestRandomUniform(void)
{   
   
    Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();
    std::ofstream randomFile;
    randomFile.open("results/sidelink/uniformRandom.csv", std::ios_base::app);
    for(uint32_t it = 0 ; it <100000; it++){
       randomFile << rand -> GetValue(0,1) << "\r\n";
    }
    randomFile.close();
}

void Print () {
        uint32_t ID;
        std::ofstream positFile;
        positFile.open("results/sidelink/posFile.txt",std::ofstream::app);
        for ( L = i; k != L; ++L)
        {
            Ptr<Node> node = *L;
            ID = node->GetId ();
                
            Ptr<MobilityModel> mob = node->GetObject<MobilityModel> ();
            if (! mob) continue; // Strange -- node has no mobility model installed. Skip.
            Vector pos = mob->GetPosition ();
          //  Vector vel = mob->GetVelocity ();
          //  std::cout << "Node "<< std::dec << ID <<" is at (" << pos.x << ", " << pos.y << ", " << pos.z << ")\n";
           // std::cout << "Node "<< std::dec << ID <<" is at (" << PrevX[ID-1] << ", " << PrevY[ID-1] << ", " << PrevZ[ID-1] << ")\n";
        //    std::cout << "Measurement Interval "<< MeasInterval << "\n";
            VelX[ID-1] = 0;
            VelY[ID-1] = 0;
            VelZ[ID-1] = 0;

            VelX[ID-1] = (pos.x-PrevX[ID-1])/MeasInterval;
            VelY[ID-1] = (pos.y-PrevY[ID-1])/MeasInterval;
            VelZ[ID-1] = (pos.z-PrevZ[ID-1])/MeasInterval;
                
            //std::cout << "Node "<< std::dec << ID <<" is travelling at (" << VelX[ID-1] << ", " << VelY[ID-1] << ", " << VelZ[ID-1] << ")\n";  

            PosX[ID-1] = pos.x;
            PosY[ID-1] = pos.y;
            PosZ[ID-1] = pos.z;
            
            if ((pos.x >= 1000) && (pos.x <= 3000)){
            positFile << Simulator::Now().GetSeconds() << "," << ID << "," << pos.x << "," << pos.y << "," << pos.z << "," << VelX[ID-1] << "," << VelY[ID-1] << "," << VelZ[ID-1] << "," << (int)VehicleTraffic[ID-1] << "\r\n";
           }
            
           
           PrevX[ID-1] = pos.x;
           PrevY[ID-1] = pos.y;
           PrevZ[ID-1] = pos.z;


        }
         
        Simulator::Schedule (MilliSeconds (TrepPrint), &Print);
      
        positFile.close();
    
       //std::cout << "Printing... \n";
}

void ETSICheck () 
{
  uint32_t ID;
  std::ofstream TempFile;
  TempFile.open("results/sidelink/TESTFile.txt",std::ofstream::app);
  for ( L = i; k != L; ++L)
  {
      Ptr<Node> node = *L;
      ID = node->GetId ();
      if (CHECK[ID-1] != 0)
      {
         T_Check_Counter[ID-1]++;
         if (T_Check_Counter[ID-1] >= 100)
         {
            T_Check_Counter[ID-1] = 0;
         }
         if (((std::abs(VelX[ID-1]-OriginVel[ID-1]) > 0.5) && (OriginVel[ID-1] != 0)) || (std::abs(PosX[ID-1]-OriginPos[ID-1]) > 4))
         {
            T_GenCam[ID-1] = (T_Check_Counter[ID-1])*T_Check;
            if (T_GenCam[ID-1] == 0)
            {
                T_GenCam[ID-1] += 100;       
            }
            TempFile << Simulator::Now().GetSeconds() << "| Node ID " << ID << " T_GenCAM = " << T_GenCam[ID-1] << " Posi: " << std::abs(PosX[ID-1]-OriginPos[ID-1]) << " Velo: " << std::abs(VelX[ID-1]-OriginVel[ID-1]) << "\r\n" ;
            CHECK[ID-1] = 0;
	    if(VehicleTraffic[ID-1] == 0x01)
            {
            Simulator::Remove (CONTEXT[ID-1]);  
            Simulator::ScheduleNow (&UdpClient::Send, THIS[ID-1]);  
	    }
         }
      }
  }
  TempFile.close();
  Simulator::Schedule (MilliSeconds (T_Check), &ETSICheck);

}


//--------------------------------------------------------------------------------------------------
//INIZIO
//MAIN
//--------------------------------------------------------------------------------------------------


int
main (int argc, char *argv[])
{


//-----DEBUGGING TOOLS------------------------------------------------------------------------
//  LogComponentEnable("NistLteUeMac", LOG_LEVEL_ALL);
//  LogComponentEnable("NistLteUePhy", LOG_LEVEL_ALL);
//  LogComponentEnable("NistLteRlcUm", LOG_LEVEL_ALL);
//  LogComponentEnable("NistLtePhyErrorModel", LOG_LEVEL_ALL);
//    LogComponentEnable("NistLteSpectrumPhy", LOG_LEVEL_ALL);
//    LogComponentEnable ("wns3_2017_pssch", LOG_LEVEL_ALL);
  //LogComponentEnable ("UdpClient", LOG_LEVEL_INFO);
//--------------------------------------------------------------------------------------------
  
  // Provides uniform random variables.
  Ptr<UniformRandomVariable> random = CreateObject<UniformRandomVariable>(); 
 
  // Initialize some values
  uint32_t mcs = 10; // The Modulation and Coding Scheme
  uint32_t rbSize = 8;  // Useless but needed to compile :)
  uint32_t ktrp = 1; // number of allocated subframes within the the TRP --> Mode 2 Parameter: Useless for V2V, but needed to compile :)
  uint32_t pscchLength = 8;
  std::string period="sf40";
  double dist = 20; // [m] : the distance between consecutive V-UEs in a single lane
  double simTime = 24; // The corrected simulation Time --> this is actually more
  double ueTxPower = 23.0; // [dBm]
  uint32_t ueCount = vehiclesCount; // Number of V-UEs 
  bool verbose = true;
  double trep = 0.1; // [s] the CAM repetition period
  uint32_t packetSize = 158; // [Bytes] The number be --> 190Bytes at PHY layer

  uint32_t pucchSize = 0;          // PUCCH size in RBs --> zero because we assume we do not use the uplink; sidelink occurs over a distinct frequency band (5.9GHz)


//@DOC: Questi parametri servono per la topologia della rete fissa (eNodeB); noi non usiamo eNodeB,
// Ma il simulatore ne ha bisogno in una fase iniziale, per configurare i V-UEs all'inizio, come in una rete reale.
// A questo proposito il codice andrebbe perfezionato e orientato solo alle comunicazioni autonome, ma in questo modo rimane flessibile e
// utilizzabile anche per comunicazioni infrastrutturate 
// For Infrastructure 
  uint32_t nbRings = 1;            // Number of rings in hexagon cell topology 
  double isd = 100;                 // Inter Site Distance
  double minCenterDist = 1;        // Minimum deploy distance to center of cell site


  bool CtrlErrorModelEnabled = true; // Enable error model in the PSCCH
  bool DropOnCollisionEnabled = false; // Drop PSSCH and PSCCH messages on conflicting scheduled resources

  bool randomV2VSelection = false; // --> Resources are not randomly selected; rather, the Mode4 algo is employed
 //uint32_t numPackets = 1;
 
// Change the random run  
  uint32_t seed = 867; // this is the default seed;
  uint32_t runNumber = 1; // this is the default run --> this will be overridden shortly...
  uint32_t maxDENMPackets = 0; // the maximum allowable number of generated DENM packets --> if 0, only CAMs are generated
  
  double offsetStartTime = 0; // [s] Time at which V-UEs different from V-UE number 1 can start to generate traffic
  double mean = 50; // [ms]
  double bound = 0.0;  
  
  TESIlike = false;
//@DOC: Queste istruzioni servono ad impostare lo script in modo da accettare dei parametri in ingresso quando
// viene lanciato da terminale; questo è estremamente utile per le simulazioni batch in cui si itera, ad esempio, 
// sul numero di run random (per fare simulazioni scorrelate) 
// All'inizio, queste variabili, che noi rendiamo "speciali", devono essere inizializzate, no idea why, poi, con 
// il metodo cmd.Parse, se nel comando di lancio c'è il parametro specificato, il valore viene caricato nella variabile

 CommandLine cmd;
  cmd.AddValue ("period", "Sidelink period", period);
  cmd.AddValue ("pscchLength", "Length of PSCCH.", pscchLength);
  cmd.AddValue ("ktrp", "Repetition.", ktrp);
  cmd.AddValue ("mcs", "MCS.", mcs);
  cmd.AddValue ("rbSize", "Allocation size.", rbSize);
  cmd.AddValue ("verbose", "Print time progress.", verbose);
  cmd.AddValue ("trep", "The Application-layer message repetition", trep);
  cmd.AddValue ("dist", "The distance between V-UEs", dist);
  cmd.AddValue ("ueTxPower", "The UE TX Power", ueTxPower);
  cmd.AddValue ("packetSize", "The Application-layer Packet Size in Byte", packetSize);
  cmd.AddValue ("simTime", "The simulated time in seconds", simTime);
  cmd.AddValue ("seed", "The random seed", seed);
  cmd.AddValue ("runNo", "The run number", runNumber);
  cmd.AddValue ("offsetStartTime", "The time offset [s] when UEs 2:end start to generate packets", offsetStartTime);
  cmd.AddValue ("randomV2VSelection", "Whether V2V resources are randomly selected in autonomous scheduling mode", randomV2VSelection);
   cmd.AddValue ("maxDENMPackets", "The maximum number of DENM packets to be generated", maxDENMPackets);

  cmd.Parse(argc, argv);


//@DOC: Lascia stare: è solo un trucchetto per fare in modo che, se c'è un solo trasmettitore (offsetStartTime > 0), 
// il tempo di simulazione venga allungato un po', in modo da avere comunque un numero accettabile di pacchetti su cui fare statistiche.
// Valutare le performance dei CAM con singolo TX (no collisioni) è importante per separare gli errori di collisione da quelli dovuti
// ad uno scarso SNR risultante da un'attenuazione eccessiva (es. deep fade, lunga distanza tra TX e RX)
   if (offsetStartTime > 0)
     {
       simTime = simTime * 5;
       //ueCount = (uint32_t) (1400 / dist);
     }  
 
 //Clear files
 /* remove("results/sidelink/TxFile.txt");
  remove("results/sidelink/RxFile.txt");
  remove("results/sidelink/nTBFile.csv");
  remove("results/sidelink/tbRxFile.csv");
  remove("results/sidelink/tbTxFile.csv");
  remove("results/sidelink/RBFile.csv");
  remove("results/sidelink/PsschRsrp.csv");
  remove("results/sidelink/collisions.csv");
  remove("results/sidelink/collisionFile.unimo");
  remove("results/sidelink/detectedRBFile.unimo");
  remove("results/sidelink/L1Size.csv");
  remove("results/sidelink/L2fileAlert.txt");
  remove("results/sidelink/RLCtxQueueFile.csv");
  remove("results/sidelink/initialTx.csv");
  remove("results/sidelink/SFOffset.csv");
  remove("results/sidelink/sensingDebug.txt");
  remove("results/sidelink/allocationFile.txt");
  remove("results/sidelink/allocationFilePHY.txt");*/
   

  std::ofstream readme;
  readme.open ("results/sidelink/simREADME.txt");
  readme << "----------------------" << "\r\n" << "\r\n" << "Simulation:" << "\r\n - seed = " << seed << "\r\n - run = " << runNumber << "\r\n - RandomV2VSelection = " <<  (int)randomV2VSelection << "\r\n - UDP packet size = " << packetSize << " B\r\n - T_rep = " << trep << " s\r\n - UE count = " << ueCount << "\r\n - UE distance = " << dist << "m \r\n - MCS = " << mcs << "\r\n" ;
  readme.close ();
  

// Set the random seed and run
  RngSeedManager::SetSeed (seed);
  RngSeedManager::SetRun (runNumber);

  //@LUCA setting the random variable generator for working with aperiodic traffic
  RndExp = CreateObject<ExponentialRandomVariable> ();
  RndExp->SetAttribute ("Mean", DoubleValue(mean));
  RndExp->SetAttribute ("Bound", DoubleValue(bound));

// Using the ns3::Config::* Functions to set up the simulations
  NS_LOG_INFO ("Configuring UE settings...");
  Config::SetDefault ("ns3::NistLteUeMac::SlGrantMcs", UintegerValue (mcs)); //The MCS of the SL grant, must be [0..15] (default 0)
  Config::SetDefault ("ns3::NistLteUeMac::SlGrantSize", UintegerValue (rbSize)); //The number of RBs allocated per UE for sidelink (default 1)
  Config::SetDefault ("ns3::NistLteUeMac::Ktrp", UintegerValue (ktrp)); //The repetition for PSSCH. Default = 0
  Config::SetDefault ("ns3::NistLteUeMac::PucchSize", UintegerValue (pucchSize)); 

  // Configure Power Control
  Config::SetDefault ("ns3::NistLteUePhy::TxPower", DoubleValue (ueTxPower));
  Config::SetDefault ("ns3::NistLteUePowerControl::Pcmax", DoubleValue (ueTxPower));
  Config::SetDefault ("ns3::NistLteUePowerControl::PoNominalPusch", IntegerValue (-106));
  Config::SetDefault ("ns3::NistLteUePowerControl::PscchTxPower", DoubleValue (ueTxPower)); // Setting the Transmission Power for the control channel
  Config::SetDefault ("ns3::NistLteUePowerControl::PsschTxPower", DoubleValue (ueTxPower)); // Setting the Transmission Power for the sidelink channel
  Config::SetDefault ("ns3::NistLteUePhy::RsrpUeMeasThreshold", DoubleValue (-10.0)); //Setting the RSRP threshold (further investigation needed)

  // Configure error model
  Config::SetDefault ("ns3::NistLteSpectrumPhy::CtrlErrorModelEnabled", BooleanValue (CtrlErrorModelEnabled));
  Config::SetDefault ("ns3::NistLteSpectrumPhy::CtrlFullDuplexEnabled", BooleanValue (!CtrlErrorModelEnabled));
  Config::SetDefault ("ns3::NistLteSpectrumPhy::DropRbOnCollisionEnabled", BooleanValue (DropOnCollisionEnabled));  // The mechanism is quite different
 

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

  lteHelper->SetPathlossModelType ("ns3::Nist3gppPropagationLossModel");  //

// Anche qui: potresti aver bisogno (o voglia) di cambiare modello di propagazione; altri modelli 
// ci sono ma sono disseminati in varie classi, quindi è meglio settare la frequenza in più posti :(  
  Config::SetDefault ("ns3::FriisPropagationLossModel::Frequency", DoubleValue (5.9e9));

  /*Attributes for Two-Ray Ground Propagation Model*/
  Config::SetDefault ("ns3::TwoRayGroundPropagationLossModel::Frequency", DoubleValue (5.9e9)); // [Hz]
  Config::SetDefault ("ns3::TwoRayGroundPropagationLossModel::HeightAboveZ", DoubleValue (0.5)); // [m]

  /*Attributes for Log-Distance Propagation Model*/
  Config::SetDefault ("ns3::LogDistancePropagationLossModel::ReferenceLoss", DoubleValue (47.85)); // 
                                          
  /*Lower antenna height yields higher propagation loss*/
  
  //lteHelper->SetPathlossModelType ("ns3::NakagamiPropagationLossModel");


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
    

  
//@DOC: Import SUMO traces
/*
std::string traceFile = "scratch/highwayTrace.tcl";

Ns2MobilityHelper ns2 = Ns2MobilityHelper (traceFile);
ns2.Install (ueResponders.Begin(), ueResponders.End()); // configure movements for each node, while reading trace file

*/

// The number of ueResponders must match the number of nodes in the SUMO trace (also < works ?)
// Using the ConstantPositionMobilityModel just for testing reasons. So that I didn't have to generate a new SUMO trace file every time that I change the numer of UEs
  Ns2MobilityHelper ns2 = Ns2MobilityHelper("/home/luca/Desktop/SumoTraces/Random/3+3Lanes/25s/850VEH/RUN1/RUN1.tcl");
  ns2.Install(ueResponders.Begin(), ueResponders.End());
/*  MobilityHelper mobilityUE;
  mobilityUE.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobilityUE.Install (ueResponders);*/

/*  MobilityHelper mobilityUE;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject <ListPositionAllocator>();
  positionAlloc ->Add(Vector(0, 0, 0)); // node1
  positionAlloc ->Add(Vector(400, 0, 0)); // node2
  positionAlloc ->Add(Vector(1000, 0, 0)); // node3
  positionAlloc ->Add(Vector(1500, 0, 0)); // node4
  mobilityUE.SetPositionAllocator(positionAlloc);
  mobilityUE.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobilityUE.Install (ueResponders);*/

 // Needed for the Print function
  i = ueResponders.Begin();   
  k = ueResponders.End(); // Get an iterator which indicates past-the-last Node in the container

// Nodes can be retrieved from the container in two ways. First, using Get() and referring directly to an index into the container.
// Second, using an iterator, which is the best solution in a for-loop to run through the Nodes

   for ( L = i; k != L; ++L)
  {  
     int ID;
     Ptr<Node> node = *L;
     ID = node->GetId ();
     NS_LOG_DEBUG("ID: " << ID << " Memory address " << node);
     AddressMap.insert(std::pair< int, Ptr<Node> >(ID, node)); // Store the node address and use it at spectrum level
  //   std::cin.get();
     Ptr<MobilityModel> mob = node->GetObject<MobilityModel> ();
     if (! mob) continue; // Strange -- node has no mobility model installed. Skip.
     Vector pos = mob->GetPosition ();
     PrevX[ID-1] = pos.x;
     PrevY[ID-1] = pos.y;
     PrevZ[ID-1] = pos.z;
     T_GenCam[ID-1] = 1000;  //default value is 1000ms
     T_Check_Counter[ID-1] = 0;
     OriginPos[ID-1] = 0;
     OriginVel[ID-1] = 0;
     VelX[ID-1] = 0;
     VelY[ID-1] = 0;
     VelZ[ID-1] = 0;
     PosX[ID-1] = 0;
     PosY[ID-1] = 0;
     PosZ[ID-1] = 0;
     GeneratorCounter[ID-1] = 0;
     CHECK[ID-1] = 0;   
     FIRST[ID-1] = 1;
     VehicleTraffic[ID-1] = 0x01;
     if (ID % 10 == 0)
        VehicleTraffic[ID-1] = 0x00;
   }

//  VehicleTraffic[0] = 0x01;
//  VehicleTraffic[1] = 0x01;
//  VehicleTraffic[2] = 0x00;
//  VehicleTraffic[3] = 0x01;

  MeasInterval = ((double)TrepPrint)/1000;
 
 // Simulator::ScheduleNow (&Print);  // Print the initial position of the nodes in the output file


  //mobility.SetPositionAllocator (positionAlloc);

  // @NEW, Luca:
  NS_LOG_INFO ("Installing UE network devices...");
  NetDeviceContainer ueDevs = lteHelper->InstallUeDevice (ueResponders);
//@OLD version, Lorenzo:
/*
NetDeviceContainer ueDevs;
NetDeviceContainer ueRespondersDevs = lteHelper->InstallUeDevice (ueResponders); 
ueDevs.Add (ueRespondersDevs);
*/

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
//      clientApps.Get(i) -> SetStartTime(Seconds (rand -> GetValue(0.001, 1.0)));
      clientApps.Get(i)-> SetStartTime(MilliSeconds (std::round(rand -> GetValue(1, 100))*10));
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
  clientRespondersSrvApps.Start (Seconds (0.05));  // Start ASAP
  clientRespondersSrvApps.Stop (Seconds (simTime+0.9));


  // At this point, each V-UE mounts both a Client and a Server (Sink) application
 



  
  
  NS_LOG_INFO ("Creating sidelink configuration...");
  uint32_t groupL2Address = 0x00;
  


 Ptr<NistSlTft> tftRx = Create<NistSlTft> (NistSlTft::BIDIRECTIONAL, groupAddress, groupL2Address);
  proseHelper->ActivateSidelinkBearer (Seconds(0.1), ueDevs, tftRx); // Set accordingly with clientApps StartTime! 0.1 Seconds is the ActivationTime of the Sidelink bearer


  Ptr<LteUeRrcSl> ueSidelinkConfiguration = CreateObject<LteUeRrcSl> ();
  ueSidelinkConfiguration->SetSlEnabled (true);

  NistLteRrcSap::SlPreconfiguration preconfiguration;
  preconfiguration.preconfigGeneral.carrierFreq = 54900; //original: 23330
  preconfiguration.preconfigGeneral.slBandwidth = 50;    //original: 50 PRBs --> ~ 10MHz (assuming 200kHz per RB) 
  preconfiguration.preconfigComm.nbPools = 1; // the number of pools 


  NistSlPreconfigPoolFactory pfactory;
  NistSlResourcePoolFactory commfactory;
 

  //build PSCCH bitmap value
  uint64_t pscchBitmapValue = 0x0;
  for (uint32_t i = 0 ; i < pscchLength; i++) {
    pscchBitmapValue = pscchBitmapValue >> 1 | 0x8000000000;
  }
  std::cout << "bitmap=" << std::hex << pscchBitmapValue << '\n'; // this is the PSCCH subframe pool bitmap : 1111 1111 0000 0000 0000 0000 0000 0000 0000 0000b --> the first 8 subframes can be used for the PSCCH transmission, the remaining subframes are eligible for PSSCH allocation. 

/*  std::ofstream bitmapfile;
  bitmapfile.open("bitmapfile.txt",std::ios_base::app);
  bitmapfile << "bitmap=" << std::hex << pscchBitmapValue << '\n';
  bitmapfile.close();*/

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
  lteHelper->EnableTraces (); // Non le uso, ma ok
  

  //Simulator::Schedule(Seconds(1.0), &MyTestRandomUniform);


//@DOC: Questi sono i metodi della classe ns3::Simulator, che servono ad orchestrare il simulatore
// NS3 è un simulatore event-triggered: viene costruita una lista di eventi, ad ognuno dei quali viene associato l'istante di tempo
// al quale l'evento deve essere fatto avvenire (risolto e rimosso dalla event queue).
// Si può aggiungere manualmente un evento alla lista con il metodo ns3::Simulator::Schedule
  Simulator::ScheduleNow (&Print);
  Simulator::ScheduleNow (&ETSICheck);


  NS_LOG_INFO ("Starting simulation...");
  Simulator::Stop (Seconds (simTime+1)); // Teniamo un po' di margine per essere sicuri che i messaggi arrivino tutti 
  Simulator::Run ();
  Simulator::Destroy ();

  NS_LOG_INFO ("Done.");

  return 0;

}

