/* Little World Arcade - shared touch input helpers.
 *
 * SPDX-License-Identifier: MIT
 */
#include "ui_internal.h"

static struct {
    lv_point_t start;
    int active;
    int threshold;
    arcade_swipe_cb_t cb;
    void *user;
} s_swipe;

static void swipe_pressed(lv_event_t *event)
{
    lv_indev_t *indev = lv_indev_active();
    if (indev == NULL) {
        return;
    }
    lv_indev_get_point(indev, &s_swipe.start);
    s_swipe.active = 1;
    (void)event;
}

static void swipe_released(lv_event_t *event)
{
    if (!s_swipe.active || s_swipe.cb == NULL) {
        s_swipe.active = 0;
        return;
    }
    s_swipe.active = 0;

    lv_indev_t *indev = lv_indev_active();
    if (indev == NULL) {
        return;
    }
    lv_point_t end;
    lv_indev_get_point(indev, &end);

    int dx = (int)end.x - (int)s_swipe.start.x;
    int dy = (int)end.y - (int)s_swipe.start.y;
    int adx = dx < 0 ? -dx : dx;
    int ady = dy < 0 ? -dy : dy;
    if (adx < s_swipe.threshold && ady < s_swipe.threshold) {
        return;
    }

    arcade_dir_t dir;
    if (adx >= ady) {
        dir = (dx > 0) ? ARCADE_DIR_RIGHT : ARCADE_DIR_LEFT;
    } else {
        dir = (dy > 0) ? ARCADE_DIR_DOWN : ARCADE_DIR_UP;
    }
    s_swipe.cb(dir, s_swipe.user);
    (void)event;
}

void arcade_attach_swipe(lv_obj_t *obj, int threshold, arcade_swipe_cb_t cb, void *user)
{
    s_swipe.active = 0;
    s_swipe.threshold = threshold > 8 ? threshold : 8;
    s_swipe.cb = cb;
    s_swipe.user = user;
    lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(obj, swipe_pressed, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(obj, swipe_released, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(obj, swipe_released, LV_EVENT_PRESS_LOST, NULL);
}

int arcade_pointer_x_in(lv_obj_t *obj)
{
    lv_indev_t *indev = lv_indev_active();
    if (indev == NULL || obj == NULL) {
        return 0;
    }
    lv_point_t point;
    lv_indev_get_point(indev, &point);
    lv_area_t area;
    lv_obj_get_coords(obj, &area);
    return (int)point.x - (int)area.x1;
}
