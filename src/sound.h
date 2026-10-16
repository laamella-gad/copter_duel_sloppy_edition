// The SID: the copters' drone on voice 0, explosions on voice 1, rockets
// being fired on voice 2
#ifndef SOUND_H
#define SOUND_H

// Sets up the voices, silent
void sound_setup(void);

void sound_drone_on(void);
void sound_drone_off(void);

// Starts the explosion noise over; it sounds until sound_explosion_end, then fades
void sound_explosion(void);
void sound_explosion_end(void);

// Starts the rocket's whoosh over: noise falling in pitch as it fades
void sound_rocket(void);

// Each pass of a round: the whoosh falls in pitch
void sound_update(void);

// Silences the explosion and the whoosh
void sound_round_end(void);

#pragma compile("sound.c")

#endif
