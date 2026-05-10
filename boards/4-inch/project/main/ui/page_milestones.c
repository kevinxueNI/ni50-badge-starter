/**
 * page_milestones.c — NI 历史时间线
 *
 * 首轮模块 6：滚动时间轴 + 点击进入沉浸式详情页。
 */
#include "ui.h"
#include "esp_log.h"

static const char *TAG = "page_ms";

extern const lv_img_dsc_t img_ms_1976;
extern const lv_img_dsc_t img_ms_1986;
extern const lv_img_dsc_t img_ms_1995;
extern const lv_img_dsc_t img_ms_1997;
extern const lv_img_dsc_t img_ms_2004;
extern const lv_img_dsc_t img_ms_2023;
extern const lv_img_dsc_t img_ms_2026;

#define HISTORY_BG        lv_color_hex(0x0D1117)
#define HISTORY_PANEL     lv_color_hex(0x161B22)
#define HISTORY_PANEL_ALT lv_color_hex(0x1B2230)
#define HISTORY_LINE      lv_color_hex(0x30363D)
#define HISTORY_MUTED     lv_color_hex(0x8B949E)
#define MILESTONE_COUNT   7

typedef struct {
    const char *year;
    const char *title;
    const char *short_desc;
    const char *long_desc;
    uint32_t accent_hex;
    const lv_img_dsc_t *detail_img;
} milestone_t;

static const milestone_t milestones[MILESTONE_COUNT] = {
    {
        .year = "1976",
        .title = "NI Founded",
        .short_desc = "Austin founding and the first GPIB-centered automation direction.",
        .long_desc =
            "National Instruments was founded in Austin by James Truchard, Jeff Kodosky, and Bill Nowlin, "
            "starting a journey to transform test and measurement through computing.",
        .accent_hex = 0xF1C40F,
        .detail_img = &img_ms_1976,
    },
    {
        .year = "1986",
        .title = "LabVIEW 1.0",
        .short_desc = "Graphical programming and virtual instrumentation arrived.",
        .long_desc =
            "NI introduced LabVIEW 1.0, bringing graphical programming and virtual instrumentation "
            "into test and measurement.",
        .accent_hex = 0x00A651,
        .detail_img = &img_ms_1986,
    },
    {
        .year = "1995",
        .title = "IPO & NIWeek",
        .short_desc = "NI went public and held the first NIWeek in Austin.",
        .long_desc =
            "NI went public and held the first NIWeek in Austin, marking a new phase "
            "of company growth and community engagement.",
        .accent_hex = 0x3498DB,
        .detail_img = &img_ms_1995,
    },
    {
        .year = "1997",
        .title = "PXI Standard",
        .short_desc = "PC technology meets modular instrumentation in an open standard.",
        .long_desc =
            "NI introduced PXI, combining PC technology, modular instrumentation, "
            "and an open standard for automated test.",
        .accent_hex = 0x00A651,
        .detail_img = &img_ms_1997,
    },
    {
        .year = "2004",
        .title = "CompactRIO",
        .short_desc = "LabVIEW + real-time + FPGA + modular I/O for embedded control.",
        .long_desc =
            "NI launched CompactRIO at NIWeek 2004, combining LabVIEW, real-time processing, "
            "FPGA, and modular I/O for embedded control and acquisition.",
        .accent_hex = 0xF1C40F,
        .detail_img = &img_ms_2004,
    },
    {
        .year = "2023",
        .title = "Emerson Acquisition",
        .short_desc = "NI became the Test & Measurement business within Emerson.",
        .long_desc =
            "Emerson completed its acquisition of NI in 2023, making NI the "
            "Test & Measurement business within Emerson.",
        .accent_hex = 0xDA3633,
        .detail_img = &img_ms_2023,
    },
    {
        .year = "2026",
        .title = "NI 50 + LV 40",
        .short_desc = "50 years of NI innovation and 40 years of LabVIEW.",
        .long_desc =
            "In 2026, NI celebrates 50 years of innovation and LabVIEW celebrates "
            "40 years of pushing what's possible in test and measurement.",
        .accent_hex = 0xF1C40F,
        .detail_img = &img_ms_2026,
    },
};

