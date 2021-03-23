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

#ifndef LTE_NODE_H
#define LTE_NODE_H

#include "ns3/vector.h"
#include "ns3/node.h"

namespace ns3 {

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


} //namespace ns3
#endif /* LTE_NODE_H */
