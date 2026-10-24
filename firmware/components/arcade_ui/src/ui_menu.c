/* Little World Arcade - home menu and shell entry point.
 *
 * SPDX-License-Identifier: MIT
 */
#include "ui_internal.h"

#include "esp_log.h"

static const arcade_game_module_t *const s_games[] = {
    &arcade_game_2048,
    &arcade_game_snake,
    &arcade_game_breakout,
};

#define GAMES_COUNT ((int)(sizeof(s_games) / sizeof(s_games[0])))

/* --------------------------------------------------------------- primitives */

static lv_obj_t *mini_block(lv_obj_t *parent, int x, int y, int w, int h, uint32_t color, int radius)
{
    lv_obj_t *block = lv_obj_create(parent);
    lv_obj_remove_style_all(block);
    lv_obj_set_pos(block, x, y);
    lv_obj_set_size(block, w, h);
    lv_obj_set_style_bg_color(block, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(block, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(block, radius, 0);
    lv_obj_remove_flag(block, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(block, LV_OBJ_FLAG_SCROLLABLE);
    return block;
}

static lv_obj_t *mini_block_text(lv_obj_t *parent, int x, int y, int w, int h, uint32_t color, uint32_t text_color,
                                 const char *text)
{
    lv_obj_t *block = mini_block(parent, x, y, w, h, color, 8);
    lv_obj_t *label = lv_label_create(block);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, &arcade_font_22, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(text_color), 0);
    lv_obj_center(label);
    return block;
}

static void preview_2048(lv_obj_t *plate)
{
    static const uint16_t values[4] = {2, 4, 8, 16};
    static const uint32_t colors[4] = {0xFFF1DC, 0xFFE3C2, 0xFFCF9C, 0xFFB877};
    static const char *texts[4] = {"2", "4", "8", "16"};
    for (int i = 0; i < 4; i++) {
        int x = 81 + (i % 2) * 50;
        int y = 12 + (i / 2) * 50;
        mini_block_text(plate, x, y, 44, 44, colors[i], 0x8A5A2B, texts[i]);
        (void)values;
    }
}

static void preview_snake(lv_obj_t *plate)
{
    static const int path[7][2] = {{60, 72}, {82, 72}, {104, 72}, {126, 72}, {148, 72}, {148, 50}, {148, 28}};
    for (int i = 0; i < 7; i++) {
        uint32_t color = (i == 0) ? ARC_MINT_DEEP : ((i & 1) ? 0x8AD9B6 : 0xA9E6CC);
        mini_block(plate, path[i][0], path[i][1], 20, 20, color, 7);
    }
    mini_block(plate, 38, 44, 16, 16, ARC_PINK, 8);
    mini_block(plate, 34, 40, 6, 6, 0xFFFFFF, 3);
}

static void preview_breakout(lv_obj_t *plate)
{
    static const uint32_t row_colors[3] = {0xFFC2D4, 0xE7D6FF, 0xBFEBD8};
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 6; col++) {
            mini_block(plate, 50 + col * 26, 20 + row * 14, 22, 8, row_colors[row], 4);
        }
    }
    mini_block(plate, 93, 96, 70, 8, ARC_PINK_DEEP, 4);
    mini_block(plate, 124, 76, 10, 10, 0xFFFFFF, 5);
}

/* -------------------------------------------------------------------- menu */

static void sound_clicked(lv_event_t *event)
{
    lv_obj_t *button = lv_event_get_user_data(event);
    arcade_sound_toggle();
    arcade_sound_refresh_icon(button);
}

static void info_clicked(lv_event_t *event)
{
    (void)event;
    arcade_diag_show();
}

static void card_clicked(lv_event_t *event)
{
    const arcade_game_module_t *module = lv_event_get_user_data(event);
    if (module != NULL) {
        arcade_nav_game(module);
    }
}

