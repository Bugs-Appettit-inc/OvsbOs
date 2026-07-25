#include "owt_statusbar.h"
#include "owt_draw.h"
#include "owt_theme.h"

static void statusbar_draw(owt_widget_t *w) {
    owt_statusbar_t *sb = (owt_statusbar_t *)w;
    owt_theme_t *t = owt_theme_get();
    owt_draw_rect(w->x, w->y, w->w, 20, t->bg_secondary);
    owt_draw_rect(w->x, w->y, w->w, 1, t->border);
    owt_draw_text(w->x + 4, w->y + 3, sb->text, t->text_secondary);
}

owt_statusbar_t *owt_statusbar_create(int x, int y, int w) {
    owt_statusbar_t *sb = (owt_statusbar_t *)owt_widget_create(x, y, w, 20);
    if (!sb) return 0;
    sb->text[0] = 'P'; sb->text[1] = 'r'; sb->text[2] = 'o'; sb->text[3] = 'n'; sb->text[4] = 't'; sb->text[5] = 'o'; sb->text[6] = 0;
    sb->base.draw = statusbar_draw;
    return sb;
}

void owt_statusbar_set_text(owt_statusbar_t *sb, const char *text) {
    int i = 0;
    while (text[i] && i < 255) { sb->text[i] = text[i]; i++; }
    sb->text[i] = 0;
}