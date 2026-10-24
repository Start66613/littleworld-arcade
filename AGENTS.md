# Little World Arcade — 仓库规则

本仓库是正式工程 `C:\Projects\LittleWorld` 的独立实验分支。任何会话都必须先读 `README.md`、`docs/STATUS.md`、`docs/RECOVERY.md`。

## 硬件基线
- 唯一目标板：Waveshare **ESP32-P4-WIFI6-Touch-LCD-7B**（1024×600、GT911、ESP32-P4 v3.2、32 MiB Flash）。
- 固定组合：ESP-IDF v5.5.5（提交 `b774170f`）、官方 BSP 3.0.1、LVGL 9.5.0、esp_lvgl_adapter 0.6.4、官方 `config/esp32p4_rev3_x.defaults`。
- 不猜引脚；显示/触摸/音频只通过官方 BSP 访问。不得混用 7、7A、P4-Platform 或 ESP32-S3 的配置。

## 与正式工程的关系
- 只读复用：`D:\Espressif\LittleWorld`（工具链）与 `C:\Projects\LittleWorld`（文档、锁文件）。
- **不写入、不修改、不覆盖**正式工程里的任何文件；不把实验代码合进正式工程（除非用户另行要求）。
- 正式工程的安全规则（`C:\Projects\LittleWorld\AGENTS.md`）同样适用。

## 远程仓库与隐私
- 用户于 2026-10-25 明确要求把本工程上传 GitHub，仓库建成**私有**（`Start66613/littleworld-arcade`）。
  未获新指示不得改为公开。
- **永远不进入仓库**：整片 Flash 备份、设备原始串口日志、MAC/凭据、正式工程的 `private/` 内容。
  备份只存在于 `D:\Espressif\LittleWorld\private\backups\`，由 `.gitignore` 之外的人工纪律保护。
- 不启用云构建、GitHub Actions 或任何遥测；推送只包含源码、文档、截图与字体子集。

## 设备写入
- 默认禁止：flash / erase / 分区改写 / eFuse / C6 固件 / OTA。
- 每次写入都需要用户**针对这一次**的明确许可；过去的口头许可不延续。
- 刷机前必须：已校验的整片备份 + 版本匹配 + 明确的恢复步骤 + 用户批准。
- 串口复位、进入 ROM 下载模式、读取 Flash 同样需要单独许可。

## 代码规则
- 游戏规则放 `components/arcade_core`，不得引入 LVGL/IDF 头文件（保证可在主机上测试）。
- 界面放 `components/arcade_ui`；每个游戏实现 `create/destroy/set_paused`，`destroy` 必须删除自己创建的定时器与堆内存。
- 不在 `app_main.c` 里堆业务逻辑；不写阻塞主循环（>20 ms 的忙等）。
- 中文文案直接写在源码里，然后重跑 `tools/gen_fonts.ps1`，否则会出现方框。
- 新增依赖必须固定版本，并记录到 `docs/STATUS.md`。

## 许可规则
- 只允许引入 MIT / Apache-2.0 / BSD / OFL 等宽松许可的代码或素材，并在 `docs/LICENSES.md` 记录来源。
- 没有许可证的仓库（默认版权保留）、GPL/LGPL 代码、第三方游戏素材/字体/ROM 一律不得复制进本仓库。
- 玩法机制可以重新实现，代码与美术资源不可以照搬。

## 记录规则
- 状态只用：待办 / 进行中 / 通过 / 受阻；未执行的项不得写“通过”。
- 编译、主机测试、板上测试分别记录，不得互相替代。
- 失败要记录命令、退出码和下一步，不隐藏。
