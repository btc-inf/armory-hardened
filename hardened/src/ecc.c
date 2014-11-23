/*
 * Copyright (c) 2013, Mike Ryan
 * Copyright (c) 2014, Moritz Schuhmann
 * Based on micro-ecc, Copyright (c) 2013, Kenneth MacKay
 * All rights reserved.

 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *  * Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "ecc.h"

#include <string.h>
#include <stdlib.h>

typedef unsigned int uint;

#define CONCAT1(a, b) a##b
#define CONCAT(a, b) CONCAT1(a, b)

#define Curve_P_32 {0x2F, 0xFC, 0xFF, 0xFF, \
					0xFE, 0xFF, 0xFF, 0xFF, \
					0xFF, 0xFF, 0xFF, 0xFF, \
					0xFF, 0xFF, 0xFF, 0xFF, \
					0xFF, 0xFF, 0xFF, 0xFF, \
					0xFF, 0xFF, 0xFF, 0xFF, \
					0xFF, 0xFF, 0xFF, 0xFF, \
					0xFF, 0xFF, 0xFF, 0xFF }

#define Curve_B_32 { \
	7, 0, 0, 0, \
	0, 0, 0, 0, \
	0, 0, 0, 0, \
	0, 0, 0, 0, \
	0, 0, 0, 0, \
	0, 0, 0, 0, \
	0, 0, 0, 0, \
	0, 0, 0, 0 }

/*#define Curve_G_32 { \
    {0x96, 0xC2, 0x98, 0xD8, 0x45, 0x39, 0xA1, 0xF4, 0xA0, 0x33, 0xEB, 0x2D, \
        0x81, 0x7D, 0x03, 0x77, 0xF2, 0x40, 0xA4, 0x63, 0xE5, 0xE6, 0xBC, \
        0xF8, 0x47, 0x42, 0x2C, 0xE1, 0xF2, 0xD1, 0x17, 0x6B}, \
    {0xF5, 0x51, 0xBF, 0x37, 0x68, 0x40, 0xB6, 0xCB, 0xCE, 0x5E, 0x31, 0x6B, \
        0x57, 0x33, 0xCE, 0x2B, 0x16, 0x9E, 0x0F, 0x7C, 0x4A, 0xEB, 0xE7, \
        0x8E, 0x9B, 0x7F, 0x1A, 0xFE, 0xE2, 0x42, 0xE3, 0x4F}}*/
#define Curve_G_32 { \
   {0x98, 0x17, 0xF8, 0x16, \
	0x5B, 0x81, 0xF2, 0x59, \
	0xD9, 0x28, 0xCE, 0x2D, \
	0xDB, 0xFC, 0x9B, 0x02, \
	0x07, 0x0B, 0x87, 0xCE, \
	0x95, 0x62, 0xA0, 0x55, \
	0xAC, 0xBB, 0xDC, 0xF9, \
	0x7E, 0x66, 0xBE, 0x79}, \
   {0xB8, 0xD4, 0x10, 0xFB, \
	0x8F, 0xD0, 0x47, 0x9C, \
	0x19, 0x54, 0x85, 0xA6, \
	0x48, 0xB4, 0x17, 0xFD, \
	0xA8, 0x08, 0x11, 0x0E, \
	0xFC, 0xFB, 0xA4, 0x5D, \
	0x65, 0xC4, 0xA3, 0x26, \
	0x77, 0xDA, 0x3A, 0x48 }}
/* g.x = 79 BE 66 7E F9 DC BB AC 55 A0 62 95 CE 87 0B 07 02 9B FC DB 2D CE 28 D9 59 F2 81 5B 16 F8 17 98
   g.y = 48 3A DA 77 26 A3 C4 65 5D A4 FB FC 0E 11 08 A8 FD 17 B4 48 A6 85 54 19 9C 47 D0 8F FB 10 D4 B8 */
#define Curve_G_48 { \
    {0xB7, 0x0A, 0x76, 0x72, 0x38, 0x5E, 0x54, 0x3A, 0x6C, 0x29, 0x55, 0xBF, \
        0x5D, 0xF2, 0x02, 0x55, 0x38, 0x2A, 0x54, 0x82, 0xE0, 0x41, 0xF7, \
        0x59, 0x98, 0x9B, 0xA7, 0x8B, 0x62, 0x3B, 0x1D, 0x6E, 0x74, 0xAD, \
        0x20, 0xF3, 0x1E, 0xC7, 0xB1, 0x8E, 0x37, 0x05, 0x8B, 0xBE, 0x22, \
        0xCA, 0x87, 0xAA}, \
    {0x5F, 0x0E, 0xEA, 0x90, 0x7C, 0x1D, 0x43, 0x7A, 0x9D, 0x81, 0x7E, 0x1D, \
        0xCE, 0xB1, 0x60, 0x0A, 0xC0, 0xB8, 0xF0, 0xB5, 0x13, 0x31, 0xDA, \
        0xE9, 0x7C, 0x14, 0x9A, 0x28, 0xBD, 0x1D, 0xF4, 0xF8, 0x29, 0xDC, \
        0x92, 0x92, 0xBF, 0x98, 0x9E, 0x5D, 0x6F, 0x2C, 0x26, 0x96, 0x4A, \
        0xDE, 0x17, 0x36}}

/*#define Curve_N_32 {  0x51, 0x25, 0x63, 0xFC, 0xC2, 0xCA, 0xB9, 0xF3, 0x84, \
    0x9E, 0x17, 0xA7, 0xAD, 0xFA, 0xE6, 0xBC, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, \
    0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF }*/
#define Curve_N_32 { 0x41, 0x41, 0x36, 0xD0, 0x8C, 0x5E, 0xD2, 0xBF, 0x3B, 0xA0, 0x48, 0xAF, 0xE6, 0xDC, 0xAE, 0xBA, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF  }
	/* FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFE BA AE DC E6 AF 48 A0 3B BF D2 5E 8C D0 36 41 41 */

static uint8_t curve_p[NUM_ECC_DIGITS] = CONCAT(Curve_P_, ECC_CURVE);
static uint8_t curve_b[NUM_ECC_DIGITS] = CONCAT(Curve_B_, ECC_CURVE);
static EccPoint curve_G = CONCAT(Curve_G_, ECC_CURVE);
uint8_t curve_n[NUM_ECC_DIGITS] = CONCAT(Curve_N_, ECC_CURVE);

static void vli_clear(uint8_t *p_vli)
{
    uint i;
    for(i=0; i<NUM_ECC_DIGITS; ++i)
    {
        p_vli[i] = 0;
    }
}

/* Returns 1 if p_vli == 0, 0 otherwise. */
static int vli_isZero(uint8_t *p_vli)
{
    uint i;
    for(i = 0; i < NUM_ECC_DIGITS; ++i)
    {
        if(p_vli[i])
        {
            return 0;
        }
    }
    return 1;
}

/* Returns nonzero if bit p_bit of p_vli is set. */
static uint8_t vli_testBit(uint8_t *p_vli, uint p_bit)
{
    return (p_vli[p_bit/8] & (1 << (p_bit % 8)));
}

/* Counts the number of 8-bit "digits" in p_vli. */
static uint vli_numDigits(uint8_t *p_vli)
{
    int i;
    /* Search from the end until we find a non-zero digit.
       We do it in reverse because we expect that most digits will be nonzero. */
    for(i = NUM_ECC_DIGITS - 1; i >= 0 && p_vli[i] == 0; --i)
    {
    }

    return (i + 1);
}

/* Counts the number of bits required for p_vli. */
static uint vli_numBits(uint8_t *p_vli)
{
    uint i;
    uint8_t l_digit;

    uint l_numDigits = vli_numDigits(p_vli);
    if(l_numDigits == 0)
    {
        return 0;
    }

    l_digit = p_vli[l_numDigits - 1];
    for(i=0; l_digit; ++i)
    {
        l_digit >>= 1;
    }

    return ((l_numDigits - 1) * 8 + i);
}

/* Sets p_dest = p_src. */
static void vli_set(uint8_t *p_dest, uint8_t *p_src)
{
    uint i;
    for(i=0; i<NUM_ECC_DIGITS; ++i)
    {
        p_dest[i] = p_src[i];
    }
}

/* Returns sign of p_left - p_right. */
static int vli_cmp(uint8_t *p_left, uint8_t *p_right)
{
    int i;
    for(i = NUM_ECC_DIGITS-1; i >= 0; --i)
    {
        if(p_left[i] > p_right[i])
        {
            return 1;
        }
        else if(p_left[i] < p_right[i])
        {
            return -1;
        }
    }
    return 0;
}

