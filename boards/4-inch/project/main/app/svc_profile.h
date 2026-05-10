#pragma once

#include <stdbool.h>
#include "esp_err.h"

typedef struct {
    char name[64];
    char dept[128];
} badge_profile_t;

esp_err_t svc_profile_init(void);
esp_err_t svc_profile_get(badge_profile_t *profile);
esp_err_t svc_profile_set(const badge_profile_t *profile);
bool svc_profile_is_ready(void);
