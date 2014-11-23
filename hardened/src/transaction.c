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

#include <asf.h>
#include <string.h>

#include "wallet.h"
#include "bitcoin.h"
#include "base64.h"
#include "ecc.h"

#include "bytestream.h"
#include "transaction.h"

/*
*
* Functions to read Armory compliant decoded transactions:
*
*/
void armtx_get_uniqueid(BYT *armtx_bp, char *uniqueid)
{
	uint8_t txhash[32];
	char b58txhash[45];
	armtx_build_unsigned_txhash(armtx_bp, 0xFFFF, txhash); // build hash of tx with all sigscripts empty
	base58_encode(txhash, 32, b58txhash);
	memcpy(uniqueid, b58txhash, 8); // trim to 8 chars
	*(uniqueid + 9) = '\0'; // terminate id string
}

uint16_t armtx_get_txin_cnt(BYT *armtx_bp)
{
	b_rewind(armtx_bp); // this method is the base for every other op, so we make sure to start at 0+rewofs
	b_seek(armtx_bp, b_tell(armtx_bp) + 12); // work relational for the case of limited workspace
	return b_getvint(armtx_bp);
}

size_t armtx_get_txin_pos(BYT *armtx_bp, const uint16_t txin)
{
	if(txin >= armtx_get_txin_cnt(armtx_bp)) return 0; // verify txin while moving over txin_cnt

	for(uint16_t i = 0; i < txin; i++) {
		b_seek(armtx_bp, b_getvint(armtx_bp) + b_tell(armtx_bp));
	}
	return b_tell(armtx_bp);
}

size_t armtx_get_txin_len(BYT *armtx_bp, const uint16_t txin)
{
	armtx_get_txin_pos(armtx_bp, txin);
	return b_getvint(armtx_bp);
}

size_t armtx_get_txin_data_pos(BYT *armtx_bp, const uint16_t txin)
{
	armtx_get_txin_len(armtx_bp, txin); // move over txin len varint
	return b_tell(armtx_bp);
}

uint32_t armtx_get_txin_previndex(BYT *armtx_bp, const uint16_t txin)
{
	uint32_t index;
	armtx_get_txin_data_pos(armtx_bp, txin);
	b_seek(armtx_bp, b_tell(armtx_bp) + 4 + 4 + 32); // skip version, mbyte, hash
	b_read(armtx_bp, (uint8_t *) &index, 4); // copy index (is little endian)
	return index;
}

size_t armtx_get_txin_supptx_pos(BYT *armtx_bp, const uint16_t txin)
{
	armtx_get_txin_data_pos(armtx_bp, txin);
	b_seek(armtx_bp, b_tell(armtx_bp) + 4 + 4 + 32 + 4); // skip version, mbyte, hash, index
	return b_tell(armtx_bp);
}

size_t armtx_get_txin_supptx_len(BYT *armtx_bp, const uint16_t txin)
{
	armtx_get_txin_supptx_pos(armtx_bp, txin);
	return b_getvint(armtx_bp);
}

uint64_t armtx_get_txin_inputvalue(BYT *armtx_bp, const uint16_t txin)
{
	uint64_t val;
	uint32_t i = armtx_get_txin_previndex(armtx_bp, txin);

	armtx_get_txin_supptx_len(armtx_bp, txin); // goto supptx (moves over the len varint)
	b_setrewofs(armtx_bp); // limit the workspace for the tx handlers
	val = tx_get_txout_value(armtx_bp, i);
	b_unsetrewofs(armtx_bp); // get back full workspace for following ops
	return val;
}

