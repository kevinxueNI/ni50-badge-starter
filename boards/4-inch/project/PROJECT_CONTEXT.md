# ESP32 LCD-4 Badge — 开发上下文

> 用途: 4 英寸项目的 session 快速加载入口。详细归档见 `wiki/projects/esp32-s3-lcd4-badge.md`。

## 项目一句话

NI 50 周年 & LabVIEW 40 周年纪念电子胸卡的 4 英寸升级版，基于 ESP32-S3 + LVGL 8.4 + 480×480 方屏。

## Canonical 路径

- 项目根目录: `dev/esp32-badge/boards/4-inch/project/`
- 板卡入口说明: `dev/esp32-badge/boards/4-inch/README.md`

## 环境

- ESP-IDF: v6.0.1 (`D:\esp\v6.0.1\esp-idf`)
- 板卡: Waveshare ESP32-S3-Touch-LCD-4
- 激活:
  - `Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass`
  - `. "C:\Espressif\tools\Microsoft.v6.0.1.PowerShell_profile.ps1"`

## 当前状态（2026-05-06）

- 480×480 Badge UI 已迁入 Kevin_wiki canonical 目录
- 当前已完成主页、Milestones、Gallery、Calendar、Config、彩蛋入口和 Trivia game 改造
- 资源转换脚本已切到 workspace 内相对路径

## 关键文件速查

- `main/ui/page_card.c` — 首页
- `main/ui/page_milestones.c` — 时间轴
- `main/ui/page_gallery.c` — Gallery
- `main/ui/page_calendar.c` — 日历
- `main/ui/page_config.c` — 设置页
- `main/ui/page_easter_egg.c` — 彩蛋菜单
- `main/ui/game_trivia.c` — Trivia 游戏
- `tools/convert_all_assets.py` — 图片素材转换

## 迁移规则

- 提到“4 英寸 / 4寸 / lcd-4”时，默认指向本目录
- 不再把 `raw/` 下的代码镜像当作活跃工程
