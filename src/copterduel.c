// Copter Duel, (c) 1987 Ronald Mayer: the game rewritten in C for oscar64.
// This file is the original's BASIC program (the comments give its line
// numbers); engine.c is its machine code round, window.c its window routines.
#include <c64/vic.h>
#include <c64/cia.h>
#include "engine.h"
#include "window.h"

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

#define POKE(a, v)  (*(volatile char *)(a) = (v))
#define PEEK(a)     (*(volatile char *)(a))

// KERNAL variables
#define KEY_PRESSED     203     // 64: no key
#define TEXT_COLOUR     646
#define SCREEN_PAGE     648
#define JIFFY_LO        0xa2

#define sid_reg ((volatile char *)0xd400)

// PETSCII control codes
#define CR      "\x0d"
#define CLR     "\x93"
#define HOME    "\x13"
#define DOWN    "\x11"
#define RGHT    "\x1d"
#define WHT     "\x05"
#define RED     "\x1c"
#define GRN     "\x1e"
#define LBLU    "\x9a"
#define GRY3    "\x9b"
#define YEL     "\x9e"

#define DOWN4   DOWN DOWN DOWN DOWN
#define RGHT4   RGHT RGHT RGHT RGHT

static void chrout(char c)
{
	__asm {
		lda c
		jsr $ffd2
	}
}

static void print(const char * s)
{
	while (*s)
		chrout(*s++);
}

// Waits the time of a BASIC FOR loop: forj=1to2000:next is 124 jiffies
static void wait_jiffies(char n)
{
	char t = PEEK(JIFFY_LO);
	while ((char)(PEEK(JIFFY_LO) - t) < n)
		;
}

static bool no_key_or_button(void)
{
	return PEEK(KEY_PRESSED) == 64 && cia1.pra == 127 && cia1.prb == 255;
}

// 1079-1109: the playfield and the status panels, yellow top, red bottom:
// 1000 screen codes, then 1000 colours
static const char playfield[2000] = {
	#embed "playfield.bin"
};

// KERNAL screen line link table: bit 7 set where a logical line starts
#define LINE_LINKS  ((volatile char *)0xd9)

static void draw_screen(void)
{
	vic.ctrl1 = 0;
	print(CLR);
	for (unsigned i = 0; i < 1000; i++)
	{
		screen[i] = playfield[i];
		colour_ram[i] = playfield[1000 + i];
	}
	// Printed, the 40 character rows joined into 80 character logical lines
	for (char i = 1; i < 24; i += 2)
		LINE_LINKS[i] &= 0x7f;
	vic.ctrl1 = 27;
}

static void sid_setup(void)
{
	for (char i = 0; i <= 24; i++)
		sid_reg[i] = 0;
	sid_reg[6] = 240;
	sid_reg[4] = 64;
	sid_reg[4] = 65;
	sid_reg[2] = 1;
	sid_reg[3] = 1;
	sid_reg[24] = 15;
	sid_reg[13] = 250;
	sid_reg[11] = 128;
	sid_reg[8] = 10;
}

static const char fade[7] = {11, 12, 15, 1, 15, 12, 11};

#define PRESENTS    HOME DOWN4 DOWN4 DOWN DOWN DOWN RGHT4 RGHT4 RGHT RGHT RGHT p"mr-soft  presents" CR

// 1017-1048: the title, until a key or button is pressed
static void title(void)
{
	print(CLR);
	vic.spr_enable = 0;
	vic.ctrl2 = 200;
	vic.color_back = 0;
	vic.color_border = 0;
	vic.spr_multi = 255;
	vic.spr_mcolor0 = 7;
	vic.spr_mcolor1 = 9;
	for (char i = 0; i <= 5; i++)
		vic.spr_color[i] = 10;
	vic.spr_color[6] = 7;
	vic.spr_color[7] = 7;
	sid_setup();

	for (char i = 0; i <= 3; i++)
	{
		POKE(TEXT_COLOUR, fade[i]);
		print(PRESENTS);
		wait_jiffies(7);
	}
	wait_jiffies(124);
	for (char i = 4; i <= 6; i++)
	{
		POKE(TEXT_COLOUR, fade[i]);
		print(PRESENTS);
		wait_jiffies(7);
	}

	print(CLR);
	for (char i = 0; i <= 5; i++)
	{
		vic.spr_pos[i].y = 80;
		vic.spr_pos[i].x = 110 + i * 25;
	}
	for (char i = 0; i <= 7; i++)
		sprite_pointers[i] = 43 + i;
	vic.spr_pos[6].x = 160;
	vic.spr_pos[7].x = 184;
	vic.spr_pos[6].y = 110;
	vic.spr_pos[7].y = 110;
	vic.spr_enable = 255;
	print(HOME DOWN4 DOWN4 DOWN4 DOWN RGHT4 RGHT4 RGHT LBLU p"@ 1987 by ronald mayer" CR);
	print(DOWN4 RGHT4 GRN p"press any key or button to start" CR);

	while (no_key_or_button())
		;
}

