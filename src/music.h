// The title music: "Gory Ghost (preview)", a SID tune at $5400, played at
// 50 Hz from a raster IRQ
#ifndef MUSIC_H
#define MUSIC_H

// Starts the tune from the beginning
void music_start(void);

// Stops the tune and gives the IRQ back to the KERNAL; the SID needs
// sound_setup afterwards
void music_stop(void);

#pragma compile("music.c")

#endif
