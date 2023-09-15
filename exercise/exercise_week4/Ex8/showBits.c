// Convert 32-bit values to string
// e.g. "01010101 01100110 10000001 01100110"

#include "bits.h"

char *showBits(uint32 val, char *buf)
{
	int i,j; char ch;
	uint32 bit = 0x80000000;

	i = j = 0;
	while (bit != 0) {
		ch = ((val & bit) != 0) ? '1' : '0';
		buf[i++] = ch;
		j++;
		if (j%8 == 0) buf[i++] = ' ';
		bit = bit >> 1;
	}
	buf[--i] = '\0';
	return buf;
}

// 1st iter : bit == 0x80000000
// 2nd iter : bit == 0x40000000
// 3rd iter : bit == 0x20000000
// 4th iter : bit == 0x10000000
// 5th iter : bit == 0x08000000
