/* Little World Arcade - NVS persistence for high scores and settings.
 *
 * A failed NVS (missing partition, corrupted namespace, no free pages) must
 * never stop the games from running: in that case defaults stay in RAM and
 * every commit becomes a no-op that only logs.
 *
 * SPDX-License-Identifier: MIT
 */
#include "arcade/arcade_save.h"
#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "ui_internal.h"

static const char *TAG = "arcade/store";
static const char *NVS_NAMESPACE = "arcade";
static const char *NVS_KEY = "save";

static arcade_save_t s_save;
static nvs_handle_t s_handle;
static int s_nvs_ready;

void arcade_store_init(void)
{
    arcade_save_defaults(&s_save);

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS needs a format (%s), erasing", esp_err_to_name(err));
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_flash_init failed (%s); scores will not persist", esp_err_to_name(err));
        return;
    }
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &s_handle) != ESP_OK) {
        ESP_LOGE(TAG, "nvs_open failed; scores will not persist");
        return;
    }

    uint8_t blob[ARCADE_SAVE_BYTES];
    size_t length = sizeof(blob);
    esp_err_t read = nvs_get_blob(s_handle, NVS_KEY, blob, &length);
    if (read == ESP_OK && arcade_save_decode(&s_save, blob, length) == 0) {
        ESP_LOGI(TAG, "loaded save record (2048=%u snake=%u breakout=%u sound=%u)", (unsigned)s_save.best[0],
                 (unsigned)s_save.best[1], (unsigned)s_save.best[2], (unsigned)s_save.sound_on);
        s_nvs_ready = 1;
        return;
    }

    ESP_LOGW(TAG, "no valid save record (%s, %u bytes); using defaults", esp_err_to_name(read), (unsigned)length);
    s_nvs_ready = 1;
    arcade_store_commit();
}

arcade_save_t *arcade_store_data(void)
{
    return &s_save;
}

void arcade_store_commit(void)
{
    if (!s_nvs_ready) {
        return;
    }
    uint8_t blob[ARCADE_SAVE_BYTES];
    if (arcade_save_encode(&s_save, blob, sizeof(blob)) != ARCADE_SAVE_BYTES) {
        ESP_LOGE(TAG, "cannot encode save record");
        return;
    }
    esp_err_t err = nvs_set_blob(s_handle, NVS_KEY, blob, sizeof(blob));
    if (err == ESP_OK) {
        err = nvs_commit(s_handle);
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "cannot persist save record (%s)", esp_err_to_name(err));
    }
}
