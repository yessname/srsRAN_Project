/*
 *
 * Copyright 2013-2022 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#pragma once

#include "../adapters/f1_adapters.h"
#include "../adapters/mac_adapters.h"
#include "../adapters/rlc_adapters.h"
#include "srsgnb/adt/optional.h"
#include "srsgnb/adt/slotted_array.h"
#include "srsgnb/ran/lcid.h"
#include "srsgnb/rlc/rlc_config.h"
#include "srsgnb/rlc/rlc_entity.h"

namespace srsgnb {
namespace srs_du {

/// \brief Connector of the MAC, RLC and F1 for a given DU UE SRB bearer.
struct du_srb_connector {
  mac_sdu_rx_adapter              mac_rx_sdu_notifier;
  mac_sdu_tx_adapter              mac_tx_sdu_notifier;
  rlc_rx_rrc_sdu_adapter          rlc_rx_sdu_notif;
  rlc_tx_data_notifier            rlc_tx_data_notif;
  rlc_tx_control_notifier         rlc_tx_ctrl_notif;
  rlc_tx_mac_buffer_state_updater rlc_tx_buffer_state_notif;
  f1c_rx_sdu_rlc_adapter          f1c_rx_sdu_notif;

  /// \brief Connect bearers of MAC, RLC and F1AP layers.
  void connect(du_ue_index_t                       ue_index,
               srb_id_t                            srb_id,
               f1c_bearer&                         f1_bearer,
               rlc_entity&                         rlc_bearer,
               mac_ue_control_information_handler& mac_ue_info_handler);
};

/// \brief Connector of the MAC, RLC and F1 for a given DU UE DRB bearer.
struct du_drb_connector {
  mac_sdu_rx_adapter              mac_rx_sdu_notifier;
  mac_sdu_tx_adapter              mac_tx_sdu_notifier;
  rlc_f1u_tx_sdu_adapter          rlc_rx_sdu_notif;
  rlc_tx_data_notifier            rlc_tx_data_notif;
  rlc_tx_control_notifier         rlc_tx_ctrl_notif;
  rlc_tx_mac_buffer_state_updater rlc_tx_buffer_state_notif;
  f1u_rx_rlc_sdu_adapter          f1u_rx_sdu_notif;

  /// \brief Connect MAC, RLC and F1AP layers if bearer is a DRB.
  void connect(du_ue_index_t                       ue_index,
               drb_id_t                            drb_id,
               lcid_t                              lcid,
               f1u_bearer&                         f1_bearer,
               rlc_entity&                         rlc_bearer,
               mac_ue_control_information_handler& mac_ue_info_handler);
};

/// \brief SRB instance in DU manager. It contains SRB configuration information, RLC entity and adapters between
/// layers.
struct du_ue_srb {
  srb_id_t                    srb_id;
  rlc_config                  rlc_cfg;
  std::unique_ptr<rlc_entity> rlc_bearer;
  du_srb_connector            connector;

  lcid_t lcid() const { return srb_id_to_lcid(srb_id); }
};

/// \brief DRB instance in DU manager. It contains DRB configuration information, RLC entity and adapters between
/// layers.
struct du_ue_drb {
  drb_id_t                    drb_id;
  lcid_t                      lcid;
  rlc_config                  rlc_cfg;
  std::unique_ptr<rlc_entity> rlc_bearer;
  du_drb_connector            connector;
};

/// \brief Bearer container for a UE object in the DU manager.
class du_ue_bearer_manager
{
  struct drb_id_to_index {
    constexpr size_t   get_index(drb_id_t drb_id) const { return static_cast<size_t>(drb_id) - 1; }
    constexpr drb_id_t get_id(size_t idx) const { return static_cast<drb_id_t>(idx + 1); }
  };

public:
  void add_srb(srb_id_t srb_id, const rlc_config& rlc_cfg);
  void add_drb(drb_id_t drb_id, lcid_t lcid, const rlc_config& rlc_cfg);

  const slotted_id_table<srb_id_t, du_ue_srb, MAX_NOF_SRBS>&                  srbs() const { return srbs_; }
  slotted_id_table<srb_id_t, du_ue_srb, MAX_NOF_SRBS>&                        srbs() { return srbs_; }
  const slotted_id_table<drb_id_t, du_ue_drb, MAX_NOF_DRBS, drb_id_to_index>& drbs() const { return drbs_; };
  slotted_id_table<drb_id_t, du_ue_drb, MAX_NOF_DRBS, drb_id_to_index>&       drbs() { return drbs_; };

  optional<lcid_t> allocate_lcid() const;

private:
  slotted_id_table<srb_id_t, du_ue_srb, MAX_NOF_SRBS>                  srbs_;
  slotted_id_table<drb_id_t, du_ue_drb, MAX_NOF_DRBS, drb_id_to_index> drbs_;
};

} // namespace srs_du
} // namespace srsgnb