/**
 * page_easter_egg.c — Hidden Developer Mode / Game Menu
 *
 * Entry: long-press 2s on "Kevin Xue" (page_card.c)
 * Shows game menu with LabVIEW grid background.
 * STOP button returns to main screen.
 * Adapted from 2.8" version → 480×480.
 */
#include "ui.h"
#include "esp_log.h"
#include <stdio.h>

static const char *TAG = "EasterEgg";

/* Colors */
#define EE_BG          lv_color_hex(0x1C2128)
#define EE_GRID        lv_color_hex(0x30363D)
#define EE_YELLOW      lv_color_hex(0xD29922)
#define EE_GREEN       lv_color_hex(0x00A651)
#define EE_RED         lv_color_hex(0xDA3633)
#define EE_RED_BORDER  lv_color_hex(0xFF7B72)
#define EE_TEXT        lv_color_hex(0xC9D1D9)
#define EE_MUTED       lv_color_hex(0x8B949E)
#define EE_CARD_BG     lv_color_hex(0x161B22)
#define EE_BORDER      lv_color_hex(0x30363D)
#define EE_BLUE        lv_color_hex(0x58A6FF)

static lv_obj_t *ee_screen = NULL;

static struct {
    lv_obj_t *screen;
    lv_obj_t *roller_platform;
    lv_obj_t *roller_hardware;
    lv_obj_t *roller_software;
    lv_obj_t *lbl_result_title;
    lv_obj_t *lbl_result_summary;
    char result_title[96];
    char result_summary[256];
} sb;

static const char *platform_options = "PXI\nCompactDAQ\nCompactRIO";
static const char *hardware_options = "DMM\nOscilloscope\nFPGA/RIO I/O\nRF/VST";
static const char *software_options = "LabVIEW\nTestStand\nVeriStand\nSystemLink";

static const char *const platform_labels[] = {
    "PXI",
    "CompactDAQ",
    "CompactRIO",
};
static const char *const hardware_labels[] = {
    "DMM",
    "Oscilloscope",
    "FPGA/RIO I/O",
    "RF/VST",
};
static const char *const software_labels[] = {
    "LabVIEW",
    "TestStand",
    "VeriStand",
    "SystemLink",
};

/* 3 platforms x 4 hardware x 4 software = 48 solution names */
static const char *const solution_names[3][4][4] = {
    /* PXI */ {
        /* DMM */        {"Modular Parametric Measurement Development Platform",
                          "Parametric ATE Production Test System",
                          "Real-Time Electrical Measurement Validation System",
                          "Managed Parametric Test Asset and Data Platform"},
        /* Oscilloscope */ {"High-Speed Waveform Validation Development Platform",
                          "ATE Automated Validation Test System",
                          "Real-Time Signal Validation and HIL Measurement System",
                          "Distributed Waveform Test Data Management Platform"},
        /* FPGA/RIO */   {"FPGA-Based Custom Measurement and Control Platform",
                          "FPGA-Assisted Automated Test Sequence System",
                          "Deterministic Real-Time HIL Test System",
                          "Managed Real-Time Test Infrastructure Platform"},
        /* RF/VST */     {"RF Measurement Automation Development Platform",
                          "RF Production Test ATE System",
                          "RF Scenario Validation and Real-Time Test Platform",
                          "RF Lab Asset and Test Data Management Platform"},
    },
    /* CompactDAQ */ {
        /* DMM */        {"Bench Electrical Measurement and DAQ Dev Platform",
                          "Functional Electrical Test Sequence Platform",
                          "Electrical Monitoring Validation Bench",
                          "Distributed Electrical Measurement Data Platform"},
        /* Oscilloscope */ {"Sensor and Waveform DAQ Development Platform",
                          "Automated Bench Validation Test System",
                          "Signal Monitoring and Validation Bench",
                          "Distributed Sensor and Waveform Data Platform"},
        /* FPGA/RIO */   {"Custom DAQ and Control Prototype Platform",
                          "DAQ-Based Functional Test Automation Platform",
                          "Lightweight Real-Time Validation Candidate",
                          "Managed DAQ Prototype and Edge Data Platform"},
        /* RF/VST */     {"Mixed DAQ and RF Measurement Prototype Platform",
                          "Bench-Level Mixed-Signal Automated Test Candidate",
                          "Mixed-Signal Validation Candidate",
                          "Mixed DAQ/RF Lab Data Management Platform"},
    },
    /* CompactRIO */ {
        /* DMM */        {"Embedded Electrical Measurement and Control Platform",
                          "Embedded Functional Test Sequence System",
                          "Electrical HIL Monitoring and Control System",
                          "Managed Embedded Measurement Node Platform"},
        /* Oscilloscope */ {"Embedded Waveform Acquisition and Control Platform",
                          "Embedded Validation Test Sequence System",
                          "Real-Time HIL Signal Validation System",
                          "Distributed Embedded Waveform Data Platform"},
        /* FPGA/RIO */   {"Embedded Control and Acquisition Dev Platform",
                          "Real-Time Device Validation Sequence System",
                          "HIL System for Embedded Controller Validation",
                          "Managed HIL and Edge Test Infrastructure Platform"},
        /* RF/VST */     {"Edge-Controlled RF Measurement Integration Platform",
                          "Hybrid RF Test Sequence System",
                          "RF HIL and Real-Time Scenario Simulation Platform",
                          "Mixed RF and Edge Test Asset Management Platform"},
    },
};

