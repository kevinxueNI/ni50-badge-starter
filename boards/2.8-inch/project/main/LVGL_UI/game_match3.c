/**
 * game_match3.c — "恩了个恩" 3D 堆叠消消乐
 *
 * 游戏逻辑 + LVGL UI 实现
 */

#include "game_match3.h"
#include "game_tiles/game_tiles.h"
#include "esp_log.h"
#include "esp_random.h"
#include <string.h>

static const char *TAG = "Match3";

/* ═══════════════════════════════════════════════════
 * 数据层
 * ═══════════════════════════════════════════════════ */

typedef enum {
    TS_FIELD_BLOCKED = 0, /* 在场上，被遮挡 */
    TS_FIELD_FREE,        /* 在场上，可点击 */
    TS_IN_TRAY,           /* 已入槽位 */
    TS_REMOVED,           /* 已消除 */
} tile_state_t;

typedef struct {
    int16_t  x, y;        /* 像素坐标（游戏区内） */
    int8_t   z;           /* 层级 0=底层 */
    int8_t   type;        /* 图案类型索引 (0..M3_TYPES_PER_GAME-1) */
    tile_state_t state;
    lv_obj_t *obj;        /* LVGL img 对象 */
} game_tile_t;

typedef struct {
    game_tile_t tiles[M3_TOTAL_TILES];
    int tray[M3_TRAY_SIZE];   /* 槽位中的 tile 索引，-1=空 */
    int tray_count;
    int remaining;            /* 场上剩余方块数 */
    int types_used[M3_TYPES_PER_GAME]; /* 本局选用的 tile pool 索引 */
    bool game_over;
    bool game_won;
} match3_state_t;

static match3_state_t gs;

/* ═══════════════════════════════════════════════════
 * UI 层
 * ═══════════════════════════════════════════════════ */

static lv_obj_t *game_container = NULL; /* 游戏区容器 */
static lv_obj_t *tray_container = NULL; /* 槽位容器 */
static lv_obj_t *tray_slots[M3_TRAY_SIZE]; /* 槽位背景 */
static lv_obj_t *tray_imgs[M3_TRAY_SIZE];  /* 槽位内图片 */
static lv_obj_t *lbl_status = NULL; /* 状态文字 */
static lv_obj_t *btn_reset = NULL;
static lv_obj_t *page_parent = NULL;

/* 游戏区偏移 (相对于 tile page) */
#define GAME_AREA_X    0
#define GAME_AREA_Y    28
#define GAME_AREA_W    240
#define GAME_AREA_H    232
#define TRAY_Y         264
#define TRAY_H         44
#define TRAY_SLOT_W    32
#define TRAY_SLOT_GAP  2

/* 每层方块数: 10+8+6 = 24 */
#define LAYER0_COUNT  10
#define LAYER1_COUNT   8
#define LAYER2_COUNT   6

/* ═══════════════════════════════════════════════════
 * 随机数辅助
 * ═══════════════════════════════════════════════════ */
static int rand_range(int min, int max)
{
    return min + (int)(esp_random() % (uint32_t)(max - min + 1));
}

/* Fisher-Yates shuffle */
static void shuffle_int(int *arr, int n)
{
    for (int i = n - 1; i > 0; i--) {
        int j = esp_random() % (uint32_t)(i + 1);
        int tmp = arr[i]; arr[i] = arr[j]; arr[j] = tmp;
    }
}

/* ═══════════════════════════════════════════════════
 * 遮挡检测
 * ═══════════════════════════════════════════════════ */
static bool tiles_overlap(const game_tile_t *a, const game_tile_t *b)
{
    /* 两方块包围盒是否重叠（阈值 = tile 边长的一半即可视为遮挡） */
    int dx = a->x - b->x;
    int dy = a->y - b->y;
    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;
    return (dx < M3_TILE_W - 4) && (dy < M3_TILE_H - 4);
}

