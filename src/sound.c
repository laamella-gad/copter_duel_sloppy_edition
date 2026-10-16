#include "sound.h"
#include <c64/sid.h>

#define sid_reg ((volatile char *)0xd400)

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
