/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 TELEMATICS LAB, DEE - Politecnico di Bari
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
 * Author: Giuseppe Piro  <g.piro@poliba.it>
 *         Nicola Baldo <nbaldo@cttc.es>
 * Modified by: NIST
 * Modified by: Luca Lusvarghi <luca.lusvarghi5@unimore.it>
 */

#include <map>
#include <cmath>

#include <ns3/log.h>
#include <ns3/fatal-error.h>
#include <algorithm>
#include <fstream>

#include "nist-lte-spectrum-value-helper.h"

// just needed to log a std::vector<int> properly...
namespace std {

ostream&
operator << (ostream& os, const vector<int>& v)
{
  vector<int>::const_iterator it = v.begin ();
  while (it != v.end ())
    {
      os << *it << " ";
      ++it;
    }
  os << endl;
  return os;
}

}

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("NistLteSpectrumValueHelper");

/**
 * Table 5.7.3-1 "E-UTRA channel numbers" from 3GPP TS 36.101
 * The table was converted to C syntax doing a cut & paste from TS 36.101 and running the following filter:
 * awk '{if ((NR % 7) == 1) printf("{"); printf ("%s",$0); if ((NR % 7) == 0) printf("},\n"); else printf(", ");}' | sed 's/ – /, /g'
 */
static const struct EutraChannelNumbers
{
  uint8_t band;
  double fDlLow;
  uint16_t nOffsDl; 
  uint16_t rangeNdl1;
  uint16_t rangeNdl2;
  double fUlLow;
  uint16_t nOffsUl; 
  uint16_t rangeNul1;
  uint16_t rangeNul2;
} g_eutraChannelNumbers[] = {
  { 1, 2110, 0, 0, 599, 1920, 18000, 18000, 18599},
  { 2, 1930, 600, 600, 1199, 1850, 18600, 18600, 19199},
  { 3, 1805, 1200, 1200, 1949, 1710, 19200, 19200, 19949},
  { 4, 2110, 1950, 1950, 2399, 1710, 19950, 19950, 20399},
  { 5, 869, 2400, 2400, 2649, 824, 20400, 20400, 20649},
  { 6, 875, 2650, 2650, 2749, 830, 20650, 20650, 20749},
  { 7, 2620, 2750, 2750, 3449, 2500, 20750, 20750, 21449},
  { 8, 925, 3450, 3450, 3799, 880, 21450, 21450, 21799},
  { 9, 1844.9, 3800, 3800, 4149, 1749.9, 21800, 21800, 22149},
  { 10, 2110, 4150, 4150, 4749, 1710, 22150, 22150, 22749},
  { 11, 1475.9, 4750, 4750, 4949, 1427.9, 22750, 22750, 22949},
  { 12, 728, 5000, 5000, 5179, 698, 23000, 23000, 23179},
  { 13, 746, 5180, 5180, 5279, 777, 23180, 23180, 23279},
  { 14, 758, 5280, 5280, 5379, 788, 23280, 23280, 23379},
  { 17, 734, 5730, 5730, 5849, 704, 23730, 23730, 23849},
  { 18, 860, 5850, 5850, 5999, 815, 23850, 23850, 23999},
  { 19, 875, 6000, 6000, 6149, 830, 24000, 24000, 24149},
  { 20, 791, 6150, 6150, 6449, 832, 24150, 24150, 24449},
  { 21, 1495.9, 6450, 6450, 6599, 1447.9, 24450, 24450, 24599},
  { 33, 1900, 36000, 36000, 36199, 1900, 36000, 36000, 36199},
  { 34, 2010, 36200, 36200, 36349, 2010, 36200, 36200, 36349},
  { 35, 1850, 36350, 36350, 36949, 1850, 36350, 36350, 36949},
  { 36, 1930, 36950, 36950, 37549, 1930, 36950, 36950, 37549},
  { 37, 1910, 37550, 37550, 37749, 1910, 37550, 37550, 37749},
  { 38, 2570, 37750, 37750, 38249, 2570, 37750, 37750, 38249},
  { 39, 1880, 38250, 38250, 38649, 1880, 38250, 38250, 38649},
  { 40, 2300, 38650, 38650, 39649, 2300, 38650, 38650, 39649},
  { 47, 5855, 54540, 54540, 55239, 5855, 54540, 54540, 55239}  //New band (47) added for ITS band (as per Rel' 14, Sept. 2017)
};

#define NUM_EUTRA_BANDS (sizeof (g_eutraChannelNumbers) / sizeof (EutraChannelNumbers))

