/* Little World Arcade - 2048 screen.
 *
 * The rules live in arcade_core; this file only maps the board to LVGL objects
 * and turns swipe gestures into moves.
 *
 * SPDX-License-Identifier: MIT
 */
#include "esp_random.h"
#include "ui_internal.h"

#define BOARD_PX 450
#define CELL_PX  100
#define GAP_PX   10

static arcade_2048_t s_model;
static arcade_rng_t s_rng;
static lv_obj_t *s_tiles[ARCADE_2048_CELLS];
static lv_obj_t *s_labels[ARCADE_2048_CELLS];
static const arcade_game_host_t *s_host;
static int s_paused;
static int s_win_shown;

static uint32_t tile_bg(uint16_t value)
{
    switch (value) {
    case 2:
        return 0xFFF4E3;
    case 4:
        return 0xFFE7C6;
    case 8:
        return 0xFFD3A0;
    case 16:
        return 0xFFB877;
    case 32:
        return 0xFF9E76;
    case 64:
        return 0xF97F63;
    case 128:
        return 0xF3C96B;
    case 256:
        return 0xEFBC55;
    case 512:
        return 0xE9AD3F;
    case 1024:
        return 0xE09A2C;
    case 2048:
        return 0xD98B2B;
    default:
        return 0x8F6BC4;
    }
}

static uint32_t tile_fg(uint16_t value)
{
    return value <= 4 ? 0x8A5A2B : 0xFFFFFF;
}

static void refresh_view(void)
{
    for (int i = 0; i < ARCADE_2048_CELLS; i++) {
        uint16_t value = s_model.cells[i];
        if (value == 0) {
            lv_obj_add_flag(s_tiles[i], LV_OBJ_FLAG_HIDDEN);
            continue;
        }
        lv_obj_remove_flag(s_tiles[i], LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_bg_color(s_tiles[i], lv_color_hex(tile_bg(value)), 0);
        lv_obj_set_style_text_color(s_labels[i], lv_color_hex(tile_fg(value)), 0);
        lv_obj_set_style_text_font(s_labels[i], value < 100 ? &arcade_font_44 : &arcade_font_28, 0);
        lv_label_set_text_fmt(s_labels[i], "%u", (unsigned)value);
    }
}

static void swipe_cb(arcade_dir_t dir, void *user)
{
    (void)user;
    if (s_paused || s_host == NULL || s_model.over) {
        return;
    }

    arcade_2048_result_t result = arcade_2048_move(&s_model, dir, &s_rng);
    if (!result.moved) {
        return;
    }
    refresh_view();
    s_host->set_score(s_model.score);

    if (s_model.won && !s_win_shown) {
        s_win_shown = 1;
        s_host->game_over(s_model.score, 1);
        return;
    }
    if (s_model.over) {
        s_host->game_over(s_model.score, 0);
    }
}

static void create(lv_obj_t *playfield, const arcade_game_host_t *host)
{
    s_host = host;
    s_paused = 0;
    s_win_shown = 0;
    arcade_rng_seed(&s_rng, esp_random());
    arcade_2048_init(&s_model, &s_rng);

    lv_obj_t *board = lv_obj_create(playfield);
    lv_obj_remove_style_all(board);
    lv_obj_set_size(board, BOARD_PX, BOARD_PX);
    lv_obj_center(board);
    lv_obj_set_style_bg_color(board, lv_color_hex(0xF2DFC9), 0);
    lv_obj_set_style_bg_opa(board, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(board, 20, 0);
    lv_obj_remove_flag(board, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(board, LV_OBJ_FLAG_SCROLLABLE);

    for (int i = 0; i < ARCADE_2048_CELLS; i++) {
        lv_obj_t *tile = lv_obj_create(board);
        lv_obj_remove_style_all(tile);
        lv_obj_set_size(tile, CELL_PX, CELL_PX);
        lv_obj_set_pos(tile, GAP_PX + (i % ARCADE_2048_N) * (CELL_PX + GAP_PX),
                       GAP_PX + (i / ARCADE_2048_N) * (CELL_PX + GAP_PX));
        lv_obj_set_style_radius(tile, 14, 0);
        lv_obj_set_style_bg_opa(tile, LV_OPA_COVER, 0);
        lv_obj_remove_flag(tile, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_remove_flag(tile, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *label = lv_label_create(tile);
        arcade_style_label(label, &arcade_font_44, 0xFFFFFF);
        lv_obj_center(label);

        s_tiles[i] = tile;
        s_labels[i] = label;
    }

    arcade_attach_swipe(playfield, 40, swipe_cb, NULL);

    host->set_score(0);
    host->set_extra("目标 2048");
    refresh_view();
}

static void destroy(void)
{
    s_host = NULL;
}

static void set_paused(int paused)
{
    s_paused = paused;
}

const arcade_game_module_t arcade_game_2048 = {
    .id = ARCADE_GAME_2048,
    .title = "2048",
    .subtitle = "滑动合并数字",
    .hint = "在棋盘上向任意方向滑动，两个相同数字合成一个",
    .accent = ARC_GOLD,
    .soft = 0xFFF3E2,
    .create = create,
    .destroy = destroy,
    .set_paused = set_paused,
    .build_controls = NULL,
};
