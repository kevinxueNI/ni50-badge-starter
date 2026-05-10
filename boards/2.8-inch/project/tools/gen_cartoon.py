import numpy as np
from PIL import Image

img = Image.open(r'D:\NI\98 AI\AI_Workspace\Kevin_wiki\raw\电子名片项目\卡通人物图.png').convert('RGB')
src_w, src_h = img.size
target_w, target_h = 240, 320
# crop-to-fill: fit width, crop height
new_h = int(src_w / (target_w / target_h))
top = (src_h - new_h) // 2
img = img.crop((0, top, src_w, top + new_h)).resize((target_w, target_h), Image.LANCZOS)
arr = np.array(img, dtype=np.uint8)
r, g, b = arr[:,:,0], arr[:,:,1], arr[:,:,2]
c565 = ((r.astype(np.uint16) >> 3) << 11) | ((g.astype(np.uint16) >> 2) << 5) | (b.astype(np.uint16) >> 3)
lo = (c565 & 0xFF).astype(np.uint8)
hi = ((c565 >> 8) & 0xFF).astype(np.uint8)
data = np.stack([lo, hi], axis=-1).flatten()
print(f'data len: {len(data)}')

lines = []
lines.append('#include "lvgl.h"\n')
lines.append('#ifndef LV_ATTRIBUTE_MEM_ALIGN\n#define LV_ATTRIBUTE_MEM_ALIGN\n#endif\n')
lines.append('static LV_ATTRIBUTE_MEM_ALIGN const uint8_t img_cartoon_map[] = {')
for i in range(0, len(data), 32):
    chunk = data[i:i+32]
    lines.append('  ' + ', '.join(f'0x{b:02x}' for b in chunk) + ',')
lines.append('};\n')
lines.append('const lv_img_dsc_t img_cartoon = {')
lines.append('  .header.always_zero = 0,')
lines.append(f'  .header.w = {target_w},')
lines.append(f'  .header.h = {target_h},')
lines.append(f'  .data_size = {len(data)},')
lines.append('  .header.cf = LV_IMG_CF_TRUE_COLOR,')
lines.append('  .data = img_cartoon_map,')
lines.append('};')

with open(r'main\LVGL_UI\img_cartoon.c', 'w') as f:
    f.write('\n'.join(lines))
print('Done: img_cartoon.c')
