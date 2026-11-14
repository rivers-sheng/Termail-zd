#include "commands.h"
#include "terminal/terminal.h"
#include "utils/utils.h"

#include <QFile>
#include <QDir>
#include <QProcess>
#include <QFileInfo>
#include <QStandardPaths>
#include <QDesktopServices>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QEventLoop>
#include <QTimer>
#include <QDateTime>
#include <QSysInfo>
#include <QStorageInfo>
#include <QThread>
#include <QRegularExpression>
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QApplication>
#include <QClipboard>
#include <QFontDatabase>
#include <QSettings>
#include <QTextStream>
#include <QUuid>
#include <QRandomGenerator>
#include <QTcpSocket>
#include <QWidget>

// ==========================================
// 【雷霆检查】辅助函数：检查系统到底有没有这个二进制
// ==========================================
static bool checkSystemBinary(const QString& cmd, TerminalWidget* ui) {
    QString path = QStandardPaths::findExecutable(cmd);
    if (path.isEmpty()) {
        ui->appendOutput(QString("[系统组件缺失] 您的设备未安装 '%1' 或该命令不属于系统核心白名单。\n").arg(cmd), ERROR_COLOR);
        ui->appendOutput("提示: 依赖外部环境的命令需自行安装第三方工具包。\n", INFO_COLOR);
        return false;
    }
    return true;
}

