// ============================================================
// Termail Cloud - HTTP 文件服务器（单文件实现）
// 命令：serve [端口] / stop-serve
// 分批拼接文件，本段为第 1 批：文件头与全局状态
// ============================================================

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
#include <QDirIterator>
#include <QDateTime>
#include <QDate>
#include <QRegularExpression>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QMap>
#include <QSet>
#include <QVector>
#include <QList>
#include <QPair>
#include <QTimer>
#include <QElapsedTimer>
#include <QBuffer>
#include <QByteArray>
#include <QStringList>
#include <algorithm>

namespace {

// ============================================================
// 全局状态
// ============================================================

QTcpServer* g_httpServer = nullptr;
TerminalWidget* g_httpUi = nullptr;

// ---- 权限位 ----
enum class Perm { Read = 1, Write = 2, Exec = 4 };

// ---- 用户 ----
struct HttpUser {
    QString salt;
    QString hash;
    bool isRoot = false;
    bool r = false;
    bool w = false;
    bool x = false;
};

// ---- 会话 ----
struct SessionInfo {
    QString username;
    qint64 createdAt = 0;
    qint64 lastActive = 0;
    QString ip;
};

// ---- 在线用户 ----
struct OnlineInfo {
    qint64 lastSeen = 0;
    QString ip;
};

// ---- 聊天消息 ----
struct ChatMessage {
    qint64 timestamp = 0;
    QString username;
    QString text;
    bool isRoot = false;
};

// ---- 权限请求 ----
enum class RequestState { Pending, Approved, Rejected, Expired };
enum class RequestTarget { Read, Write, Exec, Root };

struct PermissionRequest {
    QString id;
    QString username;
    RequestTarget target = RequestTarget::Read;
    qint64 createdAt = 0;
    qint64 expireAt = 0;
    RequestState state = RequestState::Pending;
    QString handledBy;
    qint64 handledAt = 0;
    QString cooldownUntilKey; // 保留字段，便于后续扩展
};

// ---- 频率限制 ----
struct RateLimitEntry {
    qint64 windowStart = 0;
    int count = 0;
    qint64 banUntil = 0;
};

// ============================================================
// 全局容器
// ============================================================

QMap<QString, HttpUser> g_users;              // 用户名 -> 用户
QMap<QString, QString> g_sessions;            // sid -> 用户名
QMap<QString, SessionInfo> g_sessionInfo;     // sid -> 会话详情
QMap<QString, OnlineInfo> g_onlineUsers;      // 用户名 -> 在线信息
QSet<QString> g_blacklistedIPs;               // IP 黑名单
QMap<QString, RateLimitEntry> g_rateLimits;   // IP -> 频率限制

QVector<ChatMessage> g_chatMessages;          // 聊天记录（内存，最近 N 条）
QSet<QString> g_mutedUsers;                   // 被禁言用户
QMap<QString, QList<PermissionRequest>> g_requests; // 用户名 -> 请求列表
QMap<QString, qint64> g_requestCooldown;      // 用户名 -> 冷却截止时间

bool g_safeMode = false;                      // 安全模式状态（不持久化）
bool g_themeDark = true;                      // 当前主题（默认暗色）

// ============================================================
// 常量
// ============================================================

constexpr int kMaxChatMessages = 200;
constexpr int kOnlineTimeoutSecs = 30;
constexpr int kSessionIdleTimeoutSecs = 3600;
constexpr int kRequestTimeoutSecs = 30;
constexpr int kRequestCooldownSecs = 60;
constexpr int kMaxPreviewBytes = 1024 * 1024;   // 1 MB
constexpr int kMaxDirEntries = 1000;
constexpr int kMaxSearchEntries = 500;
constexpr int kMaxUploadBytes = 512 * 1024 * 1024; // 512 MB 防护上限
constexpr int kRateLimitWindowSecs = 60;
constexpr int kRateLimitMaxRequests = 300;
constexpr int kRateLimitBanSecs = 300;
constexpr int kMaxRequestPerUser = 50;

// 默认端口与 root 初始密码
constexpr int kDefaultPort = 8080;
const char* const kDefaultRootPassword = "2510";

// ============================================================
// 函数前置声明（后续批次实现）
// ============================================================

// --- 工具层 ---
QString usersFilePath();
QString blacklistFilePath();
QString logDirPath();
QString hashPassword(const QString& salt, const QString& password);
QString randomSalt();
QString randomToken();
QString generateId();
void logRequest(const QString& msg);
QString nowString();
QString dateString();
QString htmlEscape(const QString& s);

// --- 路径安全 ---
QString safeJoin(const QString& baseDir, const QString& relPath, bool* ok = nullptr);
bool isInsideBase(const QString& baseDir, const QString& absPath);
bool isSensitivePath(const QString& relPath);
QString normalizedRelPath(const QString& relPath);

// --- 用户存取 ---
QMap<QString, HttpUser> loadUsers();
void saveUsers(const QMap<QString, HttpUser>& users);
void loadBlacklist();
void saveBlacklist();
bool verifyPassword(const HttpUser& user, const QString& password);
bool verifyRootPassword(const QString& password);

// --- 会话 ---
QString extractSid(const QString& headers);
QString currentUser(const QString& headers);
QString createSession(const QString& username, const QString& ip);
void destroySession(const QString& sid);
void destroySessionsOf(const QString& username, const QString& exceptSid = QString());
bool hasActiveSession(const QString& username);

// --- 在线 ---
void touchOnline(const QString& username, const QString& ip);
void removeOnline(const QString& username);
int countOnline();
bool isOnline(const QString& username);

// --- 频率限制 / 防火墙 ---
bool checkRateLimit(const QString& ip);
void cleanupRateLimits();

// --- 权限 ---
bool canRead(const HttpUser& u);
bool canWrite(const HttpUser& u);
bool canExec(const HttpUser& u);
QString permString(const HttpUser& u);

// --- 请求（权限申请） ---
void submitPermissionRequest(const QString& username, RequestTarget target);
void expirePermissionRequests();
QList<PermissionRequest> pendingRequests();
PermissionRequest* findRequest(const QString& username, const QString& id);
void handlePermissionRequest(const QString& username, const QString& id, bool approve, const QString& byWhom);

// --- 聊天室 ---
void addChatMessage(const QString& username, const QString& text, bool isRoot);
QJsonArray chatMessagesJson();

// --- ZIP ---
QByteArray buildZipStore(const QList<QPair<QString, QByteArray>>& files);

// --- HTTP 工具 ---
struct HttpRequest {
    QString method;
    QString rawPath;
    QString urlPath;
    QMap<QString, QString> query;
    QMap<QString, QString> headers; // 小写键
    QByteArray body;
    QString clientIp;
    QString sid;
    QString username;
    bool valid = false;
};

HttpRequest parseHttpRequest(const QByteArray& data, const QString& clientIp);
void sendResponse(QTcpSocket* client, int code, const QString& contentType,
                  const QByteArray& body, const QString& extraHeaders = QString());
void redirect(QTcpSocket* client, const QString& location,
              const QString& extraHeaders = QString());
QMap<QString, QString> parseFormBody(const QString& body);
QMap<QString, QString> parseQueryString(const QString& query);
QByteArray buildMultipartZipResponse(const QByteArray& zipData, const QString& filename);

// --- 页面 ---
QString pageShell(const HttpRequest& req, const QString& title,
                  const QString& body, const HttpUser* user);
QString loginPage(const QString& error = QString());
QString emergencyPage(const QString& error = QString(), const QString& ok = QString());
QString generateIndexHtml(const HttpRequest& req, const HttpUser& user,
                          const QString& currentPath, const QString& dirPath);
QString editPage(const HttpRequest& req, const HttpUser& user,
                 const QString& relPath, const QString& content, bool canEdit);
QString mediaPage(const HttpRequest& req, const HttpUser& user,
                  const QString& relPath, const QString& mediaType);
QString adminPage(const HttpRequest& req, const HttpUser& user,
                  const QString& msg = QString(), bool isErr = false);
QString adminOnlinePage(const HttpRequest& req, const HttpUser& user);
QString adminRequestsPage(const HttpRequest& req, const HttpUser& user);
QString adminLogsPage(const HttpRequest& req, const HttpUser& user,
                      const QString& date, const QString& content);
QString adminBlacklistPage(const HttpRequest& req, const HttpUser& user,
                           const QString& msg = QString(), bool isErr = false);
QString safeModePage(const HttpRequest& req, const HttpUser& user);
QString chatPage(const HttpRequest& req, const HttpUser& user);

// --- 路由 ---
void handleRequest(QTcpSocket* client, const HttpRequest& req);
void routeAuth(QTcpSocket* client, const HttpRequest& req, bool& handled);
void routeFiles(QTcpSocket* client, const HttpRequest& req, const HttpUser& user, bool& handled);
void routeAdmin(QTcpSocket* client, const HttpRequest& req, const HttpUser& user, bool& handled);
void routeApi(QTcpSocket* client, const HttpRequest& req, const HttpUser& user, bool& handled);

} // namespace

// ===== END BATCH 1 =====

// ===== BATCH 2: 工具层 / 路径安全 / 用户存取 / 会话在线 / 频率限制 / ZIP =====

namespace {

// ============================================================
// 路径与文件位置
// ============================================================

QString usersFilePath() {
    return Utils::configDir() + "/http_users.json";
}

QString blacklistFilePath() {
    return Utils::configDir() + "/http_blacklist.json";
}

QString logDirPath() {
    return g_httpUi ? QDir(g_httpUi->currentDir).absoluteFilePath("serve-log") : QString();
}

QString nowString() {
    return QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
}

QString dateString() {
    return QDate::currentDate().toString("yyyy-MM-dd");
}

QString htmlEscape(const QString& s) {
    return s.toHtmlEscaped();
}

// ============================================================
// 哈希与随机
// ============================================================

QString hashPassword(const QString& salt, const QString& password) {
    QByteArray data = (salt + password).toUtf8();
    return QString::fromLatin1(
        QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex());
}

static QByteArray randomBytes(int n) {
    QByteArray buf(n, 0);
    for (int i = 0; i < n; ++i)
        buf[i] = char(QRandomGenerator::global()->bounded(256));
    return buf;
}

QString randomSalt() {
    return QString::fromLatin1(randomBytes(16).toHex());
}

QString randomToken() {
    return QString::fromLatin1(randomBytes(24).toHex());
}

QString generateId() {
    return QString::fromLatin1(randomBytes(12).toHex());
}

// ============================================================
// 日志
// ============================================================

void logRequest(const QString& msg) {
    const QString stamp = nowString();
    if (g_httpUi) {
        g_httpUi->appendOutput(
            QString("[%1] %2\n").arg(stamp, msg), INFO_COLOR);
    }
    const QString dir = logDirPath();
    if (dir.isEmpty()) return;
    QDir().mkpath(dir);
    QFile f(dir + "/" + dateString() + ".log");
    if (f.open(QIODevice::Append | QIODevice::Text)) {
        f.write(QString("[%1] %2\n").arg(stamp, msg).toUtf8());
        f.close();
    }
}

// ============================================================
// 路径安全（防目录穿越 / 防敏感文件）
// ============================================================

// 将相对路径规范化：去掉开头的 '/'，折叠 ".." 与 "."，阻止逃逸
QString normalizedRelPath(const QString& relPath) {
    QString p = relPath;
    p.replace('\\', '/');
    while (p.startsWith('/')) p.remove(0, 1);

    QStringList out;
    for (const QString& seg : p.split('/', Qt::SkipEmptyParts)) {
        if (seg == "." ) continue;
        if (seg == "..") {
            if (!out.isEmpty()) out.removeLast();
            // 已经在根上，忽略多余的 ".."，等价于夹在 baseDir 内
            continue;
        }
        out.append(seg);
    }
    return out.join('/');
}

// 把 relPath 安全地拼到 baseDir 下；返回绝对路径。
// 只要出现越界（.. 逃逸）或结果不在 baseDir 内，ok=false。
QString safeJoin(const QString& baseDir, const QString& relPath, bool* ok) {
    if (ok) *ok = false;
    const QString base = QDir(baseDir).absolutePath();
    const QString clean = normalizedRelPath(relPath);
    const QString abs = QDir(base).absoluteFilePath(clean);
    if (!isInsideBase(base, abs)) return QString();
    if (ok) *ok = true;
    return QDir::cleanPath(abs);
}

bool isInsideBase(const QString& baseDir, const QString& absPath) {
    const QString base = QDir::cleanPath(QDir(baseDir).absolutePath());
    const QString target = QDir::cleanPath(QDir(absPath).absolutePath());
    if (target == base) return true;
    return target.startsWith(base + '/');
}

// 敏感文件判断：密码库、日志目录、黑名单
bool isSensitivePath(const QString& relPath) {
    const QString p = normalizedRelPath(relPath).toLower();
    if (p.isEmpty()) return false;
    if (p == "http_users.json") return true;
    if (p == "http_blacklist.json") return true;
    if (p.startsWith("serve-log/")) return true;
    if (p.contains(".tmp_download_")) return true;
    return false;
}

// ============================================================
// 用户存取
// ============================================================

QMap<QString, HttpUser> loadUsers() {
    QMap<QString, HttpUser> users;
    QFile f(usersFilePath());
    if (f.open(QIODevice::ReadOnly)) {
        const QJsonObject obj = QJsonDocument::fromJson(f.readAll()).object();
        f.close();
        for (auto it = obj.begin(); it != obj.end(); ++it) {
            const QJsonObject u = it.value().toObject();
            HttpUser user;
            user.salt  = u.value("salt").toString();
            user.hash  = u.value("hash").toString();
            user.isRoot= u.value("isRoot").toBool(false);
            user.r     = u.value("r").toBool(false);
            user.w     = u.value("w").toBool(false);
            user.x     = u.value("x").toBool(false);
            users.insert(it.key(), user);
        }
    }
    // 保证 root 存在
    if (!users.contains("root")) {
        HttpUser root;
        root.salt  = randomSalt();
        root.hash  = hashPassword(root.salt, kDefaultRootPassword);
        root.isRoot= true;
        root.r = root.w = root.x = true;
        users.insert("root", root);
        saveUsers(users);
    }
    return users;
}

void saveUsers(const QMap<QString, HttpUser>& users) {
    QJsonObject obj;
    for (auto it = users.begin(); it != users.end(); ++it) {
        QJsonObject u;
        u["salt"]   = it.value().salt;
        u["hash"]   = it.value().hash;
        u["isRoot"] = it.value().isRoot;
        u["r"]      = it.value().r;
        u["w"]      = it.value().w;
        u["x"]      = it.value().x;
        obj[it.key()] = u;
    }
    const QString path = usersFilePath();
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile out(path);
    if (out.open(QIODevice::WriteOnly)) {
        out.write(QJsonDocument(obj).toJson(QJsonDocument::Compact));
        out.close();
    }
}

void loadBlacklist() {
    g_blacklistedIPs.clear();
    QFile f(blacklistFilePath());
    if (f.open(QIODevice::ReadOnly)) {
        const QJsonArray arr = QJsonDocument::fromJson(f.readAll()).array();
        for (const QJsonValue& v : arr)
            g_blacklistedIPs.insert(v.toString());
        f.close();
    }
}

void saveBlacklist() {
    QJsonArray arr;
    for (const QString& ip : g_blacklistedIPs) arr.append(ip);
    const QString path = blacklistFilePath();
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile f(path);
    if (f.open(QIODevice::WriteOnly)) {
        f.write(QJsonDocument(arr).toJson(QJsonDocument::Compact));
        f.close();
    }
}

bool verifyPassword(const HttpUser& user, const QString& password) {
    if (user.salt.isEmpty()) return false;
    return hashPassword(user.salt, password) == user.hash;
}

bool verifyRootPassword(const QString& password) {
    const HttpUser root = g_users.value("root");
    if (root.salt.isEmpty()) return false;
    return verifyPassword(root, password);
}

// ============================================================
// 会话
// ============================================================

QString extractSid(const QString& headers) {
    static const QRegularExpression re(
        "Cookie:\\s*([^\\r\\n]+)", QRegularExpression::CaseInsensitiveOption);
    const auto m = re.match(headers);
    if (!m.hasMatch()) return QString();
    for (const QString& part : m.captured(1).split(';')) {
        const QString p = part.trimmed();
        if (p.startsWith("sid=")) return p.mid(4);
    }
    return QString();
}

QString currentUser(const QString& headers) {
    const QString sid = extractSid(headers);
    if (sid.isEmpty()) return QString();
    return g_sessions.value(sid, QString());
}

bool hasActiveSession(const QString& username) {
    for (auto it = g_sessions.begin(); it != g_sessions.end(); ++it) {
        if (it.value() == username) return true;
    }
    return false;
}

QString createSession(const QString& username, const QString& ip) {
    const QString sid = randomToken();
    g_sessions[sid] = username;
    SessionInfo info;
    info.username = username;
    info.createdAt = QDateTime::currentSecsSinceEpoch();
    info.lastActive = info.createdAt;
    info.ip = ip;
    g_sessionInfo[sid] = info;
    touchOnline(username, ip);
    return sid;
}

void destroySession(const QString& sid) {
    if (sid.isEmpty()) return;
    g_sessionInfo.remove(sid);
    g_sessions.remove(sid);
}

void destroySessionsOf(const QString& username, const QString& exceptSid) {
    QStringList toRemove;
    for (auto it = g_sessions.begin(); it != g_sessions.end(); ++it) {
        if (it.value() == username && it.key() != exceptSid)
            toRemove.append(it.key());
    }
    for (const QString& sid : toRemove) {
        g_sessionInfo.remove(sid);
        g_sessions.remove(sid);
    }
    removeOnline(username);
}

// ============================================================
// 在线用户
// ============================================================

void touchOnline(const QString& username, const QString& ip) {
    if (username.isEmpty()) return;
    OnlineInfo info;
    info.lastSeen = QDateTime::currentSecsSinceEpoch();
    info.ip = ip;
    g_onlineUsers[username] = info;
}

void removeOnline(const QString& username) {
    g_onlineUsers.remove(username);
}

int countOnline() {
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    int n = 0;
    for (auto it = g_onlineUsers.begin(); it != g_onlineUsers.end(); ++it) {
        if (now - it.value().lastSeen < kOnlineTimeoutSecs) ++n;
    }
    return n;
}

bool isOnline(const QString& username) {
    if (!g_onlineUsers.contains(username)) return false;
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    return (now - g_onlineUsers.value(username).lastSeen) < kOnlineTimeoutSecs;
}

// ============================================================
// 防火墙：频率限制
// ============================================================

bool checkRateLimit(const QString& ip) {
    if (ip.isEmpty()) return true;
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    RateLimitEntry& e = g_rateLimits[ip];

    // 处于封禁期
    if (e.banUntil > now) return false;

    // 窗口过期，重置
    if (now - e.windowStart >= kRateLimitWindowSecs || e.windowStart == 0) {
        e.windowStart = now;
        e.count = 0;
    }
    ++e.count;
    if (e.count > kRateLimitMaxRequests) {
        e.banUntil = now + kRateLimitBanSecs;
        logRequest(QString("[防火墙] IP %1 请求过频，临时封禁 %2 秒")
                       .arg(ip).arg(kRateLimitBanSecs));
        return false;
    }
    return true;
}

void cleanupRateLimits() {
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    for (auto it = g_rateLimits.begin(); it != g_rateLimits.end(); ) {
        const bool staleWindow = (now - it.value().windowStart) > 3600;
        const bool banExpired  = it.value().banUntil <= now;
        if (staleWindow && banExpired) it = g_rateLimits.erase(it);
        else ++it;
    }
}

// ============================================================
// 权限辅助
// ============================================================

bool canRead(const HttpUser& u) {
    return u.isRoot || u.r;
}

bool canWrite(const HttpUser& u) {
    // 写必须建立在读之上
    return u.isRoot || (u.r && u.w);
}

bool canExec(const HttpUser& u) {
    // 运行必须建立在读之上
    return u.isRoot || (u.r && u.x);
}

QString permString(const HttpUser& u) {
    if (u.isRoot) return "root";
    QString s;
    if (u.r) s += 'r';
    if (u.w) s += 'w';
    if (u.x) s += 'x';
    return s.isEmpty() ? "-" : s;
}

// ============================================================
// ZIP（store 模式，无压缩，零依赖）
// ============================================================

static void putU16(QByteArray& out, quint16 v) {
    out.append(char(v & 0xFF));
    out.append(char((v >> 8) & 0xFF));
}

static void putU32(QByteArray& out, quint32 v) {
    out.append(char(v & 0xFF));
    out.append(char((v >> 8) & 0xFF));
    out.append(char((v >> 16) & 0xFF));
    out.append(char((v >> 24) & 0xFF));
}

static quint32 crc32(const QByteArray& data) {
    static quint32 table[256];
    static bool inited = false;
    if (!inited) {
        for (quint32 i = 0; i < 256; ++i) {
            quint32 c = i;
            for (int k = 0; k < 8; ++k)
                c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
            table[i] = c;
        }
        inited = true;
    }
    quint32 crc = 0xFFFFFFFFu;
    for (int i = 0; i < data.size(); ++i)
        crc = table[(crc ^ quint8(data[i])) & 0xFF] ^ (crc >> 8);
    return crc ^ 0xFFFFFFFFu;
}

// files: 每项为 (zip 内路径, 文件内容)
QByteArray buildZipStore(const QList<QPair<QString, QByteArray>>& files) {
    QByteArray out;
    struct CentralEntry {
        QByteArray name;
        quint32 crc;
        quint32 size;
        quint32 offset;
    };
    QVector<CentralEntry> central;

    for (const auto& f : files) {
        const QByteArray name = f.first.toUtf8();
        const QByteArray& data = f.second;
        const quint32 crc = crc32(data);
        const quint32 size = quint32(data.size());
        const quint32 offset = quint32(out.size());

        // local file header
        putU32(out, 0x04034b50);
        putU16(out, 20);        // version needed
        putU16(out, 0);         // flags
        putU16(out, 0);         // method: store
        putU16(out, 0);         // mod time
        putU16(out, 0);         // mod date
        putU32(out, crc);
        putU32(out, size);      // compressed size
        putU32(out, size);      // uncompressed size
        putU16(out, quint16(name.size()));
        putU16(out, 0);         // extra len
        out.append(name);
        out.append(data);

        central.append({name, crc, size, offset});
    }

    const quint32 centralOffset = quint32(out.size());
    for (const auto& c : central) {
        putU32(out, 0x02014b50);
        putU16(out, 20);        // version made by
        putU16(out, 20);        // version needed
        putU16(out, 0);         // flags
        putU16(out, 0);         // method
        putU16(out, 0);         // mod time
        putU16(out, 0);         // mod date
        putU32(out, c.crc);
        putU32(out, c.size);
        putU32(out, c.size);
        putU16(out, quint16(c.name.size()));
        putU16(out, 0);         // extra
        putU16(out, 0);         // comment
        putU16(out, 0);         // disk start
        putU16(out, 0);         // internal attr
        putU32(out, 0);         // external attr
        putU32(out, c.offset);
        out.append(c.name);
    }
    const quint32 centralSize = quint32(out.size()) - centralOffset;

    // end of central directory
    putU32(out, 0x06054b50);
    putU16(out, 0);             // disk number
    putU16(out, 0);             // disk with central
    putU16(out, quint16(central.size()));
    putU16(out, quint16(central.size()));
    putU32(out, centralSize);
    putU32(out, centralOffset);
    putU16(out, 0);             // comment len
    return out;
}

} // namespace