/* Computes p_result = p_in << c, returning carry. Can modify in place (if p_result == p_in). 0 < p_shift < 8. */
static uint8_t vli_lshift(uint8_t *p_result, uint8_t *p_in, uint p_shift)
{
    uint8_t l_carry = 0;
    uint i;
    for(i = 0; i < NUM_ECC_DIGITS; ++i)
    {
        uint8_t l_temp = p_in[i];
        p_result[i] = (l_temp << p_shift) | l_carry;
        l_carry = l_temp >> (8 - p_shift);
    }

    return l_carry;
}

/* Computes p_vli = p_vli >> 1. */
static void vli_rshift1(uint8_t *p_vli)
{
    uint8_t *l_end = p_vli;
    uint8_t l_carry = 0;

    p_vli += NUM_ECC_DIGITS;
    while(p_vli-- > l_end)
    {
        uint8_t l_temp = *p_vli;
        *p_vli = (l_temp >> 1) | l_carry;
        l_carry = l_temp << 7;
    }
}

/* Computes p_result = p_left + p_right, returning carry. Can modify in place. */
uint8_t vli_add(uint8_t *p_result, uint8_t *p_left, uint8_t *p_right)
{
    uint8_t l_carry = 0;
    uint i;
    for(i=0; i<NUM_ECC_DIGITS; ++i)
    {
        uint8_t l_sum = p_left[i] + p_right[i] + l_carry;
        if(l_sum != p_left[i])
        {
            l_carry = (l_sum < p_left[i]);
        }
        p_result[i] = l_sum;
    }
    return l_carry;
}

/* Computes p_result = p_left - p_right, returning borrow. Can modify in place. */
static uint8_t vli_sub(uint8_t *p_result, uint8_t *p_left, uint8_t *p_right)
{
    uint8_t l_borrow = 0;
    uint i;
    for(i=0; i<NUM_ECC_DIGITS; ++i)
    {
        uint8_t l_diff = p_left[i] - p_right[i] - l_borrow;
        if(l_diff != p_left[i])
        {
            l_borrow = (l_diff > p_left[i]);
        }
        p_result[i] = l_diff;
    }
    return l_borrow;
}

/* Computes p_result = p_left * p_right. */
void vli_mult(uint8_t *p_result, uint8_t *p_left, uint8_t *p_right)
{
    uint16_t r01 = 0;
    uint8_t r2 = 0;

    uint i, k;

    /* Compute each digit of p_result in sequence, maintaining the carries. */
    for(k=0; k < NUM_ECC_DIGITS*2 - 1; ++k)
    {
        uint l_min = (k < NUM_ECC_DIGITS ? 0 : (k + 1) - NUM_ECC_DIGITS);
        for(i=l_min; i<=k && i<NUM_ECC_DIGITS; ++i)
        {
            uint16_t l_product = (uint16_t)p_left[i] * p_right[k-i];
            r01 += l_product;
            r2 += (r01 < l_product);
        }
        p_result[k] = (uint8_t)r01;
        r01 = (r01 >> 8) | (((uint16_t)r2) << 8);
        r2 = 0;
    }

    p_result[NUM_ECC_DIGITS*2 - 1] = (uint8_t)r01;
}

/* Computes p_result = (p_left + p_right) % p_mod.
   Assumes that p_left < p_mod and p_right < p_mod, p_result != p_mod. */
void vli_modAdd(uint8_t *p_result, uint8_t *p_left, uint8_t *p_right, uint8_t *p_mod)
{
    uint8_t l_carry = vli_add(p_result, p_left, p_right);
    if(l_carry || vli_cmp(p_result, p_mod) >= 0)
    { /* p_result > p_mod (p_result = p_mod + remainder), so subtract p_mod to get remainder. */
        vli_sub(p_result, p_result, p_mod);
    }
}

/* Computes p_result = (p_left - p_right) % p_mod.
   Assumes that p_left < p_mod and p_right < p_mod, p_result != p_mod. */
static void vli_modSub(uint8_t *p_result, uint8_t *p_left, uint8_t *p_right, uint8_t *p_mod)
{
    uint8_t l_borrow = vli_sub(p_result, p_left, p_right);
    if(l_borrow)
    { /* In this case, p_result == -diff == (max int) - diff.
         Since -x % d == d - x, we can get the correct result from p_result + p_mod (with overflow). */
        vli_add(p_result, p_result, p_mod);
    }
}

#if ECC_CURVE == secp128r1

/* Computes p_result = p_product % curve_p.
   See algorithm 5 and 6 from http://www.isys.uni-klu.ac.at/PDF/2001-0126-MT.pdf */
static void vli_mmod_fast(uint8_t *p_result, uint8_t *p_product)
{
    uint8_t l_tmp[NUM_ECC_DIGITS];
    int l_carry;

    vli_set(p_result, p_product);

    l_tmp[0]  = p_product[16];
    l_tmp[1]  = p_product[17];
    l_tmp[2]  = p_product[18];
    l_tmp[3]  = p_product[19];
    l_tmp[4]  = p_product[20];
    l_tmp[5]  = p_product[21];
    l_tmp[6]  = p_product[22];
    l_tmp[7]  = p_product[23];
    l_tmp[8]  = p_product[24];
    l_tmp[9]  = p_product[25];
    l_tmp[10] = p_product[26];
    l_tmp[11] = p_product[27];
    l_tmp[12] = (p_product[28] &  1) | (p_product[16] << 1);
    l_tmp[13] = (p_product[16] >> 7) | (p_product[17] << 1);
    l_tmp[14] = (p_product[17] >> 7) | (p_product[18] << 1);
    l_tmp[15] = (p_product[18] >> 7) | (p_product[19] << 1);
    l_carry = vli_add(p_result, p_result, l_tmp);

    l_tmp[0]  = (p_product[19] >> 7) | (p_product[20] << 1);
    l_tmp[1]  = (p_product[20] >> 7) | (p_product[21] << 1);
    l_tmp[2]  = (p_product[21] >> 7) | (p_product[22] << 1);
    l_tmp[3]  = (p_product[22] >> 7) | (p_product[23] << 1);
    l_tmp[4]  = (p_product[23] >> 7) | (p_product[24] << 1);
    l_tmp[5]  = (p_product[24] >> 7) | (p_product[25] << 1);
    l_tmp[6]  = (p_product[25] >> 7) | (p_product[26] << 1);
    l_tmp[7]  = (p_product[26] >> 7) | (p_product[27] << 1);
    l_tmp[8]  = (p_product[27] >> 7) | (p_product[28] << 1);
    l_tmp[9]  = (p_product[28] >> 7) | (p_product[29] << 1);
    l_tmp[10] = (p_product[29] >> 7) | (p_product[30] << 1);
    l_tmp[11] = (p_product[30] >> 7) | (p_product[31] << 1);
    l_tmp[12] = (p_product[31] >> 7) | ((p_product[19] & 0x80) >> 6) | (p_product[20] << 2);
    l_tmp[13] = (p_product[20] >> 6) | (p_product[21] << 2);
    l_tmp[14] = (p_product[21] >> 6) | (p_product[22] << 2);
    l_tmp[15] = (p_product[22] >> 6) | (p_product[23] << 2);
    l_carry += vli_add(p_result, p_result, l_tmp);

    l_tmp[0]  = (p_product[23] >> 6) | (p_product[24] << 2);
    l_tmp[1]  = (p_product[24] >> 6) | (p_product[25] << 2);
    l_tmp[2]  = (p_product[25] >> 6) | (p_product[26] << 2);
    l_tmp[3]  = (p_product[26] >> 6) | (p_product[27] << 2);
    l_tmp[4]  = (p_product[27] >> 6) | (p_product[28] << 2);
    l_tmp[5]  = (p_product[28] >> 6) | (p_product[29] << 2);
    l_tmp[6]  = (p_product[29] >> 6) | (p_product[30] << 2);
    l_tmp[7]  = (p_product[30] >> 6) | (p_product[31] << 2);
    l_tmp[8]  = (p_product[31] >> 6);
    l_tmp[9]  = 0;
    l_tmp[10] = 0;
    l_tmp[11] = 0;
    l_tmp[12] = ((p_product[23] & 0xC0) >> 5) | (p_product[24] << 3);
    l_tmp[13] = (p_product[24] >> 5) | (p_product[25] << 3);
    l_tmp[14] = (p_product[25] >> 5) | (p_product[26] << 3);
    l_tmp[15] = (p_product[26] >> 5) | (p_product[27] << 3);
    l_carry += vli_add(p_result, p_result, l_tmp);

    l_tmp[0]  = (p_product[27] >> 5) | (p_product[28] << 3);
    l_tmp[1]  = (p_product[28] >> 5) | (p_product[29] << 3);
    l_tmp[2]  = (p_product[29] >> 5) | (p_product[30] << 3);
    l_tmp[3]  = (p_product[30] >> 5) | (p_product[31] << 3);
    l_tmp[4]  = (p_product[31] >> 5);
    l_tmp[5]  = 0;
    l_tmp[6]  = 0;
    l_tmp[7]  = 0;
    l_tmp[8]  = 0;
    l_tmp[9]  = 0;
    l_tmp[10] = 0;
    l_tmp[11] = 0;
    l_tmp[12] = ((p_product[27] & 0xE0) >> 4) | (p_product[28] << 4);
    l_tmp[13] = (p_product[28] >> 4) | (p_product[29] << 4);
    l_tmp[14] = (p_product[29] >> 4) | (p_product[30] << 4);
    l_tmp[15] = (p_product[30] >> 4) | (p_product[31] << 4);
    l_carry += vli_add(p_result, p_result, l_tmp);

    l_tmp[0]  = (p_product[31] >> 4);
    l_tmp[1]  = 0;
    l_tmp[2]  = 0;
    l_tmp[3]  = 0;
    l_tmp[4]  = 0;
    l_tmp[5]  = 0;
    l_tmp[6]  = 0;
    l_tmp[7]  = 0;
    l_tmp[8]  = 0;
    l_tmp[9]  = 0;
    l_tmp[10] = 0;
    l_tmp[11] = 0;
    l_tmp[12] = (p_product[28] & 0xFE);
    l_tmp[13] = p_product[29];
    l_tmp[14] = p_product[30];
    l_tmp[15] = p_product[31];
    l_carry += vli_add(p_result, p_result, l_tmp);

    l_tmp[0]  = 0;
    l_tmp[1]  = 0;
    l_tmp[2]  = 0;
    l_tmp[3]  = 0;
    l_tmp[4]  = 0;
    l_tmp[5]  = 0;
    l_tmp[6]  = 0;
    l_tmp[7]  = 0;
    l_tmp[8]  = 0;
    l_tmp[9]  = 0;
    l_tmp[10] = 0;
    l_tmp[11] = 0;
    l_tmp[12] = ((p_product[31] & 0xF0) >> 3);
    l_tmp[13] = 0;
    l_tmp[14] = 0;
    l_tmp[15] = 0;
    l_carry += vli_add(p_result, p_result, l_tmp);

    while(l_carry || vli_cmp(curve_p, p_result) != 1)
    {
        l_carry -= vli_sub(p_result, p_result, curve_p);
    }
}

