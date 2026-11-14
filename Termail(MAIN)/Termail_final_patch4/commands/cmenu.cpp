#include "commands.h"
#include "terminal/terminal.h"
#include "utils/utils.h"

#include <QFile>
#include <QDir>
#include <QCoreApplication>

void CommandRegistry::registerCmenuCommands(TerminalWidget* ui) {

    registerCommand("cmenu", [ui](const QStringList&) {
        QString output = R"(
════════════════════════════════════════
           扩展命令菜单
════════════════════════════════════════
  使用方法:
     bb <命令> [参数]

  示例:
     bb ls -la
     bb tar -xzf archive.tar.gz
     bb grep "text" file.txt
════════════════════════════════════════
)";
        ui->appendOutput(output, ACCENT_COLOR);
    }, "扩展菜单", "cmenu");

    registerCommand("get-busybox", [ui](const QStringList&) {
        ui->appendOutput("新版已无需手动安装，直接使用 bb <命令> 即可。\n", INFO_COLOR);
    }, "安装提示", "get-busybox");

    registerCommand("bb", [ui](const QStringList& args) {
        if (args.isEmpty()) {
            ui->appendOutput("用法: bb <命令> [参数]\n", ERROR_COLOR);
            return;
        }

        // 动态检测系统可用的扩展工具
        QString sysTool;
        if (QFile::exists("/system/bin/toybox")) {
            sysTool = "/system/bin/toybox";
        } else if (QFile::exists("/system/bin/busybox")) {
            sysTool = "/system/bin/busybox";
        }

        if (sysTool.isEmpty()) {
            ui->appendOutput("错误: 系统未提供 toybox 或 busybox，无法执行扩展命令。\n", ERROR_COLOR);
            return;
        }

        // 直接通过系统的工具执行
        CommandHelper::runExternal(sysTool, args, ui->currentDir, ui);
    }, "系统扩展调用", "bb <命令>");
}
