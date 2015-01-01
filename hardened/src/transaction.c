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
	*(uniqueid + 8) = '\0'; // terminate id string
}

uint16_t armtx_get_txin_cnt(BYT *armtx_bp)
{
	b_seek(armtx_bp, 4 + 4 + 4); // start at 0, skip version, mbyte, locktime
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

static size_t armtx_get_txin_inputvalue_rebuff(BYT *bp, size_t ofs)
{
	typedef struct {
		BYT *armtx_bp;
		uint16_t txin;
	} ctx;

	ctx *supptx_ctx;
	supptx_ctx = (*bp).inh_ctx;

	size_t len = armtx_get_txin_supptx_len(supptx_ctx->armtx_bp, supptx_ctx->txin);
	size_t btc = len - ofs < bp->res ? len - ofs : bp->res;

	b_seek( supptx_ctx->armtx_bp, b_tell(supptx_ctx->armtx_bp) + ofs);
	//b_rewind(bp);
	//b_copy(supptx_ctx->armtx_bp, bp, btc);
	b_read(supptx_ctx->armtx_bp, b_addr(bp), btc);

	return btc;
}

uint64_t armtx_get_txin_inputvalue(BYT *armtx_bp, const uint16_t txin)
{
	uint64_t val;
	uint32_t i = armtx_get_txin_previndex(armtx_bp, txin);

	BYT supptx_bp;
	uint8_t supptx[400];
	//void *supptx_addr;

	struct {
		BYT *armtx_bp;
		uint16_t txin;
	} supptx_ctx = {armtx_bp, txin};

	//armtx_get_txin_supptx_len(armtx_bp, txin); // goto supptx (moves over the len varint)
	//supptx_ctx.txin = txin;//b_tell(armtx_bp);
	//supptx_ctx.armtx_bp = armtx_bp;
	//ctx.armtx_bp = armtx_bp;
	//ctx.ofs =

	b_open(&supptx_bp, supptx, sizeof(supptx));
	b_setbufin(&supptx_bp, &armtx_get_txin_inputvalue_rebuff, &supptx_ctx);

	//armtx_get_txin_supptx_len(armtx_bp, txin); // goto supptx (moves over the len varint)
	//b_setrewofs(armtx_bp); // limit the workspace for the tx handlers
	//val = tx_get_txout_value(armtx_bp, i);
	val = tx_get_txout_value(&supptx_bp, i);
	//b_unsetrewofs(armtx_bp); // get back full workspace for following ops
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
	b_seek(tx_bp, 4); // start at 0, skip ver
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
	//static volatile uint8_t veri = 0;

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

uint8_t armtx_insert_sigs(BYT *u_armtx_bp, BYT *s_armtx_bp, void *pub2privkey_wrp)
{
	size_t txin_cnt;
	uint8_t pubkey[65], privkey[32];
	char q[21];
	uint8_t (*pub2privkey_wrp_ptr)(const uint8_t *pubkey, uint8_t *privkey);

	pub2privkey_wrp_ptr = pub2privkey_wrp;

	BYT sig_bp;
	uint8_t sigdata[0x49]; // max. P2PKH sig len is 73 (0x49) bytes
	b_open(&sig_bp, sigdata, sizeof(sigdata)); // open sigdata buffer

	// copy first 12 bytes:
	b_rewind(u_armtx_bp);
	b_copy(u_armtx_bp, s_armtx_bp, 12);

	txin_cnt = armtx_get_txin_cnt(u_armtx_bp);
	b_putvint(s_armtx_bp, txin_cnt); // push txin cnt vint to file

	for(uint16_t i = 0; i < txin_cnt; i++) {

		#ifdef CONF_SSD1306_H_INCLUDED
		gfx_mono_draw_filled_rect(0, 11, 128, 7, 0);
		sprintf(q, "Signing TxIn %u/%u...", i+1, txin_cnt);
		gfx_mono_draw_string(q,	0, 11, &sysfont);
		#endif

		// get the corresponding pubkey for txin and compute privkey; stop if unsuccessful:
		armtx_get_txin_pubkey(u_armtx_bp, i, pubkey);
		if(!pub2privkey_wrp_ptr(pubkey, privkey)) {
			memset(privkey, 0, 32);
			return 0;
		}

		// create vint with new length of txin (old len + sig len) and push to file.
		// This works without extra bytes because the siglen byte is set as 0x00 before and max 0x49 after.
		// The sig itself is meanwhile built and kept in sig_bp
		b_putvint(s_armtx_bp, armtx_get_txin_len(u_armtx_bp, i) + armtx_build_txin_sig(u_armtx_bp, i, privkey, &sig_bp));
		memset(privkey, 0, 32); // erase privkey

		// copy txin data from version to pubkey. we cheat a bit here and expect just 2 more bytes to follow (2*0x00 for siglist len and loclist len).
		// by getting the txin length as an arg for b_copy we also set the pointer of armtx_bp to the beginning of the txin data for the copying process
		b_copy(u_armtx_bp, s_armtx_bp, armtx_get_txin_len(u_armtx_bp, i) - 2);

		b_putvint(s_armtx_bp, b_size(&sig_bp)); // make vint from sig len and push to file
		b_write(s_armtx_bp, b_addr(&sig_bp), b_size(&sig_bp)); // push sig to file

		b_putvint(s_armtx_bp, 0x00); // make loclist and push to file
	}

	// copy outputs:
	b_copy(u_armtx_bp, s_armtx_bp, b_srcsize(u_armtx_bp) - armtx_get_txout_cnt_pos(u_armtx_bp)); // trick like with txin data above
	b_flush(s_armtx_bp); // push what's left to the file

	return 1;
}

/*
*
* Tx file operation functions:
*
*/
void txfile_build_signed_file_snkwrp(BYT *bp)
{
	char buff_b64[64+1];
	size_t s = bp->size;

	b_rewind(bp);

	while(s > 48) {
		base64enc(buff_b64, b_posaddr(bp), 48);

		f_puts(buff_b64, bp->outh_ctx);
		f_puts("\r\n", bp->outh_ctx);

		b_seek(bp, b_tell(bp) + 48);
		s -= 48;
	}

	base64enc(buff_b64, b_posaddr(bp), s);
	f_puts(buff_b64, bp->outh_ctx);
	f_puts("\r\n", bp->outh_ctx);
}

uint8_t txfile_build_signed_file_pub2privkeywrp(const uint8_t *pubkey, uint8_t *privkey)
{
	char b58uniqueid[10];
	char wlt_filename[100];

	extern WLT ui_wallet;

	base58_encode(ui_wallet.uniqueid, 6, b58uniqueid);

	if(!scan_for_file(".watchonly.wallet", b58uniqueid, wlt_filename)) return 0;
	if(!armwlt_get_privkey_from_pubkey(&ui_wallet, wlt_filename, pubkey, privkey)) return 0;

	return 1;
}

uint8_t txfile_build_signed_file(BYT *u_armtx_bp)
{
	FIL fp_tx;
	char fn[100], uniqueid[9];

	BYT s_armtx_bp;
	uint8_t s_armtx_buff[48*4]; // must be multiple of 3 (for base64) and should be multiple of 48 because of line break in Armory tx files after 64 bytes

	// open signed tx buffer with outgoing handler:
	b_open(&s_armtx_bp, s_armtx_buff, sizeof(s_armtx_buff));
	b_setbufout(&s_armtx_bp, &txfile_build_signed_file_snkwrp, &fp_tx);

	// compute Armory's unique tx id:
	armtx_get_uniqueid(u_armtx_bp, uniqueid);

	sprintf(fn, "armory_%s.hardened.SIGNED.tx", uniqueid);
	f_open(&fp_tx, fn, FA_READ | FA_WRITE | FA_CREATE_ALWAYS); // create signed tx file

	// signed tx file header:
	f_puts("=====TXSIGCOLLECT-", &fp_tx);
	f_puts(uniqueid, &fp_tx);
	f_puts("======================================\r\n", &fp_tx);

	if(!armtx_insert_sigs(u_armtx_bp, &s_armtx_bp, &txfile_build_signed_file_pub2privkeywrp)){
		f_close(&fp_tx);
		f_unlink(fn);
		return 0;
	}

	f_puts("================================================================\r\n", &fp_tx); // tx file footer
	f_close(&fp_tx);
	return 1;
}

size_t txfile_fetch_unsigned_file_srcwrp(BYT *bp, size_t ofs)
{
	FIL fp;
	UINT br;
	//memcpy(bp->bs, bp->inh_ctx + ofs, bp->res);

	//size_t len = armtx_get_txin_supptx_len(supptx_ctx->armtx_bp, supptx_ctx->txin);
	size_t btc = bp->srclen - ofs < bp->res ? bp->srclen - ofs : bp->res;

	f_open(&fp, "workspace.bin.temp", FA_READ);
	f_lseek(&fp, ofs);
	f_read(&fp, b_addr(bp), (UINT) btc, &br);

	//size_t rem = ui_test_bp.size - ofs;

	//bp->srclen = ui_test_bp.size;

	//if(bp->res > rem) {
	//	memcpy(bp->bs, ui_test_bp.bs + ofs, rem);
	//	return rem;
	//}

	//memcpy(bp->bs, ui_test_bp.bs + ofs, bp->res);
	//return bp->res;

	f_close(&fp);

	return (size_t) br;
}


uint8_t txfile_fetch_unsigned_file(char *fn, char *uniqueid, BYT *armtx_bp)
{
	uint8_t oversize = 0;
	UINT br;
	FIL fp_b64, fp_bin;
	//char fn_bin[100];

	char armtx_rawbuff[100];//[67];
	uint8_t armtx_binbuff[100];//[46];

	b_reopen(armtx_bp);
	f_open(&fp_b64, fn, FA_READ);

	if(f_size(&fp_b64)*3/4-2 > b_ressize(armtx_bp)) { // taking whole file size to safely avoid buffer overflow
		oversize = 1;
		//sprintf(fn_bin, "%s.bin.temp", fn);
		//strcpy(fn, fn_bin);
		f_open(&fp_bin, "workspace.bin.temp", FA_READ | FA_WRITE | FA_CREATE_ALWAYS);
	}

	f_gets((TCHAR *) armtx_rawbuff, 67, &fp_b64);
	//memset(uniqueid, 0, 9);
	//memcpy(uniqueid, armtx_rawbuff + 18, 8);

	while(!f_eof(&fp_b64)) {
		f_gets((TCHAR *) armtx_rawbuff, 67, &fp_b64);
		if(armtx_rawbuff[0] != '=') {
			base64dec(armtx_binbuff, armtx_rawbuff, 0);
			if(oversize) {
				f_write(&fp_bin, armtx_binbuff, base64_binlength(armtx_rawbuff, 0), &br);
			} else {
				b_write(armtx_bp, armtx_binbuff, base64_binlength(armtx_rawbuff, 0));
			}
		}
	}
	f_close(&fp_b64);

	if(oversize) {
		armtx_bp->srclen = f_size(&fp_bin);
		f_close(&fp_bin);
		b_setbufin(armtx_bp, &txfile_fetch_unsigned_file_srcwrp, NULL);
	}

	armtx_get_uniqueid(armtx_bp, uniqueid);
	return 1;
}