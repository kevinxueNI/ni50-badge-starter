# ESP32-S3-Touch-LCD-4 Project Template

Board: **Waveshare ESP32-S3-Touch-LCD-4** (480×480 RGB ST7701, GT911 touch, 16MB Flash, 8MB PSRAM)

## Prerequisites

- ESP-IDF v6.0.1 (`D:\esp\v6.0.1\esp-idf`)
- Activation: `& "C:\Espressif\tools\Microsoft.v6.0.1.PowerShell_profile.ps1"`

## First Build (requires internet for component download)

```powershell
cd D:\NI\98 AI\AI_Workspace\Kevin_wiki\dev\esp32-badge\boards\4-inch\project
idf.py set-target esp32s3
idf.py build          # Will fail on first run (BSP incompatible with IDF v6.0.1)
.\patches\apply_patches.ps1   # Apply compatibility patches
idf.py build          # Should succeed
```

## Flash

```powershell
idf.py -p COM3 flash monitor
```

## ESP-IDF v6.0.1 Compatibility Notes

Waveshare BSP v2.0.0 was written for IDF 5.3-5.4. The following patches are needed for v6.0.1:

1. **`custom_io_expander_ch32v003`**: `driver/i2c_master.h` moved to `esp_driver_i2c`
2. **`esp32_s3_touch_lcd_4`**: `driver/ledc.h` moved to `esp_driver_ledc`
3. **`esp_lcd_rgb_panel_config_t`**: `.psram_trans_align` → `.dma_burst_size`, `.bits_per_pixel` removed
4. **Warnings-as-errors**: ST7701 timing macro triggers `-Werror=override-init` and `-Werror=missing-braces`

All handled by `patches/apply_patches.ps1` and top-level `CMakeLists.txt` flags.

## Architecture

```
main/main.c           - App entry, UI code
main/idf_component.yml - BSP + LVGL dependencies (auto-fetched)
sdkconfig.defaults    - Hardware config (flash, PSRAM, LVGL)
partitions.csv        - 8MB app + 7MB SPIFFS storage
patches/              - IDF v6.0.1 compat patches for BSP
```

## BSP API Quick Reference

```c
bsp_display_start();           // Init display + touch + backlight
bsp_display_lock(timeout_ms);  // Lock LVGL mutex (0 = wait forever)
bsp_display_unlock();          // Unlock LVGL mutex
bsp_display_backlight_on();    // Backlight control
bsp_display_backlight_off();
```

Display: 480×480 px, 16-bit RGB565, double-buffered in PSRAM.
