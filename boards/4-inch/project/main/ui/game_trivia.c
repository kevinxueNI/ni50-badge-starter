/**
 * game_trivia.c — NI Knowledge Trivia Game (480×480 version)
 *
 * Binary choice (A/B) quiz with LabVIEW Boolean visual style.
 * 10 questions per round, immediate feedback.
 */
#include "ui.h"
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
    {"What year was NI founded?",                       "1976", "1986", 0},
    {"Jeff Kodosky is best known for which NI product?", "LabVIEW", "CompactDAQ", 0},
    {"What year was LabVIEW 1.0 released?",              "1986", "1997", 0},
    {"LabVIEW was originally designed for?",             "Graphical\nprogramming", "Command-line\nscripts", 0},
    {"NI's early core product direction involved?",      "Connecting\ninstruments to PCs", "Building\nprinters", 0},
    {"NI's first key instrument bus was?",               "GPIB", "HDMI", 0},
    {"In 1995, NI completed which milestone?",           "IPO", "Launched\nCompactRIO", 0},
    {"PXI was introduced in the 1990s as?",              "Modular automated\ntest platform", "Office software\nplatform", 0},
    {"A key PXI feature is combining PC tech with?",     "Modular instrument\narchitecture", "Handheld\nmultimeter shells", 0},
    {"PXI is most commonly used for?",                   "Automated test\n& measurement", "Document\nlayout", 0},
    {"CompactRIO launched in 2004 is positioned as?",    "Embedded control\n& acquisition", "Email client", 0},
    {"CompactRIO combines RT processor, modular I/O and?", "FPGA", "DVD drive", 0},
    {"CompactDAQ is most commonly used for?",            "Data acquisition\n& sensor meas.", "Video conference\ntranscription", 0},
    {"In NI's portfolio, DAQ stands for?",               "Data Acquisition", "Digital Art\nQuality", 0},
    {"NI hardware products typically emphasize?",        "Connecting to real\nphysical signals", "Auto-generating\nsocial media", 0},
    {"A PXI system typically includes chassis, controller and?", "Modular\ninstruments", "Keyboard\nprotectors", 0},
    {"PXI vs traditional box instruments: advantage?",   "High density,\nmodular, automated", "Single-unit\nmanual only", 0},
    {"NI RF & wireless test products serve?",            "RF verification\n& production test", "Restaurant\nordering", 0},
    {"NI VST is typically used for?",                    "RF signal gen\n& analysis", "Keyboard lifetime\ntesting", 0},
    {"NI SMU is suitable for?",                          "Source output\n& V/I measurement", "Meeting room\nbooking", 0},
    {"NI DMM refers to?",                                "Digital\nMultimeter", "Database Migration\nManager", 0},
    {"NI Switch modules are used for?",                  "Switching between\ntest channels", "Switching PPT\nthemes", 0},
    {"LabVIEW programs consist of which two parts?",     "Front Panel &\nBlock Diagram", "Inbox &\nCalendar", 0},
    {"In LabVIEW, VI stands for?",                       "Virtual\nInstrument", "Visual\nInvoice", 0},
    {"LabVIEW's graphical language is called?",          "G", "SQL", 0},
    {"LabVIEW excels at expressing?",                    "Dataflow, meas.,\ncontrol & auto.", "Novel\ntypesetting", 0},
    {"TestStand is typically used for?",                 "Test sequence\ndev & execution", "Email signature\ndesign", 0},
    {"In test systems, TestStand focuses on?",           "Test flow, steps\n& execution mgmt", "Disk\ndefragmentation", 0},
    {"LabVIEW is primarily for developing?",             "Measurement,\ncontrol & test apps", "Web ad\ncreation only", 0},
    {"SystemLink is commonly used for?",                 "Test system &\ndata management", "Personal photo\nbeautification", 0},
    {"SystemLink's value to test labs?",                 "Connect systems,\nmanage assets", "Replace all\nhardware sensors", 0},
    {"DIAdem is typically used for?",                    "Data analysis\n& report generation", "Chassis thermal\ncontrol HW", 0},
    {"FlexLogger is best suited for?",                   "No-code data\nlogging & sensing", "Enterprise\nfinancial ERP", 0},
    {"InstrumentStudio is used for?",                    "Interactive instr.\nconfig & debug", "Employee leave\napproval", 0},
    {"VeriStand is commonly seen in?",                   "Real-time test,\nHIL & validation", "Image compression\n& cropping", 0},
    {"In test, HIL stands for?",                         "Hardware-in-\nthe-Loop", "Human-in-\nLibrary", 0},
    {"NI automotive applications include?",              "ADAS, EV, Battery\n& HIL test", "Restaurant\nqueue management", 0},
    {"NI semiconductor applications include?",           "Chip validation\n& production test", "Movie subtitle\ntranslation", 0},
    {"NI test platform philosophy emphasizes?",          "SW-connected\nmodular HW & data", "Fixed single-\nfunction only", 0},
    {"NI joined Emerson in which year?",                 "2023", "2013", 0},
    {"After joining Emerson, NI is positioned as?",      "Test & Measurement\nbusiness", "Food & retail\nbusiness", 0},
    {"2026 is special for NI and LabVIEW because?",      "NI 50th &\nLabVIEW 40th", "PXI 100th\nanniversary", 0},
    {"LabVIEW+ Suite is best described as?",             "T&M software\nsuite", "Office renovation\ndesign suite", 0},
    {"Nigel AI is optimized for?",                       "Test & measurement\nengineering", "Restaurant menu\nrecommendation", 0},
    {"Nigel AI's role in NI software?",                  "AI assistant for\ntest dev & review", "Replace all engrs\nin safety decisions", 0},
    {"AI-generated test code should be?",                "Reviewed &\nvalidated by engrs", "Deployed directly\nwithout review", 0},
    {"NI software serves which test phases?",            "Design verification\nto production", "Meeting minutes\nonly", 0},
    {"Python's role in NI test development?",            "Data analysis,\nautomation scripts", "Replace all HW\nacquisition modules", 0},
    {"A benefit of modular test platforms?",             "Combine modules\n& reuse arch.", "Must redesign\nall HW each time", 0},
    {"NI's long-term engineering value?",                "Help engrs test\nfaster & reliably", "Office document\nauto-layout only", 0},
};
#define BANK_SIZE (sizeof(question_bank) / sizeof(question_bank[0]))
#define ROUND_SIZE 15

