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

#include "ui.h"

WLT ui_wallet;
WLT ui_wallet_tmp;

char ui_fname[100];
BYT ui_armtx_bp;
uint8_t ui_armtx[3072];
char ui_armtx_uniqueid[9];

void (*_ui_sw0_ptr)(void);
void (*_ui_sw1_ptr)(void);
void (*_ui_sw0_lastptr)(void);
void (*_ui_sw1_lastptr)(void);
void (*_ui_qtb_ptr)(uint8_t i);

uint8_t _ui_qtb_i = 0;
uint8_t _ui_qtb_i_max = 0;

void _ui_init(void)
{
	gfx_mono_init();

	app_touch_init();

	tc_enable(&TCC1);
	tc_set_direction(&TCC1, TC_UP);
	tc_write_count(&TCC1, 0);

	ui_screen_home();
}

void _ui_scan_buttons(void)
{

	static bool accept_input = true;
	static bool qtb0_last_state = false;
	static bool qtb1_last_state = false;

	bool qtb_state;

	if( (tc_read_count(&TCC1) > 12000 || tc_is_overflow(&TCC1))) {
		tc_write_clock_source(&TCC1, TC_CLKSEL_OFF_gc); /* 24MHz / 1024 */
		tc_clear_overflow(&TCC1);
		tc_write_count(&TCC1, 0);
		accept_input = true;
	}

	if(accept_input) {
		if(ioport_pin_is_low(GPIO_PUSH_BUTTON_0) && _ui_sw0_ptr) {
			accept_input = false;
		tc_write_clock_source(&TCC1, TC_CLKSEL_DIV1024_gc); /* 24MHz / 1024 */
			_ui_sw0_ptr();
		} else if(ioport_pin_is_low(GPIO_PUSH_BUTTON_1) && _ui_sw1_ptr) {
			accept_input = false;
		tc_write_clock_source(&TCC1, TC_CLKSEL_DIV1024_gc); /* 24MHz / 1024 */
			_ui_sw1_ptr();
		}

		/* Manage frequency sample through QTouch buttons */
		qtb_state = app_touch_check_key_pressed(0);
		if (qtb_state != qtb0_last_state) {
			/* QTouch button have changed */
			qtb0_last_state = qtb_state;
			if (!qtb_state) {
				/* It is a press */
				accept_input = false;
				tc_write_clock_source(&TCC1, TC_CLKSEL_DIV1024_gc); /* 24MHz / 1024 */
				_ui_qtb_action(0);
			}
		}

		qtb_state = app_touch_check_key_pressed(1);
		if (qtb_state != qtb1_last_state) {
			/* QTouch button have changed */
			qtb1_last_state = qtb_state;
			if (!qtb_state) {
				/* It is a press */
				accept_input = false;
				tc_write_clock_source(&TCC1, TC_CLKSEL_DIV1024_gc); /* 24MHz / 1024 */
				_ui_qtb_action(1);
			}
		}
	}
	app_touch_task();
}

void _ui_clear(void)
{
	gfx_mono_init();
	_ui_set_qtb(NULL, 0);
	_ui_set_sw0(NULL, 0);
	_ui_set_sw1(NULL, 0);
}

void _ui_set_qtb(void *dest, uint8_t i_max)
{
	_ui_qtb_ptr = dest;
	_ui_qtb_i = 0;
	_ui_qtb_i_max = i_max;
	if(_ui_qtb_ptr) {
		gfx_mono_draw_string(" QTB>", 48, 25, &sysfont);
		_ui_qtb_ptr(0);
	}
}

void _ui_qtb_action(uint8_t dir)
{
	if(_ui_qtb_ptr) {
		if(dir && _ui_qtb_i < _ui_qtb_i_max) {
			_ui_qtb_ptr(++_ui_qtb_i);
			} else if(!dir && _ui_qtb_i > 0) {
			_ui_qtb_ptr(--_ui_qtb_i);
		}

		if(_ui_qtb_i == 0) {
			gfx_mono_draw_string(" QTB>", 48, 25, &sysfont);
			} else if(_ui_qtb_i == _ui_qtb_i_max) {
			gfx_mono_draw_string("<QTB ", 48, 25, &sysfont);
			} else {
			gfx_mono_draw_string("<QTB>", 48, 25, &sysfont);
		}
	}
}

