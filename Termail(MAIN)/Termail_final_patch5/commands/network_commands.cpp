#include "commands.h"
#include "terminal/terminal.h"
#include "utils/utils.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QNetworkInterface>
#include <QHostInfo>
#include <QHostAddress>
#include <QTcpSocket>
#include <QUrl>
#include <QEventLoop>
#include <QTimer>
#include <QStandardPaths>
#include <QDir>
#include <QFile>

void CommandRegistry::registerNetworkCommands(TerminalWidget* ui) {

    registerCommand("ping", [ui](const QStringList& args) {
        if (args.isEmpty()) { ui->appendOutput("用法: ping <主机>\n", ERROR_COLOR); return; }
        CommandHelper::runExternal("ping", args, ui->currentDir, ui, 0, true);
    }, "Ping主机", "ping <主机>");

    registerCommand("curl", [ui](const QStringList& args) {
        if (args.isEmpty()) { ui->appendOutput("用法: curl <URL>\n", ERROR_COLOR); return; }
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
                else
                    ui->appendOutput(QString("请求失败: %1\n").arg(reply->errorString()), ERROR_COLOR);
                reply->deleteLater(); mgr->deleteLater();
            } else {
                ui->appendOutput("请提供有效 URL\n", ERROR_COLOR);
            }
        }
    }, "HTTP请求", "curl <URL>");

    registerCommand("wget", [ui](const QStringList& args) {
        if (args.isEmpty()) { ui->appendOutput("用法: wget <URL>\n", ERROR_COLOR); return; }
        QString url = args.last();
        if (!url.startsWith("http")) {
            ui->appendOutput("请提供有效 URL\n", ERROR_COLOR);
            return;
        }
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
            if (file.open(QIODevice::WriteOnly)) {
                file.write(reply->readAll());
                file.close();
                ui->appendOutput(QString("已下载: %1\n").arg(fn), INFO_COLOR);
            }
        } else {
            ui->appendOutput(QString("下载失败: %1\n").arg(reply->errorString()), ERROR_COLOR);
        }
        reply->deleteLater(); mgr->deleteLater();
    }, "下载文件", "wget <URL>");

    registerCommand("ifconfig", [ui](const QStringList&) {
        for (const QNetworkInterface& iface : QNetworkInterface::allInterfaces()) {
            ui->appendOutput(QString("%1: %2\n").arg(iface.humanReadableName(),
                iface.flags() & QNetworkInterface::IsUp ? "UP" : "DOWN"));
            for (const QNetworkAddressEntry& entry : iface.addressEntries())
                ui->appendOutput(QString("  inet %1\n").arg(entry.ip().toString()));
        }
    }, "网络接口", "ifconfig");

    registerCommand("ipconfig", [ui](const QStringList& args) {
        ui->executeInternalCommand("ifconfig " + args.join(' '));
    }, "IP配置", "ipconfig");

    registerCommand("netstat", [ui](const QStringList& args) {
        CommandHelper::runExternal("netstat", args.isEmpty() ? QStringList{"-an"} : args,
                                   ui->currentDir, ui);
    }, "网络连接", "netstat");

    registerCommand("nslookup", [ui](const QStringList& args) {
        if (args.isEmpty()) { ui->appendOutput("用法: nslookup <域名>\n", ERROR_COLOR); return; }
        QEventLoop loop;
        QHostInfo::lookupHost(args[0], [ui, &loop](const QHostInfo& info) {
            if (info.error() == QHostInfo::NoError)
                for (const QHostAddress& addr : info.addresses())
                    ui->appendOutput(addr.toString() + "\n");
            else ui->appendOutput("解析失败\n", ERROR_COLOR);
            loop.quit();
        });
        QTimer::singleShot(5000, &loop, &QEventLoop::quit);
        loop.exec();
    }, "DNS查询", "nslookup <域名>");

    registerCommand("tracert", [ui](const QStringList& args) {
        CommandHelper::runExternal("traceroute", args, ui->currentDir, ui, 60000, true);
    }, "路由追踪", "tracert");

    registerCommand("traceroute", [ui](const QStringList& args) {
        ui->executeInternalCommand("tracert " + args.join(' '));
    }, "路由追踪", "traceroute");

    registerCommand("route", [ui](const QStringList& args) {
        CommandHelper::runExternal("route", args, ui->currentDir, ui);
    }, "路由表", "route");

    registerCommand("arp", [ui](const QStringList& args) {
        CommandHelper::runExternal("arp", args.isEmpty() ? QStringList{"-a"} : args,
                                   ui->currentDir, ui);
    }, "ARP表", "arp");

    registerCommand("whois", [ui](const QStringList& args) {
        if (args.isEmpty()) { ui->appendOutput("用法: whois <域名>\n", ERROR_COLOR); return; }
        QTcpSocket* sock = new QTcpSocket();
        sock->connectToHost("whois.iana.org", 43);
        if (sock->waitForConnected(5000)) {
            sock->write(QString("%1\r\n").arg(args[0]).toUtf8());
            sock->waitForReadyRead(5000);
            ui->appendOutput(sock->readAll().left(2000));
        } else {
            ui->appendOutput("WHOIS连接失败\n", ERROR_COLOR);
        }
        sock->deleteLater();
    }, "WHOIS查询", "whois");
}