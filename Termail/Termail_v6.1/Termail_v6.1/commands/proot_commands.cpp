#include "commands.h"
#include "terminal/terminal.h"
#include "utils/utils.h"

#include <QFile>
#include <QDir>
#include <QProcess>
#include <QFileInfo>

void CommandRegistry::registerProotCommands(TerminalWidget* ui) {

    // proot-setup - 安装 PRoot 环境
    registerCommand("proot-setup", [ui](const QStringList&) {
        QString baseDir = QDir(ui->currentDir).absoluteFilePath("proot-env");
        QString prootPath = QDir(ui->currentDir).absoluteFilePath("proot");
        QString rootfsPath = baseDir + "/rootfs";
        QString tmpDir = QDir(ui->currentDir).absoluteFilePath("tmp");

        QDir().mkpath(tmpDir);

        if (QFile::exists(rootfsPath + "/bin")) {
            ui->appendOutput("PRoot 已安装\n", ERROR_COLOR);
            ui->appendOutput("用 proot-remove 删除后重试\n", INFO_COLOR);
            return;
        }

        ui->appendOutput("正在安装 PRoot 环境...\n", ACCENT_COLOR);

        QFile prootRes(":/resources/proot/proot");
        if (!prootRes.open(QIODevice::ReadOnly)) {
            ui->appendOutput("proot 资源未找到\n", ERROR_COLOR);
            return;
        }
        QByteArray prootData = prootRes.readAll();
        prootRes.close();

        QFile prootDest(prootPath);
        if (!prootDest.open(QIODevice::WriteOnly)) {
            ui->appendOutput("无法写入 proot\n", ERROR_COLOR);
            return;
        }
        prootDest.write(prootData);
        prootDest.close();
        QFile::setPermissions(prootPath,
            QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner |
            QFileDevice::ReadGroup | QFileDevice::ExeGroup |
            QFileDevice::ReadOther | QFileDevice::ExeOther);
        ui->appendOutput(QString("proot 已复制 (%1 KB)\n")
            .arg(prootData.size() / 1024), INFO_COLOR);

        QDir().mkpath(baseDir);
        QString tarPath = baseDir + "/alpine.tar";

        QFile tarRes(":/resources/proot/alpine.tar");
        if (!tarRes.open(QIODevice::ReadOnly)) {
            ui->appendOutput("Alpine rootfs 资源未找到\n", ERROR_COLOR);
            return;
        }
        QByteArray tarData = tarRes.readAll();
        tarRes.close();

        QFile tarDest(tarPath);
        if (!tarDest.open(QIODevice::WriteOnly)) {
            ui->appendOutput("无法写入压缩包\n", ERROR_COLOR);
            return;
        }
        tarDest.write(tarData);
        tarDest.close();
        ui->appendOutput(QString("rootfs 已复制 (%1 KB)\n")
            .arg(tarData.size() / 1024), INFO_COLOR);

        QDir().mkpath(rootfsPath);
        ui->appendOutput("解压中...\n", ACCENT_COLOR);

        QProcess tarProc;
        tarProc.setWorkingDirectory(baseDir);

        QString busyboxPath = QDir(ui->currentDir).absoluteFilePath("busybox");
        if (QFile::exists(busyboxPath)) {
            tarProc.start(busyboxPath, QStringList() << "tar" << "-xzf" << tarPath << "-C" << rootfsPath);
        } else {
            tarProc.start("tar", QStringList() << "-xzf" << tarPath << "-C" << rootfsPath);
        }
        tarProc.waitForFinished(60000);

        QFile::remove(tarPath);

        if (QFile::exists(rootfsPath + "/bin")) {
            ui->appendOutput("PRoot 环境安装完成！\n", PROMPT_COLOR);
            ui->appendOutput(QString("位置: %1\n").arg(rootfsPath));
            ui->appendOutput("输入 proot-mirror 切换国内镜像\n", INFO_COLOR);
            ui->appendOutput("输入 proot-install gcc 安装编译器\n", INFO_COLOR);
        } else {
            ui->appendOutput("解压失败\n", ERROR_COLOR);
        }
    }, "安装PRoot环境", "proot-setup");

    // proot-shell - 进入 PRoot 执行命令（永不超时）
    registerCommand("proot-shell", [ui](const QStringList& args) {
        QString prootPath = QDir(ui->currentDir).absoluteFilePath("proot");
        QString rootfsPath = QDir(ui->currentDir).absoluteFilePath("proot-env/rootfs");
        QString tmpDir = QDir(ui->currentDir).absoluteFilePath("tmp");

        if (!QFile::exists(prootPath)) {
            ui->appendOutput("proot 未找到\n", ERROR_COLOR);
            return;
        }
        if (!QFile::exists(rootfsPath + "/bin")) {
            ui->appendOutput("rootfs 未找到\n", ERROR_COLOR);
            return;
        }

        QDir().mkpath(tmpDir);

        QString cmd = args.isEmpty() ? "sh" : args.join(' ');
        QString fullCmd = QString("PROOT_TMP_DIR=%1 %2 -r %3 -b /dev -b /proc -b /sys -w / /bin/busybox sh -c \"%4\"")
            .arg(tmpDir, prootPath, rootfsPath, cmd);

        ui->appendOutput(QString("proot: %1\n").arg(cmd), ACCENT_COLOR);
        CommandHelper::runExternal("sh", QStringList() << "-c" << fullCmd,
                                   ui->currentDir, ui, 0, true);
    }, "进入PRoot", "proot-shell [命令]");

    // proot-mirror
    registerCommand("proot-mirror", [ui](const QStringList&) {
        QString prootPath = QDir(ui->currentDir).absoluteFilePath("proot");
        QString rootfsPath = QDir(ui->currentDir).absoluteFilePath("proot-env/rootfs");
        QString tmpDir = QDir(ui->currentDir).absoluteFilePath("tmp");
        QDir().mkpath(tmpDir);

        ui->appendOutput("设置DNS + 阿里云镜像...\n", ACCENT_COLOR);

        QString fullCmd = QString("PROOT_TMP_DIR=%1 %2 -r %3 -b /dev -b /proc -b /sys -w / /bin/busybox sh -c \"echo nameserver 8.8.8.8 > /etc/resolv.conf; echo https://mirrors.aliyun.com/alpine/v3.19/main > /etc/apk/repositories; echo https://mirrors.aliyun.com/alpine/v3.19/community >> /etc/apk/repositories\"")
            .arg(tmpDir, prootPath, rootfsPath);

        CommandHelper::runExternal("sh", QStringList() << "-c" << fullCmd,
                                   ui->currentDir, ui, 0, true);
        ui->appendOutput("完成\n", PROMPT_COLOR);
    }, "切换国内镜像", "proot-mirror");

    // proot-install
    registerCommand("proot-install", [ui](const QStringList& args) {
        if (args.isEmpty()) {
            ui->appendOutput("用法: proot-install <包名>\n", ERROR_COLOR);
            return;
        }
        QString packages = args.join(' ');
        ui->appendOutput(QString("安装中: %1\n").arg(packages), ACCENT_COLOR);
        ui->executeInternalCommand(QString("proot-shell /sbin/apk add --no-cache %1").arg(packages));
    }, "PRoot安装软件", "proot-install <包名>");

    // proot-update
    registerCommand("proot-update", [ui](const QStringList&) {
        ui->appendOutput("更新索引...\n", ACCENT_COLOR);
        ui->executeInternalCommand("proot-shell /sbin/apk update");
    }, "更新包索引", "proot-update");

    // proot-status
    registerCommand("proot-status", [ui](const QStringList&) {
        QString baseDir = QDir(ui->currentDir).absoluteFilePath("proot-env");
        QString rootfsPath = baseDir + "/rootfs";
        QString prootPath = QDir(ui->currentDir).absoluteFilePath("proot");

        ui->appendOutput("=== PRoot 状态 ===\n", ACCENT_COLOR);
        ui->appendOutput(QString("proot: %1\n").arg(QFile::exists(prootPath) ? "已安装" : "未安装"),
                         QFile::exists(prootPath) ? PROMPT_COLOR : ERROR_COLOR);
        ui->appendOutput(QString("rootfs: %1\n").arg(QFile::exists(rootfsPath + "/bin") ? "已安装" : "未安装"),
                         QFile::exists(rootfsPath + "/bin") ? PROMPT_COLOR : ERROR_COLOR);
    }, "PRoot状态", "proot-status");

    // proot-remove
    registerCommand("proot-remove", [ui](const QStringList&) {
        QDir(ui->currentDir + "/proot-env").removeRecursively();
        QFile::remove(ui->currentDir + "/proot");
        ui->appendOutput("已删除\n", INFO_COLOR);
    }, "删除PRoot环境", "proot-remove");
}