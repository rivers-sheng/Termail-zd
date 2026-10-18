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

void CommandRegistry::registerMany4Commands(TerminalWidget* ui) {

    // ========== 文件管理 ==========

    // open - 用系统应用打开文件
    registerCommand("open", [ui](const QStringList& args) {
        if (args.isEmpty()) {
            ui->appendOutput("用法: open <文件>\n", ERROR_COLOR);
            return;
        }
        QString path = QDir(ui->currentDir).absoluteFilePath(args[0]);
        if (!QFile::exists(path)) {
            ui->appendOutput("文件不存在\n", ERROR_COLOR);
            return;
        }
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
        ui->appendOutput(QString("已打开: %1\n").arg(args[0]), INFO_COLOR);
    }, "系统打开文件", "open <文件>");

    // share - 分享文件
    registerCommand("share", [ui](const QStringList& args) {
        if (args.isEmpty()) {
            ui->appendOutput("用法: share <文件>\n", ERROR_COLOR);
            return;
        }
        QString path = QDir(ui->currentDir).absoluteFilePath(args[0]);
        if (!QFile::exists(path)) {
            ui->appendOutput("文件不存在\n", ERROR_COLOR);
            return;
        }
        ui->appendOutput(QString("分享: %1\n").arg(args[0]), INFO_COLOR);
        ui->appendOutput("提示: 长按输出区选中路径，用系统分享\n", INFO_COLOR);
    }, "分享文件", "share <文件>");

    // mount-sd - 挂载存储路径
    registerCommand("mount-sd", [ui](const QStringList&) {
        QStringList paths = {"/sdcard", "/storage/emulated/0", "/storage/emulated/0/Download"};
        ui->appendOutput("可用存储路径:\n", ACCENT_COLOR);
        for (const QString& p : paths) {
            if (QDir(p).exists())
                ui->appendOutput("  " + p + "\n", PROMPT_COLOR);
        }
        ui->appendOutput("用 cd /sdcard 进入\n", INFO_COLOR);
    }, "查看存储路径", "mount-sd");

    // ========== 网络 ==========

    // ip-info - 本机公网IP
    registerCommand("ip-info", [ui](const QStringList&) {
        ui->appendOutput("查询公网IP...\n", ACCENT_COLOR);
        QNetworkAccessManager* mgr = new QNetworkAccessManager();
        QNetworkReply* reply = mgr->get(QNetworkRequest{QUrl("http://ip.3322.net")});
        QEventLoop loop;
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        QTimer::singleShot(8000, &loop, &QEventLoop::quit);
        loop.exec();
        if (reply->error() == QNetworkReply::NoError)
            ui->appendOutput("公网IP: " + QString(reply->readAll()).trimmed() + "\n", PROMPT_COLOR);
        else
            ui->appendOutput("查询失败\n", ERROR_COLOR);
        reply->deleteLater();
        mgr->deleteLater();
    }, "公网IP", "ip-info");

    // http-get - 简易GET
    registerCommand("http-get", [ui](const QStringList& args) {
        if (args.isEmpty()) {
            ui->appendOutput("用法: http-get <URL>\n", ERROR_COLOR);
            return;
        }
        QString url = args[0];
        if (!url.startsWith("http")) {
            ui->appendOutput("URL 必须以 http 开头\n", ERROR_COLOR);
            return;
        }
        QNetworkAccessManager* mgr = new QNetworkAccessManager();
        QNetworkReply* reply = mgr->get(QNetworkRequest{QUrl(url)});
        QEventLoop loop;
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        QTimer::singleShot(10000, &loop, &QEventLoop::quit);
        loop.exec();
        if (reply->error() == QNetworkReply::NoError)
            ui->appendOutput(reply->readAll().left(5000));
        else
            ui->appendOutput("请求失败: " + reply->errorString() + "\n", ERROR_COLOR);
        reply->deleteLater();
        mgr->deleteLater();
    }, "HTTP GET", "http-get <URL>");

    // port-check - 端口检测
    registerCommand("port-check", [ui](const QStringList& args) {
        if (args.size() < 2) {
            ui->appendOutput("用法: port-check <主机> <端口>\n", ERROR_COLOR);
            return;
        }
        QString host = args[0];
        int port = args[1].toInt();
        if (port <= 0 || port > 65535) {
            ui->appendOutput("端口无效\n", ERROR_COLOR);
            return;
        }
        QTcpSocket sock;
        sock.connectToHost(host, port);
        if (sock.waitForConnected(3000))
            ui->appendOutput(QString("%1:%2 开放\n").arg(host).arg(port), PROMPT_COLOR);
        else
            ui->appendOutput(QString("%1:%2 关闭或超时\n").arg(host).arg(port), ERROR_COLOR);
    }, "端口检测", "port-check <主机> <端口>");

    // dns-flush - 清DNS缓存（提示）
    registerCommand("dns-flush", [ui](const QStringList&) {
        ui->appendOutput("Android 不支持清理DNS缓存\n", ERROR_COLOR);
        ui->appendOutput("可重启网络: 开关飞行模式\n", INFO_COLOR);
    }, "清DNS缓存", "dns-flush");

    // speedtest - 网速测试
    registerCommand("speedtest", [ui](const QStringList&) {
        ui->appendOutput("测试网速（下载10MB测试文件）...\n", ACCENT_COLOR);
        QNetworkAccessManager* mgr = new QNetworkAccessManager();
        QNetworkReply* reply = mgr->get(QNetworkRequest{
            QUrl("http://speedtest.tele2.net/10MB.zip")});
        QDateTime start = QDateTime::currentDateTime();
        QEventLoop loop;
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        QTimer::singleShot(30000, &loop, &QEventLoop::quit);
        loop.exec();
        qint64 elapsed = start.msecsTo(QDateTime::currentDateTime());
        if (reply->error() == QNetworkReply::NoError) {
            qint64 size = reply->bytesAvailable();
            double speed = (size / 1024.0 / 1024.0) / (elapsed / 1000.0);
            ui->appendOutput(QString("下载: %1MB, 耗时: %2秒\n")
                .arg(size / 1024.0 / 1024.0, 0, 'f', 2).arg(elapsed / 1000.0, 0, 'f', 1));
            ui->appendOutput(QString("速度: %1 MB/s\n").arg(speed, 0, 'f', 2), PROMPT_COLOR);
        } else {
            ui->appendOutput("测试失败（可能网络不通或该测试源不可用）\n", ERROR_COLOR);
        }
        reply->deleteLater();
        mgr->deleteLater();
    }, "网速测试", "speedtest");

    // ========== 系统 ==========

    // meminfo - 内存信息
    registerCommand("meminfo", [ui](const QStringList&) {
        QFile f("/proc/meminfo");
        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            ui->appendOutput(f.readAll());
            f.close();
        } else {
            ui->appendOutput("无法读取内存信息\n", ERROR_COLOR);
        }
    }, "内存详情", "meminfo");

    // cpu-temp - CPU温度
    registerCommand("cpu-temp", [ui](const QStringList&) {
        QStringList paths = {
            "/sys/class/thermal/thermal_zone0/temp",
            "/sys/class/thermal/thermal_zone1/temp"
        };
        bool found = false;
        for (const QString& p : paths) {
            QFile f(p);
            if (f.open(QIODevice::ReadOnly)) {
                int temp = QString(f.readAll()).trimmed().toInt();
                f.close();
                if (temp > 0) {
                    ui->appendOutput(QString("CPU温度: %1°C\n")
                        .arg(temp / 1000.0, 0, 'f', 1), PROMPT_COLOR);
                    found = true;
                }
            }
        }
        if (!found) ui->appendOutput("无法读取CPU温度（需要root）\n", ERROR_COLOR);
    }, "CPU温度", "cpu-temp");

    // battery - 电池信息
    registerCommand("battery", [ui](const QStringList&) {
        QStringList paths = {
            "/sys/class/power_supply/battery/capacity",
            "/sys/class/power_supply/BAT0/capacity"
        };
        bool found = false;
        for (const QString& p : paths) {
            QFile f(p);
            if (f.open(QIODevice::ReadOnly)) {
                QString cap = QString(f.readAll()).trimmed();
                f.close();
                if (!cap.isEmpty()) {
                    ui->appendOutput(QString("电量: %1%\n").arg(cap), PROMPT_COLOR);
                    found = true;
                }
            }
        }
        // 状态
        QFile statusFile("/sys/class/power_supply/battery/status");
        if (statusFile.open(QIODevice::ReadOnly)) {
            ui->appendOutput("状态: " + QString(statusFile.readAll()).trimmed() + "\n", INFO_COLOR);
            statusFile.close();
        }
        if (!found) ui->appendOutput("无法读取电池信息\n", ERROR_COLOR);
    }, "电池信息", "battery");

    // disk-usage - 磁盘占用
    registerCommand("disk-usage", [ui](const QStringList&) {
        QStorageInfo storage(ui->currentDir);
        if (storage.isValid()) {
            qint64 total = storage.bytesTotal();
            qint64 free = storage.bytesAvailable();
            qint64 used = total - free;
            double percent = total > 0 ? (used * 100.0 / total) : 0;
            ui->appendOutput(QString("总容量: %1\n").arg(Utils::formatSize(total)));
            ui->appendOutput(QString("已使用: %1 (%2%)\n")
                .arg(Utils::formatSize(used)).arg(percent, 0, 'f', 1));
            ui->appendOutput(QString("可用: %1\n").arg(Utils::formatSize(free)), PROMPT_COLOR);
        } else {
            ui->appendOutput("无法读取磁盘信息\n", ERROR_COLOR);
        }
    }, "磁盘占用", "disk-usage");

    // process-top - 占用最高的进程
    registerCommand("process-top", [ui](const QStringList&) {
        ui->appendOutput("正在获取进程列表...\n", ACCENT_COLOR);
        QProcess p;
        p.start("ps", QStringList() << "-eo" << "pid,comm,%cpu,%mem" << "--sort=-%cpu");
        p.waitForFinished(5000);
        QString output = p.readAllStandardOutput();
        if (output.isEmpty()) {
            p.start("ps", QStringList() << "aux");
            p.waitForFinished(5000);
            output = p.readAllStandardOutput();
        }
        QStringList lines = output.split('\n');
        for (int i = 0; i < qMin(15, lines.size()); ++i) {
            if (!lines[i].trimmed().isEmpty())
                ui->appendOutput(lines[i] + "\n");
        }
    }, "占用最高进程", "process-top");

    // kill-all - 批量杀进程
    registerCommand("kill-all", [ui](const QStringList& args) {
        if (args.isEmpty()) {
            ui->appendOutput("用法: kill-all <进程名>\n", ERROR_COLOR);
            return;
        }
        CommandHelper::runExternal("pkill", QStringList() << "-f" << args[0],
                                   ui->currentDir, ui);
    }, "批量杀进程", "kill-all <名称>");

    // ========== 文本 ==========

    // md5 - 文件哈希
    registerCommand("md5", [ui](const QStringList& args) {
        if (args.isEmpty()) {
            ui->appendOutput("用法: md5 <文件>\n", ERROR_COLOR);
            return;
        }
        QFile f(QDir(ui->currentDir).absoluteFilePath(args[0]));
        if (!f.open(QIODevice::ReadOnly)) {
            ui->appendOutput("文件不存在\n", ERROR_COLOR);
            return;
        }
        QCryptographicHash hash(QCryptographicHash::Md5);
        if (hash.addData(&f)) {
            ui->appendOutput(QString("MD5: %1\n").arg(QString(hash.result().toHex())));
        }
        f.close();
    }, "文件MD5", "md5 <文件>");

    // json-format - JSON格式化
    registerCommand("json-format", [ui](const QStringList& args) {
        if (args.isEmpty()) {
            ui->appendOutput("用法: json-format <文件>\n", ERROR_COLOR);
            return;
        }
        QFile f(QDir(ui->currentDir).absoluteFilePath(args[0]));
        if (!f.open(QIODevice::ReadOnly)) {
            ui->appendOutput("文件不存在\n", ERROR_COLOR);
            return;
        }
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
        f.close();
        if (err.error != QJsonParseError::NoError) {
            ui->appendOutput("JSON 格式错误: " + err.errorString() + "\n", ERROR_COLOR);
            return;
        }
        ui->appendOutput(doc.toJson(QJsonDocument::Indented));
    }, "JSON格式化", "json-format <文件>");

    // hex-view - 十六进制查看
    registerCommand("hex-view", [ui](const QStringList& args) {
        if (args.isEmpty()) {
            ui->appendOutput("用法: hex-view <文件>\n", ERROR_COLOR);
            return;
        }
        QFile f(QDir(ui->currentDir).absoluteFilePath(args[0]));
        if (!f.open(QIODevice::ReadOnly)) {
            ui->appendOutput("文件不存在\n", ERROR_COLOR);
            return;
        }
        QByteArray data = f.read(1024);  // 只显示前1KB
        f.close();
        for (int i = 0; i < data.size(); i += 16) {
            QString line = QString("%1  ").arg(i, 8, 16, QChar('0'));
            for (int j = 0; j < 16; ++j) {
                if (i + j < data.size())
                    line += QString("%1 ").arg((unsigned char)data[i + j], 2, 16, QChar('0'));
                else
                    line += "   ";
                if (j == 7) line += " ";
            }
            line += " |";
            for (int j = 0; j < 16 && i + j < data.size(); ++j) {
                char c = data[i + j];
                line += (c >= 32 && c < 127) ? c : '.';
            }
            line += "|";
            ui->appendOutput(line + "\n");
        }
    }, "十六进制查看", "hex-view <文件>");

    // diff-color - 彩色diff
    registerCommand("diff-color", [ui](const QStringList& args) {
        if (args.size() < 2) {
            ui->appendOutput("用法: diff-color <文件1> <文件2>\n", ERROR_COLOR);
            return;
        }
        QFile f1(QDir(ui->currentDir).absoluteFilePath(args[0]));
        QFile f2(QDir(ui->currentDir).absoluteFilePath(args[1]));
        if (!f1.open(QIODevice::ReadOnly) || !f2.open(QIODevice::ReadOnly)) {
            ui->appendOutput("文件不存在\n", ERROR_COLOR);
            return;
        }
        QStringList l1 = QString(f1.readAll()).split('\n');
        QStringList l2 = QString(f2.readAll()).split('\n');
        for (int i = 0; i < qMax(l1.size(), l2.size()); ++i) {
            QString a = i < l1.size() ? l1[i] : "";
            QString b = i < l2.size() ? l2[i] : "";
            if (a != b) {
                if (!a.isEmpty()) ui->appendOutput("< " + a + "\n", ERROR_COLOR);
                if (!b.isEmpty()) ui->appendOutput("> " + b + "\n", PROMPT_COLOR);
            }
        }
    }, "彩色diff", "diff-color <文件1> <文件2>");

    // ========== 终端 ==========

    // title - 设置窗口标题
    registerCommand("title", [ui](const QStringList& args) {
        if (args.isEmpty()) {
            ui->appendOutput("用法: title <文本>\n", ERROR_COLOR);
            return;
        }
        QString title = args.join(' ');
        QWidget* w = ui->window();
        if (w) w->setWindowTitle(title);
        ui->appendOutput(QString("标题已设置: %1\n").arg(title), INFO_COLOR);
    }, "设置标题", "title <文本>");

    // copy - 复制到剪贴板
    registerCommand("copy", [ui](const QStringList& args) {
        if (args.isEmpty()) {
            ui->appendOutput("用法: copy <文本>\n", ERROR_COLOR);
            return;
        }
        QApplication::clipboard()->setText(args.join(' '));
        ui->appendOutput("已复制到剪贴板\n", INFO_COLOR);
    }, "复制到剪贴板", "copy <文本>");

    // paste - 从剪贴板粘贴
    registerCommand("paste-clip", [ui](const QStringList&) {
        QString text = QApplication::clipboard()->text();
        ui->appendOutput("剪贴板内容:\n" + text + "\n", PROMPT_COLOR);
    }, "查看剪贴板", "paste-clip");

    // bell - 响铃
    registerCommand("bell", [ui](const QStringList&) {
        QApplication::beep();
        ui->appendOutput("🔔\n");
    }, "响铃", "bell");

    // notify - 发送通知
    registerCommand("notify", [ui](const QStringList& args) {
        if (args.isEmpty()) {
            ui->appendOutput("用法: notify <文本>\n", ERROR_COLOR);
            return;
        }
        ui->appendOutput(QString("📢 %1\n").arg(args.join(' ')), ACCENT_COLOR);
    }, "发送通知", "notify <文本>");

    // font-list - 可用字体
    registerCommand("font-list", [ui](const QStringList&) {
        QStringList fonts = QFontDatabase::families();
        ui->appendOutput(QString("共 %1 个字体\n").arg(fonts.size()), ACCENT_COLOR);
        int count = 0;
        for (const QString& f : fonts) {
            ui->appendOutput(f + "  ");
            if (++count % 3 == 0) ui->appendOutput("\n");
        }
        ui->appendOutput("\n");
    }, "可用字体", "font-list");

    // ========== 工具 ==========

    // pass-gen - 生成密码
    registerCommand("pass-gen", [ui](const QStringList& args) {
        int len = args.isEmpty() ? 16 : args[0].toInt();
        if (len < 4) len = 4;
        if (len > 128) len = 128;
        QString chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*";
        QString pass;
        for (int i = 0; i < len; ++i)
            pass += chars[QRandomGenerator::global()->bounded(chars.size())];
        ui->appendOutput(QString("密码(%1位): %2\n").arg(len).arg(pass), PROMPT_COLOR);
    }, "生成密码", "pass-gen [长度]");

    // base64-decode - Base64解码
    registerCommand("base64-decode", [ui](const QStringList& args) {
        if (args.isEmpty()) {
            ui->appendOutput("用法: base64-decode <文本>\n", ERROR_COLOR);
            return;
        }
        QByteArray decoded = QByteArray::fromBase64(args.join(' ').toUtf8());
        ui->appendOutput(decoded + "\n");
    }, "Base64解码", "base64-decode <文本>");

    // url-encode - URL编码
    registerCommand("url-encode", [ui](const QStringList& args) {
        if (args.isEmpty()) return;
        ui->appendOutput(QUrl::toPercentEncoding(args.join(' ')) + "\n");
    }, "URL编码", "url-encode <文本>");

    // url-decode - URL解码
    registerCommand("url-decode", [ui](const QStringList& args) {
        if (args.isEmpty()) return;
        ui->appendOutput(QUrl::fromPercentEncoding(args.join(' ').toUtf8()) + "\n");
    }, "URL解码", "url-decode <文本>");

    // timestamp - 时间戳转换
    registerCommand("timestamp", [ui](const QStringList& args) {
        if (args.isEmpty()) {
            qint64 now = QDateTime::currentSecsSinceEpoch();
            ui->appendOutput(QString("当前时间戳: %1\n").arg(now), PROMPT_COLOR);
            ui->appendOutput(QString("当前时间: %1\n")
                .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")));
            ui->appendOutput("用法: timestamp <时间戳>\n", INFO_COLOR);
            return;
        }
        bool ok;
        qint64 ts = args[0].toLongLong(&ok);
        if (!ok) {
            ui->appendOutput("时间戳格式错误\n", ERROR_COLOR);
            return;
        }
        QDateTime dt = QDateTime::fromSecsSinceEpoch(ts);
        ui->appendOutput(QString("%1 → %2\n")
            .arg(ts).arg(dt.toString("yyyy-MM-dd hh:mm:ss")), PROMPT_COLOR);
    }, "时间戳转换", "timestamp [时间戳]");

    // random-str - 随机字符串
    registerCommand("random-str", [ui](const QStringList& args) {
        int len = args.isEmpty() ? 8 : args[0].toInt();
        QString chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
        QString result;
        for (int i = 0; i < len; ++i)
            result += chars[QRandomGenerator::global()->bounded(chars.size())];
        ui->appendOutput(result + "\n", PROMPT_COLOR);
    }, "随机字符串", "random-str [长度]");

    // uuid-batch - 批量UUID
    registerCommand("uuid-batch", [ui](const QStringList& args) {
        int n = args.isEmpty() ? 5 : args[0].toInt();
        if (n > 100) n = 100;
        for (int i = 0; i < n; ++i)
            ui->appendOutput(QUuid::createUuid().toString(QUuid::WithoutBraces) + "\n");
    }, "批量UUID", "uuid-batch [数量]");

    // ========== 游戏增强 ==========

    // game-score - 历史最高分
    registerCommand("game-score", [ui](const QStringList&) {
        QSettings settings("termail", "games");
        ui->appendOutput("=== 游戏最高分 ===\n", ACCENT_COLOR);
        ui->appendOutput(QString("贪吃蛇: %1\n").arg(settings.value("snake_high", 0).toInt()), PROMPT_COLOR);
        ui->appendOutput(QString("俄罗斯方块: %1\n").arg(settings.value("tetris_high", 0).toInt()), PROMPT_COLOR);
        ui->appendOutput(QString("2048: %1\n").arg(settings.value("g2048_high", 0).toInt()), PROMPT_COLOR);
    }, "游戏最高分", "game-score");

    // pong - 乒乓球
    registerCommand("pong", [ui](const QStringList&) {
        ui->appendOutput("乒乓球游戏开发中...\n", INFO_COLOR);
    }, "乒乓球", "pong");

    // ========== 会话增强 ==========

    // session rename
    registerCommand("session-rename", [ui](const QStringList& args) {
        if (args.size() < 2) {
            ui->appendOutput("用法: session-rename <编号> <新名>\n", ERROR_COLOR);
            return;
        }
        bool ok;
        int id = args[0].toInt(&ok);
        if (!ok || !ui->sessions.contains(id)) {
            ui->appendOutput("会话不存在\n", ERROR_COLOR);
            return;
        }
        ui->sessions[id].name = args.mid(1).join(' ');
        ui->appendOutput(QString("会话 %1 重命名为: %2\n").arg(id).arg(ui->sessions[id].name), INFO_COLOR);
    }, "重命名会话", "session-rename <编号> <新名>");

    // session info
    registerCommand("session-info", [ui](const QStringList& args) {
        int id = args.isEmpty() ? ui->currentSessionId : args[0].toInt();
        if (!ui->sessions.contains(id)) {
            ui->appendOutput("会话不存在\n", ERROR_COLOR);
            return;
        }
        Session& s = ui->sessions[id];
        ui->appendOutput("=== 会话信息 ===\n", ACCENT_COLOR);
        ui->appendOutput(QString("编号: %1\n").arg(s.id));
        ui->appendOutput(QString("名称: %1\n").arg(s.name));
        ui->appendOutput(QString("目录: %1\n").arg(s.currentDir));
        ui->appendOutput(QString("历史: %1 条\n").arg(s.history.size()));
    }, "会话详情", "session-info [编号]");

    // session export
    registerCommand("session-export", [ui](const QStringList& args) {
        int id = args.isEmpty() ? ui->currentSessionId : args[0].toInt();
        if (!ui->sessions.contains(id)) {
            ui->appendOutput("会话不存在\n", ERROR_COLOR);
            return;
        }
        QString fn = QString("session_%1_history.txt").arg(id);
        QFile f(QDir(ui->currentDir).absoluteFilePath(fn));
        if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&f);
            for (const QString& h : ui->sessions[id].history)
                out << h << "\n";
            f.close();
            ui->appendOutput(QString("已导出到: %1\n").arg(fn), PROMPT_COLOR);
        }
    }, "导出会话历史", "session-export [编号]");

    // ========== 监控 ==========

    // watch-cpu
    registerCommand("watch-cpu", [ui](const QStringList&) {
        QFile f("/proc/stat");
        if (!f.open(QIODevice::ReadOnly)) {
            ui->appendOutput("无法读取CPU信息\n", ERROR_COLOR);
            return;
        }
        QString line = f.readLine();
        f.close();
        QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (parts.size() >= 5) {
            qint64 user = parts[1].toLongLong();
            qint64 nice = parts[2].toLongLong();
            qint64 sys = parts[3].toLongLong();
            qint64 idle = parts[4].toLongLong();
            qint64 total = user + nice + sys + idle;
            double usage = total > 0 ? ((total - idle) * 100.0 / total) : 0;
            ui->appendOutput(QString("CPU使用率: %1%\n").arg(usage, 0, 'f', 1), PROMPT_COLOR);
            ui->appendOutput(QString("用户: %1  系统: %2  空闲: %3\n")
                .arg(user).arg(sys).arg(idle), INFO_COLOR);
        }
    }, "CPU使用率", "watch-cpu");

    // watch-mem
    registerCommand("watch-mem", [ui](const QStringList&) {
        QFile f("/proc/meminfo");
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            ui->appendOutput("无法读取内存\n", ERROR_COLOR);
            return;
        }
        QTextStream in(&f);
        qint64 total = 0, free = 0, avail = 0;
        while (!in.atEnd()) {
            QString line = in.readLine();
            if (line.startsWith("MemTotal:")) total = line.split(QRegularExpression("\\s+"))[1].toLongLong();
            else if (line.startsWith("MemFree:")) free = line.split(QRegularExpression("\\s+"))[1].toLongLong();
            else if (line.startsWith("MemAvailable:")) avail = line.split(QRegularExpression("\\s+"))[1].toLongLong();
        }
        f.close();
        qint64 used = total - avail;
        double percent = total > 0 ? (used * 100.0 / total) : 0;
        ui->appendOutput(QString("总内存: %1 MB\n").arg(total / 1024), INFO_COLOR);
        ui->appendOutput(QString("已用: %1 MB (%2%)\n").arg(used / 1024).arg(percent, 0, 'f', 1), PROMPT_COLOR);
        ui->appendOutput(QString("可用: %1 MB\n").arg(avail / 1024), INFO_COLOR);
    }, "内存使用率", "watch-mem");

    // watch-battery
    registerCommand("watch-battery", [ui](const QStringList&) {
        ui->executeInternalCommand("battery");
    }, "电池监控", "watch-battery");

    // watch-net
    registerCommand("watch-net", [ui](const QStringList&) {
        QFile f("/proc/net/dev");
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            ui->appendOutput("无法读取网络信息\n", ERROR_COLOR);
            return;
        }
        QTextStream in(&f);
        ui->appendOutput("=== 网络流量 ===\n", ACCENT_COLOR);
        int lineNum = 0;
        while (!in.atEnd()) {
            QString line = in.readLine();
            lineNum++;
            if (lineNum <= 2) continue;
            QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
            if (parts.size() >= 10) {
                QString iface = parts[0];
                iface.remove(':');
                if (iface == "lo") continue;
                qint64 rx = parts[1].toLongLong();
                qint64 tx = parts[9].toLongLong();
                ui->appendOutput(QString("%1: ↓%2 ↑%3\n")
                    .arg(iface).arg(Utils::formatSize(rx)).arg(Utils::formatSize(tx)), PROMPT_COLOR);
            }
        }
        f.close();
    }, "网络流量", "watch-net");

}