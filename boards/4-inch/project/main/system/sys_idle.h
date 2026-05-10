#pragma once

#include <stdbool.h>
#include <stdint.h>

void sys_idle_init(void);
void sys_idle_set_user_brightness(uint8_t percent);
uint8_t sys_idle_get_user_brightness(void);
void sys_idle_set_timeout(uint32_t timeout_ms);
uint32_t sys_idle_get_timeout(void);
void sys_idle_mark_activity(void);
bool sys_idle_is_sleeping(void);