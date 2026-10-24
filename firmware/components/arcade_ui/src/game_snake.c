/* Little World Arcade - snake screen.
 *
 * Swipe anywhere on the field or use the arrow keys in the header. The step
 * interval comes from arcade_core and shrinks as the snake grows.
 *
 * SPDX-License-Identifier: MIT
 */
#include "esp_random.h"
#include <stdio.h>
#include "ui_internal.h"

#define SNAKE_CELL_PX 36
#define SEGMENT_PX    32

static arcade_snake_t s_model;
static arcade_rng_t s_rng;
static lv_obj_t *s_playfield;
static lv_obj_t *s_segments[ARCADE_SNAKE_MAX_LEN];
static int s_segment_count;
static lv_obj_t *s_food;
static lv_timer_t *s_timer;
static const arcade_game_host_t *s_host;
static int s_paused;
static int s_over;

static void position_food(void)
{
    lv_obj_set_pos(s_food, s_model.food.x * SNAKE_CELL_PX + 4, s_model.food.y * SNAKE_CELL_PX + 4);
}

static void build_segment(int index)
{
    lv_obj_t *seg = lv_obj_create(s_playfield);
    lv_obj_remove_style_all(seg);
    lv_obj_set_size(seg, SEGMENT_PX, SEGMENT_PX);
    lv_obj_set_style_radius(seg, 9, 0);
    lv_obj_set_style_bg_opa(seg, LV_OPA_COVER, 0);
    lv_obj_remove_flag(seg, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(seg, LV_OBJ_FLAG_SCROLLABLE);
    s_segments[index] = seg;
    s_segment_count = index + 1;
}

static void refresh_view(void)
{
    for (int i = 0; i < s_model.length; i++) {
        if (i >= s_segment_count) {
            build_segment(i);
        }
        uint32_t color;
        if (i == 0) {
            color = ARC_MINT_DEEP;
        } else {
            color = (i & 1) ? 0x8AD9B6 : 0xA9E6CC;
        }
        lv_obj_set_style_bg_color(s_segments[i], lv_color_hex(color), 0);
        lv_obj_set_pos(s_segments[i], s_model.body[i].x * SNAKE_CELL_PX + 2,
                       s_model.body[i].y * SNAKE_CELL_PX + 2);
        lv_obj_remove_flag(s_segments[i], LV_OBJ_FLAG_HIDDEN);
    }
    for (int i = s_model.length; i < s_segment_count; i++) {
        lv_obj_add_flag(s_segments[i], LV_OBJ_FLAG_HIDDEN);
    }
    position_food();
}

static void step_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    if (s_paused || s_over) {
        return;
    }

    int events = arcade_snake_step(&s_model, &s_rng);

    if (events & ARCADE_SNAKE_EVENT_DIED) {
        s_over = 1;
        s_host->game_over(s_model.score, 0);
        return;
    }
    if (events & ARCADE_SNAKE_EVENT_WON) {
        s_over = 1;
        s_host->game_over(s_model.score, 1);
        return;
    }
    if (events & ARCADE_SNAKE_EVENT_MOVED) {
        refresh_view();
    }
    if (events & ARCADE_SNAKE_EVENT_ATE) {
        s_host->set_score(s_model.score);
        char text[24];
        snprintf(text, sizeof(text), "长度 %d", (int)s_model.length);
        s_host->set_extra(text);
        lv_timer_set_period(s_timer, arcade_snake_step_ms(&s_model));
    }
}

static void swipe_cb(arcade_dir_t dir, void *user)
{
    (void)user;
    if (!s_paused && !s_over) {
        (void)arcade_snake_turn(&s_model, dir);
    }
}

static void turn_clicked(lv_event_t *event)
{
    arcade_dir_t dir = (arcade_dir_t)(uintptr_t)lv_event_get_user_data(event);
    if (!s_paused && !s_over) {
        (void)arcade_snake_turn(&s_model, dir);
    }
}

static void build_controls(lv_obj_t *bar)
{
    static const char *const symbols[4] = {LV_SYMBOL_LEFT, LV_SYMBOL_UP, LV_SYMBOL_DOWN, LV_SYMBOL_RIGHT};
    static const arcade_dir_t dirs[4] = {ARCADE_DIR_LEFT, ARCADE_DIR_UP, ARCADE_DIR_DOWN, ARCADE_DIR_RIGHT};
    for (int i = 0; i < 4; i++) {
        lv_obj_t *button = arcade_icon_button(bar, symbols[i], 44, 0xFFFFFF, ARC_MINT_DEEP);
        lv_obj_set_pos(button, 186 + i * 48, 20);
        lv_obj_add_event_cb(button, turn_clicked, LV_EVENT_CLICKED, (void *)(uintptr_t)dirs[i]);
    }
}

static void create(lv_obj_t *playfield, const arcade_game_host_t *host)
{
    s_host = host;
    s_playfield = playfield;
    s_paused = 0;
    s_over = 0;
    s_segment_count = 0;
    arcade_rng_seed(&s_rng, esp_random());
    arcade_snake_init(&s_model, &s_rng);

    s_food = lv_obj_create(playfield);
    lv_obj_remove_style_all(s_food);
    lv_obj_set_size(s_food, 28, 28);
    lv_obj_set_style_radius(s_food, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(s_food, lv_color_hex(ARC_PINK_DEEP), 0);
    lv_obj_set_style_bg_opa(s_food, LV_OPA_COVER, 0);
    lv_obj_remove_flag(s_food, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(s_food, LV_OBJ_FLAG_SCROLLABLE);

    refresh_view();

    arcade_attach_swipe(playfield, 40, swipe_cb, NULL);
    s_timer = lv_timer_create(step_timer_cb, arcade_snake_step_ms(&s_model), NULL);

    host->set_score(0);
    host->set_extra("长度 3");
}

static void destroy(void)
{
    if (s_timer != NULL) {
        lv_timer_delete(s_timer);
        s_timer = NULL;
    }
    s_host = NULL;
    s_playfield = NULL;
    s_segment_count = 0;
}

static void set_paused(int paused)
{
    s_paused = paused;
    if (s_timer == NULL) {
        return;
    }
    if (paused) {
        lv_timer_pause(s_timer);
    } else {
        lv_timer_resume(s_timer);
    }
}

const arcade_game_module_t arcade_game_snake = {
    .id = ARCADE_GAME_SNAKE,
    .title = "贪吃蛇",
    .subtitle = "越吃越快的小蛇",
    .hint = "滑动屏幕或在右上角点方向键，撞墙和撞到自己都会结束",
    .accent = ARC_MINT,
    .soft = 0xEAF9F1,
    .create = create,
    .destroy = destroy,
    .set_paused = set_paused,
    .build_controls = build_controls,
};