// ===== END BATCH 2 =====

// ===== BATCH 3: 页面 - 登录 / 应急通道 / 文件列表 / 编辑 / 媒体 / 日志 / 黑名单 =====

namespace {

// ============================================================
// 公共 JS 与主题片段
// ============================================================

static QString themeCssDark() {
    return R"(
:root{
  --bg:#0a0a0a;--bg2:#141414;--card:#161616;--border:#2a2a2a;
  --text:#e6e6e6;--muted:#8a8a8a;--primary:#00d2ff;--danger:#ff4757;
  --success:#2ed573;--warn:#ffa502;--radius:12px;--shadow:0 8px 24px rgba(0,0,0,.4);
}
*{box-sizing:border-box;margin:0;padding:0;transition:background .2s,color .2s,border-color .2s;}
body{background:var(--bg);color:var(--text);font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,"Helvetica Neue",sans-serif;font-size:14px;line-height:1.6;padding:16px;}
a{color:var(--primary);text-decoration:none;}
a:hover{opacity:.85;}
h1{font-size:20px;margin-bottom:16px;}
.topbar{display:flex;align-items:center;justify-content:space-between;gap:12px;flex-wrap:wrap;margin-bottom:24px;padding:14px 18px;background:var(--card);border:1px solid var(--border);border-radius:var(--radius);box-shadow:var(--shadow);}
.brand{font-size:19px;color:var(--primary);font-weight:700;letter-spacing:.5px;}
.userbox{font-size:13px;color:var(--muted);display:flex;align-items:center;gap:10px;flex-wrap:wrap;}
.userbox b{color:var(--text);}
.crumb{color:var(--muted);font-size:13px;margin-bottom:16px;word-break:break-all;}
.toolbar{display:flex;gap:8px;margin-bottom:16px;flex-wrap:wrap;align-items:center;}
.btn{background:var(--bg2);color:var(--text);border:1px solid var(--border);padding:9px 15px;border-radius:8px;cursor:pointer;font-size:13px;font-weight:500;display:inline-flex;align-items:center;justify-content:center;gap:6px;}
.btn:hover{background:#222;}
.btn-primary{background:var(--primary);color:#000;border:none;font-weight:600;}
.btn-primary:hover{background:#00b8e6;box-shadow:0 4px 14px rgba(0,210,255,.3);}
.btn-danger{background:var(--danger);color:#fff;border:none;}
.btn-danger:hover{background:#e04050;}
.btn-sm{padding:6px 11px;font-size:12px;}
table{width:100%;border-collapse:separate;border-spacing:0 7px;font-size:13px;}
th{color:var(--muted);font-weight:500;font-size:12px;text-transform:uppercase;letter-spacing:.4px;text-align:left;padding:0 14px 6px;}
tr td{background:var(--card);padding:11px 14px;border-top:1px solid var(--border);border-bottom:1px solid var(--border);}
tr td:first-child{border-left:1px solid var(--border);border-top-left-radius:var(--radius);border-bottom-left-radius:var(--radius);}
tr td:last-child{border-right:1px solid var(--border);border-top-right-radius:var(--radius);border-bottom-right-radius:var(--radius);}
tr:hover td{background:#1a1a1a;}
.icon{margin-right:7px;font-size:15px;}
.muted{color:var(--muted);font-size:12px;}
.empty{color:var(--muted);text-align:center;padding:36px;font-style:italic;}
.card{background:var(--card);border:1px solid var(--border);border-radius:var(--radius);padding:20px;margin-top:18px;box-shadow:var(--shadow);}
.card h3{color:var(--primary);font-size:15px;margin-bottom:14px;font-weight:600;display:flex;align-items:center;gap:8px;}
input[type=text],input[type=password],input[type=file],textarea,select{background:#050505;color:var(--text);border:1px solid var(--border);padding:11px 14px;border-radius:8px;font-size:14px;width:100%;max-width:320px;font-family:inherit;}
input:focus,textarea:focus,select:focus{outline:none;border-color:var(--primary);box-shadow:0 0 0 3px rgba(0,210,255,.12);}
textarea{width:100%;max-width:100%;min-height:420px;font-family:ui-monospace,SFMono-Regular,Menlo,Consolas,monospace;font-size:13px;resize:vertical;line-height:1.55;}
label{display:block;color:var(--muted);font-size:12px;margin-bottom:7px;}
.form-row{display:flex;gap:10px;align-items:center;flex-wrap:wrap;background:var(--bg2);padding:14px;border-radius:8px;border:1px solid var(--border);}
.msg{padding:11px 15px;border-radius:8px;margin-bottom:18px;font-size:13px;font-weight:500;}
.msg-err{background:rgba(255,71,87,.1);color:#ff6b81;border:1px solid rgba(255,71,87,.25);}
.msg-ok{background:rgba(46,213,115,.1);color:#2ed573;border:1px solid rgba(46,213,115,.25);}
.badge{display:inline-block;padding:3px 8px;border-radius:6px;font-size:11px;font-weight:700;}
.badge-root{background:rgba(0,210,255,.15);color:var(--primary);}
.badge-user{background:#333;color:#ccc;}
.badge-r{background:#1b5e20;color:#a5d6a7;}
.badge-w{background:#e65100;color:#ffcc80;}
.badge-x{background:rgba(156,39,176,.2);color:#ce93d8;}
.badge-online{background:rgba(46,213,115,.2);color:var(--success);box-shadow:0 0 8px rgba(46,213,115,.3);}
.badge-offline{background:#333;color:#888;}
.checkbox{width:17px;height:17px;cursor:pointer;accent-color:var(--primary);margin:0;vertical-align:middle;}
.toast{position:fixed;top:20px;left:50%;transform:translateX(-50%);background:var(--card);color:var(--text);padding:11px 22px;border-radius:8px;box-shadow:0 10px 30px rgba(0,0,0,.5);border:1px solid var(--border);display:none;z-index:9999;font-size:13px;}
.upload-progress{width:100%;height:8px;background:#222;border-radius:4px;margin-top:10px;display:none;overflow:hidden;}
.upload-bar{height:100%;background:var(--primary);width:0%;transition:width .1s linear;border-radius:4px;}
.preview-img{max-width:100%;border-radius:10px;display:block;margin-top:12px;}
.preview-video{max-width:100%;border-radius:10px;display:block;margin-top:12px;}
.chatbox{height:60vh;overflow-y:auto;background:#050505;border:1px solid var(--border);border-radius:10px;padding:14px;margin-bottom:12px;display:flex;flex-direction:column;gap:8px;}
.chat-msg{padding:8px 12px;border-radius:8px;background:#141414;max-width:80%;word-break:break-word;}
.chat-msg.root{background:rgba(0,210,255,.12);border:1px solid rgba(0,210,255,.2);align-self:flex-end;}
.chat-meta{font-size:11px;color:var(--muted);margin-bottom:3px;}
.mode-toggle{background:var(--bg2);border:1px solid var(--border);border-radius:8px;padding:7px 11px;cursor:pointer;font-size:12px;color:var(--text);}
)";
}

static QString themeCssLight() {
    return R"(
:root{
  --bg:#f5f6f8;--bg2:#eceef2;--card:#ffffff;--border:#e2e5ea;
  --text:#1a1d22;--muted:#6b7280;--primary:#0088cc;--danger:#e03e4e;
  --success:#18a558;--warn:#d98800;--radius:12px;--shadow:0 4px 16px rgba(0,0,0,.06);
}
*{box-sizing:border-box;margin:0;padding:0;transition:background .2s,color .2s,border-color .2s;}
body{background:var(--bg);color:var(--text);font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,"Helvetica Neue",sans-serif;font-size:14px;line-height:1.6;padding:16px;}
a{color:var(--primary);text-decoration:none;}
a:hover{opacity:.85;}
h1{font-size:20px;margin-bottom:16px;}
.topbar{display:flex;align-items:center;justify-content:space-between;gap:12px;flex-wrap:wrap;margin-bottom:24px;padding:14px 18px;background:var(--card);border:1px solid var(--border);border-radius:var(--radius);box-shadow:var(--shadow);}
.brand{font-size:19px;color:var(--primary);font-weight:700;letter-spacing:.5px;}
.userbox{font-size:13px;color:var(--muted);display:flex;align-items:center;gap:10px;flex-wrap:wrap;}
.userbox b{color:var(--text);}
.crumb{color:var(--muted);font-size:13px;margin-bottom:16px;word-break:break-all;}
.toolbar{display:flex;gap:8px;margin-bottom:16px;flex-wrap:wrap;align-items:center;}
.btn{background:var(--bg2);color:var(--text);border:1px solid var(--border);padding:9px 15px;border-radius:8px;cursor:pointer;font-size:13px;font-weight:500;display:inline-flex;align-items:center;justify-content:center;gap:6px;}
.btn:hover{background:#e0e3e8;}
.btn-primary{background:var(--primary);color:#fff;border:none;font-weight:600;}
.btn-primary:hover{background:#0077b3;}
.btn-danger{background:var(--danger);color:#fff;border:none;}
.btn-danger:hover{background:#c73343;}
.btn-sm{padding:6px 11px;font-size:12px;}
table{width:100%;border-collapse:separate;border-spacing:0 7px;font-size:13px;}
th{color:var(--muted);font-weight:500;font-size:12px;text-transform:uppercase;letter-spacing:.4px;text-align:left;padding:0 14px 6px;}
tr td{background:var(--card);padding:11px 14px;border-top:1px solid var(--border);border-bottom:1px solid var(--border);}
tr td:first-child{border-left:1px solid var(--border);border-top-left-radius:var(--radius);border-bottom-left-radius:var(--radius);}
tr td:last-child{border-right:1px solid var(--border);border-top-right-radius:var(--radius);border-bottom-right-radius:var(--radius);}
tr:hover td{background:#fafbfc;}
.icon{margin-right:7px;font-size:15px;}
.muted{color:var(--muted);font-size:12px;}
.empty{color:var(--muted);text-align:center;padding:36px;font-style:italic;}
.card{background:var(--card);border:1px solid var(--border);border-radius:var(--radius);padding:20px;margin-top:18px;box-shadow:var(--shadow);}
.card h3{color:var(--primary);font-size:15px;margin-bottom:14px;font-weight:600;display:flex;align-items:center;gap:8px;}
input[type=text],input[type=password],input[type=file],textarea,select{background:#fff;color:var(--text);border:1px solid var(--border);padding:11px 14px;border-radius:8px;font-size:14px;width:100%;max-width:320px;font-family:inherit;}
input:focus,textarea:focus,select:focus{outline:none;border-color:var(--primary);box-shadow:0 0 0 3px rgba(0,136,204,.12);}
textarea{width:100%;max-width:100%;min-height:420px;font-family:ui-monospace,SFMono-Regular,Menlo,Consolas,monospace;font-size:13px;resize:vertical;line-height:1.55;}
label{display:block;color:var(--muted);font-size:12px;margin-bottom:7px;}
.form-row{display:flex;gap:10px;align-items:center;flex-wrap:wrap;background:var(--bg2);padding:14px;border-radius:8px;border:1px solid var(--border);}
.msg{padding:11px 15px;border-radius:8px;margin-bottom:18px;font-size:13px;font-weight:500;}
.msg-err{background:rgba(224,62,78,.08);color:#c73343;border:1px solid rgba(224,62,78,.25);}
.msg-ok{background:rgba(24,165,88,.08);color:#18a558;border:1px solid rgba(24,165,88,.25);}
.badge{display:inline-block;padding:3px 8px;border-radius:6px;font-size:11px;font-weight:700;}
.badge-root{background:rgba(0,136,204,.12);color:var(--primary);}
.badge-user{background:#e8eaed;color:#444;}
.badge-r{background:#dcf3e3;color:#1b7a43;}
.badge-w{background:#fdecd9;color:#b05e00;}
.badge-x{background:#f0e0f5;color:#8a2fa5;}
.badge-online{background:rgba(24,165,88,.14);color:var(--success);}
.badge-offline{background:#e8eaed;color:#777;}
.checkbox{width:17px;height:17px;cursor:pointer;accent-color:var(--primary);margin:0;vertical-align:middle;}
.toast{position:fixed;top:20px;left:50%;transform:translateX(-50%);background:#fff;color:var(--text);padding:11px 22px;border-radius:8px;box-shadow:0 10px 30px rgba(0,0,0,.15);border:1px solid var(--border);display:none;z-index:9999;font-size:13px;}
.upload-progress{width:100%;height:8px;background:#ddd;border-radius:4px;margin-top:10px;display:none;overflow:hidden;}
.upload-bar{height:100%;background:var(--primary);width:0%;transition:width .1s linear;border-radius:4px;}
.preview-img{max-width:100%;border-radius:10px;display:block;margin-top:12px;}
.preview-video{max-width:100%;border-radius:10px;display:block;margin-top:12px;}
.chatbox{height:60vh;overflow-y:auto;background:var(--bg);border:1px solid var(--border);border-radius:10px;padding:14px;margin-bottom:12px;display:flex;flex-direction:column;gap:8px;}
.chat-msg{padding:8px 12px;border-radius:8px;background:#fff;max-width:80%;word-break:break-word;border:1px solid var(--border);}
.chat-msg.root{background:rgba(0,136,204,.08);border:1px solid rgba(0,136,204,.2);align-self:flex-end;}
.chat-meta{font-size:11px;color:var(--muted);margin-bottom:3px;}
.mode-toggle{background:var(--bg2);border:1px solid var(--border);border-radius:8px;padding:7px 11px;cursor:pointer;font-size:12px;color:var(--text);}
)";
}

// 主题切换 + toast + 通用 JS
static QString commonJs() {
    return R"(
function showToast(msg){
  const t=document.getElementById('toast');
  if(!t)return;
  t.innerText=msg;t.style.display='block';
  clearTimeout(window.__toastTimer);
  window.__toastTimer=setTimeout(()=>{t.style.display='none';},2200);
}
function toggleTheme(){
  const html=document.documentElement;
  const cur=html.getAttribute('data-theme')||'dark';
  const next=cur==='dark'?'light':'dark';
  html.setAttribute('data-theme',next);
  localStorage.setItem('termail_theme',next);
  const dark=document.getElementById('css-dark');
  const light=document.getElementById('css-light');
  if(dark)dark.disabled=(next!=='dark');
  if(light)light.disabled=(next!=='light');
}
(function(){
  const saved=localStorage.getItem('termail_theme')||'dark';
  document.addEventListener('DOMContentLoaded',function(){
    const html=document.documentElement;
    html.setAttribute('data-theme',saved);
    const dark=document.getElementById('css-dark');
    const light=document.getElementById('css-light');
    if(dark)dark.disabled=(saved!=='dark');
    if(light)light.disabled=(saved!=='light');
  });
})();
)";
}

// ============================================================
// 页面外壳
// ============================================================

QString pageShell(const HttpRequest& req, const QString& title,
                  const QString& body, const HttpUser* user) {
    const bool loggedIn = user && !req.username.isEmpty();
    QString html;
    html += R"(<!DOCTYPE html>
<html lang="zh-CN" data-theme="dark">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no">
<title>)";
    html += htmlEscape(title);
    html += R"( - Termail Cloud</title>
<style id="css-dark" type="text/css">)";
    html += themeCssDark();
    html += R"(</style>
<style id="css-light" type="text/css" disabled>)";
    html += themeCssLight();
    html += R"(</style>
</head>
<body>
<div class="topbar">
  <div class="brand">☁ Termail Cloud</div>
  <div class="userbox">)";

    if (loggedIn) {
        html += QString("<span>当前: <b>%1</b></span>").arg(htmlEscape(req.username));
        if (user->isRoot) {
            html += " <span class='badge badge-root'>ROOT</span>";
            html += " <span id='online-count' class='badge badge-online' style='cursor:pointer;' onclick=\"location.href='/admin/online'\">0 人在线</span>";
        }
        html += QString("<a href='/chat' style='margin-left:10px;'>聊天室</a>");
        if (user->isRoot)
            html += QString("<a href='/admin/users' style='margin-left:10px;font-weight:600;'>控制台</a>");
        html += QString("<a href='/logout' style='margin-left:10px;color:var(--danger);'>退出</a>");
html += R"html(<button class="mode-toggle" onclick="toggleTheme()" title="切换主题">◐</button>)html";
        if (g_safeMode)
            html += " <span class='badge' style='background:rgba(255,71,87,.2);color:#ff6b81;'>安全模式</span>";
    }

    html += R"(  </div>
</div>
<div id="toast" class="toast"></div>
<script>)";
    html += commonJs();
    html += R"(</script>
)";

    if (loggedIn && user->isRoot) {
        html += R"(
<script>
setInterval(function(){
  fetch('/api/online_count').then(r=>r.text()).then(c=>{
    const el=document.getElementById('online-count');
    if(el)el.innerText=c+' 人在线';
  }).catch(()=>{});
},5000);
</script>
)";
    }

    html += body;
    html += "\n</body></html>\n";
    return html;
}

// ============================================================
// 登录页
// ============================================================

QString loginPage(const QString& error) {
    QString html = R"(<!DOCTYPE html>
<html lang="zh-CN" data-theme="dark">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no">
<title>登录 - Termail</title>
<style id="css-dark">)";
    html += themeCssDark();
    html += R"(</style>
<style id="css-light" disabled>)";
    html += themeCssLight();
    html += R"(</style>
<style>
body{display:flex;align-items:center;justify-content:center;min-height:100vh;}
.login-card{background:var(--card);border:1px solid var(--border);border-radius:18px;padding:38px 34px;width:100%;max-width:380px;box-shadow:var(--shadow);}
.login-card h1{color:var(--primary);font-size:27px;text-align:center;font-weight:800;letter-spacing:1px;margin-bottom:6px;}
.login-card .sub{color:var(--muted);font-size:13px;text-align:center;margin-bottom:28px;}
.login-card input{margin-bottom:16px;max-width:100%;}
.login-card button{width:100%;margin-top:10px;padding:13px;font-size:15px;font-weight:700;}
.emergency-link{display:block;text-align:center;margin-top:20px;font-size:12px;color:var(--muted);}
.emergency-link:hover{color:var(--danger);}
</style>
</head>
<body>
<div class="login-card">
  <h1>☁ Termail</h1>
  <div class="sub">多用户文件管理服务</div>
)";
    if (!error.isEmpty())
        html += QString("<div class='msg msg-err'>%1</div>\n").arg(htmlEscape(error));
    if (g_safeMode)
        html += "<div class='msg msg-err'>系统处于安全模式，仅 root 可登录。</div>\n";

    html += R"(
  <form method="post" action="/login">
    <label>用户名</label>
    <input type="text" name="username" required autofocus autocomplete="username">
    <label>密码</label>
    <input type="password" name="password" required autocomplete="current-password">
    <button type="submit" class="btn btn-primary">安全登录</button>
  </form>
  <a class="emergency-link" href="/emergency">⚠ 应急通道（账号被占用时使用）</a>
</div>
<div id="toast" class="toast"></div>
<script>)";
    html += commonJs();
    html += R"(</script>
</body></html>
)";
    return html;
}

// ============================================================
// 应急通道页
// ============================================================

QString emergencyPage(const QString& error, const QString& ok) {
    QString html = R"(<!DOCTYPE html>
<html lang="zh-CN" data-theme="dark">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no">
<title>应急通道 - Termail</title>
<style id="css-dark">)";
    html += themeCssDark();
    html += R"(</style>
<style id="css-light" disabled>)";
    html += themeCssLight();
    html += R"(</style>
<style>
body{display:flex;align-items:center;justify-content:center;min-height:100vh;}
.ec-card{background:var(--card);border:1px solid var(--border);border-radius:18px;padding:34px;width:100%;max-width:420px;box-shadow:var(--shadow);}
.ec-card h1{font-size:22px;color:var(--danger);text-align:center;margin-bottom:8px;}
.ec-card .sub{color:var(--muted);font-size:12px;text-align:center;margin-bottom:24px;line-height:1.7;}
.ec-card input{margin-bottom:14px;max-width:100%;}
.ec-card button{width:100%;padding:13px;font-size:15px;font-weight:700;margin-top:8px;}
</style>
</head>
<body>
<div class="ec-card">
  <h1>⚠ 应急通道</h1>
  <div class="sub">当你的账号被他人占用登录时，<br>可在此验证身份并强制收回账号。</div>
)";
    if (!error.isEmpty())
        html += QString("<div class='msg msg-err'>%1</div>\n").arg(htmlEscape(error));
    if (!ok.isEmpty())
        html += QString("<div class='msg msg-ok'>%1</div>\n").arg(htmlEscape(ok));

    html += R"(
  <form method="post" action="/emergency">
    <label>用户名</label>
    <input type="text" name="username" required autofocus autocomplete="username">
    <label>当前密码</label>
    <input type="password" name="password" required autocomplete="current-password">
    <label>新密码</label>
    <input type="password" name="new_password" required autocomplete="new-password">
    <button type="submit" class="btn btn-danger">验证并强制收回</button>
  </form>
  <div style="text-align:center;margin-top:16px;">
    <a href="/login" style="font-size:12px;color:var(--muted);">← 返回登录</a>
  </div>
</div>
</body></html>
)";
    return html;
}

// ============================================================
// 文件列表页
// ============================================================

QString generateIndexHtml(const HttpRequest& req, const HttpUser& user,
                          const QString& currentPath, const QString& dirPath) {
    const bool isRoot = user.isRoot;
    const bool canR = canRead(user);
    const bool canW = canWrite(user);
    const bool canX = canExec(user);
    const QString search = req.query.value("q", QString());
    const QString curPath = currentPath.isEmpty() ? QStringLiteral("/") : currentPath;

    QDir dir(dirPath);
    QStringList files;

    if (!search.isEmpty()) {
        const QStringList filters = {"*" + search + "*"};
        files = dir.entryList(filters, QDir::AllEntries | QDir::NoDotAndDotDot,
                              QDir::Name | QDir::DirsFirst);
        if (files.size() < kMaxSearchEntries) {
            QDirIterator it(dirPath, filters,
                            QDir::AllEntries | QDir::NoDotAndDotDot,
                            QDirIterator::Subdirectories);
            while (it.hasNext() && files.size() < kMaxSearchEntries) {
                const QString rel = dir.relativeFilePath(it.next());
                if (!files.contains(rel)) files.append(rel);
            }
        }
    } else {
        files = dir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot,
                              QDir::Name | QDir::DirsFirst);
        if (files.size() > kMaxDirEntries) files = files.mid(0, kMaxDirEntries);
    }

    // 隐藏敏感文件与临时目录
    files.erase(std::remove_if(files.begin(), files.end(), [](const QString& n){
        return n.startsWith(".tmp_download_");
    }), files.end());

    QString body;
    body += QString("<div class='crumb'>📍 路径: <b>%1</b></div>\n").arg(htmlEscape(curPath));

    body += "<div class='toolbar'>\n";
    if (curPath != "/" && !curPath.isEmpty()) {
        QString parent = curPath;
        if (parent.endsWith('/')) parent.chop(1);
        const int idx = parent.lastIndexOf('/');
        parent = (idx > 0) ? parent.left(idx + 1) : QStringLiteral("/");
        body += QString("<a class='btn' href='%1'>← 上级</a>\n").arg(parent);
    }
    body += "<a class='btn' href='/'>⌂ 根目录</a>\n";
    if (canW) {
        body += "<button class='btn btn-primary btn-sm' onclick=\"batchDownload()\">⬇ 下载选中</button>\n";
        body += "<button class='btn btn-danger btn-sm' onclick=\"batchDelete()\">🗑 删除选中</button>\n";
    }
    body += "</div>\n";

    body += "<form class='toolbar' method='get' action=''>\n";
    body += QString("<input type='text' name='q' placeholder='递归搜索文件...' value='%1' style='max-width:280px;'>\n")
                .arg(htmlEscape(search));
    body += "<button type='submit' class='btn'>搜索</button>\n";
    if (!search.isEmpty())
        body += QString("<a class='btn' href='%1'>清除</a>\n").arg(htmlEscape(curPath));
    body += "</form>\n";

    body += "<table>\n<thead><tr>\n";
    if (canW)
        body += "<th style='width:36px'><input type='checkbox' class='checkbox' id='chkAll' onclick=\"toggleAll(this)\"></th>\n";
    body += "<th>名称</th><th style='width:90px'>大小</th><th style='width:150px'>修改时间</th><th style='width:300px'>操作</th>\n";
    body += "</tr></thead>\n<tbody>\n";

    if (files.isEmpty()) {
        body += QString("<tr><td colspan='5' class='empty'>%1</td></tr>\n")
                    .arg(search.isEmpty() ? QStringLiteral("空目录") : QStringLiteral("无匹配文件"));
    } else {
        for (const QString& name : files) {
            QFileInfo info(dir.absoluteFilePath(name));
            const bool isDir = info.isDir();
            const QString icon = isDir ? "📁" : "📄";
            const QString size = isDir ? QStringLiteral("-") : Utils::formatSize(info.size());
            const QString mtime = info.lastModified().toString("yyyy-MM-dd hh:mm");
            const QString relFile = curPath + (curPath.endsWith('/') ? "" : "/") + name;
            const QString href = isDir
                ? (relFile + "/")
                : ("/download?path=" + QUrl::toPercentEncoding(relFile));

            body += "<tr>\n";
            if (canW)
                body += QString("<td><input type='checkbox' class='checkbox rowchk' value='%1'></td>\n")
                            .arg(htmlEscape(name));
            body += QString("<td><span class='icon'>%1</span>").arg(icon);
            if (isDir || canR)
                body += QString("<a href='%1' style='font-weight:500;'>%2%3</a>")
                            .arg(href, htmlEscape(name), isDir ? "/" : "");
            else
                body += htmlEscape(name);
            body += "</td>\n";
            body += QString("<td class='muted'>%1</td>\n").arg(size);
            body += QString("<td class='muted'>%1</td>\n").arg(mtime);
            body += "<td>";

            if (!isDir && canR) {
                const QString lower = name.toLower();
                if (lower.endsWith(".html") || lower.endsWith(".htm")) {
                    if (canX)
                        body += QString("<a class='btn btn-sm' style='background:#9c27b0;color:#fff;border:none;' href='/run?path=%1' target='_blank'>▶ 运行</a> ")
                                    .arg(QUrl::toPercentEncoding(relFile));
                    else
                        body += "<button class='btn btn-sm' style='background:#333;color:#666;border:none;cursor:not-allowed;' title='无运行权限'>运行</button> ";
                }
                body += QString("<a class='btn btn-sm' href='/view?path=%1'>✎ 预览</a> ")
                            .arg(QUrl::toPercentEncoding(relFile));
                body += QString("<a class='btn btn-sm' href='%1' download>⬇ 下载</a> ").arg(href);
            }
            if (canW) {
                const QString safeName = htmlEscape(name).replace("'", "\\'");
                body += QString("<button class='btn btn-sm' style='background:#444;' onclick=\"renameFile('%1')\">重命名</button> ").arg(safeName);
                body += QString("<button class='btn btn-sm' style='background:#444;' onclick=\"moveFile('%1')\">移动</button> ").arg(safeName);
                body += QString("<button class='btn btn-sm' style='background:#444;' onclick=\"showProps('%1')\">属性</button> ").arg(safeName);
                body += QString("<button class='btn btn-sm btn-danger' onclick=\"delFile('%1')\">删除</button>").arg(safeName);
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
    <input type='file' name='file' id='fileInput' required style='max-width:100%;margin-bottom:12px;'>
    <div class='upload-progress' id='progBar'><div class='upload-bar' id='progFill'></div></div>
    <button type='submit' class='btn btn-primary' style='margin-top:10px;'>开始上传</button>
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
    <input type='text' name='name' placeholder='文件名（如 test.txt）' required>
    <button type='submit' class='btn btn-primary'>新建文件</button>
  </form>
</div>
)").arg(htmlEscape(curPath));

        body += QString(R"(
<div class='card' id='propsCard' style='display:none;'>
  <h3>ℹ 文件属性</h3>
  <div id='propsBody' style='font-size:13px;line-height:2;'></div>
</div>
<script>
function toggleAll(cb){document.querySelectorAll('.rowchk').forEach(c=>c.checked=cb.checked);}
function delFile(name){
  if(!confirm('确定删除 '+name+' 吗？'))return;
  fetch('/delete',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'path='+encodeURIComponent(name)+'&cur_dir='+encodeURIComponent('%1')}).then(()=>location.reload());
}
function renameFile(name){
  const n=prompt('请输入新名称（含后缀）:',name);
  if(!n||n===name)return;
  fetch('/rename',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'old_name='+encodeURIComponent(name)+'&new_name='+encodeURIComponent(n)+'&cur_dir='+encodeURIComponent('%1')}).then(()=>location.reload());
}
function moveFile(name){
  const d=prompt('请输入目标相对路径（例如 /subdir）:','/');
  if(!d)return;
  fetch('/move',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'file_name='+encodeURIComponent(name)+'&dest_dir='+encodeURIComponent(d)+'&cur_dir='+encodeURIComponent('%1')}).then(()=>location.reload());
}
function showProps(name){
  fetch('/props?cur_dir='+encodeURIComponent('%1')+'&name='+encodeURIComponent(name))
    .then(r=>r.json()).then(d=>{
      const box=document.getElementById('propsBody');
      const card=document.getElementById('propsCard');
      box.innerHTML='<b>名称：</b>'+d.name+'<br><b>大小：</b>'+d.size+'<br><b>类型：</b>'+d.type
        +'<br><b>路径：</b>'+d.path+'<br><b>修改时间：</b>'+d.mtime+'<br><b>权限：</b>'+d.perm;
      card.style.display='block';
      card.scrollIntoView({behavior:'smooth'});
    });
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
  const f=document.createElement('form');
  f.method='POST';f.action='/batch-download';f.target='_blank';
  const i1=document.createElement('input');i1.type='hidden';i1.name='paths';i1.value=names.join('\n');
  const i2=document.createElement('input');i2.type='hidden';i2.name='cur_dir';i2.value='%1';
  f.appendChild(i1);f.appendChild(i2);
  document.body.appendChild(f);f.submit();document.body.removeChild(f);
}
document.getElementById('uploadForm').onsubmit=function(e){
  e.preventDefault();
  const file=document.getElementById('fileInput').files[0];
  if(!file)return;
  const fd=new FormData();fd.append('file',file);fd.append('cur_dir','%1');
  const xhr=new XMLHttpRequest();xhr.open('POST','/upload',true);
  document.getElementById('progBar').style.display='block';
  xhr.upload.onprogress=function(e){
    if(e.lengthComputable)document.getElementById('progFill').style.width=(e.loaded/e.total)*100+'%';
  };
  xhr.onload=function(){location.reload();};
  xhr.send(fd);
};
</script>
)").arg(htmlEscape(curPath));
    }

    return pageShell(req, "文件管理", body, &user);
}

// ============================================================
// 编辑页
// ============================================================

QString editPage(const HttpRequest& req, const HttpUser& user,
                 const QString& relPath, const QString& content, bool canEdit) {
    QString body;
    body += QString("<div class='crumb'>📄 编辑文件: <b>%1</b></div>\n")
                .arg(htmlEscape(relPath));
    body += "<form method='post' action='/save'>\n";
    body += QString("<input type='hidden' name='path' value='%1'>\n").arg(htmlEscape(relPath));
    body += QString("<textarea name='content' %1>%2</textarea>\n")
                .arg(canEdit ? QString() : QStringLiteral("readonly"), htmlEscape(content));
    body += "<div class='toolbar' style='margin-top:18px'>\n";
    if (canEdit)
        body += "<button type='submit' class='btn btn-primary'>💾 保存修改</button>\n";
    body += "<a class='btn' href='/'>取消返回</a>\n</div></form>\n";
    return pageShell(req, "编辑文件", body, &user);
}

// ============================================================
// 媒体预览页
// ============================================================

QString mediaPage(const HttpRequest& req, const HttpUser& user,
                  const QString& relPath, const QString& mediaType) {
    const QString rawUrl = "/raw?path=" + QUrl::toPercentEncoding(relPath);
    QString body;
    body += QString("<div class='crumb'>%1</div>\n").arg(htmlEscape(relPath));
    if (mediaType == "video")
        body += QString("<video src='%1' class='preview-video' controls autoplay></video>\n").arg(rawUrl);
    else
        body += QString("<img src='%1' class='preview-img'>\n").arg(rawUrl);
    body += "<div style='margin-top:18px'><a class='btn' href='/'>← 返回列表</a></div>";
    return pageShell(req, "媒体预览", body, &user);
}

// ============================================================
// 日志页
// ============================================================

QString adminLogsPage(const HttpRequest& req, const HttpUser& user,
                      const QString& date, const QString& content) {
    QString body;
    body += QString("<div class='crumb'>📜 日志查看: <b>%1</b></div>\n").arg(htmlEscape(date));
    body += R"(
<div class='card'>
  <form method='get' action='/admin/logs' class='form-row'>
    <input type='text' name='date' placeholder='日期（如 2026-11-13，留空看最新）' style='max-width:220px;'>
    <button type='submit' class='btn btn-primary'>查看</button>
  </form>
</div>
)";
    body += "<div class='card'><pre style='white-space:pre-wrap;font-family:monospace;color:#2ed573;background:#050505;padding:18px;border-radius:8px;overflow:auto;max-height:65vh;font-size:12px;'>"
            + htmlEscape(content) + "</pre></div>";
    body += "<a class='btn' href='/admin/users' style='margin-top:12px;'>← 返回控制台</a>";
    return pageShell(req, "系统日志", body, &user);
}

// ============================================================
// 黑名单页（独立页）
// ============================================================

QString adminBlacklistPage(const HttpRequest& req, const HttpUser& user,
                           const QString& msg, bool isErr) {
    QString body;
    body += "<div class='toolbar'><a class='btn' href='/admin/users'>← 返回控制台</a></div>\n";
    if (!msg.isEmpty())
        body += QString("<div class='msg %1'>%2</div>\n")
                    .arg(isErr ? "msg-err" : "msg-ok", htmlEscape(msg));

    body += R"(
<div class='card'>
  <h3>🛡 IP 黑名单管理</h3>
  <form method='post' action='/admin/blacklist' class='form-row'>
    <input type='text' name='ip' placeholder='输入要封禁的 IP（如 192.168.1.5）' required style='max-width:220px;'>
    <button type='submit' class='btn btn-danger'>加入黑名单</button>
    <button type='submit' formaction='/admin/unblacklist' class='btn'>移出黑名单</button>
  </form>
  <div style='margin-top:14px;display:flex;gap:8px;flex-wrap:wrap;'>
)";
    if (g_blacklistedIPs.isEmpty())
        body += "<span class='muted'>当前黑名单为空</span>";
    else
        for (const QString& ip : g_blacklistedIPs)
            body += QString("<span class='badge badge-offline' style='padding:6px 12px;'>%1</span>\n")
                        .arg(htmlEscape(ip));
    body += "  </div>\n</div>\n";
    return pageShell(req, "IP 黑名单", body, &user);
}

} // namespace

// ===== END BATCH 3 =====

// ===== BATCH 4: 页面 - 控制台 / 在线用户 / 权限请求 / 安全模式 / 聊天室 =====

namespace {

// ============================================================
// 控制台（用户管理）
// ============================================================

QString adminPage(const HttpRequest& req, const HttpUser& user,
                  const QString& msg, bool isErr) {
    QString body;
    body += "<div class='toolbar'>\n";
    body += "<a class='btn' href='/'>← 返回文件管理</a>\n";
    body += "<a class='btn' href='/admin/online'>👥 在线用户</a>\n";
    body += "<a class='btn' href='/admin/requests'>📩 权限请求</a>\n";
    body += "<a class='btn' href='/admin/blacklist'>🛡 IP 黑名单</a>\n";
    body += "<a class='btn' href='/admin/logs'>📜 日志</a>\n";
    if (g_safeMode)
        body += "<a class='btn btn-danger' href='/admin/safe-mode'>⚠ 安全模式（已启用）</a>\n";
    else
        body += "<a class='btn' href='/admin/safe-mode'>⚠ 安全模式</a>\n";
    body += "</div>\n";

    if (!msg.isEmpty())
        body += QString("<div class='msg %1'>%2</div>\n")
                    .arg(isErr ? "msg-err" : "msg-ok", htmlEscape(msg));

    // ---- 模块：创建账户 ----
    body += R"(
<div class='card'>
  <h3>➕ 创建账户</h3>
  <form method='post' action='/admin/create' class='form-row'>
    <input type='text' name='username' placeholder='用户名' required>
    <input type='password' name='password' placeholder='密码' required>
    <label style='display:flex;align-items:center;gap:6px;margin:0;color:var(--text);'>
      <input type='checkbox' name='r' checked class='checkbox'> 读</label>
    <label style='display:flex;align-items:center;gap:6px;margin:0;color:var(--text);'>
      <input type='checkbox' name='w' class='checkbox'> 写</label>
    <label style='display:flex;align-items:center;gap:6px;margin:0;color:var(--text);'>
      <input type='checkbox' name='x' class='checkbox'> 运行</label>
    <input type='password' name='root_pw' placeholder='当前 root 密码' required style='max-width:180px'>
    <button type='submit' class='btn btn-primary'>创建账户</button>
  </form>
</div>
)";

    // ---- 模块：修改自身密码 ----
    body += R"(
<div class='card'>
  <h3>🔑 修改 Root 密码</h3>
  <form method='post' action='/admin/passwd' class='form-row'>
    <input type='hidden' name='username' value='root'>
    <input type='password' name='old_password' placeholder='当前 root 密码' required style='max-width:200px'>
    <input type='password' name='password' placeholder='新 root 密码' required style='max-width:200px'>
    <button type='submit' class='btn btn-danger'>修改密码</button>
  </form>
</div>
)";

    // ---- 模块：用户列表 ----
    body += R"(
<div class='card'>
  <h3>👥 用户管理</h3>
  <table>
    <thead><tr><th>用户</th><th>类型 / 权限</th><th>状态</th><th>操作</th></tr></thead>
    <tbody>
)";

    for (auto it = g_users.begin(); it != g_users.end(); ++it) {
        const QString& name = it.key();
        const HttpUser& u = it.value();

        body += "<tr>\n";
        body += QString("<td><b style='font-size:14px;'>%1</b></td>\n").arg(htmlEscape(name));

        // 权限徽章
        body += "<td>";
        if (u.isRoot) {
            body += "<span class='badge badge-root'>ROOT</span>";
        } else {
            body += "<span class='badge badge-user'>USER</span> ";
            if (u.r) body += "<span class='badge badge-r'>R</span> ";
            if (u.w) body += "<span class='badge badge-w'>W</span> ";
            if (u.x) body += "<span class='badge badge-x'>X</span>";
            if (!u.r && !u.w && !u.x) body += "<span class='muted'>无权限</span>";
        }
        body += "</td>\n";

        // 在线状态
        body += "<td>";
        if (isOnline(name))
            body += "<span class='badge badge-online'>在线</span>";
        else
            body += "<span class='badge badge-offline'>离线</span>";
        if (g_mutedUsers.contains(name))
            body += " <span class='badge' style='background:rgba(255,71,87,.2);color:#ff6b81;'>禁言</span>";
        body += "</td>\n";

        // 操作面板
        body += "<td>";
        if (name == "root" && u.isRoot) {
            body += "<span class='muted'>Root 账户受最高保护，不可在此操作</span>";
        } else {
            body += QString(
                "<form method='post' action='/admin/manage' style='display:flex;gap:6px;flex-wrap:wrap;align-items:center;'>"
                "<input type='hidden' name='username' value='%1'>"
                "<select name='action' style='width:auto;padding:7px;'>"
                "<option value='grant'>改权限 (R/W/X)</option>"
                "<option value='passwd'>修改密码</option>"
                "<option value='promote'>授予 ROOT</option>"
                "<option value='revoke'>撤销 ROOT</option>"
                "<option value='delete'>删除账户</option>"
                "</select>"
                "<input type='text' name='new_val' placeholder='新值（如 rwx 或新密码）' style='width:180px;'>"
                "<input type='password' name='root_pw' placeholder='root 密码验证' required style='width:160px;'>"
                "<button type='submit' class='btn btn-sm btn-primary'>执行</button>"
                "</form>").arg(htmlEscape(name));
        }
        body += "</td>\n</tr>\n";
    }

    body += "</tbody></table></div>\n";
    return pageShell(req, "控制台", body, &user);
}

// ============================================================
// 在线用户独立页（5 秒轮询）
// ============================================================

QString adminOnlinePage(const HttpRequest& req, const HttpUser& user) {
    QString body;
    body += "<div class='toolbar'><a class='btn' href='/admin/users'>← 返回控制台</a></div>\n";
    body += R"(
<div class='card'>
  <h3>👥 实时在线用户 <span class='muted' style='font-weight:400;'>（每 5 秒刷新）</span></h3>
  <div id='onlineTable'>
    <div class='empty'>加载中...</div>
  </div>
</div>
<script>
function renderOnline(list){
  const box=document.getElementById('onlineTable');
  if(!list.length){box.innerHTML="<div class='empty'>当前没有在线用户</div>";return;}
  let html="<table><thead><tr><th>用户</th><th>IP</th><th>最后活跃</th><th>操作</th></tr></thead><tbody>";
  list.forEach(u=>{
    html+="<tr><td><b>"+u.name+"</b>"+(u.root?" <span class='badge badge-root'>ROOT</span>":"")+"</td>";
    html+="<td class='muted'>"+u.ip+"</td>";
    html+="<td class='muted'>"+u.last+"</td>";
    html+="<td>";
    if(!u.root){
      html+="<button class='btn btn-sm btn-danger' onclick=\"kickUser('"+u.name+"')\">踢出</button> ";
      if(u.muted)
        html+="<button class='btn btn-sm' onclick=\"muteUser('"+u.name+"',false)\">解除禁言</button>";
      else
        html+="<button class='btn btn-sm' onclick=\"muteUser('"+u.name+"',true)\">禁言</button>";
    } else {
      html+="<span class='muted'>-</span>";
    }
    html+="</td></tr>";
  });
  html+="</tbody></table>";
  box.innerHTML=html;
}
function fetchOnline(){
  fetch('/api/admin/online').then(r=>r.json()).then(renderOnline).catch(()=>{});
}
function kickUser(name){
  if(!confirm('确定踢出用户 '+name+' 吗？'))return;
  fetch('/admin/kick',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'username='+encodeURIComponent(name)}).then(()=>fetchOnline());
}
function muteUser(name,on){
  fetch('/admin/mute',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'username='+encodeURIComponent(name)+'&mute='+(on?'1':'0')}).then(()=>fetchOnline());
}
fetchOnline();
setInterval(fetchOnline,5000);
</script>
)";
    return pageShell(req, "在线用户", body, &user);
}

// ============================================================
// 权限请求独立页（5 秒轮询）
// ============================================================

QString adminRequestsPage(const HttpRequest& req, const HttpUser& user) {
    QString body;
    body += "<div class='toolbar'><a class='btn' href='/admin/users'>← 返回控制台</a></div>\n";
    body += R"(
<div class='card'>
  <h3>📩 权限请求 <span class='muted' style='font-weight:400;'>（每 5 秒刷新，30 秒未处理自动取消）</span></h3>
  <div id='reqTable'><div class='empty'>加载中...</div></div>
</div>
<script>
function renderRequests(list){
  const box=document.getElementById('reqTable');
  if(!list.length){box.innerHTML="<div class='empty'>当前没有待处理请求</div>";return;}
  let html="<table><thead><tr><th>用户</th><th>申请</th><th>剩余时间</th><th>操作</th></tr></thead><tbody>";
  list.forEach(r=>{
    html+="<tr><td><b>"+r.user+"</b></td><td>"+r.target+"</td>";
    html+="<td class='muted'>"+r.left+" 秒</td><td>";
    html+="<button class='btn btn-sm btn-primary' onclick=\"handleReq('"+r.user+"','"+r.id+"',true)\">通过</button> ";
    html+="<button class='btn btn-sm btn-danger' onclick=\"handleReq('"+r.user+"','"+r.id+"',false)\">拒绝</button>";
    html+="</td></tr>";
  });
  html+="</tbody></table>";
  box.innerHTML=html;
}
function fetchRequests(){
  fetch('/api/admin/requests').then(r=>r.json()).then(renderRequests).catch(()=>{});
}
function handleReq(user,id,ok){
  fetch('/admin/request-handle',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'username='+encodeURIComponent(user)+'&id='+encodeURIComponent(id)+'&approve='+(ok?'1':'0')})
    .then(()=>fetchRequests());
}
fetchRequests();
setInterval(fetchRequests,5000);
</script>
)";
    return pageShell(req, "权限请求", body, &user);
}

// ============================================================
// 安全模式页
// ============================================================

QString safeModePage(const HttpRequest& req, const HttpUser& user) {
    QString body;
    body += "<div class='toolbar'><a class='btn' href='/admin/users'>← 返回控制台</a></div>\n";

    if (g_safeMode) {
        body += R"(
<div class='card' style='border-color:rgba(255,71,87,.4);'>
  <h3 style='color:var(--danger);'>⚠ 安全模式已启用</h3>
  <p style='font-size:13px;color:var(--muted);line-height:1.9;'>
    当前系统处于安全模式：所有非 root 用户已被强制下线，且无法登录。<br>
    Root 用户可正常使用全部功能。<br>
    退出安全模式后，普通用户即可正常登录。
  </p>
  <form method='post' action='/admin/safe-mode' class='form-row' style='margin-top:16px;'>
    <input type='hidden' name='action' value='exit'>
    <input type='password' name='root_pw' placeholder='输入 root 密码以退出安全模式' required style='max-width:260px;'>
    <button type='submit' class='btn btn-primary'>退出安全模式</button>
  </form>
</div>
)";
    } else {
        body += R"(
<div class='card'>
  <h3>⚠ 进入安全模式</h3>
  <div style='font-size:13px;color:var(--muted);line-height:1.9;background:rgba(255,71,87,.06);border:1px solid rgba(255,71,87,.2);padding:16px;border-radius:8px;'>
    <b style='color:var(--danger);'>请注意以下后果：</b><br>
    1. 进入后，所有非 root 用户将被 <b>立即强制下线</b>；<br>
    2. 安全模式期间，除 root 外 <b>任何用户无法登录</b>；<br>
    3. Root 用户仍可正常使用全部功能；<br>
    4. 退出安全模式后，普通用户恢复正常登录；<br>
    5. 安全模式状态 <b>不持久化</b>，服务器重启后自动解除。
  </div>
  <form method='post' action='/admin/safe-mode' class='form-row' style='margin-top:16px;'>
    <input type='hidden' name='action' value='enter'>
    <input type='password' name='root_pw' placeholder='输入 root 密码以确认' required style='max-width:260px;'>
    <button type='submit' class='btn btn-danger'>我已了解，进入安全模式</button>
  </form>
</div>
)";
    }
    return pageShell(req, "安全模式", body, &user);
}

// ============================================================
// 聊天室独立页（5 秒轮询）
// ============================================================

QString chatPage(const HttpRequest& req, const HttpUser& user) {
    const bool muted = g_mutedUsers.contains(req.username);
    const bool isRoot = user.isRoot;

    QString body;
    body += "<div class='toolbar'><a class='btn' href='/'>← 返回文件管理</a></div>\n";

    // 权限申请区（仅非 root）
    if (!isRoot) {
        body += R"(
<div class='card'>
  <h3>📩 向 Root 申请权限</h3>
  <div style='display:flex;gap:10px;flex-wrap:wrap;align-items:center;'>
)";
        if (muted) {
            body += "<span class='msg msg-err' style='margin:0;'>你已被禁言，暂时无法发送消息或提交权限申请。</span>";
        } else {
            body += R"(
    <select id='reqTarget' style='max-width:180px;'>
      <option value='read'>读权限 (R)</option>
      <option value='write'>写权限 (W)</option>
      <option value='exec'>运行权限 (X)</option>
      <option value='root'>ROOT 权限</option>
    </select>
    <button class='btn btn-primary' onclick='submitRequest()'>提交申请</button>
    <span class='muted' id='reqHint'></span>
)";
        }
        body += "  </div>\n</div>\n";
    }

    // 聊天区
    body += QString(R"(
<div class='card'>
  <h3>💬 公共聊天室 <span class='muted' style='font-weight:400;'>（每 5 秒刷新）</span></h3>
  <div class='chatbox' id='chatBox'><div class='empty'>加载中...</div></div>
)");

    if (muted) {
        body += "<div class='msg msg-err'>你已被 root 禁言，无法发送消息。</div>";
    } else {
        body += QString(R"(
  <form id='chatForm' class='form-row'>
    <input type='text' id='chatInput' placeholder='输入消息...' maxlength='500' style='max-width:none;flex:1;' autocomplete='off'>
    <button type='submit' class='btn btn-primary'>发送</button>
  </form>
)");
    }

    body += R"(
