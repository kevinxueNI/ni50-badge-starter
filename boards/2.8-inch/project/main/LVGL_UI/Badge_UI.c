/**
 * Badge_UI.c — NI 50th & LabVIEW 40th 纪念电子胸卡 UI
 *
 * 四页横向 tileview：
 *   Page 0: 名片主页（头像 / 姓名 / 部门 / 周年 Logo）
 *   Page 1: 个人卡通图
 *   Page 2: 记忆翻牌游戏
 *   Page 3: 2048 游戏
 */

#include "Badge_UI.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <dirent.h>
#include <sys/stat.h>
#include "esp_log.h"
#include "esp_random.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "card_images.h"
#include "QMI8658.h"
#include "game_match3.h"
#include "page_config.h"
#include "config_server.h"
#include "page_settings.h"
#include "page_calendar.h"
#include "page_easter_egg.h"
#include "page_gallery.h"

/* 外部图片资源声明 */
extern const lv_img_dsc_t img_avatar;
extern const lv_img_dsc_t img_ni5040;
extern const lv_img_dsc_t img_bg_pattern;
extern const lv_img_dsc_t img_cartoon;

static const char *TAG = "BadgeUI";

/* Global reference to main screen (used by settings/easter egg to return) */
lv_obj_t *badge_main_screen = NULL;

/* ── 全局 LVGL 控件句柄 ── */
static lv_obj_t *tileview       = NULL;

#ifdef CONFIG_PAGE_CARD
static lv_obj_t *tile_card      = NULL;
#endif
#ifdef CONFIG_PAGE_CALENDAR
static lv_obj_t *tile_calendar  = NULL;
#endif
#ifdef CONFIG_PAGE_MILESTONE
static lv_obj_t *tile_milestone = NULL;
#endif
#ifdef CONFIG_PAGE_HISTORY
static lv_obj_t *tile_history   = NULL;
#endif
#ifdef CONFIG_PAGE_DAQ
static lv_obj_t *tile_daq       = NULL;
#endif
#ifdef CONFIG_PAGE_IMPACT
static lv_obj_t *tile_impact    = NULL;
#endif
#ifdef CONFIG_PAGE_IDENTITY
static lv_obj_t *tile_identity  = NULL;
#endif
#ifdef CONFIG_PAGE_GAME
static lv_obj_t *tile_game      = NULL;
#endif
#ifdef CONFIG_PAGE_2048
static lv_obj_t *tile_2048      = NULL;
#endif
#ifdef CONFIG_PAGE_PLAYER
static lv_obj_t *tile_player    = NULL;
#endif
#ifdef CONFIG_PAGE_MATCH3
static lv_obj_t *tile_match3    = NULL;
#endif
#ifdef CONFIG_PAGE_CONFIG
static lv_obj_t *tile_config    = NULL;
#endif
#ifdef CONFIG_PAGE_GALLERY
static lv_obj_t *tile_gallery   = NULL;
#endif

/* 状态栏控件 */
static lv_obj_t *lbl_time      = NULL;
static lv_obj_t *lbl_battery   = NULL;
static lv_obj_t *lbl_card_name = NULL;  /* name label on card page for animation */

/* 页码指示 — 编译期计算页面总数 */
static int num_pages = 0;  /* computed at runtime in Badge_UI_Init */
#define MAX_PAGES 12
static lv_obj_t *indicator_dots[MAX_PAGES];
static int       current_page = 0;
#ifdef CONFIG_PAGE_MATCH3
static bool      match3_loaded = false;
#endif

/* Forward declarations */
static void settings_btn_cb(lv_event_t *e);
static void name_press_cb(lv_event_t *e);

/* ── Home button (appears on every non-card page) ── */
static void home_btn_cb(lv_event_t *e)
{
    (void)e;
    settings_touch_activity();
    lv_obj_set_tile_id(tileview, 0, 0, LV_ANIM_ON);
}

static void create_home_btn(lv_obj_t *parent)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 46, 20);
    lv_obj_set_pos(btn, 6, 6);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x30363D), 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_80, 0);
    lv_obj_set_style_radius(btn, 10, 0);
    lv_obj_set_style_pad_all(btn, 0, 0);
    lv_obj_add_event_cb(btn, home_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, LV_SYMBOL_HOME);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
    lv_obj_center(lbl);
}

static void build_lazy_placeholder(lv_obj_t *parent, const char *title, const char *hint)
{
    lv_obj_set_style_bg_color(parent, COLOR_BG_DARK, 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

    lv_obj_t *lbl_title = lv_label_create(parent);
    lv_label_set_text(lbl_title, title);
    lv_obj_set_style_text_color(lbl_title, COLOR_ACCENT_YELLOW, 0);
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_18, 0);
    lv_obj_align(lbl_title, LV_ALIGN_CENTER, 0, -12);

    lv_obj_t *lbl_hint = lv_label_create(parent);
    lv_label_set_text(lbl_hint, hint);
    lv_obj_set_style_text_color(lbl_hint, lv_color_hex(0x8B949E), 0);
    lv_obj_set_style_text_font(lbl_hint, &lv_font_montserrat_12, 0);
    lv_obj_align(lbl_hint, LV_ALIGN_CENTER, 0, 18);
}

/* ═══════════════════════════════════════════════════════════════
 *  背景：NI50 Pattern 图片铺满整页
 * ═══════════════════════════════════════════════════════════════ */
