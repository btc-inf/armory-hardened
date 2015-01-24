/*
 * rand.c
 *
 * Created: 05.01.2015 17:52:51
 *  Author: Mo
 */
#include "rand.h"

void byteshuffle(const uint8_t *src, const size_t len, uint8_t *dest)
{
	size_t i, j = 0;

	for(i = 0; i < len; i++) {
		if(j != i) {
			dest[i] = dest[j];
		}
		dest[j] = src[i];
		j = (size_t) rand32() % (i+1);
	}
}

uint32_t rand32(void)
{
	uint32_t t;
	adc_rand((uint8_t *) &t, 4);
	return t;
}

void adc_rand(uint8_t *dest, size_t len)
{
	size_t pos = 0;
	uint8_t i;

	memset(dest, 0, len);

	while(pos < len) {
		for(i = 0; i < 8; i++) {
			ADCA.CH0.CTRL |= ADC_CH_START_bm;		// start ADCB channel 0
			while(!ADCA.CH0.INTFLAGS);				// wait for ADC ready flag
			ADCA.CH0.INTFLAGS = ADC_CH_CHIF_bm;		// clear flag (cleared by writing 1)
			dest[pos] |= ((ADCA.CH0.RES & 0x01) ^ ((ADCA.CH0.RES & 0x02) >> 1)) << i;	// XOR lsb and lsb+1 and feed current seed byte
		}
		pos++;
	}
}

void adc_rand_init(void)
{
	ADCA.CTRLA = ADC_ENABLE_bm; // enable
	ADCA.CTRLB = ADC_CONMODE_bm; // no currlim, signed, no freerun, 12 bit
	ADCA.REFCTRL = ADC_REFSEL_INTVCC_gc;
	ADCA.PRESCALER = ADC_PRESCALER_DIV256_gc;

	ADCA.CH0.CTRL = ADC_CH_INPUTMODE_DIFF_gc;
	ADCA.CH0.MUXCTRL = ADC_CH_MUXPOS_PIN6_gc | ADC_CH_MUXNEG_PIN2_gc;
}