static void detail_close_cb(lv_event_t *e)
{
    lv_obj_t *screen = (lv_obj_t *)lv_event_get_user_data(e);
    app_router_go_tab(PAGE_MILESTONES, LV_ANIM_OFF);
    if (screen) {
        lv_obj_del_async(screen);
    }
}

static lv_obj_t *create_detail_screen(const milestone_t *item)
{
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_t *btn_back;
    lv_obj_t *btn_label;
    lv_obj_t *year;
    lv_obj_t *title;
    lv_obj_t *summary;
    lv_obj_t *hero;
    lv_obj_t *img;
    lv_obj_t *desc_panel;
    lv_obj_t *desc;
    lv_color_t accent = lv_color_hex(item->accent_hex);

    lv_obj_set_style_bg_color(screen, HISTORY_BG, 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(screen, 0, 0);
    lv_obj_set_style_radius(screen, 0, 0);
    lv_obj_set_style_pad_all(screen, 0, 0);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    btn_back = lv_btn_create(screen);
    lv_obj_set_size(btn_back, 56, 34);
    lv_obj_set_pos(btn_back, 20, 48);
    lv_obj_set_style_bg_color(btn_back, HISTORY_LINE, 0);
    lv_obj_set_style_radius(btn_back, 8, 0);
    lv_obj_set_style_shadow_width(btn_back, 0, 0);
    lv_obj_add_event_cb(btn_back, detail_close_cb, LV_EVENT_CLICKED, screen);

    btn_label = lv_label_create(btn_back);
    lv_label_set_text(btn_label, LV_SYMBOL_LEFT);
    lv_obj_set_style_text_color(btn_label, COLOR_TEXT_WHITE, 0);
    lv_obj_center(btn_label);

    year = lv_label_create(screen);
    lv_label_set_text(year, item->year);
    lv_obj_set_style_text_color(year, accent, 0);
    lv_obj_set_style_text_font(year, &lv_font_montserrat_20, 0);
    lv_obj_align(year, LV_ALIGN_TOP_RIGHT, -22, 52);

    title = lv_label_create(screen);
    lv_label_set_text(title, item->title);
    lv_obj_set_style_text_color(title, COLOR_TEXT_WHITE, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_width(title, 360);
    lv_label_set_long_mode(title, LV_LABEL_LONG_WRAP);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 26, 72);

    summary = lv_label_create(screen);
    lv_label_set_text(summary, item->short_desc);
    lv_obj_set_style_text_color(summary, HISTORY_MUTED, 0);
    lv_obj_set_style_text_font(summary, &lv_font_montserrat_14, 0);
    lv_obj_set_width(summary, 430);
    lv_label_set_long_mode(summary, LV_LABEL_LONG_WRAP);
    lv_obj_align(summary, LV_ALIGN_TOP_LEFT, 26, 112);

    hero = lv_obj_create(screen);
    lv_obj_set_size(hero, 428, 170);
    lv_obj_align(hero, LV_ALIGN_TOP_MID, 0, 158);
    lv_obj_set_style_bg_color(hero, HISTORY_PANEL, 0);
    lv_obj_set_style_bg_opa(hero, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(hero, accent, 0);
    lv_obj_set_style_border_width(hero, 1, 0);
    lv_obj_set_style_radius(hero, 18, 0);
    lv_obj_set_style_pad_all(hero, 12, 0);
    lv_obj_set_style_shadow_width(hero, 0, 0);
    lv_obj_set_style_clip_corner(hero, true, 0);
    lv_obj_clear_flag(hero, LV_OBJ_FLAG_SCROLLABLE);

    img = lv_img_create(hero);
    lv_img_set_src(img, item->detail_img);
    lv_obj_center(img);

    desc_panel = lv_obj_create(screen);
    lv_obj_set_size(desc_panel, 428, 118);
    lv_obj_align(desc_panel, LV_ALIGN_BOTTOM_MID, 0, -22);
    lv_obj_set_style_bg_color(desc_panel, HISTORY_PANEL_ALT, 0);
    lv_obj_set_style_bg_opa(desc_panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(desc_panel, HISTORY_LINE, 0);
    lv_obj_set_style_border_width(desc_panel, 1, 0);
    lv_obj_set_style_radius(desc_panel, 16, 0);
    lv_obj_set_style_pad_all(desc_panel, 16, 0);
    lv_obj_set_style_shadow_width(desc_panel, 0, 0);
    lv_obj_set_scroll_dir(desc_panel, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(desc_panel, LV_SCROLLBAR_MODE_ACTIVE);

    desc = lv_label_create(desc_panel);
    lv_label_set_text(desc, item->long_desc);
    lv_obj_set_style_text_color(desc, lv_color_hex(0xC9D1D9), 0);
    lv_obj_set_style_text_font(desc, &lv_font_montserrat_14, 0);
    lv_obj_set_width(desc, 392);
    lv_label_set_long_mode(desc, LV_LABEL_LONG_WRAP);
    lv_obj_align(desc, LV_ALIGN_TOP_LEFT, 0, 0);

    return screen;
}

static void milestone_open_cb(lv_event_t *e)
{
    const milestone_t *item = (const milestone_t *)lv_event_get_user_data(e);
    lv_obj_t *detail_screen;

    if (!item) {
        return;
    }

    detail_screen = create_detail_screen(item);
    app_router_enter_immersive(detail_screen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 240, 0, false);
    ESP_LOGI(TAG, "Open milestone detail: %s", item->title);
}

static void create_timeline_row(lv_obj_t *parent, const milestone_t *item, bool is_last)
{
    lv_obj_t *row;
    lv_obj_t *year_chip;
    lv_obj_t *year_label;
    lv_obj_t *line;
    lv_obj_t *dot;
    lv_obj_t *card;
    lv_obj_t *title;
    lv_obj_t *desc;
    lv_obj_t *hint;
    lv_color_t accent = lv_color_hex(item->accent_hex);

    row = lv_obj_create(parent);
    lv_obj_set_size(row, lv_pct(100), 104);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_radius(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_style_shadow_width(row, 0, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    year_chip = lv_obj_create(row);
    lv_obj_set_size(year_chip, 74, 28);
    lv_obj_set_pos(year_chip, 6, 6);
    lv_obj_set_style_bg_color(year_chip, accent, 0);
    lv_obj_set_style_bg_opa(year_chip, LV_OPA_20, 0);
    lv_obj_set_style_border_color(year_chip, accent, 0);
    lv_obj_set_style_border_width(year_chip, 1, 0);
    lv_obj_set_style_radius(year_chip, 14, 0);
    lv_obj_set_style_pad_all(year_chip, 0, 0);
    lv_obj_set_style_shadow_width(year_chip, 0, 0);
    lv_obj_clear_flag(year_chip, LV_OBJ_FLAG_SCROLLABLE);

    year_label = lv_label_create(year_chip);
    lv_label_set_text(year_label, item->year);
    lv_obj_set_style_text_color(year_label, accent, 0);
    lv_obj_set_style_text_font(year_label, &lv_font_montserrat_14, 0);
    lv_obj_center(year_label);

    if (!is_last) {
        line = lv_obj_create(row);
        lv_obj_set_size(line, 2, 64);
        lv_obj_set_pos(line, 42, 34);
        lv_obj_set_style_bg_color(line, HISTORY_LINE, 0);
        lv_obj_set_style_bg_opa(line, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(line, 0, 0);
        lv_obj_set_style_radius(line, 1, 0);
        lv_obj_set_style_shadow_width(line, 0, 0);
        lv_obj_clear_flag(line, LV_OBJ_FLAG_SCROLLABLE);
    }

    dot = lv_obj_create(row);
    lv_obj_set_size(dot, 14, 14);
    lv_obj_set_pos(dot, 35, 44);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(dot, accent, 0);
    lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(dot, 0, 0);
    lv_obj_set_style_shadow_width(dot, 12, 0);
    lv_obj_set_style_shadow_color(dot, accent, 0);
    lv_obj_clear_flag(dot, LV_OBJ_FLAG_SCROLLABLE);

    card = lv_obj_create(row);
    lv_obj_set_size(card, 350, 88);
    lv_obj_set_pos(card, 92, 8);
    lv_obj_set_style_bg_color(card, HISTORY_PANEL, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(card, HISTORY_LINE, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_radius(card, 16, 0);
    lv_obj_set_style_pad_left(card, 16, 0);
    lv_obj_set_style_pad_right(card, 16, 0);
    lv_obj_set_style_pad_top(card, 12, 0);
    lv_obj_set_style_pad_bottom(card, 12, 0);
    lv_obj_set_style_shadow_width(card, 0, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(card, milestone_open_cb, LV_EVENT_CLICKED, (void *)item);

    title = lv_label_create(card);
    lv_label_set_text(title, item->title);
    lv_obj_set_style_text_color(title, COLOR_TEXT_WHITE, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 0, 0);

    desc = lv_label_create(card);
    lv_label_set_text(desc, item->short_desc);
    lv_obj_set_style_text_color(desc, HISTORY_MUTED, 0);
    lv_obj_set_style_text_font(desc, &lv_font_montserrat_14, 0);
    lv_obj_set_width(desc, 286);
    lv_label_set_long_mode(desc, LV_LABEL_LONG_WRAP);
    lv_obj_align(desc, LV_ALIGN_TOP_LEFT, 0, 28);

    hint = lv_label_create(card);
    lv_label_set_text(hint, "Tap for details");
    lv_obj_set_style_text_color(hint, accent, 0);
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_14, 0);
    lv_obj_align(hint, LV_ALIGN_TOP_RIGHT, 0, 2);
}

lv_obj_t *page_milestones_create(lv_obj_t *parent)
{
    lv_obj_t *title;
    lv_obj_t *sub;
    lv_obj_t *scroll;
    lv_obj_t *footer;
    size_t index;

    lv_obj_set_style_bg_color(parent, HISTORY_BG, 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

    title = lv_label_create(parent);
    lv_label_set_text(title, "HISTORY");
    lv_obj_set_style_text_color(title, COLOR_TEXT_WHITE, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_letter_space(title, 2, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    sub = lv_label_create(parent);
    lv_label_set_text(sub, "Scrollable NI timeline with tap-in details");
    lv_obj_set_style_text_color(sub, HISTORY_MUTED, 0);
    lv_obj_set_style_text_font(sub, &lv_font_montserrat_14, 0);
    lv_obj_align(sub, LV_ALIGN_TOP_MID, 0, 30);

    scroll = lv_obj_create(parent);
    lv_obj_set_size(scroll, 444, 360);
    lv_obj_align(scroll, LV_ALIGN_TOP_MID, 0, 52);
    lv_obj_set_style_bg_opa(scroll, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(scroll, 0, 0);
    lv_obj_set_style_radius(scroll, 0, 0);
    lv_obj_set_style_pad_left(scroll, 0, 0);
    lv_obj_set_style_pad_right(scroll, 0, 0);
    lv_obj_set_style_pad_top(scroll, 4, 0);
    lv_obj_set_style_pad_bottom(scroll, 16, 0);
    lv_obj_set_style_pad_row(scroll, 12, 0);
    lv_obj_set_style_shadow_width(scroll, 0, 0);
    lv_obj_set_flex_flow(scroll, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scroll_dir(scroll, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(scroll, LV_SCROLLBAR_MODE_ACTIVE);

    for (index = 0; index < MILESTONE_COUNT; index++) {
        create_timeline_row(scroll, &milestones[index], index == (MILESTONE_COUNT - 1));
    }

    (void)footer;  /* footer removed — content fits in scroll area */

    ESP_LOGI(TAG, "History page created with %u milestones", (unsigned)MILESTONE_COUNT);
    return parent;
}
