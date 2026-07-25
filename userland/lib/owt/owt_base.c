#include "owt_base.h"

owt_widget_t *owt_widget_create(int x, int y, int width, int height) {
    static owt_widget_t widgets[64];
    static int count = 0;
    if (count >= 64) return 0;
    owt_widget_t *widget = &widgets[count++];
    widget->x = x; widget->y = y; widget->w = width; widget->h = height;
    widget->visible = 1;
    widget->enabled = 1;
    widget->child_count = 0;
    widget->draw = 0;
    widget->on_click = 0;
    widget->on_key = 0;
    widget->parent = 0;
    widget->children = 0;
    return widget;
}

void owt_widget_draw(owt_widget_t *w) {
    if (!w || !w->visible) return;
    if (w->draw) w->draw(w);
    for (int i = 0; i < w->child_count; i++)
        owt_widget_draw(w->children[i]);
}

void owt_widget_add_child(owt_widget_t *parent, owt_widget_t *child) {
    if (parent->child_count < 64) {
        static owt_widget_t *child_ptrs[64];
        parent->children = child_ptrs;
        parent->children[parent->child_count++] = child;
        child->parent = parent;
    }
}