#include "commands.h"
#include "terminal/terminal.h"
#include "utils/utils.h"

#include <QFile>
#include <QDir>
#include <QCryptographicHash>
#include <QCoreApplication>
#include <QStandardPaths>

void CommandRegistry::registerCmenuCommands(TerminalWidget* ui) {

    registerCommand("cmenu", [ui](const QStringList&) {
        QString output = R"(
════════════════════════════════════════
           busybox 扩展命令菜单
════════════════════════════════════════
  使用方法:
     bb <命令> [参数]
════════════════════════════════════════
)";
        ui->appendOutput(output, ACCENT_COLOR);
    }, "busybox菜单", "cmenu");

    registerCommand("get-busybox", [ui](const QStringList&) {
        ui->appendOutput("新版已内置 busybox，直接使用 bb <命令> 即可。\n", INFO_COLOR);
    }, "安装busybox", "get-busybox");

    registerCommand("bb", [ui](const QStringList& args) {
        if (args.isEmpty()) {
            ui->appendOutput("用法: bb <busybox命令> [参数]\n", ERROR_COLOR);
            return;
        }

        // 直接从 native 库目录读取
        QString busyboxPath = QCoreApplication::applicationDirPath() + "/libbusybox.so";

        if (!QFile::exists(busyboxPath)) {
            ui->appendOutput(QString("未找到内置的 busybox (%1)。\n").arg(busyboxPath), ERROR_COLOR);
            return;
        }

        // 直接执行 (这个目录内核允许 exec)
        CommandHelper::runExternal(busyboxPath, args, ui->currentDir, ui);
    }, "busybox调用", "bb <命令>");
}
