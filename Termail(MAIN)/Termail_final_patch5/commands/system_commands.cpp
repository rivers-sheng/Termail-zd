#include <QClipboard>
#include <QApplication>
#ifdef Q_OS_ANDROID
#include <QJniObject>
#endif
#include <QApplication>
#include "commands.h"
#include "terminal/terminal.h"
#include "utils/utils.h"
#include "utils/selfcheck.h"

#include <QProcess>
#include <QSysInfo>
#include <QStorageInfo>
#include <QDateTime>
#include <QThread>
#include <QTextStream>
#include <QDir>
#include <QFile>
#include <QJsonObject>
#include <QTimer>
#include <QNetworkInterface>
#include <QHostAddress>
#include <QAbstractSocket>

// 终极白名单：纯 C++ 自带 + 绝对的底层可用
static QStringList getCoreBuiltins() {
    return QStringList() << 
        // --- 纯 C++ 自带（绝对无需测试） ---
        "help" << "exit" << "quit" << "clear" << "cls" << "reset" << "echo" << "date" << "time" << "whoami" << "sysinfo" << "env" << 
        "ls" << "cd" << "pwd" << "mkdir" << "rm" << "rmdir" << "cat" << "cp" << "mv" << "touch" << "tree" << "find" << "grep" << 
        "wc" << "head" << "tail" << "more" << "catmore" << "less" << "diff" << "sort" << "uniq" << "cut" << "stat" << "file" << "du" << "df" << "chmod" << "which" << "dir" << "copy" << "move" << "rename" << "type" << "del" <<
        "ip-info" << "http-get" << "port-check" << "dns-flush" << "speedtest" << "watch-net" << "process-top" << "kill-all" <<
        "tr" << "rev" << "tac" << "nl" << "paste" << "split" << "join" << "fmt" << "sed" << "hash" << "base64" << "base64-decode" << "url-encode" << "url-decode" << 
        "uuid" << "random" << "timestamp" << "random-str" << "uuid-batch" << "json-format" << "hex-view" << "diff-color" << "md5" <<
        "calc" << "bc" << "expr" << "convert" << "units" << "hex" << "bin" << "oct" << "cowsay" << "fortune" << "banner" << "figlet" << "cmatrix" << 
        "sl" << "lolcat" << "ascii" << "morse" << "prime" << "factor" << "fibonacci" << "pi" << "e" << "phi" << "pass-gen" << "game-score" << "pong" << "tetris" << "snake" << "g2048" <<
        "session" << "snew" << "slist" << "ssw" << "skill" << "session-rename" << "session-info" << "session-export" << 
        "history" << "alias" << "unalias" << "export" << "source" << "exec" << "script" << "title" << "copy" << "paste-clip" << "bell" << "notify" <<
        "open" << "share" << "mount-sd" << "brightness" << "clip" <<
        "meminfo" << "cpu-temp" << "battery" << "disk-usage" << "watch-cpu" << "watch-mem" << "watch-battery" <<
        "edit" << "nano" << "create" << "lua" << "luainfo" << "luademo" << "bb" << "cmenu" << "pre-position" << 
        "serve" << "stop-serve" << "setfont" << "addcmd" << "delcmd" << "listcmd" <<
        "config-info" << "backup" << "restore" << "export-config" << "import-config" << "reset-all" << 
        "license" << "log" << "log-termail" << "exit-log" << "stop" << "root" << "lsroot" <<
        // --- 安卓自带的系统命令（硬编码，测都不用测） ---
        "about" << "who" << "w" << "users" << "last" << "id" << "ip" << "ifconfig" << "netstat" << "route" << "arp" << "ping" << "ps" << "kill" << "killall" << "top" << "free" << "uname" << "uptime" << "hostname" << "mount" << "umount" << "sync" << "reboot" << "poweroff" << "sleep" << "yes" << "toybox" << "tar" << "gzip" << "gunzip" << "xz";
}

