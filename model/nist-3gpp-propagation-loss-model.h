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
 * It was tested under ns-3.21
 */

#ifndef NIST_3GPP_PROPAGATION_LOSS_MODEL_H
#define NIST_3GPP_PROPAGATION_LOSS_MODEL_H

#include <ns3/buildings-propagation-loss-model.h>
#include <ns3/propagation-environment.h>
#include <ns3/traced-callback.h>
#include "ns3/node.h"

namespace ns3 {

class NistIndoorPropagationLossModel;
class NistOutdoorPropagationLossModel;
class NistHybridPropagationLossModel;
class NistUrbanmacrocellPropagationLossModel;

/**
 * \ingroup nist
 *
 *  \brief The Nist3gppPropagationModel is a compound of different models able to evaluate the pathloss in different environments and with buildings (i.e., indoor and outdoor communications).
 *
 *  This model includes different models defined by 3GPP TR 36.843 V12.0.0
 *  (2014-03) - Section A.2.1.2, which are combined in order
 *  to be able to evaluate the pathloss under different scenarios, in detail:
 *  - Environments: urban, suburban, open-areas;
 *  - frequency: 700 MHz for public safety and 2 GHz for general scenarios
 *  - D2D communications
 *  - Node position respect to buildings: indoor, outdoor and hybrid (indoor <-> outdoor)
 *  - Building penetretation loss
 *  - floors, etc...
 *
 *  \warning This model works only with MobilityBuildingInfo
 *
 */

class Nist3gppPropagationLossModel : public BuildingsPropagationLossModel
{

public:
  
  /**
   * structure to save the two nodes mobility models
   */
  struct MobilityDuo
  {
    Ptr<MobilityModel> a; ///< mobility model of node 1
    Ptr<MobilityModel> b; ///< mobility model of node 2

    /**
     * equality function
     * \param mo1 mobility model for node 1
     * \param mo2 mobility model for node 2
     */
    friend bool operator==(const MobilityDuo& mo1, const MobilityDuo& mo2)
    {
      return (mo1.a == mo2.a && mo1.b == mo2.b);
    }
     
    /**
     * less than function
     * \param mo1 mobility model for node 1
     * \param mo2 mobility model for node 2
     */
    friend bool operator<(const MobilityDuo& mo1, const MobilityDuo& mo2)
    {
      return (mo1.a < mo2.a || ( (mo1.a == mo2.a) && (mo1.b < mo2.b)));
    }

  };
  static TypeId GetTypeId (void);
  Nist3gppPropagationLossModel ();
  ~Nist3gppPropagationLossModel ();

  /** 
   * set the propagation frequency
   * 
   * \param freq 
   */
  void SetFrequency (double freq);

  /** 
   * set the rooftop height
   * 
   * \param rooftopHeight
   */
  void SetRooftopHeight (double rooftopHeight);

  /**
   * Compute the pathloss based on the positions of the two nodes
   *
   * \param a the mobility model of the source
   * \param b the mobility model of the destination
   * \returns the propagation loss (in dBm)
   */
  virtual double GetLoss (Ptr<MobilityModel> a, Ptr<MobilityModel> b) const;
  
  /**
   * \param a first mobility model
   * \param b second mobility model
   *
   * \return the shadowing value
   */
  double GetShadowing (Ptr<MobilityModel> a, Ptr<MobilityModel> b) const;
  
  /**
   * Evaluate the shadowing standard deviation based on the positions of the two nodes
   *
   * \param a the mobility model of the source
   * \param b the mobility model of the destination
   * \returns the shadowing standard deviation ""sigma"" (in dBm)
  */
  virtual double EvaluateSigma (Ptr<MobilityBuildingInfo> a, Ptr<MobilityBuildingInfo> b) const;

private:

  std::pair<double, bool> Indoor (Ptr<MobilityModel> a, Ptr<MobilityModel> b) const;
  double Outdoor (Ptr<MobilityModel> a, Ptr<MobilityModel> b) const;
  double Hybrid (Ptr<MobilityModel> a, Ptr<MobilityModel> b) const;
  double Urbanmacrocell (Ptr<MobilityModel> a, Ptr<MobilityModel> b) const;

  Ptr<NistIndoorPropagationLossModel> m_indoor;
  Ptr<NistOutdoorPropagationLossModel> m_outdoor;
  Ptr<NistHybridPropagationLossModel> m_hybrid;
  Ptr<NistUrbanmacrocellPropagationLossModel> m_urbanmacrocell;

  double m_frequency;
  //bool m_los;
  TracedCallback<double, Ptr<Node>, Ptr<Node>, double, bool, bool> m_nistPathlossTrace;

  // Map to keep track of shadowing values
  mutable std::map<Ptr<MobilityModel>,  std::map<Ptr<MobilityModel>, double> > m_shadowingLossMap;

  bool m_cacheLoss;
  mutable std::map<MobilityDuo, double> m_lossMap; //cache for loss values
};

}

#endif /* NIST_3GPP_PROPAGATION_LOSS_MODEL_H */
