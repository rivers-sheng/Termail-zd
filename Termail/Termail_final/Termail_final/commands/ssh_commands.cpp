#include "commands.h"
#include "terminal/terminal.h"
#include "utils/utils.h"

#include <QFile>
#include <QDir>
#include <QProcess>
#include <QFileInfo>
#include <QStandardPaths>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>

void CommandRegistry::registerSshCommands(TerminalWidget* ui) {

    QString sshDir = QDir(ui->currentDir).absoluteFilePath(".ssh");
    QString hostsPath = sshDir + "/hosts.json";

    QDir().mkpath(sshDir);

    // ========== 主机管理 ==========

    // ssh-host - 主机管理
    registerCommand("ssh-host", [ui, sshDir, hostsPath](const QStringList& args) {
        // 读取配置
        auto loadHosts = [hostsPath]() -> QJsonObject {
            QFile f(hostsPath);
            if (f.open(QIODevice::ReadOnly)) {
                QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
                f.close();
                return doc.object();
            }
            return QJsonObject();
        };
        auto saveHosts = [hostsPath](const QJsonObject& obj) -> bool {
            QFile f(hostsPath);
            if (f.open(QIODevice::WriteOnly)) {
                f.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
                f.close();
                return true;
            }
            return false;
        };

        if (args.isEmpty() || args[0] == "list") {
            QJsonObject hosts = loadHosts();
            if (hosts.isEmpty()) {
                ui->appendOutput("无保存的主机\n", INFO_COLOR);
                ui->appendOutput("用法: ssh-host add <名> <用户@主机> [-p 端口]\n", INFO_COLOR);
                return;
            }
            ui->appendOutput("=== SSH 主机列表 ===\n", ACCENT_COLOR);
            for (auto it = hosts.begin(); it != hosts.end(); ++it) {
                QJsonObject h = it.value().toObject();
                ui->appendOutput(QString("  %1 → %2@%3:%4\n")
                    .arg(it.key(),
                         h["user"].toString(),
                         h["host"].toString(),
                         h["port"].toString()),
                    PROMPT_COLOR);
            }
            return;
        }

        QString sub = args[0].toLower();

        // add
        if (sub == "add") {
            if (args.size() < 3) {
                ui->appendOutput("用法: ssh-host add <名> <用户@主机> [-p 端口]\n", ERROR_COLOR);
                return;
            }
            QString name = args[1];
            QString target = args[2];
            QString port = "22";

            // 解析 -p 参数
            for (int i = 3; i < args.size() - 1; ++i) {
                if (args[i] == "-p") port = args[i + 1];
            }

            // 解析 user@host
            QString user, host;
            if (target.contains('@')) {
                QStringList parts = target.split('@');
                user = parts[0];
                host = parts[1];
            } else {
                user = "root";
                host = target;
            }

            QJsonObject hosts = loadHosts();
            QJsonObject entry;
            entry["user"] = user;
            entry["host"] = host;
            entry["port"] = port;
            hosts[name] = entry;
            saveHosts(hosts);

            ui->appendOutput(QString("已添加主机: %1\n").arg(name), PROMPT_COLOR);
            ui->appendOutput(QString("  %1@%2:%3\n").arg(user, host, port), INFO_COLOR);
            return;
        }

        // rm
        if (sub == "rm" || sub == "remove") {
            if (args.size() < 2) {
                ui->appendOutput("用法: ssh-host rm <名>\n", ERROR_COLOR);
                return;
            }
            QJsonObject hosts = loadHosts();
            if (hosts.contains(args[1])) {
                hosts.remove(args[1]);
                saveHosts(hosts);
                ui->appendOutput(QString("已删除: %1\n").arg(args[1]), INFO_COLOR);
            } else {
                ui->appendOutput("主机不存在\n", ERROR_COLOR);
            }
            return;
        }

        // edit
        if (sub == "edit") {
            if (args.size() < 4) {
                ui->appendOutput("用法: ssh-host edit <名> <用户@主机> [-p 端口]\n", ERROR_COLOR);
                return;
            }
            QJsonObject hosts = loadHosts();
            if (!hosts.contains(args[1])) {
                ui->appendOutput("主机不存在\n", ERROR_COLOR);
                return;
            }
            QString target = args[2];
            QString user = "root", host = target;
            if (target.contains('@')) {
                QStringList parts = target.split('@');
                user = parts[0];
                host = parts[1];
            }
            QJsonObject entry = hosts[args[1]].toObject();
            entry["user"] = user;
            entry["host"] = host;
            for (int i = 3; i < args.size() - 1; ++i)
                if (args[i] == "-p") entry["port"] = args[i + 1];
            hosts[args[1]] = entry;
            saveHosts(hosts);
            ui->appendOutput(QString("已更新: %1\n").arg(args[1]), INFO_COLOR);
            return;
        }

        // show
        if (sub == "show") {
            if (args.size() < 2) return;
            QJsonObject hosts = loadHosts();
            if (hosts.contains(args[1])) {
                QJsonObject h = hosts[args[1]].toObject();
                ui->appendOutput(QString("主机: %1\n").arg(args[1]), ACCENT_COLOR);
                ui->appendOutput(QString("  用户: %1\n").arg(h["user"].toString()));
                ui->appendOutput(QString("  地址: %1\n").arg(h["host"].toString()));
                ui->appendOutput(QString("  端口: %1\n").arg(h["port"].toString()));
            }
            return;
        }

        ui->appendOutput("用法:\n", ERROR_COLOR);
        ui->appendOutput("  ssh-host add <名> <用户@主机> [-p 端口]\n", INFO_COLOR);
        ui->appendOutput("  ssh-host list\n", INFO_COLOR);
        ui->appendOutput("  ssh-host rm <名>\n", INFO_COLOR);
        ui->appendOutput("  ssh-host edit <名> <用户@主机>\n", INFO_COLOR);
        ui->appendOutput("  ssh-host show <名>\n", INFO_COLOR);
    }, "SSH主机管理", "ssh-host");

    // ssh-connect - 快速连接
    registerCommand("ssh-connect", [ui, sshDir, hostsPath](const QStringList& args) {
        if (args.isEmpty()) {
            ui->appendOutput("用法: ssh-connect <主机名>\n", ERROR_COLOR);
            return;
        }
        QFile f(hostsPath);
        if (!f.open(QIODevice::ReadOnly)) {
            ui->appendOutput("无主机配置，先运行 ssh-host add\n", ERROR_COLOR);
            return;
        }
        QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
        f.close();
        QJsonObject hosts = doc.object();

        if (!hosts.contains(args[0])) {
            ui->appendOutput(QString("主机不存在: %1\n").arg(args[0]), ERROR_COLOR);
            return;
        }

        QJsonObject h = hosts[args[0]].toObject();
        QString user = h["user"].toString();
        QString host = h["host"].toString();
        QString port = h["port"].toString();

        // 检查密钥
        QString keyPath = sshDir + "/id_rsa";
        QString keyOpt = "";
        if (QFile::exists(keyPath)) {
            keyOpt = QString(" -i %1").arg(keyPath);
        }

        QString cmd = QString("ssh -p %1%2 %3@%4").arg(port, keyOpt, user, host);
        ui->appendOutput(QString("连接: %1@%2:%3\n").arg(user, host, port), ACCENT_COLOR);
        ui->executeInternalCommand(cmd);
    }, "SSH快速连接", "ssh-connect <主机名>");

    // ssh-config-gen - 生成 ~/.ssh/config
    registerCommand("ssh-config-gen", [ui, sshDir, hostsPath](const QStringList&) {
        QFile f(hostsPath);
        if (!f.open(QIODevice::ReadOnly)) {
            ui->appendOutput("无主机配置\n", ERROR_COLOR);
            return;
        }
        QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
        f.close();
        QJsonObject hosts = doc.object();

        if (hosts.isEmpty()) {
            ui->appendOutput("无主机配置\n", ERROR_COLOR);
            return;
        }

        QString configPath = sshDir + "/config";
        QFile config(configPath);
        if (!config.open(QIODevice::WriteOnly | QIODevice::Text)) {
            ui->appendOutput("无法写入配置\n", ERROR_COLOR);
            return;
        }

        QTextStream out(&config);
        for (auto it = hosts.begin(); it != hosts.end(); ++it) {
            QJsonObject h = it.value().toObject();
            out << "Host " << it.key() << "\n";
            out << "    HostName " << h["host"].toString() << "\n";
            out << "    User " << h["user"].toString() << "\n";
            out << "    Port " << h["port"].toString() << "\n";
            QString keyPath = sshDir + "/id_rsa";
            if (QFile::exists(keyPath))
                out << "    IdentityFile " << keyPath << "\n";
            out << "\n";
        }
        config.close();
        ui->appendOutput(QString("已生成: %1\n").arg(configPath), PROMPT_COLOR);
        ui->appendOutput("现在可以直接用: ssh <主机名>\n", INFO_COLOR);
    }, "生成SSH配置", "ssh-config-gen");

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
            ui->appendOutput("输入 ssh-pub 查看公钥\n", INFO_COLOR);
        } else {
            ui->appendOutput("生成失败，系统可能没有 ssh-keygen\n", ERROR_COLOR);
            ui->appendOutput("提示: proot-install openssh\n", INFO_COLOR);
        }
    }, "生成SSH密钥", "ssh-keygen [-t rsa|ed25519] [-C 注释]");

    // ssh-keygen -p - 给密钥加密码
    registerCommand("ssh-keygen-pass", [ui, sshDir](const QStringList&) {
        QString keyPath = sshDir + "/id_rsa";
        if (!QFile::exists(keyPath)) {
            ui->appendOutput("密钥不存在，先运行 ssh-keygen\n", ERROR_COLOR);
            return;
        }
        ui->appendOutput("提示: 接下来会要求输入新密码短语\n", ACCENT_COLOR);
        CommandHelper::runExternal("ssh-keygen",
            QStringList() << "-p" << "-f" << keyPath,
            ui->currentDir, ui, 0, true);
    }, "密钥加密码", "ssh-keygen-pass");

    // ssh-pub - 查看公钥
    registerCommand("ssh-pub", [ui, sshDir](const QStringList& args) {
        QString fileName = args.isEmpty() ? "id_rsa" : args[0];
        QString pubPath = sshDir + "/" + fileName + ".pub";

        if (!QFile::exists(pubPath)) {
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
    }, "查看公钥", "ssh-pub");

    // ssh-list - 列出密钥
    registerCommand("ssh-list", [ui, sshDir](const QStringList&) {
        QDir dir(sshDir);
        if (!dir.exists()) {
            ui->appendOutput("无 .ssh 目录\n", ERROR_COLOR);
            return;
        }
        ui->appendOutput("=== SSH 密钥 ===\n", ACCENT_COLOR);
        QStringList files = dir.entryList(QDir::Files);
        for (const QString& f : files) {
            if (f == "hosts.json" || f == "config") continue;
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
            return;
        }
        QString target = args[0];
        QString pubPath = sshDir + "/id_rsa.pub";
        if (!QFile::exists(pubPath)) pubPath = sshDir + "/id_ed25519.pub";
        if (!QFile::exists(pubPath)) {
            ui->appendOutput("公钥不存在，先运行 ssh-keygen\n", ERROR_COLOR);
            return;
        }

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

    // ssh-tunnel - 端口转发
    registerCommand("ssh-tunnel", [ui, sshDir, hostsPath](const QStringList& args) {
        if (args.size() < 2) {
            ui->appendOutput("用法: ssh-tunnel <本地端口>:<目标主机>:<目标端口> <SSH主机>\n", ERROR_COLOR);
            ui->appendOutput("例如: ssh-tunnel 8080:localhost:80 myserver\n", INFO_COLOR);
            ui->appendOutput("说明: 本地8080 → SSH主机能访问的localhost:80\n", INFO_COLOR);
            return;
        }

        QString mapping = args[0];
        QString hostName = args[1];

        // 查主机配置
        QFile f(hostsPath);
        QString user = "root", host = hostName, port = "22";
        if (f.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
            f.close();
            QJsonObject hosts = doc.object();
            if (hosts.contains(hostName)) {
                QJsonObject h = hosts[hostName].toObject();
                user = h["user"].toString();
                host = h["host"].toString();
                port = h["port"].toString();
            }
        }

        QString keyPath = sshDir + "/id_rsa";
        QString keyOpt = QFile::exists(keyPath) ? QString(" -i %1").arg(keyPath) : "";

        QString cmd = QString("ssh -N -L %1 -p %2%3 %4@%5")
            .arg(mapping, port, keyOpt, user, host);

        ui->appendOutput(QString("建立隧道: %1\n").arg(mapping), ACCENT_COLOR);
        ui->appendOutput(QString("SSH: %1@%2:%3\n").arg(user, host, port), INFO_COLOR);
        ui->appendOutput("按 stop 停止\n", INFO_COLOR);

        CommandHelper::runExternal("sh", QStringList() << "-c" << cmd,
                                   ui->currentDir, ui, 0, true);
    }, "SSH端口转发", "ssh-tunnel <映射> <主机>");
}