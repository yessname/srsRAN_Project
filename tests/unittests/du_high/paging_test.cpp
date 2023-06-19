/*
 *
 * Copyright 2021-2023 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

/// \file
/// \brief Tests that check the transmission of Paging messages by the DU-high class.

#include "lib/f1ap/common/f1ap_asn1_packer.h"
#include "test_utils/du_high_test_bench.h"
#include "tests/unittests/gateways/test_helpers.h"
#include "srsran/ran/bcd_helpers.h"
#include "srsran/support/test_utils.h"

using namespace srsran;
using namespace srs_du;
using namespace asn1::f1ap;

class paging_tester : public du_high_test_bench, public testing::Test
{};

f1ap_message generate_paging_message(uint64_t five_g_tmsi, const nr_cell_global_id_t& nr_cgi)
{
  f1ap_message msg;
  msg.pdu.set_init_msg().load_info_obj(ASN1_F1AP_ID_PAGING);
  paging_s& paging = msg.pdu.init_msg().value.paging();

  // Add ue id idx value.
  // UE Identity Index value is defined as: UE_ID 5G-S-TMSI mod 1024  (see TS 38.304 section 7.1).
  paging->ue_id_idx_value.value.set_idx_len10().from_number(five_g_tmsi % 1024);

  // Add paging id.
  paging->paging_id.value.set_cn_ue_paging_id().set_five_g_s_tmsi().from_number(five_g_tmsi);

  // Add paging DRX.
  paging->paging_drx_present = true;
  paging->paging_drx->value  = asn1::f1ap::paging_drx_opts::v32;

  // Add paging cell list.
  asn1::protocol_ie_single_container_s<asn1::f1ap::paging_cell_item_ies_o> asn1_paging_cell_item_container;
  auto& asn1_paging_cell_item = asn1_paging_cell_item_container->paging_cell_item();
  asn1_paging_cell_item.nr_cgi.nr_cell_id.from_number(nr_cgi.nci);
  asn1_paging_cell_item.nr_cgi.plmn_id.from_number(plmn_string_to_bcd(nr_cgi.plmn));
  paging->paging_cell_list.value.push_back(asn1_paging_cell_item_container);

  return msg;
}

TEST_F(paging_tester, when_paging_message_is_received_its_relayed_to_ue)
{
  static constexpr uint64_t five_g_tmsi = 0x01011066fef7;

  // Receive F1AP paging message.
  cu_notifier.last_f1ap_msgs.clear();
  ASSERT_TRUE(not this->du_high_cfg.cells.empty());
  const auto du_cell_cfg = this->du_high_cfg.cells[0];
  this->du_obj.get_f1ap_message_handler().handle_message(generate_paging_message(five_g_tmsi, du_cell_cfg.nr_cgi));
  // Flag indicating whether UE is Paged or not.
  bool ue_is_paged{false};

  const unsigned MAX_COUNT = 50 * this->next_slot.nof_slots_per_frame();
  for (unsigned i = 0; i != MAX_COUNT; ++i) {
    this->run_slot();

    for (const auto& pg_grant : this->phy.cell.last_dl_res->dl_res->paging_grants) {
      const auto& pg_ue_it =
          std::find_if(pg_grant.paging_ue_list.begin(), pg_grant.paging_ue_list.end(), [](const paging_ue_info& ue) {
            return ue.paging_type_indicator == srsran::paging_ue_info::cn_ue_paging_identity and
                   ue.paging_identity == five_g_tmsi;
          });
      if (pg_ue_it != pg_grant.paging_ue_list.end()) {
        ue_is_paged = true;
        break;
      }
    }
  }
  ASSERT_TRUE(ue_is_paged);
}
