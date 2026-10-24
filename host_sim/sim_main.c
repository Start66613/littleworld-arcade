/* Little World Arcade - host simulator.
 *
 * Builds the real arcade_ui + arcade_core + generated fonts against a desktop
 * LVGL 9.5 with a 1024x600 dummy display, drives a scripted pointer and saves
 * BMP screenshots. It exists to catch UI crashes and layout problems before the
 * one flash cycle on the real board.
 *
 * SPDX-License-Identifier: MIT
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arcade_ui.h"
#include "lvgl.h"

#define W 1024
#define H 600

static uint16_t fb[W * H];
static uint32_t g_tick;

/* --------------------------------------------------------------- platform */

static uint32_t tick_cb(void)
{
    return g_tick;
}

static void flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    (void)area;
    (void)px_map; /* FULL render mode draws straight into fb */
    lv_display_flush_ready(disp);
}

typedef struct {
    int x;
    int y;
    int pressed;
} pointer_t;

static pointer_t g_ptr;

static void indev_read(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;
    data->point.x = (int32_t)g_ptr.x;
    data->point.y = (int32_t)g_ptr.y;
    data->state = g_ptr.pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

static void pump(int ms)
{
    int elapsed = 0;
    while (elapsed < ms) {
        g_tick += 10;
        lv_timer_handler();
        elapsed += 10;
    }
}

/* ------------------------------------------------------------- screenshot */

static void save_bmp(const char *path)
{
    FILE *file = fopen(path, "wb");
    if (file == NULL) {
        printf("cannot open %s\n", path);
        return;
    }
    const int row_bytes = (W * 3 + 3) & ~3;
    const int data_size = row_bytes * H;
    uint8_t header[54];
    memset(header, 0, sizeof(header));
    header[0] = 'B';
    header[1] = 'M';
    const uint32_t file_size = 54u + (uint32_t)data_size;
    memcpy(header + 2, &file_size, 4);
    uint32_t offset = 54;
    memcpy(header + 10, &offset, 4);
    uint32_t dib = 40;
    memcpy(header + 14, &dib, 4);
    int32_t width = W;
    int32_t height = H;
    memcpy(header + 18, &width, 4);
    memcpy(header + 22, &height, 4);
    uint16_t planes = 1;
    uint16_t bpp = 24;
    memcpy(header + 26, &planes, 2);
    memcpy(header + 28, &bpp, 2);
    uint32_t image_size = (uint32_t)data_size;
    memcpy(header + 34, &image_size, 4);
    fwrite(header, 1, sizeof(header), file);

    uint8_t *row = malloc((size_t)row_bytes);
    if (row == NULL) {
        fclose(file);
        return;
    }
    for (int y = H - 1; y >= 0; y--) {
        memset(row, 0, (size_t)row_bytes);
        for (int x = 0; x < W; x++) {
            uint16_t px = fb[y * W + x];
            uint8_t r = (uint8_t)(((px >> 11) & 0x1F) * 255 / 31);
            uint8_t g = (uint8_t)(((px >> 5) & 0x3F) * 255 / 63);
            uint8_t b = (uint8_t)((px & 0x1F) * 255 / 31);
            row[x * 3 + 0] = b;
            row[x * 3 + 1] = g;
            row[x * 3 + 2] = r;
        }
        fwrite(row, 1, (size_t)row_bytes, file);
    }
    free(row);
    fclose(file);
    printf("saved %s\n", path);
}

static void shot(const char *name)
{
    char path[256];
    snprintf(path, sizeof(path), "reports/screenshots/%s.bmp", name);
    lv_obj_invalidate(lv_screen_active());
    lv_refr_now(lv_display_get_default());
    save_bmp(path);
}

/* --------------------------------------------------------------- gestures */

static lv_obj_t *g_last_button;

static lv_obj_t *find_label(lv_obj_t *root, const char *text)
{
    uint32_t count = lv_obj_get_child_count(root);
    for (uint32_t i = 0; i < count; i++) {
        lv_obj_t *child = lv_obj_get_child(root, i);
        if (lv_obj_check_type(child, &lv_label_class) && strcmp(lv_label_get_text(child), text) == 0) {
            return child;
        }
        lv_obj_t *found = find_label(child, text);
        if (found != NULL) {
            return found;
        }
    }
    return NULL;
}

static void click(int x, int y)
{
    g_ptr.x = x;
    g_ptr.y = y;
    g_ptr.pressed = 1;
    pump(120);
    g_ptr.pressed = 0;
    pump(160);
}

/* Click the centre of the widget that carries @p text (the label itself is not
 * clickable, so the click lands on its parent button/card). */
static int click_text(const char *text)
{
    lv_obj_t *label = find_label(lv_screen_active(), text);
    if (label == NULL) {
        printf("WARNING: no widget labelled '%s'\n", text);
        return 0;
    }
    lv_area_t area;
    lv_obj_get_coords(label, &area);
    int x = (area.x1 + area.x2) / 2;
    int y = (area.y1 + area.y2) / 2;
    g_last_button = label;
    click(x, y);
    return 1;
}

static void swipe(int x1, int y1, int x2, int y2)
{
    g_ptr.x = x1;
    g_ptr.y = y1;
    g_ptr.pressed = 1;
    pump(90);
    for (int step = 1; step <= 4; step++) {
        g_ptr.x = x1 + (x2 - x1) * step / 4;
        g_ptr.y = y1 + (y2 - y1) * step / 4;
        pump(40);
    }
    g_ptr.pressed = 0;
    pump(260);
}

static void drag(int x1, int y1, int x2, int y2)
{
    g_ptr.x = x1;
    g_ptr.y = y1;
    g_ptr.pressed = 1;
    pump(120);
    for (int step = 1; step <= 5; step++) {
        g_ptr.x = x1 + (x2 - x1) * step / 5;
        g_ptr.y = y1 + (y2 - y1) * step / 5;
        pump(60);
    }
    g_ptr.pressed = 0;
    pump(160);
}

/* ------------------------------------------------------------------- main */

static int count_objects(lv_obj_t *obj)
{
    int count = 1;
    uint32_t children = lv_obj_get_child_count(obj);
    for (uint32_t i = 0; i < children; i++) {
        count += count_objects(lv_obj_get_child(obj, i));
    }
    return count;
}

int main(void)
{
    lv_init();
    lv_tick_set_cb(tick_cb);

    lv_display_t *disp = lv_display_create(W, H);
    lv_display_set_flush_cb(disp, flush_cb);
    lv_display_set_buffers(disp, fb, NULL, sizeof(fb), LV_DISPLAY_RENDER_MODE_FULL);
    lv_display_set_default(disp);

    lv_theme_default_init(disp, lv_palette_main(LV_PALETTE_PINK), lv_palette_main(LV_PALETTE_PURPLE), false,
                          LV_FONT_DEFAULT);

    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, indev_read);

    printf("Little World Arcade host simulator (LVGL %d.%d.%d)\n", LVGL_VERSION_MAJOR, LVGL_VERSION_MINOR,
           LVGL_VERSION_PATCH);

    arcade_ui_start();
    pump(600);
    shot("01-menu");

    /* ---- 2048 ------------------------------------------------------- */
    click_text("2048");
    pump(900);
    shot("02-2048");
    swipe(512, 430, 512, 250);
    swipe(512, 430, 512, 250);
    pump(400);
    shot("03-2048-after-swipes");
    click(42, 42);
    pump(700);
    shot("04-menu-again");

    /* ---- diagnostics overlay ---------------------------------------- */
    click(884, 64);
    pump(900);
    shot("05-diagnostics");
    click_text("关闭");
    pump(400);

    /* ---- snake ------------------------------------------------------ */
    click_text("贪吃蛇");
    pump(1500);
    shot("06-snake");
    /* The on-screen D-pad sits in the header: left/up/down/right at this pitch.
     * (Clicking by symbol would be ambiguous with the back arrow.) */
    click(256, 42); /* up */
    pump(700);
    click(352, 42); /* right */
    pump(700);
    shot("07-snake-turned");
    click(922, 42);
    pump(500);
    shot("08-snake-paused");
    click_text("继续游戏");
    pump(700);
    shot("09-snake-resumed");
    click(922, 42);
    pump(400);
    click_text("重新开始");
    pump(600);
    shot("10-snake-restarted");
    click(922, 42);
    pump(400);
    click_text("返回主页");
    pump(800);
    shot("11-menu-after-snake");

    /* ---- breakout --------------------------------------------------- */
    click_text("打砖块");
    pump(900);
    shot("12-breakout");
    click(512, 400); /* launch the ball */
    pump(2600);
    shot("13-breakout-playing");
    drag(700, 470, 260, 470);
    pump(1200);
    shot("14-breakout-after-drag");
    click(922, 42);
    pump(500);
    shot("15-breakout-paused");
    click_text("继续游戏");
    pump(700);
    shot("16-breakout-resumed");

    /* ---- screen lifecycle / leak check ------------------------------ */
    click(42, 42); /* back to the menu */
    pump(700);
    click_text("2048");
    pump(600);
    click(42, 42);
    pump(700);
    int base_objects = count_objects(lv_screen_active());
    for (int i = 0; i < 20; i++) {
        if (!click_text("2048")) {
            printf("FAIL: menu card missing on cycle %d\n", i + 1);
            return 1;
        }
        pump(600);
        click(42, 42);
        pump(700);
    }
    int after_objects = count_objects(lv_screen_active());
    shot("17-after-20-open-close-cycles");
    printf("live objects: after 1 cycle = %d, after 21 cycles = %d\n", base_objects, after_objects);
    if (after_objects != base_objects) {
        printf("FAIL: screen objects leaked across open/close cycles\n");
        return 1;
    }
    printf("PASS: no object growth across 21 open/close cycles\n");

    printf("simulator finished without crashing\n");
    return 0;
}
