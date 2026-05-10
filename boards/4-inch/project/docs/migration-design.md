# P3 迁移设计包：Badge → LCD-4 (480×480)

> 从 2.8 寸 (240×320) 迁移到 4 寸 (480×480) 的信息架构、资源管线和组件边界。

---

## 1. 页面信息架构 (480×480)

### 1.1 页面清单与导航

| # | 页面 | 内容 | 导航方式 | 优先级 |
|---|------|------|---------|--------|
| 0 | Card (名片) | 头像、姓名、部门、Logo | Swipe → | 必做 |
| 1 | Milestones (里程碑) | NI 50 / LV 40 时间线 | Swipe ← → | 必做 |
| 2 | Gallery (图库) | NI 精选图片轮播 | Swipe ← → | 必做 |
| 3 | Calendar (日历) | 当月日历 + 纪念日标记 | Swipe ← → | 可选 |
| 4 | Game (游戏) | 翻翻乐/2048/恩了个恩 | Swipe ← | 可选 |
| 5 | Config (配置) | WiFi/亮度/电池/关于 | 长按 Home | 可选 |

导航模型：**水平 Tileview**（同 2.8 寸方案），底部圆点指示当前页。

### 1.2 Card 页面布局 (480×480)

```
┌──────────────────────────────────────────────────┐
│ ▸ 状态栏 (32px): 时间 | WiFi | 电池              │
├──────────────────────────────────────────────────┤
│                                                  │
│             ┌────────────┐                       │
│             │  头像 160px │                       │
│             │  (圆形裁切) │                       │
│             └────────────┘                       │
│                                                  │
│          Kevin Xue  (28px bold)                  │
│    NI China Innovation Center (20px)             │
│                                                  │
│         ┌──────────────────────┐                 │
│         │   NI 50 + LV 40 Logo │                 │
│         │   (200×80 image)     │                 │
│         └──────────────────────┘                 │
│                                                  │
│              ● ○ ○ ○ ○  (page dots)              │
└──────────────────────────────────────────────────┘
```

### 1.3 Milestones 页面布局

```
┌──────────────────────────────────────────────────┐
│ [←] Milestones                      状态栏 (32px)│
├──────────────────────────────────────────────────┤
│                                                  │
│  ┌─────┐  1976  NI 成立于 Austin, TX            │
│  │icon │  ─────────────────────────              │
│  └─────┘  1986  LabVIEW 1.0 发布                │
│           ─────────────────────────              │
│           1996  PXI 标准诞生                     │
│           ─────────────────────────              │
│           2006  CompactRIO                       │
│           ─────────────────────────              │
│           2016  NI → 软件定义                    │
│           ─────────────────────────              │
│           2026  NI 50 周年 🎉                    │
│                                                  │
│              ○ ● ○ ○ ○  (page dots)              │
└──────────────────────────────────────────────────┘
```

### 1.4 Gallery 页面布局

```
┌──────────────────────────────────────────────────┐
│ [←] Gallery                         状态栏 (32px)│
├──────────────────────────────────────────────────┤
│                                                  │
│  ┌──────────────────────────────────────────┐    │
│  │                                          │    │
│  │        全屏图片 (440×360)                │    │
│  │        JPEG from SPIFFS                  │    │
│  │                                          │    │
│  └──────────────────────────────────────────┘    │
│                                                  │
│        < 图片标题 (居中, 16px) >                  │
│             1 / 12                               │
│                                                  │
│              ○ ○ ● ○ ○  (page dots)              │
└──────────────────────────────────────────────────┘
```

---

## 2. 资源目录结构

