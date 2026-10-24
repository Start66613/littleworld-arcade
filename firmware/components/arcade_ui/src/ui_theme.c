/* Little World Arcade - shared visual language.
 *
 * Cream / pink / lavender, rounded cards, soft shadows; built only from LVGL
 * primitives so no image assets are required.
 *
 * SPDX-License-Identifier: MIT
 */
#include "ui_internal.h"

static lv_style_t s_card;
static lv_style_t s_card_pressed;
static lv_style_t s_button;
static lv_style_t s_button_pressed;
static lv_style_t s_pill;
static int s_ready;

void arcade_theme_init(void)
{
    if (s_ready) {
        return;
    }

    lv_style_init(&s_card);
    lv_style_set_radius(&s_card, 26);
    lv_style_set_bg_color(&s_card, lv_color_hex(ARC_CARD));
    lv_style_set_bg_opa(&s_card, LV_OPA_COVER);
    lv_style_set_border_width(&s_card, 0);
    lv_style_set_outline_width(&s_card, 0);
    lv_style_set_pad_all(&s_card, 0);
    lv_style_set_shadow_width(&s_card, 18);
    lv_style_set_shadow_color(&s_card, lv_color_hex(ARC_SHADOW));
    lv_style_set_shadow_opa(&s_card, LV_OPA_40);
    lv_style_set_shadow_offset_y(&s_card, 6);
    lv_style_set_text_font(&s_card, &arcade_font_22);
    lv_style_set_text_color(&s_card, lv_color_hex(ARC_TEXT));

    lv_style_init(&s_card_pressed);
    lv_style_set_shadow_width(&s_card_pressed, 6);
    lv_style_set_shadow_offset_y(&s_card_pressed, 2);
    lv_style_set_translate_y(&s_card_pressed, 2);

    lv_style_init(&s_button);
    lv_style_set_radius(&s_button, LV_RADIUS_CIRCLE);
    lv_style_set_border_width(&s_button, 0);
    lv_style_set_pad_all(&s_button, 0);
    lv_style_set_text_font(&s_button, &arcade_font_22);
    lv_style_set_shadow_width(&s_button, 10);
    lv_style_set_shadow_color(&s_button, lv_color_hex(ARC_SHADOW));
    lv_style_set_shadow_opa(&s_button, LV_OPA_30);
    lv_style_set_shadow_offset_y(&s_button, 4);

    lv_style_init(&s_button_pressed);
    lv_style_set_translate_y(&s_button_pressed, 2);
    lv_style_set_shadow_width(&s_button_pressed, 2);

    lv_style_init(&s_pill);
    lv_style_set_radius(&s_pill, LV_RADIUS_CIRCLE);
    lv_style_set_border_width(&s_pill, 0);
    lv_style_set_pad_hor(&s_pill, 16);
    lv_style_set_pad_ver(&s_pill, 6);
    lv_style_set_text_font(&s_pill, &arcade_font_22);

    s_ready = 1;
}

void arcade_style_card(lv_obj_t *obj)
{
    lv_obj_add_style(obj, &s_card, 0);
    lv_obj_add_style(obj, &s_card_pressed, LV_STATE_PRESSED);
}

void arcade_style_pill(lv_obj_t *obj, uint32_t bg)
{
    lv_obj_add_style(obj, &s_pill, 0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(bg), 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
}

void arcade_style_label(lv_obj_t *obj, const lv_font_t *font, uint32_t color)
{
    lv_obj_set_style_text_font(obj, font, 0);
    lv_obj_set_style_text_color(obj, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
}

lv_obj_t *arcade_container(lv_obj_t *parent, int x, int y, int w, int h)
{
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_remove_style_all(obj);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, w, h);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    return obj;
}

lv_obj_t *arcade_button(lv_obj_t *parent, const char *text, int w, int h, uint32_t bg, uint32_t fg,
                        const lv_font_t *font)
{
    lv_obj_t *btn = lv_obj_create(parent);
    lv_obj_remove_style_all(btn);
    lv_obj_add_style(btn, &s_button, 0);
    lv_obj_add_style(btn, &s_button_pressed, LV_STATE_PRESSED);
    lv_obj_set_size(btn, w, h);
    lv_obj_set_style_bg_color(btn, lv_color_hex(bg), 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_remove_flag(btn, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, font != NULL ? font : &arcade_font_22, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(fg), 0);
    lv_obj_center(label);
    return btn;
}

lv_obj_t *arcade_icon_button(lv_obj_t *parent, const char *symbol, int size, uint32_t bg, uint32_t fg)
{
    lv_obj_t *btn = lv_obj_create(parent);
    lv_obj_remove_style_all(btn);
    lv_obj_add_style(btn, &s_button, 0);
    lv_obj_add_style(btn, &s_button_pressed, LV_STATE_PRESSED);
    lv_obj_set_size(btn, size, size);
    lv_obj_set_style_bg_color(btn, lv_color_hex(bg), 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_remove_flag(btn, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, symbol);
    /* The built-in Montserrat fonts carry the FontAwesome symbols. */
    lv_obj_set_style_text_font(label, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(fg), 0);
    lv_obj_center(label);
    return btn;
}
