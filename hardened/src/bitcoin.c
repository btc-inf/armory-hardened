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

#include "bitcoin.h"

void hash256(const uint8_t *data, size_t len, uint8_t *digest)
{
	sha256_Raw(data, len, digest);
	sha256_Raw(digest, SHA256_DIGEST_LENGTH, digest);
}

void hash160(const uint8_t *data, size_t len, uint8_t *digest)
{
	uint8_t hash[SHA256_DIGEST_LENGTH];
	sha256_Raw(data, len, hash);
	ripemd160(hash, SHA256_DIGEST_LENGTH, digest);
}

void compute_checksum(const uint8_t *data, size_t len, uint8_t checksum[4])
{
	uint8_t hash[SHA256_DIGEST_LENGTH];
	hash256(data, len, hash);
	memcpy(checksum, hash, 4);
}

uint8_t verify_checksum(const uint8_t *data, const size_t len, const uint8_t checksum[4])
{
	uint8_t cs[4];
	compute_checksum(data, len, cs);

	return (cs[0] == checksum[0] && \
			cs[1] == checksum[1] && \
			cs[2] == checksum[2] && \
			cs[3] == checksum[3]) ? 1 : 0;
}

uint8_t fix_verify_checksum(uint8_t *data, const size_t len, uint8_t checksum[4])
{
	uint16_t i, c;
	size_t pos = 0;

	if(!verify_checksum(data, len, checksum)) {
		// search data for error:
		while(pos < len) {
			c = data[pos];

			while(!verify_checksum(data, len, checksum) && i <= 0xFF) {
				data[pos] = i++;
			}

			if(verify_checksum(data, len, checksum)) return 2; // fixed successfully
			data[pos++] = c;
			i = 0;
		}
		// search checksum for error:
		pos = 0;
		while(pos < 4) {
			c = checksum[pos];

			while(!verify_checksum(data, len, checksum) && i <= 0xFF) {
				checksum[pos] = i++;
			}

			if(verify_checksum(data, len, checksum)) return 2; // fixed successfully
			checksum[pos++] = c;
			i = 0;
		}

	} else return 1; // checksum fine

	return 0; // not fixed
}

void privkey_to_pubkey_compressed(uint8_t *privkey, uint8_t *pubkey)
{
	EccPoint R;
	ecc_make_key(&R, privkey);

	vli_swap(R.x,R.x);

	pubkey[0] = 0x02 | (R.y[0] & 0x01);
	memcpy(pubkey + 1, &R.x, 32);
}

void privkey_to_pubkey(uint8_t *privkey, uint8_t *pubkey)
{
	EccPoint R;
	vli_swap(privkey,privkey); // we store all bitcoin data big endian, but do calcs with little endian

	ecc_make_key(&R, privkey);

	vli_swap(privkey,privkey); // swap back from le to be
	vli_swap(R.x,R.x);
	vli_swap(R.y,R.y);

	pubkey[0] = 0x04;
	memcpy(pubkey + 1, &R.x, 32);
	memcpy(pubkey + 33, &R.y, 32);
}

void pubkey_to_pubkeyhash(const uint8_t *pubkey, uint8_t *pubkeyhash)
{
	if (pubkey[0] == 0x04) {  // uncompressed format
		hash160(pubkey, 65, pubkeyhash);
	} else if (pubkey[0] == 0x00) {
		hash160(pubkey, 1, pubkeyhash); // point at infinity
	} else {
		hash160(pubkey, 33, pubkeyhash); // expecting compressed format
	}
}

void pubkeyhash_to_addr(const uint8_t *pubkeyhash, const uint8_t version, char *addr)
{
	uint8_t rawaddr[25];
	rawaddr[0] = version;
	memcpy(rawaddr + 1, pubkeyhash, 20);
	compute_checksum(rawaddr, 21, rawaddr + 21);
	base58_encode(rawaddr, 25, addr);
}

void pubkey_to_addr(const uint8_t *pubkey, const uint8_t version, char *addr)
{
	uint8_t pubkeyhash[20];
	pubkey_to_pubkeyhash(pubkey, pubkeyhash);
	pubkeyhash_to_addr(pubkeyhash, version, addr);
}

void privkey_to_pubkeyhash(uint8_t *privkey, uint8_t *pubkeyhash)
{
	uint8_t temp[65];
	privkey_to_pubkey(privkey,temp);
	pubkey_to_pubkeyhash(temp,pubkeyhash);
}



void pubkey_to_p2pkhscript(uint8_t *pubkey, uint8_t *pkscript)
{
	uint8_t hash[20];
	pubkey_to_pubkeyhash(pubkey,hash);
	pkscript[0] = 0x76; // OP_DUP
	pkscript[1] = 0xa9; // OP_HASH160
	pkscript[2] = 0x14; // PUSHDATA 20 bytes
	memcpy(&pkscript[3], hash, 20);
	pkscript[23] = 0x88; // OP_EQUALVERIFY
	pkscript[24] = 0xac; // OP_CHECKSIG
}

void pkscript_to_addr(const uint8_t *pkscript, const size_t len, char *addr)
{
	if(len == 0x19 &&
		pkscript[0] == 0x76 && // OP_DUP
		pkscript[1] == 0xa9 && // OP_HASH160
		pkscript[2] == 0x14 && // PUSHDATA 20 bytes
		pkscript[23] == 0x88 && // OP_EQUALVERIFY
		pkscript[24] == 0xac) { // OP_CHECKSIG

		pubkeyhash_to_addr(&pkscript[3], 0x00, addr); // is P2PKH

	} else if(len == 0x17 &&
		pkscript[0] == 0xa9 && // OP_HASH160
		pkscript[1] == 0x14 && // PUSHDATA 20 bytes
		pkscript[22] == 0x87) { // OP_EQUAL

		pubkeyhash_to_addr(&pkscript[2], 0x05, addr); // is P2SH

	} else {
		strcpy(addr, "unknown"); // Multisig, P2PK, OP_RETURN not supported
	}
}

void pkscript_to_pubkeyhash(const uint8_t *pkscript, const size_t len, uint8_t *pubkeyhash)
{
	if(len == 0x19 &&
		pkscript[0] == 0x76 && // OP_DUP
		pkscript[1] == 0xa9 && // OP_HASH160
		pkscript[2] == 0x14 && // PUSHDATA 20 bytes
		pkscript[23] == 0x88 && // OP_EQUALVERIFY
		pkscript[24] == 0xac) { // OP_CHECKSIG

		memcpy(pubkeyhash, &pkscript[3], 20); // is P2PKH

	} else if(len == 0x17 &&
		pkscript[0] == 0xa9 && // OP_HASH160
		pkscript[1] == 0x14 && // PUSHDATA 20 bytes
		pkscript[22] == 0x87) { // OP_EQUAL

		memcpy(pubkeyhash, &pkscript[2], 20); // is P2PKH

	} else {
		memset(pubkeyhash, 0, 20); // Multisig, P2PK, OP_RETURN not supported
	}
}