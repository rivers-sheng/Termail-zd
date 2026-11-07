#ifndef LUA_ENGINE_H
#define LUA_ENGINE_H

#include <QString>
#include <QStringList>

class TerminalWidget;

class LuaEngine {
public:
    LuaEngine(TerminalWidget* ui);
    ~LuaEngine();

    bool runFile(const QString& path);
    bool runString(const QString& code);

private:
    TerminalWidget* ui;
};

#endif