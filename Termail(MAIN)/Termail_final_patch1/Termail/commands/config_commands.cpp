#include "commands.h"
#include "terminal/terminal.h"
#include "utils/utils.h"

#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDateTime>
#include <QProcess>

void CommandRegistry::registerConfigCommands(TerminalWidget* ui) {

    auto configDir = []() -> QString {
        return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/termail";
    };

    // backup - 一键备份配置
    registerCommand("backup", [ui](const QStringList&) {
        QString backupDir = QDir(ui->currentDir).absoluteFilePath(
            QString("termail-backup-%1")
                .arg(QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss")));
        QDir().mkpath(backupDir);

        // 备份文件列表
        QStringList files = {
            "custom_commands.json",
            "aliases.json"
        };

        QString srcDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/termail";

        int count = 0;
        for (const QString& f : files) {
            QString src = srcDir + "/" + f;
            if (QFile::exists(src)) {
                QFile::copy(src, backupDir + "/" + f);
                count++;
            }
        }

        // 备份 .ssh 目录
        QString sshDir = QDir(ui->currentDir).absoluteFilePath(".ssh");
        if (QDir(sshDir).exists()) {
            QDir().mkpath(backupDir + "/.ssh");
            QDir ssh(sshDir);
            for (const QString& f : ssh.entryList(QDir::Files)) {
                QFile::copy(sshDir + "/" + f, backupDir + "/.ssh/" + f);
                count++;
            }
        }

        ui->appendOutput(QString("备份完成: %1\n").arg(backupDir), PROMPT_COLOR);
        ui->appendOutput(QString("共 %1 个文件\n").arg(count), INFO_COLOR);
    }, "备份配置", "backup");

    // restore - 恢复备份
    registerCommand("restore", [ui](const QStringList& args) {
        if (args.isEmpty()) {
            ui->appendOutput("用法: restore <备份目录>\n", ERROR_COLOR);
            ui->appendOutput("例如: restore termail-backup-20250919-120000\n", INFO_COLOR);
            return;
        }
        QString backupDir = QDir(ui->currentDir).absoluteFilePath(args[0]);
        if (!QDir(backupDir).exists()) {
            ui->appendOutput("备份目录不存在\n", ERROR_COLOR);
            return;
        }

        QString targetDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/termail";
        QDir().mkpath(targetDir);

        int count = 0;
        // 恢复配置文件
        QStringList files = {"custom_commands.json", "aliases.json"};
        for (const QString& f : files) {
            QString src = backupDir + "/" + f;
            if (QFile::exists(src)) {
                QFile::remove(targetDir + "/" + f);
                QFile::copy(src, targetDir + "/" + f);
                count++;
            }
        }

        // 恢复 .ssh
        QString sshBackup = backupDir + "/.ssh";
        if (QDir(sshBackup).exists()) {
            QString sshTarget = QDir(ui->currentDir).absoluteFilePath(".ssh");
            QDir().mkpath(sshTarget);
            QDir ssh(sshBackup);
            for (const QString& f : ssh.entryList(QDir::Files)) {
                QFile::remove(sshTarget + "/" + f);
                QFile::copy(sshBackup + "/" + f, sshTarget + "/" + f);
                count++;
            }
        }

        ui->appendOutput(QString("恢复完成，共 %1 个文件\n").arg(count), PROMPT_COLOR);
        ui->appendOutput("重启 Termail 后生效\n", INFO_COLOR);
    }, "恢复配置", "restore <备份目录>");

    // export-config - 导出配置为JSON
    registerCommand("export-config", [ui](const QStringList&) {
        QString fn = QString("termail-config-%1.json")
            .arg(QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss"));
        QString path = QDir(ui->currentDir).absoluteFilePath(fn);

        QJsonObject root;
        root["export_time"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        root["version"] = APP_VERSION;

        // 自定义命令
        QJsonObject cmds;
        for (auto it = ui->customCommands.begin(); it != ui->customCommands.end(); ++it)
            cmds[it.key()] = it.value();
        root["custom_commands"] = cmds;

        // 别名
        QJsonObject aliasObj;
        for (auto it = ui->aliases.begin(); it != ui->aliases.end(); ++it)
            aliasObj[it.key()] = it.value();
        root["aliases"] = aliasObj;

        // 字体
        root["font_size"] = ui->outputArea->font().pointSize();

        QFile f(path);
        if (f.open(QIODevice::WriteOnly)) {
            f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
            f.close();
            ui->appendOutput(QString("已导出: %1\n").arg(fn), PROMPT_COLOR);
        } else {
            ui->appendOutput("导出失败\n", ERROR_COLOR);
        }
    }, "导出配置", "export-config");

    // import-config - 导入配置
    registerCommand("import-config", [ui](const QStringList& args) {
        if (args.isEmpty()) {
            ui->appendOutput("用法: import-config <文件.json>\n", ERROR_COLOR);
            return;
        }
        QFile f(QDir(ui->currentDir).absoluteFilePath(args[0]));
        if (!f.open(QIODevice::ReadOnly)) {
            ui->appendOutput("文件不存在\n", ERROR_COLOR);
            return;
        }
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
        f.close();
        if (err.error != QJsonParseError::NoError) {
            ui->appendOutput("JSON 格式错误\n", ERROR_COLOR);
            return;
        }
        QJsonObject root = doc.object();

        // 导入自定义命令
        if (root.contains("custom_commands")) {
            QJsonObject cmds = root["custom_commands"].toObject();
            for (auto it = cmds.begin(); it != cmds.end(); ++it)
                ui->customCommands[it.key()] = it.value().toObject();
        }

        // 导入别名
        if (root.contains("aliases")) {
            QJsonObject aliasObj = root["aliases"].toObject();
            for (auto it = aliasObj.begin(); it != aliasObj.end(); ++it)
                ui->aliases[it.key()] = it.value().toString();
        }

        // 导入字体
        if (root.contains("font_size")) {
            int size = root["font_size"].toInt();
            if (size >= 8 && size <= 32) {
                QFont f(ui->outputArea->font().family(), size);
                ui->outputArea->setFont(f);
                ui->inputField->setFont(f);
                ui->promptLabel->setFont(f);
            }
        }

        ui->saveCustomCommands();
        ui->saveAliases();
        ui->appendOutput("导入完成\n", PROMPT_COLOR);
    }, "导入配置", "import-config <文件>");

    // reset-all - 恢复出厂设置
    registerCommand("reset-all", [ui](const QStringList& args) {
        if (args.isEmpty() || args[0] != "-y") {
            ui->appendOutput("⚠️ 此操作将删除所有配置！\n", ERROR_COLOR);
            ui->appendOutput("包括: 自定义命令、别名、字体设置\n", ERROR_COLOR);
            ui->appendOutput("确认请输入: reset-all -y\n", INFO_COLOR);
            return;
        }

        QString dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/termail";
        QDir(dir).removeRecursively();

        ui->customCommands.clear();
        ui->aliases.clear();

        ui->appendOutput("已恢复出厂设置\n", PROMPT_COLOR);
        ui->appendOutput("重启 Termail 后生效\n", INFO_COLOR);
    }, "恢复出厂", "reset-all -y");

    // config-info - 配置信息
    registerCommand("config-info", [ui](const QStringList&) {
        QString dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/termail";
        ui->appendOutput("=== 配置信息 ===\n", ACCENT_COLOR);
        ui->appendOutput(QString("配置目录: %1\n").arg(dir), INFO_COLOR);
        ui->appendOutput(QString("自定义命令: %1 个\n").arg(ui->customCommands.size()));
        ui->appendOutput(QString("别名: %1 个\n").arg(ui->aliases.size()));
        ui->appendOutput(QString("字号: %1\n").arg(ui->outputArea->font().pointSize()));

        // 检查文件
        QStringList files = {"custom_commands.json", "aliases.json"};
        for (const QString& f : files) {
            QString path = dir + "/" + f;
            if (QFile::exists(path)) {
                QFileInfo info(path);
                ui->appendOutput(QString("%1: %2 字节\n")
                    .arg(f).arg(info.size()), PROMPT_COLOR);
            }
        }
    }, "配置信息", "config-info");
}