size_t armtx_get_txin_sequence_pos(BYT *armtx_bp, const uint16_t txin)
{
	b_seek(armtx_bp, armtx_get_txin_supptx_len(armtx_bp, txin) + b_tell(armtx_bp)); // skip supptx
	b_seek(armtx_bp, b_getvint(armtx_bp) + b_tell(armtx_bp)); // skip p2shScr
	b_seek(armtx_bp, b_getvint(armtx_bp) + b_tell(armtx_bp)); // skip contrib
	b_seek(armtx_bp, b_getvint(armtx_bp) + b_tell(armtx_bp)); // skip contribLbl
	return b_tell(armtx_bp);
}

size_t armtx_get_txin_pubkey_pos(BYT *armtx_bp, const uint16_t txin)
{
	armtx_get_txin_sequence_pos(armtx_bp, txin);
	b_seek(armtx_bp, b_tell(armtx_bp) + 4 + 1); // skip sequence, nEntry
	return b_tell(armtx_bp);
}

size_t armtx_get_txin_pubkey(BYT *armtx_bp, const uint16_t txin, uint8_t *pubkey)
{
	armtx_get_txin_pubkey_pos(armtx_bp, txin); // goto txin pubkey
	b_read(armtx_bp, pubkey, b_getvint(armtx_bp)); // copy pubkey to destination
	return b_tell(armtx_bp); // return pointer to byte after pubkey (=siglist varint)
}

/*uint32_t armtx_get_txin_siglist_pos(BYT *armtx_bp, uint16_t txin)
{
	armtx_get_txin_pubkey_pos(armtx_bp, txin); // goto txin pubkey
	b_lseek(armtx_bp, b_getvint(armtx_bp) + b_tell(armtx_bp)); // skip pubkey
	return b_tell(armtx_bp);
}*/

size_t armtx_get_txout_cnt_pos(BYT *armtx_bp)
{
	uint16_t last_txin = armtx_get_txin_cnt(armtx_bp) - 1; // search last txin
	armtx_get_txin_data_pos(armtx_bp, last_txin); // goto last txin
	b_seek(armtx_bp, b_tell(armtx_bp) + armtx_get_txin_len(armtx_bp, last_txin)); // move over last txin
	return b_tell(armtx_bp);
}

uint16_t armtx_get_txout_cnt(BYT *armtx_bp)
{
	armtx_get_txout_cnt_pos(armtx_bp);
	return b_getvint(armtx_bp);
}

size_t armtx_get_txout_pos(BYT *armtx_bp, const uint16_t txout)
{
	if(txout >= armtx_get_txout_cnt(armtx_bp)) return 0; // verify txout while moving over txout_cnt

	for(uint16_t i = 0; i < txout; i++) {
		b_seek(armtx_bp, b_getvint(armtx_bp) + b_tell(armtx_bp));
	}
	return b_tell(armtx_bp);
}

size_t armtx_get_txout_len(BYT *armtx_bp, const uint16_t txout)
{
	armtx_get_txout_pos(armtx_bp, txout);
	return b_getvint(armtx_bp);
}

size_t armtx_get_txout_data_pos(BYT *armtx_bp, const uint16_t txout)
{
	armtx_get_txout_len(armtx_bp, txout);
	return b_tell(armtx_bp);
}

size_t armtx_get_txout_script_pos(BYT *armtx_bp, const uint16_t txout)
{
	armtx_get_txout_data_pos(armtx_bp, txout);
	b_seek(armtx_bp, b_tell(armtx_bp) + 4 + 4); // skip version, mbyte
	return b_tell(armtx_bp);
}

size_t armtx_get_txout_script_len(BYT *armtx_bp, const uint16_t txout)
{
	armtx_get_txout_script_pos(armtx_bp, txout);
	return b_getvint(armtx_bp);
}

size_t armtx_get_txout_script(BYT *armtx_bp, const uint16_t txout, uint8_t *script)
{
	size_t len = armtx_get_txout_script_len(armtx_bp, txout);
	b_read(armtx_bp, script, len); // copy script to destination
	return len;
}