</div>
<script>
const MY_NAME=)"
        + QJsonDocument(QJsonArray{req.username}).toJson(QJsonDocument::Compact)
        + R"(;
const MY_NAME_STR=MY_NAME[0];
function esc(s){const d=document.createElement('div');d.innerText=s;return d.innerHTML;}
function renderChat(list){
  const box=document.getElementById('chatBox');
  if(!list.length){box.innerHTML="<div class='empty'>暂无消息，来说第一句吧</div>";return;}
  let stick=box.scrollTop+box.clientHeight>=box.scrollHeight-30;
  let html="";
  list.forEach(m=>{
    const cls=m.root?"chat-msg root":"chat-msg";
    html+="<div class='"+cls+"'><div class='chat-meta'>"+esc(m.user)+" · "+m.time+"</div>"+esc(m.text)+"</div>";
  });
  box.innerHTML=html;
  if(stick)box.scrollTop=box.scrollHeight;
}
function fetchChat(){
  fetch('/api/chat').then(r=>r.json()).then(renderChat).catch(()=>{});
}
const form=document.getElementById('chatForm');
if(form){
  form.onsubmit=function(e){
    e.preventDefault();
    const input=document.getElementById('chatInput');
    const text=input.value.trim();
    if(!text)return;
    input.value='';
    fetch('/api/chat/send',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},
      body:'text='+encodeURIComponent(text)}).then(()=>fetchChat());
  };
}
function submitRequest(){
  const t=document.getElementById('reqTarget');
  if(!t)return;
  fetch('/api/request',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'target='+encodeURIComponent(t.value)})
    .then(r=>r.text()).then(msg=>{
      const h=document.getElementById('reqHint');
      if(h)h.innerText=msg;
    }).catch(()=>{});
}
fetchChat();
setInterval(fetchChat,5000);
</script>
)";
    return pageShell(req, "聊天室", body, &user);
}

} // namespace

