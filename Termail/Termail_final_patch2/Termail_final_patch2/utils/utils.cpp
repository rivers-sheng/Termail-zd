#include "utils.h"
#include "terminal/terminal.h"
#include "config.h"

#include <QCoreApplication>
#include <QStandardPaths>
#include <QTimer>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QElapsedTimer>
#include <memory>

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

    // 计时器用值语义，lambda 捕获拷贝，生命周期跟随 lambda，不涉及手动释放
    QElapsedTimer elapsed;
    elapsed.start();

    // 共享的"已清理"标志：所有回调共用，保证只清理一次
    // 用 shared_ptr 是因为多个 lambda 需要共享同一份状态
    auto cleaned = std::make_shared<bool>(false);

    // 统一的收尾逻辑，只在第一次调用时生效
    auto doCleanup = [process, ui, cleaned]() {
        if (*cleaned) return;
        *cleaned = true;
        if (ui->currentProcess == process) ui->currentProcess = nullptr;
        process->deleteLater();
    };

    if (realtime) {
        QObject::connect(process, &QProcess::readyReadStandardOutput, process, [process, ui]() {
            ui->appendOutput(process->readAllStandardOutput());
            QCoreApplication::processEvents();
        });
        QObject::connect(process, &QProcess::readyReadStandardError, process, [process, ui]() {
            ui->appendOutput(process->readAllStandardError(), ERROR_COLOR);
            QCoreApplication::processEvents();
        });
    }

    // finished 可能来自正常结束、kill 后的崩溃退出；也可能与 errorOccurred 先后到达
    QObject::connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                     process, [process, ui, realtime, elapsed, program, doCleanup](int, QProcess::ExitStatus status) {
        qint64 ms = elapsed.elapsed();
        if (ms > 1000) {
            ui->appendOutput(QString("[耗时] %1: %2 ms\n").arg(program).arg(ms), INFO_COLOR);
        }

        if (!realtime) {
            QString o = process->readAllStandardOutput();
            if (!o.isEmpty()) ui->appendOutput(o);
            QString e = process->readAllStandardError();
            if (!e.isEmpty()) ui->appendOutput(e, ERROR_COLOR);
        }
        if (status == QProcess::CrashExit)
            ui->appendOutput("[错误] 进程崩溃\n", ERROR_COLOR);

        doCleanup();
    });

    // 启动失败等错误路径，同样走 doCleanup 防重入
    QObject::connect(process, &QProcess::errorOccurred, process, [ui, program, doCleanup](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart)
            ui->appendOutput(QString("[错误] 命令未找到: %1\n").arg(program), ERROR_COLOR);
        doCleanup();
    });

    if (timeout > 0) {
        QTimer* timer = new QTimer();
        timer->setSingleShot(true);
        // 超时只负责终止进程，清理统一交给 finished 回调，避免抢清理
        QObject::connect(timer, &QTimer::timeout, process, [process, ui, timeout]() {
            if (process->state() != QProcess::NotRunning) {
                process->terminate();
                if (!process->waitForFinished(1000)) {
                    process->kill();
                    process->waitForFinished(500);
                }
                ui->appendOutput(QString("[超时] %1秒\n").arg(timeout / 1000), ERROR_COLOR);
            }
        });
        timer->start(timeout);
        // 进程结束时销毁定时器，避免泄漏
        QObject::connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                         timer, &QTimer::deleteLater);
        // 若进程因启动失败提前 errorOccurred，也要销毁定时器
        QObject::connect(process, &QProcess::errorOccurred, timer, &QTimer::deleteLater);
    }

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

void CommandHelper::runExternal(const QString& cmd, const QStringList& args,
                                 const QString& workDir, TerminalWidget* ui,
                                 int timeout, bool realtime) {
    Utils::runExternal(cmd, args, workDir, ui, timeout, realtime);
}

void CommandHelper::runShell(const QString& command, const QString& workDir,
                              TerminalWidget* ui, int timeout) {
    Utils::runSystemCommand(command, workDir, ui, timeout);
}