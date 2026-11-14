# Termail v6.6.0 更新日志

## 更新
- 版本号从 6.5.1 升级至 6.6.0，`CMakeLists`、`config.h`、`main.cpp` 中的应用版本号同步更新
- **移除语言国际化系统**：彻底移除 `lang.h` / `lang.cpp` 及 `lang_dict.json` 相关逻辑，回归纯中文界面，提升终端输出性能，避免中英混搭乱码
- **废弃 PRoot 容器**：彻底移除 proot 相关资源与代码，移除对内置 busybox 的依赖，有效减小 APK 体积
- **系统底层调用重构**：`bb` 命令改为直接调用系统 `toybox`，并在调用失败时给出明确提示；`proot-setup` 和 `proot-shell` 被彻底移除
- **命令分类与优化**：
  - `help` 命令拆分为“核心命令”与“外部组件命令”两部分展示，界面大幅精简
  - 新增 `more` 命令，用于查看 300+ 个依赖外部环境的高级扩展命令（如 `gcc`、`python`、`ssh` 等）
  - 核心命令列表 (`getCoreBuiltins`) 提取为全局函数，消除代码重复
- **HTTP 服务器大幅增强**：
  - **全新运行器**：新增 `/run` 路由，支持在浏览器直接运行 HTML 文件，严格受权限限制（非 `root` 且无 `X` 权限时拒绝运行）
  - **重命名与移动**：新增文件重命名（`/rename`）和跨目录移动（`/move`）功能
  - **媒体预览**：新增 `/raw` 路由，支持图片（jpg/png/gif/webp）及视频（mp4）的在线预览与播放
  - **静默刷新**：移除 `<meta refresh>` 强制整页刷新，改为前端 JS 定时请求 `/api/online_count`，实现无感实时在线人数更新
  - **日志持久化**：所有 HTTP 请求实时写入 `serve-log/YYYY-MM-DD.log`，并记录 IP 与用户信息
  - **IP 黑名单**：新增 `/admin/blacklist` 功能，支持在控制台封禁/解封 IP
  - **控制台重构**：新增返回按钮，拆分在线状态、黑名单、日志查看、账户管理四大模块；支持独立修改 Root 密码
- **文件操作**：
  - **Root 权限删除**：`rm` / `rmdir` 改为通过 `su -c` 执行，实现真正的底层强制删除
  - **取消系统保护拦截**：彻底移除 `isSystemProtectedPath` 拦截，遵循“Linux 极客精神”，交由用户自行确认
- **构建环境适配**：移除对 `libbusybox.so` 的打包与编译配置，APK 不再包含冗余的底层二进制文件

## 新增
- 新增 `more` 命令，用于查看外部扩展命令列表
- 新增 `lang` 命令（已废弃）及对应的中英字典加载机制
- `about` 命令更新版本号及开源协议信息
- `changelog` 命令更新指向地址
- HTTP 后台新增 `/run`、`/raw`、`/rename`、`/move`、`/api/online_count` 路由
- HTTP 控制台新增 IP 黑名单输入框及日志查看日期选择框

## 删除
- 移除 `utils/lang.cpp` 与 `utils/lang.h`
- 移除 `resources/lang_dict.json` 资源打包
- 移除 `resources/busybox/busybox`、`resources/proot/proot`、`resources/proot/alpine.tar` 等资源
- 移除 CMake 中 Android 平台对 `libbusybox.so` 的 `POST_BUILD` 复制逻辑
- 移除 `commands.h` 及 `commands.cpp` 中与 PRoot / Many1~4 / Lang 相关的函数声明与调用
- 移除 `terminal.cpp` 及 `main.cpp` 中对 `g_isEnglish` 及 `loadLanguageDict` 的引用
- 移除 `system_commands.cpp` 中多余的组件检测与 `lang` 命令