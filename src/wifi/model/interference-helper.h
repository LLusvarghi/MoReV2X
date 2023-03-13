/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005,2006 INRIA
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#ifndef INTERFERENCE_HELPER_H
#define INTERFERENCE_HELPER_H

#include <stdint.h>
#include <vector>
#include <list>
#include "wifi-mode.h"
#include "wifi-preamble.h"
#include "wifi-phy-standard.h"
#include "ns3/nstime.h"
#include "ns3/simple-ref-count.h"
#include "ns3/wifi-tx-vector.h"

namespace ns3 {

class ErrorRateModel;

/**
 * \ingroup wifi
 * \brief handles interference calculations
 */
class InterferenceHelper
{
public:
  /**
   * Signal event for a packet.
   */
  class Event : public SimpleRefCount<InterferenceHelper::Event>
  {
public:
    /**
     * Create an Event with the given parameters.
     *
     * \param size packet size
     * \param payloadMode Wi-Fi mode used for the payload
     * \param preamble preamble type
     * \param duration duration of the signal
     * \param rxPower the receive power (w)
     * \param txvector TXVECTOR of the packet
     */
    Event (uint32_t size, WifiMode payloadMode,
           enum WifiPreamble preamble,
           Time duration, double rxPower, WifiTxVector txvector);
    ~Event ();

    /**
     * Return the duration of the signal.
     *
     * \return the duration of the signal
     */
    Time GetDuration (void) const;
    /**
     * Return the start time of the signal.
     *
     * \return the start time of the signal
     */
    Time GetStartTime (void) const;
    /**
     * Return the end time of the signal.
     *
     * \return the end time of the signal
     */
    Time GetEndTime (void) const;
    /**
     * Return the receive power (w).
     *
     * \return the receive power (w)
     */
    double GetRxPowerW (void) const;
    /**
     * Return the size of the packet (bytes).
     *
     * \return the size of the packet (bytes)
     */
    uint32_t GetSize (void) const;
    /**
     * Return the Wi-Fi mode used for the payload.
     *
     * \return the Wi-Fi mode used for the payload
     */
    WifiMode GetPayloadMode (void) const;
    /**
     * Return the preamble type of the packet.
     *
     * \return the preamble type of the packet
     */
    enum WifiPreamble GetPreambleType (void) const;
    /**
     * Return the TXVECTOR of the packet.
     *
     * \return the TXVECTOR of the packet
     */
    WifiTxVector GetTxVector (void) const;
private:
    uint32_t m_size;
    WifiMode m_payloadMode;
    enum WifiPreamble m_preamble;
    Time m_startTime;
    Time m_endTime;
    double m_rxPowerW;
    WifiTxVector m_txVector;
  };
  /**
   * A struct for both SNR and PER
   */
  struct SnrPer
  {
    double snr;
    double per;
  };

  InterferenceHelper ();
  ~InterferenceHelper ();

  /**
   * Set the noise figure.
   *
   * \param value noise figure
   */
  void SetNoiseFigure (double value);
  /**
   * Set the error rate model for this interference helper.
   *
   * \param rate Error rate model
   */
  void SetErrorRateModel (Ptr<ErrorRateModel> rate);

  /**
   * Return the noise figure.
   *
   * \return the noise figure
   */
  double GetNoiseFigure (void) const;
  /**
   * Return the error rate model.
   *
   * \return Error rate model
   */
  Ptr<ErrorRateModel> GetErrorRateModel (void) const;


  /**
   * \param energyW the minimum energy (W) requested
   * \returns the expected amount of time the observed
   *          energy on the medium will be higher than
   *          the requested threshold.
   */
  Time GetEnergyDuration (double energyW);

