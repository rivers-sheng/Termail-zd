#include "commands.h"
#include "ui/terminal_ui.h"
#include "utils/utils.h"

#include <QProcess>
#include <QTcpSocket>
#include <QHostInfo>
#include <QNetworkInterface>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QEventLoop>
#include <QStorageInfo>
#include <QStandardPaths>

// ============ 网络命令注册 ============
void CommandRegistry::registerNetworkCommands(TerminalUI* ui) {

    // ping
    registerCommand("ping", [ui](const QStringList& args) {
        if (args.isEmpty()) {
            ui->appendOutput("用法: ping [-n count] <主机>\n", THEME.yellow.name());
            return;
        }
        QStringList cmdArgs;
        cmdArgs << "ping";
        for (int i = 0; i < args.size(); ++i) {
            cmdArgs << args[i];
        }
        CommandHelper::runExternal(cmdArgs[0], cmdArgs.mid(1), ui->currentDir, ui, 30000, true);
    }, "Ping 网络主机", "ping [-n count] <主机>");

    // curl
    registerCommand("curl", [ui](const QStringList& args) {
        if (args.isEmpty()) {
            ui->appendOutput("用法: curl [选项] <URL>\n", THEME.yellow.name());
            return;
        }
        QString curlPath = QStandardPaths::findExecutable("curl");
        if (!curlPath.isEmpty()) {
            CommandHelper::runExternal("curl", args, ui->currentDir, ui);
        } else {
            // 使用 Qt 内置网络请求
            QString url = args.last();
            if (url.startsWith("http")) {
                QNetworkAccessManager* manager = new QNetworkAccessManager();
                QNetworkRequest request{QUrl(url)};
                QNetworkReply* reply = manager->get(request);
                QEventLoop loop;
                QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
                QTimer::singleShot(10000, &loop, &QEventLoop::quit);
                loop.exec();
                if (reply->error() == QNetworkReply::NoError) {
                    ui->appendOutput(reply->readAll().left(2000));
                } else {
                    ui->appendOutput(QString("错误: %1\n").arg(reply->errorString()), THEME.red.name());
                }
                reply->deleteLater();
                manager->deleteLater();
            } else {
                ui->appendOutput("请提供有效的 URL\n", THEME.red.name());
            }
        }
    }, "网络请求工具", "curl [选项] <URL>");

    // wget
    registerCommand("wget", [ui](const QStringList& args) {
        if (args.isEmpty()) {
            ui->appendOutput("用法: wget <URL>\n", THEME.yellow.name());
            return;
        }
        QString wgetPath = QStandardPaths::findExecutable("wget");
        if (!wgetPath.isEmpty()) {
            CommandHelper::runExternal("wget", args, ui->currentDir, ui, 60000, true);
        } else {
            QString url = args.last();
            if (url.startsWith("http")) {
                QNetworkAccessManager* manager = new QNetworkAccessManager();
                QNetworkRequest request{QUrl(url)};
                QNetworkReply* reply = manager->get(request);
                QEventLoop loop;
                QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
                QTimer::singleShot(30000, &loop, &QEventLoop::quit);
                loop.exec();
                if (reply->error() == QNetworkReply::NoError) {
                    QString fn = url.split('/').last();
                    if (fn.isEmpty()) fn = "index.html";
                    QString path = QDir(ui->currentDir).absoluteFilePath(fn);
                    QFile file(path);
                    if (file.open(QIODevice::WriteOnly)) {
                        file.write(reply->readAll());
                        file.close();
                        ui->appendOutput(QString("已下载: %1\n").arg(fn), THEME.green.name());
                    }
                } else {
                    ui->appendOutput(QString("错误: %1\n").arg(reply->errorString()), THEME.red.name());
                }
                reply->deleteLater();
                manager->deleteLater();
            }
        }
    }, "下载文件", "wget <URL>");

    // ifconfig
    registerCommand("ifconfig", [ui](const QStringList& args) {
#ifdef Q_OS_WIN
        CommandHelper::runExternal("ipconfig", args, ui->currentDir, ui);
#else
        CommandHelper::runExternal("ifconfig", args, ui->currentDir, ui);
#endif
        // 后备方案：使用 Qt 网络接口
        if (args.isEmpty()) {
            QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
            for (const QNetworkInterface& iface : interfaces) {
                ui->appendOutput(QString("%1: flags=%2\n")
                    .arg(iface.humanReadableName())
                    .arg(iface.flags() & QNetworkInterface::IsUp ? "UP" : "DOWN"));
                QList<QNetworkAddressEntry> entries = iface.addressEntries();
                for (const QNetworkAddressEntry& entry : entries) {
                    ui->appendOutput(QString("        inet %1  netmask %2\n")
                        .arg(entry.ip().toString())
                        .arg(entry.netmask().toString()));
                }
            }
        }
    }, "网络接口配置", "ifconfig");

    // ipconfig
    registerCommand("ipconfig", [ui](const QStringList& args) {
#ifdef Q_OS_WIN
        CommandHelper::runExternal("ipconfig", args, ui->currentDir, ui);
#else
        ui->executeInternalCommand("ifconfig " + args.join(' '));
#endif
    }, "IP配置 (Windows)", "ipconfig");

    // netstat
    registerCommand("netstat", [ui](const QStringList& args) {
        QStringList cmdArgs;
        cmdArgs << "netstat";
        if (args.isEmpty()) {
            cmdArgs << "-an";
        } else {
            cmdArgs << args;
        }
        CommandHelper::runExternal(cmdArgs[0], cmdArgs.mid(1), ui->currentDir, ui);
    }, "网络连接状态", "netstat [-an]");

    // nslookup
    registerCommand("nslookup", [ui](const QStringList& args) {
        if (args.isEmpty()) {
            CommandHelper::runExternal("nslookup", {}, ui->currentDir, ui);
            return;
        }
        // 使用 Qt DNS 查询
        QHostInfo::lookupHost(args[0], [ui](const QHostInfo& info) {
            if (info.error() == QHostInfo::NoError) {
                ui->appendOutput(QString("服务器: %1\n").arg(QHostInfo::localHostName()));
                ui->appendOutput(QString("名称: %1\n").arg(info.hostName()));
                for (const QHostAddress& addr : info.addresses()) {
                    ui->appendOutput(QString("地址: %1\n").arg(addr.toString()));
                }
            } else {
                ui->appendOutput(QString("DNS查询失败: %1\n").arg(info.errorString()), THEME.red.name());
            }
        });
    }, "DNS查询", "nslookup <域名>");

    // tracert / traceroute
    registerCommand("tracert", [ui](const QStringList& args) {
#ifdef Q_OS_WIN
        CommandHelper::runExternal("tracert", args, ui->currentDir, ui, 60000, true);
#else
        CommandHelper::runExternal("traceroute", args, ui->currentDir, ui, 60000, true);
#endif
    }, "路由追踪", "tracert <主机>");

    registerCommand("traceroute", [ui](const QStringList& args) {
        ui->executeInternalCommand("tracert " + args.join(' '));
    }, "路由追踪 (Linux)", "traceroute <主机>");

    // route
    registerCommand("route", [ui](const QStringList& args) {
#ifdef Q_OS_WIN
        QStringList cmdArgs = {"route", "print"};
#else
        QStringList cmdArgs = {"route", "-n"};
#endif
        cmdArgs << args;
        CommandHelper::runExternal(cmdArgs[0], cmdArgs.mid(1), ui->currentDir, ui);
    }, "路由表", "route");

    // arp
    registerCommand("arp", [ui](const QStringList& args) {
#ifdef Q_OS_WIN
        QStringList cmdArgs = {"arp", "-a"};
#else
        QStringList cmdArgs = {"arp", "-n"};
#endif
        cmdArgs << args;
        CommandHelper::runExternal(cmdArgs[0], cmdArgs.mid(1), ui->currentDir, ui);
    }, "ARP表", "arp");

    // whois
    registerCommand("whois", [ui](const QStringList& args) {
        if (args.isEmpty()) {
            ui->appendOutput("用法: whois <域名>\n", THEME.yellow.name());
            return;
        }
        QTcpSocket* socket = new QTcpSocket();
        socket->connectToHost("whois.iana.org", 43);
        if (socket->waitForConnected(5000)) {
            socket->write(QString("%1\r\n").arg(args[0]).toUtf8());
            socket->waitForBytesWritten(3000);
            socket->waitForReadyRead(5000);
            QString response = socket->readAll();
            ui->appendOutput(response.left(2000));
        } else {
            ui->appendOutput(QString("WHOIS 查询失败: 无法连接\n"), THEME.red.name());
        }
        socket->deleteLater();
    }, "WHOIS查询", "whois <域名>");
}