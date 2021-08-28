#ifndef _DRAW_H
#define _DRAW_H

#include <stdio.h>
#include "gfunc.h"

void draw_char(uint32_t x, uint32_t y, char c, color_t back_color, color_t front_color);
void draw_str(uint32_t x, uint32_t y, char *str, color_t back_color, color_t front_color);

#endif
