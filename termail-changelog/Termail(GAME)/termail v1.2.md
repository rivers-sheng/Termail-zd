# Termail Games v1.2.0 更新日志

## 更新
- 版本号从 1.1.0 升级至 1.2.0，`CMakeLists`、`main.cpp`、`AndroidManifest` 同步更新
- 游戏表在 `htmlgames.cpp` 与 `help_commands.cpp` 两处同步扩充，编号范围扩展到 30
- 游戏列表逻辑统一：`help_commands.cpp` 与 `htmlgames.cpp` 共用同一份 `g_games` 数据源
- Android `versionCode` 更新为 3，`versionName` 更新为 1.2.0
- 说明：`config.h` 中 `APP_VERSION` 仍为 "1.1.0"，建议下次一并修正（此处仅记录状态，不视为 bug）

## 新增
- 新增 2 个 HTML 游戏（编号 29–30）：`dou-di-zhu`（斗地主）、`guard-the-blue`（守护蔚蓝）
- `guard-the-blue` 游戏即作者作者的获奖作品《守护蔚蓝》，获"深蓝百万里提升海洋素养数智创意传播大赛"赛道三·数智交互体验赛道初中组二等奖

## 删除
- 无