// The SID: the copters' drone on voice 0, explosions on voice 1
#ifndef SOUND_H
#define SOUND_H

// Sets up both voices, silent
void sound_setup(void);

void sound_drone_on(void);
void sound_drone_off(void);

// Starts the explosion noise over; it sounds until sound_explosion_end, then fades
void sound_explosion(void);
void sound_explosion_end(void);

#pragma compile("sound.c")

#endif
