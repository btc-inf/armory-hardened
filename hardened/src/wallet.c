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

#include <avr/eeprom.h>
#include "wallet.h"

void armwlt_make_rootkey_file(const uint8_t *rootkey)
{
	return;

	FIL fp;
	char easy16buff[45];
	//sprintf(filename, "armory_.decryt.rootkey.txt", b58uniqueid);
	f_open(&fp, "armory_.decryt.rootkey.txt", FA_WRITE | FA_CREATE_ALWAYS);

	makeSixteenBytesEasy(rootkey, easy16buff);
	f_puts(easy16buff, &fp);
	f_puts("\r\n", &fp);

	makeSixteenBytesEasy(rootkey + 16, easy16buff);
	f_puts(easy16buff, &fp);

	f_close(&fp);
}

uint8_t armwlt_build_rootpubkey_file(const WLT *wallet)
{
	char b58uniqueid[10];
	char filename[50];
	FIL fp;

	uint8_t rootid[20];
	char easy16buff[45];

	// Wallet public data backup rootid is non-compatible to normal 16 easy bytes, so we do it manually:
	rootid[0] = 0x01; // version
	rootid[0] |= (wallet->rootpubkey[64] & 0x01) ? 0x80 : 0x00; // pubkey y-coord sign encoding
	memcpy(&rootid[1], wallet->uniqueid, 6); // wallet uniqueid
	compute_checksum(rootid, 7, rootid + 7); // checksum of first byte and uniqueid
	binary_to_easyType16(rootid, 9, easy16buff); // make 9 easy bytes rootid

	base58_encode(wallet->uniqueid, 6, b58uniqueid);
	sprintf(filename, "armory_%s.hardened.rootpubkey", b58uniqueid);
	f_open(&fp, filename, FA_WRITE | FA_CREATE_ALWAYS); // always create new

	// push "1" (Armory wants this) and rootid:
	f_puts("1\n", &fp);
	f_puts(easy16buff, &fp);
	f_puts("\n", &fp);

	// push pubkey x-coord:
	makeSixteenBytesEasy(wallet->rootpubkey + 1, easy16buff);
	f_puts(easy16buff, &fp);
	f_puts("\n", &fp);

	makeSixteenBytesEasy(wallet->rootpubkey + 17, easy16buff);
	f_puts(easy16buff, &fp);
	f_puts("\n", &fp);

	// push chaincode:
	makeSixteenBytesEasy(wallet->chaincode, easy16buff);
	f_puts(easy16buff, &fp);
	f_puts("\n", &fp);

	makeSixteenBytesEasy(wallet->chaincode + 16, easy16buff);
	f_puts(easy16buff, &fp);

	f_close(&fp);
	return 1;
}


uint8_t armwlt_eep_get_actwlt(void)
{
	uint8_t actwlt = eeprom_read_byte(EEP_ACTWLT);
	if(actwlt == 0xFF) return 0;
	return actwlt;
}
uint8_t armwlt_eep_get_wltnum(void)
{
	uint8_t wltnum = eeprom_read_byte(EEP_WLTNUM);
	if(wltnum == 0xFF) return 0;
	return wltnum;
}

uint8_t armwlt_eep_get_next_wltid(uint8_t wltid)
{
	//uint16_t i = 0;
	//uint16_t wltid = 0;
	WLT_EEP eepwlt;
	//uint8_t wltnum = eeprom_read_byte(EEP_WLTNUM);

	if(!wltid || wltid > EEP_WLTNUMMAX || !armwlt_eep_get_wltnum()) return 0;

	do
	{
		eeprom_read_block(&eepwlt, armwlt_get_eep_walletpos(wltid), sizeof(WLT_EEP));
		if(eepwlt.set == WLT_EEP_SETBYTE) return wltid;
	} while (++wltid < EEP_WLTNUMMAX);

	return armwlt_eep_get_next_wltid(1);
}
uint8_t armwlt_eep_get_prev_slot(uint8_t slot)
{
	//uint16_t i = 0;
	//uint16_t wltid = 0;
	WLT_EEP eepwlt;
	//uint8_t wltnum = eeprom_read_byte(EEP_WLTNUM);

	if(!slot || slot > EEP_WLTNUMMAX || !armwlt_eep_get_wltnum()) return 0;

	do
	{
		eeprom_read_block(&eepwlt, armwlt_get_eep_walletpos(slot), sizeof(WLT_EEP));
		if(eepwlt.set == WLT_EEP_SETBYTE) return slot;
	} while (--slot < EEP_WLTNUMMAX);

	return 0;//armwlt_eep_get_next_wltid(1);
}


