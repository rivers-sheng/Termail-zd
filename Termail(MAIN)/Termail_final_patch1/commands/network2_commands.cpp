#include "commands.h"
#include "terminal/terminal.h"
#include "utils/utils.h"

#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>
#include <QNetworkInterface>
#include <QUrl>
#include <QUrlQuery>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDateTime>
#include <QRegularExpression>

static QTcpServer* g_httpServer = nullptr;
static TerminalWidget* g_httpUi = nullptr;

// 生成首页 HTML
static QString generateIndexHtml(const QString& currentPath, const QString& dirPath) {
    QDir dir(dirPath);
    QStringList files = dir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot,
                                       QDir::Name | QDir::DirsFirst);

    QString html = R"(<!DOCTYPE html>
<html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Termail 文件管理</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{background:#111;color:#eee;font-family:-apple-system,'Segoe UI',sans-serif;padding:16px}
h1{color:#0cf;font-size:20px;margin-bottom:8px}
.path{color:#888;font-size:13px;margin-bottom:16px;word-break:break-all}
.toolbar{display:flex;gap:8px;margin-bottom:16px;flex-wrap:wrap}
.btn{background:#2a2a2a;color:#eee;border:1px solid #444;padding:8px 14px;border-radius:6px;cursor:pointer;font-size:14px}
.btn:hover{background:#333}
.btn-primary{background:#0cf;color:#000;border:none}
.btn-primary:hover{background:#0af}
.btn-danger{background:#d32f2f;color:#fff;border:none}
.list{list-style:none}
.item{display:flex;align-items:center;padding:12px;background:#1a1a1a;border-radius:8px;margin-bottom:6px}
.item:hover{background:#222}
.item a{color:#eee;text-decoration:none;flex:1;font-size:14px;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
.item a:hover{color:#0cf}
.icon{margin-right:10px;font-size:18px}
.size{color:#666;font-size:12px;margin-right:12px}
.del{background:#d32f2f;color:#fff;border:none;padding:4px 10px;border-radius:4px;cursor:pointer;font-size:12px;margin-left:8px}
.upload{background:#1a1a1a;padding:16px;border-radius:8px;margin-top:20px}
.upload h3{color:#0cf;margin-bottom:12px;font-size:15px}
input[type=file]{color:#eee;margin-bottom:8px;display:block}
input[type=text]{background:#222;color:#eee;border:1px solid #444;padding:8px;border-radius:4px;width:200px;margin-right:8px}
.empty{color:#666;text-align:center;padding:40px 0}
</style></head><body>
)";

    html += QString("<h1>Termail 文件管理</h1>\n");
    html += QString("<div class='path'>%1</div>\n").arg(currentPath.toHtmlEscaped());

    // 工具栏
    html += "<div class='toolbar'>\n";
    if (currentPath != "/") {
        QString parent = currentPath;
        if (parent.endsWith('/')) parent.chop(1);
        int idx = parent.lastIndexOf('/');
        parent = idx > 0 ? parent.left(idx + 1) : "/";
        html += QString("<a class='btn' href='%1'>← 返回上级</a>\n").arg(parent);
    }
    html += QString("<a class='btn' href='/'>⌂ 根目录</a>\n");
    html += "</div>\n";

    // 文件列表
    html += "<ul class='list'>\n";
    if (files.isEmpty()) {
        html += "<div class='empty'>空目录</div>\n";
    } else {
        for (const QString& name : files) {
            QFileInfo info(dir.absoluteFilePath(name));
            QString icon = info.isDir() ? "📁" : "📄";
            QString size = info.isDir() ? "" : Utils::formatSize(info.size());
            QString href;
            if (info.isDir()) {
                href = currentPath + (currentPath.endsWith('/') ? "" : "/") + name + "/";
            } else {
                href = "/download?path=" + QUrl::toPercentEncoding(
                    currentPath + (currentPath.endsWith('/') ? "" : "/") + name);
            }

            html += "<li class='item'>\n";
            html += QString("<span class='icon'>%1</span>\n").arg(icon);
            if (info.isDir()) {
                html += QString("<a href='%1'>%2/</a>\n").arg(href, name.toHtmlEscaped());
            } else {
                html += QString("<a href='%1' download>%2</a>\n").arg(href, name.toHtmlEscaped());
            }
            html += QString("<span class='size'>%1</span>\n").arg(size);
            html += QString("<button class='del' onclick=\"delFile('%1')\">删除</button>\n")
                .arg(name.toHtmlEscaped());
            html += "</li>\n";
        }
    }
    html += "</ul>\n";

    // 上传区
    html += R"(
<div class='upload'>
<h3>📤 上传文件</h3>
<form action='/upload' method='post' enctype='multipart/form-data'>
<input type='file' name='file' required>
<button type='submit' class='btn btn-primary'>上传</button>
</form>
</div>

<div class='upload'>
<h3>📁 新建文件夹</h3>
<form action='/mkdir' method='post'>
<input type='text' name='name' placeholder='文件夹名' required>
<button type='submit' class='btn btn-primary'>创建</button>
</form>
</div>

<script>
function delFile(name) {
    if (!confirm('确定删除 ' + name + ' 吗？')) return;
    fetch('/delete', {
        method: 'POST',
        headers: {'Content-Type': 'application/x-www-form-urlencoded'},
        body: 'path=' + encodeURIComponent(name)
    }).then(() => location.reload());
}
</script>
</body></html>
)";

    return html;
}

// HTTP 响应辅助
static void sendResponse(QTcpSocket* client, int code, const QString& contentType, const QByteArray& body) {
    QString status = code == 200 ? "200 OK" :
                     code == 404 ? "404 Not Found" :
                     code == 400 ? "400 Bad Request" : "500 Error";
    QByteArray resp = "HTTP/1.1 " + status.toUtf8() + "\r\n";
    resp += "Content-Type: " + contentType.toUtf8() + "\r\n";
    resp += "Content-Length: " + QByteArray::number(body.size()) + "\r\n";
    resp += "Connection: close\r\n\r\n";
    resp += body;
    client->write(resp);
    client->flush();
    client->waitForBytesWritten(3000);
    client->close();
}

// 解析请求路径和参数
static void handleRequest(QTcpSocket* client, const QString& request, TerminalWidget* ui) {
    QStringList lines = request.split("\r\n");
    if (lines.isEmpty()) return;

    QStringList reqLine = lines[0].split(' ');
    if (reqLine.size() < 2) return;

    QString method = reqLine[0];
    QString path = reqLine[1];

    // 解析路径参数
    QString urlPath = path;
    QMap<QString, QString> params;
    if (path.contains('?')) {
        int idx = path.indexOf('?');
        urlPath = path.left(idx);
        QString query = path.mid(idx + 1);
        for (const QString& pair : query.split('&')) {
            QStringList kv = pair.split('=');
            if (kv.size() == 2) {
                params[QUrl::fromPercentEncoding(kv[0].toUtf8())] =
                    QUrl::fromPercentEncoding(kv[1].toUtf8());
            }
        }
    }

    // 当前目录
    QString baseDir = ui->currentDir;
    QString currentPath = "/";

    ui->appendOutput(QString("[%1] %2 %3\n")
        .arg(QDateTime::currentDateTime().toString("hh:mm:ss"), method, path), INFO_COLOR);

    // GET /
    if (method == "GET" && urlPath == "/") {
        QString html = generateIndexHtml("/", baseDir);
        sendResponse(client, 200, "text/html; charset=utf-8", html.toUtf8());
        return;
    }

    // GET /xxx/ 目录
    if (method == "GET" && urlPath.endsWith("/")) {
        QString subPath = urlPath;
        QString fullDir = QDir(baseDir).absoluteFilePath(subPath.mid(1));
        if (QDir(fullDir).exists()) {
            QString html = generateIndexHtml(subPath, fullDir);
            sendResponse(client, 200, "text/html; charset=utf-8", html.toUtf8());
        } else {
            sendResponse(client, 404, "text/plain", "Not Found");
        }
        return;
    }

    // GET /download?path=xxx
    if (method == "GET" && urlPath == "/download") {
        QString filePath = params.value("path");
        QString fullPath = QDir(baseDir).absoluteFilePath(filePath.mid(1));
        QFileInfo info(fullPath);
        if (info.exists() && info.isFile()) {
            QFile f(fullPath);
            if (f.open(QIODevice::ReadOnly)) {
                QByteArray data = f.readAll();
                f.close();
                sendResponse(client, 200, "application/octet-stream", data);
            } else {
                sendResponse(client, 500, "text/plain", "Read Error");
            }
        } else {
            sendResponse(client, 404, "text/plain", "Not Found");
        }
        return;
    }

    // POST /delete
    if (method == "POST" && urlPath == "/delete") {
        // 从 body 里读 path
        int bodyIdx = request.indexOf("\r\n\r\n");
        if (bodyIdx > 0) {
            QString body = request.mid(bodyIdx + 4);
            for (const QString& pair : body.split('&')) {
                QStringList kv = pair.split('=');
                if (kv.size() == 2 && kv[0] == "path") {
                    QString name = QUrl::fromPercentEncoding(kv[1].toUtf8());
                    QString fullPath = QDir(baseDir).absoluteFilePath(name);
                    QFileInfo info(fullPath);
                    if (info.isDir()) QDir(fullPath).removeRecursively();
                    else QFile::remove(fullPath);
                    ui->appendOutput(QString("[HTTP] 删除: %1\n").arg(name), ERROR_COLOR);
                }
            }
        }
        sendResponse(client, 200, "text/plain", "OK");
        return;
    }

    // POST /mkdir
    if (method == "POST" && urlPath == "/mkdir") {
        int bodyIdx = request.indexOf("\r\n\r\n");
        if (bodyIdx > 0) {
            QString body = request.mid(bodyIdx + 4);
            for (const QString& pair : body.split('&')) {
                QStringList kv = pair.split('=');
                if (kv.size() == 2 && kv[0] == "name") {
                    QString name = QUrl::fromPercentEncoding(kv[1].toUtf8());
                    QDir(baseDir).mkpath(name);
                    ui->appendOutput(QString("[HTTP] 新建: %1\n").arg(name), PROMPT_COLOR);
                }
            }
        }
        // 重定向回首页
        QByteArray resp = "HTTP/1.1 302 Found\r\nLocation: /\r\nConnection: close\r\n\r\n";
        client->write(resp);
        client->flush();
        client->waitForBytesWritten(3000);
        client->close();
        return;
    }

    // POST /upload
    if (method == "POST" && urlPath == "/upload") {
        int headerEnd = request.indexOf("\r\n\r\n");
        if (headerEnd > 0) {
            QString headers = request.left(headerEnd);
            QByteArray body = request.mid(headerEnd + 4).toUtf8();

            // 解析 boundary
            QRegularExpression bRe("boundary=([^;\\r\\n]+)");
            QRegularExpressionMatch bMatch = bRe.match(headers);
            if (bMatch.hasMatch()) {
                QString boundary = "--" + bMatch.captured(1);
                QByteArray bBytes = boundary.toUtf8();

                int start = body.indexOf(bBytes);
                if (start >= 0) {
                    int hEnd = body.indexOf("\r\n\r\n", start);
                    if (hEnd > 0) {
                        QByteArray fileHeader = body.mid(start, hEnd - start);
                        // 提取文件名
                        QRegularExpression fRe("filename=\"([^\"]+)\"");
                        QRegularExpressionMatch fMatch = fRe.match(QString::fromUtf8(fileHeader));
                        if (fMatch.hasMatch()) {
                            QString filename = fMatch.captured(1);
                            int dataStart = hEnd + 4;
                            int dataEnd = body.indexOf("\r\n" + bBytes, dataStart);
                            if (dataEnd > 0) {
                                QByteArray fileData = body.mid(dataStart, dataEnd - dataStart);
                                QString savePath = QDir(baseDir).absoluteFilePath(filename);
                                QFile f(savePath);
                                if (f.open(QIODevice::WriteOnly)) {
                                    f.write(fileData);
                                    f.close();
                                    ui->appendOutput(QString("[HTTP] 上传: %1 (%2 KB)\n")
                                        .arg(filename).arg(fileData.size() / 1024), PROMPT_COLOR);
                                }
                            }
                        }
                    }
                }
            }
        }
        QByteArray resp = "HTTP/1.1 302 Found\r\nLocation: /\r\nConnection: close\r\n\r\n";
        client->write(resp);
        client->flush();
        client->waitForBytesWritten(3000);
        client->close();
        return;
    }

    // 404
    sendResponse(client, 404, "text/plain", "Not Found");
}

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
        g_httpUi = ui;

        QObject::connect(g_httpServer, &QTcpServer::newConnection, [ui]() {
            while (g_httpServer->hasPendingConnections()) {
                QTcpSocket* client = g_httpServer->nextPendingConnection();
                QObject::connect(client, &QTcpSocket::readyRead, [client, ui]() {
                    client->setProperty("data", client->property("data").toByteArray() + client->readAll());
                    QByteArray data = client->property("data").toByteArray();

                    // 等待完整请求
                    int headerEnd = data.indexOf("\r\n\r\n");
                    if (headerEnd < 0) return;

                    // 检查 Content-Length
                    QString headers = QString::fromUtf8(data.left(headerEnd));
                    int contentLength = 0;
                    QRegularExpression clRe("Content-Length:\\s*(\\d+)", QRegularExpression::CaseInsensitiveOption);
                    QRegularExpressionMatch clMatch = clRe.match(headers);
                    if (clMatch.hasMatch()) {
                        contentLength = clMatch.captured(1).toInt();
                        if (data.size() < headerEnd + 4 + contentLength) return;
                    }

                    QString request = QString::fromUtf8(data);
                    client->setProperty("data", QByteArray());
                    handleRequest(client, request, ui);
                });
                QObject::connect(client, &QTcpSocket::disconnected, client, &QTcpSocket::deleteLater);
            }
        });

        if (g_httpServer->listen(QHostAddress::Any, port)) {
            ui->appendOutput("HTTP服务器已启动\n", PROMPT_COLOR);
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
            ui->appendOutput("功能: 浏览 / 下载 / 上传 / 新建文件夹 / 删除\n", INFO_COLOR);
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
}
namespace Network2Cleanup {
    void cleanup() {
        if (g_httpServer && g_httpServer->isListening()) {
            g_httpServer->close();
            delete g_httpServer;
            g_httpServer = nullptr;
        }
    }
}
