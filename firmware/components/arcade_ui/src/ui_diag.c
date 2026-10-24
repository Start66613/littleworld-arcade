/* Little World Arcade - on-device diagnostics overlay.
 *
 * Shows live heap/PSRAM numbers and the LVGL timer tick rate. The tick rate is
 * the cheapest on-device proxy for "the UI task is not blocked": a game loop
 * that hogs the LVGL task makes it drop below the nominal 50 Hz.
 *
 * SPDX-License-Identifier: MIT
 */
#include "arcade/arcade_save.h"
#include "esp_heap_caps.h"
#include "esp_system.h"
#include "ui_internal.h"

#define DIAG_TICK_MS 20

static lv_obj_t *s_overlay;
static lv_obj_t *s_label;
static lv_timer_t *s_rate_timer;
static uint32_t s_ticks;
static uint32_t s_rate;

static void close_cb(lv_event_t *event)
{
    (void)event;
    arcade_diag_hide();
}

static void tick_cb(lv_timer_t *timer)
{
    (void)timer;
    s_ticks++;
}

static void refresh_label(void)
{
    if (s_label == NULL) {
        return;
    }
    lv_label_set_text_fmt(s_label,
                          "LVGL tick rate: %u /s  (nominal 50)\n"
                          "Internal free: %u KB\n"
                          "Internal min free: %u KB\n"
                          "PSRAM free: %u KB\n"
                          "Rounds played: %u / %u / %u\n"
                          "Font: Noto Sans SC subset (SIL OFL 1.1)",
                          (unsigned)s_rate, (unsigned)(esp_get_free_heap_size() / 1024),
                          (unsigned)(esp_get_minimum_free_heap_size() / 1024),
                          (unsigned)(heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024),
                          (unsigned)arcade_store_data()->plays[0],
                          (unsigned)arcade_store_data()->plays[1],
                          (unsigned)arcade_store_data()->plays[2]);
}

static void rate_cb(lv_timer_t *timer)
{
    (void)timer;
    s_rate = s_ticks;
    s_ticks = 0;
    if (s_overlay != NULL) {
        refresh_label();
    }
}

void arcade_diag_start_monitor(void)
{
    lv_timer_create(tick_cb, DIAG_TICK_MS, NULL);
    s_rate_timer = lv_timer_create(rate_cb, 1000, NULL);
    (void)s_rate_timer;
}

void arcade_diag_show(void)
{
    if (s_overlay != NULL) {
        return;
    }

    lv_obj_t *screen = lv_screen_active();
    s_overlay = lv_obj_create(screen);
    lv_obj_remove_style_all(s_overlay);
    lv_obj_set_size(s_overlay, 1024, 600);
    lv_obj_set_pos(s_overlay, 0, 0);
    lv_obj_set_style_bg_color(s_overlay, lv_color_hex(0x2A1E26), 0);
    lv_obj_set_style_bg_opa(s_overlay, LV_OPA_50, 0);
    lv_obj_add_flag(s_overlay, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_move_foreground(s_overlay);

    lv_obj_t *card = lv_obj_create(s_overlay);
    lv_obj_remove_style_all(card);
    arcade_style_card(card);
    lv_obj_set_size(card, 660, 400);
    lv_obj_center(card);

    lv_obj_t *title = lv_label_create(card);
    arcade_style_label(title, &arcade_font_28, ARC_TEXT);
    lv_label_set_text(title, "运行状态");
    lv_obj_set_pos(title, 32, 26);

    s_label = lv_label_create(card);
    arcade_style_label(s_label, &arcade_font_22, ARC_TEXT);
    lv_obj_set_pos(s_label, 32, 84);
    lv_obj_set_style_text_line_space(s_label, 10, 0);

    lv_obj_t *close = arcade_button(card, "关闭", 160, 52, ARC_PINK_DEEP, 0xFFFFFF, &arcade_font_22);
    lv_obj_align(close, LV_ALIGN_BOTTOM_MID, 0, -26);
    lv_obj_add_event_cb(close, close_cb, LV_EVENT_CLICKED, NULL);

    refresh_label();
}

void arcade_diag_hide(void)
{
    if (s_overlay != NULL) {
        lv_obj_delete(s_overlay);
        s_overlay = NULL;
        s_label = NULL;
    }
}
