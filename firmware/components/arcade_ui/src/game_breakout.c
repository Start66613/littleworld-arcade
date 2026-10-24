/* Little World Arcade - breakout screen.
 *
 * The ball runs on a fixed 16 ms simulation tick from arcade_core, so the game
 * speed does not depend on the render frame rate. Dragging anywhere on the
 * field moves the paddle; the first tap also launches the ball.
 *
 * SPDX-License-Identifier: MIT
 */
#include <stdio.h>

#include "ui_internal.h"

static arcade_breakout_t s_model;
static lv_obj_t *s_bricks[ARCADE_BREAKOUT_ROWS][ARCADE_BREAKOUT_COLS];
static lv_obj_t *s_paddle;
static lv_obj_t *s_ball;
static lv_obj_t *s_playfield;
static lv_timer_t *s_timer;
static const arcade_game_host_t *s_host;
static int s_paused;
static int s_over;
static uint8_t s_last_lives;
static uint32_t s_last_score;
static uint32_t s_last_tick_ms;

static uint32_t row_color(int row)
{
    static const uint32_t colors[ARCADE_BREAKOUT_ROWS] = {0xFFC2D4, 0xFFD9C2, 0xE7D6FF,
                                                          0xD6E4FF, 0xBFEBD8, 0xF7E9A8};
    return colors[row % ARCADE_BREAKOUT_ROWS];
}

static void refresh_view(void)
{
    for (int row = 0; row < ARCADE_BREAKOUT_ROWS; row++) {
        for (int col = 0; col < ARCADE_BREAKOUT_COLS; col++) {
            if (s_model.brick[row][col]) {
                lv_obj_remove_flag(s_bricks[row][col], LV_OBJ_FLAG_HIDDEN);
            } else {
                lv_obj_add_flag(s_bricks[row][col], LV_OBJ_FLAG_HIDDEN);
            }
        }
    }
    lv_obj_set_pos(s_paddle, (int)(s_model.paddle_x >> 8), ARCADE_BREAKOUT_PADDLE_Y);
    lv_obj_set_pos(s_ball, (int)(s_model.x >> 8) - ARCADE_BREAKOUT_BALL_R,
                   (int)(s_model.y >> 8) - ARCADE_BREAKOUT_BALL_R);
}

static void update_lives_hud(void)
{
    char text[24];
    snprintf(text, sizeof(text), "生命 %u", (unsigned)s_model.lives);
    s_host->set_extra(text);
}

static void tick_cb(lv_timer_t *timer)
{
    (void)timer;
    if (s_paused || s_over) {
        return;
    }

    /* LVGL runs the timer once per handler pass, so a slow frame would otherwise
     * slow the ball down. Advance as many fixed 16 ms steps as actually elapsed
     * (capped so a long stall cannot teleport the ball through bricks). */
    uint32_t now = lv_tick_get();
    uint32_t elapsed = now - s_last_tick_ms;
    if (elapsed < ARCADE_BREAKOUT_TICK_MS) {
        return;
    }
    uint32_t steps = elapsed / ARCADE_BREAKOUT_TICK_MS;
    if (steps > 3) {
        steps = 3;
    }
    s_last_tick_ms += steps * ARCADE_BREAKOUT_TICK_MS;

    for (uint32_t i = 0; i < steps; i++) {
        arcade_breakout_tick(&s_model);
    }
    refresh_view();

    if (s_model.lives != s_last_lives) {
        s_last_lives = s_model.lives;
        update_lives_hud();
    }
    if (s_model.score != s_last_score) {
        s_last_score = s_model.score;
        s_host->set_score(s_model.score);
    }

    if (s_model.state == ARCADE_STATE_WON) {
        s_over = 1;
        s_host->game_over(s_model.score, 1);
    } else if (s_model.state == ARCADE_STATE_OVER) {
        s_over = 1;
        s_host->game_over(s_model.score, 0);
    }
}

