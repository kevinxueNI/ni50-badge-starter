#!/usr/bin/env python3
"""
Fix 2048 tilt sensitivity + add page 6 (image slideshow from SD card)
"""
import sys
from pathlib import Path

filepath = Path(__file__).resolve().parents[1] / 'main' / 'LVGL_UI' / 'Badge_UI.c'

with open(filepath, 'r', encoding='utf-8') as f:
    content = f.read()

# ============================================================
# 1. Fix tilt parameters: increase threshold, add EMA smoothing
# ============================================================
old_params = """/* \u503e\u659c\u63a7\u5236\u53c2\u6570 */
#define TILT_THRESHOLD   0.38f   /* \u89e6\u53d1\u79fb\u52a8\u7684\u6700\u5c0f\u503e\u659c\u89d2 (G) */
#define TILT_COOLDOWN_MS 350     /* \u4e24\u6b21\u79fb\u52a8\u4e4b\u95f4\u7684\u6700\u77ed\u95f4\u9694 */"""

new_params = """/* \u503e\u659c\u63a7\u5236\u53c2\u6570 */
#define TILT_THRESHOLD   0.55f   /* \u89e6\u53d1\u79fb\u52a8\u7684\u6700\u5c0f\u503e\u659c\u89d2 (G) \u2014\u2014 \u589e\u5927\u6b7b\u533a\u51cf\u5c11\u8bef\u89e6\u53d1 */
#define TILT_COOLDOWN_MS 380     /* \u4e24\u6b21\u79fb\u52a8\u4e4b\u95f4\u7684\u6700\u77ed\u95f4\u9694 */
#define TILT_EMA_ALPHA   0.3f    /* EMA \u4f4e\u901a\u6ee4\u6ce2\u7cfb\u6570 (0-1, \u8d8a\u5c0f\u8d8a\u5e73\u6ed1) */
#define TILT_CONFIRM_CNT 2       /* \u8fde\u7eed N \u6b21\u540c\u65b9\u5411\u624d\u89e6\u53d1\u79fb\u52a8 */"""

assert old_params in content, "Tilt params not found!"
content = content.replace(old_params, new_params, 1)

# ============================================================
# 2. Add EMA state and confirm counter to struct
# ============================================================
old_struct_end = """    bool calibrated;
    bool game_over;
    bool animating;
} g2048;"""

new_struct_end = """    bool calibrated;
    bool game_over;
    bool animating;
    /* EMA \u6ee4\u6ce2\u72b6\u6001 */
    float ema_lr;
    float ema_up;
    int   confirm_dir;     /* \u4e0a\u6b21\u68c0\u6d4b\u5230\u7684\u65b9\u5411 (-1=none) */
    int   confirm_count;   /* \u8fde\u7eed\u540c\u65b9\u5411\u8ba1\u6570 */
} g2048;"""

assert old_struct_end in content, "Struct end not found!"
content = content.replace(old_struct_end, new_struct_end, 1)