void _ui_set_sw0(void *dest, char label)
{
	_ui_sw0_lastptr = _ui_sw0_ptr;
	_ui_sw0_ptr = dest;

	if(dest && label) {
		gfx_mono_draw_string("SW0:", 0, 25, &sysfont);
		gfx_mono_draw_char(label, 24, 25, &sysfont);
	}
}

void _ui_set_sw1(void *dest, char label)
{
	_ui_sw1_lastptr = _ui_sw1_ptr;
	_ui_sw1_ptr = dest;

	if(dest && label) {
		gfx_mono_draw_string("SW1:", 96, 25, &sysfont);
		gfx_mono_draw_char(label, 120, 25, &sysfont);
	}
}



void ui_screen_menu(void)
{
	_ui_clear();
	gfx_mono_draw_string("SELECT MENU ITEM:", 0, 0, &sysfont);
	_ui_set_qtb(&ui_screen_menu_list, 5);
	_ui_set_sw0(&ui_screen_home,'B');
}

void ui_screen_menu_list(uint8_t i)
{
	gfx_mono_draw_filled_rect(0, 11, 128, 7, 0);
	if(i == 0) {
		gfx_mono_draw_string("> Select wallet", 0, 11, &sysfont);
		_ui_set_sw1(&ui_screen_menu_selectwlt,'S');
	} else if(i == 1) {
		gfx_mono_draw_string("> Browse Tx's", 0, 11, &sysfont);
		_ui_set_sw1(&ui_screen_menu_browsetxs,'S');
	} else if(i == 2) {
		gfx_mono_draw_string("> Set up wallet", 0, 11, &sysfont);
		_ui_set_sw1(&ui_screen_menu_setupwlt,'S');
	} else if(i == 3) {
		gfx_mono_draw_string("> Export wallet", 0, 11, &sysfont);
		_ui_set_sw1(&ui_screen_menu_exportwlt,'S');
	} else if(i == 4) {
		gfx_mono_draw_string("> Erase wallet", 0, 11, &sysfont);
		_ui_set_sw1(&ui_screen_menu_erasewlt,'S');
	} else if(i == 5) {
		gfx_mono_draw_string("> Change PIN", 0, 11, &sysfont);
		_ui_set_sw1(&ui_screen_menu_changepin,'S');
	}
}

void ui_screen_menu_erasewlt(void)
{
	char b58uniqueid[10], q[21];

	_ui_clear();
	gfx_mono_draw_string("ERASE WALLET:", 0, 0, &sysfont);

	base58_encode(&ui_wallet.uniqueid, 6, b58uniqueid);
	sprintf(q, "Erase %s?", b58uniqueid);
	gfx_mono_draw_string(q, 0, 11, &sysfont);

	_ui_set_sw0(&ui_screen_menu,'N');
	_ui_set_sw1(&ui_screen_menu_erasewlt_erase,'Y');
}

void ui_screen_menu_erasewlt_erase(void)
{
	char b58uniqueid[10], q[21];

	_ui_set_sw1(NULL, 0);
	_ui_set_sw0(NULL, 0);

	gfx_mono_draw_filled_rect(0, 11, 128, 21, 0);
	gfx_mono_draw_string("Erasing wallet...", 0, 11, &sysfont);

	if(armwlt_eep_erase_wallet(0)) {
		base58_encode(&ui_wallet.uniqueid, 6, b58uniqueid);
		memset(&ui_wallet, 0, sizeof(WLT));
		sprintf(q, "Erased %s.", b58uniqueid);
		gfx_mono_draw_filled_rect(0, 11, 128, 7, 0);
		gfx_mono_draw_string(q, 0, 11, &sysfont);
		_ui_set_sw0(&ui_screen_home,'B');

		return;
	}

	gfx_mono_draw_filled_rect(0, 11, 128, 7, 0);
	gfx_mono_draw_string("Erasing failed.", 0, 11, &sysfont);
	_ui_set_sw0(&ui_screen_menu,'B');
}

void ui_screen_menu_setupwlt(void)
{
	_ui_clear();
	gfx_mono_draw_string("WALLET SETUP:", 0, 0, &sysfont);
	_ui_set_qtb(&ui_screen_menu_setupwlt_list, 2);
	if(eeprom_read_word(EEP_WLTCNT)) {
		_ui_set_sw0(&ui_screen_menu,'B');
	} else {
		_ui_set_sw0(&ui_screen_home,'B');
	}
}

