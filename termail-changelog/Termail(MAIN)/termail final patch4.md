# Termail v6.6.0 更新日志

## 更新
- 版本号从 6.5.1 升级至 6.6.0，`CMakeLists`、`config.h`、`main.cpp` 中的应用版本号同步更新
- **核心底层重构：彻底移除 PRoot 与内置 busybox**
  - 移除 `proot_commands.cpp` 及相关资源，彻底摆脱 Android 10+ 的 `noexec` 限制与 SELinux 困扰
  - `busybox` 改为直接调用系统底层 `toybox`，`get-busybox` 改为提示无需安装
- **新增自检系统与白名单动态分类**
  - 启动时在状态栏显示进度，并异步检测系统的底层命令兼容性
  - 将纯 C++ 内置命令与系统确定的命令（如 `ping`、`vi`、`tee`）直接写入安全白名单（免测）
  - 不再依赖 `QProcess` 强制测试带参命令（如 `ping`），彻底杜绝误杀
  - 通过底层物理路径探测 (`access`) 快速识别外部命令，保证 UI 绝不卡死
- **`help` 与 `more` 命令全面重写**
  - `help` 命令现在只展示 242 个“核心命令”列表，并支持 `help <命令名>` 查询用法及模糊筛选
  - 新增 `more` 命令，专门展示 118 个“外部组件命令”（如 `gcc`、`python`、`ssh` 等），支持关键词筛选
  - 废弃硬编码的 `helpMap`，改为从注册表动态读取 `desc` 与 `usage`
- **无参提示全面补全**
  - 修复了所有“无参数时静默失败”的命令（如 `rev`、`join`、`tee`、`yum` 等），全都会输出 `用法: xxx` 提示
- **HTTP 服务器（`serve`）深度优化**
  - 界面 UI 彻底丝滑：引入 CSS 动画、悬浮卡片、Toast 提示
  - 新增“运行器”功能：可通过网页直接运行 HTML 文件（严格受 `X` 权限控制）
  - 新增“重命名”、“移动”文件功能，修复了上传文件至子目录的错误
  - 新增日志持久化：所有请求实时写入 `serve-log/YYYY-MM-DD.log`
  - 新增实时在线状态、IP 黑名单、隐藏上传进度条
  - 重构控制台页面，新增返回按钮，修复 Root 改密死循环 Bug
  - 密码文件（`http_users.json`）加入严格黑名单，非 root 用户绝对无法读写
- **编辑器 UI 修复**
  - 顶部工具栏改回 `QGridLayout` 自适应布局，解决移动端按钮溢出问题
  - 真正实装了左侧行号栏，并与主编辑区滚动同步
- **文件操作调整**
  - `rm` / `rmdir` 改为底层 `su -c` 执行真实删除，防止假删除
  - 移除针对根目录及系统目录的硬性拦截，交由用户自行确认（符合 Linux 极客精神）
- **底层防崩溃修复**
  - 重构 `runExternal` 的 `QProcess` 生命周期管理，彻底修复 `ping` 后点 `stop` 引起的 `Scudo` 内存崩溃
  - 修复 `confirm` 高危命令确认的死循环

## 新增
- 新增 `utils/selfcheck.h`，用于底层命令的自检与白名单写入
- 新增 `more` 命令，用于查看 118 个外部组件命令
- 新增 `catmore` 命令，用于替代原本的 `more` 文件查看功能（原 `more` 命令已被重构为扩展列表）
- HTTP 服务器新增 `/run`、`/raw`、`/rename`、`/move`、`/api/online_count` 路由
- HTTP 控制台新增 IP 黑名单、日志查看、用户管理快捷操作面板
- 编辑器新增 `QPlainTextEdit` 行号栏及同步滚动

## 删除
- 移除 `commands/proot_commands.cpp`
- 移除 `resources/busybox/busybox`、`resources/proot/proot`、`resources/proot/alpine.tar`
- 移除 `utils/lang.cpp` 与 `utils/lang.h`（废弃国际化支持）
- 移除 CMake 中 Android 平台对 `libbusybox.so` 及 `proot` 的 `POST_BUILD` 拷贝逻辑
- 移除 `terminal.h` 及 `terminal.cpp` 中的跨线程 `QProcess` 异步检测逻辑
- 移除 `system_commands.cpp` 中大量硬编码的 `helpMap`
- 移除 `many1.cpp` 至 `many4.cpp`，合并至 `many_extra.cpp`
