# Termail v6.1.0 更新日志

## 更新
- 版本号从 6.0.0 升级至 6.1.0，应用名改为 `Termail_v6.1`
- 超时机制调整：`runExternal` / `runSystemCommand` 默认超时改为 `0`（不限时），仅传入正数时才启动定时器
- `ping`、`proot-shell` 改为不限时执行，`tracert` 仍保留 60 秒
- 欢迎信息新增"辅助制作:DeepSeek Chat"署名
- Android `versionCode` 更新为 10，新增 `MANAGE_EXTERNAL_STORAGE` 与媒体读取权限，开启 `requestLegacyExternalStorage`

## 新增
- 新增 `commands/proot_commands.cpp`：PRoot 容器环境支持
  - `proot-setup`：释放内置 proot 与 alpine rootfs 并解压安装
  - `proot-shell`：进入 PRoot 执行命令（不限时）
  - `proot-mirror`：设置 DNS 与阿里云 Alpine 镜像源
  - `proot-install` / `proot-update`：在 PRoot 内安装软件、更新索引
  - `proot-status` / `proot-remove`：查看状态、删除环境
- 资源新增 `resources/proot/proot`、`resources/proot/alpine.tar`
- `storage` 命令回归：列出并切换 Android 存储目录
- `root` 命令：检测设备 root 状态
- `lsroot` 命令：查看根目录
- `stop` 命令：停止当前运行中的进程

## 删除
- 无