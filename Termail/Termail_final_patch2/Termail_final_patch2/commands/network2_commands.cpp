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
#include <QJsonObject>
#include <QJsonDocument>
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QMap>
#include <QSet>

static QTcpServer* g_httpServer = nullptr;
static TerminalWidget* g_httpUi = nullptr;

// ============ 用户与会话 ============

// 用户结构：全局 r/w/x，root 额外标记
struct HttpUser {
    QString salt;
    QString hash;
    bool isRoot = false;
    bool r = false;
    bool w = false;
    bool x = false;
};

// token -> 用户名
static QMap<QString, QString> g_sessions;

static QString usersFilePath() {
    return Utils::configDir() + "/http_users.json";
}

// SHA-256(salt + password)
static QString hashPassword(const QString& salt, const QString& password) {
    QByteArray data = (salt + password).toUtf8();
    return QString::fromLatin1(QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex());
}

static QString randomSalt() {
    QByteArray buf(16, 0);
    for (int i = 0; i < buf.size(); ++i)
        buf[i] = char(QRandomGenerator::global()->bounded(256));
    return QString::fromLatin1(buf.toHex());
}

static QString randomToken() {
    QByteArray buf(24, 0);
    for (int i = 0; i < buf.size(); ++i)
        buf[i] = char(QRandomGenerator::global()->bounded(256));
    return QString::fromLatin1(buf.toHex());
}

// 加载用户；不存在则初始化 root/2510
static QMap<QString, HttpUser> loadUsers() {
    QMap<QString, HttpUser> users;
    QFile f(usersFilePath());
    if (f.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
        f.close();
        QJsonObject obj = doc.object();
        for (auto it = obj.begin(); it != obj.end(); ++it) {
            QJsonObject u = it.value().toObject();
            HttpUser user;
            user.salt = u["salt"].toString();
            user.hash = u["hash"].toString();
            user.isRoot = u["isRoot"].toBool(false);
            user.r = u["r"].toBool(false);
            user.w = u["w"].toBool(false);
            user.x = u["x"].toBool(false);
            users[it.key()] = user;
        }
    }
    // 首次初始化 root
    if (!users.contains("root")) {
        HttpUser root;
        root.salt = randomSalt();
        root.hash = hashPassword(root.salt, "2510");
        root.isRoot = true;
        root.r = root.w = root.x = true;
        users["root"] = root;
        // 写盘
        QJsonObject obj;
        for (auto it = users.begin(); it != users.end(); ++it) {
            QJsonObject u;
            u["salt"] = it.value().salt;
            u["hash"] = it.value().hash;
            u["isRoot"] = it.value().isRoot;
            u["r"] = it.value().r;
            u["w"] = it.value().w;
            u["x"] = it.value().x;
            obj[it.key()] = u;
        }
        QDir().mkpath(QFileInfo(usersFilePath()).absolutePath());
        QFile out(usersFilePath());
        if (out.open(QIODevice::WriteOnly)) {
            out.write(QJsonDocument(obj).toJson());
            out.close();
        }
    }
    return users;
}

static void saveUsers(const QMap<QString, HttpUser>& users) {
    QJsonObject obj;
    for (auto it = users.begin(); it != users.end(); ++it) {
        QJsonObject u;
        u["salt"] = it.value().salt;
        u["hash"] = it.value().hash;
        u["isRoot"] = it.value().isRoot;
        u["r"] = it.value().r;
        u["w"] = it.value().w;
        u["x"] = it.value().x;
        obj[it.key()] = u;
    }
    QDir().mkpath(QFileInfo(usersFilePath()).absolutePath());
    QFile out(usersFilePath());
    if (out.open(QIODevice::WriteOnly)) {
        out.write(QJsonDocument(obj).toJson());
        out.close();
    }
}

