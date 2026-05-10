/**
 * page_gallery.c — NI Gallery
 *
 * Tile page: shows "NI Gallery" branding with Enter button.
 * Independent screen: two album choices -> swipeable image viewer.
 *
 * Albums:
 *   1. NI Logo (7 images, 1976–2020, with description text)
 *   2. LabVIEW 1.0 (3 images: Block Diagram, Splash Screen, News)
 */

#include "page_gallery.h"
#include "gallery/gallery_images.h"
#include "page_easter_egg.h"
#include "esp_log.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "Gallery";

/* Import main screen reference for returning */
extern lv_obj_t *badge_main_screen;

/* ── Colors (consistent with project dark theme) ── */
#define GAL_BG         lv_color_hex(0x0D1117)
#define GAL_CARD_BG    lv_color_hex(0x161B22)
#define GAL_GREEN      lv_color_hex(0x00A651)
#define GAL_YELLOW     lv_color_hex(0xF1C40F)
#define GAL_TEXT       lv_color_hex(0xC9D1D9)
#define GAL_MUTED      lv_color_hex(0x8B949E)
#define GAL_BORDER     lv_color_hex(0x30363D)
#define GAL_WHITE      lv_color_hex(0xFFFFFF)

/* ── NI Logo descriptions (condensed) ── */
static const char *nilogo_desc[NILOGO_COUNT] = {
    "1976 - First NI logo.\nTraced from an office\ntrash can oval.",
    "1979 - \"NI Eagle\" logo.\nDesigned by The Marketing\nGroup, Dallas. $1500.",
    "1986 - Added tagline\n\"The Software is the\nInstrument\" for LabVIEW.",
    "1995 - Sans-serif font\nfor IPO. Easier to print\non PCBs and collateral.",
    "1999 - Tagline dropped.\nExpanded beyond M&A to\nbroader product lines.",
    "2020 - Major rebrand.\nNational Instruments -> NI.\nNew logo & color.",
    "NI new logo.\nThe current brand identity\nused globally today.",
};

/* ── LabVIEW 1.0 descriptions ── */
static const char *lv1_desc[LV1_COUNT] = {
    "LabVIEW 1.0 Block Diagram\nThe original graphical\nprogramming interface.",
    "LabVIEW 1.0 Splash Screen\nCopyright 1986-1988\nNational Instruments.",
    "LabVIEW Launch News\nRevolutionizing test &\nmeasurement since 1986.",
};

/* ── Gallery state ── */
static struct {
    lv_obj_t *screen;       /* independent gallery screen */
    lv_obj_t *viewer_screen;/* image viewer screen */
    int album;              /* 0 = NI Logo, 1 = LabVIEW 1.0 */
    int current_idx;        /* current image index */
    int max_idx;            /* max index for current album */
    lv_obj_t *img_obj;      /* image widget */
    lv_obj_t *lbl_desc;     /* description label */
    lv_obj_t *lbl_counter;  /* page counter "3/7" */
} gal;

/* ═══════════════════════════════════════════════════════════════
 *  Image Viewer (swipeable single-image view)
 * ═══════════════════════════════════════════════════════════════ */

static void viewer_update(void)
{
    /* Update image */
    const lv_img_dsc_t *img_src;
    const char *desc;

    if (gal.album == 0) {
        img_src = gallery_nilogo_pool[gal.current_idx];
        desc = nilogo_desc[gal.current_idx];
    } else {
        img_src = gallery_lv1_pool[gal.current_idx];
        desc = lv1_desc[gal.current_idx];
    }

    lv_img_set_src(gal.img_obj, img_src);
    lv_label_set_text(gal.lbl_desc, desc);

    /* Update counter */
    char buf[32];
    snprintf(buf, sizeof(buf), "%d / %d", gal.current_idx + 1, gal.max_idx + 1);
    lv_label_set_text(gal.lbl_counter, buf);
}

static void viewer_prev_cb(lv_event_t *e)
{
    (void)e;
    if (gal.current_idx > 0) {
        gal.current_idx--;
        viewer_update();
    }
}

static void viewer_next_cb(lv_event_t *e)
{
    (void)e;
    if (gal.current_idx < gal.max_idx) {
        gal.current_idx++;
        viewer_update();
    }
}

static void viewer_back_cb(lv_event_t *e)
{
    (void)e;
    /* Return to album selection (auto_del=true deletes viewer_screen) */
    if (gal.screen) {
        lv_scr_load_anim(gal.screen, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 250, 0, true);
    }
    gal.viewer_screen = NULL;
}

