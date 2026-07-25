#ifndef OWT_BUTTON_H
#define OWT_BUTTON_H
#include "owt_base.h"

typedef struct owt_button {
    owt_widget_t base;
    char text[128];
    int is_pressed;
    int is_hovered;
    void (*on_click)(struct owt_button *btn);
} owt_button_t;

owt_button_t *owt_button_create(const char *text, int x, int y, int w, int h);
void owt_button_set_text(owt_button_t *btn, const char *text);
int owt_button_is_hovered(owt_button_t *btn, int mouse_x, int mouse_y);
void owt_button_click(owt_button_t *btn);

#endif