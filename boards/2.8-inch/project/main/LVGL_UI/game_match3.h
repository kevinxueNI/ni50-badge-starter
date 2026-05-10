#pragma once

#include "lvgl.h"

/**
 * game_match3.h — "恩了个恩" 3D 堆叠消消乐
 *
 * 类似"羊了个羊"的三消游戏：
 * - 多层堆叠，上层遮挡下层
 * - 点击可见方块放入底部槽位
 * - 槽位中凑齐 3 个相同即消除
 * - 槽位满 7 个未消除则游戏结束
 */

/* 游戏参数 */
#define M3_TILE_W         46
#define M3_TILE_H         46
#define M3_TYPES_PER_GAME 8    /* 每局使用的图案种类数 */
#define M3_TOTAL_TILES    24   /* 每局方块总数 (8×3) */
#define M3_LAYERS         3
#define M3_TRAY_SIZE      7

/**
 * @brief 在指定父容器中构建"恩了个恩"游戏页
 * @param parent 父 LVGL 对象（tileview 的一个 tile）
 */
void build_page_match3(lv_obj_t *parent);

/**
 * @brief 重置游戏（重新发牌）
 */
void match3_reset(void);
