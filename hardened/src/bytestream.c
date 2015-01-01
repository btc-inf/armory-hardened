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
	bp->bptr = 0;
	bp->size = 0;

	bp->inh = 0;
	bp->outh = 0;
	bp->vofs = 0;
}

void b_setbufin(BYT *bp, void *inh, void *ctx)
{
	bp->inh = inh;
	bp->inh_ctx = ctx;
	b_setpos(bp, 0);
}

void b_setbufout(BYT *bp, void *outh, void *ctx)
{
	bp->outh = outh;
	bp->outh_ctx = ctx;
}

uint8_t *b_addr(BYT *bp)
{
	return bp->bs;
}
uint8_t *b_posaddr(BYT *bp)
{
	return bp->bs + bp->bptr;
}

size_t b_tell(BYT *bp)
{
	return bp->vofs + bp->bptr;
}

size_t b_size(BYT *bp)
{
	return bp->size;
}
size_t b_ressize(BYT *bp)
{
	return bp->res;
}
size_t b_srcsize(BYT *bp)
{
	if(!bp->srclen) return bp->size;
	return bp->srclen;
}
size_t b_snksize(BYT *bp)
{
	if(!bp->snklen) return bp->size;
	return bp->snklen;
}

size_t b_seek(BYT *bp, size_t ofs)
{
	//if(bp->ofs + ofs > bp->size) {
	//	ofs = bp->size - bp->ofs;
	//}

	if(ofs < bp->vofs || ofs > bp->vofs+bp->res) {
		if(bp->inh) {
			b_setpos(bp, ofs);
		} else if(bp->outh) {
			if(ofs < bp->vofs) {
				ofs = bp->vofs;
			} else if(ofs > bp->vofs+bp->res) {
				ofs = bp->vofs+bp->res;
			}
		}
	}

	return bp->bptr = ofs - bp->vofs;
}

void b_rewind(BYT *bp)
{
	b_seek(bp, 0);
}

size_t b_truncate(BYT *bp)
{
	return bp->size = bp->bptr;
}

size_t b_reopen(BYT *bp)
{
	uint16_t size = b_size(bp);
	b_rewind(bp);
	b_truncate(bp);
	return size; // return number of bytes discarded
}

size_t b_setpos(BYT *bp, size_t ofs)
{
	if((bp->inh)) {
		bp->size = bp->inh(bp, ofs);
		bp->vofs = ofs;
		bp->bptr = 0;
	}
	return 0;
}

size_t b_read(BYT *bp, uint8_t *buff, size_t btr)
{
	size_t chunk, br = 0;

	while((bp->inh) && btr > (chunk = bp->size - bp->bptr)) {
		memcpy(buff + br, bp->bs + bp->bptr, chunk);
		br += chunk;
		btr -= chunk;
		b_setpos(bp, bp->vofs + bp->bptr + br);
	}

	memcpy(buff + br, bp->bs + bp->bptr, btr);

	bp->bptr = br ? btr : bp->bptr + btr;
	br += btr;

	return br;
}

size_t b_write(BYT *bp, const uint8_t *buff, size_t btw)
{
	size_t chunk, bw = 0;

	while((bp->outh) && btw > (chunk = bp->res - bp->bptr)) {
		memcpy(bp->bs + bp->bptr, buff, chunk);
		bw += chunk;
		btw -= chunk;

		bp->bptr = bp->res;
		bp->size = bp->res;

		b_flush(bp);
	}

	if(btw > (chunk = bp->res - bp->bptr)) btw = chunk;

	memcpy(bp->bs + bp->bptr, buff + bw, btw);
	bp->bptr += btw;
	bw += btw;
	if(bp->bptr > bp->size) bp->size = bp->bptr;

	return bw;
}

void b_flush(BYT *bp)
{
	if((bp->outh)) {
		bp->outh(bp);

		bp->vofs += bp->size;
		bp->bptr = 0;
		bp->size = 0;
	}
	//return 0;
}

size_t b_copy(BYT *bp_from, BYT *bp_to, size_t btc)
{
	uint8_t buff[32];

	if(btc <= sizeof(buff)) {
		b_read(bp_from, buff, btc);
		b_write(bp_to, buff, btc);
	} else {
		uint8_t rem = btc % sizeof(buff);
		uint16_t s = 0;

		while(s + rem < btc) {
			b_read(bp_from, buff, sizeof(buff));
			b_write(bp_to, buff, sizeof(buff));
			s += sizeof(buff);
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