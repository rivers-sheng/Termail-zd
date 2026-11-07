# Termail v6.2.0_final 更新日志

## 更新
- 应用名改为 `Termail_final`，Android 标签与 `AndroidManifest` 同步更新，`versionCode` 保持 12
- 状态栏、提示符、会话管理、日志上色等沿用 6.2.0 基础
- `help` 说明表小幅补充
- `cat` 增加 5MB 大小限制并截断提示
- `ls` 增强配色
- `alias` 支持 `alias a='ls | grep'` 带引号含管道写法
- `proot-mirror` 的 DNS 改为 `119.29.29.29`，并在 `pre-position` 中补充 `set-dns` 说明
- 游戏（贪吃蛇 / 俄罗斯方块 / 2048）加入最高分持久化（`QSettings`），贪吃蛇网格尺寸调整

## 新增
- 新增源文件 `commands/many4.cpp`：`open`、`share`、`mount-sd`、`ip-info`、`http-get`、`port-check`、`speedtest`、`meminfo`、`cpu-temp`、`battery`、`disk-usage`、`process-top`、`kill-all`、`md5`、`json-format`、`hex-view`、`diff-color`、`title`、`copy`、`paste-clip`、`bell`、`notify`、`font-list`、`pass-gen`、`base64-decode`、`url-encode`、`url-decode`、`timestamp`、`random-str`、`uuid-batch`、`game-score`、`pong`、`session-rename`、`session-info`、`session-export`、`watch-cpu`、`watch-mem`、`watch-battery`、`watch-net`
- 新增源文件 `commands/config_commands.cpp`：`backup`、`restore`、`export-config`、`import-config`、`reset-all`、`config-info`
- 新增源文件 `commands/about_commands.cpp`：`welcome`、`about`、`changelog`、`tips`、`version`
- 新增源文件 `commands/enhanced_commands.cpp`：`multi`、`parallel`、`repeat`、`benchmark`、`seq-exec`、`timeit`、`xarg`、`until`、`now`、`countdown`
- 新增源文件 `commands/notify_commands.cpp`：`notify-on`、`notify-off`、`wakelock`、`notify-status`（Android 通知栏与唤醒锁）
- `clip`（剪贴板 get/set/clear）、`brightness`（屏幕亮度）、`page`（分页查看）系统命令
- 主程序新增崩溃日志（`crash.log`）、Android 外部分享文本接收、启动前台通知服务
- 进程并发上限（同时运行超过 10 个进程告警）、命令未找到时的智能纠错建议（`suggestCommand`）
- 命令操作写入 `.termail_cmd.log`（超 1MB 自动截断），历史记录异或加密保存（`saveHistory`）
- Android 震动反馈（执行命令时 50ms）
- 高危命令清单扩充（`dd if=`/`dd of=`、`wipefs`、`mkswap`、`chmod 777`、`chown root` 等）
- SSH 增强：`ssh-host`（add/list/rm/edit/show 主机管理，`hosts.json` 持久化）、`ssh-connect`、`ssh-config-gen`、`ssh-keygen-pass`、`ssh-tunnel`（端口转发）
- 资源新增 `resources/proot/ubuntu.tar`
- Java 新增 `TermailService.java`（前台通知服务、唤醒锁切换）、`strings.xml`（快捷方式文案）
- `AndroidManifest` 新增前台服务、唤醒锁、通知、分享接收 intent-filter

## 删除
- 扫雷游戏（`games/minesweeper.*`）及其相关命令
- 旧版 `Termail_v6.2` 应用名