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

#ifndef BYTESTREAM_H_
#define BYTESTREAM_H_

#include <asf.h>
#include "string.h"

typedef struct bytestream {
	uint8_t *bs; // pointer to buffer
	uint16_t res, size, bptr; // reserved buffer space, current content size, cursor pos
	uint16_t rewofs; // rewind offset (to limit the workspace)
} BYT;

void b_open(BYT *bp, uint8_t *bytestrm, size_t reserved);
uint8_t *b_addr(BYT *bp);
size_t b_tell(BYT *bp);
size_t b_size(BYT *bp);

size_t b_seek(BYT *bp, size_t ofs);
size_t b_setrewofs(BYT *bp);
size_t b_unsetrewofs(BYT *bp);
void b_rewind(BYT *bp);
size_t b_truncate(BYT *bp);
size_t b_reopen(BYT *bp);

size_t b_read(BYT *bp, uint8_t *buff, size_t btr);
size_t b_write(BYT *bp, const uint8_t *buff, size_t btw);
size_t b_copy(BYT *bp_from, BYT *bp_to, size_t btc);

size_t b_putc(BYT *bp, char chr);
char b_getc(BYT *bp);

size_t b_putvint(BYT *bp, uint32_t vint);
uint32_t b_getvint(BYT *bp);

#endif /* BYTESTREAM_H_ */