// 从 cookie 头取 sid
static QString extractSid(const QString& headers) {
    QRegularExpression re("Cookie:\\s*([^\\r\\n]+)", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch m = re.match(headers);
    if (!m.hasMatch()) return "";
    QString cookie = m.captured(1);
    for (const QString& part : cookie.split(';')) {
        QString p = part.trimmed();
        if (p.startsWith("sid=")) return p.mid(4);
    }
    return "";
}

// 判断当前请求用户是否有效
static QString currentUser(const QString& headers) {
    QString sid = extractSid(headers);
    if (sid.isEmpty()) return "";
    return g_sessions.value(sid, "");
}

// ============ HTML 辅助 ============

static QString htmlEscape(const QString& s) {
    return s.toHtmlEscaped();
}

// 统一页面外壳
static QString pageShell(const QString& title, const QString& user,
                         const QString& body, bool isRoot, bool canW, bool canR) {
    QString html = R"(<!DOCTYPE html>
<html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>)" + htmlEscape(title) + R"(</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{background:#0d0d0d;color:#e0e0e0;font-family:-apple-system,'Segoe UI',Roboto,sans-serif;padding:16px;line-height:1.5}
a{color:#4fc3f7;text-decoration:none}
a:hover{text-decoration:underline}
h1{color:#4fc3f7;font-size:20px;margin-bottom:6px}
.topbar{display:flex;align-items:center;justify-content:space-between;gap:12px;flex-wrap:wrap;margin-bottom:16px;padding-bottom:12px;border-bottom:1px solid #222}
.brand{font-size:18px;color:#4fc3f7;font-weight:600}
.userbox{font-size:13px;color:#888}
.userbox b{color:#e0e0e0}
.crumb{color:#888;font-size:13px;margin-bottom:14px;word-break:break-all}
.crumb a{color:#4fc3f7}
.toolbar{display:flex;gap:8px;margin-bottom:14px;flex-wrap:wrap}
.btn{background:#222;color:#e0e0e0;border:1px solid #333;padding:8px 14px;border-radius:6px;cursor:pointer;font-size:13px;display:inline-block}
.btn:hover{background:#2e2e2e}
.btn-primary{background:#4fc3f7;color:#000;border:none;font-weight:600}
.btn-primary:hover{background:#29b6f6}
.btn-danger{background:#d32f2f;color:#fff;border:none}
.btn-danger:hover{background:#b71c1c}
.btn-sm{padding:4px 10px;font-size:12px}
table{width:100%;border-collapse:collapse;font-size:13px}
th,td{padding:10px 8px;text-align:left;border-bottom:1px solid #1e1e1e}
th{color:#888;font-weight:500;font-size:12px;text-transform:uppercase;letter-spacing:.5px}
tr:hover td{background:#161616}
.icon{margin-right:8px;font-size:16px}
.muted{color:#666;font-size:12px}
.empty{color:#555;text-align:center;padding:40px 0;font-size:14px}
.card{background:#141414;border:1px solid #222;border-radius:10px;padding:18px;margin-top:18px}
.card h3{color:#4fc3f7;font-size:15px;margin-bottom:14px}
input[type=text],input[type=password],input[type=file],textarea{background:#1a1a1a;color:#e0e0e0;border:1px solid #333;padding:9px 12px;border-radius:6px;font-size:14px;width:100%;max-width:320px;font-family:inherit}
input[type=text]:focus,input[type=password]:focus,textarea:focus{outline:none;border-color:#4fc3f7}
textarea{width:100%;max-width:100%;min-height:340px;font-family:monospace;font-size:13px;line-height:1.4;resize:vertical}
label{display:block;color:#888;font-size:12px;margin-bottom:6px;margin-top:12px}
.form-row{display:flex;gap:8px;align-items:flex-end;flex-wrap:wrap}
.msg{padding:10px 14px;border-radius:6px;margin-bottom:14px;font-size:13px}
.msg-err{background:#3a1414;color:#ef9a9a;border:1px solid #5c1e1e}
.msg-ok{background:#12301c;color:#a5d6a7;border:1px solid #1e4d2e}
.badge{display:inline-block;padding:2px 8px;border-radius:10px;font-size:11px;font-weight:600}
.badge-root{background:#4fc3f7;color:#000}
.badge-user{background:#333;color:#aaa}
.badge-r{background:#1b5e20;color:#a5d6a7}
.badge-w{background:#e65100;color:#ffcc80}
.badge-x{background:#4a148c;color:#ce93d8}
.searchbox{display:flex;gap:8px;margin-bottom:14px;flex-wrap:wrap}
.searchbox input{max-width:280px}
.checkbox{width:16px;height:16px;cursor:pointer}
</style></head><body>
<div class='topbar'>
<div class='brand'>Termail 文件管理</div>
<div class='userbox'>)";

    if (!user.isEmpty()) {
        html += QString("当前: <b>%1</b> ").arg(htmlEscape(user));
        if (isRoot) html += "<span class='badge badge-root'>ROOT</span>";
        else html += "<span class='badge badge-user'>USER</span>";
        if (isRoot) html += " <a href='/admin/users' style='margin-left:10px'>用户管理</a>";
        html += " <a href='/logout' style='margin-left:10px'>退出</a>";
    }

    html += "</div></div>\n";
    html += body;
    html += "\n</body></html>\n";
    return html;
}

static QString loginPage(const QString& error = "") {
    QString html = R"(<!DOCTYPE html>
<html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>登录 - Termail</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{background:#0d0d0d;color:#e0e0e0;font-family:-apple-system,'Segoe UI',sans-serif;display:flex;align-items:center;justify-content:center;min-height:100vh;padding:16px}
.card{background:#141414;border:1px solid #222;border-radius:12px;padding:32px;width:100%;max-width:360px}
h1{color:#4fc3f7;font-size:22px;margin-bottom:6px;text-align:center}
.sub{color:#666;font-size:13px;text-align:center;margin-bottom:24px}
label{display:block;color:#888;font-size:12px;margin-bottom:6px;margin-top:14px}
input{background:#1a1a1a;color:#e0e0e0;border:1px solid #333;padding:11px 14px;border-radius:6px;font-size:14px;width:100%}
input:focus{outline:none;border-color:#4fc3f7}
button{background:#4fc3f7;color:#000;border:none;padding:11px;border-radius:6px;font-size:14px;font-weight:600;width:100%;margin-top:20px;cursor:pointer}
button:hover{background:#29b6f6}
.msg-err{background:#3a1414;color:#ef9a9a;border:1px solid #5c1e1e;padding:10px 14px;border-radius:6px;margin-bottom:14px;font-size:13px}
</style></head><body>
<div class='card'>
<h1>Termail</h1>
<div class='sub'>文件管理服务</div>
)";
    if (!error.isEmpty())
        html += QString("<div class='msg-err'>%1</div>\n").arg(htmlEscape(error));
    html += R"(
<form method='post' action='/login'>
<label>用户名</label>
<input type='text' name='username' required autofocus>
<label>密码</label>
<input type='password' name='password' required>
<button type='submit'>登录</button>
</form>
</div>
</body></html>
)";
    return html;
}

// 主页：目录列表 + 搜索 + 批量删除
static QString generateIndexHtml(const QString& user, const QString& currentPath,
                                 const QString& dirPath, bool isRoot, bool canR, bool canW,
                                 const QString& search) {
    QDir dir(dirPath);
    QStringList files;
    if (search.isEmpty()) {
        files = dir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot,
                              QDir::Name | QDir::DirsFirst);
    } else {
        files = dir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot,
                              QDir::Name | QDir::DirsFirst);
        QStringList filtered;
        for (const QString& f : files)
            if (f.contains(search, Qt::CaseInsensitive)) filtered.append(f);
        files = filtered;
    }

    QString body;
    body += QString("<div class='crumb'>%1</div>\n").arg(
        currentPath.isEmpty() ? "/" : htmlEscape(currentPath));

    // 工具栏
    body += "<div class='toolbar'>\n";
    if (currentPath != "/" && !currentPath.isEmpty()) {
        QString parent = currentPath;
        if (parent.endsWith('/')) parent.chop(1);
        int idx = parent.lastIndexOf('/');
        parent = idx > 0 ? parent.left(idx + 1) : "/";
        body += QString("<a class='btn' href='%1'>← 返回上级</a>\n").arg(parent);
    }
    body += "<a class='btn' href='/'>⌂ 根目录</a>\n";
    if (canW) {
        body += "<button class='btn btn-danger btn-sm' onclick=\"batchDelete()\">删除选中</button>\n";
    }
    body += "</div>\n";

    // 搜索框
    body += "<form class='searchbox' method='get' action=''>\n";
    body += QString("<input type='text' name='q' placeholder='搜索当前目录...' value='%1'>\n")
        .arg(htmlEscape(search));
    body += "<button type='submit' class='btn'>搜索</button>\n";
    if (!search.isEmpty())
        body += "<a class='btn' href='" + (currentPath.isEmpty() ? "/" : currentPath) + "'>清除</a>\n";
    body += "</form>\n";

    // 文件表格
    body += "<table>\n<thead><tr>\n";
    if (canW) body += "<th style='width:30px'><input type='checkbox' class='checkbox' id='chkAll' onclick=\"toggleAll(this)\"></th>\n";
    body += "<th>名称</th><th style='width:80px'>大小</th><th style='width:150px'>修改时间</th><th style='width:150px'>操作</th>\n";
    body += "</tr></thead>\n<tbody>\n";

    if (files.isEmpty()) {
        body += QString("<tr><td colspan='5' class='empty'>%1</td></tr>\n")
            .arg(search.isEmpty() ? "空目录" : "无匹配文件");
    } else {
        for (const QString& name : files) {
            QFileInfo info(dir.absoluteFilePath(name));
            QString icon = info.isDir() ? "📁" : "📄";
            QString size = info.isDir() ? "-" : Utils::formatSize(info.size());
            QString mtime = info.lastModified().toString("yyyy-MM-dd hh:mm");
            QString href;
            if (info.isDir()) {
                href = currentPath + (currentPath.endsWith('/') ? "" : "/") + name + "/";
            } else {
                href = "/download?path=" + QUrl::toPercentEncoding(
                    currentPath + (currentPath.endsWith('/') ? "" : "/") + name);
            }

            body += "<tr>\n";
            if (canW)
                body += QString("<td><input type='checkbox' class='checkbox rowchk' value='%1'></td>\n")
                    .arg(htmlEscape(name));
            body += QString("<td><span class='icon'>%1</span>").arg(icon);
            if (info.isDir() || canR) {
                body += QString("<a href='%1'>%2%3</a>").arg(href, htmlEscape(name),
                    info.isDir() ? "/" : "");
            } else {
                body += htmlEscape(name);
            }
            body += "</td>\n";
            body += QString("<td class='muted'>%1</td>\n").arg(size);
            body += QString("<td class='muted'>%1</td>\n").arg(mtime);
            body += "<td>";
            if (!info.isDir() && canR) {
                body += QString("<a class='btn btn-sm' href='/view?path=%1'>预览</a> ")
                    .arg(QUrl::toPercentEncoding(
                        currentPath + (currentPath.endsWith('/') ? "" : "/") + name));
                body += QString("<a class='btn btn-sm' href='%1' download>下载</a> ")
                    .arg(href);
            }
            if (canW)
                body += QString("<button class='btn btn-danger btn-sm' onclick=\"delFile('%1')\">删除</button>")
                    .arg(htmlEscape(name).replace("'", "\\'"));
            body += "</td>\n</tr>\n";
        }
    }
    body += "</tbody></table>\n";

    // 上传 + 新建文件夹
    if (canW) {
        body += R"(
<div class='card'>
<h3>📤 上传文件</h3>
<form action='/upload' method='post' enctype='multipart/form-data'>
<input type='file' name='file' required style='margin-bottom:12px'>
<button type='submit' class='btn btn-primary'>上传</button>
</form>
</div>

<div class='card'>
<h3>📁 新建文件夹</h3>
<form action='/mkdir' method='post' class='form-row'>
<input type='text' name='name' placeholder='文件夹名' required>
<button type='submit' class='btn btn-primary'>创建</button>
</form>
</div>

<script>
function toggleAll(cb){document.querySelectorAll('.rowchk').forEach(c=>c.checked=cb.checked);}
function delFile(name){
  if(!confirm('确定删除 '+name+' 吗？'))return;
  fetch('/delete',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'path='+encodeURIComponent(name)}).then(()=>location.reload());
}
function batchDelete(){
  const names=Array.from(document.querySelectorAll('.rowchk:checked')).map(c=>c.value);
  if(names.length===0){alert('请先勾选文件');return;}
  if(!confirm('确定删除选中的 '+names.length+' 项吗？'))return;
  fetch('/batch-delete',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'paths='+encodeURIComponent(names.join('\n'))}).then(()=>location.reload());
}
</script>
)";
    }

    return pageShell("文件管理", user, body, isRoot, canW, canR);
}

// 预览/编辑页
static QString viewPage(const QString& user, const QString& relPath, const QString& content,
                        bool isRoot, bool canW, const QString& msg = "") {
    QString body;
    body += QString("<div class='crumb'><a href='/'>/</a> %1</div>\n").arg(htmlEscape(relPath));
    if (!msg.isEmpty())
        body += QString("<div class='msg msg-ok'>%1</div>\n").arg(htmlEscape(msg));
    body += "<form method='post' action='/save'>\n";
    body += QString("<input type='hidden' name='path' value='%1'>\n").arg(htmlEscape(relPath));
    body += QString("<textarea name='content' %1>%2</textarea>\n")
        .arg(canW ? "" : "readonly", htmlEscape(content));
    body += "<div class='toolbar' style='margin-top:12px'>\n";
    if (canW) body += "<button type='submit' class='btn btn-primary'>保存</button>\n";
    body += "<a class='btn' href='/'>返回</a>\n";
    body += "</div></form>\n";
    return pageShell("预览 - " + relPath, user, body, isRoot, canW, true);
}

// 用户管理页（仅 root）
static QString adminPage(const QString& user, const QMap<QString, HttpUser>& users,
                         const QString& msg = "", bool isErr = false) {
    QString body;
    if (!msg.isEmpty())
        body += QString("<div class='msg %1'>%2</div>\n")
            .arg(isErr ? "msg-err" : "msg-ok", htmlEscape(msg));

    body += R"(
<div class='card'>
<h3>➕ 创建账户</h3>
<form method='post' action='/admin/create' class='form-row'>
<input type='text' name='username' placeholder='用户名' required>
<input type='password' name='password' placeholder='密码' required>
<label style='display:flex;align-items:center;gap:4px;margin:0'><input type='checkbox' name='r' checked style='width:auto'> 读</label>
<label style='display:flex;align-items:center;gap:4px;margin:0'><input type='checkbox' name='w' style='width:auto'> 写</label>
<label style='display:flex;align-items:center;gap:4px;margin:0'><input type='checkbox' name='x' style='width:auto'> 运行</label>
<input type='password' name='root_pw' placeholder='root 密码' required style='max-width:160px'>
<button type='submit' class='btn btn-primary'>创建</button>
</form>
</div>

<div class='card'>
<h3>👥 用户列表</h3>
<table>
<thead><tr><th>用户名</th><th>类型</th><th>权限</th><th>操作</th></tr></thead>
<tbody>
)";

    for (auto it = users.begin(); it != users.end(); ++it) {
        const QString& name = it.key();
        const HttpUser& u = it.value();
        body += "<tr>\n";
        body += QString("<td><b>%1</b></td>\n").arg(htmlEscape(name));
        body += QString("<td>%1</td>\n").arg(u.isRoot
            ? "<span class='badge badge-root'>ROOT</span>"
            : "<span class='badge badge-user'>USER</span>");
        body += "<td>";
        if (u.isRoot) body += "<span class='muted'>全部</span>";
        else {
            if (u.r) body += "<span class='badge badge-r'>读</span> ";
            if (u.w) body += "<span class='badge badge-w'>写</span> ";
            if (u.x) body += "<span class='badge badge-x'>运行</span>";
            if (!u.r && !u.w && !u.x) body += "<span class='muted'>无</span>";
        }
        body += "</td>\n<td>";

        if (name == "root") {
            body += "<span class='muted'>—</span>";
        } else {
            // 改权限
            body += QString("<form method='post' action='/admin/grant' style='display:inline'>"
                "<input type='hidden' name='username' value='%1'>"
                "<label style='display:inline;margin:0'><input type='checkbox' name='r' %2 style='width:auto'> R</label> "
                "<label style='display:inline;margin:0'><input type='checkbox' name='w' %3 style='width:auto'> W</label> "
                "<label style='display:inline;margin:0'><input type='checkbox' name='x' %4 style='width:auto'> X</label> "
                "<input type='password' name='root_pw' placeholder='root密码' required style='width:100px;padding:4px;margin-left:4px'>"
                "<button type='submit' class='btn btn-sm btn-primary'>改权限</button></form> ")
                .arg(htmlEscape(name), u.r ? "checked" : "", u.w ? "checked" : "", u.x ? "checked" : "");

            // 改密码
            body += QString("<form method='post' action='/admin/passwd' style='display:inline'>"
                "<input type='hidden' name='username' value='%1'>"
                "<input type='password' name='password' placeholder='新密码' required style='width:100px;padding:4px'>"
                "<input type='password' name='root_pw' placeholder='root密码' required style='width:100px;padding:4px'>"
                "<button type='submit' class='btn btn-sm'>改密码</button></form> ")
                .arg(htmlEscape(name));

            // 授予/撤销 root
            if (u.isRoot) {
                body += QString("<form method='post' action='/admin/revoke' style='display:inline'>"
                    "<input type='hidden' name='username' value='%1'>"
                    "<input type='password' name='root_pw' placeholder='root密码' required style='width:100px;padding:4px'>"
                    "<button type='submit' class='btn btn-sm btn-danger'>撤销ROOT</button></form> ")
                    .arg(htmlEscape(name));
            } else {
                body += QString("<form method='post' action='/admin/promote' style='display:inline'>"
                    "<input type='hidden' name='username' value='%1'>"
                    "<input type='password' name='root_pw' placeholder='root密码' required style='width:100px;padding:4px'>"
                    "<button type='submit' class='btn btn-sm btn-primary'>授予ROOT</button></form> ")
                    .arg(htmlEscape(name));
            }

            // 删除账户
            body += QString("<form method='post' action='/admin/delete' style='display:inline'>"
                "<input type='hidden' name='username' value='%1'>"
                "<input type='password' name='root_pw' placeholder='root密码' required style='width:100px;padding:4px'>"
                "<button type='submit' class='btn btn-sm btn-danger' onclick=\"return confirm('确定删除此账户？')\">删除</button></form>")
                .arg(htmlEscape(name));
        }
        body += "</td>\n</tr>\n";
    }

    body += "</tbody></table></div>\n";
    return pageShell("用户管理", user, body, true, true, true);
}

// ============ HTTP 响应 ============

static void sendResponse(QTcpSocket* client, int code, const QString& contentType,
                         const QByteArray& body, const QString& extraHeaders = "") {
    QString status = code == 200 ? "200 OK" :
                     code == 302 ? "302 Found" :
                     code == 401 ? "401 Unauthorized" :
                     code == 403 ? "403 Forbidden" :
                     code == 404 ? "404 Not Found" :
                     code == 400 ? "400 Bad Request" : "500 Error";
    QByteArray resp = "HTTP/1.1 " + status.toUtf8() + "\r\n";
    resp += "Content-Type: " + contentType.toUtf8() + "\r\n";
    resp += "Content-Length: " + QByteArray::number(body.size()) + "\r\n";
    if (!extraHeaders.isEmpty()) resp += extraHeaders.toUtf8() + "\r\n";
    resp += "Connection: close\r\n\r\n";
    resp += body;
    client->write(resp);
    client->flush();
    client->waitForBytesWritten(3000);
    client->close();
}

static void redirect(QTcpSocket* client, const QString& location, const QString& extraHeaders = "") {
    QByteArray resp = "HTTP/1.1 302 Found\r\n";
    resp += "Location: " + location.toUtf8() + "\r\n";
    if (!extraHeaders.isEmpty()) resp += extraHeaders.toUtf8() + "\r\n";
    resp += "Content-Length: 0\r\n";
    resp += "Connection: close\r\n\r\n";
    client->write(resp);
    client->flush();
    client->waitForBytesWritten(3000);
    client->close();
}

// 解析表单 body
static QMap<QString, QString> parseFormBody(const QString& body) {
    QMap<QString, QString> params;
    for (const QString& pair : body.split('&')) {
        int eq = pair.indexOf('=');
        if (eq < 0) continue;
        QString k = QUrl::fromPercentEncoding(pair.left(eq).toUtf8());
        QString v = QUrl::fromPercentEncoding(pair.mid(eq + 1).toUtf8());
        params[k] = v;
    }
    return params;
}

// ============ 请求处理 ============

static void handleRequest(QTcpSocket* client, const QString& request, TerminalWidget* ui) {
    QStringList lines = request.split("\r\n");
    if (lines.isEmpty()) return;
    QStringList reqLine = lines[0].split(' ');
    if (reqLine.size() < 2) return;

    QString method = reqLine[0];
    QString rawPath = reqLine[1];

    int headerEnd = request.indexOf("\r\n\r\n");
    QString headers = headerEnd > 0 ? request.left(headerEnd) : request;
    QString body = headerEnd > 0 ? request.mid(headerEnd + 4) : "";

    // 解析路径与 query
    QString urlPath = rawPath;
    QMap<QString, QString> query;
    if (rawPath.contains('?')) {
        int idx = rawPath.indexOf('?');
        urlPath = rawPath.left(idx);
        query = parseFormBody(rawPath.mid(idx + 1));
    }

    QMap<QString, HttpUser> users = loadUsers();
    QString username = currentUser(headers);

    ui->appendOutput(QString("[%1] %2 %3 (user=%4)\n")
        .arg(QDateTime::currentDateTime().toString("hh:mm:ss"),
             method, rawPath, username.isEmpty() ? "-" : username), INFO_COLOR);

    // 登录相关不需要 session
    if (urlPath == "/login" && method == "GET") {
        if (!username.isEmpty()) { redirect(client, "/"); return; }
        sendResponse(client, 200, "text/html; charset=utf-8", loginPage().toUtf8());
        return;
    }
    if (urlPath == "/login" && method == "POST") {
        QMap<QString, QString> form = parseFormBody(body);
        QString u = form.value("username");
        QString p = form.value("password");
        if (users.contains(u)) {
            const HttpUser& user = users[u];
            if (hashPassword(user.salt, p) == user.hash) {
                QString token = randomToken();
                g_sessions[token] = u;
                QString cookie = "Set-Cookie: sid=" + token + "; Path=/; HttpOnly";
                redirect(client, "/", cookie);
                return;
            }
        }
        sendResponse(client, 200, "text/html; charset=utf-8",
                     loginPage("用户名或密码错误").toUtf8());
        return;
    }
    if (urlPath == "/logout") {
        QString sid = extractSid(headers);
        if (!sid.isEmpty()) g_sessions.remove(sid);
        redirect(client, "/login", "Set-Cookie: sid=; Path=/; Max-Age=0");
        return;
    }

    // 以下全部需要登录
    if (username.isEmpty()) {
        redirect(client, "/login");
        return;
    }

    const HttpUser& me = users[username];
    bool isRoot = me.isRoot;
    bool canR = isRoot || me.r;
    bool canW = isRoot || me.w;

    QString baseDir = ui->currentDir;

    // 主页
    if (method == "GET" && (urlPath == "/" || urlPath.isEmpty())) {
        QString html = generateIndexHtml(username, "/", baseDir, isRoot, canR, canW,
                                          query.value("q", ""));
        sendResponse(client, 200, "text/html; charset=utf-8", html.toUtf8());
        return;
    }

    // 目录浏览
    if (method == "GET" && urlPath.endsWith("/")) {
        if (!canR) { sendResponse(client, 403, "text/plain", "无读权限"); return; }
        QString subPath = urlPath;
        QString fullDir = QDir(baseDir).absoluteFilePath(subPath.mid(1));
        if (QDir(fullDir).exists()) {
            QString html = generateIndexHtml(username, subPath, fullDir, isRoot, canR, canW,
                                              query.value("q", ""));
            sendResponse(client, 200, "text/html; charset=utf-8", html.toUtf8());
        } else {
            sendResponse(client, 404, "text/plain", "Not Found");
        }
        return;
    }

    // 下载
    if (method == "GET" && urlPath == "/download") {
        if (!canR) { sendResponse(client, 403, "text/plain", "无读权限"); return; }
        QString filePath = query.value("path");
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

    // 预览/编辑
    if (method == "GET" && urlPath == "/view") {
        if (!canR) { sendResponse(client, 403, "text/plain", "无读权限"); return; }
        QString rel = query.value("path");
        QString full = QDir(baseDir).absoluteFilePath(rel.mid(1));
        QFileInfo info(full);
        if (!info.exists() || !info.isFile()) {
            sendResponse(client, 404, "text/plain", "文件不存在");
            return;
        }
        if (info.size() > 1024 * 1024) {
            sendResponse(client, 200, "text/plain; charset=utf-8", "文件过大，无法预览（>1MB）");
            return;
        }
        QFile f(full);
        if (!f.open(QIODevice::ReadOnly)) {
            sendResponse(client, 500, "text/plain", "读取失败");
            return;
        }
        QString content = QString::fromUtf8(f.readAll());
        f.close();
        QString html = viewPage(username, rel.mid(1), content, isRoot, canW);
        sendResponse(client, 200, "text/html; charset=utf-8", html.toUtf8());
        return;
    }

    // 保存（在线编辑）
    if (method == "POST" && urlPath == "/save") {
        if (!canW) { sendResponse(client, 403, "text/plain", "无写权限"); return; }
        QMap<QString, QString> form = parseFormBody(body);
        QString rel = form.value("path");
        QString content = form.value("content");
        QString full = QDir(baseDir).absoluteFilePath(rel.mid(1));
        QFile f(full);
        if (f.open(QIODevice::WriteOnly)) {
            f.write(content.toUtf8());
            f.close();
            ui->appendOutput(QString("[HTTP] 保存: %1\n").arg(rel), PROMPT_COLOR);
        }
        redirect(client, "/view?path=" + QUrl::toPercentEncoding(rel));
        return;
    }

    // 删除
    if (method == "POST" && urlPath == "/delete") {
        if (!canW) { sendResponse(client, 403, "text/plain", "无写权限"); return; }
        QMap<QString, QString> form = parseFormBody(body);
        QString name = form.value("path");
        QString full = QDir(baseDir).absoluteFilePath(name);
        QFileInfo info(full);
        if (info.isDir()) QDir(full).removeRecursively();
        else QFile::remove(full);
        ui->appendOutput(QString("[HTTP] 删除: %1\n").arg(name), ERROR_COLOR);
        sendResponse(client, 200, "text/plain", "OK");
        return;
    }

    // 批量删除
    if (method == "POST" && urlPath == "/batch-delete") {
        if (!canW) { sendResponse(client, 403, "text/plain", "无写权限"); return; }
        QMap<QString, QString> form = parseFormBody(body);
        QString paths = form.value("paths");
        for (const QString& name : paths.split('\n')) {
            QString n = name.trimmed();
            if (n.isEmpty()) continue;
            QString full = QDir(baseDir).absoluteFilePath(n);
            QFileInfo info(full);
            if (info.isDir()) QDir(full).removeRecursively();
            else QFile::remove(full);
        }
        sendResponse(client, 200, "text/plain", "OK");
        return;
    }

    // 新建文件夹
    if (method == "POST" && urlPath == "/mkdir") {
        if (!canW) { sendResponse(client, 403, "text/plain", "无写权限"); return; }
        QMap<QString, QString> form = parseFormBody(body);
        QString name = form.value("name");
        QDir(baseDir).mkpath(name);
        ui->appendOutput(QString("[HTTP] 新建: %1\n").arg(name), PROMPT_COLOR);
        redirect(client, "/");
        return;
    }

    // 上传
    if (method == "POST" && urlPath == "/upload") {
        if (!canW) { sendResponse(client, 403, "text/plain", "无写权限"); return; }
        QRegularExpression bRe("boundary=([^;\\r\\n]+)");
        QRegularExpressionMatch bMatch = bRe.match(headers);
        if (bMatch.hasMatch()) {
            QString boundary = "--" + bMatch.captured(1);
            QByteArray bBytes = boundary.toUtf8();
            QByteArray bodyBytes = body.toUtf8();
            int start = bodyBytes.indexOf(bBytes);
            if (start >= 0) {
                int hEnd = bodyBytes.indexOf("\r\n\r\n", start);
                if (hEnd > 0) {
                    QByteArray fileHeader = bodyBytes.mid(start, hEnd - start);
                    QRegularExpression fRe("filename=\"([^\"]+)\"");
                    QRegularExpressionMatch fMatch = fRe.match(QString::fromUtf8(fileHeader));
                    if (fMatch.hasMatch()) {
                        QString filename = fMatch.captured(1);
                        int dataStart = hEnd + 4;
                        int dataEnd = bodyBytes.indexOf("\r\n" + bBytes, dataStart);
                        if (dataEnd > 0) {
                            QByteArray fileData = bodyBytes.mid(dataStart, dataEnd - dataStart);
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
        redirect(client, "/");
        return;
    }

    // ============ 用户管理（仅 root） ============
    if (urlPath.startsWith("/admin/")) {
        if (!isRoot) { sendResponse(client, 403, "text/plain", "需要 root 权限"); return; }

        if (method == "GET" && urlPath == "/admin/users") {
            QString html = adminPage(username, users);
            sendResponse(client, 200, "text/html; charset=utf-8", html.toUtf8());
            return;
        }

        QMap<QString, QString> form = parseFormBody(body);
        QString rootPw = form.value("root_pw");
        // 二次验证 root 密码
        if (hashPassword(users["root"].salt, rootPw) != users["root"].hash) {
            QString html = adminPage(username, users, "root 密码错误", true);
            sendResponse(client, 200, "text/html; charset=utf-8", html.toUtf8());
            return;
        }

        if (method == "POST" && urlPath == "/admin/create") {
            QString u = form.value("username").trimmed();
            QString p = form.value("password");
            if (u.isEmpty() || p.isEmpty() || users.contains(u)) {
                sendResponse(client, 200, "text/html; charset=utf-8",
                    adminPage(username, users, "用户名无效或已存在", true).toUtf8());
                return;
            }
            HttpUser nu;
            nu.salt = randomSalt();
            nu.hash = hashPassword(nu.salt, p);
            nu.isRoot = false;
            nu.r = form.contains("r");
            nu.w = form.contains("w");
            nu.x = form.contains("x");
            users[u] = nu;
            saveUsers(users);
            ui->appendOutput(QString("[HTTP] 创建账户: %1\n").arg(u), PROMPT_COLOR);
            sendResponse(client, 200, "text/html; charset=utf-8",
                adminPage(username, users, QString("账户 %1 已创建").arg(u)).toUtf8());
            return;
        }

        if (method == "POST" && urlPath == "/admin/grant") {
            QString u = form.value("username");
            if (!users.contains(u) || u == "root") { redirect(client, "/admin/users"); return; }
            HttpUser& target = users[u];
            target.r = form.contains("r");
            target.w = form.contains("w");
            target.x = form.contains("x");
            saveUsers(users);
            sendResponse(client, 200, "text/html; charset=utf-8",
                adminPage(username, users, QString("已更新 %1 的权限").arg(u)).toUtf8());
            return;
        }

        if (method == "POST" && urlPath == "/admin/passwd") {
            QString u = form.value("username");
            QString p = form.value("password");
            if (!users.contains(u) || p.isEmpty()) { redirect(client, "/admin/users"); return; }
            users[u].salt = randomSalt();
            users[u].hash = hashPassword(users[u].salt, p);
            saveUsers(users);
            sendResponse(client, 200, "text/html; charset=utf-8",
                adminPage(username, users, QString("%1 的密码已修改").arg(u)).toUtf8());
            return;
        }

        if (method == "POST" && urlPath == "/admin/promote") {
            QString u = form.value("username");
            if (!users.contains(u) || u == "root") { redirect(client, "/admin/users"); return; }
            users[u].isRoot = true;
            saveUsers(users);
            sendResponse(client, 200, "text/html; charset=utf-8",
                adminPage(username, users, QString("%1 已获得 ROOT").arg(u)).toUtf8());
            return;
        }

        if (method == "POST" && urlPath == "/admin/revoke") {
            QString u = form.value("username");
            if (!users.contains(u) || u == "root") { redirect(client, "/admin/users"); return; }
            users[u].isRoot = false;
            saveUsers(users);
            sendResponse(client, 200, "text/html; charset=utf-8",
                adminPage(username, users, QString("%1 的 ROOT 已撤销").arg(u)).toUtf8());
            return;
        }

        if (method == "POST" && urlPath == "/admin/delete") {
            QString u = form.value("username");
            if (!users.contains(u) || u == "root") { redirect(client, "/admin/users"); return; }
            users.remove(u);
            saveUsers(users);
            sendResponse(client, 200, "text/html; charset=utf-8",
                adminPage(username, users, QString("账户 %1 已删除").arg(u)).toUtf8());
            return;
        }
    }

    sendResponse(client, 404, "text/plain", "Not Found");
}

// ============ 命令注册 ============

void CommandRegistry::registerNetwork2Commands(TerminalWidget* ui) {

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

        // 初始化用户文件（首次自动生成 root/2510）
        loadUsers();

        g_httpServer = new QTcpServer();
        g_httpUi = ui;

        QObject::connect(g_httpServer, &QTcpServer::newConnection, [ui]() {
            while (g_httpServer->hasPendingConnections()) {
                QTcpSocket* client = g_httpServer->nextPendingConnection();
                QObject::connect(client, &QTcpSocket::readyRead, [client, ui]() {
                    client->setProperty("data", client->property("data").toByteArray() + client->readAll());
                    QByteArray data = client->property("data").toByteArray();

                    int headerEnd = data.indexOf("\r\n\r\n");
                    if (headerEnd < 0) return;

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
            ui->appendOutput("默认账户: root / 2510（请登录后立即修改）\n", INFO_COLOR);
            ui->appendOutput("功能: 登录 / 浏览 / 下载 / 上传 / 新建 / 删除 / 预览 / 编辑 / 搜索 / 用户管理\n", INFO_COLOR);
            ui->appendOutput("输入 stop-serve 关闭\n", INFO_COLOR);
        } else {
            ui->appendOutput("服务器启动失败\n", ERROR_COLOR);
            delete g_httpServer;
            g_httpServer = nullptr;
        }
    }, "HTTP文件服务器", "serve [端口]");

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