# Termail v6.2.0 更新日志

## 更新
- 版本号从 6.1.0 升级至 6.2.0，应用名改为 `Termail_v6.2`
- 主界面新增状态栏，实时显示当前目录、磁盘占用与会话编号（每 2 秒刷新）
- 提示符改为带会话编号格式 `[会话ID] 用户:路径$`
- `ls` 增强配色：目录蓝、可执行文件绿、符号链接青、隐藏文件灰
- 管道/重定向执行改为无超时（`waitForFinished(-1)`）
- `log` 系统日志新增按日志级别着色（E/F 红、W 橙黄、D 灰、V 深灰、I 白）
- `log-termail` 支持 `类型|内容` 前缀输入并按类型着色（错误/警告/调试/提示）
- 别名与自定义命令改为持久化存储（新增 `aliases.json`）
- 欢迎信息补充 `session new` 提示
- Android `versionCode` 更新为 11

## 新增
- 多会话管理：`session` / `snew` / `slist` / `ssw` / `skill`，支持新建、列出、切换、删除会话并保留各自目录与历史
- 新增源文件 `commands/session_commands.cpp`、`commands/network2_commands.cpp`、`commands/ssh_commands.cpp`、`commands/game_commands.cpp`
- 内置三款游戏：`tetris`（俄罗斯方块）、`snake`（贪吃蛇）、`g2048`（2048），以及 `games` 游戏菜单
- 新增游戏源文件 `games/tetris.*`、`games/snake.*`、`games/game2048.*`
- `serve` / `stop-serve`：内置 HTTP 文件服务器（支持目录列表）
- `netscan`：局域网存活主机扫描
- `mget`：异步 HTTP 下载
- SSH 密钥工具：`ssh-keygen`、`ssh-pub`、`ssh-list`、`ssh-copy`、`ssh-rm`
- `root` / `lsroot`：root 检测与根目录查看
- `stop`：停止当前运行进程
- 状态栏磁盘信息（`/data` 或 `/`）

## 删除
- 扫雷游戏及其 `mine` / `flag` 命令与 `games/minesweeper.*` 源文件
- `storage` 存储目录快捷切换命令
- 彩蛋触发逻辑（`game` / `play` / `xyzzy` / 科乐美秘技）内容置空