/* Little World Arcade - application entry point.
 *
 * Thin on purpose: board bring-up is delegated to the pinned Waveshare BSP
 * (MIPI-DSI panel + GT911 touch + esp_lvgl_adapter), all screens live in the
 * arcade_ui component and all game rules live in the LVGL-free arcade_core
 * component.
 *
 * The log lines below are deliberately explicit: on the first flash the serial
 * console is the only way to tell how far the boot got.
 *
 * SPDX-License-Identifier: MIT
 */
#include "bsp/esp-bsp.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "lvgl.h"

#include "arcade_ui.h"

static const char *TAG = "arcade";

void app_main(void)
{
    ESP_LOGI(TAG, "boot: reset reason %d, free heap %u B", (int)esp_reset_reason(),
             (unsigned)esp_get_free_heap_size());

    bsp_display_cfg_t cfg = {
        .lv_adapter_cfg = ESP_LV_ADAPTER_DEFAULT_CONFIG(),
        .rotation = ESP_LV_ADAPTER_ROTATE_180,
        .tear_avoid_mode = ESP_LV_ADAPTER_TEAR_AVOID_MODE_TRIPLE_PARTIAL,
        .touch_flags = {
            .swap_xy = 0,
            .mirror_x = 1,
            .mirror_y = 1,
        },
    };

    ESP_LOGI(TAG, "boot: initialising MIPI-DSI panel + GT911 touch");
    lv_display_t *display = bsp_display_start_with_config(&cfg);
    if (display == NULL) {
        ESP_LOGE(TAG, "bsp_display_start_with_config failed - giving up before the UI");
        return;
    }
    ESP_LOGI(TAG, "boot: display %dx%d ready, free heap %u B", (int)lv_display_get_horizontal_resolution(display),
             (int)lv_display_get_vertical_resolution(display), (unsigned)esp_get_free_heap_size());

    esp_err_t backlight = bsp_display_backlight_on();
    if (backlight != ESP_OK) {
        ESP_LOGW(TAG, "backlight on failed: %s", esp_err_to_name(backlight));
    }

    if (!bsp_display_lock(-1)) {
        ESP_LOGE(TAG, "cannot take the LVGL lock");
        return;
    }

    ESP_LOGI(TAG, "boot: building the arcade UI");
    arcade_ui_start();

    bsp_display_unlock();

    ESP_LOGI(TAG, "boot: Little World Arcade ready, free heap %u B (min %u B)", (unsigned)esp_get_free_heap_size(),
             (unsigned)esp_get_minimum_free_heap_size());
}
