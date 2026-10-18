#include <QDir>
#include <QFile>
#include "commands.h"
#include "terminal/terminal.h"
#include "utils/utils.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QNetworkInterface>
#include <QHostInfo>
#include <QTcpSocket>
#include <QUrl>
#include <QEventLoop>
#include <QTimer>
#include <QStandardPaths>

void CommandRegistry::registerNetworkCommands(TerminalWidget* ui) {

    // ping
    registerCommand("ping", [ui](const QStringList& args) {
        if (args.isEmpty()) { ui->appendOutput("用法: ping <主机>\n", ERROR_COLOR); return; }
        CommandHelper::runExternal("ping", args, ui->currentDir, ui, 30000, true);
    }, "Ping主机", "ping <主机>");

    // curl
    registerCommand("curl", [ui](const QStringList& args) {
        if (args.isEmpty()) return;
        QString curlPath = QStandardPaths::findExecutable("curl");
        if (!curlPath.isEmpty()) {
            CommandHelper::runExternal("curl", args, ui->currentDir, ui);
        } else {
            QString url = args.last();
            if (url.startsWith("http")) {
                QNetworkAccessManager* mgr = new QNetworkAccessManager();
                QNetworkReply* reply = mgr->get(QNetworkRequest{QUrl(url)});
                QEventLoop loop;
                QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
                QTimer::singleShot(10000, &loop, &QEventLoop::quit);
                loop.exec();
                if (reply->error() == QNetworkReply::NoError)
                    ui->appendOutput(reply->readAll().left(2000));
                else ui->appendOutput("请求失败\n", ERROR_COLOR);
                reply->deleteLater(); mgr->deleteLater();
            }
        }
    }, "HTTP请求", "curl <URL>");

    // wget
    registerCommand("wget", [ui](const QStringList& args) {
        if (args.isEmpty()) return;
        QString url = args.last();
        if (url.startsWith("http")) {
            QNetworkAccessManager* mgr = new QNetworkAccessManager();
            QNetworkReply* reply = mgr->get(QNetworkRequest{QUrl(url)});
            QEventLoop loop;
            QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
            QTimer::singleShot(30000, &loop, &QEventLoop::quit);
            loop.exec();
            if (reply->error() == QNetworkReply::NoError) {
                QString fn = url.split('/').last();
                if (fn.isEmpty()) fn = "index.html";
                QFile file(QDir(ui->currentDir).absoluteFilePath(fn));
                if (file.open(QIODevice::WriteOnly)) { file.write(reply->readAll()); file.close(); }
                ui->appendOutput(QString("已下载: %1\n").arg(fn), INFO_COLOR);
            }
            reply->deleteLater(); mgr->deleteLater();
        }
    }, "下载文件", "wget <URL>");

    // ifconfig
    registerCommand("ifconfig", [ui](const QStringList&) {
        for (const QNetworkInterface& iface : QNetworkInterface::allInterfaces()) {
            ui->appendOutput(QString("%1: %2\n").arg(iface.humanReadableName(),
                iface.flags() & QNetworkInterface::IsUp ? "UP" : "DOWN"));
            for (const QNetworkAddressEntry& entry : iface.addressEntries())
                ui->appendOutput(QString("  inet %1\n").arg(entry.ip().toString()));
        }
    }, "网络接口", "ifconfig");

    // ipconfig
    registerCommand("ipconfig", [ui](const QStringList& args) {
        ui->executeInternalCommand("ifconfig " + args.join(' '));
    }, "IP配置", "ipconfig");

    // netstat
    registerCommand("netstat", [ui](const QStringList& args) {
        CommandHelper::runExternal("netstat", args.isEmpty() ? QStringList{"-an"} : args,
                                   ui->currentDir, ui);
    }, "网络连接", "netstat");

    // nslookup
    registerCommand("nslookup", [ui](const QStringList& args) {
        if (args.isEmpty()) return;
        QHostInfo::lookupHost(args[0], [ui](const QHostInfo& info) {
            if (info.error() == QHostInfo::NoError)
                for (const QHostAddress& addr : info.addresses())
                    ui->appendOutput(addr.toString() + "\n");
            else ui->appendOutput("解析失败\n", ERROR_COLOR);
        });
    }, "DNS查询", "nslookup <域名>");

    // tracert / traceroute
    registerCommand("tracert", [ui](const QStringList& args) {
        CommandHelper::runExternal("traceroute", args, ui->currentDir, ui, 60000, true);
    }, "路由追踪", "tracert <主机>");

    registerCommand("traceroute", [ui](const QStringList& args) {
        ui->executeInternalCommand("tracert " + args.join(' '));
    }, "路由追踪", "traceroute <主机>");

    // route
    registerCommand("route", [ui](const QStringList& args) {
        CommandHelper::runExternal("route", args, ui->currentDir, ui);
    }, "路由表", "route");

    // arp
    registerCommand("arp", [ui](const QStringList& args) {
        CommandHelper::runExternal("arp", args.isEmpty() ? QStringList{"-a"} : args,
                                   ui->currentDir, ui);
    }, "ARP表", "arp");

    // whois
    registerCommand("whois", [ui](const QStringList& args) {
        if (args.isEmpty()) return;
        QTcpSocket* sock = new QTcpSocket();
        sock->connectToHost("whois.iana.org", 43);
        if (sock->waitForConnected(5000)) {
            sock->write(QString("%1\r\n").arg(args[0]).toUtf8());
            sock->waitForReadyRead(5000);
            ui->appendOutput(sock->readAll().left(2000));
        }
        sock->deleteLater();
    }, "WHOIS查询", "whois <域名>");
}