#elif ECC_CURVE == secp192r1

/* Computes p_result = p_product % curve_p.
   See algorithm 5 and 6 from http://www.isys.uni-klu.ac.at/PDF/2001-0126-MT.pdf */
static void vli_mmod_fast(uint8_t *p_result, uint8_t *p_product)
{
    uint8_t l_tmp[NUM_ECC_DIGITS];
    int l_carry;

    vli_set(p_result, p_product);

    vli_set(l_tmp, &p_product[NUM_ECC_DIGITS]); // the top 192 bits of result
    l_carry = vli_add(p_result, p_result, l_tmp);

    l_tmp[0] = l_tmp[1] = l_tmp[2] = l_tmp[3] = 0;
    l_tmp[4] = l_tmp[5] = l_tmp[6] = l_tmp[7] = 0;

    l_tmp[8]  = p_product[24];
    l_tmp[9]  = p_product[25];
    l_tmp[10] = p_product[26];
    l_tmp[11] = p_product[27];
    l_tmp[12] = p_product[28];
    l_tmp[13] = p_product[29];
    l_tmp[14] = p_product[30];
    l_tmp[15] = p_product[31];
    l_tmp[16] = p_product[32];
    l_tmp[17] = p_product[33];
    l_tmp[18] = p_product[34];
    l_tmp[19] = p_product[35];
    l_tmp[20] = p_product[36];
    l_tmp[21] = p_product[37];
    l_tmp[22] = p_product[38];
    l_tmp[23] = p_product[39];
    l_carry += vli_add(p_result, p_result, l_tmp);

    l_tmp[0]  = l_tmp[8]  = p_product[40];
    l_tmp[1]  = l_tmp[9]  = p_product[41];
    l_tmp[2]  = l_tmp[10] = p_product[42];
    l_tmp[3]  = l_tmp[11] = p_product[43];
    l_tmp[4]  = l_tmp[12] = p_product[44];
    l_tmp[5]  = l_tmp[13] = p_product[45];
    l_tmp[6]  = l_tmp[14] = p_product[46];
    l_tmp[7]  = l_tmp[15] = p_product[47];
    l_tmp[16] = l_tmp[17] = l_tmp[18] = l_tmp[19] = 0;
    l_tmp[20] = l_tmp[21] = l_tmp[22] = l_tmp[23] = 0;
    l_carry += vli_add(p_result, p_result, l_tmp);

    while(l_carry || vli_cmp(curve_p, p_result) != 1)
    {
        l_carry -= vli_sub(p_result, p_result, curve_p);
    }
}


#elif (ECC_CURVE == secp256k1)
/* omega_mult() is defined farther below for the different curves / word sizes */
static void omega_mult(uint8_t *p_result, uint8_t *p_right);



static void muladd(uint8_t a, uint8_t b, uint8_t *r0, uint8_t *r1, uint8_t *r2)
{

	uint16_t p = (uint16_t)a * b;
	uint16_t r01 = ((uint16_t)(*r1) << 8) | *r0;
	r01 += p;
	*r2 += (r01 < p);
	*r1 = r01 >> 8;
	*r0 = (uint8_t)r01;
}











/* Computes p_result = p_product % curve_p
    see http://www.isys.uni-klu.ac.at/PDF/2001-0126-MT.pdf page 354

    Note that this only works if log2(omega) < log2(p)/2 */
static void vli_mmod_fast(uint8_t *p_result, uint8_t *p_product)
{
    uint8_t l_tmp[2*NUM_ECC_DIGITS];
    int l_carry;

    vli_clear(l_tmp);
    vli_clear(l_tmp + NUM_ECC_DIGITS);

    omega_mult(l_tmp, p_product + NUM_ECC_DIGITS); /* (Rq, q) = q * c */

    l_carry = vli_add(p_result, p_product, l_tmp); /* (C, r) = r + q       */
    vli_clear(p_product);
    omega_mult(p_product, l_tmp + NUM_ECC_DIGITS); /* Rq*c */
    l_carry += vli_add(p_result, p_result, p_product); /* (C1, r) = r + Rq*c */

    while(l_carry > 0)
    {
        --l_carry;
        vli_sub(p_result, p_result, curve_p);
    }

    if(vli_cmp(p_result, curve_p) > 0)
    {
        vli_sub(p_result, p_result, curve_p);
    }
}

static void omega_mult(uint8_t *p_result, uint8_t *p_right)
{
	/* Multiply by (2^32 + 2^9 + 2^8 + 2^7 + 2^6 + 2^4 + 1). */
	uint8_t r0 = 0;
	uint8_t r1 = 0;
	uint8_t r2 = 0;

	uint8_t k;

	/* Multiply by (2^9 + 2^8 + 2^7 + 2^6 + 2^4 + 1). */
	muladd(0xD1, p_right[0], &r0, &r1, &r2);
	p_result[0] = r0;
	r0 = r1;
	r1 = r2;
	/* r2 is still 0 */

	for(k = 1; k < NUM_ECC_DIGITS; ++k)
	{
		muladd(0x03, p_right[k-1], &r0, &r1, &r2);
		muladd(0xD1, p_right[k], &r0, &r1, &r2);
		p_result[k] = r0;
		r0 = r1;
		r1 = r2;
		r2 = 0;
	}

	muladd(0x03, p_right[NUM_ECC_DIGITS-1], &r0, &r1, &r2);
	p_result[NUM_ECC_DIGITS] = r0;
	p_result[NUM_ECC_DIGITS + 1] = r1;

	p_result[4 + NUM_ECC_DIGITS] = vli_add(p_result + 4, p_result + 4, p_right); /* add the 2^32 multiple */
}


#elif ECC_CURVE == secp256k1 +1

/* Computes p_result = p_product % curve_p
   from http://www.nsa.gov/ia/_files/nist-routines.pdf */
