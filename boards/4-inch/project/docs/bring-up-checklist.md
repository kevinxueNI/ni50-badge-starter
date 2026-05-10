# P2 实机 Bring-up 准备包

> 板子 ready 后照此文档执行，不再回头补流程。

## 1. Flash 命令

### 标准烧录（COM3，460800 baud）

```powershell
# 激活 IDF 环境
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
& "C:\Espressif\tools\Microsoft.v6.0.1.PowerShell_profile.ps1"

# 进入工程目录
cd D:\NI\98 AI\AI_Workspace\Kevin_wiki\dev\esp32-badge\boards\4-inch\project

# 烧录 + 串口监视
idf.py -p COM3 flash monitor
```

### 仅烧录 app（跳过 bootloader / partition table）

```powershell
idf.py -p COM3 -b 921600 app-flash
```

### 手动 esptool（备用）

```powershell
python -m esptool --chip esp32s3 -p COM3 -b 460800 `
  --before default-reset --after hard-reset write-flash `
  --flash-mode dio --flash-size 16MB --flash-freq 80m `
  0x0 build/bootloader/bootloader.bin `
  0x8000 build/partition_table/partition-table.bin `
  0x10000 build/esp32s3_lcd4_badge.bin
```

### COM 口被占用时

```powershell
# 自动杀占用进程
$port = "COM3"
$pnp = Get-CimInstance Win32_SerialPort | Where-Object { $_.DeviceID -eq $port }
if ($pnp) {
    $proc = Get-Process | Where-Object { $_.Id -ne $PID } | Where-Object {
        try { $_.Modules | Where-Object { $_.FileName -match 'serial|uart|com' } } catch {}
    }
    if ($proc) { $proc | Stop-Process -Force; Start-Sleep 1 }
}
idf.py -p $port flash monitor
```

---

## 2. 验证 Checklist

板子上电后逐项确认，每项标 ✅ / ❌ + 备注。

| # | 验证项 | 预期结果 | 通过 | 备注 |
|---|--------|---------|------|------|
| 1 | 串口日志启动 | 能看到 `Starting Badge app...` | | |
| 2 | 屏幕亮 | 背光点亮，无白屏/花屏 | | |
| 3 | LVGL 初始化 | 显示 "Hello LCD-4!" 居中 | | |
| 4 | 颜色正确 | 黑底白字，无色偏/BGR反转 | | |
| 5 | 触摸响应 | 串口打印触摸坐标（需临时加 log） | | |
| 6 | 触摸坐标准确 | 左上角 (0,0)，右下角接近 (479,479) | | |
| 7 | 刷新流畅 | 无明显撕裂/闪烁/低帧率 | | |
| 8 | PSRAM 生效 | 日志显示 `Detected size: 8MB` | | |
| 9 | 无 WDT 重启 | 运行 60 秒无自动重启 | | |
| 10 | 内存余量 | `heap_caps_get_free_size(MALLOC_CAP_INTERNAL)` > 100KB | | |
| 11 | 5 分钟稳定性 | 持续运行无崩溃，触摸持续响应 | | |
| 12 | 功耗正常 | USB 供电电流 < 500mA (无外设全开) | | |

### 触摸坐标验证代码片段

在 `app_main()` 的 `bsp_display_unlock()` 之后加：

```c
/* Temporary: print touch coords to serial */
static void touch_debug_cb(lv_event_t *e) {
    lv_point_t p;
    lv_indev_get_point(lv_indev_get_act(), &p);
    ESP_LOGI("TOUCH", "x=%d y=%d", p.x, p.y);
}
lv_obj_add_event_cb(lv_scr_act(), touch_debug_cb, LV_EVENT_PRESSING, NULL);
```

---

## 3. 日志模板

每次 bring-up session 用此模板记录：

```markdown
## Bring-up Session [YYYY-MM-DD HH:MM]

**环境**: ESP-IDF v6.0.1 | BSP v2.0.0 | COM3
**固件**: build/esp32s3_lcd4_badge.bin (size: ___ bytes)
**供电**: USB / 电池

### Checklist 结果

| # | 通过 | 备注 |
|---|------|------|
| 1 | | |
| 2 | | |
| ... | | |

### 异常记录

- 现象:
- 串口日志关键行:
- 复现步骤:
- 根因:
- 修复:

### 性能数据

- Free heap (internal): ___ KB
- Free heap (PSRAM): ___ KB
- LVGL FPS (估算): ___
- Boot-to-display time: ___ ms

### 结论 & 下一步

-
```

---

## 4. 异常回退方案

| 场景 | 处理 |
|------|------|
| 烧录后白屏 | 检查 RGB timing，尝试降低 PCLK（sdkconfig 或 BSP 宏） |
| 触摸无响应 | 检查 I2C 地址 (GT911 = 0x5D/0x14)，确认 RST/INT GPIO |
| 颜色 BGR 反转 | 在 BSP 中切换 `LCD_RGB_ELEMENT_ORDER_BGR` ↔ `_RGB` |
| Boot loop (WDT) | 降低 LVGL 复杂度，检查是否有 PSRAM 访问失败 |
| Flash 失败 | 按住 BOOT 键再按 RST，进入下载模式；或检查 USB 线质量 |
| COM 口不出现 | 检查 USB-C 线是否支持数据传输（非充电线） |
| Ninja Permission Denied | 删除 build/ 目录重新编译 |
