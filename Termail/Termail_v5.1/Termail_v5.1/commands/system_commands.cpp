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

        // 查看具体命令帮助
        QString cmd = args[0].toLower();
        QMap<QString, QString> helpMap = {
            // 基础命令
            {"help", "help [命令名] - 查看命令列表或具体命令帮助"},
            {"exit", "exit - 退出终端"},
            {"quit", "quit - 退出终端"},
            {"clear", "clear - 清屏"},
            {"cls", "cls - 清屏"},
            {"reset", "reset - 重置终端显示"},
            {"echo", "echo <文本> - 输出文本"},
            {"date", "date - 显示日期时间"},
            {"time", "time - 显示当前时间"},
            {"whoami", "whoami - 显示当前用户"},
            {"env", "env - 显示环境变量"},
            {"sysinfo", "sysinfo - 显示系统信息"},
            {"cmenu", "cmenu - 显示菜单"},
            {"license", "license - 查看GPL许可证"},
            {"setfont", "setfont <字号8-32> - 设置终端字号"},

            // 文件操作
            {"ls", "ls [-a] [-l] [目录] - 列出目录内容\n  -a 显示隐藏文件\n  -l 长格式显示"},
            {"dir", "dir - 列出目录(Windows习惯)"},
            {"cd", "cd [目录] - 切换目录\n  cd ~ 回到家目录\n  cd - 回到上一个目录"},
            {"pwd", "pwd - 显示当前目录"},
            {"mkdir", "mkdir <目录> - 创建目录"},
            {"rmdir", "rmdir <目录> - 删除空目录"},
            {"rm", "rm <文件/目录> - 删除文件或目录(递归)"},
            {"del", "del <文件> - 删除文件(Windows习惯)"},
            {"touch", "touch <文件> - 创建空文件"},
            {"cat", "cat <文件> - 查看文件内容"},
            {"type", "type <文件> - 查看文件(Windows习惯)"},
            {"cp", "cp <源> <目标> - 复制文件或目录"},
            {"copy", "copy <源> <目标> - 复制(Windows习惯)"},
            {"mv", "mv <源> <目标> - 移动文件"},
            {"move", "move <源> <目标> - 移动(Windows习惯)"},
            {"rename", "rename <旧名> <新名> - 重命名"},
            {"tree", "tree - 树形显示目录结构"},
            {"find", "find <关键词> - 查找文件"},
            {"grep", "grep <关键词> <文件> - 搜索文件内容"},
            {"wc", "wc <文件> - 统计行数/词数/字符数"},
            {"head", "head [-n N] <文件> - 显示前N行(默认10)"},
            {"tail", "tail [-n N] <文件> - 显示后N行(默认10)"},
            {"more", "more <文件> - 分页查看文件"},
            {"less", "less <文件> - 分页查看文件"},
            {"diff", "diff <文件1> <文件2> - 对比文件差异"},
            {"sort", "sort [-r] <文件> - 排序(-r逆序)"},
            {"uniq", "uniq <文件> - 去重"},
            {"cut", "cut -d<分隔符> -f<字段> <文件> - 剪切字段"},
            {"stat", "stat <文件> - 查看文件状态"},
            {"file", "file <文件> - 检测文件类型"},
            {"du", "du [目录] - 磁盘使用量"},
            {"df", "df - 磁盘空间"},
            {"chmod", "chmod <权限> <文件> - 更改权限(八进制)"},
            {"which", "which <命令> - 查找命令位置"},

            // 网络命令
            {"ping", "ping <主机> - Ping网络主机"},
            {"curl", "curl <URL> - HTTP请求"},
            {"wget", "wget <URL> - 下载文件"},
            {"ifconfig", "ifconfig - 显示网络接口"},
            {"ipconfig", "ipconfig - IP配置"},
            {"netstat", "netstat [-an] - 网络连接状态"},
            {"nslookup", "nslookup <域名> - DNS查询"},
            {"tracert", "tracert <主机> - 路由追踪"},
            {"traceroute", "traceroute <主机> - 路由追踪"},
            {"route", "route - 路由表"},
            {"arp", "arp [-a] - ARP表"},
            {"whois", "whois <域名> - WHOIS查询"},

            // 系统命令
            {"ps", "ps - 进程列表"},
            {"tasklist", "tasklist - 任务列表"},
            {"top", "top - 系统监控"},
            {"kill", "kill <PID> - 结束进程"},
            {"taskkill", "taskkill <PID> - 结束进程"},
            {"jobs", "jobs - 后台任务"},
            {"uname", "uname - 系统内核信息"},
            {"hostname", "hostname - 主机名"},
            {"uptime", "uptime - 运行时间"},
            {"free", "free - 内存/磁盘空间"},
            {"lscpu", "lscpu - CPU信息"},
            {"id", "id - 用户ID"},
            {"groups", "groups - 用户组"},
            {"users", "users - 在线用户"},
            {"w", "w - 谁在登录"},
            {"who", "who - 谁在登录"},
            {"last", "last - 登录历史"},
            {"python", "python [代码] - Python解释器"},
            {"python3", "python3 [代码] - Python3解释器"},
            {"pip", "pip <参数> - Python包管理"},
            {"npm", "npm <参数> - Node包管理"},
            {"git", "git <参数> - 版本控制"},
            {"vim", "vim - 文本编辑器"},
            {"vi", "vi - 文本编辑器"},
            {"nano", "nano <文件> - 文本编辑器"},
            {"make", "make - 构建工具"},
            {"gcc", "gcc <参数> - C编译器"},
            {"g++", "g++ <参数> - C++编译器"},
            {"java", "java <参数> - Java运行时"},
            {"javac", "javac <参数> - Java编译器"},
            {"mysql", "mysql - MySQL客户端"},
            {"psql", "psql - PostgreSQL客户端"},
            {"sqlite", "sqlite - SQLite客户端"},
            {"redis", "redis - Redis客户端"},
            {"alias", "alias [名称=命令] - 设置别名"},
            {"unalias", "unalias <名称> - 删除别名"},
            {"history", "history - 命令历史"},
            {"export", "export 变量=值 - 导出环境变量"},
            {"source", "source <文件> - 执行脚本"},
            {"exec", "exec <命令> - 执行命令"},
            {"script", "script - 记录会话"},
            {"apt", "apt <参数> - APT包管理"},
            {"apt-get", "apt-get <参数> - APT包管理"},
            {"yum", "yum <参数> - YUM包管理"},
            {"dnf", "dnf <参数> - DNF包管理"},
            {"pacman", "pacman <参数> - Pacman包管理"},
            {"brew", "brew <参数> - Homebrew包管理"},

            // 文本处理
            {"tr", "tr <集1> <集2> [文本] - 字符替换"},
            {"rev", "rev <文本> - 反转文本"},
            {"tac", "tac <文件> - 反向显示文件"},
            {"nl", "nl <文件> - 带行号显示"},
            {"paste", "paste <文件1> <文件2> - 合并文件"},
            {"split", "split [-l N] <文件> - 分割文件"},
            {"join", "join <文件...> - 连接文件"},
            {"fmt", "fmt <文件> - 格式化文本"},
            {"sed", "sed 's/旧/新/' <文件> - 流编辑器"},
            {"zip", "zip <压缩包> <文件...> - 压缩"},
            {"unzip", "unzip <压缩包> - 解压"},
            {"tar", "tar <参数> <文件> - 归档"},
            {"hash", "hash <文本> - 计算哈希(MD5/SHA256)"},
            {"base64", "base64 <文本> - Base64编码"},
            {"uuid", "uuid - 生成UUID"},
            {"random", "random [最小] [最大] - 随机数"},
            {"sleep", "sleep <秒> - 延时"},
            {"watch", "watch <命令> - 重复执行3次"},
            {"tee", "tee <文件> <文本> - 输出并保存"},
            {"yes", "yes [文本] - 重复输出10次"},
            {"seq", "seq [起] [步] <止> - 序列生成"},
            {"shuf", "shuf <项目...> - 随机排列"},
            {"timeout", "timeout <秒> <命令> - 超时执行"},
            {"crontab", "crontab - 定时任务"},
            {"at", "at - 定时执行"},
            {"batch", "batch - 批处理"},

            // 趣味命令
            {"calc", "calc <表达式> - 计算器\n  如: calc 3+5*2"},
            {"bc", "bc <表达式> - 计算器"},
            {"expr", "expr <表达式> - 计算表达式"},
            {"convert", "convert <值> <单位1> to <单位2> - 单位转换"},
            {"units", "units - 单位转换"},
            {"hex", "hex <数字> - 十进制转十六进制"},
            {"bin", "bin <数字> - 十进制转二进制"},
            {"oct", "oct <数字> - 十进制转八进制"},
            {"cowsay", "cowsay [文本] - 牛说"},
            {"fortune", "fortune - 随机格言"},
            {"banner", "banner <文本> - 横幅文字"},
            {"figlet", "figlet <文本> - 大字"},
            {"cmatrix", "cmatrix - 矩阵效果"},
            {"sl", "sl - 蒸汽火车"},
            {"lolcat", "lolcat [文本] - 彩虹输出"},
            {"ascii", "ascii [名称] - ASCII艺术\n  可用: cat, heart, dog"},
            {"morse", "morse [文本] - 摩斯电码"},
            {"prime", "prime [上限] - 素数"},
            {"factor", "factor <数字> - 因数分解"},
            {"fibonacci", "fibonacci [项数] - 斐波那契数列"},
            {"pi", "pi [小数位] - 圆周率"},
            {"e", "e [小数位] - 自然常数"},
            {"phi", "phi [小数位] - 黄金比例"},

            // 扫雷
            {"mine", "mine [坐标|new|easy|medium|hard] - 扫雷游戏\n  坐标格式: A1 (行字母+列数字)\n  mine new 新游戏\n  mine easy/medium/hard 换难度"},
            {"flag", "flag <坐标> - 扫雷插旗\n  如: flag B3"},
            {"game", "game - 扫雷入口"},
            {"play", "play - 扫雷入口"},
            {"xyzzy", "xyzzy - 扫雷入口"},

            // 自定义命令
            {"addcmd", "addcmd <名称> <命令> [描述] - 添加自定义命令\n  如: addcmd myip curl ifconfig.me"},
            {"delcmd", "delcmd <名称> - 删除自定义命令"},
            {"listcmd", "listcmd - 列出自定义命令"},

            // busybox
            {"get-busybox", "get-busybox - 下载busybox扩展命令集"},
            {"bb", "bb <busybox命令> - 调用busybox\n  如: bb ls -la"}
        };

        if (helpMap.contains(cmd)) {
            ui->appendOutput(QString("=== %1 ===\n").arg(cmd), ACCENT_COLOR);
            ui->appendOutput(helpMap[cmd] + "\n");
        } else {
            ui->appendOutput(QString("未找到命令: %1\n").arg(args[0]), ERROR_COLOR);
            ui->appendOutput("输入 help 查看全部命令列表\n", INFO_COLOR);
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