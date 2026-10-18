#include <QApplication>
#include "commands.h"
#include "terminal/terminal.h"
#include "utils/utils.h"

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
            ui->appendOutput("输入 help <命令名> 查看用法\n", INFO_COLOR);
            return;
        }
        QString cmd = args[0].toLower();
        QMap<QString, QString> helpMap = {
            {"help", "help [命令名] - 帮助"},
            {"exit", "exit - 退出"},
            {"quit", "quit - 退出"},
            {"clear", "clear - 清屏"},
            {"cls", "cls - 清屏"},
            {"echo", "echo <文本> - 输出"},
            {"date", "date - 日期时间"},
            {"whoami", "whoami - 当前用户"},
            {"sysinfo", "sysinfo - 系统信息"},
            {"ls", "ls [-a] [-l] - 列出目录"},
            {"cd", "cd [目录] - 切换目录"},
            {"pwd", "pwd - 当前目录"},
            {"mkdir", "mkdir <目录> - 创建目录"},
            {"rm", "rm [-r] <文件> - 删除"},
            {"touch", "touch <文件> - 创建空文件"},
            {"cat", "cat <文件> - 查看"},
            {"cp", "cp <源> <目标> - 复制"},
            {"mv", "mv <源> <目标> - 移动"},
            {"find", "find <关键词> - 查找"},
            {"grep", "grep <词> <文件> - 搜索"},
            {"ping", "ping <主机> - Ping"},
            {"curl", "curl <URL> - HTTP请求"},
            {"wget", "wget <URL> - 下载"},
            {"ssh", "ssh <用户@主机> - SSH连接"},
            {"calc", "calc <表达式> - 计算器"},
            {"cowsay", "cowsay [文本] - 牛说"},
            {"log", "log - 系统日志"},
            {"log-termail", "log-termail - 程序日志"},
            {"exit-log", "exit-log - 关闭日志"},
            {"cmenu", "cmenu - busybox菜单"},
            {"get-busybox", "get-busybox - 安装busybox"},
            {"bb", "bb <命令> - busybox调用"},
            {"setfont", "setfont <8-32> - 设置字号"},
            {"addcmd", "addcmd <名> <命令> - 添加自定义命令"},
            {"delcmd", "delcmd <名> - 删除"},
            {"listcmd", "listcmd - 列出"},
            {"confirm", "confirm - 确认执行高危命令"},
        };
        if (helpMap.contains(cmd)) {
            ui->appendOutput(QString("=== %1 ===\n").arg(cmd), ACCENT_COLOR);
            ui->appendOutput(helpMap[cmd] + "\n");
        } else {
            ui->appendOutput(QString("未找到: %1\n").arg(args[0]), ERROR_COLOR);
        }
    }, "帮助", "help");

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

    // confirm
    registerCommand("confirm", [ui](const QStringList&) {
        if (!ui->pendingCommand.isEmpty()) {
            QString cmd = ui->pendingCommand;
            ui->pendingCommand.clear();
            ui->executeInternalCommand(cmd);
        } else {
            ui->appendOutput("无待确认命令\n", ERROR_COLOR);
        }
    }, "确认执行", "confirm");

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
        ui->appendOutput(args.join(' ') + "\n");
    }, "输出", "echo");

    // date / time
    registerCommand("date", [ui](const QStringList&) {
        ui->appendOutput(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss\n"));
    }, "日期", "date");

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
        QStorageInfo storage(ui->currentDir);
        ui->appendOutput(QString(
            "系统: %1\n架构: %2\n主机: %3\n用户: %4\nCPU: %5\n磁盘: %6\n")
            .arg(QSysInfo::prettyProductName(), QSysInfo::currentCpuArchitecture(),
                 ui->hostname, ui->username,
                 QString::number(QThread::idealThreadCount()),
                 Utils::formatSize(storage.bytesTotal())), ACCENT_COLOR);
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
        ui->appendOutput("top需要交互式终端\n", ERROR_COLOR);
    }, "系统监控", "top");

    // kill
    registerCommand("kill", [ui](const QStringList& args) {
        CommandHelper::runExternal("kill", args, ui->currentDir, ui);
    }, "结束进程", "kill");

    registerCommand("taskkill", [ui](const QStringList& args) {
        CommandHelper::runExternal("kill", args, ui->currentDir, ui);
    }, "结束进程", "taskkill");

    // jobs
    registerCommand("jobs", [ui](const QStringList&) {
        ui->appendOutput("无后台任务\n");
    }, "后台任务", "jobs");

    // uname / hostname
    registerCommand("uname", [ui](const QStringList& args) {
        if (args.contains("-a"))
            ui->appendOutput(QString("%1 %2 %3\n")
                .arg(QSysInfo::kernelType(), QSysInfo::kernelVersion(),
                     QSysInfo::currentCpuArchitecture()));
        else ui->appendOutput(QSysInfo::kernelType() + "\n");
    }, "系统信息", "uname");

    registerCommand("hostname", [ui](const QStringList& args) {
        if (args.contains("-I") || args.contains("-i")) {
            for (const QHostAddress& addr : QNetworkInterface::allAddresses())
                if (addr.protocol() == QAbstractSocket::IPv4Protocol && !addr.isLoopback())
                    ui->appendOutput(addr.toString() + "\n");
        } else {
            ui->appendOutput(ui->hostname + "\n");
        }
    }, "主机名", "hostname [-I]");

    // uptime / free / lscpu
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

    // id / groups / users
    registerCommand("id", [ui](const QStringList&) {
        ui->appendOutput(QString("uid=1000(%1)\n").arg(ui->username));
    }, "用户ID", "id");

    registerCommand("groups", [ui](const QStringList&) {
        ui->appendOutput(ui->username + "\n");
    }, "用户组", "groups");

    registerCommand("users", [ui](const QStringList&) {
        ui->appendOutput(ui->username + "\n");
    }, "在线用户", "users");

    // w / who / last
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

    // python / pip / npm / git
    registerCommand("python", [ui](const QStringList& args) {
        if (args.isEmpty()) { ui->appendOutput("用法: python <代码>\n", ERROR_COLOR); return; }
        CommandHelper::runExternal("python3", args, ui->currentDir, ui);
    }, "Python", "python");

    registerCommand("python3", [ui](const QStringList& args) {
        ui->executeInternalCommand("python " + args.join(' '));
    }, "Python3", "python3");

    registerCommand("pip", [ui](const QStringList& args) {
        CommandHelper::runExternal("pip3", args, ui->currentDir, ui);
    }, "pip", "pip");

    registerCommand("npm", [ui](const QStringList& args) {
        CommandHelper::runExternal("npm", args, ui->currentDir, ui);
    }, "npm", "npm");

    registerCommand("git", [ui](const QStringList& args) {
        CommandHelper::runExternal("git", args, ui->currentDir, ui);
    }, "git", "git");

    // vim / vi / nano
    registerCommand("vim", [ui](const QStringList&) {
        ui->appendOutput("简易Vim不可用，请用cat查看文件\n", ERROR_COLOR);
    }, "vim", "vim");

    registerCommand("vi", [ui](const QStringList& args) {
        ui->executeInternalCommand("vim " + args.join(' '));
    }, "vi", "vi");

    registerCommand("nano", [ui](const QStringList& args) {
        CommandHelper::runExternal("nano", args, ui->currentDir, ui);
    }, "nano", "nano");

    // make / gcc / g++
    registerCommand("make", [ui](const QStringList& args) {
        CommandHelper::runExternal("make", args, ui->currentDir, ui);
    }, "make", "make");

    registerCommand("gcc", [ui](const QStringList& args) {
        if (args.isEmpty()) { ui->appendOutput("Android无GCC\n", ERROR_COLOR); return; }
        CommandHelper::runExternal("gcc", args, ui->currentDir, ui);
    }, "gcc", "gcc");

    registerCommand("g++", [ui](const QStringList& args) {
        if (args.isEmpty()) { ui->appendOutput("Android无G++\n", ERROR_COLOR); return; }
        CommandHelper::runExternal("g++", args, ui->currentDir, ui);
    }, "g++", "g++");

    // java / javac
    registerCommand("java", [ui](const QStringList& args) {
        if (args.isEmpty()) { ui->appendOutput("Android无Java\n", ERROR_COLOR); return; }
        CommandHelper::runExternal("java", args, ui->currentDir, ui);
    }, "java", "java");

    registerCommand("javac", [ui](const QStringList& args) {
        ui->executeInternalCommand("java " + args.join(' '));
    }, "javac", "javac");

    // 数据库
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

    // alias / unalias
    registerCommand("alias", [ui](const QStringList& args) {
        if (args.isEmpty()) {
            for (auto it = ui->aliases.begin(); it != ui->aliases.end(); ++it)
                ui->appendOutput(QString("alias %1='%2'\n").arg(it.key(), it.value()));
            return;
        }
        if (!args[0].contains('=')) {
            if (ui->aliases.contains(args[0]))
                ui->appendOutput(QString("%1='%2'\n").arg(args[0], ui->aliases[args[0]]));
            return;
        }
        QStringList p = args[0].split('=');
        ui->aliases[p[0].trimmed()] = p[1].trimmed().remove('\'').remove('"');
        ui->appendOutput(QString("别名: %1 → %2\n").arg(p[0].trimmed(), p[1].trimmed()), INFO_COLOR);
    }, "别名", "alias");

    registerCommand("unalias", [ui](const QStringList& args) {
        if (!args.isEmpty() && ui->aliases.remove(args[0]))
            ui->appendOutput(QString("已删除: %1\n").arg(args[0]), INFO_COLOR);
    }, "删除别名", "unalias");

    // history
    registerCommand("history", [ui](const QStringList&) {
        QStringList hist = ui->getHistory();
        int start = qMax(0, hist.size() - 50);
        for (int i = start; i < hist.size(); ++i)
            ui->appendOutput(QString("%1  %2\n").arg(i + 1, 5).arg(hist[i]));
    }, "历史", "history");

    // export / source / exec
    registerCommand("export", [ui](const QStringList& args) {
        if (!args.isEmpty() && args[0].contains('=')) {
            QStringList p = args[0].split('=');
            qputenv(p[0].toUtf8(), p[1].toUtf8());
            ui->appendOutput(QString("导出: %1=%2\n").arg(p[0], p[1]), INFO_COLOR);
        }
    }, "导出变量", "export");

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
    }, "执行脚本", "source");

    registerCommand("exec", [ui](const QStringList& args) {
        if (!args.isEmpty()) ui->executeInternalCommand(args.join(' '));
    }, "执行命令", "exec");

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
        if (args.isEmpty()) { ui->appendOutput("用法: setfont <8-32>\n", ERROR_COLOR); return; }
        bool ok; int size = args[0].toInt(&ok);
        if (!ok || size < 8 || size > 32) return;
        QFont f(FONT_FAMILY, size);
        ui->outputArea->setFont(f);
        ui->inputField->setFont(f);
        ui->promptLabel->setFont(f);
        ui->appendOutput(QString("字体: %1\n").arg(size), INFO_COLOR);
    }, "设置字号", "setfont");

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
    }, "添加自定义命令", "addcmd");

    registerCommand("delcmd", [ui](const QStringList& args) {
        if (!args.isEmpty() && ui->customCommands.remove(args[0])) {
            ui->saveCustomCommands();
            ui->appendOutput(QString("已删除: %1\n").arg(args[0]), INFO_COLOR);
        }
    }, "删除自定义命令", "delcmd");

    registerCommand("listcmd", [ui](const QStringList&) {
        for (auto it = ui->customCommands.begin(); it != ui->customCommands.end(); ++it)
            ui->appendOutput(QString("%1 - %2\n").arg(it.key(), it.value()["command"].toString()));
    }, "列出自定义命令", "listcmd");

    // timeout
    registerCommand("timeout", [ui](const QStringList& args) {
        if (args.size() < 2) return;
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
    }, "超时执行", "timeout");
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
}