# ESP32 Badge 2.8-inch

## 别名

- 2.8 英寸
- 2.8寸
- lcd-2.8
- badge-2.8
- ESP32-S3 Badge

## Share Package Path

- Project: `boards/2.8-inch/project/`
- Quick Start: `docs/quick-start.md`

## Board

Waveshare ESP32-S3-Touch-LCD-2.8, 240×320, ST7789 SPI, CST328 touch.

## Current Status

- 作为分享仓中的轻量起步工程保留
- 适合第一次 build / flash / 个性化修改

## Build

```powershell
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
. "C:\Espressif\tools\Microsoft.v6.0.1.PowerShell_profile.ps1"
Push-Location ".\boards\2.8-inch\project"
idf.py build
Pop-Location
```
