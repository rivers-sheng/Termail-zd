#include "commands.h"
#include "terminal/terminal.h"
#include "lua/lua_engine.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

void CommandRegistry::registerLuaCommands(TerminalWidget* ui) {

    // lua - 执行 Lua 脚本
    registerCommand("lua", [ui](const QStringList& args) {
        if (args.isEmpty()) {
            ui->appendOutput("用法:\n", ERROR_COLOR);
            ui->appendOutput("  lua <文件.lua>        运行脚本\n", INFO_COLOR);
            ui->appendOutput("  lua -e \"print(1+1)\"   直接执行\n", INFO_COLOR);
            return;
        }

        LuaEngine engine(ui);

        // lua -e "代码"
        if (args[0] == "-e" && args.size() > 1) {
            QString code = args.mid(1).join(' ');
            code.remove('"');
            engine.runString(code);
            return;
        }

        // lua 文件.lua
        QString path = QDir(ui->currentDir).absoluteFilePath(args[0]);
        if (!QFile::exists(path)) {
            ui->appendOutput(QString("文件不存在: %1\n").arg(args[0]), ERROR_COLOR);
            return;
        }
        ui->appendOutput(QString("[Lua] 运行 %1\n").arg(args[0]), ACCENT_COLOR);
        engine.runFile(path);
    }, "Lua 脚本", "lua <文件> | lua -e <代码>");

    // luainfo - Lua 信息
    registerCommand("luainfo", [ui](const QStringList&) {
        ui->appendOutput("=== Lua 引擎 ===\n", ACCENT_COLOR);
        ui->appendOutput("版本: Lua 5.4.6\n", PROMPT_COLOR);
        ui->appendOutput("\n用法:\n", INFO_COLOR);
        ui->appendOutput("  lua script.lua       运行脚本\n", FG_COLOR);
        ui->appendOutput("  lua -e \"print(1+1)\"   执行代码\n", FG_COLOR);
        ui->appendOutput("\n示例脚本:\n", INFO_COLOR);
        ui->appendOutput("  print(\"Hello\")\n", FG_COLOR);
        ui->appendOutput("  for i=1,10 do print(i) end\n", FG_COLOR);
        ui->appendOutput("  function fib(n) if n<2 then return n end return fib(n-1)+fib(n-2) end\n", FG_COLOR);
    }, "Lua信息", "luainfo");

    // luademo - 生成示例脚本
    registerCommand("luademo", [ui](const QStringList&) {
        QString path = QDir(ui->currentDir).absoluteFilePath("demo.lua");
        QFile file(path);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QString demo = R"(-- Termail Lua 示例脚本

-- 变量
name = "Termail"
print("Hello, " .. name)

-- 数学
print("2 + 3 = " .. (2 + 3))

-- 循环
print("循环 1-5:")
for i = 1, 5 do
    print("  第 " .. i .. " 次")
end

-- 斐波那契
function fib(n)
    if n < 2 then return n end
    return fib(n-1) + fib(n-2)
end
print("斐波那契第10项: " .. fib(10))

-- 表
t = {a=1, b=2, c=3}
sum = 0
for k, v in pairs(t) do
    sum = sum + v
end
print("表求和: " .. sum)

print("脚本结束")
)";
            file.write(demo.toUtf8());
            file.close();
            ui->appendOutput(QString("已生成: demo.lua\n").arg(path), PROMPT_COLOR);
            ui->appendOutput("运行: lua demo.lua\n", INFO_COLOR);
        }
    }, "生成Lua示例", "luademo");
}