double 
NistLteSpectrumValueHelper::GetCarrierFrequency (uint16_t earfcn)
{
  NS_LOG_FUNCTION (earfcn);
  if (earfcn < 7000)
    {
      // FDD downlink
      return GetDownlinkCarrierFrequency (earfcn);
    }
  else 
    {
      // either FDD uplink or TDD (for which uplink & downlink have same frequency)
      return GetUplinkCarrierFrequency (earfcn);
    }
}

double 
NistLteSpectrumValueHelper::GetDownlinkCarrierFrequency (uint16_t nDl)
{
  NS_LOG_FUNCTION (nDl);
  for (uint16_t i = 0; i < NUM_EUTRA_BANDS; ++i)
    {
      if ((g_eutraChannelNumbers[i].rangeNdl1 <= nDl)
          && (g_eutraChannelNumbers[i].rangeNdl2 >= nDl))
        {
          NS_LOG_LOGIC ("entry " << i << " fDlLow=" << g_eutraChannelNumbers[i].fDlLow);
          return 1.0e6 * (g_eutraChannelNumbers[i].fDlLow + 0.1 * (nDl - g_eutraChannelNumbers[i].nOffsDl));
        }
    }
  NS_LOG_ERROR ("invalid EARFCN " << nDl);
  return 0.0;
}

double 
NistLteSpectrumValueHelper::GetUplinkCarrierFrequency (uint16_t nUl)
{
  NS_LOG_FUNCTION (nUl);
  for (uint16_t i = 0; i < NUM_EUTRA_BANDS; ++i)
    {
      if ((g_eutraChannelNumbers[i].rangeNul1 <= nUl)
          && (g_eutraChannelNumbers[i].rangeNul2 >= nUl))
        {
          NS_LOG_LOGIC ("entry " << i << " fUlLow=" << g_eutraChannelNumbers[i].fUlLow);
          return 1.0e6 * (g_eutraChannelNumbers[i].fUlLow + 0.1 * (nUl - g_eutraChannelNumbers[i].nOffsUl));
        }
    }
  NS_LOG_ERROR ("invalid EARFCN " << nUl);
  return 0.0;
}

double 
NistLteSpectrumValueHelper::GetChannelBandwidth (uint8_t transmissionBandwidth)
{
  NS_LOG_FUNCTION ((uint16_t) transmissionBandwidth);
  switch (transmissionBandwidth)
    { 
    case 6:
      return 1.4e6;
    case 15:
      return 3.0e6;
    case 25:
      return 5.0e6;
    case 50:
      return 10.0e6;
    case 75:
      return 15.0e6;
    case 100:
      return 20.0e6;
    default:
      NS_FATAL_ERROR ("invalid bandwidth value " << (uint16_t) transmissionBandwidth);
    }
}




struct LteSpectrumModelId
{
  LteSpectrumModelId (uint16_t f, uint8_t b);
  uint16_t earfcn;
  uint8_t  bandwidth;
};

LteSpectrumModelId::LteSpectrumModelId (uint16_t f, uint8_t b)
  : earfcn (f), 
    bandwidth (b)
{
}

bool
operator < (const LteSpectrumModelId& a, const LteSpectrumModelId& b)
{
  return ( (a.earfcn < b.earfcn) || ( (a.earfcn == b.earfcn) && (a.bandwidth < b.bandwidth) ) );
}
 

static std::map<LteSpectrumModelId, Ptr<SpectrumModel> > g_lteSpectrumModelMap;


Ptr<SpectrumModel>
NistLteSpectrumValueHelper::GetSpectrumModel (uint16_t earfcn, uint8_t txBandwidthConfiguration)
{
  NS_LOG_FUNCTION (earfcn << (uint16_t) txBandwidthConfiguration);
  Ptr<SpectrumModel> ret;
  LteSpectrumModelId key (earfcn, txBandwidthConfiguration);
  std::map<LteSpectrumModelId, Ptr<SpectrumModel> >::iterator it = g_lteSpectrumModelMap.find (key);
  if (it != g_lteSpectrumModelMap.end ())
    {
      ret = it->second;
    }
  else
    {
      double fc = GetCarrierFrequency (earfcn);
      NS_ASSERT_MSG (fc != 0, "invalid EARFCN=" << earfcn);

      double f = fc - (txBandwidthConfiguration * 180e3 / 2.0);
      Bands rbs;
      for (uint8_t numrb = 0; numrb < txBandwidthConfiguration; ++numrb)
        {
          BandInfo rb; 
          rb.fl = f;
          f += 90e3;
          rb.fc = f;
          f += 90e3;
          rb.fh = f;
          rbs.push_back (rb);
        }
      ret = Create<SpectrumModel> (rbs);
      g_lteSpectrumModelMap.insert (std::pair<LteSpectrumModelId, Ptr<SpectrumModel> > (key, ret));
    }
  NS_LOG_LOGIC ("returning SpectrumModel::GetUid () == " << ret->GetUid ());
  return ret;
}