// 0 if equal, else which 5 digit score is higher: 2 yellow (p1), 1 red (p2)
static char compare_scores(void)
{
	for (char i = 0; i < 5; i++)
	{
		if (yellow_score[i] > red_score[i])
			return 2;
		if (red_score[i] > yellow_score[i])
			return 1;
	}
	return 0;
}

// 1049-1078: one game, then who won
static void game(void)
{
	vic.spr_enable = 0;
	print(CLR);
	vic.color_border = 8;
	vic.color_back = 14;
	vic.color_back1 = 11;
	vic.color_back2 = 8;
	vic.ctrl2 = 216;
	draw_screen();

	vic.spr_multi = 240;
	sprite_pointers[0] = 22;
	sprite_pointers[1] = 17;
	sprite_pointers[2] = 32;
	sprite_pointers[3] = 32;
	sprite_pointers[4] = 33;
	sprite_pointers[5] = 33;
	vic.spr_color[0] = 7;
	vic.spr_color[1] = 2;
	vic.spr_color[2] = 0;
	vic.spr_color[3] = 0;
	vic.spr_color[4] = 8;
	vic.spr_color[5] = 8;
	vic.spr_color[6] = 3;
	vic.spr_color[7] = 13;
	vic.spr_priority = 255;
	vic.spr_sprcol = 0;
	vic.spr_backcol = 0;
	vic.spr_pos[0].x = 40;
	vic.spr_pos[1].x = 230;
	vic.spr_pos[0].y = 225;
	vic.spr_pos[1].y = 225;
	vic.spr_enable = 3;

	sid_reg[0] = 190;
	play_round();
	sid_reg[0] = 0;

	char x = game_over_flags;

	window_open(screen + 242, 26, 10, 3);
	window_open(screen + 283, 24, 8, 6);
	print(HOME DOWN4 DOWN4 DOWN RGHT4 RGHT4 RGHT RGHT RGHT RED p"game over" CR);

	if (x != 1 && x != 2)
		x = compare_scores();
	if (x == 2)
		print(DOWN DOWN DOWN RGHT4 RGHT RGHT YEL p"yellow copter wins!" CR);
	else if (x == 1)
		print(DOWN DOWN DOWN RGHT4 RGHT4 RED p"red copter wins!" CR);
	else
	{
		print(DOWN DOWN RGHT4 RGHT RGHT RGHT WHT p"both players have" CR);
		print(RGHT4 RGHT4 RGHT WHT p"equal points!" CR);
	}

	vic.spr_enable = 0;
	wait_jiffies(125);
	while (no_key_or_button())
		;
	window_close();
	window_close();
	window_close();
}

int main(void)
{
	// 1010: shown while the original loaded the engine
	vic.color_back = 0;
	vic.color_border = 0;
	print(CLR DOWN4 DOWN4 DOWN DOWN DOWN RGHT4 RGHT4 RGHT4 RGHT GRY3 p"please wait..." CR);

	// 1014-1016: no case switching, VIC bank 1 with the screen at $4000
	// and the character set at $7800
	chrout(8);
	cia2.pra = 2;
	vic.memptr = 15;
	POKE(SCREEN_PAGE, 64);
	chrout(147);
	vic.spr_enable = 0;
	vic.spr_msbx = 0;
	vic.spr_expand_y = 0;
	vic.spr_expand_x = 0;
	vic.color_back1 = 7;
	vic.color_back2 = 9;

	for (;;)
	{
		title();
		game();
	}
	return 0;
}