size_t armtx_get_txout_value_pos(BYT *armtx_bp, const uint16_t txout)
{
	armtx_get_txout_script_pos(armtx_bp, txout);
	b_seek(armtx_bp, b_getvint(armtx_bp) + b_tell(armtx_bp)); // skip script
	return b_tell(armtx_bp);
}

uint64_t armtx_get_txout_value(BYT *armtx_bp, const uint16_t txout)
{
	uint64_t val;
	armtx_get_txout_value_pos(armtx_bp, txout);
	b_read(armtx_bp, (uint8_t *) &val, 8);
	return val;
}

uint64_t armtx_get_total_inputvalue(BYT *armtx_bp)
{
	uint64_t val = 0;
	uint16_t txin_cnt = armtx_get_txin_cnt(armtx_bp);

	for(uint16_t i = 0; i < txin_cnt; i++) {
		val += armtx_get_txin_inputvalue(armtx_bp, i);
	}
	return val;
}

uint64_t armtx_get_total_outputvalue(BYT *armtx_bp)
{
	uint64_t val = 0;
	uint16_t txout_cnt = armtx_get_txout_cnt(armtx_bp);

	for(uint16_t i = 0; i < txout_cnt; i++) {
		val += armtx_get_txout_value(armtx_bp, i);
	}
	return val;
}

uint64_t armtx_get_fee(BYT *armtx_bp)
{
	return
		armtx_get_total_inputvalue(armtx_bp) -
		armtx_get_total_outputvalue(armtx_bp);
}

/*
*
*Functions to read Bitcoin compliant transactions:
*
*/
uint16_t tx_get_txin_cnt(BYT *tx_bp)
{
	b_rewind(tx_bp); // this method is the base for every other op, so we make sure to start at 0+rewofs
	b_seek(tx_bp, b_tell(tx_bp) + 4); // so we always start relational: skip ver
	return b_getvint(tx_bp);
}

size_t tx_get_txin_pos(BYT *tx_bp, const uint16_t txin)
{
	if(txin >= tx_get_txin_cnt(tx_bp)) return 0; // verify txin while moving over txin_cnt

	for(uint16_t i = 0; i < txin; i++) {
		b_seek(tx_bp, b_tell(tx_bp) + 32 + 4); // skip hash, index
		b_seek(tx_bp, b_getvint(tx_bp) + b_tell(tx_bp) + 4); // skip sigscript, sequence
	}
	return b_tell(tx_bp);
}

size_t tx_get_txout_cnt_pos(BYT *tx_bp)
{
	uint16_t last_txin = tx_get_txin_cnt(tx_bp) - 1;

	tx_get_txin_pos(tx_bp, last_txin); // go to last tx_in
	b_seek(tx_bp, b_tell(tx_bp) + 32 + 4); // skip hash, index
	b_seek(tx_bp, b_getvint(tx_bp) + b_tell(tx_bp) + 4); // skip sigscript, sequence
	return b_tell(tx_bp);
}

uint16_t tx_get_txout_cnt(BYT *tx_bp)
{
	tx_get_txout_cnt_pos(tx_bp);
	return b_getvint(tx_bp);
}

size_t tx_get_txout_pos(BYT *tx_bp, const uint16_t txout)
{
	if(txout >= tx_get_txout_cnt(tx_bp)) return 0; // verify txout while moving over txout_cnt

	for(uint16_t i = 0; i < txout; i++) {
		b_seek(tx_bp, b_tell(tx_bp) + 8); // skip value
		b_seek(tx_bp, b_getvint(tx_bp) + b_tell(tx_bp)); // skip pkscript
	}
	return b_tell(tx_bp);
}

size_t tx_get_txout_script_pos(BYT *tx_bp, const uint16_t txout)
{
	tx_get_txout_pos(tx_bp, txout); // goto txout
	b_seek(tx_bp, b_tell(tx_bp) + 8); // skip value
	return b_tell(tx_bp);
}

