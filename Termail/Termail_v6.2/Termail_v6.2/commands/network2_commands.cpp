#include "commands.h"
#include "terminal/terminal.h"
#include "utils/utils.h"

#include <QTcpServer>
#include <QTcpSocket>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QHostAddress>
#include <QNetworkInterface>
#include <QUrl>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QEventLoop>
#include <QTimer>
#include <QThread>
#include <QDateTime>

// 全局 HTTP 服务器
static QTcpServer* g_httpServer = nullptr;

void CommandRegistry::registerNetwork2Commands(TerminalWidget* ui) {

    // serve - HTTP 文件服务器
    registerCommand("serve", [ui](const QStringList& args) {
        int port = 8080;
        if (!args.isEmpty()) {
            bool ok;
            int p = args[0].toInt(&ok);
            if (ok && p > 0 && p < 65536) port = p;
        }

        if (g_httpServer && g_httpServer->isListening()) {
            ui->appendOutput(QString("服务器已在端口 %1 运行\n")
                .arg(g_httpServer->serverPort()), ERROR_COLOR);
            ui->appendOutput("输入 stop-serve 关闭\n", INFO_COLOR);
            return;
        }

        g_httpServer = new QTcpServer();

        QObject::connect(g_httpServer, &QTcpServer::newConnection, [ui]() {
            QTcpSocket* client = g_httpServer->nextPendingConnection();
            QObject::connect(client, &QTcpSocket::readyRead, [client, ui]() {
                QString request = QString::fromUtf8(client->readAll());
                QStringList lines = request.split("\r\n");
                if (lines.isEmpty()) { client->close(); return; }

                QStringList parts = lines[0].split(' ');
                if (parts.size() < 2) { client->close(); return; }

                QString method = parts[0];
                QString path = parts[1];
                if (path == "/") path = "/index.html";

                ui->appendOutput(QString("[%1] %2 %3\n")
                    .arg(QDateTime::currentDateTime().toString("hh:mm:ss"), method, path), INFO_COLOR);

                QString filePath = QDir(ui->currentDir).absoluteFilePath(path.mid(1));
                QFileInfo info(filePath);

                QString response;
                if (info.exists() && info.isFile()) {
                    QFile file(filePath);
                    if (file.open(QIODevice::ReadOnly)) {
                        QByteArray content = file.readAll();
                        file.close();
                        response = "HTTP/1.1 200 OK\r\n";
                        response += "Content-Type: application/octet-stream\r\n";
                        response += QString("Content-Length: %1\r\n").arg(content.size());
                        response += "Connection: close\r\n\r\n";
                        client->write(response.toUtf8());
                        client->write(content);
                        client->flush();
                        client->waitForBytesWritten(3000);
                        client->close();
                        return;
                    }
                }

                // 404 或目录列表
                if (info.exists() && info.isDir()) {
                    QString html = "<html><body><h1>目录: " + path + "</h1><ul>";
                    QDir dir(filePath);
                    for (const QString& f : dir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot))
                        html += QString("<li><a href='%1'>%2</a></li>")
                            .arg(path + (path.endsWith('/') ? "" : "/") + f, f);
                    html += "</ul></body></html>";
                    QByteArray content = html.toUtf8();
                    response = "HTTP/1.1 200 OK\r\n";
                    response += "Content-Type: text/html; charset=utf-8\r\n";
                    response += QString("Content-Length: %1\r\n").arg(content.size());
                    response += "Connection: close\r\n\r\n";
                    client->write(response.toUtf8());
                    client->write(content);
                } else {
                    response = "HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n404";
                    client->write(response.toUtf8());
                }
                client->flush();
                client->waitForBytesWritten(3000);
                client->close();
            });
            QObject::connect(client, &QTcpSocket::disconnected, client, &QTcpSocket::deleteLater);
        });

        if (g_httpServer->listen(QHostAddress::Any, port)) {
            ui->appendOutput(QString("HTTP服务器已启动\n"), PROMPT_COLOR);
            ui->appendOutput(QString("端口: %1\n").arg(port), INFO_COLOR);
            for (const QNetworkInterface& iface : QNetworkInterface::allInterfaces()) {
                for (const QNetworkAddressEntry& entry : iface.addressEntries()) {
                    if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol &&
                        !entry.ip().isLoopback()) {
                        ui->appendOutput(QString("访问: http://%1:%2\n")
                            .arg(entry.ip().toString()).arg(port), ACCENT_COLOR);
                    }
                }
            }
            ui->appendOutput("输入 stop-serve 关闭\n", INFO_COLOR);
        } else {
            ui->appendOutput("服务器启动失败\n", ERROR_COLOR);
            delete g_httpServer;
            g_httpServer = nullptr;
        }
    }, "HTTP文件服务器", "serve [端口]");

    // stop-serve
    registerCommand("stop-serve", [ui](const QStringList&) {
        if (g_httpServer && g_httpServer->isListening()) {
            g_httpServer->close();
            delete g_httpServer;
            g_httpServer = nullptr;
            ui->appendOutput("服务器已关闭\n", INFO_COLOR);
        } else {
            ui->appendOutput("服务器未运行\n", ERROR_COLOR);
        }
    }, "关闭HTTP服务器", "stop-serve");

    // netscan - 局域网扫描
    registerCommand("netscan", [ui](const QStringList& args) {
        QString prefix = "192.168.1.";
        int start = 1, end = 254;
        if (!args.isEmpty()) {
            QString net = args[0];
            if (net.contains('/')) {
                prefix = net.left(net.lastIndexOf('.')) + ".";
            } else if (net.endsWith('.')) prefix = net;
            else if (net.contains('.')) {
                prefix = net.left(net.lastIndexOf('.')) + ".";
            }
        }
        if (args.size() >= 2) start = args[1].toInt();
        if (args.size() >= 3) end = args[2].toInt();

        if (end - start > 254) end = start + 254;

        ui->appendOutput(QString("扫描 %1%2-%3 ...\n").arg(prefix).arg(start).arg(end), ACCENT_COLOR);

        int found = 0;
        for (int i = start; i <= end; ++i) {
            QString ip = prefix + QString::number(i);
            QProcess ping;
            ping.start("ping", QStringList() << "-c" << "1" << "-W" << "1" << ip);
            ping.waitForFinished(1500);
            QString output = ping.readAllStandardOutput();
            if (output.contains("bytes from") || output.contains("ttl=")) {
                ui->appendOutput(QString("  %1 在线\n").arg(ip), PROMPT_COLOR);
                found++;
            }
        }
        ui->appendOutput(QString("扫描完成，发现 %1 个设备\n").arg(found), INFO_COLOR);
    }, "局域网扫描", "netscan [网段] [起始] [结束]");

    // mget - 多线程下载
    registerCommand("mget", [ui](const QStringList& args) {
        if (args.isEmpty()) {
            ui->appendOutput("用法: mget <URL>\n", ERROR_COLOR);
            return;
        }
        QString url = args[0];
        if (!url.startsWith("http")) {
            ui->appendOutput("URL 必须以 http 开头\n", ERROR_COLOR);
            return;
        }

        ui->appendOutput(QString("开始下载: %1\n").arg(url), ACCENT_COLOR);

        QNetworkAccessManager* mgr = new QNetworkAccessManager();
        QNetworkReply* reply = mgr->get(QNetworkRequest{QUrl(url)});

        QEventLoop loop;
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        QTimer::singleShot(0, &loop, &QEventLoop::quit); // 不阻塞，异步

        // 使用信号槽异步下载
        QObject::connect(reply, &QNetworkReply::finished, [reply, mgr, ui, url]() {
            if (reply->error() == QNetworkReply::NoError) {
                QString fn = url.split('/').last();
                if (fn.isEmpty()) fn = "download";
                QString path = QDir(ui->currentDir).absoluteFilePath(fn);
                QFile file(path);
                if (file.open(QIODevice::WriteOnly)) {
                    file.write(reply->readAll());
                    file.close();
                    ui->appendOutput(QString("下载完成: %1\n").arg(fn), PROMPT_COLOR);
                }
            } else {
                ui->appendOutput(QString("下载失败: %1\n").arg(reply->errorString()), ERROR_COLOR);
            }
            reply->deleteLater();
            mgr->deleteLater();
        });
    }, "多线程下载", "mget <URL>");
}