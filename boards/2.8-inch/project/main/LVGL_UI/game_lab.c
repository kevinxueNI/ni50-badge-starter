/**
 * game_lab.c — NI Lab System Builder
 *
 * Decision tree with 3 steps:
 *   Step 1: Platform (PXI or CompactDAQ)
 *   Step 2: Modules (depends on Step 1)
 *   Step 3: Software (LabVIEW or TestStand)
 *
 * Result: reveals the industry application.
 * UI: progress breadcrumb + selection cards + BUILD NEXT button.
 */

#include "game_lab.h"
#include "page_easter_egg.h"
#include "esp_log.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "NI_Lab";

/* ── Colors ── */
#define LAB_BG         lv_color_hex(0x0D1117)
#define LAB_GREEN      lv_color_hex(0x00A651)
#define LAB_YELLOW     lv_color_hex(0xF1C40F)
#define LAB_TEXT       lv_color_hex(0xC9D1D9)
#define LAB_MUTED      lv_color_hex(0x8B949E)
#define LAB_CARD_BG    lv_color_hex(0x1F2428)
#define LAB_CARD_SEL   lv_color_hex(0x161B22)
#define LAB_BORDER     lv_color_hex(0x30363D)
#define LAB_STOP_RED   lv_color_hex(0xDA3633)

/* ── Decision tree data ── */
typedef struct {
    const char *title;
    const char *subtitle;
} option_t;

typedef struct {
    const char *step_title;
    option_t options[2];
} step_def_t;

/* Step definitions — Step 2 varies based on Step 1 choice */
static const step_def_t step1_def = {
    "Select Platform",
    {{"PXI Express", "High Performance & Sync"},
     {"CompactDAQ", "Portable & Distributed"}}
};

/* Step 2 for PXI */
static const step_def_t step2_pxi = {
    "Select Modules (PXI)",
    {{"SMU & Digitizer", "Precision DC/AC Measurement"},
     {"VST", "RF & Wireless Transceiver"}}
};

/* Step 2 for cDAQ */
static const step_def_t step2_cdaq = {
    "Select Modules (cDAQ)",
    {{"CAN / LIN", "Vehicle Bus & Temp"},
     {"Sound & Vibration", "Acoustics & NVH"}}
};

static const step_def_t step3_def = {
    "Select Software",
    {{"LabVIEW", "Graphical Development"},
     {"TestStand", "Test Management"}}
};

/* Result lookup [platform][module][software] */
typedef struct {
    const char *title_line1;
    const char *title_line2;
    const char *subtitle;
    uint32_t glow_color;
} result_t;

static const result_t results[2][2][2] = {
    /* PXI */
    {
        /* PXI + SMU */
        {
            {"Semiconductor", "ATE System", "High-Volume Wafer & Packaged Test", 0x00A651},
            {"Semiconductor", "ATE System", "Production Test Sequencing", 0x00A651},
        },
        /* PXI + VST */
        {
            {"5G/6G Wireless", "Test & Prototyping", "Wideband RF & Baseband Validation", 0xF1C40F},
            {"5G/6G Wireless", "Production Test", "RF Parametric Test Sequencing", 0xF1C40F},
        },
    },
    /* cDAQ */
    {
        /* cDAQ + CAN */
        {
            {"Automotive EV", "HIL & Validation", "In-Vehicle Datalogging & Sim", 0x58A6FF},
            {"Automotive EV", "EOL Test", "Production Line Validation", 0x58A6FF},
        },
        /* cDAQ + S&V */
        {
            {"NVH Analysis", "Sound Quality", "Acoustics R&D & Validation", 0x00A651},
            {"NVH Analysis", "Production NVH", "End-of-Line Sound Testing", 0x00A651},
        },
    },
};

