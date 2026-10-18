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
    registerCommand("help", [this, ui](const QStringList&) {
        QStringList cmds = getCommandNames();
        cmds.sort();
        ui->appendOutput("=== Termail命令列表 ===\n\n", ACCENT_COLOR);
        int count = 0;
        for (const QString& c : cmds) {
            ui->appendOutput(QString("%1").arg(c, -14));
            if (++count % 5 == 0) ui->appendOutput("\n");
        }
        ui->appendOutput(QString("\n\n共 %1 个命令\n").arg(cmds.size()), INFO_COLOR);
    }, "帮助", "help");

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