static void vli_mmod_fast(uint8_t *p_result, uint8_t *p_product)
{
    uint8_t l_tmp[NUM_ECC_DIGITS];
    int l_carry;

    /* t */
    vli_set(p_result, p_product);

    /* s1 */
    l_tmp[0]  = l_tmp[1] = l_tmp[2]  = l_tmp[3] = 0;
    l_tmp[4]  = l_tmp[5] = l_tmp[6]  = l_tmp[7] = 0;
    l_tmp[8]  = l_tmp[9] = l_tmp[10] = l_tmp[11] = 0;
    l_tmp[12] = p_product[44];
    l_tmp[13] = p_product[45];
    l_tmp[14] = p_product[46];
    l_tmp[15] = p_product[47];
    l_tmp[16] = p_product[48];
    l_tmp[17] = p_product[49];
    l_tmp[18] = p_product[50];
    l_tmp[19] = p_product[51];
    l_tmp[20] = p_product[52];
    l_tmp[21] = p_product[53];
    l_tmp[22] = p_product[54];
    l_tmp[23] = p_product[55];
    l_tmp[24] = p_product[56];
    l_tmp[25] = p_product[57];
    l_tmp[26] = p_product[58];
    l_tmp[27] = p_product[59];
    l_tmp[28] = p_product[60];
    l_tmp[29] = p_product[61];
    l_tmp[30] = p_product[62];
    l_tmp[31] = p_product[63];
    l_carry = vli_lshift(l_tmp, l_tmp, 1);
    l_carry += vli_add(p_result, p_result, l_tmp);

    /* s2 */
    l_tmp[12] = p_product[48];
    l_tmp[13] = p_product[49];
    l_tmp[14] = p_product[50];
    l_tmp[15] = p_product[51];
    l_tmp[16] = p_product[52];
    l_tmp[17] = p_product[53];
    l_tmp[18] = p_product[54];
    l_tmp[19] = p_product[55];
    l_tmp[20] = p_product[56];
    l_tmp[21] = p_product[57];
    l_tmp[22] = p_product[58];
    l_tmp[23] = p_product[59];
    l_tmp[24] = p_product[60];
    l_tmp[25] = p_product[61];
    l_tmp[26] = p_product[62];
    l_tmp[27] = p_product[63];
    l_tmp[28] = 0;
    l_tmp[29] = 0;
    l_tmp[30] = 0;
    l_tmp[31] = 0;
    l_carry += vli_lshift(l_tmp, l_tmp, 1);
    l_carry += vli_add(p_result, p_result, l_tmp);

    /* s3 */
    l_tmp[0]  = p_product[32];
    l_tmp[1]  = p_product[33];
    l_tmp[2]  = p_product[34];
    l_tmp[3]  = p_product[35];
    l_tmp[4]  = p_product[36];
    l_tmp[5]  = p_product[37];
    l_tmp[6]  = p_product[38];
    l_tmp[7]  = p_product[39];
    l_tmp[8]  = p_product[40];
    l_tmp[9]  = p_product[41];
    l_tmp[10] = p_product[42];
    l_tmp[11] = p_product[43];
    l_tmp[12] = l_tmp[13] = l_tmp[14] = l_tmp[15] = 0;
    l_tmp[16] = l_tmp[17] = l_tmp[18] = l_tmp[19] = 0;
    l_tmp[20] = l_tmp[21] = l_tmp[22] = l_tmp[23] = 0;
    l_tmp[24] = p_product[56];
    l_tmp[25] = p_product[57];
    l_tmp[26] = p_product[58];
    l_tmp[27] = p_product[59];
    l_tmp[28] = p_product[60];
    l_tmp[29] = p_product[61];
    l_tmp[30] = p_product[62];
    l_tmp[31] = p_product[63];
    l_carry += vli_add(p_result, p_result, l_tmp);

    /* s4 */
    l_tmp[0]  = p_product[36];
    l_tmp[1]  = p_product[37];
    l_tmp[2]  = p_product[38];
    l_tmp[3]  = p_product[39];
    l_tmp[4]  = p_product[40];
    l_tmp[5]  = p_product[41];
    l_tmp[6]  = p_product[42];
    l_tmp[7]  = p_product[43];
    l_tmp[8]  = p_product[44];
    l_tmp[9]  = p_product[45];
    l_tmp[10] = p_product[46];
    l_tmp[11] = p_product[47];
    l_tmp[12] = p_product[52];
    l_tmp[13] = p_product[53];
    l_tmp[14] = p_product[54];
    l_tmp[15] = p_product[55];
    l_tmp[16] = p_product[56];
    l_tmp[17] = p_product[57];
    l_tmp[18] = p_product[58];
    l_tmp[19] = p_product[59];
    l_tmp[20] = p_product[60];
    l_tmp[21] = p_product[61];
    l_tmp[22] = p_product[62];
    l_tmp[23] = p_product[63];
    l_tmp[24] = p_product[52];
    l_tmp[25] = p_product[53];
    l_tmp[26] = p_product[54];
    l_tmp[27] = p_product[55];
    l_tmp[28] = p_product[32];
    l_tmp[29] = p_product[33];
    l_tmp[30] = p_product[34];
    l_tmp[31] = p_product[35];
    l_carry += vli_add(p_result, p_result, l_tmp);

    /* d1 */
    l_tmp[0]  = p_product[44];
    l_tmp[1]  = p_product[45];
    l_tmp[2]  = p_product[46];
    l_tmp[3]  = p_product[47];
    l_tmp[4]  = p_product[48];
    l_tmp[5]  = p_product[49];
    l_tmp[6]  = p_product[50];
    l_tmp[7]  = p_product[51];
    l_tmp[8]  = p_product[52];
    l_tmp[9]  = p_product[53];
    l_tmp[10] = p_product[54];
    l_tmp[11] = p_product[55];
    l_tmp[12] = l_tmp[13] = l_tmp[14] = l_tmp[15] = 0;
    l_tmp[16] = l_tmp[17] = l_tmp[18] = l_tmp[19] = 0;
    l_tmp[20] = l_tmp[21] = l_tmp[22] = l_tmp[23] = 0;
    l_tmp[24] = p_product[32];
    l_tmp[25] = p_product[33];
    l_tmp[26] = p_product[34];
    l_tmp[27] = p_product[35];
    l_tmp[28] = p_product[40];
    l_tmp[29] = p_product[41];
    l_tmp[30] = p_product[42];
    l_tmp[31] = p_product[43];
    l_carry -= vli_sub(p_result, p_result, l_tmp);

    /* d2 */
    l_tmp[0]  = p_product[48];
    l_tmp[1]  = p_product[49];
    l_tmp[2]  = p_product[50];
    l_tmp[3]  = p_product[51];
    l_tmp[4]  = p_product[52];
    l_tmp[5]  = p_product[53];
    l_tmp[6]  = p_product[54];
    l_tmp[7]  = p_product[55];
    l_tmp[8]  = p_product[56];
    l_tmp[9]  = p_product[57];
    l_tmp[10] = p_product[58];
    l_tmp[11] = p_product[59];
    l_tmp[12] = p_product[60];
    l_tmp[13] = p_product[61];
    l_tmp[14] = p_product[62];
    l_tmp[15] = p_product[63];
    l_tmp[16] = l_tmp[17] = l_tmp[18] = l_tmp[19] = 0;
    l_tmp[20] = l_tmp[21] = l_tmp[22] = l_tmp[23] = 0;
    l_tmp[24] = p_product[36];
    l_tmp[25] = p_product[37];
    l_tmp[26] = p_product[38];
    l_tmp[27] = p_product[39];
    l_tmp[28] = p_product[44];
    l_tmp[29] = p_product[45];
    l_tmp[30] = p_product[46];
    l_tmp[31] = p_product[47];
    l_carry -= vli_sub(p_result, p_result, l_tmp);

    /* d3 */
    l_tmp[0]  = p_product[52];
    l_tmp[1]  = p_product[53];
    l_tmp[2]  = p_product[54];
    l_tmp[3]  = p_product[55];
    l_tmp[4]  = p_product[56];
    l_tmp[5]  = p_product[57];
    l_tmp[6]  = p_product[58];
    l_tmp[7]  = p_product[59];
    l_tmp[8]  = p_product[60];
    l_tmp[9]  = p_product[61];
    l_tmp[10] = p_product[62];
    l_tmp[11] = p_product[63];
    l_tmp[12] = p_product[32];
    l_tmp[13] = p_product[33];
    l_tmp[14] = p_product[34];
    l_tmp[15] = p_product[35];
    l_tmp[16] = p_product[36];
    l_tmp[17] = p_product[37];
    l_tmp[18] = p_product[38];
    l_tmp[19] = p_product[39];
    l_tmp[20] = p_product[40];
    l_tmp[21] = p_product[41];
    l_tmp[22] = p_product[42];
    l_tmp[23] = p_product[43];
    l_tmp[24] = l_tmp[25] = l_tmp[26] = l_tmp[27] = 0;
    l_tmp[28] = p_product[48];
    l_tmp[29] = p_product[49];
    l_tmp[30] = p_product[50];
    l_tmp[31] = p_product[51];
    l_carry -= vli_sub(p_result, p_result, l_tmp);

    /* d4 */
    l_tmp[0]  = p_product[56];
    l_tmp[1]  = p_product[57];
    l_tmp[2]  = p_product[58];
    l_tmp[3]  = p_product[59];
    l_tmp[4]  = p_product[60];
    l_tmp[5]  = p_product[61];
    l_tmp[6]  = p_product[62];
    l_tmp[7]  = p_product[63];
    l_tmp[8]  = l_tmp[9]  = l_tmp[10]  = l_tmp[11] = 0;
    l_tmp[12] = p_product[36];
    l_tmp[13] = p_product[37];
    l_tmp[14] = p_product[38];
    l_tmp[15] = p_product[39];
    l_tmp[16] = p_product[40];
    l_tmp[17] = p_product[41];
    l_tmp[18] = p_product[42];
    l_tmp[19] = p_product[43];
    l_tmp[20] = p_product[44];
    l_tmp[21] = p_product[45];
    l_tmp[22] = p_product[46];
    l_tmp[23] = p_product[47];
    l_tmp[24] = l_tmp[25] = l_tmp[26] = l_tmp[27] = 0;
    l_tmp[28] = p_product[52];
    l_tmp[29] = p_product[53];
    l_tmp[30] = p_product[54];
    l_tmp[31] = p_product[55];
    l_carry -= vli_sub(p_result, p_result, l_tmp);

    if(l_carry < 0)
    {
        do
        {
            l_carry += vli_add(p_result, p_result, curve_p);
        } while(l_carry < 0);
    }
    else
    {
        while(l_carry || vli_cmp(curve_p, p_result) != 1)
        {
            l_carry -= vli_sub(p_result, p_result, curve_p);
        }
    }
}

