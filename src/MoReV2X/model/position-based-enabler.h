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

#ifndef POSITION_BASED_ENABLER_H
#define POSITION_BASED_ENABLER_H

#include "ns3/vector.h"
#include <vector>


namespace ns3 {


struct Point{
  int x;
  int y;
};

struct BoundingBox{  // Bounding box
  int x1;
  int x2;
  int y1;
  int y2;
};

class PosEnabler
{
  public:
    PosEnabler ();   
    void initPolygon(Point polygon[], int n, std::string polyType);
    void initBBs(std::vector<BoundingBox> BBs);

    void EnableChecker(void);
    void DisableChecker(void);
    
    //Polygons
    bool onSegment(Point p, Point q, Point r);
    int orientation(Point p, Point q, Point r);
    bool doIntersect(Point p1, Point q1, Point p2, Point q2);
    bool isInsidePoly(std::string polyType, Point p);

    //Bounding boxes
    bool isEnabled(Vector pos); 

  private: 
    Point *m_polygonTX, *m_polygonRX;
    int m_TXsize, m_RXsize;
    std::vector<BoundingBox> m_BBs; 
    bool m_enabled;

};



} //namespace ns3
#endif /* POSITION_BASED_ENABLER_H */
