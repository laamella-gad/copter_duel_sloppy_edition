// Framed screen windows: sysws ($8800) and syswc ($8803) in the original
#ifndef WINDOW_H
#define WINDOW_H

// Opens a window at a screen address, with inner size width x height:
// the frame covers width + 1 columns and height + 2 rows. What it covers is
// saved; up to 6 windows can be open.
void window_open(volatile char * at, char width, char height, char colour);

// Closes the last window opened
void window_close(void);

#pragma compile("window.c")

#endif
