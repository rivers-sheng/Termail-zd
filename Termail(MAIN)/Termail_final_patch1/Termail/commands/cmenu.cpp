#include "commands.h"
#include "terminal/terminal.h"
#include "utils/utils.h"

#include <QFile>
#include <QDir>
#include <QCryptographicHash>

void CommandRegistry::registerCmenuCommands(TerminalWidget* ui) {

    registerCommand("cmenu", [ui](const QStringList&) {
        QString output = R"(
════════════════════════════════════════
           busybox 扩展命令菜单
════════════════════════════════════════

  未安装 busybox 时请先运行:
     get-busybox

  安装后使用方法:
     bb <命令> [参数]

  示例:
     bb ls -la
     bb wget http://example.com/file.txt
     bb tar -xzf archive.tar.gz

════════════════════════════════════════
  busybox 提供 300+ 个 Linux 命令
════════════════════════════════════════
)";
        ui->appendOutput(output, ACCENT_COLOR);
    }, "busybox菜单", "cmenu");

    registerCommand("get-busybox", [ui](const QStringList&) {
        QString destPath = QDir(ui->currentDir).absoluteFilePath("busybox");
        QFile resFile(":/resources/busybox/busybox");
        if (!resFile.open(QIODevice::ReadOnly)) {
            ui->appendOutput("busybox资源未找到\n", ERROR_COLOR);
            return;
        }
        QByteArray data = resFile.readAll();
        resFile.close();

        QFile dest(destPath);
        if (!dest.open(QIODevice::WriteOnly)) {
            ui->appendOutput("目录不可写\n", ERROR_COLOR);
            return;
        }
        dest.write(data);
        dest.close();

        if (QFile::setPermissions(destPath, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner |
                                              QFileDevice::ReadGroup | QFileDevice::ExeGroup |
                                              QFileDevice::ReadOther | QFileDevice::ExeOther)) {
            ui->appendOutput(QString("busybox已安装: %1 (%2 KB)\n")
                .arg(destPath).arg(data.size() / 1024), PROMPT_COLOR);
            ui->appendOutput("用法: bb <命令>\n", INFO_COLOR);
        } else {
            ui->appendOutput("权限设置失败\n", ERROR_COLOR);
        }
    }, "安装busybox", "get-busybox");

    registerCommand("bb", [ui](const QStringList& args) {
        if (args.isEmpty()) {
            ui->appendOutput("用法: bb <busybox命令> [参数]\n", ERROR_COLOR);
            return;
        }
        QString busyboxPath = QDir(ui->currentDir).absoluteFilePath("busybox");
        if (!QFile::exists(busyboxPath)) {
            ui->appendOutput("未找到busybox，请先运行 get-busybox\n", ERROR_COLOR);
            return;
        }
        CommandHelper::runExternal(busyboxPath, args, ui->currentDir, ui);
    }, "busybox调用", "bb <命令>");
}