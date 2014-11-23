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

#define EEP_VERSION (void *) 0
#define EEP_WLTCNT (void *) 2
#define EEP_WLTACT (void *) 4
#define armwlt_get_eep_walletpos(i) (void *) (6 + i * sizeof(WLT_EEP))
#define armwlt_get_eep_privkeypos(i) (void *) (6 + i * sizeof(WLT_EEP) + 11)

#include <string.h>
#include <avr/io.h>

#include <asf.h>

#include "utils.h"
#include "bitcoin.h"
#include "sha2.h"
#include "hmac.h"
#include "ecc.h"
#include "bytestream.h"


#ifndef ARMORY_H_
#define ARMORY_H_


typedef struct armoryReducedAddr1 {
	uint8_t hash160[20];
	uint8_t hash160_chksum[4];
	uint8_t privkey[32];
	uint8_t privkey_chksum[4];
	uint8_t pubkey[65];
	uint8_t pubkey_chksum[4];
	} WLT_ADDR;

typedef struct armoryReducedWallet2 {
	uint8_t version[4];
	uint8_t magicBytes[4];
	uint8_t WltFlags[8];
	uint8_t createDate[8];
	uint8_t uniqueid[6];
	uint8_t labelName[32];
	uint32_t highestUsedChainIndex;
	//WLT_ADDR rootaddr;
} WLT_FIL;

typedef struct EEPROMWalletData {
	uint8_t flags;
	uint8_t uniqueid[6];
	uint8_t uniqueid_cs[4];
	uint8_t privkey[32];
	uint8_t privkey_cs[4];
} WLT_EEP;

typedef struct ArmoryWalletInstance2 {
	uint16_t id;
	uint8_t uniqueid[6];
	uint8_t rootkey[32];
	uint8_t chaincode[32];
	uint8_t rootpubkey[65];
	uint8_t rootpubkeyhash[20];
	uint8_t addrprivkey[32];
	uint8_t addrpubkey[65];
	uint8_t addrpubkeyhash[20];
} WLT;

#define WLT_COMPUTE_UNIQUEID	1
#define WLT_COMPUTE_FIRST_ADDR	(1 << 1)
#define WLT_DELETE_ROOTKEY		(1 << 2)
#define WLT_DELETE_ADDRPRIVKEY	(1 << 3)



void compute_chained_privkey(uint8_t *privkey, uint8_t *chaincode, uint8_t *pubkey, uint8_t *next_privkey);
void armwlt_compute_chained_pubkey(uint8_t *pubkey, uint8_t *chaincode, uint8_t *next_pubkey);


uint8_t armwl_get_privkey_from_pubkey(const WLT *wallet, FIL *fp, const uint8_t *pubkey, uint8_t *privkey);

uint8_t safe_extend_privkey(uint8_t *privkey, uint8_t *chaincode, uint8_t *pubkey, uint8_t *next_privkey, uint8_t *next_privkey_checksum);
uint8_t safe_extend_pubkey(uint8_t *privkey, uint8_t *pubkey, uint8_t *pubkey_checksum, uint8_t *pubkeyhash, uint8_t *pubkeyhash_checksum);

void derive_chaincode_from_rootkey(uint8_t *privkey, uint8_t *chaincode);

//uint8_t create_wallet_from_rootkey(uint8_t rootkey[32], uint8_t checksum[4], armory_wallet *wallet, armory_addr *addrlist);
uint8_t get_wallet_mapblock(FIL *fp, uint16_t blockstart, uint32_t *ci_map);

uint8_t armwlt_create_instance(uint16_t walletid, WLT *wallet);
uint8_t armwlt_create_instance2(WLT *wallet, const uint8_t options);

//uint8_t safe_extend_addrlist(armory_wallet *wallet, armory_addr *addrlist, const uint8_t num);

uint8_t armwlt_read_rootkey_file(const char *filename, uint8_t *rootkey);
uint8_t armwlt_read_wallet_file(const char *filename, uint8_t *rootkey);
uint8_t armwlt_build_rootpubkey_file(const WLT *wallet);
void armwlt_make_rootkey_file(const uint8_t *rootkey);

uint16_t armwlt_eep_get_first_free_slot(void);
uint8_t armwlt_eep_read_wallet(WLT *wallet);
uint8_t armwlt_eep_create_wallet(const uint8_t *rootkey, const uint8_t *uniqueid);
uint8_t armwlt_eep_erase_wallet(const uint16_t walletid);

//uint8_t armwlt_lookup_pubkeyhash(WLT *wallet, uint8_t *pubkeyhash);

#endif /* ARMORY_H_ */