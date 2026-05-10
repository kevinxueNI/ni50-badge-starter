# apply_patches.ps1
# Patches Waveshare BSP v2.0.0 components for ESP-IDF v6.0.1 compatibility
# Run AFTER first `idf.py build` downloads managed_components (it will fail, then run this, then rebuild)

$root = Split-Path $PSScriptRoot -Parent
$mc = Join-Path $root "managed_components"

# 1. custom_io_expander_ch32v003: add esp_driver_i2c dependency
$ch32 = Join-Path $mc "waveshare__custom_io_expander_ch32v003\CMakeLists.txt"
if (Test-Path $ch32) {
    $content = Get-Content $ch32 -Raw
    if ($content -notmatch 'esp_driver_i2c') {
        $content = $content -replace 'REQUIRES "driver"', 'REQUIRES "driver" "esp_driver_i2c"'
        Set-Content $ch32 $content -NoNewline
        Write-Host "[PATCH] $ch32 - added esp_driver_i2c"
    }
}

# 2. esp32_s3_touch_lcd_4: add esp_driver_ledc + fix psram_trans_align
$bsp = Join-Path $mc "waveshare__esp32_s3_touch_lcd_4\CMakeLists.txt"
if (Test-Path $bsp) {
    $content = Get-Content $bsp -Raw
    if ($content -notmatch 'esp_driver_ledc') {
        $content = $content -replace 'REQUIRES\s+esp_driver_i2c\s+esp_driver_gpio\s+esp_lcd', 'REQUIRES esp_driver_i2c esp_driver_gpio esp_lcd esp_driver_ledc esp_driver_sdmmc'
        Set-Content $bsp $content -NoNewline
        Write-Host "[PATCH] $bsp - added esp_driver_ledc esp_driver_sdmmc"
    }
}

$bspSrc = Join-Path $mc "waveshare__esp32_s3_touch_lcd_4\esp32_s3_touch_lcd_4.c"
if (Test-Path $bspSrc) {
    $content = Get-Content $bspSrc -Raw
    if ($content -match '\.psram_trans_align') {
        $content = $content -replace '\.psram_trans_align\s*=\s*64', '.dma_burst_size = 64'
        # Remove bits_per_pixel from rgb_config (not panel_config)
        $content = $content -replace '\.data_width = BSP_RGB_DATA_WIDTH,\s*\r?\n\s*\.bits_per_pixel = BSP_LCD_BITS_PER_PIXEL,', '.data_width = BSP_RGB_DATA_WIDTH,'
        Set-Content $bspSrc $content -NoNewline
        Write-Host "[PATCH] $bspSrc - fixed psram_trans_align and bits_per_pixel"
    }
}

Write-Host "`nPatches applied. Run 'idf.py build' again."
