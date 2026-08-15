#ifndef OWT_WINDOW_H
#define OWT_WINDOW_H
#include "owt_base.h"

typedef struct {
    owt_widget_t base;
    char title[128];
    int can_close;
    owt_widget_t *content;
} owt_window_t;

owt_window_t *owt_window_create(const char *title, int x, int y, int w, int h);
void owt_window_set_content(owt_window_t *win, owt_widget_t *content);
void owt_window_draw(owt_window_t *win);

#endif