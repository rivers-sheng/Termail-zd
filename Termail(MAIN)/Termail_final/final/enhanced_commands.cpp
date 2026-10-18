#include <QCoreApplication>
#include <QApplication>
#include "commands.h"
#include "terminal/terminal.h"
#include "utils/utils.h"

#include <QProcess>
#include <QDateTime>
#include <QElapsedTimer>
#include <QThread>
#include <QTimer>

void CommandRegistry::registerEnhancedCommands(TerminalWidget* ui) {

    // multi - 串行执行（分号分隔）
    registerCommand("multi", [ui](const QStringList& args) {
        if (args.isEmpty()) {
            ui->appendOutput("用法: multi <命令1> ; <命令2> ; ...\n", ERROR_COLOR);
            ui->appendOutput("例如: multi echo a ; echo b ; echo c\n", INFO_COLOR);
            return;
        }
        QString full = args.join(' ');
        QStringList cmds = full.split(';');
        ui->appendOutput(QString("串行执行 %1 条命令\n").arg(cmds.size()), ACCENT_COLOR);
        for (const QString& cmd : cmds) {
            QString c = cmd.trimmed();
            if (!c.isEmpty()) {
                ui->appendOutput(QString("> %1\n").arg(c), INFO_COLOR);
                ui->executeInternalCommand(c);
            }
        }
    }, "串行执行", "multi <命令1> ; <命令2>");

    // parallel - 并行执行（& 分隔）
    registerCommand("parallel", [ui](const QStringList& args) {
        if (args.isEmpty()) {
            ui->appendOutput("用法: parallel <命令1> & <命令2>\n", ERROR_COLOR);
            return;
        }
        QString full = args.join(' ');
        QStringList cmds = full.split('&');
        ui->appendOutput(QString("并行执行 %1 条命令\n").arg(cmds.size()), ACCENT_COLOR);

        for (const QString& cmd : cmds) {
            QString c = cmd.trimmed();
            if (c.isEmpty()) continue;

            QProcess* proc = new QProcess();
            proc->setWorkingDirectory(ui->currentDir);

            QObject::connect(proc, &QProcess::readyReadStandardOutput, [proc, ui]() {
                ui->appendOutput(proc->readAllStandardOutput());
            });
            QObject::connect(proc, &QProcess::readyReadStandardError, [proc, ui]() {
                ui->appendOutput(proc->readAllStandardError(), ERROR_COLOR);
            });
            QObject::connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                             [proc](int, QProcess::ExitStatus) {
                proc->deleteLater();
            });

            ui->appendOutput(QString("  [并行] %1\n").arg(c), INFO_COLOR);
            proc->start("sh", QStringList() << "-c" << c);
        }
    }, "并行执行", "parallel <命令1> & <命令2>");

    // repeat - 重复执行
    registerCommand("repeat", [ui](const QStringList& args) {
        if (args.size() < 2) {
            ui->appendOutput("用法: repeat <次数> <命令>\n", ERROR_COLOR);
            ui->appendOutput("例如: repeat 5 echo hello\n", INFO_COLOR);
            return;
        }
        bool ok;
        int n = args[0].toInt(&ok);
        if (!ok || n <= 0) {
            ui->appendOutput("次数必须是正整数\n", ERROR_COLOR);
            return;
        }
        if (n > 1000) {
            ui->appendOutput("次数不能超过1000\n", ERROR_COLOR);
            return;
        }
        QString cmd = args.mid(1).join(' ');
        ui->appendOutput(QString("重复执行 %1 次: %2\n").arg(n).arg(cmd), ACCENT_COLOR);
        for (int i = 0; i < n; ++i) {
            ui->appendOutput(QString("[%1/%2] ").arg(i + 1).arg(n), INFO_COLOR);
            ui->executeInternalCommand(cmd);
        }
    }, "重复执行", "repeat <次数> <命令>");

    // benchmark - 测试命令耗时
    registerCommand("benchmark", [ui](const QStringList& args) {
        if (args.isEmpty()) {
            ui->appendOutput("用法: benchmark <命令>\n", ERROR_COLOR);
            return;
        }
        QString cmd = args.join(' ');

        QElapsedTimer timer;
        timer.start();

        QProcess proc;
        proc.setWorkingDirectory(ui->currentDir);
        proc.start("sh", QStringList() << "-c" << cmd);
        proc.waitForFinished(-1);

        qint64 ms = timer.elapsed();

        QString output = proc.readAllStandardOutput();
        if (!output.isEmpty()) ui->appendOutput(output);

        ui->appendOutput(QString("\n⏱️ 耗时: %1 ms (%2 秒)\n")
            .arg(ms).arg(ms / 1000.0, 0, 'f', 3), PROMPT_COLOR);
    }, "测试耗时", "benchmark <命令>");

    // seq-exec - 批量执行（带变量）
    registerCommand("seq-exec", [ui](const QStringList& args) {
        if (args.size() < 3) {
            ui->appendOutput("用法: seq-exec <起始> <结束> <命令模板>\n", ERROR_COLOR);
            ui->appendOutput("模板中用 $1 代表当前值\n", INFO_COLOR);
            ui->appendOutput("例如: seq-exec 1 5 echo 第$1次\n", INFO_COLOR);
            return;
        }
        int start = args[0].toInt();
        int end = args[1].toInt();
        QString tmpl = args.mid(2).join(' ');

        if (end < start) { int t = start; start = end; end = t; }
        if (end - start > 100) {
            ui->appendOutput("范围不能超过100\n", ERROR_COLOR);
            return;
        }

        ui->appendOutput(QString("批量执行 %1 到 %2\n").arg(start).arg(end), ACCENT_COLOR);
        for (int i = start; i <= end; ++i) {
            QString cmd = tmpl;
            cmd.replace("$1", QString::number(i));
            ui->appendOutput(QString("> %1\n").arg(cmd), INFO_COLOR);
            ui->executeInternalCommand(cmd);
        }
    }, "批量执行", "seq-exec <起始> <结束> <模板>");

    // timeit - 简化版 benchmark
    registerCommand("timeit", [ui](const QStringList& args) {
        ui->executeInternalCommand("benchmark " + args.join(' '));
    }, "测试耗时", "timeit <命令>");

    // xarg - 参数传递执行
    registerCommand("xarg", [ui](const QStringList& args) {
        if (args.size() < 2) {
            ui->appendOutput("用法: xarg <命令> <参数1> <参数2> ...\n", ERROR_COLOR);
            ui->appendOutput("例如: xarg echo a b c\n", INFO_COLOR);
            return;
        }
        QString cmd = args[0];
        for (int i = 1; i < args.size(); ++i) {
            ui->executeInternalCommand(cmd + " " + args[i]);
        }
    }, "参数传递", "xarg <命令> <参数...>");

    // until - 循环执行直到成功
    registerCommand("until", [ui](const QStringList& args) {
        if (args.size() < 2) {
            ui->appendOutput("用法: until <次数> <命令>\n", ERROR_COLOR);
            ui->appendOutput("例如: until 5 ping -c 1 google.com\n", INFO_COLOR);
            return;
        }
        int maxTry = args[0].toInt();
        if (maxTry <= 0 || maxTry > 100) maxTry = 10;
        QString cmd = args.mid(1).join(' ');

        ui->appendOutput(QString("循环执行（最多%1次）: %2\n").arg(maxTry).arg(cmd), ACCENT_COLOR);
        for (int i = 1; i <= maxTry; ++i) {
            ui->appendOutput(QString("\n--- 第 %1 次 ---\n").arg(i), INFO_COLOR);
            ui->executeInternalCommand(cmd);
        }
    }, "循环执行", "until <次数> <命令>");

    // now - 显示当前时间（多格式）
    registerCommand("now", [ui](const QStringList&) {
        QDateTime now = QDateTime::currentDateTime();
        ui->appendOutput(QString("日期: %1\n").arg(now.toString("yyyy-MM-dd")), PROMPT_COLOR);
        ui->appendOutput(QString("时间: %1\n").arg(now.toString("hh:mm:ss")), PROMPT_COLOR);
        ui->appendOutput(QString("星期: %1\n").arg(now.toString("dddd")), INFO_COLOR);
        ui->appendOutput(QString("时间戳: %1\n").arg(now.toSecsSinceEpoch()), INFO_COLOR);
        ui->appendOutput(QString("ISO: %1\n").arg(now.toString(Qt::ISODate)), INFO_COLOR);
    }, "当前时间", "now");

    // countdown - 倒计时
    registerCommand("countdown", [ui](const QStringList& args) {
        if (args.isEmpty()) {
            ui->appendOutput("用法: countdown <秒数>\n", ERROR_COLOR);
            return;
        }
        int secs = args[0].toInt();
        if (secs <= 0 || secs > 3600) {
            ui->appendOutput("秒数需在 1-3600 之间\n", ERROR_COLOR);
            return;
        }
        ui->appendOutput(QString("倒计时 %1 秒开始...\n").arg(secs), ACCENT_COLOR);
        for (int i = secs; i > 0; --i) {
            ui->appendOutput(QString("\r剩余: %1 秒").arg(i), PROMPT_COLOR);
            QThread::msleep(1000);
            QCoreApplication::processEvents();
        }
        ui->appendOutput("\n⏰ 时间到！\n", PROMPT_COLOR);
        QApplication::beep();
    }, "倒计时", "countdown <秒数>");
}