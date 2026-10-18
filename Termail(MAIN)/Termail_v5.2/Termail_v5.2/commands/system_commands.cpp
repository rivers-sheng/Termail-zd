#include <QApplication>
#include "commands.h"
#include "terminal/terminal.h"
#include "utils/utils.h"
#include "games/minesweeper.h"

#include <QProcess>
#include <QSysInfo>
#include <QStorageInfo>
#include <QDateTime>
#include <QThread>
#include <QRandomGenerator>

void CommandRegistry::registerSystemCommands(TerminalWidget* ui) {

    // help
    registerCommand("help", [this, ui](const QStringList& args) {
        if (args.isEmpty()) {
            QStringList cmds = getCommandNames();
            cmds.sort();
            ui->appendOutput("=== Termail命令列表 ===\n\n", ACCENT_COLOR);
            int count = 0;
            for (const QString& c : cmds) {
                ui->appendOutput(QString("%1").arg(c, -14));
                if (++count % 5 == 0) ui->appendOutput("\n");
            }
            ui->appendOutput(QString("\n\n共 %1 个命令\n").arg(cmds.size()), INFO_COLOR);
            ui->appendOutput("输入 help <命令名> 查看具体用法\n", INFO_COLOR);
            return;
        }

        QString cmd = args[0].toLower();
        QMap<QString, QString> helpMap = {
            // 基础
            {"help", "help [命令名] - 查看命令帮助"},
            {"exit", "exit - 退出终端"},
            {"quit", "quit - 退出终端"},
            {"logout", "logout - 注销退出"},
            {"clear", "clear - 清屏"},
            {"cls", "cls - 清屏"},
            {"reset", "reset - 重置显示"},
            {"echo", "echo <文本> - 输出文本"},
            {"date", "date - 日期时间"},
            {"time", "time - 当前时间"},
            {"whoami", "whoami - 当前用户"},
            {"env", "env - 环境变量"},
            {"printenv", "printenv - 打印环境变量"},
            {"sysinfo", "sysinfo - 系统信息"},
            {"cmenu", "cmenu - busybox菜单"},
            {"license", "license - GPL许可证"},
            {"setfont", "setfont <8-32> - 设置字号"},
            {"log", "log - 系统日志窗口"},
            {"log-termail", "log-termail - Termail日志窗口"},
            {"exit-log", "exit-log - 关闭日志窗口"},

            // 文件
            {"ls", "ls [-a] [-l] [目录] - 列出目录"},
            {"dir", "dir - 列出目录"},
            {"cd", "cd [目录] - 切换目录"},
            {"pwd", "pwd - 当前目录"},
            {"mkdir", "mkdir <目录> - 创建目录"},
            {"rmdir", "rmdir <目录> - 删除空目录"},
            {"rm", "rm <文件> - 删除文件"},
            {"del", "del <文件> - 删除文件"},
            {"touch", "touch <文件> - 创建空文件"},
            {"cat", "cat <文件> - 查看文件"},
            {"type", "type <文件> - 查看文件"},
            {"cp", "cp <源> <目标> - 复制"},
            {"copy", "copy <源> <目标> - 复制"},
            {"mv", "mv <源> <目标> - 移动"},
            {"move", "move <源> <目标> - 移动"},
            {"rename", "rename <旧> <新> - 重命名"},
            {"tree", "tree - 树形目录"},
            {"find", "find <关键词> - 查找"},
            {"grep", "grep <词> <文件> - 搜索"},
            {"wc", "wc <文件> - 统计"},
            {"head", "head [-n N] <文件> - 前N行"},
            {"tail", "tail [-n N] <文件> - 后N行"},
            {"more", "more <文件> - 分页查看"},
            {"less", "less <文件> - 分页查看"},
            {"diff", "diff <文件1> <文件2> - 对比"},
            {"sort", "sort [-r] <文件> - 排序"},
            {"uniq", "uniq <文件> - 去重"},
            {"cut", "cut -d<分隔> -f<字段> <文件>"},
            {"stat", "stat <文件> - 状态"},
            {"file", "file <文件> - 类型"},
            {"du", "du [目录] - 磁盘使用"},
            {"df", "df - 磁盘空间"},
            {"chmod", "chmod <权限> <文件>"},
            {"chown", "chown <用户> <文件>"},
            {"chgrp", "chgrp <组> <文件>"},
            {"which", "which <命令> - 查找位置"},
            {"whereis", "whereis <命令> - 查找位置"},

            // 网络
            {"ping", "ping <主机> - Ping测试"},
            {"curl", "curl <URL> - HTTP请求"},
            {"wget", "wget <URL> - 下载"},
            {"ifconfig", "ifconfig - 网络接口"},
            {"ipconfig", "ipconfig - IP配置"},
            {"netstat", "netstat - 网络连接"},
            {"nslookup", "nslookup <域名> - DNS查询"},
            {"tracert", "tracert <主机> - 路由追踪"},
            {"traceroute", "traceroute <主机> - 路由追踪"},
            {"route", "route - 路由表"},
            {"arp", "arp - ARP表"},
            {"whois", "whois <域名> - WHOIS查询"},
            {"ssh", "ssh <主机> - SSH连接"},
            {"scp", "scp <源> <目标> - 远程拷贝"},
            {"ftp", "ftp <主机> - FTP连接"},
            {"telnet", "telnet <主机> - Telnet"},
            {"nc", "nc <主机> <端口> - Netcat"},
            {"dig", "dig <域名> - DNS查询"},
            {"host", "host <域名> - DNS查询"},
            {"ip", "ip <参数> - 网络配置"},

            // 系统
            {"ps", "ps - 进程列表"},
            {"tasklist", "tasklist - 任务列表"},
            {"top", "top - 系统监控"},
            {"htop", "htop - 交互式监控"},
            {"kill", "kill <PID> - 结束进程"},
            {"killall", "killall <名称> - 按名结束"},
            {"pkill", "pkill <名称> - 按名结束"},
            {"pgrep", "pgrep <名称> - 查找进程"},
            {"jobs", "jobs - 后台任务"},
            {"uname", "uname - 系统信息"},
            {"hostname", "hostname - 主机名"},
            {"uptime", "uptime - 运行时间"},
            {"free", "free - 内存"},
            {"lscpu", "lscpu - CPU信息"},
            {"id", "id - 用户ID"},
            {"groups", "groups - 用户组"},
            {"users", "users - 在线用户"},
            {"w", "w - 谁在登录"},
            {"who", "who - 谁在登录"},
            {"last", "last - 登录历史"},
            {"python", "python [代码] - Python"},
            {"python3", "python3 [代码] - Python3"},
            {"pip", "pip <参数> - pip"},
            {"npm", "npm <参数> - npm"},
            {"git", "git <参数> - git"},
            {"gcc", "gcc <参数> - C编译"},
            {"g++", "g++ <参数> - C++编译"},
            {"make", "make - 构建"},
            {"cmake", "cmake <参数> - CMake构建"},
            {"gdb", "gdb <程序> - 调试"},
            {"java", "java <参数> - Java"},
            {"javac", "javac <参数> - Java编译"},
            {"mysql", "mysql - MySQL"},
            {"psql", "psql - PostgreSQL"},
            {"sqlite", "sqlite - SQLite"},
            {"redis", "redis - Redis"},
            {"alias", "alias [名称=命令] - 别名"},
            {"unalias", "unalias <名称> - 删除别名"},
            {"history", "history - 历史"},
            {"export", "export 变量=值 - 导出"},
            {"source", "source <文件> - 执行脚本"},
            {"apt", "apt <参数> - APT"},
            {"apt-get", "apt-get <参数> - APT"},
            {"yum", "yum <参数> - YUM"},
            {"dnf", "dnf <参数> - DNF"},
            {"pacman", "pacman <参数> - Pacman"},
            {"brew", "brew <参数> - Homebrew"},
            {"systemctl", "systemctl <参数> - 服务管理"},
            {"service", "service <名称> <操作> - 服务"},
            {"shutdown", "shutdown - 关机"},
            {"reboot", "reboot - 重启"},

            // 文本
            {"tr", "tr <集1> <集2> [文本] - 替换"},
            {"rev", "rev <文本> - 反转"},
            {"tac", "tac <文件> - 反向显示"},
            {"nl", "nl <文件> - 行号"},
            {"paste", "paste <文件1> <文件2> - 合并"},
            {"split", "split [-l N] <文件> - 分割"},
            {"join", "join <文件...> - 连接"},
            {"fmt", "fmt <文件> - 格式化"},
            {"sed", "sed 's/旧/新/' <文件> - 替换"},
            {"awk", "awk <脚本> <文件> - 文本处理"},
            {"zip", "zip <压缩包> <文件...> - 压缩"},
            {"unzip", "unzip <压缩包> - 解压"},
            {"tar", "tar <参数> - 归档"},
            {"gzip", "gzip <文件> - 压缩"},
            {"gunzip", "gunzip <文件> - 解压"},
            {"bzip2", "bzip2 <文件> - 压缩"},
            {"bunzip2", "bunzip2 <文件> - 解压"},
            {"hash", "hash <文本> - 哈希"},
            {"base64", "base64 <文本> - Base64"},
            {"uuid", "uuid - UUID"},
            {"random", "random [最小] [最大] - 随机数"},
            {"sleep", "sleep <秒> - 延时"},
            {"watch", "watch <命令> - 重复执行"},
            {"tee", "tee <文件> <文本> - 保存"},
            {"yes", "yes [文本] - 重复输出"},
            {"seq", "seq [起] [步] <止> - 序列"},
            {"shuf", "shuf <项目...> - 随机排列"},
            {"timeout", "timeout <秒> <命令> - 超时"},
            {"crontab", "crontab - 定时任务"},

            // 趣味
            {"calc", "calc <表达式> - 计算器"},
            {"bc", "bc <表达式> - 计算器"},
            {"expr", "expr <表达式> - 计算"},
            {"convert", "convert <值> <单位1> to <单位2>"},
            {"hex", "hex <数字> - 十六进制"},
            {"bin", "bin <数字> - 二进制"},
            {"oct", "oct <数字> - 八进制"},
            {"cowsay", "cowsay [文本] - 牛说"},
            {"fortune", "fortune - 格言"},
            {"banner", "banner <文本> - 横幅"},
            {"figlet", "figlet <文本> - 大字"},
            {"cmatrix", "cmatrix - 矩阵"},
            {"sl", "sl - 火车"},
            {"lolcat", "lolcat [文本] - 彩虹"},
            {"ascii", "ascii [名称] - ASCII艺术"},
            {"morse", "morse [文本] - 摩斯码"},
            {"prime", "prime [上限] - 素数"},
            {"factor", "factor <数字> - 因数"},
            {"fibonacci", "fibonacci [项数] - 斐波那契"},
            {"pi", "pi [小数位] - 圆周率"},
            {"e", "e [小数位] - 自然常数"},
            {"phi", "phi [小数位] - 黄金比例"},

            // 扫雷
            {"mine", "mine [坐标|new|easy|medium|hard] - 扫雷\n  坐标: A1 (行字母+列数字)"},
            {"flag", "flag <坐标> - 插旗\n  如: flag B3"},
            {"game", "game - 扫雷入口"},

            // busybox
            {"get-busybox", "get-busybox - 安装busybox"},
            {"bb", "bb <命令> - busybox调用\n  如: bb ls -la"},
        };

        if (helpMap.contains(cmd)) {
            ui->appendOutput(QString("=== %1 ===\n").arg(cmd), ACCENT_COLOR);
            ui->appendOutput(helpMap[cmd] + "\n");
        } else {
            ui->appendOutput(QString("未找到命令: %1\n").arg(args[0]), ERROR_COLOR);
            ui->appendOutput("输入 help 查看全部命令\n", INFO_COLOR);
        }
    }, "帮助", "help [命令名]");

    // license（独立命令，不嵌在 help 里）
    registerCommand("license", [ui](const QStringList&) {
        QFile file(":/LICENSE/GPL.txt");
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            ui->appendOutput(file.readAll());
            file.close();
        } else {
            ui->appendOutput("GPL 许可证文件未找到\n", ERROR_COLOR);
        }
    }, "查看GPL许可证", "license");

    // exit / quit
    registerCommand("exit", [ui](const QStringList&) {
        qApp->quit();
    }, "退出", "exit");

    registerCommand("quit", [ui](const QStringList&) {
        qApp->quit();
    }, "退出", "quit");

    // clear / cls / reset
    registerCommand("clear", [ui](const QStringList&) { ui->clearScreen(); }, "清屏", "clear");
    registerCommand("cls", [ui](const QStringList&) { ui->clearScreen(); }, "清屏", "cls");
    registerCommand("reset", [ui](const QStringList&) { ui->clearScreen(); }, "重置", "reset");

    // echo
    registerCommand("echo", [ui](const QStringList& args) {
        ui->appendOutput(args.join(' ') + "\n");
    }, "输出文本", "echo <文本>");

    // date / time
    registerCommand("date", [ui](const QStringList&) {
        ui->appendOutput(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss\n"));
    }, "日期时间", "date");

    registerCommand("time", [ui](const QStringList&) {
        ui->appendOutput(QDateTime::currentDateTime().toString("hh:mm:ss\n"));
    }, "时间", "time");

    // whoami
    registerCommand("whoami", [ui](const QStringList&) {
        ui->appendOutput(ui->username + "\n");
    }, "当前用户", "whoami");

    // env
    registerCommand("env", [ui](const QStringList&) {
        QStringList vars = QProcess::systemEnvironment();
        vars.sort();
        for (const QString& v : vars) ui->appendOutput(v + "\n");
    }, "环境变量", "env");

    // sysinfo
    registerCommand("sysinfo", [ui](const QStringList&) {
        ui->appendOutput(QString(
            "系统: %1\n架构: %2\n主机: %3\n用户: %4\n目录: %5\n")
            .arg(QSysInfo::prettyProductName(), QSysInfo::currentCpuArchitecture(),
                 ui->hostname, ui->username, ui->currentDir), ACCENT_COLOR);
    }, "系统信息", "sysinfo");

    // ps / tasklist
    registerCommand("ps", [ui](const QStringList& args) {
        CommandHelper::runExternal("ps", QStringList{"aux"} + args, ui->currentDir, ui);
    }, "进程列表", "ps");

    registerCommand("tasklist", [ui](const QStringList& args) {
        CommandHelper::runExternal("ps", QStringList{"aux"} + args, ui->currentDir, ui);
    }, "任务列表", "tasklist");

    // top
    registerCommand("top", [ui](const QStringList&) {
        ui->appendOutput("top 需要交互式终端\n", ERROR_COLOR);
    }, "系统监控", "top");

    // kill / taskkill
    registerCommand("kill", [ui](const QStringList& args) {
        CommandHelper::runExternal("kill", args, ui->currentDir, ui);
    }, "结束进程", "kill <PID>");

    registerCommand("taskkill", [ui](const QStringList& args) {
        CommandHelper::runExternal("kill", args, ui->currentDir, ui);
    }, "结束进程", "taskkill <PID>");

    // jobs
    registerCommand("jobs", [ui](const QStringList&) {
        ui->appendOutput("无后台任务\n");
    }, "后台任务", "jobs");

    // uname
    registerCommand("uname", [ui](const QStringList&) {
        ui->appendOutput(QString("%1 %2 %3\n")
            .arg(QSysInfo::kernelType(), QSysInfo::kernelVersion(),
                 QSysInfo::currentCpuArchitecture()));
    }, "系统信息", "uname");

    // hostname
    registerCommand("hostname", [ui](const QStringList&) {
        ui->appendOutput(ui->hostname + "\n");
    }, "主机名", "hostname");

    // uptime
    registerCommand("uptime", [ui](const QStringList&) {
        ui->appendOutput("终端运行中...\n");
    }, "运行时间", "uptime");

    // free
    registerCommand("free", [ui](const QStringList&) {
        QStorageInfo storage("/");
        if (storage.isValid())
            ui->appendOutput(QString("总量:%1 可用:%2\n")
                .arg(Utils::formatSize(storage.bytesTotal()),
                     Utils::formatSize(storage.bytesAvailable())));
    }, "内存/磁盘", "free");

    // lscpu
    registerCommand("lscpu", [ui](const QStringList&) {
        ui->appendOutput(QString("架构:%1 核心:%2\n")
            .arg(QSysInfo::currentCpuArchitecture()).arg(QThread::idealThreadCount()));
    }, "CPU信息", "lscpu");

    // id
    registerCommand("id", [ui](const QStringList&) {
        ui->appendOutput(QString("uid=1000(%1)\n").arg(ui->username));
    }, "用户ID", "id");

    // groups
    registerCommand("groups", [ui](const QStringList&) {
        ui->appendOutput(ui->username + "\n");
    }, "用户组", "groups");

    // users
    registerCommand("users", [ui](const QStringList&) {
        ui->appendOutput(ui->username + "\n");
    }, "在线用户", "users");

    // w / who
    registerCommand("w", [ui](const QStringList&) {
        ui->appendOutput(QString("%1 pts/0 %2\n").arg(ui->username)
            .arg(QDateTime::currentDateTime().toString("hh:mm")));
    }, "谁在登录", "w");

    registerCommand("who", [ui](const QStringList& args) {
        ui->executeInternalCommand("w " + args.join(' '));
    }, "谁在登录", "who");

    // last
    registerCommand("last", [ui](const QStringList&) {
        ui->appendOutput(QString("%1 pts/0 still logged in\n").arg(ui->username));
    }, "登录历史", "last");

    // python / python3
    registerCommand("python", [ui](const QStringList& args) {
        CommandHelper::runExternal("python3", args, ui->currentDir, ui);
    }, "Python", "python");

    registerCommand("python3", [ui](const QStringList& args) {
        CommandHelper::runExternal("python3", args, ui->currentDir, ui);
    }, "Python3", "python3");

    // pip
    registerCommand("pip", [ui](const QStringList& args) {
        CommandHelper::runExternal("pip3", args, ui->currentDir, ui);
    }, "pip", "pip");

    // npm
    registerCommand("npm", [ui](const QStringList& args) {
        CommandHelper::runExternal("npm", args, ui->currentDir, ui);
    }, "npm", "npm");

    // git
    registerCommand("git", [ui](const QStringList& args) {
        CommandHelper::runExternal("git", args, ui->currentDir, ui);
    }, "git", "git");

    // vim / vi
    registerCommand("vim", [ui](const QStringList&) {
        ui->appendOutput("简易Vim不可用，请用cat查看文件\n", ERROR_COLOR);
    }, "vim", "vim");

    registerCommand("vi", [ui](const QStringList& args) {
        ui->executeInternalCommand("vim " + args.join(' '));
    }, "vi", "vi");

    // nano
    registerCommand("nano", [ui](const QStringList& args) {
        CommandHelper::runExternal("nano", args, ui->currentDir, ui);
    }, "nano", "nano");

    // make / gcc / g++
    registerCommand("make", [ui](const QStringList& args) {
        CommandHelper::runExternal("make", args, ui->currentDir, ui);
    }, "make", "make");

    registerCommand("gcc", [ui](const QStringList& args) {
        CommandHelper::runExternal("gcc", args, ui->currentDir, ui);
    }, "gcc", "gcc");

    registerCommand("g++", [ui](const QStringList& args) {
        CommandHelper::runExternal("g++", args, ui->currentDir, ui);
    }, "g++", "g++");

    // java / javac
    registerCommand("java", [ui](const QStringList& args) {
        CommandHelper::runExternal("java", args, ui->currentDir, ui);
    }, "java", "java");

    registerCommand("javac", [ui](const QStringList& args) {
        CommandHelper::runExternal("javac", args, ui->currentDir, ui);
    }, "javac", "javac");

    // 数据库客户端
    registerCommand("mysql", [ui](const QStringList& args) {
        CommandHelper::runExternal("mysql", args, ui->currentDir, ui);
    }, "mysql", "mysql");

    registerCommand("psql", [ui](const QStringList& args) {
        CommandHelper::runExternal("psql", args, ui->currentDir, ui);
    }, "psql", "psql");

    registerCommand("sqlite", [ui](const QStringList& args) {
        CommandHelper::runExternal("sqlite3", args, ui->currentDir, ui);
    }, "sqlite", "sqlite");

    registerCommand("redis", [ui](const QStringList& args) {
        CommandHelper::runExternal("redis-cli", args, ui->currentDir, ui);
    }, "redis", "redis");

    // alias
    registerCommand("alias", [ui](const QStringList& args) {
        if (args.isEmpty()) {
            for (auto it = ui->aliases.begin(); it != ui->aliases.end(); ++it)
                ui->appendOutput(QString("alias %1='%2'\n").arg(it.key(), it.value()));
            return;
        }
        for (const QString& a : args) {
            if (a.contains('=')) {
                QStringList p = a.split('=');
                ui->aliases[p[0].trimmed()] = p[1].trimmed().remove('\'').remove('"');
                ui->appendOutput(QString("别名: %1 → %2\n").arg(p[0].trimmed(), p[1].trimmed()), INFO_COLOR);
            }
        }
    }, "设置别名", "alias [名=命令]");

    // unalias
    registerCommand("unalias", [ui](const QStringList& args) {
        if (!args.isEmpty() && ui->aliases.remove(args[0]))
            ui->appendOutput(QString("已删除: %1\n").arg(args[0]), INFO_COLOR);
    }, "删除别名", "unalias <名>");

    // history
    registerCommand("history", [ui](const QStringList&) {
        QStringList hist = ui->getHistory();
        for (int i = 0; i < hist.size(); ++i)
            ui->appendOutput(QString("%1  %2\n").arg(i + 1, 5).arg(hist[i]));
    }, "命令历史", "history");

    // export
    registerCommand("export", [ui](const QStringList& args) {
        if (!args.isEmpty() && args[0].contains('=')) {
            QStringList p = args[0].split('=');
            qputenv(p[0].toUtf8(), p[1].toUtf8());
            ui->appendOutput(QString("导出: %1=%2\n").arg(p[0], p[1]), INFO_COLOR);
        }
    }, "导出环境变量", "export 变量=值");

    // source
    registerCommand("source", [ui](const QStringList& args) {
        if (args.isEmpty()) return;
        QFile file(QDir(ui->currentDir).absoluteFilePath(args[0]));
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            while (!in.atEnd()) {
                QString line = in.readLine().trimmed();
                if (!line.isEmpty() && !line.startsWith('#'))
                    ui->executeInternalCommand(line);
            }
            file.close();
        }
    }, "执行脚本", "source <文件>");

    // exec
    registerCommand("exec", [ui](const QStringList& args) {
        if (!args.isEmpty()) ui->executeInternalCommand(args.join(' '));
    }, "执行命令", "exec <命令>");

    // script
    registerCommand("script", [ui](const QStringList&) {
        ui->appendOutput("会话记录功能\n", INFO_COLOR);
    }, "记录会话", "script");

    // 包管理
    registerCommand("apt", [ui](const QStringList& args) {
        CommandHelper::runExternal("apt", args, ui->currentDir, ui);
    }, "apt", "apt");

    registerCommand("apt-get", [ui](const QStringList& args) {
        CommandHelper::runExternal("apt-get", args, ui->currentDir, ui);
    }, "apt-get", "apt-get");

    registerCommand("yum", [ui](const QStringList& args) {
        CommandHelper::runExternal("yum", args, ui->currentDir, ui);
    }, "yum", "yum");

    registerCommand("dnf", [ui](const QStringList& args) {
        CommandHelper::runExternal("dnf", args, ui->currentDir, ui);
    }, "dnf", "dnf");

    registerCommand("pacman", [ui](const QStringList& args) {
        CommandHelper::runExternal("pacman", args, ui->currentDir, ui);
    }, "pacman", "pacman");

    registerCommand("brew", [ui](const QStringList& args) {
        CommandHelper::runExternal("brew", args, ui->currentDir, ui);
    }, "brew", "brew");

    // setfont
    registerCommand("setfont", [ui](const QStringList& args) {
        if (args.isEmpty()) { ui->appendOutput("用法: setfont <字号 8-32>\n", ERROR_COLOR); return; }
        bool ok; int size = args[0].toInt(&ok);
        if (!ok || size < 8 || size > 32) { ui->appendOutput("字号需在8-32之间\n", ERROR_COLOR); return; }
        QFont f = ui->outputArea->font(); f.setPointSize(size);
        ui->outputArea->setFont(f);
        ui->inputField->setFont(f);
        ui->promptLabel->setFont(f);
        ui->appendOutput(QString("字体: %1\n").arg(size), INFO_COLOR);
    }, "设置字号", "setfont <字号>");

    // addcmd
    registerCommand("addcmd", [ui](const QStringList& args) {
        if (args.size() < 2) { ui->appendOutput("用法: addcmd <名称> <命令> [描述]\n", ERROR_COLOR); return; }
        QJsonObject obj;
        obj["command"] = args[1];
        obj["desc"] = args.size() > 2 ? args.mid(2).join(' ') : "";
        ui->customCommands[args[0]] = obj;
        ui->saveCustomCommands();
        ui->appendOutput(QString("已添加: %1\n").arg(args[0]), INFO_COLOR);
    }, "添加自定义命令", "addcmd <名> <命令> [描述]");

    // delcmd
    registerCommand("delcmd", [ui](const QStringList& args) {
        if (args.isEmpty()) return;
        if (ui->customCommands.remove(args[0])) {
            ui->saveCustomCommands();
            ui->appendOutput(QString("已删除: %1\n").arg(args[0]), INFO_COLOR);
        } else ui->appendOutput("不存在\n", ERROR_COLOR);
    }, "删除自定义命令", "delcmd <名>");

    // listcmd
    registerCommand("listcmd", [ui](const QStringList&) {
        for (auto it = ui->customCommands.begin(); it != ui->customCommands.end(); ++it)
            ui->appendOutput(QString("%1 - %2\n").arg(it.key(), it.value()["desc"].toString()));
    }, "列出自定义命令", "listcmd");

        // mine 扫雷
    static MinesweeperEngine* mineEngine = nullptr;

    registerCommand("mine", [ui](const QStringList& args) {
        if (!mineEngine) mineEngine = new MinesweeperEngine(ui);

        if (args.isEmpty() || args[0] == "new") {
            mineEngine->init(8, 8, 10);
        } else if (args[0] == "easy") {
            mineEngine->init(8, 8, 10);
        } else if (args[0] == "medium") {
            mineEngine->init(12, 12, 25);
        } else if (args[0] == "hard") {
            mineEngine->init(16, 16, 40);
        } else if (args[0].size() >= 2) {
            QString coord = args[0].toUpper();
            QChar rowChar = coord[0];
            QString colStr = coord.mid(1);
            bool ok;
            int col = colStr.toInt(&ok) - 1;
            int row = rowChar.unicode() - 'A';
            if (ok && row >= 0 && row < 16 && col >= 0 && col < 16) {
                mineEngine->reveal(row, col);
            } else {
                ui->appendOutput("坐标格式: A1 到 P16\n", ERROR_COLOR);
            }
        } else {
            mineEngine->render();
        }
    }, "扫雷游戏", "mine [坐标|new|easy|medium|hard]");

    // flag 扫雷插旗
    registerCommand("flag", [ui](const QStringList& args) {
        if (!mineEngine) {
            ui->appendOutput("请先输入 mine 开始游戏\n", ERROR_COLOR);
            return;
        }
        if (args.isEmpty()) {
            ui->appendOutput("用法: flag <坐标>  如 flag B3\n", ERROR_COLOR);
            return;
        }
        QString coord = args[0].toUpper();
        if (coord.size() >= 2) {
            QChar rowChar = coord[0];
            QString colStr = coord.mid(1);
            bool ok;
            int col = colStr.toInt(&ok) - 1;
            int row = rowChar.unicode() - 'A';
            if (ok && row >= 0 && row < 16 && col >= 0 && col < 16) {
                mineEngine->flag(row, col);
            }
        }
    }, "扫雷插旗", "flag <坐标>");
}