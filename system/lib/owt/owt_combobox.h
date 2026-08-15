#ifndef OWT_COMBOBOX_H
#define OWT_COMBOBOX_H
#include "owt_base.h"

typedef struct owt_combobox {
    owt_widget_t base;
    char items[16][64];
    int item_count;
    int selected_index;
    int is_open;
    void (*on_select)(struct owt_combobox *cb, int index);
} owt_combobox_t;

owt_combobox_t *owt_combobox_create(int x, int y, int w);
void owt_combobox_add_item(owt_combobox_t *cb, const char *item);
const char *owt_combobox_get_selected(owt_combobox_t *cb);
void owt_combobox_click(owt_combobox_t *cb, int mouse_y);

#endif