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
#ifndef ERROR_RATE_MODEL_H
#define ERROR_RATE_MODEL_H

#include <stdint.h>
#include "wifi-mode.h"
#include "ns3/object.h"

namespace ns3 {
/**
 * \ingroup wifi
 * \brief the interface for Wifi's error models
 *
 */
class ErrorRateModel : public Object
{
public:
  static TypeId GetTypeId (void);

  /**
   * \param txMode a specific transmission mode
   * \param ber a target ber
   * \returns the snr which corresponds to the requested
   *          ber.
   */
  double CalculateSnr (WifiMode txMode, double ber) const;

  /**
   * A pure virtual method that must be implemented in the subclass.
   * This method returns the probability that the given 'chuck' of the
   * packet will be successfully received by the PHY.
   *
   * A chuck can be viewed as a part of a packet with equal SNR.
   * The probability of successfully receiving the chunk depends on
   * the mode, the SNR, and the size of the chunk.
   *
   * \param mode the Wi-Fi mode the chunk is sent
   * \param snr the SNR of the chunk
   * \param nbits the number of bits in this chunk
   * \return probability of successfully receiving the chunk
   */
  virtual double GetChunkSuccessRate (WifiMode mode, double snr, uint32_t nbits) const = 0;
};

} // namespace ns3

#endif /* ERROR_RATE_MODEL_H */
