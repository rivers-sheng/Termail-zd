#include "commands.h"
#include "terminal/terminal.h"
#include "utils/utils.h"

#include <QDialog>
#include <QTextEdit>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QLabel>
#include <QDateTime>
#include <QScrollBar>
#include <QProcess>
#include <QDir>
#include <QFile>

// 全局日志窗口指针
static QDialog* g_logDialog = nullptr;
static QDialog* g_termailLogDialog = nullptr;

void CommandRegistry::registerLogCommands(TerminalWidget* ui) {

    // ============ log - 系统日志 ============
    registerCommand("log", [ui](const QStringList&) {
        if (g_logDialog && g_logDialog->isVisible()) {
            g_logDialog->close();
        }

        QDialog* dlg = new QDialog(ui);
        g_logDialog = dlg;
        dlg->setWindowTitle("系统日志");
        dlg->setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
        dlg->setModal(false);
        dlg->resize(ui->width(), ui->height());
        dlg->move(0, 0);
        dlg->setStyleSheet("background-color: #000000;");

        QVBoxLayout* layout = new QVBoxLayout(dlg);
        layout->setContentsMargins(4, 4, 4, 4);
        layout->setSpacing(4);

        QTextEdit* output = new QTextEdit(dlg);
        output->setReadOnly(true);
        output->setFont(QFont(FONT_FAMILY, FONT_SIZE));
        output->setStyleSheet("QTextEdit { background: #000; color: #fff; border: 1px solid #333; }");
        output->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        output->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        output->verticalScrollBar()->setStyleSheet(
            "QScrollBar:vertical { width: 20px; background: #111; }"
            "QScrollBar::handle:vertical { background: #555; min-height: 30px; }"
            "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
        );
        layout->addWidget(output, 1);

        QLineEdit* input = new QLineEdit(dlg);
        input->setFont(QFont(FONT_FAMILY, FONT_SIZE));
        input->setStyleSheet("QLineEdit { background: #111; color: #0f0; border: 1px solid #333; padding: 4px; }");
        input->setPlaceholderText("过滤词回车刷新 | /命令执行 | /exit-log退出");
        layout->addWidget(input);

        auto refreshLog = [output](const QString& filter = "") {
            output->clear();
            QProcess* proc = new QProcess();
#ifdef Q_OS_ANDROID
            proc->start("logcat", QStringList() << "-d");
#else
            proc->start("journalctl", QStringList() << "--no-pager" << "-n" << "100");
#endif
            proc->waitForFinished(5000);
            QString logText = proc->readAllStandardOutput();
            if (logText.isEmpty()) logText = proc->readAllStandardError();

            if (!filter.isEmpty()) {
                QStringList lines = logText.split('\n');
                QStringList filtered;
                for (const QString& line : lines) {
                    if (line.contains(filter, Qt::CaseInsensitive)) filtered.append(line);
                }
                logText = filtered.join('\n');
            }
            if (logText.size() > 50000) logText = logText.right(50000);
            output->setPlainText(logText.isEmpty() ? "无日志数据" : logText);
            proc->deleteLater();
        };

        QObject::connect(input, &QLineEdit::returnPressed, [refreshLog, input, output, ui, dlg]() {
            QString text = input->text().trimmed();
            if (text.isEmpty()) return;
            input->clear();

            // 退出命令
            if (text == "/exit-log") {
                dlg->close();
                g_logDialog = nullptr;
                ui->appendOutput("系统日志窗口已关闭\n", INFO_COLOR);
                return;
            }

            // 切换程序日志
            if (text == "/log-termail") {
                dlg->close();
                g_logDialog = nullptr;
                ui->executeInternalCommand("log-termail");
                return;
            }

            // 命令执行
            if (text.startsWith('/')) {
                QString cmd = text.mid(1);
                output->moveCursor(QTextCursor::End);
                output->insertPlainText(QString("> %1\n").arg(cmd));
                QProcess* proc = new QProcess();
                proc->setWorkingDirectory(ui->currentDir);
                proc->start("sh", QStringList() << "-c" << cmd);
                proc->waitForFinished(5000);
                QString result = proc->readAllStandardOutput();
                QString err = proc->readAllStandardError();
                output->moveCursor(QTextCursor::End);
                if (!result.isEmpty()) output->insertPlainText(result);
                if (!err.isEmpty())
                    output->insertHtml(QString("<span style='color:#ff4444;'>%1</span>")
                        .arg(err.toHtmlEscaped().replace("\n", "<br>")));
                proc->deleteLater();
            } else {
                refreshLog(text);
            }
        });

        refreshLog();
        dlg->setAttribute(Qt::WA_DeleteOnClose);
        dlg->show();
        input->setFocus();
    }, "系统日志", "log");

    // ============ log-termail - 程序日志 ============
    registerCommand("log-termail", [ui](const QStringList&) {
        if (g_termailLogDialog && g_termailLogDialog->isVisible()) {
            g_termailLogDialog->close();
        }

        QDialog* dlg = new QDialog(ui);
        g_termailLogDialog = dlg;
        dlg->setWindowTitle("Termail 程序日志");
        dlg->setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
        dlg->setModal(false);
        dlg->resize(ui->width(), ui->height());
        dlg->move(0, 0);
        dlg->setStyleSheet("background-color: #000000;");

        QVBoxLayout* layout = new QVBoxLayout(dlg);
        layout->setContentsMargins(4, 4, 4, 4);
        layout->setSpacing(4);

        QTextEdit* output = new QTextEdit(dlg);
        output->setReadOnly(true);
        output->setFont(QFont(FONT_FAMILY, FONT_SIZE));
        output->setStyleSheet("QTextEdit { background: #000; color: #fff; border: 1px solid #333; }");
        output->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        output->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        output->verticalScrollBar()->setStyleSheet(
            "QScrollBar:vertical { width: 20px; background: #111; }"
            "QScrollBar::handle:vertical { background: #555; min-height: 30px; }"
            "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
        );
        layout->addWidget(output, 1);

        QString logPath = QDir(ui->currentDir).absoluteFilePath("termail.log");
        QFile logFile(logPath);
        if (logFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            output->setPlainText(logFile.readAll());
            logFile.close();
        } else {
            output->setPlainText("暂无 Termail 日志\n\n日志文件: " + logPath);
        }

        QLineEdit* input = new QLineEdit(dlg);
        input->setFont(QFont(FONT_FAMILY, FONT_SIZE));
        input->setStyleSheet("QLineEdit { background: #111; color: #fff; border: 1px solid #333; padding: 4px; }");
        input->setPlaceholderText("日志内容回车写入 | /命令执行 | /exit-log退出");
        layout->addWidget(input);

        QObject::connect(input, &QLineEdit::returnPressed, [output, input, logPath, ui, dlg]() {
            QString text = input->text().trimmed();
            if (text.isEmpty()) return;
            input->clear();

            // 退出命令
            if (text == "/exit-log") {
                dlg->close();
                g_termailLogDialog = nullptr;
                ui->appendOutput("Termail日志窗口已关闭\n", INFO_COLOR);
                return;
            }

            // 切换系统日志
            if (text == "/log") {
                dlg->close();
                g_termailLogDialog = nullptr;
                ui->executeInternalCommand("log");
                return;
            }

            // 命令执行
            if (text.startsWith('/')) {
                QString cmd = text.mid(1);
                output->moveCursor(QTextCursor::End);
                output->insertPlainText(QString("> %1\n").arg(cmd));
                QProcess* proc = new QProcess();
                proc->setWorkingDirectory(ui->currentDir);
                proc->start("sh", QStringList() << "-c" << cmd);
                proc->waitForFinished(5000);
                QString result = proc->readAllStandardOutput();
                QString err = proc->readAllStandardError();
                output->moveCursor(QTextCursor::End);
                if (!result.isEmpty()) output->insertPlainText(result);
                if (!err.isEmpty())
                    output->insertHtml(QString("<span style='color:#ff4444;'>%1</span>")
                        .arg(err.toHtmlEscaped().replace("\n", "<br>")));
                proc->deleteLater();
                return;
            }

            // 写日志
            QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
            QString entry = QString("[%1] %2\n").arg(timestamp, text);
            QFile f(logPath);
            if (f.open(QIODevice::Append | QIODevice::Text)) {
                f.write(entry.toUtf8());
                f.close();
            }
            output->moveCursor(QTextCursor::End);
            output->insertPlainText(entry);
        });

        dlg->setAttribute(Qt::WA_DeleteOnClose);
        dlg->show();
        input->setFocus();
    }, "Termail程序日志", "log-termail");

    // ============ exit-log - 关闭日志窗口（主终端用） ============
    registerCommand("exit-log", [ui](const QStringList&) {
        bool closed = false;
        if (g_logDialog && g_logDialog->isVisible()) {
            g_logDialog->close();
            g_logDialog = nullptr;
            ui->appendOutput("系统日志窗口已关闭\n", INFO_COLOR);
            closed = true;
        }
        if (g_termailLogDialog && g_termailLogDialog->isVisible()) {
            g_termailLogDialog->close();
            g_termailLogDialog = nullptr;
            ui->appendOutput("Termail日志窗口已关闭\n", INFO_COLOR);
            closed = true;
        }
        if (!closed) {
            ui->appendOutput("没有打开的日志窗口\n", ERROR_COLOR);
        }
    }, "关闭日志窗口", "exit-log");
}