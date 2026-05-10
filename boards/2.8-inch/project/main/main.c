#include "ST7789.h"
#include "PCF85063.h"
#include "QMI8658.h"
#include "SD_MMC.h"
#include "Wireless.h"
#include "Badge_UI.h"
#include "BAT_Driver.h"
#include "PWR_Key.h"
#include "PCM5101.h"
#include "esp_log.h"
#include <string.h>
#include <stdlib.h>

/* 编译时间解析，用于初始化 RTC（仅当 RTC 时间无效时才设置） */
static void set_rtc_from_compile_time(void)
{
    /* 先读取当前 RTC 时间 */
    datetime_t now = {0};
    PCF85063_Read_Time(&now);

    /* __DATE__ = "May  1 2026", __TIME__ = "14:30:00" */
    const char *date = __DATE__;
    const char *time_str = __TIME__;

    const char *months[] = {"Jan","Feb","Mar","Apr","May","Jun",
                            "Jul","Aug","Sep","Oct","Nov","Dec"};
    datetime_t ct = {0};

    /* 解析月 */
    char mon[4] = {0};
    memcpy(mon, date, 3);
    for (int i = 0; i < 12; i++) {
        if (strcmp(mon, months[i]) == 0) { ct.month = i + 1; break; }
    }
    /* 解析日 */
    ct.day = atoi(date + 4);
    /* 解析年 */
    ct.year = atoi(date + 7);
    /* 解析时分秒 */
    ct.hour   = (time_str[0] - '0') * 10 + (time_str[1] - '0');
    ct.minute = (time_str[3] - '0') * 10 + (time_str[4] - '0');
    ct.second = (time_str[6] - '0') * 10 + (time_str[7] - '0');

    /* 仅当 RTC 时间无效或早于编译时间时才更新 */
    bool need_set = false;
    if (now.year < 2025 || now.year > 2099) {
        need_set = true;  /* 年份明显无效 */
    } else if (now.year < ct.year) {
        need_set = true;
    } else if (now.year == ct.year && now.month < ct.month) {
        need_set = true;
    } else if (now.year == ct.year && now.month == ct.month && now.day < ct.day) {
        need_set = true;
    }

    if (need_set) {
        PCF85063_Set_All(ct);
        ESP_LOGI("RTC", "RTC set to compile time: %d-%02d-%02d %02d:%02d:%02d",
                 ct.year, ct.month, ct.day, ct.hour, ct.minute, ct.second);
    } else {
        ESP_LOGI("RTC", "RTC already valid: %d-%02d-%02d %02d:%02d:%02d",
                 now.year, now.month, now.day, now.hour, now.minute, now.second);
    }
}

void Driver_Loop(void *parameter)
{
    /* Wireless_Init() removed — config_server handles WiFi AP directly */
    while(1)
    {
        QMI8658_Loop();
        PCF85063_Loop();
        BAT_Get_Volts();
        PWR_Loop();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    vTaskDelete(NULL);
}
void Driver_Init(void)
{
    PWR_Init();
    BAT_Init();
    I2C_Init();
    PCF85063_Init();
    set_rtc_from_compile_time();  /* 用编译时间设置 RTC */
    QMI8658_Init();
    Flash_Searching();
    xTaskCreatePinnedToCore(
        Driver_Loop, 
        "Other Driver task",
        4096, 
        NULL, 
        3, 
        NULL, 
        0);
}
void app_main(void)
{
    Driver_Init();

    SD_Init();
    LCD_Init();
    Audio_Init();
    // Play_Music("/sdcard","AAA.mp3");
    LVGL_Init();   // returns the screen object

/********************* Badge UI *********************/
    Badge_UI_Init();
    // lv_demo_widgets();
    // lv_demo_keypad_encoder();
    // lv_demo_benchmark();
    // lv_demo_stress();
    // lv_demo_music();

    while (1) {
        // raise the task priority of LVGL and/or reduce the handler period can improve the performance
        vTaskDelay(pdMS_TO_TICKS(10));
        // The task running lv_timer_handler should have lower priority than that running `lv_tick_inc`
        lv_timer_handler();
    }
}






