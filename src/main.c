// Copter Duel, (c) 1987 Ronald Mayer, rewritten in C for oscar64
#include <c64/vic.h>
#include <c64/cia.h>
#include "system.h"
#include "title.h"
#include "game.h"

// Code and variables stay out of VIC bank 1 ($4000-$7fff) where the
// screen ($4000), the sprites ($4400) and the character set ($7800) are.
#pragma region(main, 0x0a00, 0x4000, , , {code, data, bss, heap, stack})
#pragma stacksize(1024)
#pragma heapsize(0)

#pragma section(sprites, 0)
#pragma region(sprites, 0x4400, 0x4cc0, , , {sprites})
#pragma section(charset, 0)
#pragma region(charset, 0x7800, 0x8000, , , {charset})

#pragma data(sprites)
__export const char sprite_data[] = {
	#embed "sprites.bin"
};
#pragma data(charset)
__export const char charset_data[] = {
	#embed "charset.bin"
};
#pragma data(data)

// KERNAL variable: 128 = Shift + C= doesn't switch the character set
#define SHIFT_MODE      ((volatile char *)657)

int main(void)
{
	vic.color_back = BLACK;
	vic.color_border = BLACK;

	// VIC bank 1 with the screen at $4000 and the character set at $7800
	*SHIFT_MODE = 128;
	cia2.pra = 2;
	vic.memptr = 15;
	clear_screen();
	vic.spr_enable = 0;
	vic.spr_msbx = 0;
	vic.spr_expand_y = 0;
	vic.spr_expand_x = 0;
	vic.color_back1 = 7;
	vic.color_back2 = 9;

	presents();
	for (;;)
	{
		title();
		game();
	}
	return 0;
}
