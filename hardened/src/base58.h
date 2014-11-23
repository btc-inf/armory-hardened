// Copyright (c) 2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// Port 2014 by Moritz Schuhmann

#ifndef BASE58_H_
#define BASE58_H_

#include <string.h>
#include <stdint.h>

void base58_encode(uint8_t *src, uint16_t len, char *dest);

#endif /* BASE58_H_ */