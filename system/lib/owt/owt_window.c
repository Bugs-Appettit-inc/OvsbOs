#include "owt_window.h"
#include "owt_draw.h"
#include "owt_theme.h"

static void window_draw(owt_widget_t *w) {
    owt_window_t *win = (owt_window_t *)w;
    owt_theme_t *t = owt_theme_get();
    
    owt_draw_rect(win->base.x, win->base.y, win->base.w, win->base.h, t->bg_primary);
    owt_draw_rect(win->base.x, win->base.y, win->base.w, 24, t->accent);
    owt_draw_text(win->base.x + 8, win->base.y + 4, win->title, t->text_primary);
    
    owt_draw_rect(win->base.x + win->base.w - 20, win->base.y + 4, 16, 16, 0xFFCC2222);
    owt_draw_text(win->base.x + win->base.w - 16, win->base.y + 3, "x", t->text_primary);
    
    if (win->content)
        owt_widget_draw(win->content);
}

owt_window_t *owt_window_create(const char *title, int x, int y, int w, int h) {
    owt_window_t *win = (owt_window_t *)owt_widget_create(x, y, w, h);
    if (!win) return 0;
    int i = 0;
    while (title[i] && i < 127) { win->title[i] = title[i]; i++; }
    win->title[i] = 0;
    win->base.draw = window_draw;
    return win;
}

void owt_window_set_content(owt_window_t *win, owt_widget_t *content) {
    win->content = content;
    content->parent = (owt_widget_t *)win;
}

void owt_window_draw(owt_window_t *win) {
    owt_widget_draw((owt_widget_t *)win);
}