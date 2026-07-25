#ifndef OWT_LISTVIEW_H
#define OWT_LISTVIEW_H
#include "owt_base.h"

typedef struct owt_listview {
    owt_widget_t base;
    char items[32][64];
    int item_count;
    int selected_index;
    int scroll_offset;
    void (*on_select)(struct owt_listview *lv, int index);
} owt_listview_t;

owt_listview_t *owt_listview_create(int x, int y, int w, int h);
void owt_listview_add_item(owt_listview_t *lv, const char *item);
void owt_listview_clear(owt_listview_t *lv);
const char *owt_listview_get_selected(owt_listview_t *lv);
void owt_listview_click(owt_listview_t *lv, int mouse_y);

#endif