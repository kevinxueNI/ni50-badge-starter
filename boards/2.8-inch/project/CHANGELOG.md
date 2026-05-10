# Changelog

All notable changes to the ESP32-S3-Badge project will be documented in this file.

Format follows [Keep a Changelog](https://keepachangelog.com/).

## [v0.3.0] - 2026-05-03

### Added
- Calendar page (`PAGE_CALENDAR`) — monthly view with NI milestone highlighting
- Settings page — brightness slider + auto-sleep control (gear button on Card page)
- Easter egg entry — 2-second long-press on name opens hidden game menu
- NI Trivia game — 15-question knowledge quiz with True/False feedback
- Lab Match game — NI product matching tile game
- Long-press visual feedback — name label pulses (zoom + opacity blink) during press
- Perf monitor visibility control — FPS/CPU display only on DAQ page
- Answer randomization — trivia options randomly swap A/B positions each question
- `build.ps1` — one-click fullclean + build script

### Fixed
- Crash loop on boot (CONFIG_PAGE_IMPACT/IDENTITY referencing uninitialized data)
- Touch freeze (LVGL memory pool exhaustion at 48KB → increased to 128KB)

### Changed
- LVGL heap: 48KB → 128KB (`CONFIG_LV_MEM_SIZE_KILOBYTES`)
- Disabled PAGE_IMPACT and PAGE_IDENTITY by default (memory optimization)
- CONFIG system now covers all 12 page types via `Kconfig.projbuild`

## [v0.2.0] - 2026-05-01

### Added
- Page config system (`Kconfig.projbuild`) — compile-time page enable/disable
- WiFi AP mode with config web server (`PAGE_CONFIG`)
- RTC time set API via web UI
- Profile customization (name/department) via web UI
- Image Player page (`PAGE_PLAYER`) — SD card photo slideshow
- Match-3 game (`PAGE_MATCH3`) — NI-themed, lazy-loaded
- 2048 game (`PAGE_2048`) — tilt-controlled via IMU
- Impact Radar page — 5-axis static radar chart
- Identity/QR page — digital identity with orbit animation

### Changed
- Tileview expanded from 3 to 11 configurable pages
- Page indicators now dynamically sized based on active page count

## [v0.1.0] - 2026-05-01

### Added
- Initial project creation from Waveshare template
- 3-page horizontal tileview (Card, History placeholder, Game placeholder)
- Card page: geometric background, status bar, avatar, name block, NI50 logo
- Status bar: real-time clock (PCF85063) + battery percentage
- Page indicator dots with active highlight
- Avatar/logo converted to LVGL C image arrays