/* ── Game state ── */
static struct {
    lv_obj_t *screen;
    int step;          /* 0,1,2 = selection steps; 3 = result */
    int choices[3];    /* 0 or 1 for each step */
    int current_sel;   /* current highlighted option (0 or 1) */
    /* UI */
    lv_obj_t *dots[3];
    lv_obj_t *lines[2];
    lv_obj_t *lbl_step;
    lv_obj_t *card_a;
    lv_obj_t *card_b;
    lv_obj_t *lbl_a_title;
    lv_obj_t *lbl_a_sub;
    lv_obj_t *lbl_b_title;
    lv_obj_t *lbl_b_sub;
    lv_obj_t *check_a;
    lv_obj_t *check_b;
    lv_obj_t *btn_next;
} lab;

/* ── Forward declarations ── */
static void render_step(void);
static void show_result(void);

/* ── Get step definition based on current state ── */
static const step_def_t *get_step_def(int step)
{
    switch (step) {
    case 0: return &step1_def;
    case 1: return (lab.choices[0] == 0) ? &step2_pxi : &step2_cdaq;
    case 2: return &step3_def;
    default: return &step1_def;
    }
}

/* ── Callbacks ── */
static void card_a_cb(lv_event_t *e)
{
    (void)e;
    lab.current_sel = 0;
    /* Update card visuals */
    lv_obj_set_style_border_color(lab.card_a, LAB_GREEN, 0);
    lv_obj_set_style_border_width(lab.card_a, 2, 0);
    lv_obj_set_style_border_color(lab.card_b, LAB_BORDER, 0);
    lv_obj_set_style_border_width(lab.card_b, 1, 0);
    lv_obj_clear_flag(lab.check_a, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(lab.check_b, LV_OBJ_FLAG_HIDDEN);
}

static void card_b_cb(lv_event_t *e)
{
    (void)e;
    lab.current_sel = 1;
    lv_obj_set_style_border_color(lab.card_b, LAB_GREEN, 0);
    lv_obj_set_style_border_width(lab.card_b, 2, 0);
    lv_obj_set_style_border_color(lab.card_a, LAB_BORDER, 0);
    lv_obj_set_style_border_width(lab.card_a, 1, 0);
    lv_obj_add_flag(lab.check_a, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(lab.check_b, LV_OBJ_FLAG_HIDDEN);
}

static void next_btn_cb(lv_event_t *e)
{
    (void)e;
    lab.choices[lab.step] = lab.current_sel;
    lab.step++;
    lab.current_sel = 0;
    if (lab.step >= 3) {
        show_result();
    } else {
        render_step();
    }
}

static void stop_cb(lv_event_t *e)
{
    (void)e;
    easter_egg_exit();
}

static void restart_cb(lv_event_t *e)
{
    (void)e;
    lab.screen = NULL;
    game_lab_show();
}

/* ── Update breadcrumb dots ── */
static void update_breadcrumb(void)
{
    for (int i = 0; i < 3; i++) {
        if (i < lab.step) {
            lv_obj_set_style_bg_color(lab.dots[i], LAB_GREEN, 0);
            lv_obj_set_style_bg_opa(lab.dots[i], LV_OPA_COVER, 0);
        } else if (i == lab.step) {
            lv_obj_set_style_bg_color(lab.dots[i], LAB_GREEN, 0);
            lv_obj_set_style_bg_opa(lab.dots[i], LV_OPA_30, 0);
            lv_obj_set_style_border_color(lab.dots[i], LAB_GREEN, 0);
            lv_obj_set_style_border_width(lab.dots[i], 1, 0);
        } else {
            lv_obj_set_style_bg_color(lab.dots[i], LAB_CARD_BG, 0);
            lv_obj_set_style_bg_opa(lab.dots[i], LV_OPA_COVER, 0);
            lv_obj_set_style_border_color(lab.dots[i], LAB_BORDER, 0);
            lv_obj_set_style_border_width(lab.dots[i], 1, 0);
        }
    }
    for (int i = 0; i < 2; i++) {
        lv_obj_set_style_bg_color(lab.lines[i], (i < lab.step) ? LAB_GREEN : LAB_BORDER, 0);
    }
}

/* ── Render current step ── */
static void render_step(void)
{
    const step_def_t *def = get_step_def(lab.step);

    update_breadcrumb();
    lv_label_set_text(lab.lbl_step, def->step_title);

    /* Update card A */
    lv_label_set_text(lab.lbl_a_title, def->options[0].title);
    lv_label_set_text(lab.lbl_a_sub, def->options[0].subtitle);

    /* Update card B */
    lv_label_set_text(lab.lbl_b_title, def->options[1].title);
    lv_label_set_text(lab.lbl_b_sub, def->options[1].subtitle);

    /* Default select A */
    lab.current_sel = 0;
    lv_obj_set_style_border_color(lab.card_a, LAB_GREEN, 0);
    lv_obj_set_style_border_width(lab.card_a, 2, 0);
    lv_obj_set_style_border_color(lab.card_b, LAB_BORDER, 0);
    lv_obj_set_style_border_width(lab.card_b, 1, 0);
    lv_obj_clear_flag(lab.check_a, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(lab.check_b, LV_OBJ_FLAG_HIDDEN);

    /* Show cards and button */
    lv_obj_clear_flag(lab.card_a, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(lab.card_b, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(lab.btn_next, LV_OBJ_FLAG_HIDDEN);
}

/* ── Show Result ── */
static void show_result(void)
{
    const result_t *res = &results[lab.choices[0]][lab.choices[1]][lab.choices[2]];

    /* Hide selection UI */
    lv_obj_add_flag(lab.card_a, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(lab.card_b, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(lab.btn_next, LV_OBJ_FLAG_HIDDEN);

    update_breadcrumb();
    lv_label_set_text(lab.lbl_step, "SYSTEM BUILT!");
    lv_obj_set_style_text_color(lab.lbl_step, LAB_GREEN, 0);

    /* Result display */
    lv_obj_t *line1 = lv_label_create(lab.screen);
    lv_label_set_text(line1, res->title_line1);
    lv_obj_set_style_text_color(line1, lv_color_white(), 0);
    lv_obj_set_style_text_font(line1, &lv_font_montserrat_18, 0);
    lv_obj_align(line1, LV_ALIGN_CENTER, 0, -30);

    lv_obj_t *line2 = lv_label_create(lab.screen);
    lv_label_set_text(line2, res->title_line2);
    lv_obj_set_style_text_color(line2, lv_color_white(), 0);
    lv_obj_set_style_text_font(line2, &lv_font_montserrat_18, 0);
    lv_obj_align(line2, LV_ALIGN_CENTER, 0, -6);

    lv_obj_t *sub = lv_label_create(lab.screen);
    lv_label_set_text(sub, res->subtitle);
    lv_obj_set_style_text_color(sub, LAB_MUTED, 0);
    lv_obj_set_style_text_font(sub, &lv_font_montserrat_10, 0);
    lv_obj_align(sub, LV_ALIGN_CENTER, 0, 18);

    /* Restart button */
    lv_obj_t *btn_restart = lv_btn_create(lab.screen);
    lv_obj_set_size(btn_restart, 140, 34);
    lv_obj_align(btn_restart, LV_ALIGN_CENTER, 0, 60);
    lv_obj_set_style_bg_color(btn_restart, LAB_CARD_BG, 0);
    lv_obj_set_style_border_color(btn_restart, LAB_BORDER, 0);
    lv_obj_set_style_border_width(btn_restart, 1, 0);
    lv_obj_set_style_radius(btn_restart, 14, 0);
    lv_obj_add_event_cb(btn_restart, restart_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *rl = lv_label_create(btn_restart);
    lv_label_set_text(rl, "RESTART BUILDER");
    lv_obj_set_style_text_font(rl, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(rl, LAB_TEXT, 0);
    lv_obj_center(rl);

    ESP_LOGI(TAG, "Result: %s %s", res->title_line1, res->title_line2);
}

/* ── Show NI Lab ── */
void game_lab_show(void)
{
    ESP_LOGI(TAG, "Starting NI Lab System Builder...");

    if (lab.screen) {
        lv_obj_del(lab.screen);
    }

    memset(&lab, 0, sizeof(lab));
    lab.screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(lab.screen, LAB_BG, 0);
    lv_obj_set_style_bg_opa(lab.screen, LV_OPA_COVER, 0);

    /* ── Top: SYSTEM BUILDER label ── */
    lv_obj_t *hdr = lv_label_create(lab.screen);
    lv_label_set_text(hdr, "SYSTEM BUILDER");
    lv_obj_set_style_text_color(hdr, LAB_MUTED, 0);
    lv_obj_set_style_text_font(hdr, &lv_font_montserrat_10, 0);
    lv_obj_align(hdr, LV_ALIGN_TOP_MID, 0, 8);

    /* ── Breadcrumb (3 dots + 2 lines) ── */
    int dot_r = 5;
    int dot_y = 30;
    int dot_spacing = 30;
    int start_x = 120 - dot_spacing;  /* center the 3 dots */

    for (int i = 0; i < 3; i++) {
        lab.dots[i] = lv_obj_create(lab.screen);
        lv_obj_set_size(lab.dots[i], dot_r * 2, dot_r * 2);
        lv_obj_set_pos(lab.dots[i], start_x + i * dot_spacing - dot_r, dot_y - dot_r);
        lv_obj_set_style_radius(lab.dots[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_pad_all(lab.dots[i], 0, 0);
        lv_obj_clear_flag(lab.dots[i], LV_OBJ_FLAG_SCROLLABLE);
    }
    for (int i = 0; i < 2; i++) {
        lab.lines[i] = lv_obj_create(lab.screen);
        lv_obj_set_size(lab.lines[i], dot_spacing - dot_r * 2 - 2, 2);
        lv_obj_set_pos(lab.lines[i], start_x + dot_r + i * dot_spacing + 1, dot_y - 1);
        lv_obj_set_style_bg_color(lab.lines[i], LAB_BORDER, 0);
        lv_obj_set_style_bg_opa(lab.lines[i], LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(lab.lines[i], 0, 0);
        lv_obj_clear_flag(lab.lines[i], LV_OBJ_FLAG_SCROLLABLE);
    }

    /* ── Step title ── */
    lab.lbl_step = lv_label_create(lab.screen);
    lv_obj_set_style_text_color(lab.lbl_step, lv_color_white(), 0);
    lv_obj_set_style_text_font(lab.lbl_step, &lv_font_montserrat_14, 0);
    lv_obj_align(lab.lbl_step, LV_ALIGN_TOP_MID, 0, 48);

    /* ── Card A ── */
    lab.card_a = lv_obj_create(lab.screen);
    lv_obj_set_size(lab.card_a, 200, 72);
    lv_obj_align(lab.card_a, LV_ALIGN_TOP_MID, 0, 76);
    lv_obj_set_style_bg_color(lab.card_a, LAB_CARD_BG, 0);
    lv_obj_set_style_bg_opa(lab.card_a, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(lab.card_a, 8, 0);
    lv_obj_set_style_pad_all(lab.card_a, 10, 0);
    lv_obj_clear_flag(lab.card_a, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(lab.card_a, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(lab.card_a, card_a_cb, LV_EVENT_CLICKED, NULL);

    lab.lbl_a_title = lv_label_create(lab.card_a);
    lv_obj_set_style_text_color(lab.lbl_a_title, lv_color_white(), 0);
    lv_obj_set_style_text_font(lab.lbl_a_title, &lv_font_montserrat_14, 0);
    lv_obj_align(lab.lbl_a_title, LV_ALIGN_TOP_LEFT, 0, 4);

    lab.lbl_a_sub = lv_label_create(lab.card_a);
    lv_obj_set_style_text_color(lab.lbl_a_sub, LAB_MUTED, 0);
    lv_obj_set_style_text_font(lab.lbl_a_sub, &lv_font_montserrat_10, 0);
    lv_obj_align(lab.lbl_a_sub, LV_ALIGN_TOP_LEFT, 0, 24);

    /* Check mark for card A */
    lab.check_a = lv_label_create(lab.card_a);
    lv_label_set_text(lab.check_a, LV_SYMBOL_OK);
    lv_obj_set_style_text_color(lab.check_a, LAB_GREEN, 0);
    lv_obj_set_style_text_font(lab.check_a, &lv_font_montserrat_12, 0);
    lv_obj_align(lab.check_a, LV_ALIGN_TOP_RIGHT, 0, 4);

    /* ── Card B ── */
    lab.card_b = lv_obj_create(lab.screen);
    lv_obj_set_size(lab.card_b, 200, 72);
    lv_obj_align(lab.card_b, LV_ALIGN_TOP_MID, 0, 158);
    lv_obj_set_style_bg_color(lab.card_b, LAB_CARD_BG, 0);
    lv_obj_set_style_bg_opa(lab.card_b, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(lab.card_b, 8, 0);
    lv_obj_set_style_pad_all(lab.card_b, 10, 0);
    lv_obj_clear_flag(lab.card_b, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(lab.card_b, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(lab.card_b, card_b_cb, LV_EVENT_CLICKED, NULL);

    lab.lbl_b_title = lv_label_create(lab.card_b);
    lv_obj_set_style_text_color(lab.lbl_b_title, lv_color_white(), 0);
    lv_obj_set_style_text_font(lab.lbl_b_title, &lv_font_montserrat_14, 0);
    lv_obj_align(lab.lbl_b_title, LV_ALIGN_TOP_LEFT, 0, 4);

    lab.lbl_b_sub = lv_label_create(lab.card_b);
    lv_obj_set_style_text_color(lab.lbl_b_sub, LAB_MUTED, 0);
    lv_obj_set_style_text_font(lab.lbl_b_sub, &lv_font_montserrat_10, 0);
    lv_obj_align(lab.lbl_b_sub, LV_ALIGN_TOP_LEFT, 0, 24);

    /* Check mark for card B (hidden initially) */
    lab.check_b = lv_label_create(lab.card_b);
    lv_label_set_text(lab.check_b, LV_SYMBOL_OK);
    lv_obj_set_style_text_color(lab.check_b, LAB_GREEN, 0);
    lv_obj_set_style_text_font(lab.check_b, &lv_font_montserrat_12, 0);
    lv_obj_align(lab.check_b, LV_ALIGN_TOP_RIGHT, 0, 4);
    lv_obj_add_flag(lab.check_b, LV_OBJ_FLAG_HIDDEN);

    /* ── BUILD NEXT button ── */
    lab.btn_next = lv_btn_create(lab.screen);
    lv_obj_set_size(lab.btn_next, 190, 36);
    lv_obj_align(lab.btn_next, LV_ALIGN_TOP_MID, 0, 248);
    lv_obj_set_style_bg_color(lab.btn_next, LAB_GREEN, 0);
    lv_obj_set_style_radius(lab.btn_next, 6, 0);
    lv_obj_add_event_cb(lab.btn_next, next_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *nl = lv_label_create(lab.btn_next);
    lv_label_set_text(nl, "BUILD NEXT");
    lv_obj_set_style_text_font(nl, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(nl, lv_color_white(), 0);
    lv_obj_center(nl);

    /* ── STOP button ── */
    lv_obj_t *btn_stop = lv_btn_create(lab.screen);
    lv_obj_set_size(btn_stop, 50, 24);
    lv_obj_align(btn_stop, LV_ALIGN_BOTTOM_RIGHT, -8, -8);
    lv_obj_set_style_bg_color(btn_stop, LAB_STOP_RED, 0);
    lv_obj_set_style_radius(btn_stop, 4, 0);
    lv_obj_add_event_cb(btn_stop, stop_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *sl = lv_label_create(btn_stop);
    lv_label_set_text(sl, "STOP");
    lv_obj_set_style_text_font(sl, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(sl, lv_color_white(), 0);
    lv_obj_center(sl);

    /* Render first step */
    render_step();

    /* Load screen */
    lv_scr_load_anim(lab.screen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, true);
}
