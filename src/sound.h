// The SID: the copters' drone on voice 0, explosions on voice 1, rockets
// being fired and copters touching the background on voice 2
#ifndef SOUND_H
#define SOUND_H

// Sets up the voices, silent
void sound_setup(void);

void sound_drone_on(void);
void sound_drone_off(void);

// Starts the explosion noise over with a hard restart: silent for about
// 53 ms, then it sounds until sound_explosion_end and fades
void sound_explosion(void);
void sound_explosion_end(void);

// Starts the rocket's whoosh over: noise falling in pitch as it fades
void sound_rocket(void);

// A copter touches the background this pass: a low buzz while it lasts
void sound_scrape(void);

// Each pass of a round: a started explosion sounds, the whoosh falls in
// pitch, the buzz starts or stops
void sound_update(void);

// Silences the explosion, the whoosh and the buzz
void sound_round_end(void);

#pragma compile("sound.c")

#endif
