# NI 50th & LabVIEW 40th 纪念电子胸卡

基于 Waveshare ESP32-S3-Touch-LCD-2.8 开发板的电子胸卡/名片项目。
NI 50 周年 & LabVIEW 40 周年极客纪念品，内置 7 页横向 Tileview：

| 页码 | 内容 | 交互方式 |
|------|------|----------|
| 0 | 名片主页（头像/姓名/部门/周年Logo） | — |
| 1 | NI 历史里程碑 Timeline | 自动动画 |
| 2 | 个人卡通图 | — |
| 3 | 记忆翻牌游戏 (12对) | 触摸点击 |
| 4 | 2048 游戏 | IMU 倾斜控制 |
| 5 | 图片播放器 (SD卡) | 触摸翻页/自动播放 |
| 6 | 恩了个恩 3D消消乐 | 触摸点击 |

## 硬件

| 参数 | 值 |
|------|-----|
| MCU | ESP32-S3 (QFN56) rev v0.2 |
| Flash | 16 MB |
| PSRAM | 8 MB |
| LCD | ST7789, 240×320, SPI |
| 触摸 | CST328 电容触摸, I2C |
| 传感器 | QMI8658 (IMU) + PCF85063 (RTC), I2C |
| 音频 | PCM5101 DAC, I2S |
| 背光 | GPIO5 (LEDC PWM) |

## 引脚定义

### LCD (SPI3_HOST)
- MOSI: GPIO45, SCLK: GPIO40, CS: GPIO42, DC: GPIO41, RST: GPIO39

### Touch I2C (Port 1)
- SDA: GPIO1, SCL: GPIO3, INT: GPIO4, RST: GPIO2

### Sensor I2C (Port 0)
- SDA: GPIO11, SCL: GPIO10

## 开发环境

- **ESP-IDF**: v6.0.1 (`D:\esp\v6.0.1\esp-idf`)
- **LVGL**: 8.3.11
- **激活环境**: `& "C:\Espressif\tools\Microsoft.v6.0.1.PowerShell_profile.ps1"`

## 编译 & 烧录

```powershell
# 激活 ESP-IDF 环境
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
& "C:\Espressif\tools\Microsoft.v6.0.1.PowerShell_profile.ps1"

# 设置目标芯片（首次）
idf.py set-target esp32s3

# 编译
idf.py build

# 烧录 + 串口监视
idf.py -p COM3 flash monitor
```

## ESP-IDF v6.0.1 兼容性修复（已完成）

原始 Waveshare demo 基于旧版 ESP-IDF，以下改动已包含在本模板中：

1. **I2S 驱动依赖** — `esp_driver_i2s` 添加到 audio-player 组件的 REQUIRES
2. **LCD 枚举类型** — `lcd_color_rgb_endian_t` → `lcd_rgb_element_order_t`
3. **I2C 驱动迁移** — 全部使用新 API (`i2c_master_bus_handle_t`)，不混用新旧驱动
4. **Touch SCL 频率** — CST328 config 宏中补充 `.scl_speed_hz` 字段

## 目录结构

```
main/
├── main.c                  # 入口, 初始化各驱动 + LVGL 主循环
├── LCD_Driver/             # ST7789 LCD 驱动 (SPI)
├── Touch_Driver/           # CST328 触摸驱动 (I2C)
├── LVGL_Driver/            # LVGL 显示/输入设备绑定
├── LVGL_UI/                # Badge UI (7页 tileview)
│   ├── Badge_UI.c/h        # 主界面逻辑
│   ├── game_match3.c/h     # 恩了个恩消消乐
│   ├── cards/              # 翻牌游戏图片资源
│   └── game_tiles/         # 消消乐图片资源
├── I2C_Driver/             # I2C 总线驱动 (传感器用)
├── Audio_Driver/           # PCM5101 I2S 音频
├── BAT_Driver/             # 电池电压 ADC
├── QMI8658/                # 六轴 IMU
├── PCF85063/               # RTC 时钟
├── SD_Card/                # SD 卡 (SDMMC)
├── PWR_Key/                # 电源键
└── Wireless/               # Wi-Fi/BT 扫描
components/
├── chmorgan__esp-audio-player/
├── chmorgan__esp-libhelix-mp3/
└── lvgl__lvgl/             # LVGL 8.3.11
tools/
├── convert_game_tiles.py   # 游戏图片转 C 数组脚本
├── gen_cards.py            # 翻牌游戏图片生成
├── img_to_lvgl.py          # 通用图片转 LVGL 工具
└── patch_*.py              # 开发过程中的批量修补脚本
```

## SD 卡内容（图片播放器）

在 SD 卡根目录创建 `photos/` 文件夹，放入 `.jpg`/`.png`/`.bmp` 图片即可。

## 基于模板新建项目

1. 复制本目录为新项目名
2. 修改顶层 `CMakeLists.txt` 中的 `project(...)` 名称
3. 在 `main/LVGL_UI/` 中编写新界面
4. `main/main.c` 中调用你的新 UI 函数