#elif ECC_CURVE == secp384r1

static void omega_mult(uint8_t *p_result, uint8_t *p_right)
{
    /* Multiply by (2^128 + 2^96 - 2^32 + 1). */
    vli_set(p_result, p_right); /* 1 */
    p_result[16 + NUM_ECC_DIGITS] = vli_add(p_result + 12, p_result + 12, p_right); /* 2^96 + 1 */
    p_result[17 + NUM_ECC_DIGITS] = 0;
    p_result[18 + NUM_ECC_DIGITS] = 0;
    p_result[19 + NUM_ECC_DIGITS] = 0;
    p_result[20 + NUM_ECC_DIGITS] = vli_add(p_result + 16, p_result + 16, p_right); /* 2^128 + 2^96 + 1 */
    p_result[21 + NUM_ECC_DIGITS] = 0;
    p_result[22 + NUM_ECC_DIGITS] = 0;
    p_result[23 + NUM_ECC_DIGITS] = 0;
    if(vli_sub(p_result + 4, p_result + 4, p_right)) /* 2^128 + 2^96 - 2^32 + 1 */
    { /* Propagate borrow if necessary. */
        uint i;
        for(i = 4 + NUM_ECC_DIGITS; ; ++i)
        {
            --p_result[i];
            if(p_result[i] != 0xff)
            {
                break;
            }
        }
    }
}

/* Computes p_result = p_product % curve_p
    see PDF "Comparing Elliptic Curve Cryptography and RSA on 8-bit CPUs"
    section "Curve-Specific Optimizations" */
static void vli_mmod_fast(uint8_t *p_result, uint8_t *p_product)
{
    uint8_t l_tmp[2*NUM_ECC_DIGITS];

    while(!vli_isZero(p_product + NUM_ECC_DIGITS)) /* While c1 != 0 */
    {
        uint8_t l_carry = 0;
        uint i;

        vli_clear(l_tmp);
        vli_clear(l_tmp + NUM_ECC_DIGITS);
        omega_mult(l_tmp, p_product + NUM_ECC_DIGITS); /* tmp = w * c1 */
        vli_clear(p_product + NUM_ECC_DIGITS); /* p = c0 */

        /* (c1, c0) = c0 + w * c1 */
        for(i=0; i<NUM_ECC_DIGITS+20; ++i)
        {
            uint8_t l_sum = p_product[i] + l_tmp[i] + l_carry;
            if(l_sum != p_product[i])
            {
                l_carry = (l_sum < p_product[i]);
            }
            p_product[i] = l_sum;
        }
    }

    while(vli_cmp(p_product, curve_p) > 0)
    {
        vli_sub(p_product, p_product, curve_p);
    }
    vli_set(p_result, p_product);
}

#endif

/* Computes p_result = (p_left * p_right) % curve_p. */
void vli_modMult_fast(uint8_t *p_result, uint8_t *p_left, uint8_t *p_right)
{
   uint8_t l_product[2 * NUM_ECC_DIGITS];
   vli_mult(l_product, p_left, p_right);
   vli_mmod_fast(p_result, l_product);

   //vli_modMult(p_result, p_left, p_right, curve_n);
}

#if ECC_SQUARE_FUNC

/* Computes p_result = p_left^2. */
static void vli_square(uint8_t *p_result, uint8_t *p_left)
{
    uint16_t r01 = 0;
    uint8_t r2 = 0;

    uint i, k;
    for(k=0; k < NUM_ECC_DIGITS*2 - 1; ++k)
    {
        uint l_min = (k < NUM_ECC_DIGITS ? 0 : (k + 1) - NUM_ECC_DIGITS);
        for(i=l_min; i<=k && i<=k-i; ++i)
        {
            uint16_t l_product = (uint16_t)p_left[i] * p_left[k-i];
            if(i < k-i)
            {
                r2 += l_product >> 15;
                l_product *= 2;
            }
            r01 += l_product;
            r2 += (r01 < l_product);
        }
        p_result[k] = (uint8_t)r01;
        r01 = (r01 >> 8) | (((uint16_t)r2) << 8);
        r2 = 0;
    }

    p_result[NUM_ECC_DIGITS*2 - 1] = (uint8_t)r01;
}

/* Computes p_result = p_left^2 % curve_p. */
static void vli_modSquare_fast(uint8_t *p_result, uint8_t *p_left)
{
    uint8_t l_product[2 * NUM_ECC_DIGITS];
    vli_square(l_product, p_left);
    vli_mmod_fast(p_result, l_product);
}

#else /* ECC_SQUARE_FUNC */

#define vli_square(result, left, size) vli_mult((result), (left), (left), (size))
#define vli_modSquare_fast(result, left) vli_modMult_fast((result), (left), (left))

#endif /* ECC_SQUARE_FUNC */

#define EVEN(vli) (!(vli[0] & 1))
/* Computes p_result = (1 / p_input) % p_mod. All VLIs are the same size.
   See "From Euclid's GCD to Montgomery Multiplication to the Great Divide"
   https://labs.oracle.com/techrep/2001/smli_tr-2001-95.pdf */
static void vli_modInv(uint8_t *p_result, uint8_t *p_input, uint8_t *p_mod)
{
    uint8_t a[NUM_ECC_DIGITS], b[NUM_ECC_DIGITS], u[NUM_ECC_DIGITS], v[NUM_ECC_DIGITS];
    uint8_t l_carry;

    vli_set(a, p_input);
    vli_set(b, p_mod);
    vli_clear(u);
    u[0] = 1;
    vli_clear(v);

    int l_cmpResult;
    while((l_cmpResult = vli_cmp(a, b)) != 0)
    {
        l_carry = 0;
        if(EVEN(a))
        {
            vli_rshift1(a);
            if(!EVEN(u))
            {
                l_carry = vli_add(u, u, p_mod);
            }
            vli_rshift1(u);
            if(l_carry)
            {
                u[NUM_ECC_DIGITS-1] |= 0x80;
            }
        }
        else if(EVEN(b))
        {
            vli_rshift1(b);
            if(!EVEN(v))
            {
                l_carry = vli_add(v, v, p_mod);
            }
            vli_rshift1(v);
            if(l_carry)
            {
                v[NUM_ECC_DIGITS-1] |= 0x80;
            }
        }
        else if(l_cmpResult > 0)
        {
            vli_sub(a, a, b);
            vli_rshift1(a);
            if(vli_cmp(u, v) < 0)
            {
                vli_add(u, u, p_mod);
            }
            vli_sub(u, u, v);
            if(!EVEN(u))
            {
                l_carry = vli_add(u, u, p_mod);
            }
            vli_rshift1(u);
            if(l_carry)
            {
                u[NUM_ECC_DIGITS-1] |= 0x80;
            }
        }
        else
        {
            vli_sub(b, b, a);
            vli_rshift1(b);
            if(vli_cmp(v, u) < 0)
            {
                vli_add(v, v, p_mod);
            }
            vli_sub(v, v, u);
            if(!EVEN(v))
            {
                l_carry = vli_add(v, v, p_mod);
            }
            vli_rshift1(v);
            if(l_carry)
            {
                v[NUM_ECC_DIGITS-1] |= 0x80;
            }
        }
    }

    vli_set(p_result, u);
}