/* Main tileview reference for return */
extern lv_obj_t *g_tileview;

/* Forward: game_trivia_show */
extern void game_trivia_show(void);

static void solution_builder_show(void);

static void build_grid_background(lv_obj_t *parent)
{
    for (int x = 0; x <= 480; x += 24) {
        lv_obj_t *vl = lv_obj_create(parent);
        lv_obj_set_size(vl, 1, 480);
        lv_obj_set_pos(vl, x, 0);
        lv_obj_set_style_bg_color(vl, EE_GRID, 0);
        lv_obj_set_style_bg_opa(vl, LV_OPA_30, 0);
        lv_obj_set_style_border_width(vl, 0, 0);
        lv_obj_set_style_shadow_width(vl, 0, 0);
        lv_obj_clear_flag(vl, LV_OBJ_FLAG_SCROLLABLE);
    }
    for (int y = 0; y <= 480; y += 24) {
        lv_obj_t *hl = lv_obj_create(parent);
        lv_obj_set_size(hl, 480, 1);
        lv_obj_set_pos(hl, 0, y);
        lv_obj_set_style_bg_color(hl, EE_GRID, 0);
        lv_obj_set_style_bg_opa(hl, LV_OPA_30, 0);
        lv_obj_set_style_border_width(hl, 0, 0);
        lv_obj_set_style_shadow_width(hl, 0, 0);
        lv_obj_clear_flag(hl, LV_OBJ_FLAG_SCROLLABLE);
    }
}

