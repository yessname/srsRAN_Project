/*
 *
 * Copyright 2021-2024 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#pragma once

#include "../../f1ap/common/asn1_helpers.h"
#include "../cu_cp_impl_interface.h"
#include "../du_processor/du_processor_impl_interface.h"
#include "../du_processor/du_setup_handler.h"
#include "srsran/cu_cp/cu_cp.h"
#include "srsran/f1ap/cu_cp/f1ap_cu.h"
#include "srsran/pdcp/pdcp_rx.h"

namespace srsran {
namespace srs_cu_cp {

/// Adapter between F1AP and CU-CP
class f1ap_cu_cp_adapter : public f1ap_ue_removal_notifier
{
public:
  void connect_cu_cp(cu_cp_ue_removal_handler& ue_removal_handler_) { ue_removal_handler = &ue_removal_handler_; }

  void on_ue_removal_required(ue_index_t ue_index) override
  {
    srsran_assert(ue_removal_handler != nullptr, "CU-CP UE removal handler must not be nullptr");
    return ue_removal_handler->handle_ue_removal_request(ue_index);
  }

private:
  cu_cp_ue_removal_handler* ue_removal_handler = nullptr;
};

/// Adapter between F1AP and DU repository, to handle DU specific procedure outcomes (e.g. F1 Remove)
class f1ap_du_repository_adapter : public f1ap_du_management_notifier
{
public:
  void connect_du_repository(cu_cp_f1c_handler& du_handler_) { du_handler = &du_handler_; }

  void on_du_remove_request_received(const du_index_t du_index) override
  {
    srsran_assert(du_handler != nullptr, "DU handler must not be nullptr");
    du_handler->handle_du_remove_request(du_index);
  }

private:
  cu_cp_f1c_handler* du_handler = nullptr;
};

/// Adapter between F1AP and DU processor
class f1ap_du_processor_adapter : public f1ap_du_processor_notifier
{
public:
  f1ap_du_processor_adapter(du_setup_handler& du_setup_hdlr_) : du_setup_hdlr(&du_setup_hdlr_) {}

  void connect_du_processor(du_processor_f1ap_interface& du_processor_f1ap_) { du_f1ap_handler = &du_processor_f1ap_; }

  du_setup_result on_new_du_setup_request(const du_setup_request& msg) override
  {
    srsran_assert(du_setup_hdlr != nullptr, "F1AP handler must not be nullptr");
    return du_setup_hdlr->handle_du_setup_request(msg);
  }

  du_index_t get_du_index() override { return du_f1ap_handler->get_du_index(); }

  cu_cp_ue_creation_response on_ue_creation_request(const cu_cp_ue_creation_request& msg) override
  {
    srsran_assert(du_f1ap_handler != nullptr, "F1AP handler must not be nullptr");
    return du_f1ap_handler->handle_ue_creation_request(msg);
  }

  void on_du_initiated_ue_context_release_request(const f1ap_ue_context_release_request& req) override
  {
    srsran_assert(du_f1ap_handler != nullptr, "F1AP handler must not be nullptr");
    du_f1ap_handler->handle_du_initiated_ue_context_release_request(req);
  }

private:
  du_setup_handler*            du_setup_hdlr   = nullptr;
  du_processor_f1ap_interface* du_f1ap_handler = nullptr;
};

/// Adapter between F1AP and RRC UE
class f1ap_rrc_ue_adapter : public f1ap_rrc_message_notifier
{
public:
  void connect_rrc_ue(rrc_ul_ccch_pdu_handler& rrc_ul_ccch_handler_, rrc_ul_dcch_pdu_handler& rrc_ul_dcch_handler_)
  {
    rrc_ul_ccch_handler = &rrc_ul_ccch_handler_;
    rrc_ul_dcch_handler = &rrc_ul_dcch_handler_;
  }

  void on_ul_ccch_pdu(byte_buffer pdu) override
  {
    srsran_assert(rrc_ul_ccch_handler != nullptr, "RRC UL CCCH handler must not be nullptr");
    rrc_ul_ccch_handler->handle_ul_ccch_pdu(std::move(pdu));
  }

  void on_ul_dcch_pdu(const srb_id_t srb_id, byte_buffer pdu) override
  {
    srsran_assert(rrc_ul_dcch_handler != nullptr, "RRC UL DCCH handler must not be nullptr");
    rrc_ul_dcch_handler->handle_ul_dcch_pdu(srb_id, std::move(pdu));
  }

private:
  rrc_ul_ccch_pdu_handler* rrc_ul_ccch_handler = nullptr;
  rrc_ul_dcch_pdu_handler* rrc_ul_dcch_handler = nullptr;
};

} // namespace srs_cu_cp
} // namespace srsran
