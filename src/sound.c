#include "sound.h"
#include <c64/sid.h>

#define sid_reg ((volatile char *)0xd400)

// The rocket's whoosh: noise from WHOOSH_START falling by WHOOSH_STEP per
// pass to WHOOSH_END, fading out in 300 ms
#define WHOOSH_START    0x3000
#define WHOOSH_STEP     0x0200
#define WHOOSH_END      0x0800

static unsigned whoosh;

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
	sid_reg[13] = 250;
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

// The gate goes off and on again, so the envelope starts over
void sound_explosion(void)
{
	sid.voices[1].ctrl = SID_CTRL_NOISE;
	sid.voices[1].ctrl = SID_CTRL_NOISE | SID_CTRL_GATE;
}

void sound_explosion_end(void)
{
	sid.voices[1].ctrl = SID_CTRL_NOISE;
}

void sound_rocket(void)
{
	whoosh = WHOOSH_START;
	sid.voices[2].ctrl = SID_CTRL_NOISE;
	sid.voices[2].attdec = SID_ATK_2 | SID_DKY_300;
	sid.voices[2].susrel = 0;
	sid.voices[2].freq = whoosh;
	sid.voices[2].ctrl = SID_CTRL_NOISE | SID_CTRL_GATE;
}

void sound_update(void)
{
	if (whoosh > WHOOSH_END)
	{
		whoosh -= WHOOSH_STEP;
		sid.voices[2].freq = whoosh;
	}
}

void sound_round_end(void)
{
	sid.voices[1].ctrl = SID_CTRL_NOISE;
	sid.voices[2].ctrl = SID_CTRL_NOISE;
	whoosh = 0;
}
