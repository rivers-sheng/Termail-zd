# Termail v6.5.0 更新日志

## 更新
- 版本号从 6.4.0 升级至 6.5.0（`CMakeLists` 与 `config.h` 同步），但 `main.cpp` 中 `setApplicationVersion` 仍为 "6.4.0"，建议下次一并修正
- `Utils::runExternal` 进程生命周期管理重构：改用共享"已清理"标志防止 `finished` / `errorOccurred` 回调重复清理，定时器在两条退出路径上都会销毁
- `executeRedirect` 改为用正则精确匹配重定向符（不在引号内），支持含空格的命令与更准确的文件名解析
- `executeInternalCommand` 引号处理改为记录引号类型，避免混合引号误判
- `serve` HTTP 服务器重大重写：新增完整账户体系（默认 `root / 2510`）、登录会话 Cookie、读/写/运行权限、在线预览编辑、搜索、批量删除、用户管理后台（创建/改权限/改密码/授予/撤销 ROOT/删除，均需 root 密码二次验证）
- PRoot 统一为 Alpine 单发行版，`proot-setup` 强化前置检查与失败回滚
- `proot-mirror` 的 DNS 改为 `119.29.29.29`
- Android `versionCode` 更新为 14

## 新增
- HTTP 服务器用户系统：`http_users.json` 持久化、SHA-256 加盐哈希密码、会话 token 管理
- HTTP 服务器页面：登录页、文件管理页（搜索/预览/编辑/上传/新建/删除/批量删除）、用户管理页
- `set-dns` 命令：查看 / 设置 PRoot 内 DNS
- Lua 引擎、图形化编辑器、配置备份恢复等沿用 6.4.0 能力

## 删除
- 无