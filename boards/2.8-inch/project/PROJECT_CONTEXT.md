# ESP32-S3 Badge — 开发上下文

> **用途**: Copilot 新 session 快速加载。详细归档见 `wiki/projects/esp32-s3-badge.md`。

## 项目一句话

NI 50 周年 & LabVIEW 40 周年纪念电子胸卡 — ESP32-S3 + LVGL 8.3 + 2.8 寸触摸屏。

## 环境

- ESP-IDF: v6.0.1 (`D:\esp\v6.0.1\esp-idf`)
- 项目根目录: `D:\NI\98 AI\AI_Workspace\Kevin_wiki\dev\esp32-badge\boards\2.8-inch\project`
- 构建: `idf.py build`
- 烧录: `idf.py -p COM3 flash`
- COM3 占用: 自动 kill 占用进程并重试，不要问用户

## 当前状态（2026-05-03）

**6 页启用** | 二进制 0x22e3c0 bytes | 27% 空间剩余

| # | 页面 | 核心文件 |
|---|------|----------|
| 0 | 名片主页 | Badge_UI.c `build_page_card` |
| 1 | 卡通人物照 | Badge_UI.c `build_page_history` |
| 2 | Milestones 时间轴 | Badge_UI.c `build_page_milestone` |
| 3 | NI Gallery (独立屏) | page_gallery.c |
| 4 | 日历 + Today 按钮 | page_calendar.c |
| 5 | Config | page_config.c |

每个非首页有 Home 按钮（左上角）。

## 关键文件速查

| 文件 | 职责 | 大小 |
|------|------|------|
| `main/LVGL_UI/Badge_UI.c` | 主 UI 控制器、tileview、内嵌页面 | ~92KB |
| `main/LVGL_UI/page_gallery.c` | NI Gallery 独立图片浏览 | ~16KB |
| `main/LVGL_UI/page_calendar.c` | 日历 + 里程碑标记 | ~15KB |
| `main/LVGL_UI/page_config.c` | 配置页（WiFi AP 入口） | ~4KB |
| `main/LVGL_UI/page_settings.c` | 设置弹出屏 | ~11KB |
| `main/LVGL_UI/page_easter_egg.c` | 彩蛋（长按姓名 2s） | ~8KB |
| `main/LVGL_UI/game_match3.c` | 消消乐（延迟加载） | ~20KB |
| `main/Kconfig.projbuild` | 页面 CONFIG_PAGE_* 开关 | — |
| `sdkconfig` | 实际生效配置（改这里立即生效） | — |
| `tools/convert_gallery_images.py` | PIL→RGB565 C 数组 | — |

## 架构要点

1. **Tileview**: 水平滑动，`CONFIG_PAGE_*` 编译期控制页面启用/禁用
2. **独立屏**: 进入用 `auto_del=false`（保留主屏），退出用 `auto_del=true`（删除独立屏）
3. **全局返回**: `badge_main_screen` 全局变量存主屏引用
4. **延迟加载**: Match3 等重页面首次滑入才构建
5. **性能红线**: 禁止 shadow / 禁止嵌套容器 / 禁止全屏背景图 / 每页 <50 对象

## 已踩的坑（速查）

- `sdkconfig.defaults` 仅 fullclean 后生效 → 日常改 `sdkconfig`
- PNG 透明图转 RGB565 需先合成白底再转换
- `gallery_images.h` 数组声明用 `static const`，不能 `static inline`
- WiFi AP 和 STA 模式不能混用
- RTC (PCF85063) 无纽扣电池时断电重置 → 加 validity check
- I2C 新旧驱动不能共存 → 全部用 `i2c_master.h` 新 API