void ui_screen_menu_setupwlt_list(uint8_t i)
{
	gfx_mono_draw_filled_rect(0, 11, 128, 7, 0);
	if(i == 0) {

	gfx_mono_draw_string("> Create on-chip", 0, 11, &sysfont);
	_ui_set_sw1(&ui_screen_menu_setupwlt_createonchip,'S');
	} else if(i == 1) {
	gfx_mono_draw_string("> Take wallet file", 0, 11, &sysfont);
	_ui_set_sw1(&ui_screen_menu_setupwlt_wltfile,'S');
	} else if(i == 2) {
	gfx_mono_draw_string("> Take root key file", 0, 11, &sysfont);
	_ui_set_sw1(&ui_screen_menu_setupwlt_rootkeyfile,'S');
	}
}

void ui_screen_menu_setupwlt_createonchip(void)
{
	_ui_clear();
	gfx_mono_draw_string("CREATE WALLET:", 0, 0, &sysfont);
	gfx_mono_draw_string("Not yet supported.", 0, 11, &sysfont);
	_ui_set_sw0(&ui_screen_menu_setupwlt,'B');
}

void ui_screen_menu_setupwlt_wltfile(void)
{
	char b58uniqueid[10], q[21];

	_ui_clear();
	udc_stop();

	gfx_mono_draw_string("WALLET FILE IMPORT:", 0, 0, &sysfont);

	if(!scan_for_file("decrypt.wallet", "", ui_fname) || !armwlt_read_wallet_file(ui_fname, ui_wallet_tmp.rootkey)) {
		gfx_mono_draw_string("No valid data found.", 0, 11, &sysfont);
		udc_start();
		_ui_set_sw0(&ui_screen_menu_setupwlt,'B');
		return;
	}

	gfx_mono_draw_string("Computing wallet...", 0, 11, &sysfont);
	armwlt_create_instance2(&ui_wallet_tmp, WLT_COMPUTE_UNIQUEID);

	gfx_mono_draw_filled_rect(0, 11, 128, 7, 0);
	base58_encode(&ui_wallet_tmp.uniqueid, 6, b58uniqueid);
	sprintf(q, "Import %s?", b58uniqueid);
	gfx_mono_draw_string(q, 0, 11, &sysfont);

	_ui_set_sw1(&ui_screen_menu_setupwlt_import,'Y');
	_ui_set_sw0(&ui_screen_menu_setupwlt_erase,'N');
}

void ui_screen_menu_setupwlt_rootkeyfile(void)
{
	char b58uniqueid[10], q[21];

	_ui_clear();
	udc_stop();

	gfx_mono_draw_string("ROOTKEY FILE IMPORT:", 0, 0, &sysfont);

	if(!scan_for_file("rootkey", "", ui_fname) || !armwlt_read_rootkey_file(ui_fname, ui_wallet_tmp.rootkey)) {
		gfx_mono_draw_string("No valid data found.", 0, 11, &sysfont);
		udc_start();
		_ui_set_sw0(&ui_screen_menu_setupwlt,'B');
		return;
	}

	gfx_mono_draw_string("Computing wallet...", 0, 11, &sysfont);
	armwlt_create_instance2(&ui_wallet_tmp, WLT_COMPUTE_UNIQUEID);

	gfx_mono_draw_filled_rect(0, 11, 128, 7, 0);
	base58_encode(&ui_wallet_tmp.uniqueid, 6, b58uniqueid);
	sprintf(q, "Import %s?", b58uniqueid);
	gfx_mono_draw_string(q, 0, 11, &sysfont);

	_ui_set_sw1(&ui_screen_menu_setupwlt_import,'Y');
	_ui_set_sw0(&ui_screen_menu_setupwlt_erase,'N');
}


