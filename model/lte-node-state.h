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
 * Author: Luca Lusvarghi <luca.lusvarghi5@unimore.it>	
 * University of Modena and Reggio Emilia 
 * 
 */

#ifndef LTE_NODE_STATE_H
#define LTE_NODE_STATE_H

#include "ns3/vector.h"
#include "ns3/node.h"

namespace ns3 {

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


} //namespace ns3
#endif /* LTE_NODE_STATE_H */
