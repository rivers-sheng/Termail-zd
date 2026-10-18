#include "commands.h"
#include "terminal/terminal.h"
#include "utils/utils.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QEventLoop>
#include <QTimer>
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
            ui->appendOutput("busybox 资源未找到\n", ERROR_COLOR);
            return;
        }
        QByteArray data = resFile.readAll();
        resFile.close();

        // sha256 校验
        QCryptographicHash hash(QCryptographicHash::Sha256);
        hash.addData(data);
        QString fileHash = hash.result().toHex();
        // 预置的sha256（首次构建时输出后填入）
        QString expectedHash = "141adb1b625a6f44c4b114f76b4387b4ea4f7ab802b88eb40e0d2f6adcccb1c3"; // TODO: 填入实际值
        if (!expectedHash.isEmpty() && fileHash != expectedHash) {
            ui->appendOutput("busybox 校验失败，文件可能损坏\n", ERROR_COLOR);
            return;
        }

        QFile dest(destPath);
        if (!dest.open(QIODevice::WriteOnly)) {
            ui->appendOutput("目录不可写，无法安装\n", ERROR_COLOR);
            return;
        }
        dest.write(data);
        dest.close();

        if (!QFile::setPermissions(destPath, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner |
                                              QFileDevice::ReadGroup | QFileDevice::ExeGroup |
                                              QFileDevice::ReadOther | QFileDevice::ExeOther)) {
            ui->appendOutput("权限设置失败\n", ERROR_COLOR);
            return;
        }

        // 验证可执行
        QProcess* testProc = new QProcess();
        testProc->start(destPath, QStringList() << "--help");
        testProc->waitForFinished(3000);
        if (testProc->exitStatus() == QProcess::NormalExit) {
            ui->appendOutput(QString("busybox 已安装: %1 (%2 KB)\n")
                .arg(destPath).arg(data.size() / 1024), PROMPT_COLOR);
            ui->appendOutput("用法: bb <命令>\n", INFO_COLOR);
        } else {
            ui->appendOutput("此设备不支持 busybox 执行\n", ERROR_COLOR);
            QFile::remove(destPath);
        }
        testProc->deleteLater();
    }, "安装busybox", "get-busybox");

    registerCommand("bb", [ui](const QStringList& args) {
        if (args.isEmpty()) {
            ui->appendOutput("用法: bb <busybox命令> [参数]\n", ERROR_COLOR);
            return;
        }
        QString busyboxPath = QDir(ui->currentDir).absoluteFilePath("busybox");
        if (!QFile::exists(busyboxPath)) {
            ui->appendOutput("未找到 busybox，请先运行 get-busybox\n", ERROR_COLOR);
            return;
        }
        CommandHelper::runExternal(busyboxPath, args, ui->currentDir, ui);
    }, "busybox快捷调用", "bb <命令>");
    // storage - 切换到存储目录
    registerCommand("storage", [ui](const QStringList& args) {
        QStringList paths;
        paths << "/sdcard" << "/storage/emulated/0" << "/storage/emulated/0/Download"
              << "/storage/emulated/0/Documents" << "/storage/emulated/0/DCIM";
        
        if (args.isEmpty()) {
            ui->appendOutput("可用存储路径:\n", ACCENT_COLOR);
            for (const QString& p : paths) {
                if (QDir(p).exists())
                    ui->appendOutput("  " + p + "\n", PROMPT_COLOR);
            }
            ui->appendOutput("用法: storage <路径> 或 storage 0/1/2/3/4\n", INFO_COLOR);
            return;
        }
        
        QString target;
        if (args[0].length() == 1 && args[0][0].isDigit()) {
            int idx = args[0].toInt();
            if (idx >= 0 && idx < paths.size()) target = paths[idx];
        } else {
            target = args[0];
        }
        
        if (QDir(target).exists()) {
            ui->currentDir = target;
            QDir::setCurrent(target);
            ui->updatePrompt();
            ui->appendOutput(QString("已切换到: %1\n").arg(target), INFO_COLOR);
        } else {
            ui->appendOutput("路径不存在\n", ERROR_COLOR);
        }
    }, "存储目录切换", "storage [路径|索引]");
}