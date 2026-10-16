#include "sound.h"
#include <c64/sid.h>

#define sid_reg ((volatile char *)0xd400)

// The rocket's whoosh: noise from WHOOSH_START falling by WHOOSH_STEP per
// pass to WHOOSH_END, fading out in 300 ms
#define WHOOSH_START    0x3000
#define WHOOSH_STEP     0x0200
#define WHOOSH_END      0x0800

// A copter touching the background: a low square wave, 70 Hz
#define SCRAPE_FREQ     SID_FREQ_PAL(70)
#define SCRAPE_WIDTH    0x0800

// A bonus picked up: a high square wave bip, 2500 Hz for BIP_PASSES passes
// (about 80 ms)
#define BIP_FREQ        SID_FREQ_PAL(2500)
#define BIP_WIDTH       0x0800
#define BIP_PASSES      6

// Voice 1's envelope: attack 0, decay 0, sustain 15, release 10
#define EXPLOSION_AD    0x00
#define EXPLOSION_SR    0xfa

// Passes the gate stays off in a hard restart: at least 2 frames (40 ms)
#define RESTART_PASSES  4

static unsigned whoosh;
static bool scraping, scrape_on;
static char explosion_restart, bip;

void sound_setup(void)
{
	for (char i = 0; i <= 24; i++)
		sid_reg[i] = 0;

	// Voice 0: pulse, gate on, at frequency 0 until the drone starts
	sid_reg[6] = 240;
	sid_reg[4] = 64;
	sid_reg[4] = 65;
	sid_reg[2] = 1;
	sid_reg[3] = 1;
	sid_reg[24] = 15;

	// Voice 1: noise, sustain 15, release 10
	sid_reg[13] = EXPLOSION_SR;
	sid_reg[11] = SID_CTRL_NOISE;
	sid_reg[8] = 10;
}

void sound_drone_on(void)
{
	sid_reg[0] = 190;
}

void sound_drone_off(void)
{
	sid_reg[0] = 0;
}

// A hard restart: the gate goes off and the envelope registers to 0 for
// RESTART_PASSES passes (about 53 ms), then sound_update sets them back and
// turns the gate on. Gating on again right away doesn't reliably start a
// new attack, especially on the 6581 (the SID's ADSR bug).
void sound_explosion(void)
{
	sid.voices[1].ctrl = SID_CTRL_NOISE;
	sid.voices[1].attdec = 0;
	sid.voices[1].susrel = 0;
	explosion_restart = RESTART_PASSES;
}

void sound_explosion_end(void)
{
	sid.voices[1].ctrl = SID_CTRL_NOISE;
	explosion_restart = 0;
}

void sound_rocket(void)
{
	whoosh = WHOOSH_START;
	scrape_on = false;
	sid.voices[2].ctrl = SID_CTRL_NOISE;
	sid.voices[2].attdec = SID_ATK_2 | SID_DKY_300;
	sid.voices[2].susrel = 0;
	sid.voices[2].freq = whoosh;
	sid.voices[2].ctrl = SID_CTRL_NOISE | SID_CTRL_GATE;
}

void sound_bonus(void)
{
	whoosh = 0;
	scrape_on = false;
	bip = BIP_PASSES;
	sid.voices[2].ctrl = SID_CTRL_RECT;
	sid.voices[2].attdec = 0;
	sid.voices[2].susrel = 0xf0;
	sid.voices[2].freq = BIP_FREQ;
	sid.voices[2].pwm = BIP_WIDTH;
	sid.voices[2].ctrl = SID_CTRL_RECT | SID_CTRL_GATE;
}

void sound_scrape(void)
{
	scraping = true;
}

// Voice 2 plays the bip, else the whoosh while it falls, else the scrape
// while a copter touched the background in the last pass
void sound_update(void)
{
	if (explosion_restart && !--explosion_restart)
	{
		sid.voices[1].attdec = EXPLOSION_AD;
		sid.voices[1].susrel = EXPLOSION_SR;
		sid.voices[1].ctrl = SID_CTRL_NOISE | SID_CTRL_GATE;
	}

	if (bip)
	{
		if (!--bip)
			sid.voices[2].ctrl = SID_CTRL_RECT;
	}
	else if (whoosh > WHOOSH_END)
	{
		whoosh -= WHOOSH_STEP;
		sid.voices[2].freq = whoosh;
	}
	else if (scraping && !scrape_on)
	{
		sid.voices[2].ctrl = SID_CTRL_RECT;
		sid.voices[2].attdec = SID_ATK_2 | SID_DKY_6;
		sid.voices[2].susrel = 0xa0 | SID_DKY_24;
		sid.voices[2].freq = SCRAPE_FREQ;
		sid.voices[2].pwm = SCRAPE_WIDTH;
		sid.voices[2].ctrl = SID_CTRL_RECT | SID_CTRL_GATE;
		scrape_on = true;
	}
	else if (!scraping && scrape_on)
	{
		sid.voices[2].ctrl = SID_CTRL_RECT;
		scrape_on = false;
	}
	scraping = false;
}

void sound_round_end(void)
{
	sid.voices[1].ctrl = SID_CTRL_NOISE;
	sid.voices[1].attdec = EXPLOSION_AD;
	sid.voices[1].susrel = EXPLOSION_SR;
	explosion_restart = 0;
	sid.voices[2].ctrl = SID_CTRL_NOISE;
	whoosh = 0;
	bip = 0;
	scraping = scrape_on = false;
}