/* ------ Point operations ------ */

/* Returns 1 if p_point is the point at infinity, 0 otherwise. */
static int EccPoint_isZero(EccPoint *p_point)
{
    return (vli_isZero(p_point->x) && vli_isZero(p_point->y));
}

/* Point multiplication algorithm using Montgomery's ladder with co-Z coordinates.
From http://eprint.iacr.org/2011/338.pdf
*/

/* Double in place */
static void EccPoint_double_jacobian(uint8_t *X1, uint8_t *Y1, uint8_t *Z1)
{
    /* t1 = X, t2 = Y, t3 = Z */
    uint8_t t4[NUM_ECC_DIGITS];
    uint8_t t5[NUM_ECC_DIGITS];

    if(vli_isZero(Z1))
    {
        return;
    }



	vli_modSquare_fast(t5, Y1);   /* t5 = y1^2 */
	vli_modMult_fast(t4, X1, t5); /* t4 = x1*y1^2 = A */
	vli_modSquare_fast(X1, X1);   /* t1 = x1^2 */
	vli_modSquare_fast(t5, t5);   /* t5 = y1^4 */
	vli_modMult_fast(Z1, Y1, Z1); /* t3 = y1*z1 = z3 */

	vli_modAdd(Y1, X1, X1, curve_p); /* t2 = 2*x1^2 */
	vli_modAdd(Y1, Y1, X1, curve_p); /* t2 = 3*x1^2 */
	if(vli_testBit(Y1, 0))
	{
		uint8_t l_carry = vli_add(Y1, Y1, curve_p);
		vli_rshift1(Y1);
		Y1[NUM_ECC_DIGITS-1] |= l_carry << (8 - 1);
	}
	else
	{
		vli_rshift1(Y1);
	}
	/* t2 = 3/2*(x1^2) = B */

	vli_modSquare_fast(X1, Y1);   /* t1 = B^2 */
	vli_modSub(X1, X1, t4, curve_p); /* t1 = B^2 - A */
	vli_modSub(X1, X1, t4, curve_p); /* t1 = B^2 - 2A = x3 */

	vli_modSub(t4, t4, X1, curve_p); /* t4 = A - x3 */
	vli_modMult_fast(Y1, Y1, t4);    /* t2 = B * (A - x3) */
	vli_modSub(Y1, Y1, t5, curve_p); /* t2 = B * (A - x3) - y1^4 = y3 */


    //vli_modSquare_fast(t4, Y1);   /* t4 = y1^2 */
    //vli_modMult_fast(t5, X1, t4); /* t5 = x1*y1^2 = A */
    //vli_modSquare_fast(t4, t4);   /* t4 = y1^4 */
    //vli_modMult_fast(Y1, Y1, Z1); /* t2 = y1*z1 = z3 */
    //vli_modSquare_fast(Z1, Z1);   /* t3 = z1^2 */

    //vli_modAdd(X1, X1, Z1, curve_p); /* t1 = x1 + z1^2 */
    //vli_modAdd(Z1, Z1, Z1, curve_p); /* t3 = 2*z1^2 */
    //vli_modSub(Z1, X1, Z1, curve_p); /* t3 = x1 - z1^2 */
    //vli_modMult_fast(X1, X1, Z1);    /* t1 = x1^2 - z1^4 */

    //vli_modAdd(Z1, X1, X1, curve_p); /* t3 = 2*(x1^2 - z1^4) */
    //vli_modAdd(X1, X1, Z1, curve_p); /* t1 = 3*(x1^2 - z1^4) */
    //if(vli_testBit(X1, 0))
    //{
    //    uint8_t l_carry = vli_add(X1, X1, curve_p);
    //    vli_rshift1(X1);
    //    X1[NUM_ECC_DIGITS-1] |= l_carry << 7;
    //}
    //else
    //{
    //    vli_rshift1(X1);
    //}
                     /* t1 = 3/2*(x1^2 - z1^4) = B */

    //vli_modSquare_fast(Z1, X1);      /* t3 = B^2 */
    //vli_modSub(Z1, Z1, t5, curve_p); /* t3 = B^2 - A */
    //vli_modSub(Z1, Z1, t5, curve_p); /* t3 = B^2 - 2A = x3 */
    //vli_modSub(t5, t5, Z1, curve_p); /* t5 = A - x3 */
    //vli_modMult_fast(X1, X1, t5);    /* t1 = B * (A - x3) */
    //vli_modSub(t4, X1, t4, curve_p); /* t4 = B * (A - x3) - y1^4 = y3 */

    //vli_set(X1, Z1);
    //vli_set(Z1, Y1);
    //vli_set(Y1, t4);*/
}

/* Modify (x1, y1) => (x1 * z^2, y1 * z^3) */
static void apply_z(uint8_t *X1, uint8_t *Y1, uint8_t *Z)
{
    uint8_t t1[NUM_ECC_DIGITS];

    vli_modSquare_fast(t1, Z);    /* z^2 */
    vli_modMult_fast(X1, X1, t1); /* x1 * z^2 */
    vli_modMult_fast(t1, t1, Z);  /* z^3 */
    vli_modMult_fast(Y1, Y1, t1); /* y1 * z^3 */
}

/* P = (x1, y1) => 2P, (x2, y2) => P' */
static void XYcZ_initial_double(uint8_t *X1, uint8_t *Y1, uint8_t *X2, uint8_t *Y2, uint8_t *p_initialZ)
{
    uint8_t z[NUM_ECC_DIGITS];

    vli_set(X2, X1);
    vli_set(Y2, Y1);

    vli_clear(z);
    z[0] = 1;
    if(p_initialZ)
    {
        vli_set(z, p_initialZ);
    }
    apply_z(X1, Y1, z);

    EccPoint_double_jacobian(X1, Y1, z);

    apply_z(X2, Y2, z);
}

/* Input P = (x1, y1, Z), Q = (x2, y2, Z)
   Output P' = (x1', y1', Z3), P + Q = (x3, y3, Z3)
   or P => P', Q => P + Q
*/
static void XYcZ_add(uint8_t *X1, uint8_t *Y1, uint8_t *X2, uint8_t *Y2)
{
    /* t1 = X1, t2 = Y1, t3 = X2, t4 = Y2 */
    uint8_t t5[NUM_ECC_DIGITS];

    vli_modSub(t5, X2, X1, curve_p); /* t5 = x2 - x1 */
    vli_modSquare_fast(t5, t5);      /* t5 = (x2 - x1)^2 = A */
    vli_modMult_fast(X1, X1, t5);    /* t1 = x1*A = B */
    vli_modMult_fast(X2, X2, t5);    /* t3 = x2*A = C */
    vli_modSub(Y2, Y2, Y1, curve_p); /* t4 = y2 - y1 */
    vli_modSquare_fast(t5, Y2);      /* t5 = (y2 - y1)^2 = D */

    vli_modSub(t5, t5, X1, curve_p); /* t5 = D - B */
    vli_modSub(t5, t5, X2, curve_p); /* t5 = D - B - C = x3 */
    vli_modSub(X2, X2, X1, curve_p); /* t3 = C - B */
    vli_modMult_fast(Y1, Y1, X2);    /* t2 = y1*(C - B) */
    vli_modSub(X2, X1, t5, curve_p); /* t3 = B - x3 */
    vli_modMult_fast(Y2, Y2, X2);    /* t4 = (y2 - y1)*(B - x3) */
    vli_modSub(Y2, Y2, Y1, curve_p); /* t4 = y3 */

    vli_set(X2, t5);
}

