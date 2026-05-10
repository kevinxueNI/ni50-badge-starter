#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "lvgl.h"

/* ─── 屏幕参数 ─── */
#define SCREEN_W  480
#define SCREEN_H  480

/* ─── 颜色定义 ─── */
#define COLOR_BG_DARK       lv_color_hex(0x1C233B)   /* 深蓝背景 */
#define COLOR_BG_DARKER     lv_color_hex(0x141B2D)   /* 更深底色 */
#define COLOR_ACCENT_YELLOW lv_color_hex(0xF1C40F)   /* NI 经典黄 */
#define COLOR_NI_GREEN      lv_color_hex(0x00A651)   /* NI 经典绿 */
#define COLOR_TEXT_WHITE    lv_color_hex(0xFFFFFF)
#define COLOR_TEXT_DIM      lv_color_hex(0x8B949E)   /* 次要文字 */
#define COLOR_LINE_DIM      lv_color_hex(0x2A3258)   /* 几何线条 */
#define COLOR_CARD_BG       lv_color_hex(0x1E2740)   /* 卡片背景 */
#define COLOR_ACCENT_BLUE   lv_color_hex(0x3498DB)   /* 科技蓝 */

/* ─── 页面枚举 ─── */
typedef enum {
    PAGE_CARD = 0,
    PAGE_MILESTONES,
    PAGE_LABVIEW,
    PAGE_CALENDAR,
    PAGE_CONFIG,
    PAGE_COUNT          /* 编译期页面总数 */
} page_id_t;

/* ─── Tileview 全局引用 (供 Home 按钮使用) ─── */
extern lv_obj_t *g_tileview;

/* ─── 公共入口 ─── */
void ui_init(void);
void ui_set_statusbar_visible(bool visible);
void app_boot_start(void);

/* ─── Router ─── */
void app_router_register_main(lv_obj_t *main_screen, lv_obj_t *tileview, lv_obj_t *home_btn);
bool app_router_is_immersive_active(void);
void app_router_set_home_visible(bool visible);
void app_router_go_tab(page_id_t page, lv_anim_enable_t anim_en);
void app_router_show_main(lv_scr_load_anim_t anim_type, uint32_t time, uint32_t delay, bool auto_del);
void app_router_enter_immersive(lv_obj_t *screen, lv_scr_load_anim_t anim_type, uint32_t time, uint32_t delay, bool auto_del);
void app_router_go_home(void);

/* ─── 页面接口 ─── */
lv_obj_t *page_card_create(lv_obj_t *parent);
lv_obj_t *page_milestones_create(lv_obj_t *parent);
lv_obj_t *page_labview_create(lv_obj_t *parent);
lv_obj_t *page_calendar_create(lv_obj_t *parent);
lv_obj_t *page_easter_egg_create(lv_obj_t *parent);
lv_obj_t *page_config_create(lv_obj_t *parent);

/* ─── Widget 接口 ─── */
lv_obj_t *widget_statusbar_create(lv_obj_t *parent);
void      widget_statusbar_set_visible(bool visible);
void      widget_statusbar_set_battery(int percent);
void      widget_statusbar_set_time(int hour, int min);
int       widget_statusbar_get_hour(void);
int       widget_statusbar_get_min(void);
lv_obj_t *sys_tabbar_create(lv_obj_t *parent, lv_event_cb_t cb);
void      sys_tabbar_set_active(lv_obj_t *tabbar, int idx);
lv_obj_t *widget_pagedots_create(lv_obj_t *parent, int count);
void      widget_pagedots_set_active(lv_obj_t *dots_container, int idx);
lv_obj_t *widget_home_btn_create(lv_obj_t *parent, lv_event_cb_t cb);

/* ─── Easter Egg / Game 接口 ─── */
void easter_egg_show(void);
void easter_egg_exit(void);
void game_trivia_show(void);
