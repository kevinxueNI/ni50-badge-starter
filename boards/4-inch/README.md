# ESP32 Badge 4-inch

## 别名

- 4 英寸
- 4寸
- lcd-4
- badge-4
- ESP32-S3 LCD-4 Badge

## Share Package Path

- Project: `boards/4-inch/project/`
- Quick Start: `docs/quick-start.md`

## Board

Waveshare ESP32-S3-Touch-LCD-4, 480×480, ST7701 RGB, GT911 touch.

## Current Status

- 作为分享仓中的完整展示工程保留
- 适合更完整的 UI 表达、页面扩展和展示型改造

## Build

```powershell
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
. "C:\Espressif\tools\Microsoft.v6.0.1.PowerShell_profile.ps1"
Push-Location ".\boards\4-inch\project"
idf.py build
Pop-Location
```