# ============================================================
# 3. Replace tilt timer callback with smoothed + confirmed version
# ============================================================
old_tilt = """/* \u2500\u2500 \u503e\u659c\u63a7\u5236\u5b9a\u65f6\u5668 \u2500\u2500 */
static void g2048_tilt_timer_cb(lv_timer_t *t)
{
    (void)t;
    if (current_page != 4) return;
    if (g2048.game_over || g2048.animating) return;
    if (g2048.cal_step > 0) return;  /* \u6821\u51c6\u4e2d\u4e0d\u54cd\u5e94 */
    if (!g2048.calibrated) return;   /* \u672a\u6821\u51c6\u4e0d\u54cd\u5e94 */

    uint32_t now = lv_tick_get();
    if (now - g2048.last_move_ms < TILT_COOLDOWN_MS) return;

    /* \u8ba1\u7b97\u5f53\u524d\u52a0\u901f\u5ea6\u4e0e\u4e2d\u7acb\u4f4d\u7684\u5dee\u503c */
    float dx = Accel.x - g2048.cal_n[0];
    float dy = Accel.y - g2048.cal_n[1];
    float dz = Accel.z - g2048.cal_n[2];

    /* \u6295\u5f71\u5230 LR \u8f74 (\u6b63=\u53f3) */
    float lr_proj = dx * g2048.lr_dir[0] + dy * g2048.lr_dir[1] + dz * g2048.lr_dir[2];
    /* \u6295\u5f71\u5230 UP \u8f74 (\u6b63=\u524d/UP) */
    float up_proj = dx * g2048.up_dir[0] + dy * g2048.up_dir[1] + dz * g2048.up_dir[2];

    float abs_lr = (lr_proj < 0) ? -lr_proj : lr_proj;
    float abs_up = (up_proj < 0) ? -up_proj : up_proj;

    int dir = -1;
    if (abs_lr > abs_up && abs_lr > TILT_THRESHOLD) {
        dir = (lr_proj > 0) ? 1 : 0;   /* 1=right, 0=left */
    } else if (abs_up > abs_lr && abs_up > TILT_THRESHOLD) {
        dir = (up_proj > 0) ? 2 : 3;   /* 2=up, 3=down (but we handle down below) */
    } else {
        /* \u8fd1\u4e2d\u7acb\u4f4d\u7f6e (\u7ad6\u76f4) = \u81ea\u52a8 DOWN */
        dir = 3;
    }

    /* \u4fdd\u5b58\u79fb\u52a8\u524d\u72b6\u6001\u7528\u4e8e\u52a8\u753b */
    memcpy(g2048.prev_board, g2048.board, sizeof(g2048.board));

    if (g2048_move(dir)) {
        g2048.last_move_ms = now;
        g2048_spawn();
        if (!g2048_can_move()) g2048.game_over = true;
        g2048_animate_move(dir);
    }
}"""

new_tilt = """/* \u2500\u2500 \u503e\u659c\u63a7\u5236\u5b9a\u65f6\u5668 (EMA \u6ee4\u6ce2 + \u65b9\u5411\u786e\u8ba4) \u2500\u2500 */
static void g2048_tilt_timer_cb(lv_timer_t *t)
{
    (void)t;
    if (current_page != 4) return;
    if (g2048.game_over || g2048.animating) return;
    if (g2048.cal_step > 0) return;
    if (!g2048.calibrated) return;

    /* \u8ba1\u7b97\u5f53\u524d\u52a0\u901f\u5ea6\u4e0e\u4e2d\u7acb\u4f4d\u7684\u5dee\u503c */
    float dx = Accel.x - g2048.cal_n[0];
    float dy = Accel.y - g2048.cal_n[1];
    float dz = Accel.z - g2048.cal_n[2];

    /* \u6295\u5f71\u5230\u4e24\u4e2a\u8f74 */
    float lr_raw = dx * g2048.lr_dir[0] + dy * g2048.lr_dir[1] + dz * g2048.lr_dir[2];
    float up_raw = dx * g2048.up_dir[0] + dy * g2048.up_dir[1] + dz * g2048.up_dir[2];

    /* EMA \u4f4e\u901a\u6ee4\u6ce2 */
    g2048.ema_lr = TILT_EMA_ALPHA * lr_raw + (1.0f - TILT_EMA_ALPHA) * g2048.ema_lr;
    g2048.ema_up = TILT_EMA_ALPHA * up_raw + (1.0f - TILT_EMA_ALPHA) * g2048.ema_up;

    float abs_lr = (g2048.ema_lr < 0) ? -g2048.ema_lr : g2048.ema_lr;
    float abs_up = (g2048.ema_up < 0) ? -g2048.ema_up : g2048.ema_up;

    /* \u5224\u65ad\u65b9\u5411 (\u7ad6\u76f4/\u4e2d\u7acb = \u4e0d\u52a8) */
    int dir = -1;
    if (abs_lr > abs_up && abs_lr > TILT_THRESHOLD) {
        dir = (g2048.ema_lr > 0) ? 1 : 0;   /* 1=right, 0=left */
    } else if (abs_up > abs_lr && abs_up > TILT_THRESHOLD) {
        dir = (g2048.ema_up > 0) ? 2 : 3;   /* 2=up, 3=down */
    }

    /* \u6ca1\u8d85\u8fc7\u9608\u503c = \u4e2d\u7acb\u4f4d\u4e0d\u52a8 */
    if (dir < 0) {
        g2048.confirm_dir = -1;
        g2048.confirm_count = 0;
        return;
    }

    /* \u65b9\u5411\u786e\u8ba4\u673a\u5236: \u8fde\u7eed N \u6b21\u540c\u65b9\u5411\u624d\u89e6\u53d1 */
    if (dir != g2048.confirm_dir) {
        g2048.confirm_dir = dir;
        g2048.confirm_count = 1;
        return;
    }
    g2048.confirm_count++;
    if (g2048.confirm_count < TILT_CONFIRM_CNT) return;

    /* \u51b7\u5374\u68c0\u67e5 */
    uint32_t now = lv_tick_get();
    if (now - g2048.last_move_ms < TILT_COOLDOWN_MS) return;

    /* \u6267\u884c\u79fb\u52a8 */
    memcpy(g2048.prev_board, g2048.board, sizeof(g2048.board));

    if (g2048_move(dir)) {
        g2048.last_move_ms = now;
        g2048.confirm_count = 0;  /* \u91cd\u7f6e\u786e\u8ba4\u8ba1\u6570\u9632\u6b62\u8fde\u7eed\u89e6\u53d1 */
        g2048_spawn();
        if (!g2048_can_move()) g2048.game_over = true;
        g2048_animate_move(dir);
    }
}"""

