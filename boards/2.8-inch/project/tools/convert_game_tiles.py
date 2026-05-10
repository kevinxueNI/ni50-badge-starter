"""
Convert game tile images to LVGL C arrays (40x40, TRUE_COLOR_ALPHA).
Usage: python convert_game_tiles.py
"""
import os
from pathlib import Path
from PIL import Image

PROJECT_ROOT = Path(__file__).resolve().parents[1]
WORKSPACE_ROOT = PROJECT_ROOT.parents[4]
SRC_DIR = WORKSPACE_ROOT / "raw" / "esp32-badge" / "source-materials" / "恩了个恩游戏图片"
OUT_DIR = PROJECT_ROOT / "main" / "LVGL_UI" / "game_tiles"
TILE_SIZE = 46

# All images to convert (mapped to C-friendly names)
IMAGE_MAP = {
    "1.png": "pcie_6357",
    "2.png": "pxie_4322",
    "3.png": "ni_logo_dark",
    "4.png": "pxi_4110",
    "5.png": "pxie_chassis",
    "6.png": "crio_9045",
    "7.png": "pxi_2567",
    "8.png": "usb_6343",
    "9.png": "pxi_4461",
    "10.png": "scope_5105",
    "11.png": "cdaq_9171",
    "12.png": "pxie_5622",
    "13.png": "pxi_system",
    "14.png": "compactrio",
    "15.png": "ni_switch",
    "DIAdem.png": "diadem",
    "Flexlogger.png": "flexlogger",
    "LabVIEW.png": "labview",
    "NI_50_Badge-02.png": "ni50_badge",
    "SystemLink.png": "systemlink",
    "TestStand.png": "teststand",
    "VeriStand.png": "veristand",
}


def center_crop_resize(img, size):
    """Resize image: fit shorter side to size, center crop to square, composite on white."""
    w, h = img.size
    # Make square by cropping
    if w > h:
        left = (w - h) // 2
        img = img.crop((left, 0, left + h, h))
    elif h > w:
        top = (h - w) // 2
        img = img.crop((0, top, w, top + w))
    # Resize to target
    img = img.resize((size, size), Image.LANCZOS)
    # Composite onto white background (eliminate transparency issues)
    bg = Image.new("RGBA", (size, size), (255, 255, 255, 255))
    img = img.convert("RGBA")
    bg.paste(img, (0, 0), img)
    return bg


def img_to_c_array(img, var_name):
    """Convert RGBA image to LVGL TRUE_COLOR_ALPHA C array (RGB565 + A8)."""
    img = img.convert("RGBA")
    pixels = list(img.getdata())
    data = []
    for r, g, b, a in pixels:
        # RGB565: RRRRR GGGGGG BBBBB (little-endian)
        rgb565 = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)
        lo = rgb565 & 0xFF
        hi = (rgb565 >> 8) & 0xFF
        data.extend([lo, hi, a])

    # Generate C file
    lines = []
    lines.append('#include "lvgl.h"\n')
    lines.append("#ifndef LV_ATTRIBUTE_MEM_ALIGN")
    lines.append("#define LV_ATTRIBUTE_MEM_ALIGN")
    lines.append("#endif\n")
    lines.append(
        f"static LV_ATTRIBUTE_MEM_ALIGN const uint8_t img_tile_{var_name}_map[] = {{"
    )

    # Write data in rows of 32 bytes
    for i in range(0, len(data), 32):
        chunk = data[i : i + 32]
        hex_str = ", ".join(f"0x{b:02x}" for b in chunk)
        lines.append(f"  {hex_str},")

    lines.append("};\n")
    lines.append(f"const lv_img_dsc_t img_tile_{var_name} = {{")
    lines.append("  .header.always_zero = 0,")
    lines.append(f"  .header.w = {TILE_SIZE},")
    lines.append(f"  .header.h = {TILE_SIZE},")
    lines.append(f"  .data_size = {len(data)},")
    lines.append("  .header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA,")
    lines.append(f"  .data = img_tile_{var_name}_map,")
    lines.append("};\n")

    return "\n".join(lines)


def generate_header(names):
    """Generate the header file with extern declarations and pool array."""
    lines = []
    lines.append("#pragma once")
    lines.append('#include "lvgl.h"\n')
    lines.append(f"#define GAME_TILE_COUNT {len(names)}\n")

    for name in names:
        lines.append(f"extern const lv_img_dsc_t img_tile_{name};")

    lines.append("")
    lines.append(
        "static const lv_img_dsc_t *game_tile_pool[GAME_TILE_COUNT] = {"
    )
    for name in names:
        lines.append(f"    &img_tile_{name},")
    lines.append("};\n")

    return "\n".join(lines)


def main():
    os.makedirs(OUT_DIR, exist_ok=True)

    names = []
    for filename, var_name in IMAGE_MAP.items():
        src_path = SRC_DIR / filename
        if not os.path.exists(src_path):
            print(f"WARNING: {src_path} not found, skipping")
            continue

        img = Image.open(src_path)
        img = center_crop_resize(img, TILE_SIZE)
        c_code = img_to_c_array(img, var_name)

        out_path = OUT_DIR / f"img_tile_{var_name}.c"
        with open(out_path, "w", encoding="utf-8") as f:
            f.write(c_code)
        print(f"  OK: {filename} -> img_tile_{var_name}.c ({TILE_SIZE}x{TILE_SIZE})")
        names.append(var_name)

    # Generate header
    header = generate_header(names)
    header_path = OUT_DIR / "game_tiles.h"
    with open(header_path, "w", encoding="utf-8") as f:
        f.write(header)
    print(f"\n  Header: {header_path}")
    print(f"  Total: {len(names)} tiles converted")


if __name__ == "__main__":
    main()
