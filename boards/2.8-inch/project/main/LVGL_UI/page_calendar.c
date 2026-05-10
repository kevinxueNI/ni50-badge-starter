/**
 * page_calendar.c — NI Milestone Calendar
 *
 * Dark-themed monthly calendar with:
 * - Today in NI green circle
 * - NI milestone dates in yellow outline + dot
 * - Bottom event detail card on tap
 * - Left/Right arrows to navigate months
 *
 * Date source: PCF85063 RTC (extern datetime global)
 * Milestone data: hardcoded C array
 */

#include "page_calendar.h"
#include "PCF85063.h"
#include "esp_log.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "Calendar";

/* ── Colors ── */
#define CAL_BG         lv_color_hex(0x0D1117)
#define CAL_TEXT       lv_color_hex(0xC9D1D9)
#define CAL_MUTED      lv_color_hex(0x8B949E)
#define CAL_GREEN      lv_color_hex(0x00A651)
#define CAL_YELLOW     lv_color_hex(0xF1C40F)
#define CAL_CARD_BG    lv_color_hex(0x161B22)
#define CAL_BORDER     lv_color_hex(0x30363D)

/* ── Milestone data ── */
typedef struct {
    uint8_t month;
    uint8_t day;
    const char *title;
    const char *location;
} milestone_event_t;

static const milestone_event_t milestones[] = {
    { 1, 15, "NI Founded (1976)",         "Austin, Texas"           },
    { 4, 15, "LabVIEW 1.0 (1986)",        "Austin, Texas"           },
    { 5, 14, "NI 50th Anniversary",       "Austin & Global Live"    },
    { 6,  3, "PXI Standard (1997)",       "Industry Consortium"     },
    { 8,  2, "NI Week (Annual)",          "Austin Convention Center" },
    {10, 29, "Join Emerson (2023)",       "Global"                  },
    {11, 18, "LabVIEW 40th Birthday",     "Worldwide"               },
};
#define MILESTONE_COUNT (sizeof(milestones) / sizeof(milestones[0]))

/* ── Calendar layout constants ── */
#define CAL_GRID_X      18
#define CAL_GRID_Y      95
#define CAL_CELL_W      30
#define CAL_CELL_H      28
#define CAL_COLS        7
#define CAL_ROWS        6

/* ── State ── */
static struct {
    int view_year;
    int view_month;
    lv_obj_t *parent;
    lv_obj_t *lbl_month_year;
    lv_obj_t *day_labels[CAL_ROWS * CAL_COLS];
    lv_obj_t *day_bgs[CAL_ROWS * CAL_COLS];
    lv_obj_t *day_dots[CAL_ROWS * CAL_COLS];
    lv_obj_t *event_card;
    lv_obj_t *event_date;
    lv_obj_t *event_title;
    lv_obj_t *event_location;
} cal;

/* ── Utility: days in month ── */
static int days_in_month(int year, int month)
{
    static const int dim[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    int d = dim[month - 1];
    if (month == 2 && ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)))
        d = 29;
    return d;
}

/* Day of week for the 1st of given month (0=Mon, 6=Sun) using Zeller-like */
static int first_dow(int year, int month)
{
    /* Tomohiko Sakamoto's algorithm, returns 0=Sunday..6=Saturday */
    static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    int y = year;
    if (month < 3) y--;
    int dow = (y + y/4 - y/100 + y/400 + t[month-1] + 1) % 7;
    /* Convert to 0=Mon..6=Sun */
    return (dow + 6) % 7;
}

/* Check if a date is a milestone */
static const milestone_event_t *find_milestone(int month, int day)
{
    for (int i = 0; i < (int)MILESTONE_COUNT; i++) {
        if (milestones[i].month == month && milestones[i].day == day)
            return &milestones[i];
    }
    return NULL;
}

/* ── Show/hide event card ── */
static void show_event_card(const milestone_event_t *evt, int day)
{
    char buf[32];
    static const char *month_names[] = {
        "JAN","FEB","MAR","APR","MAY","JUN",
        "JUL","AUG","SEP","OCT","NOV","DEC"
    };
    snprintf(buf, sizeof(buf), "%s %d, %d", month_names[cal.view_month-1], day, cal.view_year);
    lv_label_set_text(cal.event_date, buf);
    lv_label_set_text(cal.event_title, evt->title);
    lv_label_set_text(cal.event_location, evt->location);
    lv_obj_clear_flag(cal.event_card, LV_OBJ_FLAG_HIDDEN);
}