assert old_tilt in content, "Tilt timer not found!"
content = content.replace(old_tilt, new_tilt, 1)

# ============================================================
# 4. Add Page 6 — Image slideshow from SD card
# ============================================================
# Add tile declaration
old_tile_decl = """static lv_obj_t *tile_2048      = NULL;  /* Page 4 */"""
new_tile_decl = """static lv_obj_t *tile_2048      = NULL;  /* Page 4 */
static lv_obj_t *tile_player    = NULL;  /* Page 5 \u2014 Image Player */"""
assert old_tile_decl in content, "Tile decl not found!"
content = content.replace(old_tile_decl, new_tile_decl, 1)

# Update NUM_PAGES
content = content.replace('#define NUM_PAGES 5', '#define NUM_PAGES 6', 1)

# Add tile_player to event callback
old_evt = """    else if (tile == tile_2048)     update_indicators(4);
}"""
new_evt = """    else if (tile == tile_2048)     update_indicators(4);
    else if (tile == tile_player)  update_indicators(5);
}"""
assert old_evt in content, "Event callback not found!"
content = content.replace(old_evt, new_evt, 1)

# Add the slideshow page code before build_page_card
build_card_marker = content.find('\nstatic void build_page_card(')
assert build_card_marker != -1, "build_page_card not found!"

