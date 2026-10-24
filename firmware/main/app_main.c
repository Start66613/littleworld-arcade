/* Little World Arcade - application entry point.
 *
 * Thin on purpose: board bring-up is delegated to the pinned Waveshare BSP
 * (MIPI-DSI panel + GT911 touch + esp_lvgl_adapter), all screens live in the
 * arcade_ui component and all game rules live in the LVGL-free arcade_core
 * component.
 *
 * SPDX-License-Identifier: MIT
 */
#include "bsp/esp-bsp.h"
#include "esp_err.h"
#include "esp_log.h"
#include "lvgl.h"

#include "arcade_ui.h"

static const char *TAG = "arcade";

void app_main(void)
{
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

    lv_display_t *display = bsp_display_start_with_config(&cfg);
    if (display == NULL) {
        ESP_LOGE(TAG, "bsp_display_start_with_config failed");
        return;
    }

    bsp_display_backlight_on();

    if (!bsp_display_lock(-1)) {
        ESP_LOGE(TAG, "cannot take LVGL lock");
        return;
    }

    arcade_ui_start();

    bsp_display_unlock();

    ESP_LOGI(TAG, "Little World Arcade UI started (%dx%d)", (int)lv_display_get_horizontal_resolution(display),
             (int)lv_display_get_vertical_resolution(display));
}