static void hide_event_card(void)
{
    lv_obj_add_flag(cal.event_card, LV_OBJ_FLAG_HIDDEN);
}

/* ── Day cell click callback ── */
static void day_click_cb(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    /* Extract day number from label */
    const char *txt = lv_label_get_text(cal.day_labels[idx]);
    if (!txt || txt[0] == '\0') return;
    int day = atoi(txt);
    if (day <= 0) return;

    const milestone_event_t *evt = find_milestone(cal.view_month, day);
    if (evt) {
        show_event_card(evt, day);
    } else {
        hide_event_card();
    }
}

/* ── Render calendar grid for current view_month/view_year ── */
static void render_month(void)
{
    /* Update header */
    static const char *month_names[] = {
        "JANUARY","FEBRUARY","MARCH","APRIL","MAY","JUNE",
        "JULY","AUGUST","SEPTEMBER","OCTOBER","NOVEMBER","DECEMBER"
    };
    char hdr[32];
    snprintf(hdr, sizeof(hdr), "%s %d", month_names[cal.view_month-1], cal.view_year);
    lv_label_set_text(cal.lbl_month_year, hdr);

    int dow = first_dow(cal.view_year, cal.view_month);
    int dim = days_in_month(cal.view_year, cal.view_month);
    int today_day = (datetime.year == cal.view_year && datetime.month == cal.view_month)
                    ? datetime.day : -1;

    for (int i = 0; i < CAL_ROWS * CAL_COLS; i++) {
        int cell_day = i - dow + 1;
        bool valid = (cell_day >= 1 && cell_day <= dim);

        if (valid) {
            char buf[12];
            snprintf(buf, sizeof(buf), "%d", cell_day);
            lv_label_set_text(cal.day_labels[i], buf);

            /* Determine style */
            bool is_today = (cell_day == today_day);
            const milestone_event_t *ms = find_milestone(cal.view_month, cell_day);

            if (is_today) {
                /* Green filled circle */
                lv_obj_set_style_bg_color(cal.day_bgs[i], CAL_GREEN, 0);
                lv_obj_set_style_bg_opa(cal.day_bgs[i], LV_OPA_COVER, 0);
                lv_obj_set_style_border_width(cal.day_bgs[i], 0, 0);
                lv_obj_set_style_text_color(cal.day_labels[i], lv_color_white(), 0);
            } else if (ms) {
                /* Yellow outline circle */
                lv_obj_set_style_bg_opa(cal.day_bgs[i], LV_OPA_0, 0);
                lv_obj_set_style_border_width(cal.day_bgs[i], 1, 0);
                lv_obj_set_style_border_color(cal.day_bgs[i], CAL_YELLOW, 0);
                lv_obj_set_style_text_color(cal.day_labels[i], CAL_YELLOW, 0);
            } else {
                /* Normal */
                lv_obj_set_style_bg_opa(cal.day_bgs[i], LV_OPA_0, 0);
                lv_obj_set_style_border_width(cal.day_bgs[i], 0, 0);
                lv_obj_set_style_text_color(cal.day_labels[i], CAL_TEXT, 0);
            }

            /* Yellow dot for milestones */
            if (ms && !is_today) {
                lv_obj_clear_flag(cal.day_dots[i], LV_OBJ_FLAG_HIDDEN);
            } else {
                lv_obj_add_flag(cal.day_dots[i], LV_OBJ_FLAG_HIDDEN);
            }
        } else {
            lv_label_set_text(cal.day_labels[i], "");
            lv_obj_set_style_bg_opa(cal.day_bgs[i], LV_OPA_0, 0);
            lv_obj_set_style_border_width(cal.day_bgs[i], 0, 0);
            lv_obj_add_flag(cal.day_dots[i], LV_OBJ_FLAG_HIDDEN);
        }
    }

    hide_event_card();
}

/* ── Navigation callbacks ── */
static void prev_month_cb(lv_event_t *e)
{
    (void)e;
    cal.view_month--;
    if (cal.view_month < 1) { cal.view_month = 12; cal.view_year--; }
    render_month();
}

