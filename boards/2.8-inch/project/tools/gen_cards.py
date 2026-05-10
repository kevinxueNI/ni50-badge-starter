"""批量转换翻翻乐卡片图片为 40x40 LVGL C 数组"""
import numpy as np
from PIL import Image
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parents[1]
WORKSPACE_ROOT = PROJECT_ROOT.parents[4]
SRC_DIR = WORKSPACE_ROOT / "raw" / "esp32-badge" / "source-materials" / "翻翻乐游戏图片"
OUT_DIR = PROJECT_ROOT / "main" / "LVGL_UI" / "cards"
OUT_DIR.mkdir(exist_ok=True)

SIZE = 40  # 40x40 pixels

files = sorted(SRC_DIR.glob("*.png"))
print(f"Found {len(files)} images")

manifest = []  # (var_name, filename)

for f in files:
    img = Image.open(f).convert("RGBA")
    # Resize to 40x40 with aspect-fill (crop center)
    w, h = img.size
    scale = max(SIZE / w, SIZE / h)
    new_w, new_h = int(w * scale), int(h * scale)
    img = img.resize((new_w, new_h), Image.LANCZOS)
    left = (new_w - SIZE) // 2
    top = (new_h - SIZE) // 2
    img = img.crop((left, top, left + SIZE, top + SIZE))
    
    arr = np.array(img, dtype=np.uint8)
    r, g, b, a = arr[:,:,0], arr[:,:,1], arr[:,:,2], arr[:,:,3]
    c565 = ((r.astype(np.uint16) >> 3) << 11) | ((g.astype(np.uint16) >> 2) << 5) | (b.astype(np.uint16) >> 3)
    lo = (c565 & 0xFF).astype(np.uint8)
    hi = ((c565 >> 8) & 0xFF).astype(np.uint8)
    # Interleave: lo, hi, alpha
    data = np.stack([lo, hi, a], axis=-1).flatten()
    
    # Variable name from filename
    stem = f.stem.lower().replace(" ", "_").replace("-", "_")
    var_name = f"img_card_{stem}"
    out_path = OUT_DIR / f"{var_name}.c"
    
    lines = []
    lines.append('#include "lvgl.h"\n')
    lines.append('#ifndef LV_ATTRIBUTE_MEM_ALIGN\n#define LV_ATTRIBUTE_MEM_ALIGN\n#endif\n')
    lines.append(f'static LV_ATTRIBUTE_MEM_ALIGN const uint8_t {var_name}_map[] = {{')
    for i in range(0, len(data), 32):
        chunk = data[i:i+32]
        lines.append('  ' + ', '.join(f'0x{b:02x}' for b in chunk) + ',')
    lines.append('};\n')
    lines.append(f'const lv_img_dsc_t {var_name} = {{')
    lines.append(f'  .header.always_zero = 0,')
    lines.append(f'  .header.w = {SIZE},')
    lines.append(f'  .header.h = {SIZE},')
    lines.append(f'  .data_size = {len(data)},')
    lines.append(f'  .header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA,')
    lines.append(f'  .data = {var_name}_map,')
    lines.append('};')
    
    with open(out_path, 'w') as fout:
        fout.write('\n'.join(lines))
    
    manifest.append((var_name, out_path.name))
    print(f"  {f.name} -> {var_name} ({len(data)} bytes)")

# Generate header with extern declarations
header_path = OUT_DIR / "card_images.h"
with open(header_path, 'w') as fh:
    fh.write('#pragma once\n#include "lvgl.h"\n\n')
    fh.write(f'#define CARD_IMAGE_COUNT {len(manifest)}\n\n')
    for var_name, _ in manifest:
        fh.write(f'extern const lv_img_dsc_t {var_name};\n')
    fh.write(f'\nstatic const lv_img_dsc_t *card_image_pool[CARD_IMAGE_COUNT] = {{\n')
    for var_name, _ in manifest:
        fh.write(f'    &{var_name},\n')
    fh.write('};\n')

print(f"\nDone! {len(manifest)} cards converted. Header: {header_path}")