// ===== END BATCH 4 =====

// ===== BATCH 5: HTTP 底层 - 请求解析 / 响应 / 表单 / multipart / 路由骨架 =====

namespace {

// ============================================================
// HTTP 请求解析
// ============================================================

// 将 "a=1&b=2" 解析为 QMap（已做百分号解码）
QMap<QString, QString> parseFormBody(const QString& body) {
    QMap<QString, QString> params;
    for (const QString& pair : body.split('&', Qt::SkipEmptyParts)) {
        const int eq = pair.indexOf('=');
        if (eq < 0) continue;
        const QString key = QUrl::fromPercentEncoding(pair.left(eq).toUtf8());
        const QString val = QUrl::fromPercentEncoding(pair.mid(eq + 1).toUtf8());
        params[key] = val;
    }
    return params;
}

QMap<QString, QString> parseQueryString(const QString& query) {
    return parseFormBody(query);
}

// 解析完整 HTTP 请求（header + body 已完整读入 data）
HttpRequest parseHttpRequest(const QByteArray& data, const QString& clientIp) {
    HttpRequest req;
    req.clientIp = clientIp;

    const int headerEnd = data.indexOf("\r\n\r\n");
    if (headerEnd < 0) return req;

    const QByteArray headerBytes = data.left(headerEnd);
    req.body = data.mid(headerEnd + 4);
    const QString headerText = QString::fromUtf8(headerBytes);

    const QStringList lines = headerText.split("\r\n", Qt::SkipEmptyParts);
    if (lines.isEmpty()) return req;

    // 请求行
    const QStringList reqLine = lines[0].split(' ');
    if (reqLine.size() < 2) return req;
    req.method = reqLine[0];
    req.rawPath = reqLine[1];

    // 分离 query
    if (req.rawPath.contains('?')) {
        const int idx = req.rawPath.indexOf('?');
        req.urlPath = req.rawPath.left(idx);
        req.query = parseQueryString(req.rawPath.mid(idx + 1));
    } else {
        req.urlPath = req.rawPath;
    }

    // headers（小写键）
    for (int i = 1; i < lines.size(); ++i) {
        const int colon = lines[i].indexOf(':');
        if (colon < 0) continue;
        const QString key = lines[i].left(colon).trimmed().toLower();
        const QString val = lines[i].mid(colon + 1).trimmed();
        req.headers[key] = val;
    }

    // sid
    req.sid = extractSid(headerText);
    if (!req.sid.isEmpty())
        req.username = g_sessions.value(req.sid, QString());

    req.valid = true;
    return req;
}

// ============================================================
// 响应发送
// ============================================================

static QString statusText(int code) {
    switch (code) {
    case 200: return "200 OK";
    case 302: return "302 Found";
    case 400: return "400 Bad Request";
    case 401: return "401 Unauthorized";
    case 403: return "403 Forbidden";
    case 404: return "404 Not Found";
    case 405: return "405 Method Not Allowed";
    case 413: return "413 Payload Too Large";
    case 429: return "429 Too Many Requests";
    case 500: return "500 Internal Server Error";
    default:  return "500 Internal Server Error";
    }
}

void sendResponse(QTcpSocket* client, int code, const QString& contentType,
                  const QByteArray& body, const QString& extraHeaders) {
    QByteArray resp;
    resp += "HTTP/1.1 " + statusText(code).toUtf8() + "\r\n";
    resp += "Content-Type: " + contentType.toUtf8() + "\r\n";
    resp += "Content-Length: " + QByteArray::number(body.size()) + "\r\n";
    resp += "X-Content-Type-Options: nosniff\r\n";
    resp += "X-Frame-Options: SAMEORIGIN\r\n";
    if (!extraHeaders.isEmpty())
        resp += extraHeaders.toUtf8() + "\r\n";
    resp += "Connection: close\r\n\r\n";
    resp += body;

    client->write(resp);
    client->flush();
    client->waitForBytesWritten(5000);
    client->close();
}

void redirect(QTcpSocket* client, const QString& location,
              const QString& extraHeaders) {
    QByteArray resp;
    resp += "HTTP/1.1 302 Found\r\n";
    resp += "Location: " + location.toUtf8() + "\r\n";
    if (!extraHeaders.isEmpty())
        resp += extraHeaders.toUtf8() + "\r\n";
    resp += "Content-Length: 0\r\nConnection: close\r\n\r\n";

    client->write(resp);
    client->flush();
    client->waitForBytesWritten(3000);
    client->close();
}

// 构造带文件名（保留后缀）的下载响应头
QByteArray buildMultipartZipResponse(const QByteArray& zipData, const QString& filename) {
    // 实际下载文件名在调用处通过 Content-Disposition 设置，这里仅返回数据
    Q_UNUSED(filename);
    return zipData;
}

// ============================================================
// Multipart 上传解析
// ============================================================

struct MultipartFile {
    QString filename;
    QByteArray data;
    bool ok = false;
};

// 只解析单个文件字段（name="file"），同时提取 cur_dir 文本字段
static MultipartFile parseMultipartUpload(const QByteArray& body,
                                          const QString& boundary,
                                          QString* curDir) {
    MultipartFile out;
    if (boundary.isEmpty()) return out;

    const QByteArray delim = "--" + boundary.toUtf8();
    const int start = body.indexOf(delim);
    if (start < 0) return out;

    // 遍历每个 part
    int pos = start;
    while (pos < body.size()) {
        const int next = body.indexOf(delim, pos + delim.size());
        if (next < 0) break;

        const QByteArray part = body.mid(pos + delim.size(), next - (pos + delim.size()));
        const int hEnd = part.indexOf("\r\n\r\n");
        if (hEnd >= 0) {
            const QByteArray partHeader = part.left(hEnd);
            const QByteArray partData = part.mid(hEnd + 4, part.size() - hEnd - 6); // 去掉尾部 \r\n

            const QString headerStr = QString::fromUtf8(partHeader);
            static const QRegularExpression nameRe(
                "name=\"([^\"]*)\"", QRegularExpression::CaseInsensitiveOption);
            static const QRegularExpression fileRe(
                "filename=\"([^\"]*)\"", QRegularExpression::CaseInsensitiveOption);

            const auto nameMatch = nameRe.match(headerStr);
            const auto fileMatch = fileRe.match(headerStr);

            if (nameMatch.hasMatch()) {
                const QString field = nameMatch.captured(1);
                if (field == "cur_dir" && curDir) {
                    *curDir = QString::fromUtf8(partData).trimmed();
                } else if (field == "file" && fileMatch.hasMatch()) {
                    out.filename = fileMatch.captured(1);
                    out.data = partData;
                    out.ok = true;
                }
            }
        }
        pos = next;
    }
    return out;
}

// ============================================================
// Cookie 辅助
// ============================================================

static QString buildSetCookie(const QString& sid, int maxAge) {
    QString cookie = "Set-Cookie: sid=" + sid + "; Path=/; HttpOnly; SameSite=Lax";
    if (maxAge >= 0)
        cookie += "; Max-Age=" + QString::number(maxAge);
    return cookie;
}

// ============================================================
// 权限判定 + 访问拦截辅助
// ============================================================

// 统一拦截：非 root 访问敏感路径
static bool rejectSensitive(QTcpSocket* client, const HttpRequest& req,
                            const QString& relPath) {
    if (isSensitivePath(relPath)) {
        const HttpUser u = g_users.value(req.username);
        if (!u.isRoot) {
            sendResponse(client, 403, "text/plain; charset=utf-8",
                         "403 Forbidden: 无权访问系统敏感文件");
            return true;
        }
    }
    return false;
}

// 统一要求登录；返回 true 表示已处理（未登录）
static bool requireLogin(QTcpSocket* client, const HttpRequest& req) {
    if (req.username.isEmpty() || !g_users.contains(req.username)) {
        redirect(client, "/login");
        return true;
    }
    return false;
}

// ============================================================
// 路由骨架
// ============================================================

void routeAuth(QTcpSocket* client, const HttpRequest& req, bool& handled);
void routeFiles(QTcpSocket* client, const HttpRequest& req, const HttpUser& user, bool& handled);
void routeAdmin(QTcpSocket* client, const HttpRequest& req, const HttpUser& user, bool& handled);
void routeApi(QTcpSocket* client, const HttpRequest& req, const HttpUser& user, bool& handled);

void handleRequest(QTcpSocket* client, const HttpRequest& req) {
    if (!req.valid) {
        sendResponse(client, 400, "text/plain; charset=utf-8", "400 Bad Request");
        return;
    }

    const QString ip = req.clientIp;

    // ---- IP 黑名单 ----
    if (g_blacklistedIPs.contains(ip)) {
        sendResponse(client, 403, "text/plain; charset=utf-8",
                     "403 Forbidden: Your IP has been blacklisted.");
        return;
    }

    // ---- 频率限制（静态资源与在线人数接口豁免） ----
    const bool exempt = (req.urlPath == "/favicon.ico" ||
                         req.urlPath == "/api/online_count");
    if (!exempt && !checkRateLimit(ip)) {
        sendResponse(client, 429, "text/plain; charset=utf-8",
                     "429 Too Many Requests");
        return;
    }

    // ---- 请求日志（跳过静态与轮询） ----
    if (!exempt && req.urlPath != "/api/chat" &&
        req.urlPath != "/api/admin/online" &&
        req.urlPath != "/api/admin/requests") {
        logRequest(QString("IP:%1 %2 %3 (user=%4)")
                       .arg(ip, req.method, req.rawPath,
                            req.username.isEmpty() ? "-" : req.username));
    }

    // ---- 在线心跳 ----
    if (!req.username.isEmpty())
        touchOnline(req.username, ip);

    bool handled = false;

    // 认证相关路由优先
    routeAuth(client, req, handled);
    if (handled) return;

    // 以下路由需登录
    if (requireLogin(client, req)) return;

    const HttpUser user = g_users.value(req.username);

    // API 路由
    routeApi(client, req, user, handled);
    if (handled) return;

    // 管理路由
    routeAdmin(client, req, user, handled);
    if (handled) return;

    // 文件路由
    routeFiles(client, req, user, handled);
    if (handled) return;

    sendResponse(client, 404, "text/plain; charset=utf-8", "404 Not Found");
}

} // namespace

