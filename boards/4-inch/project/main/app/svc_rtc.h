#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

typedef struct {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t weekday;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} svc_rtc_time_t;

esp_err_t svc_rtc_init(void);
esp_err_t svc_rtc_get_now(svc_rtc_time_t *out_time);
esp_err_t svc_rtc_set_now(const svc_rtc_time_t *time_value);
bool svc_rtc_is_hw_available(void);
bool svc_rtc_is_time_valid(const svc_rtc_time_t *time_value);
