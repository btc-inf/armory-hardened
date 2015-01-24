// Copyright (c) 2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// Port 2014 by Moritz Schuhmann

#include "base58.h"

const char pszBase58[] = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

void base58_encode(const uint8_t *src, const uint16_t len, char *dest)
{
	uint8_t pos = 0;
	uint8_t destpos = 0;
	uint32_t carry = 0;

	uint8_t b58[50];
	int8_t b58pos = 0;
	uint8_t b58len = len * 138 / 100 + 1; // log(256) / log(58), rounded up.

	memset(b58, 0, sizeof(b58));

    while (pos < len && !src[pos]) {
        destpos++;
		pos++;
    }

	while (pos < len) {
		carry = src[pos];
		for (b58pos = b58len - 1; b58pos >= 0; b58pos--) {
            carry += 256 * b58[b58pos];
            b58[b58pos] = carry % 58;
            carry /= 58;
        }
		pos++;
	}

	// Skip leading zeroes in base58 result.
    b58pos = 0;
    while (b58pos != b58len && b58[b58pos] == 0) {
        b58pos++;
	}

    // Translate the result into a string.
	memset(dest, '1', destpos);

    while (b58pos != b58len) {
		dest[destpos++] = pszBase58[b58[b58pos++]];
	}
	dest[destpos] = '\0';
}