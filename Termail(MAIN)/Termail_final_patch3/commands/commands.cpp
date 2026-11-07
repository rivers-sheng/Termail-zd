#include "commands.h"
#include "terminal/terminal.h"

CommandRegistry::CommandRegistry(TerminalWidget* terminal) : term(terminal) {}
CommandRegistry::~CommandRegistry() {}

void CommandRegistry::registerCommand(const QString& name, CommandFunc func,
                                       const QString& desc, const QString& usage) {
    if (commands.contains(name)) return; // 防重复注册
    commands[name] = func;
    descriptions[name] = desc;
    usages[name] = usage;
}

bool CommandRegistry::execute(const QString& name, const QStringList& args) {
    if (commands.contains(name)) {
        commands[name](args);
        return true;
    }
    return false;
}

QStringList CommandRegistry::getCommandNames() const {
    return commands.keys();
}

void CommandRegistry::registerAllBuiltins(TerminalWidget* ui) {
    registerFileCommands(ui);
    registerNetworkCommands(ui);
    registerSystemCommands(ui);
    registerTextCommands(ui);
    registerFunCommands(ui);
    registerCmenuCommands(ui);
    registerMany1Commands(ui);
    registerMany2Commands(ui);
    registerMany3Commands(ui);
    registerLogCommands(ui);
    registerProotCommands(ui);
    registerSessionCommands(ui);
    registerNetwork2Commands(ui);
    registerSshCommands(ui);
    registerMany4Commands(ui);
    registerConfigCommands(ui);
    registerAboutCommands(ui);
    registerEnhancedCommands(ui);
    registerNotifyCommands(ui);
    registerEditorCommands(ui);
    registerLuaCommands(ui);
}