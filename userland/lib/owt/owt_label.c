#include "owt_label.h"
#include "owt_draw.h"
#include "owt_theme.h"

static void label_draw(owt_widget_t *w) {
    owt_label_t *lbl = (owt_label_t *)w;
    owt_theme_t *t = owt_theme_get();
    owt_draw_text(w->x, w->y, lbl->text, t->text_primary);
}

owt_label_t *owt_label_create(const char *text, int x, int y) {
    owt_label_t *lbl = (owt_label_t *)owt_widget_create(x, y, 200, 20);
    if (!lbl) return 0;
    int i = 0;
    while (text[i] && i < 255) { lbl->text[i] = text[i]; i++; }
    lbl->text[i] = 0;
    lbl->base.draw = label_draw;
    return lbl;
}

void owt_label_set_text(owt_label_t *lbl, const char *text) {
    int i = 0;
    while (text[i] && i < 255) { lbl->text[i] = text[i]; i++; }
    lbl->text[i] = 0;
}