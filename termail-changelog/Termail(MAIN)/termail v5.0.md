# Termail v5.0.0 更新日志

## 更新
- 版本号从 4.0.0 升级至 5.0.0
- 主界面重构：由 `QMainWindow + 菜单栏` 改为纯 `QWidget` 终端界面（移除菜单栏、工具栏、状态区）
- 终端配色统一为黑底绿提示符风格，配色常量集中到 `config.h`
- 字体配置简化，默认字号由 11 调整为 12
- 命令注册表接口全面切换到 `TerminalWidget`，注册流程精简
- 扫雷重写为纯文本终端玩法（`mine` / `flag` 命令），移除独立 GUI 扫雷窗口
- `calc` 改为系统 shell 算术运算，`nslookup`/`ifconfig` 改为纯 Qt 网络接口实现
- 欢迎信息与版权标识更新（GPL 声明、作者信息）
- Android 清单精简，屏幕方向由 `landscape` 改为 `unspecified`

## 新增
- `cmenu` 命令：完整命令菜单一览
- `license` 命令：查看 GPL 许可证
- `setfont` 命令：运行时调整字号（8–32）
- `addcmd` / `delcmd` / `listcmd` 命令：命令行的自定义命令管理
- `mine` / `flag` 命令：终端版扫雷游戏
- GPL 许可证资源文件 `LICENSE/GPL.txt`
- 终端版扫雷引擎 `MinesweeperEngine`（`games/minesweeper.*`）
- `registerCmenuCommands` 命令注册分类

## 删除
- 独立 GUI 扫雷窗口及相关头文件引用
- 自定义命令管理对话框 `custom_commands_dialog.*`
- 计算器 GUI 窗口 `openCalculator`
- 菜单栏及全部菜单项（文件/编辑/视图/工具/帮助）
- 管理员权限检测与提权重启逻辑（`isAdmin` / `runAsAdmin`）
- 平台安装规则、Windows 图标资源（`termail.rc`）及 macOS 打包配置
- 部分文件命令：`ln`、`find` 的深度参数、`which` 的外部路径查找分支
- 部分 ASCII 艺术（fish / dragon）、摩斯电码完整映射
- `CommandRegistry` 头文件中的 `term` 冗余引用与部分未用成员