Ptr<SpectrumValue> 
NistLteSpectrumValueHelper::CreateTxPowerSpectralDensity (uint16_t earfcn, uint8_t txBandwidthConfiguration, double powerTx, std::vector <int> activeRbs)
{ 
  NS_LOG_FUNCTION (earfcn << (uint16_t) txBandwidthConfiguration << powerTx << activeRbs);

  Ptr<SpectrumModel> model = GetSpectrumModel (earfcn, txBandwidthConfiguration);
  Ptr<SpectrumValue> txPsd = Create <SpectrumValue> (model);

  // powerTx is expressed in dBm. We must convert it into natural unit.
  double powerTxW = std::pow (10., (powerTx - 30) / 10);

  double txPowerDensity = (powerTxW / (txBandwidthConfiguration * 180000));

  for (std::vector <int>::iterator it = activeRbs.begin (); it != activeRbs.end (); it++)
    {
      int rbId = (*it);
      //NS_LOG_UNCOND(rbId);
      (*txPsd)[rbId] = txPowerDensity;
    }

  NS_LOG_LOGIC (*txPsd);

  return txPsd;
}

Ptr<SpectrumValue> 
NistLteSpectrumValueHelper::CreateUlTxPowerSpectralDensity (uint16_t earfcn, uint16_t txBandwidthConfiguration, double powerTx, std::vector <int> activeRbs, double slotDuration, uint16_t SCS, uint16_t mcsIndex, bool IBE)
{
  NS_LOG_FUNCTION ((uint16_t) txBandwidthConfiguration << powerTx << activeRbs << SCS << mcsIndex);

  bool InBandEmissions = IBE;
  if (InBandEmissions)
    NS_ASSERT_MSG(powerTx > 10, "In-band emissions are implemented only for Tx power > 10 dBm");

  Ptr<SpectrumModel> model = GetSpectrumModel (earfcn, txBandwidthConfiguration);
  Ptr<SpectrumValue> txPsd = Create <SpectrumValue> (model);

  double EVM;
  if (mcsIndex <= 9) //QPSK
    EVM = 17.5;
  else if ((mcsIndex >= 10) && (mcsIndex <= 16)) //16QAM
    EVM = 12.5;
  else  //64 QAM
    EVM = 8.0;

  // powerTx is expressed in dBm. We must convert it into natural unit.
  double powerTxW = std::pow (10., (powerTx - 30) / 10);

  double txPowerDensity = (powerTxW / (activeRbs.size() * 180000 / slotDuration));
//  double txPowerDensity = powerTxW / activeRbs.size();
//  double txPowerDensity = powerTxW;

  NS_LOG_INFO("Transmission power is " << powerTx << " dBm. The Tx power density is " << txPowerDensity << " W/Hz with slot duration: " << slotDuration << " ms");
  
  double P_rb = powerTx - activeRbs.size();
  double attenuation_IQ = -28, attenuation_CarrLeakage = -28;

  std::vector<double> attenuation_values; // store the attenuation value in dB

  for (int rbId = 0; rbId < txBandwidthConfiguration; rbId++)
  {
    if (std::find(activeRbs.begin(), activeRbs.end(), rbId) != activeRbs.end())
      (*txPsd)[rbId] = txPowerDensity; //the same for all the active RBs
    else
    {
      if (InBandEmissions)
      {
        double tmp, attenuation_general, attenuation_total, attenuation;
        if (rbId < activeRbs.front())
          tmp = std::max(-25 - 10*std::log10(txBandwidthConfiguration/activeRbs.size()), 20*std::log10(EVM) -3 -5*(activeRbs.front() - rbId -1)/activeRbs.size() );
        else
          tmp = std::max(-25 - 10*std::log10(txBandwidthConfiguration/activeRbs.size()), 20*std::log10(EVM) -3 -5*(rbId - activeRbs.back() -1)/activeRbs.size() );
        attenuation_general = std::max(tmp, -57 + 10*std::log10(SCS/15) - P_rb );

        attenuation_total = attenuation_general + attenuation_IQ + attenuation_CarrLeakage;
        NS_LOG_DEBUG("Attenuation: general = " << attenuation_general << ", IQ = " << attenuation_IQ << ", Carrier leakage = " << attenuation_CarrLeakage << ". Total = " << attenuation_total);
        attenuation = std::max(attenuation_total, P_rb-30);
        NS_LOG_DEBUG(attenuation << " dB attenuation on RB " << rbId);

        attenuation_values.push_back(attenuation);

        (*txPsd)[rbId] = txPowerDensity * std::pow(10, attenuation/10);
      }
      else
        (*txPsd)[rbId] = 0; 
    }
  }

 /* for (std::vector <int>::iterator it = activeRbs.begin (); it != activeRbs.end (); it++)
    {
      int rbId = (*it);
      NS_LOG_INFO("RB ID " <<rbId);
      (*txPsd)[rbId] = txPowerDensity; //the same for all the RBs
    }*/

  if (InBandEmissions && attenuation_values.size() > 0)
  {
    double avgAttenuation = 0;
    for (uint16_t kk = 0; kk < attenuation_values.size(); kk++)
      avgAttenuation += attenuation_values[kk];
    avgAttenuation /= attenuation_values.size();
    NS_LOG_DEBUG("Average attenuation = " << avgAttenuation << " dB");

/*    std::ofstream IBElog;
    IBElog.open ("results/sidelink/IBE.txt", std::ios_base::app);
    IBElog << powerTx << "," << SCS << "," << avgAttenuation << std::endl;
    IBElog.close();*/

  }

  NS_LOG_LOGIC ("TX PSD: " << *txPsd);

  //std::cin.get();

  return txPsd;
}

