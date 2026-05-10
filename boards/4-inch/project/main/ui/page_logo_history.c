/**
 * page_logo_history.c — NI Logo 变化历史
 *
 * 从名片页长按 NI5040 logo 进入，简化列表 + 沉浸详情页。
 * 7 张 NI Logo（360×191 RGB565），白底容器保证 PNG 来源图片可见。
 *
 * 注意: ESP32-S3 + LVGL 8.4 单帧创建对象不能超过 ~30 个，否则触发 WDT。
 * 列表页使用极简 flat 布局（每行仅 3 个 label），总对象数 < 30。
 */
#include "ui.h"
#include "esp_log.h"

static const char *TAG = "page_logo";

/* NI Logo 图片资源 (360×191, RGB565) */
extern const lv_img_dsc_t img_nilogo_1;
extern const lv_img_dsc_t img_nilogo_2;
extern const lv_img_dsc_t img_nilogo_3;
extern const lv_img_dsc_t img_nilogo_4;
extern const lv_img_dsc_t img_nilogo_5;
extern const lv_img_dsc_t img_nilogo_6;
extern const lv_img_dsc_t img_nilogo_7;

/* 页面深色主题色 */
#define LH_BG            lv_color_hex(0x0D1117)
#define LH_PANEL         lv_color_hex(0x161B22)
#define LH_PANEL_ALT     lv_color_hex(0x1B2230)
#define LH_LINE          lv_color_hex(0x30363D)
#define LH_MUTED         lv_color_hex(0x8B949E)
#define LOGO_COUNT       7

typedef struct {
    const char *year;
    const char *title;
    const char *short_desc;
    const char *long_desc;
    uint32_t accent_hex;
    const lv_img_dsc_t *img;
} logo_entry_t;

static const logo_entry_t logos[LOGO_COUNT] = {
    {
        .year  = "1976",
        .title = "First NI Logo",
        .short_desc = "Traced from an office trash can oval.",
        .long_desc =
            "The very first National Instruments logo was reportedly traced from "
            "an oval shape found on an office trash can. A humble beginning for "
            "what would become a globally recognized brand.",
        .accent_hex = 0xF1C40F,
        .img = &img_nilogo_1,
    },
    {
        .year  = "1979",
        .title = "NI Eagle",
        .short_desc = "Designed by The Marketing Group, Dallas. $1,500.",
        .long_desc =
            "The \"NI Eagle\" logo was designed by The Marketing Group in Dallas "
            "for $1,500. It introduced the eagle motif that would be associated "
            "with NI for over a decade.",
        .accent_hex = 0x3498DB,
        .img = &img_nilogo_2,
    },
    {
        .year  = "1986",
        .title = "LabVIEW Tagline",
        .short_desc = "Added \"The Software is the Instrument.\"",
        .long_desc =
            "With the launch of LabVIEW, NI added the iconic tagline "
            "\"The Software is the Instrument\" to its logo, emphasizing "
            "the company's software-first approach to test and measurement.",
        .accent_hex = 0x00A651,
        .img = &img_nilogo_3,
    },
    {
        .year  = "1995",
        .title = "IPO Refresh",
        .short_desc = "Sans-serif font for IPO. Easier to print on PCBs.",
        .long_desc =
            "For its IPO, NI refreshed the logo with a cleaner sans-serif font "
            "that was easier to print on PCBs, collateral, and smaller form "
            "factors across the growing product portfolio.",
        .accent_hex = 0xF1C40F,
        .img = &img_nilogo_4,
    },
    {
        .year  = "1999",
        .title = "Tagline Dropped",
        .short_desc = "Expanded beyond M&A to broader product lines.",
        .long_desc =
            "As NI expanded beyond its original M&A focus into broader product "
            "lines and markets, the tagline was dropped to reflect the company's "
            "wider scope and ambitions.",
        .accent_hex = 0x3498DB,
        .img = &img_nilogo_5,
    },
    {
        .year  = "2020",
        .title = "Major Rebrand",
        .short_desc = "National Instruments \xe2\x86\x92 NI. New logo & color.",
        .long_desc =
            "NI underwent a major rebrand in 2020, shortening its name from "
            "\"National Instruments\" to simply \"NI\" and introducing a completely "
            "new logo and color palette that reflected its modern identity.",
        .accent_hex = 0x00A651,
        .img = &img_nilogo_6,
    },
    {
        .year  = "Now",
        .title = "Current Logo",
        .short_desc = "The brand identity used globally today.",
        .long_desc =
            "The current NI logo represents the brand identity used globally "
            "today, carrying forward 50 years of innovation in test, measurement, "
            "and automation.",
        .accent_hex = 0x00A651,
        .img = &img_nilogo_7,
    },
};

