/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This software was developed at the National Institute of Standards and
 * Technology by employees of the Federal Government in the course of
 * their official duties. Pursuant to titleElement 17 Section 105 of the United
 * States Code this software is not subject to copyright protection and
 * is in the public domain.
 * NIST assumes no responsibility whatsoever for its use by other parties,
 * and makes no guarantees, expressed or implied, about its quality,
 * reliability, or any other characteristic.

 * We would appreciate acknowledgement if the software is used.

 * NIST ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS" CONDITION AND
 * DISCLAIM ANY LIABILITY OF ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING
 * FROM THE USE OF THIS SOFTWARE.
 *
 * Modified by: NIST
 */

#include "nist-sl-resource-pool-factory.h"
#include "ns3/log.h"

#include <iostream>
#include <fstream>
namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("NistSlResourcePoolFactory");

NistSlResourcePoolFactory::NistSlResourcePoolFactory () :
  m_scCpLen("NORMAL"),
  m_period("sf40"),
  m_scPrbNum(22),
  m_scPrbStart(0),
  m_scPrbEnd(49),
  m_scOffset(0),
  //m_scBitmapSize(40),
  m_scBitmapValue(0xFF00000000),
  m_dataCpLen("NORMAL"),
  m_hoppingParameters(0),
  m_subbands("ns4"),
  m_rbOffset(0),
  m_ueSelected(true),
  m_haveTrptSubset(false),
  //m_trptSubsetSize(3),
  m_trptSubsetValue(0x7),
  m_dataPrbNum(25),
  m_dataPrbStart(0),
  m_dataPrbEnd(49),
  m_dataOffset(8),
  //m_dataBitmapSize(40),
  m_dataBitmapValue(0xFFFFFFFFFF),
  m_txParam(true),
  m_txAlpha("al09"),
  m_txP0(-40),
  m_dataAlpha("al09"),
  m_dataP0(-40)
{
  NS_LOG_FUNCTION (this);
}