uint8_t armwlt_eep_get_next_free_wltid(uint8_t wltid)
{
	//uint16_t i = 0;
	//uint16_t wltid = 0;
	WLT_EEP eepwlt;
	//uint16_t wltcnt = eeprom_read_byte(EEP_WLTNUM);

	if(!wltid || wltid >= EEP_WLTNUMMAX || armwlt_eep_get_wltnum() >= EEP_WLTNUMMAX) return 0;

	do
	{
		eeprom_read_block(&eepwlt, armwlt_get_eep_walletpos(wltid), sizeof(WLT_EEP));
		if(eepwlt.set != WLT_EEP_SETBYTE) return wltid;
	} while (++wltid < EEP_WLTNUMMAX);

	return armwlt_eep_get_next_free_wltid(1);
}

uint8_t armwlt_eep_create_wallet(const uint8_t *rootkey, const uint8_t *uniqueid, const uint8_t setact)
{
	WLT_EEP eepwlt, eepwlt_chk;
	uint8_t wltid = armwlt_eep_get_next_free_wltid(1);
	uint8_t wltnum = armwlt_eep_get_wltnum();

	if(wltnum >= EEP_WLTNUMMAX) return 0;

	compute_checksum(rootkey, 32, eepwlt.privkey_cs);
	//compute_checksum(uniqueid, 6, eepwlt.uniqueid_cs);
	memcpy(eepwlt.privkey, rootkey, 32);
	memcpy(eepwlt.uniqueid, uniqueid, 6);
	eepwlt.set = WLT_EEP_SETBYTE;
	eepwlt.flags = 0;

	eeprom_update_block(&eepwlt, armwlt_get_eep_walletpos(wltid), sizeof(WLT_EEP));
	eeprom_read_block(&eepwlt_chk, armwlt_get_eep_walletpos(wltid), sizeof(WLT_EEP));

	// we do not do more verifying than checking eeprom content against data prepared to store before;
	// as long as we did not create the rootkey on-chip (not supported yet), previous corruption will not affect safety
	if(memcmp(&eepwlt, &eepwlt_chk, sizeof(WLT_EEP))) {
		memset(&eepwlt, 0, sizeof(WLT_EEP));
		memset(&eepwlt_chk, 0, sizeof(WLT_EEP));
		return 0; // eeprom corrupted
	}

	eeprom_update_byte(EEP_WLTNUM, wltnum+1);
	if(setact) {
		eeprom_update_byte(EEP_ACTWLT, wltid);
	}

	memset(&eepwlt, 0, sizeof(WLT_EEP));
	memset(&eepwlt_chk, 0, sizeof(WLT_EEP));
	return wltid;
}

uint8_t armwlt_eep_erase_wallet(const uint8_t wltid)
{
	WLT_EEP eepwlt, eepwlt_chk;

	if(!wltid) return 0;

	memset(&eepwlt, 0xFF, sizeof(WLT_EEP));

	eeprom_update_block(&eepwlt, armwlt_get_eep_walletpos(wltid), sizeof(WLT_EEP));
	eeprom_read_block(&eepwlt_chk, armwlt_get_eep_walletpos(wltid), sizeof(WLT_EEP));

	if(memcmp(&eepwlt, &eepwlt_chk, sizeof(WLT_EEP))) {
		memset(&eepwlt_chk, 0, sizeof(WLT_EEP));
		return 0; // eeprom corrupted
	}

	eeprom_update_byte(EEP_WLTNUM, armwlt_eep_get_wltnum()-1);
	eeprom_update_byte(EEP_ACTWLT, armwlt_eep_get_next_wltid(wltid));
	return 1;
}