// ===== END BATCH 5 =====

// ===== BATCH 6: 路由实现（一）- 认证 / 文件操作 =====

namespace {

// ============================================================
// 认证路由：登录 / 登出 / 应急通道
// ============================================================

void routeAuth(QTcpSocket* client, const HttpRequest& req, bool& handled) {
    // ---- GET /login ----
    if (req.urlPath == "/login" && req.method == "GET") {
        handled = true;
        if (!req.username.isEmpty()) { redirect(client, "/"); return; }
        sendResponse(client, 200, "text/html; charset=utf-8", loginPage().toUtf8());
        return;
    }

    // ---- POST /login ----
    if (req.urlPath == "/login" && req.method == "POST") {
        handled = true;
        const QMap<QString, QString> form = parseFormBody(QString::fromUtf8(req.body));
        const QString u = form.value("username").trimmed();
        const QString p = form.value("password");

        if (!g_users.contains(u) || !verifyPassword(g_users.value(u), p)) {
            sendResponse(client, 200, "text/html; charset=utf-8",
                         loginPage("用户名或密码错误").toUtf8());
            return;
        }

        // 安全模式：仅 root 可登录
        if (g_safeMode && !g_users.value(u).isRoot) {
            sendResponse(client, 200, "text/html; charset=utf-8",
                         loginPage("系统处于安全模式，仅 root 可登录").toUtf8());
            return;
        }

        // 会话全局唯一：已有活跃会话则拒绝
        if (hasActiveSession(u)) {
            sendResponse(client, 200, "text/html; charset=utf-8",
                         loginPage("该账号已在其他位置登录，如非本人请使用应急通道").toUtf8());
            return;
        }

        const QString sid = createSession(u, req.clientIp);
        logRequest(QString("[认证] 用户 %1 登录成功 (IP:%2)").arg(u, req.clientIp));
        redirect(client, "/", buildSetCookie(sid, 86400));
        return;
    }

    // ---- /logout ----
    if (req.urlPath == "/logout") {
        handled = true;
        if (!req.sid.isEmpty()) {
            const QString u = g_sessions.value(req.sid);
            destroySession(req.sid);
            if (!u.isEmpty()) {
                removeOnline(u);
                logRequest(QString("[认证] 用户 %1 退出登录").arg(u));
            }
        }
        redirect(client, "/login", buildSetCookie("", 0));
        return;
    }

    // ---- GET /emergency ----
    if (req.urlPath == "/emergency" && req.method == "GET") {
        handled = true;
        sendResponse(client, 200, "text/html; charset=utf-8",
                     emergencyPage().toUtf8());
        return;
    }

    // ---- POST /emergency ----
    if (req.urlPath == "/emergency" && req.method == "POST") {
        handled = true;
        const QMap<QString, QString> form = parseFormBody(QString::fromUtf8(req.body));
        const QString u = form.value("username").trimmed();
        const QString p = form.value("password");
        const QString np = form.value("new_password");

        if (!g_users.contains(u) || !verifyPassword(g_users.value(u), p)) {
            sendResponse(client, 200, "text/html; charset=utf-8",
                         emergencyPage("用户名或密码错误").toUtf8());
            return;
        }
        if (np.isEmpty()) {
            sendResponse(client, 200, "text/html; charset=utf-8",
                         emergencyPage("新密码不能为空").toUtf8());
            return;
        }

        // 改密码
        HttpUser& target = g_users[u];
        target.salt = randomSalt();
        target.hash = hashPassword(target.salt, np);
        saveUsers(g_users);

        // 踢出该用户所有会话
        destroySessionsOf(u);

        logRequest(QString("[认证] 用户 %1 通过应急通道改密并强制下线").arg(u));
        sendResponse(client, 200, "text/html; charset=utf-8",
                     emergencyPage(QString(), "密码已修改，原登录会话已失效，请重新登录").toUtf8());
        return;
    }
}

// ============================================================
// 文件路由
// ============================================================

// 统一处理"文件不存在/越界"错误
static void replyBadPath(QTcpSocket* client) {
    sendResponse(client, 400, "text/plain; charset=utf-8", "400 非法路径");
}

void routeFiles(QTcpSocket* client, const HttpRequest& req,
                const HttpUser& user, bool& handled) {
    const QString baseDir = g_httpUi ? g_httpUi->currentDir : QString();
    if (baseDir.isEmpty()) {
        handled = true;
        sendResponse(client, 500, "text/plain; charset=utf-8", "服务器根目录未设置");
        return;
    }

    const bool isRoot = user.isRoot;
    const bool canR = canRead(user);
    const bool canW = canWrite(user);
    const bool canX = canExec(user);

    // ========== GET / 与目录浏览 ==========
    if (req.method == "GET" && (req.urlPath == "/" || req.urlPath.isEmpty())) {
        handled = true;
        if (!canR) { sendResponse(client, 403, "text/plain; charset=utf-8", "403 无读权限"); return; }
        const QString html = generateIndexHtml(req, user, "/", baseDir);
        sendResponse(client, 200, "text/html; charset=utf-8", html.toUtf8());
        return;
    }

    if (req.method == "GET" && req.urlPath.endsWith('/') && req.urlPath != "/") {
        handled = true;
        if (!canR) { sendResponse(client, 403, "text/plain; charset=utf-8", "403 无读权限"); return; }

        const QString rel = req.urlPath; // 以 / 开头
        if (rejectSensitive(client, req, rel)) return;

        bool ok = false;
        const QString abs = safeJoin(baseDir, rel, &ok);
        if (!ok) { replyBadPath(client); return; }
        if (!QDir(abs).exists()) {
            sendResponse(client, 404, "text/plain; charset=utf-8", "404 Not Found");
            return;
        }
        const QString html = generateIndexHtml(req, user, rel, abs);
        sendResponse(client, 200, "text/html; charset=utf-8", html.toUtf8());
        return;
    }

    // ========== GET /download 与 /raw ==========
    if (req.method == "GET" && (req.urlPath == "/download" || req.urlPath == "/raw")) {
        handled = true;
        if (!canR) { sendResponse(client, 403, "text/plain; charset=utf-8", "403 无读权限"); return; }

        const QString rel = req.query.value("path");
        if (rejectSensitive(client, req, rel)) return;

        bool ok = false;
        const QString abs = safeJoin(baseDir, rel, &ok);
        if (!ok) { replyBadPath(client); return; }

        QFileInfo info(abs);
        if (!info.exists() || !info.isFile()) {
            sendResponse(client, 404, "text/plain; charset=utf-8", "404 Not Found");
            return;
        }

        QFile f(abs);
        if (!f.open(QIODevice::ReadOnly)) {
            sendResponse(client, 500, "text/plain; charset=utf-8", "500 读取失败");
            return;
        }
        const QByteArray data = f.readAll();
        f.close();

        QString contentType = "application/octet-stream";
        QString extraHeaders;
        const QString ext = info.suffix().toLower();

        if (req.urlPath == "/raw") {
            if (ext == "jpg" || ext == "jpeg") contentType = "image/jpeg";
            else if (ext == "png") contentType = "image/png";
            else if (ext == "gif") contentType = "image/gif";
            else if (ext == "webp") contentType = "image/webp";
            else if (ext == "mp4") contentType = "video/mp4";
            else if (ext == "mp3") contentType = "audio/mpeg";
            else if (ext == "webm") contentType = "video/webm";
        } else {
            // 下载：保留原始文件名与后缀
            const QString encoded = QString::fromLatin1(
                QUrl::toPercentEncoding(info.fileName()));
            extraHeaders = "Content-Disposition: attachment; filename*=UTF-8''" + encoded;
        }

        sendResponse(client, 200, contentType, data, extraHeaders);
        if (req.urlPath == "/download")
            logRequest(QString("[文件] 下载: %1").arg(info.fileName()));
        return;
    }

    // ========== POST /batch-download ==========
    if (req.method == "POST" && req.urlPath == "/batch-download") {
        handled = true;
        if (!canR) { sendResponse(client, 403, "text/plain; charset=utf-8", "403 无读权限"); return; }

        const QMap<QString, QString> form = parseFormBody(QString::fromUtf8(req.body));
        const QString curDir = form.value("cur_dir", "/");
        const QStringList names = form.value("paths").split('\n', Qt::SkipEmptyParts);
        if (names.isEmpty()) {
            sendResponse(client, 400, "text/plain; charset=utf-8", "400 未选择文件");
            return;
        }

        // 创建临时目录
        const QString tmpName = ".tmp_download_" + generateId();
        bool ok = false;
        const QString tmpAbs = safeJoin(baseDir, tmpName, &ok);
        if (!ok) { replyBadPath(client); return; }
        QDir().mkpath(tmpAbs);

        QList<QPair<QString, QByteArray>> entries;

        for (const QString& name : names) {
            const QString rel = curDir + (curDir.endsWith('/') ? "" : "/") + name;
            bool fok = false;
            const QString abs = safeJoin(baseDir, rel, &fok);
            if (!fok) continue;
            if (isSensitivePath(rel) && !isRoot) continue;

            QFileInfo info(abs);
            if (!info.exists()) continue;

            if (info.isFile()) {
                QFile f(abs);
                if (f.open(QIODevice::ReadOnly)) {
                    entries.append({name, f.readAll()});
                    f.close();
                }
            } else if (info.isDir()) {
                // 递归收集目录内文件
                QDirIterator it(abs, QDir::Files | QDir::NoDotAndDotDot,
                                QDirIterator::Subdirectories);
                const QString prefix = name + "/";
                while (it.hasNext()) {
                    const QString filePath = it.next();
                    const QString relInDir = QDir(abs).relativeFilePath(filePath);
                    QFile f(filePath);
                    if (f.open(QIODevice::ReadOnly)) {
                        entries.append({prefix + relInDir, f.readAll()});
                        f.close();
                    }
                }
            }
        }

        // 清理临时目录（本实现不落盘临时文件，直接内存打包；此处删除以防残留）
        QDir(tmpAbs).removeRecursively();

        if (entries.isEmpty()) {
            sendResponse(client, 404, "text/plain; charset=utf-8", "404 无有效文件");
            return;
        }

        const QByteArray zip = buildZipStore(entries);
        const QString zipName = "download_" +
            QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".zip";
        const QString encoded = QString::fromLatin1(QUrl::toPercentEncoding(zipName));
        const QString extra = "Content-Disposition: attachment; filename*=UTF-8''" + encoded;
        sendResponse(client, 200, "application/zip", zip, extra);
        logRequest(QString("[文件] 批量下载 %1 个文件").arg(entries.size()));
        return;
    }

    // ========== POST /delete ==========
    if (req.method == "POST" && req.urlPath == "/delete") {
        handled = true;
        if (!canW) { sendResponse(client, 403, "text/plain; charset=utf-8", "403 无写权限"); return; }

        const QMap<QString, QString> form = parseFormBody(QString::fromUtf8(req.body));
        const QString curDir = form.value("cur_dir", "/");
        const QString name = form.value("path");
        const QString rel = curDir + (curDir.endsWith('/') ? "" : "/") + name;
        if (rejectSensitive(client, req, rel)) return;

        bool ok = false;
        const QString abs = safeJoin(baseDir, rel, &ok);
        if (!ok) { replyBadPath(client); return; }

        QFileInfo info(abs);
        if (!info.exists()) { sendResponse(client, 404, "text/plain", "404"); return; }
        if (info.isDir()) QDir(abs).removeRecursively();
        else QFile::remove(abs);

        logRequest(QString("[文件] 删除: %1").arg(rel));
        sendResponse(client, 200, "text/plain; charset=utf-8", "OK");
        return;
    }

    // ========== POST /batch-delete ==========
    if (req.method == "POST" && req.urlPath == "/batch-delete") {
        handled = true;
        if (!canW) { sendResponse(client, 403, "text/plain; charset=utf-8", "403 无写权限"); return; }

        const QMap<QString, QString> form = parseFormBody(QString::fromUtf8(req.body));
        const QString curDir = form.value("cur_dir", "/");
        const QStringList names = form.value("paths").split('\n', Qt::SkipEmptyParts);

        for (const QString& name : names) {
            const QString rel = curDir + (curDir.endsWith('/') ? "" : "/") + name;
            if (isSensitivePath(rel) && !isRoot) continue;
            bool ok = false;
            const QString abs = safeJoin(baseDir, rel, &ok);
            if (!ok) continue;
            QFileInfo info(abs);
            if (!info.exists()) continue;
            if (info.isDir()) QDir(abs).removeRecursively();
            else QFile::remove(abs);
        }
        logRequest(QString("[文件] 批量删除 %1 项").arg(names.size()));
        sendResponse(client, 200, "text/plain; charset=utf-8", "OK");
        return;
    }

    // ========== POST /rename ==========
    if (req.method == "POST" && req.urlPath == "/rename") {
        handled = true;
        if (!canW) { sendResponse(client, 403, "text/plain; charset=utf-8", "403 无写权限"); return; }

        const QMap<QString, QString> form = parseFormBody(QString::fromUtf8(req.body));
        const QString curDir = form.value("cur_dir", "/");
        const QString oldName = form.value("old_name");
        const QString newName = form.value("new_name");

        const QString oldRel = curDir + (curDir.endsWith('/') ? "" : "/") + oldName;
        const QString newRel = curDir + (curDir.endsWith('/') ? "" : "/") + newName;
        if (rejectSensitive(client, req, oldRel) || rejectSensitive(client, req, newRel)) return;

        bool ok1 = false, ok2 = false;
        const QString oldAbs = safeJoin(baseDir, oldRel, &ok1);
        const QString newAbs = safeJoin(baseDir, newRel, &ok2);
        if (!ok1 || !ok2) { replyBadPath(client); return; }

        if (QFile::exists(oldAbs)) {
            if (QFile::exists(newAbs)) QFile::remove(newAbs);
            QFile::rename(oldAbs, newAbs);
            logRequest(QString("[文件] 重命名: %1 -> %2").arg(oldName, newName));
        }
        sendResponse(client, 200, "text/plain; charset=utf-8", "OK");
        return;
    }

    // ========== POST /move ==========
    if (req.method == "POST" && req.urlPath == "/move") {
        handled = true;
        if (!canW) { sendResponse(client, 403, "text/plain; charset=utf-8", "403 无写权限"); return; }

        const QMap<QString, QString> form = parseFormBody(QString::fromUtf8(req.body));
        const QString curDir = form.value("cur_dir", "/");
        const QString fileName = form.value("file_name");
        const QString destDir = form.value("dest_dir");

        const QString srcRel = curDir + (curDir.endsWith('/') ? "" : "/") + fileName;
        const QString dstRel = destDir + (destDir.endsWith('/') ? "" : "/") + fileName;
        if (rejectSensitive(client, req, srcRel) || rejectSensitive(client, req, dstRel)) return;

        bool ok1 = false, ok2 = false;
        const QString srcAbs = safeJoin(baseDir, srcRel, &ok1);
        const QString dstAbs = safeJoin(baseDir, dstRel, &ok2);
        if (!ok1 || !ok2) { replyBadPath(client); return; }

        QDir().mkpath(QFileInfo(dstAbs).absolutePath());
        if (QFile::exists(srcAbs)) {
            if (QFile::exists(dstAbs)) QFile::remove(dstAbs);
            QFile::rename(srcAbs, dstAbs);
            logRequest(QString("[文件] 移动: %1 -> %2").arg(fileName, destDir));
        }
        sendResponse(client, 200, "text/plain; charset=utf-8", "OK");
        return;
    }

    // ========== POST /mkdir 与 /touch ==========
    if (req.method == "POST" && (req.urlPath == "/mkdir" || req.urlPath == "/touch")) {
        handled = true;
        if (!canW) { sendResponse(client, 403, "text/plain; charset=utf-8", "403 无写权限"); return; }

        const QMap<QString, QString> form = parseFormBody(QString::fromUtf8(req.body));
        const QString curDir = form.value("cur_dir", "/");
        const QString name = form.value("name");
        const QString rel = curDir + (curDir.endsWith('/') ? "" : "/") + name;
        if (rejectSensitive(client, req, rel)) return;

        bool ok = false;
        const QString abs = safeJoin(baseDir, rel, &ok);
        if (!ok) { replyBadPath(client); return; }

        if (req.urlPath == "/mkdir") {
            QDir().mkpath(abs);
            logRequest(QString("[文件] 新建目录: %1").arg(rel));
        } else {
            QFile f(abs);
            if (!f.exists()) { f.open(QIODevice::WriteOnly); f.close(); }
            logRequest(QString("[文件] 新建文件: %1").arg(rel));
        }
        redirect(client, curDir);
        return;
    }

    // ========== POST /upload ==========
    if (req.method == "POST" && req.urlPath == "/upload") {
        handled = true;
        if (!canW) { sendResponse(client, 403, "text/plain; charset=utf-8", "403 无写权限"); return; }

        // 解析 boundary
        const QString ct = req.headers.value("content-type");
        static const QRegularExpression bRe("boundary=([^;\\s]+)");
        const auto bMatch = bRe.match(ct);
        if (!bMatch.hasMatch()) {
            sendResponse(client, 400, "text/plain; charset=utf-8", "400 缺少 boundary");
            return;
        }

        QString curDir = "/";
        const MultipartFile mf = parseMultipartUpload(req.body, bMatch.captured(1), &curDir);
        if (!mf.ok) {
            sendResponse(client, 400, "text/plain; charset=utf-8", "400 上传内容无效");
            return;
        }
        if (mf.data.size() > kMaxUploadBytes) {
            sendResponse(client, 413, "text/plain; charset=utf-8", "413 文件过大");
            return;
        }

        // 文件名安全检查：去掉路径分隔符
        QString filename = QFileInfo(mf.filename).fileName();
        if (filename.isEmpty()) filename = "upload_" + generateId();

        const QString rel = curDir + (curDir.endsWith('/') ? "" : "/") + filename;
        if (rejectSensitive(client, req, rel)) return;

        bool ok = false;
        const QString abs = safeJoin(baseDir, rel, &ok);
        if (!ok) { replyBadPath(client); return; }

        QDir().mkpath(QFileInfo(abs).absolutePath());
        QFile f(abs);
        if (f.open(QIODevice::WriteOnly)) {
            f.write(mf.data);
            f.close();
            logRequest(QString("[文件] 上传: %1 (%2 KB)")
                           .arg(filename).arg(mf.data.size() / 1024));
        }
        redirect(client, curDir.isEmpty() ? "/" : curDir);
        return;
    }

    // ========== GET /view（文本预览/编辑） ==========
    if (req.method == "GET" && req.urlPath == "/view") {
        handled = true;
        if (!canR) { sendResponse(client, 403, "text/plain; charset=utf-8", "403 无读权限"); return; }

        const QString rel = req.query.value("path");
        if (rejectSensitive(client, req, rel)) return;

        bool ok = false;
        const QString abs = safeJoin(baseDir, rel, &ok);
        if (!ok) { replyBadPath(client); return; }

        QFileInfo info(abs);
        if (!info.exists() || !info.isFile()) {
            sendResponse(client, 404, "text/plain; charset=utf-8", "404 文件不存在");
            return;
        }

        const QString ext = info.suffix().toLower();
        if (ext == "jpg" || ext == "jpeg" || ext == "png" || ext == "gif" ||
            ext == "webp" || ext == "mp4" || ext == "webm") {
            const QString type = (ext == "mp4" || ext == "webm") ? "video" : "image";
            const QString html = mediaPage(req, user, rel.mid(1), type);
            sendResponse(client, 200, "text/html; charset=utf-8", html.toUtf8());
            return;
        }

        if (info.size() > kMaxPreviewBytes) {
            sendResponse(client, 200, "text/plain; charset=utf-8",
                         "文件过大，无法预览（>1MB）");
            return;
        }

        QFile f(abs);
        if (!f.open(QIODevice::ReadOnly)) {
            sendResponse(client, 500, "text/plain; charset=utf-8", "500 读取失败");
            return;
        }
        const QByteArray raw = f.readAll();
        f.close();

        // 默认 UTF-8；非法序列回退本地编码
        QString content = QString::fromUtf8(raw);
        if (content.contains(QChar(0xFFFD)))
            content = QString::fromLocal8Bit(raw);

        const QString html = editPage(req, user, rel.mid(1), content, canW);
        sendResponse(client, 200, "text/html; charset=utf-8", html.toUtf8());
        return;
    }

    // ========== POST /save ==========
    if (req.method == "POST" && req.urlPath == "/save") {
        handled = true;
        if (!canW) { sendResponse(client, 403, "text/plain; charset=utf-8", "403 无写权限"); return; }

        const QMap<QString, QString> form = parseFormBody(QString::fromUtf8(req.body));
        const QString rel = form.value("path");
        if (rejectSensitive(client, req, rel)) return;

        bool ok = false;
        const QString abs = safeJoin(baseDir, rel, &ok);
        if (!ok) { replyBadPath(client); return; }

        QFile f(abs);
        if (f.open(QIODevice::WriteOnly)) {
            f.write(form.value("content").toUtf8());
            f.close();
            logRequest(QString("[文件] 保存: %1").arg(rel));
        }
        redirect(client, "/view?path=" + QUrl::toPercentEncoding(rel));
        return;
    }

    // ========== GET /run（运行 HTML） ==========
    if (req.method == "GET" && req.urlPath == "/run") {
        handled = true;
        if (!canR) { sendResponse(client, 403, "text/plain; charset=utf-8", "403 无读权限"); return; }
        if (!canX) {
            sendResponse(client, 403, "text/html; charset=utf-8",
                         "<h3>403 Forbidden</h3><p>您没有运行 HTML 的权限（需要 X 权限）。</p>");
            return;
        }

        const QString rel = req.query.value("path");
        if (rejectSensitive(client, req, rel)) return;

        bool ok = false;
        const QString abs = safeJoin(baseDir, rel, &ok);
        if (!ok) { replyBadPath(client); return; }

        QFileInfo info(abs);
        if (!info.exists() || !info.isFile()) {
            sendResponse(client, 404, "text/plain; charset=utf-8", "404 文件不存在");
            return;
        }

        const QString ext = info.suffix().toLower();
        if (ext != "html" && ext != "htm") {
            sendResponse(client, 403, "text/plain; charset=utf-8", "403 仅可运行 HTML 文件");
            return;
        }

        QFile f(abs);
        if (!f.open(QIODevice::ReadOnly)) {
            sendResponse(client, 500, "text/plain; charset=utf-8", "500 读取失败");
            return;
        }
        const QByteArray content = f.readAll();
        f.close();
        sendResponse(client, 200, "text/html; charset=utf-8", content);
        logRequest(QString("[文件] 运行 HTML: %1").arg(info.fileName()));
        return;
    }

    // ========== GET /props（文件属性 JSON） ==========
    if (req.method == "GET" && req.urlPath == "/props") {
        handled = true;
        if (!canR) { sendResponse(client, 403, "text/plain; charset=utf-8", "403 无读权限"); return; }

        const QString curDir = req.query.value("cur_dir", "/");
        const QString name = req.query.value("name");
        const QString rel = curDir + (curDir.endsWith('/') ? "" : "/") + name;
        if (rejectSensitive(client, req, rel)) return;

        bool ok = false;
        const QString abs = safeJoin(baseDir, rel, &ok);
        if (!ok) { replyBadPath(client); return; }

        QFileInfo info(abs);
        if (!info.exists()) {
            sendResponse(client, 404, "text/plain; charset=utf-8", "404 文件不存在");
            return;
        }

        QJsonObject obj;
        obj["name"] = info.fileName();
        obj["size"] = info.isDir() ? QString("-") : Utils::formatSize(info.size());
        obj["type"] = info.isDir() ? "文件夹" : (info.suffix().isEmpty() ? "文件" : info.suffix());
        obj["path"] = rel;
        obj["mtime"] = info.lastModified().toString("yyyy-MM-dd hh:mm:ss");
        obj["perm"] = info.isDir() ? "目录" :
                      (info.isWritable() ? "可读写" : "只读");

        sendResponse(client, 200, "application/json; charset=utf-8",
                     QJsonDocument(obj).toJson(QJsonDocument::Compact));
        return;
    }
}

} // namespace

