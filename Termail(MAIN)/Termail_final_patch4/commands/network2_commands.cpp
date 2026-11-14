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
#include <QJsonArray>
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
static QMap<QString, qint64> g_onlineUsers;
static QSet<QString> g_blacklistedIPs;

static QString usersFilePath() {
    return Utils::configDir() + "/http_users.json";
}

static QString blacklistFilePath() {
    return Utils::configDir() + "/http_blacklist.json";
}

static QString logDirPath() {
    return g_httpUi ? QDir(g_httpUi->currentDir).absoluteFilePath("serve-log") : "";
}

// ============ 辅助函数 ============
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

static void logRequest(const QString& msg) {
    if (!g_httpUi) return;
    QString timeStr = QDateTime::currentDateTime().toString("hh:mm:ss");
    g_httpUi->appendOutput(QString("[%1] %2\n").arg(timeStr, msg), INFO_COLOR);

    QString logDir = logDirPath();
    QDir().mkpath(logDir);
    QFile logFile(logDir + "/" + QDateTime::currentDateTime().toString("yyyy-MM-dd") + ".log");
    if (logFile.open(QIODevice::Append | QIODevice::Text)) {
        logFile.write(QString("[%1] %2\n").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"), msg).toUtf8());
        logFile.close();
    }
}

// ============ 数据加载 ============
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

static void loadBlacklist() {
    g_blacklistedIPs.clear();
    QFile f(blacklistFilePath());
    if (f.open(QIODevice::ReadOnly)) {
        QJsonArray arr = QJsonDocument::fromJson(f.readAll()).array();
        for (const auto& ip : arr) g_blacklistedIPs.insert(ip.toString());
        f.close();
    }
}

static void saveBlacklist() {
    QJsonArray arr;
    for (const QString& ip : g_blacklistedIPs) arr.append(ip);
    QDir().mkpath(QFileInfo(blacklistFilePath()).absolutePath());
    QFile f(blacklistFilePath());
    if (f.open(QIODevice::WriteOnly)) { f.write(QJsonDocument(arr).toJson()); f.close(); }
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

// ============ UI 辅助 ============
static QString htmlEscape(const QString& s) { return s.toHtmlEscaped(); }

static QString pageShell(const QString& title, const QString& user, const QString& body, bool isRoot, bool canW, bool canR, const QString& curDir = "") {
    QString html = R"(<!DOCTYPE html>
<html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no">
<title>)" + htmlEscape(title) + R"(</title>
<style>
:root{--bg:#0a0a0a;--card:#141414;--border:#2a2a2a;--text:#e0e0e0;--text-muted:#888;--primary:#00d2ff;--danger:#ff4757;--success:#2ed573;--radius:12px;}
*{box-sizing:border-box;margin:0;padding:0;transition:all 0.2s ease;}
body{background:var(--bg);color:var(--text);font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,sans-serif;padding:16px;line-height:1.6;font-size:14px;}
a{color:var(--primary);text-decoration:none;}
a:hover{opacity:0.8;}
h1{font-size:20px;margin-bottom:16px;}
.topbar{display:flex;align-items:center;justify-content:space-between;gap:12px;flex-wrap:wrap;margin-bottom:24px;padding:16px;background:var(--card);border-radius:var(--radius);box-shadow:0 4px 12px rgba(0,0,0,0.2);}
.brand{font-size:20px;color:var(--primary);font-weight:700;letter-spacing:1px;}
.userbox{font-size:13px;color:var(--text-muted);display:flex;align-items:center;gap:12px;}
.userbox b{color:var(--text);}
.crumb{color:var(--text-muted);font-size:13px;margin-bottom:16px;word-break:break-all;}
.toolbar{display:flex;gap:10px;margin-bottom:16px;flex-wrap:wrap;align-items:center;}
.btn{background:#222;color:var(--text);border:1px solid var(--border);padding:10px 16px;border-radius:8px;cursor:pointer;font-size:13px;display:inline-flex;align-items:center;justify-content:center;font-weight:500;}
.btn:hover{background:#333;transform:translateY(-1px);}
.btn-primary{background:var(--primary);color:#000;border:none;font-weight:600;}
.btn-primary:hover{background:#00b8e6;box-shadow:0 4px 12px rgba(0,210,255,0.3);}
.btn-danger{background:var(--danger);color:#fff;border:none;}
.btn-danger:hover{background:#e04050;box-shadow:0 4px 12px rgba(255,71,87,0.3);}
.btn-sm{padding:6px 12px;font-size:12px;}
table{width:100%;border-collapse:separate;border-spacing:0 8px;font-size:13px;}
th{color:var(--text-muted);font-weight:500;font-size:12px;text-transform:uppercase;text-align:left;padding:0 16px 8px;}
tr td{background:var(--card);padding:12px 16px;border-top:1px solid var(--border);border-bottom:1px solid var(--border);}
tr td:first-child{border-left:1px solid var(--border);border-top-left-radius:var(--radius);border-bottom-left-radius:var(--radius);}
tr td:last-child{border-right:1px solid var(--border);border-top-right-radius:var(--radius);border-bottom-right-radius:var(--radius);}
tr:hover td{background:#1a1a1a;}
.icon{margin-right:8px;font-size:16px;}
.muted{color:var(--text-muted);font-size:12px;}
.empty{color:var(--text-muted);text-align:center;padding:40px;font-style:italic;}
.card{background:var(--card);border:1px solid var(--border);border-radius:var(--radius);padding:20px;margin-top:20px;}
.card h3{color:var(--primary);font-size:16px;margin-bottom:16px;font-weight:600;display:flex;align-items:center;gap:8px;}
input[type=text],input[type=password],input[type=file],textarea,select{background:#050505;color:var(--text);border:1px solid var(--border);padding:12px 16px;border-radius:8px;font-size:14px;width:100%;max-width:320px;font-family:inherit;}
input:focus,textarea:focus,select:focus{outline:none;border-color:var(--primary);box-shadow:0 0 0 3px rgba(0,210,255,0.1);}
textarea{width:100%;max-width:100%;min-height:400px;font-family:ui-monospace,SFMono-Regular,Menlo,Monaco,Consolas,monospace;font-size:14px;resize:vertical;line-height:1.5;}
label{display:block;color:var(--text-muted);font-size:12px;margin-bottom:8px;}
.form-row{display:flex;gap:12px;align-items:center;flex-wrap:wrap;background:#111;padding:16px;border-radius:8px;border:1px solid var(--border);}
.msg{padding:12px 16px;border-radius:8px;margin-bottom:20px;font-size:13px;font-weight:500;}
.msg-err{background:rgba(255,71,87,0.1);color:#ff6b81;border:1px solid rgba(255,71,87,0.2);}
.msg-ok{background:rgba(46,213,115,0.1);color:#2ed573;border:1px solid rgba(46,213,115,0.2);}
.badge{display:inline-block;padding:4px 8px;border-radius:6px;font-size:11px;font-weight:700;}
.badge-root{background:rgba(0,210,255,0.15);color:var(--primary);}
.badge-user{background:#333;color:#ccc;}
.badge-r{background:#1b5e20;color:#a5d6a7;}
.badge-w{background:#e65100;color:#ffcc80;}
.badge-x{background:rgba(156, 39, 176, 0.2);color:#ce93d8;}
.badge-online{background:rgba(46,213,115,0.2);color:var(--success);box-shadow:0 0 8px rgba(46,213,115,0.3);}
.badge-offline{background:#333;color:#888;}
.checkbox{width:18px;height:18px;cursor:pointer;accent-color:var(--primary);margin:0;}
.searchbox{display:flex;gap:10px;margin-bottom:20px;flex-wrap:wrap;}
.upload-progress{width:100%;height:8px;background:#222;border-radius:4px;margin-top:12px;display:none;overflow:hidden;}
.upload-bar{height:100%;background:var(--primary);width:0%;transition:width 0.1s linear;border-radius:4px;}
.toast{position:fixed;top:20px;left:50%;transform:translateX(-50%);background:var(--card);color:var(--text);padding:12px 24px;border-radius:8px;box-shadow:0 8px 24px rgba(0,0,0,0.5);border:1px solid var(--border);display:none;z-index:9999;}
</style></head><body>
<div class='topbar'>
<div class='brand'>Termail Cloud</div>
<div class='userbox'>)";

    if (!user.isEmpty()) {
        html += QString("<span>当前: <b>%1</b></span> ").arg(htmlEscape(user));
        if (isRoot) html += "<span class='badge badge-root'>ROOT</span>";
        
        // 使用 span 包裹，方便 JS 局部刷新
        html += " <span id='online-count' class='badge badge-online'>0 人在线</span>";

        if (isRoot) html += " <a href='/admin/users' style='margin-left:16px;font-weight:bold;'>控制台</a>";
        html += " <a href='/logout' style='margin-left:16px;color:var(--danger);'>退出</a>";
    }

    html += R"(</div></div>
<div id='toast' class='toast'></div>
<script>
    // 静默轮询在线人数，绝不刷新页面
    setInterval(() => {
        fetch('/api/online_count')
            .then(r => r.text())
            .then(count => {
                const el = document.getElementById('online-count');
                if (el) el.innerText = count + ' 人在线';
            }).catch(() => {});
    }, 5000);
</script>
)";
    html += body + "\n</body></html>\n";
    return html;
}

// 登录页
static QString loginPage(const QString& error = "") {
    QString html = R"(<!DOCTYPE html>
<html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no">
<title>登录 - Termail</title>
<style>
*{box-sizing:border-box;margin:0;padding:0;}
body{background:#0a0a0a;color:#e0e0e0;font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,sans-serif;display:flex;align-items:center;justify-content:center;min-height:100vh;padding:16px;}
.card{background:#141414;border:1px solid #2a2a2a;border-radius:16px;padding:40px;width:100%;max-width:380px;box-shadow:0 10px 30px rgba(0,0,0,0.5);}
h1{color:#00d2ff;font-size:28px;margin-bottom:8px;text-align:center;font-weight:800;letter-spacing:1px;}
.sub{color:#888;font-size:14px;text-align:center;margin-bottom:32px;}
label{display:block;color:#888;font-size:13px;margin-bottom:8px;margin-top:20px;}
input{background:#050505;color:#e0e0e0;border:1px solid #2a2a2a;padding:14px;border-radius:8px;font-size:15px;width:100%;transition:all 0.2s;}
input:focus{outline:none;border-color:#00d2ff;box-shadow:0 0 0 3px rgba(0,210,255,0.1);}
button{background:#00d2ff;color:#000;border:none;padding:14px;border-radius:8px;font-size:16px;font-weight:700;width:100%;margin-top:32px;cursor:pointer;transition:all 0.2s;}
button:hover{background:#00b8e6;transform:translateY(-2px);box-shadow:0 6px 20px rgba(0,210,255,0.3);}
.msg-err{background:rgba(255,71,87,0.1);color:#ff6b81;border:1px solid rgba(255,71,87,0.2);padding:12px;border-radius:8px;margin-bottom:20px;font-size:13px;text-align:center;}
</style></head><body>
<div class='card'>
<h1>Termail</h1>
<div class='sub'>多用户文件管理服务</div>
)";
    if (!error.isEmpty()) html += QString("<div class='msg-err'>%1</div>\n").arg(htmlEscape(error));
    html += R"(
<form method='post' action='/login'>
<label>用户名</label>
<input type='text' name='username' required autofocus autocomplete='username'>
<label>密码</label>
<input type='password' name='password' required autocomplete='current-password'>
<button type='submit'>安全登录</button>
</form>
</div>
</body></html>
)";
    return html;
}

static QString generateIndexHtml(const QString& user, const QString& currentPath, const QString& dirPath, bool isRoot, bool canR, bool canW, bool canX, const QString& search, const QString& baseDir) {
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
    body += QString("<div class='crumb'>📍 路径: %1</div>\n").arg(currentPath.isEmpty() ? "/" : htmlEscape(currentPath));

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
    if (canW) body += "<th style='width:40px'><input type='checkbox' class='checkbox' id='chkAll' onclick=\"toggleAll(this)\"></th>\n";
    body += "<th>文件名称</th><th style='width:100px'>大小</th><th style='width:160px'>修改时间</th><th style='width:280px'>操作</th>\n";
    body += "</tr></thead>\n<tbody>\n";

    if (files.isEmpty()) {
        body += QString("<tr><td colspan='5' class='empty'>%1</td></tr>\n").arg(search.isEmpty() ? "空目录" : "无匹配文件");
    } else {
        for (const QString& name : files) {
            QFileInfo info(dir.absoluteFilePath(name));
            QString icon = info.isDir() ? "📁" : "📄";
            QString size = info.isDir() ? "-" : Utils::formatSize(info.size());
            QString mtime = info.lastModified().toString("yyyy-MM-dd hh:mm");
            QString href = info.isDir() 
                ? (currentPath + (currentPath.endsWith('/') ? "" : "/") + name + "/")
                : ("/download?path=" + QUrl::toPercentEncoding(currentPath + (currentPath.endsWith('/') ? "" : "/") + name));

            body += "<tr>\n";
            if (canW) body += QString("<td><input type='checkbox' class='checkbox rowchk' value='%1'></td>\n").arg(htmlEscape(name));
            body += QString("<td><span class='icon'>%1</span>").arg(icon);
            if (info.isDir() || canR) {
                body += QString("<a href='%1' style='font-weight:500;'>%2%3</a>").arg(href, htmlEscape(name), info.isDir() ? "/" : "");
            } else {
                body += htmlEscape(name);
            }
            body += "</td>\n";
            body += QString("<td class='muted'>%1</td>\n").arg(size);
            body += QString("<td class='muted'>%1</td>\n").arg(mtime);
            body += "<td>";
            if (!info.isDir() && canR) {
                QString lowerName = name.toLower();
                if (lowerName.endsWith(".html") || lowerName.endsWith(".htm")) {
                    if (canX || isRoot) {
                        body += QString("<a class='btn btn-sm' style='background:#9c27b0;color:#fff;border:none;' href='/run?path=%1' target='_blank'>运行</a> ").arg(QUrl::toPercentEncoding(currentPath + (currentPath.endsWith('/') ? "" : "/") + name));
                    } else {
                        body += QString("<button class='btn btn-sm' style='background:#333;color:#666;border:none;cursor:not-allowed;' title='无运行权限'>运行</button> ");
                    }
                }
                body += QString("<a class='btn btn-sm' href='/view?path=%1'>预览</a> ").arg(QUrl::toPercentEncoding(currentPath + (currentPath.endsWith('/') ? "" : "/") + name));
                body += QString("<a class='btn btn-sm' href='%1' download>下载</a> ").arg(href);
            }
            if (canW) {
                body += QString("<button class='btn btn-sm' style='background:#444;' onclick=\"renameFile('%1')\">重命名</button> ").arg(htmlEscape(name).replace("'", "\\'"));
                body += QString("<button class='btn btn-sm' style='background:#444;' onclick=\"moveFile('%1')\">移动</button> ").arg(htmlEscape(name).replace("'", "\\'"));
                body += QString("<button class='btn btn-danger btn-sm' onclick=\"delFile('%1')\">删除</button>").arg(htmlEscape(name).replace("'", "\\'"));
            }
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
<input type='file' name='file' id='fileInput' required style='margin-bottom:16px;max-width:100%;'>
<div class='upload-progress' id='progBar'><div class='upload-bar' id='progFill'></div></div>
<button type='submit' class='btn btn-primary'>开始上传</button>
</form>
</div>

<div class='card'>
<h3>📁 新建</h3>
<form action='/mkdir' method='post' class='form-row'>
<input type='hidden' name='cur_dir' value='%1'>
<input type='text' name='name' placeholder='文件夹名' required>
<button type='submit' class='btn btn-primary'>新建目录</button>
</form>
<form action='/touch' method='post' class='form-row' style='margin-top:12px'>
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
function renameFile(name){
  const newName = prompt('请输入新名称 (包含后缀):', name);
  if(!newName || newName === name) return;
  fetch('/rename',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'old_name='+encodeURIComponent(name)+'&new_name='+encodeURIComponent(newName)+'&cur_dir='+encodeURIComponent('%1')}).then(()=>location.reload());
}
function moveFile(name){
  const dest = prompt('请输入目标相对路径 (例如: /subdir):', '/');
  if(!dest) return;
  fetch('/move',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'file_name='+encodeURIComponent(name)+'&dest_dir='+encodeURIComponent(dest)+'&cur_dir='+encodeURIComponent('%1')}).then(()=>location.reload());
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
  form.method = 'POST'; form.action = '/batch-download'; form.target = '_blank';
  const input1 = document.createElement('input'); input1.type = 'hidden'; input1.name = 'paths'; input1.value = names.join('\n');
  const input2 = document.createElement('input'); input2.type = 'hidden'; input2.name = 'cur_dir'; input2.value = '%1';
  form.appendChild(input1); form.appendChild(input2);
  document.body.appendChild(form); form.submit(); document.body.removeChild(form);
}
document.getElementById('uploadForm').onsubmit = function(e) {
    e.preventDefault();
    const file = document.getElementById('fileInput').files[0];
    if(!file) return;
    const fd = new FormData();
    fd.append('file', file); fd.append('cur_dir', '%1');
    const xhr = new XMLHttpRequest();
    xhr.open('POST', '/upload', true);
    document.getElementById('progBar').style.display = 'block';
    xhr.upload.onprogress = function(e) {
        if (e.lengthComputable) document.getElementById('progFill').style.width = (e.loaded / e.total) * 100 + '%';
    };
    xhr.onload = function() { location.reload(); };
    xhr.send(fd);
};
</script>
)").arg(htmlEscape(currentPath));
    }
    return pageShell("文件管理", user, body, isRoot, canW, canR, currentPath);
}

static QString adminPage(const QString& user, const QMap<QString, HttpUser>& users, const QString& msg = "", bool isErr = false) {
    QString body;
    
    // 顶部工具栏（带返回按钮）
    body += R"(
<div class='toolbar'>
    <a class='btn' href='/' style='background:#333;'>← 返回文件管理</a>
</div>
)";

    if (!msg.isEmpty()) body += QString("<div class='msg %1'>%2</div>\n").arg(isErr ? "msg-err" : "msg-ok", htmlEscape(msg));

    // 模块一：在线状态
    qint64 now = QDateTime::currentSecsSinceEpoch();
    body += "<div class='card'><h3>🌐 实时在线用户状态</h3><table><thead><tr><th>用户</th><th>状态</th><th>最后活跃时间</th></tr></thead><tbody>\n";
    for (auto it = users.begin(); it != users.end(); ++it) {
        QString uName = it.key();
        qint64 lastTime = g_onlineUsers.value(uName, 0);
        bool isOnline = (now - lastTime < 30) && (lastTime > 0);
        
        QString statusBadge = isOnline ? "<span class='badge badge-online'>在线</span>" : "<span class='badge badge-offline'>离线</span>";
        QString timeStr = lastTime > 0 ? QDateTime::fromSecsSinceEpoch(lastTime).toString("hh:mm:ss") : "-";
        
        body += QString("<tr><td><b>%1</b></td><td>%2</td><td class='muted'>%3</td></tr>\n").arg(htmlEscape(uName), statusBadge, timeStr);
    }
    body += "</tbody></table></div>\n";

    // 模块二：黑名单
    body += R"(
<div class='card'>
<h3>🛡️ 黑名单 IP 管理</h3>
<form method='post' action='/admin/blacklist' class='form-row'>
<input type='text' name='ip' placeholder='输入要封禁的 IP (如 192.168.1.5)' required style='width:220px;'>
<button type='submit' class='btn btn-danger'>加入黑名单</button>
<button type='submit' formaction='/admin/unblacklist' class='btn'>移出黑名单</button>
</form>
<div style='margin-top:12px;display:flex;gap:8px;flex-wrap:wrap;'>
)";
    for (const QString& ip : g_blacklistedIPs) {
        body += QString("<span class='badge badge-offline' style='padding:6px 12px;'>%1</span>\n").arg(htmlEscape(ip));
    }
    body += "</div></div>\n";

    // 模块三：日志
    body += R"(
<div class='card'>
<h3>📜 系统日志查看 (仅限 Root)</h3>
<form method='get' action='/admin/logs' class='form-row'>
<input type='text' name='date' placeholder='日期 (如 2026-11-13，留空看最新)' style='width:200px;'>
<button type='submit' class='btn btn-primary'>查看日志</button>
</form>
</div>
)";

    // 模块四：账户管理
    body += R"(
<div class='card'>
<h3>➕ 创建普通账户</h3>
<form method='post' action='/admin/create' class='form-row'>
<input type='text' name='username' placeholder='用户名' required>
<input type='password' name='password' placeholder='密码' required>
<label style='display:flex;align-items:center;gap:6px;margin:0;color:var(--text);'><input type='checkbox' name='r' checked class='checkbox'> 读</label>
<label style='display:flex;align-items:center;gap:6px;margin:0;color:var(--text);'><input type='checkbox' name='w' class='checkbox'> 写</label>
<label style='display:flex;align-items:center;gap:6px;margin:0;color:var(--text);'><input type='checkbox' name='x' class='checkbox'> 运行</label>
<input type='password' name='root_pw' placeholder='当前 root 密码' required style='max-width:180px'>
<button type='submit' class='btn btn-primary'>创建账户</button>
</form>
</div>

<div class='card'>
<h3>🛡️ Root 修改自身密码</h3>
<form method='post' action='/admin/passwd' class='form-row'>
<input type='hidden' name='username' value='root'>
<input type='password' name='password' placeholder='输入新的 Root 密码' required style='max-width:240px'>
<button type='submit' class='btn btn-danger'>修改 Root 密码</button>
</form>
</div>

<div class='card'>
<h3>👥 用户管理控制台</h3>
<table>
<thead><tr><th>用户</th><th>类型/权限</th><th>统一操作面板</th></tr></thead>
<tbody>
)";

    for (auto it = users.begin(); it != users.end(); ++it) {
        const QString& name = it.key();
        const HttpUser& u = it.value();
        body += "<tr>\n";
        body += QString("<td><b style='font-size:15px;'>%1</b></td>\n").arg(htmlEscape(name));
        body += "<td>";
        if (u.isRoot) body += "<span class='badge badge-root'>ROOT</span>";
        else {
            body += "<span class='badge badge-user'>USER</span><br><span class='muted' style='margin-top:4px;display:inline-block;'>";
            if (u.r) body += "<span class='badge badge-r'>R</span> "; 
            if (u.w) body += "<span class='badge badge-w'>W</span> "; 
            if (u.x) body += "<span class='badge badge-x'>X</span>";
            if (!u.r && !u.w && !u.x) body += "无权限";
            body += "</span>";
        }
        body += "</td>\n<td>";

        if (name == "root" && u.isRoot) {
            body += "<span class='muted'>Root账户受最高保护，不可在此操作</span>";
        } else {
            body += QString("<form method='post' action='/admin/manage' style='display:flex;gap:8px;flex-wrap:wrap;align-items:center;background:transparent;border:none;padding:0;'>"
                "<input type='hidden' name='username' value='%1'>"
                "<input type='password' name='root_pw' placeholder='输入 root 密码验证' required style='width:160px'>"
                "<select name='action' style='width:auto;padding:8px;'>"
                "  <option value='grant'>改权限 (R/W/X)</option>"
                "  <option value='passwd'>修改密码</option>"
                "  <option value='promote'>授予ROOT</option>"
                "  <option value='revoke'>撤销ROOT</option>"
                "  <option value='delete'>删除账户</option>"
                "</select>"
                "<input type='text' name='new_val' placeholder='新值 (如 rwx 或 新密码)' style='width:160px;'>"
                "<button type='submit' class='btn btn-sm btn-primary'>执行操作</button>"
                "</form>").arg(htmlEscape(name));
        }
        body += "</td>\n</tr>\n";
    }
    body += "</tbody></table></div>\n";
    return pageShell("控制台", user, body, true, true, true);
}

static void sendResponse(QTcpSocket* client, int code, const QString& contentType, const QByteArray& body, const QString& extraHeaders = "") {
    QString status = code == 200 ? "200 OK" : code == 302 ? "302 Found" : code == 401 ? "401 Unauthorized" : code == 403 ? "403 Forbidden" : code == 404 ? "404 Not Found" : code == 400 ? "400 Bad Request" : "500 Error";
    QByteArray resp = "HTTP/1.1 " + status.toUtf8() + "\r\n";
    resp += "Content-Type: " + contentType.toUtf8() + "\r\n";
    resp += "Content-Length: " + QByteArray::number(body.size()) + "\r\n";
    if (!extraHeaders.isEmpty()) resp += extraHeaders.toUtf8() + "\r\n";
    resp += "Connection: close\r\n\r\n";
    resp += body;
    client->write(resp); client->flush(); client->waitForBytesWritten(5000); client->close();
}

static void redirect(QTcpSocket* client, const QString& location, const QString& extraHeaders = "") {
    QByteArray resp = "HTTP/1.1 302 Found\r\n";
    resp += "Location: " + location.toUtf8() + "\r\n";
    if (!extraHeaders.isEmpty()) resp += extraHeaders.toUtf8() + "\r\n";
    resp += "Content-Length: 0\r\nConnection: close\r\n\r\n";
    client->write(resp); client->flush(); client->waitForBytesWritten(3000); client->close();
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

static void handleRequest(QTcpSocket* client, const QString& request, TerminalWidget* ui) {
    QString ip = client->peerAddress().toString();
    if (ip.startsWith("::ffff:")) ip = ip.mid(7);

    if (g_blacklistedIPs.contains(ip)) {
        sendResponse(client, 403, "text/plain; charset=utf-8", "403 Forbidden: Your IP has been blacklisted.");
        return;
    }

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

    if (urlPath != "/favicon.ico" && urlPath != "/api/online_count") {
        QString logMsg = QString("IP:%1 %2 %3 (user=%4)").arg(ip, method, rawPath, username.isEmpty() ? "-" : username);
        logRequest(logMsg);
    }

    if (!username.isEmpty()) {
        g_onlineUsers[username] = QDateTime::currentSecsSinceEpoch();
    }

    // === API：静默获取在线人数 ===
    if (urlPath == "/api/online_count" && method == "GET") {
        qint64 now = QDateTime::currentSecsSinceEpoch();
        int count = 0;
        for (auto it = g_onlineUsers.begin(); it != g_onlineUsers.end(); ++it) {
            if (now - it.value() < 30) count++;
        }
        sendResponse(client, 200, "text/plain", QString::number(count).toUtf8());
        return;
    }

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
            g_onlineUsers[u] = QDateTime::currentSecsSinceEpoch();
            redirect(client, "/", "Set-Cookie: sid=" + token + "; Path=/; HttpOnly");
            return;
        }
        sendResponse(client, 200, "text/html; charset=utf-8", loginPage("用户名或密码错误").toUtf8());
        return;
    }
    if (urlPath == "/logout") {
        QString sid = extractSid(headers);
        if (!sid.isEmpty()) {
            QString u = g_sessions.take(sid);
            g_onlineUsers.remove(u);
        }
        redirect(client, "/login", "Set-Cookie: sid=; Path=/; Max-Age=0");
        return;
    }

    if (username.isEmpty()) { redirect(client, "/login"); return; }

    const HttpUser& me = users[username];
    bool isRoot = me.isRoot;
    bool canR = isRoot || me.r;
    bool canW = isRoot || me.w;
    bool canX = isRoot || me.x;
    QString baseDir = ui->currentDir;

    if (method == "POST" && urlPath != "/login") {
        QMap<QString, QString> form = parseFormBody(body);
        
        if (!isRoot) {
            QString reqPath = form.value("path") + form.value("cur_dir") + form.value("paths") + form.value("old_name") + form.value("new_name");
            if (reqPath.contains("http_users.json") || reqPath.contains("serve-log") || reqPath.contains("http_blacklist.json")) {
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

        if (urlPath == "/rename") {
            if (!canW) { sendResponse(client, 403, "text/plain", "无写权限"); return; }
            QString curD = form.value("cur_dir", "/");
            QString oldName = form.value("old_name");
            QString newName = form.value("new_name");
            QString oldPath = QDir(baseDir).absoluteFilePath(curD.mid(1)) + "/" + oldName;
            QString newPath = QDir(baseDir).absoluteFilePath(curD.mid(1)) + "/" + newName;
            if (QFile::exists(oldPath)) {
                QFile::rename(oldPath, newPath);
                logRequest(QString("[HTTP] 重命名: %1 -> %2").arg(oldName, newName));
            }
            sendResponse(client, 200, "text/plain", "OK");
            return;
        }

        if (urlPath == "/move") {
            if (!canW) { sendResponse(client, 403, "text/plain", "无写权限"); return; }
            QString curD = form.value("cur_dir", "/");
            QString fileName = form.value("file_name");
            QString destDir = form.value("dest_dir");
            
            QString srcPath = QDir(baseDir).absoluteFilePath(curD.mid(1)) + "/" + fileName;
            QString dstPath = QDir(baseDir).absoluteFilePath(destDir.mid(1)) + "/" + fileName;
            
            QDir().mkpath(QFileInfo(dstPath).absolutePath());
            if (QFile::exists(srcPath)) {
                if (QFile::exists(dstPath)) QFile::remove(dstPath);
                QFile::rename(srcPath, dstPath);
                logRequest(QString("[HTTP] 移动文件: %1 -> %2").arg(fileName, destDir));
            }
            sendResponse(client, 200, "text/plain", "OK");
            return;
        }

        if (urlPath == "/mkdir" || urlPath == "/touch") {
            if (!canW) { sendResponse(client, 403, "text/plain", "无写权限"); return; }
            QString curD = form.value("cur_dir", "/");
            QString name = form.value("name");
            QString fullPath = QDir(baseDir).absoluteFilePath(curD.mid(1)) + "/" + name;
            
            if (urlPath == "/mkdir") {
                QDir().mkpath(fullPath);
                logRequest(QString("[HTTP] 新建目录: %1").arg(name));
            } else {
                QFile f(fullPath);
                if (!f.exists()) { f.open(QIODevice::WriteOnly); f.close(); }
                logRequest(QString("[HTTP] 新建文件: %1").arg(name));
            }
            redirect(client, curD);
            return;
        }

        if (urlPath == "/upload") {
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
                                if (f.open(QIODevice::WriteOnly)) { 
                                    f.write(bodyBytes.mid(dataStart, dataEnd - dataStart)); 
                                    f.close(); 
                                    logRequest(QString("[HTTP] 上传文件: %1 (%2 KB)").arg(filename).arg(f.size()/1024));
                                }
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

            if (method == "POST" && urlPath == "/admin/blacklist") {
                QString ip = form.value("ip").trimmed();
                if (!ip.isEmpty()) {
                    g_blacklistedIPs.insert(ip);
                    saveBlacklist();
                    logRequest(QString("[HTTP] 封禁IP: %1").arg(ip));
                }
                redirect(client, "/admin/users");
                return;
            }
            if (method == "POST" && urlPath == "/admin/unblacklist") {
                QString ip = form.value("ip").trimmed();
                if (!ip.isEmpty()) {
                    g_blacklistedIPs.remove(ip);
                    saveBlacklist();
                    logRequest(QString("[HTTP] 解封IP: %1").arg(ip));
                }
                redirect(client, "/admin/users");
                return;
            }
            if (method == "POST" && urlPath == "/admin/passwd") {
                QString u = form.value("username");
                QString p = form.value("password");
                if (!users.contains(u) || p.isEmpty()) { redirect(client, "/admin/users"); return; }
                users[u].salt = randomSalt();
                users[u].hash = hashPassword(users[u].salt, p);
                saveUsers(users);
                logRequest(QString("[HTTP] Root密码已更新"));
                sendResponse(client, 200, "text/html; charset=utf-8", adminPage(username, users, "Root 密码修改成功").toUtf8());
                return;
            }

            QString rootPw = form.value("root_pw");
            if (hashPassword(users["root"].salt, rootPw) != users["root"].hash) {
                sendResponse(client, 200, "text/html; charset=utf-8", adminPage(username, users, "root 密码验证错误", true).toUtf8());
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
                logRequest(QString("[HTTP] 创建用户: %1").arg(u));
                sendResponse(client, 200, "text/html; charset=utf-8", adminPage(username, users, QString("账户 %1 已创建").arg(u)).toUtf8());
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
                logRequest(QString("[HTTP] 用户管理操作: %1 -> %2").arg(u, action));
                sendResponse(client, 200, "text/html; charset=utf-8", adminPage(username, users, "操作已执行").toUtf8());
                return;
            }
        }
    }

    if (method == "GET" && (urlPath == "/" || urlPath.isEmpty())) {
        QString html = generateIndexHtml(username, "/", baseDir, isRoot, canR, canW, canX, query.value("q", ""), baseDir);
        sendResponse(client, 200, "text/html; charset=utf-8", html.toUtf8());
        return;
    }

    if (method == "GET" && urlPath.endsWith("/")) {
        if (!canR) { sendResponse(client, 403, "text/plain", "无读权限"); return; }
        if (QDir(QDir(baseDir).absoluteFilePath(urlPath.mid(1))).exists()) {
            QString html = generateIndexHtml(username, urlPath, QDir(baseDir).absoluteFilePath(urlPath.mid(1)), isRoot, canR, canW, canX, query.value("q", ""), baseDir);
            sendResponse(client, 200, "text/html; charset=utf-8", html.toUtf8());
        } else {
            sendResponse(client, 404, "text/plain", "Not Found");
        }
        return;
    }

    if (method == "GET" && (urlPath == "/download" || urlPath == "/raw")) {
        if (!canR) { sendResponse(client, 403, "text/plain", "无读权限"); return; }
        QString filePath = query.value("path");
        QString fullPath = QDir(baseDir).absoluteFilePath(filePath.mid(1));
        QFileInfo info(fullPath);
        
        if (info.exists() && info.isFile()) {
            QFile f(fullPath);
            if (f.open(QIODevice::ReadOnly)) {
                QString contentType = "application/octet-stream";
                if (urlPath == "/raw") {
                    QString ext = info.suffix().toLower();
                    if (ext == "jpg" || ext == "jpeg") contentType = "image/jpeg";
                    else if (ext == "png") contentType = "image/png";
                    else if (ext == "gif") contentType = "image/gif";
                    else if (ext == "webp") contentType = "image/webp";
                    else if (ext == "mp4") contentType = "video/mp4";
                    else if (ext == "mp3") contentType = "audio/mpeg";
                }
                sendResponse(client, 200, contentType, f.readAll());
                f.close();
                if (urlPath == "/download") logRequest(QString("[HTTP] 下载文件: %1").arg(info.fileName()));
            } else {
                sendResponse(client, 500, "text/plain", "Read Error");
            }
        } else {
            sendResponse(client, 404, "text/plain", "Not Found");
        }
        return;
    }

    if (method == "GET" && urlPath == "/run") {
        if (!canR) { sendResponse(client, 403, "text/plain", "无读权限"); return; }
        if (!canX && !isRoot) { 
            sendResponse(client, 403, "text/html; charset=utf-8", "<h3>403 Forbidden</h3><p>您没有运行 HTML 的权限 (需要 X 权限)。</p>"); 
            return; 
        }
        QString rel = query.value("path");
        QString full = QDir(baseDir).absoluteFilePath(rel.mid(1));
        QFileInfo info(full);
        if (!info.exists() || !info.isFile()) { sendResponse(client, 404, "text/plain", "文件不存在"); return; }
        
        QFile f(full);
        if (f.open(QIODevice::ReadOnly)) {
            QByteArray htmlContent = f.readAll();
            f.close();
            sendResponse(client, 200, "text/html; charset=utf-8", htmlContent);
            logRequest(QString("[HTTP] 运行 HTML: %1").arg(info.fileName()));
        } else {
            sendResponse(client, 500, "text/plain", "读取失败");
        }
        return;
    }

    if (method == "GET" && urlPath == "/view") {
        if (!canR) { sendResponse(client, 403, "text/plain", "无读权限"); return; }
        QString rel = query.value("path");
        QString full = QDir(baseDir).absoluteFilePath(rel.mid(1));
        QFileInfo info(full);
        if (!info.exists() || !info.isFile()) { sendResponse(client, 404, "text/plain", "文件不存在"); return; }
        
        QString ext = info.suffix().toLower();
        if (ext == "jpg" || ext == "jpeg" || ext == "png" || ext == "gif" || ext == "webp" || ext == "mp4") {
            QString rawUrl = "/raw?path=" + QUrl::toPercentEncoding(rel);
            QString body = QString("<div class='crumb'>%1</div>\n").arg(htmlEscape(rel.mid(1)));
            if (ext == "mp4") body += QString("<video src='%1' controls autoplay style='max-width:100%%;border-radius:12px;'></video>\n").arg(rawUrl);
            else body += QString("<img src='%1' class='preview-img'>\n").arg(rawUrl);
            body += "<br><a class='btn' href='/'>返回列表</a>";
            sendResponse(client, 200, "text/html; charset=utf-8", pageShell("媒体预览", username, body, isRoot, canW, true).toUtf8());
            return;
        }

        if (info.size() > 1024 * 1024) { sendResponse(client, 200, "text/plain; charset=utf-8", "文件过大，无法预览（>1MB）"); return; }
        QFile f(full);
        if (!f.open(QIODevice::ReadOnly)) { sendResponse(client, 500, "text/plain", "读取失败"); return; }
        QByteArray fileData = f.readAll();
        f.close();
        QString content = QString::fromUtf8(fileData);
        if (content.contains(QChar(0xFFFD))) content = QString::fromLocal8Bit(fileData);
        
        QString body = QString("<div class='crumb'>📄 编辑文件: %1</div>\n").arg(htmlEscape(rel.mid(1)));
        body += "<form method='post' action='/save'>\n";
        body += QString("<input type='hidden' name='path' value='%1'>\n").arg(htmlEscape(rel));
        body += QString("<textarea name='content' %1>%2</textarea>\n").arg(canW ? "" : "readonly", htmlEscape(content));
        body += "<div class='toolbar' style='margin-top:20px'>\n";
        if (canW) body += "<button type='submit' class='btn btn-primary'>保存修改</button>\n";
        body += "<a class='btn' href='/'>取消返回</a>\n</div></form>\n";
        sendResponse(client, 200, "text/html; charset=utf-8", pageShell("编辑文件", username, body, isRoot, canW, true).toUtf8());
        return;
    }

    if (urlPath == "/admin/users" && method == "GET") {
        if (!isRoot) { sendResponse(client, 403, "text/plain", "需要 root 权限"); return; }
        sendResponse(client, 200, "text/html; charset=utf-8", adminPage(username, users).toUtf8());
        return;
    }

    if (urlPath == "/admin/logs" && method == "GET") {
        if (!isRoot) { sendResponse(client, 403, "text/plain", "需要 root 权限"); return; }
        QString date = query.value("date");
        if (date.isEmpty()) date = QDate::currentDate().toString("yyyy-MM-dd");
        QString logPath = logDirPath() + "/" + date + ".log";
        QFile f(logPath);
        QString content = "暂无日志或文件不存在";
        if (f.open(QIODevice::ReadOnly)) { content = QString::fromUtf8(f.readAll()); f.close(); }
        
        QString body = QString("<div class='crumb'>📜 日志查看: %1</div>\n").arg(htmlEscape(date));
        body += "<div class='card'><pre style='white-space:pre-wrap;font-family:monospace;color:#0f0;background:#000;padding:20px;border-radius:8px;overflow:auto;max-height:70vh;'>" + htmlEscape(content) + "</pre></div>";
        body += "<a class='btn' href='/admin/users'>返回控制台</a>";
        sendResponse(client, 200, "text/html; charset=utf-8", pageShell("日志", username, body, true, true, true).toUtf8());
        return;
    }

    sendResponse(client, 404, "text/plain", "Not Found");
}

void CommandRegistry::registerNetwork2Commands(TerminalWidget* ui) {
    registerCommand("serve", [ui](const QStringList& args) {
        int port = 8080;
        if (!args.isEmpty()) {
            bool ok; int p = args[0].toInt(&ok);
            if (ok && p > 0 && p < 65536) port = p;
        }

        if (g_httpServer && g_httpServer->isListening()) {
            ui->appendOutput(QString("服务器已在端口 %1 运行\n").arg(g_httpServer->serverPort()), ERROR_COLOR);
            return;
        }

        loadUsers();
        loadBlacklist();
        g_httpServer = new QTcpServer();
        g_httpUi = ui;

        QTimer* timer = new QTimer(g_httpServer);
        QObject::connect(timer, &QTimer::timeout, [](){
            qint64 now = QDateTime::currentSecsSinceEpoch();
            for (auto it = g_onlineUsers.begin(); it != g_onlineUsers.end(); ) {
                if (now - it.value() > 300) it = g_onlineUsers.erase(it);
                else ++it;
            }
        });
        timer->start(60000);

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
            ui->appendOutput("默认账户: root / 2510\n", INFO_COLOR);
            ui->appendOutput("日志目录: serve-log/\n", INFO_COLOR);
        } else {
            ui->appendOutput("服务器启动失败\n", ERROR_COLOR);
            delete g_httpServer; g_httpServer = nullptr;
        }
    }, "HTTP文件服务器", "serve [端口]");

    registerCommand("stop-serve", [ui](const QStringList&) {
        if (g_httpServer && g_httpServer->isListening()) {
            g_httpServer->close(); delete g_httpServer; g_httpServer = nullptr;
            g_onlineUsers.clear();
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
            g_onlineUsers.clear();
        }
    }
}
