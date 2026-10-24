/* Little World Arcade - LVGL UI component public interface.
 *
 * All functions must be called from the LVGL context (i.e. with the display
 * lock held, or from an LVGL timer/event callback).
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/** Build the Little World Arcade root screen and show the home menu. */
void arcade_ui_start(void);

#ifdef __cplusplus
}
#endif
