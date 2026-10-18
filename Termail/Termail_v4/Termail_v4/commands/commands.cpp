#include "commands.h"
#include "ui/terminal_ui.h"

CommandRegistry::CommandRegistry(TerminalUI* terminal) : term(terminal) {}

CommandRegistry::~CommandRegistry() {}

void CommandRegistry::registerCommand(const QString& name, CommandFunc func,
                                       const QString& desc, const QString& usage) {
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

void CommandRegistry::registerAllBuiltins(TerminalUI* ui) {
    registerFileCommands(ui);
    registerNetworkCommands(ui);
    registerSystemCommands(ui);
    registerTextCommands(ui);
    registerFunCommands(ui);
}