/* Input P = (x1, y1, Z), Q = (x2, y2, Z)
   Output P + Q = (x3, y3, Z3), P - Q = (x3', y3', Z3)
   or P => P - Q, Q => P + Q
*/
static void XYcZ_addC(uint8_t *X1, uint8_t *Y1, uint8_t *X2, uint8_t *Y2)
{
    /* t1 = X1, t2 = Y1, t3 = X2, t4 = Y2 */
    uint8_t t5[NUM_ECC_DIGITS];
    uint8_t t6[NUM_ECC_DIGITS];
    uint8_t t7[NUM_ECC_DIGITS];

    vli_modSub(t5, X2, X1, curve_p); /* t5 = x2 - x1 */
    vli_modSquare_fast(t5, t5);      /* t5 = (x2 - x1)^2 = A */
    vli_modMult_fast(X1, X1, t5);    /* t1 = x1*A = B */
    vli_modMult_fast(X2, X2, t5);    /* t3 = x2*A = C */
    vli_modAdd(t5, Y2, Y1, curve_p); /* t4 = y2 + y1 */
    vli_modSub(Y2, Y2, Y1, curve_p); /* t4 = y2 - y1 */

    vli_modSub(t6, X2, X1, curve_p); /* t6 = C - B */
    vli_modMult_fast(Y1, Y1, t6);    /* t2 = y1 * (C - B) */
    vli_modAdd(t6, X1, X2, curve_p); /* t6 = B + C */
    vli_modSquare_fast(X2, Y2);      /* t3 = (y2 - y1)^2 */
    vli_modSub(X2, X2, t6, curve_p); /* t3 = x3 */

    vli_modSub(t7, X1, X2, curve_p); /* t7 = B - x3 */
    vli_modMult_fast(Y2, Y2, t7);    /* t4 = (y2 - y1)*(B - x3) */
    vli_modSub(Y2, Y2, Y1, curve_p); /* t4 = y3 */

    vli_modSquare_fast(t7, t5);      /* t7 = (y2 + y1)^2 = F */
    vli_modSub(t7, t7, t6, curve_p); /* t7 = x3' */
    vli_modSub(t6, t7, X1, curve_p); /* t6 = x3' - B */
    vli_modMult_fast(t6, t6, t5);    /* t6 = (y2 + y1)*(x3' - B) */
    vli_modSub(Y1, t6, Y1, curve_p); /* t2 = y3' */

    vli_set(X1, t7);
}

void EccPoint_mult(EccPoint *p_result, EccPoint *p_point, uint8_t *p_scalar, uint8_t *p_initialZ)
{
    /* R0 and R1 */
    uint8_t Rx[2][NUM_ECC_DIGITS];
    uint8_t Ry[2][NUM_ECC_DIGITS];
    uint8_t z[NUM_ECC_DIGITS];

    uint i, nb;

    vli_set(Rx[1], p_point->x);
    vli_set(Ry[1], p_point->y);

    XYcZ_initial_double(Rx[1], Ry[1], Rx[0], Ry[0], p_initialZ);

    for(i = vli_numBits(p_scalar) - 2; i > 0; --i)
    {
        nb = !vli_testBit(p_scalar, i);
        XYcZ_addC(Rx[1-nb], Ry[1-nb], Rx[nb], Ry[nb]);
        XYcZ_add(Rx[nb], Ry[nb], Rx[1-nb], Ry[1-nb]);
    }

    nb = !vli_testBit(p_scalar, 0);
    XYcZ_addC(Rx[1-nb], Ry[1-nb], Rx[nb], Ry[nb]);

    /* Find final 1/Z value. */
    vli_modSub(z, Rx[1], Rx[0], curve_p); /* X1 - X0 */
    vli_modMult_fast(z, z, Ry[1-nb]);     /* Yb * (X1 - X0) */
    vli_modMult_fast(z, z, p_point->x);   /* xP * Yb * (X1 - X0) */
    vli_modInv(z, z, curve_p);            /* 1 / (xP * Yb * (X1 - X0)) */
    vli_modMult_fast(z, z, p_point->y);   /* yP / (xP * Yb * (X1 - X0)) */
    vli_modMult_fast(z, z, Rx[1-nb]);     /* Xb * yP / (xP * Yb * (X1 - X0)) */
    /* End 1/Z calculation */

    XYcZ_add(Rx[nb], Ry[nb], Rx[1-nb], Ry[1-nb]);

    apply_z(Rx[0], Ry[0], z);

    vli_set(p_result->x, Rx[0]);
    vli_set(p_result->y, Ry[0]);
}

int ecc_make_key(EccPoint *p_publicKey, uint8_t p_privateKey[NUM_ECC_DIGITS])//, uint8_t p_random[NUM_ECC_DIGITS])
{
    /* Make sure the private key is in the range [1, n-1].
       For the supported curves, n is always large enough that we only need to subtract once at most. */
    //vli_set(p_privateKey, p_random);
    if(vli_cmp(curve_n, p_privateKey) != 1)
    {
        vli_sub(p_privateKey, p_privateKey, curve_n);
    }

    if(vli_isZero(p_privateKey))
    {
        return 0; /* The private key cannot be 0 (mod p). */
    }

    EccPoint_mult(p_publicKey, &curve_G, p_privateKey, NULL);
    return 1;
}

int ecc_valid_public_key(EccPoint *p_publicKey)
{
    uint8_t na[NUM_ECC_DIGITS] = {3}; /* -a = 3 */
    uint8_t l_tmp1[NUM_ECC_DIGITS];
    uint8_t l_tmp2[NUM_ECC_DIGITS];

    if(EccPoint_isZero(p_publicKey))
    {
        return 0;
    }

    if(vli_cmp(curve_p, p_publicKey->x) != 1 || vli_cmp(curve_p, p_publicKey->y) != 1)
    {
        return 0;
    }

    vli_modSquare_fast(l_tmp1, p_publicKey->y); /* tmp1 = y^2 */

    vli_modSquare_fast(l_tmp2, p_publicKey->x); /* tmp2 = x^2 */
    vli_modSub(l_tmp2, l_tmp2, na, curve_p); /* tmp2 = x^2 + a = x^2 - 3 */
    vli_modMult_fast(l_tmp2, l_tmp2, p_publicKey->x); /* tmp2 = x^3 + ax */
    vli_modAdd(l_tmp2, l_tmp2, curve_b, curve_p); /* tmp2 = x^3 + ax + b */

    /* Make sure that y^2 == x^3 + ax + b */
    if(vli_cmp(l_tmp1, l_tmp2) != 0)
    {
        return 0;
    }

    return 1;
}

int ecdh_shared_secret(uint8_t p_secret[NUM_ECC_DIGITS], EccPoint *p_publicKey, uint8_t p_privateKey[NUM_ECC_DIGITS], uint8_t p_random[NUM_ECC_DIGITS])
{
    EccPoint l_product;

    EccPoint_mult(&l_product, p_publicKey, p_privateKey, p_random);
    if(EccPoint_isZero(&l_product))
    {
        return 0;
    }

    vli_set(p_secret, l_product.x);

    return 1;
}

#if ECC_ECDSA

/* -------- ECDSA code -------- */

/* Computes p_result = (p_left * p_right) % p_mod. */
void vli_modMult(uint8_t *p_result, uint8_t *p_left, uint8_t *p_right, uint8_t *p_mod)
{
    uint8_t l_product[2 * NUM_ECC_DIGITS];
    uint8_t l_modMultiple[2 * NUM_ECC_DIGITS];
    uint l_digitShift, l_bitShift;
    uint l_productBits;
    uint l_modBits = vli_numBits(p_mod);

    vli_mult(l_product, p_left, p_right);
    l_productBits = vli_numBits(l_product + NUM_ECC_DIGITS);
    if(l_productBits)
    {
        l_productBits += NUM_ECC_DIGITS * 8;
    }
    else
    {
        l_productBits = vli_numBits(l_product);
    }

    if(l_productBits < l_modBits)
    { /* l_product < p_mod. */
        vli_set(p_result, l_product);
        return;
    }

    /* Shift p_mod by (l_leftBits - l_modBits). This multiplies p_mod by the largest
       power of two possible while still resulting in a number less than p_left. */
    vli_clear(l_modMultiple);
    vli_clear(l_modMultiple + NUM_ECC_DIGITS);
    l_digitShift = (l_productBits - l_modBits) / 8;
    l_bitShift = (l_productBits - l_modBits) % 8;
    if(l_bitShift)
    {
        l_modMultiple[l_digitShift + NUM_ECC_DIGITS] = vli_lshift(l_modMultiple + l_digitShift, p_mod, l_bitShift);
    }
    else
    {
        vli_set(l_modMultiple + l_digitShift, p_mod);
    }

    /* Subtract all multiples of p_mod to get the remainder. */
    vli_clear(p_result);
    p_result[0] = 1; /* Use p_result as a temp var to store 1 (for subtraction) */
    while(l_productBits > NUM_ECC_DIGITS * 8 || vli_cmp(l_modMultiple, p_mod) >= 0)
    {
        int l_cmp = vli_cmp(l_modMultiple + NUM_ECC_DIGITS, l_product + NUM_ECC_DIGITS);
        if(l_cmp < 0 || (l_cmp == 0 && vli_cmp(l_modMultiple, l_product) <= 0))
        {
            if(vli_sub(l_product, l_product, l_modMultiple))
            { /* borrow */
                vli_sub(l_product + NUM_ECC_DIGITS, l_product + NUM_ECC_DIGITS, p_result);
            }
            vli_sub(l_product + NUM_ECC_DIGITS, l_product + NUM_ECC_DIGITS, l_modMultiple + NUM_ECC_DIGITS);
        }
        uint8_t l_carry = (l_modMultiple[NUM_ECC_DIGITS] & 0x01) << 7;
        vli_rshift1(l_modMultiple + NUM_ECC_DIGITS);
        vli_rshift1(l_modMultiple);
        l_modMultiple[NUM_ECC_DIGITS-1] |= l_carry;

        --l_productBits;
    }
    vli_set(p_result, l_product);
}

