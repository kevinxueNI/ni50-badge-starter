"""
Convert gallery images to LVGL C arrays (RGB565, Little Endian).
Target display area: 200x120 (fit within, maintain aspect ratio, white-pad).

Usage:
    python convert_gallery_images.py

Output:
    main/LVGL_UI/gallery/ folder with img_nilogo_*.c and img_lv1_*.c files
    main/LVGL_UI/gallery/gallery_images.h header with externs and arrays
"""

import os
import struct
from PIL import Image

# Configuration
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(SCRIPT_DIR)
OUTPUT_DIR = os.path.join(PROJECT_ROOT, "main", "LVGL_UI", "gallery")

# Image target sizes
NILOGO_W = 200
NILOGO_H = 120
LV1_W = 200
LV1_H = 134

# Source paths
NILOGO_SRC = r"D:\NI\98 AI\AI_Workspace\Kevin_wiki\raw\电子名片项目\NI Logo"
LV1_SRC = r"D:\NI\98 AI\AI_Workspace\Kevin_wiki\raw\电子名片项目\LabVIEW 1.0"

# Background color for padding (dark theme)
BG_COLOR = (13, 17, 23)  # 0x0D1117


def rgb888_to_rgb565(r, g, b):
    """Convert 8-bit RGB to 16-bit RGB565 (little endian bytes)."""
    rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3)
    return struct.pack('<H', rgb565)


def resize_fit(img, target_w, target_h, bg_color=BG_COLOR):
    """Resize image to fit within target, maintain aspect ratio, pad with bg.
    For images with alpha channel, composite onto white first."""
    if img.mode in ('RGBA', 'LA', 'PA'):
        # Composite alpha onto white background before converting
        white_bg = Image.new('RGB', img.size, (255, 255, 255))
        if img.mode != 'RGBA':
            img = img.convert('RGBA')
        white_bg.paste(img, mask=img.split()[3])
        img = white_bg
    else:
        img = img.convert('RGB')
    w, h = img.size
    scale = min(target_w / w, target_h / h)
    new_w = int(w * scale)
    new_h = int(h * scale)
    img = img.resize((new_w, new_h), Image.LANCZOS)

    # Create padded canvas
    canvas = Image.new('RGB', (target_w, target_h), bg_color)
    x_off = (target_w - new_w) // 2
    y_off = (target_h - new_h) // 2
    canvas.paste(img, (x_off, y_off))
    return canvas


def image_to_c_array(img, var_name, target_w, target_h):
    """Convert PIL Image to LVGL C source file content."""
    img = resize_fit(img, target_w, target_h)

    # Convert to RGB565 bytes
    pixels = []
    for y in range(target_h):
        for x in range(target_w):
            r, g, b = img.getpixel((x, y))
            pixels.append(rgb888_to_rgb565(r, g, b))

    raw_bytes = b''.join(pixels)

    # Generate C file
    lines = []
    lines.append('#include "lvgl.h"\n')
    lines.append(f'/* Auto-generated from convert_gallery_images.py */\n')
    lines.append(f'/* Size: {target_w}x{target_h}, Format: RGB565, {len(raw_bytes)} bytes */\n\n')
    lines.append(f'static const uint8_t {var_name}_data[] = {{\n')

    # Write data in rows of 16 bytes
    for i in range(0, len(raw_bytes), 16):
        chunk = raw_bytes[i:i+16]
        hex_str = ', '.join(f'0x{b:02X}' for b in chunk)
        lines.append(f'    {hex_str},\n')

    lines.append('};\n\n')
    lines.append(f'const lv_img_dsc_t {var_name} = {{\n')
    lines.append(f'    .header.always_zero = 0,\n')
    lines.append(f'    .header.w = {target_w},\n')
    lines.append(f'    .header.h = {target_h},\n')
    lines.append(f'    .header.cf = LV_IMG_CF_TRUE_COLOR,\n')
    lines.append(f'    .data_size = {len(raw_bytes)},\n')
    lines.append(f'    .data = {var_name}_data,\n')
    lines.append('};\n')

    return ''.join(lines)


def main():
    os.makedirs(OUTPUT_DIR, exist_ok=True)

    print("Converting NI Logo images...")
    for i in range(1, 8):
        src_path = os.path.join(NILOGO_SRC, f"{i}.png")
        var_name = f"img_nilogo_{i}"
        img = Image.open(src_path)
        c_content = image_to_c_array(img, var_name, NILOGO_W, NILOGO_H)
        out_path = os.path.join(OUTPUT_DIR, f"img_nilogo_{i}.c")
        with open(out_path, 'w') as f:
            f.write(c_content)
        print(f"  {i}.png -> {var_name} ({NILOGO_W}x{NILOGO_H})")

    print("Converting LabVIEW 1.0 images...")
    lv1_files = ["LV1BlockDiagram.jpg", "LV1SplashScreen.jpg", "news.png"]
    lv1_names = ["img_lv1_block_diagram", "img_lv1_splash", "img_lv1_news"]
    for fname, var_name in zip(lv1_files, lv1_names):
        src_path = os.path.join(LV1_SRC, fname)
        img = Image.open(src_path)
        c_content = image_to_c_array(img, var_name, LV1_W, LV1_H)
        out_path = os.path.join(OUTPUT_DIR, f"{var_name}.c")
        with open(out_path, 'w') as f:
            f.write(c_content)
        print(f"  {fname} -> {var_name} ({LV1_W}x{LV1_H})")

    # Generate header file
    print("Generating gallery_images.h...")
    header = []
    header.append('#pragma once\n')
    header.append('#include "lvgl.h"\n\n')
    header.append('/* NI Logo images (200x120 RGB565) */\n')
    header.append('#define NILOGO_IMG_W  200\n')
    header.append('#define NILOGO_IMG_H  120\n')
    header.append('#define NILOGO_COUNT  7\n\n')
    for i in range(1, 8):
        header.append(f'extern const lv_img_dsc_t img_nilogo_{i};\n')
    header.append('\n/* LabVIEW 1.0 images (200x134 RGB565) */\n')
    header.append('#define LV1_IMG_W  200\n')
    header.append('#define LV1_IMG_H  134\n')
    header.append('#define LV1_COUNT  3\n\n')
    for name in lv1_names:
        header.append(f'extern const lv_img_dsc_t {name};\n')
    header.append('\n/* Image pool arrays for gallery code */\n')
    header.append('static const lv_img_dsc_t * const gallery_nilogo_pool[] = {\n')
    for i in range(1, 8):
        header.append(f'    &img_nilogo_{i},\n')
    header.append('};\n\n')
    header.append('static const lv_img_dsc_t * const gallery_lv1_pool[] = {\n')
    for name in lv1_names:
        header.append(f'    &{name},\n')
    header.append('};\n')

    header_path = os.path.join(OUTPUT_DIR, "gallery_images.h")
    with open(header_path, 'w') as f:
        f.write(''.join(header))

    print(f"\nDone! Files written to: {OUTPUT_DIR}")
    total_bytes = NILOGO_W * NILOGO_H * 2 * 7 + LV1_W * LV1_H * 2 * 3
    print(f"Total image data: {total_bytes:,} bytes ({total_bytes/1024:.1f} KB)")


if __name__ == '__main__':
    main()
