#ifndef OWT_LABEL_H
#define OWT_LABEL_H
#include "owt_base.h"

typedef struct {
    owt_widget_t base;
    char text[256];
} owt_label_t;

owt_label_t *owt_label_create(const char *text, int x, int y);
void owt_label_set_text(owt_label_t *lbl, const char *text);

#endif