slideshow_code = """
/* \u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550
 *  Page 5 \u2014 Image Slideshow (SD Card)
 * \u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550 */
#define PLAYER_MAX_FILES 50
#define PLAYER_DIR "/sdcard/photos"

static struct {
    char filenames[PLAYER_MAX_FILES][100];
    int  file_count;
    int  current_idx;
    lv_obj_t *img;
    lv_obj_t *lbl_name;
    lv_obj_t *lbl_counter;
    lv_obj_t *btn_prev;
    lv_obj_t *btn_next;
    lv_obj_t *btn_auto;
    lv_timer_t *auto_timer;
    bool auto_playing;
    uint8_t *img_buf;       /* \u52a8\u6001\u5206\u914d\u7684\u56fe\u7247\u7f13\u51b2 */
    size_t   img_buf_size;
} player;

static void player_show_image(void)
{
    if (player.file_count <= 0) {
        lv_label_set_text(player.lbl_name, "No images found");
        lv_label_set_text(player.lbl_counter, "0/0");
        return;
    }

    /* \u91ca\u653e\u4e4b\u524d\u7684\u7f13\u51b2 */
    if (player.img_buf) {
        free(player.img_buf);
        player.img_buf = NULL;
        player.img_buf_size = 0;
    }

    /* \u6784\u5efa\u5b8c\u6574\u8def\u5f84 */
    char path[220];
    snprintf(path, sizeof(path), "%s/%s", PLAYER_DIR, player.filenames[player.current_idx]);

    /* \u8bfb\u53d6\u6574\u4e2a\u6587\u4ef6\u5230\u5185\u5b58 */
    FILE *f = fopen(path, "rb");
    if (!f) {
        lv_label_set_text(player.lbl_name, "Open failed");
        return;
    }
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    /* \u9650\u5236\u5355\u5f20\u56fe\u7247\u5927\u5c0f (2MB max) */
    if (fsize > 2 * 1024 * 1024 || fsize <= 0) {
        fclose(f);
        lv_label_set_text(player.lbl_name, "File too large");
        return;
    }

    player.img_buf = (uint8_t *)malloc(fsize);
    if (!player.img_buf) {
        fclose(f);
        lv_label_set_text(player.lbl_name, "Out of memory");
        return;
    }
    fread(player.img_buf, 1, fsize, f);
    fclose(f);
    player.img_buf_size = fsize;

    /* \u786e\u5b9a\u56fe\u7247\u683c\u5f0f\u5e76\u8bbe\u7f6e LVGL \u56fe\u7247\u6e90 */
    const char *ext = strrchr(player.filenames[player.current_idx], '.');
    char src_str[240];
    if (ext && (strcasecmp(ext, ".png") == 0)) {
        /* \u4f7f\u7528 LVGL \u6587\u4ef6\u8def\u5f84 (P:\u524d\u7f00 = POSIX) */
        snprintf(src_str, sizeof(src_str), "P:%s/%s", PLAYER_DIR, player.filenames[player.current_idx]);
        lv_img_set_src(player.img, src_str);
    } else if (ext && (strcasecmp(ext, ".jpg") == 0 || strcasecmp(ext, ".jpeg") == 0)) {
        snprintf(src_str, sizeof(src_str), "P:%s/%s", PLAYER_DIR, player.filenames[player.current_idx]);
        lv_img_set_src(player.img, src_str);
    } else if (ext && (strcasecmp(ext, ".bmp") == 0)) {
        snprintf(src_str, sizeof(src_str), "P:%s/%s", PLAYER_DIR, player.filenames[player.current_idx]);
        lv_img_set_src(player.img, src_str);
    } else {
        lv_label_set_text(player.lbl_name, "Unsupported fmt");
        return;
    }

    /* \u66f4\u65b0\u6587\u4ef6\u540d\u548c\u8ba1\u6570\u5668 */
    lv_label_set_text(player.lbl_name, player.filenames[player.current_idx]);
    char cnt_str[16];
    snprintf(cnt_str, sizeof(cnt_str), "%d/%d", player.current_idx + 1, player.file_count);
    lv_label_set_text(player.lbl_counter, cnt_str);
}

static void player_next_cb(lv_event_t *e)
{
    (void)e;
    if (player.file_count <= 0) return;
    player.current_idx = (player.current_idx + 1) % player.file_count;
    player_show_image();
}

static void player_prev_cb(lv_event_t *e)
{
    (void)e;
    if (player.file_count <= 0) return;
    player.current_idx = (player.current_idx - 1 + player.file_count) % player.file_count;
    player_show_image();
}

static void player_auto_timer_cb(lv_timer_t *t)
{
    (void)t;
    if (current_page != 5) return;
    player_next_cb(NULL);
}

static void player_auto_cb(lv_event_t *e)
{
    (void)e;
    player.auto_playing = !player.auto_playing;
    if (player.auto_playing) {
        if (!player.auto_timer) {
            player.auto_timer = lv_timer_create(player_auto_timer_cb, 3000, NULL);
        } else {
            lv_timer_resume(player.auto_timer);
        }
        lv_label_set_text(lv_obj_get_child(player.btn_auto, 0), LV_SYMBOL_PAUSE);
    } else {
        if (player.auto_timer) lv_timer_pause(player.auto_timer);
        lv_label_set_text(lv_obj_get_child(player.btn_auto, 0), LV_SYMBOL_PLAY);
    }
}

static void player_scan_files(void)
{
    /* \u626b\u63cf\u5e38\u89c1\u56fe\u7247\u683c\u5f0f */
    player.file_count = 0;
    player.current_idx = 0;

    DIR *dir = opendir(PLAYER_DIR);
    if (!dir) {
        ESP_LOGW(TAG, "Player: cannot open %s", PLAYER_DIR);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && player.file_count < PLAYER_MAX_FILES) {
        if (entry->d_name[0] == '.') continue;
        const char *dot = strrchr(entry->d_name, '.');
        if (!dot) continue;
        if (strcasecmp(dot, ".jpg") == 0 || strcasecmp(dot, ".jpeg") == 0 ||
            strcasecmp(dot, ".png") == 0 || strcasecmp(dot, ".bmp") == 0) {
            strncpy(player.filenames[player.file_count], entry->d_name, 99);
            player.filenames[player.file_count][99] = '\\0';
            player.file_count++;
        }
    }
    closedir(dir);
    ESP_LOGI(TAG, "Player: found %d images in %s", player.file_count, PLAYER_DIR);
}

static void build_page_player(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0D1117), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

    /* \u6807\u9898 */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, LV_SYMBOL_IMAGE " Gallery");
    lv_obj_set_style_text_color(title, COLOR_TEXT_WHITE, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 6);

    /* \u56fe\u7247\u663e\u793a\u533a\u57df */
    player.img = lv_img_create(parent);
    lv_obj_set_size(player.img, 220, 220);
    lv_obj_align(player.img, LV_ALIGN_CENTER, 0, -10);
    lv_img_set_size_mode(player.img, LV_IMG_SIZE_MODE_REAL);
    lv_obj_set_style_radius(player.img, 8, 0);
    lv_obj_set_style_clip_corner(player.img, true, 0);

    /* \u6587\u4ef6\u540d */
    player.lbl_name = lv_label_create(parent);
    lv_label_set_text(player.lbl_name, "Scanning...");
    lv_obj_set_style_text_color(player.lbl_name, lv_color_hex(0x7F8C8D), 0);
    lv_obj_set_style_text_font(player.lbl_name, &lv_font_montserrat_10, 0);
    lv_obj_set_width(player.lbl_name, 200);
    lv_label_set_long_mode(player.lbl_name, LV_LABEL_LONG_DOT);
    lv_obj_align(player.lbl_name, LV_ALIGN_BOTTOM_MID, 0, -36);

    /* \u8ba1\u6570\u5668 */
    player.lbl_counter = lv_label_create(parent);
    lv_label_set_text(player.lbl_counter, "0/0");
    lv_obj_set_style_text_color(player.lbl_counter, COLOR_TEXT_WHITE, 0);
    lv_obj_set_style_text_font(player.lbl_counter, &lv_font_montserrat_10, 0);
    lv_obj_align(player.lbl_counter, LV_ALIGN_TOP_RIGHT, -8, 8);

    /* \u63a7\u5236\u6309\u94ae\u884c: < | \u25b6/\u23f8 | > */
    int btn_y = -8;
    int btn_h = 30;
    int btn_w = 50;

    /* Prev */
    player.btn_prev = lv_btn_create(parent);
    lv_obj_set_size(player.btn_prev, btn_w, btn_h);
    lv_obj_align(player.btn_prev, LV_ALIGN_BOTTOM_LEFT, 20, btn_y);
    lv_obj_set_style_bg_color(player.btn_prev, lv_color_hex(0x2C3E50), 0);
    lv_obj_set_style_radius(player.btn_prev, 15, 0);
    lv_obj_add_event_cb(player.btn_prev, player_prev_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lp = lv_label_create(player.btn_prev);
    lv_label_set_text(lp, LV_SYMBOL_LEFT);
    lv_obj_center(lp);

    /* Auto/Pause */
    player.btn_auto = lv_btn_create(parent);
    lv_obj_set_size(player.btn_auto, btn_w, btn_h);
    lv_obj_align(player.btn_auto, LV_ALIGN_BOTTOM_MID, 0, btn_y);
    lv_obj_set_style_bg_color(player.btn_auto, COLOR_NI_GREEN, 0);
    lv_obj_set_style_radius(player.btn_auto, 15, 0);
    lv_obj_add_event_cb(player.btn_auto, player_auto_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *la = lv_label_create(player.btn_auto);
    lv_label_set_text(la, LV_SYMBOL_PLAY);
    lv_obj_center(la);

    /* Next */
    player.btn_next = lv_btn_create(parent);
    lv_obj_set_size(player.btn_next, btn_w, btn_h);
    lv_obj_align(player.btn_next, LV_ALIGN_BOTTOM_RIGHT, -20, btn_y);
    lv_obj_set_style_bg_color(player.btn_next, lv_color_hex(0x2C3E50), 0);
    lv_obj_set_style_radius(player.btn_next, 15, 0);
    lv_obj_add_event_cb(player.btn_next, player_next_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *ln = lv_label_create(player.btn_next);
    lv_label_set_text(ln, LV_SYMBOL_RIGHT);
    lv_obj_center(ln);

    /* \u521d\u59cb\u5316 */
    player.img_buf = NULL;
    player.img_buf_size = 0;
    player.auto_playing = false;
    player.auto_timer = NULL;
    player_scan_files();
    player_show_image();
}
"""

