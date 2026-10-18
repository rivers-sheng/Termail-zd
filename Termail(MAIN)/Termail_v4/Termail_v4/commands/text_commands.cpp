#include <QThread>
#include "commands.h"
#include "ui/terminal_ui.h"
#include "utils/utils.h"

#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QCryptographicHash>
#include <QUuid>
#include <QRandomGenerator>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <algorithm>
#include <cmath>
#include <random>

// ============ 文本处理命令注册 ============
void CommandRegistry::registerTextCommands(TerminalUI* ui) {

    // tr
    registerCommand("tr", [ui](const QStringList& args) {
        if (args.size() < 2) return;
        QString text = args.mid(2).join(' ');
        if (text.isEmpty()) text = args.join(' ');
        QString result;
        for (const QChar& ch : text) {
            int idx = args[0].indexOf(ch);
            if (idx >= 0 && idx < args[1].size()) {
                result += args[1][idx];
            } else {
                result += ch;
            }
        }
        ui->appendOutput(result + "\n");
    }, "字符替换", "tr <集合1> <集合2> [文本]");

    // rev
    registerCommand("rev", [ui](const QStringList& args) {
        QString text = args.join(' ');
        std::reverse(text.begin(), text.end());
        ui->appendOutput(text + "\n");
    }, "反转文本", "rev <文本>");

    // tac
    registerCommand("tac", [ui](const QStringList& args) {
        if (args.isEmpty()) return;
        QString path = QDir(ui->currentDir).absoluteFilePath(args[0]);
        QFile file(path);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QStringList lines = QString(file.readAll()).split('\n');
            file.close();
            std::reverse(lines.begin(), lines.end());
            for (const QString& line : lines) {
                if (!line.isEmpty()) ui->appendOutput(line + "\n");
            }
        }
    }, "反向显示文件", "tac <文件>");

    // nl
    registerCommand("nl", [ui](const QStringList& args) {
        if (args.isEmpty()) return;
        QString path = QDir(ui->currentDir).absoluteFilePath(args[0]);
        QFile file(path);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            int lineNum = 1;
            while (!in.atEnd()) {
                ui->appendOutput(QString("%1\t%2\n").arg(lineNum++, 6).arg(in.readLine()));
            }
            file.close();
        }
    }, "带行号显示", "nl <文件>");

    // paste
    registerCommand("paste", [ui](const QStringList& args) {
        if (args.size() < 2) {
            ui->appendOutput("用法: paste <文件1> <文件2>\n", THEME.red.name());
            return;
        }
        QString path1 = QDir(ui->currentDir).absoluteFilePath(args[0]);
        QString path2 = QDir(ui->currentDir).absoluteFilePath(args[1]);
        QFile f1(path1), f2(path2);
        if (f1.open(QIODevice::ReadOnly) && f2.open(QIODevice::ReadOnly)) {
            QStringList l1 = QString(f1.readAll()).split('\n');
            QStringList l2 = QString(f2.readAll()).split('\n');
            int maxLen = qMax(l1.size(), l2.size());
            for (int i = 0; i < maxLen; ++i) {
                QString a = i < l1.size() ? l1[i] : "";
                QString b = i < l2.size() ? l2[i] : "";
                ui->appendOutput(a + "\t" + b + "\n");
            }
        }
    }, "合并文件行", "paste <文件1> <文件2>");

    // split
    registerCommand("split", [ui](const QStringList& args) {
        if (args.isEmpty()) return;
        int n = 1000;
        int argIdx = 0;
        if (args[0] == "-l" && args.size() > 1) {
            n = args[1].toInt();
            argIdx = 2;
        }
        if (argIdx >= args.size()) return;
        QString path = QDir(ui->currentDir).absoluteFilePath(args[argIdx]);
        QFile file(path);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QStringList lines = QString(file.readAll()).split('\n');
            file.close();
            for (int i = 0; i < lines.size(); i += n) {
                QString partName = QString("%1.part%2").arg(args[argIdx]).arg(i / n + 1);
                QString partPath = QDir(ui->currentDir).absoluteFilePath(partName);
                QFile out(partPath);
                if (out.open(QIODevice::WriteOnly)) {
                    for (int j = i; j < qMin(i + n, lines.size()); ++j) {
                        out.write((lines[j] + "\n").toUtf8());
                    }
                    out.close();
                    ui->appendOutput(QString("创建: %1\n").arg(partName), THEME.green.name());
                }
            }
        }
    }, "分割文件", "split [-l N] <文件>");

    // join
    registerCommand("join", [ui](const QStringList& args) {
        if (args.isEmpty()) return;
        for (const QString& fn : args) {
            QString path = QDir(ui->currentDir).absoluteFilePath(fn);
            QFile file(path);
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                ui->appendOutput(file.readAll());
                file.close();
            }
        }
    }, "连接文件", "join <文件...>");

    // fmt
    registerCommand("fmt", [ui](const QStringList& args) {
        if (args.isEmpty()) return;
        QString path = QDir(ui->currentDir).absoluteFilePath(args[0]);
        QFile file(path);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString text = file.readAll();
            file.close();
            QStringList words = text.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
            QString line;
            int width = 72;
            for (const QString& word : words) {
                if (line.size() + word.size() + 1 > width) {
                    ui->appendOutput(line.trimmed() + "\n");
                    line.clear();
                }
                line += word + " ";
            }
            if (!line.isEmpty()) ui->appendOutput(line.trimmed() + "\n");
        }
    }, "格式化文本", "fmt <文件>");

    // sed
    registerCommand("sed", [ui](const QStringList& args) {
        if (args.size() < 2) return;
        QString pattern = args[0];
        QString path = QDir(ui->currentDir).absoluteFilePath(args.last());
        if (pattern.startsWith("s/") && pattern.endsWith("/")) {
            QString inner = pattern.mid(2, pattern.size() - 3);
            int sepIdx = inner.indexOf('/');
            if (sepIdx > 0) {
                QString from = inner.left(sepIdx);
                QString to = inner.mid(sepIdx + 1);
                QFile file(path);
                if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    QString content = file.readAll();
                    file.close();
                    content.replace(QRegularExpression(from), to);
                    ui->appendOutput(content);
                }
            }
        }
    }, "流编辑器", "sed 's/模式/替换/' <文件>");

    // zip
    registerCommand("zip", [ui](const QStringList& args) {
        if (args.size() < 2) {
            ui->appendOutput("用法: zip <压缩包> <文件...>\n", THEME.red.name());
            return;
        }
        // 使用系统zip命令
        CommandHelper::runExternal("zip", args, ui->currentDir, ui);
    }, "压缩文件", "zip <压缩包> <文件...>");

    // unzip
    registerCommand("unzip", [ui](const QStringList& args) {
        if (args.isEmpty()) {
            ui->appendOutput("用法: unzip <压缩包>\n", THEME.red.name());
            return;
        }
        CommandHelper::runExternal("unzip", args, ui->currentDir, ui);
    }, "解压文件", "unzip <压缩包>");

    // tar
    registerCommand("tar", [ui](const QStringList& args) {
        CommandHelper::runExternal("tar", args, ui->currentDir, ui);
    }, "归档工具", "tar [选项] <文件>");

    // hash
    registerCommand("hash", [ui](const QStringList& args) {
        if (args.isEmpty()) return;
        QString text = args.join(' ');
        QByteArray data = text.toUtf8();
        ui->appendOutput(QString("MD5: %1\n").arg(QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex()));
        ui->appendOutput(QString("SHA256: %1\n").arg(QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex()));
        ui->appendOutput(QString("SHA512: %1\n").arg(QCryptographicHash::hash(data, QCryptographicHash::Sha512).toHex()));
    }, "计算哈希", "hash <文本>");

    // base64
    registerCommand("base64", [ui](const QStringList& args) {
        if (args.isEmpty()) return;
        QString text = args.join(' ');
        ui->appendOutput(text.toUtf8().toBase64() + "\n");
    }, "Base64编码", "base64 <文本>");

    // uuid
    registerCommand("uuid", [ui](const QStringList&) {
        ui->appendOutput(QUuid::createUuid().toString(QUuid::WithoutBraces) + "\n");
    }, "生成UUID", "uuid");

    // random
    registerCommand("random", [ui](const QStringList& args) {
        if (args.isEmpty()) {
            ui->appendOutput(QString::number(QRandomGenerator::global()->generateDouble()) + "\n");
        } else if (args.size() == 1) {
            int max = args[0].toInt();
            ui->appendOutput(QString::number(QRandomGenerator::global()->bounded(max + 1)) + "\n");
        } else {
            int min = args[0].toInt();
            int max = args[1].toInt();
            ui->appendOutput(QString::number(QRandomGenerator::global()->bounded(min, max + 1)) + "\n");
        }
    }, "随机数", "random [最小值] [最大值]");

    // sleep
    registerCommand("sleep", [ui](const QStringList& args) {
        if (!args.isEmpty()) {
            QThread::msleep(static_cast<unsigned long>(args[0].toDouble() * 1000));
        }
    }, "延时", "sleep <秒>");

    // watch
    registerCommand("watch", [ui](const QStringList& args) {
        if (args.isEmpty()) return;
        QString cmd = args.join(' ');
        QTimer* timer = new QTimer();
        int* count = new int(0);
        QObject::connect(timer, &QTimer::timeout, [ui, cmd, count, timer]() {
            if (*count >= 3) {
                timer->stop();
                timer->deleteLater();
                delete count;
                return;
            }
            (*count)++;
            ui->appendOutput(QString("\n--- %1 ---\n").arg(QDateTime::currentDateTime().toString("hh:mm:ss")));
            ui->executeInternalCommand(cmd);
        });
        timer->start(2000);
    }, "重复执行命令", "watch <命令>");

    // tee
    registerCommand("tee", [ui](const QStringList& args) {
        if (args.isEmpty()) return;
        QString fn = args.last();
        QString text = args.mid(0, args.size() - 1).join(' ');
        QString path = QDir(ui->currentDir).absoluteFilePath(fn);
        QFile file(path);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            file.write(text.toUtf8());
            file.close();
        }
        ui->appendOutput(text + "\n");
    }, "输出并保存", "tee <文件> <文本>");

    // yes
    registerCommand("yes", [ui](const QStringList& args) {
        QString text = args.isEmpty() ? "y" : args.join(' ');
        for (int i = 0; i < 10; ++i) {
            ui->appendOutput(text + "\n");
        }
    }, "重复输出", "yes [文本]");

    // seq
    registerCommand("seq", [ui](const QStringList& args) {
        if (args.isEmpty()) return;
        if (args.size() == 1) {
            int end = args[0].toInt();
            for (int i = 1; i <= end; ++i) {
                ui->appendOutput(QString::number(i) + "\n");
            }
        } else if (args.size() == 2) {
            int start = args[0].toInt();
            int end = args[1].toInt();
            for (int i = start; i <= end; ++i) {
                ui->appendOutput(QString::number(i) + "\n");
            }
        } else {
            int start = args[0].toInt();
            int step = args[1].toInt();
            int end = args[2].toInt();
            for (int i = start; i <= end; i += step) {
                ui->appendOutput(QString::number(i) + "\n");
            }
        }
    }, "序列生成", "seq [起始] [步长] <终止>");

    // shuf
    registerCommand("shuf", [ui](const QStringList& args) {
        QStringList shuffled = args;
        std::shuffle(shuffled.begin(), shuffled.end(), std::mt19937(std::random_device()()));
        for (const QString& item : shuffled) {
            ui->appendOutput(item + "\n");
        }
    }, "随机排列", "shuf <项目...>");

    // timeout
    registerCommand("timeout", [ui](const QStringList& args) {
        if (args.size() < 2) return;
        double secs = args[0].toDouble();
        QTimer::singleShot(static_cast<int>(secs * 1000), [ui, args]() {
            ui->executeInternalCommand(args.mid(1).join(' '));
        });
    }, "超时执行", "timeout <秒> <命令>");

    // crontab
    registerCommand("crontab", [ui](const QStringList& args) {
        CommandHelper::runExternal("crontab", args, ui->currentDir, ui);
    }, "定时任务", "crontab");

    // at
    registerCommand("at", [ui](const QStringList& args) {
        CommandHelper::runExternal("at", args, ui->currentDir, ui);
    }, "定时执行", "at");

    // batch
    registerCommand("batch", [ui](const QStringList& args) {
        CommandHelper::runExternal("batch", args, ui->currentDir, ui);
    }, "批处理", "batch");
}