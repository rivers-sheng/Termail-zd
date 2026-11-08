# Termail v6.2.0_Pro 更新日志

## 更新
- 应用名改为 `Termail_v6.2_Pro`，Android 标签同步更新，`versionCode` 更新为 12
- PRoot 改为多发行版架构：Alpine / Ubuntu 共用一个 proot 二进制，通过 `proot-env/distro` 标记文件记录当前发行版
- `proot-shell`、`proot-mirror`、`proot-install`、`proot-status` 均按当前发行版自动选用对应 rootfs、shell 与包管理器
- Ubuntu 使用 `/bin/bash`，Alpine 使用 `/bin/sh`
- `pre-position` 命令升级为完整的前置命令清单与安装顺序指引

## 新增
- Ubuntu 发行版支持：资源新增 `resources/proot/ubuntu.tar`
- `distro` 命令：查看 / 安装并切换发行版（`distro list` / `alpine` / `ubuntu`），首次切到 Ubuntu 时自动分块解压安装
- `proot-mirror` 新增 Ubuntu 阿里云 `ubuntu-ports` 镜像配置
- `pre-position` 命令：查看全部前置命令及推荐安装顺序

## 删除
- 无