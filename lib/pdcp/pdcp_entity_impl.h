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

#include "pdcp_bearer_logger.h"
#include "pdcp_entity_rx.h"
#include "pdcp_entity_tx.h"
#include "srsgnb/pdcp/pdcp_config.h"
#include "srsgnb/pdcp/pdcp_entity.h"
#include "srsgnb/support/timers.h"
#include <cstdio>
#include <memory>

namespace srsgnb {

class pdcp_entity_impl : public pdcp_entity
{
public:
  pdcp_entity_impl(uint32_t                        ue_index,
                   rb_id_t                         rb_id,
                   pdcp_config                     config,
                   pdcp_tx_lower_notifier&         tx_lower_dn,
                   pdcp_tx_upper_control_notifier& tx_upper_cn,
                   pdcp_rx_upper_data_notifier&    rx_upper_dn,
                   pdcp_rx_upper_control_notifier& rx_upper_cn,
                   timer_manager&                  timers) :
    logger("PDCP", {ue_index, rb_id})
  {
    tx = std::make_unique<pdcp_entity_tx>(ue_index, rb_id, config.tx, tx_lower_dn, tx_upper_cn, timers);
    logger.log_info("PDCP TX configured: {}", config.tx);
    rx = std::make_unique<pdcp_entity_rx>(ue_index, rb_id, config.rx, rx_upper_dn, rx_upper_cn, timers);
    logger.log_info("PDCP RX configured: {}", config.rx);

    // Tx/Rx interconnect
    tx->set_status_provider(rx.get());
    rx->set_status_handler(tx.get());
  }
  ~pdcp_entity_impl() override = default;
  pdcp_tx_upper_control_interface& get_tx_upper_control_interface() final { return (*tx); };
  pdcp_tx_upper_data_interface&    get_tx_upper_data_interface() final { return (*tx); };
  pdcp_tx_lower_interface&         get_tx_lower_interface() final { return (*tx); };
  pdcp_rx_upper_control_interface& get_rx_upper_control_interface() final { return (*rx); };
  pdcp_rx_lower_interface&         get_rx_lower_interface() final { return (*rx); };

  pdcp_metrics_container get_metrics() final
  {
    pdcp_metrics_container m;
    m.tx = tx->get_metrics();
    m.rx = rx->get_metrics();
    return m;
  };

private:
  pdcp_bearer_logger logger;

  std::unique_ptr<pdcp_entity_tx> tx = {};
  std::unique_ptr<pdcp_entity_rx> rx = {};
};
} // namespace srsgnb
