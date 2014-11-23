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

#include "bytestream.h"

void b_open(BYT *bp, uint8_t *bytestrm, size_t reserved)
{
	bp->bs = bytestrm;
	bp->res = reserved;
	bp->rewofs = 0;
	bp->bptr = 0;
	bp->size = 0;
}

uint8_t *b_addr(BYT *bp)
{
	return bp->bs;
}

size_t b_tell(BYT *bp)
{
	return bp->bptr;
}

size_t b_size(BYT *bp)
{
	return bp->size;
}

size_t b_seek(BYT *bp, size_t ofs)
{
	//if(bp->ofs + ofs > bp->size) {
	//	ofs = bp->size - bp->ofs;
	//}
	return bp->bptr = ofs;
}

void b_rewind(BYT *bp)
{
	b_seek(bp, bp->rewofs);
}

size_t b_truncate(BYT *bp)
{
	if(bp->rewofs) return 0; // allow truncating only when the full buff is available
	// (we expect methods which work in a limited workspace do this without altering the parent)

	return bp->size = bp->bptr;
}

size_t b_setrewofs(BYT *bp)
{
	return bp->rewofs = bp->bptr;
}
size_t b_unsetrewofs(BYT *bp)
{
	return bp->rewofs = 0;
}

size_t b_reopen(BYT *bp)
{
	if(bp->rewofs) return 0; // allow reopening only when using full buff

	uint16_t size = b_size(bp);
	b_rewind(bp);
	b_truncate(bp);
	return size; // return number of bytes flushed
}

size_t b_read(BYT *bp, uint8_t *buff, size_t btr)
{
	//if(bp->bptr + btr > bp->maxsize) {
	//	btr = bp->maxsize - bp->bptr;
	//}

	memcpy(buff, bp->bs + bp->bptr, btr);
	bp->bptr += btr;

	return btr;
}

size_t b_write(BYT *bp, const uint8_t *buff, size_t btw)
{
	//if(bp->bptr + btw > bp->maxsize) {
	//	btw = bp->maxsize - bp->bptr;
	//}

	memcpy(bp->bs + bp->bptr, buff, btw);
	bp->bptr += btw;

	if(bp->bptr > bp->size) {
		bp->size = bp->bptr;
	}

	return btw;
}

size_t b_copy(BYT *bp_from, BYT *bp_to, size_t btc)
{
	uint8_t buff[8];

	if(btc <= 8) {
		b_read(bp_from, buff, btc);
		b_write(bp_to, buff, btc);
	} else {
		uint8_t rem = btc % 8;
		uint16_t s = 0;

		while(s + rem < btc) {
			b_read(bp_from, buff, 8);
			b_write(bp_to, buff, 8);
			s += 8;
		}

		if(rem > 0) {
			b_read(bp_from, buff, rem);
			b_write(bp_to, buff, rem);
		}
	}
	return btc;
}

size_t b_putc(BYT *bp, char chr)
{
	return b_write(bp, (uint8_t *) &chr, 1);
}

char b_getc(BYT *bp)
{
	char chr;
	b_read(bp, (uint8_t *) &chr, 1);
	return chr;
}

size_t b_putvint(BYT *bp, uint32_t vint)
{
	uint8_t fb;

	if(vint < 0xfd) { // vint length 1
		b_write(bp, (uint8_t *) &vint, 1);
		return 1;
	} else if(vint < 0xffff){ // vint length 3
		fb = 0xfd;
		b_write(bp, &fb, 1);
		b_write(bp, (uint8_t *) &vint, 2);
		return 3;
	} else if(vint < 0xffffffff) { // vint length 5
		fb = 0xfe;
		b_write(bp, &fb, 1);
		b_write(bp, (uint8_t *) &vint, 4);
		return 5;
	} else { // not expecting 64 bit integers in a tx
		return 0;
	}
}

uint32_t b_getvint(BYT *bp)
{
	uint8_t fb;
	uint32_t b;

	b_read(bp, &fb, 1);

	if(fb < 0xfd) { // vint length 1
		return fb;
	} else if(fb < 0xfe) { // vint length 3
		b_read(bp, (uint8_t *) &b, 2);
		return b;
	} else if(fb < 0xff) { // vint length 5
		b_read(bp, (uint8_t *) &b, 4);
		return b;
	} else { // assuming we never get 64 bit integers in a tx
		return 0;
	}
}