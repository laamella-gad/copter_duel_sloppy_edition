#include "game.h"
#include "system.h"
#include "sound.h"
#include "window.h"
#include "playfield.h"
#include "panel.h"
#include "player.h"
#include "copter.h"
#include "rocket.h"
#include "explosion.h"
#include <c64/vic.h>
#include <c64/cia.h>

// One pass of the original's round loop took 13136 cycles on average,
// measured in VICE; CIA 2 timer A paces the loop at that rate.
#define PASS_CYCLES     13136

// Sprite blocks: the rocket, and the copters at the start of a game,
// level, yellow to the right and red to the left
#define ROCKET_SPRITE   0x20
#define YELLOW_START    0x16
#define RED_START       0x11

// Sprites: the copters and their rockets in hires, the explosion multicolour,
// all behind the playfield
static void setup_sprites(void)
{
	vic.spr_multi = 1 << EXPLOSION_SPRITE;
	vic.spr_mcolor0 = 7;
	vic.spr_mcolor1 = 9;
	vic.spr_priority = 255;

	sprite_pointers[yellow.sprite] = YELLOW_START;
	sprite_pointers[red.sprite] = RED_START;
	vic.spr_color[yellow.sprite] = yellow.colour;
	vic.spr_color[red.sprite] = red.colour;
	vic.spr_pos[yellow.sprite].x = yellow.home_x;
	vic.spr_pos[red.sprite].x = red.home_x;
	vic.spr_pos[yellow.sprite].y = 225;
	vic.spr_pos[red.sprite].y = 225;

	sprite_pointers[yellow.sprite + 2] = ROCKET_SPRITE;
	sprite_pointers[red.sprite + 2] = ROCKET_SPRITE;
	vic.spr_color[yellow.sprite + 2] = BLACK;
	vic.spr_color[red.sprite + 2] = BLACK;

	sprite_pointers[EXPLOSION_SPRITE] = EXPLOSION_FIRST;
	vic.spr_color[EXPLOSION_SPRITE] = ORANGE;

	vic.spr_sprcol = 0;
	vic.spr_backcol = 0;
	vic.spr_enable = (1 << yellow.sprite) | (1 << red.sprite);
}

// Sprites collide with each other and the background
static void collisions(void)
{
	char sprites = vic.spr_sprcol;
	char background = vic.spr_backcol;

	rocket_wall(&yellow, background & (4 << yellow.sprite));
	rocket_wall(&red, background & (4 << red.sprite));
	copter_wall(&yellow, background & (1 << yellow.sprite));
	copter_wall(&red, background & (1 << red.sprite));
	copters_collide(sprites);
	rocket_hit(&red, &yellow, sprites);
	rocket_hit(&yellow, &red, sprites);
}

// Until a player is out and the crashes are over
static void play_round(void)
{
	copters_start();
	players_reset();
	explosion_reset();
	bonuses_reset();
	sound_drone_on();

	cia2.ta = PASS_CYCLES - 1;
	cia2.cra = 0x11;	// start, continuous
	cia2.icr;

	while (!game_over_flags || yellow.crashed || red.crashed)
	{
		while (!(cia2.icr & 0x01))
			;

		explosion_update();
		copters_gravity();
		copter_control(&yellow);
		copter_control(&red);
		rocket_move(&yellow);
		rocket_move(&red);
		player_tick(&yellow);
		player_tick(&red);
		collisions();
		rocket_bonus(&yellow);
		rocket_bonus(&red);
		bonuses_update();
	}

	cia2.cra = 0;
	sound_drone_off();
	sound_explosion_end();
}

// Who won: a player who knocked the other out, else the one with more points
static void show_winner(void)
{
	char winner = game_over_flags;

	window_open(screen + 242, 26, 10, 3);
	window_open(screen + 283, 24, 8, 6);
	put_text(9, 11, RED, s"game over");

	if (winner != 1 && winner != 2)
		winner = compare_scores();
	if (winner == 2)
		put_text(13, 6, YELLOW, s"yellow copter wins!");
	else if (winner == 1)
		put_text(13, 8, RED, s"red copter wins!");
	else
	{
		put_text(12, 7, WHITE, s"both players have");
		put_text(14, 9, WHITE, s"equal points!");
	}

	vic.spr_enable = 0;
	wait_jiffies(125);
	while (!key_or_button())
		;
	window_close();
	window_close();
	window_close();
}

void game(void)
{
	vic.spr_enable = 0;
	clear_screen();
	vic.color_border = ORANGE;
	vic.color_back = LIGHT_BLUE;
	vic.color_back1 = 11;
	vic.color_back2 = 8;
	vic.ctrl2 = 216;
	playfield_draw();
	setup_sprites();

	play_round();
	show_winner();
}