static uint max(uint a, uint b)
{
    return (a > b ? a : b);
}

int ecdsa_sign(uint8_t r[NUM_ECC_DIGITS], uint8_t s[NUM_ECC_DIGITS], uint8_t p_privateKey[NUM_ECC_DIGITS],
    uint8_t p_random[NUM_ECC_DIGITS], uint8_t p_hash[NUM_ECC_DIGITS])
{
    uint8_t k[NUM_ECC_DIGITS];
    EccPoint p;

    if(vli_isZero(p_random))
    { /* The random number must not be 0. */
        return 0;
    }

    vli_set(k, p_random);
    if(vli_cmp(curve_n, k) != 1)
    {
        vli_sub(k, k, curve_n);
    }

    /* tmp = k * G */
    EccPoint_mult(&p, &curve_G, k, NULL);

    /* r = x1 (mod n) */
    vli_set(r, p.x);
    if(vli_cmp(curve_n, r) != 1)
    {
        vli_sub(r, r, curve_n);
    }
    if(vli_isZero(r))
    { /* If r == 0, fail (need a different random number). */
        return 0;
    }

    vli_modMult(s, r, p_privateKey, curve_n); /* s = r*d */
    vli_modAdd(s, p_hash, s, curve_n); /* s = e + r*d */
    vli_modInv(k, k, curve_n); /* k = 1 / k */
    vli_modMult(s, s, k, curve_n); /* s = (e + r*d) / k */

    return 1;
}

int ecdsa_verify(EccPoint *p_publicKey, uint8_t p_hash[NUM_ECC_DIGITS], uint8_t r[NUM_ECC_DIGITS], uint8_t s[NUM_ECC_DIGITS])
{
    uint8_t u1[NUM_ECC_DIGITS], u2[NUM_ECC_DIGITS];
    uint8_t z[NUM_ECC_DIGITS];
    EccPoint l_sum;
    uint8_t rx[NUM_ECC_DIGITS];
    uint8_t ry[NUM_ECC_DIGITS];
    uint8_t tx[NUM_ECC_DIGITS];
    uint8_t ty[NUM_ECC_DIGITS];
    uint8_t tz[NUM_ECC_DIGITS];

    if(vli_isZero(r) || vli_isZero(s))
    { /* r, s must not be 0. */
        return 0;
    }

    if(vli_cmp(curve_n, r) != 1 || vli_cmp(curve_n, s) != 1)
    { /* r, s must be < n. */
        return 0;
    }

    /* Calculate u1 and u2. */
    vli_modInv(z, s, curve_n); /* Z = s^-1 */
    vli_modMult(u1, p_hash, z, curve_n); /* u1 = e/s */
    vli_modMult(u2, r, z, curve_n); /* u2 = r/s */

    /* Calculate l_sum = G + Q. */
    vli_set(l_sum.x, p_publicKey->x);
    vli_set(l_sum.y, p_publicKey->y);
    vli_set(tx, curve_G.x);
    vli_set(ty, curve_G.y);
    vli_modSub(z, l_sum.x, tx, curve_p); /* Z = x2 - x1 */
    XYcZ_add(tx, ty, l_sum.x, l_sum.y);
    vli_modInv(z, z, curve_p); /* Z = 1/Z */
    apply_z(l_sum.x, l_sum.y, z);

    /* Use Shamir's trick to calculate u1*G + u2*Q */
    EccPoint *l_points[4] = {NULL, &curve_G, p_publicKey, &l_sum};
    uint l_numBits = max(vli_numBits(u1), vli_numBits(u2));

    EccPoint *l_point = l_points[(!!vli_testBit(u1, l_numBits-1)) | ((!!vli_testBit(u2, l_numBits-1)) << 1)];
    vli_set(rx, l_point->x);
    vli_set(ry, l_point->y);
    vli_clear(z);
    z[0] = 1;

    int i;
    for(i = l_numBits - 2; i >= 0; --i)
    {
        EccPoint_double_jacobian(rx, ry, z);

        int l_index = (!!vli_testBit(u1, i)) | ((!!vli_testBit(u2, i)) << 1);
        EccPoint *l_point = l_points[l_index];
        if(l_point)
        {
            vli_set(tx, l_point->x);
            vli_set(ty, l_point->y);
            apply_z(tx, ty, z);
            vli_modSub(tz, rx, tx, curve_p); /* Z = x2 - x1 */
            XYcZ_add(tx, ty, rx, ry);
            vli_modMult_fast(z, z, tz);
        }
    }

    vli_modInv(z, z, curve_p); /* Z = 1/Z */
    apply_z(rx, ry, z);

    /* v = x1 (mod n) */
    if(vli_cmp(curve_n, rx) != 1)
    {
        vli_sub(rx, rx, curve_n);
    }

    /* Accept only if v == r. */
    return (vli_cmp(rx, r) == 0);
}

#endif /* ECC_ECDSA */

void ecc_bytes2native(uint8_t p_native[NUM_ECC_DIGITS], uint8_t p_bytes[NUM_ECC_DIGITS*4])
{
    unsigned i;
    for(i=0; i<NUM_ECC_DIGITS; ++i)
    {
        p_native[i] = p_bytes[NUM_ECC_DIGITS-i-1];
    }
}

void ecc_native2bytes(uint8_t p_bytes[NUM_ECC_DIGITS*4], uint8_t p_native[NUM_ECC_DIGITS])
{
    unsigned i;
    for(i=0; i<NUM_ECC_DIGITS; ++i)
    {
        p_bytes[NUM_ECC_DIGITS-i-1] = p_native[i];
    }
}


void vli_swap(const uint8_t *src, uint8_t *dest)
{
	uint8_t temp[NUM_ECC_DIGITS];
    unsigned i;
    for(i=0; i<NUM_ECC_DIGITS; ++i)
    {
        temp[i] = src[NUM_ECC_DIGITS-i-1];
    }
	memcpy(dest, &temp, NUM_ECC_DIGITS);
}


// generate deterministic k according to -http://tools.ietf.org/html/rfc6979
int ecdsa_derive_k(const uint8_t p_privateKey[NUM_ECC_DIGITS], const uint8_t p_hash[NUM_ECC_DIGITS], uint8_t k[NUM_ECC_DIGITS])//bignum256 *secret, const uint8_t *priv_key, const uint8_t *hash)
{
	uint8_t v[NUM_ECC_DIGITS], msg[3*NUM_ECC_DIGITS+1], x_h1[2*NUM_ECC_DIGITS], t[NUM_ECC_DIGITS], z2[NUM_ECC_DIGITS];

	// 2.3.4:
	if(vli_cmp(p_hash, curve_n) >= 0) {
		vli_sub(z2, p_hash, curve_n);
	} else {
		memcpy(z2, p_hash, 32);
	}
	vli_swap(z2, z2);

	// prep for d, f:
	memcpy(x_h1, p_privateKey, 32);
	memcpy(x_h1 + 32, z2, 32);

	memset(v, 0x01, 32); // b
	memset(k, 0x00, 32); // c

	// d:
	memcpy(msg, v, 32);
	msg[32] = 0x00;
	memcpy(msg + 32 + 1, x_h1, 64);
	hmac_sha256(k, 32, msg, 97, k);

	hmac_sha256(k, 32, v, 32, v); // e

	// f:
	memcpy(msg, v, 32);
	msg[32] = 0x01;
	memcpy(msg + 32 + 1, x_h1, 64);
	hmac_sha256(k, 32, msg, 97, k);

	hmac_sha256(k, 32, v, 32, v); // g

	// h:
	while(1) {
		hmac_sha256(k, 32, v, 32, t); // h.2. (tlen always equals qlen)

		vli_swap(t, t);

		// h.3.:
		if(!vli_isZero(t) && vli_cmp(curve_n, t) > 0) { // (0<t<q)
			memcpy(k, t, 32);
			return 1;
		}
		// else
		memcpy(msg, v, 32);
		msg[32] = 0x00;
		hmac_sha256(k, 32, msg, 97, k);
		hmac_sha256(k, 32, v, 32, v);
	}
	return 0;
}