// ===== END BATCH 6 =====

// ===== BATCH 7: 路由实现（二）- Admin / API / 命令注册 =====

namespace {

// ============================================================
// Admin 路由
// ============================================================

void routeAdmin(QTcpSocket* client, const HttpRequest& req,
                const HttpUser& user, bool& handled) {
    if (!req.urlPath.startsWith("/admin")) return;

    handled = true;

    // 所有 /admin 路由要求 root
    if (!user.isRoot) {
        sendResponse(client, 403, "text/plain; charset=utf-8",
                     "403 Forbidden: 需要 root 权限");
        return;
    }

    // ========== GET /admin/users 控制台 ==========
    if (req.urlPath == "/admin/users" && req.method == "GET") {
        sendResponse(client, 200, "text/html; charset=utf-8",
                     adminPage(req, user).toUtf8());
        return;
    }

    // ========== GET /admin/online 在线用户独立页 ==========
    if (req.urlPath == "/admin/online" && req.method == "GET") {
        sendResponse(client, 200, "text/html; charset=utf-8",
                     adminOnlinePage(req, user).toUtf8());
        return;
    }

    // ========== GET /admin/requests 权限请求独立页 ==========
    if (req.urlPath == "/admin/requests" && req.method == "GET") {
        sendResponse(client, 200, "text/html; charset=utf-8",
                     adminRequestsPage(req, user).toUtf8());
        return;
    }

    // ========== GET /admin/blacklist 黑名单独立页 ==========
    if (req.urlPath == "/admin/blacklist" && req.method == "GET") {
        sendResponse(client, 200, "text/html; charset=utf-8",
                     adminBlacklistPage(req, user).toUtf8());
        return;
    }

    // ========== GET /admin/logs 日志 ==========
    if (req.urlPath == "/admin/logs" && req.method == "GET") {
        QString date = req.query.value("date");
        if (date.isEmpty()) date = dateString();
        const QString logPath = logDirPath() + "/" + date + ".log";
        QFile f(logPath);
        QString content = "暂无日志或文件不存在";
        if (f.open(QIODevice::ReadOnly)) {
            content = QString::fromUtf8(f.readAll());
            f.close();
        }
        sendResponse(client, 200, "text/html; charset=utf-8",
                     adminLogsPage(req, user, date, content).toUtf8());
        return;
    }

    // ========== GET /admin/safe-mode 安全模式页 ==========
    if (req.urlPath == "/admin/safe-mode" && req.method == "GET") {
        sendResponse(client, 200, "text/html; charset=utf-8",
                     safeModePage(req, user).toUtf8());
        return;
    }

    // ========== POST /admin/create 创建账户 ==========
    if (req.urlPath == "/admin/create" && req.method == "POST") {
        const QMap<QString, QString> form = parseFormBody(QString::fromUtf8(req.body));

        if (!verifyRootPassword(form.value("root_pw"))) {
            sendResponse(client, 200, "text/html; charset=utf-8",
                         adminPage(req, user, "root 密码验证错误", true).toUtf8());
            return;
        }
        const QString u = form.value("username").trimmed();
        const QString p = form.value("password");
        if (u.isEmpty() || p.isEmpty() || g_users.contains(u)) {
            sendResponse(client, 200, "text/html; charset=utf-8",
                         adminPage(req, user, "用户名无效或已存在", true).toUtf8());
            return;
        }
        HttpUser nu;
        nu.salt = randomSalt();
        nu.hash = hashPassword(nu.salt, p);
        nu.r = form.contains("r");
        nu.w = form.contains("w");
        nu.x = form.contains("x");
        // 写/运行必须有读
        if (!nu.r) { nu.w = false; nu.x = false; }
        g_users[u] = nu;
        saveUsers(g_users);
        logRequest(QString("[管理] 创建用户: %1 (%2)").arg(u, permString(nu)));
        sendResponse(client, 200, "text/html; charset=utf-8",
                     adminPage(req, user, QString("账户 %1 已创建").arg(u)).toUtf8());
        return;
    }

    // ========== POST /admin/passwd 修改 root 自身密码 ==========
    if (req.urlPath == "/admin/passwd" && req.method == "POST") {
        const QMap<QString, QString> form = parseFormBody(QString::fromUtf8(req.body));
        const QString oldPw = form.value("old_password");
        const QString newPw = form.value("password");

        if (!verifyRootPassword(oldPw)) {
            sendResponse(client, 200, "text/html; charset=utf-8",
                         adminPage(req, user, "当前 root 密码错误", true).toUtf8());
            return;
        }
        if (newPw.isEmpty()) {
            sendResponse(client, 200, "text/html; charset=utf-8",
                         adminPage(req, user, "新密码不能为空", true).toUtf8());
            return;
        }
        HttpUser& root = g_users["root"];
        root.salt = randomSalt();
        root.hash = hashPassword(root.salt, newPw);
        saveUsers(g_users);
        logRequest("[管理] Root 密码已更新");
        sendResponse(client, 200, "text/html; charset=utf-8",
                     adminPage(req, user, "Root 密码修改成功").toUtf8());
        return;
    }

    // ========== POST /admin/manage 用户管理 ==========
    if (req.urlPath == "/admin/manage" && req.method == "POST") {
        const QMap<QString, QString> form = parseFormBody(QString::fromUtf8(req.body));
        if (!verifyRootPassword(form.value("root_pw"))) {
            sendResponse(client, 200, "text/html; charset=utf-8",
                         adminPage(req, user, "root 密码验证错误", true).toUtf8());
            return;
        }
        const QString u = form.value("username");
        const QString action = form.value("action");
        const QString val = form.value("new_val");

        if (!g_users.contains(u) || u == "root") {
            redirect(client, "/admin/users");
            return;
        }

        HttpUser& target = g_users[u];
        if (action == "grant") {
            target.r = val.contains("r", Qt::CaseInsensitive);
            target.w = val.contains("w", Qt::CaseInsensitive);
            target.x = val.contains("x", Qt::CaseInsensitive);
            if (!target.r) { target.w = false; target.x = false; }
            destroySessionsOf(u); // 权限变更后强制重新登录
        } else if (action == "passwd") {
            if (!val.isEmpty()) {
                target.salt = randomSalt();
                target.hash = hashPassword(target.salt, val);
                destroySessionsOf(u);
            }
        } else if (action == "promote") {
            target.isRoot = true;
            target.r = target.w = target.x = true;
        } else if (action == "revoke") {
            target.isRoot = false;
        } else if (action == "delete") {
            g_users.remove(u);
            destroySessionsOf(u);
        }
        saveUsers(g_users);
        logRequest(QString("[管理] 用户操作: %1 -> %2").arg(u, action));
        sendResponse(client, 200, "text/html; charset=utf-8",
                     adminPage(req, user, "操作已执行").toUtf8());
        return;
    }

    // ========== POST /admin/blacklist 与 /admin/unblacklist ==========
    if (req.method == "POST" &&
        (req.urlPath == "/admin/blacklist" || req.urlPath == "/admin/unblacklist")) {
        const QMap<QString, QString> form = parseFormBody(QString::fromUtf8(req.body));
        const QString ip = form.value("ip").trimmed();
        if (!ip.isEmpty()) {
            if (req.urlPath == "/admin/blacklist") {
                g_blacklistedIPs.insert(ip);
                logRequest(QString("[防火墙] 封禁 IP: %1").arg(ip));
            } else {
                g_blacklistedIPs.remove(ip);
                logRequest(QString("[防火墙] 解封 IP: %1").arg(ip));
            }
            saveBlacklist();
        }
        redirect(client, "/admin/blacklist");
        return;
    }

    // ========== POST /admin/kick 踢出用户 ==========
    if (req.urlPath == "/admin/kick" && req.method == "POST") {
        const QMap<QString, QString> form = parseFormBody(QString::fromUtf8(req.body));
        const QString u = form.value("username");
        if (!u.isEmpty() && u != "root") {
            destroySessionsOf(u);
            logRequest(QString("[管理] 踢出用户: %1").arg(u));
        }
        sendResponse(client, 200, "text/plain; charset=utf-8", "OK");
        return;
    }

    // ========== POST /admin/mute 禁言/解禁 ==========
    if (req.urlPath == "/admin/mute" && req.method == "POST") {
        const QMap<QString, QString> form = parseFormBody(QString::fromUtf8(req.body));
        const QString u = form.value("username");
        const bool mute = form.value("mute") == "1";
        if (!u.isEmpty() && u != "root") {
            if (mute) g_mutedUsers.insert(u);
            else g_mutedUsers.remove(u);
            logRequest(QString("[管理] %1 用户: %2").arg(mute ? "禁言" : "解除禁言", u));
        }
        sendResponse(client, 200, "text/plain; charset=utf-8", "OK");
        return;
    }

    // ========== POST /admin/request-handle 处理权限请求 ==========
    if (req.urlPath == "/admin/request-handle" && req.method == "POST") {
        const QMap<QString, QString> form = parseFormBody(QString::fromUtf8(req.body));
        const QString u = form.value("username");
        const QString id = form.value("id");
        const bool approve = form.value("approve") == "1";
        handlePermissionRequest(u, id, approve, req.username);
        sendResponse(client, 200, "text/plain; charset=utf-8", "OK");
        return;
    }

    // ========== POST /admin/safe-mode 进入/退出安全模式 ==========
    if (req.urlPath == "/admin/safe-mode" && req.method == "POST") {
        const QMap<QString, QString> form = parseFormBody(QString::fromUtf8(req.body));
        if (!verifyRootPassword(form.value("root_pw"))) {
            sendResponse(client, 200, "text/html; charset=utf-8",
                         safeModePage(req, user).toUtf8());
            return;
        }
        const QString action = form.value("action");
        if (action == "enter" && !g_safeMode) {
            g_safeMode = true;
            // 踢出所有非 root 会话
            QStringList nonRootSids;
            for (auto it = g_sessions.begin(); it != g_sessions.end(); ++it) {
                const QString uname = it.value();
                if (uname != "root") nonRootSids.append(it.key());
            }
            for (const QString& sid : nonRootSids) {
                const QString uname = g_sessions.value(sid);
                destroySession(sid);
                removeOnline(uname);
            }
            logRequest("[管理] 已进入安全模式，所有非 root 用户被强制下线");
        } else if (action == "exit" && g_safeMode) {
            g_safeMode = false;
            logRequest("[管理] 已退出安全模式");
        }
        redirect(client, "/admin/safe-mode");
        return;
    }

    // 未匹配的 /admin 路由
    sendResponse(client, 404, "text/plain; charset=utf-8", "404 Not Found");
}

// ============================================================
// API 路由（在线人数 / 在线列表 / 权限请求 / 聊天）
// ============================================================

void routeApi(QTcpSocket* client, const HttpRequest& req,
              const HttpUser& user, bool& handled) {
    if (!req.urlPath.startsWith("/api/")) return;

    handled = true;

    // ---- GET /api/online_count（登录即可）----
    if (req.urlPath == "/api/online_count" && req.method == "GET") {
        sendResponse(client, 200, "text/plain; charset=utf-8",
                     QByteArray::number(countOnline()));
        return;
    }

    // ---- GET /api/admin/online（root）----
    if (req.urlPath == "/api/admin/online" && req.method == "GET") {
        if (!user.isRoot) {
            sendResponse(client, 403, "text/plain; charset=utf-8", "403");
            return;
        }
        const qint64 now = QDateTime::currentSecsSinceEpoch();
        QJsonArray arr;
        for (auto it = g_onlineUsers.begin(); it != g_onlineUsers.end(); ++it) {
            if (now - it.value().lastSeen >= kOnlineTimeoutSecs) continue;
            const QString uname = it.key();
            QJsonObject o;
            o["name"] = uname;
            o["ip"] = it.value().ip;
            o["last"] = QDateTime::fromSecsSinceEpoch(it.value().lastSeen)
                            .toString("hh:mm:ss");
            o["root"] = g_users.value(uname).isRoot;
            o["muted"] = g_mutedUsers.contains(uname);
            arr.append(o);
        }
        sendResponse(client, 200, "application/json; charset=utf-8",
                     QJsonDocument(arr).toJson(QJsonDocument::Compact));
        return;
    }

    // ---- GET /api/admin/requests（root）----
    if (req.urlPath == "/api/admin/requests" && req.method == "GET") {
        if (!user.isRoot) {
            sendResponse(client, 403, "text/plain; charset=utf-8", "403");
            return;
        }
        expirePermissionRequests();
        const qint64 now = QDateTime::currentSecsSinceEpoch();
        QJsonArray arr;
        for (auto it = g_requests.begin(); it != g_requests.end(); ++it) {
            for (const PermissionRequest& r : it.value()) {
                if (r.state != RequestState::Pending) continue;
                QJsonObject o;
                o["id"] = r.id;
                o["user"] = r.username;
                o["target"] = (r.target == RequestTarget::Read ? "读权限 R" :
                               r.target == RequestTarget::Write ? "写权限 W" :
                               r.target == RequestTarget::Exec ? "运行权限 X" : "ROOT");
                o["left"] = qMax<qint64>(0, r.expireAt - now);
                arr.append(o);
            }
        }
        sendResponse(client, 200, "application/json; charset=utf-8",
                     QJsonDocument(arr).toJson(QJsonDocument::Compact));
        return;
    }

    // ---- POST /api/request（非 root 提交权限申请）----
    if (req.urlPath == "/api/request" && req.method == "POST") {
        if (user.isRoot) {
            sendResponse(client, 200, "text/plain; charset=utf-8",
                         "root 无需申请权限");
            return;
        }
        if (g_mutedUsers.contains(req.username)) {
            sendResponse(client, 200, "text/plain; charset=utf-8",
                         "你已被禁言，无法提交申请");
            return;
        }
        const qint64 now = QDateTime::currentSecsSinceEpoch();
        const qint64 cd = g_requestCooldown.value(req.username, 0);
if (cd > now) {
    sendResponse(client, 200, "text/plain; charset=utf-8",
                 QString("冷却中，请 %1 秒后再试").arg(cd - now).toUtf8());
    return;
}
        const QMap<QString, QString> form = parseFormBody(QString::fromUtf8(req.body));
        const QString t = form.value("target");
        RequestTarget target = RequestTarget::Read;
        if (t == "write") target = RequestTarget::Write;
        else if (t == "exec") target = RequestTarget::Exec;
        else if (t == "root") target = RequestTarget::Root;

        submitPermissionRequest(req.username, target);
        sendResponse(client, 200, "text/plain; charset=utf-8",
                     "申请已提交，30 秒内 root 未处理将自动取消");
        return;
    }

    // ---- GET /api/chat（登录即可）----
    if (req.urlPath == "/api/chat" && req.method == "GET") {
        sendResponse(client, 200, "application/json; charset=utf-8",
                     QJsonDocument(chatMessagesJson()).toJson(QJsonDocument::Compact));
        return;
    }

    // ---- POST /api/chat/send ----
    if (req.urlPath == "/api/chat/send" && req.method == "POST") {
        if (g_mutedUsers.contains(req.username)) {
            sendResponse(client, 403, "text/plain; charset=utf-8", "已被禁言");
            return;
        }
        const QMap<QString, QString> form = parseFormBody(QString::fromUtf8(req.body));
        QString text = form.value("text").trimmed();
        if (text.isEmpty()) {
            sendResponse(client, 400, "text/plain; charset=utf-8", "空消息");
            return;
        }
        if (text.size() > 500) text = text.left(500);
        addChatMessage(req.username, text, user.isRoot);
        sendResponse(client, 200, "text/plain; charset=utf-8", "OK");
        return;
    }

    sendResponse(client, 404, "text/plain; charset=utf-8", "404 Not Found");
}

} // namespace