static void pointer_cb(lv_event_t *event)
{
    (void)event;
    if (s_over) {
        return;
    }
    int x = arcade_pointer_x_in(s_playfield);
    if (x < 0) {
        x = 0;
    }
    if (x > ARCADE_BREAKOUT_W) {
        x = ARCADE_BREAKOUT_W;
    }
    arcade_breakout_move_paddle(&s_model, x * ARCADE_FP_ONE);
    if (s_model.state == ARCADE_STATE_READY) {
        arcade_breakout_launch(&s_model);
    }
    refresh_view();
}

static void create(lv_obj_t *playfield, const arcade_game_host_t *host)
{
    s_host = host;
    s_playfield = playfield;
    s_paused = 0;
    s_over = 0;
    arcade_breakout_init(&s_model);
    s_last_lives = s_model.lives;
    s_last_score = s_model.score;

    for (int row = 0; row < ARCADE_BREAKOUT_ROWS; row++) {
        for (int col = 0; col < ARCADE_BREAKOUT_COLS; col++) {
            int x, y, w, h;
            arcade_breakout_brick_rect(row, col, &x, &y, &w, &h);
            lv_obj_t *brick = lv_obj_create(playfield);
            lv_obj_remove_style_all(brick);
            lv_obj_set_pos(brick, x, y);
            lv_obj_set_size(brick, w, h);
            lv_obj_set_style_radius(brick, 8, 0);
            lv_obj_set_style_bg_color(brick, lv_color_hex(row_color(row)), 0);
            lv_obj_set_style_bg_opa(brick, LV_OPA_COVER, 0);
            lv_obj_remove_flag(brick, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_remove_flag(brick, LV_OBJ_FLAG_SCROLLABLE);
            s_bricks[row][col] = brick;
        }
    }

    s_paddle = lv_obj_create(playfield);
    lv_obj_remove_style_all(s_paddle);
    lv_obj_set_size(s_paddle, ARCADE_BREAKOUT_PADDLE_W, ARCADE_BREAKOUT_PADDLE_H);
    lv_obj_set_style_radius(s_paddle, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(s_paddle, lv_color_hex(ARC_PINK_DEEP), 0);
    lv_obj_set_style_bg_opa(s_paddle, LV_OPA_COVER, 0);
    lv_obj_remove_flag(s_paddle, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(s_paddle, LV_OBJ_FLAG_SCROLLABLE);

    s_ball = lv_obj_create(playfield);
    lv_obj_remove_style_all(s_ball);
    lv_obj_set_size(s_ball, ARCADE_BREAKOUT_BALL_R * 2, ARCADE_BREAKOUT_BALL_R * 2);
    lv_obj_set_style_radius(s_ball, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(s_ball, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(s_ball, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_ball, 2, 0);
    lv_obj_set_style_border_color(s_ball, lv_color_hex(ARC_PINK_DEEP), 0);
    lv_obj_remove_flag(s_ball, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(s_ball, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(playfield, pointer_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(playfield, pointer_cb, LV_EVENT_PRESSING, NULL);

    refresh_view();
    s_last_tick_ms = lv_tick_get();
    s_timer = lv_timer_create(tick_cb, ARCADE_BREAKOUT_TICK_MS / 2, NULL);

    host->set_score(0);
    update_lives_hud();
}

static void destroy(void)
{
    if (s_timer != NULL) {
        lv_timer_delete(s_timer);
        s_timer = NULL;
    }
    s_host = NULL;
    s_playfield = NULL;
}

static void set_paused(int paused)
{
    s_paused = paused;
    if (!paused) {
        s_last_tick_ms = lv_tick_get(); /* do not catch up on paused time */
    }
    if (s_timer == NULL) {
        return;
    }
    if (paused) {
        lv_timer_pause(s_timer);
    } else {
        lv_timer_resume(s_timer);
    }
}

const arcade_game_module_t arcade_game_breakout = {
    .id = ARCADE_GAME_BREAKOUT,
    .title = "打砖块",
    .subtitle = "接住小球，打光砖块",
    .hint = "在屏幕上拖动接住小球，轻触发球，掉球会损失一条命",
    .accent = ARC_PINK,
    .soft = 0xFFF0F4,
    .create = create,
    .destroy = destroy,
    .set_paused = set_paused,
    .build_controls = NULL,
};
