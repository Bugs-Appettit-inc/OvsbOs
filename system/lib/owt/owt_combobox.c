#include "owt_combobox.h"
#include "owt_draw.h"
#include "owt_theme.h"

static void combobox_draw(owt_widget_t *w) {
    owt_combobox_t *cb = (owt_combobox_t *)w;
    owt_theme_t *t = owt_theme_get();
    
    owt_draw_rect(w->x, w->y, w->w, 24, t->bg_secondary);
    owt_draw_rect(w->x, w->y, w->w, 1, t->border);
    owt_draw_rect(w->x, w->y + 23, w->w, 1, t->border);
    owt_draw_rect(w->x, w->y, 1, 24, t->border);
    owt_draw_rect(w->x + w->w - 1, w->y, 1, 24, t->border);
    
    owt_draw_text(w->x + w->w - 20, w->y + 4, "v", t->text_primary);
    
    if (cb->selected_index >= 0 && cb->selected_index < cb->item_count)
        owt_draw_text(w->x + 4, w->y + 4, cb->items[cb->selected_index], t->text_primary);
    else
        owt_draw_text(w->x + 4, w->y + 4, "Selecione...", t->text_secondary);
    
    if (cb->is_open) {
        int h = cb->item_count * 24;
        owt_draw_rect(w->x, w->y + 24, w->w, h, t->bg_primary);
        for (int i = 0; i < cb->item_count; i++) {
            uint32_t bg = (i == cb->selected_index) ? t->accent : t->bg_secondary;
            owt_draw_rect(w->x, w->y + 24 + i * 24, w->w, 24, bg);
            owt_draw_text(w->x + 4, w->y + 28 + i * 24, cb->items[i], t->text_primary);
        }
    }
}

owt_combobox_t *owt_combobox_create(int x, int y, int w) {
    owt_combobox_t *cb = (owt_combobox_t *)owt_widget_create(x, y, w, 24);
    if (!cb) return 0;
    cb->item_count = 0;
    cb->selected_index = -1;
    cb->is_open = 0;
    cb->on_select = 0;
    cb->base.draw = combobox_draw;
    return cb;
}

void owt_combobox_add_item(owt_combobox_t *cb, const char *item) {
    if (cb->item_count >= 16) return;
    int i = 0;
    while (item[i] && i < 63) { cb->items[cb->item_count][i] = item[i]; i++; }
    cb->items[cb->item_count][i] = 0;
    cb->item_count++;
}

const char *owt_combobox_get_selected(owt_combobox_t *cb) {
    if (cb->selected_index >= 0 && cb->selected_index < cb->item_count)
        return cb->items[cb->selected_index];
    return 0;
}

void owt_combobox_click(owt_combobox_t *cb, int mouse_y) {
    if (!cb->is_open) {
        cb->is_open = 1;
        return;
    }
    int rel_y = mouse_y - cb->base.y - 24;
    int index = rel_y / 24;
    if (index >= 0 && index < cb->item_count) {
        cb->selected_index = index;
        if (cb->on_select) cb->on_select(cb, index);
    }
    cb->is_open = 0;
}