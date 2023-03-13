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

NS_LOG_COMPONENT_DEFINE ("wns3_2017_pssch");

using namespace ns3;

uint64_t packetID = 0;
uint64_t nTransportBlocks = 0;
uint64_t nTotPackets = 0;
Ipv4Address groupAddress; //use multicast address as destination --> broadcast 

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
  m_vehicle = true;
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
  return true;
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



// define this class in a public header
class MyTag : public Tag
{
public:
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (TagBuffer i) const;
  virtual void Deserialize (TagBuffer i);
  virtual void Print (std::ostream &os) const;

  // these are our accessors to our tag structure

  //Setters

  void SetSimpleValue (uint8_t value);
  void SetIntValue (uint64_t value);
  void SetDoubleValue (double value);

  void SetGenTime(double value);
  void SetPacketId(uint64_t value);
  void SetGenPosX(double value);
  void SetGenPosY(double value);
  void SetNodeId(uint64_t value);
  void SetNumHops(uint32_t value);
  
  void SetMessageType(uint8_t value);

 // Getters
  uint8_t GetSimpleValue (void) const;
  uint64_t GetIntValue (void) const;
  double GetDoubleValue (void) const ;

  double GetGenTime(void) const;
  uint64_t GetPacketId(void) const;
  double GetGenPosX(void) const;
  double GetGenPosY(void) const;
  uint64_t GetNodeId(void) const;
  uint32_t GetNumHops(void) const;

  uint8_t GetMessageType(void) const;

private:
  uint8_t m_simpleValue;
  uint64_t m_intValue;
  double m_doubleValue;

  double m_genTime;
  uint64_t m_packetId;
  double m_genPosX;
  double m_genPosY;
  uint64_t m_nodeId;
  uint32_t m_numHops;

  uint8_t m_messageType;  // 0x00 = CAM
                          // 0x01 = DENM
  
};

TypeId 
MyTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::MyTag")
    .SetParent<Tag> ()
    .AddConstructor<MyTag> ()
    .AddAttribute ("SimpleValue",
                   "A simple value",
                   EmptyAttributeValue (),
                   MakeUintegerAccessor (&MyTag::GetSimpleValue),
                   MakeUintegerChecker<uint8_t> ())
  ;
  return tid;
}
TypeId 
MyTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}
uint32_t 
MyTag::GetSerializedSize (void) const
{
   return sizeof m_doubleValue + sizeof m_intValue + sizeof m_simpleValue + sizeof m_genTime + sizeof m_packetId + sizeof m_genPosX + sizeof m_genPosY + sizeof m_nodeId + sizeof m_numHops + sizeof m_messageType;
}
void 
MyTag::Serialize (TagBuffer i) const
{
  i.WriteU8 (m_simpleValue);
  i.WriteU64 (m_intValue);
  i.WriteDouble(m_doubleValue);

  i.WriteDouble(m_genTime);
  i.WriteDouble(m_genPosX);
  i.WriteDouble(m_genPosY);
  i.WriteU64 (m_packetId);
  i.WriteU64 (m_nodeId);
  i.WriteU32 (m_numHops);

  i.WriteU8 (m_messageType);

}
void 
MyTag::Deserialize (TagBuffer i)
{
  m_simpleValue = i.ReadU8 ();
  m_intValue = i.ReadU64 ();
  m_doubleValue = i.ReadDouble();

  m_genTime = i.ReadDouble();
  m_genPosX = i.ReadDouble();
  m_genPosY = i.ReadDouble();
  m_packetId = i.ReadU64();
  m_nodeId = i.ReadU64();
  m_numHops = i.ReadU32();
 
  m_messageType = i.ReadU8();
  
}
void 
MyTag::Print (std::ostream &os) const
{
  os << "int64 = " << (uint64_t)m_intValue << "\nint8 = " << (uint8_t)m_simpleValue;

  std::ofstream outfile;
  outfile.open("tagFile.txt", std::ios_base::app);
  outfile << "int64 = " << (uint64_t)m_intValue << "\nint8 = " << (uint8_t)m_simpleValue;
  outfile.close();
}
void 
MyTag::SetSimpleValue (uint8_t value)
{
  m_simpleValue = value;
}

void
MyTag::SetIntValue (uint64_t value)
{
  m_intValue = value;
}

void
MyTag::SetDoubleValue (double value)
{
  m_doubleValue = value;
}

void
MyTag::SetGenTime (double value)
{
  m_genTime = value;
}

void
MyTag::SetGenPosX (double value)
{
  m_genPosX = value;
}

void
MyTag::SetGenPosY (double value)
{
  m_genPosY = value;
}

void
MyTag::SetPacketId (uint64_t value)
{
  m_packetId = value;
}

