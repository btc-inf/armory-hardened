/*
 * utils.h
 *
 */


#ifndef UTILS_H_
#define UTILS_H_

#include <string.h>
#include <avr/io.h>

#include <asf.h>

#include "bitcoin.h"
#include "sha2.h"
#include "hmac.h"
#include "ecc.h"
#include "bytestream.h"

size_t binary_to_easyType16(const uint8_t *src, const size_t len, char *dest);
size_t easyType16_to_binary(const char *src, uint8_t *dest, const size_t destlen);

void makeSixteenBytesEasy(const uint8_t *src, char *dest);
size_t readSixteenEasyBytes(const char *src, uint8_t *dest);

uint8_t securely_erase_file(const char *filename);
uint8_t scan_for_file(const char *s1, const char *s2, char *filename);

#endif /* UTILS_H_ */