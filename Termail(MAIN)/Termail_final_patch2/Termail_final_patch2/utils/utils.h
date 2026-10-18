#ifndef UTILS_H
#define UTILS_H

#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QProcess>

class TerminalWidget;

class Utils {
public:
    static void runSystemCommand(const QString& command, const QString& workDir,
                                  TerminalWidget* ui, int timeout = 0);
    static void runExternal(const QString& program, const QStringList& args,
                             const QString& workDir, TerminalWidget* ui,
                             int timeout = 0, bool realtime = false);
    static bool copyDir(const QString& src, const QString& dst);
    static QJsonObject loadJsonFile(const QString& path);
    static bool saveJsonFile(const QString& path, const QJsonObject& obj);
    static QString configDir();
    static QString customCommandsPath();
    static QString formatSize(qint64 bytes);
};

#endif