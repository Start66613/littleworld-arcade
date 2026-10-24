# 本工程的可执行工具

所有脚本都在 `tools/` 下，默认只做本机操作；唯一会接触开发板的是 `backup_flash.ps1`（只读），
且必须显式传入 `-Authorized` 才会运行。

| 脚本 | 作用 | 是否接触设备 | 备注 |
|---|---|---|---|
| `activate_env.ps1` | 在当前 PowerShell 进程里激活已安装的 ESP-IDF v5.5.5 环境 | 否 | 只设置进程环境变量，不安装、不改 PATH |
| `build.ps1` | 编译固件（`-Clean` 重新 configure，`-ImageInfo` 额外打印镜像头） | 否 | 产物在 `D:\LittleWorld-Arcade\build` |
| `run_host_tests.ps1` | 用 MSVC 编译并运行 `arcade_core` 主机单元测试 | 否 | `/W4 /WX`，1393 项检查 |
| `gen_fonts.ps1` | 扫描源码中的中文字符，生成 Noto Sans SC 子集字体 | 否 | 需要 `tools/fonts/cache` 里的 OTF 与 `node_modules/lv_font_conv` |
| `check_fonts.py` | 校验 UI 用到的每个非 ASCII 字符都在字体里 | 否 | 防止刷机后出现方框 |
| `image_check.py` | 离线解析镜像头：芯片、修订范围、Flash、分区表 | 否 | 判断“能不能在这块 v3.2 板子上启动” |
| `verify.ps1` | 依次运行字体检查、主机测试、镜像头检查 | 否 | 一条命令给出总结 |
| `backup_flash.ps1` | 整片 32 MiB 只读双遍备份（分块、超时、断点续读） | **是（只读）** | 需要用户明确授权；绝不写 Flash |

## 常用命令

```powershell
# 一条命令跑完全部本地检查
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/verify.ps1

# 重新生成中文字体（改动了中文文案之后）
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/gen_fonts.ps1

# 备份（示例；必须先获得授权）
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/backup_flash.ps1 -Port COM5 -Baud 460800 -Authorized
# 断点续读已有目录
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/backup_flash.ps1 -Port COM5 -Baud 460800 -Authorized -WorkDir <已有目录>
```

## 备份脚本的可靠性设计

- 4 MiB 分块，每块独立一次 ROM 会话；单块失败只重读该块。
- 每块有硬超时（按字节数/波特率估算的 2 倍 + 60 秒），卡死会被杀掉重试。
- 连续失败会自动把波特率降到 230400 再试。
- 已存在的、大小正确的块会被复用（断点续读）。
- 两遍完整读取必须字节数一致且 SHA-256 相同，才写 `manifest.json` 并标记 `verified`。
- 命令白名单只允许 `flash_id` 与 `read_flash`；脚本里没有任何写入路径。