void CommandRegistry::registerManyExtraCommands(TerminalWidget* ui) {

    // ==========================================
    // 一、原 many4.cpp 中纯 C++ 手写的真·内置命令（必定能跑）
    // ==========================================
    
    registerCommand("open", [ui](const QStringList& args) {
        if (args.isEmpty()) { ui->appendOutput("用法: open <文件>\n", ERROR_COLOR); return; }
        QString path = QDir(ui->currentDir).absoluteFilePath(args[0]);
        if (!QFile::exists(path)) { ui->appendOutput("文件不存在\n", ERROR_COLOR); return; }
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
        ui->appendOutput(QString("已打开: %1\n").arg(args[0]), INFO_COLOR);
    }, "系统打开文件", "open <文件>");

    registerCommand("share", [ui](const QStringList& args) {
        if (args.isEmpty()) { ui->appendOutput("用法: share <文件>\n", ERROR_COLOR); return; }
        QString path = QDir(ui->currentDir).absoluteFilePath(args[0]);
        if (!QFile::exists(path)) { ui->appendOutput("文件不存在\n", ERROR_COLOR); return; }
        ui->appendOutput(QString("分享: %1\n").arg(args[0]), INFO_COLOR);
        ui->appendOutput("提示: 长按输出区选中路径，用系统分享\n", INFO_COLOR);
    }, "分享文件", "share <文件>");

    registerCommand("mount-sd", [ui](const QStringList&) {
        QStringList paths = {"/sdcard", "/storage/emulated/0", "/storage/emulated/0/Download"};
        ui->appendOutput("可用存储路径:\n", ACCENT_COLOR);
        for (const QString& p : paths) { if (QDir(p).exists()) ui->appendOutput("  " + p + "\n", PROMPT_COLOR); }
        ui->appendOutput("用 cd /sdcard 进入\n", INFO_COLOR);
    }, "查看存储路径", "mount-sd");

    registerCommand("ip-info", [ui](const QStringList&) {
        ui->appendOutput("查询公网IP...\n", ACCENT_COLOR);
        QNetworkAccessManager* mgr = new QNetworkAccessManager();
        QNetworkReply* reply = mgr->get(QNetworkRequest{QUrl("http://ip.3322.net")});
        QEventLoop loop;
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        QTimer::singleShot(8000, &loop, &QEventLoop::quit);
        loop.exec();
        if (reply->error() == QNetworkReply::NoError) ui->appendOutput("公网IP: " + QString(reply->readAll()).trimmed() + "\n", PROMPT_COLOR);
        else ui->appendOutput("查询失败\n", ERROR_COLOR);
        reply->deleteLater(); mgr->deleteLater();
    }, "公网IP", "ip-info");

    registerCommand("http-get", [ui](const QStringList& args) {
        if (args.isEmpty()) { ui->appendOutput("用法: http-get <URL>\n", ERROR_COLOR); return; }
        QString url = args[0];
        if (!url.startsWith("http")) { ui->appendOutput("URL 必须以 http 开头\n", ERROR_COLOR); return; }
        QNetworkAccessManager* mgr = new QNetworkAccessManager();
        QNetworkReply* reply = mgr->get(QNetworkRequest{QUrl(url)});
        QEventLoop loop;
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        QTimer::singleShot(10000, &loop, &QEventLoop::quit);
        loop.exec();
        if (reply->error() == QNetworkReply::NoError) ui->appendOutput(reply->readAll().left(5000));
        else ui->appendOutput("请求失败: " + reply->errorString() + "\n", ERROR_COLOR);
        reply->deleteLater(); mgr->deleteLater();
    }, "HTTP GET", "http-get <URL>");

    registerCommand("port-check", [ui](const QStringList& args) {
        if (args.size() < 2) { ui->appendOutput("用法: port-check <主机> <端口>\n", ERROR_COLOR); return; }
        QString host = args[0]; int port = args[1].toInt();
        if (port <= 0 || port > 65535) { ui->appendOutput("端口无效\n", ERROR_COLOR); return; }
        QTcpSocket sock;
        sock.connectToHost(host, port);
        if (sock.waitForConnected(3000)) ui->appendOutput(QString("%1:%2 开放\n").arg(host).arg(port), PROMPT_COLOR);
        else ui->appendOutput(QString("%1:%2 关闭或超时\n").arg(host).arg(port), ERROR_COLOR);
    }, "端口检测", "port-check <主机> <端口>");

    registerCommand("dns-flush", [ui](const QStringList&) {
        ui->appendOutput("Android 不支持清理DNS缓存\n", ERROR_COLOR);
        ui->appendOutput("可重启网络: 开关飞行模式\n", INFO_COLOR);
    }, "清DNS缓存", "dns-flush");

    registerCommand("speedtest", [ui](const QStringList&) {
        ui->appendOutput("测试网速（下载10MB测试文件）...\n", ACCENT_COLOR);
        QNetworkAccessManager* mgr = new QNetworkAccessManager();
        QNetworkReply* reply = mgr->get(QNetworkRequest{QUrl("http://speedtest.tele2.net/10MB.zip")});
        QDateTime start = QDateTime::currentDateTime();
        QEventLoop loop;
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        QTimer::singleShot(30000, &loop, &QEventLoop::quit);
        loop.exec();
        qint64 elapsed = start.msecsTo(QDateTime::currentDateTime());
        if (reply->error() == QNetworkReply::NoError) {
            qint64 size = reply->bytesAvailable();
            double speed = (size / 1024.0 / 1024.0) / (elapsed / 1000.0);
            ui->appendOutput(QString("下载: %1MB, 耗时: %2秒\n").arg(size / 1024.0 / 1024.0, 0, 'f', 2).arg(elapsed / 1000.0, 0, 'f', 1));
            ui->appendOutput(QString("速度: %1 MB/s\n").arg(speed, 0, 'f', 2), PROMPT_COLOR);
        } else ui->appendOutput("测试失败（可能网络不通或该测试源不可用）\n", ERROR_COLOR);
        reply->deleteLater(); mgr->deleteLater();
    }, "网速测试", "speedtest");

    registerCommand("meminfo", [ui](const QStringList&) {
        QFile f("/proc/meminfo");
        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) { ui->appendOutput(f.readAll()); f.close(); }
        else ui->appendOutput("无法读取内存信息\n", ERROR_COLOR);
    }, "内存详情", "meminfo");

    registerCommand("cpu-temp", [ui](const QStringList&) {
        QStringList paths = {"/sys/class/thermal/thermal_zone0/temp", "/sys/class/thermal/thermal_zone1/temp"};
        bool found = false;
        for (const QString& p : paths) {
            QFile f(p);
            if (f.open(QIODevice::ReadOnly)) {
                int temp = QString(f.readAll()).trimmed().toInt(); f.close();
                if (temp > 0) { ui->appendOutput(QString("CPU温度: %1°C\n").arg(temp / 1000.0, 0, 'f', 1), PROMPT_COLOR); found = true; }
            }
        }
        if (!found) ui->appendOutput("无法读取CPU温度（需要root）\n", ERROR_COLOR);
    }, "CPU温度", "cpu-temp");

    registerCommand("battery", [ui](const QStringList&) {
        QStringList paths = {"/sys/class/power_supply/battery/capacity", "/sys/class/power_supply/BAT0/capacity"};
        bool found = false;
        for (const QString& p : paths) {
            QFile f(p);
            if (f.open(QIODevice::ReadOnly)) {
                QString cap = QString(f.readAll()).trimmed(); f.close();
                if (!cap.isEmpty()) { ui->appendOutput(QString("电量: %1%\n").arg(cap), PROMPT_COLOR); found = true; }
            }
        }
        QFile statusFile("/sys/class/power_supply/battery/status");
        if (statusFile.open(QIODevice::ReadOnly)) { ui->appendOutput("状态: " + QString(statusFile.readAll()).trimmed() + "\n", INFO_COLOR); statusFile.close(); }
        if (!found) ui->appendOutput("无法读取电池信息\n", ERROR_COLOR);
    }, "电池信息", "battery");

    registerCommand("disk-usage", [ui](const QStringList&) {
        QStorageInfo storage(ui->currentDir);
        if (storage.isValid()) {
            qint64 total = storage.bytesTotal(); qint64 free = storage.bytesAvailable(); qint64 used = total - free;
            double percent = total > 0 ? (used * 100.0 / total) : 0;
            ui->appendOutput(QString("总容量: %1\n").arg(Utils::formatSize(total)));
            ui->appendOutput(QString("已使用: %1 (%2%)\n").arg(Utils::formatSize(used)).arg(percent, 0, 'f', 1));
            ui->appendOutput(QString("可用: %1\n").arg(Utils::formatSize(free)), PROMPT_COLOR);
        } else ui->appendOutput("无法读取磁盘信息\n", ERROR_COLOR);
    }, "磁盘占用", "disk-usage");

    registerCommand("md5", [ui](const QStringList& args) {
        if (args.isEmpty()) { ui->appendOutput("用法: md5 <文件>\n", ERROR_COLOR); return; }
        QFile f(QDir(ui->currentDir).absoluteFilePath(args[0]));
        if (!f.open(QIODevice::ReadOnly)) { ui->appendOutput("文件不存在\n", ERROR_COLOR); return; }
        QCryptographicHash hash(QCryptographicHash::Md5);
        if (hash.addData(&f)) ui->appendOutput(QString("MD5: %1\n").arg(QString(hash.result().toHex())));
        f.close();
    }, "文件MD5", "md5 <文件>");

    registerCommand("json-format", [ui](const QStringList& args) {
        if (args.isEmpty()) { ui->appendOutput("用法: json-format <文件>\n", ERROR_COLOR); return; }
        QFile f(QDir(ui->currentDir).absoluteFilePath(args[0]));
        if (!f.open(QIODevice::ReadOnly)) { ui->appendOutput("文件不存在\n", ERROR_COLOR); return; }
        QJsonParseError err; QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err); f.close();
        if (err.error != QJsonParseError::NoError) { ui->appendOutput("JSON 格式错误: " + err.errorString() + "\n", ERROR_COLOR); return; }
        ui->appendOutput(doc.toJson(QJsonDocument::Indented));
    }, "JSON格式化", "json-format <文件>");

    registerCommand("hex-view", [ui](const QStringList& args) {
        if (args.isEmpty()) { ui->appendOutput("用法: hex-view <文件>\n", ERROR_COLOR); return; }
        QFile f(QDir(ui->currentDir).absoluteFilePath(args[0]));
        if (!f.open(QIODevice::ReadOnly)) { ui->appendOutput("文件不存在\n", ERROR_COLOR); return; }
        QByteArray data = f.read(1024); f.close();
        for (int i = 0; i < data.size(); i += 16) {
            QString line = QString("%1  ").arg(i, 8, 16, QChar('0'));
            for (int j = 0; j < 16; ++j) {
                if (i + j < data.size()) line += QString("%1 ").arg((unsigned char)data[i + j], 2, 16, QChar('0'));
                else line += "   ";
                if (j == 7) line += " ";
            }
            line += " |";
            for (int j = 0; j < 16 && i + j < data.size(); ++j) {
                char c = data[i + j]; line += (c >= 32 && c < 127) ? c : '.';
            }
            line += "|"; ui->appendOutput(line + "\n");
        }
    }, "十六进制查看", "hex-view <文件>");

    registerCommand("pass-gen", [ui](const QStringList& args) {
        int len = args.isEmpty() ? 16 : args[0].toInt(); if (len < 4) len = 4; if (len > 128) len = 128;
        QString chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*"; QString pass;
        for (int i = 0; i < len; ++i) pass += chars[QRandomGenerator::global()->bounded(chars.size())];
        ui->appendOutput(QString("密码(%1位): %2\n").arg(len).arg(pass), PROMPT_COLOR);
    }, "生成密码", "pass-gen [长度]");

    registerCommand("watch-cpu", [ui](const QStringList&) {
        QFile f("/proc/stat"); if (!f.open(QIODevice::ReadOnly)) return; QString line = f.readLine(); f.close();
        QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (parts.size() >= 5) {
            qint64 user = parts[1].toLongLong(), nice = parts[2].toLongLong(), sys = parts[3].toLongLong(), idle = parts[4].toLongLong();
            qint64 total = user + nice + sys + idle;
            ui->appendOutput(QString("CPU使用率: %1%\n").arg(total > 0 ? ((total - idle) * 100.0 / total) : 0, 0, 'f', 1), PROMPT_COLOR);
        }
    }, "CPU使用率", "watch-cpu");

    // ==========================================
    // 二、需要系统外部组件的命令（雷霆检测版）
    // ==========================================

    // 辅助宏，用于快速生成带检测的外部命令（C++ 大法好）
    #define REG_EXT_CMD(cmd_name, desc) \
        registerCommand(cmd_name, [ui](const QStringList& a){ \
            if (checkSystemBinary(cmd_name, ui)) CommandHelper::runExternal(cmd_name, a, ui->currentDir, ui); \
        }, desc, cmd_name);

    // 开发工具
    REG_EXT_CMD("gdb", "调试器"); REG_EXT_CMD("gprof", "性能分析"); REG_EXT_CMD("strace", "系统调用追踪");
    REG_EXT_CMD("ltrace", "库调用追踪"); REG_EXT_CMD("valgrind", "内存检测"); REG_EXT_CMD("cmake", "构建工具");
    REG_EXT_CMD("automake", "自动化构建"); REG_EXT_CMD("autoconf", "配置生成"); REG_EXT_CMD("pkg-config", "包配置");
    REG_EXT_CMD("ldd", "依赖查看"); REG_EXT_CMD("nm", "符号表"); REG_EXT_CMD("objdump", "反汇编");
    REG_EXT_CMD("readelf", "ELF查看"); REG_EXT_CMD("strings", "字符串提取"); REG_EXT_CMD("strip", "符号剥离");
    REG_EXT_CMD("ar", "静态库"); REG_EXT_CMD("ranlib", "库索引"); REG_EXT_CMD("as", "汇编器"); REG_EXT_CMD("ld", "链接器");

    // 安全与提权
    REG_EXT_CMD("openssl", "加密工具"); REG_EXT_CMD("gpg", "GPG加密"); REG_EXT_CMD("md5sum", "MD5校验");
    REG_EXT_CMD("sha1sum", "SHA1校验"); REG_EXT_CMD("sha256sum", "SHA256校验"); REG_EXT_CMD("sha512sum", "SHA512校验");
    REG_EXT_CMD("chroot", "切换根目录"); REG_EXT_CMD("sudo", "提权执行"); REG_EXT_CMD("su", "切换用户"); REG_EXT_CMD("passwd", "修改密码");

    // 压缩
    REG_EXT_CMD("gzip", "压缩"); REG_EXT_CMD("gunzip", "解压"); REG_EXT_CMD("bzip2", "压缩");
    REG_EXT_CMD("bunzip2", "解压"); REG_EXT_CMD("xz", "压缩"); REG_EXT_CMD("unxz", "解压");
    REG_EXT_CMD("7z", "7zip"); REG_EXT_CMD("rar", "RAR压缩"); REG_EXT_CMD("unrar", "RAR解压"); REG_EXT_CMD("zcat", "查看压缩");

    // 网络
    REG_EXT_CMD("ssh", "SSH客户端"); REG_EXT_CMD("scp", "SCP传输"); REG_EXT_CMD("rsync", "同步工具");
    REG_EXT_CMD("ftp", "FTP客户端"); REG_EXT_CMD("sftp", "SFTP客户端"); REG_EXT_CMD("telnet", "Telnet");
    REG_EXT_CMD("nc", "Netcat"); REG_EXT_CMD("socat", "Socket工具"); REG_EXT_CMD("nmap", "端口扫描");
    REG_EXT_CMD("tcpdump", "抓包"); REG_EXT_CMD("dig", "DNS查询"); REG_EXT_CMD("host", "DNS查询");

    // 磁盘/硬件
    REG_EXT_CMD("mount", "挂载"); REG_EXT_CMD("umount", "卸载"); REG_EXT_CMD("fdisk", "分区");
    REG_EXT_CMD("mkfs", "格式化"); REG_EXT_CMD("fsck", "检查修复"); REG_EXT_CMD("dd", "磁盘拷贝");
    REG_EXT_CMD("sync", "同步磁盘"); REG_EXT_CMD("swapon", "启用交换"); REG_EXT_CMD("swapoff", "禁用交换");
    REG_EXT_CMD("lspci", "PCI设备"); REG_EXT_CMD("lsusb", "USB设备"); REG_EXT_CMD("lshw", "硬件列表");
    REG_EXT_CMD("dmidecode", "DMI信息"); REG_EXT_CMD("hdparm", "磁盘参数"); REG_EXT_CMD("sensors", "传感器");

    // 进程与用户
    REG_EXT_CMD("htop", "交互式进程"); REG_EXT_CMD("killall", "按名杀进程"); REG_EXT_CMD("pkill", "按名杀进程");
    REG_EXT_CMD("pgrep", "查找进程"); REG_EXT_CMD("nice", "调整优先级"); REG_EXT_CMD("renice", "调整优先级");
    REG_EXT_CMD("nohup", "后台运行"); REG_EXT_CMD("pidof", "查找PID"); REG_EXT_CMD("useradd", "添加用户");
    REG_EXT_CMD("userdel", "删除用户"); REG_EXT_CMD("usermod", "修改用户"); REG_EXT_CMD("groupadd", "添加组");
    REG_EXT_CMD("groupdel", "删除组"); REG_EXT_CMD("chown", "更改所有者"); REG_EXT_CMD("chgrp", "更改组");

    // 系统服务与日志
    REG_EXT_CMD("systemctl", "服务管理"); REG_EXT_CMD("service", "服务管理"); REG_EXT_CMD("shutdown", "关机");
    REG_EXT_CMD("reboot", "重启"); REG_EXT_CMD("poweroff", "关机"); REG_EXT_CMD("dmesg", "内核日志"); REG_EXT_CMD("journalctl", "日志查看");

    // 查找/文本补充
    REG_EXT_CMD("whereis", "查找命令"); REG_EXT_CMD("locate", "快速查找"); REG_EXT_CMD("man", "手册");
    REG_EXT_CMD("whatis", "简介"); REG_EXT_CMD("apropos", "搜索手册"); REG_EXT_CMD("awk", "文本处理");
    REG_EXT_CMD("perl", "Perl"); REG_EXT_CMD("egrep", "grep"); REG_EXT_CMD("fgrep", "grep");
    REG_EXT_CMD("printf", "格式化输出"); REG_EXT_CMD("column", "列格式化"); REG_EXT_CMD("od", "八进制显示");
    REG_EXT_CMD("hexdump", "十六进制"); REG_EXT_CMD("xxd", "十六进制"); REG_EXT_CMD("patch", "补丁");
    REG_EXT_CMD("comm", "对比"); REG_EXT_CMD("cmp", "对比"); REG_EXT_CMD("fold", "折叠");
    REG_EXT_CMD("cal", "日历"); REG_EXT_CMD("ncal", "日历"); REG_EXT_CMD("hwclock", "硬件时钟");
    REG_EXT_CMD("ip", "网络配置"); REG_EXT_CMD("ss", "Socket统计"); REG_EXT_CMD("iptables", "防火墙");
    REG_EXT_CMD("ethtool", "网卡工具"); REG_EXT_CMD("iwconfig", "无线配置"); REG_EXT_CMD("dc", "计算器");
    REG_EXT_CMD("numfmt", "数字格式化"); REG_EXT_CMD("truncate", "截断"); REG_EXT_CMD("mktemp", "临时文件");
    REG_EXT_CMD("realpath", "绝对路径"); REG_EXT_CMD("readlink", "链接目标"); REG_EXT_CMD("basename", "文件名");
    REG_EXT_CMD("dirname", "目录名"); REG_EXT_CMD("nproc", "CPU数"); REG_EXT_CMD("ulimit", "资源限制");
    REG_EXT_CMD("xargs", "参数传递"); REG_EXT_CMD("printenv", "环境");

    // 特殊处理
    registerCommand("shred", [ui](const QStringList&) {
        ui->appendOutput("shred在Android不可用，请用rm\n", ERROR_COLOR);
    }, "安全删除", "shred");
    
    registerCommand("seq", [ui](const QStringList& args) {
        if (args.size() == 1) {
            for (int i = 1; i <= args[0].toInt(); ++i) ui->appendOutput(QString::number(i) + "\n");
        } else if (args.size() == 2) {
            int start = args[0].toInt(), end = args[1].toInt();
            if (start <= end) for (int i = start; i <= end; ++i) ui->appendOutput(QString::number(i) + "\n");
            else for (int i = start; i >= end; --i) ui->appendOutput(QString::number(i) + "\n");
        } else if (args.size() >= 3) {
            int start = args[0].toInt(), step = args[1].toInt(), end = args[2].toInt();
            if (step == 0) { ui->appendOutput("步长不能为0\n", ERROR_COLOR); return; }
            if (step > 0) for (int i = start; i <= end; i += step) ui->appendOutput(QString::number(i) + "\n");
            else for (int i = start; i >= end; i += step) ui->appendOutput(QString::number(i) + "\n");
        }
    }, "序列", "seq");
}