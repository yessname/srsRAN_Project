/*
 *
 * Copyright 2013-2022 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "pdcp_entity_tx.h"
#include "srsgnb/support/srsgnb_assert.h"

using namespace srsgnb;

/// \brief Receive an SDU from the upper layers, apply encription
/// and integrity protection and pass the resulting PDU
/// to the lower layers.
///
/// \param buf Buffer that hold the SDU from higher layers.
/// \ref TS 38.323 section 5.2.1: Transmit operation
void pdcp_entity_tx::handle_sdu(byte_buffer buf)
{
  bool max_hfn_reached = false; // FIXME actually check HFN
  if (max_hfn_reached) {
    upper_cn.on_max_hfn_reached();
  }

  // Start discard timer
  // TODO

  // Perform header compression
  // TODO

  // Write PDCP header info
  byte_buffer hdr = {};
  write_data_pdu_header(hdr, st.tx_next);

  // Apply ciphering and integrity protection
  byte_buffer protected_buf = {};
  apply_ciphering_and_integrity_protection(hdr, buf, st.tx_next, protected_buf);

  // Set meta-data for RLC (TODO)
  // sdu->md.pdcp_sn = tx_next;

  logger.log_info(protected_buf.begin(),
                  protected_buf.end(),
                  "TX PDU ({}B), COUNT={}, HFN={}, SN={}, integrity={}, encryption={}",
                  protected_buf.length(),
                  st.tx_next,
                  HFN(st.tx_next),
                  SN(st.tx_next),
                  integrity_enabled,
                  ciphering_enabled);

  // Check if PDCP is associated with more than on RLC entity TODO
  // Write to lower layers
  lower_dn.on_new_pdu(std::move(protected_buf));

  // Increment TX_NEXT
  st.tx_next++;
}

/*
 * Ciphering and Integrity Protection Helpers
 */
void pdcp_entity_tx::apply_ciphering_and_integrity_protection(byte_buffer& hdr,
                                                              byte_buffer& sdu,
                                                              uint32_t     count,
                                                              byte_buffer& protected_buf)
{
  // TS 38.323, section 5.9: Integrity protection
  // The data unit that is integrity protected is the PDU header
  // and the data part of the PDU before ciphering.
  sec_mac mac = {};
  if (integrity_enabled == pdcp_integrity_enabled::enabled) {
    byte_buffer buf = {};
    buf.append(hdr);
    buf.append(sdu);
    integrity_generate(buf, count, mac);
  }

  // TS 38.323, section 5.8: Ciphering
  // The data unit that is ciphered is the MAC-I and the
  // data part of the PDCP Data PDU except the
  // SDAP header and the SDAP Control PDU if included in the PDCP SDU.
  byte_buffer ct;
  if (ciphering_enabled == pdcp_ciphering_enabled::enabled) {
    byte_buffer buf = {};
    buf.append(sdu);
    // Append MAC-I
    if (is_srb() || (is_drb() && (integrity_enabled == pdcp_integrity_enabled::enabled))) {
      buf.append(mac);
    }
    cipher_encrypt(buf, count, ct);
  } else {
    ct.append(sdu);
    // Append MAC-I
    if (is_srb() || (is_drb() && (integrity_enabled == pdcp_integrity_enabled::enabled))) {
      ct.append(mac);
    }
  }

  // Construct the protected buffer
  protected_buf.append(hdr);
  protected_buf.append(ct);
}

void pdcp_entity_tx::integrity_generate(byte_buffer& buf, uint32_t count, sec_mac& mac)
{
  // If control plane use RRC integrity key. If data use user plane key
  const sec_128_as_key& k_int = is_srb() ? sec_cfg.k_128_rrc_int : sec_cfg.k_128_up_int;
  switch (sec_cfg.integ_algo) {
    case integrity_algorithm::nia0:
      break;
    case integrity_algorithm::nia1:
      security_nia1(k_int, count, lcid - 1, direction, buf, mac);
      break;
    case integrity_algorithm::nia2:
      // security_nia2(&k128, count, 0, cfg.tx_direction, msg, msg_len, mac);
      break;
    case integrity_algorithm::nia3:
      // security_nia3(&k128, count, 0, cfg.tx_direction, msg, msg_len, mac);
      break;
    default:
      break;
  }

  logger.log_debug("Integrity gen input: COUNT {}, Bearer ID {}, Direction {}", count, lcid, direction);
  logger.log_debug((uint8_t*)k_int.data(), k_int.size(), "Integrity gen key:");
  logger.log_debug(buf.begin(), buf.end(), "Integrity gen input message:");
  logger.log_debug((uint8_t*)mac.data(), mac.size(), "MAC (generated)");
}

void pdcp_entity_tx::cipher_encrypt(byte_buffer_view msg, uint32_t count, byte_buffer& ct)
{
  // If control plane use RRC integrity key. If data use user plane key
  const sec_128_as_key& k_enc = is_srb() ? sec_cfg.k_128_rrc_enc : sec_cfg.k_128_up_enc;

  logger.log_debug("Cipher encrypt input: COUNT: {}, Bearer ID: {}, Direction {}", count, lcid, direction);
  logger.log_debug((uint8_t*)k_enc.data(), k_enc.size(), "Cipher encrypt key:");
  logger.log_debug(msg.begin(), msg.end(), "Cipher encrypt input msg");

  switch (sec_cfg.cipher_algo) {
    case ciphering_algorithm::nea0:
      break;
    case ciphering_algorithm::nea1:
      ct = security_nea1(k_enc, count, lcid - 1, direction, msg);
      break;
    case ciphering_algorithm::nea2:
      // security_128_eea2(&(k_enc[16]), count, cfg.bearer_id - 1, cfg.tx_direction, msg, msg_len, ct_tmp);
      // memcpy(ct, ct_tmp, msg_len);
      break;
    case ciphering_algorithm::nea3:
      // security_128_eea3(&(k_enc[16]), count, cfg.bearer_id - 1, cfg.tx_direction, msg, msg_len, ct_tmp);
      // memcpy(ct, ct_tmp, msg_len);
      break;
    default:
      break;
  }
  logger.log_debug(ct.begin(), ct.end(), "Cipher encrypt output msg");
}

/*
 * PDU Helpers
 */
bool pdcp_entity_tx::write_data_pdu_header(byte_buffer& buf, uint32_t count)
{
  // Sanity check: 18bit SRB not allowed
  srsgnb_assert(!(is_srb() && cfg.sn_size == pdcp_sn_size::size18bits), "Invalid 18 bit SRB PDU");

  byte_buffer_writer hdr_writer = buf;

  // Set D/C if required
  if (is_drb()) {
    hdr_writer.append(0x80); // D/C bit field (1).
  } else {
    hdr_writer.append(0x00); // No D/C bit field.
  }

  // Set SN
  uint32_t sn = SN(count);
  // Add SN
  switch (cfg.sn_size) {
    case pdcp_sn_size::size12bits:
      hdr_writer.back() |= (sn & 0x00000f00U) >> 8U;
      hdr_writer.append((sn & 0x000000ffU));
      break;
    case pdcp_sn_size::size18bits:
      hdr_writer.back() |= (sn & 0x00030000U) >> 16U;
      hdr_writer.append((sn & 0x0000ff00U) >> 8U);
      hdr_writer.append((sn & 0x000000ffU));
      break;
    default:
      logger.log_error("Invalid SN length configuration: {} bits", cfg.sn_size);
  }
  return true;
}