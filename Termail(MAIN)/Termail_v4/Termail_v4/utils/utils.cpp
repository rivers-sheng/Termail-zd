#include "utils.h"
#include "ui/terminal_ui.h"
#include "config.h"
#include <QCoreApplication>
#include <QStandardPaths>
#include <QThread>
#include <QTimer>
#include <QDirIterator>

void Utils::runSystemCommand(const QString& command, const QString& workDir,
                              TerminalUI* ui, int timeout) {
    runExternal("sh", QStringList() << "-c" << command, workDir, ui, timeout, true);
}

void Utils::runExternal(const QString& program, const QStringList& args,
                         const QString& workDir, TerminalUI* ui,
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
            ui->appendOutput(process->readAllStandardError(), THEME.red.name());
            QCoreApplication::processEvents();
        });
    }

    QObject::connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                     [process, ui, realtime](int, QProcess::ExitStatus status) {
        if (!realtime) {
            QString o = process->readAllStandardOutput();
            if (!o.isEmpty()) ui->appendOutput(o);
            QString e = process->readAllStandardError();
            if (!e.isEmpty()) ui->appendOutput(e, THEME.red.name());
        }
        if (status == QProcess::CrashExit)
            ui->appendOutput("[错误] 进程崩溃\n", THEME.red.name());
        ui->currentProcess = nullptr;
        process->deleteLater();
    });

    QObject::connect(process, &QProcess::errorOccurred, [process, ui, program](QProcess::ProcessError) {
        ui->appendOutput(QString("[错误] 命令未找到: %1\n").arg(program), THEME.red.name());
        ui->currentProcess = nullptr;
        process->deleteLater();
    });

    QTimer* timer = new QTimer();
    timer->setSingleShot(true);
    QObject::connect(timer, &QTimer::timeout, [process, ui, timeout]() {
        if (process->state() != QProcess::NotRunning) {
            process->kill();
            ui->appendOutput(QString("[超时] %1 秒\n").arg(timeout / 1000), THEME.red.name());
            ui->currentProcess = nullptr;
        }
    });
    timer->start(timeout);
    QObject::connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                     timer, &QTimer::deleteLater);

    process->start(program, args);
}

void Utils::runShell(const QString& command, const QString& workDir,
                      TerminalUI* ui, int timeout) {
    runSystemCommand(command, workDir, ui, timeout);
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

bool Utils::removeDir(const QString& path) { return QDir(path).removeRecursively(); }

QJsonObject Utils::loadJsonFile(const QString& path) {
    QFile f(path);
    if (f.open(QIODevice::ReadOnly)) {
        auto doc = QJsonDocument::fromJson(f.readAll()); f.close();
        return doc.object();
    }
    return QJsonObject();
}

bool Utils::saveJsonFile(const QString& path, const QJsonObject& obj) {
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile f(path);
    if (f.open(QIODevice::WriteOnly)) { f.write(QJsonDocument(obj).toJson()); f.close(); return true; }
    return false;
}

QString Utils::configDir() { return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/termail"; }
QString Utils::customCommandsPath() { return configDir() + "/custom_commands.json"; }

QString Utils::formatSize(qint64 b) {
    if (b < 1024) return QString("%1B").arg(b);
    if (b < 1048576) return QString("%1K").arg(b / 1024.0, 0, 'f', 1);
    if (b < 1073741824) return QString("%1M").arg(b / 1048576.0, 0, 'f', 1);
    return QString("%1G").arg(b / 1073741824.0, 0, 'f', 1);
}

void CommandHelper::runExternal(const QString& c, const QStringList& a, const QString& d, TerminalUI* u, int t, bool r) {
    Utils::runExternal(c, a, d, u, t, r);
}
void CommandHelper::runShell(const QString& c, const QString& d, TerminalUI* u, int t) {
    Utils::runSystemCommand(c, d, u, t);
}