static lv_obj_t *build_stop_button(lv_obj_t *parent, lv_event_cb_t cb)
{
    lv_obj_t *btn_stop = lv_btn_create(parent);
    lv_obj_set_size(btn_stop, 80, 36);
    lv_obj_set_pos(btn_stop, 380, 52);
    lv_obj_set_style_bg_color(btn_stop, EE_RED, 0);
    lv_obj_set_style_border_color(btn_stop, EE_RED_BORDER, 0);
    lv_obj_set_style_border_width(btn_stop, 2, 0);
    lv_obj_set_style_radius(btn_stop, 6, 0);
    lv_obj_set_style_shadow_width(btn_stop, 0, 0);
    lv_obj_add_event_cb(btn_stop, cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *stop_lbl = lv_label_create(btn_stop);
    lv_label_set_text(stop_lbl, "STOP");
    lv_obj_set_style_text_font(stop_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(stop_lbl, lv_color_white(), 0);
    lv_obj_center(stop_lbl);
    return btn_stop;
}

static void destroy_hub_if_present(void)
{
    if (ee_screen) {
        lv_obj_del_async(ee_screen);
        ee_screen = NULL;
    }
}

static lv_obj_t *create_hub_card(lv_obj_t *parent, int y, lv_color_t border_color, const char *icon,
                                 const char *title_text, const char *desc_text, lv_event_cb_t cb)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, 360, 104);
    lv_obj_align(card, LV_ALIGN_TOP_MID, 0, y);
    lv_obj_set_style_bg_color(card, EE_CARD_BG, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(card, border_color, 0);
    lv_obj_set_style_border_width(card, 2, 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_pad_all(card, 16, 0);
    lv_obj_set_style_shadow_width(card, 0, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    if (cb) {
        lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(card, cb, LV_EVENT_CLICKED, NULL);
    }

    lv_obj_t *icon_lbl = lv_label_create(card);
    lv_label_set_text(icon_lbl, icon);
    lv_obj_set_style_text_font(icon_lbl, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(icon_lbl, border_color, 0);
    lv_obj_align(icon_lbl, LV_ALIGN_LEFT_MID, 0, 0);

    lv_obj_t *title_lbl = lv_label_create(card);
    lv_label_set_text(title_lbl, title_text);
    lv_obj_set_style_text_color(title_lbl, lv_color_white(), 0);
    lv_obj_set_style_text_font(title_lbl, &lv_font_montserrat_20, 0);
    lv_obj_align(title_lbl, LV_ALIGN_TOP_LEFT, 46, 4);

    lv_obj_t *desc_lbl = lv_label_create(card);
    lv_label_set_text(desc_lbl, desc_text);
    lv_obj_set_style_text_color(desc_lbl, EE_MUTED, 0);
    lv_obj_set_style_text_font(desc_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_width(desc_lbl, 280);
    lv_label_set_long_mode(desc_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_align(desc_lbl, LV_ALIGN_TOP_LEFT, 46, 34);

    return card;
}

/* ── Exit callback ── */
static void stop_btn_cb(lv_event_t *e)
{
    (void)e;
    easter_egg_exit();
}

/* ── Game launch callbacks ── */
static void trivia_btn_cb(lv_event_t *e)
{
    (void)e;
    destroy_hub_if_present();
    game_trivia_show();
    lv_refr_now(NULL);
}

static void solution_builder_close_cb(lv_event_t *e)
{
    (void)e;
    lv_obj_t *screen = sb.screen;
    sb.screen = NULL;
    app_router_go_tab(PAGE_CALENDAR, LV_ANIM_OFF);
    if (screen) {
        lv_obj_del_async(screen);
    }
}

static void solution_build_cb(lv_event_t *e)
{
    (void)e;

    int platform_idx = (int)lv_roller_get_selected(sb.roller_platform);
    int hardware_idx = (int)lv_roller_get_selected(sb.roller_hardware);
    int software_idx = (int)lv_roller_get_selected(sb.roller_software);

    if (platform_idx >= 3) platform_idx = 2;
    if (hardware_idx >= 4) hardware_idx = 3;
    if (software_idx >= 4) software_idx = 3;

    const char *name = solution_names[platform_idx][hardware_idx][software_idx];

    snprintf(sb.result_title, sizeof(sb.result_title), "%s", name);
    snprintf(sb.result_summary, sizeof(sb.result_summary), "%s + %s + %s",
             platform_labels[platform_idx], hardware_labels[hardware_idx], software_labels[software_idx]);

    lv_label_set_text(sb.lbl_result_title, sb.result_title);
    lv_label_set_text(sb.lbl_result_summary, sb.result_summary);
    ESP_LOGI(TAG, "Solution built: %s", sb.result_title);
}

static void solution_builder_btn_cb(lv_event_t *e)
{
    (void)e;
    destroy_hub_if_present();
    solution_builder_show();
    lv_refr_now(NULL);
}

static lv_obj_t *create_labeled_roller(lv_obj_t *parent, int x, const char *caption, const char *options)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, caption);
    lv_obj_set_style_text_color(label, EE_MUTED, 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(label, x, 124);

    lv_obj_t *roller = lv_roller_create(parent);
    lv_obj_set_size(roller, 138, 110);
    lv_obj_set_pos(roller, x, 150);
    lv_roller_set_options(roller, options, LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(roller, 3);
    lv_obj_set_style_bg_color(roller, EE_CARD_BG, 0);
    lv_obj_set_style_bg_opa(roller, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(roller, EE_BORDER, 0);
    lv_obj_set_style_border_width(roller, 1, 0);
    lv_obj_set_style_radius(roller, 10, 0);
    lv_obj_set_style_text_color(roller, EE_TEXT, 0);
    lv_obj_set_style_text_font(roller, &lv_font_montserrat_16, 0);
    lv_obj_set_style_bg_color(roller, EE_GRID, LV_PART_SELECTED);
    lv_obj_set_style_text_color(roller, lv_color_white(), LV_PART_SELECTED);
    return roller;
}

static void solution_builder_show(void)
{
    sb.screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(sb.screen, EE_BG, 0);
    lv_obj_set_style_bg_opa(sb.screen, LV_OPA_COVER, 0);
    lv_obj_clear_flag(sb.screen, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *banner = lv_obj_create(sb.screen);
    lv_obj_set_size(banner, 480, 40);
    lv_obj_set_pos(banner, 0, 0);
    lv_obj_set_style_bg_color(banner, EE_BLUE, 0);
    lv_obj_set_style_bg_opa(banner, LV_OPA_80, 0);
    lv_obj_set_style_radius(banner, 0, 0);
    lv_obj_set_style_border_width(banner, 0, 0);
    lv_obj_set_style_shadow_width(banner, 0, 0);
    lv_obj_clear_flag(banner, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *banner_txt = lv_label_create(banner);
    lv_label_set_text(banner_txt, "> SOLUTION BUILDER_");
    lv_obj_set_style_text_color(banner_txt, lv_color_hex(0x111111), 0);
    lv_obj_set_style_text_font(banner_txt, &lv_font_montserrat_18, 0);
    lv_obj_center(banner_txt);

    build_stop_button(sb.screen, solution_builder_close_cb);

    lv_obj_t *title = lv_label_create(sb.screen);
    lv_label_set_text(title, "Compose A NI Stack");
    lv_obj_set_style_text_color(title, EE_TEXT, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 72);

    lv_obj_t *sub = lv_label_create(sb.screen);
    lv_label_set_text(sub, "Pick platform, hardware, and software.");
    lv_obj_set_style_text_color(sub, EE_MUTED, 0);
    lv_obj_set_style_text_font(sub, &lv_font_montserrat_14, 0);
    lv_obj_align(sub, LV_ALIGN_TOP_MID, 0, 102);

    sb.roller_platform = create_labeled_roller(sb.screen, 12, "Platform", platform_options);
    sb.roller_hardware = create_labeled_roller(sb.screen, 171, "Hardware", hardware_options);
    sb.roller_software = create_labeled_roller(sb.screen, 330, "Software", software_options);

    lv_obj_t *btn_build = lv_btn_create(sb.screen);
    lv_obj_set_size(btn_build, 156, 42);
    lv_obj_align(btn_build, LV_ALIGN_TOP_MID, 0, 278);
    lv_obj_set_style_bg_color(btn_build, EE_GREEN, 0);
    lv_obj_set_style_radius(btn_build, 10, 0);
    lv_obj_set_style_shadow_width(btn_build, 0, 0);
    lv_obj_add_event_cb(btn_build, solution_build_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *build_lbl = lv_label_create(btn_build);
    lv_label_set_text(build_lbl, "BUILD");
    lv_obj_set_style_text_color(build_lbl, lv_color_white(), 0);
    lv_obj_set_style_text_font(build_lbl, &lv_font_montserrat_18, 0);
    lv_obj_center(build_lbl);

    lv_obj_t *result_card = lv_obj_create(sb.screen);
    lv_obj_set_size(result_card, 420, 120);
    lv_obj_align(result_card, LV_ALIGN_BOTTOM_MID, 0, -26);
    lv_obj_set_style_bg_color(result_card, EE_CARD_BG, 0);
    lv_obj_set_style_bg_opa(result_card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(result_card, EE_BLUE, 0);
    lv_obj_set_style_border_width(result_card, 1, 0);
    lv_obj_set_style_radius(result_card, 14, 0);
    lv_obj_set_style_pad_all(result_card, 14, 0);
    lv_obj_set_style_shadow_width(result_card, 0, 0);
    lv_obj_clear_flag(result_card, LV_OBJ_FLAG_SCROLLABLE);

    sb.lbl_result_title = lv_label_create(result_card);
    lv_label_set_text(sb.lbl_result_title, "Build a first-round recommendation");
    lv_obj_set_style_text_color(sb.lbl_result_title, lv_color_white(), 0);
    lv_obj_set_style_text_font(sb.lbl_result_title, &lv_font_montserrat_18, 0);
    lv_obj_align(sb.lbl_result_title, LV_ALIGN_TOP_LEFT, 0, 0);

    sb.lbl_result_summary = lv_label_create(result_card);
    lv_label_set_text(sb.lbl_result_summary, "Use the three rollers above to assemble a platform story. Curated mappings exist for common NI combinations, while unknown mixes fall back to a generic exploratory recommendation.");
    lv_obj_set_style_text_color(sb.lbl_result_summary, EE_MUTED, 0);
    lv_obj_set_style_text_font(sb.lbl_result_summary, &lv_font_montserrat_14, 0);
    lv_obj_set_width(sb.lbl_result_summary, 390);
    lv_label_set_long_mode(sb.lbl_result_summary, LV_LABEL_LONG_WRAP);
    lv_obj_align(sb.lbl_result_summary, LV_ALIGN_TOP_LEFT, 0, 34);

    app_router_enter_immersive(sb.screen, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
}

/* ── Build Menu ── */
void easter_egg_show(void)
{
    if (ee_screen) return;

    ESP_LOGI(TAG, "Entering Developer Mode...");

    ee_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(ee_screen, EE_BG, 0);
    lv_obj_set_style_bg_opa(ee_screen, LV_OPA_COVER, 0);
    lv_obj_clear_flag(ee_screen, LV_OBJ_FLAG_SCROLLABLE);

    /* Grid background removed — 42-object grid caused WDT on entry */

    /* Top banner: DEVELOPER MODE */
    lv_obj_t *banner = lv_obj_create(ee_screen);
    lv_obj_set_size(banner, 480, 40);
    lv_obj_set_pos(banner, 0, 0);
    lv_obj_set_style_bg_color(banner, EE_YELLOW, 0);
    lv_obj_set_style_bg_opa(banner, LV_OPA_90, 0);
    lv_obj_set_style_radius(banner, 0, 0);
    lv_obj_set_style_border_width(banner, 0, 0);
    lv_obj_set_style_shadow_width(banner, 0, 0);
    lv_obj_clear_flag(banner, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *banner_txt = lv_label_create(banner);
    lv_label_set_text(banner_txt, "> DEVELOPER MODE_");
    lv_obj_set_style_text_color(banner_txt, lv_color_hex(0x111111), 0);
    lv_obj_set_style_text_font(banner_txt, &lv_font_montserrat_18, 0);
    lv_obj_center(banner_txt);

    build_stop_button(ee_screen, stop_btn_cb);

    /* Title */
    lv_obj_t *title = lv_label_create(ee_screen);
    lv_label_set_text(title, "Easter Egg Hub");
    lv_obj_set_style_text_color(title, EE_TEXT, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 94);

    lv_obj_t *sub = lv_label_create(ee_screen);
    lv_label_set_text(sub, "Trivia, stack-building, and future geek tools");
    lv_obj_set_style_text_color(sub, EE_MUTED, 0);
    lv_obj_set_style_text_font(sub, &lv_font_montserrat_14, 0);
    lv_obj_align(sub, LV_ALIGN_TOP_MID, 0, 126);

    create_hub_card(ee_screen, 164, EE_GREEN, "?", "NI Trivia",
                    "10 randomized questions with immediate feedback.", trivia_btn_cb);
    create_hub_card(ee_screen, 284, EE_BLUE, LV_SYMBOL_SETTINGS, "Solution Builder",
                    "Compose a NI stack from platform, hardware, and software.", solution_builder_btn_cb);

    /* Footer hint */
    lv_obj_t *hint = lv_label_create(ee_screen);
    lv_label_set_text(hint, "// NI 50th Anniversary Easter Egg Hub");
    lv_obj_set_style_text_color(hint, EE_MUTED, 0);
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_14, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -20);

    /* Load screen — immediate since transition screen already cleared old scene */
    app_router_enter_immersive(ee_screen, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
}

void easter_egg_exit(void)
{
    ESP_LOGI(TAG, "Exiting Developer Mode");
    ee_screen = NULL;
    sb.screen = NULL;
    app_router_go_home();
}

/* ── Tab-inline version (replaces Calendar page for testing) ── */
lv_obj_t *page_easter_egg_create(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, EE_BG, 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

    /* Top banner */
    lv_obj_t *banner = lv_obj_create(parent);
    lv_obj_set_size(banner, 480, 40);
    lv_obj_set_pos(banner, 0, 0);
    lv_obj_set_style_bg_color(banner, EE_YELLOW, 0);
    lv_obj_set_style_bg_opa(banner, LV_OPA_90, 0);
    lv_obj_set_style_radius(banner, 0, 0);
    lv_obj_set_style_border_width(banner, 0, 0);
    lv_obj_set_style_shadow_width(banner, 0, 0);
    lv_obj_clear_flag(banner, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *banner_txt = lv_label_create(banner);
    lv_label_set_text(banner_txt, "> DEVELOPER MODE_");
    lv_obj_set_style_text_color(banner_txt, lv_color_hex(0x111111), 0);
    lv_obj_set_style_text_font(banner_txt, &lv_font_montserrat_18, 0);
    lv_obj_center(banner_txt);

    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Easter Egg Hub");
    lv_obj_set_style_text_color(title, EE_TEXT, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 64);

    lv_obj_t *sub = lv_label_create(parent);
    lv_label_set_text(sub, "Trivia, stack-building, and future geek tools");
    lv_obj_set_style_text_color(sub, EE_MUTED, 0);
    lv_obj_set_style_text_font(sub, &lv_font_montserrat_14, 0);
    lv_obj_align(sub, LV_ALIGN_TOP_MID, 0, 96);

    create_hub_card(parent, 134, EE_GREEN, "?", "NI Trivia",
                    "10 randomized questions with immediate feedback.", trivia_btn_cb);
    create_hub_card(parent, 254, EE_BLUE, LV_SYMBOL_SETTINGS, "Solution Builder",
                    "Compose a NI stack from platform, hardware, and software.", solution_builder_btn_cb);

    /* Footer hint */
    lv_obj_t *hint = lv_label_create(parent);
    lv_label_set_text(hint, "// NI 50th Anniversary Easter Egg Hub");
    lv_obj_set_style_text_color(hint, EE_MUTED, 0);
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_14, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -56);

    return parent;
}
