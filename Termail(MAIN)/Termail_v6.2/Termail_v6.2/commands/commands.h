#ifndef COMMANDS_H
#define COMMANDS_H

#include <QString>
#include <QStringList>
#include <QMap>
#include <functional>

#include "config.h"

class TerminalWidget;

using CommandFunc = std::function<void(const QStringList&)>;

class CommandRegistry {
public:
    CommandRegistry(TerminalWidget* terminal);
    ~CommandRegistry();

    void registerCommand(const QString& name, CommandFunc func,
                         const QString& desc = "", const QString& usage = "");
    bool execute(const QString& name, const QStringList& args);
    QStringList getCommandNames() const;

    void registerAllBuiltins(TerminalWidget* ui);

private:
    void registerFileCommands(TerminalWidget* ui);
    void registerNetworkCommands(TerminalWidget* ui);
    void registerSystemCommands(TerminalWidget* ui);
    void registerTextCommands(TerminalWidget* ui);
    void registerFunCommands(TerminalWidget* ui);
    void registerCmenuCommands(TerminalWidget* ui);
    void registerMany1Commands(TerminalWidget* ui);
    void registerMany2Commands(TerminalWidget* ui);
    void registerMany3Commands(TerminalWidget* ui);
    void registerLogCommands(TerminalWidget* ui);
    void registerProotCommands(TerminalWidget* ui);
    void registerSessionCommands(TerminalWidget* ui);
    void registerNetwork2Commands(TerminalWidget* ui);
    void registerSshCommands(TerminalWidget* ui);
    void registerGameCommands(TerminalWidget* ui);

    QMap<QString, CommandFunc> commands;
    QMap<QString, QString> descriptions;
    QMap<QString, QString> usages;
    TerminalWidget* term;
};

namespace CommandHelper {
    void runExternal(const QString& cmd, const QStringList& args,
                     const QString& workDir, TerminalWidget* ui,
                     int timeout = 0, bool realtime = false);
    void runShell(const QString& command, const QString& workDir,
                  TerminalWidget* ui, int timeout = 0);
}

#endif