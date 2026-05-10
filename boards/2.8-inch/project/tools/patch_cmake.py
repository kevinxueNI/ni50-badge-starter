"""Patch CMakeLists.txt to add game_match3 sources."""
from pathlib import Path

FILE = Path(__file__).resolve().parents[1] / "main" / "CMakeLists.txt"

with open(FILE, "r", encoding="utf-8") as f:
    src = f.read()

# Add game source files after Badge_UI.c
new_srcs = [
    '"./LVGL_UI/game_match3.c"',
    '"./LVGL_UI/game_tiles/img_tile_pcie_6357.c"',
    '"./LVGL_UI/game_tiles/img_tile_pxie_4322.c"',
    '"./LVGL_UI/game_tiles/img_tile_ni_logo_dark.c"',
    '"./LVGL_UI/game_tiles/img_tile_pxi_4110.c"',
    '"./LVGL_UI/game_tiles/img_tile_pxie_chassis.c"',
    '"./LVGL_UI/game_tiles/img_tile_crio_9045.c"',
    '"./LVGL_UI/game_tiles/img_tile_pxi_2567.c"',
    '"./LVGL_UI/game_tiles/img_tile_usb_6343.c"',
    '"./LVGL_UI/game_tiles/img_tile_pxi_4461.c"',
    '"./LVGL_UI/game_tiles/img_tile_scope_5105.c"',
    '"./LVGL_UI/game_tiles/img_tile_cdaq_9171.c"',
    '"./LVGL_UI/game_tiles/img_tile_pxie_5622.c"',
    '"./LVGL_UI/game_tiles/img_tile_pxi_system.c"',
    '"./LVGL_UI/game_tiles/img_tile_compactrio.c"',
    '"./LVGL_UI/game_tiles/img_tile_ni_switch.c"',
    '"./LVGL_UI/game_tiles/img_tile_diadem.c"',
    '"./LVGL_UI/game_tiles/img_tile_flexlogger.c"',
    '"./LVGL_UI/game_tiles/img_tile_labview.c"',
    '"./LVGL_UI/game_tiles/img_tile_ni50_badge.c"',
    '"./LVGL_UI/game_tiles/img_tile_systemlink.c"',
    '"./LVGL_UI/game_tiles/img_tile_teststand.c"',
    '"./LVGL_UI/game_tiles/img_tile_veristand.c"',
]

indent = "                              "
new_lines = "\n".join(indent + s for s in new_srcs)

src = src.replace(
    '"./LVGL_UI/Badge_UI.c"',
    '"./LVGL_UI/Badge_UI.c"\n' + new_lines
)

# Add include dir
src = src.replace(
    '"./LVGL_UI/cards"',
    '"./LVGL_UI/cards"\n' + indent + '"./LVGL_UI/game_tiles"'
)

with open(FILE, "w", encoding="utf-8") as f:
    f.write(src)

print("CMakeLists.txt updated with 23 new source files")
