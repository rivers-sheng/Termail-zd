#include <QTimer>
#include <QDir>
#include "commands.h"
#include "terminal/terminal.h"
#include "utils/utils.h"

#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QCryptographicHash>
#include <QUuid>
#include <QRandomGenerator>
#include <QJsonDocument>
#include <QJsonObject>
#include <QThread>
#include <algorithm>
#include <cmath>
#include <random>

void CommandRegistry::registerTextCommands(TerminalWidget* ui) {

    // tr
    registerCommand("tr", [ui](const QStringList& args) {
        if (args.size() < 2) return;
        QString text = args.mid(2).join(' ');
        QString result;
        for (const QChar& ch : text) {
            int idx = args[0].indexOf(ch);
            result += (idx >= 0 && idx < args[1].size()) ? args[1][idx] : ch;
        }
        ui->appendOutput(result + "\n");
    }, "字符替换", "tr <集1> <集2> [文本]");

    // rev
    registerCommand("rev", [ui](const QStringList& args) {
        QString text = args.join(' ');
        std::reverse(text.begin(), text.end());
        ui->appendOutput(text + "\n");
    }, "反转文本", "rev <文本>");

    // tac
    registerCommand("tac", [ui](const QStringList& args) {
        if (args.isEmpty()) return;
        QFile file(QDir(ui->currentDir).absoluteFilePath(args[0]));
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QStringList lines = QString(file.readAll()).split('\n');
            file.close();
            std::reverse(lines.begin(), lines.end());
            for (const QString& l : lines)
                if (!l.isEmpty()) ui->appendOutput(l + "\n");
        }
    }, "反向文件", "tac <文件>");

    // nl
    registerCommand("nl", [ui](const QStringList& args) {
        if (args.isEmpty()) return;
        QFile file(QDir(ui->currentDir).absoluteFilePath(args[0]));
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            int n = 1;
            while (!in.atEnd())
                ui->appendOutput(QString("%1\t%2\n").arg(n++, 6).arg(in.readLine()));
            file.close();
        }
    }, "带行号显示", "nl <文件>");

    // paste
    registerCommand("paste", [ui](const QStringList& args) {
        if (args.size() < 2) return;
        QFile f1(QDir(ui->currentDir).absoluteFilePath(args[0]));
        QFile f2(QDir(ui->currentDir).absoluteFilePath(args[1]));
        if (f1.open(QIODevice::ReadOnly) && f2.open(QIODevice::ReadOnly)) {
            QStringList l1 = QString(f1.readAll()).split('\n');
            QStringList l2 = QString(f2.readAll()).split('\n');
            for (int i = 0; i < qMax(l1.size(), l2.size()); ++i)
                ui->appendOutput(QString("%1\t%2\n")
                    .arg(i < l1.size() ? l1[i] : "", i < l2.size() ? l2[i] : ""));
        }
    }, "合并文件", "paste <文件1> <文件2>");

    // split
    registerCommand("split", [ui](const QStringList& args) {
        if (args.isEmpty()) return;
        int n = 1000, idx = 0;
        if (args[0] == "-l" && args.size() > 1) { n = args[1].toInt(); idx = 2; }
        if (idx >= args.size()) return;
        QFile file(QDir(ui->currentDir).absoluteFilePath(args[idx]));
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QStringList lines = QString(file.readAll()).split('\n');
            file.close();
            for (int i = 0; i < lines.size(); i += n) {
                QFile out(QDir(ui->currentDir).absoluteFilePath(
                    QString("%1.part%2").arg(args[idx]).arg(i / n + 1)));
                if (out.open(QIODevice::WriteOnly)) {
                    for (int j = i; j < qMin(i + n, lines.size()); ++j)
                        out.write((lines[j] + "\n").toUtf8());
                    out.close();
                }
            }
        }
    }, "分割文件", "split [-l N] <文件>");

    // join
    registerCommand("join", [ui](const QStringList& args) {
        for (const QString& fn : args) {
            QFile file(QDir(ui->currentDir).absoluteFilePath(fn));
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                ui->appendOutput(file.readAll());
                file.close();
            }
        }
    }, "连接文件", "join <文件...>");

    // fmt
    registerCommand("fmt", [ui](const QStringList& args) {
        if (args.isEmpty()) return;
        QFile file(QDir(ui->currentDir).absoluteFilePath(args[0]));
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QStringList words = QString(file.readAll()).split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
            file.close();
            QString line;
            for (const QString& w : words) {
                if (line.size() + w.size() + 1 > 72) {
                    ui->appendOutput(line.trimmed() + "\n");
                    line.clear();
                }
                line += w + " ";
            }
            if (!line.isEmpty()) ui->appendOutput(line.trimmed() + "\n");
        }
    }, "格式化文本", "fmt <文件>");

    // sed
    registerCommand("sed", [ui](const QStringList& args) {
        if (args.size() < 2) return;
        QString pattern = args[0];
        if (pattern.startsWith("s/") && pattern.endsWith("/")) {
            QString inner = pattern.mid(2, pattern.size() - 3);
            int sep = inner.indexOf('/');
            if (sep > 0) {
                QFile file(QDir(ui->currentDir).absoluteFilePath(args.last()));
                if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    QString content = file.readAll();
                    file.close();
                    content.replace(QRegularExpression(inner.left(sep)), inner.mid(sep + 1));
                    ui->appendOutput(content);
                }
            }
        }
    }, "流编辑器", "sed 's/旧/新/' <文件>");

    // zip / unzip / tar
    registerCommand("zip", [ui](const QStringList& args) {
        CommandHelper::runExternal("zip", args, ui->currentDir, ui);
    }, "压缩", "zip");

    registerCommand("unzip", [ui](const QStringList& args) {
        CommandHelper::runExternal("unzip", args, ui->currentDir, ui);
    }, "解压", "unzip");

    registerCommand("tar", [ui](const QStringList& args) {
        CommandHelper::runExternal("tar", args, ui->currentDir, ui);
    }, "归档", "tar");

    // hash
    registerCommand("hash", [ui](const QStringList& args) {
        if (args.isEmpty()) return;
        QByteArray data = args.join(' ').toUtf8();
        ui->appendOutput(QString("MD5: %1\n").arg(QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex()));
        ui->appendOutput(QString("SHA256: %1\n").arg(QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex()));
    }, "计算哈希", "hash <文本>");

    // base64
    registerCommand("base64", [ui](const QStringList& args) {
        if (!args.isEmpty())
            ui->appendOutput(args.join(' ').toUtf8().toBase64() + "\n");
    }, "Base64编码", "base64 <文本>");

    // uuid
    registerCommand("uuid", [ui](const QStringList&) {
        ui->appendOutput(QUuid::createUuid().toString(QUuid::WithoutBraces) + "\n");
    }, "生成UUID", "uuid");

    // random
    registerCommand("random", [ui](const QStringList& args) {
        if (args.isEmpty())
            ui->appendOutput(QString::number(QRandomGenerator::global()->generateDouble()) + "\n");
        else if (args.size() == 1)
            ui->appendOutput(QString::number(QRandomGenerator::global()->bounded(args[0].toInt() + 1)) + "\n");
        else
            ui->appendOutput(QString::number(QRandomGenerator::global()->bounded(args[0].toInt(), args[1].toInt() + 1)) + "\n");
    }, "随机数", "random [最小] [最大]");

    // sleep
    registerCommand("sleep", [ui](const QStringList& args) {
        if (!args.isEmpty())
            QThread::msleep(static_cast<unsigned long>(args[0].toDouble() * 1000));
    }, "延时", "sleep <秒>");

    // watch
    registerCommand("watch", [ui](const QStringList& args) {
        if (args.isEmpty()) return;
        QString cmd = args.join(' ');
        for (int i = 0; i < 3; ++i) {
            QThread::sleep(2);
            ui->appendOutput(QString("\n--- %1 ---\n").arg(QDateTime::currentDateTime().toString("hh:mm:ss")));
            ui->executeInternalCommand(cmd);
        }
    }, "重复执行", "watch <命令>");

    // tee
    registerCommand("tee", [ui](const QStringList& args) {
        if (args.isEmpty()) return;
        QString fn = args.last();
        QString text = args.mid(0, args.size() - 1).join(' ');
        QFile file(QDir(ui->currentDir).absoluteFilePath(fn));
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            file.write(text.toUtf8());
            file.close();
        }
        ui->appendOutput(text + "\n");
    }, "输出并保存", "tee <文件> <文本>");

    // yes
    registerCommand("yes", [ui](const QStringList& args) {
        QString text = args.isEmpty() ? "y" : args.join(' ');
        for (int i = 0; i < 10; ++i) ui->appendOutput(text + "\n");
    }, "重复输出", "yes [文本]");

    // seq
    registerCommand("seq", [ui](const QStringList& args) {
        if (args.size() == 1)
            for (int i = 1; i <= args[0].toInt(); ++i) ui->appendOutput(QString::number(i) + "\n");
        else if (args.size() == 2)
            for (int i = args[0].toInt(); i <= args[1].toInt(); ++i) ui->appendOutput(QString::number(i) + "\n");
        else if (args.size() >= 3)
            for (int i = args[0].toInt(); i <= args[2].toInt(); i += args[1].toInt())
                ui->appendOutput(QString::number(i) + "\n");
    }, "序列", "seq [起] [步] <止>");

    // shuf
    registerCommand("shuf", [ui](const QStringList& args) {
        QStringList shuffled = args;
        std::shuffle(shuffled.begin(), shuffled.end(), std::mt19937(std::random_device()()));
        for (const QString& s : shuffled) ui->appendOutput(s + "\n");
    }, "随机排列", "shuf <项目...>");

    // timeout
    registerCommand("timeout", [ui](const QStringList& args) {
        if (args.size() < 2) return;
        double secs = args[0].toDouble();
        QTimer::singleShot(static_cast<int>(secs * 1000), [ui, args]() {
            ui->executeInternalCommand(args.mid(1).join(' '));
        });
    }, "超时执行", "timeout <秒> <命令>");

    // crontab / at / batch
    registerCommand("crontab", [ui](const QStringList& args) {
        CommandHelper::runExternal("crontab", args, ui->currentDir, ui);
    }, "crontab", "crontab");

    registerCommand("at", [ui](const QStringList& args) {
        CommandHelper::runExternal("at", args, ui->currentDir, ui);
    }, "at", "at");

    registerCommand("batch", [ui](const QStringList& args) {
        CommandHelper::runExternal("batch", args, ui->currentDir, ui);
    }, "batch", "batch");
}