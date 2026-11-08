# Termail v6.0.0 更新日志

## 更新
- 版本号从 5.4.0（beta）升级至 6.0.0，应用名改为 `Termail_v6`
- 字体配置简化为 `monospace` / `Consolas` 直接宏定义（移除字体探测函数）
- `help` 命令说明表精简，聚焦常用命令
- `sysinfo` 输出格式精简（合并为单块显示）
- `hostname` 仅保留 `-I` / `-i` 显示 IP 与默认主机名
- `alias` 简化为单条赋值，不再循环处理多个参数
- `find` 移除 `-maxdepth` 解析与系统目录排除，仅保留结果数上限
- `help`、`license` 输出内容精简
- Android `versionCode` 更新为 9，标签更新为 `Termail_v6`

## 新增
- 无

## 删除
- `get-busybox` 的 SHA256 校验与可执行性验证逻辑
- `storage` 命令（存储目录快速切换）
- 大量 `help` 命令说明条目（仅保留常用命令）
- `many1` / `many2` / `many3` 中部分冗余命令（`flex`、`unzipsfx`、`tftp`、`chage`、`lastlog`、`init`、`runlevel`、`halt`、`iwlist`、`resolvectl`、`hostid`、`getconf`、`unset`、`log` 等）
- `find` 的最大深度解析与系统目录保护
- `addcmd` 循环处理多个别名参数的能力