Ptr<SpectrumValue>
NistLteSpectrumValueHelper::CreateTxPowerSpectralDensity (uint16_t earfcn, uint8_t txBandwidthConfiguration, double powerTx, std::map<int, double> powerTxMap, std::vector <int> activeRbs)
{
  NS_LOG_FUNCTION (earfcn << (uint16_t) txBandwidthConfiguration << activeRbs);

  Ptr<SpectrumModel> model = GetSpectrumModel (earfcn, txBandwidthConfiguration);
  Ptr<SpectrumValue> txPsd = Create <SpectrumValue> (model);

  // powerTx is expressed in dBm. We must convert it into natural unit.
  double powerTxW = std::pow (10., (powerTx - 30) / 10);
  double basicPowerTxW = std::pow (10., (powerTx - 30) / 10);

  double txPowerDensity = (powerTxW / (txBandwidthConfiguration * 180000));

  for (std::vector <int>::iterator it = activeRbs.begin (); it != activeRbs.end (); it++)
    {
      int rbId = (*it);
      
      std::map<int, double>::iterator powerIt = powerTxMap.find (rbId);

      if (powerIt != powerTxMap.end ())
        {
          powerTxW = std::pow (10., (powerIt->second - 30) / 10);
          txPowerDensity = (powerTxW / (txBandwidthConfiguration * 180000));
        }
      else
        {
          txPowerDensity = (basicPowerTxW / (txBandwidthConfiguration * 180000));
        }

      (*txPsd)[rbId] = txPowerDensity;
    }

  NS_LOG_LOGIC (*txPsd);

  return txPsd;
}


Ptr<SpectrumValue>
NistLteSpectrumValueHelper::CreateNoisePowerSpectralDensity (uint16_t earfcn, uint8_t txBandwidthConfiguration, double noiseFigure)
{
  NS_LOG_FUNCTION (earfcn << (uint16_t) txBandwidthConfiguration << noiseFigure);
  Ptr<SpectrumModel> model = GetSpectrumModel (earfcn, txBandwidthConfiguration);
  return CreateNoisePowerSpectralDensity (noiseFigure, model);
}

Ptr<SpectrumValue>
NistLteSpectrumValueHelper::CreateNoisePowerSpectralDensity (double noiseFigureDb, Ptr<SpectrumModel> spectrumModel)
{
  NS_LOG_FUNCTION (noiseFigureDb << spectrumModel);

  // see "LTE - From theory to practice"
  // Section 22.4.4.2 Thermal Noise and Receiver Noise Figure
  const double kT_dBm_Hz = -174.0;  // dBm/Hz
  double kT_W_Hz = std::pow (10.0, (kT_dBm_Hz - 30) / 10.0);
  double noiseFigureLinear = std::pow (10.0, noiseFigureDb / 10.0);
  double noisePowerSpectralDensity =  kT_W_Hz * noiseFigureLinear;

  Ptr<SpectrumValue> noisePsd = Create <SpectrumValue> (spectrumModel);
  (*noisePsd) = noisePowerSpectralDensity;
//  NS_LOG_UNCOND("Noise figure " << noiseFigureDb << " kt_W_Hz " << kT_W_Hz << " noisePSD " << noisePowerSpectralDensity);
//  std::cin.get();
  return noisePsd;
}

} // namespace ns3