```
D:\NI\98 AI\AI_Workspace\Kevin_wiki\dev\esp32-badge\boards\4-inch\project\
├── main/
│   ├── main.c
│   ├── CMakeLists.txt
│   ├── idf_component.yml
│   ├── ui/                          # UI 逻辑代码
│   │   ├── ui.h                     # 公共入口
│   │   ├── page_card.c              # Card 页面
│   │   ├── page_milestones.c        # 里程碑页面
│   │   ├── page_gallery.c           # 图库页面
│   │   ├── page_calendar.c          # 日历页面 (可选)
│   │   ├── page_game.c              # 游戏页面 (可选)
│   │   ├── page_config.c            # 配置页面 (可选)
│   │   ├── widget_statusbar.c       # 通用状态栏组件
│   │   ├── widget_page_dots.c       # 通用页面指示器
│   │   └── widget_nav.c             # 通用 Home/Back 按钮
│   ├── assets/                      # 编译期嵌入资源 (C arrays)
│   │   ├── img_avatar_160.c         # 头像 160×160 RGB565
│   │   ├── img_ni5040_logo.c        # NI50+LV40 Logo 200×80
│   │   ├── img_milestone_icons.c    # 时间线小图标
│   │   └── fonts/
│   │       ├── font_montserrat_20.c # 英文 20px
│   │       ├── font_montserrat_28.c # 英文 28px (标题)
│   │       └── font_cjk_16.c        # 中文 16px (可选)
│   └── Kconfig.projbuild           # 页面开关 (CONFIG_PAGE_xxx)
├── spiffs/                          # SPIFFS 分区内容 (运行时读取)
│   ├── gallery/
│   │   ├── 01_pxi.jpg              # 440×360, quality=80
│   │   ├── 02_labview.jpg
│   │   ├── ...
│   │   └── manifest.json           # {filename, title, order}
│   └── config.json                 # 用户配置持久化
├── tools/
│   ├── img2lvgl.py                  # PNG/WebP → LVGL C array 转换脚本
│   ├── spiffs_pack.py               # 打包 spiffs/ 目录为 .bin
│   └── font_conv.sh                 # LVGL font converter wrapper
├── patches/
│   └── apply_patches.ps1            # BSP 兼容性补丁 (已有)
├── docs/
│   ├── bring-up-checklist.md        # P2 准备包 (已有)
│   └── migration-design.md          # 本文件
└── ...
```

### 资源策略

| 资源类型 | 存储方式 | 理由 |
|---------|---------|------|
| 头像 (160×160) | C array (编译嵌入) | 开机即用，无 IO 延迟 |
| Logo (200×80) | C array | 同上 |
| 里程碑图标 (32×32 × N) | C array | 小且页面必用 |
| Gallery 图片 (440×360) | SPIFFS JPEG | 数量多、可更换、节省 app 分区 |
| 字体 (英文) | LVGL 内建 Montserrat | 已在 sdkconfig 配置 |
| 字体 (中文) | 自定义 C array 或不用 | Montserrat 无 CJK，暂用英文 |
| 用户配置 | SPIFFS JSON | 可读写 |

### 内存预算 (估算)

| 项目 | 占用 |
|------|------|
| LVGL draw buffer (480×40×2 bytes × 2) | ~75 KB |
| LVGL heap (lv_mem) | 48 KB (sdkconfig) |
| 头像 C array (160×160×2) | ~50 KB |
| Logo C array (200×80×2) | ~31 KB |
| 里程碑图标 (~6 × 32×32×2) | ~12 KB |
| JPEG 解码临时 buffer | ~20 KB |
| App 逻辑 + stack | ~40 KB |
| **合计 (Internal DRAM)** | **~276 KB** |
| ESP32-S3 可用 Internal DRAM | ~370 KB |
| **余量** | **~94 KB ✓** |

> Gallery JPEG 直接解码到 PSRAM frame buffer line-by-line，不占 internal。

---

## 3. 组件拆分方案

### 3.1 通用 Widget 组件

这些组件在多个页面复用，独立文件：

| 组件 | 文件 | 功能 | API |
|------|------|------|-----|
| StatusBar | `widget_statusbar.c` | 时间+WiFi+电池 (32px 高) | `statusbar_create(parent)` |
| PageDots | `widget_page_dots.c` | 底部页面指示圆点 | `pagedots_create(parent, count)` / `pagedots_set_active(idx)` |
| NavButton | `widget_nav.c` | Home/Back 按钮 | `nav_home_create(parent, cb)` |

### 3.2 页面组件接口

每个 page_xxx.c 统一实现：

```c
// page_card.h
lv_obj_t *page_card_create(lv_obj_t *parent);
void page_card_on_enter(void);   // 页面可见时调用
void page_card_on_leave(void);   // 页面离开时调用
```

Tileview 的 `LV_EVENT_VALUE_CHANGED` 回调统一调度 `on_enter` / `on_leave`，控制动画启停和资源加载释放。

### 3.3 从 2.8 寸代码可直接复用的

