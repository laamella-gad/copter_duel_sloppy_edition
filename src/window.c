#include "window.h"

#define MAX_WINDOWS     6
#define SAVE_SIZE       2048

// Colour RAM is this far above the screen at $4000
#define COLOUR_OFFSET   0x9800

// Frame characters
#define TOP_LEFT        0x1c
#define TOP_RIGHT       0x1d
#define BOTTOM_LEFT     0x1e
#define BOTTOM_RIGHT    0x1f
#define TOP             0x28
#define BOTTOM          0x29
#define LEFT            0x2a
#define RIGHT           0x2b
#define FILL            0x20

struct Window
{
	volatile char * at;
	char            width, height;
	unsigned        save;
};

static struct Window windows[MAX_WINDOWS];
static char window_count;

// What the windows cover, characters and colours, one window after the other
static char char_save[SAVE_SIZE], colour_save[SAVE_SIZE];
static unsigned save_end;

// Draws one row of a window, saving what it covers
static void draw_row(volatile char * at, char width, char colour, char left, char middle, char right)
{
	for (char column = 0; column <= width; column++)
	{
		char c = column == 0 ? left : column == width ? right : middle;
		char_save[save_end] = at[column];
		colour_save[save_end] = at[column + COLOUR_OFFSET];
		save_end++;
		at[column] = c;
		at[column + COLOUR_OFFSET] = colour;
	}
}

void window_open(volatile char * at, char width, char height, char colour)
{
	if (window_count >= MAX_WINDOWS)
		return;

	struct Window * w = windows + window_count++;
	w->at = at;
	w->width = width;
	w->height = height;
	w->save = save_end;

	draw_row(at, width, colour, TOP_LEFT, TOP, TOP_RIGHT);
	for (char row = 0; row < height; row++)
	{
		at += 40;
		draw_row(at, width, colour, LEFT, FILL, RIGHT);
	}
	draw_row(at + 40, width, colour, BOTTOM_LEFT, BOTTOM, BOTTOM_RIGHT);
}

void window_close(void)
{
	if (!window_count)
		return;

	struct Window * w = windows + --window_count;
	volatile char * at = w->at;
	unsigned i = w->save;

	for (char row = 0; row < w->height + 2; row++)
	{
		for (char column = 0; column <= w->width; column++)
		{
			at[column] = char_save[i];
			at[column + COLOUR_OFFSET] = colour_save[i];
			i++;
		}
		at += 40;
	}
	save_end = w->save;
}
