/* Little World Arcade - host unit tests for the LVGL-free game core.
 *
 * Build & run with tools/run_host_tests.ps1 (MSVC x64).
 * Exit code 0 = all checks passed.
 *
 * SPDX-License-Identifier: MIT
 */
#include <stdio.h>
#include <string.h>

#include "arcade/arcade_2048.h"
#include "arcade/arcade_breakout.h"
#include "arcade/arcade_common.h"
#include "arcade/arcade_save.h"
#include "arcade/arcade_snake.h"

static int g_failures;
static int g_checks;

#define CHECK(cond)                                                       \
    do {                                                                  \
        g_checks++;                                                       \
        if (!(cond)) {                                                    \
            printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);        \
            g_failures++;                                                 \
        }                                                                 \
    } while (0)

#define CHECK_EQ_INT(actual, expected)                                                       \
    do {                                                                                     \
        g_checks++;                                                                          \
        long _a = (long)(actual);                                                            \
        long _e = (long)(expected);                                                          \
        if (_a != _e) {                                                                      \
            printf("FAIL %s:%d  %s == %s (actual %ld, expected %ld)\n", __FILE__, __LINE__,   \
                   #actual, #expected, _a, _e);                                              \
            g_failures++;                                                                    \
        }                                                                                    \
    } while (0)

/* ------------------------------------------------------------------- rng */

static void test_rng(void)
{
    arcade_rng_t a, b;
    arcade_rng_seed(&a, 12345u);
    arcade_rng_seed(&b, 12345u);
    for (int i = 0; i < 64; i++) {
        CHECK(arcade_rng_next(&a) == arcade_rng_next(&b));
    }

    arcade_rng_seed(&a, 0u); /* must not stick at zero */
    uint32_t v = 0;
    for (int i = 0; i < 8; i++) {
        v |= arcade_rng_next(&a);
    }
    CHECK(v != 0u);

    arcade_rng_seed(&a, 7u);
    for (int i = 0; i < 500; i++) {
        int32_t r = arcade_rng_below(&a, 10);
        CHECK(r >= 0 && r < 10);
    }
    CHECK_EQ_INT(arcade_rng_below(&a, 0), 0);
}

/* ------------------------------------------------------------------ 2048 */

static void board_2048(arcade_2048_t *g, const uint16_t cells[16])
{
    memset(g, 0, sizeof(*g));
    memcpy(g->cells, cells, sizeof(g->cells));
}

/* Remove the randomly spawned tile so the moved board can be compared. */
static void strip_spawn_2048(arcade_2048_t *g, const arcade_2048_result_t *r)
{
    if (r->spawn_index >= 0 && r->spawn_index < ARCADE_2048_CELLS) {
        g->cells[r->spawn_index] = 0;
    }
}

static void test_2048(void)
{
    arcade_rng_t rng;
    arcade_rng_seed(&rng, 42u);

    arcade_2048_t g;
    const uint16_t four[16] = {2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    board_2048(&g, four);
    arcade_2048_result_t r = arcade_2048_move(&g, ARCADE_DIR_LEFT, &rng);
    CHECK_EQ_INT(r.moved, 1);
    CHECK_EQ_INT(r.gained, 4);
    strip_spawn_2048(&g, &r);
    CHECK_EQ_INT(g.cells[0], 4);
    CHECK_EQ_INT(g.score, 4);
    CHECK_EQ_INT(g.moves, 1);

    /* 2 2 2 0 -> 4 2 (only one merge per tile and per move) */
    const uint16_t three[16] = {2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    board_2048(&g, three);
    r = arcade_2048_move(&g, ARCADE_DIR_LEFT, &rng);
    CHECK_EQ_INT(r.gained, 4);
    strip_spawn_2048(&g, &r);
    CHECK_EQ_INT(g.cells[0], 4);
    CHECK_EQ_INT(g.cells[1], 2);
    CHECK_EQ_INT(g.cells[2], 0);

    /* 2 2 2 2 -> 4 4 */
    const uint16_t all_two[16] = {2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    board_2048(&g, all_two);
    r = arcade_2048_move(&g, ARCADE_DIR_LEFT, &rng);
    CHECK_EQ_INT(r.gained, 8);
    strip_spawn_2048(&g, &r);
    CHECK_EQ_INT(g.cells[0], 4);
    CHECK_EQ_INT(g.cells[1], 4);
    CHECK_EQ_INT(g.cells[2], 0);

    /* 4 4 8 8 -> 8 16 */
    const uint16_t mixed[16] = {4, 4, 8, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    board_2048(&g, mixed);
    r = arcade_2048_move(&g, ARCADE_DIR_LEFT, &rng);
    CHECK_EQ_INT(r.gained, 24);
    strip_spawn_2048(&g, &r);
    CHECK_EQ_INT(g.cells[0], 8);
    CHECK_EQ_INT(g.cells[1], 16);

    /* no move: 2 4 2 4 */
    const uint16_t stuck[16] = {2, 4, 2, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    board_2048(&g, stuck);
    r = arcade_2048_move(&g, ARCADE_DIR_LEFT, &rng);
    CHECK_EQ_INT(r.moved, 0);
    CHECK_EQ_INT(r.spawn_index, -1);
    CHECK_EQ_INT(g.score, 0);

    /* right: 0 0 2 2 -> 0 0 0 4 */
    const uint16_t right_row[16] = {0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    board_2048(&g, right_row);
    r = arcade_2048_move(&g, ARCADE_DIR_RIGHT, &rng);
    strip_spawn_2048(&g, &r);
    CHECK_EQ_INT(g.cells[3], 4);
    CHECK_EQ_INT(g.cells[2], 0);

    /* down: column with 2 2 0 0 -> 0 0 0 4 at the bottom */
    const uint16_t down_col[16] = {2, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    board_2048(&g, down_col);
    r = arcade_2048_move(&g, ARCADE_DIR_DOWN, &rng);
    strip_spawn_2048(&g, &r);
    CHECK_EQ_INT(g.cells[12], 4);
    CHECK_EQ_INT(g.cells[0], 0);

    /* up: same column moves to the top */
    board_2048(&g, down_col);
    r = arcade_2048_move(&g, ARCADE_DIR_UP, &rng);
    strip_spawn_2048(&g, &r);
    CHECK_EQ_INT(g.cells[0], 4);

    /* win: 1024 + 1024 -> 2048 */
    const uint16_t near_win[16] = {1024, 1024, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    board_2048(&g, near_win);
    r = arcade_2048_move(&g, ARCADE_DIR_LEFT, &rng);
    CHECK_EQ_INT(g.cells[0], 2048);
    CHECK_EQ_INT(r.merged_mask & 1u, 1u);
    CHECK_EQ_INT(g.won, 1);
    CHECK_EQ_INT(arcade_2048_max_tile(&g), 2048);

    /* game over: full board without merges */
    const uint16_t dead[16] = {2, 4, 2, 4, 4, 2, 4, 2, 2, 4, 2, 4, 4, 2, 4, 2};
    board_2048(&g, dead);
    CHECK_EQ_INT(arcade_2048_can_move(&g), 0);
    r = arcade_2048_move(&g, ARCADE_DIR_LEFT, &rng);
    CHECK_EQ_INT(r.moved, 0);
    CHECK_EQ_INT(g.over, 1);

    /* init places exactly two tiles */
    arcade_2048_t fresh;
    arcade_2048_init(&fresh, &rng);
    int non_zero = 0;
    for (int i = 0; i < ARCADE_2048_CELLS; i++) {
        if (fresh.cells[i] != 0) {
            non_zero++;
        }
    }
    CHECK_EQ_INT(non_zero, 2);
    CHECK_EQ_INT(fresh.score, 0);
    CHECK_EQ_INT(fresh.over, 0);
}

/* ----------------------------------------------------------------- snake */

static void test_snake(void)
{
    arcade_rng_t rng;
    arcade_rng_seed(&rng, 99u);
    arcade_snake_t s;
    arcade_snake_init(&s, &rng);

    CHECK_EQ_INT(s.length, 3);
    CHECK_EQ_INT(s.body[0].x, ARCADE_SNAKE_COLS / 2);
    CHECK_EQ_INT(s.dir, ARCADE_DIR_RIGHT);
    CHECK(s.food.x >= 0 && s.food.x < ARCADE_SNAKE_COLS);
    CHECK(s.food.y >= 0 && s.food.y < ARCADE_SNAKE_ROWS);
    CHECK(s.food.x != s.body[0].x || s.food.y != s.body[0].y);

    /* reversed turns are rejected */
    CHECK_EQ_INT(arcade_snake_turn(&s, ARCADE_DIR_LEFT), 0);
    CHECK_EQ_INT(arcade_snake_turn(&s, ARCADE_DIR_RIGHT), 0);
    CHECK_EQ_INT(arcade_snake_turn(&s, ARCADE_DIR_UP), 1);
    CHECK_EQ_INT(arcade_snake_turn(&s, ARCADE_DIR_DOWN), 0); /* opposite of queued UP */
    CHECK_EQ_INT(arcade_snake_turn(&s, ARCADE_DIR_LEFT), 1);
    CHECK_EQ_INT(arcade_snake_turn(&s, ARCADE_DIR_DOWN), 0); /* queue is full */

    /* one step moves the head, keeps the length */
    arcade_snake_init(&s, &rng);
    s.food.x = 0;
    s.food.y = 0;
    int head_x = s.body[0].x;
    int events = arcade_snake_step(&s, &rng);
    CHECK_EQ_INT(events & ARCADE_SNAKE_EVENT_MOVED, ARCADE_SNAKE_EVENT_MOVED);
    CHECK_EQ_INT(s.body[0].x, head_x + 1);
    CHECK_EQ_INT(s.length, 3);

    /* eating grows the snake, scores and places new food */
    arcade_snake_init(&s, &rng);
    s.food.x = (int16_t)(s.body[0].x + 1);
    s.food.y = s.body[0].y;
    events = arcade_snake_step(&s, &rng);
    CHECK_EQ_INT(events & ARCADE_SNAKE_EVENT_ATE, ARCADE_SNAKE_EVENT_ATE);
    CHECK_EQ_INT(s.length, 4);
    CHECK_EQ_INT(s.score, 10);
    CHECK_EQ_INT(s.foods_eaten, 1);
    CHECK(s.food.x != s.body[0].x || s.food.y != s.body[0].y);

    /* speed ramps down but never below the floor */
    uint32_t previous = arcade_snake_step_ms(&s);
    for (int i = 0; i < 40; i++) {
        s.foods_eaten++;
        uint32_t current = arcade_snake_step_ms(&s);
        CHECK(current <= previous);
        previous = current;
    }
    CHECK_EQ_INT(previous, ARCADE_SNAKE_MIN_MS);

    /* walking into the right wall kills the snake */
    arcade_snake_init(&s, &rng);
    s.body[0].x = (int16_t)(ARCADE_SNAKE_COLS - 1);
    s.body[1].x = (int16_t)(ARCADE_SNAKE_COLS - 2);
    s.body[2].x = (int16_t)(ARCADE_SNAKE_COLS - 3);
    s.dir = ARCADE_DIR_RIGHT;
    events = arcade_snake_step(&s, &rng);
    CHECK_EQ_INT(events, ARCADE_SNAKE_EVENT_DIED);
    CHECK_EQ_INT(s.alive, 0);
    CHECK_EQ_INT(arcade_snake_step(&s, &rng), ARCADE_SNAKE_EVENT_NONE);

    /* tail chasing is legal when the tail moves away */
    arcade_snake_init(&s, &rng);
    s.length = 4;
    s.body[0].x = 5;
    s.body[0].y = 4;
    s.body[1].x = 5;
    s.body[1].y = 5;
    s.body[2].x = 4;
    s.body[2].y = 5;
    s.body[3].x = 4;
    s.body[3].y = 4;
    s.dir = ARCADE_DIR_LEFT;
    s.food.x = 20;
    s.food.y = 10;
    events = arcade_snake_step(&s, &rng);
    CHECK_EQ_INT(events, ARCADE_SNAKE_EVENT_MOVED);
    CHECK_EQ_INT(s.body[0].x, 4);
    CHECK_EQ_INT(s.body[0].y, 4);

    /* biting the body dies */
    arcade_snake_init(&s, &rng);
    s.length = 5;
    s.body[0].x = 5;
    s.body[0].y = 4;
    s.body[1].x = 5;
    s.body[1].y = 5;
    s.body[2].x = 4;
    s.body[2].y = 5;
    s.body[3].x = 4;
    s.body[3].y = 4;
    s.body[4].x = 3;
    s.body[4].y = 4;
    s.dir = ARCADE_DIR_LEFT;
    s.food.x = 20;
    s.food.y = 10;
    events = arcade_snake_step(&s, &rng);
    CHECK_EQ_INT(events, ARCADE_SNAKE_EVENT_DIED);

    /* food never spawns on the body */
    arcade_rng_seed(&rng, 3u);
    arcade_snake_init(&s, &rng);
    for (int i = 0; i < 200; i++) {
        for (int j = 0; j < s.length; j++) {
            CHECK(!(s.food.x == s.body[j].x && s.food.y == s.body[j].y));
        }
        arcade_snake_place_food(&s, &rng);
    }
}

/* -------------------------------------------------------------- breakout */

static void test_breakout(void)
{
    arcade_breakout_t g;
    arcade_breakout_init(&g);

    CHECK_EQ_INT(g.bricks_left, ARCADE_BREAKOUT_BRICKS);
    CHECK_EQ_INT(g.lives, ARCADE_BREAKOUT_LIVES);
    CHECK_EQ_INT(g.state, ARCADE_STATE_READY);
    CHECK_EQ_INT(g.score, 0);

    /* ball is parked on the paddle */
    int32_t expected_center = g.paddle_x + ARCADE_FP(ARCADE_BREAKOUT_PADDLE_W) / 2;
    CHECK_EQ_INT(g.x, expected_center);
    CHECK_EQ_INT(g.y, ARCADE_FP(ARCADE_BREAKOUT_PADDLE_Y) - ARCADE_FP(ARCADE_BREAKOUT_BALL_R));

    /* ready state does not move the ball */
    int32_t parked_x = g.x;
    arcade_breakout_tick(&g);
    CHECK_EQ_INT(g.x, parked_x);

    arcade_breakout_launch(&g);
    CHECK_EQ_INT(g.state, ARCADE_STATE_RUNNING);
    CHECK(g.vy < 0);

    int32_t y_before = g.y;
    arcade_breakout_tick(&g);
    CHECK(g.y < y_before);
    CHECK_EQ_INT(g.ticks, 1);

    /* paddle follows the finger and keeps the parked ball */
    arcade_breakout_init(&g);
    arcade_breakout_move_paddle(&g, ARCADE_FP(900));
    CHECK_EQ_INT(g.paddle_x, ARCADE_FP(900) - ARCADE_FP(ARCADE_BREAKOUT_PADDLE_W) / 2);
    CHECK_EQ_INT(g.x, g.paddle_x + ARCADE_FP(ARCADE_BREAKOUT_PADDLE_W) / 2);
    arcade_breakout_move_paddle(&g, ARCADE_FP(-500));
    CHECK_EQ_INT(g.paddle_x, 0);
    arcade_breakout_move_paddle(&g, ARCADE_FP(5000));
    CHECK_EQ_INT(g.paddle_x, ARCADE_FP(ARCADE_BREAKOUT_W - ARCADE_BREAKOUT_PADDLE_W));

    /* left wall bounce (y below the brick field so no brick can interfere) */
    arcade_breakout_init(&g);
    arcade_breakout_launch(&g);
    g.x = ARCADE_FP(ARCADE_BREAKOUT_BALL_R) - 1;
    g.y = ARCADE_FP(400);
    g.vx = -ARCADE_FP(4);
    g.vy = ARCADE_FP(1);
    int32_t vx_before = g.vx;
    arcade_breakout_tick(&g);
    CHECK_EQ_INT(g.vx, -vx_before);
    CHECK_EQ_INT(g.x, ARCADE_FP(ARCADE_BREAKOUT_BALL_R));

    /* brick hit: score, removal, and a vertical bounce for a top hit */
    arcade_breakout_init(&g);
    arcade_breakout_launch(&g);
    int bx, by, bw, bh;
    arcade_breakout_brick_rect(ARCADE_BREAKOUT_ROWS - 1, 4, &bx, &by, &bw, &bh);
    g.x = ARCADE_FP(bx + bw / 2);
    g.y = ARCADE_FP(by + bh) + ARCADE_FP(ARCADE_BREAKOUT_BALL_R) + ARCADE_FP(2);
    g.vx = 0;
    g.vy = -ARCADE_FP(6);
    uint16_t bricks_before = g.bricks_left;
    arcade_breakout_tick(&g);
    CHECK_EQ_INT(g.bricks_left, bricks_before - 1);
    CHECK_EQ_INT(g.brick[ARCADE_BREAKOUT_ROWS - 1][4], 0);
    CHECK_EQ_INT(g.score, ARCADE_BREAKOUT_SCORE_BRICK);
    CHECK(g.vy > 0);

    /* paddle bounce sends the ball back up */
    arcade_breakout_init(&g);
    arcade_breakout_move_paddle(&g, ARCADE_FP(500));
    arcade_breakout_launch(&g);
    g.x = g.paddle_x + ARCADE_FP(ARCADE_BREAKOUT_PADDLE_W) / 2;
    g.y = ARCADE_FP(ARCADE_BREAKOUT_PADDLE_Y) - ARCADE_FP(ARCADE_BREAKOUT_BALL_R) - ARCADE_FP(1);
    g.vx = ARCADE_FP(1);
    g.vy = ARCADE_FP(6);
    arcade_breakout_tick(&g);
    CHECK(g.vy < 0);
    CHECK(g.y < ARCADE_FP(ARCADE_BREAKOUT_PADDLE_Y));

    /* losing the ball costs a life and re-parks it */
    arcade_breakout_init(&g);
    arcade_breakout_launch(&g);
    g.y = ARCADE_FP(ARCADE_BREAKOUT_H) + ARCADE_FP(50);
    g.x = ARCADE_FP(24);
    arcade_breakout_tick(&g);
    CHECK_EQ_INT(g.lives, ARCADE_BREAKOUT_LIVES - 1);
    CHECK_EQ_INT(g.state, ARCADE_STATE_READY);

    /* last three lives then game over */
    arcade_breakout_init(&g);
    for (int life = ARCADE_BREAKOUT_LIVES; life > 0; life--) {
        arcade_breakout_launch(&g);
        g.y = ARCADE_FP(ARCADE_BREAKOUT_H) + ARCADE_FP(50);
        arcade_breakout_tick(&g);
    }
    CHECK_EQ_INT(g.lives, 0);
    CHECK_EQ_INT(g.state, ARCADE_STATE_OVER);
    arcade_breakout_launch(&g); /* must not resurrect the game */
    CHECK_EQ_INT(g.state, ARCADE_STATE_OVER);

    /* clearing every brick wins */
    arcade_breakout_init(&g);
    arcade_breakout_launch(&g);
    for (int row = 0; row < ARCADE_BREAKOUT_ROWS; row++) {
        for (int col = 0; col < ARCADE_BREAKOUT_COLS; col++) {
            g.brick[row][col] = 0;
        }
    }
    g.brick[0][0] = 1;
    g.bricks_left = 1;
    arcade_breakout_brick_rect(0, 0, &bx, &by, &bw, &bh);
    g.x = ARCADE_FP(bx + bw / 2);
    g.y = ARCADE_FP(by + bh) + ARCADE_FP(ARCADE_BREAKOUT_BALL_R) + ARCADE_FP(2);
    g.vx = 0;
    g.vy = -ARCADE_FP(6);
    arcade_breakout_tick(&g);
    CHECK_EQ_INT(g.bricks_left, 0);
    CHECK_EQ_INT(g.state, ARCADE_STATE_WON);

    /* the ball never leaves the field horizontally over a long run */
    arcade_breakout_init(&g);
    arcade_breakout_launch(&g);
    for (int i = 0; i < 2000; i++) {
        arcade_breakout_tick(&g);
        CHECK(g.x >= 0 && g.x <= ARCADE_FP(ARCADE_BREAKOUT_W));
        if (g.state != ARCADE_STATE_RUNNING) {
            break;
        }
    }
}

/* ------------------------------------------------------------------ save */

static void test_save(void)
{
    uint8_t blob[ARCADE_SAVE_BYTES];
    arcade_save_t save;
    arcade_save_defaults(&save);
    CHECK_EQ_INT(save.sound_on, 1);

    CHECK_EQ_INT(arcade_save_encode(&save, blob, sizeof(blob)), ARCADE_SAVE_BYTES);
    CHECK_EQ_INT(arcade_save_encode(&save, blob, 4), -1);

    arcade_save_t decoded;
    CHECK_EQ_INT(arcade_save_decode(&decoded, blob, sizeof(blob)), 0);
    CHECK_EQ_INT(decoded.sound_on, 1);
    CHECK_EQ_INT(decoded.best[ARCADE_GAME_2048], 0);

    save.best[ARCADE_GAME_2048] = 12345;
    save.best[ARCADE_GAME_SNAKE] = 678;
    save.best[ARCADE_GAME_BREAKOUT] = 42;
    save.plays[ARCADE_GAME_SNAKE] = 7;
    save.sound_on = 0;
    CHECK_EQ_INT(arcade_save_encode(&save, blob, sizeof(blob)), ARCADE_SAVE_BYTES);
    CHECK_EQ_INT(arcade_save_decode(&decoded, blob, sizeof(blob)), 0);
    CHECK_EQ_INT(decoded.best[ARCADE_GAME_2048], 12345);
    CHECK_EQ_INT(decoded.best[ARCADE_GAME_SNAKE], 678);
    CHECK_EQ_INT(decoded.best[ARCADE_GAME_BREAKOUT], 42);
    CHECK_EQ_INT(decoded.plays[ARCADE_GAME_SNAKE], 7);
    CHECK_EQ_INT(decoded.sound_on, 0);

    /* corruption is rejected by the checksum */
    uint8_t corrupt[ARCADE_SAVE_BYTES];
    memcpy(corrupt, blob, sizeof(corrupt));
    corrupt[10] ^= 0x5A;
    CHECK_EQ_INT(arcade_save_decode(&decoded, corrupt, sizeof(corrupt)), -1);

    /* wrong magic, wrong size, wrong version */
    memcpy(corrupt, blob, sizeof(corrupt));
    corrupt[0] = 'X';
    CHECK_EQ_INT(arcade_save_decode(&decoded, corrupt, sizeof(corrupt)), -1);
    CHECK_EQ_INT(arcade_save_decode(&decoded, blob, sizeof(blob) - 1), -1);

    memcpy(corrupt, blob, sizeof(corrupt));
    corrupt[4] = 99;
    CHECK_EQ_INT(arcade_save_decode(&decoded, corrupt, sizeof(corrupt)), -1);

    /* high score bookkeeping */
    arcade_save_t s;
    arcade_save_defaults(&s);
    CHECK_EQ_INT(arcade_save_submit(&s, ARCADE_GAME_2048, 100), 1);
    CHECK_EQ_INT(s.best[ARCADE_GAME_2048], 100);
    CHECK_EQ_INT(s.plays[ARCADE_GAME_2048], 1);
    CHECK_EQ_INT(arcade_save_submit(&s, ARCADE_GAME_2048, 50), 0);
    CHECK_EQ_INT(s.best[ARCADE_GAME_2048], 100);
    CHECK_EQ_INT(s.plays[ARCADE_GAME_2048], 2);
    CHECK_EQ_INT(arcade_save_submit(&s, (arcade_game_id_t)7, 999), 0);

    /* CRC-32 known answer */
    const uint8_t digits[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
    CHECK_EQ_INT(arcade_crc32(digits, sizeof(digits)), 0xCBF43926u);
}

int main(void)
{
    test_rng();
    test_2048();
    test_snake();
    test_breakout();
    test_save();

    printf("\n%d checks, %d failure(s)\n", g_checks, g_failures);
    if (g_failures != 0) {
        printf("HOST TESTS FAILED\n");
        return 1;
    }
    printf("HOST TESTS PASSED\n");
    return 0;
}