void ui_screen_menu_setupwlt_import(void)
{
	char b58uniqueid[10], q[21];

	_ui_set_sw1(NULL, 0);
	_ui_set_sw0(NULL, 0);

	gfx_mono_draw_filled_rect(0, 11, 128, 21, 0);
	gfx_mono_draw_string("Importing wallet...", 0, 11, &sysfont);

	if(armwlt_eep_create_wallet(ui_wallet_tmp.rootkey, ui_wallet_tmp.uniqueid)) {
		base58_encode(&ui_wallet_tmp.uniqueid, 6, b58uniqueid);
		sprintf(q, "Imported %s.", b58uniqueid);
		memcpy(&ui_wallet, &ui_wallet_tmp, sizeof(WLT));
		memset(ui_wallet.rootkey, 0, 32);
		memset(ui_wallet.addrprivkey, 0, 32);
		ui_wallet_tmp.id = 0xFF;
		gfx_mono_draw_filled_rect(0, 11, 128, 7, 0);
		gfx_mono_draw_string(q, 0, 11, &sysfont);
		_ui_set_sw0(&ui_screen_menu_setupwlt_erase,'B');
	} else {
		memset(&ui_wallet_tmp, 0, sizeof(WLT));
		gfx_mono_draw_filled_rect(0, 11, 128, 7, 0);
		gfx_mono_draw_string("Import failed.", 0, 11, &sysfont);
		udc_start();
		_ui_set_sw0(&ui_screen_menu_setupwlt,'B');
	}
}
void ui_screen_menu_setupwlt_erase(void)
{
	_ui_set_sw1(NULL, 0);
	_ui_set_sw0(NULL, 0);

	gfx_mono_draw_filled_rect(0, 11, 128, 21, 0);
	gfx_mono_draw_string("Erasing data...", 0, 11, &sysfont);

	if(securely_erase_file(ui_fname)) {
		udc_start();
		if(ui_wallet_tmp.id) {
			ui_screen_home();
		} else {
			ui_screen_menu_setupwlt();
		}
		memset(&ui_wallet_tmp, 0, sizeof(WLT));
		return;
	}

	memset(&ui_wallet_tmp, 0, sizeof(WLT));

	gfx_mono_draw_filled_rect(0, 11, 128, 7, 0);
	gfx_mono_draw_string("Could not erase file.", 0, 11, &sysfont);

	udc_start();
	_ui_set_sw0(&ui_screen_menu_setupwlt,'B');
}

void ui_screen_menu_exportwlt(void)
{
	_ui_clear();
	gfx_mono_draw_string("WALLET EXPORT:", 0, 0, &sysfont);
	_ui_set_qtb(&ui_screen_menu_exportwlt_list, 2);
	_ui_set_sw0(&ui_screen_menu,'B');
}

void ui_screen_menu_exportwlt_list(uint8_t i)
{
	gfx_mono_draw_filled_rect(0, 11, 128, 7, 0);
	if(i == 0) {

	gfx_mono_draw_string("> Watchonly data file", 0, 11, &sysfont);
	_ui_set_sw1(&ui_screen_menu_exportwlt_watchonly,'S');
	} else if(i == 1) {
	gfx_mono_draw_string("> Rootkey file", 0, 11, &sysfont);
	_ui_set_sw1(&ui_screen_menu_exportwlt_rootkeyfile,'S');
	} else if(i == 2) {
	gfx_mono_draw_string("> Show rootkey", 0, 11, &sysfont);
	_ui_set_sw1(&ui_screen_menu_exportwlt_showrootkey,'S');
	}
}

void ui_screen_menu_exportwlt_rootkeyfile(void)
{
	_ui_clear();
	gfx_mono_draw_string("MAKE ROOTKEY FILE:", 0, 0, &sysfont);
	gfx_mono_draw_string("Disabled.", 0, 11, &sysfont);
	_ui_set_sw0(&ui_screen_menu_exportwlt,'B');
}

void ui_screen_menu_exportwlt_watchonly(void)
{
	char q[21], b58uniqueid[10];

	_ui_clear();

	gfx_mono_draw_string("MAKE WATCHONLY FILE:", 0, 0, &sysfont);
	_ui_set_sw0(&ui_screen_menu_exportwlt_erase,'B');

	if(!armwlt_eep_read_wallet(&ui_wallet)) {
		gfx_mono_draw_string("Memory corrupted.", 0, 11, &sysfont);
		return;
	}

	udc_stop();
	armwlt_build_rootpubkey_file(&ui_wallet);
	udc_start();
	base58_encode(ui_wallet.uniqueid, 6, b58uniqueid);
	sprintf(q, "%s exported.", b58uniqueid);
	gfx_mono_draw_string(q, 0, 11, &sysfont);
}

void ui_screen_menu_exportwlt_showrootkey(void)
{
	_ui_clear();
	gfx_mono_draw_string("SHOW WALLET ROOTKEY:", 0, 0, &sysfont);
	gfx_mono_draw_string("Disabled.", 0, 11, &sysfont);
	_ui_set_sw0(&ui_screen_menu_exportwlt,'B');

	return;

	gfx_mono_draw_string("SHOW WALLET ROOTKEY:", 0, 0, &sysfont);
	_ui_set_sw0(&ui_screen_menu_exportwlt_erase,'B');

	if(!armwlt_eep_read_wallet(&ui_wallet)) {
		gfx_mono_draw_string("Memory corrupted.", 0, 11, &sysfont);
		return;
	}

	_ui_set_qtb(&ui_screen_menu_exportwlt_showrootkey_list, 6);
}

