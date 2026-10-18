#ifndef UTILS_H
#define UTILS_H

#include <QString>
#include <QStringList>
#include <QProcess>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <functional>

class TerminalUI;

// ============ 工具类 ============
class Utils {
public:
    static void runSystemCommand(const QString& command, const QString& workDir,
                                  TerminalUI* ui, int timeout = 30000);
    static void runExternal(const QString& program, const QStringList& args,
                             const QString& workDir, TerminalUI* ui,
                             int timeout = 30000, bool realtime = false);
    static void runShell(const QString& command, const QString& workDir,
                          TerminalUI* ui, int timeout = 30000);
    static bool copyDir(const QString& src, const QString& dst);
    static bool removeDir(const QString& path);
    static QJsonObject loadJsonFile(const QString& path);
    static bool saveJsonFile(const QString& path, const QJsonObject& obj);
    static QString configDir();
    static QString customCommandsPath();
    static QString formatSize(qint64 bytes);
};

// ============ 命令执行辅助（无默认参数） ============
namespace CommandHelper {
    void runExternal(const QString& cmd, const QStringList& args,
                     const QString& workDir, TerminalUI* ui,
                     int timeout, bool realtime);
    void runShell(const QString& command, const QString& workDir,
                  TerminalUI* ui, int timeout);
}

#endif // UTILS_H