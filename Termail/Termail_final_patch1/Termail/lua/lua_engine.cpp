#include "lua_engine.h"
#include "terminal/terminal.h"
#include "config.h"

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QStringConverter>

// 全局 ui 指针（简单可靠）
static TerminalWidget* g_luaUi = nullptr;

// print 函数的 C 实现
static int lua_print(lua_State* L) {
    int n = lua_gettop(L);
    QString output;
    for (int i = 1; i <= n; ++i) {
        if (i > 1) output += "\t";
        size_t len = 0;
        const char* s = luaL_tolstring(L, i, &len);
        if (s) output += QString::fromUtf8(s, static_cast<int>(len));
        lua_pop(L, 1);
    }
    output += "\n";

    if (g_luaUi) {
        g_luaUi->appendOutput(output, PROMPT_COLOR);
    }
    return 0;
}

LuaEngine::LuaEngine(TerminalWidget* terminal) : ui(terminal) {}

LuaEngine::~LuaEngine() {}

bool LuaEngine::runString(const QString& code) {
    lua_State* L = luaL_newstate();
    if (!L) {
        ui->appendOutput("无法创建 Lua 状态\n", ERROR_COLOR);
        return false;
    }

    g_luaUi = ui;

    luaL_openlibs(L);
    lua_pushcfunction(L, lua_print);
    lua_setglobal(L, "print");

    int result = luaL_dostring(L, code.toUtf8().constData());

    if (result != LUA_OK) {
        const char* err = lua_tostring(L, -1);
        if (err) {
            ui->appendOutput(QString("Lua 错误: %1\n").arg(QString::fromUtf8(err)), ERROR_COLOR);
        }
        lua_pop(L, 1);
        lua_close(L);
        g_luaUi = nullptr;
        return false;
    }

    lua_close(L);
    g_luaUi = nullptr;
    return true;
}

bool LuaEngine::runFile(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        ui->appendOutput(QString("无法打开文件: %1\n").arg(path), ERROR_COLOR);
        return false;
    }
    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    QString code = in.readAll();
    file.close();

    return runString(code);
}
