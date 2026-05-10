#include "svc_rtc.h"

#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/i2c_master.h"
#include "bsp/esp32_s3_touch_lcd_4.h"

#define RTC_I2C_ADDR         0x51
#define RTC_REG_SECONDS      0x04
#define RTC_REG_YEARS        0x0A
#define RTC_I2C_TIMEOUT_MS   100

static const char *TAG = "svc_rtc";

static bool s_initialized;
static bool s_hw_available;
static i2c_master_dev_handle_t s_rtc_dev;
static svc_rtc_time_t s_soft_base_time;
static int64_t s_soft_base_us;

static int rtc_days_in_month(int year, int month)
{
    static const int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int value = days[month - 1];
    if (month == 2 && ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))) {
        value = 29;
    }
    return value;
}

static uint8_t rtc_weekday(int year, int month, int day)
{
    static const int offsets[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    if (month < 3) {
        year--;
    }
    return (uint8_t)((year + year / 4 - year / 100 + year / 400 + offsets[month - 1] + day) % 7);
}

static uint8_t rtc_bcd_to_dec(uint8_t value)
{
    return (uint8_t)(((value >> 4) * 10) + (value & 0x0F));
}

static uint8_t rtc_dec_to_bcd(uint8_t value)
{
    return (uint8_t)(((value / 10) << 4) | (value % 10));
}

static int rtc_month_from_abbrev(const char *month_str)
{
    static const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                   "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    for (int index = 0; index < 12; index++) {
        if (strncmp(month_str, months[index], 3) == 0) {
            return index + 1;
        }
    }
    return 1;
}

static svc_rtc_time_t rtc_compile_time(void)
{
    svc_rtc_time_t time_value = {0};
    char month_buf[4] = {0};
    int day = 1;
    int year = 2026;
    int hour = 0;
    int minute = 0;
    int second = 0;

    memcpy(month_buf, __DATE__, 3);
    month_buf[3] = '\0';

    sscanf(__DATE__ + 4, "%d %d", &day, &year);
    sscanf(__TIME__, "%d:%d:%d", &hour, &minute, &second);

    time_value.year = (uint16_t)year;
    time_value.month = (uint8_t)rtc_month_from_abbrev(month_buf);
    time_value.day = (uint8_t)day;
    time_value.hour = (uint8_t)hour;
    time_value.minute = (uint8_t)minute;
    time_value.second = (uint8_t)second;
    time_value.weekday = rtc_weekday(year, time_value.month, day);
    return time_value;
}

bool svc_rtc_is_time_valid(const svc_rtc_time_t *time_value)
{
    if (!time_value) {
        return false;
    }

    if (time_value->year < 2025 || time_value->year > 2099) {
        return false;
    }

    if (time_value->month < 1 || time_value->month > 12) {
        return false;
    }

    if (time_value->day < 1 || time_value->day > rtc_days_in_month(time_value->year, time_value->month)) {
        return false;
    }

    if (time_value->hour > 23 || time_value->minute > 59 || time_value->second > 59) {
        return false;
    }

    return true;
}

static bool rtc_is_older_than(const svc_rtc_time_t *lhs, const svc_rtc_time_t *rhs)
{
    if (lhs->year != rhs->year) {
        return lhs->year < rhs->year;
    }
    if (lhs->month != rhs->month) {
        return lhs->month < rhs->month;
    }
    if (lhs->day != rhs->day) {
        return lhs->day < rhs->day;
    }
    if (lhs->hour != rhs->hour) {
        return lhs->hour < rhs->hour;
    }
    if (lhs->minute != rhs->minute) {
        return lhs->minute < rhs->minute;
    }
    return lhs->second < rhs->second;
}

static void rtc_add_days(svc_rtc_time_t *time_value, int64_t day_delta)
{
    while (day_delta > 0) {
        int dim = rtc_days_in_month(time_value->year, time_value->month);
        if (time_value->day < dim) {
            time_value->day++;
        } else {
            time_value->day = 1;
            if (time_value->month < 12) {
                time_value->month++;
            } else {
                time_value->month = 1;
                time_value->year++;
            }
        }
        day_delta--;
    }
    time_value->weekday = rtc_weekday(time_value->year, time_value->month, time_value->day);
}

static void rtc_add_seconds(svc_rtc_time_t *time_value, int64_t delta_seconds)
{
    int64_t total_seconds = (int64_t)time_value->hour * 3600 + (int64_t)time_value->minute * 60 + time_value->second + delta_seconds;
    int64_t day_delta = total_seconds / 86400;
    int64_t sec_of_day = total_seconds % 86400;

    if (sec_of_day < 0) {
        sec_of_day += 86400;
        day_delta--;
    }

    if (day_delta > 0) {
        rtc_add_days(time_value, day_delta);
    }

    time_value->hour = (uint8_t)(sec_of_day / 3600);
    time_value->minute = (uint8_t)((sec_of_day % 3600) / 60);
    time_value->second = (uint8_t)(sec_of_day % 60);
}

static esp_err_t rtc_soft_get_now(svc_rtc_time_t *out_time)
{
    *out_time = s_soft_base_time;
    rtc_add_seconds(out_time, (esp_timer_get_time() - s_soft_base_us) / 1000000);
    return ESP_OK;
}

