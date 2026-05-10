/**
 * game_trivia.c — NI Knowledge Trivia Game
 *
 * Binary choice (A/B) quiz with LabVIEW Boolean visual style.
 * 10 questions per round, immediate True/False feedback.
 * Score displayed at end with restart option.
 */

#include "game_trivia.h"
#include "page_easter_egg.h"
#include "esp_random.h"
#include "esp_log.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "Trivia";

/* ── Colors ── */
#define TRV_BG         lv_color_hex(0x0D1117)
#define TRV_GREEN      lv_color_hex(0x00A651)
#define TRV_RED        lv_color_hex(0xDA3633)
#define TRV_YELLOW     lv_color_hex(0xF1C40F)
#define TRV_TEXT       lv_color_hex(0xC9D1D9)
#define TRV_MUTED      lv_color_hex(0x8B949E)
#define TRV_CARD_BG    lv_color_hex(0x161B22)
#define TRV_BTN_BG     lv_color_hex(0x1F2428)
#define TRV_BORDER     lv_color_hex(0x30363D)
#define TRV_STOP_RED   lv_color_hex(0xDA3633)

/* ── Question database ── */
typedef struct {
    const char *question;
    const char *option_a;
    const char *option_b;
    int correct;  /* 0=A, 1=B */
} trivia_question_t;

static const trivia_question_t question_bank[] = {
    {"What year was LabVIEW 1.0\nreleased?",
     "1986", "1996", 0},
    {"NI's modular instrument\nstandard is?",
     "PXI", "VME", 0},
    {"NI was founded in which\ncity?",
     "Austin, TX", "San Jose, CA", 0},
    {"What year was NI founded?",
     "1976", "1982", 0},
    {"NI joined which company\nin 2023?",
     "Emerson", "Siemens", 0},
    {"LabVIEW programming is\nbased on?",
     "Dataflow", "Object-Oriented", 0},
    {"NI's real-time OS for\nPXI controllers?",
     "NI Linux RT", "Windows CE", 0},
    {"CompactDAQ is designed\nfor?",
     "Portable DAQ", "RF Testing", 0},
    {"NI VST stands for?",
     "Vector Signal\nTransceiver", "Virtual\nSoftware Tool", 0},
    {"PXI backplane clock\nfrequency?",
     "10 MHz", "100 MHz", 0},
    {"LabVIEW wire color for\nBoolean?",
     "Green", "Blue", 0},
    {"NI DAQmx is used for?",
     "Data Acquisition", "RF Analysis", 0},
    {"TestStand is mainly for?",
     "Test Sequencing", "Data Logging", 0},
    {"NI GPIB standard is\nalso known as?",
     "IEEE 488", "IEEE 802", 0},
    {"What does NI stand for?",
     "National\nInstruments", "Network\nInterface", 0},
};
#define BANK_SIZE (sizeof(question_bank) / sizeof(question_bank[0]))
#define ROUND_SIZE 10

/* ── Game state ── */
static struct {
    lv_obj_t *screen;
    int question_order[BANK_SIZE];
    int current_q;
    int score;
    int total;
    bool swap_current;  /* true = options A/B are visually swapped */
    /* UI handles */
    lv_obj_t *lbl_progress;
    lv_obj_t *progress_bar;
    lv_obj_t *question_box;
    lv_obj_t *lbl_question;
    lv_obj_t *btn_a;
    lv_obj_t *btn_b;
    lv_obj_t *lbl_a;
    lv_obj_t *lbl_b;
    lv_obj_t *led_a;
    lv_obj_t *led_b;
} trv;

/* ── Forward declarations ── */
static void show_question(void);
static void show_result_screen(void);
static void next_question_timer_cb(lv_timer_t *t);

/* ── Shuffle question order ── */
static void shuffle_questions(void)
{
    for (int i = 0; i < (int)BANK_SIZE; i++) trv.question_order[i] = i;
    for (int i = (int)BANK_SIZE - 1; i > 0; i--) {
        int j = esp_random() % (i + 1);
        int tmp = trv.question_order[i];
        trv.question_order[i] = trv.question_order[j];
        trv.question_order[j] = tmp;
    }
}