uint8_t armwlt_eep_read_wallet(WLT *wallet)
{
	WLT_EEP eepwlt;
	uint8_t cs_verify;

	eeprom_busy_wait();
	eeprom_read_block(&eepwlt, armwlt_get_eep_walletpos(wallet->id), sizeof(WLT_EEP));

	//if(!(cs_verify = fix_verify_checksum(eepwlt.uniqueid, 6, eepwlt.uniqueid_cs))) return 0;

	//if(cs_verify == 2) {
	//	eeprom_busy_wait();
	//	eeprom_update_block(&eepwlt, armwlt_get_eep_walletpos(wallet->id), sizeof(WLT_EEP));
	//}

	if(!(cs_verify = fix_verify_checksum(eepwlt.privkey, 32, eepwlt.privkey_cs))) return 0;

	if(cs_verify == 2) {
		eeprom_busy_wait();
		eeprom_update_block(&eepwlt, armwlt_get_eep_walletpos(wallet->id), sizeof(WLT_EEP));
	}

	memcpy(wallet->uniqueid, eepwlt.uniqueid, 6);
	memcpy(wallet->rootkey, eepwlt.privkey, 32);

	memset(&eepwlt, 0, sizeof(WLT_EEP));
	return 1;
}

uint8_t armwlt_create_instance(WLT *wallet, const uint8_t options)
{
	derive_chaincode_from_rootkey(wallet->rootkey, wallet->chaincode);
	privkey_to_pubkey(wallet->rootkey, wallet->rootpubkey);
	pubkey_to_pubkeyhash(wallet->rootpubkey, wallet->rootpubkeyhash);

	if(options & (WLT_COMPUTE_FIRST_ADDR | WLT_COMPUTE_UNIQUEID)) {
		compute_chained_privkey(wallet->rootkey, wallet->chaincode, wallet->rootpubkey, wallet->addrprivkey);
		privkey_to_pubkey(wallet->addrprivkey, wallet->addrpubkey);
		pubkey_to_pubkeyhash(wallet->addrpubkey, wallet->addrpubkeyhash);

		if(options & WLT_COMPUTE_UNIQUEID) {
			wallet->uniqueid[0] = wallet->addrpubkeyhash[4];
			wallet->uniqueid[1] = wallet->addrpubkeyhash[3];
			wallet->uniqueid[2] = wallet->addrpubkeyhash[2];
			wallet->uniqueid[3] = wallet->addrpubkeyhash[1];
			wallet->uniqueid[4] = wallet->addrpubkeyhash[0];
			wallet->uniqueid[5] = 0x00;
		}

		if(options & WLT_DELETE_ADDRPRIVKEY) {
			memset(wallet->addrprivkey, 0, 32);
		}
	}

	if(options & WLT_DELETE_ROOTKEY) {
		memset(wallet->rootkey, 0, 32);
	}

	return 1;
}

uint8_t armwlt_read_wallet_file(const char *filename, uint8_t *rootkey)
{
	FIL fp;
	uint8_t buff[36];
	UINT br;

	if(f_open(&fp, filename, FA_READ)) return 0;
	f_lseek(&fp, 954);
	if(f_read(&fp, buff, 36, &br)) return 0;
	f_close(&fp);

	if(!verify_checksum(buff, 32, buff + 32)) {
		memset(buff, 0, 36);
		return 0;
	}

	memcpy(rootkey, buff, 32);
	memset(buff, 0, 36);
	return 1;
}