NistLteRrcSap::SlCommResourcePool
NistSlResourcePoolFactory::CreatePool ()
{
  // Control
  //pool.scCpLen.cplen = m_scCpLen;
  if (m_scCpLen == "NORMAL") {m_pool.scCpLen.cplen = NistLteRrcSap::SlCpLen::NORMAL;}
  else if (m_scCpLen == "EXTENDED") {m_pool.scCpLen.cplen = NistLteRrcSap::SlCpLen::EXTENDED;}
  else {NS_FATAL_ERROR ("UNSUPPORTED CONTROL CP LENGTH");}
  //pool.scPeriod.period = m_period;
  if (m_period == "sf40") {m_pool.scPeriod.period = NistLteRrcSap::SlPeriodComm::sf40;}
  else if (m_period == "sf60") {m_pool.scPeriod.period = NistLteRrcSap::SlPeriodComm::sf60;}
  else if (m_period == "sf70") {m_pool.scPeriod.period = NistLteRrcSap::SlPeriodComm::sf70;}
  else if (m_period == "sf80") {m_pool.scPeriod.period = NistLteRrcSap::SlPeriodComm::sf80;}
  else if (m_period == "sf120") {m_pool.scPeriod.period = NistLteRrcSap::SlPeriodComm::sf120;}
  else if (m_period == "sf140") {m_pool.scPeriod.period = NistLteRrcSap::SlPeriodComm::sf140;}
  else if (m_period == "sf160") {m_pool.scPeriod.period = NistLteRrcSap::SlPeriodComm::sf160;}
  else if (m_period == "sf240") {m_pool.scPeriod.period = NistLteRrcSap::SlPeriodComm::sf240;}
  else if (m_period == "sf280") {m_pool.scPeriod.period = NistLteRrcSap::SlPeriodComm::sf280;}
  else if (m_period == "sf320") {m_pool.scPeriod.period = NistLteRrcSap::SlPeriodComm::sf320;}
  else {NS_FATAL_ERROR ("UNSUPPORTED CONTROL PERIOD LENGTH");}
  m_pool.scTfResourceConfig.prbNum = m_scPrbNum;
  m_pool.scTfResourceConfig.prbStart = m_scPrbStart;
  m_pool.scTfResourceConfig.prbEnd = m_scPrbEnd;
  m_pool.scTfResourceConfig.offsetIndicator.offset = m_scOffset;
  m_pool.scTfResourceConfig.subframeBitmap.bitmap = std::bitset<40> (m_scBitmapValue);

  // Data
  //pool.dataCpLen.cplen = m_dataCpLen;
  if (m_dataCpLen == "NORMAL") {m_pool.dataCpLen.cplen = NistLteRrcSap::SlCpLen::NORMAL;}
  else if (m_dataCpLen == "EXTENDED") {m_pool.dataCpLen.cplen = NistLteRrcSap::SlCpLen::EXTENDED;}
  else {NS_FATAL_ERROR ("UNSUPPORTED CONTROL CP LENGTH");}
  m_pool.dataHoppingConfig.hoppingParameters = m_hoppingParameters;
  //pool.dataHoppingConfig.numSubbands = m_subbands;
  if (m_subbands == "ns1") {m_pool.dataHoppingConfig.numSubbands = NistLteRrcSap::SlHoppingConfigComm::ns1;}
  else if (m_subbands == "ns2") {m_pool.dataHoppingConfig.numSubbands = NistLteRrcSap::SlHoppingConfigComm::ns2;}
  else if (m_subbands == "ns4") {m_pool.dataHoppingConfig.numSubbands = NistLteRrcSap::SlHoppingConfigComm::ns4;}
  else {NS_FATAL_ERROR ("UNSUPPORTED NUMBER OF SUBBANDS");}
  m_pool.dataHoppingConfig.rbOffset = m_rbOffset;

  // Ue Selected Parameters
  m_pool.haveUeSelectedResourceConfig = m_ueSelected;
  if (m_ueSelected)
  {
    m_pool.ueSelectedResourceConfig.haveTrptSubset = m_haveTrptSubset;
    if (m_haveTrptSubset)
    {
      m_pool.ueSelectedResourceConfig.trptSubset.subset = std::bitset<3> (m_trptSubsetValue);
    }
    m_pool.ueSelectedResourceConfig.dataTfResourceConfig.prbNum = m_dataPrbNum;
    m_pool.ueSelectedResourceConfig.dataTfResourceConfig.prbStart = m_dataPrbStart;
    m_pool.ueSelectedResourceConfig.dataTfResourceConfig.prbEnd = m_dataPrbEnd;
    m_pool.ueSelectedResourceConfig.dataTfResourceConfig.offsetIndicator.offset = m_dataOffset;
    m_pool.ueSelectedResourceConfig.dataTfResourceConfig.subframeBitmap.bitmap = std::bitset<40> (m_dataBitmapValue);
  }

  // Tx Parameters
  m_pool.haveTxParameters = m_txParam;
  if (m_txParam)
  {
    //pool.txParameters.scTxParameters.alpha = m_txAlpha;
    if (m_txAlpha == "al0") {m_pool.txParameters.scTxParameters.alpha = NistLteRrcSap::SlTxParameters::al0;}
    else if (m_txAlpha == "al04") {m_pool.txParameters.scTxParameters.alpha = NistLteRrcSap::SlTxParameters::al04;}
    else if (m_txAlpha == "al05") {m_pool.txParameters.scTxParameters.alpha = NistLteRrcSap::SlTxParameters::al05;}
    else if (m_txAlpha == "al06") {m_pool.txParameters.scTxParameters.alpha = NistLteRrcSap::SlTxParameters::al06;}
    else if (m_txAlpha == "al07") {m_pool.txParameters.scTxParameters.alpha = NistLteRrcSap::SlTxParameters::al07;}
    else if (m_txAlpha == "al08") {m_pool.txParameters.scTxParameters.alpha = NistLteRrcSap::SlTxParameters::al08;}
    else if (m_txAlpha == "al09") {m_pool.txParameters.scTxParameters.alpha = NistLteRrcSap::SlTxParameters::al09;}
    else if (m_txAlpha == "al1") {m_pool.txParameters.scTxParameters.alpha = NistLteRrcSap::SlTxParameters::al1;}
    else {NS_FATAL_ERROR ("UNSUPPORTED CONTROL TX ALPHA");}
    m_pool.txParameters.scTxParameters.p0 = m_txP0;
    //pool.txParameters.dataTxParameters.alpha = m_dataAlpha;
    if (m_dataAlpha == "al0") {m_pool.txParameters.dataTxParameters.alpha = NistLteRrcSap::SlTxParameters::al0;}
    else if (m_dataAlpha == "al04") {m_pool.txParameters.dataTxParameters.alpha = NistLteRrcSap::SlTxParameters::al04;}
    else if (m_dataAlpha == "al05") {m_pool.txParameters.dataTxParameters.alpha = NistLteRrcSap::SlTxParameters::al05;}
    else if (m_dataAlpha == "al06") {m_pool.txParameters.dataTxParameters.alpha = NistLteRrcSap::SlTxParameters::al06;}
    else if (m_dataAlpha == "al07") {m_pool.txParameters.dataTxParameters.alpha = NistLteRrcSap::SlTxParameters::al07;}
    else if (m_dataAlpha == "al08") {m_pool.txParameters.dataTxParameters.alpha = NistLteRrcSap::SlTxParameters::al08;}
    else if (m_dataAlpha == "al09") {m_pool.txParameters.dataTxParameters.alpha = NistLteRrcSap::SlTxParameters::al09;}
    else if (m_dataAlpha == "al1") {m_pool.txParameters.dataTxParameters.alpha = NistLteRrcSap::SlTxParameters::al1;}
    else {NS_FATAL_ERROR ("UNSUPPORTED DATA TX ALPHA");}
    m_pool.txParameters.dataTxParameters.p0 = m_dataP0;
  }

  return m_pool;
}

