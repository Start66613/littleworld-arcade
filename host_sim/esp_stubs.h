/* Host-side stubs for the small set of ESP-IDF APIs the arcade UI touches.
 * Only used by the simulator; the firmware uses the real implementations.
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include <stdint.h>
#include <stddef.h>

typedef int esp_err_t;
#define ESP_OK 0
#define ESP_FAIL -1
#define ESP_ERR_NVS_NO_FREE_PAGES 0x110D
#define ESP_ERR_NVS_NEW_VERSION_FOUND 0x1110
#define ESP_ERR_NVS_NOT_FOUND 0x1102

/* esp_log.h */
#define ESP_LOGI(tag, fmt, ...) printf("[I %s] " fmt "\n", tag, ##__VA_ARGS__)
#define ESP_LOGW(tag, fmt, ...) printf("[W %s] " fmt "\n", tag, ##__VA_ARGS__)
#define ESP_LOGE(tag, fmt, ...) printf("[E %s] " fmt "\n", tag, ##__VA_ARGS__)
#define ESP_LOGD(tag, fmt, ...) ((void)0)

/* esp_random.h */
uint32_t esp_random(void);

/* esp_system.h */
uint32_t esp_get_free_heap_size(void);
uint32_t esp_get_minimum_free_heap_size(void);

/* esp_heap_caps.h */
#define MALLOC_CAP_SPIRAM (1 << 10)
size_t heap_caps_get_free_size(uint32_t caps);

/* nvs.h */
#define NVS_READONLY 0
#define NVS_READWRITE 1
typedef uint32_t nvs_handle_t;
esp_err_t nvs_open(const char *name, int open_mode, nvs_handle_t *out_handle);
void nvs_close(nvs_handle_t handle);
esp_err_t nvs_get_blob(nvs_handle_t handle, const char *key, void *out_value, size_t *length);
esp_err_t nvs_set_blob(nvs_handle_t handle, const char *key, const void *value, size_t length);
esp_err_t nvs_commit(nvs_handle_t handle);

/* nvs_flash.h */
esp_err_t nvs_flash_init(void);
esp_err_t nvs_flash_erase(void);

/* esp_err.h helper */
const char *esp_err_to_name(esp_err_t code);
void esp_error_check_impl(esp_err_t code, const char *file, int line);
#define ESP_ERROR_CHECK(x) esp_error_check_impl((x), __FILE__, __LINE__)