/* Gesture detection for swipe left/right */
static void viewer_gesture_cb(lv_event_t *e)
{
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
    if (dir == LV_DIR_LEFT && gal.current_idx < gal.max_idx) {
        gal.current_idx++;
        viewer_update();
    } else if (dir == LV_DIR_RIGHT && gal.current_idx > 0) {
        gal.current_idx--;
        viewer_update();
    }
}

static void open_viewer(int album)
{
    gal.album = album;
    gal.current_idx = 0;
    gal.max_idx = (album == 0) ? (NILOGO_COUNT - 1) : (LV1_COUNT - 1);

    ESP_LOGI(TAG, "Opening viewer: album=%d, count=%d", album, gal.max_idx + 1);

    gal.viewer_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(gal.viewer_screen, GAL_BG, 0);
    lv_obj_set_style_bg_opa(gal.viewer_screen, LV_OPA_COVER, 0);

    /* Enable gesture on the screen */
    lv_obj_clear_flag(gal.viewer_screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(gal.viewer_screen, viewer_gesture_cb, LV_EVENT_GESTURE, NULL);

    /* Back button (top-left) */
    lv_obj_t *btn_back = lv_btn_create(gal.viewer_screen);
    lv_obj_set_size(btn_back, 40, 24);
    lv_obj_set_pos(btn_back, 8, 6);
    lv_obj_set_style_bg_color(btn_back, GAL_BORDER, 0);
    lv_obj_set_style_radius(btn_back, 4, 0);
    lv_obj_add_event_cb(btn_back, viewer_back_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *bl = lv_label_create(btn_back);
    lv_label_set_text(bl, LV_SYMBOL_LEFT);
    lv_obj_set_style_text_color(bl, GAL_WHITE, 0);
    lv_obj_set_style_text_font(bl, &lv_font_montserrat_12, 0);
    lv_obj_center(bl);

    /* Page counter (top-right) */
    gal.lbl_counter = lv_label_create(gal.viewer_screen);
    lv_obj_set_style_text_color(gal.lbl_counter, GAL_MUTED, 0);
    lv_obj_set_style_text_font(gal.lbl_counter, &lv_font_montserrat_12, 0);
    lv_obj_align(gal.lbl_counter, LV_ALIGN_TOP_RIGHT, -12, 10);

    /* Album title */
    lv_obj_t *title = lv_label_create(gal.viewer_screen);
    lv_label_set_text(title, (album == 0) ? "NI Logo" : "LabVIEW 1.0");
    lv_obj_set_style_text_color(title, GAL_YELLOW, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_12, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    /* Image area (centered in upper portion) */
    int img_w = (album == 0) ? NILOGO_IMG_W : LV1_IMG_W;
    int img_h = (album == 0) ? NILOGO_IMG_H : LV1_IMG_H;

    gal.img_obj = lv_img_create(gal.viewer_screen);
    lv_obj_align(gal.img_obj, LV_ALIGN_TOP_MID, 0, 36);
    lv_obj_set_size(gal.img_obj, img_w, img_h);

    /* Description text (below image) */
    int desc_y = 36 + img_h + 8;
    gal.lbl_desc = lv_label_create(gal.viewer_screen);
    lv_obj_set_style_text_color(gal.lbl_desc, GAL_TEXT, 0);
    lv_obj_set_style_text_font(gal.lbl_desc, &lv_font_montserrat_12, 0);
    lv_obj_set_width(gal.lbl_desc, 220);
    lv_label_set_long_mode(gal.lbl_desc, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(gal.lbl_desc, 10, desc_y);

    /* Navigation arrows (bottom) */
    lv_obj_t *btn_prev = lv_btn_create(gal.viewer_screen);
    lv_obj_set_size(btn_prev, 50, 30);
    lv_obj_align(btn_prev, LV_ALIGN_BOTTOM_LEFT, 20, -10);
    lv_obj_set_style_bg_color(btn_prev, GAL_BORDER, 0);
    lv_obj_set_style_radius(btn_prev, 6, 0);
    lv_obj_add_event_cb(btn_prev, viewer_prev_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *pl = lv_label_create(btn_prev);
    lv_label_set_text(pl, LV_SYMBOL_LEFT);
    lv_obj_set_style_text_color(pl, GAL_WHITE, 0);
    lv_obj_center(pl);

    lv_obj_t *btn_next = lv_btn_create(gal.viewer_screen);
    lv_obj_set_size(btn_next, 50, 30);
    lv_obj_align(btn_next, LV_ALIGN_BOTTOM_RIGHT, -20, -10);
    lv_obj_set_style_bg_color(btn_next, GAL_BORDER, 0);
    lv_obj_set_style_radius(btn_next, 6, 0);
    lv_obj_add_event_cb(btn_next, viewer_next_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *nl = lv_label_create(btn_next);
    lv_label_set_text(nl, LV_SYMBOL_RIGHT);
    lv_obj_set_style_text_color(nl, GAL_WHITE, 0);
    lv_obj_center(nl);

    /* Swipe hint */
    lv_obj_t *hint = lv_label_create(gal.viewer_screen);
    lv_label_set_text(hint, "< swipe >");
    lv_obj_set_style_text_color(hint, GAL_MUTED, 0);
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_10, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -16);

    /* Show first image */
    viewer_update();

    /* Load screen (auto_del=false to preserve album selection screen) */
    lv_scr_load_anim(gal.viewer_screen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 250, 0, false);
}

/* ═══════════════════════════════════════════════════════════════
 *  Album Selection Screen (independent screen)
 * ═══════════════════════════════════════════════════════════════ */

static void album_nilogo_cb(lv_event_t *e)
{
    (void)e;
    open_viewer(0);
}

static void album_lv1_cb(lv_event_t *e)
{
    (void)e;
    open_viewer(1);
}

static void gallery_exit_cb(lv_event_t *e)
{
    (void)e;
    ESP_LOGI(TAG, "Exiting NI Gallery");
    if (badge_main_screen) {
        lv_scr_load_anim(badge_main_screen, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 300, 0, true);
    }
    gal.screen = NULL;
    gal.viewer_screen = NULL;
}

/* Prevent click events on the screen from propagating */
static void screen_click_absorb_cb(lv_event_t *e)
{
    (void)e;
    /* Do nothing - just absorb the click */
}

static void show_gallery_screen(void)
{
    ESP_LOGI(TAG, "Opening NI Gallery...");

    if (gal.screen) {
        lv_obj_del(gal.screen);
    }

    gal.screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(gal.screen, GAL_BG, 0);
    lv_obj_set_style_bg_opa(gal.screen, LV_OPA_COVER, 0);
    lv_obj_clear_flag(gal.screen, LV_OBJ_FLAG_SCROLLABLE);

    /* Header */
    lv_obj_t *title = lv_label_create(gal.screen);
    lv_label_set_text(title, "NI GALLERY");
    lv_obj_set_style_text_color(title, GAL_YELLOW, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_letter_space(title, 2, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 30);

    lv_obj_t *sub = lv_label_create(gal.screen);
    lv_label_set_text(sub, "Choose an album");
    lv_obj_set_style_text_color(sub, GAL_MUTED, 0);
    lv_obj_set_style_text_font(sub, &lv_font_montserrat_12, 0);
    lv_obj_align(sub, LV_ALIGN_TOP_MID, 0, 56);

    /* Separator */
    lv_obj_t *sep = lv_obj_create(gal.screen);
    lv_obj_set_size(sep, 100, 1);
    lv_obj_align(sep, LV_ALIGN_TOP_MID, 0, 76);
    lv_obj_set_style_bg_color(sep, GAL_BORDER, 0);
    lv_obj_set_style_bg_opa(sep, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(sep, 0, 0);
    lv_obj_clear_flag(sep, LV_OBJ_FLAG_SCROLLABLE);

    /* Album 1: NI Logo */
    lv_obj_t *card1 = lv_obj_create(gal.screen);
    lv_obj_set_size(card1, 200, 70);
    lv_obj_align(card1, LV_ALIGN_TOP_MID, 0, 96);
    lv_obj_set_style_bg_color(card1, GAL_CARD_BG, 0);
    lv_obj_set_style_bg_opa(card1, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(card1, GAL_BORDER, 0);
    lv_obj_set_style_border_width(card1, 1, 0);
    lv_obj_set_style_radius(card1, 10, 0);
    lv_obj_set_style_pad_all(card1, 12, 0);
    lv_obj_clear_flag(card1, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(card1, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(card1, album_nilogo_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *c1_title = lv_label_create(card1);
    lv_label_set_text(c1_title, LV_SYMBOL_IMAGE "  NI Logo");
    lv_obj_set_style_text_color(c1_title, GAL_WHITE, 0);
    lv_obj_set_style_text_font(c1_title, &lv_font_montserrat_16, 0);
    lv_obj_align(c1_title, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t *c1_desc = lv_label_create(card1);
    lv_label_set_text(c1_desc, "Logo evolution 1976-2020 (7)");
    lv_obj_set_style_text_color(c1_desc, GAL_MUTED, 0);
    lv_obj_set_style_text_font(c1_desc, &lv_font_montserrat_10, 0);
    lv_obj_align(c1_desc, LV_ALIGN_BOTTOM_LEFT, 0, 0);

    /* Album 2: LabVIEW 1.0 */
    lv_obj_t *card2 = lv_obj_create(gal.screen);
    lv_obj_set_size(card2, 200, 70);
    lv_obj_align(card2, LV_ALIGN_TOP_MID, 0, 180);
    lv_obj_set_style_bg_color(card2, GAL_CARD_BG, 0);
    lv_obj_set_style_bg_opa(card2, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(card2, GAL_BORDER, 0);
    lv_obj_set_style_border_width(card2, 1, 0);
    lv_obj_set_style_radius(card2, 10, 0);
    lv_obj_set_style_pad_all(card2, 12, 0);
    lv_obj_clear_flag(card2, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(card2, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(card2, album_lv1_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *c2_title = lv_label_create(card2);
    lv_label_set_text(c2_title, LV_SYMBOL_IMAGE "  LabVIEW 1.0");
    lv_obj_set_style_text_color(c2_title, GAL_WHITE, 0);
    lv_obj_set_style_text_font(c2_title, &lv_font_montserrat_16, 0);
    lv_obj_align(c2_title, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t *c2_desc = lv_label_create(card2);
    lv_label_set_text(c2_desc, "The original LabVIEW (3)");
    lv_obj_set_style_text_color(c2_desc, GAL_MUTED, 0);
    lv_obj_set_style_text_font(c2_desc, &lv_font_montserrat_10, 0);
    lv_obj_align(c2_desc, LV_ALIGN_BOTTOM_LEFT, 0, 0);

    /* Exit button */
    lv_obj_t *btn_exit = lv_btn_create(gal.screen);
    lv_obj_set_size(btn_exit, 80, 32);
    lv_obj_align(btn_exit, LV_ALIGN_BOTTOM_MID, 0, -24);
    lv_obj_set_style_bg_color(btn_exit, GAL_BORDER, 0);
    lv_obj_set_style_radius(btn_exit, 6, 0);
    lv_obj_add_event_cb(btn_exit, gallery_exit_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *el = lv_label_create(btn_exit);
    lv_label_set_text(el, LV_SYMBOL_LEFT " Back");
    lv_obj_set_style_text_color(el, GAL_WHITE, 0);
    lv_obj_set_style_text_font(el, &lv_font_montserrat_12, 0);
    lv_obj_center(el);

    /* Load screen with animation (auto_del=false to preserve badge_main_screen) */
    lv_scr_load_anim(gal.screen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, false);
}

/* ═══════════════════════════════════════════════════════════════
 *  Tile Page (shown in main tileview)
 * ═══════════════════════════════════════════════════════════════ */

static void enter_gallery_cb(lv_event_t *e)
{
    (void)e;
    show_gallery_screen();
}

void build_page_gallery(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, GAL_BG, 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "NI GALLERY");
    lv_obj_set_style_text_color(title, GAL_YELLOW, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_letter_space(title, 2, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, -40);

    /* Subtitle */
    lv_obj_t *sub = lv_label_create(parent);
    lv_label_set_text(sub, "Logo History & LabVIEW 1.0");
    lv_obj_set_style_text_color(sub, GAL_MUTED, 0);
    lv_obj_set_style_text_font(sub, &lv_font_montserrat_12, 0);
    lv_obj_align(sub, LV_ALIGN_CENTER, 0, -8);

    /* Enter button */
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 120, 38);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, 40);
    lv_obj_set_style_bg_color(btn, GAL_GREEN, 0);
    lv_obj_set_style_radius(btn, 8, 0);
    lv_obj_add_event_cb(btn, enter_gallery_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_lbl = lv_label_create(btn);
    lv_label_set_text(btn_lbl, "Enter " LV_SYMBOL_RIGHT);
    lv_obj_set_style_text_color(btn_lbl, GAL_WHITE, 0);
    lv_obj_set_style_text_font(btn_lbl, &lv_font_montserrat_14, 0);
    lv_obj_center(btn_lbl);

    ESP_LOGI(TAG, "Gallery tile page built");
}