content = content[:build_card_marker] + slideshow_code + content[build_card_marker:]

# ============================================================
# 5. Update Badge_UI_Init: 6 tiles
# ============================================================
old_tiles = """    /* \u4e94\u4e2a tile: \u6a2a\u5411\u6392\u5217\uff0c\u53ea\u5141\u8bb8\u6c34\u5e73\u6ed1\u52a8 */
    tile_card      = lv_tileview_add_tile(tileview, 0, 0, LV_DIR_RIGHT);
    tile_milestone = lv_tileview_add_tile(tileview, 1, 0, LV_DIR_LEFT | LV_DIR_RIGHT);
    tile_history   = lv_tileview_add_tile(tileview, 2, 0, LV_DIR_LEFT | LV_DIR_RIGHT);
    tile_game      = lv_tileview_add_tile(tileview, 3, 0, LV_DIR_LEFT | LV_DIR_RIGHT);
    tile_2048      = lv_tileview_add_tile(tileview, 4, 0, LV_DIR_LEFT);

    /* \u6784\u5efa\u5404\u9875\u5185\u5bb9 */
    build_page_card(tile_card);
    build_page_milestone(tile_milestone);
    build_page_history(tile_history);
    build_page_game(tile_game);
    build_page_2048(tile_2048);"""

