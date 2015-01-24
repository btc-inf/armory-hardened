/*
 * rand.h
 *
 * Created: 05.01.2015 17:52:36
 *  Author: Mo
 */


#ifndef RAND_H_
#define RAND_H_

#include <asf.h>
#include <string.h>
#include <avr/eeprom.h>
#include <stddef.h>
#include <avr/io.h>

void adc_rand_init(void);
void adc_rand(uint8_t *dest, size_t len);

uint32_t rand32(void);
void byteshuffle(const uint8_t *src, const size_t len, uint8_t *dest);

#endif /* RAND_H_ */