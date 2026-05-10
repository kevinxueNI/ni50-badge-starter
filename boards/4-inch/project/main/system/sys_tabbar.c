/**
 * sys_tabbar.c - bottom tab-first navigation bar for top-level pages
 */
#include "ui/ui.h"

#define TABBAR_H  48

static lv_obj_t *s_tab_buttons[PAGE_COUNT];
static lv_obj_t *s_tab_labels[PAGE_COUNT];

static const char *s_tab_titles[PAGE_COUNT] = {
    "Badge",
    "History",
    "LabVIEW",
    "Egg",
    "Config",
};

lv_obj_t *sys_tabbar_create(lv_obj_t *parent, lv_event_cb_t cb)
{
    lv_obj_t *bar = lv_obj_create(parent);
    lv_obj_set_size(bar, SCREEN_W, TABBAR_H);
    lv_obj_align(bar, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x0B1020), 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_80, 0);
    lv_obj_set_style_border_width(bar, 0, 0);
    lv_obj_set_style_radius(bar, 0, 0);
    lv_obj_set_style_pad_all(bar, 2, 0);
    lv_obj_set_style_pad_column(bar, 4, 0);
    lv_obj_set_style_shadow_width(bar, 0, 0);
    lv_obj_set_layout(bar, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bar, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

    for (int i = 0; i < PAGE_COUNT; i++) {
        lv_obj_t *btn = lv_btn_create(bar);
        lv_obj_set_size(btn, 88, 44);
        lv_obj_set_style_radius(btn, 10, 0);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x141B2D), 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_0, 0);
        lv_obj_set_style_border_width(btn, 0, 0);
        lv_obj_set_style_pad_all(btn, 0, 0);
        lv_obj_set_style_shadow_width(btn, 0, 0);
        lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);

        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, s_tab_titles[i]);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(lbl, COLOR_TEXT_DIM, 0);
        lv_obj_center(lbl);

        s_tab_buttons[i] = btn;
        s_tab_labels[i] = lbl;
    }

    return bar;
}

void sys_tabbar_set_active(lv_obj_t *tabbar, int idx)
{
    (void)tabbar;
    for (int i = 0; i < PAGE_COUNT; i++) {
        bool active = (i == idx);
        lv_obj_set_style_bg_opa(s_tab_buttons[i], active ? LV_OPA_70 : LV_OPA_0, 0);
        lv_obj_set_style_bg_color(s_tab_buttons[i], active ? lv_color_hex(0x1E2740) : lv_color_hex(0x141B2D), 0);
        lv_obj_set_style_text_color(s_tab_labels[i], active ? COLOR_ACCENT_YELLOW : COLOR_TEXT_DIM, 0);
    }
}