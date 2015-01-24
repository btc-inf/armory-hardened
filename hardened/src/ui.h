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

#ifndef UI_H_
#define UI_H_

#include <asf.h>
#include <string.h>
#include <avr/eeprom.h>

#include "utils.h"
#include "app_touch.h"
#include "bitcoin.h"
#include "base64.h"
#include "base58.h"
#include "ecc.h"
#include "sha2.h"
#include "bytestream.h"
#include "transaction.h"
#include "rand.h"

void _ui_clear(void);
void _ui_qtb_action(uint8_t dir);
void _ui_set_sw0(void *dest, char label);
void _ui_set_sw1(void *dest, char label);
void _ui_set_qtb(void *dest, uint8_t i_max);

void _ui_init(void);
void _ui_scan_buttons(void);
void _ui_format_amount(const uint64_t val, char *output);

void ui_screen_home(void);
void ui_screen_menu(void);
void ui_screen_menu_list(uint8_t i);

void ui_screen_menu_shuffcode(void);
void ui_screen_menu_shuffcode_list(uint8_t i);
void ui_screen_menu_shuffcode_refresh(void);

void ui_screen_menu_browsetxs(void);
void ui_screen_opentx(void);
void ui_screen_opentx_list(uint8_t i);
void ui_screen_signtx(void);

void ui_screen_menu_exportwlt(void);
void ui_screen_menu_exportwlt_list(uint8_t i);
void ui_screen_menu_exportwlt_erase(void);

void ui_screen_menu_exportwlt_watchonly(void);
void ui_screen_menu_exportwlt_rootkeyfile(void);
void ui_screen_menu_exportwlt_showrootkey(void);
void ui_screen_menu_exportwlt_showrootkey_list(uint8_t i);

void ui_screen_menu_setupwlt(void);
void ui_screen_menu_setupwlt_list(uint8_t i);
void ui_screen_menu_setupwlt_rootkeyfile(void);
void ui_screen_menu_setupwlt_shuffrootkeyfile(void);
void ui_screen_menu_setupwlt_wltfile(void);
void ui_screen_menu_setupwlt_createonchip(void);

void ui_screen_menu_setupwlt_import(void);
void ui_screen_menu_setupwlt_erase(void);

void ui_screen_menu_selectwlt(void);
void ui_screen_menu_selectwlt_list(uint8_t i);
void ui_screen_menu_selectwlt_setact(void);

void ui_screen_menu_changepin(void);

void ui_screen_menu_erasewlt(void);
void ui_screen_menu_erasewlt_erase(void);

#endif /* UI_H_ */