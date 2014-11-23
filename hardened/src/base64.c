/* base64_dec.c */
/*
 *   This file is part of the AVR-Crypto-Lib.
 *   Copyright (C) 2006, 2007, 2008  Daniel Otte (daniel.otte@rub.de)
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


/**
 * base64 decoder (RFC3548)
 * Author: Daniel Otte
 * License: GPLv3
 * 
 * 
 */

#include <stdint.h>
#include "base64.h"

//#include "cli.h"











#if 1
#include <avr/pgmspace.h>

const char base64_alphabet[64] PROGMEM = {
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
	'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
	'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
	'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
	'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
	'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
	'w', 'x', 'y', 'z', '0', '1', '2', '3',
'4', '5', '6', '7', '8', '9', '+', '/' };

static
char bit6toAscii(uint8_t a){
	a &= (uint8_t)0x3F;
	return pgm_read_byte(base64_alphabet+a);
}

#else

static
char bit6toAscii(uint8_t a){
	a &= (uint8_t)0x3F;
	
	if(a<=25){
		return a+'A';
		} else {
		if(a<=51){
			return a-26+'a';
			} else {
			if(a<=61){
				return a-52+'0';
				} else {
				if(a==62){
					return '+';
					} else {
					return '/'; /* a == 63 */
				}
			}
		}
	}
}

#endif

void base64enc(char *dest,const void *src, uint16_t length){
	uint16_t i,j;
	uint8_t a[4];
	for(i=0; i<length/3; ++i){
		a[0]= (((uint8_t*)src)[i*3+0])>>2;
		a[1]= (((((uint8_t*)src)[i*3+0])<<4) | ((((uint8_t*)src)[i*3+1])>>4)) & 0x3F;
		a[2]= (((((uint8_t*)src)[i*3+1])<<2) | ((((uint8_t*)src)[i*3+2])>>6)) & 0x3F;
		a[3]= (((uint8_t*)src)[i*3+2]) & 0x3F;
		for(j=0; j<4; ++j){
			*dest++=bit6toAscii(a[j]);
		}
	}
	/* now we do the rest */
	switch(length%3){
		case 0:
		break;
		case 1:
		a[0]=(((uint8_t*)src)[i*3+0])>>2;
		a[1]=((((uint8_t*)src)[i*3+0])<<4)&0x3F;
		*dest++ = bit6toAscii(a[0]);
		*dest++ = bit6toAscii(a[1]);
		*dest++ = '=';
		*dest++ = '=';
		break;
		case 2:
		a[0]= (((uint8_t*)src)[i*3+0])>>2;
		a[1]= (((((uint8_t*)src)[i*3+0])<<4) | ((((uint8_t*)src)[i*3+1])>>4)) & 0x3F;
		a[2]= ((((uint8_t*)src)[i*3+1])<<2) & 0x3F;
		*dest++ = bit6toAscii(a[0]);
		*dest++ = bit6toAscii(a[1]);
		*dest++ = bit6toAscii(a[2]);
		*dest++ = '=';
		break;
		default: /* this will not happen! */
		break;
	}
	/*  finalize: */
	*dest='\0';
}



















/*
 #define USE_GCC_EXTENSION
*/
#if 1

#ifdef USE_GCC_EXTENSION

static
int ascii2bit6(char a){
	switch(a){
		case 'A'...'Z':
			return a-'A';
		case 'a'...'z':
			return a-'a'+26;
		case '0'...'9':
			return a-'0'+52;
		case '+':
		case '-':
			return 62;
		case '/':
		case '_':
			return 63;
		default:
			return -1;
	}
}

#else

static
uint8_t ascii2bit6(char a){
	int r;
	switch(a>>4){
		case 0x5:
		case 0x4: 
			r=a-'A';
			if(r<0 || r>25){
				return -1;
			} else {
				return r;
			}
		case 0x7:
		case 0x6: 
			r=a-'a';
			if(r<0 || r>25){
				return -1;
			} else {
				return r+26;
			}
			break;
		case 0x3:
			if(a>'9')
				return -1;
			return a-'0'+52;
		default:
			break;	
	}
	switch (a){
		case '+':
		case '-':
			return 62;
		case '/':
		case '_':
			return 63;
		default:
			return 0xff;
	}
}

#endif

#else

static 
uint8_t ascii2bit6(uint8_t a){
	if(a>='A' && a<='Z'){
		return a-'A';
	} else {
		if(a>='a' && a<= 'z'){
			return a-'a'+26;
		} else {
			if(a>='0' && a<='9'){
				return a-'0'+52;
			} else {
				if(a=='+' || a=='-'){
					return 62;
				} else {
					if(a=='/' || a=='_'){
						return 63;
					} else {
						return 0xff;
					}
				}
			}
		}
	}
}

#endif

int base64_binlength(char *str, uint8_t strict){
	int l=0;
	uint8_t term=0;
	for(;;){
		if(*str=='\0')
			break;
		if(*str=='\n' || *str=='\r'){
			str++;
			continue;
		}
		if(*str=='='){
			term++;
			str++;
			if(term==2){
				break;
			}
			continue;
		}
		if(term)
			return -1;
		if(ascii2bit6(*str)==-1){
			if(strict)
				return -1;
		} else {
			l++;
		}
		str++;
	}
	switch(term){
		case 0:
			if(l%4!=0)
				return -1;
			return l/4*3;
		case 1:
			if(l%4!=3)
				return -1;
			return (l+1)/4*3-1;
		case 2:
			if(l%4!=2)
				return -1;
			return (l+2)/4*3-2;
		default:
			return -1;
	}
}

/*
  |543210543210543210543210|
  |765432107654321076543210|

        .      .      .     .
  |54321054|32105432|10543210|
  |76543210|76543210|76543210|

*/

int base64dec(void *dest, const char *b64str, uint8_t strict){
	uint8_t buffer[4];
	uint8_t idx=0;
	uint8_t term=0;
	for(;;){
//		cli_putstr_P(PSTR("\r\n  DBG: got 0x"));
//		cli_hexdump(b64str, 1);
		buffer[idx]= ascii2bit6(*b64str);
//		cli_putstr_P(PSTR(" --> 0x"));
//		cli_hexdump(buffer+idx, 1);
		
		if(buffer[idx]==0xFF){
			if(*b64str=='='){
				term++;
				b64str++;
				if(term==2)
					goto finalize; /* definitly the end */
			}else{
				if(*b64str == '\0'){
					goto finalize; /* definitly the end */
				}else{
					if(*b64str == '\r' || *b64str == '\n' || !(strict)){
						b64str++; /* charcters that we simply ignore */
					}else{
						return -1;
					}
				}
			}
		}else{
			if(term)
				return -1; /* this happens if we get a '=' in the stream */
			idx++;
			b64str++;
		}
		if(idx==4){
			((uint8_t*)dest)[0] = buffer[0]<<2 | buffer[1]>>4;
			((uint8_t*)dest)[1] = buffer[1]<<4 | buffer[2]>>2;
			((uint8_t*)dest)[2] = buffer[2]<<6 | buffer[3];
			dest = (uint8_t*)dest +3;
			idx=0;
		}
	}
  finalize:	
	/* the final touch */
	if(idx==0)
		return 0;
	if(term==1){
		((uint8_t*)dest)[0] = buffer[0]<<2 | buffer[1]>>4;
		((uint8_t*)dest)[1] = buffer[1]<<4 | buffer[2]>>2;			
		return 0;
	}
	if(term==2){
		((uint8_t*)dest)[0] = buffer[0]<<2 | buffer[1]>>4;
		return 0;
	}
	return -1;
}
