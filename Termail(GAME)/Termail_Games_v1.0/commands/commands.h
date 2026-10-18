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
    QString suggestCommand(const QString& cmd);

    void registerAllBuiltins(TerminalWidget* ui);

private:
    void registerHelpCommands(TerminalWidget* ui);
    void registerFunCommands(TerminalWidget* ui);

    QMap<QString, CommandFunc> commands;
    QMap<QString, QString> descriptions;
    QMap<QString, QString> usages;
    TerminalWidget* term;
};

#endif