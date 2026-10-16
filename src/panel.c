#include "panel.h"

#define ICON_LOST       0x3d

void score_add(volatile char * score, signed char digit)
{
	for (;;)
	{
		if (score[digit] < '9')
		{
			score[digit]++;
			return;
		}
		score[digit] = '0';
		if (--digit < 0)
			return;
	}
}

char compare_scores(void)
{
	for (char i = 0; i < 5; i++)
	{
		if (yellow_score[i] > red_score[i])
			return 2;
		if (red_score[i] > yellow_score[i])
			return 1;
	}
	return 0;
}

bool bar_take(volatile char * bar)
{
	for (signed char x = BAR_LENGTH - 1; x >= 0; x--)
	{
		if (bar[x] < BAR_EMPTY)
		{
			bar[x]++;
			return true;
		}
	}
	return false;
}

void bar_refill(volatile char * bar, char n)
{
	do
	{
		for (char x = 0; x < BAR_LENGTH; x++)
		{
			if (bar[x] > BAR_FULL)
			{
				bar[x]--;
				break;
			}
		}
	} while (--n);
}

void bar_fill(volatile char * bar)
{
	for (char x = 0; x < BAR_LENGTH; x++)
		bar[x] = BAR_FULL;
}

bool lose_copter(volatile char * icons)
{
	for (signed char x = 4; x >= 0; x -= 2)
	{
		if (icons[x] != ICON_LOST)
		{
			icons[x] = ICON_LOST;
			return true;
		}
	}
	return false;
}
