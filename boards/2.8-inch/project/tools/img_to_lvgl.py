"""
将图片转换为 LVGL 8.x 兼容的 C 数组文件 (RGB565, 带 Alpha 通道可选)
用法:
  python img_to_lvgl.py <input_image> <output.c> <width> <height> [--cf CF_TRUE_COLOR_ALPHA]
"""
import sys
from pathlib import Path
from PIL import Image

def rgb888_to_rgb565(r, g, b):
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)

def convert_image(input_path, output_path, width, height, cf="CF_TRUE_COLOR_ALPHA"):
    img = Image.open(input_path).convert("RGBA")
    img = img.resize((width, height), Image.LANCZOS)

    var_name = Path(output_path).stem

    use_alpha = "ALPHA" in cf

    pixels = []
    for y in range(height):
        for x in range(width):
            r, g, b, a = img.getpixel((x, y))
            c565 = rgb888_to_rgb565(r, g, b)
            lo = c565 & 0xFF
            hi = (c565 >> 8) & 0xFF
            if use_alpha:
                pixels.append((lo, hi, a))
            else:
                pixels.append((lo, hi))

    # Write C file
    with open(output_path, 'w', encoding='utf-8') as f:
        f.write('#include "lvgl.h"\n\n')

        if use_alpha:
            total_bytes = width * height * 3
            f.write(f'#ifndef LV_ATTRIBUTE_MEM_ALIGN\n#define LV_ATTRIBUTE_MEM_ALIGN\n#endif\n\n')
            f.write(f'static LV_ATTRIBUTE_MEM_ALIGN const uint8_t {var_name}_map[] = {{\n')
            for i, (lo, hi, a) in enumerate(pixels):
                if i % 12 == 0:
                    f.write('  ')
                f.write(f'0x{lo:02x}, 0x{hi:02x}, 0x{a:02x}, ')
                if (i + 1) % 12 == 0:
                    f.write('\n')
            f.write('\n};\n\n')
            f.write(f'const lv_img_dsc_t {var_name} = {{\n')
            f.write(f'  .header.always_zero = 0,\n')
            f.write(f'  .header.w = {width},\n')
            f.write(f'  .header.h = {height},\n')
            f.write(f'  .data_size = {total_bytes},\n')
            f.write(f'  .header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA,\n')
            f.write(f'  .data = {var_name}_map,\n')
            f.write(f'}};\n')
        else:
            total_bytes = width * height * 2
            f.write(f'#ifndef LV_ATTRIBUTE_MEM_ALIGN\n#define LV_ATTRIBUTE_MEM_ALIGN\n#endif\n\n')
            f.write(f'static LV_ATTRIBUTE_MEM_ALIGN const uint8_t {var_name}_map[] = {{\n')
            for i, (lo, hi) in enumerate(pixels):
                if i % 16 == 0:
                    f.write('  ')
                f.write(f'0x{lo:02x}, 0x{hi:02x}, ')
                if (i + 1) % 16 == 0:
                    f.write('\n')
            f.write('\n};\n\n')
            f.write(f'const lv_img_dsc_t {var_name} = {{\n')
            f.write(f'  .header.always_zero = 0,\n')
            f.write(f'  .header.w = {width},\n')
            f.write(f'  .header.h = {height},\n')
            f.write(f'  .data_size = {total_bytes},\n')
            f.write(f'  .header.cf = LV_IMG_CF_TRUE_COLOR,\n')
            f.write(f'  .data = {var_name}_map,\n')
            f.write(f'}};\n')

    print(f"Done: {output_path} ({width}x{height}, {total_bytes} bytes)")

if __name__ == "__main__":
    if len(sys.argv) < 5:
        print("Usage: python img_to_lvgl.py <input> <output.c> <width> <height> [--cf CF_TRUE_COLOR_ALPHA]")
        sys.exit(1)

    input_img = sys.argv[1]
    output_c = sys.argv[2]
    w = int(sys.argv[3])
    h = int(sys.argv[4])
    cf = "CF_TRUE_COLOR_ALPHA"
    if len(sys.argv) > 5 and sys.argv[5] == "--cf":
        cf = sys.argv[6]

    convert_image(input_img, output_c, w, h, cf)
