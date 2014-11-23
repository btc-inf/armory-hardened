/**
 * Copyright (c) 2014 Moritz Schuhmann
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES
 * OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef TRANSACTION_H_
#define TRANSACTION_H_

#include <asf.h>
#include <string.h>

#include "bitcoin.h"
#include "base64.h"
#include "ecc.h"

#include "bytestream.h"
#include "transaction.h"
#include "wallet.h"

typedef struct _B64_CTX {
	BYT buff_bp;
	uint8_t buff[48];
	FIL fp;
} B64_CTX;

void armtx_fetch_unsigned_file(const char *filename, char *uniqueid, BYT *armtx_bp);
uint8_t armtx_build_signed_file(BYT *armtx_bp, const char *uniqueid, const WLT *wallet, const char *wlt_filename);
uint8_t armtx_get_latest_unsigned_file(char *filename);

void armtx_push_base64blocks_to_file_init(B64_CTX *ctx, const char *filename, const char *uniqueid);
void armtx_push_base64blocks_to_file_update(B64_CTX *ctx, const uint8_t *src, const uint16_t len);
void armtx_push_base64blocks_to_file_final(B64_CTX *ctx);

void armtx_get_uniqueid(BYT *armtx_bp, char *uniqueid);

uint16_t armtx_get_txin_cnt(BYT *armtx_bp);
size_t armtx_get_txin_pos(BYT *armtx_bp, const uint16_t txin);
size_t armtx_get_txin_len(BYT *armtx_bp, const uint16_t txin);
size_t armtx_get_txin_data_pos(BYT *armtx_bp, const uint16_t txin);
uint32_t armtx_get_txin_previndex(BYT *armtx_bp, const uint16_t txin);
size_t armtx_get_txin_supptx_pos(BYT *armtx_bp, const uint16_t txin);
size_t armtx_get_txin_supptx_len(BYT *armtx_bp, const uint16_t txin);
uint64_t armtx_get_txin_inputvalue(BYT *armtx_bp, const uint16_t txin);
size_t armtx_get_txin_pubkey_pos(BYT *armtx_bp, const uint16_t txin);
size_t armtx_get_txin_pubkey(BYT *armtx_bp, const uint16_t txin, uint8_t *pubkey);
size_t armtx_get_txin_sequence_pos(BYT *armtx_bp, const uint16_t txin);

size_t armtx_get_txout_cnt_pos(BYT *armtx_bp);
uint16_t armtx_get_txout_cnt(BYT *armtx_bp);
size_t armtx_get_txout_pos(BYT *armtx_bp, const uint16_t txout);
size_t armtx_get_txout_len(BYT *armtx_bp, const uint16_t txout);
size_t armtx_get_txout_data_pos(BYT *armtx_bp, const uint16_t txout);
size_t armtx_get_txout_script_pos(BYT *armtx_bp, const uint16_t txout);
size_t armtx_get_txout_script_len(BYT *armtx_bp, const uint16_t txout);
size_t armtx_get_txout_script(BYT *armtx_bp, const uint16_t txout, uint8_t *script);
size_t armtx_get_txout_value_pos(BYT *armtx_bp, const uint16_t txout);
uint64_t armtx_get_txout_value(BYT *armtx_bp, const uint16_t txout);

uint64_t armtx_get_total_inputvalue(BYT *armtx_bp);
uint64_t armtx_get_total_outputvalue(BYT *armtx_bp);
uint64_t armtx_get_fee(BYT *armtx_bp);

void armtx_build_unsigned_txhash_limbuffer(BYT *armtx_bp, const uint16_t txin, uint8_t *txhash);
void armtx_build_unsigned_txhash(BYT *armtx_bp, const uint16_t txin, uint8_t *txhash);

uint8_t armtx_build_txin_sig(BYT *armtx_bp, const uint16_t txin, const /*WLT *wallet*/uint8_t *privkey, BYT *sig_bp);

uint8_t build_DER_sig(const uint8_t *hashtosign, const uint8_t *privkey, BYT *sig_bp);

uint16_t tx_get_txin_cnt(BYT *tx_bp);
size_t tx_get_txin_pos(BYT *tx_bp, const uint16_t txin);
size_t tx_get_txin_sig_pos(BYT *tx_bp, const uint16_t txin);
size_t tx_get_txin_sig_len(BYT *tx_bp, const uint16_t txin);

size_t tx_get_txout_cnt_pos(BYT *tx_bp);
uint16_t tx_get_txout_cnt(BYT *tx_bp);
size_t tx_get_txout_pos(BYT *tx_bp, const uint16_t txout);
size_t tx_get_txout_script_pos(BYT *tx_bp, const uint16_t txout);
size_t tx_get_txout_value_pos(BYT *tx_bp, const uint16_t txout);
uint64_t tx_get_txout_value(BYT *tx_bp, const uint16_t txout);

#endif /* TRANSACTION_H_ */