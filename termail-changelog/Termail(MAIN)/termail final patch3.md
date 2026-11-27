# Termail v6.5.1 更新日志

## 更新
- 版本号从 6.5.0 升级至 6.5.1，`CMakeLists`、`config.h`、`main.cpp` 中的应用版本号同步更新
- 编辑器的行号功能真正实现：改用独立 `QPlainTextEdit` 行号栏，与主编辑区滚动同步，可通过"行号"按钮开关
- 编辑器顶部工具栏改为 `QGridLayout`，适配窄屏（查找框与按钮分行排列）
- 修复高危命令确认的死循环问题：`confirm` 命令单独提前处理，待确认命令唤醒时直接放行，避免反复拦截
- `proot-setup` 解压改为动态探测系统工具（`/system/bin/tar` → `toybox tar` → `busybox tar`），不再依赖内置 busybox
- `proot-shell` / `proot-mirror` / `set-dns` 命令注入 `PROOT_NO_SECCOMP=1` 并挂载 `/sdcard`，提升兼容性；默认进入 `/bin/sh`
- `proot-status` / `distro` 的安装检测改以 `/bin/sh` 为准
- `bb` 命令改为直接调用系统 `toybox` / `busybox`，`get-busybox` 改为提示无需手动安装
- HTTP 服务器大幅增强：
  - 新增"新建文件"（`/touch`）、"批量下载"（打包到时间戳目录）
  - 上传支持自定义目标目录（`cur_dir`）、带进度条
  - 搜索改为递归搜索（上限 500 条）
  - 目录列表上限 1000 条
  - 管理接口合并为 `/admin/manage`（改权限/改密码/授予/撤销/删除）
  - 非 root 用户禁止访问 `http_users.json`、`.termail_cmd.log` 等敏感文件
  - 界面样式重构（CSS 变量、表格圆角、徽章等）
- 文件操作安全强化：
  - 新增危险路径拦截（`/`、`/sdcard`、`/storage` 等）与系统保护目录检测（`/system`、`/vendor`、`/data/adb` 等需 root）
  - `rm` 改用底层 `rm -rf` 执行并校验退出码；`rmdir` 增加同类检查
- `about` 命令 GitHub 地址更新为 `rivers-sheng`，许可改为 AGPL；`changelog` 改为指向 GitHub 仓库
- CMake 移除对 busybox 资源的打包；Android `versionCode` 更新为 15

## 新增
- `editor_dialog.h` 新增 `QPlainTextEdit* lineNumberArea` 及 `updateLineNumberAreaWidth` / `updateLineNumberArea` 槽函数
- CMake 中 Android 平台将 busybox 复制为 `libbusybox.so` 的 `POST_BUILD` 自定义命令（本版源码中该步骤随资源移除而失效）
- `pre-position` 清单更新（提示改用 `bb` 直连、proot 安装流程不变）

## 删除
- 移除 `commands.h` 中 `help` 命令里的 `confirm` 帮助条目（改由终端层直接处理）
- 移除 `resources/busybox/busybox` 资源打包项
- 移除 CMake 中实际未生效的 busybox 复制逻辑所依赖的资源