uint8_t armwlt_read_rootkey_file(const char *filename, uint8_t *rootkey)
{
	FIL fp;
	char easy16buff[100];
	UINT br;

	if(f_open(&fp, filename, FA_READ)) return 0;
	if(f_read(&fp, easy16buff, 100, &br)) return 0;
	f_close(&fp);
	easy16buff[br] = '\0';

	if(!readSixteenEasyBytes(easy16buff + readSixteenEasyBytes(easy16buff, rootkey), rootkey + 16)) {
		memset(easy16buff, 0, sizeof(easy16buff));
		memset(rootkey, 0, 32);
		return 0;
	}

	memset(easy16buff, 0, sizeof(easy16buff));
	return 1;
}
uint8_t armwlt_read_shuffrootkey_file(const char *filename, const char *code, uint8_t *rootkey)
{
	FIL fp;
	char easy16buff[100];
	UINT br;
	static volatile uint8_t i, j;

	if(f_open(&fp, filename, FA_READ)) return 0;
	if(f_read(&fp, easy16buff, 100, &br)) return 0;
	f_close(&fp);
	easy16buff[br] = '\0';

	const char transl[] = "0123456789ABCDEF";

	for(i = 0; i < br; i++) {
		for(j = 0; j < 16; j++) {
			if(easy16buff[i] == transl[j]) {
				easy16buff[i] = code[j];
			}
		}
	}

	if(!readSixteenEasyBytes(easy16buff + readSixteenEasyBytes(easy16buff, rootkey), rootkey + 16)) {
		memset(easy16buff, 0, sizeof(easy16buff));
		memset(rootkey, 0, 32);
		return 0;
	}

	memset(easy16buff, 0, sizeof(easy16buff));
	return 1;
}


uint8_t armwlt_get_privkey_from_pubkey(const WLT *wallet, /*FIL *fp*/const char *wltfn, const uint8_t *pubkey, uint8_t *privkey)
{
	uint8_t pubkeyhash[20], pubkeyhash_buf[20], pubkey_buf[65];
	uint32_t ci_map[50];
	uint16_t ci_map_offset = 0;
	//uint8_t chaincode[32];

	//FRESULT fr;
	FIL fp;
	//char filename[100];
	uint32_t br;

	pubkey_to_pubkeyhash(pubkey, pubkeyhash);

	//eeprom_busy_wait();
	//eeprom_read_block(wallet->rootkey, armwlt_get_eep_privkeypos(wallet->id), 32);

	//derive_chaincode_from_rootkey(wallet->privk, chaincode);
	compute_chained_privkey(wallet->rootkey, wallet->chaincode, wallet->rootpubkey, privkey);

	//memset(wallet->rootkey, 0, 32);

	f_open(&fp, wltfn, FA_READ);

	uint8_t eom;
	do {

		eom = get_wallet_mapblock(&fp, ci_map_offset, ci_map);

		for(uint8_t i = 0; i < eom; i++) {
			f_lseek (&fp, ci_map[i]);
			f_read(&fp, (void *) pubkeyhash_buf, 20, (UINT*) &br);
			if(memcmp(pubkeyhash, pubkeyhash_buf, 20) == 0) {

				return 1;
				} else {
				f_lseek (&fp, (DWORD) f_tell(&fp) + 144);
				f_read(&fp, (void *) pubkey_buf, 65, (UINT*) &br);

				compute_chained_privkey(privkey, wallet->chaincode, pubkey_buf, privkey);
			}
		}

		if(memcmp(pubkeyhash, pubkeyhash_buf, 20) == 0) {
			return 1;
		}

		ci_map_offset += 50;

	} while (eom != 0 && !f_eof(&fp));

	f_close(&fp);
	return 0;
}