// ============================================================
// 权限请求 / 聊天室 业务实现（放在路由之后，前面已声明）
// ============================================================

namespace {

void submitPermissionRequest(const QString& username, RequestTarget target) {
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    PermissionRequest r;
    r.id = generateId();
    r.username = username;
    r.target = target;
    r.createdAt = now;
    r.expireAt = now + kRequestTimeoutSecs;
    r.state = RequestState::Pending;

    QList<PermissionRequest>& list = g_requests[username];
    list.append(r);
    while (list.size() > kMaxRequestPerUser) list.removeFirst();

    logRequest(QString("[请求] 用户 %1 申请权限: %2")
                   .arg(username,
                        target == RequestTarget::Read ? "R" :
                        target == RequestTarget::Write ? "W" :
                        target == RequestTarget::Exec ? "X" : "ROOT"));
}

void expirePermissionRequests() {
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    for (auto it = g_requests.begin(); it != g_requests.end(); ++it) {
        for (PermissionRequest& r : it.value()) {
            if (r.state == RequestState::Pending && now >= r.expireAt) {
                r.state = RequestState::Expired;
                r.handledAt = now;
                g_requestCooldown[r.username] = now + kRequestCooldownSecs;
                logRequest(QString("[请求] 用户 %1 的权限申请已超时自动取消").arg(r.username));
            }
        }
    }
}

QList<PermissionRequest> pendingRequests() {
    expirePermissionRequests();
    QList<PermissionRequest> out;
    for (auto it = g_requests.begin(); it != g_requests.end(); ++it) {
        for (const PermissionRequest& r : it.value()) {
            if (r.state == RequestState::Pending) out.append(r);
        }
    }
    return out;
}

PermissionRequest* findRequest(const QString& username, const QString& id) {
    auto it = g_requests.find(username);
    if (it == g_requests.end()) return nullptr;
    for (PermissionRequest& r : it.value()) {
        if (r.id == id) return &r;
    }
    return nullptr;
}

void handlePermissionRequest(const QString& username, const QString& id,
                             bool approve, const QString& byWhom) {
    PermissionRequest* r = findRequest(username, id);
    if (!r || r->state != RequestState::Pending) return;

    const qint64 now = QDateTime::currentSecsSinceEpoch();
    r->state = approve ? RequestState::Approved : RequestState::Rejected;
    r->handledBy = byWhom;
    r->handledAt = now;
    g_requestCooldown[username] = now + kRequestCooldownSecs;

    if (approve && g_users.contains(username)) {
        HttpUser& u = g_users[username];
        switch (r->target) {
        case RequestTarget::Read:  u.r = true; break;
        case RequestTarget::Write: u.r = true; u.w = true; break;
        case RequestTarget::Exec:  u.r = true; u.x = true; break;
        case RequestTarget::Root:  u.isRoot = true; u.r = u.w = u.x = true; break;
        }
        saveUsers(g_users);
        destroySessionsOf(username);
        logRequest(QString("[请求] 已批准 %1 的权限申请，需重新登录生效").arg(username));
    } else {
        logRequest(QString("[请求] 已拒绝 %1 的权限申请").arg(username));
    }
}

void addChatMessage(const QString& username, const QString& text, bool isRoot) {
    ChatMessage m;
    m.timestamp = QDateTime::currentSecsSinceEpoch();
    m.username = username;
    m.text = text;
    m.isRoot = isRoot;
    g_chatMessages.append(m);
    while (g_chatMessages.size() > kMaxChatMessages)
        g_chatMessages.removeFirst();
}

QJsonArray chatMessagesJson() {
    QJsonArray arr;
    for (const ChatMessage& m : g_chatMessages) {
        QJsonObject o;
        o["user"] = m.username;
        o["text"] = m.text;
        o["time"] = QDateTime::fromSecsSinceEpoch(m.timestamp).toString("hh:mm:ss");
        o["root"] = m.isRoot;
        arr.append(o);
    }
    return arr;
}

} // namespace

