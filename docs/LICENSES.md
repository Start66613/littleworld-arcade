# 许可与来源清单

更新：2026-10-25。原则：只允许宽松许可（MIT / Apache-2.0 / BSD / OFL）的代码与素材进入本仓库；玩法机制可以重新实现，代码和美术资源不能照搬。

## 1. 本仓库

| 内容 | 许可 |
|---|---|
| `firmware/`、`tools/`、`tests/` 全部自研代码 | MIT（见 `LICENSE`） |

## 2. 运行时依赖（不随仓库分发，由组件管理器拉取）

| 组件 | 版本 | 许可 | 用途 |
|---|---|---|---|
| `lvgl/lvgl` | 9.5.0 | MIT | 图形库 |
| `waveshare/esp32_p4_wifi6_touch_lcd_7b` | 3.0.1 | Apache-2.0 | 官方 7B BSP（MIPI-DSI、GT911、背光、音频） |
| `espressif/esp_lvgl_adapter` | 0.6.4 | Apache-2.0 | LVGL 与 esp_lcd 适配层 |
| `espressif/esp_lcd_touch(_gt911)` 等 | 传递依赖 | Apache-2.0 | 触摸驱动 |
| ESP-IDF | v5.5.5 | Apache-2.0 | 框架 |

## 3. 字体

| 内容 | 来源 | 许可 |
|---|---|---|
| `arcade_font_22/28/44.c` 中的字形位图 | Noto Sans SC（`github.com/notofonts/noto-cjk`，`Sans/SubsetOTF/SC/NotoSansSC-Regular.otf`、`-Bold.otf`） | **SIL Open Font License 1.1**，全文见 `firmware/components/arcade_ui/fonts/OFL.txt` |

生成方式：只栅格化源码中真实出现的 114 个非 ASCII 字符 + ASCII 0x20–0x7E，未修改字形本身。
OFL 允许嵌入与再分发，要求保留版权与许可声明（已随字体文件保存）。

构建期工具 `lv_font_conv` 1.5.2（MIT）只在本机使用，不进入固件，也不随仓库分发。

## 4. 参考过的开源项目（**未复制代码或素材**）

| 仓库 | 许可（实测） | 结论 |
|---|---|---|
| `LHYhorion/LVGL_Game_on_ESP32` | **无许可证**（默认版权保留） | 仅阅读玩法。其 PvZ 贴图、打地鼠美术、`lv_font_simsun_12.c` 均不可用；且仅适配 LVGL 8。 |
| `sirisakG2/JC3248W535C` | **无许可证** | 仅阅读玩法与交互（其“按键比滑动可靠”的提示已影响本项目的方向键设计）。未复制其 Arduino 驱动或代码。 |
| `tangstoney/esp32p4-nesemu` | **LGPL-2.0**（nofrendo 核心）+ Apache-2.0 胶水，且仓库内含 Nintendo ROM | 不引入（会把本项目变成 LGPL，且 ROM 无授权）。仅借鉴“把虚拟手柄放在画面留白处”的思路。 |
| `Ellenp2p/esp32-pet-tamagotchi` | **MIT** | 法律上可复用，但本轮是独立实现；后续宠物功能可参考其分层设计。 |

本项目的 2048 / 贪吃蛇 / 打砖块均为按公开玩法规则**从零实现**（规则本身不受版权保护），未使用上述仓库的任何代码、图像、音效或字体。

## 5. 素材政策

- 本版本**没有**任何图片、音频或第三方字体文件：界面全部由 LVGL 原生图形绘制。
- 不包含任何游戏 ROM、商业游戏素材或未授权字体。
- 后续若引入美术资源，必须记录来源与许可，并在 `docs/` 中更新本文件。