void ui_screen_menu_exportwlt_erase(void)
{
	memset(ui_wallet.rootkey, 0, 32);
	ui_screen_menu_exportwlt();
}

void ui_screen_menu_exportwlt_showrootkey_list(uint8_t i)
{
	char easy16buff[100], disp[15], q[21];

	if(i > 0 && i < 4) {
		makeSixteenBytesEasy(ui_wallet.rootkey, easy16buff);
		memcpy(disp, easy16buff + (i-1)*15, 15);
	} else if(i > 0){
		makeSixteenBytesEasy(ui_wallet.rootkey + 16, easy16buff);
		memcpy(disp, easy16buff + (i-4)*15, 15);
	}

	disp[14] = '\0';

	gfx_mono_draw_filled_rect(0, 11, 128, 7, 0);
	if(i == 0) {
		gfx_mono_draw_string("QTB1: Reveal it!", 0, 11, &sysfont);
	} else {
		sprintf(q, "%u/6: %s", i, disp);
		gfx_mono_draw_string(q, 0, 11, &sysfont);
	}
}

void ui_screen_menu_browsetxs(void)
{
	_ui_clear();
	gfx_mono_draw_string("BROWSE TX'S:", 0, 0, &sysfont);
	gfx_mono_draw_string("Not yet supported.", 0, 11, &sysfont);
	_ui_set_sw0(&ui_screen_menu,'B');
}

void ui_screen_menu_selectwlt(void)
{
	_ui_clear();
	gfx_mono_draw_string("SELECT WALLET:", 0, 0, &sysfont);
	gfx_mono_draw_string("Not yet supported.", 0, 11, &sysfont);
	_ui_set_sw0(&ui_screen_menu,'B');
}


void ui_screen_menu_changepin(void)
{
	_ui_clear();
	gfx_mono_draw_string("CHANGE PIN:", 0, 0, &sysfont);
	gfx_mono_draw_string("Not yet supported.", 0, 11, &sysfont);
	_ui_set_sw0(&ui_screen_menu,'B');
}

void ui_screen_opentx_list(uint8_t i)
{
	gfx_mono_draw_filled_rect(0, 0, 128, 18, 0);
	char q[21];
	char amount[10];
	uint16_t txout;
	uint8_t script[100];
	char addr[35];

	if(i == 0) {
		sprintf(q, "SIGN TX %s:", ui_armtx_uniqueid);
		gfx_mono_draw_string(q,	0, 0, &sysfont);
		gfx_mono_draw_string("QTB1: Show details", 0, 11, &sysfont);
	} else if(i == 1) {
		_ui_format_amount(armtx_get_total_inputvalue(&ui_armtx_bp), amount);
		sprintf(q, "TxIn#: %u (%s)", armtx_get_txin_cnt(&ui_armtx_bp), amount);
		gfx_mono_draw_string(q,	0, 0, &sysfont);

		_ui_format_amount(armtx_get_fee(&ui_armtx_bp), amount);
		sprintf(q, "Fee:   %s", amount);
		gfx_mono_draw_string(q,	0, 11, &sysfont);

	} /*else if(i == 2) {
		_ui_format_amount(armtx_get_fee(&ui_armtx_bp), amount);
		sprintf(q, "Fee: %s", amount);
		gfx_mono_draw_string(q,	0, 11, &sysfont);
	} */else if(i >= 2) {


		txout = _ui_qtb_i_max - i;
	_ui_format_amount(armtx_get_txout_value(&ui_armtx_bp, txout), amount);

		pkscript_to_addr(script, armtx_get_txout_script(&ui_armtx_bp, txout, script), addr);
		addr[18] = '\0';

		sprintf(q, "TxOut %u/%u: %s to", i-1, armtx_get_txout_cnt(&ui_armtx_bp), amount);
		gfx_mono_draw_string(q,	0, 0, &sysfont);



		sprintf(q, "%s...", addr);
		gfx_mono_draw_string(q,	0, 11, &sysfont);
	}
}

