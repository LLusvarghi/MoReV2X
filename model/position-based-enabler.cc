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

#include "ns3/position-based-enabler.h"
#include <algorithm>    // std::max
#include "ns3/assert.h"

#define  INF 10000

namespace ns3 {

PosEnabler::PosEnabler() // Create the object
{
  m_enabled = true;
}

void
PosEnabler::initPolygon(Point *polygon, int n, std::string polyType)
{
  if (polyType == "TX")
  {
    m_polygonTX = polygon;
    m_TXsize = n;
  }
  else if (polyType == "RX")
  {
    m_polygonRX = polygon;
    m_RXsize = n;    
  }
  else 
  {
    NS_ASSERT_MSG(false, "Invalid polygon type");
  }
  
}

void
PosEnabler::initBBs(std::vector<BoundingBox> BBs)
{
  for(std::vector<BoundingBox>::iterator it_BB = BBs.begin(); it_BB != BBs.end(); it_BB++)
  {
    NS_ASSERT_MSG(it_BB->y2 >= it_BB->y1, "y2 must be greater than y1");
    NS_ASSERT_MSG(it_BB->x2 >= it_BB->x1, "x2 must be greater than x1");
    m_BBs.push_back(*it_BB);
  }
}

void
PosEnabler::EnableChecker(void)
{
  m_enabled = true;
}

void
PosEnabler::DisableChecker(void)
{
  m_enabled = false;
}

bool
PosEnabler::onSegment(Point p, Point q, Point r)
{
  if (q.x <= std::max(p.x, r.x) && q.x >= std::min(p.x, r.x) && q.y <= std::max(p.y, r.y) && q.y >= std::min(p.y, r.y)) 
        return true; 
    return false; 
}

// To find orientation of ordered triplet (p, q, r). 
// The function returns following values 
// 0 --> p, q and r are colinear 
// 1 --> Clockwise 
// 2 --> Counterclockwise 
int
PosEnabler::orientation(Point p, Point q, Point r) 
{ 
  int val = (q.y - p.y) * (r.x - q.x) - (q.x - p.x) * (r.y - q.y); 
  if (val == 0) return 0;  // colinear 
    return (val > 0)? 1: 2; // clock or counterclock wise 
} 


// The function that returns true if line segment 'p1q1' 
// and 'p2q2' intersect. 
bool
PosEnabler::doIntersect(Point p1, Point q1, Point p2, Point q2) 
{ 
  // Find the four orientations needed for general and 
  // special cases 
  int o1 = orientation(p1, q1, p2); 
  int o2 = orientation(p1, q1, q2); 
  int o3 = orientation(p2, q2, p1); 
  int o4 = orientation(p2, q2, q1); 
  // General case 
  if (o1 != o2 && o3 != o4) 
    return true; 
  // Special Cases 
  // p1, q1 and p2 are colinear and p2 lies on segment p1q1 
  if (o1 == 0 && onSegment(p1, p2, q1)) return true;  
  // p1, q1 and p2 are colinear and q2 lies on segment p1q1 
  if (o2 == 0 && onSegment(p1, q2, q1)) return true;   
  // p2, q2 and p1 are colinear and p1 lies on segment p2q2 
  if (o3 == 0 && onSegment(p2, p1, q2)) return true;   
   // p2, q2 and q1 are colinear and q1 lies on segment p2q2 
  if (o4 == 0 && onSegment(p2, q1, q2)) return true; 
  
  return false; // Doesn't fall in any of the above cases 
} 


// Returns true if the point p lies inside the polygon[] with n vertices 
bool
PosEnabler::isInsidePoly(std::string polyType, Point p) 
{ 
  Point *polygon;
  int n;
  if (polyType == "TX")
  {
    polygon = m_polygonTX;
    n = m_TXsize;
  }
  else if (polyType == "RX")
  {
    polygon = m_polygonRX;
    n = m_RXsize    ;
  }
  else 
  {
    NS_ASSERT_MSG(false, "Invalid polygon type");
  }
  // There must be at least 3 vertices in polygon[] 
  if (n < 3)  return false; 
  // Create a point for line segment from p to infinite 
  Point extreme = {INF, p.y}; 
  // Count intersections of the above line with sides of polygon 
  int count = 0, i = 0; 
  do
  { 
    int next = (i+1)%n;   
    // Check if the line segment from 'p' to 'extreme' intersects 
    // with the line segment from 'polygon[i]' to 'polygon[next]' 
    if (doIntersect(polygon[i], polygon[next], p, extreme)) 
    { 
      // If the point 'p' is colinear with line segment 'i-next', 
      // then check if it lies on segment. If it lies, return true, 
      // otherwise false 
      if (orientation(polygon[i], p, polygon[next]) == 0) 
        return onSegment(polygon[i], p, polygon[next]); 
      count++; 
    } 
    i = next; 
  } while (i != 0); 
  
  // Return true if count is odd, false otherwise 
  if (m_enabled)
    return count&1;  // Same as (count%2 == 1) 
  else
    return true;
} 


bool
PosEnabler::isEnabled(Vector pos) 
{
  if (m_enabled)
  {
    for(std::vector<BoundingBox>::iterator it_BB = m_BBs.begin(); it_BB != m_BBs.end(); it_BB++)
    {
      if ( ( (pos.x >= it_BB->x1) && (pos.x <= it_BB->x2) ) && ( (pos.y >= it_BB->y1) && (pos.y <= it_BB->y2) ) )
        return false;
    }
    return true;
  }
  else
    return true;
}



} //namespace ns3