new_tiles = """    /* \u516d\u4e2a tile: \u6a2a\u5411\u6392\u5217\uff0c\u53ea\u5141\u8bb8\u6c34\u5e73\u6ed1\u52a8 */
    tile_card      = lv_tileview_add_tile(tileview, 0, 0, LV_DIR_RIGHT);
    tile_milestone = lv_tileview_add_tile(tileview, 1, 0, LV_DIR_LEFT | LV_DIR_RIGHT);
    tile_history   = lv_tileview_add_tile(tileview, 2, 0, LV_DIR_LEFT | LV_DIR_RIGHT);
    tile_game      = lv_tileview_add_tile(tileview, 3, 0, LV_DIR_LEFT | LV_DIR_RIGHT);
    tile_2048      = lv_tileview_add_tile(tileview, 4, 0, LV_DIR_LEFT | LV_DIR_RIGHT);
    tile_player    = lv_tileview_add_tile(tileview, 5, 0, LV_DIR_LEFT);

    /* \u6784\u5efa\u5404\u9875\u5185\u5bb9 */
    build_page_card(tile_card);
    build_page_milestone(tile_milestone);
    build_page_history(tile_history);
    build_page_game(tile_game);
    build_page_2048(tile_2048);
    build_page_player(tile_player);"""

assert old_tiles in content, "Tiles init not found!"
content = content.replace(old_tiles, new_tiles, 1)

# Update the log message
content = content.replace(
    'ESP_LOGI(TAG, "Badge UI ready \u2014 5 tiles created");',
    'ESP_LOGI(TAG, "Badge UI ready \u2014 6 tiles created");',
    1
)

# Update the comment at top about "tileview" to mention 5 pages
old_comment = """    /* \u521b\u5efa tileview\uff08\u6a2a\u5411\u4e94\u9875\uff09 */"""
new_comment = """    /* \u521b\u5efa tileview\uff08\u6a2a\u5411\u516d\u9875\uff09 */"""
content = content.replace(old_comment, new_comment, 1)

# ============================================================
# 6. Add #include <dirent.h> and <sys/stat.h> for file operations
# ============================================================
if '#include <dirent.h>' not in content:
    content = content.replace('#include <math.h>', '#include <math.h>\n#include <dirent.h>\n#include <sys/stat.h>', 1)

# ============================================================
# Write back
# ============================================================
with open(filepath, 'w', encoding='utf-8', newline='\n') as f:
    f.write(content)

print("All modifications applied successfully!")
print(f"File size: {len(content)} bytes")