void CommandRegistry::registerSystemCommands(TerminalWidget* ui) {

    // help
    registerCommand("help", [this, ui](const QStringList& args) {
        if (args.isEmpty()) {
            QStringList coreCmds;
            QStringList extraCmds;
            QStringList builtins = getAvailableCommands();
            
            if (builtins.isEmpty()) {
                builtins = getSafeBuiltins(); // 👈 这里改了
            }

            QStringList allCmds = getCommandNames();
            allCmds.sort();

            for (const QString& c : allCmds) {
                if (builtins.contains(c)) coreCmds.append(c);
                else extraCmds.append(c);
            }

            ui->appendOutput("=== Termail 核心命令 ===\n\n", ACCENT_COLOR);
            int count = 0;
            for (const QString& c : coreCmds) {
                ui->appendOutput(QString("%1").arg(c, -14));
                if (++count % 5 == 0) ui->appendOutput("\n");
            }
            if (count % 5 != 0) ui->appendOutput("\n");

            ui->appendOutput(QString("\n核心命令共 %1 个\n").arg(coreCmds.size()), INFO_COLOR);
            ui->appendOutput("\n=== 外部组件命令 ===\n\n", ACCENT_COLOR);
            ui->appendOutput(QString("共 %1 个扩展命令，输入 more 查看完整列表\n").arg(extraCmds.size()), PROMPT_COLOR);
            return;
        }

        QString cmd = args[0].toLower();
        if (getCommandNames().contains(cmd)) {
            ui->appendOutput(QString("=== %1 ===\n").arg(cmd), ACCENT_COLOR);
            ui->appendOutput(QString("简介: %1\n").arg(getCommandDesc(cmd)), INFO_COLOR);
            ui->appendOutput(QString("用法: %1\n").arg(getCommandUsage(cmd)), PROMPT_COLOR);
            return;
        }

        ui->appendOutput(QString("未找到命令: %1，为你列出以下相关/核心命令：\n\n").arg(args[0]), ERROR_COLOR);
        QStringList allCmds = getCommandNames();
        allCmds.sort();
        int count = 0;
        for (const QString& c : allCmds) {
            if (c.contains(cmd, Qt::CaseInsensitive)) {
                ui->appendOutput(QString("%1").arg(c, -14), ACCENT_COLOR);
                if (++count % 5 == 0) ui->appendOutput("\n");
            }
        }
        if (count == 0) {
            ui->appendOutput("没有找到任何相关的命令。\n", INFO_COLOR);
            ui->appendOutput("输入 help 查看所有核心命令。\n", INFO_COLOR);
        } else if (count % 5 != 0) {
            ui->appendOutput("\n");
        }
    }, "显示帮助信息", "help [命令名]");

    // more
    registerCommand("more", [this, ui](const QStringList& args) {
        if (args.isEmpty()) {
            QStringList extraCmds;
            QStringList builtins = getAvailableCommands();
            if (builtins.isEmpty()) builtins = getSafeBuiltins(); // 👈 这里改了
            
            QStringList allCmds = getCommandNames();
            allCmds.sort();
            for (const QString& c : allCmds) {
                if (!builtins.contains(c)) extraCmds.append(c);
            }

            ui->appendOutput("=== 外部组件命令列表 ===\n\n", ACCENT_COLOR);
            ui->appendOutput(QString("共 %1 个命令，依赖系统环境运行。\n\n").arg(extraCmds.size()), INFO_COLOR);
            
            int count = 0;
            for (const QString& c : extraCmds) {
                ui->appendOutput(QString("%1").arg(c, -14));
                if (++count % 5 == 0) ui->appendOutput("\n");
            }
            if (count % 5 != 0) ui->appendOutput("\n");
            return;
        }

        QString keyword = args[0].toLower();
        QStringList extraCmds;
        QStringList builtins = getAvailableCommands();
        if (builtins.isEmpty()) builtins = getSafeBuiltins(); // 👈 这里改了
        
        QStringList allCmds = getCommandNames();
        allCmds.sort();
        for (const QString& c : allCmds) {
            if (!builtins.contains(c) && c.contains(keyword, Qt::CaseInsensitive)) {
                extraCmds.append(c);
            }
        }

        ui->appendOutput(QString("=== 包含 '%1' 的外部组件命令 ===\n\n").arg(keyword), ACCENT_COLOR);
        if (extraCmds.isEmpty()) {
            ui->appendOutput("没有匹配的扩展命令。\n", INFO_COLOR);
        } else {
            int count = 0;
            for (const QString& c : extraCmds) {
                ui->appendOutput(QString("%1").arg(c, -14));
                if (++count % 5 == 0) ui->appendOutput("\n");
            }
            if (count % 5 != 0) ui->appendOutput("\n");
        }
    }, "查看扩展命令列表", "more [关键词]");
    
    // license
    registerCommand("license", [ui](const QStringList&) {
        QFile file(":/LICENSE/GPL.txt");
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QStringList lines = QString(file.readAll()).split('\n');
            file.close();
            for (int i = 0; i < qMin(50, lines.size()); ++i)
                ui->appendOutput(lines[i] + "\n");
        } else {
            ui->appendOutput("GPL未找到\n", ERROR_COLOR);
        }
    }, "GPL许可证", "license");

    // exit / quit
    registerCommand("exit", [ui](const QStringList&) {
        ui->saveCustomCommands();
        qApp->quit();
    }, "退出", "exit");
    registerCommand("quit", [ui](const QStringList&) {
        ui->saveCustomCommands();
        qApp->quit();
    }, "退出", "quit");

    // clear
    registerCommand("clear", [ui](const QStringList&) { ui->clearScreen(); }, "清屏", "clear");
    registerCommand("cls", [ui](const QStringList&) { ui->clearScreen(); }, "清屏", "cls");
    registerCommand("reset", [ui](const QStringList&) { ui->clearScreen(); }, "重置", "reset");

    // echo
    registerCommand("echo", [ui](const QStringList& args) {
        if (args.isEmpty()) { ui->appendOutput("用法: echo <文本>\n", ERROR_COLOR); return; }
        ui->appendOutput(args.join(' ') + "\n");
    }, "输出文本", "echo <文本>");

    // date / time
    registerCommand("date", [ui](const QStringList&) {
        ui->appendOutput(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss\n"));
    }, "日期", "date");
    registerCommand("time", [ui](const QStringList&) {
        ui->appendOutput(QDateTime::currentDateTime().toString("hh:mm:ss\n"));
    }, "时间", "time");

    // whoami / env / sysinfo
    registerCommand("whoami", [ui](const QStringList&) {
        ui->appendOutput(ui->username + "\n");
    }, "当前用户", "whoami");
    registerCommand("env", [ui](const QStringList&) {
        QStringList vars = QProcess::systemEnvironment();
        vars.sort();
        for (const QString& v : vars) ui->appendOutput(v + "\n");
    }, "环境变量", "env");
    registerCommand("sysinfo", [ui](const QStringList&) {
        QStorageInfo storage(ui->currentDir);
        ui->appendOutput(QString(
            "系统: %1\n架构: %2\n主机: %3\n用户: %4\nCPU: %5\n磁盘: %6\n")
            .arg(QSysInfo::prettyProductName(), QSysInfo::currentCpuArchitecture(),
                 ui->hostname, ui->username,
                 QString::number(QThread::idealThreadCount()),
                 Utils::formatSize(storage.bytesTotal())), ACCENT_COLOR);
    }, "系统信息", "sysinfo");

    // ps / tasklist / top / kill / taskkill / jobs
    registerCommand("ps", [ui](const QStringList& args) {
        CommandHelper::runExternal("ps", QStringList{"aux"} + args, ui->currentDir, ui);
    }, "进程列表", "ps");
    registerCommand("tasklist", [ui](const QStringList& args) {
        CommandHelper::runExternal("ps", QStringList{"aux"} + args, ui->currentDir, ui);
    }, "任务列表", "tasklist");
    registerCommand("top", [ui](const QStringList&) {
        ui->appendOutput("top需要交互式终端\n", ERROR_COLOR);
    }, "系统监控", "top");
    registerCommand("kill", [ui](const QStringList& args) {
        if (args.isEmpty()) { ui->appendOutput("用法: kill <PID>\n", ERROR_COLOR); return; }
        CommandHelper::runExternal("kill", args, ui->currentDir, ui);
    }, "结束进程", "kill <PID>");
    registerCommand("taskkill", [ui](const QStringList& args) {
        if (args.isEmpty()) { ui->appendOutput("用法: taskkill <PID>\n", ERROR_COLOR); return; }
        CommandHelper::runExternal("kill", args, ui->currentDir, ui);
    }, "结束进程", "taskkill <PID>");
    registerCommand("jobs", [ui](const QStringList&) {
        ui->appendOutput("无后台任务\n");
    }, "后台任务", "jobs");

    // uname / hostname / uptime / free / lscpu
    registerCommand("uname", [ui](const QStringList& args) {
        if (args.contains("-a"))
            ui->appendOutput(QString("%1 %2 %3\n")
                .arg(QSysInfo::kernelType(), QSysInfo::kernelVersion(),
                     QSysInfo::currentCpuArchitecture()));
        else ui->appendOutput(QSysInfo::kernelType() + "\n");
    }, "系统信息", "uname [-a]");
    registerCommand("hostname", [ui](const QStringList& args) {
        if (args.contains("-I") || args.contains("-i")) {
            for (const QHostAddress& addr : QNetworkInterface::allAddresses())
                if (addr.protocol() == QAbstractSocket::IPv4Protocol && !addr.isLoopback())
                    ui->appendOutput(addr.toString() + "\n");
        } else {
            ui->appendOutput(ui->hostname + "\n");
        }
    }, "主机名", "hostname [-I]");
    registerCommand("uptime", [ui](const QStringList&) {
        ui->appendOutput("终端运行中...\n");
    }, "运行时间", "uptime");
    registerCommand("free", [ui](const QStringList&) {
        QStorageInfo storage("/");
        if (storage.isValid())
            ui->appendOutput(QString("总量:%1 可用:%2\n")
                .arg(Utils::formatSize(storage.bytesTotal()), Utils::formatSize(storage.bytesAvailable())));
    }, "内存/磁盘", "free");
    registerCommand("lscpu", [ui](const QStringList&) {
        ui->appendOutput(QString("架构:%1 核心:%2\n")
            .arg(QSysInfo::currentCpuArchitecture()).arg(QThread::idealThreadCount()));
    }, "CPU信息", "lscpu");

    // id / groups / users / w / who / last
    registerCommand("id", [ui](const QStringList&) {
        ui->appendOutput(QString("uid=1000(%1)\n").arg(ui->username));
    }, "用户ID", "id");
    registerCommand("groups", [ui](const QStringList&) {
        ui->appendOutput(ui->username + "\n");
    }, "用户组", "groups");
    registerCommand("users", [ui](const QStringList&) {
        ui->appendOutput(ui->username + "\n");
    }, "在线用户", "users");
    registerCommand("w", [ui](const QStringList&) {
        ui->appendOutput(QString("%1 pts/0 %2\n").arg(ui->username)
            .arg(QDateTime::currentDateTime().toString("hh:mm")));
    }, "谁在登录", "w");
    registerCommand("who", [ui](const QStringList&) {
        ui->appendOutput(QString("%1 pts/0\n").arg(ui->username));
    }, "谁在登录", "who");
    registerCommand("last", [ui](const QStringList&) {
        ui->appendOutput(QString("%1 pts/0 still logged in\n").arg(ui->username));
    }, "登录历史", "last");

    // 外部语言与环境 (加上了无参提示)
    registerCommand("python", [ui](const QStringList& args) {
        if (args.isEmpty()) { ui->appendOutput("用法: python <代码>\n", ERROR_COLOR); return; }
        CommandHelper::runExternal("python3", args, ui->currentDir, ui);
    }, "Python", "python <代码>");
    registerCommand("python3", [ui](const QStringList& args) {
        if (args.isEmpty()) { ui->appendOutput("用法: python3 <代码>\n", ERROR_COLOR); return; }
        ui->executeInternalCommand("python " + args.join(' '));
    }, "Python3", "python3 <代码>");
    registerCommand("pip", [ui](const QStringList& args) {
        if (args.isEmpty()) { ui->appendOutput("用法: pip <参数>\n", ERROR_COLOR); return; }
        CommandHelper::runExternal("pip3", args, ui->currentDir, ui);
    }, "pip", "pip <参数>");
    registerCommand("npm", [ui](const QStringList& args) {
        if (args.isEmpty()) { ui->appendOutput("用法: npm <参数>\n", ERROR_COLOR); return; }
        CommandHelper::runExternal("npm", args, ui->currentDir, ui);
    }, "npm", "npm <参数>");
    registerCommand("git", [ui](const QStringList& args) {
        if (args.isEmpty()) { ui->appendOutput("用法: git <参数>\n", ERROR_COLOR); return; }
        CommandHelper::runExternal("git", args, ui->currentDir, ui);
    }, "git", "git <参数>");

    registerCommand("vim", [ui](const QStringList&) {
        ui->appendOutput("简易Vim不可用，请用cat查看文件\n", ERROR_COLOR);
    }, "vim", "vim");
    registerCommand("vi", [ui](const QStringList& args) {
        if (args.isEmpty()) { ui->appendOutput("用法: vi <文件>\n", ERROR_COLOR); return; }
        ui->executeInternalCommand("vim " + args.join(' '));
    }, "vi", "vi <文件>");

    // make / gcc / g++
    registerCommand("make", [ui](const QStringList& args) {
        if (args.isEmpty()) { ui->appendOutput("用法: make <参数>\n", ERROR_COLOR); return; }
        CommandHelper::runExternal("make", args, ui->currentDir, ui);
    }, "make", "make <参数>");
    registerCommand("gcc", [ui](const QStringList& args) {
        if (args.isEmpty()) { ui->appendOutput("Android无GCC\n", ERROR_COLOR); return; }
        CommandHelper::runExternal("gcc", args, ui->currentDir, ui);
    }, "gcc", "gcc <参数>");
    registerCommand("g++", [ui](const QStringList& args) {
        if (args.isEmpty()) { ui->appendOutput("Android无G++\n", ERROR_COLOR); return; }
        CommandHelper::runExternal("g++", args, ui->currentDir, ui);
    }, "g++", "g++ <参数>");

    registerCommand("java", [ui](const QStringList& args) {
        if (args.isEmpty()) { ui->appendOutput("Android无Java\n", ERROR_COLOR); return; }
        CommandHelper::runExternal("java", args, ui->currentDir, ui);
    }, "java", "java <参数>");
    registerCommand("javac", [ui](const QStringList& args) {
        if (args.isEmpty()) { ui->appendOutput("用法: javac <文件>\n", ERROR_COLOR); return; }
        ui->executeInternalCommand("java " + args.join(' '));
    }, "javac", "javac <文件>");

    // 数据库
    registerCommand("mysql", [ui](const QStringList& args) {
        if (args.isEmpty()) { ui->appendOutput("用法: mysql <参数>\n", ERROR_COLOR); return; }
        CommandHelper::runExternal("mysql", args, ui->currentDir, ui);
    }, "mysql", "mysql <参数>");
    registerCommand("psql", [ui](const QStringList& args) {
        if (args.isEmpty()) { ui->appendOutput("用法: psql <参数>\n", ERROR_COLOR); return; }
        CommandHelper::runExternal("psql", args, ui->currentDir, ui);
    }, "psql", "psql <参数>");
    registerCommand("sqlite", [ui](const QStringList& args) {
        if (args.isEmpty()) { ui->appendOutput("用法: sqlite <参数>\n", ERROR_COLOR); return; }
        CommandHelper::runExternal("sqlite3", args, ui->currentDir, ui);
    }, "sqlite", "sqlite <参数>");
    registerCommand("redis", [ui](const QStringList& args) {
        if (args.isEmpty()) { ui->appendOutput("用法: redis <参数>\n", ERROR_COLOR); return; }
        CommandHelper::runExternal("redis-cli", args, ui->currentDir, ui);
    }, "redis", "redis <参数>");

    // alias / unalias
    registerCommand("alias", [ui](const QStringList& args) {
        if (args.isEmpty()) {
            for (auto it = ui->aliases.begin(); it != ui->aliases.end(); ++it)
                ui->appendOutput(QString("alias %1='%2'\n").arg(it.key(), it.value()));
            return;
        }
        QString fullLine = args.join(' ');
        int eqPos = fullLine.indexOf('=');
        if (eqPos < 0) {
            QString name = fullLine.trimmed();
            if (ui->aliases.contains(name))
                ui->appendOutput(QString("%1='%2'\n").arg(name, ui->aliases[name]));
            else
                ui->appendOutput(QString("别名不存在: %1\n").arg(name), ERROR_COLOR);
            return;
        }
        QString name = fullLine.left(eqPos).trimmed();
        QString value = fullLine.mid(eqPos + 1).trimmed();
        if (value.startsWith('\'') && value.endsWith('\''))
            value = value.mid(1, value.length() - 2);
        else if (value.startsWith('"') && value.endsWith('"'))
            value = value.mid(1, value.length() - 2);

        if (name.isEmpty() || value.isEmpty()) {
            ui->appendOutput("用法: alias <名>='<命令>'\n", ERROR_COLOR);
            return;
        }
        ui->aliases[name] = value;
        ui->saveAliases();
        ui->appendOutput(QString("别名: %1 → %2\n").arg(name, value), INFO_COLOR);
    }, "别名", "alias [名='命令']");
    registerCommand("unalias", [ui](const QStringList& args) {
        if (args.isEmpty()) { ui->appendOutput("用法: unalias <别名>\n", ERROR_COLOR); return; }
        if (ui->aliases.remove(args[0]))
            ui->appendOutput(QString("已删除: %1\n").arg(args[0]), INFO_COLOR);
        else
            ui->appendOutput(QString("别名不存在: %1\n").arg(args[0]), ERROR_COLOR);
    }, "删除别名", "unalias <别名>");

    // history
    registerCommand("history", [ui](const QStringList&) {
        QStringList hist = ui->getHistory();
        int start = qMax(0, hist.size() - 50);
        for (int i = start; i < hist.size(); ++i)
            ui->appendOutput(QString("%1  %2\n").arg(i + 1, 5).arg(hist[i]));
    }, "历史记录", "history");

    // export / source / exec / script
    registerCommand("export", [ui](const QStringList& args) {
        if (args.isEmpty() || !args[0].contains('=')) {
            ui->appendOutput("用法: export 变量=值\n", ERROR_COLOR);
            return;
        }
        QStringList p = args[0].split('=');
        qputenv(p[0].toUtf8(), p[1].toUtf8());
        ui->appendOutput(QString("导出: %1=%2\n").arg(p[0], p[1]), INFO_COLOR);
    }, "导出变量", "export 变量=值");

    registerCommand("source", [ui](const QStringList& args) {
        if (args.isEmpty()) { ui->appendOutput("用法: source <脚本文件>\n", ERROR_COLOR); return; }
        QFile file(QDir(ui->currentDir).absoluteFilePath(args[0]));
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            while (!in.atEnd()) {
                QString line = in.readLine().trimmed();
                if (!line.isEmpty() && !line.startsWith('#'))
                    ui->executeInternalCommand(line);
            }
            file.close();
        } else {
            ui->appendOutput(QString("无法读取文件: %1\n").arg(args[0]), ERROR_COLOR);
        }
    }, "执行脚本", "source <脚本文件>");

    registerCommand("exec", [ui](const QStringList& args) {
        if (args.isEmpty()) { ui->appendOutput("用法: exec <命令>\n", ERROR_COLOR); return; }
        ui->executeInternalCommand(args.join(' '));
    }, "执行命令", "exec <命令>");

    registerCommand("script", [ui](const QStringList&) {
        ui->appendOutput("会话记录功能\n", INFO_COLOR);
    }, "记录会话", "script");

    // 包管理
    registerCommand("apt", [ui](const QStringList& args) {
        if (args.isEmpty()) { ui->appendOutput("用法: apt <参数>\n", ERROR_COLOR); return; }
        CommandHelper::runExternal("apt", args, ui->currentDir, ui);
    }, "apt", "apt <参数>");
    registerCommand("apt-get", [ui](const QStringList& args) {
        if (args.isEmpty()) { ui->appendOutput("用法: apt-get <参数>\n", ERROR_COLOR); return; }
        CommandHelper::runExternal("apt-get", args, ui->currentDir, ui);
    }, "apt-get", "apt-get <参数>");
    registerCommand("yum", [ui](const QStringList& args) {
        if (args.isEmpty()) { ui->appendOutput("用法: yum <参数>\n", ERROR_COLOR); return; }
        CommandHelper::runExternal("yum", args, ui->currentDir, ui);
    }, "yum", "yum <参数>");
    registerCommand("dnf", [ui](const QStringList& args) {
        if (args.isEmpty()) { ui->appendOutput("用法: dnf <参数>\n", ERROR_COLOR); return; }
        CommandHelper::runExternal("dnf", args, ui->currentDir, ui);
    }, "dnf", "dnf <参数>");
    registerCommand("pacman", [ui](const QStringList& args) {
        if (args.isEmpty()) { ui->appendOutput("用法: pacman <参数>\n", ERROR_COLOR); return; }
        CommandHelper::runExternal("pacman", args, ui->currentDir, ui);
    }, "pacman", "pacman <参数>");
    registerCommand("brew", [ui](const QStringList& args) {
        if (args.isEmpty()) { ui->appendOutput("用法: brew <参数>\n", ERROR_COLOR); return; }
        CommandHelper::runExternal("brew", args, ui->currentDir, ui);
    }, "brew", "brew <参数>");

    // setfont
    registerCommand("setfont", [ui](const QStringList& args) {
        if (args.isEmpty()) { ui->appendOutput("用法: setfont <8-32>\n", ERROR_COLOR); return; }
        bool ok; int size = args[0].toInt(&ok);
        if (!ok || size < 8 || size > 32) { ui->appendOutput("字号需在 8-32 之间\n", ERROR_COLOR); return; }
        QFont f(FONT_FAMILY, size);
        ui->outputArea->setFont(f);
        ui->inputField->setFont(f);
        ui->promptLabel->setFont(f);
        ui->appendOutput(QString("字体: %1\n").arg(size), INFO_COLOR);
    }, "设置字号", "setfont <8-32>");

    // addcmd / delcmd / listcmd
    registerCommand("addcmd", [ui](const QStringList& args) {
        if (args.size() < 2) { ui->appendOutput("用法: addcmd <名> <命令>\n", ERROR_COLOR); return; }
        QString name = args[0];
        if (name.contains(' ') || name.contains('|') || name.contains('>')) {
            ui->appendOutput("命令名不能包含特殊字符\n", ERROR_COLOR);
            return;
        }
        QString command = args.mid(1).join(' ');
        command.remove('"').remove('\'');
        QJsonObject obj;
        obj["command"] = command;
        obj["desc"] = "";
        ui->customCommands[name] = obj;
        ui->saveCustomCommands();
        ui->appendOutput(QString("已添加: %1\n").arg(name), INFO_COLOR);
    }, "添加自定义命令", "addcmd <名> <命令>");

    registerCommand("delcmd", [ui](const QStringList& args) {
        if (args.isEmpty()) { ui->appendOutput("用法: delcmd <名>\n", ERROR_COLOR); return; }
        if (ui->customCommands.remove(args[0])) {
            ui->saveCustomCommands();
            ui->appendOutput(QString("已删除: %1\n").arg(args[0]), INFO_COLOR);
        } else {
            ui->appendOutput(QString("命令不存在: %1\n").arg(args[0]), ERROR_COLOR);
        }
    }, "删除自定义命令", "delcmd <名>");

    registerCommand("listcmd", [ui](const QStringList&) {
        if (ui->customCommands.isEmpty()) {
            ui->appendOutput("暂无自定义命令\n", INFO_COLOR);
            return;
        }
        for (auto it = ui->customCommands.begin(); it != ui->customCommands.end(); ++it)
            ui->appendOutput(QString("%1 - %2\n").arg(it.key(), it.value()["command"].toString()));
    }, "列出自定义命令", "listcmd");

    // timeout
    registerCommand("timeout", [ui](const QStringList& args) {
        if (args.size() < 2) { ui->appendOutput("用法: timeout <秒数> <命令>\n", ERROR_COLOR); return; }
        double secs = args[0].toDouble();
        QString cmd = args.mid(1).join(' ');
        QProcess* proc = new QProcess();
        proc->setWorkingDirectory(ui->currentDir);
        QTimer* timer = new QTimer();
        timer->setSingleShot(true);
        QObject::connect(timer, &QTimer::timeout, [proc, ui, secs]() {
            proc->kill();
            ui->appendOutput(QString("[超时] %1秒\n").arg(secs), ERROR_COLOR);
            proc->deleteLater();
        });
        QObject::connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                         [proc, timer, ui](int, QProcess::ExitStatus) {
            ui->appendOutput(proc->readAllStandardOutput());
            timer->deleteLater();
            proc->deleteLater();
        });
        proc->start("sh", QStringList() << "-c" << cmd);
        timer->start(static_cast<int>(secs * 1000));
    }, "超时执行", "timeout <秒数> <命令>");

    // root - 检测root
    registerCommand("root", [ui](const QStringList&) {
        QProcess proc;
        proc.start("which", QStringList() << "su");
        proc.waitForFinished(3000);
        QString suPath = proc.readAllStandardOutput().trimmed();
        if (!suPath.isEmpty()) {
            ui->appendOutput(QString("已检测到root: %1\n").arg(suPath), PROMPT_COLOR);
            ui->appendOutput("可访问完整根目录\n", INFO_COLOR);
        } else {
            ui->appendOutput("未检测到root\n", ERROR_COLOR);
            ui->appendOutput("仅能访问: /sdcard, /storage\n", INFO_COLOR);
        }
    }, "检测root", "root");

    // lsroot - 查看根目录
    registerCommand("lsroot", [ui](const QStringList&) {
        QProcess proc;
        proc.start("su", QStringList() << "-c" << "ls /");
        proc.waitForFinished(5000);
        QString output = proc.readAllStandardOutput();
        if (output.isEmpty()) output = proc.readAllStandardError();
        if (output.isEmpty()) {
            ui->appendOutput("无法访问根目录（需root）\n", ERROR_COLOR);
        } else {
            ui->appendOutput(output);
        }
    }, "查看根目录", "lsroot");
    
    // stop - 停止当前进程
    registerCommand("stop", [ui](const QStringList&) {
        if (ui->currentProcess && ui->currentProcess->state() != QProcess::NotRunning) {
            ui->currentProcess->kill();
            ui->currentProcess = nullptr;
            ui->appendOutput("已停止\n", INFO_COLOR);
        } else {
            ui->appendOutput("无运行中的进程\n", ERROR_COLOR);
        }
    }, "停止当前进程", "stop");

    // clip - 剪贴板操作
    registerCommand("clip", [ui](const QStringList& args) {
        if (args.isEmpty() || args[0] == "get") {
            ui->appendOutput("剪贴板: " + QApplication::clipboard()->text() + "\n", PROMPT_COLOR);
        } else if (args[0] == "set" && args.size() > 1) {
            QApplication::clipboard()->setText(args.mid(1).join(' '));
            ui->appendOutput("已复制到剪贴板\n", INFO_COLOR);
        } else if (args[0] == "clear") {
            QApplication::clipboard()->clear();
            ui->appendOutput("剪贴板已清空\n", INFO_COLOR);
        } else {
            ui->appendOutput("用法: clip [get|set <文本>|clear]\n", ERROR_COLOR);
        }
    }, "剪贴板", "clip [get|set|clear]");

    registerCommand("brightness", [ui](const QStringList& args) {
        if (args.isEmpty()) { ui->appendOutput("用法: brightness <0-100>\n", ERROR_COLOR); return; }
        int level = args[0].toInt();
        if (level < 0 || level > 100) { ui->appendOutput("亮度需在 0-100\n", ERROR_COLOR); return; }
#ifdef Q_OS_ANDROID
        QJniObject activity = QJniObject::callStaticObjectMethod(
            "org/qtproject/qt/android/QtNative", "activity",
            "()Landroid/app/Activity;");
        if (activity.isValid()) {
            QJniObject window = activity.callObjectMethod(
                "getWindow", "()Landroid/view/Window;");
            if (window.isValid()) {
                QJniObject attrs = window.callObjectMethod(
                    "getAttributes", "()Landroid/view/WindowManager$LayoutParams;");
                if (attrs.isValid()) {
                    double brightness = level / 100.0;
                    attrs.setField<jdouble>("screenBrightness", brightness);
                    window.callMethod<void>("setAttributes",
                        "(Landroid/view/WindowManager$LayoutParams;)V", attrs.object());
                    ui->appendOutput(QString("亮度已设置为 %1%\n").arg(level), INFO_COLOR);
                }
            }
        }
#else
        ui->appendOutput("仅 Android 支持\n", ERROR_COLOR);
#endif
    }, "屏幕亮度", "brightness <0-100>");
    
    // page - 分页查看
    registerCommand("page", [ui](const QStringList& args) {
        if (args.isEmpty()) { ui->appendOutput("用法: page <文件>\n", ERROR_COLOR); return; }
        QFile file(QDir(ui->currentDir).absoluteFilePath(args[0]));
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            ui->appendOutput("文件不存在\n", ERROR_COLOR);
            return;
        }
        QStringList lines = QString(file.readAll()).split('\n');
        file.close();

        int pageSize = 30;
        int totalPages = (lines.size() + pageSize - 1) / pageSize;
        ui->appendOutput(QString("共 %1 行，%2 页\n").arg(lines.size()).arg(totalPages), INFO_COLOR);

        for (int i = 0; i < qMin(pageSize, lines.size()); ++i)
            ui->appendOutput(lines[i] + "\n");

        if (lines.size() > pageSize)
            ui->appendOutput(QString("\n...还有 %1 行，用 bb cat 查看完整\n").arg(lines.size() - pageSize), INFO_COLOR);
    }, "分页查看", "page <文件>");


}