/* ── Answer callback ── */
static void answer_cb(lv_event_t *e)
{
    int picked = (int)(intptr_t)lv_event_get_user_data(e);
    int q_idx = trv.question_order[trv.current_q];
    const trivia_question_t *q = &question_bank[q_idx];

    /* Map visual pick back to logical answer considering swap */
    int logical_pick = picked;
    if (trv.swap_current) logical_pick = 1 - picked;
    bool correct = (logical_pick == q->correct);

    if (correct) {
        trv.score++;
        /* Flash green on correct button */
        lv_obj_t *btn = (picked == 0) ? trv.btn_a : trv.btn_b;
        lv_obj_t *led = (picked == 0) ? trv.led_a : trv.led_b;
        lv_obj_set_style_bg_color(btn, TRV_GREEN, 0);
        lv_obj_set_style_bg_color(led, TRV_GREEN, 0);
        lv_obj_set_style_bg_opa(led, LV_OPA_COVER, 0);
    } else {
        /* Flash red on wrong, green on correct */
        lv_obj_t *wrong_btn = (picked == 0) ? trv.btn_a : trv.btn_b;
        lv_obj_t *right_btn = (picked == 0) ? trv.btn_b : trv.btn_a;
        lv_obj_t *wrong_led = (picked == 0) ? trv.led_a : trv.led_b;
        lv_obj_t *right_led = (picked == 0) ? trv.led_b : trv.led_a;
        lv_obj_set_style_bg_color(wrong_btn, TRV_RED, 0);
        lv_obj_set_style_bg_color(wrong_led, TRV_RED, 0);
        lv_obj_set_style_bg_opa(wrong_led, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(right_btn, TRV_GREEN, 0);
        lv_obj_set_style_bg_color(right_led, TRV_GREEN, 0);
        lv_obj_set_style_bg_opa(right_led, LV_OPA_COVER, 0);
    }

    /* Disable buttons temporarily */
    lv_obj_clear_flag(trv.btn_a, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(trv.btn_b, LV_OBJ_FLAG_CLICKABLE);

    trv.current_q++;
    trv.total++;

    /* Advance after brief delay */
    lv_timer_create(next_question_timer_cb, 800, NULL);
}

static void next_question_timer_cb(lv_timer_t *t)
{
    lv_timer_del(t);
    if (trv.current_q >= ROUND_SIZE || trv.current_q >= (int)BANK_SIZE) {
        show_result_screen();
    } else {
        show_question();
    }
}

/* ── Show current question ── */
static void show_question(void)
{
    int q_idx = trv.question_order[trv.current_q];
    const trivia_question_t *q = &question_bank[q_idx];

    /* Update progress */
    char prog[24];
    snprintf(prog, sizeof(prog), "Q: %02d/%02d", trv.current_q + 1, ROUND_SIZE);
    lv_label_set_text(trv.lbl_progress, prog);
    lv_bar_set_value(trv.progress_bar, (trv.current_q + 1) * 100 / ROUND_SIZE, LV_ANIM_ON);

    /* Update question */
    lv_label_set_text(trv.lbl_question, q->question);

    /* Randomly swap option positions */
    bool swap = (esp_random() % 2) == 1;
    if (swap) {
        lv_label_set_text(trv.lbl_a, q->option_b);
        lv_label_set_text(trv.lbl_b, q->option_a);
        /* Remap correct: if original correct=0(A), after swap correct is now B(1) */
        trv.swap_current = true;
    } else {
        lv_label_set_text(trv.lbl_a, q->option_a);
        lv_label_set_text(trv.lbl_b, q->option_b);
        trv.swap_current = false;
    }

    /* Reset button styles */
    lv_obj_set_style_bg_color(trv.btn_a, TRV_BTN_BG, 0);
    lv_obj_set_style_border_color(trv.btn_a, TRV_GREEN, 0);
    lv_obj_set_style_bg_color(trv.btn_b, TRV_BTN_BG, 0);
    lv_obj_set_style_border_color(trv.btn_b, TRV_BORDER, 0);

    /* Reset LED indicators */
    lv_obj_set_style_bg_color(trv.led_a, lv_color_hex(0x0D1117), 0);
    lv_obj_set_style_bg_opa(trv.led_a, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(trv.led_b, lv_color_hex(0x0D1117), 0);
    lv_obj_set_style_bg_opa(trv.led_b, LV_OPA_COVER, 0);

    /* Re-enable buttons */
    lv_obj_add_flag(trv.btn_a, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(trv.btn_b, LV_OBJ_FLAG_CLICKABLE);
}

/* ── STOP button callback ── */
static void stop_cb(lv_event_t *e)
{
    (void)e;
    easter_egg_exit();
}

/* ── Restart callback ── */
static void restart_cb(lv_event_t *e)
{
    (void)e;
    /* Delete current screen and relaunch */
    trv.screen = NULL;
    game_trivia_show();
}

/* ── Result screen ── */
static void show_result_screen(void)
{
    lv_obj_clean(trv.screen);

    lv_obj_t *title = lv_label_create(trv.screen);
    lv_label_set_text(title, "NI TRIVIA");
    lv_obj_set_style_text_color(title, TRV_YELLOW, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

    lv_obj_t *result = lv_label_create(trv.screen);
    char buf[32];
    snprintf(buf, sizeof(buf), "%d / %d", trv.score, ROUND_SIZE);
    lv_label_set_text(result, buf);
    lv_obj_set_style_text_color(result, lv_color_white(), 0);
    lv_obj_set_style_text_font(result, &lv_font_montserrat_28, 0);
    lv_obj_align(result, LV_ALIGN_CENTER, 0, -30);

    const char *msg;
    if (trv.score == ROUND_SIZE) msg = "Perfect! NI Expert!";
    else if (trv.score >= 8) msg = "Excellent!";
    else if (trv.score >= 6) msg = "Good job!";
    else msg = "Keep learning!";

    lv_obj_t *lbl_msg = lv_label_create(trv.screen);
    lv_label_set_text(lbl_msg, msg);
    lv_obj_set_style_text_color(lbl_msg, TRV_GREEN, 0);
    lv_obj_set_style_text_font(lbl_msg, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_msg, LV_ALIGN_CENTER, 0, 10);

    /* Restart button */
    lv_obj_t *btn_restart = lv_btn_create(trv.screen);
    lv_obj_set_size(btn_restart, 120, 36);
    lv_obj_align(btn_restart, LV_ALIGN_CENTER, 0, 60);
    lv_obj_set_style_bg_color(btn_restart, TRV_GREEN, 0);
    lv_obj_set_style_radius(btn_restart, 8, 0);
    lv_obj_add_event_cb(btn_restart, restart_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *rl = lv_label_create(btn_restart);
    lv_label_set_text(rl, LV_SYMBOL_REFRESH " Restart");
    lv_obj_set_style_text_font(rl, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(rl, lv_color_white(), 0);
    lv_obj_center(rl);

    /* STOP button */
    lv_obj_t *btn_stop = lv_btn_create(trv.screen);
    lv_obj_set_size(btn_stop, 120, 36);
    lv_obj_align(btn_stop, LV_ALIGN_CENTER, 0, 108);
    lv_obj_set_style_bg_color(btn_stop, TRV_STOP_RED, 0);
    lv_obj_set_style_radius(btn_stop, 8, 0);
    lv_obj_add_event_cb(btn_stop, stop_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *sl = lv_label_create(btn_stop);
    lv_label_set_text(sl, "STOP");
    lv_obj_set_style_text_font(sl, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(sl, lv_color_white(), 0);
    lv_obj_center(sl);
}

/* ── Show Trivia Game ── */
void game_trivia_show(void)
{
    ESP_LOGI(TAG, "Starting NI Trivia...");

    if (trv.screen) {
        lv_obj_del(trv.screen);
    }

    trv.screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(trv.screen, TRV_BG, 0);
    lv_obj_set_style_bg_opa(trv.screen, LV_OPA_COVER, 0);

    /* Init game state */
    shuffle_questions();
    trv.current_q = 0;
    trv.score = 0;
    trv.total = 0;

    /* Header */
    lv_obj_t *hdr = lv_label_create(trv.screen);
    lv_label_set_text(hdr, "NI TRIVIA");
    lv_obj_set_style_text_color(hdr, TRV_MUTED, 0);
    lv_obj_set_style_text_font(hdr, &lv_font_montserrat_10, 0);
    lv_obj_align(hdr, LV_ALIGN_TOP_LEFT, 12, 8);

    trv.lbl_progress = lv_label_create(trv.screen);
    lv_obj_set_style_text_color(trv.lbl_progress, TRV_YELLOW, 0);
    lv_obj_set_style_text_font(trv.lbl_progress, &lv_font_montserrat_10, 0);
    lv_obj_align(trv.lbl_progress, LV_ALIGN_TOP_RIGHT, -12, 8);

    /* Progress bar */
    trv.progress_bar = lv_bar_create(trv.screen);
    lv_obj_set_size(trv.progress_bar, 216, 4);
    lv_obj_align(trv.progress_bar, LV_ALIGN_TOP_MID, 0, 24);
    lv_obj_set_style_bg_color(trv.progress_bar, lv_color_hex(0x1C2128), LV_PART_MAIN);
    lv_obj_set_style_bg_color(trv.progress_bar, TRV_YELLOW, LV_PART_INDICATOR);
    lv_bar_set_range(trv.progress_bar, 0, 100);

    /* Question box */
    trv.question_box = lv_obj_create(trv.screen);
    lv_obj_set_size(trv.question_box, 210, 70);
    lv_obj_align(trv.question_box, LV_ALIGN_TOP_MID, 0, 38);
    lv_obj_set_style_bg_color(trv.question_box, TRV_CARD_BG, 0);
    lv_obj_set_style_bg_opa(trv.question_box, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(trv.question_box, TRV_BORDER, 0);
    lv_obj_set_style_border_width(trv.question_box, 1, 0);
    lv_obj_set_style_radius(trv.question_box, 8, 0);
    lv_obj_set_style_pad_all(trv.question_box, 8, 0);
    lv_obj_clear_flag(trv.question_box, LV_OBJ_FLAG_SCROLLABLE);

    trv.lbl_question = lv_label_create(trv.question_box);
    lv_obj_set_style_text_color(trv.lbl_question, TRV_TEXT, 0);
    lv_obj_set_style_text_font(trv.lbl_question, &lv_font_montserrat_12, 0);
    lv_obj_center(trv.lbl_question);

    /* ── Option A (upper button) ── */
    trv.btn_a = lv_obj_create(trv.screen);
    lv_obj_set_size(trv.btn_a, 210, 55);
    lv_obj_align(trv.btn_a, LV_ALIGN_TOP_MID, 0, 120);
    lv_obj_set_style_bg_color(trv.btn_a, TRV_BTN_BG, 0);
    lv_obj_set_style_bg_opa(trv.btn_a, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(trv.btn_a, TRV_GREEN, 0);
    lv_obj_set_style_border_width(trv.btn_a, 2, 0);
    lv_obj_set_style_radius(trv.btn_a, 8, 0);
    lv_obj_set_style_pad_all(trv.btn_a, 4, 0);
    lv_obj_clear_flag(trv.btn_a, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(trv.btn_a, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(trv.btn_a, answer_cb, LV_EVENT_CLICKED, (void *)(intptr_t)0);

    /* Boolean LED indicator */
    trv.led_a = lv_obj_create(trv.btn_a);
    lv_obj_set_size(trv.led_a, 12, 12);
    lv_obj_align(trv.led_a, LV_ALIGN_LEFT_MID, 4, 0);
    lv_obj_set_style_radius(trv.led_a, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(trv.led_a, lv_color_hex(0x0D1117), 0);
    lv_obj_set_style_bg_opa(trv.led_a, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(trv.led_a, TRV_GREEN, 0);
    lv_obj_set_style_border_width(trv.led_a, 1, 0);
    lv_obj_clear_flag(trv.led_a, LV_OBJ_FLAG_SCROLLABLE);

    trv.lbl_a = lv_label_create(trv.btn_a);
    lv_obj_set_style_text_color(trv.lbl_a, TRV_GREEN, 0);
    lv_obj_set_style_text_font(trv.lbl_a, &lv_font_montserrat_16, 0);
    lv_obj_align(trv.lbl_a, LV_ALIGN_CENTER, 10, 0);

    /* ── Option B (lower button) ── */
    trv.btn_b = lv_obj_create(trv.screen);
    lv_obj_set_size(trv.btn_b, 210, 55);
    lv_obj_align(trv.btn_b, LV_ALIGN_TOP_MID, 0, 186);
    lv_obj_set_style_bg_color(trv.btn_b, TRV_BTN_BG, 0);
    lv_obj_set_style_bg_opa(trv.btn_b, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(trv.btn_b, TRV_BORDER, 0);
    lv_obj_set_style_border_width(trv.btn_b, 2, 0);
    lv_obj_set_style_radius(trv.btn_b, 8, 0);
    lv_obj_set_style_pad_all(trv.btn_b, 4, 0);
    lv_obj_clear_flag(trv.btn_b, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(trv.btn_b, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(trv.btn_b, answer_cb, LV_EVENT_CLICKED, (void *)(intptr_t)1);

    trv.led_b = lv_obj_create(trv.btn_b);
    lv_obj_set_size(trv.led_b, 12, 12);
    lv_obj_align(trv.led_b, LV_ALIGN_LEFT_MID, 4, 0);
    lv_obj_set_style_radius(trv.led_b, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(trv.led_b, lv_color_hex(0x0D1117), 0);
    lv_obj_set_style_bg_opa(trv.led_b, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(trv.led_b, lv_color_hex(0x4A4A4A), 0);
    lv_obj_set_style_border_width(trv.led_b, 1, 0);
    lv_obj_clear_flag(trv.led_b, LV_OBJ_FLAG_SCROLLABLE);

    trv.lbl_b = lv_label_create(trv.btn_b);
    lv_obj_set_style_text_color(trv.lbl_b, TRV_MUTED, 0);
    lv_obj_set_style_text_font(trv.lbl_b, &lv_font_montserrat_16, 0);
    lv_obj_align(trv.lbl_b, LV_ALIGN_CENTER, 10, 0);

    /* ── Bottom: LabVIEW data flow decoration ── */
    lv_obj_t *flow = lv_obj_create(trv.screen);
    lv_obj_set_size(flow, 180, 2);
    lv_obj_align(flow, LV_ALIGN_BOTTOM_MID, 0, -30);
    lv_obj_set_style_bg_color(flow, TRV_GREEN, 0);
    lv_obj_set_style_bg_opa(flow, LV_OPA_50, 0);
    lv_obj_set_style_border_width(flow, 0, 0);
    lv_obj_clear_flag(flow, LV_OBJ_FLAG_SCROLLABLE);

    /* STOP button at bottom */
    lv_obj_t *btn_stop = lv_btn_create(trv.screen);
    lv_obj_set_size(btn_stop, 50, 24);
    lv_obj_align(btn_stop, LV_ALIGN_BOTTOM_RIGHT, -8, -4);
    lv_obj_set_style_bg_color(btn_stop, TRV_STOP_RED, 0);
    lv_obj_set_style_radius(btn_stop, 4, 0);
    lv_obj_add_event_cb(btn_stop, stop_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *sl = lv_label_create(btn_stop);
    lv_label_set_text(sl, "STOP");
    lv_obj_set_style_text_font(sl, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(sl, lv_color_white(), 0);
    lv_obj_center(sl);

    /* Show first question */
    show_question();

    /* Load screen */
    lv_scr_load_anim(trv.screen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, true);
}
