#include "commands.h"
#include "terminal/terminal.h"
#include "utils/utils.h"

#include <QFile>
#include <QDir>
#include <QProcess>
#include <QFileInfo>
#include <QStandardPaths>
#include <QDateTime>

void CommandRegistry::registerSshCommands(TerminalWidget* ui) {

    QString sshDir = QDir(ui->currentDir).absoluteFilePath(".ssh");

    // ssh-keygen - 生成密钥
    registerCommand("ssh-keygen", [ui, sshDir](const QStringList& args) {
        QDir().mkpath(sshDir);

        QString keyType = "rsa";
        QString comment = ui->username + "@termail";
        QString fileName = "id_rsa";

        for (int i = 0; i < args.size(); ++i) {
            if (args[i] == "-t" && i + 1 < args.size()) {
                keyType = args[i + 1];
                if (keyType == "ed25519") fileName = "id_ed25519";
                else if (keyType == "rsa") fileName = "id_rsa";
                else if (keyType == "ecdsa") fileName = "id_ecdsa";
            }
            if (args[i] == "-C" && i + 1 < args.size()) comment = args[i + 1];
            if (args[i] == "-f" && i + 1 < args.size()) fileName = args[i + 1];
        }

        QString keyPath = sshDir + "/" + fileName;
        if (QFile::exists(keyPath)) {
            ui->appendOutput(QString("密钥已存在: %1\n").arg(keyPath), ERROR_COLOR);
            ui->appendOutput("删除后重试 或 用 ssh-keygen -f <新名>\n", INFO_COLOR);
            return;
        }

        ui->appendOutput(QString("生成 %1 密钥...\n").arg(keyType), ACCENT_COLOR);

        QProcess proc;
        proc.start("ssh-keygen", QStringList()
            << "-t" << keyType
            << "-b" << "2048"
            << "-C" << comment
            << "-f" << keyPath
            << "-N" << "");
        proc.waitForFinished(30000);

        QString output = proc.readAllStandardOutput();
        QString err = proc.readAllStandardError();
        if (!output.isEmpty()) ui->appendOutput(output);
        if (!err.isEmpty()) ui->appendOutput(err, ERROR_COLOR);

        if (QFile::exists(keyPath)) {
            ui->appendOutput(QString("密钥已生成: %1\n").arg(keyPath), PROMPT_COLOR);
            ui->appendOutput(QString("公钥: %1.pub\n").arg(keyPath), INFO_COLOR);
            ui->appendOutput("输入 ssh-pub 查看公钥\n", INFO_COLOR);
        } else {
            ui->appendOutput("生成失败，系统可能没有 ssh-keygen\n", ERROR_COLOR);
            ui->appendOutput("提示: 可用 PRoot 安装 openssh\n", INFO_COLOR);
            ui->appendOutput("  proot-install openssh\n", INFO_COLOR);
        }
    }, "生成SSH密钥", "ssh-keygen [-t rsa|ed25519] [-C 注释]");

    // ssh-pub - 显示公钥
    registerCommand("ssh-pub", [ui, sshDir](const QStringList& args) {
        QString fileName = args.isEmpty() ? "id_rsa" : args[0];
        QString pubPath = sshDir + "/" + fileName + ".pub";

        if (!QFile::exists(pubPath)) {
            // 尝试其他常见名称
            QStringList candidates = {"id_rsa.pub", "id_ed25519.pub", "id_ecdsa.pub"};
            for (const QString& c : candidates) {
                QString p = sshDir + "/" + c;
                if (QFile::exists(p)) { pubPath = p; break; }
            }
        }

        QFile file(pubPath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            ui->appendOutput("=== 公钥 ===\n", ACCENT_COLOR);
            ui->appendOutput(file.readAll());
            file.close();
            ui->appendOutput("\n复制上面内容到服务器的 ~/.ssh/authorized_keys\n", INFO_COLOR);
        } else {
            ui->appendOutput("公钥不存在，先运行 ssh-keygen\n", ERROR_COLOR);
        }
    }, "查看公钥", "ssh-pub [密钥名]");

    // ssh-list - 列出所有密钥
    registerCommand("ssh-list", [ui, sshDir](const QStringList&) {
        QDir dir(sshDir);
        if (!dir.exists()) {
            ui->appendOutput("无 .ssh 目录，先运行 ssh-keygen\n", ERROR_COLOR);
            return;
        }
        ui->appendOutput("=== SSH 密钥 ===\n", ACCENT_COLOR);
        QStringList files = dir.entryList(QDir::Files);
        for (const QString& f : files) {
            QFileInfo info(dir.absoluteFilePath(f));
            ui->appendOutput(QString("%1  %2  %3\n")
                .arg(info.size(), 8)
                .arg(info.lastModified().toString("yyyy-MM-dd hh:mm"))
                .arg(f),
                f.endsWith(".pub") ? INFO_COLOR : PROMPT_COLOR);
        }
    }, "列出密钥", "ssh-list");

    // ssh-copy - 复制公钥到服务器
    registerCommand("ssh-copy", [ui, sshDir](const QStringList& args) {
        if (args.isEmpty()) {
            ui->appendOutput("用法: ssh-copy <用户@主机>\n", ERROR_COLOR);
            ui->appendOutput("如: ssh-copy root@192.168.1.100\n", INFO_COLOR);
            return;
        }
        QString target = args[0];
        QString pubPath = sshDir + "/id_rsa.pub";
        if (!QFile::exists(pubPath)) pubPath = sshDir + "/id_ed25519.pub";
        if (!QFile::exists(pubPath)) {
            ui->appendOutput("公钥不存在，先运行 ssh-keygen\n", ERROR_COLOR);
            return;
        }

        QFile file(pubPath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            ui->appendOutput("读取公钥失败\n", ERROR_COLOR);
            return;
        }
        QString pubKey = file.readAll().trimmed();
        file.close();

        ui->appendOutput(QString("复制公钥到 %1 ...\n").arg(target), ACCENT_COLOR);

        QString cmd = QString("cat %1 | ssh %2 'mkdir -p ~/.ssh && cat >> ~/.ssh/authorized_keys && chmod 600 ~/.ssh/authorized_keys'")
            .arg(pubPath, target);

        CommandHelper::runExternal("sh", QStringList() << "-c" << cmd,
                                   ui->currentDir, ui, 0, true);
    }, "复制公钥", "ssh-copy <用户@主机>");

    // ssh-rm - 删除密钥
    registerCommand("ssh-rm", [ui, sshDir](const QStringList& args) {
        if (args.isEmpty()) {
            ui->appendOutput("用法: ssh-rm <密钥名>\n", ERROR_COLOR);
            return;
        }
        QString name = args[0];
        QString keyPath = sshDir + "/" + name;
        QString pubPath = keyPath + ".pub";
        bool removed = false;
        if (QFile::remove(keyPath)) removed = true;
        if (QFile::remove(pubPath)) removed = true;
        if (removed) ui->appendOutput(QString("已删除: %1\n").arg(name), INFO_COLOR);
        else ui->appendOutput("密钥不存在\n", ERROR_COLOR);
    }, "删除密钥", "ssh-rm <密钥名>");
}