void _ui_format_amount(const uint64_t input, char *output)
{
	uint64_t rem, val = input;
	if(val < 100000) { // < 1 milliBTC, using µB notation
		rem = val % 100;
		if(rem >= 50) val += 100;
		val /= 100;
		sprintf(output, "%uuB", (uint16_t) val);
	} else if(val < 100000000) { // < 1 BTC, using mB notation
		rem = val % 100000;
		if(rem >= 50000) val += 100000;
		val /= 100000;
		sprintf(output, "%umB", (uint16_t) val);
	} else { // >= 1 BTC, using B notation
		rem = val % 100000000;
		if(rem >= 50000000) val += 100000000;
		sprintf(output, "%uB", (uint16_t) val);
	}
}


void ui_screen_opentx(void)
{
	if(!b_size(&ui_armtx_bp)) {
		udc_stop();
		if(!(*ui_fname)) {
			armtx_get_latest_unsigned_file(ui_fname);
		}
		armtx_fetch_unsigned_file(ui_fname, ui_armtx_uniqueid, &ui_armtx_bp);
	}

	_ui_clear();

	_ui_set_sw0(&ui_screen_home,'N');
	_ui_set_sw1(&ui_screen_signtx,'Y');
	_ui_set_qtb(&ui_screen_opentx_list, armtx_get_txout_cnt(&ui_armtx_bp)+1);
}

void ui_screen_signtx(void)
{
	_ui_clear();
	char q[21], b58uniqueid[10];
	char wlt_filename[100];
	sprintf(q, "BUILDING TX %s:", ui_armtx_uniqueid);
	gfx_mono_draw_string(q,	0, 10 * 0, &sysfont);

	base58_encode(ui_wallet.uniqueid, 6, b58uniqueid);

	if(!scan_for_file(".watchonly.wallet", b58uniqueid, wlt_filename)) {
		gfx_mono_draw_string("Wallet file missing!", 0, 11, &sysfont);
		_ui_set_sw0(&ui_screen_home,'B');
		return;
	}

	eeprom_busy_wait();
	eeprom_read_block(ui_wallet.rootkey, armwlt_get_eep_privkeypos(ui_wallet.id), 32);

	//armtx_fetch_unsigned_file("armory.unsigned.tx", uniqueid, &armtx_bp);

	if(!armtx_build_signed_file(&ui_armtx_bp, ui_armtx_uniqueid, &ui_wallet, wlt_filename)) {
		memset(ui_wallet.rootkey, 0, 32);
		gfx_mono_draw_filled_rect(0, 11, 128, 7, 0);
		gfx_mono_draw_string("Key not found!", 0, 11, &sysfont);
		_ui_set_sw0(&ui_screen_home,'B');
		return;
	}


	memset(ui_wallet.rootkey, 0, 32);

	gfx_mono_draw_filled_rect(0, 11, 128, 7, 0);
	gfx_mono_draw_string("Signing done!", 0, 11, &sysfont);

	f_unlink((TCHAR *) ui_fname);

	//*ui_fname = '\0';
	b_reopen(&ui_armtx_bp);

	udc_start();
	_ui_set_sw0(&ui_screen_home,'B');
}

void ui_screen_home(void)
{
	char walletid[10];
	_ui_clear();
	*ui_fname = '\0';
	b_open(&ui_armtx_bp, ui_armtx, sizeof(ui_armtx));

	udc_start();

	if(eeprom_read_word(EEP_WLTCNT)) {

		base58_encode(ui_wallet.uniqueid, 6, walletid);

		gfx_mono_draw_string("Wallet:", 0, 0, &sysfont);//PaV7rsYj
		gfx_mono_draw_string(walletid, 48, 0, &sysfont);
		//gfx_mono_draw_string("ARMORY HARDENED",
		//19, 10 * 0, &sysfont);
		gfx_mono_draw_horizontal_line(0, 11, 128, 1);
		gfx_mono_draw_string("SW0: Open latest Tx",
		0, 16, &sysfont);
		gfx_mono_draw_string("SW1: Menu",
		0, 25, &sysfont);

		_ui_set_sw1(&ui_screen_menu, 0);
		_ui_set_sw0(&ui_screen_opentx, 0);
	} else {
		gfx_mono_draw_string("Wallet: None", 0, 0, &sysfont);//PaV7rsYj
		gfx_mono_draw_horizontal_line(0, 11, 128, 1);
		gfx_mono_draw_string("SW1: Set up wallet", 0, 25, &sysfont);

		_ui_set_sw1(&ui_screen_menu_setupwlt, 0);
	}
}