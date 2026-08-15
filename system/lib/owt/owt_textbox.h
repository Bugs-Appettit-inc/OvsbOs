#ifndef OWT_TEXTBOX_H
#define OWT_TEXTBOX_H
#include "owt_base.h"

typedef struct owt_textbox {
    owt_widget_t base;
    char buffer[256];
    int cursor_pos;
    int max_length;
    void (*on_enter)(struct owt_textbox *tb);
} owt_textbox_t;

owt_textbox_t *owt_textbox_create(int x, int y, int w);
void owt_textbox_set_text(owt_textbox_t *tb, const char *text);
const char *owt_textbox_get_text(owt_textbox_t *tb);
void owt_textbox_key(owt_textbox_t *tb, char key);

#endif