static void update_tile_states(void)
{
    for (int i = 0; i < M3_TOTAL_TILES; i++) {
        if (gs.tiles[i].state != TS_FIELD_BLOCKED &&
            gs.tiles[i].state != TS_FIELD_FREE)
            continue;

        bool blocked = false;
        for (int j = 0; j < M3_TOTAL_TILES; j++) {
            if (j == i) continue;
            if (gs.tiles[j].state != TS_FIELD_BLOCKED &&
                gs.tiles[j].state != TS_FIELD_FREE)
                continue;
            if (gs.tiles[j].z <= gs.tiles[i].z) continue;
            if (tiles_overlap(&gs.tiles[i], &gs.tiles[j])) {
                blocked = true;
                break;
            }
        }
        gs.tiles[i].state = blocked ? TS_FIELD_BLOCKED : TS_FIELD_FREE;
    }
}

/* ═══════════════════════════════════════════════════
 * UI: 更新方块视觉状态
 * ═══════════════════════════════════════════════════ */
static void update_tile_visuals(void)
{
    for (int i = 0; i < M3_TOTAL_TILES; i++) {
        lv_obj_t *obj = gs.tiles[i].obj;
        if (!obj) continue;

        switch (gs.tiles[i].state) {
        case TS_FIELD_BLOCKED:
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_img_recolor(obj, lv_color_hex(0x333333), 0);
            lv_obj_set_style_img_recolor_opa(obj, 120, 0);
            lv_obj_set_style_outline_color(obj, lv_color_hex(0x666666), 0);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
            break;
        case TS_FIELD_FREE:
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_img_recolor_opa(obj, 0, 0);
            lv_obj_set_style_outline_color(obj, lv_color_hex(0xCCCCCC), 0);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
            break;
        case TS_IN_TRAY:
        case TS_REMOVED:
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
            break;
        }
    }
}

/* ═══════════════════════════════════════════════════
 * UI: 更新槽位显示
 * ═══════════════════════════════════════════════════ */
