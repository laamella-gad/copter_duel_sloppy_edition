#include "music.h"
#include <c64/vic.h>

// The tune sits in VIC bank 1 between the sprites and the character set.
// Besides its own memory it uses $ae/$af and $0334-$03ff.
#define MUSIC_INIT      0x5400
#define MUSIC_PLAY      0x5403
#define MUSIC_LINE      250

#pragma section(music, 0)
#pragma region(music, 0x5400, 0x5b83, , , {music})

// The PSID file: a 124 byte header and the load address before the tune
#pragma data(music)
__export const char music_data[] = {
	#embed 1923 126 "gory_ghost.sid"
};
#pragma data(data)

// The raster IRQ plays the tune; the KERNAL's 60 Hz CIA IRQ still scans
// the keyboard
__asm music_irq
{
	lda $d019
	and #$01
	beq music_kernal
	sta $d019
	jsr MUSIC_PLAY
	jmp $ea81
music_kernal:
	jmp $ea31
}

void music_start(void)
{
	__asm { sei }
	__asm
	{
		lda #$00
		jsr MUSIC_INIT
	}
	*(void **)0x0314 = music_irq;
	vic.raster = MUSIC_LINE;
	vic.ctrl1 &= 0x7f;
	vic.intr_ctrl = 0xff;
	vic.intr_enable = 1;
	__asm { cli }
}

void music_stop(void)
{
	__asm { sei }
	vic.intr_enable = 0;
	vic.intr_ctrl = 0xff;
	*(void **)0x0314 = (void *)0xea31;
	__asm { cli }
}
