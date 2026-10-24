idf_component_register(
    SRCS
        "src/ui_theme.c"
        "src/ui_store.c"
        "src/ui_sfx.c"
        "src/ui_input.c"
        "src/ui_diag.c"
        "src/ui_menu.c"
        "src/ui_game.c"
        "src/game_2048.c"
        "src/game_snake.c"
        "src/game_breakout.c"
        "fonts/arcade_font_22.c"
        "fonts/arcade_font_28.c"
        "fonts/arcade_font_44.c"
    INCLUDE_DIRS "include"
    REQUIRES lvgl__lvgl arcade_core
    PRIV_REQUIRES esp32_p4_wifi6_touch_lcd_7b nvs_flash esp_driver_gpio
)