uint64_t tx_get_txout_value(BYT *tx_bp, const uint16_t txout)
{
	uint64_t val;
	tx_get_txout_pos(tx_bp, txout); // goto txout
	b_read(tx_bp, (uint8_t *) &val, 8); // copy value (is little endian)
	return val;
}

/*
*
* Tx signing and support functions:
*
*/
void armtx_build_unsigned_txhash(BYT *armtx_bp, const uint16_t txin, uint8_t *txhash)
{
	uint8_t buf[70];
	uint8_t pubkey[65];
	uint8_t script[25];
	volatile static uint16_t cnt, scriptlen;

	SHA256_CTX ctx;
	sha256_Init(&ctx); // we push the tx data block-wise to the sha buffer to allow an unlimited tx size

	BYT rawtx_bp;
	b_open(&rawtx_bp, buf, sizeof(buf));

	b_rewind(armtx_bp);
	b_copy(armtx_bp, &rawtx_bp, 4); // copy version

	cnt = armtx_get_txin_cnt(armtx_bp);
	b_putvint(&rawtx_bp, cnt);

	sha256_Update(&ctx, b_addr(&rawtx_bp), b_reopen(&rawtx_bp)); // push 5-13 bytes

	for(uint16_t i = 0; i < cnt; i++) {

		b_seek(armtx_bp, armtx_get_txin_data_pos(armtx_bp, i) + 8); // goto prev hash

		b_copy(armtx_bp, &rawtx_bp, 32+4); // copy prev hash and index

		if(i == txin) {
			armtx_get_txin_pubkey(armtx_bp, i, pubkey);
			pubkey_to_p2pkhscript(pubkey, script); // make temp sigscript for signing

			b_putc(&rawtx_bp, 0x19); // write script len
			b_write(&rawtx_bp, script, 25); // write temp sigscript
			} else {
			b_putc(&rawtx_bp, 0x00); // if this is not the input to sign, set empty sigscript
		}

		armtx_get_txin_sequence_pos(armtx_bp, i);
		b_copy(armtx_bp, &rawtx_bp, 4);

		sha256_Update(&ctx, b_addr(&rawtx_bp), b_reopen(&rawtx_bp)); // push 41-66 bytes
	}

	cnt = armtx_get_txout_cnt(armtx_bp);
	b_putvint(&rawtx_bp, cnt);

	sha256_Update(&ctx, b_addr(&rawtx_bp), b_reopen(&rawtx_bp)); // push 1-9 bytes

	for(uint32_t i = 0; i < cnt; i++) {

		armtx_get_txout_value_pos(armtx_bp, i);
		b_copy(armtx_bp, &rawtx_bp, 8);

		armtx_get_txout_script_pos(armtx_bp, i);
		scriptlen = b_getvint(armtx_bp);
		b_putvint(&rawtx_bp, scriptlen);
		b_copy(armtx_bp, &rawtx_bp, scriptlen);

		sha256_Update(&ctx, b_addr(&rawtx_bp), b_reopen(&rawtx_bp)); // push 32-34 bytes if P2SH or P2PKH
	}

	b_seek(armtx_bp, 8);
	b_copy(armtx_bp, &rawtx_bp, 4); // write locktime

	// append hash code type (SIGHASH_ALL) if this hash is built for signing:
	if(txin < 0xFFFF) {
		b_putc(&rawtx_bp, 0x01);
		b_putc(&rawtx_bp, 0x00);
		b_putc(&rawtx_bp, 0x00);
		b_putc(&rawtx_bp, 0x00);
	}

	sha256_Update(&ctx, b_addr(&rawtx_bp), b_reopen(&rawtx_bp)); // push 4-8 bytes
	sha256_Final(txhash, &ctx); // finalize
	sha256_Raw(txhash, 32, txhash); // 2nd sha round because we need hash256() as output
}

