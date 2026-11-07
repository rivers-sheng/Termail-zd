# Termail v5.1.0 更新日志

## 更新
- 版本号从 5.0.0 升级至 5.1.0，应用名改为 `Termail_v5.1`
- 欢迎信息提示由 `cmenu` 改为 `get-busybox`
- `help` 命令增强：支持 `help <命令名>` 查看单条命令的具体用法，新增完整命令说明表
- Android `versionCode` 更新为 6，应用标签更新为 `Termail_v5.1`

## 新增
- 内置 busybox 二进制资源 `resources/busybox/busybox`（300+ Linux 命令扩展）
- `get-busybox` 命令：释放并安装内置 busybox 到当前目录并赋予可执行权限
- `bb` 命令：通过 busybox 调用外部 Linux 命令
- `cmenu` 改为 busybox 使用说明菜单

## 删除
- `cmenu` 原有的完整命令列表内容（改为 busybox 指引）
- 部分文件中重复的 `#include <QStorageInfo>` 冗余引用