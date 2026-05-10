/**
 * page_calendar.c - Monthly calendar with NI milestone events
 * Reference: 2.8" badge version, adapted for 480x480 and wired to svc_rtc
 */
#include "ui.h"
#include "svc_rtc.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "page_cal";

/* ── Milestone data ── */
typedef struct { int month; int day; const char *title; const char *location; } milestone_t;
static const milestone_t milestones[] = {
    { 3, 15, "NI IPO (1995)",                "NASDAQ"},
    { 4, 12, "Emerson-NI Agreement (2023)",   ""},
    { 5,  1, "Agilent Joins PXI SA (2007)",   ""},
    { 6, 23, "LabVIEW 1.0 Announced (1986)",  ""},
    { 7,  6, "NI acquires OptimalPlus (2020)", ""},
    { 8, 17, "CompactRIO Launch (2004)",       "NIWeek"},
    {10, 11, "NI Joins Emerson (2023)",        ""},
    {11,  9, "PXI 25th Anniversary (2022)",    ""},
};
#define MS_COUNT (sizeof(milestones)/sizeof(milestones[0]))

static const char *month_names[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
static const char *dow_headers[] = {"Su","Mo","Tu","We","Th","Fr","Sa"};

/* ── Layout constants (480x480) ── */
#define CAL_GRID_X    36
#define CAL_GRID_Y    120
#define CAL_CELL_W    60
#define CAL_CELL_H    48
#define CAL_DOT_R     18

/* ── State ── */
static int cur_year = 2026;
static int cur_month = 5;
static int today_year = 2026;
static int today_month = 5;
static int today_day = 14;

static lv_obj_t *page_parent = NULL;
static lv_obj_t *lbl_header  = NULL;
static lv_obj_t *grid_cont   = NULL;
static lv_obj_t *card_event  = NULL;
static lv_obj_t *lbl_event_title = NULL;
static lv_obj_t *lbl_event_loc   = NULL;

/* ── Utility ── */
static int days_in_month(int y, int m) {
    static const int dm[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    int d = dm[m-1];
    if (m == 2 && ((y%4==0 && y%100!=0) || y%400==0)) d = 29;
    return d;
}

static int day_of_week(int y, int m, int d) {
    if (m < 3) { m += 12; y--; }
    int h = (d + (13*(m+1))/5 + y + y/4 - y/100 + y/400) % 7;
    return (h + 6) % 7; /* 0=Sun */
}

static bool is_milestone(int m, int d, int *idx) {
    for (int i = 0; i < (int)MS_COUNT; i++) {
        if (milestones[i].month == m && milestones[i].day == d) {
            if (idx) *idx = i;
            return true;
        }
    }
    return false;
}

static void sync_today_from_rtc(void)
{
    svc_rtc_time_t now = {0};
    if (svc_rtc_get_now(&now) != ESP_OK) {
        return;
    }

    today_year = now.year;
    today_month = now.month;
    today_day = now.day;
}

/* ── Event card show/hide ── */
static void show_event(int ms_idx) {
    lv_label_set_text(lbl_event_title, milestones[ms_idx].title);
    lv_label_set_text(lbl_event_loc, milestones[ms_idx].location);
    lv_obj_clear_flag(card_event, LV_OBJ_FLAG_HIDDEN);
}

/* ── Day cell click ── */
static void day_click_cb(lv_event_t *e) {
    int data = (int)(intptr_t)lv_event_get_user_data(e);
    int ms_idx;
    if (is_milestone(cur_month, data, &ms_idx)) {
        show_event(ms_idx);
    } else {
        lv_obj_add_flag(card_event, LV_OBJ_FLAG_HIDDEN);
    }
}

/* ── Rebuild grid ── */
static void rebuild_grid(void) {
    sync_today_from_rtc();

    /* Update header */
    char buf[32];
    snprintf(buf, sizeof(buf), "%s %d", month_names[cur_month-1], cur_year);
    lv_label_set_text(lbl_header, buf);

    /* Clear old grid children */
    lv_obj_clean(grid_cont);

    /* Hide event card */
    lv_obj_add_flag(card_event, LV_OBJ_FLAG_HIDDEN);

    int first_dow = day_of_week(cur_year, cur_month, 1);
    int total = days_in_month(cur_year, cur_month);

    /* Weekday headers */
    for (int i = 0; i < 7; i++) {
        lv_obj_t *lbl = lv_label_create(grid_cont);
        lv_label_set_text(lbl, dow_headers[i]);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0x8B949E), 0);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
        lv_obj_set_pos(lbl, i * CAL_CELL_W + CAL_CELL_W/2 - 8, 0);
    }

    /* Day cells */
    int row = 0, col = first_dow;
    for (int d = 1; d <= total; d++) {
        int x = col * CAL_CELL_W;
        int y = 24 + row * CAL_CELL_H;

        bool is_today = (d == today_day && cur_month == today_month && cur_year == today_year);
        int ms_idx = -1;
        bool is_ms = is_milestone(cur_month, d, &ms_idx);

        if (is_today) {
            /* Green filled circle for today */
            lv_obj_t *circ = lv_obj_create(grid_cont);
            lv_obj_set_size(circ, CAL_DOT_R*2, CAL_DOT_R*2);
            lv_obj_set_pos(circ, x + CAL_CELL_W/2 - CAL_DOT_R, y);
            lv_obj_set_style_radius(circ, LV_RADIUS_CIRCLE, 0);
            lv_obj_set_style_bg_color(circ, lv_color_hex(0x00A651), 0);
            lv_obj_set_style_bg_opa(circ, LV_OPA_COVER, 0);
            lv_obj_set_style_border_width(circ, 0, 0);
            lv_obj_set_style_shadow_width(circ, 0, 0);
            lv_obj_clear_flag(circ, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_add_flag(circ, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_add_event_cb(circ, day_click_cb, LV_EVENT_CLICKED, (void*)(intptr_t)d);

            lv_obj_t *lbl = lv_label_create(circ);
            char db[4]; snprintf(db, 4, "%d", d);
            lv_label_set_text(lbl, db);
            lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
            lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
            lv_obj_center(lbl);
        } else if (is_ms) {
            /* Yellow outline for milestone */
            lv_obj_t *circ = lv_obj_create(grid_cont);
            lv_obj_set_size(circ, CAL_DOT_R*2, CAL_DOT_R*2);
            lv_obj_set_pos(circ, x + CAL_CELL_W/2 - CAL_DOT_R, y);
            lv_obj_set_style_radius(circ, LV_RADIUS_CIRCLE, 0);
            lv_obj_set_style_bg_opa(circ, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_width(circ, 2, 0);
            lv_obj_set_style_border_color(circ, lv_color_hex(0xF1C40F), 0);
            lv_obj_set_style_shadow_width(circ, 0, 0);
            lv_obj_clear_flag(circ, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_add_flag(circ, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_add_event_cb(circ, day_click_cb, LV_EVENT_CLICKED, (void*)(intptr_t)d);

            lv_obj_t *lbl = lv_label_create(circ);
            char db[4]; snprintf(db, 4, "%d", d);
            lv_label_set_text(lbl, db);
            lv_obj_set_style_text_color(lbl, lv_color_hex(0xF1C40F), 0);
            lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
            lv_obj_center(lbl);

            /* Small yellow dot below */
            lv_obj_t *dot = lv_obj_create(grid_cont);
            lv_obj_set_size(dot, 6, 6);
            lv_obj_set_pos(dot, x + CAL_CELL_W/2 - 3, y + CAL_DOT_R*2 + 2);
            lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
            lv_obj_set_style_bg_color(dot, lv_color_hex(0xF1C40F), 0);
            lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
            lv_obj_set_style_border_width(dot, 0, 0);
            lv_obj_set_style_shadow_width(dot, 0, 0);
            lv_obj_clear_flag(dot, LV_OBJ_FLAG_SCROLLABLE);
        } else {
            /* Normal day label */
            lv_obj_t *lbl = lv_label_create(grid_cont);
            char db[4]; snprintf(db, 4, "%d", d);
            lv_label_set_text(lbl, db);
            lv_obj_set_style_text_color(lbl, lv_color_hex(0xC9D1D9), 0);
            lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
            lv_obj_set_pos(lbl, x + CAL_CELL_W/2 - 8, y + 4);
            lv_obj_add_flag(lbl, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_add_event_cb(lbl, day_click_cb, LV_EVENT_CLICKED, (void*)(intptr_t)d);
        }

        col++;
        if (col > 6) { col = 0; row++; }
    }
}

/* ── Nav callbacks ── */
static void prev_month_cb(lv_event_t *e) {
    (void)e;
    cur_month--;
    if (cur_month < 1) { cur_month = 12; cur_year--; }
    rebuild_grid();
}
static void next_month_cb(lv_event_t *e) {
    (void)e;
    cur_month++;
    if (cur_month > 12) { cur_month = 1; cur_year++; }
    rebuild_grid();
}
static void today_cb(lv_event_t *e) {
    (void)e;
    sync_today_from_rtc();
    cur_year = today_year;
    cur_month = today_month;
    rebuild_grid();
}

/* ── Page create ── */
lv_obj_t *page_calendar_create(lv_obj_t *parent)
{
    page_parent = parent;
    sync_today_from_rtc();
    cur_year = today_year;
    cur_month = today_month;

    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0D1117), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

    /* Title bar: < month year > [Today] */
    lv_obj_t *btn_prev = lv_btn_create(parent);
    lv_obj_set_size(btn_prev, 40, 36);
    lv_obj_set_pos(btn_prev, 30, 50);
    lv_obj_set_style_bg_color(btn_prev, lv_color_hex(0x30363D), 0);
    lv_obj_set_style_radius(btn_prev, 6, 0);
    lv_obj_set_style_shadow_width(btn_prev, 0, 0);
    lv_obj_add_event_cb(btn_prev, prev_month_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *pl = lv_label_create(btn_prev);
    lv_label_set_text(pl, LV_SYMBOL_LEFT);
    lv_obj_set_style_text_color(pl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(pl);

    lbl_header = lv_label_create(parent);
    lv_obj_set_style_text_color(lbl_header, lv_color_hex(0xF1C40F), 0);
    lv_obj_set_style_text_font(lbl_header, &lv_font_montserrat_24, 0);
    lv_obj_align(lbl_header, LV_ALIGN_TOP_MID, 0, 54);

    lv_obj_t *btn_next = lv_btn_create(parent);
    lv_obj_set_size(btn_next, 40, 36);
    lv_obj_set_pos(btn_next, 410, 50);
    lv_obj_set_style_bg_color(btn_next, lv_color_hex(0x30363D), 0);
    lv_obj_set_style_radius(btn_next, 6, 0);
    lv_obj_set_style_shadow_width(btn_next, 0, 0);
    lv_obj_add_event_cb(btn_next, next_month_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *nl = lv_label_create(btn_next);
    lv_label_set_text(nl, LV_SYMBOL_RIGHT);
    lv_obj_set_style_text_color(nl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(nl);

    /* Today button */
    lv_obj_t *btn_today = lv_btn_create(parent);
    lv_obj_set_size(btn_today, 70, 30);
    lv_obj_set_pos(btn_today, 380, 92);
    lv_obj_set_style_bg_color(btn_today, lv_color_hex(0x00A651), 0);
    lv_obj_set_style_radius(btn_today, 6, 0);
    lv_obj_set_style_shadow_width(btn_today, 0, 0);
    lv_obj_add_event_cb(btn_today, today_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *tl = lv_label_create(btn_today);
    lv_label_set_text(tl, "Today");
    lv_obj_set_style_text_color(tl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(tl, &lv_font_montserrat_14, 0);
    lv_obj_center(tl);

    /* Grid container */
    grid_cont = lv_obj_create(parent);
    lv_obj_set_size(grid_cont, 420, 330);
    lv_obj_set_pos(grid_cont, CAL_GRID_X, CAL_GRID_Y);
    lv_obj_set_style_bg_opa(grid_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(grid_cont, 0, 0);
    lv_obj_set_style_pad_all(grid_cont, 0, 0);
    lv_obj_set_style_shadow_width(grid_cont, 0, 0);
    lv_obj_clear_flag(grid_cont, LV_OBJ_FLAG_SCROLLABLE);

    /* Event detail card (hidden) */
    card_event = lv_obj_create(parent);
    lv_obj_set_size(card_event, 380, 56);
    lv_obj_align(card_event, LV_ALIGN_BOTTOM_MID, 0, -70);
    lv_obj_set_style_bg_color(card_event, lv_color_hex(0x161B22), 0);
    lv_obj_set_style_bg_opa(card_event, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(card_event, lv_color_hex(0xF1C40F), 0);
    lv_obj_set_style_border_width(card_event, 1, 0);
    lv_obj_set_style_radius(card_event, 10, 0);
    lv_obj_set_style_pad_all(card_event, 10, 0);
    lv_obj_set_style_shadow_width(card_event, 0, 0);
    lv_obj_clear_flag(card_event, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(card_event, LV_OBJ_FLAG_HIDDEN);

    lbl_event_title = lv_label_create(card_event);
    lv_obj_set_style_text_color(lbl_event_title, lv_color_hex(0xF1C40F), 0);
    lv_obj_set_style_text_font(lbl_event_title, &lv_font_montserrat_16, 0);
    lv_obj_align(lbl_event_title, LV_ALIGN_TOP_LEFT, 0, 0);

    lbl_event_loc = lv_label_create(card_event);
    lv_obj_set_style_text_color(lbl_event_loc, lv_color_hex(0x8B949E), 0);
    lv_obj_set_style_text_font(lbl_event_loc, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_event_loc, LV_ALIGN_BOTTOM_LEFT, 0, 0);

    /* Build initial grid */
    rebuild_grid();

    ESP_LOGI(TAG, "Calendar page created");
    return parent;
}