static void rtc_soft_set_base(const svc_rtc_time_t *time_value)
{
    s_soft_base_time = *time_value;
    s_soft_base_us = esp_timer_get_time();
}

static esp_err_t rtc_hw_read_time(svc_rtc_time_t *out_time)
{
    uint8_t reg = RTC_REG_SECONDS;
    uint8_t raw[7] = {0};
    esp_err_t ret = i2c_master_transmit_receive(s_rtc_dev, &reg, sizeof(reg), raw, sizeof(raw), RTC_I2C_TIMEOUT_MS);
    if (ret != ESP_OK) {
        return ret;
    }

    out_time->second = rtc_bcd_to_dec(raw[0] & 0x7F);
    out_time->minute = rtc_bcd_to_dec(raw[1] & 0x7F);
    out_time->hour = rtc_bcd_to_dec(raw[2] & 0x3F);
    out_time->day = rtc_bcd_to_dec(raw[3] & 0x3F);
    out_time->weekday = rtc_bcd_to_dec(raw[4] & 0x07);
    out_time->month = rtc_bcd_to_dec(raw[5] & 0x1F);
    out_time->year = (uint16_t)(2000 + rtc_bcd_to_dec(raw[6]));
    return ESP_OK;
}

static esp_err_t rtc_hw_write_time(const svc_rtc_time_t *time_value)
{
    uint8_t raw[8] = {
        RTC_REG_SECONDS,
        rtc_dec_to_bcd(time_value->second),
        rtc_dec_to_bcd(time_value->minute),
        rtc_dec_to_bcd(time_value->hour),
        rtc_dec_to_bcd(time_value->day),
        rtc_dec_to_bcd(time_value->weekday),
        rtc_dec_to_bcd(time_value->month),
        rtc_dec_to_bcd((uint8_t)(time_value->year % 100)),
    };

    return i2c_master_transmit(s_rtc_dev, raw, sizeof(raw), RTC_I2C_TIMEOUT_MS);
}

static esp_err_t rtc_init_hw_device(void)
{
    i2c_master_bus_handle_t bus_handle = bsp_i2c_get_handle();
    esp_err_t ret;

    if (!bus_handle) {
        return ESP_ERR_INVALID_STATE;
    }

    ret = i2c_master_probe(bus_handle, RTC_I2C_ADDR, RTC_I2C_TIMEOUT_MS);
    if (ret != ESP_OK) {
        return ret;
    }

    const i2c_device_config_t dev_cfg = {
        .device_address = RTC_I2C_ADDR,
        .scl_speed_hz = CONFIG_BSP_I2C_CLK_SPEED_HZ,
    };
    return i2c_master_bus_add_device(bus_handle, &dev_cfg, &s_rtc_dev);
}

esp_err_t svc_rtc_init(void)
{
    svc_rtc_time_t compile_time = rtc_compile_time();
    svc_rtc_time_t current_time = {0};

    if (s_initialized) {
        return ESP_OK;
    }

    rtc_soft_set_base(&compile_time);
    s_initialized = true;

    if (rtc_init_hw_device() != ESP_OK) {
        ESP_LOGW(TAG, "RTC chip not detected, using software fallback");
        return ESP_OK;
    }

    s_hw_available = true;

    if (rtc_hw_read_time(&current_time) != ESP_OK || !svc_rtc_is_time_valid(&current_time) || rtc_is_older_than(&current_time, &compile_time)) {
        ESP_LOGW(TAG, "RTC time invalid or stale, seeding from firmware build time");
        if (rtc_hw_write_time(&compile_time) == ESP_OK) {
            current_time = compile_time;
        } else {
            ESP_LOGW(TAG, "RTC write failed, keeping software fallback only");
            s_hw_available = false;
        }
    }

    rtc_soft_set_base(s_hw_available ? &current_time : &compile_time);
    ESP_LOGI(TAG, "RTC source: %s", s_hw_available ? "PCF85063" : "software fallback");
    return ESP_OK;
}

esp_err_t svc_rtc_get_now(svc_rtc_time_t *out_time)
{
    esp_err_t ret;

    if (!out_time) {
        return ESP_ERR_INVALID_ARG;
    }

    ret = svc_rtc_init();
    if (ret != ESP_OK) {
        return ret;
    }

    if (s_hw_available) {
        ret = rtc_hw_read_time(out_time);
        if (ret == ESP_OK && svc_rtc_is_time_valid(out_time)) {
            rtc_soft_set_base(out_time);
            return ESP_OK;
        }

        ESP_LOGW(TAG, "RTC read failed, degrading to software fallback");
        s_hw_available = false;
    }

    return rtc_soft_get_now(out_time);
}

esp_err_t svc_rtc_set_now(const svc_rtc_time_t *time_value)
{
    esp_err_t ret = svc_rtc_init();
    if (ret != ESP_OK) {
        return ret;
    }

    if (!svc_rtc_is_time_valid(time_value)) {
        return ESP_ERR_INVALID_ARG;
    }

    rtc_soft_set_base(time_value);

    if (s_hw_available) {
        ret = rtc_hw_write_time(time_value);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "RTC write failed, software fallback remains active");
            s_hw_available = false;
        }
    }

    return ESP_OK;
}

bool svc_rtc_is_hw_available(void)
{
    return s_hw_available;
}