// Control
void
NistSlResourcePoolFactory::SetControlCpLen (std::string cpLen)
{
  NS_LOG_FUNCTION (this << cpLen);
  m_scCpLen = cpLen;
}

void
NistSlResourcePoolFactory::SetControlPeriod (std::string period)
{
  NS_LOG_FUNCTION (this << period);
  m_period = period;
}

void
NistSlResourcePoolFactory::SetControlPrbNum (int prbNum)
{
  NS_LOG_FUNCTION (this << prbNum);
  m_scPrbNum = prbNum;
}

void
NistSlResourcePoolFactory::SetControlPrbStart (int prbStart)
{
  NS_LOG_FUNCTION (this << prbStart);
  m_scPrbStart = prbStart;
}

void
NistSlResourcePoolFactory::SetControlPrbEnd (int prbEnd)
{
  NS_LOG_FUNCTION (this << prbEnd);
  m_scPrbEnd = prbEnd;
}

void
NistSlResourcePoolFactory::SetControlOffset (int offset)
{
  NS_LOG_FUNCTION (this << offset);
  m_scOffset = offset;
}

void
NistSlResourcePoolFactory::SetControlBitmap (uint64_t value) //PSCCH Subframe Pool
{
  NS_LOG_FUNCTION (this << value);
  m_scBitmapValue = value;
}

// Data
void
NistSlResourcePoolFactory::SetDataCpLen (std::string cpLen)
{
  NS_LOG_FUNCTION (this << cpLen);
  m_dataCpLen = cpLen;
}

void
NistSlResourcePoolFactory::SetDataHoppingParameters (int hoppingParameters)
{
  NS_LOG_FUNCTION (this << hoppingParameters);
  m_hoppingParameters = hoppingParameters;
}

void
NistSlResourcePoolFactory::SetDataHoppingSubbands (std::string subbands)
{
  NS_LOG_FUNCTION (this << subbands);
  m_subbands = subbands;
}

void
NistSlResourcePoolFactory::SetDataHoppingOffset (int rbOffset)
{
  NS_LOG_FUNCTION (this << rbOffset);
  m_rbOffset = rbOffset;
}

// UE Selected Parameters
void
NistSlResourcePoolFactory::SetHaveUeSelectedResourceConfig (bool ueSelected)
{
  NS_LOG_FUNCTION (this << ueSelected);
  m_ueSelected = ueSelected;
  //trace
  std::ofstream setUeSelectedFile;
  setUeSelectedFile.open("ModeSwitch.unimo", std::ios_base::app);
  setUeSelectedFile << this << ueSelected << "\r\n";
  setUeSelectedFile.close();
}

void
NistSlResourcePoolFactory::SetHaveTrptSubset (bool haveTrptSubset)
{
  NS_LOG_FUNCTION (this << haveTrptSubset);
  m_haveTrptSubset = haveTrptSubset;
}

void
NistSlResourcePoolFactory::SetTrptSubset (int value)
{
  NS_LOG_FUNCTION (this << value);
  m_trptSubsetValue = value;
}

void
NistSlResourcePoolFactory::SetDataPrbNum (int prbNum)
{
  NS_LOG_FUNCTION (this << prbNum);
  m_dataPrbNum = prbNum;
}

void
NistSlResourcePoolFactory::SetDataPrbStart (int prbStart)
{
  NS_LOG_FUNCTION (this << prbStart);
  m_dataPrbStart = prbStart;
}

void
NistSlResourcePoolFactory::SetDataPrbEnd (int prbEnd)
{
  NS_LOG_FUNCTION (this << prbEnd);
  m_dataPrbEnd = prbEnd;
}

void
NistSlResourcePoolFactory::SetDataOffset (int offset)
{
  NS_LOG_FUNCTION (this << offset);
  m_dataOffset = offset;
}

void
NistSlResourcePoolFactory::SetDataBitmap (int value)
{
  NS_LOG_FUNCTION (this << value);
  m_dataBitmapValue = value;
}

// Tx Parameters
void
NistSlResourcePoolFactory::SetHaveTxParameters (bool txParam)
{
  NS_LOG_FUNCTION (this << txParam);
  m_txParam = txParam;
}

void
NistSlResourcePoolFactory::SetControlTxAlpha (std::string alpha)
{
  NS_LOG_FUNCTION (this << alpha);
  m_txAlpha = alpha;
}

void
NistSlResourcePoolFactory::SetControlTxP0 (int p0)
{
  NS_LOG_FUNCTION (this << p0);
  m_txP0 = p0;
}

void
NistSlResourcePoolFactory::SetDataTxAlpha (std::string alpha)
{
  NS_LOG_FUNCTION (this << alpha);
  m_dataAlpha = alpha;
}

void
NistSlResourcePoolFactory::SetDataTxP0 (int p0)
{
  NS_LOG_FUNCTION (this << p0);
  m_dataP0 = p0;
}

} // namespace ns3