static void build_card(lv_obj_t *parent, const arcade_game_module_t *module, int x, unsigned best)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_remove_style_all(card);
    arcade_style_card(card);
    lv_obj_set_pos(card, x, 164);
    lv_obj_set_size(card, 296, 330);
    lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(card, card_clicked, LV_EVENT_CLICKED, (void *)module);

    lv_obj_t *plate = mini_block(card, 20, 20, 256, 118, module->soft, 20);
    if (module->id == ARCADE_GAME_2048) {
        preview_2048(plate);
    } else if (module->id == ARCADE_GAME_SNAKE) {
        preview_snake(plate);
    } else {
        preview_breakout(plate);
    }

    lv_obj_t *title = lv_label_create(card);
    arcade_style_label(title, &arcade_font_28, ARC_TEXT);
    lv_label_set_text(title, module->title);
    lv_obj_set_pos(title, 26, 154);

    lv_obj_t *sub = lv_label_create(card);
    arcade_style_label(sub, &arcade_font_22, ARC_TEXT_MUTED);
    lv_label_set_text(sub, module->subtitle);
    lv_obj_set_pos(sub, 26, 196);

    lv_obj_t *best_label = lv_label_create(card);
    arcade_style_label(best_label, &arcade_font_22, ARC_TEXT_MUTED);
    lv_label_set_text_fmt(best_label, "最高分 %u", best);
    lv_obj_set_pos(best_label, 26, 236);

    lv_obj_t *pill = mini_block(card, 26, 268, 150, 44, module->accent, LV_RADIUS_CIRCLE);
    lv_obj_t *pill_label = lv_label_create(pill);
    lv_label_set_text(pill_label, "开始游戏");
    lv_obj_set_style_text_font(pill_label, &arcade_font_22, 0);
    lv_obj_set_style_text_color(pill_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(pill_label);
}

void arcade_nav_menu(void)
{
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_remove_style_all(screen);
    lv_obj_set_style_bg_color(screen, lv_color_hex(ARC_BG_TOP), 0);
    lv_obj_set_style_bg_grad_color(screen, lv_color_hex(ARC_BG_BOTTOM), 0);
    lv_obj_set_style_bg_grad_dir(screen, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_text_font(screen, &arcade_font_22, 0);
    lv_obj_set_style_text_color(screen, lv_color_hex(ARC_TEXT), 0);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(screen);
    arcade_style_label(title, &arcade_font_44, ARC_TEXT);
    lv_label_set_text(title, "Little World Arcade");
    lv_obj_set_pos(title, 44, 26);

    lv_obj_t *subtitle = lv_label_create(screen);
    arcade_style_label(subtitle, &arcade_font_22, ARC_TEXT_MUTED);
    lv_label_set_text(subtitle, "小世界 · 游戏厅");
    lv_obj_set_pos(subtitle, 46, 92);

    lv_obj_t *sound = arcade_icon_button(screen, LV_SYMBOL_VOLUME_MAX, 52, 0xFFFFFF, ARC_PINK_DEEP);
    lv_obj_set_pos(sound, 928, 38);
    lv_obj_add_event_cb(sound, sound_clicked, LV_EVENT_CLICKED, sound);
    arcade_sound_refresh_icon(sound);

    lv_obj_t *info = arcade_icon_button(screen, LV_SYMBOL_SETTINGS, 52, 0xFFFFFF, ARC_LAVENDER);
    lv_obj_set_pos(info, 858, 38);
    lv_obj_add_event_cb(info, info_clicked, LV_EVENT_CLICKED, NULL);

    arcade_save_t *save = arcade_store_data();
    for (int i = 0; i < GAMES_COUNT; i++) {
        build_card(screen, s_games[i], 44 + i * 320, save->best[s_games[i]->id]);
    }

    lv_obj_t *hint = lv_label_create(screen);
    arcade_style_label(hint, &arcade_font_22, ARC_TEXT_MUTED);
    lv_label_set_text(hint, "轻触卡片开始游戏");
    lv_obj_align(hint, LV_ALIGN_TOP_MID, 0, 522);

    lv_obj_t *version = lv_label_create(screen);
    arcade_style_label(version, &arcade_font_22, 0xB9A7B2);
    lv_label_set_text(version, "Little World Arcade v0.1 · 实验固件");
    lv_obj_align(version, LV_ALIGN_TOP_MID, 0, 558);

    lv_screen_load_anim(screen, LV_SCREEN_LOAD_ANIM_FADE_IN, 180, 0, true);
}

void arcade_ui_start(void)
{
    ESP_LOGI("arcade/ui", "ui: theme + store + diagnostics");
    arcade_theme_init();
    arcade_store_init();
    arcade_diag_start_monitor();
    ESP_LOGI("arcade/ui", "ui: building the home menu");
    arcade_nav_menu();
    ESP_LOGI("arcade/ui", "ui: home menu loaded");
}
