/* SPDX-License-Identifier: MIT */
#include "esp_stubs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Deterministic RNG so a simulator run is reproducible. */
static uint32_t s_rng = 0x12345678u;

uint32_t esp_random(void)
{
    s_rng ^= s_rng << 13;
    s_rng ^= s_rng >> 17;
    s_rng ^= s_rng << 5;
    return s_rng;
}

uint32_t esp_get_free_heap_size(void)
{
    return 320 * 1024;
}

uint32_t esp_get_minimum_free_heap_size(void)
{
    return 280 * 1024;
}

size_t heap_caps_get_free_size(uint32_t caps)
{
    (void)caps;
    return 24u * 1024 * 1024;
}

/* --- tiny NVS stand-in: one blob, kept in RAM ---------------------------- */
static uint8_t s_blob[64];
static size_t s_blob_len;
static int s_open;

esp_err_t nvs_open(const char *name, int open_mode, nvs_handle_t *out_handle)
{
    (void)name;
    (void)open_mode;
    s_open = 1;
    *out_handle = 1;
    return ESP_OK;
}

void nvs_close(nvs_handle_t handle)
{
    (void)handle;
    s_open = 0;
}

esp_err_t nvs_get_blob(nvs_handle_t handle, const char *key, void *out_value, size_t *length)
{
    (void)handle;
    (void)key;
    if (!s_open || s_blob_len == 0) {
        return ESP_ERR_NVS_NOT_FOUND;
    }
    if (*length < s_blob_len) {
        return ESP_FAIL;
    }
    memcpy(out_value, s_blob, s_blob_len);
    *length = s_blob_len;
    return ESP_OK;
}

esp_err_t nvs_set_blob(nvs_handle_t handle, const char *key, const void *value, size_t length)
{
    (void)handle;
    (void)key;
    if (length > sizeof(s_blob)) {
        return ESP_FAIL;
    }
    memcpy(s_blob, value, length);
    s_blob_len = length;
    return ESP_OK;
}

esp_err_t nvs_commit(nvs_handle_t handle)
{
    (void)handle;
    return ESP_OK;
}

esp_err_t nvs_flash_init(void)
{
    return ESP_OK;
}

esp_err_t nvs_flash_erase(void)
{
    s_blob_len = 0;
    return ESP_OK;
}

const char *esp_err_to_name(esp_err_t code)
{
    static char buffer[32];
    switch (code) {
    case ESP_OK:
        return "ESP_OK";
    case ESP_ERR_NVS_NOT_FOUND:
        return "ESP_ERR_NVS_NOT_FOUND";
    default:
        snprintf(buffer, sizeof(buffer), "esp_err_0x%X", (unsigned)code);
        return buffer;
    }
}

void esp_error_check_impl(esp_err_t code, const char *file, int line)
{
    if (code != ESP_OK) {
        printf("[E] ESP_ERROR_CHECK failed: %s at %s:%d\n", esp_err_to_name(code), file, line);
        exit(3);
    }
}
