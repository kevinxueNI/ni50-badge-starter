/**
 * page_easter_egg.c — Hidden game space with Developer Mode menu
 *
 * Entry: long-press 2s on name label (handled in Badge_UI.c)
 * Shows a game menu with LabVIEW Block Diagram grid background.
 * Two game options: NI Trivia and NI Lab (System Builder).
 * LabVIEW red STOP button exits back to main screen.
 */

#include "page_easter_egg.h"
#include "game_trivia.h"
#include "game_lab.h"
#include "esp_log.h"

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

static lv_obj_t *ee_screen = NULL;

/* Main screen reference (set by Badge_UI) */
extern lv_obj_t *badge_main_screen;

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
    game_trivia_show();
}

static void lab_btn_cb(lv_event_t *e)
{
    (void)e;
    game_lab_show();
}

/* ── Build Menu ── */
void easter_egg_show(void)
{
    if (ee_screen) return;

    ESP_LOGI(TAG, "Entering Developer Mode...");

    ee_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(ee_screen, EE_BG, 0);
    lv_obj_set_style_bg_opa(ee_screen, LV_OPA_COVER, 0);

    /* Grid pattern background (simple horizontal + vertical lines) */
    for (int x = 0; x <= 240; x += 20) {
        lv_obj_t *vl = lv_obj_create(ee_screen);
        lv_obj_set_size(vl, 1, 320);
        lv_obj_set_pos(vl, x, 0);
        lv_obj_set_style_bg_color(vl, EE_GRID, 0);
        lv_obj_set_style_bg_opa(vl, LV_OPA_30, 0);
        lv_obj_set_style_border_width(vl, 0, 0);
        lv_obj_clear_flag(vl, LV_OBJ_FLAG_SCROLLABLE);
    }
    for (int y = 0; y <= 320; y += 20) {
        lv_obj_t *hl = lv_obj_create(ee_screen);
        lv_obj_set_size(hl, 240, 1);
        lv_obj_set_pos(hl, 0, y);
        lv_obj_set_style_bg_color(hl, EE_GRID, 0);
        lv_obj_set_style_bg_opa(hl, LV_OPA_30, 0);
        lv_obj_set_style_border_width(hl, 0, 0);
        lv_obj_clear_flag(hl, LV_OBJ_FLAG_SCROLLABLE);
    }

    /* Top banner: DEVELOPER MODE */
    lv_obj_t *banner = lv_obj_create(ee_screen);
    lv_obj_set_size(banner, 240, 30);
    lv_obj_set_pos(banner, 0, 0);
    lv_obj_set_style_bg_color(banner, EE_YELLOW, 0);
    lv_obj_set_style_bg_opa(banner, LV_OPA_90, 0);
    lv_obj_set_style_radius(banner, 0, 0);
    lv_obj_set_style_border_width(banner, 0, 0);
    lv_obj_clear_flag(banner, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *banner_txt = lv_label_create(banner);
    lv_label_set_text(banner_txt, "> DEVELOPER MODE_");
    lv_obj_set_style_text_color(banner_txt, lv_color_hex(0x111111), 0);
    lv_obj_set_style_text_font(banner_txt, &lv_font_montserrat_12, 0);
    lv_obj_center(banner_txt);

    /* LabVIEW STOP button (top-right) */
    lv_obj_t *btn_stop = lv_btn_create(ee_screen);
    lv_obj_set_size(btn_stop, 50, 24);
    lv_obj_set_pos(btn_stop, 182, 38);
    lv_obj_set_style_bg_color(btn_stop, EE_RED, 0);
    lv_obj_set_style_border_color(btn_stop, EE_RED_BORDER, 0);
    lv_obj_set_style_border_width(btn_stop, 1, 0);
    lv_obj_set_style_radius(btn_stop, 4, 0);
    lv_obj_add_event_cb(btn_stop, stop_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *stop_lbl = lv_label_create(btn_stop);
    lv_label_set_text(stop_lbl, "STOP");
    lv_obj_set_style_text_font(stop_lbl, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(stop_lbl, lv_color_white(), 0);
    lv_obj_center(stop_lbl);

    /* Title */
    lv_obj_t *title = lv_label_create(ee_screen);
    lv_label_set_text(title, "Select Game");
    lv_obj_set_style_text_color(title, EE_TEXT, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 75);

    /* ── Game Card 1: NI Trivia ── */
    lv_obj_t *card1 = lv_obj_create(ee_screen);
    lv_obj_set_size(card1, 200, 80);
    lv_obj_align(card1, LV_ALIGN_TOP_MID, 0, 110);
    lv_obj_set_style_bg_color(card1, EE_CARD_BG, 0);
    lv_obj_set_style_bg_opa(card1, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(card1, EE_GREEN, 0);
    lv_obj_set_style_border_width(card1, 2, 0);
    lv_obj_set_style_radius(card1, 10, 0);
    lv_obj_set_style_pad_all(card1, 12, 0);
    lv_obj_clear_flag(card1, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(card1, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(card1, trivia_btn_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *c1_icon = lv_label_create(card1);
    lv_label_set_text(c1_icon, "?");
    lv_obj_set_style_text_font(c1_icon, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(c1_icon, EE_GREEN, 0);
    lv_obj_align(c1_icon, LV_ALIGN_LEFT_MID, 0, 0);

    lv_obj_t *c1_title = lv_label_create(card1);
    lv_label_set_text(c1_title, "NI Trivia");
    lv_obj_set_style_text_color(c1_title, lv_color_white(), 0);
    lv_obj_set_style_text_font(c1_title, &lv_font_montserrat_16, 0);
    lv_obj_align(c1_title, LV_ALIGN_TOP_LEFT, 40, 4);

    lv_obj_t *c1_desc = lv_label_create(card1);
    lv_label_set_text(c1_desc, "Test your NI knowledge!");
    lv_obj_set_style_text_color(c1_desc, EE_MUTED, 0);
    lv_obj_set_style_text_font(c1_desc, &lv_font_montserrat_10, 0);
    lv_obj_align(c1_desc, LV_ALIGN_TOP_LEFT, 40, 26);

    /* ── Game Card 2: NI Lab ── */
    lv_obj_t *card2 = lv_obj_create(ee_screen);
    lv_obj_set_size(card2, 200, 80);
    lv_obj_align(card2, LV_ALIGN_TOP_MID, 0, 205);
    lv_obj_set_style_bg_color(card2, EE_CARD_BG, 0);
    lv_obj_set_style_bg_opa(card2, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(card2, EE_YELLOW, 0);
    lv_obj_set_style_border_width(card2, 2, 0);
    lv_obj_set_style_radius(card2, 10, 0);
    lv_obj_set_style_pad_all(card2, 12, 0);
    lv_obj_clear_flag(card2, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(card2, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(card2, lab_btn_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *c2_icon = lv_label_create(card2);
    lv_label_set_text(c2_icon, LV_SYMBOL_SETTINGS);
    lv_obj_set_style_text_font(c2_icon, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(c2_icon, EE_YELLOW, 0);
    lv_obj_align(c2_icon, LV_ALIGN_LEFT_MID, 0, 0);

    lv_obj_t *c2_title = lv_label_create(card2);
    lv_label_set_text(c2_title, "NI Lab");
    lv_obj_set_style_text_color(c2_title, lv_color_white(), 0);
    lv_obj_set_style_text_font(c2_title, &lv_font_montserrat_16, 0);
    lv_obj_align(c2_title, LV_ALIGN_TOP_LEFT, 40, 4);

    lv_obj_t *c2_desc = lv_label_create(card2);
    lv_label_set_text(c2_desc, "Build your test system!");
    lv_obj_set_style_text_color(c2_desc, EE_MUTED, 0);
    lv_obj_set_style_text_font(c2_desc, &lv_font_montserrat_10, 0);
    lv_obj_align(c2_desc, LV_ALIGN_TOP_LEFT, 40, 26);

    /* Load screen */
    lv_scr_load_anim(ee_screen, LV_SCR_LOAD_ANIM_FADE_ON, 400, 0, false);
}

void easter_egg_exit(void)
{
    ESP_LOGI(TAG, "Exiting Developer Mode");
    if (badge_main_screen) {
        lv_scr_load_anim(badge_main_screen, LV_SCR_LOAD_ANIM_FADE_ON, 300, 0, true);
    }
    ee_screen = NULL;
}