/* ── Game state ── */
static struct {
    lv_obj_t *screen;
    int question_order[BANK_SIZE];
    int current_q;
    int score;
    bool swap_current;
    /* UI handles */
    lv_obj_t *lbl_progress;
    lv_obj_t *progress_bar;
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

/* ── Shuffle ── */
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

/* ── Next question timer ── */
static void next_q_timer_cb(lv_timer_t *t)
{
    lv_timer_del(t);
    if (trv.current_q >= ROUND_SIZE || trv.current_q >= (int)BANK_SIZE) {
        show_result_screen();
    } else {
        show_question();
    }
}

/* ── Answer callback ── */
static void answer_cb(lv_event_t *e)
{
    int picked = (int)(intptr_t)lv_event_get_user_data(e);
    int q_idx = trv.question_order[trv.current_q];
    const trivia_question_t *q = &question_bank[q_idx];

    int logical_pick = picked;
    if (trv.swap_current) logical_pick = 1 - picked;
    bool correct = (logical_pick == q->correct);

    if (correct) {
        trv.score++;
        lv_obj_t *btn = (picked == 0) ? trv.btn_a : trv.btn_b;
        lv_obj_t *led = (picked == 0) ? trv.led_a : trv.led_b;
        lv_obj_set_style_bg_color(btn, TRV_GREEN, 0);
        lv_obj_set_style_bg_color(led, TRV_GREEN, 0);
        lv_obj_set_style_bg_opa(led, LV_OPA_COVER, 0);
    } else {
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

    lv_obj_clear_flag(trv.btn_a, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(trv.btn_b, LV_OBJ_FLAG_CLICKABLE);
    trv.current_q++;
    lv_timer_create(next_q_timer_cb, 800, NULL);
}

/* ── Show current question ── */
static void show_question(void)
{
    int q_idx = trv.question_order[trv.current_q];
    const trivia_question_t *q = &question_bank[q_idx];

    char prog[24];
    snprintf(prog, sizeof(prog), "Q: %02d/%02d", trv.current_q + 1, ROUND_SIZE);
    lv_label_set_text(trv.lbl_progress, prog);
    lv_bar_set_value(trv.progress_bar, (trv.current_q + 1) * 100 / ROUND_SIZE, LV_ANIM_ON);

    lv_label_set_text(trv.lbl_question, q->question);

    bool swap = (esp_random() % 2) == 1;
    trv.swap_current = swap;
    lv_label_set_text(trv.lbl_a, swap ? q->option_b : q->option_a);
    lv_label_set_text(trv.lbl_b, swap ? q->option_a : q->option_b);

    /* Reset button styles */
    lv_obj_set_style_bg_color(trv.btn_a, TRV_BTN_BG, 0);
    lv_obj_set_style_border_color(trv.btn_a, TRV_GREEN, 0);
    lv_obj_set_style_bg_color(trv.btn_b, TRV_BTN_BG, 0);
    lv_obj_set_style_border_color(trv.btn_b, TRV_BORDER, 0);
    lv_obj_set_style_bg_color(trv.led_a, lv_color_hex(0x0D1117), 0);
    lv_obj_set_style_bg_opa(trv.led_a, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(trv.led_b, lv_color_hex(0x0D1117), 0);
    lv_obj_set_style_bg_opa(trv.led_b, LV_OPA_COVER, 0);
    lv_obj_add_flag(trv.btn_a, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(trv.btn_b, LV_OBJ_FLAG_CLICKABLE);
}

/* ── STOP / Restart callbacks ── */
static void stop_cb(lv_event_t *e)
{
    (void)e;
    lv_obj_t *screen = trv.screen;
    trv.screen = NULL;
    app_router_go_tab(PAGE_CALENDAR, LV_ANIM_OFF);
    if (screen) {
        lv_obj_del_async(screen);
    }
}
static void restart_cb(lv_event_t *e) { (void)e; game_trivia_show(); }

/* ── Result screen ── */
static void show_result_screen(void)
{
    lv_obj_clean(trv.screen);

    lv_obj_t *title = lv_label_create(trv.screen);
    lv_label_set_text(title, "NI TRIVIA");
    lv_obj_set_style_text_color(title, TRV_YELLOW, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 60);

    char buf[32];
    snprintf(buf, sizeof(buf), "%d / %d", trv.score, ROUND_SIZE);
    lv_obj_t *result = lv_label_create(trv.screen);
    lv_label_set_text(result, buf);
    lv_obj_set_style_text_color(result, lv_color_white(), 0);
    lv_obj_set_style_text_font(result, &lv_font_montserrat_48, 0);
    lv_obj_align(result, LV_ALIGN_CENTER, 0, -40);

    const char *msg;
    if (trv.score == ROUND_SIZE) msg = "NI Legend!";
    else if (trv.score >= 12) msg = "NI Expert!";
    else if (trv.score >= 8) msg = "Solid! One more NIWeek\nand you'll be unstoppable.";
    else msg = "Nice try! Even LabVIEW 1.0\nhad bugs.";

    lv_obj_t *lbl_msg = lv_label_create(trv.screen);
    lv_label_set_text(lbl_msg, msg);
    lv_obj_set_style_text_color(lbl_msg, TRV_GREEN, 0);
    lv_obj_set_style_text_font(lbl_msg, &lv_font_montserrat_20, 0);
    lv_obj_align(lbl_msg, LV_ALIGN_CENTER, 0, 20);

    lv_obj_t *btn_restart = lv_btn_create(trv.screen);
    lv_obj_set_size(btn_restart, 160, 48);
    lv_obj_align(btn_restart, LV_ALIGN_CENTER, 0, 90);
    lv_obj_set_style_bg_color(btn_restart, TRV_GREEN, 0);
    lv_obj_set_style_radius(btn_restart, 10, 0);
    lv_obj_set_style_shadow_width(btn_restart, 0, 0);
    lv_obj_add_event_cb(btn_restart, restart_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *rl = lv_label_create(btn_restart);
    lv_label_set_text(rl, LV_SYMBOL_REFRESH " Restart");
    lv_obj_set_style_text_font(rl, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(rl, lv_color_white(), 0);
    lv_obj_center(rl);

    lv_obj_t *btn_stop = lv_btn_create(trv.screen);
    lv_obj_set_size(btn_stop, 160, 48);
    lv_obj_align(btn_stop, LV_ALIGN_CENTER, 0, 150);
    lv_obj_set_style_bg_color(btn_stop, TRV_STOP_RED, 0);
    lv_obj_set_style_radius(btn_stop, 10, 0);
    lv_obj_set_style_shadow_width(btn_stop, 0, 0);
    lv_obj_add_event_cb(btn_stop, stop_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *sl = lv_label_create(btn_stop);
    lv_label_set_text(sl, "STOP");
    lv_obj_set_style_text_font(sl, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(sl, lv_color_white(), 0);
    lv_obj_center(sl);
}

/* ── Show Trivia Game ── */
void game_trivia_show(void)
{
    ESP_LOGI(TAG, "Starting NI Trivia...");

    if (trv.screen) lv_obj_del(trv.screen);

    trv.screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(trv.screen, TRV_BG, 0);
    lv_obj_set_style_bg_opa(trv.screen, LV_OPA_COVER, 0);
    lv_obj_clear_flag(trv.screen, LV_OBJ_FLAG_SCROLLABLE);

    shuffle_questions();
    trv.current_q = 0;
    trv.score = 0;

    /* Header */
    lv_obj_t *hdr = lv_label_create(trv.screen);
    lv_label_set_text(hdr, "NI TRIVIA");
    lv_obj_set_style_text_color(hdr, TRV_MUTED, 0);
    lv_obj_set_style_text_font(hdr, &lv_font_montserrat_14, 0);
    lv_obj_align(hdr, LV_ALIGN_TOP_LEFT, 16, 12);

    trv.lbl_progress = lv_label_create(trv.screen);
    lv_obj_set_style_text_color(trv.lbl_progress, TRV_YELLOW, 0);
    lv_obj_set_style_text_font(trv.lbl_progress, &lv_font_montserrat_14, 0);
    lv_obj_align(trv.lbl_progress, LV_ALIGN_TOP_RIGHT, -16, 12);

    /* Progress bar */
    trv.progress_bar = lv_bar_create(trv.screen);
    lv_obj_set_size(trv.progress_bar, 420, 6);
    lv_obj_align(trv.progress_bar, LV_ALIGN_TOP_MID, 0, 38);
    lv_obj_set_style_bg_color(trv.progress_bar, lv_color_hex(0x1C2128), LV_PART_MAIN);
    lv_obj_set_style_bg_color(trv.progress_bar, TRV_YELLOW, LV_PART_INDICATOR);
    lv_bar_set_range(trv.progress_bar, 0, 100);

    /* Question box */
    lv_obj_t *qbox = lv_obj_create(trv.screen);
    lv_obj_set_size(qbox, 420, 100);
    lv_obj_align(qbox, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_set_style_bg_color(qbox, TRV_CARD_BG, 0);
    lv_obj_set_style_bg_opa(qbox, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(qbox, TRV_BORDER, 0);
    lv_obj_set_style_border_width(qbox, 1, 0);
    lv_obj_set_style_radius(qbox, 10, 0);
    lv_obj_set_style_pad_all(qbox, 12, 0);
    lv_obj_set_style_shadow_width(qbox, 0, 0);
    lv_obj_clear_flag(qbox, LV_OBJ_FLAG_SCROLLABLE);

    trv.lbl_question = lv_label_create(qbox);
    lv_obj_set_style_text_color(trv.lbl_question, TRV_TEXT, 0);
    lv_obj_set_style_text_font(trv.lbl_question, &lv_font_montserrat_18, 0);
    lv_obj_set_width(trv.lbl_question, 390);
    lv_label_set_long_mode(trv.lbl_question, LV_LABEL_LONG_WRAP);
    lv_obj_center(trv.lbl_question);

    /* ── Option A ── */
    trv.btn_a = lv_obj_create(trv.screen);
    lv_obj_set_size(trv.btn_a, 420, 80);
    lv_obj_align(trv.btn_a, LV_ALIGN_TOP_MID, 0, 180);
    lv_obj_set_style_bg_color(trv.btn_a, TRV_BTN_BG, 0);
    lv_obj_set_style_bg_opa(trv.btn_a, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(trv.btn_a, TRV_GREEN, 0);
    lv_obj_set_style_border_width(trv.btn_a, 2, 0);
    lv_obj_set_style_radius(trv.btn_a, 10, 0);
    lv_obj_set_style_pad_all(trv.btn_a, 8, 0);
    lv_obj_set_style_shadow_width(trv.btn_a, 0, 0);
    lv_obj_clear_flag(trv.btn_a, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(trv.btn_a, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(trv.btn_a, answer_cb, LV_EVENT_CLICKED, (void *)(intptr_t)0);

    trv.led_a = lv_obj_create(trv.btn_a);
    lv_obj_set_size(trv.led_a, 16, 16);
    lv_obj_align(trv.led_a, LV_ALIGN_LEFT_MID, 8, 0);
    lv_obj_set_style_radius(trv.led_a, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(trv.led_a, lv_color_hex(0x0D1117), 0);
    lv_obj_set_style_bg_opa(trv.led_a, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(trv.led_a, TRV_GREEN, 0);
    lv_obj_set_style_border_width(trv.led_a, 2, 0);
    lv_obj_set_style_shadow_width(trv.led_a, 0, 0);
    lv_obj_clear_flag(trv.led_a, LV_OBJ_FLAG_SCROLLABLE);

    trv.lbl_a = lv_label_create(trv.btn_a);
    lv_obj_set_style_text_color(trv.lbl_a, TRV_GREEN, 0);
    lv_obj_set_style_text_font(trv.lbl_a, &lv_font_montserrat_20, 0);
    lv_obj_align(trv.lbl_a, LV_ALIGN_CENTER, 16, 0);

    /* ── Option B ── */
    trv.btn_b = lv_obj_create(trv.screen);
    lv_obj_set_size(trv.btn_b, 420, 80);
    lv_obj_align(trv.btn_b, LV_ALIGN_TOP_MID, 0, 280);
    lv_obj_set_style_bg_color(trv.btn_b, TRV_BTN_BG, 0);
    lv_obj_set_style_bg_opa(trv.btn_b, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(trv.btn_b, TRV_BORDER, 0);
    lv_obj_set_style_border_width(trv.btn_b, 2, 0);
    lv_obj_set_style_radius(trv.btn_b, 10, 0);
    lv_obj_set_style_pad_all(trv.btn_b, 8, 0);
    lv_obj_set_style_shadow_width(trv.btn_b, 0, 0);
    lv_obj_clear_flag(trv.btn_b, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(trv.btn_b, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(trv.btn_b, answer_cb, LV_EVENT_CLICKED, (void *)(intptr_t)1);

    trv.led_b = lv_obj_create(trv.btn_b);
    lv_obj_set_size(trv.led_b, 16, 16);
    lv_obj_align(trv.led_b, LV_ALIGN_LEFT_MID, 8, 0);
    lv_obj_set_style_radius(trv.led_b, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(trv.led_b, lv_color_hex(0x0D1117), 0);
    lv_obj_set_style_bg_opa(trv.led_b, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(trv.led_b, lv_color_hex(0x4A4A4A), 0);
    lv_obj_set_style_border_width(trv.led_b, 2, 0);
    lv_obj_set_style_shadow_width(trv.led_b, 0, 0);
    lv_obj_clear_flag(trv.led_b, LV_OBJ_FLAG_SCROLLABLE);

    trv.lbl_b = lv_label_create(trv.btn_b);
    lv_obj_set_style_text_color(trv.lbl_b, TRV_MUTED, 0);
    lv_obj_set_style_text_font(trv.lbl_b, &lv_font_montserrat_20, 0);
    lv_obj_align(trv.lbl_b, LV_ALIGN_CENTER, 16, 0);

    /* Dataflow decoration line */
    lv_obj_t *flow = lv_obj_create(trv.screen);
    lv_obj_set_size(flow, 300, 2);
    lv_obj_align(flow, LV_ALIGN_BOTTOM_MID, 0, -50);
    lv_obj_set_style_bg_color(flow, TRV_GREEN, 0);
    lv_obj_set_style_bg_opa(flow, LV_OPA_50, 0);
    lv_obj_set_style_border_width(flow, 0, 0);
    lv_obj_set_style_shadow_width(flow, 0, 0);
    lv_obj_clear_flag(flow, LV_OBJ_FLAG_SCROLLABLE);

    /* STOP button */
    lv_obj_t *btn_stop = lv_btn_create(trv.screen);
    lv_obj_set_size(btn_stop, 70, 32);
    lv_obj_align(btn_stop, LV_ALIGN_BOTTOM_LEFT, 16, -10);
    lv_obj_set_style_bg_color(btn_stop, TRV_STOP_RED, 0);
    lv_obj_set_style_radius(btn_stop, 6, 0);
    lv_obj_set_style_shadow_width(btn_stop, 0, 0);
    lv_obj_add_event_cb(btn_stop, stop_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *sl = lv_label_create(btn_stop);
    lv_label_set_text(sl, "STOP");
    lv_obj_set_style_text_font(sl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(sl, lv_color_white(), 0);
    lv_obj_center(sl);

    /* Show first question */
    show_question();

    /* Load screen — immediate to avoid dual-screen render WDT */
    app_router_enter_immersive(trv.screen, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
}