void
MyTag::SetNodeId (uint64_t value)
{
  m_nodeId = value;
}

void
MyTag::SetNumHops (uint32_t value)
{
  m_numHops = value;
}

void
MyTag::SetMessageType (uint8_t value)
{
  m_messageType = value;
}

uint8_t 
MyTag::GetSimpleValue (void) const
{
  return m_simpleValue;
}



//Getters

uint64_t 
MyTag::GetIntValue (void) const
{
  return m_intValue;
}

double
MyTag::GetDoubleValue (void) const
{
  return m_doubleValue;
}

double
MyTag::GetGenTime (void) const
{
  return m_genTime;
}

double
MyTag::GetGenPosX (void) const
{
  return m_genPosX;
}

double
MyTag::GetGenPosY (void) const
{
  return m_genPosY;
}

uint64_t
MyTag::GetPacketId (void) const
{
  return m_packetId;
}

uint64_t
MyTag::GetNodeId (void) const
{
  return m_nodeId;
}

uint32_t
MyTag::GetNumHops (void) const
{
  return m_numHops;
}

uint8_t 
MyTag::GetMessageType (void) const
{
  return m_messageType;
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
      nTransportBlocks += 1;
      std::ofstream nTBFile;
      nTBFile.open("results/sidelink/nTBFile.csv", std::ios_base::app);
      nTBFile << "Time: " << Simulator::Now().GetSeconds() << ", TBindex: " << nTransportBlocks << ", NodeId: " << txParams->nodeId << ", Duration: " << duration << "\r\n";
      nTBFile.close();
      
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

void
ReceivePacket(Ptr<Socket> socket);

static void 
GenerateTraffic (Ptr<Node> node, Ptr<Socket> source, uint32_t pktSize, 
                             uint32_t pktCount, Time pktInterval);


void
UdpClient::Send (void)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_sendEvent.IsExpired ());

  Ptr<Node> currentNode = GetNode();
  uint32_t nodeId = currentNode -> GetId();
  double numHops = 0;  

  SeqTsHeader seqTs;
  seqTs.SetSeq (m_sent);
  Ptr<Packet> p = Create<Packet> (m_size-(8+4)); // 8+4 : the size of the seqTs header
  p->AddHeader (seqTs);
  
   // create a tag.
  //MyTag newTag;
  
  V2xLteTag v2xTag;
  v2xTag.SetGenTime (Simulator::Now ().GetSeconds ());

  if (GetNode() -> GetId () == 1 && m_sentDENM < m_MaxSentDENM ) // @DOC: Solo il veicolo 1 invia DENM
     {
        v2xTag.SetMessageType (0x01); //DENM
        v2xTag.SetPPPP (0x08); // Pro-Se Per-Packet Priority value (PPPP) for DENM
        m_size = 960; // UDP Packet size in Bytes
     }
  else
     {
        v2xTag.SetMessageType (0x00); //CAM
        v2xTag.SetPPPP (0x00); // PPPP for CAM ; size has already been set
     }
  

  v2xTag.SetPrsvp ((uint32_t) (1000 * m_interval.GetSeconds ())); // the required PHY reservation interval 

  if (v2xTag.GetMessageType () == 0x01)
    {
        v2xTag.SetPdb (100);
    }
  else
    {
        v2xTag.SetPdb (v2xTag.GetPrsvp ()); // the Packet Delay Budget
    }


  

  v2xTag.SetSimpleValue (0x57); // No idea what I meant :(, probably nothing
  
/* @DOC: il Tag è un elemento che serve per associare una sorta di struttura dati (un oggetto in realtà) 
   ad un pacchetto. Serve dunque ad inserire informazioni dentro ad un pacchetto, che altrimenti
   ns3 tratterebbe come un insieme di un certo numero di byte.
   Ho creato la classe v2xTag per trasferire nel CAM e nel DENM informazioni specifiche (posizione, numero del nodo, timestamp, etc) 
*/


  packetID = packetID+1; // increment the packet ID number 
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


 // Attach the tag
  
   p->AddByteTag (v2xTag);

  



  
