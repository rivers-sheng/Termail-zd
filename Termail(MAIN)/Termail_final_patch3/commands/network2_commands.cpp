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
#include <QDirIterator>

static QTcpServer* g_httpServer = nullptr;
static TerminalWidget* g_httpUi = nullptr;

// ============ 用户与会话 ============
struct HttpUser {
    QString salt;
    QString hash;
    bool isRoot = false;
    bool r = false;
    bool w = false;
    bool x = false;
};

static QMap<QString, QString> g_sessions;

static QString usersFilePath() {
    return Utils::configDir() + "/http_users.json";
}

static QString hashPassword(const QString& salt, const QString& password) {
    QByteArray data = (salt + password).toUtf8();
    return QString::fromLatin1(QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex());
}

static QString randomSalt() {
    QByteArray buf(16, 0);
    for (int i = 0; i < buf.size(); ++i) buf[i] = char(QRandomGenerator::global()->bounded(256));
    return QString::fromLatin1(buf.toHex());
}

static QString randomToken() {
    QByteArray buf(24, 0);
    for (int i = 0; i < buf.size(); ++i) buf[i] = char(QRandomGenerator::global()->bounded(256));
    return QString::fromLatin1(buf.toHex());
}

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
    if (!users.contains("root")) {
        HttpUser root;
        root.salt = randomSalt();
        root.hash = hashPassword(root.salt, "2510");
        root.isRoot = true;
        root.r = root.w = root.x = true;
        users["root"] = root;
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
        if (out.open(QIODevice::WriteOnly)) { out.write(QJsonDocument(obj).toJson()); out.close(); }
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
    if (out.open(QIODevice::WriteOnly)) { out.write(QJsonDocument(obj).toJson()); out.close(); }
}

