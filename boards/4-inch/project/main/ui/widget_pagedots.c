/**
 * widget_pagedots.c — 底部页面圆点指示器
 */
#include "ui.h"

#define DOT_SIZE     10
#define DOT_GAP      8
#define DOT_ACTIVE_COLOR   COLOR_ACCENT_YELLOW
#define DOT_INACTIVE_COLOR lv_color_hex(0x4A5568)

static int dot_count = 0;
static lv_obj_t *dot_objs[8]; /* max 8 pages */

lv_obj_t *widget_pagedots_create(lv_obj_t *parent, int count)
{
    dot_count = count > 8 ? 8 : count;

    /* 容器 */
    lv_obj_t *cont = lv_obj_create(parent);
    int total_w = dot_count * DOT_SIZE + (dot_count - 1) * DOT_GAP;
    lv_obj_set_size(cont, total_w + 16, DOT_SIZE + 12);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_pad_all(cont, 0, 0);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(cont, DOT_GAP, 0);

    for (int i = 0; i < dot_count; i++) {
        lv_obj_t *dot = lv_obj_create(cont);
        lv_obj_set_size(dot, DOT_SIZE, DOT_SIZE);
        lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(dot, DOT_INACTIVE_COLOR, 0);
        lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(dot, 0, 0);
        lv_obj_clear_flag(dot, LV_OBJ_FLAG_SCROLLABLE);
        dot_objs[i] = dot;
    }

    return cont;
}

void widget_pagedots_set_active(lv_obj_t *dots_container, int idx)
{
    (void)dots_container;
    for (int i = 0; i < dot_count; i++) {
        if (i == idx) {
            lv_obj_set_style_bg_color(dot_objs[i], DOT_ACTIVE_COLOR, 0);
            lv_obj_set_size(dot_objs[i], DOT_SIZE + 2, DOT_SIZE + 2);
        } else {
            lv_obj_set_style_bg_color(dot_objs[i], DOT_INACTIVE_COLOR, 0);
            lv_obj_set_size(dot_objs[i], DOT_SIZE, DOT_SIZE);
        }
    }
}
