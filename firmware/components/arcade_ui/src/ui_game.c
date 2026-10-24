/* Little World Arcade - shared game screen: header, HUD, pause and result
 * overlays. Individual games only own the play field and their rules.
 *
 * SPDX-License-Identifier: MIT
 */
#include "ui_internal.h"

typedef struct {
    const arcade_game_module_t *module;
    lv_obj_t *screen;
    lv_obj_t *playfield;
    lv_obj_t *score_label;
    lv_obj_t *best_label;
    lv_obj_t *extra_label;
    lv_obj_t *overlay;
    lv_obj_t *overlay_title;
    lv_obj_t *overlay_body;
    lv_obj_t *btn_continue;
    int paused;
    int finished;
} game_ui_t;

static game_ui_t s_ui;

static void host_set_score(uint32_t score);
static void host_set_extra(const char *text);
static void host_game_over(uint32_t score, int won);

static const arcade_game_host_t s_host = {
    .set_score = host_set_score,
    .set_extra = host_set_extra,
    .game_over = host_game_over,
};

/* ------------------------------------------------------------------ helpers */

static lv_obj_t *hud_pill(lv_obj_t *parent, const char *text, uint32_t bg, uint32_t fg)
{
    lv_obj_t *label = lv_label_create(parent);
    arcade_style_label(label, &arcade_font_22, fg);
    lv_obj_set_style_bg_color(label, lv_color_hex(bg), 0);
    lv_obj_set_style_bg_opa(label, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(label, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_pad_hor(label, 16, 0);
    lv_obj_set_style_pad_ver(label, 8, 0);
    lv_label_set_text(label, text);
    return label;
}

static void overlay_hide(void)
{
    if (s_ui.overlay != NULL) {
        lv_obj_add_flag(s_ui.overlay, LV_OBJ_FLAG_HIDDEN);
    }
}

/* mode: 1 = paused, 2 = game over, 3 = objective reached */
static void overlay_show(int mode, uint32_t score)
{
    if (s_ui.overlay == NULL || s_ui.module == NULL) {
        return;
    }
    arcade_save_t *save = arcade_store_data();
    unsigned best = save->best[s_ui.module->id];

    if (mode == 1) {
        lv_label_set_text(s_ui.overlay_title, "已暂停");
        lv_label_set_text(s_ui.overlay_body, "休息一下，准备好再继续～");
        lv_obj_remove_flag(s_ui.btn_continue, LV_OBJ_FLAG_HIDDEN);
    } else if (mode == 2) {
        lv_label_set_text(s_ui.overlay_title, "游戏结束");
        lv_label_set_text_fmt(s_ui.overlay_body, "本局得分 %u\n最高分 %u", (unsigned)score, best);
        lv_obj_add_flag(s_ui.btn_continue, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_label_set_text(s_ui.overlay_title, "通关啦！");
        lv_label_set_text_fmt(s_ui.overlay_body, "本局得分 %u\n最高分 %u", (unsigned)score, best);
        lv_obj_remove_flag(s_ui.btn_continue, LV_OBJ_FLAG_HIDDEN);
    }

    lv_obj_t *continue_label = lv_obj_get_child(s_ui.btn_continue, 0);
    if (continue_label != NULL) {
        lv_label_set_text(continue_label, (mode == 1) ? "继续游戏" : "继续挑战");
    }

    lv_obj_remove_flag(s_ui.overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(s_ui.overlay);
    s_ui.finished = (mode != 1);
}

/* -------------------------------------------------------------- host hooks */

static void host_set_score(uint32_t score)
{
    if (s_ui.score_label != NULL) {
        lv_label_set_text_fmt(s_ui.score_label, "得分 %u", (unsigned)score);
    }
}

static void host_set_extra(const char *text)
{
    if (s_ui.extra_label != NULL && text != NULL) {
        lv_label_set_text(s_ui.extra_label, text);
    }
}

static void host_game_over(uint32_t score, int won)
{
    if (s_ui.module == NULL) {
        return;
    }
    arcade_save_t *save = arcade_store_data();
    (void)arcade_save_submit(save, s_ui.module->id, score);
    arcade_store_commit();

    if (s_ui.best_label != NULL) {
        lv_label_set_text_fmt(s_ui.best_label, "最高 %u", (unsigned)save->best[s_ui.module->id]);
    }
    overlay_show(won ? 3 : 2, score);
}

/* --------------------------------------------------------------- callbacks */

static void resume_clicked(lv_event_t *event)
{
    (void)event;
    overlay_hide();
    s_ui.paused = 0;
    s_ui.finished = 0;
    if (s_ui.module != NULL && s_ui.module->set_paused != NULL) {
        s_ui.module->set_paused(0);
    }
}

static void restart_game(void);

static void restart_clicked(lv_event_t *event)
{
    (void)event;
    restart_game();
}

static void back_to_menu(void);

static void home_clicked(lv_event_t *event)
{
    (void)event;
    back_to_menu();
}

static void pause_clicked(lv_event_t *event)
{
    (void)event;
    if (s_ui.finished || s_ui.module == NULL) {
        return;
    }
    if (s_ui.paused) {
        resume_clicked(NULL);
        return;
    }
    s_ui.paused = 1;
    if (s_ui.module->set_paused != NULL) {
        s_ui.module->set_paused(1);
    }
    overlay_show(1, 0);
}

static void sound_clicked(lv_event_t *event)
{
    lv_obj_t *button = lv_event_get_user_data(event);
    arcade_sound_toggle();
    arcade_sound_refresh_icon(button);
}

static void back_clicked(lv_event_t *event)
{
    (void)event;
    back_to_menu();
}

static void restart_game(void)
{
    if (s_ui.module == NULL || s_ui.playfield == NULL) {
        return;
    }
    if (s_ui.module->destroy != NULL) {
        s_ui.module->destroy();
    }
    lv_obj_clean(s_ui.playfield);
    overlay_hide();
    s_ui.paused = 0;
    s_ui.finished = 0;
    s_ui.module->create(s_ui.playfield, &s_host);
}

static void back_to_menu(void)
{
    if (s_ui.module != NULL && s_ui.module->destroy != NULL) {
        s_ui.module->destroy();
    }
    s_ui.module = NULL;
    s_ui.overlay = NULL;
    arcade_nav_menu();
}

/* ------------------------------------------------------------ screen build */

static void build_overlay(lv_obj_t *screen, const arcade_game_module_t *module)
{
    s_ui.overlay = lv_obj_create(screen);
    lv_obj_remove_style_all(s_ui.overlay);
    lv_obj_set_pos(s_ui.overlay, 0, 0);
    lv_obj_set_size(s_ui.overlay, 1024, 600);
    lv_obj_set_style_bg_color(s_ui.overlay, lv_color_hex(0x2A1E26), 0);
    lv_obj_set_style_bg_opa(s_ui.overlay, LV_OPA_50, 0);
    lv_obj_add_flag(s_ui.overlay, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(s_ui.overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(s_ui.overlay);

    lv_obj_t *card = lv_obj_create(s_ui.overlay);
    lv_obj_remove_style_all(card);
    arcade_style_card(card);
    lv_obj_set_size(card, 520, 380);
    lv_obj_center(card);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(card, 28, 0);
    lv_obj_set_style_pad_row(card, 14, 0);

    s_ui.overlay_title = lv_label_create(card);
    arcade_style_label(s_ui.overlay_title, &arcade_font_28, ARC_TEXT);
    lv_label_set_text(s_ui.overlay_title, "已暂停");

    s_ui.overlay_body = lv_label_create(card);
    arcade_style_label(s_ui.overlay_body, &arcade_font_22, ARC_TEXT_MUTED);
    lv_obj_set_style_text_align(s_ui.overlay_body, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_line_space(s_ui.overlay_body, 6, 0);
    lv_label_set_text(s_ui.overlay_body, "");

    s_ui.btn_continue = arcade_button(card, "继续游戏", 240, 52, module->accent, 0xFFFFFF, &arcade_font_22);
    lv_obj_add_event_cb(s_ui.btn_continue, resume_clicked, LV_EVENT_CLICKED, NULL);

    lv_obj_t *restart = arcade_button(card, "重新开始", 240, 52, ARC_LAVENDER, 0xFFFFFF, &arcade_font_22);
    lv_obj_add_event_cb(restart, restart_clicked, LV_EVENT_CLICKED, NULL);

    lv_obj_t *home = arcade_button(card, "返回主页", 240, 52, 0xF3E7EC, ARC_TEXT, &arcade_font_22);
    lv_obj_add_event_cb(home, home_clicked, LV_EVENT_CLICKED, NULL);
}

void arcade_nav_game(const arcade_game_module_t *module)
{
    if (module == NULL) {
        return;
    }

    s_ui.module = module;
    s_ui.paused = 0;
    s_ui.finished = 0;
    s_ui.overlay = NULL;
    s_ui.btn_continue = NULL;

    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_remove_style_all(screen);
    lv_obj_set_style_bg_color(screen, lv_color_hex(ARC_BG_TOP), 0);
    lv_obj_set_style_bg_grad_color(screen, lv_color_hex(ARC_BG_BOTTOM), 0);
    lv_obj_set_style_bg_grad_dir(screen, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_text_font(screen, &arcade_font_22, 0);
    lv_obj_set_style_text_color(screen, lv_color_hex(ARC_TEXT), 0);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    s_ui.screen = screen;

    /* Header */
    lv_obj_t *bar = lv_obj_create(screen);
    lv_obj_remove_style_all(bar);
    lv_obj_set_pos(bar, 0, 0);
    lv_obj_set_size(bar, 1024, 84);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_60, 0);
    lv_obj_remove_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *back = arcade_icon_button(bar, LV_SYMBOL_LEFT, 52, 0xFFFFFF, ARC_TEXT);
    lv_obj_set_pos(back, 16, 16);
    lv_obj_add_event_cb(back, back_clicked, LV_EVENT_CLICKED, NULL);

    lv_obj_t *title = lv_label_create(bar);
    arcade_style_label(title, &arcade_font_28, ARC_TEXT);
    lv_label_set_text(title, module->title);
    lv_obj_set_pos(title, 80, 26);

    if (module->build_controls != NULL) {
        module->build_controls(bar);
    }

    lv_obj_t *hud = lv_obj_create(bar);
    lv_obj_remove_style_all(hud);
    lv_obj_set_pos(hud, 370, 20);
    lv_obj_set_size(hud, 506, 44);
    lv_obj_set_flex_flow(hud, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(hud, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(hud, 10, 0);
    lv_obj_remove_flag(hud, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(hud, LV_OBJ_FLAG_SCROLLABLE);

    s_ui.score_label = hud_pill(hud, "得分 0", 0xFFE3EC, ARC_PINK_DEEP);
    s_ui.best_label = hud_pill(hud, "最高 0", 0xEFE6FF, 0x7A5CC0);
    s_ui.extra_label = hud_pill(hud, "准备", 0xE6F7EF, ARC_MINT_DEEP);
    lv_label_set_text_fmt(s_ui.best_label, "最高 %u", (unsigned)arcade_store_data()->best[module->id]);

    lv_obj_t *pause = arcade_icon_button(bar, LV_SYMBOL_PAUSE, 52, 0xFFFFFF, ARC_TEXT);
    lv_obj_set_pos(pause, 896, 16);
    lv_obj_add_event_cb(pause, pause_clicked, LV_EVENT_CLICKED, NULL);

    lv_obj_t *sound = arcade_icon_button(bar, LV_SYMBOL_VOLUME_MAX, 52, 0xFFFFFF, ARC_PINK_DEEP);
    lv_obj_set_pos(sound, 960, 16);
    lv_obj_add_event_cb(sound, sound_clicked, LV_EVENT_CLICKED, sound);
    arcade_sound_refresh_icon(sound);

    /* Play field */
    lv_obj_t *playfield = lv_obj_create(screen);
    lv_obj_remove_style_all(playfield);
    lv_obj_set_pos(playfield, 8, 92);
    lv_obj_set_size(playfield, 1008, 468);
    lv_obj_set_style_radius(playfield, 24, 0);
    lv_obj_set_style_bg_color(playfield, lv_color_hex(module->soft), 0);
    lv_obj_set_style_bg_opa(playfield, LV_OPA_COVER, 0);
    lv_obj_add_flag(playfield, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(playfield, LV_OBJ_FLAG_SCROLLABLE);
    s_ui.playfield = playfield;

    lv_obj_t *hint = lv_label_create(screen);
    arcade_style_label(hint, &arcade_font_22, ARC_TEXT_MUTED);
    lv_label_set_text(hint, module->hint);
    lv_obj_align(hint, LV_ALIGN_TOP_MID, 0, 566);

    build_overlay(screen, module);

    module->create(playfield, &s_host);

    lv_screen_load_anim(screen, LV_SCREEN_LOAD_ANIM_FADE_IN, 180, 0, true);
}
