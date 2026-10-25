#include "commands.h"
#include "terminal/terminal.h"

CommandRegistry::CommandRegistry(TerminalWidget* terminal) : term(terminal) {}
CommandRegistry::~CommandRegistry() {}

void CommandRegistry::registerCommand(const QString& name, CommandFunc func,
                                       const QString& desc, const QString& usage) {
    if (commands.contains(name)) return;
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

QString CommandRegistry::suggestCommand(const QString& cmd) {
    for (const QString& c : commands.keys()) {
        if (c.length() == cmd.length()) {
            int diff = 0;
            for (int i = 0; i < c.length(); ++i)
                if (c[i] != cmd[i]) diff++;
            if (diff == 1) return c;
        }
        if (c.startsWith(cmd) && cmd.length() >= 2 && c != cmd) return c;
    }
    return "";
}

void CommandRegistry::registerAllBuiltins(TerminalWidget* ui) {
    registerHelpCommands(ui);
    registerFunCommands(ui);
}