static QString extractSid(const QString& headers) {
    QRegularExpression re("Cookie:\\s*([^\\r\\n]+)", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch m = re.match(headers);
    if (!m.hasMatch()) return "";
    for (const QString& part : m.captured(1).split(';')) {
        QString p = part.trimmed();
        if (p.startsWith("sid=")) return p.mid(4);
    }
    return "";
}

static QString currentUser(const QString& headers) {
    QString sid = extractSid(headers);
    return sid.isEmpty() ? "" : g_sessions.value(sid, "");
}

// ============ 核心安全辅助 ============
static bool isSensitiveFile(const QString& path) {
    return path.contains("http_users.json") || path.endsWith(".termail_cmd.log");
}

// ============ UI 辅助 ============
static QString htmlEscape(const QString& s) { return s.toHtmlEscaped(); }

static QString pageShell(const QString& title, const QString& user, const QString& body, bool isRoot, bool canW, bool canR, const QString& curDir = "") {
    QString html = R"(<!DOCTYPE html>
<html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no">
<title>)" + htmlEscape(title) + R"(</title>
<style>
:root{--bg:#0d0d0d;--card:#161616;--border:#2a2a2a;--text:#e0e0e0;--text-muted:#888;--primary:#4fc3f7;--danger:#ef5350;--success:#66bb6a}
*{box-sizing:border-box;margin:0;padding:0}
body{background:var(--bg);color:var(--text);font-family:-apple-system,'Segoe UI',Roboto,sans-serif;padding:16px;line-height:1.5;font-size:14px}
a{color:var(--primary);text-decoration:none}
a:hover{text-decoration:underline}
h1{color:var(--primary);font-size:20px;margin-bottom:6px}
.topbar{display:flex;align-items:center;justify-content:space-between;gap:12px;flex-wrap:wrap;margin-bottom:20px;padding-bottom:12px;border-bottom:1px solid var(--border)}
.brand{font-size:20px;color:var(--primary);font-weight:600;display:flex;align-items:center;gap:8px}
.userbox{font-size:13px;color:var(--text-muted)}
.userbox b{color:var(--text)}
.crumb{color:var(--text-muted);font-size:13px;margin-bottom:14px;word-break:break-all}
.toolbar{display:flex;gap:8px;margin-bottom:14px;flex-wrap:wrap;align-items:center}
.btn{background:#222;color:var(--text);border:1px solid var(--border);padding:8px 14px;border-radius:6px;cursor:pointer;font-size:13px;display:inline-block;transition:0.2s}
.btn:hover{background:#333}
.btn-primary{background:var(--primary);color:#000;border:none;font-weight:600}
.btn-primary:hover{background:#29b6f6}
.btn-danger{background:var(--danger);color:#fff;border:none}
.btn-danger:hover{background:#e53935}
.btn-sm{padding:5px 10px;font-size:12px}
table{width:100%;border-collapse:collapse;font-size:13px;background:var(--card);border-radius:8px;overflow:hidden}
th,td{padding:12px 10px;text-align:left;border-bottom:1px solid var(--border)}
th{color:var(--text-muted);font-weight:500;font-size:12px;text-transform:uppercase;background:#1a1a1a}
tr:last-child td{border-bottom:none}
tr:hover td{background:#1c1c1c}
.icon{margin-right:8px;font-size:16px}
.muted{color:var(--text-muted);font-size:12px}
.empty{color:var(--text-muted);text-align:center;padding:40px 0;font-size:14px}
.card{background:var(--card);border:1px solid var(--border);border-radius:10px;padding:18px;margin-top:18px}
.card h3{color:var(--primary);font-size:15px;margin-bottom:14px;border-left:3px solid var(--primary);padding-left:8px}
input[type=text],input[type=password],input[type=file],textarea,select{background:#111;color:var(--text);border:1px solid var(--border);padding:8px 12px;border-radius:6px;font-size:13px;width:100%;max-width:300px;font-family:inherit}
input:focus,textarea:focus,select:focus{outline:none;border-color:var(--primary)}
.form-row{display:flex;gap:10px;align-items:center;flex-wrap:wrap;background:#111;padding:12px;border-radius:8px;margin-bottom:10px}
.msg{padding:10px 14px;border-radius:6px;margin-bottom:14px;font-size:13px}
.msg-err{background:#3a1414;color:#ef9a9a;border:1px solid #5c1e1e}
.msg-ok{background:#12301c;color:#a5d6a7;border:1px solid #1e4d2e}
.badge{display:inline-block;padding:3px 8px;border-radius:4px;font-size:11px;font-weight:600}
.badge-root{background:var(--primary);color:#000}
.badge-user{background:#333;color:#ccc}
.checkbox{width:16px;height:16px;cursor:pointer;accent-color:var(--primary)}
.searchbox{display:flex;gap:8px;margin-bottom:14px;flex-wrap:wrap}
.upload-progress{width:100%;height:6px;background:#333;border-radius:3px;margin-top:10px;display:none;overflow:hidden}
.upload-bar{height:100%;background:var(--primary);width:0%;transition:width 0.1s}
</style></head><body>
<div class='topbar'>
<div class='brand'>🌐 Termail 文件管理</div>
<div class='userbox'>)";

    if (!user.isEmpty()) {
        html += QString("当前: <b>%1</b> ").arg(htmlEscape(user));
        if (isRoot) html += "<span class='badge badge-root'>ROOT</span>";
        else html += "<span class='badge badge-user'>USER</span>";
        if (isRoot) html += " <a href='/admin/users' style='margin-left:10px'>用户管理</a>";
        html += " <a href='/logout' style='margin-left:10px'>退出</a>";
    }

    html += "</div></div>\n" + body + "\n</body></html>\n";
    return html;
}

static QString loginPage(const QString& error = "") {
    QString html = R"(<!DOCTYPE html>
<html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no">
<title>登录 - Termail</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{background:#0d0d0d;color:#e0e0e0;font-family:-apple-system,'Segoe UI',sans-serif;display:flex;align-items:center;justify-content:center;min-height:100vh;padding:16px}
.card{background:#161616;border:1px solid #2a2a2a;border-radius:12px;padding:32px;width:100%;max-width:360px}
h1{color:#4fc3f7;font-size:24px;margin-bottom:6px;text-align:center}
.sub{color:#888;font-size:13px;text-align:center;margin-bottom:24px}
label{display:block;color:#888;font-size:12px;margin-bottom:6px;margin-top:16px}
input{background:#111;color:#e0e0e0;border:1px solid #333;padding:12px;border-radius:6px;font-size:14px;width:100%}
input:focus{outline:none;border-color:#4fc3f7}
button{background:#4fc3f7;color:#000;border:none;padding:12px;border-radius:6px;font-size:14px;font-weight:600;width:100%;margin-top:24px;cursor:pointer;transition:0.2s}
button:hover{background:#29b6f6}
.msg-err{background:#3a1414;color:#ef9a9a;border:1px solid #5c1e1e;padding:10px 14px;border-radius:6px;margin-bottom:14px;font-size:13px}
</style></head><body>
<div class='card'>
<h1>Termail</h1>
<div class='sub'>文件管理系统</div>
)";
    if (!error.isEmpty()) html += QString("<div class='msg-err'>%1</div>\n").arg(htmlEscape(error));
    html += R"(
<form method='post' action='/login'>
<label>用户名</label>
<input type='text' name='username' required autofocus>
<label>密码</label>
<input type='password' name='password' required>
<button type='submit'>登 录</button>
</form>
</div>
</body></html>
)";
    return html;
}

static QString generateIndexHtml(const QString& user, const QString& currentPath,
                                 const QString& dirPath, bool isRoot, bool canR, bool canW,
                                 const QString& search, const QString& baseDir) {
    QDir dir(dirPath);
    QStringList files;
    
    if (!search.isEmpty()) {
        int maxCount = 500;
        QStringList filters = {"*" + search + "*"};
        files = dir.entryList(filters, QDir::AllEntries | QDir::NoDotAndDotDot, QDir::Name | QDir::DirsFirst);
        if (files.size() < maxCount) {
            QDirIterator it(dirPath, filters, QDir::AllEntries | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
            while (it.hasNext() && files.size() < maxCount) {
                QString rel = dir.relativeFilePath(it.next());
                if (!files.contains(rel)) files.append(rel);
            }
        }
    } else {
        files = dir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot, QDir::Name | QDir::DirsFirst);
        if (files.size() > 1000) files = files.mid(0, 1000);
    }

    QString body;
    body += QString("<div class='crumb'>当前路径: <b>%1</b></div>\n").arg(currentPath.isEmpty() ? "/" : htmlEscape(currentPath));

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
        body += "<button class='btn btn-primary btn-sm' onclick=\"batchDownload()\">下载选中</button>\n";
        body += "<button class='btn btn-danger btn-sm' onclick=\"batchDelete()\">删除选中</button>\n";
    }
    body += "</div>\n";

    body += "<form class='searchbox' method='get' action=''>\n";
    body += QString("<input type='text' name='q' placeholder='递归搜索文件...' value='%1'>\n").arg(htmlEscape(search));
    body += "<button type='submit' class='btn'>搜索</button>\n";
    if (!search.isEmpty()) body += "<a class='btn' href='"+ (currentPath.isEmpty() ? "/" : currentPath) +"'>清除</a>\n";
    body += "</form>\n";

    body += "<table>\n<thead><tr>\n";
    if (canW) body += "<th style='width:30px'><input type='checkbox' class='checkbox' id='chkAll' onclick=\"toggleAll(this)\"></th>\n";
    body += "<th>名称</th><th style='width:80px'>大小</th><th style='width:120px'>操作</th>\n";
    body += "</tr></thead>\n<tbody>\n";

    if (files.isEmpty()) {
        body += QString("<tr><td colspan='4' class='empty'>%1</td></tr>\n").arg(search.isEmpty() ? "空目录" : "无匹配文件");
    } else {
        for (const QString& name : files) {
            QFileInfo info(dir.absoluteFilePath(name));
            QString icon = info.isDir() ? "📁" : "📄";
            QString size = info.isDir() ? "-" : Utils::formatSize(info.size());
            QString href = info.isDir() 
                ? (currentPath + (currentPath.endsWith('/') ? "" : "/") + name + "/")
                : ("/download?path=" + QUrl::toPercentEncoding(currentPath + (currentPath.endsWith('/') ? "" : "/") + name));

            body += "<tr>\n";
            if (canW) body += QString("<td><input type='checkbox' class='checkbox rowchk' value='%1'></td>\n").arg(htmlEscape(name));
            body += QString("<td><span class='icon'>%1</span>").arg(icon);
            if (info.isDir() || canR) {
                body += QString("<a href='%1'>%2%3</a>").arg(href, htmlEscape(name), info.isDir() ? "/" : "");
            } else {
                body += htmlEscape(name);
            }
            body += "</td>\n";
            body += QString("<td class='muted'>%1</td>\n").arg(size);
            body += "<td>";
            if (!info.isDir() && canR) {
                body += QString("<a class='btn btn-sm' href='/view?path=%1'>预览</a> ").arg(QUrl::toPercentEncoding(currentPath + (currentPath.endsWith('/') ? "" : "/") + name));
                body += QString("<a class='btn btn-sm' href='%1' download>下载</a> ").arg(href);
            }
            if (canW) body += QString("<button class='btn btn-danger btn-sm' onclick=\"delFile('%1')\">删</button>").arg(htmlEscape(name).replace("'", "\\'"));
            body += "</td>\n</tr>\n";
        }
    }
    body += "</tbody></table>\n";

    if (canW) {
        body += QString(R"(
<div class='card'>
<h3>📤 上传文件</h3>
<form id='uploadForm' method='post' action='/upload' enctype='multipart/form-data'>
<input type='hidden' name='cur_dir' value='%1'>
<input type='file' name='file' id='fileInput' required style='margin-bottom:12px'>
<div class='upload-progress' id='progBar'><div class='upload-bar' id='progFill'></div></div>
<button type='submit' class='btn btn-primary'>上传</button>
</form>
</div>

<div class='card'>
<h3>📁 新建</h3>
<form action='/mkdir' method='post' class='form-row'>
<input type='hidden' name='cur_dir' value='%1'>
<input type='text' name='name' placeholder='文件夹名' required>
<button type='submit' class='btn btn-primary'>新建目录</button>
</form>
<form action='/touch' method='post' class='form-row' style='margin-top:10px'>
<input type='hidden' name='cur_dir' value='%1'>
<input type='text' name='name' placeholder='文件名 (如 test.txt)' required>
<button type='submit' class='btn btn-primary'>新建文件</button>
</form>
</div>

<script>
function toggleAll(cb){document.querySelectorAll('.rowchk').forEach(c=>c.checked=cb.checked);}
function delFile(name){
  if(!confirm('确定删除 '+name+' 吗？'))return;
  fetch('/delete',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'path='+encodeURIComponent(name)+'&cur_dir='+encodeURIComponent('%1')}).then(()=>location.reload());
}
function batchDelete(){
  const names=Array.from(document.querySelectorAll('.rowchk:checked')).map(c=>c.value);
  if(names.length===0){alert('请先勾选文件');return;}
  if(!confirm('确定删除选中的 '+names.length+' 项吗？'))return;
  fetch('/batch-delete',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'paths='+encodeURIComponent(names.join('\n'))+'&cur_dir='+encodeURIComponent('%1')}).then(()=>location.reload());
}
function batchDownload(){
  const names=Array.from(document.querySelectorAll('.rowchk:checked')).map(c=>c.value);
  if(names.length===0){alert('请先勾选文件');return;}
  const form = document.createElement('form');
  form.method = 'POST';
  form.action = '/batch-download';
  form.target = '_blank';
  const input1 = document.createElement('input');
  input1.type = 'hidden'; input1.name = 'paths'; input1.value = names.join('\n');
  const input2 = document.createElement('input');
  input2.type = 'hidden'; input2.name = 'cur_dir'; input2.value = '%1';
  form.appendChild(input1); form.appendChild(input2);
  document.body.appendChild(form);
  form.submit();
  document.body.removeChild(form);
}
document.getElementById('uploadForm').onsubmit = function(e) {
    e.preventDefault();
    const file = document.getElementById('fileInput').files[0];
    if(!file) return;
    const fd = new FormData();
    fd.append('file', file);
    fd.append('cur_dir', '%1');
    const xhr = new XMLHttpRequest();
    xhr.open('POST', '/upload', true);
    document.getElementById('progBar').style.display = 'block';
    xhr.upload.onprogress = function(e) {
        if (e.lengthComputable) {
            document.getElementById('progFill').style.width = (e.loaded / e.total) * 100 + '%';
        }
    };
    xhr.onload = function() { location.reload(); };
    xhr.send(fd);
};
</script>
)").arg(htmlEscape(currentPath));
    }
    return pageShell("文件管理", user, body, isRoot, canW, canR, currentPath);
}

static QString viewPage(const QString& user, const QString& relPath, const QString& content, bool isRoot, bool canW, const QString& msg = "") {
    QString body = QString("<div class='crumb'>当前路径: <b>%1</b></div>\n").arg(htmlEscape(relPath));
    if (!msg.isEmpty()) body += QString("<div class='msg msg-ok'>%1</div>\n").arg(htmlEscape(msg));
    body += "<form method='post' action='/save'>\n";
    body += QString("<input type='hidden' name='path' value='%1'>\n").arg(htmlEscape(relPath));
    body += QString("<textarea name='content' %1>%2</textarea>\n").arg(canW ? "" : "readonly", htmlEscape(content));
    body += "<div class='toolbar' style='margin-top:12px'>\n";
    if (canW) body += "<button type='submit' class='btn btn-primary'>保存修改</button>\n";
    body += "<a class='btn' href='/'>返回</a>\n";
    body += "</div></form>\n";
    return pageShell("预览 - " + relPath, user, body, isRoot, canW, true);
}

static QString adminPage(const QString& user, const QMap<QString, HttpUser>& users, const QString& msg = "", bool isErr = false) {
    QString body;
    if (!msg.isEmpty()) body += QString("<div class='msg %1'>%2</div>\n").arg(isErr ? "msg-err" : "msg-ok", htmlEscape(msg));

    body += R"(
<div class='card'>
<h3>➕ 创建账户</h3>
<form method='post' action='/admin/create' class='form-row'>
<input type='text' name='username' placeholder='用户名' required>
<input type='password' name='password' placeholder='密码' required>
<label style='display:flex;align-items:center;gap:4px;margin:0;font-size:13px;cursor:pointer'><input type='checkbox' name='r' checked class='checkbox'> 读</label>
<label style='display:flex;align-items:center;gap:4px;margin:0;font-size:13px;cursor:pointer'><input type='checkbox' name='w' class='checkbox'> 写</label>
<label style='display:flex;align-items:center;gap:4px;margin:0;font-size:13px;cursor:pointer'><input type='checkbox' name='x' class='checkbox'> 运行</label>
<input type='password' name='root_pw' placeholder='当前root密码' required style='max-width:160px'>
<button type='submit' class='btn btn-primary'>创建</button>
</form>
</div>

<div class='card'>
<h3>👥 用户列表</h3>
<table>
<thead><tr><th>用户</th><th>权限</th><th>管理操作 (验证 root 密码后执行)</th></tr></thead>
<tbody>
)";

    for (auto it = users.begin(); it != users.end(); ++it) {
        const QString& name = it.key();
        const HttpUser& u = it.value();
        body += "<tr>\n";
        body += QString("<td><b>%1</b></td>\n").arg(htmlEscape(name));
        body += "<td>";
        if (u.isRoot) body += "<span class='badge badge-root'>ROOT</span>";
        else {
            body += "<span class='badge badge-user'>USER</span><br><span class='muted'>";
            if (u.r) body += "R "; if (u.w) body += "W "; if (u.x) body += "X";
            if (!u.r && !u.w && !u.x) body += "无权限";
            body += "</span>";
        }
        body += "</td>\n<td>";

        if (name == "root" && u.isRoot) {
            body += QString("<form method='post' action='/admin/passwd' style='display:inline-flex;gap:8px;align-items:center'>"
                "<input type='password' name='password' placeholder='新 root 密码' required style='width:140px'>"
                "<button type='submit' class='btn btn-sm'>更新密码</button></form>");
        } else {
            body += QString("<form method='post' action='/admin/manage' style='display:inline-flex;gap:8px;flex-wrap:wrap;align-items:center'>"
                "<input type='hidden' name='username' value='%1'>"
                "<input type='password' name='root_pw' placeholder='root密码' required style='width:120px'>"
                "<select name='action' style='width:auto;padding:6px'>"
                "  <option value='grant'>改权限 (R/W/X)</option>"
                "  <option value='passwd'>修改密码</option>"
                "  <option value='promote'>授予ROOT</option>"
                "  <option value='revoke'>撤销ROOT</option>"
                "  <option value='delete'>删除账户</option>"
                "</select>"
                "<input type='text' name='new_val' placeholder='新值(R/W/X或密码)' style='width:140px'>"
                "<button type='submit' class='btn btn-sm btn-primary'>执行</button>"
                "</form>").arg(htmlEscape(name));
        }
        body += "</td>\n</tr>\n";
    }
    body += "</tbody></table></div>\n";
    return pageShell("用户管理", user, body, true, true, true);
}

// ============ HTTP 响应 ============
static void sendResponse(QTcpSocket* client, int code, const QString& contentType, const QByteArray& body, const QString& extraHeaders = "") {
    QString status = code == 200 ? "200 OK" : code == 302 ? "302 Found" : code == 401 ? "401 Unauthorized" : code == 403 ? "403 Forbidden" : code == 404 ? "404 Not Found" : code == 400 ? "400 Bad Request" : "500 Error";
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

static QMap<QString, QString> parseFormBody(const QString& body) {
    QMap<QString, QString> params;
    for (const QString& pair : body.split('&')) {
        int eq = pair.indexOf('=');
        if (eq < 0) continue;
        params[QUrl::fromPercentEncoding(pair.left(eq).toUtf8())] = QUrl::fromPercentEncoding(pair.mid(eq + 1).toUtf8());
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

    QString urlPath = rawPath;
    QMap<QString, QString> query;
    if (rawPath.contains('?')) {
        int idx = rawPath.indexOf('?');
        urlPath = rawPath.left(idx);
        query = parseFormBody(rawPath.mid(idx + 1));
    }

    QMap<QString, HttpUser> users = loadUsers();
    QString username = currentUser(headers);

    // ====== 恢复终端日志打印 ======
    ui->appendOutput(QString("[%1] %2 %3 (user=%4)\n")
        .arg(QDateTime::currentDateTime().toString("hh:mm:ss"),
             method, rawPath, username.isEmpty() ? "-" : username), INFO_COLOR);

    if (urlPath == "/login" && method == "GET") {
        if (!username.isEmpty()) { redirect(client, "/"); return; }
        sendResponse(client, 200, "text/html; charset=utf-8", loginPage().toUtf8());
        return;
    }
    if (urlPath == "/login" && method == "POST") {
        QMap<QString, QString> form = parseFormBody(body);
        QString u = form.value("username");
        QString p = form.value("password");
        if (users.contains(u) && hashPassword(users[u].salt, p) == users[u].hash) {
            QString token = randomToken();
            g_sessions[token] = u;
            redirect(client, "/", "Set-Cookie: sid=" + token + "; Path=/; HttpOnly");
            return;
        }
        sendResponse(client, 200, "text/html; charset=utf-8", loginPage("用户名或密码错误").toUtf8());
        return;
    }
    if (urlPath == "/logout") {
        QString sid = extractSid(headers);
        if (!sid.isEmpty()) g_sessions.remove(sid);
        redirect(client, "/login", "Set-Cookie: sid=; Path=/; Max-Age=0");
        return;
    }

    if (username.isEmpty()) { redirect(client, "/login"); return; }

    const HttpUser& me = users[username];
    bool isRoot = me.isRoot;
    bool canR = isRoot || me.r;
    bool canW = isRoot || me.w;
    QString baseDir = ui->currentDir;

    // 处理 POST 请求
    if (method == "POST" && urlPath != "/login") {
        QMap<QString, QString> form = parseFormBody(body);
        
        if (!isRoot) {
            QString reqPath = form.value("path") + form.value("cur_dir") + form.value("paths");
            if (isSensitiveFile(reqPath)) {
                sendResponse(client, 403, "text/plain; charset=utf-8", "拒绝访问：无权操作系统敏感文件");
                return;
            }
        }

        if (urlPath == "/save") {
            if (!canW) { sendResponse(client, 403, "text/plain", "无写权限"); return; }
            QString rel = form.value("path");
            QString full = QDir(baseDir).absoluteFilePath(rel.mid(1));
            QFile f(full);
            if (f.open(QIODevice::WriteOnly)) { f.write(form.value("content").toUtf8()); f.close(); }
            redirect(client, "/view?path=" + QUrl::toPercentEncoding(rel));
            return;
        }

        if (urlPath == "/delete") {
            if (!canW) { sendResponse(client, 403, "text/plain", "无写权限"); return; }
            QString curD = form.value("cur_dir", "/");
            QString name = form.value("path");
            QString full = QDir(baseDir).absoluteFilePath(curD.mid(1)) + "/" + name;
            QFileInfo info(full);
            if (info.isDir()) QDir(full).removeRecursively(); else QFile::remove(full);
            sendResponse(client, 200, "text/plain", "OK");
            return;
        }

        if (urlPath == "/batch-delete") {
            if (!canW) { sendResponse(client, 403, "text/plain", "无写权限"); return; }
            QString curD = form.value("cur_dir", "/");
            QStringList paths = form.value("paths").split('\n');
            for (const QString& n : paths) {
                if (n.trimmed().isEmpty()) continue;
                QString full = QDir(baseDir).absoluteFilePath(curD.mid(1)) + "/" + n.trimmed();
                QFileInfo info(full);
                if (info.isDir()) QDir(full).removeRecursively(); else QFile::remove(full);
            }
            sendResponse(client, 200, "text/plain", "OK");
            return;
        }

        if (urlPath == "/mkdir") {
            if (!canW) { sendResponse(client, 403, "text/plain", "无写权限"); return; }
            QString curD = form.value("cur_dir", "/");
            QString name = form.value("name");
            QDir target(QDir(baseDir).absoluteFilePath(curD.mid(1)));
            if (target.exists(name)) {
                sendResponse(client, 200, "text/html; charset=utf-8", ("目录已存在 <a href='"+curD+"'>返回</a>").toUtf8());
                return;
            }
            target.mkpath(name);
            redirect(client, curD);
            return;
        }

        if (urlPath == "/touch") {
            if (!canW) { sendResponse(client, 403, "text/plain", "无写权限"); return; }
            QString curD = form.value("cur_dir", "/");
            QString name = form.value("name");
            QFile f(QDir(baseDir).absoluteFilePath(curD.mid(1)) + "/" + name);
            if (f.exists()) {
                sendResponse(client, 200, "text/html; charset=utf-8", ("文件已存在 <a href='"+curD+"'>返回</a>").toUtf8());
                return;
            }
            f.open(QIODevice::WriteOnly); f.close();
            redirect(client, curD);
            return;
        }

        if (urlPath == "/batch-download") {
            if (!canR) { sendResponse(client, 403, "text/plain", "无读权限"); return; }
            QString curD = form.value("cur_dir", "/");
            QStringList paths = form.value("paths").split('\n');
            
            QString tmpName = QString("Termail_Download_%1").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));
            QString tmpDir = QDir(baseDir).absoluteFilePath(tmpName);
            QDir().mkpath(tmpDir);
            
            for (const QString& n : paths) {
                if (n.trimmed().isEmpty()) continue;
                QString src = QDir(baseDir).absoluteFilePath(curD.mid(1)) + "/" + n.trimmed();
                QString dst = tmpDir + "/" + n.trimmed();
                if (QFileInfo(src).isDir()) {
                    Utils::copyDir(src, dst);
                } else if (QFileInfo(src).isFile()) {
                    QFile::copy(src, dst);
                }
            }
            redirect(client, "/" + QUrl::toPercentEncoding(tmpName) + "/");
            return;
        }
    }

    // GET 请求
    if (!isRoot && isSensitiveFile(query.value("path"))) {
        sendResponse(client, 403, "text/plain; charset=utf-8", "拒绝访问：无权查看系统敏感文件");
        return;
    }

    QString curDirRel = query.value("cur_dir", "/");
    if (curDirRel.isEmpty()) curDirRel = "/";
    QString currentPath = curDirRel;
    QString fullDir = QDir(baseDir).absoluteFilePath(currentPath.mid(1));

    if (method == "GET" && (urlPath == "/" || urlPath.isEmpty())) {
        QString html = generateIndexHtml(username, currentPath, fullDir, isRoot, canR, canW, query.value("q", ""), baseDir);
        sendResponse(client, 200, "text/html; charset=utf-8", html.toUtf8());
        return;
    }

    if (method == "GET" && urlPath.endsWith("/")) {
        if (!canR) { sendResponse(client, 403, "text/plain", "无读权限"); return; }
        QString subPath = urlPath;
        if (QDir(QDir(baseDir).absoluteFilePath(subPath.mid(1))).exists()) {
            QString html = generateIndexHtml(username, subPath, QDir(baseDir).absoluteFilePath(subPath.mid(1)), isRoot, canR, canW, query.value("q", ""), baseDir);
            sendResponse(client, 200, "text/html; charset=utf-8", html.toUtf8());
        } else {
            sendResponse(client, 404, "text/plain", "Not Found");
        }
        return;
    }

    if (method == "GET" && urlPath == "/download") {
        if (!canR) { sendResponse(client, 403, "text/plain", "无读权限"); return; }
        QString filePath = query.value("path");
        QString fullPath = QDir(baseDir).absoluteFilePath(filePath.mid(1));
        QFileInfo info(fullPath);
        if (info.exists() && info.isFile()) {
            QFile f(fullPath);
            if (f.open(QIODevice::ReadOnly)) {
                sendResponse(client, 200, "application/octet-stream", f.readAll());
                f.close();
            } else {
                sendResponse(client, 500, "text/plain", "Read Error");
            }
        } else {
            sendResponse(client, 404, "text/plain", "Not Found");
        }
        return;
    }

    if (method == "GET" && urlPath == "/view") {
        if (!canR) { sendResponse(client, 403, "text/plain", "无读权限"); return; }
        QString rel = query.value("path");
        QString full = QDir(baseDir).absoluteFilePath(rel.mid(1));
        QFileInfo info(full);
        if (!info.exists() || !info.isFile()) { sendResponse(client, 404, "text/plain", "文件不存在"); return; }
        if (info.size() > 1024 * 1024) { sendResponse(client, 200, "text/plain; charset=utf-8", "文件过大，无法预览（>1MB）"); return; }
        QFile f(full);
        if (!f.open(QIODevice::ReadOnly)) { sendResponse(client, 500, "text/plain", "读取失败"); return; }
        QByteArray fileData = f.readAll();
        QString content = QString::fromUtf8(fileData);
        if (content.contains(QChar(0xFFFD))) {
            content = QString::fromLocal8Bit(fileData);
        }
        f.close();
        sendResponse(client, 200, "text/html; charset=utf-8", viewPage(username, rel.mid(1), content, isRoot, canW).toUtf8());
        return;
    }

    if (method == "POST" && urlPath == "/upload") {
        if (!canW) { sendResponse(client, 403, "text/plain", "无写权限"); return; }
        QRegularExpression bRe("boundary=([^;\\r\\n]+)");
        QRegularExpressionMatch bMatch = bRe.match(headers);
        if (bMatch.hasMatch()) {
            QString boundary = "--" + bMatch.captured(1);
            QByteArray bBytes = boundary.toUtf8();
            QByteArray bodyBytes = body.toUtf8();
            
            QString curD = "/";
            QRegularExpression dirRe("name=\"cur_dir\"\\r\\n\\r\\n([^\\r\\n]+)");
            QRegularExpressionMatch dirMatch = dirRe.match(QString::fromUtf8(bodyBytes));
            if (dirMatch.hasMatch()) curD = dirMatch.captured(1);
            
            int start = bodyBytes.indexOf(bBytes);
            if (start >= 0) {
                int hEnd = bodyBytes.indexOf("\r\n\r\n", start);
                if (hEnd > 0) {
                    QRegularExpression fRe("filename=\"([^\"]+)\"");
                    QRegularExpressionMatch fMatch = fRe.match(QString::fromUtf8(bodyBytes.mid(start, hEnd - start)));
                    if (fMatch.hasMatch()) {
                        QString filename = fMatch.captured(1);
                        int dataStart = hEnd + 4;
                        int dataEnd = bodyBytes.indexOf("\r\n" + bBytes, dataStart);
                        if (dataEnd > 0) {
                            QFile f(QDir(baseDir).absoluteFilePath(curD.mid(1)) + "/" + filename);
                            if (f.open(QIODevice::WriteOnly)) { f.write(bodyBytes.mid(dataStart, dataEnd - dataStart)); f.close(); }
                        }
                    }
                }
            }
        }
        redirect(client, "/");
        return;
    }

    if (urlPath.startsWith("/admin/")) {
        if (!isRoot) { sendResponse(client, 403, "text/plain", "需要 root 权限"); return; }

        if (method == "GET" && urlPath == "/admin/users") {
            sendResponse(client, 200, "text/html; charset=utf-8", adminPage(username, users).toUtf8());
            return;
        }

        QMap<QString, QString> form = parseFormBody(body);
        QString rootPw = form.value("root_pw");
        
        if (hashPassword(users["root"].salt, rootPw) != users["root"].hash) {
            sendResponse(client, 200, "text/html; charset=utf-8", adminPage(username, users, "root 密码错误", true).toUtf8());
            return;
        }

        if (method == "POST" && urlPath == "/admin/create") {
            QString u = form.value("username").trimmed();
            QString p = form.value("password");
            if (u.isEmpty() || p.isEmpty() || users.contains(u)) {
                sendResponse(client, 200, "text/html; charset=utf-8", adminPage(username, users, "用户名无效或已存在", true).toUtf8());
                return;
            }
            HttpUser nu;
            nu.salt = randomSalt(); nu.hash = hashPassword(nu.salt, p);
            nu.r = form.contains("r"); nu.w = form.contains("w"); nu.x = form.contains("x");
            users[u] = nu; saveUsers(users);
            sendResponse(client, 200, "text/html; charset=utf-8", adminPage(username, users, QString("账户 %1 已创建").arg(u)).toUtf8());
            return;
        }

        if (method == "POST" && urlPath == "/admin/passwd") {
            QString u = form.value("username");
            QString p = form.value("password");
            if (!users.contains(u) || p.isEmpty()) { redirect(client, "/admin/users"); return; }
            users[u].salt = randomSalt();
            users[u].hash = hashPassword(users[u].salt, p);
            saveUsers(users);
            sendResponse(client, 200, "text/html; charset=utf-8", adminPage(username, users, QString("%1 的密码已更新").arg(u)).toUtf8());
            return;
        }

        if (method == "POST" && urlPath == "/admin/manage") {
            QString u = form.value("username");
            QString action = form.value("action");
            QString val = form.value("new_val");
            if (!users.contains(u) || u == "root") { redirect(client, "/admin/users"); return; }
            
            HttpUser& target = users[u];
            if (action == "grant") {
                target.r = val.contains("r", Qt::CaseInsensitive);
                target.w = val.contains("w", Qt::CaseInsensitive);
                target.x = val.contains("x", Qt::CaseInsensitive);
            } else if (action == "passwd") {
                if (!val.isEmpty()) { target.salt = randomSalt(); target.hash = hashPassword(target.salt, val); }
            } else if (action == "promote") {
                target.isRoot = true;
            } else if (action == "revoke") {
                target.isRoot = false;
            } else if (action == "delete") {
                users.remove(u);
            }
            saveUsers(users);
            sendResponse(client, 200, "text/html; charset=utf-8", adminPage(username, users, "操作已执行").toUtf8());
            return;
        }
    }

    sendResponse(client, 404, "text/plain", "Not Found");
}

void CommandRegistry::registerNetwork2Commands(TerminalWidget* ui) {
    registerCommand("serve", [ui](const QStringList& args) {
        int port = 8080;
        if (!args.isEmpty()) {
            bool ok;
            int p = args[0].toInt(&ok);
            if (ok && p > 0 && p < 65536) port = p;
        }

        if (g_httpServer && g_httpServer->isListening()) {
            ui->appendOutput(QString("服务器已在端口 %1 运行\n").arg(g_httpServer->serverPort()), ERROR_COLOR);
            return;
        }

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
            
            // 打印所有可用的局域网访问地址
            for (const QNetworkInterface& iface : QNetworkInterface::allInterfaces()) {
                for (const QNetworkAddressEntry& entry : iface.addressEntries()) {
                    if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol &&
                        !entry.ip().isLoopback()) {
                        ui->appendOutput(QString("访问: http://%1:%2\n")
                            .arg(entry.ip().toString()).arg(port), ACCENT_COLOR);
                    }
                }
            }

            ui->appendOutput("默认账户: root / 2510\n", INFO_COLOR);
            ui->appendOutput("输入 stop-serve 关闭\n", INFO_COLOR);
        } else {
            ui->appendOutput("服务器启动失败\n", ERROR_COLOR);
            delete g_httpServer; g_httpServer = nullptr;
        }
    }, "HTTP文件服务器", "serve [端口]");

    registerCommand("stop-serve", [ui](const QStringList&) {
        if (g_httpServer && g_httpServer->isListening()) {
            g_httpServer->close(); delete g_httpServer; g_httpServer = nullptr;
            ui->appendOutput("服务器已关闭\n", INFO_COLOR);
        } else {
            ui->appendOutput("服务器未运行\n", ERROR_COLOR);
        }
    }, "关闭HTTP服务器", "stop-serve");
}

namespace Network2Cleanup {
    void cleanup() {
        if (g_httpServer && g_httpServer->isListening()) {
            g_httpServer->close(); delete g_httpServer; g_httpServer = nullptr;
        }
    }
}
