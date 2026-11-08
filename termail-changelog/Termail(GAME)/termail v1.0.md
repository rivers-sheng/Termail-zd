分支说明：Termail Games v1.0.0
确认这是从 Termail 主线分出的独立分支，项目名 TermailGames VERSION 1.0.0，版本号 1.0.0，Android 包名 com.termail.spical（与主线 com.termail.terminal 不同），应用标签 Termail Games。

既然你说另说，那这里只做分支定位记录，不展开逐文件对比。

分支概况
定位：从 v6.5.0 剥离出来的「HTML 游戏启动器」精简版

只保留两个命令模块：help_commands.cpp（含游戏表 + 启动逻辑）、fun_commands.cpp（计算器、字符画、数学、随机数等纯终端趣味命令）

核心机制：18 个 HTML 游戏通过 Android GameWebView Activity 启动（JNI startActivity + ComponentName），help <游戏名/编号> 读取 I.txt 显示介绍

不包含：文件操作、网络、SSH、PRoot、Lua、编辑器、HTTP 服务、多会话、自定义命令、配置持久化等主线全部功能

终端层极简：无历史持久化、无管道/重定向、无别名、无 Tab 补全路径、无 CommandHelper/Utils::runExternal

关键差异点（相对主线 v6.5.0）
commands.h 只声明 registerHelpCommands / registerFunCommands，新增 suggestCommand

registerAllBuiltins 只调用这两个注册函数

terminal.cpp 的 executeInternalCommand 无引号扫描、无管道、无重定向、无别名、无递归深度，命令未找到时调 cmdRegistry->suggestCommand

utils 精简为 formatSize / readTextFile / writeTextFile / configDir，无进程执行

CMakeLists 只链 Qt6::Widgets Core Gui（无 Network、无 lua）

资源只有 LICENSE/GPL.txt（无 busybox、proot、custom_commands.json）

htmlgames.cpp 和 help_commands.cpp 里各有一份相同的 18 游戏表（g_games 与 htmlGames），是重复定义