uint8_t get_wallet_mapblock(FIL *fp, uint16_t blockstart, uint32_t *ci_map)
{
	uint8_t entrytype, i;
	static volatile uint16_t ci, N;
	uint32_t pos, br;

	f_lseek(fp, 2107);
	i = 0;
	while(i < 50 && /*f_tell(fp) < f_size(fp)*/!f_eof(fp)) {

		f_read(fp, (void *) &entrytype, 1, (UINT*) &br);

		if(entrytype == 0x00) {

			pos = f_tell(fp);
			f_lseek (fp, (DWORD) pos + 92);
			f_read(fp, (void *) &ci, 2, (UINT*) &br);
			if(ci >= blockstart && ci < blockstart + 50) {
				ci_map[ci - blockstart] = pos;
				i++;
			}
			f_lseek (fp, (DWORD) f_tell(fp) + 163);

			} else if(entrytype == 0x01) {

			f_lseek (fp, (DWORD) f_tell(fp) + 20);
			f_read(fp, (void *) &N, 2, (UINT*) &br);
			f_lseek (fp, (DWORD) f_tell(fp) + N);

			} else if(entrytype == 0x02) {

			f_lseek (fp, (DWORD) f_tell(fp) + 32);
			f_read(fp, (void *) &N, 2, (UINT*) &br);
			f_lseek (fp, (DWORD) f_tell(fp) + N);

			} else if(entrytype == 0x04) {

			f_read(fp, (void *) &N, 2, (UINT*) &br);
			f_lseek (fp, (DWORD) f_tell(fp) + N);

			} else {

			break;

		}
	}

	return i;
}


void compute_chained_privkey(const uint8_t *privkey, const uint8_t *chaincode, const uint8_t *pubkey, uint8_t *next_privkey)
{
	extern uint8_t curve_n[32];
	uint8_t chainMod[32], chainXor[32], temp[32], temp2[32];

	hash256(pubkey, 65, chainMod);

	for(uint8_t i = 0; i <= sizeof(chainXor); i++) {

		chainXor[i] = chaincode[i] ^ chainMod[i];

	}

	vli_swap(chainXor, chainXor);
	vli_swap(privkey, temp2);
	vli_modMult(temp, chainXor, temp2, curve_n);
	vli_swap(temp, temp);

	memcpy(next_privkey, temp, 32);
}

void derive_chaincode_from_rootkey(uint8_t *rootkey, uint8_t *chaincode)
{
	const char msg[] = "Derive Chaincode from Root Key"; // 0x44657269766520436861696E636F64652066726F6D20526F6F74204B6579

	static uint8_t m[30];

	memcpy(m,msg,strlen(msg));

	uint8_t hash[32];
	hash256(rootkey, 32, hash);
	//vli_swap(hash,hash);
	//_NOP();

	hmac_sha256_armory_workaround(hash, 32, m, strlen(msg), chaincode);
}

/*
uint8_t create_wallet_from_rootkey(uint8_t rootkey[32], uint8_t checksum[4], armory_wallet *wallet, armory_addr *addrlist)
{
	uint8_t i = 0;
	armory_wallet tempw;
	armory_addr tempa;

	do {

		memcpy(tempw.rootaddr.privkey, rootkey, 32); // copy rootkey
		memcpy(tempw.rootaddr.privkey_chksum, checksum, 4); // copy rootkey checksum

	} while (!verify_checksum(tempw.rootaddr.privkey, 32, tempw.rootaddr.privkey_chksum) && i++ < MAX_WRITE_ITER);


	if(i > MAX_WRITE_ITER) { // copied rootkey and checksum still valid?

		memset(&tempw, 0x00, sizeof(armory_wallet));
		return 0;

	}


	i = 0;
	uint8_t cc_mirror[32];

	do {

		derive_chaincode_from_rootkey(rootkey, tempw.chaincode);
		derive_chaincode_from_rootkey(rootkey, cc_mirror); // derive chaincode 2x to detect bit errors

	} while (memcmp(tempw.chaincode, cc_mirror, 32) != 0 && i++ < MAX_WRITE_ITER);


	if(i > MAX_WRITE_ITER) { // copied chaincode and checksum still valid?

		memset(&tempw, 0x00, sizeof(armory_wallet));
		return 0;

	}

	compute_checksum(tempw.chaincode, 32, tempw.chaincode_chksum); // get cc checksum


	if(!safe_extend_pubkey(rootkey, tempw.rootaddr.pubkey, tempw.rootaddr.pubkey_chksum, tempw.rootaddr.hash160, tempw.rootaddr.hash160_chksum)) {

		memset(&tempw, 0x00, sizeof(armory_wallet));
		return 0;

	}


	 derive first actual privkey/pubkeyhash:
	if(!safe_extend_privkey(tempw.rootaddr.privkey, tempw.chaincode, tempw.rootaddr.pubkey, tempa.privkey, tempa.privkey_chksum)) {
		return 0;
	}

	if(!safe_extend_pubkey(tempa.privkey, tempa.pubkey, tempa.pubkey_chksum, tempa.hash160, tempa.hash160_chksum)) {
		return 0;
	}


	 now set wallet idstr from pubkeyhash of first usable addr:
	pubkeyhash_to_wallet_idstr(tempa.hash160, tempw.idstr);


	 no errors so far, push wallet data to target space:
	memcpy(tempw.sname, WALLET_CREATE_NAME, strlen(WALLET_CREATE_NAME)); // copy name
	i = 0;
	do {

		memcpy(wallet, &tempw, sizeof(armory_wallet)); // copy whole local wallet struct to global wallet

	} while (memcmp(wallet, &tempw, sizeof(armory_wallet)) != 0 && i++ < MAX_WRITE_ITER);

	memcpy(&addrlist[0], &tempa, sizeof(armory_addr)); // copy first actual keypair to global address list

	// create the first WALLET_CREATE_ADDRNUM - 1 keypairs of this wallet (one already exists from above)
	uint8_t created = safe_extend_addrlist(wallet, addrlist, WALLET_CREATE_ADDRNUM - 1); // returns number of successfully created keypairs

	// WRITE TO FLASH!!!
	return 1;
}
*/