static void draw_geometric_bg(lv_obj_t *parent)
{
    /* 深蓝底色兜底 */
    lv_obj_set_style_bg_color(parent, COLOR_BG_DARK, 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

    /* 铺满背景图 */
    lv_obj_t *bg_img = lv_img_create(parent);
    lv_img_set_src(bg_img, &img_bg_pattern);
    lv_obj_align(bg_img, LV_ALIGN_TOP_LEFT, 0, 0);
}

/* ═══════════════════════════════════════════════════════════════
 *  状态栏（半透明黑底，时间 + 电池）
 * ═══════════════════════════════════════════════════════════════ */
static void create_status_bar(lv_obj_t *parent)
{
    lv_obj_t *bar = lv_obj_create(parent);
    lv_obj_set_size(bar, SCREEN_W, 28);
    lv_obj_align(bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(bar, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_40, 0);
    lv_obj_set_style_border_width(bar, 0, 0);
    lv_obj_set_style_radius(bar, 0, 0);
    lv_obj_set_style_pad_all(bar, 0, 0);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

    /* 时间 */
    lbl_time = lv_label_create(bar);
    lv_label_set_text(lbl_time, "00:00");
    lv_obj_set_style_text_color(lbl_time, COLOR_TEXT_WHITE, 0);
    lv_obj_set_style_text_font(lbl_time, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_time, LV_ALIGN_LEFT_MID, 8, 0);

    /* 电池 */
    lbl_battery = lv_label_create(bar);
    lv_label_set_text(lbl_battery, LV_SYMBOL_BATTERY_FULL);
    lv_obj_set_style_text_color(lbl_battery, COLOR_NI_GREEN, 0);
    lv_obj_set_style_text_font(lbl_battery, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_battery, LV_ALIGN_RIGHT_MID, -8, 0);

    /* 设置按钮 (电池左侧) */
    lv_obj_t *btn_settings = lv_label_create(bar);
    lv_label_set_text(btn_settings, LV_SYMBOL_SETTINGS);
    lv_obj_set_style_text_color(btn_settings, COLOR_TEXT_WHITE, 0);
    lv_obj_set_style_text_font(btn_settings, &lv_font_montserrat_14, 0);
    lv_obj_align(btn_settings, LV_ALIGN_RIGHT_MID, -30, 0);
    lv_obj_add_flag(btn_settings, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(btn_settings, settings_btn_cb, LV_EVENT_CLICKED, NULL);
}

/* 定时更新状态栏 */
static void status_bar_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    /* 更新时间 */
    char buf[16];
    snprintf(buf, sizeof(buf), "%02d:%02d", datetime.hour, datetime.minute);
    lv_label_set_text(lbl_time, buf);

    /* 更新电池图标 */
    float v = BAT_analogVolts;
    if (v > 4.0f)       lv_label_set_text(lbl_battery, LV_SYMBOL_BATTERY_FULL);
    else if (v > 3.7f)  lv_label_set_text(lbl_battery, LV_SYMBOL_BATTERY_3);
    else if (v > 3.5f)  lv_label_set_text(lbl_battery, LV_SYMBOL_BATTERY_2);
    else if (v > 3.3f)  lv_label_set_text(lbl_battery, LV_SYMBOL_BATTERY_1);
    else                 lv_label_set_text(lbl_battery, LV_SYMBOL_BATTERY_EMPTY);

    /* Auto-sleep check */
    settings_sleep_check();
}

/* ═══════════════════════════════════════════════════════════════
 *  头像区域（用 LVGL 画一个带黄色边框的圆形占位）
 * ═══════════════════════════════════════════════════════════════ */
static void create_avatar(lv_obj_t *parent)
{
    /* 头像外圈（黄色圆环） */
    lv_obj_t *ring = lv_obj_create(parent);
    lv_obj_set_size(ring, 96, 96);
    lv_obj_align(ring, LV_ALIGN_TOP_MID, 0, 40);
    lv_obj_set_style_radius(ring, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(ring, COLOR_ACCENT_YELLOW, 0);
    lv_obj_set_style_bg_opa(ring, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(ring, 0, 0);
    lv_obj_set_style_pad_all(ring, 4, 0);
    lv_obj_clear_flag(ring, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_clip_corner(ring, true, 0);

    /* 真实头像图片 */
    lv_obj_t *avatar_img = lv_img_create(ring);

    /* Try loading custom avatar from SD card (raw RGB565, 88x88) */
    static lv_img_dsc_t custom_avatar;
    static uint8_t *avatar_buf = NULL;
    if (config_has_custom_avatar()) {
        FILE *f = fopen(CFG_AVATAR_PATH, "rb");
        if (f) {
            fseek(f, 0, SEEK_END);
            long sz = ftell(f);
            fseek(f, 0, SEEK_SET);
            if (sz == 88 * 88 * 2) {
                avatar_buf = heap_caps_malloc(sz, MALLOC_CAP_SPIRAM);
                if (avatar_buf) {
                    fread(avatar_buf, 1, sz, f);
                    custom_avatar.header.always_zero = 0;
                    custom_avatar.header.w = 88;
                    custom_avatar.header.h = 88;
                    custom_avatar.header.cf = LV_IMG_CF_TRUE_COLOR;
                    custom_avatar.data_size = sz;
                    custom_avatar.data = avatar_buf;
                    lv_img_set_src(avatar_img, &custom_avatar);
                    fclose(f);
                    goto avatar_done;
                }
            }
            fclose(f);
        }
    }
    /* Fallback to built-in avatar */
    lv_img_set_src(avatar_img, &img_avatar);

avatar_done:
    lv_obj_center(avatar_img);
    lv_obj_set_style_radius(avatar_img, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_clip_corner(avatar_img, true, 0);
}

/* ═══════════════════════════════════════════════════════════════
 *  姓名 + 部门胶囊标签
 * ═══════════════════════════════════════════════════════════════ */
static void create_name_block(lv_obj_t *parent)
{
    /* Try loading custom profile from SD card */
    badge_profile_t profile = {0};
    const char *disp_name = "Kevin Xue";
    const char *disp_dept = "NI China Innovation Center";
    if (config_load_profile(&profile) == ESP_OK) {
        if (profile.name[0]) disp_name = profile.name;
        if (profile.dept[0]) disp_dept = profile.dept;
    }

    /* 姓名 */
    lv_obj_t *name = lv_label_create(parent);
    lv_label_set_text(name, disp_name);
    lv_obj_set_style_text_color(name, COLOR_TEXT_WHITE, 0);
    lv_obj_set_style_text_font(name, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_letter_space(name, 1, 0);
    lv_obj_align(name, LV_ALIGN_TOP_MID, 0, 148);
    lv_obj_set_style_transform_pivot_x(name, lv_pct(50), 0);
    lv_obj_set_style_transform_pivot_y(name, lv_pct(50), 0);
    lbl_card_name = name;
    /* Easter egg: long press 2s on name enters hidden game */
    lv_obj_add_flag(name, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_ext_click_area(name, 10);
    lv_obj_add_event_cb(name, name_press_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(name, name_press_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(name, name_press_cb, LV_EVENT_PRESS_LOST, NULL);

    /* 部门胶囊标签（半透明黑底 + 圆角 + 黄色文字） */
    lv_obj_t *pill = lv_obj_create(parent);
    lv_obj_set_size(pill, 220, 28);
    lv_obj_align(pill, LV_ALIGN_TOP_MID, 0, 176);
    lv_obj_set_style_bg_color(pill, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(pill, LV_OPA_50, 0);
    lv_obj_set_style_radius(pill, 14, 0);
    lv_obj_set_style_border_width(pill, 0, 0);
    lv_obj_set_style_pad_all(pill, 0, 0);
    lv_obj_clear_flag(pill, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *dept = lv_label_create(pill);
    lv_label_set_text(dept, disp_dept);
    lv_label_set_long_mode(dept, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_color(dept, COLOR_ACCENT_YELLOW, 0);
    lv_obj_set_style_text_font(dept, &lv_font_montserrat_12, 0);
    lv_obj_center(dept);
}

/* ═══════════════════════════════════════════════════════════════
 *  底部周年 Logo（MVP 用文字方块占位，后续替换为图片 C 数组）
 * ═══════════════════════════════════════════════════════════════ */
static void create_anniversary_logo(lv_obj_t *parent)
{
    /* NI 50 + LabVIEW 40 周年 logo 图片（缩小避免与胶囊重叠） */
    lv_obj_t *logo_img = lv_img_create(parent);
    lv_img_set_src(logo_img, &img_ni5040);
    lv_obj_align(logo_img, LV_ALIGN_BOTTOM_MID, 0, -16);
}

/* ═══════════════════════════════════════════════════════════════
 *  页码指示点
 * ═══════════════════════════════════════════════════════════════ */
static void update_indicators(int active)
{
    for (int i = 0; i < num_pages; i++) {
        if (i == active) {
            lv_obj_set_style_bg_color(indicator_dots[i], COLOR_ACCENT_YELLOW, 0);
            lv_obj_set_style_bg_opa(indicator_dots[i], LV_OPA_COVER, 0);
            lv_obj_set_size(indicator_dots[i], 8, 8);
        } else {
            lv_obj_set_style_bg_color(indicator_dots[i], COLOR_TEXT_WHITE, 0);
            lv_obj_set_style_bg_opa(indicator_dots[i], LV_OPA_40, 0);
            lv_obj_set_size(indicator_dots[i], 7, 7);
        }
    }
    current_page = active;

    /* Show perf monitor label only on DAQ page (index 4 with current config) */
#ifdef CONFIG_LV_USE_PERF_MONITOR
    lv_obj_t *sys_layer = lv_layer_sys();
    uint32_t cnt = lv_obj_get_child_cnt(sys_layer);
    for (uint32_t i = 0; i < cnt; i++) {
        lv_obj_t *child = lv_obj_get_child(sys_layer, i);
        if (active == 4) {
            lv_obj_clear_flag(child, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(child, LV_OBJ_FLAG_HIDDEN);
        }
    }
#endif
}

static void create_page_indicators(lv_obj_t *parent)
{
    int gap = 11;
    int offset = -((num_pages - 1) * gap) / 2;  /* 居中偏移 */
    for (int i = 0; i < num_pages; i++) {
        indicator_dots[i] = lv_obj_create(parent);
        lv_obj_set_style_radius(indicator_dots[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_border_width(indicator_dots[i], 0, 0);
        lv_obj_set_style_pad_all(indicator_dots[i], 0, 0);
        lv_obj_clear_flag(indicator_dots[i], LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_align(indicator_dots[i], LV_ALIGN_BOTTOM_MID, offset + i * gap, -10);
    }
    update_indicators(0);
}

/* tileview 页面切换回调 */
static void tileview_event_cb(lv_event_t *e)
{
    /* Any interaction resets sleep timer */
    settings_touch_activity();

    lv_obj_t *tv = lv_event_get_target(e);
    lv_obj_t *tile = lv_tileview_get_tile_act(tv);

    /* 遍历所有已注册 tile，找到匹配的页码 */
    int idx = 0;
#ifdef CONFIG_PAGE_CARD
    if (tile == tile_card) { update_indicators(idx); return; } idx++;
#endif
#ifdef CONFIG_PAGE_HISTORY
    if (tile == tile_history) { update_indicators(idx); return; } idx++;
#endif
#ifdef CONFIG_PAGE_MILESTONE
    if (tile == tile_milestone) { update_indicators(idx); return; } idx++;
#endif
#ifdef CONFIG_PAGE_GALLERY
    if (tile == tile_gallery) { update_indicators(idx); return; } idx++;
#endif
#ifdef CONFIG_PAGE_CALENDAR
    if (tile == tile_calendar) { update_indicators(idx); return; } idx++;
#endif
#ifdef CONFIG_PAGE_DAQ
    if (tile == tile_daq) { update_indicators(idx); return; } idx++;
#endif
#ifdef CONFIG_PAGE_IMPACT
    if (tile == tile_impact) { update_indicators(idx); return; } idx++;
#endif
#ifdef CONFIG_PAGE_IDENTITY
    if (tile == tile_identity) { update_indicators(idx); return; } idx++;
#endif
#ifdef CONFIG_PAGE_GAME
    if (tile == tile_game) { update_indicators(idx); return; } idx++;
#endif
#ifdef CONFIG_PAGE_2048
    if (tile == tile_2048) { update_indicators(idx); return; } idx++;
#endif
#ifdef CONFIG_PAGE_PLAYER
    if (tile == tile_player) { update_indicators(idx); return; } idx++;
#endif
#ifdef CONFIG_PAGE_MATCH3
    if (tile == tile_match3) {
        update_indicators(idx);
        if (!match3_loaded) {
            lv_obj_clean(tile_match3);
            build_page_match3(tile_match3);
            match3_loaded = true;
        }
        return;
    }
    idx++;
#endif
#ifdef CONFIG_PAGE_CONFIG
    if (tile == tile_config) { update_indicators(idx); return; } idx++;
#endif
    (void)idx;
}

/* ═══════════════════════════════════════════════════════════════
 *  Page 1 — NI 历史里程碑 (Timeline + 数据流光动画)
 * ═══════════════════════════════════════════════════════════════ */
#define TIMELINE_X       40
#define TIMELINE_Y_START 72
#define TIMELINE_Y_END   285
#define NODE_COUNT       5

typedef struct {
    const char *year;
    const char *event_en;
    const char *event_zh;
    uint32_t   color;  /* year color */
} milestone_t;

static const milestone_t milestones[NODE_COUNT] = {
    {"1976", "NI Founded",    "GPIB",         0xF1C40F},
    {"1986", "LabVIEW 1.0",  "Test & Meas.", 0x00A651},
    {"1997", "PXI Standard", "PXI",          0x00A651},
    {"2023", "Join Emerson", "Emerson Era",  0x00A651},
    {"2026", "50th Anniv.",  "50 Years",     0xF1C40F},
};

/* 数据流光点 */
static lv_obj_t *dataflow_dot = NULL;

static void dataflow_anim_y_cb(void *obj, int32_t v)
{
    lv_obj_set_y((lv_obj_t *)obj, v);
}

static void build_page_milestone(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0D1117), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

    /* 标题 */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "MILESTONES");
    lv_obj_set_style_text_color(title, lv_color_hex(0xC9D1D9), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_letter_space(title, 2, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 12);

    /* 副标题 */
    lv_obj_t *sub = lv_label_create(parent);
    lv_label_set_text(sub, "NI History");
    lv_obj_set_style_text_color(sub, lv_color_hex(0x8B949E), 0);
    lv_obj_set_style_text_font(sub, &lv_font_montserrat_10, 0);
    lv_obj_align(sub, LV_ALIGN_TOP_MID, 0, 30);

    /* 分隔线 */
    lv_obj_t *sep = lv_obj_create(parent);
    lv_obj_set_size(sep, 80, 1);
    lv_obj_align(sep, LV_ALIGN_TOP_MID, 0, 46);
    lv_obj_set_style_bg_color(sep, lv_color_hex(0x30363D), 0);
    lv_obj_set_style_bg_opa(sep, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(sep, 0, 0);
    lv_obj_clear_flag(sep, LV_OBJ_FLAG_SCROLLABLE);

    /* 时间轴主线 (发光效果 - 外层亮光) */
    lv_obj_t *glow = lv_obj_create(parent);
    lv_obj_set_size(glow, 6, TIMELINE_Y_END - TIMELINE_Y_START);
    lv_obj_set_pos(glow, TIMELINE_X - 3, TIMELINE_Y_START);
    lv_obj_set_style_bg_color(glow, lv_color_hex(0x00A651), 0);
    lv_obj_set_style_bg_opa(glow, LV_OPA_20, 0);
    lv_obj_set_style_radius(glow, 3, 0);
    lv_obj_set_style_border_width(glow, 0, 0);
    lv_obj_clear_flag(glow, LV_OBJ_FLAG_SCROLLABLE);

    /* 时间轴内芯线 */
    lv_obj_t *line_core = lv_obj_create(parent);
    lv_obj_set_size(line_core, 2, TIMELINE_Y_END - TIMELINE_Y_START);
    lv_obj_set_pos(line_core, TIMELINE_X - 1, TIMELINE_Y_START);
    lv_obj_set_style_bg_color(line_core, lv_color_hex(0x00A651), 0);
    lv_obj_set_style_bg_opa(line_core, LV_OPA_80, 0);
    lv_obj_set_style_radius(line_core, 1, 0);
    lv_obj_set_style_border_width(line_core, 0, 0);
    lv_obj_clear_flag(line_core, LV_OBJ_FLAG_SCROLLABLE);

    /* 时间轴节点 */
    int spacing = (TIMELINE_Y_END - TIMELINE_Y_START) / (NODE_COUNT - 1);

    for (int i = 0; i < NODE_COUNT; i++) {
        int y = TIMELINE_Y_START + i * spacing;
        bool is_highlight = (i == 0 || i == NODE_COUNT - 1);

        /* 节点圆点 */
        lv_obj_t *dot = lv_obj_create(parent);
        int dot_size = is_highlight ? 12 : 8;
        lv_obj_set_size(dot, dot_size, dot_size);
        lv_obj_set_pos(dot, TIMELINE_X - dot_size / 2, y - dot_size / 2);
        lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(dot, lv_color_hex(milestones[i].color), 0);
        lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(dot, lv_color_hex(0x00A651), 0);
        lv_obj_set_style_border_width(dot, is_highlight ? 2 : 1, 0);
        lv_obj_clear_flag(dot, LV_OBJ_FLAG_SCROLLABLE);

        /* 年份 */
        lv_obj_t *lbl_year = lv_label_create(parent);
        lv_label_set_text(lbl_year, milestones[i].year);
        lv_obj_set_style_text_color(lbl_year, lv_color_hex(milestones[i].color), 0);
        lv_obj_set_style_text_font(lbl_year, is_highlight ? &lv_font_montserrat_14 : &lv_font_montserrat_12, 0);
        lv_obj_set_pos(lbl_year, TIMELINE_X + 14, y - 10);

        /* 事件英文 */
        lv_obj_t *lbl_en = lv_label_create(parent);
        lv_label_set_text(lbl_en, milestones[i].event_en);
        lv_obj_set_style_text_color(lbl_en, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(lbl_en, &lv_font_montserrat_10, 0);
        lv_obj_set_pos(lbl_en, TIMELINE_X + 56, y - 9);

        /* 事件中文/简注 */
        lv_obj_t *lbl_zh = lv_label_create(parent);
        lv_label_set_text(lbl_zh, milestones[i].event_zh);
        lv_obj_set_style_text_color(lbl_zh, lv_color_hex(0x8B949E), 0);
        lv_obj_set_style_text_font(lbl_zh, &lv_font_montserrat_10, 0);
        lv_obj_set_pos(lbl_zh, TIMELINE_X + 14, y + 5);
    }

    /* 数据流光动画圆点 */
    dataflow_dot = lv_obj_create(parent);
    lv_obj_set_size(dataflow_dot, 6, 6);
    lv_obj_set_pos(dataflow_dot, TIMELINE_X - 3, TIMELINE_Y_START);
    lv_obj_set_style_radius(dataflow_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(dataflow_dot, lv_color_hex(0x00FF88), 0);
    lv_obj_set_style_bg_opa(dataflow_dot, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(dataflow_dot, 0, 0);
    lv_obj_set_style_shadow_width(dataflow_dot, 8, 0);
    lv_obj_set_style_shadow_color(dataflow_dot, lv_color_hex(0x00FF88), 0);
    lv_obj_set_style_shadow_opa(dataflow_dot, LV_OPA_60, 0);
    lv_obj_clear_flag(dataflow_dot, LV_OBJ_FLAG_SCROLLABLE);

    /* 无限循环上下流动动画 */
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, dataflow_dot);
    lv_anim_set_exec_cb(&a, dataflow_anim_y_cb);
    lv_anim_set_values(&a, TIMELINE_Y_START - 3, TIMELINE_Y_END - 3);
    lv_anim_set_time(&a, 2500);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
    lv_anim_start(&a);
}

/* ═══════════════════════════════════════════════════════════════
 *  Page 2 — Cartoon 卡通个人形象
 * ═══════════════════════════════════════════════════════════════ */
static void build_page_history(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, COLOR_BG_DARK, 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

    /* 卡通人物图铺满整页 */
    lv_obj_t *cartoon_img = lv_img_create(parent);

    /* Try loading custom cartoon from SD card (raw RGB565, 240x320) */
    static lv_img_dsc_t custom_cartoon;
    static uint8_t *cartoon_buf = NULL;
    if (config_has_custom_cartoon()) {
        FILE *f = fopen(CFG_CARTOON_PATH, "rb");
        if (f) {
            fseek(f, 0, SEEK_END);
            long sz = ftell(f);
            fseek(f, 0, SEEK_SET);
            if (sz == 240 * 320 * 2) {
                cartoon_buf = heap_caps_malloc(sz, MALLOC_CAP_SPIRAM);
                if (cartoon_buf) {
                    fread(cartoon_buf, 1, sz, f);
                    custom_cartoon.header.always_zero = 0;
                    custom_cartoon.header.w = 240;
                    custom_cartoon.header.h = 320;
                    custom_cartoon.header.cf = LV_IMG_CF_TRUE_COLOR;
                    custom_cartoon.data_size = sz;
                    custom_cartoon.data = cartoon_buf;
                    lv_img_set_src(cartoon_img, &custom_cartoon);
                    fclose(f);
                    goto cartoon_done;
                }
            }
            fclose(f);
        }
    }
    /* Fallback to built-in cartoon */
    lv_img_set_src(cartoon_img, &img_cartoon);

cartoon_done:
    lv_obj_align(cartoon_img, LV_ALIGN_TOP_LEFT, 0, 0);
}

/* ═══════════════════════════════════════════════════════════════
 *  Page 3 — LabVIEW DAQ Dashboard
 * ═══════════════════════════════════════════════════════════════ */
#define DASH_RSSI_POINTS 24

static struct {
    lv_obj_t *lbl_temp;
    lv_obj_t *lbl_rssi;
    lv_obj_t *lbl_mem;
    lv_obj_t *lbl_uptime;
    lv_obj_t *arc_temp;
    lv_obj_t *lbl_arc_value;
    lv_obj_t *chart;
    lv_chart_series_t *ser_rssi;
    lv_timer_t *timer;
} daq;

static int16_t dashboard_get_rssi(void)
{
    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
        return ap_info.rssi;
    }

    /* AP-only 模式没有 STA RSSI 时，用轻微抖动的模拟值 */
    return (int16_t)(-68 + ((int)(esp_random() % 9) - 4));
}

static void dashboard_timer_cb(lv_timer_t *t)
{
    (void)t;

    int16_t rssi = dashboard_get_rssi();
    uint32_t uptime_s = (uint32_t)(esp_timer_get_time() / 1000000ULL);
    uint32_t psram_kb = heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024;

    /* 估算温度值，保持平滑变化 */
    float temp_est = 41.0f + (float)((int)(esp_random() % 70) - 35) * 0.05f;
    int32_t temp_val = (int32_t)(temp_est + 0.5f);
    if (temp_val < 28) temp_val = 28;
    if (temp_val > 78) temp_val = 78;

    lv_label_set_text_fmt(daq.lbl_temp, "Temp: %d C", (int)temp_val);
    lv_label_set_text_fmt(daq.lbl_rssi, "WiFi RSSI: %d dBm", (int)rssi);
    lv_label_set_text_fmt(daq.lbl_mem, "Free PSRAM: %u KB", (unsigned)psram_kb);
    lv_label_set_text_fmt(daq.lbl_uptime, "Uptime: %02u:%02u:%02u",
                          (unsigned)(uptime_s / 3600),
                          (unsigned)((uptime_s / 60) % 60),
                          (unsigned)(uptime_s % 60));

    lv_arc_set_value(daq.arc_temp, temp_val);
    lv_label_set_text_fmt(daq.lbl_arc_value, "%d", (int)temp_val);
    lv_chart_set_next_value(daq.chart, daq.ser_rssi, rssi);
}

static void build_page_daq(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0D1117), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "LabVIEW DAQ Dashboard");
    lv_obj_set_style_text_color(title, lv_color_hex(0xC9D1D9), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t *sub = lv_label_create(parent);
    lv_label_set_text(sub, "Real-time Telemetry");
    lv_obj_set_style_text_color(sub, lv_color_hex(0x8B949E), 0);
    lv_obj_set_style_text_font(sub, &lv_font_montserrat_10, 0);
    lv_obj_align(sub, LV_ALIGN_TOP_MID, 0, 30);

    daq.arc_temp = lv_arc_create(parent);
    lv_obj_set_size(daq.arc_temp, 92, 92);
    lv_obj_align(daq.arc_temp, LV_ALIGN_TOP_LEFT, 14, 54);
    lv_arc_set_range(daq.arc_temp, 20, 80);
    lv_arc_set_bg_angles(daq.arc_temp, 135, 45);
    lv_arc_set_rotation(daq.arc_temp, 135);
    lv_arc_set_value(daq.arc_temp, 42);
    lv_obj_set_style_arc_color(daq.arc_temp, lv_color_hex(0x2D333B), LV_PART_MAIN);
    lv_obj_set_style_arc_width(daq.arc_temp, 8, LV_PART_MAIN);
    lv_obj_set_style_arc_color(daq.arc_temp, lv_color_hex(0xF1C40F), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(daq.arc_temp, 8, LV_PART_INDICATOR);
    lv_obj_remove_style(daq.arc_temp, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(daq.arc_temp, LV_OBJ_FLAG_CLICKABLE);

    daq.lbl_arc_value = lv_label_create(parent);
    lv_label_set_text(daq.lbl_arc_value, "42");
    lv_obj_set_style_text_color(daq.lbl_arc_value, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(daq.lbl_arc_value, &lv_font_montserrat_22, 0);
    lv_obj_align_to(daq.lbl_arc_value, daq.arc_temp, LV_ALIGN_CENTER, 0, -4);

    lv_obj_t *lbl_unit = lv_label_create(parent);
    lv_label_set_text(lbl_unit, "C");
    lv_obj_set_style_text_color(lbl_unit, lv_color_hex(0x8B949E), 0);
    lv_obj_set_style_text_font(lbl_unit, &lv_font_montserrat_12, 0);
    lv_obj_align_to(lbl_unit, daq.arc_temp, LV_ALIGN_CENTER, 0, 20);

    daq.chart = lv_chart_create(parent);
    lv_obj_set_size(daq.chart, 126, 96);
    lv_obj_align(daq.chart, LV_ALIGN_TOP_RIGHT, -10, 54);
    lv_obj_set_style_bg_color(daq.chart, lv_color_hex(0x161B22), 0);
    lv_obj_set_style_bg_opa(daq.chart, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(daq.chart, lv_color_hex(0x30363D), 0);
    lv_obj_set_style_border_width(daq.chart, 1, 0);
    lv_chart_set_type(daq.chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(daq.chart, DASH_RSSI_POINTS);
    lv_chart_set_range(daq.chart, LV_CHART_AXIS_PRIMARY_Y, -95, -30);
    lv_chart_set_div_line_count(daq.chart, 4, 5);
    daq.ser_rssi = lv_chart_add_series(daq.chart, lv_color_hex(0xF1C40F), LV_CHART_AXIS_PRIMARY_Y);
    for (int i = 0; i < DASH_RSSI_POINTS; i++) {
        lv_chart_set_next_value(daq.chart, daq.ser_rssi, -70);
    }

    daq.lbl_temp = lv_label_create(parent);
    lv_obj_set_style_text_color(daq.lbl_temp, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(daq.lbl_temp, &lv_font_montserrat_12, 0);
    lv_obj_align(daq.lbl_temp, LV_ALIGN_BOTTOM_LEFT, 10, -74);

    daq.lbl_rssi = lv_label_create(parent);
    lv_obj_set_style_text_color(daq.lbl_rssi, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(daq.lbl_rssi, &lv_font_montserrat_12, 0);
    lv_obj_align(daq.lbl_rssi, LV_ALIGN_BOTTOM_LEFT, 10, -56);

    daq.lbl_mem = lv_label_create(parent);
    lv_obj_set_style_text_color(daq.lbl_mem, lv_color_hex(0x00A651), 0);
    lv_obj_set_style_text_font(daq.lbl_mem, &lv_font_montserrat_12, 0);
    lv_obj_align(daq.lbl_mem, LV_ALIGN_BOTTOM_LEFT, 10, -38);

    daq.lbl_uptime = lv_label_create(parent);
    lv_obj_set_style_text_color(daq.lbl_uptime, lv_color_hex(0x8B949E), 0);
    lv_obj_set_style_text_font(daq.lbl_uptime, &lv_font_montserrat_12, 0);
    lv_obj_align(daq.lbl_uptime, LV_ALIGN_BOTTOM_LEFT, 10, -20);

    dashboard_timer_cb(NULL);
    daq.timer = lv_timer_create(dashboard_timer_cb, 1000, NULL);
}

/* ═══════════════════════════════════════════════════════════════
 *  Page 4 — Global Industry Impact Radar
 * ═══════════════════════════════════════════════════════════════ */
#define IMPACT_AXES 5
#define PI_F 3.1415926f

static void impact_point(int cx, int cy, int radius, int axis, lv_point_t *pt)
{
    float angle_deg = -90.0f + (360.0f / IMPACT_AXES) * axis;
    float rad = angle_deg * PI_F / 180.0f;
    pt->x = (lv_coord_t)(cx + radius * cosf(rad));
    pt->y = (lv_coord_t)(cy + radius * sinf(rad));
}

static void build_page_impact(lv_obj_t *parent)
{
    static const char *axis_name[IMPACT_AXES] = {
        "Semi", "Aero", "Auto", "Wireless", "Energy"
    };
    static const int score[IMPACT_AXES] = {95, 88, 92, 85, 75};

    int cx = 120;
    int cy = 166;
    int rmax = 82;

    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0D1117), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "NI 50 Years Industry Impact");
    lv_obj_set_style_text_color(title, lv_color_hex(0xC9D1D9), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t *sub = lv_label_create(parent);
    lv_label_set_text(sub, "Market Leadership Radar");
    lv_obj_set_style_text_color(sub, lv_color_hex(0x8B949E), 0);
    lv_obj_set_style_text_font(sub, &lv_font_montserrat_10, 0);
    lv_obj_align(sub, LV_ALIGN_TOP_MID, 0, 30);

    for (int level = 1; level <= 4; level++) {
        lv_point_t *ring = lv_mem_alloc(sizeof(lv_point_t) * (IMPACT_AXES + 1));
        if (!ring) continue;
        int rr = rmax * level / 4;
        for (int i = 0; i < IMPACT_AXES; i++) {
            impact_point(cx, cy, rr, i, &ring[i]);
        }
        ring[IMPACT_AXES] = ring[0];

        lv_obj_t *line = lv_line_create(parent);
        lv_line_set_points(line, ring, IMPACT_AXES + 1);
        lv_obj_set_style_line_width(line, 1, 0);
        lv_obj_set_style_line_color(line, lv_color_hex(0x30363D), 0);
        lv_obj_set_style_line_opa(line, LV_OPA_70, 0);
    }

    for (int i = 0; i < IMPACT_AXES; i++) {
        lv_point_t *axis_line = lv_mem_alloc(sizeof(lv_point_t) * 2);
        if (!axis_line) continue;
        axis_line[0].x = cx;
        axis_line[0].y = cy;
        impact_point(cx, cy, rmax, i, &axis_line[1]);

        lv_obj_t *line = lv_line_create(parent);
        lv_line_set_points(line, axis_line, 2);
        lv_obj_set_style_line_width(line, 1, 0);
        lv_obj_set_style_line_color(line, lv_color_hex(0x30363D), 0);
        lv_obj_set_style_line_opa(line, LV_OPA_60, 0);
    }

    lv_point_t *poly = lv_mem_alloc(sizeof(lv_point_t) * (IMPACT_AXES + 1));
    if (poly) {
        for (int i = 0; i < IMPACT_AXES; i++) {
            int rr = rmax * score[i] / 100;
            impact_point(cx, cy, rr, i, &poly[i]);
        }
        poly[IMPACT_AXES] = poly[0];

        lv_obj_t *line = lv_line_create(parent);
        lv_line_set_points(line, poly, IMPACT_AXES + 1);
        lv_obj_set_style_line_width(line, 2, 0);
        lv_obj_set_style_line_color(line, lv_color_hex(0x00A651), 0);
        lv_obj_set_style_line_opa(line, LV_OPA_COVER, 0);
    }

    for (int i = 0; i < IMPACT_AXES; i++) {
        lv_point_t p;
        impact_point(cx, cy, rmax + 18, i, &p);

        lv_obj_t *name = lv_label_create(parent);
        lv_label_set_text(name, axis_name[i]);
        lv_obj_set_style_text_color(name, lv_color_hex(0xC9D1D9), 0);
        lv_obj_set_style_text_font(name, &lv_font_montserrat_10, 0);
        lv_obj_set_pos(name, p.x - 18, p.y - 8);
    }

    lv_obj_t *legend = lv_label_create(parent);
    lv_label_set_text(legend, "Leadership Score: 95 / 88 / 92 / 85 / 75");
    lv_obj_set_style_text_color(legend, lv_color_hex(0x00A651), 0);
    lv_obj_set_style_text_font(legend, &lv_font_montserrat_10, 0);
    lv_obj_align(legend, LV_ALIGN_BOTTOM_MID, 0, -18);
}

/* ═══════════════════════════════════════════════════════════════
 *  Page 5 — Dynamic Digital Identity
 * ═══════════════════════════════════════════════════════════════ */
static struct {
    uint8_t *qr_buf;
    lv_img_dsc_t qr_dsc;
    bool qr_ready;
    lv_obj_t *dot[4];
    int phase_deg;
    lv_timer_t *orbit_timer;
} identity;

static void identity_render_qr_once(lv_obj_t *parent, const char *url)
{
    if (identity.qr_ready) return;

    lv_obj_t *tmp_qr = lv_qrcode_create(parent, 120, lv_color_black(), lv_color_white());
    lv_obj_add_flag(tmp_qr, LV_OBJ_FLAG_HIDDEN);
    if (lv_qrcode_update(tmp_qr, url, strlen(url)) != LV_RES_OK) {
        lv_obj_del(tmp_qr);
        return;
    }

    const lv_img_dsc_t *canvas_img = lv_canvas_get_img(tmp_qr);
    if (!canvas_img || !canvas_img->data) {
        lv_obj_del(tmp_qr);
        return;
    }

    if (canvas_img->header.cf == LV_IMG_CF_INDEXED_1BIT) {
        uint32_t w = canvas_img->header.w;
        uint32_t h = canvas_img->header.h;
        uint32_t stride = (w + 7) / 8;
        const uint8_t *src = canvas_img->data;
        const uint8_t *bmp = src + 8; /* palette 2 x 4 bytes */

        lv_color_t pal[2];
        pal[0] = lv_color_make(src[2], src[1], src[0]);
        pal[1] = lv_color_make(src[6], src[5], src[4]);

        lv_color_t *dst = heap_caps_malloc(w * h * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
        if (dst) {
            for (uint32_t y = 0; y < h; y++) {
                for (uint32_t x = 0; x < w; x++) {
                    uint8_t bytev = bmp[y * stride + (x >> 3)];
                    uint8_t bit = (bytev >> (7 - (x & 0x7))) & 0x1;
                    dst[y * w + x] = pal[bit];
                }
            }

            identity.qr_buf = (uint8_t *)dst;
            identity.qr_dsc.header.always_zero = 0;
            identity.qr_dsc.header.w = w;
            identity.qr_dsc.header.h = h;
            identity.qr_dsc.header.cf = LV_IMG_CF_TRUE_COLOR;
            identity.qr_dsc.data_size = w * h * sizeof(lv_color_t);
            identity.qr_dsc.data = identity.qr_buf;
            identity.qr_ready = true;
        }
    }

    lv_obj_del(tmp_qr);
}

static void identity_orbit_timer_cb(lv_timer_t *t)
{
    (void)t;
    int cx = 120;
    int cy = 166;
    int r = 76;

    identity.phase_deg = (identity.phase_deg + 7) % 360;
    for (int i = 0; i < 4; i++) {
        float deg = (float)(identity.phase_deg + i * 90);
        float rad = deg * PI_F / 180.0f;
        int x = (int)(cx + r * cosf(rad)) - 4;
        int y = (int)(cy + r * sinf(rad)) - 4;
        lv_obj_set_pos(identity.dot[i], x, y);
    }
}

static void build_page_identity(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0D1117), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Connect with NI");
    lv_obj_set_style_text_color(title, lv_color_hex(0xC9D1D9), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 12);

    lv_obj_t *sub = lv_label_create(parent);
    lv_label_set_text(sub, "Let's Engineer the Future");
    lv_obj_set_style_text_color(sub, lv_color_hex(0x8B949E), 0);
    lv_obj_set_style_text_font(sub, &lv_font_montserrat_10, 0);
    lv_obj_align(sub, LV_ALIGN_TOP_MID, 0, 34);

    identity_render_qr_once(parent, "https://www.ni.com");

    lv_obj_t *qr_panel = lv_obj_create(parent);
    lv_obj_set_size(qr_panel, 132, 132);
    lv_obj_align(qr_panel, LV_ALIGN_CENTER, 0, 6);
    lv_obj_set_style_radius(qr_panel, 10, 0);
    lv_obj_set_style_bg_color(qr_panel, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(qr_panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(qr_panel, 0, 0);
    lv_obj_set_style_pad_all(qr_panel, 6, 0);
    lv_obj_clear_flag(qr_panel, LV_OBJ_FLAG_SCROLLABLE);

    if (identity.qr_ready) {
        lv_obj_t *qr_img = lv_img_create(qr_panel);
        lv_img_set_src(qr_img, &identity.qr_dsc);
        lv_obj_center(qr_img);
    } else {
        lv_obj_t *fallback = lv_label_create(qr_panel);
        lv_label_set_text(fallback, "www.ni.com");
        lv_obj_set_style_text_color(fallback, lv_color_black(), 0);
        lv_obj_set_style_text_font(fallback, &lv_font_montserrat_14, 0);
        lv_obj_center(fallback);
    }

    for (int i = 0; i < 4; i++) {
        lv_obj_t *ring = lv_obj_create(parent);
        lv_obj_set_size(ring, 142 + i * 12, 142 + i * 12);
        lv_obj_align(ring, LV_ALIGN_CENTER, 0, 6);
        lv_obj_set_style_radius(ring, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_opa(ring, LV_OPA_0, 0);
        lv_obj_set_style_border_width(ring, 1, 0);
        lv_obj_set_style_border_color(ring, lv_color_hex(0x1F6FEB), 0);
        lv_obj_set_style_border_opa(ring, (i == 0) ? LV_OPA_40 : LV_OPA_20, 0);
        lv_obj_clear_flag(ring, LV_OBJ_FLAG_SCROLLABLE);
    }

    for (int i = 0; i < 4; i++) {
        identity.dot[i] = lv_obj_create(parent);
        lv_obj_set_size(identity.dot[i], 8, 8);
        lv_obj_set_style_radius(identity.dot[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(identity.dot[i], (i % 2) ? lv_color_hex(0xF5C518) : lv_color_hex(0x00A651), 0);
        lv_obj_set_style_bg_opa(identity.dot[i], LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(identity.dot[i], 0, 0);
        lv_obj_clear_flag(identity.dot[i], LV_OBJ_FLAG_SCROLLABLE);
    }

    identity.phase_deg = 0;
    identity_orbit_timer_cb(NULL);
    identity.orbit_timer = lv_timer_create(identity_orbit_timer_cb, 80, NULL);

    lv_obj_t *hint = lv_label_create(parent);
    lv_label_set_text(hint, "Scan to open www.ni.com");
    lv_obj_set_style_text_color(hint, lv_color_hex(0x8B949E), 0);
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_12, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -18);
}

/* ═══════════════════════════════════════════════════════════════
 *  Page 2 — 记忆翻牌游戏 (图片版，随机选 6 对)
 *  4列 × 3行 = 12张卡 = 6对
 * ═══════════════════════════════════════════════════════════════ */
#define GAME_COLS   4
#define GAME_ROWS   3
#define CARD_W      50
#define CARD_H      56
#define CARD_GAP_X  7
#define CARD_GAP_Y  6
#define GRID_TOP    52
#define GRID_LEFT   ((SCREEN_W - GAME_COLS * CARD_W - (GAME_COLS - 1) * CARD_GAP_X) / 2)
#define NUM_CARDS   (GAME_COLS * GAME_ROWS)
#define NUM_PAIRS   (NUM_CARDS / 2)

/* 游戏状态 */
static struct {
    lv_obj_t *card_obj[NUM_CARDS];
    lv_obj_t *card_img[NUM_CARDS];   /* 图片对象 */
    lv_obj_t *card_lbl[NUM_CARDS];   /* 背面 "?" */
    uint8_t   card_id[NUM_CARDS];    /* 对应 selected_images 索引 0-5 */
    const lv_img_dsc_t *selected_images[NUM_PAIRS]; /* 本局选中的 6 张图 */
    bool      revealed[NUM_CARDS];
    bool      flipped[NUM_CARDS];
    int       first_pick;
    int       second_pick;
    int       pairs_found;
    int       moves;
    bool      busy;
    lv_obj_t *lbl_moves;
    lv_obj_t *lbl_status;
    lv_obj_t *btn_restart;
} game;

static void game_restart(void);
static void game_flip_back_cb(lv_timer_t *t);

/* 从 12 张图池中随机选 6 张 */
static void select_random_images(void)
{
    uint8_t indices[CARD_IMAGE_COUNT];
    for (int i = 0; i < CARD_IMAGE_COUNT; i++) indices[i] = i;
    /* Fisher-Yates partial shuffle, pick first 6 */
    for (int i = CARD_IMAGE_COUNT - 1; i > 0; i--) {
        int j = esp_random() % (i + 1);
        uint8_t tmp = indices[i]; indices[i] = indices[j]; indices[j] = tmp;
    }
    for (int i = 0; i < NUM_PAIRS; i++) {
        game.selected_images[i] = card_image_pool[indices[i]];
    }
}

static void shuffle_cards(void)
{
    for (int i = 0; i < NUM_PAIRS; i++) {
        game.card_id[i * 2] = i;
        game.card_id[i * 2 + 1] = i;
    }
    for (int i = NUM_CARDS - 1; i > 0; i--) {
        int j = esp_random() % (i + 1);
        uint8_t tmp = game.card_id[i];
        game.card_id[i] = game.card_id[j];
        game.card_id[j] = tmp;
    }
}

static void show_card_face(int idx)
{
    uint8_t cid = game.card_id[idx];
    lv_obj_set_style_bg_color(game.card_obj[idx], lv_color_hex(0x2C3E50), 0);
    lv_obj_add_flag(game.card_lbl[idx], LV_OBJ_FLAG_HIDDEN);
    lv_img_set_src(game.card_img[idx], game.selected_images[cid]);
    lv_obj_clear_flag(game.card_img[idx], LV_OBJ_FLAG_HIDDEN);
    game.flipped[idx] = true;
}

static void show_card_back(int idx)
{
    lv_obj_set_style_bg_color(game.card_obj[idx], lv_color_hex(0x34495E), 0);
    lv_obj_add_flag(game.card_img[idx], LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(game.card_lbl[idx], LV_OBJ_FLAG_HIDDEN);
    game.flipped[idx] = false;
}

static void hide_card(int idx)
{
    lv_obj_set_style_bg_opa(game.card_obj[idx], LV_OPA_0, 0);
    lv_obj_set_style_border_opa(game.card_obj[idx], LV_OPA_0, 0);
    lv_obj_add_flag(game.card_lbl[idx], LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(game.card_img[idx], LV_OBJ_FLAG_HIDDEN);
    game.revealed[idx] = true;
}

static void card_click_cb(lv_event_t *e)
{
    if (game.busy) return;
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    if (game.revealed[idx] || game.flipped[idx]) return;

    show_card_face(idx);

    if (game.first_pick < 0) {
        game.first_pick = idx;
    } else {
        game.second_pick = idx;
        game.moves++;
        char buf[16];
        snprintf(buf, sizeof(buf), "%d", game.moves);
        lv_label_set_text(game.lbl_moves, buf);

        if (game.card_id[game.first_pick] == game.card_id[game.second_pick]) {
            hide_card(game.first_pick);
            hide_card(game.second_pick);
            game.pairs_found++;
            game.first_pick = -1;
            game.second_pick = -1;
            if (game.pairs_found >= NUM_PAIRS) {
                lv_label_set_text(game.lbl_status, LV_SYMBOL_OK " Complete!");
            }
        } else {
            game.busy = true;
            lv_timer_create(game_flip_back_cb, 600, NULL);
        }
    }
}

static void game_flip_back_cb(lv_timer_t *t)
{
    show_card_back(game.first_pick);
    show_card_back(game.second_pick);
    game.first_pick = -1;
    game.second_pick = -1;
    game.busy = false;
    lv_timer_del(t);
}

static void restart_click_cb(lv_event_t *e)
{
    (void)e;
    game_restart();
}

static void game_restart(void)
{
    select_random_images();
    shuffle_cards();
    game.first_pick = -1;
    game.second_pick = -1;
    game.pairs_found = 0;
    game.moves = 0;
    game.busy = false;

    for (int i = 0; i < NUM_CARDS; i++) {
        game.revealed[i] = false;
        game.flipped[i] = false;
        lv_obj_set_style_bg_opa(game.card_obj[i], LV_OPA_COVER, 0);
        lv_obj_set_style_border_opa(game.card_obj[i], LV_OPA_COVER, 0);
        show_card_back(i);
    }
    lv_label_set_text(game.lbl_moves, "0");
    lv_label_set_text(game.lbl_status, "NI Match");
}

static void build_page_game(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, COLOR_BG_DARK, 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

    /* 顶部标题 */
    game.lbl_status = lv_label_create(parent);
    lv_label_set_text(game.lbl_status, "NI Match");
    lv_obj_set_style_text_color(game.lbl_status, COLOR_ACCENT_YELLOW, 0);
    lv_obj_set_style_text_font(game.lbl_status, &lv_font_montserrat_14, 0);
    lv_obj_align(game.lbl_status, LV_ALIGN_TOP_MID, 0, 8);

    /* 步数 + Reset 按钮（顶部栏） */
    lv_obj_t *lbl_mv = lv_label_create(parent);
    lv_label_set_text(lbl_mv, "Moves:");
    lv_obj_set_style_text_color(lbl_mv, COLOR_TEXT_WHITE, 0);
    lv_obj_set_style_text_font(lbl_mv, &lv_font_montserrat_12, 0);
    lv_obj_align(lbl_mv, LV_ALIGN_TOP_LEFT, 8, 30);

    game.lbl_moves = lv_label_create(parent);
    lv_label_set_text(game.lbl_moves, "0");
    lv_obj_set_style_text_color(game.lbl_moves, COLOR_TEXT_WHITE, 0);
    lv_obj_set_style_text_font(game.lbl_moves, &lv_font_montserrat_12, 0);
    lv_obj_align(game.lbl_moves, LV_ALIGN_TOP_LEFT, 56, 30);

    /* Reset 按钮（始终可见） */
    game.btn_restart = lv_btn_create(parent);
    lv_obj_set_size(game.btn_restart, 60, 24);
    lv_obj_align(game.btn_restart, LV_ALIGN_TOP_RIGHT, -8, 28);
    lv_obj_set_style_bg_color(game.btn_restart, COLOR_NI_GREEN, 0);
    lv_obj_set_style_radius(game.btn_restart, 12, 0);
    lv_obj_add_event_cb(game.btn_restart, restart_click_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *btn_lbl = lv_label_create(game.btn_restart);
    lv_label_set_text(btn_lbl, LV_SYMBOL_REFRESH);
    lv_obj_set_style_text_font(btn_lbl, &lv_font_montserrat_12, 0);
    lv_obj_center(btn_lbl);

    /* 初始化图片选择和洗牌 */
    select_random_images();
    shuffle_cards();
    game.first_pick = -1;
    game.second_pick = -1;
    game.pairs_found = 0;
    game.moves = 0;
    game.busy = false;

    /* 创建卡片网格 */
    for (int i = 0; i < NUM_CARDS; i++) {
        int col = i % GAME_COLS;
        int row = i / GAME_COLS;
        int x = GRID_LEFT + col * (CARD_W + CARD_GAP_X);
        int y = GRID_TOP + row * (CARD_H + CARD_GAP_Y);

        lv_obj_t *card = lv_obj_create(parent);
        lv_obj_set_size(card, CARD_W, CARD_H);
        lv_obj_set_pos(card, x, y);
        lv_obj_set_style_radius(card, 6, 0);
        lv_obj_set_style_border_width(card, 2, 0);
        lv_obj_set_style_border_color(card, lv_color_hex(0x5D6D7E), 0);
        lv_obj_set_style_bg_color(card, lv_color_hex(0x34495E), 0);
        lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
        lv_obj_set_style_pad_all(card, 0, 0);
        lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(card, card_click_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        game.card_obj[i] = card;

        /* 图片（初始隐藏） */
        lv_obj_t *img = lv_img_create(card);
        lv_img_set_src(img, game.selected_images[game.card_id[i]]);
        lv_obj_center(img);
        lv_obj_add_flag(img, LV_OBJ_FLAG_HIDDEN);
        game.card_img[i] = img;

        /* 背面问号 */
        lv_obj_t *lbl = lv_label_create(card);
        lv_label_set_text(lbl, "?");
        lv_obj_set_style_text_color(lbl, lv_color_hex(0x95A5A6), 0);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, 0);
        lv_obj_center(lbl);
        game.card_lbl[i] = lbl;

        game.revealed[i] = false;
        game.flipped[i] = false;
    }
}

/* ═══════════════════════════════════════════════════════════════
 *  Page 3 — 2048 游戏 (倾斜控制 + 滑动动画)
 * ═══════════════════════════════════════════════════════════════ */
#define G2048_SIZE  4
#define CELL_SIZE   52
#define CELL_GAP    4
#define G2048_GRID_LEFT ((SCREEN_W - G2048_SIZE * CELL_SIZE - (G2048_SIZE - 1) * CELL_GAP) / 2)
#define G2048_GRID_TOP  70

/* 倾斜控制参数 */
#define TILT_THRESHOLD   0.55f   /* 触发移动的最小倾斜角 (G) —— 增大死区减少误触发 */
#define TILT_COOLDOWN_MS 380     /* 两次移动之间的最短间隔 */
#define TILT_EMA_ALPHA   0.3f    /* EMA 低通滤波系数 (0-1, 越小越平滑) */
#define TILT_CONFIRM_CNT 2       /* 连续 N 次同方向才触发移动 */

/* 动画参数 */
#define ANIM_DURATION_MS 120     /* 滑动动画时长 */

static struct {
    uint16_t board[G2048_SIZE][G2048_SIZE];
    uint16_t prev_board[G2048_SIZE][G2048_SIZE]; /* 动画用：移动前的棋盘 */
    lv_obj_t *cells[G2048_SIZE][G2048_SIZE];
    lv_obj_t *cell_lbls[G2048_SIZE][G2048_SIZE];
    lv_obj_t *grid_bg;
    lv_obj_t *lbl_score;
    lv_obj_t *lbl_title;
    lv_obj_t *lbl_tilt_hint;
    lv_obj_t *btn_reset;
    lv_obj_t *btn_calibrate;
    lv_timer_t *tilt_timer;
    uint32_t score;
    uint32_t last_move_ms;
    /* 校准状态机: 0=空闲 1=等待竖直 2=等待左 3=等待右 4=等待前 */
    int cal_step;
    /* 校准采样点 (x,y,z) */
    float cal_n[3];   /* neutral/竖直 = DOWN 方向 */
    float cal_l[3];   /* left */
    float cal_r[3];   /* right */
    float cal_u[3];   /* forward = UP 方向 */
    /* 派生方向向量 (归一化) */
    float lr_dir[3];  /* 指向右 */
    float up_dir[3];  /* 指向前/UP */
    bool calibrated;
    bool game_over;
    bool animating;
    /* EMA 滤波状态 */
    float ema_lr;
    float ema_up;
    int   confirm_dir;     /* 上次检测到的方向 (-1=none) */
    int   confirm_count;   /* 连续同方向计数 */
} g2048;

static lv_color_t g2048_tile_color(uint16_t val)
{
    switch (val) {
        case 2:    return lv_color_hex(0xEEE4DA);
        case 4:    return lv_color_hex(0xEDE0C8);
        case 8:    return lv_color_hex(0xF2B179);
        case 16:   return lv_color_hex(0xF59563);
        case 32:   return lv_color_hex(0xF67C5F);
        case 64:   return lv_color_hex(0xF65E3B);
        case 128:  return lv_color_hex(0xEDCF72);
        case 256:  return lv_color_hex(0xEDCC61);
        case 512:  return lv_color_hex(0xEDC850);
        case 1024: return lv_color_hex(0xEDC53F);
        case 2048: return lv_color_hex(0xEDC22E);
        default:   return lv_color_hex(0x3C3A32);
    }
}

static lv_color_t g2048_text_color(uint16_t val)
{
    return (val <= 4) ? lv_color_hex(0x776E65) : lv_color_hex(0xF9F6F2);
}

/* 立即刷新格子外观（无动画） */
static void g2048_update_ui(void)
{
    char buf[16];
    snprintf(buf, sizeof(buf), "%lu", (unsigned long)g2048.score);
    lv_label_set_text(g2048.lbl_score, buf);

    for (int r = 0; r < G2048_SIZE; r++) {
        for (int c = 0; c < G2048_SIZE; c++) {
            uint16_t v = g2048.board[r][c];
            if (v == 0) {
                lv_obj_set_style_bg_color(g2048.cells[r][c], lv_color_hex(0x2C3E50), 0);
                lv_label_set_text(g2048.cell_lbls[r][c], "");
            } else {
                lv_obj_set_style_bg_color(g2048.cells[r][c], g2048_tile_color(v), 0);
                snprintf(buf, sizeof(buf), "%d", v);
                lv_label_set_text(g2048.cell_lbls[r][c], buf);
                lv_obj_set_style_text_color(g2048.cell_lbls[r][c], g2048_text_color(v), 0);
                if (v >= 1024)
                    lv_obj_set_style_text_font(g2048.cell_lbls[r][c], &lv_font_montserrat_10, 0);
                else if (v >= 128)
                    lv_obj_set_style_text_font(g2048.cell_lbls[r][c], &lv_font_montserrat_12, 0);
                else
                    lv_obj_set_style_text_font(g2048.cell_lbls[r][c], &lv_font_montserrat_16, 0);
            }
        }
    }
    if (g2048.game_over) {
        lv_label_set_text(g2048.lbl_title, "Game Over!");
    }
}

/* ── 滑动动画回调 ── */
static void g2048_anim_x_cb(void *obj, int32_t v) { lv_obj_set_x((lv_obj_t *)obj, v); }
static void g2048_anim_y_cb(void *obj, int32_t v) { lv_obj_set_y((lv_obj_t *)obj, v); }

static void g2048_anim_done_cb(lv_anim_t *a)
{
    (void)a;
    g2048.animating = false;
    /* 动画完成后刷新 UI 到最终状态 */
    g2048_update_ui();
    /* 将所有 cell 重置到网格位置 */
    for (int r = 0; r < G2048_SIZE; r++) {
        for (int c = 0; c < G2048_SIZE; c++) {
            int x = 6 + c * (CELL_SIZE + CELL_GAP);
            int y = 6 + r * (CELL_SIZE + CELL_GAP);
            lv_obj_set_pos(g2048.cells[r][c], x, y);
        }
    }
}

/* 执行带动画的移动：先动画位移，再更新内容 */
static void g2048_animate_move(int dir)
{
    g2048.animating = true;
    int anim_count = 0;

    /* 计算每个格子的位移偏移量 */
    int dx = 0, dy = 0;
    switch (dir) {
        case 0: dx = -(CELL_SIZE + CELL_GAP); break; /* left */
        case 1: dx =  (CELL_SIZE + CELL_GAP); break; /* right */
        case 2: dy = -(CELL_SIZE + CELL_GAP); break; /* up */
        case 3: dy =  (CELL_SIZE + CELL_GAP); break; /* down */
    }

    /* 找出哪些格子移动了（prev_board 有值，且位置变了） */
    for (int r = 0; r < G2048_SIZE; r++) {
        for (int c = 0; c < G2048_SIZE; c++) {
            if (g2048.prev_board[r][c] == 0) continue;
            /* 这个格子原来有方块，现在如果消失了或值变了，表示发生了移动 */
            if (g2048.board[r][c] != g2048.prev_board[r][c]) {
                /* 从当前逻辑位置做一个短距离动画偏移表示"有东西移走了" */
                int cur_x = 6 + c * (CELL_SIZE + CELL_GAP);
                int cur_y = 6 + r * (CELL_SIZE + CELL_GAP);

                lv_anim_t a;
                lv_anim_init(&a);
                lv_anim_set_var(&a, g2048.cells[r][c]);
                lv_anim_set_time(&a, ANIM_DURATION_MS);
                lv_anim_set_path_cb(&a, lv_anim_path_ease_out);

                if (dx != 0) {
                    lv_anim_set_exec_cb(&a, g2048_anim_x_cb);
                    lv_anim_set_values(&a, cur_x + dx, cur_x);
                } else {
                    lv_anim_set_exec_cb(&a, g2048_anim_y_cb);
                    lv_anim_set_values(&a, cur_y + dy, cur_y);
                }

                if (anim_count == 0) {
                    lv_anim_set_ready_cb(&a, g2048_anim_done_cb);
                }
                lv_anim_start(&a);
                anim_count++;
            }
        }
    }

    if (anim_count == 0) {
        /* 没有实际动画需要播，直接更新 */
        g2048.animating = false;
        g2048_update_ui();
    }
}

static void g2048_spawn(void)
{
    uint8_t empty[16];
    int n = 0;
    for (int r = 0; r < G2048_SIZE; r++)
        for (int c = 0; c < G2048_SIZE; c++)
            if (g2048.board[r][c] == 0)
                empty[n++] = r * G2048_SIZE + c;
    if (n == 0) return;
    int pick = esp_random() % n;
    int r = empty[pick] / G2048_SIZE;
    int c = empty[pick] % G2048_SIZE;
    g2048.board[r][c] = (esp_random() % 10 < 9) ? 2 : 4;
}

static bool g2048_can_move(void)
{
    for (int r = 0; r < G2048_SIZE; r++)
        for (int c = 0; c < G2048_SIZE; c++) {
            if (g2048.board[r][c] == 0) return true;
            if (c < G2048_SIZE - 1 && g2048.board[r][c] == g2048.board[r][c + 1]) return true;
            if (r < G2048_SIZE - 1 && g2048.board[r][c] == g2048.board[r + 1][c]) return true;
        }
    return false;
}

static bool g2048_slide_row(uint16_t row[G2048_SIZE])
{
    bool changed = false;
    uint16_t tmp[G2048_SIZE] = {0};
    int pos = 0;
    for (int i = 0; i < G2048_SIZE; i++)
        if (row[i]) tmp[pos++] = row[i];
    for (int i = 0; i < G2048_SIZE - 1; i++) {
        if (tmp[i] && tmp[i] == tmp[i + 1]) {
            tmp[i] *= 2;
            g2048.score += tmp[i];
            tmp[i + 1] = 0;
            changed = true;
        }
    }
    uint16_t out[G2048_SIZE] = {0};
    pos = 0;
    for (int i = 0; i < G2048_SIZE; i++)
        if (tmp[i]) out[pos++] = tmp[i];
    for (int i = 0; i < G2048_SIZE; i++) {
        if (row[i] != out[i]) changed = true;
        row[i] = out[i];
    }
    return changed;
}

static bool g2048_move(int dir)
{
    bool changed = false;
    uint16_t line[G2048_SIZE];

    for (int i = 0; i < G2048_SIZE; i++) {
        for (int j = 0; j < G2048_SIZE; j++) {
            switch (dir) {
                case 0: line[j] = g2048.board[i][j]; break;
                case 1: line[j] = g2048.board[i][G2048_SIZE - 1 - j]; break;
                case 2: line[j] = g2048.board[j][i]; break;
                case 3: line[j] = g2048.board[G2048_SIZE - 1 - j][i]; break;
            }
        }
        if (g2048_slide_row(line)) changed = true;
        for (int j = 0; j < G2048_SIZE; j++) {
            switch (dir) {
                case 0: g2048.board[i][j] = line[j]; break;
                case 1: g2048.board[i][G2048_SIZE - 1 - j] = line[j]; break;
                case 2: g2048.board[j][i] = line[j]; break;
                case 3: g2048.board[G2048_SIZE - 1 - j][i] = line[j]; break;
            }
        }
    }
    return changed;
}

static void g2048_reset(void)
{
    memset(g2048.board, 0, sizeof(g2048.board));
    g2048.score = 0;
    g2048.game_over = false;
    g2048.animating = false;
    g2048.last_move_ms = 0;
    g2048_spawn();
    g2048_spawn();
    g2048_update_ui();
    lv_label_set_text(g2048.lbl_title, "2048");
}

static void g2048_reset_cb(lv_event_t *e) { (void)e; g2048_reset(); }

/* ── 校准状态机 ── */
static const char *cal_prompts[] = {
    "",
    "Hold VERTICAL\nthen tap screen",
    "Tilt LEFT\nthen tap screen",
    "Tilt RIGHT\nthen tap screen",
    "Tilt FORWARD\nthen tap screen",
};

static void g2048_cal_compute(void)
{
    /* LR 方向: normalize(R - L) */
    float lr[3] = {
        g2048.cal_r[0] - g2048.cal_l[0],
        g2048.cal_r[1] - g2048.cal_l[1],
        g2048.cal_r[2] - g2048.cal_l[2]
    };
    float lr_mag = sqrtf(lr[0]*lr[0] + lr[1]*lr[1] + lr[2]*lr[2]);
    if (lr_mag < 0.01f) lr_mag = 1.0f;
    g2048.lr_dir[0] = lr[0] / lr_mag;
    g2048.lr_dir[1] = lr[1] / lr_mag;
    g2048.lr_dir[2] = lr[2] / lr_mag;

    /* UP 方向: normalize(U - N), 从竖直指向前倾 = 游戏 UP */
    float ud[3] = {
        g2048.cal_u[0] - g2048.cal_n[0],
        g2048.cal_u[1] - g2048.cal_n[1],
        g2048.cal_u[2] - g2048.cal_n[2]
    };
    float ud_mag = sqrtf(ud[0]*ud[0] + ud[1]*ud[1] + ud[2]*ud[2]);
    if (ud_mag < 0.01f) ud_mag = 1.0f;
    g2048.up_dir[0] = ud[0] / ud_mag;
    g2048.up_dir[1] = ud[1] / ud_mag;
    g2048.up_dir[2] = ud[2] / ud_mag;

    g2048.calibrated = true;
    g2048.cal_step = 0;
    lv_label_set_text(g2048.lbl_tilt_hint, LV_SYMBOL_OK " Calibrated!");
}

/* 校准点击屏幕确认回调（注册在 tileview 页面上） */
static void g2048_cal_tap_cb(lv_event_t *e)
{
    (void)e;
    if (g2048.cal_step <= 0) return;

    float cur[3] = { Accel.x, Accel.y, Accel.z };

    switch (g2048.cal_step) {
    case 1: /* 记录竖直 (neutral=DOWN) */
        g2048.cal_n[0] = cur[0]; g2048.cal_n[1] = cur[1]; g2048.cal_n[2] = cur[2];
        g2048.cal_step = 2;
        lv_label_set_text(g2048.lbl_tilt_hint, cal_prompts[2]);
        break;
    case 2: /* 记录左倾 */
        g2048.cal_l[0] = cur[0]; g2048.cal_l[1] = cur[1]; g2048.cal_l[2] = cur[2];
        g2048.cal_step = 3;
        lv_label_set_text(g2048.lbl_tilt_hint, cal_prompts[3]);
        break;
    case 3: /* 记录右倾 */
        g2048.cal_r[0] = cur[0]; g2048.cal_r[1] = cur[1]; g2048.cal_r[2] = cur[2];
        g2048.cal_step = 4;
        lv_label_set_text(g2048.lbl_tilt_hint, cal_prompts[4]);
        break;
    case 4: /* 记录前倾 (UP) */
        g2048.cal_u[0] = cur[0]; g2048.cal_u[1] = cur[1]; g2048.cal_u[2] = cur[2];
        g2048_cal_compute();
        break;
    }
}

/* CAL 按钮回调：启动校准流程 */
static void g2048_calibrate_cb(lv_event_t *e)
{
    (void)e;
    g2048.cal_step = 1;
    g2048.calibrated = false;
    lv_label_set_text(g2048.lbl_tilt_hint, cal_prompts[1]);
}

/* ── 倾斜控制定时器 (EMA 滤波 + 方向确认) ── */
static void g2048_tilt_timer_cb(lv_timer_t *t)
{
    (void)t;
    if (current_page != 4) return;
    if (g2048.game_over || g2048.animating) return;
    if (g2048.cal_step > 0) return;
    if (!g2048.calibrated) return;

    /* 计算当前加速度与中立位的差值 */
    float dx = Accel.x - g2048.cal_n[0];
    float dy = Accel.y - g2048.cal_n[1];
    float dz = Accel.z - g2048.cal_n[2];

    /* 投影到两个轴 */
    float lr_raw = dx * g2048.lr_dir[0] + dy * g2048.lr_dir[1] + dz * g2048.lr_dir[2];
    float up_raw = dx * g2048.up_dir[0] + dy * g2048.up_dir[1] + dz * g2048.up_dir[2];

    /* EMA 低通滤波 */
    g2048.ema_lr = TILT_EMA_ALPHA * lr_raw + (1.0f - TILT_EMA_ALPHA) * g2048.ema_lr;
    g2048.ema_up = TILT_EMA_ALPHA * up_raw + (1.0f - TILT_EMA_ALPHA) * g2048.ema_up;

    float abs_lr = (g2048.ema_lr < 0) ? -g2048.ema_lr : g2048.ema_lr;
    float abs_up = (g2048.ema_up < 0) ? -g2048.ema_up : g2048.ema_up;

    /* 判断方向 (竖直/中立 = 不动) */
    int dir = -1;
    if (abs_lr > abs_up && abs_lr > TILT_THRESHOLD) {
        dir = (g2048.ema_lr > 0) ? 1 : 0;   /* 1=right, 0=left */
    } else if (abs_up > abs_lr && abs_up > TILT_THRESHOLD) {
        dir = (g2048.ema_up > 0) ? 2 : 3;   /* 2=up, 3=down */
    }

    /* 没超过阈值 = 中立位不动 */
    if (dir < 0) {
        g2048.confirm_dir = -1;
        g2048.confirm_count = 0;
        return;
    }

    /* 方向确认机制: 连续 N 次同方向才触发 */
    if (dir != g2048.confirm_dir) {
        g2048.confirm_dir = dir;
        g2048.confirm_count = 1;
        return;
    }
    g2048.confirm_count++;
    if (g2048.confirm_count < TILT_CONFIRM_CNT) return;

    /* 冷却检查 */
    uint32_t now = lv_tick_get();
    if (now - g2048.last_move_ms < TILT_COOLDOWN_MS) return;

    /* 执行移动 */
    memcpy(g2048.prev_board, g2048.board, sizeof(g2048.board));

    if (g2048_move(dir)) {
        g2048.last_move_ms = now;
        g2048.confirm_count = 0;  /* 重置确认计数防止连续触发 */
        g2048_spawn();
        if (!g2048_can_move()) g2048.game_over = true;
        g2048_animate_move(dir);
    }
}

static void build_page_2048(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, lv_color_hex(0x1C233B), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

    /* 标题 */
    g2048.lbl_title = lv_label_create(parent);
    lv_label_set_text(g2048.lbl_title, "2048");
    lv_obj_set_style_text_color(g2048.lbl_title, COLOR_ACCENT_YELLOW, 0);
    lv_obj_set_style_text_font(g2048.lbl_title, &lv_font_montserrat_18, 0);
    lv_obj_align(g2048.lbl_title, LV_ALIGN_TOP_LEFT, 12, 10);

    /* 分数 */
    lv_obj_t *lbl_sc = lv_label_create(parent);
    lv_label_set_text(lbl_sc, "Score:");
    lv_obj_set_style_text_color(lbl_sc, COLOR_TEXT_WHITE, 0);
    lv_obj_set_style_text_font(lbl_sc, &lv_font_montserrat_12, 0);
    lv_obj_align(lbl_sc, LV_ALIGN_TOP_LEFT, 12, 36);

    g2048.lbl_score = lv_label_create(parent);
    lv_label_set_text(g2048.lbl_score, "0");
    lv_obj_set_style_text_color(g2048.lbl_score, COLOR_TEXT_WHITE, 0);
    lv_obj_set_style_text_font(g2048.lbl_score, &lv_font_montserrat_12, 0);
    lv_obj_align(g2048.lbl_score, LV_ALIGN_TOP_LEFT, 56, 36);

    /* CAL 按钮 (左侧) */
    g2048.btn_calibrate = lv_btn_create(parent);
    lv_obj_set_size(g2048.btn_calibrate, 50, 24);
    lv_obj_align(g2048.btn_calibrate, LV_ALIGN_TOP_RIGHT, -72, 28);
    lv_obj_set_style_bg_color(g2048.btn_calibrate, lv_color_hex(0x2980B9), 0);
    lv_obj_set_style_radius(g2048.btn_calibrate, 12, 0);
    lv_obj_add_event_cb(g2048.btn_calibrate, g2048_calibrate_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *cal_lbl = lv_label_create(g2048.btn_calibrate);
    lv_label_set_text(cal_lbl, "CAL");
    lv_obj_set_style_text_font(cal_lbl, &lv_font_montserrat_10, 0);
    lv_obj_center(cal_lbl);

    /* Reset 按钮 (右侧) */
    g2048.btn_reset = lv_btn_create(parent);
    lv_obj_set_size(g2048.btn_reset, 50, 24);
    lv_obj_align(g2048.btn_reset, LV_ALIGN_TOP_RIGHT, -8, 28);
    lv_obj_set_style_bg_color(g2048.btn_reset, COLOR_NI_GREEN, 0);
    lv_obj_set_style_radius(g2048.btn_reset, 12, 0);
    lv_obj_add_event_cb(g2048.btn_reset, g2048_reset_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *rlbl = lv_label_create(g2048.btn_reset);
    lv_label_set_text(rlbl, LV_SYMBOL_REFRESH);
    lv_obj_set_style_text_font(rlbl, &lv_font_montserrat_12, 0);
    lv_obj_center(rlbl);

    /* 提示标签（校准流程 + 状态） */
    g2048.lbl_tilt_hint = lv_label_create(parent);
    lv_label_set_text(g2048.lbl_tilt_hint, "Press CAL to start");
    lv_obj_set_style_text_color(g2048.lbl_tilt_hint, lv_color_hex(0x7F8C8D), 0);
    lv_obj_set_style_text_font(g2048.lbl_tilt_hint, &lv_font_montserrat_10, 0);
    lv_obj_align(g2048.lbl_tilt_hint, LV_ALIGN_TOP_MID, 0, 55);

    /* 注册点击屏幕事件用于校准确认 */
    lv_obj_add_event_cb(parent, g2048_cal_tap_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(parent, LV_OBJ_FLAG_CLICKABLE);

    /* 网格背景 */
    g2048.grid_bg = lv_obj_create(parent);
    int grid_w = G2048_SIZE * CELL_SIZE + (G2048_SIZE - 1) * CELL_GAP + 12;
    int grid_h = grid_w;
    lv_obj_set_size(g2048.grid_bg, grid_w, grid_h);
    lv_obj_align(g2048.grid_bg, LV_ALIGN_TOP_MID, 0, G2048_GRID_TOP - 6);
    lv_obj_set_style_bg_color(g2048.grid_bg, lv_color_hex(0x1A2332), 0);
    lv_obj_set_style_bg_opa(g2048.grid_bg, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(g2048.grid_bg, 8, 0);
    lv_obj_set_style_border_width(g2048.grid_bg, 0, 0);
    lv_obj_set_style_pad_all(g2048.grid_bg, 0, 0);
    lv_obj_clear_flag(g2048.grid_bg, LV_OBJ_FLAG_SCROLLABLE);

    /* 创建格子 */
    for (int r = 0; r < G2048_SIZE; r++) {
        for (int c = 0; c < G2048_SIZE; c++) {
            int x = 6 + c * (CELL_SIZE + CELL_GAP);
            int y = 6 + r * (CELL_SIZE + CELL_GAP);
            lv_obj_t *cell = lv_obj_create(g2048.grid_bg);
            lv_obj_set_size(cell, CELL_SIZE, CELL_SIZE);
            lv_obj_set_pos(cell, x, y);
            lv_obj_set_style_radius(cell, 4, 0);
            lv_obj_set_style_border_width(cell, 0, 0);
            lv_obj_set_style_bg_color(cell, lv_color_hex(0x2C3E50), 0);
            lv_obj_set_style_bg_opa(cell, LV_OPA_COVER, 0);
            lv_obj_set_style_pad_all(cell, 0, 0);
            lv_obj_clear_flag(cell, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_clear_flag(cell, LV_OBJ_FLAG_CLICKABLE);
            g2048.cells[r][c] = cell;

            lv_obj_t *lbl = lv_label_create(cell);
            lv_label_set_text(lbl, "");
            lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
            lv_obj_center(lbl);
            g2048.cell_lbls[r][c] = lbl;
        }
    }

    /* 初始化棋盘 */
    g2048.animating = false;
    g2048.last_move_ms = 0;
    g2048_reset();

    /* 启动倾斜控制定时器（50ms 轮询，比传感器更新快） */
    g2048.tilt_timer = lv_timer_create(g2048_tilt_timer_cb, 50, NULL);
}

/* ═══════════════════════════════════════════════════════════════
 *  Page 0 — 名片主页
 * ═══════════════════════════════════════════════════════════════ */
/* ═══════════════════════════════════════════════════════════════
 *  Page 5 — Image Slideshow (SD Card)
 * ═══════════════════════════════════════════════════════════════ */
#define PLAYER_MAX_FILES 50
#define PLAYER_DIR "/sdcard/photos"

static struct {
    char filenames[PLAYER_MAX_FILES][100];
    int  file_count;
    int  current_idx;
    lv_obj_t *img;
    lv_obj_t *lbl_name;
    lv_obj_t *lbl_counter;
    lv_obj_t *btn_prev;
    lv_obj_t *btn_next;
    lv_obj_t *btn_auto;
    lv_timer_t *auto_timer;
    bool auto_playing;
    uint8_t *img_buf;       /* 动态分配的图片缓冲 */
    size_t   img_buf_size;
} player;

static void player_show_image(void)
{
    if (player.file_count <= 0) {
        lv_label_set_text(player.lbl_name, "No images found");
        lv_label_set_text(player.lbl_counter, "0/0");
        return;
    }

    /* 释放之前的缓冲 */
    if (player.img_buf) {
        free(player.img_buf);
        player.img_buf = NULL;
        player.img_buf_size = 0;
    }

    /* 构建完整路径 */
    char path[220];
    snprintf(path, sizeof(path), "%s/%s", PLAYER_DIR, player.filenames[player.current_idx]);

    /* 读取整个文件到内存 */
    FILE *f = fopen(path, "rb");
    if (!f) {
        lv_label_set_text(player.lbl_name, "Open failed");
        return;
    }
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    /* 限制单张图片大小 (2MB max) */
    if (fsize > 2 * 1024 * 1024 || fsize <= 0) {
        fclose(f);
        lv_label_set_text(player.lbl_name, "File too large");
        return;
    }

    player.img_buf = (uint8_t *)malloc(fsize);
    if (!player.img_buf) {
        fclose(f);
        lv_label_set_text(player.lbl_name, "Out of memory");
        return;
    }
    fread(player.img_buf, 1, fsize, f);
    fclose(f);
    player.img_buf_size = fsize;

    /* 确定图片格式并设置 LVGL 图片源 */
    const char *ext = strrchr(player.filenames[player.current_idx], '.');
    char src_str[240];
    if (ext && (strcasecmp(ext, ".png") == 0)) {
        /* 使用 LVGL 文件路径 (P:前缀 = POSIX) */
        snprintf(src_str, sizeof(src_str), "P:%s/%s", PLAYER_DIR, player.filenames[player.current_idx]);
        lv_img_set_src(player.img, src_str);
    } else if (ext && (strcasecmp(ext, ".jpg") == 0 || strcasecmp(ext, ".jpeg") == 0)) {
        snprintf(src_str, sizeof(src_str), "P:%s/%s", PLAYER_DIR, player.filenames[player.current_idx]);
        lv_img_set_src(player.img, src_str);
    } else if (ext && (strcasecmp(ext, ".bmp") == 0)) {
        snprintf(src_str, sizeof(src_str), "P:%s/%s", PLAYER_DIR, player.filenames[player.current_idx]);
        lv_img_set_src(player.img, src_str);
    } else {
        lv_label_set_text(player.lbl_name, "Unsupported fmt");
        return;
    }

    /* 更新文件名和计数器 */
    lv_label_set_text(player.lbl_name, player.filenames[player.current_idx]);
    char cnt_str[16];
    snprintf(cnt_str, sizeof(cnt_str), "%d/%d", player.current_idx + 1, player.file_count);
    lv_label_set_text(player.lbl_counter, cnt_str);
}

static void player_next_cb(lv_event_t *e)
{
    (void)e;
    if (player.file_count <= 0) return;
    player.current_idx = (player.current_idx + 1) % player.file_count;
    player_show_image();
}

static void player_prev_cb(lv_event_t *e)
{
    (void)e;
    if (player.file_count <= 0) return;
    player.current_idx = (player.current_idx - 1 + player.file_count) % player.file_count;
    player_show_image();
}

static void player_auto_timer_cb(lv_timer_t *t)
{
    (void)t;
    if (current_page != 5) return;
    player_next_cb(NULL);
}

static void player_auto_cb(lv_event_t *e)
{
    (void)e;
    player.auto_playing = !player.auto_playing;
    if (player.auto_playing) {
        if (!player.auto_timer) {
            player.auto_timer = lv_timer_create(player_auto_timer_cb, 3000, NULL);
        } else {
            lv_timer_resume(player.auto_timer);
        }
        lv_label_set_text(lv_obj_get_child(player.btn_auto, 0), LV_SYMBOL_PAUSE);
    } else {
        if (player.auto_timer) lv_timer_pause(player.auto_timer);
        lv_label_set_text(lv_obj_get_child(player.btn_auto, 0), LV_SYMBOL_PLAY);
    }
}

static void player_scan_files(void)
{
    /* 扫描常见图片格式 */
    player.file_count = 0;
    player.current_idx = 0;

    DIR *dir = opendir(PLAYER_DIR);
    if (!dir) {
        ESP_LOGW(TAG, "Player: cannot open %s", PLAYER_DIR);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && player.file_count < PLAYER_MAX_FILES) {
        if (entry->d_name[0] == '.') continue;
        const char *dot = strrchr(entry->d_name, '.');
        if (!dot) continue;
        if (strcasecmp(dot, ".jpg") == 0 || strcasecmp(dot, ".jpeg") == 0 ||
            strcasecmp(dot, ".png") == 0 || strcasecmp(dot, ".bmp") == 0) {
            strncpy(player.filenames[player.file_count], entry->d_name, 99);
            player.filenames[player.file_count][99] = '\0';
            player.file_count++;
        }
    }
    closedir(dir);
    ESP_LOGI(TAG, "Player: found %d images in %s", player.file_count, PLAYER_DIR);
}

static void build_page_player(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0D1117), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

    /* 标题 */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, LV_SYMBOL_IMAGE " Gallery");
    lv_obj_set_style_text_color(title, COLOR_TEXT_WHITE, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 6);

    /* 图片显示区域 */
    player.img = lv_img_create(parent);
    lv_obj_set_size(player.img, 220, 220);
    lv_obj_align(player.img, LV_ALIGN_CENTER, 0, -10);
    lv_img_set_size_mode(player.img, LV_IMG_SIZE_MODE_REAL);
    lv_obj_set_style_radius(player.img, 8, 0);
    lv_obj_set_style_clip_corner(player.img, true, 0);

    /* 文件名 */
    player.lbl_name = lv_label_create(parent);
    lv_label_set_text(player.lbl_name, "Scanning...");
    lv_obj_set_style_text_color(player.lbl_name, lv_color_hex(0x7F8C8D), 0);
    lv_obj_set_style_text_font(player.lbl_name, &lv_font_montserrat_10, 0);
    lv_obj_set_width(player.lbl_name, 200);
    lv_label_set_long_mode(player.lbl_name, LV_LABEL_LONG_DOT);
    lv_obj_align(player.lbl_name, LV_ALIGN_BOTTOM_MID, 0, -36);

    /* 计数器 */
    player.lbl_counter = lv_label_create(parent);
    lv_label_set_text(player.lbl_counter, "0/0");
    lv_obj_set_style_text_color(player.lbl_counter, COLOR_TEXT_WHITE, 0);
    lv_obj_set_style_text_font(player.lbl_counter, &lv_font_montserrat_10, 0);
    lv_obj_align(player.lbl_counter, LV_ALIGN_TOP_RIGHT, -8, 8);

    /* 控制按钮行: < | ▶/⏸ | > */
    int btn_y = -8;
    int btn_h = 30;
    int btn_w = 50;

    /* Prev */
    player.btn_prev = lv_btn_create(parent);
    lv_obj_set_size(player.btn_prev, btn_w, btn_h);
    lv_obj_align(player.btn_prev, LV_ALIGN_BOTTOM_LEFT, 20, btn_y);
    lv_obj_set_style_bg_color(player.btn_prev, lv_color_hex(0x2C3E50), 0);
    lv_obj_set_style_radius(player.btn_prev, 15, 0);
    lv_obj_add_event_cb(player.btn_prev, player_prev_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lp = lv_label_create(player.btn_prev);
    lv_label_set_text(lp, LV_SYMBOL_LEFT);
    lv_obj_center(lp);

    /* Auto/Pause */
    player.btn_auto = lv_btn_create(parent);
    lv_obj_set_size(player.btn_auto, btn_w, btn_h);
    lv_obj_align(player.btn_auto, LV_ALIGN_BOTTOM_MID, 0, btn_y);
    lv_obj_set_style_bg_color(player.btn_auto, COLOR_NI_GREEN, 0);
    lv_obj_set_style_radius(player.btn_auto, 15, 0);
    lv_obj_add_event_cb(player.btn_auto, player_auto_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *la = lv_label_create(player.btn_auto);
    lv_label_set_text(la, LV_SYMBOL_PLAY);
    lv_obj_center(la);

    /* Next */
    player.btn_next = lv_btn_create(parent);
    lv_obj_set_size(player.btn_next, btn_w, btn_h);
    lv_obj_align(player.btn_next, LV_ALIGN_BOTTOM_RIGHT, -20, btn_y);
    lv_obj_set_style_bg_color(player.btn_next, lv_color_hex(0x2C3E50), 0);
    lv_obj_set_style_radius(player.btn_next, 15, 0);
    lv_obj_add_event_cb(player.btn_next, player_next_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *ln = lv_label_create(player.btn_next);
    lv_label_set_text(ln, LV_SYMBOL_RIGHT);
    lv_obj_center(ln);

    /* 初始化 */
    player.img_buf = NULL;
    player.img_buf_size = 0;
    player.auto_playing = false;
    player.auto_timer = NULL;
    player_scan_files();
    player_show_image();
}

/* ── Settings button callback ── */
static void settings_btn_cb(lv_event_t *e)
{
    (void)e;
    page_settings_show();
}

/* ── Easter egg: long press 2s on name label ── */
static lv_timer_t *easter_egg_timer = NULL;

static void easter_egg_timer_cb(lv_timer_t *t)
{
    lv_timer_del(t);
    easter_egg_timer = NULL;
    ESP_LOGI(TAG, "Easter egg triggered! (2s long press)");
    easter_egg_show();
}

/* Helper for animating label opacity */
static void name_anim_opa_cb(void *obj, int32_t v)
{
    lv_obj_set_style_opa((lv_obj_t *)obj, (lv_opa_t)v, 0);
}

/* Helper for animating label zoom */
static void name_anim_zoom_cb(void *obj, int32_t v)
{
    lv_obj_set_style_transform_zoom((lv_obj_t *)obj, (uint16_t)v, 0);
}

static void name_press_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_PRESSED) {
        /* Start 2-second timer */
        if (easter_egg_timer) lv_timer_del(easter_egg_timer);
        easter_egg_timer = lv_timer_create(easter_egg_timer_cb, 2000, NULL);
        lv_timer_set_repeat_count(easter_egg_timer, 1);
        /* Visual feedback: scale up + blink */
        if (lbl_card_name) {
            lv_anim_t a;
            lv_anim_init(&a);
            lv_anim_set_var(&a, lbl_card_name);
            lv_anim_set_values(&a, 256, 296);  /* 100% -> ~116% scale */
            lv_anim_set_time(&a, 400);
            lv_anim_set_playback_time(&a, 400);
            lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
            lv_anim_set_exec_cb(&a, name_anim_zoom_cb);
            lv_anim_start(&a);
            /* Also pulse opacity for blink effect */
            lv_anim_t b;
            lv_anim_init(&b);
            lv_anim_set_var(&b, lbl_card_name);
            lv_anim_set_values(&b, LV_OPA_COVER, LV_OPA_60);
            lv_anim_set_time(&b, 300);
            lv_anim_set_playback_time(&b, 300);
            lv_anim_set_repeat_count(&b, LV_ANIM_REPEAT_INFINITE);
            lv_anim_set_exec_cb(&b, name_anim_opa_cb);
            lv_anim_start(&b);
        }
    } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        /* Cancel if released before 2s */
        if (easter_egg_timer) {
            lv_timer_del(easter_egg_timer);
            easter_egg_timer = NULL;
        }
        /* Reset visual state */
        if (lbl_card_name) {
            lv_anim_del(lbl_card_name, NULL);  /* remove all anims on this obj */
            lv_obj_set_style_transform_zoom(lbl_card_name, 256, 0);  /* reset to 100% */
            lv_obj_set_style_opa(lbl_card_name, LV_OPA_COVER, 0);
        }
    }
}

static void build_page_card(lv_obj_t *parent)
{
    draw_geometric_bg(parent);
    create_status_bar(parent);
    create_avatar(parent);
    create_name_block(parent);
    create_anniversary_logo(parent);
    create_page_indicators(parent);
}

/* ═══════════════════════════════════════════════════════════════
 *  公开接口
 * ═══════════════════════════════════════════════════════════════ */
void Badge_UI_Init(void)
{
    ESP_LOGI(TAG, "Badge UI initializing...");

    /* 设置显示背景为深蓝，消除 flush 间的白色撕裂线 */
    lv_disp_set_bg_color(lv_disp_get_default(), COLOR_BG_DARK);
    lv_disp_set_bg_opa(lv_disp_get_default(), LV_OPA_COVER);
    lv_obj_set_style_bg_color(lv_scr_act(), COLOR_BG_DARK, 0);
    lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, 0);

    /* 计算启用的页面总数 (顺序: Card→History→Milestone→Gallery→Calendar→...→Config) */
    num_pages = 0;
#ifdef CONFIG_PAGE_CARD
    num_pages++;
#endif
#ifdef CONFIG_PAGE_HISTORY
    num_pages++;
#endif
#ifdef CONFIG_PAGE_MILESTONE
    num_pages++;
#endif
#ifdef CONFIG_PAGE_GALLERY
    num_pages++;
#endif
#ifdef CONFIG_PAGE_CALENDAR
    num_pages++;
#endif
#ifdef CONFIG_PAGE_DAQ
    num_pages++;
#endif
#ifdef CONFIG_PAGE_IMPACT
    num_pages++;
#endif
#ifdef CONFIG_PAGE_IDENTITY
    num_pages++;
#endif
#ifdef CONFIG_PAGE_GAME
    num_pages++;
#endif
#ifdef CONFIG_PAGE_2048
    num_pages++;
#endif
#ifdef CONFIG_PAGE_PLAYER
    num_pages++;
#endif
#ifdef CONFIG_PAGE_MATCH3
    num_pages++;
#endif
#ifdef CONFIG_PAGE_CONFIG
    num_pages++;
#endif

    /* 创建 tileview */
    tileview = lv_tileview_create(lv_scr_act());
    lv_obj_set_size(tileview, SCREEN_W, SCREEN_H);
    lv_obj_align(tileview, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_bg_color(tileview, COLOR_BG_DARK, 0);
    lv_obj_set_style_bg_opa(tileview, LV_OPA_COVER, 0);
    lv_obj_set_scrollbar_mode(tileview, LV_SCROLLBAR_MODE_OFF);

    /* 动态添加 tile — 按启用顺序排列 (Card→History→Milestone→Gallery→Calendar→...→Config) */
    int col = 0;

#ifdef CONFIG_PAGE_CARD
    tile_card = lv_tileview_add_tile(tileview, col, 0,
        (col == 0 ? 0 : LV_DIR_LEFT) | (col < num_pages - 1 ? LV_DIR_RIGHT : 0));
    col++;
#endif
#ifdef CONFIG_PAGE_HISTORY
    tile_history = lv_tileview_add_tile(tileview, col, 0,
        (col == 0 ? 0 : LV_DIR_LEFT) | (col < num_pages - 1 ? LV_DIR_RIGHT : 0));
    col++;
#endif
#ifdef CONFIG_PAGE_MILESTONE
    tile_milestone = lv_tileview_add_tile(tileview, col, 0,
        (col == 0 ? 0 : LV_DIR_LEFT) | (col < num_pages - 1 ? LV_DIR_RIGHT : 0));
    col++;
#endif
#ifdef CONFIG_PAGE_GALLERY
    tile_gallery = lv_tileview_add_tile(tileview, col, 0,
        (col == 0 ? 0 : LV_DIR_LEFT) | (col < num_pages - 1 ? LV_DIR_RIGHT : 0));
    col++;
#endif
#ifdef CONFIG_PAGE_CALENDAR
    tile_calendar = lv_tileview_add_tile(tileview, col, 0,
        (col == 0 ? 0 : LV_DIR_LEFT) | (col < num_pages - 1 ? LV_DIR_RIGHT : 0));
    col++;
#endif
#ifdef CONFIG_PAGE_DAQ
    tile_daq = lv_tileview_add_tile(tileview, col, 0,
        (col == 0 ? 0 : LV_DIR_LEFT) | (col < num_pages - 1 ? LV_DIR_RIGHT : 0));
    col++;
#endif
#ifdef CONFIG_PAGE_IMPACT
    tile_impact = lv_tileview_add_tile(tileview, col, 0,
        (col == 0 ? 0 : LV_DIR_LEFT) | (col < num_pages - 1 ? LV_DIR_RIGHT : 0));
    col++;
#endif
#ifdef CONFIG_PAGE_IDENTITY
    tile_identity = lv_tileview_add_tile(tileview, col, 0,
        (col == 0 ? 0 : LV_DIR_LEFT) | (col < num_pages - 1 ? LV_DIR_RIGHT : 0));
    col++;
#endif
#ifdef CONFIG_PAGE_GAME
    tile_game = lv_tileview_add_tile(tileview, col, 0,
        (col == 0 ? 0 : LV_DIR_LEFT) | (col < num_pages - 1 ? LV_DIR_RIGHT : 0));
    col++;
#endif
#ifdef CONFIG_PAGE_2048
    tile_2048 = lv_tileview_add_tile(tileview, col, 0,
        (col == 0 ? 0 : LV_DIR_LEFT) | (col < num_pages - 1 ? LV_DIR_RIGHT : 0));
    col++;
#endif
#ifdef CONFIG_PAGE_PLAYER
    tile_player = lv_tileview_add_tile(tileview, col, 0,
        (col == 0 ? 0 : LV_DIR_LEFT) | (col < num_pages - 1 ? LV_DIR_RIGHT : 0));
    col++;
#endif
#ifdef CONFIG_PAGE_MATCH3
    tile_match3 = lv_tileview_add_tile(tileview, col, 0,
        (col == 0 ? 0 : LV_DIR_LEFT) | (col < num_pages - 1 ? LV_DIR_RIGHT : 0));
    col++;
#endif
#ifdef CONFIG_PAGE_CONFIG
    tile_config = lv_tileview_add_tile(tileview, col, 0,
        (col == 0 ? 0 : LV_DIR_LEFT) | (col < num_pages - 1 ? LV_DIR_RIGHT : 0));
    col++;
#endif

    /* 构建各页内容 (顺序: Card→History→Milestone→Gallery→Calendar→...→Config) */
#ifdef CONFIG_PAGE_CARD
    build_page_card(tile_card);
#endif
#ifdef CONFIG_PAGE_HISTORY
    build_page_history(tile_history);
    create_home_btn(tile_history);
#endif
#ifdef CONFIG_PAGE_MILESTONE
    build_page_milestone(tile_milestone);
    create_home_btn(tile_milestone);
#endif
#ifdef CONFIG_PAGE_GALLERY
    build_page_gallery(tile_gallery);
    create_home_btn(tile_gallery);
#endif
#ifdef CONFIG_PAGE_CALENDAR
    build_page_calendar(tile_calendar);
    create_home_btn(tile_calendar);
#endif
#ifdef CONFIG_PAGE_DAQ
    build_page_daq(tile_daq);
    create_home_btn(tile_daq);
#endif
#ifdef CONFIG_PAGE_IMPACT
    build_page_impact(tile_impact);
    create_home_btn(tile_impact);
#endif
#ifdef CONFIG_PAGE_IDENTITY
    build_page_identity(tile_identity);
    create_home_btn(tile_identity);
#endif
#ifdef CONFIG_PAGE_GAME
    build_page_game(tile_game);
    create_home_btn(tile_game);
#endif
#ifdef CONFIG_PAGE_2048
    build_page_2048(tile_2048);
    create_home_btn(tile_2048);
#endif
#ifdef CONFIG_PAGE_PLAYER
    build_page_player(tile_player);
    create_home_btn(tile_player);
#endif
#ifdef CONFIG_PAGE_MATCH3
    build_lazy_placeholder(tile_match3, "NI Match-3", "Slide here to load");
    create_home_btn(tile_match3);
#endif
#ifdef CONFIG_PAGE_CONFIG
    build_page_config(tile_config);
    create_home_btn(tile_config);
#endif

    /* 注册滑页事件 */
    lv_obj_add_event_cb(tileview, tileview_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* 启动状态栏定时刷新（每秒） */
    lv_timer_create(status_bar_timer_cb, 1000, NULL);

    /* Store reference to main screen for settings/easter_egg to return */
    badge_main_screen = lv_scr_act();

    /* Initialize sleep activity timer */
    settings_touch_activity();

    ESP_LOGI(TAG, "Badge UI ready — %d tiles created", num_pages);
}
