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

#ifndef BITCOIN_H_
#define BITCOIN_H_

#include <stdint.h>
#include <string.h>

#include "ecc.h"
#include "sha2.h"
#include "ripemd160.h"
#include "base58.h"

void hex_init(const char *str, uint8_t *dest);

void hash256(const uint8_t* data, size_t len, uint8_t *digest);
void hash160(const uint8_t *data, size_t len, uint8_t *digest);

void compute_checksum(const uint8_t *data, size_t len, uint8_t checksum[4]);
uint8_t verify_checksum(const uint8_t *data, size_t len, const uint8_t checksum[4]);
uint8_t fix_verify_checksum(uint8_t *data, const size_t len, uint8_t checksum[4]);

void privkey_to_pubkey(uint8_t *, uint8_t *);
void privkey_to_pubkey_compressed(uint8_t *, uint8_t *);
void pubkey_to_pubkeyhash(const uint8_t *pubkey, uint8_t *pubkeyhash);
void privkey_to_pubkeyhash(uint8_t *privkey, uint8_t *pubkeyhash);

void pubkeyhash_to_addr(const uint8_t *pubkeyhash, const uint8_t version, char *addr);
void pubkey_to_addr(const uint8_t *pubkey, const uint8_t version, char *addr);

void pkscript_to_addr(const uint8_t *pkscript, const size_t len, char *addr);
void pkscript_to_pubkeyhash(const uint8_t *pkscript, const size_t len, uint8_t *pubkeyhash);
void pubkey_to_p2pkhscript(uint8_t *pubkey, uint8_t *pkscript);

#endif /* BITCOIN_H_ */