/*
uint8_t safe_extend_addrlist(armory_wallet *wallet, armory_addr *addrlist, const uint8_t num)
{
	armory_addr tempa;
	uint32_t ci = wallet->chainindex; // chainindex
	uint8_t i = 0;

	for( ; i <= num; i++) {

		if(!safe_extend_privkey(addrlist[ci].privkey, wallet->chaincode, addrlist[ci].pubkey, tempa.privkey, tempa.privkey_chksum) ||
		!safe_extend_pubkey(tempa.privkey, tempa.pubkey, tempa.pubkey_chksum, tempa.hash160, tempa.hash160_chksum)) {
			break;
		}
		memcpy(&addrlist[++ci], &tempa, sizeof(armory_addr));
	}
	wallet->chainindex = ci;
	return i;
}


uint8_t safe_extend_privkey(uint8_t *privkey, uint8_t *chaincode, uint8_t *pubkey, uint8_t *next_privkey, uint8_t *next_privkey_checksum)
{
	uint8_t i = 0;
	uint8_t k_mirror[32];

	do {

		compute_chained_privkey(privkey, chaincode, pubkey, next_privkey);
		compute_chained_privkey(privkey, chaincode, pubkey, k_mirror);

	} while (memcmp(next_privkey, k_mirror, 32) != 0 && i++ < MAX_WRITE_ITER);

	if(i > MAX_WRITE_ITER) { return 0; }

	compute_checksum(next_privkey, 32, next_privkey_checksum);

	return 1;
}

uint8_t safe_extend_pubkey(uint8_t *privkey, uint8_t *pubkey, uint8_t *pubkey_checksum, uint8_t *pubkeyhash, uint8_t *pubkeyhash_checksum)
{
	uint8_t i = 0;
	uint8_t k_mirror[65], h_mirror[20];

	do {

		privkey_to_pubkey(privkey, pubkey);
		privkey_to_pubkey(privkey, k_mirror);

	} while (memcmp(pubkey, k_mirror, 65) != 0 && i++ < MAX_WRITE_ITER);

	if(i > MAX_WRITE_ITER) { return 0; }

	compute_checksum(pubkey, 65, pubkey_checksum);

	i = 0;
	do {

		pubkey_to_pubkeyhash(pubkey, pubkeyhash);
		pubkey_to_pubkeyhash(pubkey, h_mirror);

	} while (memcmp(pubkeyhash, h_mirror, 20) != 0 && i++ < MAX_WRITE_ITER);

	if(i > MAX_WRITE_ITER) { return 0; }

	compute_checksum(pubkeyhash, 20, pubkeyhash_checksum);

	return 1;
}
*/