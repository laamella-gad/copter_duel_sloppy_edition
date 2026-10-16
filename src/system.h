// The C64 as the game sets it up, and helpers for text, timing and input
#ifndef SYSTEM_H
#define SYSTEM_H

// VIC bank 1: the screen at $4000, the sprites at $4400, the character set at $7800
#define screen          ((volatile char *)0x4000)
#define colour_ram      ((volatile char *)0xd800)
#define sprite_pointers ((volatile char *)0x43f8)

// Colours
#define BLACK       0
#define WHITE       1
#define RED         2
#define GREEN       5
#define YELLOW      7
#define ORANGE      8
#define GREY        12
#define LIGHT_BLUE  14

// Joystick bits, 0 when pressed
#define JOY_UP      0x01
#define JOY_DOWN    0x02
#define JOY_LEFT    0x04
#define JOY_RIGHT   0x08
#define JOY_FIRE    0x10

// Fills the screen with spaces in the colour of the last text, like the KERNAL's clear
void clear_screen(void);

// Writes screen code text at a row and column
void put_text(char row, char column, char colour, const char * s);

// Waits n KERNAL jiffies (1/60 s)
void wait_jiffies(char n);

// A key or a joystick button is pressed
bool key_or_button(void);

#pragma compile("system.c")

#endif
