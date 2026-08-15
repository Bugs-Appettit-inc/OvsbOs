#ifndef OWT_DRAW_H
#define OWT_DRAW_H
#include <stdint.h>
void owt_draw_pixel(int x, int y, uint32_t color);
void owt_draw_rect(int x, int y, int w, int h, uint32_t color);
void owt_draw_text(int x, int y, const char *text, uint32_t color);
void owt_draw_char(int x, int y, char c, uint32_t color);
#endif