static void next_month_cb(lv_event_t *e)
{
    (void)e;
    cal.view_month++;
    if (cal.view_month > 12) { cal.view_month = 1; cal.view_year++; }
    render_month();
}

static void today_btn_cb(lv_event_t *e)
{
    (void)e;
    cal.view_year = datetime.year > 2000 ? datetime.year : 2026;
    cal.view_month = datetime.month > 0 ? datetime.month : 5;
    render_month();
}

/* ── Build Page ── */
void build_page_calendar(lv_obj_t *parent)
{
    cal.parent = parent;
    cal.view_year = datetime.year > 2000 ? datetime.year : 2026;
    cal.view_month = datetime.month > 0 ? datetime.month : 5;

    lv_obj_set_style_bg_color(parent, CAL_BG, 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

    /* Top label "Calendar" */
    lv_obj_t *top = lv_label_create(parent);
    lv_label_set_text(top, "Calendar");
    lv_obj_set_style_text_color(top, CAL_MUTED, 0);
    lv_obj_set_style_text_font(top, &lv_font_montserrat_10, 0);
    lv_obj_align(top, LV_ALIGN_TOP_LEFT, 16, 8);

    /* Today button (top-right) */
    lv_obj_t *btn_today = lv_btn_create(parent);
    lv_obj_set_size(btn_today, 48, 20);
    lv_obj_set_pos(btn_today, 182, 6);
    lv_obj_set_style_bg_color(btn_today, CAL_GREEN, 0);
    lv_obj_set_style_radius(btn_today, 10, 0);
    lv_obj_set_style_pad_all(btn_today, 0, 0);
    lv_obj_add_event_cb(btn_today, today_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl_today = lv_label_create(btn_today);
    lv_label_set_text(lbl_today, "Today");
    lv_obj_set_style_text_font(lbl_today, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(lbl_today, lv_color_white(), 0);
    lv_obj_center(lbl_today);

    /* Month/Year title + arrows */
    lv_obj_t *btn_left = lv_btn_create(parent);
    lv_obj_set_size(btn_left, 28, 28);
    lv_obj_set_pos(btn_left, 16, 28);
    lv_obj_set_style_bg_opa(btn_left, LV_OPA_0, 0);
    lv_obj_add_event_cb(btn_left, prev_month_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *arr_l = lv_label_create(btn_left);
    lv_label_set_text(arr_l, LV_SYMBOL_LEFT);
    lv_obj_set_style_text_color(arr_l, CAL_MUTED, 0);
    lv_obj_center(arr_l);

    cal.lbl_month_year = lv_label_create(parent);
    lv_obj_set_style_text_color(cal.lbl_month_year, lv_color_white(), 0);
    lv_obj_set_style_text_font(cal.lbl_month_year, &lv_font_montserrat_16, 0);
    lv_obj_align(cal.lbl_month_year, LV_ALIGN_TOP_MID, 0, 32);

    lv_obj_t *btn_right = lv_btn_create(parent);
    lv_obj_set_size(btn_right, 28, 28);
    lv_obj_set_pos(btn_right, 196, 28);
    lv_obj_set_style_bg_opa(btn_right, LV_OPA_0, 0);
    lv_obj_add_event_cb(btn_right, next_month_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *arr_r = lv_label_create(btn_right);
    lv_label_set_text(arr_r, LV_SYMBOL_RIGHT);
    lv_obj_set_style_text_color(arr_r, CAL_MUTED, 0);
    lv_obj_center(arr_r);

    /* Weekday headers */
    static const char *wday[] = {"M","T","W","T","F","S","S"};
    for (int i = 0; i < 7; i++) {
        lv_obj_t *lbl = lv_label_create(parent);
        lv_label_set_text(lbl, wday[i]);
        lv_obj_set_style_text_color(lbl, CAL_MUTED, 0);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_10, 0);
        lv_obj_set_pos(lbl, CAL_GRID_X + i * CAL_CELL_W + CAL_CELL_W / 2 - 3, 64);
    }

    /* Separator */
    lv_obj_t *sep = lv_obj_create(parent);
    lv_obj_set_size(sep, 210, 1);
    lv_obj_align(sep, LV_ALIGN_TOP_MID, 0, 78);
    lv_obj_set_style_bg_color(sep, CAL_BORDER, 0);
    lv_obj_set_style_bg_opa(sep, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(sep, 0, 0);
    lv_obj_clear_flag(sep, LV_OBJ_FLAG_SCROLLABLE);

    /* Day grid */
    for (int i = 0; i < CAL_ROWS * CAL_COLS; i++) {
        int col = i % CAL_COLS;
        int row = i / CAL_COLS;
        int x = CAL_GRID_X + col * CAL_CELL_W;
        int y = CAL_GRID_Y + row * CAL_CELL_H;

        /* Background circle (for today/milestone highlight) */
        lv_obj_t *bg = lv_obj_create(parent);
        lv_obj_set_size(bg, 24, 24);
        lv_obj_set_pos(bg, x + (CAL_CELL_W - 24) / 2, y);
        lv_obj_set_style_radius(bg, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_opa(bg, LV_OPA_0, 0);
        lv_obj_set_style_border_width(bg, 0, 0);
        lv_obj_set_style_pad_all(bg, 0, 0);
        lv_obj_clear_flag(bg, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(bg, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(bg, day_click_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        cal.day_bgs[i] = bg;

        /* Day number label */
        lv_obj_t *lbl = lv_label_create(bg);
        lv_label_set_text(lbl, "");
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(lbl, CAL_TEXT, 0);
        lv_obj_center(lbl);
        cal.day_labels[i] = lbl;

        /* Yellow dot (hidden by default) */
        lv_obj_t *dot = lv_obj_create(parent);
        lv_obj_set_size(dot, 4, 4);
        lv_obj_set_pos(dot, x + CAL_CELL_W / 2 - 2, y + 24);
        lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(dot, CAL_YELLOW, 0);
        lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(dot, 0, 0);
        lv_obj_clear_flag(dot, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(dot, LV_OBJ_FLAG_HIDDEN);
        cal.day_dots[i] = dot;
    }

    /* ── Bottom event detail card (hidden initially) ── */
    cal.event_card = lv_obj_create(parent);
    lv_obj_set_size(cal.event_card, 210, 50);
    lv_obj_align(cal.event_card, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_obj_set_style_bg_color(cal.event_card, CAL_CARD_BG, 0);
    lv_obj_set_style_bg_opa(cal.event_card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(cal.event_card, CAL_BORDER, 0);
    lv_obj_set_style_border_width(cal.event_card, 1, 0);
    lv_obj_set_style_radius(cal.event_card, 8, 0);
    lv_obj_set_style_pad_all(cal.event_card, 8, 0);
    lv_obj_clear_flag(cal.event_card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(cal.event_card, LV_OBJ_FLAG_HIDDEN);

    /* Yellow left accent bar */
    lv_obj_t *accent = lv_obj_create(cal.event_card);
    lv_obj_set_size(accent, 4, 36);
    lv_obj_align(accent, LV_ALIGN_LEFT_MID, -4, 0);
    lv_obj_set_style_bg_color(accent, CAL_YELLOW, 0);
    lv_obj_set_style_bg_opa(accent, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(accent, 2, 0);
    lv_obj_set_style_border_width(accent, 0, 0);
    lv_obj_clear_flag(accent, LV_OBJ_FLAG_SCROLLABLE);

    cal.event_date = lv_label_create(cal.event_card);
    lv_obj_set_style_text_color(cal.event_date, CAL_YELLOW, 0);
    lv_obj_set_style_text_font(cal.event_date, &lv_font_montserrat_10, 0);
    lv_obj_align(cal.event_date, LV_ALIGN_TOP_LEFT, 6, 0);

    cal.event_title = lv_label_create(cal.event_card);
    lv_obj_set_style_text_color(cal.event_title, lv_color_white(), 0);
    lv_obj_set_style_text_font(cal.event_title, &lv_font_montserrat_12, 0);
    lv_obj_align(cal.event_title, LV_ALIGN_TOP_LEFT, 6, 14);

    cal.event_location = lv_label_create(cal.event_card);
    lv_obj_set_style_text_color(cal.event_location, CAL_MUTED, 0);
    lv_obj_set_style_text_font(cal.event_location, &lv_font_montserrat_10, 0);
    lv_obj_align(cal.event_location, LV_ALIGN_TOP_LEFT, 6, 30);

    /* Render initial month */
    render_month();
    ESP_LOGI(TAG, "Calendar page built: %d/%d", cal.view_month, cal.view_year);
}