// ============================================================
// 命令注册
// ============================================================

void CommandRegistry::registerNetwork2Commands(TerminalWidget* ui) {
    // ---------------- serve ----------------
    registerCommand("serve", [ui](const QStringList& args) {
        int port = kDefaultPort;
        if (!args.isEmpty()) {
            bool ok = false;
            const int p = args[0].toInt(&ok);
            if (ok && p > 0 && p < 65536) port = p;
        }

        if (g_httpServer && g_httpServer->isListening()) {
            ui->appendOutput(QString("服务器已在端口 %1 运行\n")
                                 .arg(g_httpServer->serverPort()),
                             ERROR_COLOR);
            return;
        }

        // 初始化数据
        g_users = loadUsers();
        loadBlacklist();
        g_chatMessages.clear();
        g_requests.clear();
        g_requestCooldown.clear();
        g_mutedUsers.clear();
        g_onlineUsers.clear();
        g_safeMode = false;

        g_httpUi = ui;
        g_httpServer = new QTcpServer();

        // ---- 定时器：在线清理 / 请求过期 / 频率限制清理 ----
        QTimer* timer = new QTimer(g_httpServer);
        QObject::connect(timer, &QTimer::timeout, []() {
            const qint64 now = QDateTime::currentSecsSinceEpoch();

            // 在线用户清理（超时离线）
            for (auto it = g_onlineUsers.begin(); it != g_onlineUsers.end(); ) {
                if (now - it.value().lastSeen > kOnlineTimeoutSecs * 10)
                    it = g_onlineUsers.erase(it);
                else
                    ++it;
            }

            // 会话空闲清理
            for (auto it = g_sessionInfo.begin(); it != g_sessionInfo.end(); ) {
                if (now - it.value().lastActive > kSessionIdleTimeoutSecs) {
                    const QString sid = it.key();
                    const QString uname = it.value().username;
                    it = g_sessionInfo.erase(it);
                    g_sessions.remove(sid);
                    if (!g_sessions.values().contains(uname))
                        removeOnline(uname);
                } else {
                    ++it;
                }
            }

            // 权限请求过期
            expirePermissionRequests();

            // 频率限制清理
            cleanupRateLimits();
        });
        timer->start(5000);

        // ---- 新连接 ----
        QObject::connect(g_httpServer, &QTcpServer::newConnection, [ui]() {
            while (g_httpServer->hasPendingConnections()) {
                QTcpSocket* client = g_httpServer->nextPendingConnection();

                QObject::connect(client, &QTcpSocket::readyRead, [client, ui]() {
                    Q_UNUSED(ui);
                    // 累积数据
                    QByteArray buf = client->property("buf").toByteArray();
                    buf += client->readAll();
                    client->setProperty("buf", buf);

                    const int headerEnd = buf.indexOf("\r\n\r\n");
                    if (headerEnd < 0) return;

                    // 检查 Content-Length，未收全则继续等
                    const QString headerText = QString::fromUtf8(buf.left(headerEnd));
                    static const QRegularExpression clRe(
                        "Content-Length:\\s*(\\d+)", QRegularExpression::CaseInsensitiveOption);
                    const auto clMatch = clRe.match(headerText);
                    if (clMatch.hasMatch()) {
                        const int need = clMatch.captured(1).toInt();
                        if (buf.size() < headerEnd + 4 + need) return; // 继续等
                    }

                    client->setProperty("buf", QByteArray());

                    QString ip = client->peerAddress().toString();
                    if (ip.startsWith("::ffff:")) ip = ip.mid(7);

                    const HttpRequest req = parseHttpRequest(buf, ip);
                    handleRequest(client, req);
                });

                QObject::connect(client, &QTcpSocket::disconnected,
                                 client, &QTcpSocket::deleteLater);
            }
        });

        // ---- 监听 ----
        if (g_httpServer->listen(QHostAddress::Any, port)) {
            const QString localIp = []() {
                for (const QNetworkInterface& iface : QNetworkInterface::allInterfaces()) {
                    if (!(iface.flags() & QNetworkInterface::IsUp)) continue;
                    if (iface.flags() & QNetworkInterface::IsLoopBack) continue;
                    for (const QNetworkAddressEntry& e : iface.addressEntries()) {
                        if (e.ip().protocol() == QAbstractSocket::IPv4Protocol)
                            return e.ip().toString();
                    }
                }
                return QString("127.0.0.1");
            }();

            ui->appendOutput("HTTP 文件服务器已启动\n", PROMPT_COLOR);
            ui->appendOutput(QString("监听地址: http://%1:%2/\n").arg(localIp).arg(port), INFO_COLOR);
            ui->appendOutput(QString("本地地址: http://127.0.0.1:%2/\n").arg(port), INFO_COLOR);
            ui->appendOutput(QString("默认账户: root / %1\n").arg(kDefaultRootPassword), INFO_COLOR);
            ui->appendOutput("日志目录: serve-log/\n", INFO_COLOR);

            // 清理残留临时下载目录
            if (!ui->currentDir.isEmpty()) {
                QDir dir(ui->currentDir);
                for (const QString& d : dir.entryList({".tmp_download_*"},
                                                      QDir::Dirs | QDir::NoDotAndDotDot)) {
                    QDir(dir.absoluteFilePath(d)).removeRecursively();
                }
            }
        } else {
            ui->appendOutput("服务器启动失败\n", ERROR_COLOR);
            delete g_httpServer;
            g_httpServer = nullptr;
            g_httpUi = nullptr;
        }
    }, "HTTP 文件服务器", "serve [端口]");

    // ---------------- stop-serve ----------------
    registerCommand("stop-serve", [ui](const QStringList&) {
        if (g_httpServer && g_httpServer->isListening()) {
            g_httpServer->close();
            delete g_httpServer;
            g_httpServer = nullptr;
            g_onlineUsers.clear();
            g_sessions.clear();
            g_sessionInfo.clear();
            g_httpUi = nullptr;
            ui->appendOutput("服务器已关闭\n", INFO_COLOR);
        } else {
            ui->appendOutput("服务器未运行\n", ERROR_COLOR);
        }
    }, "关闭 HTTP 服务器", "stop-serve");
}

// ============================================================
// 清理
// ============================================================

namespace Network2Cleanup {
void cleanup() {
    if (g_httpServer) {
        if (g_httpServer->isListening()) g_httpServer->close();
        delete g_httpServer;
        g_httpServer = nullptr;
    }
    g_httpUi = nullptr;
    g_onlineUsers.clear();
    g_sessions.clear();
    g_sessionInfo.clear();
}
}

// ===== END BATCH 7 =====
// ===== 拼接完成 =====
