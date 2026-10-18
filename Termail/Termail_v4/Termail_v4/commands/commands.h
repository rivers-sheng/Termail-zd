#ifndef COMMANDS_H
#define COMMANDS_H

#include <QString>
#include <QStringList>
#include <QMap>
#include <QSet>
#include <QProcess>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QStorageInfo>
#include <functional>

#include "config.h"

class TerminalUI;

// ============ 命令注册表 ============
class CommandRegistry {
public:
    CommandRegistry(TerminalUI* terminal);
    ~CommandRegistry();

    void registerCommand(const QString& name, CommandFunc func,
                         const QString& desc = "", const QString& usage = "");
    bool execute(const QString& name, const QStringList& args);
    QStringList getCommandNames() const;

    // 注册所有内置命令
    void registerAllBuiltins(TerminalUI* ui);

private:
    // 分类注册
    void registerFileCommands(TerminalUI* ui);
    void registerNetworkCommands(TerminalUI* ui);
    void registerSystemCommands(TerminalUI* ui);
    void registerTextCommands(TerminalUI* ui);
    void registerFunCommands(TerminalUI* ui);

    QMap<QString, CommandFunc> commands;
    QMap<QString, QString> descriptions;
    QMap<QString, QString> usages;
    TerminalUI* term;
};

// ============ 外部命令执行辅助 ============
namespace CommandHelper {
    void runExternal(const QString& cmd, const QStringList& args,
                     const QString& workDir, TerminalUI* ui,
                     int timeout = 30000, bool realtime = false);
    void runShell(const QString& command, const QString& workDir,
                  TerminalUI* ui, int timeout = 30000);
}

#endif // COMMANDS_H