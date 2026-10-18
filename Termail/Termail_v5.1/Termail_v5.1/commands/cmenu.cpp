#include "commands.h"
#include "terminal/terminal.h"
#include "utils/utils.h"

#include <QNetworkAccessManager>
#include <QStandardPaths>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QEventLoop>
#include <QTimer>
#include <QFile>
#include <QDir>

void CommandRegistry::registerCmenuCommands(TerminalWidget* ui) {

    // cmenu - 显示busybox菜单
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
     bb vi file.txt
     bb top
     bb df -h
     bb free
     bb ps aux
     bb ping -c 4 google.com
     bb nc -l -p 8080

  常用 busybox 命令:
     ls, cp, mv, rm, cat, grep, sed, awk
     wget, ftp, nc, telnet, ping, traceroute
     tar, gzip, bzip2, xz, unzip, zip
     vi, top, ps, free, df, mount, umount
     find, which, whoami, id, date, time
     dd, du, md5sum, sha256sum, base64

════════════════════════════════════════
  busybox 提供 300+ 个 Linux 命令
════════════════════════════════════════
)";
        ui->appendOutput(output, ACCENT_COLOR);
    }, "busybox菜单", "cmenu");

    registerCommand("get-busybox", [ui](const QStringList&) {
        QString destPath = QDir(ui->currentDir).absoluteFilePath("busybox");
        QFile resFile(":/resources/busybox/busybox");
        if (resFile.open(QIODevice::ReadOnly)) {
            QByteArray data = resFile.readAll();
            resFile.close();
            QFile dest(destPath);
            if (dest.open(QIODevice::WriteOnly)) {
                dest.write(data);
                dest.close();
                QFile::setPermissions(destPath, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner |
                                                  QFileDevice::ReadGroup | QFileDevice::ExeGroup |
                                                  QFileDevice::ReadOther | QFileDevice::ExeOther);
                ui->appendOutput(QString("busybox 已安装: %1 (%2 KB)\n")
                    .arg(destPath).arg(data.size() / 1024), PROMPT_COLOR);
                ui->appendOutput("用法: bb <命令>\n", INFO_COLOR);
            }
        } else {
            ui->appendOutput("busybox 资源未找到\n", ERROR_COLOR);
        }
    }, "安装busybox", "get-busybox");

    // bb 调用 busybox
    registerCommand("bb", [ui](const QStringList& args) {
        if (args.isEmpty()) {
            ui->appendOutput("用法: bb <busybox命令> [参数]\n", ERROR_COLOR);
            ui->appendOutput("示例: bb ls -la\n", INFO_COLOR);
            return;
        }
        QString busyboxPath = QDir(ui->currentDir).absoluteFilePath("busybox");
        if (!QFile::exists(busyboxPath)) {
            ui->appendOutput("未找到 busybox，请先运行 get-busybox\n", ERROR_COLOR);
            return;
        }
        CommandHelper::runExternal(busyboxPath, args, ui->currentDir, ui);
    }, "busybox快捷调用", "bb <命令>");
}
