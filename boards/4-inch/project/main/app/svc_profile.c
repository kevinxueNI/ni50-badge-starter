#include "svc_profile.h"

#include <ctype.h>
#include <string.h>

#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"

static const char *TAG = "svc_profile";
static const char *PROFILE_NAMESPACE = "badge_profile";
static const char *KEY_NAME = "name";
static const char *KEY_DEPT = "dept";
static const char *DEFAULT_NAME = "Kevin XUE";
static const char *DEFAULT_DEPT = "China Innovation Center";

static badge_profile_t s_profile;
static bool s_ready;

static void copy_field(char *dst, size_t dst_size, const char *src)
{
    if (dst_size == 0) {
        return;
    }

    if (src == NULL) {
        dst[0] = '\0';
        return;
    }

    snprintf(dst, dst_size, "%s", src);
}

static void trim_in_place(char *text)
{
    size_t len;

    if (text == NULL) {
        return;
    }

    len = strlen(text);
    while (len > 0 && isspace((unsigned char)text[len - 1])) {
        text[--len] = '\0';
    }

    if (len == 0) {
        return;
    }

    size_t start = 0;
    while (text[start] != '\0' && isspace((unsigned char)text[start])) {
        start++;
    }

    if (start > 0) {
        memmove(text, text + start, strlen(text + start) + 1);
    }
}

static void profile_apply_defaults(badge_profile_t *profile)
{
    memset(profile, 0, sizeof(*profile));
    copy_field(profile->name, sizeof(profile->name), DEFAULT_NAME);
    copy_field(profile->dept, sizeof(profile->dept), DEFAULT_DEPT);
}

static void profile_sanitize(badge_profile_t *profile)
{
    trim_in_place(profile->name);
    trim_in_place(profile->dept);

    if (profile->name[0] == '\0') {
        copy_field(profile->name, sizeof(profile->name), DEFAULT_NAME);
    }
    if (profile->dept[0] == '\0') {
        copy_field(profile->dept, sizeof(profile->dept), DEFAULT_DEPT);
    }
}

static esp_err_t ensure_nvs_ready(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    return err;
}

static esp_err_t load_profile_from_nvs(badge_profile_t *profile)
{
    esp_err_t err;
    nvs_handle_t handle;
    size_t name_size = sizeof(profile->name);
    size_t dept_size = sizeof(profile->dept);

    err = nvs_open(PROFILE_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        return err;
    }

    err = nvs_get_str(handle, KEY_NAME, profile->name, &name_size);
    if (err == ESP_OK) {
        err = nvs_get_str(handle, KEY_DEPT, profile->dept, &dept_size);
    }

    nvs_close(handle);
    return err;
}

static esp_err_t save_profile_to_nvs(const badge_profile_t *profile)
{
    esp_err_t err;
    nvs_handle_t handle;

    err = nvs_open(PROFILE_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        return err;
    }

    err = nvs_set_str(handle, KEY_NAME, profile->name);
    if (err == ESP_OK) {
        err = nvs_set_str(handle, KEY_DEPT, profile->dept);
    }
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }

    nvs_close(handle);
    return err;
}

esp_err_t svc_profile_init(void)
{
    esp_err_t err;

    if (s_ready) {
        return ESP_OK;
    }

    err = ensure_nvs_ready();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize NVS: %s", esp_err_to_name(err));
        return err;
    }

    profile_apply_defaults(&s_profile);

    err = load_profile_from_nvs(&s_profile);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        err = save_profile_to_nvs(&s_profile);
    } else if (err == ESP_OK) {
        profile_sanitize(&s_profile);
    }

    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Using default profile due to NVS load failure: %s", esp_err_to_name(err));
        profile_apply_defaults(&s_profile);
        err = save_profile_to_nvs(&s_profile);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to persist default profile: %s", esp_err_to_name(err));
            return err;
        }
    }

    s_ready = true;
    ESP_LOGI(TAG, "Profile ready: name='%s' dept='%s'", s_profile.name, s_profile.dept);
    return ESP_OK;
}

esp_err_t svc_profile_get(badge_profile_t *profile)
{
    if (!s_ready) {
        return ESP_ERR_INVALID_STATE;
    }
    if (profile == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    *profile = s_profile;
    return ESP_OK;
}

esp_err_t svc_profile_set(const badge_profile_t *profile)
{
    badge_profile_t sanitized;
    esp_err_t err;

    if (!s_ready) {
        return ESP_ERR_INVALID_STATE;
    }
    if (profile == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    sanitized = *profile;
    profile_sanitize(&sanitized);

    err = save_profile_to_nvs(&sanitized);
    if (err != ESP_OK) {
        return err;
    }

    s_profile = sanitized;
    ESP_LOGI(TAG, "Profile updated: name='%s' dept='%s'", s_profile.name, s_profile.dept);
    return ESP_OK;
}

bool svc_profile_is_ready(void)
{
    return s_ready;
}
