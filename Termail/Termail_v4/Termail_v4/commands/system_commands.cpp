#include "commands.h"
#include "ui/terminal_ui.h"
#include "utils/utils.h"

#include <QProcess>
#include <QSysInfo>
#include <QStorageInfo>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QDateTime>
#include <QThread>
#include <QTimer>
#include <QRandomGenerator>
#include <cmath>

// ============ 系统/进程/包管理命令注册 ============
void CommandRegistry::registerSystemCommands(TerminalUI* ui) {

    // exit / quit
    registerCommand("exit", [ui](const QStringList&) {
        ui->close();
    }, "退出终端", "exit");

    registerCommand("quit", [ui](const QStringList&) {
        ui->close();
    }, "退出终端", "quit");

    // clear / cls
    registerCommand("clear", [ui](const QStringList&) {
        ui->clearScreen();
    }, "清屏", "clear");

    registerCommand("cls", [ui](const QStringList&) {
        ui->clearScreen();
    }, "清屏", "cls");

    // echo
    registerCommand("echo", [ui](const QStringList& args) {
        ui->appendOutput(args.join(' ') + "\n");
    }, "输出文本", "echo <文本>");

    // date / time
    registerCommand("date", [ui](const QStringList&) {
        ui->appendOutput(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss\n"));
    }, "显示日期时间", "date");

    registerCommand("time", [ui](const QStringList&) {
        ui->executeInternalCommand("date");
    }, "显示时间", "time");

    // whoami
    registerCommand("whoami", [ui](const QStringList&) {
        ui->appendOutput(ui->username + "\n");
    }, "显示当前用户", "whoami");

    // env
    registerCommand("env", [ui](const QStringList&) {
        QStringList envVars = QProcess::systemEnvironment();
        envVars.sort();
        for (const QString& var : envVars) {
            ui->appendOutput(var + "\n");
        }
    }, "显示环境变量", "env");

    // sysinfo
    registerCommand("sysinfo", [ui](const QStringList&) {
        QString info = QString(
            "系统: %1 %2\n"
            "架构: %3\n"
            "产品类型: %4\n"
            "主机: %5\n"
            "用户: %6 (%7)\n"
            "Qt版本: %8\n"
            "目录: %9\n"
        ).arg(QSysInfo::prettyProductName(),
              QSysInfo::kernelVersion(),
              QSysInfo::currentCpuArchitecture(),
              QSysInfo::productType(),
              ui->hostname,
              ui->username,
              ui->isAdminMode ? "管理员" : "普通用户",
              qVersion(),
              ui->currentDir);
        ui->appendOutput(info, THEME.cyan.name());
    }, "系统信息", "sysinfo");

    // ps / tasklist
    registerCommand("ps", [ui](const QStringList& args) {
#ifdef Q_OS_WIN
        CommandHelper::runExternal("tasklist", args, ui->currentDir, ui);
#else
        CommandHelper::runExternal("ps", QStringList() << "aux" << args, ui->currentDir, ui);
#endif
    }, "进程列表", "ps");

    registerCommand("tasklist", [ui](const QStringList& args) {
#ifdef Q_OS_WIN
        CommandHelper::runExternal("tasklist", args, ui->currentDir, ui);
#else
        CommandHelper::runExternal("ps", QStringList() << "aux", ui->currentDir, ui);
#endif
    }, "任务列表", "tasklist");

    // top
    registerCommand("top", [ui](const QStringList&) {
        ui->appendOutput("top 需要交互式终端\n", THEME.yellow.name());
    }, "系统监控", "top");

    // taskkill / kill
    registerCommand("taskkill", [ui](const QStringList& args) {
#ifdef Q_OS_WIN
        CommandHelper::runExternal("taskkill", args, ui->currentDir, ui);
#else
        CommandHelper::runExternal("kill", args, ui->currentDir, ui);
#endif
    }, "结束任务", "taskkill <PID>");

    registerCommand("kill", [ui](const QStringList& args) {
#ifdef Q_OS_WIN
        CommandHelper::runExternal("taskkill", QStringList() << "/PID" << args, ui->currentDir, ui);
#else
        CommandHelper::runExternal("kill", args, ui->currentDir, ui);
#endif
    }, "结束进程", "kill <PID>");

    // jobs
    registerCommand("jobs", [ui](const QStringList&) {
        ui->appendOutput("当前没有后台任务\n");
    }, "后台任务", "jobs");

    // uname
    registerCommand("uname", [ui](const QStringList&) {
        ui->appendOutput(QString("%1 %2 %3 %4\n")
            .arg(QSysInfo::kernelType())
            .arg(QSysInfo::machineHostName())
            .arg(QSysInfo::kernelVersion())
            .arg(QSysInfo::currentCpuArchitecture()));
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
        if (storage.isValid()) {
            qint64 total = storage.bytesTotal();
            qint64 avail = storage.bytesAvailable();
            qint64 used = total - avail;
            ui->appendOutput(QString("总计: %1G  已用: %2G  空闲: %3G\n")
                .arg(total / 1073741824.0, 0, 'f', 1)
                .arg(used / 1073741824.0, 0, 'f', 1)
                .arg(avail / 1073741824.0, 0, 'f', 1));
        }
    }, "内存/磁盘使用", "free");

    // lscpu
    registerCommand("lscpu", [ui](const QStringList&) {
        ui->appendOutput(QString("架构: %1\nCPU核心: %2\n")
            .arg(QSysInfo::currentCpuArchitecture())
            .arg(QThread::idealThreadCount()));
    }, "CPU信息", "lscpu");

    // lspci
    registerCommand("lspci", [ui](const QStringList& args) {
        CommandHelper::runExternal("lspci", args, ui->currentDir, ui);
    }, "PCI设备", "lspci");

    // lsusb
    registerCommand("lsusb", [ui](const QStringList& args) {
        CommandHelper::runExternal("lsusb", args, ui->currentDir, ui);
    }, "USB设备", "lsusb");

    // dmesg
    registerCommand("dmesg", [ui](const QStringList& args) {
        CommandHelper::runExternal("dmesg", args, ui->currentDir, ui);
    }, "内核日志", "dmesg");

    // id
    registerCommand("id", [ui](const QStringList&) {
        ui->appendOutput(QString("uid=1000(%1) gid=1000(%1)\n").arg(ui->username));
    }, "用户ID", "id");

    // groups
    registerCommand("groups", [ui](const QStringList&) {
        ui->appendOutput(QString("%1 : %1\n").arg(ui->username));
    }, "用户组", "groups");

    // users
    registerCommand("users", [ui](const QStringList&) {
        ui->appendOutput(ui->username + "\n");
    }, "在线用户", "users");

    // w / who
    registerCommand("w", [ui](const QStringList&) {
        ui->appendOutput(QString("%1   pts/0    %2    terminal\n")
            .arg(ui->username)
            .arg(QDateTime::currentDateTime().toString("hh:mm")));
    }, "谁在登录", "w");

    registerCommand("who", [ui](const QStringList&) {
        ui->executeInternalCommand("w");
    }, "谁在登录", "who");

    // last
    registerCommand("last", [ui](const QStringList&) {
        ui->appendOutput(QString("%1    pts/0    %2   still logged in\n")
            .arg(ui->username)
            .arg(QDateTime::currentDateTime().toString("ddd MMM dd hh:mm")));
    }, "登录历史", "last");

    // python / python3
    registerCommand("python", [ui](const QStringList& args) {
        if (args.isEmpty()) {
            CommandHelper::runExternal("python3", {}, ui->currentDir, ui);
        } else {
            CommandHelper::runExternal("python3", QStringList() << "-c" << args.join(' '), ui->currentDir, ui);
        }
    }, "Python解释器", "python [代码]");

    registerCommand("python3", [ui](const QStringList& args) {
        ui->executeInternalCommand("python " + args.join(' '));
    }, "Python3解释器", "python3");

    // pip
    registerCommand("pip", [ui](const QStringList& args) {
        CommandHelper::runExternal("pip3", args, ui->currentDir, ui);
    }, "Python包管理", "pip");

    // npm
    registerCommand("npm", [ui](const QStringList& args) {
        CommandHelper::runExternal("npm", args, ui->currentDir, ui);
    }, "Node包管理", "npm");

    // git
    registerCommand("git", [ui](const QStringList& args) {
        CommandHelper::runExternal("git", args, ui->currentDir, ui);
    }, "版本控制", "git");

    // vim / vi
    registerCommand("vim", [ui](const QStringList&) {
        ui->appendOutput("简易Vim (输入:q退出)\n", THEME.yellow.name());
    }, "文本编辑器", "vim");

    registerCommand("vi", [ui](const QStringList& args) {
        ui->executeInternalCommand("vim " + args.join(' '));
    }, "文本编辑器", "vi");

    // nano
    registerCommand("nano", [ui](const QStringList& args) {
        CommandHelper::runExternal("nano", args, ui->currentDir, ui);
    }, "文本编辑器", "nano");

    // make
    registerCommand("make", [ui](const QStringList& args) {
        CommandHelper::runExternal("make", args, ui->currentDir, ui);
    }, "构建工具", "make");

    // gcc / g++
    registerCommand("gcc", [ui](const QStringList& args) {
        CommandHelper::runExternal("gcc", args, ui->currentDir, ui);
    }, "C编译器", "gcc");

    registerCommand("g++", [ui](const QStringList& args) {
        CommandHelper::runExternal("g++", args, ui->currentDir, ui);
    }, "C++编译器", "g++");

    // java / javac
    registerCommand("java", [ui](const QStringList& args) {
        CommandHelper::runExternal("java", args, ui->currentDir, ui);
    }, "Java运行时", "java");

    registerCommand("javac", [ui](const QStringList& args) {
        CommandHelper::runExternal("javac", args, ui->currentDir, ui);
    }, "Java编译器", "javac");

    // mysql / psql / sqlite / redis
    registerCommand("mysql", [ui](const QStringList& args) {
        CommandHelper::runExternal("mysql", args, ui->currentDir, ui);
    }, "MySQL客户端", "mysql");

    registerCommand("psql", [ui](const QStringList& args) {
        CommandHelper::runExternal("psql", args, ui->currentDir, ui);
    }, "PostgreSQL客户端", "psql");

    registerCommand("sqlite", [ui](const QStringList& args) {
        CommandHelper::runExternal("sqlite3", args, ui->currentDir, ui);
    }, "SQLite客户端", "sqlite");

    registerCommand("redis", [ui](const QStringList& args) {
        CommandHelper::runExternal("redis-cli", args, ui->currentDir, ui);
    }, "Redis客户端", "redis");

    // alias
    registerCommand("alias", [ui](const QStringList& args) {
        if (args.isEmpty()) {
            for (auto it = ui->aliases.begin(); it != ui->aliases.end(); ++it) {
                ui->appendOutput(QString("alias %1='%2'\n").arg(it.key(), it.value()));
            }
            return;
        }
        for (const QString& arg : args) {
            if (arg.contains('=')) {
                QStringList parts = arg.split('=');
                QString name = parts[0].trimmed();
                QString cmd = parts[1].trimmed().remove('\'').remove('"');
                ui->aliases[name] = cmd;
                ui->appendOutput(QString("别名: %1 -> %2\n").arg(name, cmd), THEME.green.name());
            }
        }
    }, "设置别名", "alias [名称=命令]");

    // unalias
    registerCommand("unalias", [ui](const QStringList& args) {
        if (!args.isEmpty() && ui->aliases.contains(args[0])) {
            ui->aliases.remove(args[0]);
            ui->appendOutput(QString("已删除别名: %1\n").arg(args[0]), THEME.green.name());
        }
    }, "删除别名", "unalias <名称>");

    // history
    registerCommand("history", [ui](const QStringList&) {
        QList<QString> hist = ui->getHistory();
        int start = qMax(0, hist.size() - 20);
        for (int i = start; i < hist.size(); ++i) {
            ui->appendOutput(QString("%1  %2\n").arg(i + 1, 5).arg(hist[i]));
        }
    }, "命令历史", "history");

    // 包管理命令
    registerCommand("apt", [ui](const QStringList& args) {
        CommandHelper::runExternal("apt", args, ui->currentDir, ui);
    }, "APT包管理", "apt");

    registerCommand("apt-get", [ui](const QStringList& args) {
        CommandHelper::runExternal("apt-get", args, ui->currentDir, ui);
    }, "APT包管理", "apt-get");

    registerCommand("yum", [ui](const QStringList& args) {
        CommandHelper::runExternal("yum", args, ui->currentDir, ui);
    }, "YUM包管理", "yum");

    registerCommand("dnf", [ui](const QStringList& args) {
        CommandHelper::runExternal("dnf", args, ui->currentDir, ui);
    }, "DNF包管理", "dnf");

    registerCommand("pacman", [ui](const QStringList& args) {
        CommandHelper::runExternal("pacman", args, ui->currentDir, ui);
    }, "Pacman包管理", "pacman");

    registerCommand("brew", [ui](const QStringList& args) {
        CommandHelper::runExternal("brew", args, ui->currentDir, ui);
    }, "Homebrew包管理", "brew");

    // reset
    registerCommand("reset", [ui](const QStringList&) {
        ui->clearScreen();
    }, "重置终端", "reset");

    // set
    registerCommand("set", [ui](const QStringList& args) {
        if (!args.isEmpty() && args[0].contains('=')) {
            QStringList parts = args[0].split('=');
            ui->appendOutput(QString("设置: %1=%2\n").arg(parts[0], parts[1]), THEME.green.name());
        }
    }, "设置变量", "set");

    // export
    registerCommand("export", [ui](const QStringList& args) {
        if (!args.isEmpty() && args[0].contains('=')) {
            QStringList parts = args[0].split('=');
            qputenv(parts[0].toUtf8(), parts[1].toUtf8());
            ui->appendOutput(QString("导出: %1=%2\n").arg(parts[0], parts[1]), THEME.green.name());
        }
    }, "导出环境变量", "export <变量=值>");

    // source
    registerCommand("source", [ui](const QStringList& args) {
        if (args.isEmpty()) return;
        QString path = QDir(ui->currentDir).absoluteFilePath(args[0]);
        QFile file(path);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            while (!in.atEnd()) {
                QString line = in.readLine().trimmed();
                if (!line.isEmpty() && !line.startsWith('#')) {
                    ui->executeInternalCommand(line);
                }
            }
            file.close();
        }
    }, "执行脚本", "source <文件>");

    // exec
    registerCommand("exec", [ui](const QStringList& args) {
        if (!args.isEmpty()) {
            ui->executeInternalCommand(args.join(' '));
        }
    }, "执行命令", "exec <命令>");

    // script
    registerCommand("script", [ui](const QStringList&) {
        ui->appendOutput("script: 会话记录功能\n", THEME.yellow.name());
    }, "记录会话", "script");

    // mount / umount
    registerCommand("mount", [ui](const QStringList& args) {
        CommandHelper::runExternal("mount", args, ui->currentDir, ui);
    }, "挂载", "mount");

    registerCommand("umount", [ui](const QStringList& args) {
        CommandHelper::runExternal("umount", args, ui->currentDir, ui);
    }, "卸载", "umount");

    // fdisk / mkfs / fsck / dd / sync
    registerCommand("fdisk", [ui](const QStringList& args) {
        CommandHelper::runExternal("fdisk", args, ui->currentDir, ui);
    }, "磁盘分区", "fdisk");

    registerCommand("mkfs", [ui](const QStringList& args) {
        CommandHelper::runExternal("mkfs", args, ui->currentDir, ui);
    }, "创建文件系统", "mkfs");

    registerCommand("fsck", [ui](const QStringList& args) {
        CommandHelper::runExternal("fsck", args, ui->currentDir, ui);
    }, "文件系统检查", "fsck");

    registerCommand("dd", [ui](const QStringList& args) {
        CommandHelper::runExternal("dd", args, ui->currentDir, ui);
    }, "磁盘拷贝", "dd");

    registerCommand("sync", [ui](const QStringList& args) {
#ifndef Q_OS_WIN
        CommandHelper::runExternal("sync", args, ui->currentDir, ui);
#endif
    }, "同步磁盘", "sync");

    // help
    registerCommand("help", [this, ui](const QStringList&) {
        QStringList cmds = getCommandNames();
        cmds.sort();
        ui->appendOutput("=== Termail终端 命令列表 ===\n\n", THEME.cyan.name());
        int count = 0;
        for (const QString& cmd : cmds) {
            ui->appendOutput(QString("%1").arg(cmd, -15));
            count++;
            if (count % 5 == 0) ui->appendOutput("\n");
        }
        ui->appendOutput(QString("\n\n总计: %1 个命令\n").arg(cmds.size()), THEME.green.name());
        ui->appendOutput("输入命令名查看详细帮助\n");
    }, "显示帮助", "help");
}