  /**
   * Add the packet-related signal to interference helper.
   *
   * \param size packet size
   * \param payloadMode Wi-Fi mode for the payload
   * \param preamble Wi-Fi preamble for the packet
   * \param duration the duration of the signal
   * \param rxPower receive power (w)
   * \param txvector TXVECTOR of the packet
   * \return InterferenceHelper::Event
   */
  Ptr<InterferenceHelper::Event> Add (uint32_t size, WifiMode payloadMode,
                                      enum WifiPreamble preamble,
                                      Time duration, double rxPower, WifiTxVector txvector);

  /**
   * Calculate the SNIR at the start of the packet and accumulate
   * all SNIR changes in the snir vector.
   *
   * \param event the event corresponding to the first time the packet arrives
   * \return struct of SNR and PER
   */
  struct InterferenceHelper::SnrPer CalculateSnrPer (Ptr<InterferenceHelper::Event> event);
  /**
   * Notify that RX has started.
   */
  void NotifyRxStart ();
  /**
   * Notify that RX has ended.
   */
  void NotifyRxEnd ();
  /**
   * Erase all events.
   */
  void EraseEvents (void);
private:
  /**
   * Noise and Interference (thus Ni) event.
   */
  class NiChange
  {
public:
    /**
     * Create a NiChange at the given time and the amount of NI change.
     *
     * \param time time of the event
     * \param delta the power
     */
    NiChange (Time time, double delta);
    /**
     * Return the event time.
     *
     * \return the event time.
     */
    Time GetTime (void) const;
    /**
     * Return the power
     *
     * \return the power
     */
    double GetDelta (void) const;
    /**
     * Compare the event time of two NiChange objects (a < o).
     *
     * \param o
     * \return true if a < o.time, false otherwise
     */
    bool operator < (const NiChange& o) const;
private:
    Time m_time;
    double m_delta;
  };
  /**
   * typedef for a vector of NiChanges
   */
  typedef std::vector <NiChange> NiChanges;
  /**
   * typedef for a list of Events
   */
  typedef std::list<Ptr<Event> > Events;

  //InterferenceHelper (const InterferenceHelper &o);
  //InterferenceHelper &operator = (const InterferenceHelper &o);
  /**
   * Append the given Event.
   *
   * \param event
   */
  void AppendEvent (Ptr<Event> event);
  /**
   * Calculate noise and interference power in W.
   *
   * \param event
   * \param ni
   * \return noise and interference power
   */
  double CalculateNoiseInterferenceW (Ptr<Event> event, NiChanges *ni) const;
  /**
   * Calculate SNR (linear ratio) from the given signal power and noise+interference power.
   * (Mode is not currently used)
   *
   * \param signal
   * \param noiseInterference
   * \param mode
   * \return SNR in liear ratio
   */
  double CalculateSnr (double signal, double noiseInterference, WifiMode mode) const;
  /**
   * Calculate the success rate of the chunk given the SINR, duration, and Wi-Fi mode.
   * The duration and mode are used to calculate how many bits are present in the chunk.
   *
   * \param snir SINR
   * \param duration
   * \param mode
   * \return the success rate
   */
  double CalculateChunkSuccessRate (double snir, Time duration, WifiMode mode) const;
  /**
   * Calculate the error rate of the given packet. The packet can be divided into
   * multiple chunks (e.g. due to interference from other transmissions).
   *
   * \param event
   * \param ni
   * \return the error rate of the packet
   */
  double CalculatePer (Ptr<const Event> event, NiChanges *ni) const;

  double m_noiseFigure; /**< noise figure (linear) */
  Ptr<ErrorRateModel> m_errorRateModel;
  /// Experimental: needed for energy duration calculation
  NiChanges m_niChanges;
  double m_firstPower;
  bool m_rxing;
  /// Returns an iterator to the first nichange, which is later than moment
  NiChanges::iterator GetPosition (Time moment);
  /**
   * Add NiChange to the list at the appropriate position.
   *
   * \param change
   */
  void AddNiChangeEvent (NiChange change);
};

} // namespace ns3

#endif /* INTERFERENCE_HELPER_H */
