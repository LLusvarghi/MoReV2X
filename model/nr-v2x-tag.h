/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * 2018 Dipartimento di Ingegneria 'Enzo Ferrari' (DIEF),
 *      Universita' degli Studi di Modena e Reggio Emilia (UniMoRe)
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
 * Author: Lorenzo Gibellini, <lorenzo.gibellini@gmail.com> 
 * Author: Luca Lusvarghi <luca.lusvarghi5@unimore.it>	
 * University of Modena and Reggio Emilia 
 * 
 */

#ifndef NR_V2X_TAG_H
#define NR_V2X_TAG_H

#include "ns3/tag.h"

namespace ns3 {

class Tag;

class NrV2XTag : public Tag
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
  void SetNodeId(uint32_t value);
  void SetNumHops(uint32_t value);
  
  void SetMessageType(uint8_t value);
  void SetTrafficType(uint8_t value);
  void SetReselectionCounter(uint16_t value);
  void SetPacketSize(uint16_t value);
  void SetReservationSize(uint16_t value);

  void SetPPPP (uint8_t value);
  void SetPdb (double value);
  void SetPrsvp (uint32_t value);
  void SetMcs (uint32_t value);

 // Getters
  uint8_t GetSimpleValue (void) const;
  uint64_t GetIntValue (void) const;
  double GetDoubleValue (void) const ;

  double GetGenTime(void) const;
  uint64_t GetPacketId(void) const;
  double GetGenPosX(void) const;
  double GetGenPosY(void) const;
  uint32_t GetNodeId(void) const;
  uint32_t GetNumHops(void) const;

  uint8_t GetMessageType(void) const;
  uint8_t GetTrafficType(void) const;
  uint16_t GetReselectionCounter(void) const;
  uint16_t GetPacketSize(void) const;
  uint16_t GetReservationSize(void) const;
  
  uint8_t GetPPPP (void) const;
  double GetPdb (void) const;
  uint32_t GetPrsvp (void) const;
  uint32_t GetMcs (void) const;
  

private:
  uint8_t m_simpleValue;
  uint64_t m_intValue;
  double m_doubleValue;

  double m_genTime;
  uint64_t m_packetId;
  double m_genPosX;
  double m_genPosY;
  uint32_t m_nodeId;
  uint32_t m_numHops;

  uint8_t m_messageType;  // 0x00 = CAM
                          // 0x01 = DENM

  uint8_t m_trafficType;  // 0x00 = Periodic
                          // 0x01 = Aperiodic

  uint16_t m_Cresel;  // 0x00 = Periodic
                      // 0x01 = Aperiodic

  uint16_t m_PacketSize; 
  uint16_t m_ReservationSize; 

  uint8_t m_pppp; // the ProSe Per-Packet Priority
  double m_pdb; // the Packet Delay Budget (over the single hop)
  uint32_t m_p_rsvp; // the reservation period for this packet
  uint32_t m_mcs; // the Modulation and Coding Scheme to be used by lower layers for transmission
};

} //namespace ns3
#endif /* NR_V2X_TAG_H */
