/*
 *
 * Copyright 2021-2023 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#pragma once

#include "srsran/adt/bounded_bitset.h"
#include "srsran/adt/static_vector.h"
#include "srsran/phy/constants.h"
#include "srsran/phy/support/resource_grid.h"
#include "srsran/ran/cyclic_prefix.h"
#include "srsran/ran/slot_point.h"

namespace srsran {

/// Describes a DMRS for PDCCH processor interface.
class dmrs_pdcch_processor
{
public:
  /// Describes the necessary parameters to generate DMRS for a PDCCH transmission.
  struct config_t {
    /// Provides the slot timing and numerology.
    slot_point slot;
    /// Cyclic prefix.
    cyclic_prefix cp;
    /// Reference point for PDCCH DMRS \e k in RBs.
    unsigned reference_point_k_rb;
    /// Indicates the PRB used for the PDCCH transmission. The bit at position zero corresponds to CRB0.
    bounded_bitset<MAX_RB> rb_mask;
    /// CORESET start symbol index.
    unsigned start_symbol_index;
    /// CORESET duration in symbols.
    unsigned duration;
    /// Higher layer parameter PDCCH-DMRS-ScramblingID if it is given, otherwise the physical cell identifier.
    unsigned n_id;
    /// Provides the linear signal amplitude to conform with the transmission power.
    float amplitude;
    /// Port indexes the PDSCH transmission is mapped onto.
    static_vector<uint8_t, MAX_PORTS> ports;
  };

  /// Default destructor.
  virtual ~dmrs_pdcch_processor() = default;

  /// \brief Generates and maps DMRS for PDCCH according to TS 38.211 section 7.4.1.3.
  ///
  /// \param[out] grid Provides the destination resource grid.
  /// \param[in] config Provides the required configuration to generate and map the signal.
  virtual void map(resource_grid_writer& grid, const config_t& config) = 0;
};

} // namespace srsran