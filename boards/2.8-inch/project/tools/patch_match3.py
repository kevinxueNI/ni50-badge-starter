"""
Patch Badge_UI.c to add page 7 (match3 game).
"""
import re
from pathlib import Path

FILE = Path(__file__).resolve().parents[1] / "main" / "LVGL_UI" / "Badge_UI.c"

with open(FILE, "r", encoding="utf-8") as f:
    src = f.read()

# 1. Add #include "game_match3.h" after #include "QMI8658.h"
src = src.replace(
    '#include "QMI8658.h"',
    '#include "QMI8658.h"\n#include "game_match3.h"'
)

# 2. Add tile_match3 declaration after tile_player
src = src.replace(
    'static lv_obj_t *tile_player    = NULL;  /* Page 5',
    'static lv_obj_t *tile_player    = NULL;  /* Page 5 — Image Player */\n'
    'static lv_obj_t *tile_match3    = NULL;  /* Page 6 — 恩了个恩 */'
)
# Remove the duplicate comment
src = src.replace(
    'Page 5 — Image Player */\nstatic lv_obj_t *tile_match3    = NULL;  /* Page 6 — 恩了个恩 */\n — Image Player */',
    'Page 5 — Image Player */\nstatic lv_obj_t *tile_match3    = NULL;  /* Page 6 — 恩了个恩 */'
)

# 3. Change NUM_PAGES from 6 to 7
src = src.replace('#define NUM_PAGES 6', '#define NUM_PAGES 7')

# 4. Add tile_match3 to tileview_event_cb
src = src.replace(
    "else if (tile == tile_player)  update_indicators(5);\n}",
    "else if (tile == tile_player)  update_indicators(5);\n"
    "    else if (tile == tile_match3)  update_indicators(6);\n}"
)

# 5. Change tile_player from LV_DIR_LEFT to LV_DIR_LEFT | LV_DIR_RIGHT
# and add tile_match3
src = src.replace(
    'tile_player    = lv_tileview_add_tile(tileview, 5, 0, LV_DIR_LEFT);',
    'tile_player    = lv_tileview_add_tile(tileview, 5, 0, LV_DIR_LEFT | LV_DIR_RIGHT);\n'
    '    tile_match3    = lv_tileview_add_tile(tileview, 6, 0, LV_DIR_LEFT);'
)

# 6. Add build_page_match3 call after build_page_player
src = src.replace(
    'build_page_player(tile_player);',
    'build_page_player(tile_player);\n    build_page_match3(tile_match3);'
)

# 7. Update log message
src = src.replace(
    'ESP_LOGI(TAG, "Badge UI ready — 6 tiles created");',
    'ESP_LOGI(TAG, "Badge UI ready — 7 tiles created");'
)

# 8. Update tileview comment
src = src.replace(
    '/* 创建 tileview（横向六页） */',
    '/* 创建 tileview（横向七页） */'
)
src = src.replace(
    '/* 六个 tile: 横向排列，只允许水平滑动 */',
    '/* 七个 tile: 横向排列，只允许水平滑动 */'
)

with open(FILE, "w", encoding="utf-8") as f:
    f.write(src)

print("Badge_UI.c patched successfully for page 7 (match3)")
