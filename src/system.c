#include "system.h"
#include <c64/cia.h>

#define PEEK(a)         (*(volatile char *)(a))

// KERNAL variables
#define KEY_PRESSED     203     // 64: no key
#define JIFFY_LO        0xa2

static char text_colour = LIGHT_BLUE;

void clear_screen(void)
{
	for (unsigned i = 0; i < 1000; i++)
	{
		screen[i] = ' ';
		colour_ram[i] = text_colour;
	}
}

void put_text(char row, char column, char colour, const char * s)
{
	unsigned o = row * 40 + column;
	text_colour = colour;
	while (*s)
	{
		screen[o] = *s++;
		colour_ram[o] = colour;
		o++;
	}
}

void wait_jiffies(char n)
{
	char t = PEEK(JIFFY_LO);
	while ((char)(PEEK(JIFFY_LO) - t) < n)
		;
}

bool key_or_button(void)
{
	return PEEK(KEY_PRESSED) != 64 || cia1.pra != 127 || cia1.prb != 255;
}
