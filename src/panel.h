// The status panels, yellow's at the top, red's at the bottom: score,
// energy bar, rocket bar and spare copters, all screen characters
#ifndef PANEL_H
#define PANEL_H

#include "system.h"

// Scores: 5 screen digits each
#define yellow_score    (screen + 0x049)
#define red_score       (screen + 0x251)

// Score digits to add 1 to
#define POINTS_1        4
#define POINTS_10       3
#define POINTS_100      2

// Bars: 7 characters of 4 units each, from full to empty
#define BAR_FULL        0x23
#define BAR_EMPTY       0x27
#define BAR_LENGTH      7

void score_add(volatile char * score, signed char digit);

// 0 if the scores are equal, else who has more points: 2 yellow, 1 red
char compare_scores(void);

// Takes a unit from the right of the bar; false if it was empty
bool bar_take(volatile char * bar);

// Gives n units back, from the left
void bar_refill(volatile char * bar, char n);

void bar_fill(volatile char * bar);

// Crosses out a spare copter (the icons are at 0, 2 and 4); false if none was left
bool lose_copter(volatile char * icons);

#pragma compile("panel.c")

#endif
