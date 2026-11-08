# Termail v6.4.0 更新日志

## 更新
- 版本号从 6.2.0 升级至 6.4.0，应用名简化为 `Termail`
- PRoot 回归单发行版 Alpine 架构，`proot-setup` 增加完整的前置检查与失败回滚（busybox 缺失、资源为空、解压超时均自动清理）
- `proot-mirror` 的 DNS 改为 `119.29.29.29`，并新增 `set-dns` 命令设置自定义 DNS
- `distro` 简化为仅查看 Alpine 状态
- `serve` HTTP 服务器重写为完整文件管理器：支持浏览、下载、上传、新建文件夹、删除、目录导航，带响应式 Web 界面与请求体完整性等待
- `network2_commands.cpp` 移除 `netscan`、`mget` 命令
- 命令执行时超过 1 秒会附加 `[耗时]` 提示
- 重定向解析改用正则，支持含空格的命令与精确分离文件名
- 引号处理改为记录引号类型，避免混合引号误判
- `many4.cpp` 保留并整合大量实用命令
- Android `versionCode` 更新为 13，标签更新为 `Termail`

## 新增
- Lua 5.4 脚本引擎支持
  - 新增 `lua/` 子目录静态库（完整 Lua 源码 + 独立 CMake 子工程）
  - 新增 `lua/lua_engine.*` 引擎封装
  - 新增 `commands/lua_commands.cpp`：`lua`（运行脚本 / `lua -e "代码"`）、`luainfo`、`luademo`（生成示例脚本）
- 新增图形化编辑器
  - `editor/editor_dialog.*`：全屏编辑器，支持保存、查找、替换、Ctrl+S/F/H 快捷键、修改提示、关闭前保存确认
  - `editor/editor_commands.cpp`：`edit`、`nano`、`create` 命令
- `registerEditorCommands` / `registerLuaCommands` 命令注册分类
- `proot-setup` 的 `busybox` 前置依赖检查

## 删除
- `network2_commands.cpp` 中的 `netscan`（局域网扫描）与 `mget`（多线程下载）命令
- PRoot 的 Ubuntu 双发行版支持（`distro` 切换、ubuntu rootfs 解压逻辑、`resources/proot/ubuntu.tar` 资源引用）
- 游戏相关命令与源文件（`games/tetris.*`、`games/snake.*`、`games/game2048.*`、`commands/game_commands.cpp`）
- 旧版贪吃蛇 / 俄罗斯方块 / 2048 最高分记录相关命令
- `resources/proot/ubuntu.tar` 资源打包项