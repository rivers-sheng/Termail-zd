#include "commands.h"
#include "terminal/terminal.h"
#include "utils/utils.h"

#include <QDialog>
#include <QTextEdit>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QDateTime>
#include <QScrollBar>
#include <QProcess>
#include <QDir>
#include <QFile>
#include <QRegularExpression>

static QDialog* g_logDialog = nullptr;
static QDialog* g_termailLogDialog = nullptr;

void CommandRegistry::registerLogCommands(TerminalWidget* ui) {

    registerCommand("log", [ui](const QStringList&) {
        if (g_logDialog && g_logDialog->isVisible()) g_logDialog->close();

        QDialog* dlg = new QDialog(ui);
        g_logDialog = dlg;
        dlg->setWindowTitle("系统日志");
        dlg->setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
        dlg->setModal(false);
        dlg->resize(ui->width(), ui->height());
        dlg->setStyleSheet("background-color: #000000;");

        QVBoxLayout* layout = new QVBoxLayout(dlg);
        layout->setContentsMargins(4, 4, 4, 4);

        QTextEdit* output = new QTextEdit(dlg);
        output->setReadOnly(true);
        output->setFont(QFont(FONT_FAMILY, FONT_SIZE));
        output->setStyleSheet("QTextEdit { background: #000; color: #fff; border: 1px solid #333; }");
        output->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        output->verticalScrollBar()->setStyleSheet(
            "QScrollBar:vertical { width: 20px; background: #111; }"
            "QScrollBar::handle:vertical { background: #555; min-height: 30px; }");
        layout->addWidget(output, 1);

        QLineEdit* input = new QLineEdit(dlg);
        input->setFont(QFont(FONT_FAMILY, FONT_SIZE));
        input->setStyleSheet("QLineEdit { background: #111; color: #0f0; border: 1px solid #333; padding: 4px; }");
        input->setPlaceholderText("过滤词 | /命令 | /exit-log退出");
        layout->addWidget(input);

        // 日志上色函数
        auto colorizeLog = [output](const QString& logText) {
            output->clear();
            QStringList lines = logText.split('\n');
            QTextCursor cursor(output->document());
            for (const QString& line : lines) {
                QString color = "#ffffff"; // 默认白
                // Android logcat 格式: I/Tag, W/Tag, E/Tag, D/Tag, V/Tag
                if (line.contains(QRegularExpression("^\\s*[EW]/")) ||
                    line.contains(QRegularExpression("\\s[EW]/"))) {
                    if (line.contains(QRegularExpression("^\\s*E/")) ||
                        line.contains(QRegularExpression("\\sE/")))
                        color = "#ff4444"; // 错误 红
                    else
                        color = "#ffaa00"; // 警告 橙黄
                } else if (line.contains(QRegularExpression("^\\s*D/")) ||
                           line.contains(QRegularExpression("\\sD/"))) {
                    color = "#888888"; // 调试 灰
                } else if (line.contains(QRegularExpression("^\\s*V/")) ||
                           line.contains(QRegularExpression("\\sV/"))) {
                    color = "#666666"; // 详细 深灰
                } else if (line.contains(QRegularExpression("^\\s*I/")) ||
                           line.contains(QRegularExpression("\\sI/"))) {
                    color = "#ffffff"; // 信息 白
                } else if (line.contains("FATAL") || line.contains("F/")) {
                    color = "#ff0000"; // 致命 亮红
                }

                QTextCharFormat fmt;
                fmt.setForeground(QColor(color));
                cursor.insertText(line + "\n", fmt);
            }
            QScrollBar* sb = output->verticalScrollBar();
            sb->setValue(sb->maximum());
        };

        auto refreshLog = [colorizeLog](const QString& filter = "") {
            QProcess* proc = new QProcess();
#ifdef Q_OS_ANDROID
            proc->start("logcat", QStringList() << "-d" << "-v" << "brief");
#else
            proc->start("journalctl", QStringList() << "--no-pager" << "-n" << "200");
#endif
            proc->waitForFinished(5000);
            QString logText = proc->readAllStandardOutput();
            if (logText.isEmpty()) logText = proc->readAllStandardError();

            if (!filter.isEmpty()) {
                QStringList lines = logText.split('\n');
                QStringList filtered;
                for (const QString& line : lines)
                    if (line.contains(filter, Qt::CaseInsensitive)) filtered.append(line);
                logText = filtered.join('\n');
            }
            if (logText.size() > 100000) logText = logText.right(100000);
            colorizeLog(logText.isEmpty() ? "无日志数据" : logText);
            proc->deleteLater();
        };

        QObject::connect(input, &QLineEdit::returnPressed, [refreshLog, input, output, ui, dlg]() {
            QString text = input->text().trimmed();
            if (text.isEmpty()) return;
            input->clear();
            if (text == "/exit-log") {
                dlg->close();
                g_logDialog = nullptr;
                return;
            }
            if (text.startsWith('/')) {
                QString cmd = text.mid(1);
                output->moveCursor(QTextCursor::End);
                output->insertPlainText(QString("> %1\n").arg(cmd));
                QProcess* proc = new QProcess();
                proc->setWorkingDirectory(ui->currentDir);
                proc->start("sh", QStringList() << "-c" << cmd);
                proc->waitForFinished(-1);
                output->moveCursor(QTextCursor::End);
                output->insertPlainText(proc->readAllStandardOutput());
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

    registerCommand("log-termail", [ui](const QStringList&) {
        if (g_termailLogDialog && g_termailLogDialog->isVisible()) g_termailLogDialog->close();

        QDialog* dlg = new QDialog(ui);
        g_termailLogDialog = dlg;
        dlg->setWindowTitle("Termail日志");
        dlg->setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
        dlg->setModal(false);
        dlg->resize(ui->width(), ui->height());
        dlg->setStyleSheet("background-color: #000000;");

        QVBoxLayout* layout = new QVBoxLayout(dlg);
        layout->setContentsMargins(4, 4, 4, 4);

        QTextEdit* output = new QTextEdit(dlg);
        output->setReadOnly(true);
        output->setFont(QFont(FONT_FAMILY, FONT_SIZE));
        output->setStyleSheet("QTextEdit { background: #000; color: #fff; border: 1px solid #333; }");
        output->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        output->verticalScrollBar()->setStyleSheet(
            "QScrollBar:vertical { width: 20px; background: #111; }"
            "QScrollBar::handle:vertical { background: #555; min-height: 30px; }");
        layout->addWidget(output, 1);

        QString logPath = QDir(ui->currentDir).absoluteFilePath("termail.log");
        QFile logFile(logPath);
        if (logFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString content = logFile.readAll();
            logFile.close();
            // Termail 日志上色
            QStringList lines = content.split('\n');
            QTextCursor cursor(output->document());
            for (const QString& line : lines) {
                QString color = "#ffffff";
                if (line.contains("[ERROR]") || line.contains("[错误]")) color = "#ff4444";
                else if (line.contains("[WARN]") || line.contains("[警告]")) color = "#ffaa00";
                else if (line.contains("[INFO]") || line.contains("[信息]")) color = "#ffffff";
                else if (line.contains("[DEBUG]") || line.contains("[调试]")) color = "#888888";
                QTextCharFormat fmt;
                fmt.setForeground(QColor(color));
                cursor.insertText(line + "\n", fmt);
            }
        } else {
            output->setPlainText("暂无日志\n\n文件: " + logPath);
        }

        QLineEdit* input = new QLineEdit(dlg);
        input->setFont(QFont(FONT_FAMILY, FONT_SIZE));
        input->setStyleSheet("QLineEdit { background: #111; color: #fff; border: 1px solid #333; padding: 4px; }");
        input->setPlaceholderText("日志内容 | 类型|内容 | /命令 | /exit-log退出");
        layout->addWidget(input);

        QObject::connect(input, &QLineEdit::returnPressed, [output, input, logPath, ui, dlg]() {
            QString text = input->text().trimmed();
            if (text.isEmpty()) return;
            input->clear();
            if (text == "/exit-log") {
                dlg->close();
                g_termailLogDialog = nullptr;
                return;
            }
            if (text.startsWith('/')) {
                QString cmd = text.mid(1);
                output->moveCursor(QTextCursor::End);
                output->insertPlainText(QString("> %1\n").arg(cmd));
                QProcess* proc = new QProcess();
                proc->setWorkingDirectory(ui->currentDir);
                proc->start("sh", QStringList() << "-c" << cmd);
                proc->waitForFinished(-1);
                output->moveCursor(QTextCursor::End);
                output->insertPlainText(proc->readAllStandardOutput());
                proc->deleteLater();
                return;
            }

            // 解析类型前缀
            QString type = "INFO";
            QString content = text;
            QString color = "#ffffff";
            if (text.contains('|')) {
                QStringList parts = text.split('|');
                QString prefix = parts[0].trimmed().toLower();
                content = parts.mid(1).join('|').trimmed();
                if (prefix == "错误" || prefix == "error" || prefix == "e") {
                    type = "ERROR"; color = "#ff4444";
                } else if (prefix == "警告" || prefix == "warn" || prefix == "w") {
                    type = "WARN"; color = "#ffaa00";
                } else if (prefix == "调试" || prefix == "debug" || prefix == "d") {
                    type = "DEBUG"; color = "#888888";
                } else if (prefix == "提示" || prefix == "tip") {
                    type = "TIP"; color = "#00ccff";
                }
            }

            QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
            QString entry = QString("[%1] [%2] %3").arg(timestamp, type, content);

            QFile f(logPath);
            if (f.open(QIODevice::Append | QIODevice::Text)) {
                f.write((entry + "\n").toUtf8());
                f.close();
            }

            output->moveCursor(QTextCursor::End);
            QTextCharFormat fmt;
            fmt.setForeground(QColor(color));
            output->textCursor().insertText(entry + "\n", fmt);
        });

        dlg->setAttribute(Qt::WA_DeleteOnClose);
        dlg->show();
        input->setFocus();
    }, "Termail日志", "log-termail");

    registerCommand("exit-log", [ui](const QStringList&) {
        bool closed = false;
        if (g_logDialog && g_logDialog->isVisible()) {
            g_logDialog->close();
            g_logDialog = nullptr;
            ui->appendOutput("系统日志已关闭\n", INFO_COLOR);
            closed = true;
        }
        if (g_termailLogDialog && g_termailLogDialog->isVisible()) {
            g_termailLogDialog->close();
            g_termailLogDialog = nullptr;
            ui->appendOutput("Termail日志已关闭\n", INFO_COLOR);
            closed = true;
        }
        if (!closed) ui->appendOutput("无打开的日志窗口\n", ERROR_COLOR);
    }, "关闭日志", "exit-log");
}