uint8_t armtx_build_txin_sig(BYT *armtx_bp, const uint16_t txin, const /*WLT *wallet*/uint8_t *privkey, BYT *sig_bp)
{
	uint8_t len, /*pubkey[65], privkey[32],*/ hashtosign[32];

	armtx_build_unsigned_txhash(armtx_bp, txin, hashtosign);
	//armtx_get_txin_pubkey(armtx_bp, txin, pubkey);
	//armwl_get_privkey_from_pubkey(wallet, pubkey, privkey);

	b_reopen(sig_bp);
	len = build_DER_sig(hashtosign, privkey, sig_bp);
	//memset(privkey, 0, 32); // clear private key memory

	b_putc(sig_bp, 0x01); // SIGHASH_ALL

	return ++len; // +1 because of hashtype byte; actually a varint, but p2pkh sigs will not exceed 0x49 bytes max
}

uint8_t build_DER_sig(const uint8_t *hashtosign, const uint8_t *privkey, BYT *sig_bp)
{
	uint8_t h[32], p[32], k[32], r[32], s[32];
	uint8_t len = 0x44; // min sequence length

	vli_swap(hashtosign, h); // need hash to sign as little endian for vli arithmetics
	vli_swap(privkey, p); // same with private key

	ecdsa_derive_k(privkey, h, k); // hash to sign used as vli, private key not
	ecdsa_sign(r, s, p, k, h);
	//veri = ecdsa_verify(&pubkey, h, r, s);
	memset(p, 0, 32); // clear private key memory

	vli_swap(r, r); // r and s need to be represented..
	vli_swap(s, s); // ..as big endian in the sig

	if(r[0] & 0x80) len++; // most left bit == 1
	if(s[0] & 0x80) len++; // most left bit == 1

	b_putc(sig_bp, 0x30); // sequence identifier
	b_putc(sig_bp, len); // sequence length
	b_putc(sig_bp, 0x02); // int identifier

	if(r[0] & 0x80) { // most left bit == 1
		b_putc(sig_bp, 0x21); // r length
		b_putc(sig_bp, 0x00);
	} else {
		b_putc(sig_bp, 0x20); // r length
	}
	b_write(sig_bp, &r, 32); // write r
	b_putc(sig_bp, 0x02); // int identifier

	if(s[0] & 0x80) { // most left bit == 1
		b_putc(sig_bp, 0x21); // s length
		b_putc(sig_bp, 0x00);
	} else {
		b_putc(sig_bp, 0x20); // s length
	}
	b_write(sig_bp, &s, 32); // write s

	return len + 2; // +2 to include sequence byte (0x30) and the length byte itself
}

/*
*
* Tx file operation functions:
*
*/
void armtx_push_base64blocks_to_file_init(B64_CTX *ctx, const char *filename, const char *uniqueid)
	{
		f_open(&ctx->fp, filename, FA_READ | FA_WRITE | FA_CREATE_ALWAYS);

		//char header[100];
		//sprintf (header, "=====TXSIGCOLLECT-TESTTEST======================================", uniqueid);

		f_puts("=====TXSIGCOLLECT-", &ctx->fp);
		f_puts(uniqueid, &ctx->fp);
		f_puts("======================================\r\n", &ctx->fp);

		b_open(&ctx->buff_bp, &ctx->buff, sizeof(&ctx->buff));
	}

void armtx_push_base64blocks_to_file_update(B64_CTX *ctx, const uint8_t *src, const uint16_t len)
{
	char buff_b64[64+1];
	uint32_t i = 48 - b_size(&ctx->buff_bp); // how many new bytes to add to the old bytes in the buffer

	if(len >= i) { // we have enough new bytes to fill the buffer
		b_write(&ctx->buff_bp, src, i); // fill it
	} else {
		b_write(&ctx->buff_bp, src, len); // fill in all available new bytes and wait for next call
		return; // stop
	}


	base64enc(buff_b64, b_addr(&ctx->buff_bp), 48);

	b_reopen(&ctx->buff_bp);

	f_puts(buff_b64, &ctx->fp);
	f_puts("\r\n", &ctx->fp);


	uint8_t rem = (len - i) % 48;


	while(i + 46 <= len) {

		base64enc(buff_b64, src + i, 48);
		f_puts(buff_b64, &ctx->fp);
		f_puts("\r\n", &ctx->fp);

		i += 48;
	}

	b_write(&ctx->buff_bp, src + len - rem, rem);
	b_truncate(&ctx->buff_bp);
}

