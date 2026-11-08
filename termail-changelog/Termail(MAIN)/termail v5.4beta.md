# Termail v5.4.0-beta 更新日志

## 更新
- 版本号从 5.2.0 升级至 5.4.0（beta），应用名改为 `Termail_v5.4bata`
- `help` 命令说明表全面扩充，补充 `confirm`、`storage`、`ssh` 等新命令说明
- `sysinfo` 增加 CPU 核心数与磁盘容量显示
- `hostname` 支持 `-I` / `-i` 显示 IP、`-f` 显示主机名
- `uname` 支持 `-a` 完整信息
- `seq` 支持递减序列与步长校验
- `rm` 支持 `-r` / `-f` 参数，并加入系统目录保护
- `find` 支持 `-maxdepth` 并排除系统目录、限制结果数
- `tree` 增加目录深度限制（3 层）
- `license` 仅显示前 50 行并提示完整文件路径
- `addcmd` 增加命令名合法性校验
- 字体配置改为动态探测（Android 回退 monospace）

## 新增
- 交互式进程模式：`ssh` 命令进入交互会话，输入直发进程
- 高危命令确认机制：`rm -rf`、`dd`、`mkfs`、`fdisk`、`shutdown` 等需 `confirm` 二次确认
- `confirm` 命令：确认执行待定高危命令
- `storage` 命令：快速切换 Android 存储目录
- 命令历史容量上限（1000 条），退出时保存自定义命令
- 自定义命令加载时 JSON 损坏自动备份（`.bak`）并清空
- 字体大小持久化（`QSettings` 保存/加载）
- 别名展开防自引用与防递归（上限 5 层）
- 命令分隔符处理：支持 `&&`、`||`、`;`
- 引号内 `>`、`|` 不再被误判为管道/重定向
- 管道写入等待与输出截断保护（10MB 输入 / 100KB 输出）
- `get-busybox` 增加 SHA256 校验、可执行性验证、失败自动清理
- `log` / `log-termail` 日志窗口保留与切换逻辑
- 扫雷改用 `QRandomGenerator` 与 `QQueue`，棋盘渲染改用 ASCII 边框
- `prime` 改用埃氏筛（上限 100 万），`factor` 改用 `qint64`

## 删除
- 命令补全的 Tab 循环切换（改回单次匹配）
- 部分冗余的命令注册（`truncate`、`stat` 等改为外部调用）
- `shred` 在 Android 上的外部调用（改为提示不可用）

## Bug 说明
- `nslookup` 中 `QHostInfo::lookupHost` 回调捕获了栈上 `QEventLoop` 引用并用 `bool* done` 标记，但回调为异步执行，存在悬垂引用风险
- `timeout` 命令在 `system_commands.cpp` 与 `text_commands.cpp` 中重复注册，后注册的会覆盖前者，两处行为不一致
- `terminal.cpp` 中 `tabMatchIndex` 声明于 `terminal.h` 但未初始化，`onTabComplete` 循环切换相关逻辑残留
- `rm -rf` 的高危检测仅匹配以 `rm -rf` 开头的命令，`rm -r -f` 等变体不会被拦截
- `executeInternalCommand` 的 `&&`/`||`/`;` 分割未区分短路语义，`&&` 后命令会无条件执行
- `get-busybox` 的 `expectedHash` 为 TODO 占位值，实际校验会失败导致安装被拒