static void update_tray_ui(void)
{
    for (int i = 0; i < M3_TRAY_SIZE; i++) {
        if (gs.tray[i] >= 0) {
            int type = gs.tiles[gs.tray[i]].type;
            lv_img_set_src(tray_imgs[i], game_tile_pool[gs.types_used[type]]);
            lv_obj_clear_flag(tray_imgs[i], LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(tray_imgs[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
}

/* ═══════════════════════════════════════════════════
 * 游戏逻辑: 点击方块
 * ═══════════════════════════════════════════════════ */
static void check_tray_match(void)
{
    /* 检查槽位中是否有三个相同类型 */
    for (int i = 0; i < gs.tray_count; i++) {
        if (gs.tray[i] < 0) continue;
        int type_i = gs.tiles[gs.tray[i]].type;
        int matches[3] = {i, -1, -1};
        int mc = 1;

        for (int j = i + 1; j < gs.tray_count && mc < 3; j++) {
            if (gs.tray[j] < 0) continue;
            if (gs.tiles[gs.tray[j]].type == type_i) {
                matches[mc++] = j;
            }
        }

        if (mc == 3) {
            /* 消除这三个 */
            for (int k = 0; k < 3; k++) {
                int ti = gs.tray[matches[k]];
                gs.tiles[ti].state = TS_REMOVED;
                gs.tray[matches[k]] = -1;
            }
            gs.remaining -= 3;

            /* 压缩槽位数组 */
            int new_tray[M3_TRAY_SIZE];
            int nc = 0;
            for (int j = 0; j < M3_TRAY_SIZE; j++) {
                if (gs.tray[j] >= 0) new_tray[nc++] = gs.tray[j];
            }
            for (int j = nc; j < M3_TRAY_SIZE; j++) new_tray[j] = -1;
            memcpy(gs.tray, new_tray, sizeof(gs.tray));
            gs.tray_count = nc;

            /* 递归检查（可能还有匹配） */
            check_tray_match();
            return;
        }
    }
}

static void on_tile_clicked(int tile_idx)
{
    if (gs.game_over || gs.game_won) return;
    if (gs.tiles[tile_idx].state != TS_FIELD_FREE) return;
    if (gs.tray_count >= M3_TRAY_SIZE) return;

    /* 放入槽位：插入到同类型旁边 */
    int insert_pos = gs.tray_count;
    int type = gs.tiles[tile_idx].type;

    /* 找同类型的最后一个位置 */
    for (int i = gs.tray_count - 1; i >= 0; i--) {
        if (gs.tray[i] >= 0 && gs.tiles[gs.tray[i]].type == type) {
            insert_pos = i + 1;
            break;
        }
    }

    /* 右移腾出位置 */
    for (int i = gs.tray_count; i > insert_pos; i--) {
        gs.tray[i] = gs.tray[i - 1];
    }
    gs.tray[insert_pos] = tile_idx;
    gs.tray_count++;
    gs.tiles[tile_idx].state = TS_IN_TRAY;

    /* 检查消除 */
    check_tray_match();

    /* 更新遮挡状态 */
    update_tile_states();

    /* 检查胜利 */
    if (gs.remaining <= 0) {
        gs.game_won = true;
        lv_label_set_text(lbl_status, LV_SYMBOL_OK " WIN!");
        lv_obj_clear_flag(lbl_status, LV_OBJ_FLAG_HIDDEN);
    }
    /* 检查失败 */
    else if (gs.tray_count >= M3_TRAY_SIZE) {
        gs.game_over = true;
        lv_label_set_text(lbl_status, "FULL!");
        lv_obj_clear_flag(lbl_status, LV_OBJ_FLAG_HIDDEN);
    }

    /* 刷新 UI */
    update_tile_visuals();
    update_tray_ui();
}

/* ═══════════════════════════════════════════════════
 * LVGL 事件回调
 * ═══════════════════════════════════════════════════ */
static void tile_click_cb(lv_event_t *e)
{
    /* 从 user_data 获取 tile index */
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    if (idx >= 0 && idx < M3_TOTAL_TILES) {
        on_tile_clicked(idx);
    }
}

static void reset_click_cb(lv_event_t *e)
{
    (void)e;
    match3_reset();
}

/* ═══════════════════════════════════════════════════
 * 关卡生成
 * ═══════════════════════════════════════════════════ */

/* 预定义每层的位置布局 (相对于 game_container) */
/* 46px tiles, game area 240x232, 3 layers */
static const int16_t layer0_pos[][2] = {
    { 4,  4}, {52,  4}, {100,  4}, {148,  4}, {190,  4},
    {26, 54}, {74, 54}, {122, 54}, {170, 54},
    {52, 104},
};

static const int16_t layer1_pos[][2] = {
    {26, 10}, {78, 10}, {130, 10}, {174, 10},
    { 4, 60}, {100, 60}, {148, 60}, {52, 108},
};

static const int16_t layer2_pos[][2] = {
    {52, 16}, {108, 16}, {156, 16},
    {28, 64}, {80, 64}, {136, 64},
};

static void generate_level(void)
{
    /* 选取本局使用的 10 种图案 */
    int pool_indices[GAME_TILE_COUNT];
    for (int i = 0; i < GAME_TILE_COUNT; i++) pool_indices[i] = i;
    shuffle_int(pool_indices, GAME_TILE_COUNT);
    for (int i = 0; i < M3_TYPES_PER_GAME; i++) {
        gs.types_used[i] = pool_indices[i];
    }

    /* 生成方块列表: 每种类型 3 个 */
    int type_list[M3_TOTAL_TILES];
    for (int t = 0; t < M3_TYPES_PER_GAME; t++) {
        type_list[t * 3 + 0] = t;
        type_list[t * 3 + 1] = t;
        type_list[t * 3 + 2] = t;
    }
    shuffle_int(type_list, M3_TOTAL_TILES);

    /* 分配到各层 */
    int ti = 0;
    /* Layer 0 */
    for (int i = 0; i < LAYER0_COUNT; i++, ti++) {
        gs.tiles[ti].x = layer0_pos[i][0] + rand_range(-3, 3);
        gs.tiles[ti].y = layer0_pos[i][1] + rand_range(-3, 3);
        gs.tiles[ti].z = 0;
        gs.tiles[ti].type = type_list[ti];
        gs.tiles[ti].state = TS_FIELD_BLOCKED;
    }
    /* Layer 1 */
    for (int i = 0; i < LAYER1_COUNT; i++, ti++) {
        gs.tiles[ti].x = layer1_pos[i][0] + rand_range(-3, 3);
        gs.tiles[ti].y = layer1_pos[i][1] + rand_range(-3, 3);
        gs.tiles[ti].z = 1;
        gs.tiles[ti].type = type_list[ti];
        gs.tiles[ti].state = TS_FIELD_BLOCKED;
    }
    /* Layer 2 */
    for (int i = 0; i < LAYER2_COUNT; i++, ti++) {
        gs.tiles[ti].x = layer2_pos[i][0] + rand_range(-3, 3);
        gs.tiles[ti].y = layer2_pos[i][1] + rand_range(-3, 3);
        gs.tiles[ti].z = 2;
        gs.tiles[ti].type = type_list[ti];
        gs.tiles[ti].state = TS_FIELD_BLOCKED;
    }

    /* 初始化槽位 */
    for (int i = 0; i < M3_TRAY_SIZE; i++) gs.tray[i] = -1;
    gs.tray_count = 0;
    gs.remaining = M3_TOTAL_TILES;
    gs.game_over = false;
    gs.game_won = false;

    /* 计算初始遮挡 */
    update_tile_states();
}

/* ═══════════════════════════════════════════════════
 * UI 创建
 * ═══════════════════════════════════════════════════ */
static void create_game_tiles_ui(void)
{
    /* 按 z 从小到大创建，保证高层在视觉上方 */
    for (int z = 0; z < M3_LAYERS; z++) {
        for (int i = 0; i < M3_TOTAL_TILES; i++) {
            if (gs.tiles[i].z != z) continue;

            lv_obj_t *img = lv_img_create(game_container);
            int pool_idx = gs.types_used[gs.tiles[i].type];
            lv_img_set_src(img, game_tile_pool[pool_idx]);
            lv_obj_set_pos(img, gs.tiles[i].x, gs.tiles[i].y);
            lv_obj_add_flag(img, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_add_event_cb(img, tile_click_cb, LV_EVENT_CLICKED,
                                (void *)(intptr_t)i);

            /* 边框效果区分层次 */
            lv_obj_set_style_outline_width(img, 2, 0);
            lv_obj_set_style_outline_color(img, lv_color_hex(0xCCCCCC), 0);
            lv_obj_set_style_outline_pad(img, 1, 0);

            gs.tiles[i].obj = img;
        }
    }
}

static void create_tray_ui(void)
{
    tray_container = lv_obj_create(page_parent);
    lv_obj_set_size(tray_container, 238, TRAY_H);
    lv_obj_align(tray_container, LV_ALIGN_TOP_LEFT, 1, TRAY_Y);
    lv_obj_set_style_bg_color(tray_container, lv_color_hex(0x161B22), 0);
    lv_obj_set_style_bg_opa(tray_container, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(tray_container, lv_color_hex(0x30363D), 0);
    lv_obj_set_style_border_width(tray_container, 2, 0);
    lv_obj_set_style_radius(tray_container, 8, 0);
    lv_obj_set_style_pad_all(tray_container, 2, 0);
    lv_obj_clear_flag(tray_container, LV_OBJ_FLAG_SCROLLABLE);

    int slot_total_w = M3_TRAY_SIZE * (TRAY_SLOT_W + TRAY_SLOT_GAP) - TRAY_SLOT_GAP;
    int start_x = (234 - slot_total_w) / 2;

    for (int i = 0; i < M3_TRAY_SIZE; i++) {
        /* 槽位背景 */
        tray_slots[i] = lv_obj_create(tray_container);
        lv_obj_set_size(tray_slots[i], TRAY_SLOT_W, TRAY_H - 8);
        lv_obj_set_pos(tray_slots[i], start_x + i * (TRAY_SLOT_W + TRAY_SLOT_GAP), 0);
        lv_obj_set_style_bg_color(tray_slots[i], lv_color_hex(0x0D1117), 0);
        lv_obj_set_style_bg_opa(tray_slots[i], LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(tray_slots[i], 0, 0);
        lv_obj_set_style_radius(tray_slots[i], 4, 0);
        lv_obj_set_style_pad_all(tray_slots[i], 0, 0);
        lv_obj_clear_flag(tray_slots[i], LV_OBJ_FLAG_SCROLLABLE);

        /* 槽位图片 */
        tray_imgs[i] = lv_img_create(tray_slots[i]);
        lv_obj_center(tray_imgs[i]);
        lv_obj_add_flag(tray_imgs[i], LV_OBJ_FLAG_HIDDEN);
    }
}

/* ═══════════════════════════════════════════════════
 * 公共 API
 * ═══════════════════════════════════════════════════ */
void match3_reset(void)
{
    /* 清理旧 UI */
    if (game_container) {
        lv_obj_del(game_container);
        game_container = NULL;
    }
    if (tray_container) {
        lv_obj_del(tray_container);
        tray_container = NULL;
    }

    /* 清零 tile obj 指针 */
    for (int i = 0; i < M3_TOTAL_TILES; i++) {
        gs.tiles[i].obj = NULL;
    }

    /* 生成新关卡 */
    generate_level();

    /* 重建游戏区容器 */
    game_container = lv_obj_create(page_parent);
    lv_obj_set_size(game_container, GAME_AREA_W, GAME_AREA_H);
    lv_obj_align(game_container, LV_ALIGN_TOP_LEFT, GAME_AREA_X, GAME_AREA_Y);
    lv_obj_set_style_bg_color(game_container, lv_color_hex(0x0D1117), 0);
    lv_obj_set_style_bg_opa(game_container, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(game_container, 0, 0);
    lv_obj_set_style_radius(game_container, 0, 0);
    lv_obj_set_style_pad_all(game_container, 0, 0);
    lv_obj_clear_flag(game_container, LV_OBJ_FLAG_SCROLLABLE);

    /* 创建方块 UI */
    create_game_tiles_ui();

    /* 创建槽位 UI */
    create_tray_ui();

    /* 隐藏状态标签 */
    if (lbl_status) lv_obj_add_flag(lbl_status, LV_OBJ_FLAG_HIDDEN);

    /* 刷新视觉 */
    update_tile_visuals();
    update_tray_ui();

    ESP_LOGI(TAG, "Game reset, %d tiles, %d types", M3_TOTAL_TILES, M3_TYPES_PER_GAME);
}

void build_page_match3(lv_obj_t *parent)
{
    page_parent = parent;

    /* 深色背景 */
    lv_obj_set_style_bg_color(parent, lv_color_hex(0x1C233B), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

    /* 顶部标题栏 */
    lv_obj_t *lbl_title = lv_label_create(parent);
    lv_label_set_text(lbl_title, "NI Match");
    lv_obj_set_style_text_color(lbl_title, lv_color_hex(0xF1C40F), 0);
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_title, LV_ALIGN_TOP_LEFT, 8, 5);

    /* Reset 按钮 */
    btn_reset = lv_btn_create(parent);
    lv_obj_set_size(btn_reset, 50, 22);
    lv_obj_align(btn_reset, LV_ALIGN_TOP_RIGHT, -5, 3);
    lv_obj_set_style_bg_color(btn_reset, lv_color_hex(0x30363D), 0);
    lv_obj_set_style_radius(btn_reset, 4, 0);
    lv_obj_add_event_cb(btn_reset, reset_click_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_btn = lv_label_create(btn_reset);
    lv_label_set_text(lbl_btn, LV_SYMBOL_REFRESH);
    lv_obj_set_style_text_color(lbl_btn, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(lbl_btn);

    /* 状态文字 (WIN/LOSE) */
    lbl_status = lv_label_create(parent);
    lv_label_set_text(lbl_status, "");
    lv_obj_set_style_text_color(lbl_status, lv_color_hex(0xF1C40F), 0);
    lv_obj_set_style_text_font(lbl_status, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_status, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(lbl_status, LV_OBJ_FLAG_HIDDEN);

    /* 初始化游戏 */
    match3_reset();
}