void armtx_push_base64blocks_to_file_final(B64_CTX *ctx)
{
	char buff_b64[64+1];

	//armtx_push_base64blocks_to_file_update(ctx, src, len);

	if(b_size(&ctx->buff_bp) > 0) {
		base64enc(buff_b64, b_addr(&ctx->buff_bp), b_size(&ctx->buff_bp));
		f_puts(buff_b64, &ctx->fp);
		f_puts("\r\n", &ctx->fp);
	}
	memset(buff_b64, '=', 64);
	buff_b64[64] = '\0';
	f_puts(buff_b64, &ctx->fp);
	f_close(&ctx->fp);
}


void armtx_fetch_unsigned_file(const char *filename, char *uniqueid, BYT *armtx_bp)
{
	b_reopen(armtx_bp);

	FIL fp;
	f_open(&fp, filename, FA_READ);

	char armtx_rawbuff[100];//[67];
	uint8_t armtx_binbuff[100];//[46];

	f_gets((TCHAR *) armtx_rawbuff, 67, &fp);
	//memset(uniqueid, 0, 9);
	//memcpy(uniqueid, armtx_rawbuff + 18, 8);

	while(!f_eof(&fp)) {
		f_gets((TCHAR *) armtx_rawbuff, 67, &fp);
		if(armtx_rawbuff[0] != '=') {
			base64dec(armtx_binbuff, armtx_rawbuff, 0);
			b_write(armtx_bp, armtx_binbuff, base64_binlength(armtx_rawbuff, 0));
		}
	}
	f_close(&fp);

	armtx_get_uniqueid(armtx_bp, uniqueid);
}

