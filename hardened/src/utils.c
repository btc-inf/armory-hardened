/*
 * utils.c
 *
 */
#include "utils.h"


static bool usb_cdc_open = false;

const char easy16chars[] = "asdfghjkwertuion";
//const char easy16chars_obf[] = "ASDFGHJKWERTUION";

size_t binary_to_easyType16(const uint8_t *src, const size_t len, char *dest)
{
	size_t pos = 0, destpos = 0;

	while(pos < len) {
		dest[destpos++] = easy16chars[(src[pos] >> 4)];
		dest[destpos++] = easy16chars[(src[pos++] & 0x0F)];

		if(!(pos % 2) && pos < len) {
			dest[destpos++] = ' ';
		}
	}
	dest[destpos] = '\0';
	return destpos;
}

size_t easyType16_to_binary(const char *src, uint8_t *dest, const size_t destlen)
{
	uint8_t i, ii;
	size_t pos = 0, destpos = 0;

	while(src[pos] != '\0' && destpos < destlen) {
		if(src[pos] != ' ' && src[pos] != '\r' && src[pos] != '\n') {
			for(i = 0; i < 16; i++) {
				if(src[pos] == easy16chars[i]) {
					pos++;
					for(ii = 0; ii < 16; ii++) {
						if(src[pos] == easy16chars[ii]) {
							dest[destpos++] = (i << 4) | ii;
							break;
						}
					}
					break;
				}
			}
		}
		pos++;
	}
	return pos;
}


void makeSixteenBytesEasy(const uint8_t *src, char *dest)
{
	uint8_t cs[4];
	compute_checksum(src, 16, cs);
	binary_to_easyType16(src, 16, dest);
	*(dest + 39) = ' ';
	binary_to_easyType16(cs, 2, dest + 40);
}

size_t readSixteenEasyBytes(const char *src, uint8_t *dest)
{
	uint8_t cs[4], bin[18];
	size_t srcpos = easyType16_to_binary(src, bin, 18);

	compute_checksum(bin, 16, cs);

	if(cs[0] == bin[16] && cs[1] == bin[17]) {
		memcpy(dest, bin, 16);
		return srcpos;
	}
	return 0;
}

uint8_t securely_erase_file(const char *filename)
{
	FIL fp;
	char c[2];

	if(!f_open(&fp, filename, FA_WRITE | FA_READ)) {
		while(!f_eof(&fp)) {
			f_putc('-', &fp);
		}
		f_lseek(&fp, 0);
		while(!f_eof(&fp)) {
			if(!f_gets(c, 2, &fp) || c[0] != '-') return 0;
		}
		f_close(&fp);
		if(f_unlink(filename)) return 0;
		return 1;
	}
	return 0;
}


uint8_t scan_for_file(const char *s1, const char *s2, char *filename)
{
	uint32_t fdatetime_latest = 0;
	uint32_t fdatetime_current = 0;
	//uint16_t fdate = 0;
	//char fname[_MAX_LFN + 1] = "\0";

	FRESULT res;
	FILINFO fno;
	DIR dir;
	// int i;
	char *fn;

	*filename = '\0';

	static char lfn[_MAX_LFN + 1];   /// Buffer to store the LFN
	fno.lfname = lfn;
	fno.lfsize = sizeof lfn;


	res = f_opendir(&dir, "/");                       // Open the directory
	if (res == FR_OK) {
		//i = strlen(path);
		for (;;) {
			res = f_readdir(&dir, &fno);                   // Read a directory item
			if (res != FR_OK || fno.fname[0] == 0) break;  // Break on error or end of dir
			if (fno.fname[0] == '.') continue;             // Ignore dot entry

			fn = *fno.lfname ? fno.lfname : fno.fname;

			if((strstr(fn, s1) && strlen(s2) && strstr(fn, s2)) || (!strlen(s2) && strstr(fn, s1))) {

				fdatetime_current = ((uint32_t) fno.fdate << 16) | fno.ftime;

				if(fdatetime_current > fdatetime_latest) {
					fdatetime_latest = fdatetime_current;
					strcpy(filename, fn);
				}
			}
		}


		if(*filename != '\0') {
			return 1;
		}


	}

	return 0;
}



void usb_cdc_set_dtr(bool b_enable)
{
	usb_cdc_open = b_enable;
}

bool usb_cdc_is_open(void)
{
	return usb_cdc_open;
}