#include "utils.h"
#include "terminal/terminal.h"
#include "config.h"
#include <QCoreApplication>
#include <QStandardPaths>
#include <QTimer>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>

void Utils::runSystemCommand(const QString& command, const QString& workDir,
                              TerminalWidget* ui, int timeout) {
    runExternal("sh", QStringList() << "-c" << command, workDir, ui, timeout, true);
}

void Utils::runExternal(const QString& program, const QStringList& args,
                         const QString& workDir, TerminalWidget* ui,
                         int timeout, bool realtime) {
    QProcess* process = new QProcess();
    process->setWorkingDirectory(workDir);
    ui->currentProcess = process;
    if (realtime) {
        QObject::connect(process, &QProcess::readyReadStandardOutput, [process, ui]() {
            ui->appendOutput(process->readAllStandardOutput());
            QCoreApplication::processEvents();
        });
        QObject::connect(process, &QProcess::readyReadStandardError, [process, ui]() {
            ui->appendOutput(process->readAllStandardError(), ERROR_COLOR);
            QCoreApplication::processEvents();
        });
    }
    QObject::connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                     [process, ui, realtime](int, QProcess::ExitStatus status) {
        if (!realtime) {
            QString o = process->readAllStandardOutput();
            if (!o.isEmpty()) ui->appendOutput(o);
            QString e = process->readAllStandardError();
            if (!e.isEmpty()) ui->appendOutput(e, ERROR_COLOR);
        }
        if (status == QProcess::CrashExit)
            ui->appendOutput("[错误] 进程崩溃\n", ERROR_COLOR);
        ui->currentProcess = nullptr;
        process->deleteLater();
    });
    QTimer* timer = new QTimer();
    timer->setSingleShot(true);
    QObject::connect(timer, &QTimer::timeout, [process, ui, timeout]() {
        if (process->state() != QProcess::NotRunning) {
            process->kill();
            ui->appendOutput(QString("[超时] %1秒\n").arg(timeout / 1000), ERROR_COLOR);
            ui->currentProcess = nullptr;
        }
    });
    timer->start(timeout);
    QObject::connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                     timer, &QTimer::deleteLater);
    process->start(program, args);
}

bool Utils::copyDir(const QString& src, const QString& dst) {
    QDir srcDir(src);
    if (!srcDir.exists()) return false;
    QDir dstDir(dst);
    if (!dstDir.exists()) QDir().mkpath(dst);
    for (const QString& e : srcDir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot)) {
        QString sp = srcDir.absoluteFilePath(e), dp = dstDir.absoluteFilePath(e);
        if (QFileInfo(sp).isDir()) { if (!copyDir(sp, dp)) return false; }
        else if (!QFile::copy(sp, dp)) return false;
    }
    return true;
}

QJsonObject Utils::loadJsonFile(const QString& path) {
    QFile f(path);
    if (f.open(QIODevice::ReadOnly)) {
        auto doc = QJsonDocument::fromJson(f.readAll());
        f.close();
        return doc.object();
    }
    return QJsonObject();
}

bool Utils::saveJsonFile(const QString& path, const QJsonObject& obj) {
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile f(path);
    if (f.open(QIODevice::WriteOnly)) {
        f.write(QJsonDocument(obj).toJson());
        f.close();
        return true;
    }
    return false;
}

QString Utils::configDir() {
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/termail";
}

QString Utils::customCommandsPath() {
    return configDir() + "/custom_commands.json";
}

QString Utils::formatSize(qint64 b) {
    if (b < 1024) return QString("%1B").arg(b);
    if (b < 1048576) return QString("%1K").arg(b / 1024.0, 0, 'f', 1);
    if (b < 1073741824) return QString("%1M").arg(b / 1048576.0, 0, 'f', 1);
    return QString("%1G").arg(b / 1073741824.0, 0, 'f', 1);
}
// ============ CommandHelper 实现 ============
void CommandHelper::runExternal(const QString& cmd, const QStringList& args,
                                 const QString& workDir, TerminalWidget* ui,
                                 int timeout, bool realtime) {
    Utils::runExternal(cmd, args, workDir, ui, timeout, realtime);
}

void CommandHelper::runShell(const QString& command, const QString& workDir,
                              TerminalWidget* ui, int timeout) {
    Utils::runSystemCommand(command, workDir, ui, timeout);
}