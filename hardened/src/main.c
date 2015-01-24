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

#include <asf.h>
#include <string.h>
#include <avr/eeprom.h>
#include <stddef.h>
#include <avr/io.h>

#include "ui.h"
#include "bitcoin.h"
#include "base64.h"
#include "base58.h"
#include "ecc.h"
#include "sha2.h"
#include "rand.h"

#include "utils.h"

#include "bytestream.h"
#include "transaction.h"

uint8_t check_sdcard(void);

extern WLT ui_wallet;

uint8_t check_sdcard(void)
{
	Ctrl_status status = sd_mmc_test_unit_ready(0);
	if (status != CTRL_GOOD) {
		udc_stop();
		_ui_clear();

		gfx_mono_draw_string("ARMORY HARDENED", 19, 5, &sysfont);
		gfx_mono_draw_filled_rect(0, 25, 128, 7, 0);
		if(status == CTRL_NO_PRESENT){
			gfx_mono_draw_string("Insert SD card!", 0, 25, &sysfont);
		} else {
			gfx_mono_draw_string("SD card failure!", 0, 25, &sysfont);
		}

		while(sd_mmc_test_unit_ready(0) != CTRL_GOOD);
		gfx_mono_draw_filled_rect(0, 25, 128, 7, 0);
		gfx_mono_draw_string("Loading", 40, 25, &sysfont);
		return 1; // remounted card
	}
	return 0; // card is fine
}

/* first approach for remote controlling; no function yet:
void usb_check_rx(void)
{
	uint8_t buff[100];
	if(udi_cdc_is_rx_ready()) {
		printf("%u", udi_cdc_get_nb_received_data());
		udi_cdc_read_buf(buff, udi_cdc_get_nb_received_data());
	}
}*/

/*! \brief Main function. Execution starts here.
 */
int main(void)
{
	FATFS fs;

	irq_initialize_vectors();
	cpu_irq_enable();

	/* Initialize ASF services */
	//sleepmgr_init();
	sysclk_init();
	board_init();
	sd_mmc_init();
	rtc_init();

	sysclk_enable_module(SYSCLK_PORT_A, SYSCLK_ADC);

	adc_rand_init();

	_ui_clear();

	gfx_mono_draw_string("ARMORY HARDENED", 19, 5, &sysfont);
	//gfx_mono_draw_horizontal_line(0, 11, 128, 1);
	gfx_mono_draw_filled_rect(0, 25, 128, 7, 0);
	gfx_mono_draw_string("Loading", 40, 25, &sysfont);

	check_sdcard();

//	gfx_mono_draw_string("Loading", 40, 25, &sysfont);

	memset(&fs, 0, sizeof(FATFS));
	f_mount(LUN_ID_SD_MMC_0_MEM, &fs);

	stdio_usb_init();
	//udc_start();
	_ui_init();

	while(true) {
		if(check_sdcard()) {
			udc_start();
			ui_screen_home();
		}

		while(udi_msc_process_trans());
		_ui_scan_buttons();

		//usb_check_rx();
	}
}