/*@DOC: salviamo nel file TxFile.txt l'evento di generazione del pacchetto
  Struttura del file:

  packetID, generation Time [s], Node ID, Pos X [m], Pos Y [m], Message Type [0->CAM, 1->DENM]

*/
  if ((xPosition >= 1000) && (xPosition <= 3000)){
  std::ofstream filetest;
  filetest.open("results/sidelink/TxFile.txt", std::ios_base::app);
  filetest << packetID << "," << timeSec << "," << nodeId << "," << xPosition << "," << yPosition << "," << (int)v2xTag.GetMessageType ()  << "\r\n" ;
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
         Ptr<Socket> sendSocket = Socket::CreateSocket(currentNode, UDPtid);
         sendSocket -> SetAllowBroadcast(true);
  Ipv4Address destAddress = groupAddress;
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
 
  if ((sendSocket->Send (p)) >= 0)
    {
      ++m_sent;
      if (v2xTag.GetMessageType () == 0x01)
        {
            ++m_sentDENM;
        }
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
      m_sendEvent = Simulator::Schedule (m_interval, &UdpClient::Send, this); //@DOC: si potrebbe trarre spunto da qui per rischedulare un campionamento periodico della velocità?
    }
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



void
sendPcktOverSocket(Ptr<Packet> packet, Ptr<Socket> sendSocket)
{
  std::ofstream filetest, fileTx;
  Ptr<Node> currentNode = sendSocket -> GetNode();
  uint32_t nodeId = currentNode -> GetId();

  bool authorizedToSend = true; // authorization to rebroadcast the received packet

  V2xLteTag pckTag;

  //retrieve tag from packet
  
  //Time time = Simulator::Now();
  //double timeSec = time.GetSeconds();

 if (packet->FindFirstMatchingByteTag (pckTag))
   {

 //Ptr<MobilityModel> mobility = currentNode->mobilityModel>();
  //Vector currentPos = mobility -> GetPosition();
 
  //double xPosition = currentPos.x;
  //double yPosition = currentPos.y;

  uint32_t numHopsNow = pckTag.GetNumHops();
  pckTag.SetNumHops(numHopsNow+1);
  packet->RemoveAllByteTags();

  Ptr<Packet> pCopy = packet->Copy ();
  pCopy->AddByteTag(pckTag);

 if(authorizedToSend){
  if((sendSocket -> Send(pCopy) >= 0))
       { /*
           filetest.open("results/sidelink/sentPacket.txt", std::ios_base::app);
           filetest << Simulator::Now().GetSeconds() << "," << nodeId << "\r\n";
           filetest.close();


           
fileTx.open("results/sidelink/TxFile.txt", std::ios_base::app);
          fileTx <<  Simulator::Now().GetSeconds() << "," << nodeId <<  "\r\n" ;
          fileTx.close();*/
          
 
       }
    else
     {
        std::cout << "Problems in rebroadcasting from node " << nodeId << "\r\n";
     }
 }
 }

 sendSocket -> Close();
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
 
 if(rebroadcast)  // Rebroadcasting Policy
  {
      Ptr<Packet> pCopy = packet->Copy ();

  Simulator::ScheduleWithContext(sendSocket -> GetNode() -> GetId(), Seconds(0.003), &sendPcktOverSocket, pCopy, sendSocket);

 }


//EventId evento = Simulator::Schedule(Seconds(0.001), UdpClient::Send, this);

    
} //end if(multihop)
 
  SeqTsHeader seqTs;
          packet->RemoveHeader (seqTs);
          uint32_t currentSequenceNumber = seqTs.GetSeq ();

         currentSequenceNumber+=0;
 if ((rxPosX >= 950) && (rxPosX <= 3050)){
       filetest.open("results/sidelink/RxFile.txt",std::ios_base::app);
          filetest << rxPacketID << "," << tGenSec << "," <<  timeSec << "," << timeSec - tGenSec <<"," << nodeId << "," << packet->GetSize () << "," << TxRxDistance << "," << numHops << "," << (int) messageType << "," <<  (int) alreadyReceived << "\r\n" ;
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



static void 
GenerateTraffic (Ptr<Node> node, Ptr<Socket> source, uint32_t pktSize, 
                             uint32_t pktCount, Time pktInterval)
{
  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
 // Ptr<Socket> sourceGen = Socket::CreateSocket (node, tid);

   //Ptr<UdpSocket> source = DynamicCast<UdpSocket> (sourceGen);

 
  uint16_t destPort = 80;


  InetSocketAddress remote = InetSocketAddress (groupAddress, destPort);

/*
  source->SetAllowBroadcast (true);

if (Ipv4Address::IsMatchingType(groupAddress) == true)
     {
  source->Bind ();
  source->Connect (remote);
   }
else if (Ipv6Address::IsMatchingType(groupAddress) == true)
        {
          source->Bind6 ();
          source->Connect (Inet6SocketAddress (Ipv6Address::ConvertFrom(groupAddress), destPort));
        }*/

 
  //source->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());

  uint32_t nodeId = source-> GetNode() -> GetId();

  if (pktCount > 0)
    {
      Ptr<Packet> p = Create<Packet> (pktSize-(8+4)); // 8+4 : the size of the seqTs header

      SeqTsHeader seqTs;
      seqTs.SetSeq (packetID);
 
      p->AddHeader (seqTs);
  
   // create a tag.
   MyTag newTag;
  
  
  newTag.SetSimpleValue (0x57);
  
  //MyTag longTag;
  packetID = packetID+1;
  newTag.SetIntValue(packetID);
  Time time = Simulator::Now();
  double timeSec = time.GetSeconds();

  newTag.SetDoubleValue(timeSec);

  Ptr<MobilityModel> mobility = source->GetNode()->GetObject<MobilityModel>();
  Vector currentPos = mobility -> GetPosition();
 
  double xPosition = currentPos.x;
  double yPosition = currentPos.y;

  // Set generation position in tag

  newTag.SetGenPosX(xPosition);
  newTag.SetGenPosY(yPosition);
  

  //uint64_t packetID = p->GetUid();
  uint64_t tagValue;

   p->AddByteTag (newTag);
   

  


    //DelayJitterEstimation
  //DelayJitterEstimation::PrepareTx(p);

ByteTagIterator iter = p->GetByteTagIterator();
  while(iter.HasNext()){
     ByteTagIterator::Item item = iter.Next();
     NS_ASSERT (item.GetTypeId ().HasConstructor ());
      Callback<ObjectBase *> constructor = item.GetTypeId ().GetConstructor ();
      NS_ASSERT (!constructor.IsNull ());
      ObjectBase *instance = constructor ();
      MyTag *tag = dynamic_cast<MyTag *> (instance);
      NS_ASSERT (tag != 0);
      item.GetTag (*tag);
      //tag->Print (std::cout);
      tagValue = tag->GetIntValue();
       tagValue+=1;

      //delete tag;
      if (iter.HasNext ())
        {
          std::cout << " ";
        }
    } 



 if(source->SendTo (p, 0, remote)>0){
  
   
   nTotPackets += 1;
   

      NS_LOG_UNCOND ("Sent packet no. " << nTotPackets  << " at node " << source -> GetNode() -> GetId() << " at time " << Simulator::Now().GetSeconds());
   
 /* Ptr<Socket> recvSink = Socket::CreateSocket (node, tid);
  InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), 8000);
  recvSink->Bind (local);
  recvSink->SetAllowBroadcast(true);
  recvSink->Listen ();
  recvSink->ShutdownSend ();
  recvSink->SetRecvCallback (MakeCallback (&ReceivePacket)); */
 if ((xPosition >= 1000) && (xPosition <= 3000)){
  std::ofstream filetest;
        filetest.open("results/sidelink/TxFile.txt", std::ios_base::app);
          filetest << packetID << "," << timeSec << "," << nodeId << "," << xPosition << "," << yPosition << "\r\n" ;
          filetest.close(); 
}

 /*Ipv4StaticRoutingHelper ipv4RoutingHelper;

 Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (source -> GetNode()->GetObject<Ipv4> ());


      ueStaticRouting->SetDefaultMulticastRoute(1);*/

 //source->Close ();
}

      

      

      /*Simulator::Schedule (pktInterval, &GenerateTraffic, 
                           socket, pktSize,pktCount-1, pktInterval);*/
       
    }
  else
    {
      //source->Close ();
    }
}



void 
ReceivePacket (Ptr<Socket> socket)
{
  //Address from;
  Ptr<Node> node = socket -> GetNode ();
  Ptr<Packet> packet;
  Address from;
  bool multihop = false;

  std::ofstream filetest;
        

  while ((packet = socket->RecvFrom (from)))
    {
      if(packet->GetSize() == 0){
          break;
        }
   // filetest.open("results/sidelink/RxFile.txt", std::ios_base::app);
    //    filetest << node -> GetId() << " Rx Available: " << socket->GetRxAvailable() << "\r\n";
     //   filetest.close();

      NS_LOG_UNCOND ("Received packet no. " << nTotPackets  << " at node " << node -> GetId() << " at time " << Simulator::Now().GetSeconds());
 
 if(multihop)
   {
     uint32_t packetSize = 1000; // bytes
  uint32_t numPackets = 1;
  double interval = 1.0; // seconds
  Time interPacketInterval = Seconds (interval);

  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  
  /*Ptr<Socket> source = Socket::CreateSocket (node, tid);

  Ipv4Address destAddress("255.255.255.255");
  uint16_t destPort = 8000;
  InetSocketAddress remote = InetSocketAddress (destAddress, destPort);
  source->SetAllowBroadcast (true);

  if (Ipv4Address::IsMatchingType(destAddress) == true){
    source->Bind();
    source->Connect (remote);
  }
  source->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());*/

  if ( node->GetId() < 5)
     {
  Simulator::ScheduleWithContext (node->GetId (),
                                  Seconds (1), &GenerateTraffic, node, socket,
                                  packetSize, numPackets, interPacketInterval);
      }
    } // end if(multihop)

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
   double fc = 5.9;
   double hbs1 = 1.5;
   double hms1 = 1.5;
    
   double txAntHeight = hbs1;
   double rxAntHeight = hms1;
 double frequency = 5.9e9;
 double c = 299792458.0;
 double lambda = c / frequency;
 double m_systemLoss = 1.0;
   double sigma = 3; //dB
   double shadowingValue = m_randVariable->GetValue (0.0, (sigma*sigma));
          //shadowingValue = GetShadowing (a,b);
   // Fast Fading: Nakagami
   double m = 0.75; 

   double dist = a -> GetDistanceFrom (b);

      /* Schumaker2017 - V2V Highway */
   m = 2.7 * std::exp(-0.01 * (dist - 1)) + 1.0;

   Ptr<GammaRandomVariable> gammaRand = CreateObject<GammaRandomVariable>();
   double fastFading = - 10 * std::log10 (gammaRand -> GetValue (m, 1/m));

   
  /* 
    if (dist <= 4.7)
      {
           m = 3.01;
      }
    else if (dist > 4.7 && dist <= 11.7)
      {
          m = 1.18;
      }
    else if (dist > 11.7 && dist <= 28.9)
      {
          m = 1.94;
      }
    else if (dist > 28.9 && dist <= 71.6)
      {
          m = 1.86;
      }
    else if (dist > 71.6 && dist <= 177.3)
      {
          m = 0.45;
      }
    else if (dist > 177.3 && dist <= 439.0)
      {
          m = 0.32;
      }
    else if (dist > 439.0)
     {
          m = 0.32;
     }
*/


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

    }

//  double expectedLoss = 72.63 + 16*std::log10 (dist / 10) + 50;

   //double totalLoss = std::max(std::max(expectedLossWinner, expectedLossFree) + shadowingValue + fastFading, 0.0);
   double totalLoss = std::max(expectedLossWinner + shadowingValue + fastFading, 0.0);

 /* if (a -> GetDistanceFrom(b) > 0 && Simulator::Now ().GetSeconds () < 0.25)
   {
   std::ofstream streamFile;
  streamFile.open("results/sidelink/powerLoss.csv", std::ios_base::app);
  streamFile << dist << "," <<  expectedLossWinner << "," << shadowingValue << "," << fastFading << "," << totalLoss << "\r\n";
  streamFile.close(); 
 }
   */
   //NS_LOG_UNCOND("Tx Power: " << txPowerDbm << ", Expected Loss: " << expectedLossFree << ", Shadowing Loss: " << shadowingValue << ", Fast Fading: " << fastFading);
  
  Ptr<LTENodeState> nodeStateA = Create<LTENodeState>();
  Ptr<LTENodeState> nodeStateB = Create<LTENodeState>();

  nodeStateA = a->GetObject<Node>()->GetObject<LTENodeState>();
  nodeStateB = b->GetObject<Node>()->GetObject<LTENodeState>();

  bool isVehicleA = 0;
  bool isVehicleB = 0;

  isVehicleA = nodeStateA -> IsVehicle();
  isVehicleB = nodeStateB -> IsVehicle();
  
 if(isVehicleA && isVehicleB){

  /*
  std::ofstream streamFile;
  streamFile.open("results/sidelink/3GPPLoss.csv", std::ios_base::app);

  streamFile << a -> GetDistanceFrom(b) << "," << totalLoss << "," << std::max(expectedLossWinner, expectedLossFree) << "\r\n";

  streamFile.close();
 
  */
  }
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


NodeContainer::Iterator i,L;
NodeContainer::Iterator k;
int TrepPrint = 10; //milliseconds

double PrevX[274], PrevY[274], PrevZ[274], VelX[274], VelY[274], VelZ[274];  //20 is the number of nodes
double MeasInterval;


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

            
            if ((pos.x >= 1000) && (pos.x <= 3000)){
            positFile << Simulator::Now().GetSeconds() << "," << ID << "," << pos.x << "," << pos.y << "," << pos.z << "," << VelX[ID-1] << "," << VelY[ID-1] << "," << VelZ[ID-1] << "\r\n";
           }
            
           
           PrevX[ID-1] = pos.x;
           PrevY[ID-1] = pos.y;
           PrevZ[ID-1] = pos.z;


        }
         
        Simulator::Schedule (MilliSeconds (TrepPrint), &Print);
      
        positFile.close();
    
       //std::cout << "Printing... \n";
}

//--------------------------------------------------------------------------------------------------
//INIZIO
//MAIN
//--------------------------------------------------------------------------------------------------


int
main (int argc, char *argv[])
{
  //DEBUG components
  //LogComponentEnable("NistLteUeMac", LOG_LEVEL_INFO);
   // LogComponentEnable("NistLteUePhy", LOG_LEVEL_LOGIC);

    // Provides uniform random variables.
  Ptr<UniformRandomVariable> random = CreateObject<UniformRandomVariable>(); 
 
  // Initialize some values
  uint32_t mcs = 10; // The Modulation and Coding Scheme
  uint32_t rbSize = 8;  
  uint32_t ktrp = 1; // number of allocated subframes within the the TRP --> Mode 2 Parameter: Useless for V2V, but needed to compile :)
  uint32_t pscchLength = 8;
  std::string period="sf40";
  double dist = 20; // [m] : the distance between consecutive V-UEs in a single lane
  double simTime = 19; // The corrected simulation Time --> this is actually more
  double ueTxPower = 23.0; // [dBm]
  uint32_t ueCount = 274; // Number of V-UEs 
  bool verbose = true;
  double trep = 0.1; // [s] the CAM repetition period
  uint32_t packetSize = 158; // [Bytes] The number be --> 190Bytes at PHY layer

  uint32_t pucchSize = 0;          // PUCCH size in RBs --> zero because we assume we do not use the uplink; sidelink occurs over a distinct frequency band (5.9GHz)


//@DOC: Questi parametri servono per la topologia della rete fissa (eNodeB); noi non usiamo eNodeB,
// Ma il simulatore ne ha bisogno in una fase iniziale, per configurare i V-UEs all'inizio, come in una rete reale
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
  uint32_t runNumber = 9; // this is the default run --> this will be overridden shortly...
  uint32_t maxDENMPackets = 0; // the maximum allowable number of generated DENM packets --> if 0, only CAMs are generated
  
  double offsetStartTime = 0; // [s] Time at which V-UEs different from V-UE number 1 can start to generate traffic

  
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


  NS_LOG_INFO ("Configuring UE settings...");
  Config::SetDefault ("ns3::NistLteUeMac::SlGrantMcs", UintegerValue (mcs)); //The MCS of the SL grant, must be [0..15] (default 0)
  Config::SetDefault ("ns3::NistLteUeMac::SlGrantSize", UintegerValue (rbSize)); //The number of RBs allocated per UE for sidelink (default 1)
  Config::SetDefault ("ns3::NistLteUeMac::Ktrp", UintegerValue (ktrp)); //The repetition for PSSCH. Default = 0
  Config::SetDefault ("ns3::NistLteUeMac::PucchSize", UintegerValue (pucchSize));

  // Configure Power Control
  Config::SetDefault ("ns3::NistLteUePhy::TxPower", DoubleValue (ueTxPower));
  Config::SetDefault ("ns3::NistLteUePowerControl::Pcmax", DoubleValue (ueTxPower));
  Config::SetDefault ("ns3::NistLteUePowerControl::PoNominalPusch", IntegerValue (-106));
  Config::SetDefault ("ns3::NistLteUePowerControl::PscchTxPower", DoubleValue (ueTxPower));
  Config::SetDefault ("ns3::NistLteUePowerControl::PsschTxPower", DoubleValue (ueTxPower));
  Config::SetDefault ("ns3::NistLteUePhy::RsrpUeMeasThreshold", DoubleValue (-10.0));

  // Configure error model
  Config::SetDefault ("ns3::NistLteSpectrumPhy::CtrlErrorModelEnabled", BooleanValue (CtrlErrorModelEnabled));
  Config::SetDefault ("ns3::NistLteSpectrumPhy::CtrlFullDuplexEnabled", BooleanValue (!CtrlErrorModelEnabled));
  Config::SetDefault ("ns3::NistLteSpectrumPhy::DropRbOnCollisionEnabled", BooleanValue (DropOnCollisionEnabled));
 

//@DOC: Lascia stare: a noi non serve --> serve solo  far compilare; la frequenza la settiamo ad un livello più basso (fisico)
// Set the frequency to use for the Public Safety case (band 14 : 788 - 798 MHz for Uplink)
  // See TS36.101 
  Config::SetDefault ("ns3::NistLteEnbNetDevice::UlEarfcn", StringValue ("23330"));
  Config::SetDefault ("ns3::NistLteEnbNetDevice::UlBandwidth", StringValue ("50"));

  Config::SetDefault ("ns3::NistLteUeMac::RandomV2VSelection", BooleanValue (randomV2VSelection));

  NS_LOG_INFO ("Starting network configuration...");

  Config::SetDefault ("ns3::NistLteRlcUm::MaxTxBufferSize", StringValue ("100000"));
  Config::SetDefault ("ns3::DropTailQueue::MaxPackets", UintegerValue (10000000)); // In order not to run out of packets ;)
 
  ConfigStore inputConfig;
  inputConfig.ConfigureDefaults();

  NS_LOG_INFO ("Creating helpers...");
//@DOC Come tante cose, questo a noi non serve: però ns3 ha bisogno di costruire la rete fissa LTE (almeno il PDN-GW, o PGW, che dovrò assegnare gli indirizzi IP)
  Ptr<NistPointToPointEpcHelper>  epcHelper = CreateObject<NistPointToPointEpcHelper> (); 
  Ptr<Node> pgw = epcHelper->GetPgwNode ();

  Ptr<NistLteHelper> lteHelper = CreateObject<NistLteHelper> ();

//@NEW
//@DOC: ora sì che si setta la frequenza operativa
  // Set pathloss model
  Config::SetDefault ("ns3::Nist3gppPropagationLossModel::CacheLoss", BooleanValue (false));
  Config::SetDefault ("ns3::Nist3gppPropagationLossModel::Frequency", DoubleValue (5.9e9));

  lteHelper->SetPathlossModelType ("ns3::Nist3gppPropagationLossModel");

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


  lteHelper->SetAttribute ("UseSidelink", BooleanValue (true)); // WE USE SIDELINK
  lteHelper->SetEpcHelper (epcHelper);

  lteHelper->Initialize ();

  Ptr<NistLteProseHelper> proseHelper = CreateObject<NistLteProseHelper> ();
  proseHelper->SetLteHelper (lteHelper);

  NS_LOG_INFO ("Deploying UE's...");
  

//@DOC: Bene: adesso vengono creati gli UEs (nel nostro caso V-UEs) 
  NodeContainer ueResponders;

 
   
  for (uint32_t t=0; t<ueCount; ++t)
  {
    Ptr<Node> lteNode = CreateObject<Node> ();
    Ptr<LTENodeState> nodeState = CreateObject<LTENodeState> ();
    nodeState -> SetNode(lteNode);
    lteNode -> AggregateObject(nodeState);
    ueResponders.Add(lteNode);
  }
    

  
//@DOC: Codice per importare tracce da SUMO
/*
std::string traceFile = "scratch/highwayTrace.tcl";

Ns2MobilityHelper ns2 = Ns2MobilityHelper (traceFile);
ns2.Install (ueResponders.Begin(), ueResponders.End()); // configure movements for each node, while reading trace file

*/

// Numero di ueResponders deve essere uguale al numero di nodi della simulazione SUMO

  Ns2MobilityHelper ns2 = Ns2MobilityHelper("/home/merani/Desktop/SumoTraces/Random/2Lanes/RUN9/RUN9.tcl"); 
  ns2.Install(ueResponders.Begin(), ueResponders.End());
 
  i = ueResponders.Begin();
  k = ueResponders.End();

   for ( L = i; k != L; ++L)
  {  
     int ID;
     Ptr<Node> node = *L;
     ID = node->GetId ();
     Ptr<MobilityModel> mob = node->GetObject<MobilityModel> ();
     if (! mob) continue; // Strange -- node has no mobility model installed. Skip.
     Vector pos = mob->GetPosition ();
     PrevX[ID-1] = pos.x;
     PrevY[ID-1] = pos.y;
     PrevZ[ID-1] = pos.z;
     
  }

  MeasInterval = ((double)TrepPrint)/1000;
 
  Simulator::Schedule (MilliSeconds (TrepPrint), &Print);


  //mobility.SetPositionAllocator (positionAlloc);


  NS_LOG_INFO ("Installing UE network devices...");
  NetDeviceContainer ueDevs;

  NetDeviceContainer ueRespondersDevs = lteHelper->InstallUeDevice (ueResponders);
   // NetDeviceContainer ueSendersDevs = lteHelper->InstallUeDevice (ueResponders);
  ueDevs.Add (ueRespondersDevs);
   // ueDevs.Add (ueSendersDevs);



  // Add eNBs --> Ripeto: non servono a noi, ma al simulatore
    // Topology (Hex Grid)
  Ptr<NistLteHexGridEnbTopologyHelper> topoHelper = CreateObject<NistLteHexGridEnbTopologyHelper> ();
  topoHelper->SetLteHelper (lteHelper);
  topoHelper->SetNbRings (nbRings);
  topoHelper->SetInterSiteDistance (isd);
  topoHelper->SetMinimumDistance (minCenterDist);

    // Configure eNBs' antenna parameters before deploying them.
  lteHelper->SetEnbAntennaModelType ("ns3::NistParabolic3dAntennaModel");

   // Create eNbs
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

  // Install mobility (eNB) --> Anche se gli eNodeB sono fissi!
  MobilityHelper mobilityeNodeB;
  mobilityeNodeB.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobilityeNodeB.Install (sectorNodes);

 

  // Compute the position of each site and antenna orientation
  NetDeviceContainer enbDevs = topoHelper->SetPositionAndInstallEnbDevice(sectorNodes);

 for(uint32_t j=0; j< sectorNodes.GetN(); ++j)
 {
     std::cout << "eNB " << sectorNodes.Get(j) -> GetId() << " pos = (" << sectorNodes.Get(j)->GetObject<MobilityModel> ()->GetPosition ().x << "," << sectorNodes.Get(j)->GetObject<MobilityModel> ()->GetPosition ().x << ")\r\n";
 }


 //@NEW Save UE Positions

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
  Ipv4InterfaceContainer intcont = ipv4.Assign (ueRespondersDevs);
    //Ipv4InterfaceContainer intcont2 = ipv4.Assign (ueSendersDevs); 

  for (uint32_t u = 0; u < ueResponders.GetN (); ++u)
    {
      Ptr<Node> ueNode = ueResponders.Get (u);
      // Set the default gateway for the UE
      Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
     // ueStaticRouting->SetDefaultRoute (Ipv4Address::GetAny(), 1);
      //ueStaticRouting->SetDefaultMulticastRoute(1);
    }
  
  
  NS_LOG_INFO ("Attaching UE's to LTE network...");
  lteHelper->Attach (ueDevs);



  // Required to use NIST 3GPP Propagation model
  BuildingsHelper::Install (sectorNodes);

  BuildingsHelper::Install (ueResponders);
  BuildingsHelper::MakeMobilityModelConsistent ();
  
  NS_LOG_INFO ("Installing applications...");
  // UDP application --> in our case, this is the application that generates CAMs and DENMs



 TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");


 Ipv4AddressGenerator::Init(Ipv4Address ("225.0.0.0"), Ipv4Mask ("255.0.0.0"));
  groupAddress = Ipv4AddressGenerator::NextAddress (Ipv4Mask ("255.0.0.0"));


  UdpClientHelper udpClient (groupAddress , 8000); //set destination IP address and UDP port

  udpClient.SetAttribute ("MaxPackets", UintegerValue (100000));
  udpClient.SetAttribute ("Interval", TimeValue (Seconds (trep)));
  udpClient.SetAttribute ("PacketSize", UintegerValue (packetSize));
  udpClient.SetAttribute ("MaxDENMPackets", UintegerValue (maxDENMPackets));

  ApplicationContainer clientApps = udpClient.Install(ueResponders);
  Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();
 
  clientApps.Get(0) -> SetStartTime(Seconds(0.2 + rand -> GetValue(-0.03, 0.03)));
  for(uint32_t i = 1; i < ueResponders.GetN(); i++)
  {
    
      //@DOC: Randomize the generation time of CAM messages
      clientApps.Get(i)->SetStartTime (Seconds (0.2 + offsetStartTime + rand -> GetValue(-0.1, 0.1)));
  
  }
  
   clientApps.Get(0) -> SetStopTime (Seconds (simTime + 1));
 
    // Application to receive traffic
  PacketSinkHelper clientPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 8000));
  ApplicationContainer clientRespondersSrvApps = clientPacketSinkHelper.Install(ueResponders);
  clientRespondersSrvApps.Start (Seconds (0.05));
  clientRespondersSrvApps.Stop (Seconds (simTime+0.9));


 
 



  
  
  NS_LOG_INFO ("Creating sidelink configuration...");
  uint32_t groupL2Address = 0x00;
  


 Ptr<NistSlTft> tftRx = Create<NistSlTft> (NistSlTft::BIDIRECTIONAL, groupAddress, groupL2Address);
  proseHelper->ActivateSidelinkBearer (Seconds(0.1), ueRespondersDevs, tftRx); 


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
  lteHelper->InstallSidelinkConfiguration (ueRespondersDevs, ueSidelinkConfiguration);
    //lteHelper->InstallSidelinkConfiguration (ueSendersDevs, ueSidelinkConfiguration);

  NS_LOG_INFO ("Enabling LTE traces...");
  lteHelper->EnableTraces (); // Non le uso, ma ok
  

  //Simulator::Schedule(Seconds(1.0), &MyTestRandomUniform);


//@DOC: Questi sono i metodi della classe ns3::Simulator, che servono ad orchestrare il simulatore
// NS3 è un simulatore event-triggered: viene costruita una lista di eventi, ad ognuno dei quali viene associato l'istante di tempo
// al quale l'evento deve essere fatto avvenire (risolto e rimosso dalla event queue).
// Si può aggiungere manualmente un evento alla lista con il metodo ns3::Simulator::Schedule

  NS_LOG_INFO ("Starting simulation...");
  Simulator::Stop (Seconds (simTime+1)); // Teniamo un po' di margine per essere sicuri che i messaggi arrivino tutti 
  Simulator::Run ();
  Simulator::Destroy ();

  NS_LOG_INFO ("Done.");

  return 0;

}