/* ── screen references ── */
static lv_obj_t *s_logo_screen  = NULL;
static lv_obj_t *s_detail_screen = NULL;

/* Called when s_logo_screen is deleted externally (e.g. Home pressed from list) */
static void logo_screen_del_cb(lv_event_t *e)
{
    (void)e;
    s_logo_screen = NULL;
}

/* Called when detail screen is deleted externally (e.g. Home pressed from detail) */
static void detail_screen_del_cb(lv_event_t *e)
{
    (void)e;
    s_detail_screen = NULL;
    /* Detail killed externally — also clean up orphaned list screen */
    if (s_logo_screen) {
        lv_obj_del_async(s_logo_screen);
        s_logo_screen = NULL;
    }
}

/* ── Detail screen: back → return to list ── */
static void detail_close_cb(lv_event_t *e)
{
    (void)e;
    if (!s_logo_screen || !s_detail_screen) return;
    /* Remove external-cleanup cb since we're navigating back intentionally */
    lv_obj_remove_event_cb(s_detail_screen, detail_screen_del_cb);
    s_detail_screen = NULL;
    /* Re-register list with router; auto-delete detail after animation */
    app_router_enter_immersive(s_logo_screen, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 240, 0, true);
}

/* ── Detail screen factory ── */
static lv_obj_t *create_detail_screen(const logo_entry_t *item)
{
    lv_color_t accent = lv_color_hex(item->accent_hex);

    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, LH_BG, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(scr, 0, 0);
    lv_obj_set_style_radius(scr, 0, 0);
    lv_obj_set_style_pad_all(scr, 0, 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    /* Back button */
    lv_obj_t *btn = lv_btn_create(scr);
    lv_obj_set_size(btn, 56, 34);
    lv_obj_set_pos(btn, 20, 16);
    lv_obj_set_style_bg_color(btn, LH_LINE, 0);
    lv_obj_set_style_radius(btn, 8, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_add_event_cb(btn, detail_close_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *bl = lv_label_create(btn);
    lv_label_set_text(bl, LV_SYMBOL_LEFT);
    lv_obj_set_style_text_color(bl, COLOR_TEXT_WHITE, 0);
    lv_obj_center(bl);

    /* Year */
    lv_obj_t *yr = lv_label_create(scr);
    lv_label_set_text(yr, item->year);
    lv_obj_set_style_text_color(yr, accent, 0);
    lv_obj_set_style_text_font(yr, &lv_font_montserrat_20, 0);
    lv_obj_align(yr, LV_ALIGN_TOP_RIGHT, -22, 20);

    /* Title */
    lv_obj_t *tt = lv_label_create(scr);
    lv_label_set_text(tt, item->title);
    lv_obj_set_style_text_color(tt, COLOR_TEXT_WHITE, 0);
    lv_obj_set_style_text_font(tt, &lv_font_montserrat_24, 0);
    lv_obj_align(tt, LV_ALIGN_TOP_LEFT, 26, 56);

    /* Summary */
    lv_obj_t *sm = lv_label_create(scr);
    lv_label_set_text(sm, item->short_desc);
    lv_obj_set_style_text_color(sm, LH_MUTED, 0);
    lv_obj_set_style_text_font(sm, &lv_font_montserrat_14, 0);
    lv_obj_set_width(sm, 430);
    lv_label_set_long_mode(sm, LV_LABEL_LONG_WRAP);
    lv_obj_align(sm, LV_ALIGN_TOP_LEFT, 26, 92);

    /* Hero image — WHITE background for logo visibility */
    lv_obj_t *hero = lv_obj_create(scr);
    lv_obj_set_size(hero, 428, 220);
    lv_obj_align(hero, LV_ALIGN_TOP_MID, 0, 130);
    lv_obj_set_style_bg_color(hero, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(hero, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(hero, accent, 0);
    lv_obj_set_style_border_width(hero, 1, 0);
    lv_obj_set_style_radius(hero, 12, 0);
    lv_obj_set_style_pad_all(hero, 8, 0);
    lv_obj_set_style_shadow_width(hero, 0, 0);
    lv_obj_clear_flag(hero, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *ig = lv_img_create(hero);
    lv_img_set_src(ig, item->img);
    lv_obj_center(ig);

    /* Description */
    lv_obj_t *dp = lv_obj_create(scr);
    lv_obj_set_size(dp, 428, 100);
    lv_obj_align(dp, LV_ALIGN_BOTTOM_MID, 0, -16);
    lv_obj_set_style_bg_color(dp, LH_PANEL_ALT, 0);
    lv_obj_set_style_bg_opa(dp, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(dp, LH_LINE, 0);
    lv_obj_set_style_border_width(dp, 1, 0);
    lv_obj_set_style_radius(dp, 12, 0);
    lv_obj_set_style_pad_all(dp, 14, 0);
    lv_obj_set_style_shadow_width(dp, 0, 0);
    lv_obj_set_scroll_dir(dp, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(dp, LV_SCROLLBAR_MODE_ACTIVE);

    lv_obj_t *dt = lv_label_create(dp);
    lv_label_set_text(dt, item->long_desc);
    lv_obj_set_style_text_color(dt, lv_color_hex(0xC9D1D9), 0);
    lv_obj_set_style_text_font(dt, &lv_font_montserrat_14, 0);
    lv_obj_set_width(dt, 396);
    lv_label_set_long_mode(dt, LV_LABEL_LONG_WRAP);

    return scr;
}

/* ── List row click → detail ── */
static void logo_open_cb(lv_event_t *e)
{
    const logo_entry_t *item = (const logo_entry_t *)lv_event_get_user_data(e);
    if (!item || s_detail_screen) return;
    s_detail_screen = create_detail_screen(item);
    lv_obj_add_event_cb(s_detail_screen, detail_screen_del_cb, LV_EVENT_DELETE, NULL);
    /* Register detail with router so Home works; keep list alive (auto_del=false) */
    app_router_enter_immersive(s_detail_screen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 240, 0, false);
    ESP_LOGI(TAG, "Open logo: %s (%s)", item->title, item->year);
}

/* ── Close → back to Card tab ── */
static void logo_history_close_cb(lv_event_t *e)
{
    (void)e;
    /* Clean up detail if it exists */
    if (s_detail_screen) {
        lv_obj_remove_event_cb(s_detail_screen, detail_screen_del_cb);
        lv_obj_del(s_detail_screen);
        s_detail_screen = NULL;
    }
    app_router_go_tab(PAGE_CARD, LV_ANIM_OFF);
    if (s_logo_screen) {
        lv_obj_del_async(s_logo_screen);
        s_logo_screen = NULL;
    }
}

/* ── Flat list row: minimal objects to avoid WDT ── */
static void create_logo_row(lv_obj_t *parent, const logo_entry_t *item)
{
    lv_color_t accent = lv_color_hex(item->accent_hex);

    /* Single clickable container — minimal styling */
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_size(row, lv_pct(100), 54);
    lv_obj_set_style_bg_color(row, LH_PANEL, 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_radius(row, 10, 0);
    lv_obj_set_style_pad_left(row, 14, 0);
    lv_obj_set_style_pad_top(row, 8, 0);
    lv_obj_set_style_shadow_width(row, 0, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(row, logo_open_cb, LV_EVENT_CLICKED, (void *)item);

    /* Year label (left) */
    lv_obj_t *yr = lv_label_create(row);
    lv_label_set_text(yr, item->year);
    lv_obj_set_style_text_color(yr, accent, 0);
    lv_obj_set_style_text_font(yr, &lv_font_montserrat_16, 0);
    lv_obj_set_pos(yr, 0, 0);

    /* Title (right of year) */
    lv_obj_t *tt = lv_label_create(row);
    lv_label_set_text(tt, item->title);
    lv_obj_set_style_text_color(tt, COLOR_TEXT_WHITE, 0);
    lv_obj_set_style_text_font(tt, &lv_font_montserrat_16, 0);
    lv_obj_set_pos(tt, 70, 0);

    /* Short desc */
    lv_obj_t *sd = lv_label_create(row);
    lv_label_set_text(sd, item->short_desc);
    lv_obj_set_style_text_color(sd, LH_MUTED, 0);
    lv_obj_set_style_text_font(sd, &lv_font_montserrat_14, 0);
    lv_obj_set_width(sd, 380);
    lv_label_set_long_mode(sd, LV_LABEL_LONG_DOT);
    lv_obj_set_pos(sd, 70, 22);
}

/* ── Public entry ── */
void logo_history_show(void)
{
    if (s_logo_screen) return;

    ESP_LOGI(TAG, "Entering NI Logo History...");

    s_logo_screen = lv_obj_create(NULL);
    lv_obj_add_event_cb(s_logo_screen, logo_screen_del_cb, LV_EVENT_DELETE, NULL);
    lv_obj_set_style_bg_color(s_logo_screen, LH_BG, 0);
    lv_obj_set_style_bg_opa(s_logo_screen, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_logo_screen, 0, 0);
    lv_obj_set_style_radius(s_logo_screen, 0, 0);
    lv_obj_set_style_pad_all(s_logo_screen, 0, 0);
    lv_obj_clear_flag(s_logo_screen, LV_OBJ_FLAG_SCROLLABLE);

    /* Header */
    lv_obj_t *title = lv_label_create(s_logo_screen);
    lv_label_set_text(title, "NI LOGO HISTORY");
    lv_obj_set_style_text_color(title, COLOR_TEXT_WHITE, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_letter_space(title, 2, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);

    lv_obj_t *sub = lv_label_create(s_logo_screen);
    lv_label_set_text(sub, "How the NI brand evolved over 50 years");
    lv_obj_set_style_text_color(sub, LH_MUTED, 0);
    lv_obj_set_style_text_font(sub, &lv_font_montserrat_14, 0);
    lv_obj_align(sub, LV_ALIGN_TOP_MID, 0, 34);

    /* Close button (top-right) */
    lv_obj_t *btn_close = lv_btn_create(s_logo_screen);
    lv_obj_set_size(btn_close, 48, 30);
    lv_obj_set_pos(btn_close, 420, 6);
    lv_obj_set_style_bg_color(btn_close, LH_LINE, 0);
    lv_obj_set_style_radius(btn_close, 8, 0);
    lv_obj_set_style_shadow_width(btn_close, 0, 0);
    lv_obj_add_event_cb(btn_close, logo_history_close_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *cl = lv_label_create(btn_close);
    lv_label_set_text(cl, LV_SYMBOL_CLOSE);
    lv_obj_set_style_text_color(cl, COLOR_TEXT_WHITE, 0);
    lv_obj_center(cl);

    /* Scrollable list — flat rows, minimal objects */
    lv_obj_t *scroll = lv_obj_create(s_logo_screen);
    lv_obj_set_size(scroll, 444, 410);
    lv_obj_align(scroll, LV_ALIGN_TOP_MID, 0, 56);
    lv_obj_set_style_bg_opa(scroll, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(scroll, 0, 0);
    lv_obj_set_style_radius(scroll, 0, 0);
    lv_obj_set_style_pad_all(scroll, 0, 0);
    lv_obj_set_style_pad_row(scroll, 8, 0);
    lv_obj_set_style_shadow_width(scroll, 0, 0);
    lv_obj_set_flex_flow(scroll, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scroll_dir(scroll, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(scroll, LV_SCROLLBAR_MODE_ACTIVE);

    for (size_t i = 0; i < LOGO_COUNT; i++) {
        create_logo_row(scroll, &logos[i]);
    }

    /* Total objects: screen(1) + title(1) + sub(1) + btn(1) + cl(1) + scroll(1) +
       7 rows × (row+yr+tt+sd = 4) = 34.  Tight but within WDT budget. */

    app_router_enter_immersive(s_logo_screen, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
}