uint8_t armtx_build_signed_file(BYT *armtx_bp, const char *uniqueid, const WLT *wallet, const char *wlt_filename)
{
	B64_CTX ctx;
	FIL fp_wlt;
	uint16_t txin_cnt;

	char fn[100], q[21];

	uint8_t pubkey[65];

	if(f_open(&fp_wlt, wlt_filename, FA_READ)) return 0;


	sprintf(fn, "armory_%s.hardened.SIGNED.tx", uniqueid);
	armtx_push_base64blocks_to_file_init(&ctx, fn, uniqueid);
	//"=====TXSIGCOLLECT-12345678======================================"

	BYT sig_bp;
	uint8_t sigdata[0x49], privkey[32];
	b_open(&sig_bp, sigdata, sizeof(sigdata));

	BYT vintbuff_bp;
	uint8_t vintbuff[3];
	b_open(&vintbuff_bp, vintbuff, sizeof(vintbuff));

	armtx_push_base64blocks_to_file_update(&ctx, b_addr(armtx_bp), 12);

	txin_cnt = armtx_get_txin_cnt(armtx_bp);
	b_putvint(&vintbuff_bp, txin_cnt);

	armtx_push_base64blocks_to_file_update(&ctx, b_addr(&vintbuff_bp), b_reopen(&vintbuff_bp)); // push txin cnt vint to file


	for(uint16_t i = 0; i < txin_cnt; i++) {

		#ifdef CONF_SSD1306_H_INCLUDED
		gfx_mono_draw_filled_rect(0, 11, 128, 7, 0);
		sprintf(q, "Signing TxIn %u/%u...", i+1, txin_cnt);
		gfx_mono_draw_string(q,	0, 11, &sysfont);
		#endif

		armtx_get_txin_pubkey(armtx_bp, i, pubkey);
		if(!armwl_get_privkey_from_pubkey(wallet, &fp_wlt, pubkey, privkey)) {
			memset(privkey, 0, 32);
			f_close(&fp_wlt);
			f_close(&ctx.fp);
			f_unlink(fn);
			return 0;
		}

		// create vint with new length of txin (old len + sig len).
		// This works without possible extra bytes because the siglen byte is set as 0x00 before and max 0x49 after.
		// The sig itself is meanwhile built and kept in sig_bp
		b_putvint(&vintbuff_bp, armtx_get_txin_len(armtx_bp, i) + armtx_build_txin_sig(armtx_bp, i, privkey, &sig_bp));
		memset(privkey, 0, 32);

		armtx_push_base64blocks_to_file_update(&ctx, b_addr(&vintbuff_bp), b_reopen(&vintbuff_bp)); // push new txin len as vint to file

		// copy txin data beginning from version to pubkey. we cheat a bit here and expect just 2 more bytes to follow (2*0x00 for siglist len and loclist len)
		armtx_push_base64blocks_to_file_update(&ctx, b_addr(armtx_bp) + \
		armtx_get_txin_data_pos(armtx_bp, i), \
		armtx_get_txin_len(armtx_bp, i) - 2);

		{
			b_putvint(&vintbuff_bp, b_size(&sig_bp)); // make vint from sig len
			armtx_push_base64blocks_to_file_update(&ctx, b_addr(&vintbuff_bp), b_reopen(&vintbuff_bp)); // push sig len as vint to file
			armtx_push_base64blocks_to_file_update(&ctx, b_addr(&sig_bp), b_size(&sig_bp)); // push sig to file
		}

		b_putvint(&vintbuff_bp, 0x00); // make loclist
		armtx_push_base64blocks_to_file_update(&ctx, b_addr(&vintbuff_bp), b_reopen(&vintbuff_bp)); // push loclist len to file
	}

	armtx_push_base64blocks_to_file_update(&ctx, b_addr(armtx_bp) + \
	armtx_get_txout_cnt_pos(armtx_bp), b_size(armtx_bp) - armtx_get_txout_cnt_pos(armtx_bp));

	armtx_push_base64blocks_to_file_final(&ctx);
	f_close(&fp_wlt);
	return 1;
}

uint8_t armtx_get_latest_unsigned_file(char *filename)
{
	uint32_t fdatetime_latest = 0;
	uint32_t fdatetime_current = 0;
	//uint16_t fdate = 0;
	char fname[_MAX_LFN + 1] = "\0";

	FRESULT res;
	FILINFO fno;
	DIR dir;
	// int i;
	char *fn;
	#if _USE_LFN
	static char lfn[_MAX_LFN + 1];   /// Buffer to store the LFN
	fno.lfname = lfn;
	fno.lfsize = sizeof lfn;
	#endif


	res = f_opendir(&dir, "/");                       // Open the directory
	if (res == FR_OK) {
		//i = strlen(path);
		for (;;) {
			res = f_readdir(&dir, &fno);                   // Read a directory item
			if (res != FR_OK || fno.fname[0] == 0) break;  // Break on error or end of dir
			if (fno.fname[0] == '.') continue;             // Ignore dot entry
			#if _USE_LFN
			fn = *fno.lfname ? fno.lfname : fno.fname;
			#else
			fn = fno.fname;
			#endif
			if(strstr(fn, ".unsigned.tx")) {

				fdatetime_current = ((uint32_t) fno.fdate << 16) | fno.ftime;

				if(fdatetime_current > fdatetime_latest) {
					fdatetime_latest = fdatetime_current;
					strcpy(fname, fn);
				}
			}
		}


		if(fname[0] != '\0') {
			strcpy(filename, fname);
			return 1;
		}


	}

	return 0;
}