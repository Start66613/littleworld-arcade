/* Little World Arcade - sound preference.
 *
 * v1 ships without audio output on purpose: the ES8311 codec path must be
 * verified on the real board before it is allowed to block or disturb the UI.
 * The preference is persisted now so the audio layer can honour it later
 * without a data migration.
 *
 * SPDX-License-Identifier: MIT
 */
#include "ui_internal.h"

int arcade_sound_enabled(void)
{
    return arcade_store_data()->sound_on ? 1 : 0;
}

void arcade_sound_toggle(void)
{
    arcade_save_t *save = arcade_store_data();
    save->sound_on = save->sound_on ? 0 : 1;
    arcade_store_commit();
}

void arcade_sound_refresh_icon(lv_obj_t *button)
{
    if (button == NULL) {
        return;
    }
    lv_obj_t *label = lv_obj_get_child(button, 0);
    if (label == NULL) {
        return;
    }
    lv_label_set_text(label, arcade_sound_enabled() ? LV_SYMBOL_VOLUME_MAX : LV_SYMBOL_MUTE);
}