| 模块 | 2.8寸路径 | 能否直接用 | 改动点 |
|------|-----------|-----------|--------|
| 状态栏逻辑 (RTC/电池读取) | Badge_UI.c | ✅ 可复用 | GPIO/I2C 地址可能不同 |
| Tileview 框架 | Badge_UI.c | ✅ 结构可复用 | 分辨率常量改为 480 |
| 日历算法 | page_calendar.c | ✅ 纯逻辑 | 无改动 |
| NI History 数据 | NI History.md | ✅ 内容复用 | 布局从竖向列表改为时间线 |
| 游戏逻辑 | 恩了个恩/翻翻乐 | ⚠️ 部分复用 | 格子尺寸、图片尺寸全部重算 |
| 头像/Logo 图片 | C array | ❌ 重做 | 尺寸从 80→160, 120→200 |
| 背景/布局坐标 | Badge_UI.c | ❌ 重做 | 240×320 → 480×480 全部重算 |

### 3.4 BSP 差异对照

| 项目 | 2.8 寸 (ST7789) | 4 寸 (ST7701) |
|------|-----------------|---------------|
| 接口 | SPI | RGB (16-bit parallel) |
| 分辨率 | 240×320 | 480×480 |
| 触摸 IC | CST328 | GT911 |
| I2C 地址 | 0x1A | 0x5D (or 0x14) |
| PSRAM 需求 | 可选 | **必须** (frame buffer) |
| BSP 组件 | waveshare/esp32_s3_touch_lcd_2_8 | waveshare/esp32_s3_touch_lcd_4 |
| 额外依赖 | — | esp_driver_sdmmc (SD card) |
| IO Expander | 无 | CH32V003 (板载 MCU) |
| 背光控制 | GPIO PWM | LEDC (via BSP) |
| Draw buffer | 单 buffer 可用 | 双 buffer + PSRAM DMA |

### 3.5 性能约束 (LCD-4 特有)

基于 2.8 寸开发经验 + RGB 接口特性：

1. **禁止全屏重绘** — RGB 面板的 frame buffer 在 PSRAM，全屏刷新 = 480×480×2 = 450KB DMA，帧率会降。用 `lv_obj_invalidate()` 精准刷新。
2. **禁止 shadow** — `lv_obj_set_style_shadow_width(obj, 0, 0)` 全局关闭。
3. **避免透明度叠加** — `opa < 255` 的 container 会触发额外 blend，在大尺寸面板上代价更高。
4. **JPEG 解码走 PSRAM** — 不要 malloc internal DRAM 做解码 buffer，用 `heap_caps_malloc(size, MALLOC_CAP_SPIRAM)`。
5. **动画帧率控制** — `lv_anim_set_time()` ≥ 300ms，避免高频小面积重绘累积成全屏级负载。
6. **字体只保留必要字号** — 每多一个 Montserrat 字号 ≈ 20-40KB flash。

---

## 4. 迁移执行顺序 (建议)

| Step | 动作 | 依赖 |
|------|------|------|
| 1 | P2 bring-up 通过 (屏幕亮+触摸OK) | 板子到位 |
| 2 | 创建 `main/ui/` 目录结构 | Step 1 |
| 3 | 实现 widget_statusbar + widget_page_dots | Step 2 |
| 4 | 实现 page_card (头像/名字/Logo) | Step 3 + 资源转换 |
| 5 | 验证 Card 页面在实机上的显示效果 | Step 4 |
| 6 | 实现 page_milestones | Step 5 |
| 7 | 实现 page_gallery (SPIFFS + JPEG) | Step 5 + spiffs 打包 |
| 8 | 可选页面 (Calendar/Game/Config) | Step 5 |
| 9 | 整体联调 + 性能优化 | Step 6-8 |
| 10 | 最终 release build + 版本标签 | Step 9 |

---

## 5. 资源转换命令 (备忘)

### PNG → LVGL C array

```bash
# 使用 LVGL image converter (online or CLI)
# Output: CF_TRUE_COLOR (RGB565), no alpha
python tools/img2lvgl.py --input avatar.png --output main/assets/img_avatar_160.c \
  --format RGB565 --width 160 --height 160
```

### SPIFFS 打包

```powershell
# ESP-IDF 内建工具
python -m esptool --chip esp32s3 -p COM3 write_flash 0x810000 build/storage.bin

# 或使用 idf.py
idf.py -p COM3 storage-flash
```

### 字体转换

```bash
# LVGL font converter (需 Node.js)
npx lv_font_conv --bpp 4 --size 20 --font Montserrat-Medium.ttf \
  -r 0x20-0x7F --format lvgl -o font_montserrat_20.c --no-compress
```
