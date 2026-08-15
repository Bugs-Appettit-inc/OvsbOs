#include "owt_theme.h"

owt_theme_t THEME_DARK = {
    .bg_primary = 0xFF1A1A2E,
    .bg_secondary = 0xFF16213E,
    .bg_button = 0xFF0F3460,
    .bg_button_hover = 0xFF1A5276,
    .bg_button_pressed = 0xFF0A2647,
    .text_primary = 0xFFFFFFFF,
    .text_secondary = 0xFFCCCCCC,
    .accent = 0xFF533483,
    .border = 0xFF333366,
    .border_radius = 6
};

static owt_theme_t *current_theme = &THEME_DARK;

void owt_theme_set(owt_theme_t *theme) { current_theme = theme; }
owt_theme_t *owt_theme_get(void) { return current_theme; }

owt_theme_t THEME_LIGHT = {
    .bg_primary = 0xFFF0F0F0,
    .bg_secondary = 0xFFE0E0E0,
    .bg_button = 0xFFD0D0D0,
    .bg_button_hover = 0xFFC0C0C0,
    .bg_button_pressed = 0xFFB0B0B0,
    .text_primary = 0xFF000000,
    .text_secondary = 0xFF333333,
    .accent = 0xFF0044AA,
    .border = 0xFF999999,
    .border_radius = 6
};