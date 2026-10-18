#include <QDir>
#include <QStorageInfo>
#include <QStorageInfo>
#include "commands.h"
#include "terminal/terminal.h"
#include "utils/utils.h"

#include <QFile>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QDateTime>
#include <QTextStream>
#include <QRegularExpression>
#include <QStandardPaths>
#include <algorithm>
#include <random>

void CommandRegistry::registerFileCommands(TerminalWidget* ui) {

    // cd
    registerCommand("cd", [ui](const QStringList& args) {
        QString newDir;
        if (args.isEmpty()) newDir = QDir::homePath();
        else if (args[0] == "~") newDir = QDir::homePath();
        else if (args[0] == "-") newDir = ui->previousDir.isEmpty() ? QDir::currentPath() : ui->previousDir;
        else newDir = QDir(ui->currentDir).absoluteFilePath(args[0]);
        if (QDir(newDir).exists()) {
            ui->previousDir = ui->currentDir;
            ui->currentDir = newDir;
            QDir::setCurrent(newDir);
        } else {
            ui->appendOutput(QString("目录不存在: %1\n").arg(args.isEmpty() ? "~" : args[0]), ERROR_COLOR);
        }
    }, "切换目录", "cd [目录]");

    // ls / dir
    registerCommand("ls", [ui](const QStringList& args) {
        bool showHidden = args.contains("-a") || args.contains("-la") || args.contains("-al");
        bool longFormat = args.contains("-l") || args.contains("-la") || args.contains("-al");
        QString target = ui->currentDir;
        for (const QString& a : args)
            if (!a.startsWith('-')) { target = QDir(ui->currentDir).absoluteFilePath(a); break; }
        QDir dir(target);
        QStringList items = dir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot,
                                           QDir::Name | QDir::DirsFirst | QDir::IgnoreCase);
        if (!showHidden)
            items.erase(std::remove_if(items.begin(), items.end(),
                [](const QString& s){ return s.startsWith('.'); }), items.end());
        for (const QString& item : items) {
            QFileInfo info(dir.absoluteFilePath(item));
            if (longFormat) {
                QString mtime = info.lastModified().toString("yyyy-MM-dd hh:mm");
                ui->appendOutput(QString("%1 %2 %3 ").arg(info.isDir() ? "d" : "-", 8).arg(info.size(), 8).arg(mtime));
                ui->appendOutput(QString("%1%2\n").arg(item, info.isDir() ? "/" : ""),
                                 info.isDir() ? ACCENT_COLOR : FG_COLOR);
            } else {
                ui->appendOutput(item + (info.isDir() ? "/  " : "  "),
                                 info.isDir() ? ACCENT_COLOR : FG_COLOR);
            }
        }
        if (!longFormat) ui->appendOutput("\n");
    }, "列出目录", "ls [-a] [-l]");

    registerCommand("dir", [ui](const QStringList& args) {
        ui->executeInternalCommand("ls " + args.join(' '));
    }, "列出目录(Windows)", "dir");

    // pwd
    registerCommand("pwd", [ui](const QStringList&) {
        ui->appendOutput(ui->currentDir + "\n");
    }, "当前目录", "pwd");

    // mkdir
    registerCommand("mkdir", [ui](const QStringList& args) {
        if (args.isEmpty()) { ui->appendOutput("用法: mkdir <目录>\n", ERROR_COLOR); return; }
        for (const QString& d : args) {
            if (QDir().mkpath(QDir(ui->currentDir).absoluteFilePath(d)))
                ui->appendOutput(QString("创建: %1\n").arg(d), INFO_COLOR);
            else ui->appendOutput(QString("失败: %1\n").arg(d), ERROR_COLOR);
        }
    }, "创建目录", "mkdir <目录>");

    // rmdir
    registerCommand("rmdir", [ui](const QStringList& args) {
        if (args.isEmpty()) return;
        for (const QString& d : args) {
            if (QDir().rmdir(QDir(ui->currentDir).absoluteFilePath(d)))
                ui->appendOutput(QString("删除: %1\n").arg(d), INFO_COLOR);
        }
    }, "删除空目录", "rmdir <目录>");

    // rm / del
    registerCommand("rm", [ui](const QStringList& args) {
        if (args.isEmpty()) { ui->appendOutput("用法: rm <文件>\n", ERROR_COLOR); return; }
        for (const QString& f : args) {
            QString path = QDir(ui->currentDir).absoluteFilePath(f);
            QFileInfo info(path);
            if (info.isDir()) QDir(path).removeRecursively();
            else QFile::remove(path);
            ui->appendOutput(QString("删除: %1\n").arg(f), INFO_COLOR);
        }
    }, "删除文件/目录", "rm <文件>");

    registerCommand("del", [ui](const QStringList& args) {
        ui->executeInternalCommand("rm " + args.join(' '));
    }, "删除文件(Windows)", "del <文件>");

    // touch
    registerCommand("touch", [ui](const QStringList& args) {
        if (args.isEmpty()) return;
        for (const QString& f : args) {
            QFile file(QDir(ui->currentDir).absoluteFilePath(f));
            if (file.open(QIODevice::WriteOnly | QIODevice::Append)) file.close();
            ui->appendOutput(QString("创建: %1\n").arg(f), INFO_COLOR);
        }
    }, "创建空文件", "touch <文件>");

    // cat / type
    registerCommand("cat", [ui](const QStringList& args) {
        if (args.isEmpty()) return;
        for (const QString& f : args) {
            QFile file(QDir(ui->currentDir).absoluteFilePath(f));
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                ui->appendOutput(file.readAll());
                file.close();
            } else ui->appendOutput(QString("'%1' 不存在\n").arg(f), ERROR_COLOR);
        }
    }, "查看文件", "cat <文件>");

    registerCommand("type", [ui](const QStringList& args) {
        ui->executeInternalCommand("cat " + args.join(' '));
    }, "查看文件(Windows)", "type <文件>");

    // cp / copy
    registerCommand("cp", [ui](const QStringList& args) {
        if (args.size() < 2) return;
        QString src = QDir(ui->currentDir).absoluteFilePath(args[0]);
        QString dst = QDir(ui->currentDir).absoluteFilePath(args[1]);
        if (QFileInfo(src).isDir()) Utils::copyDir(src, dst);
        else QFile::copy(src, dst);
        ui->appendOutput(QString("复制: %1 → %2\n").arg(args[0], args[1]), INFO_COLOR);
    }, "复制", "cp <源> <目标>");

    registerCommand("copy", [ui](const QStringList& args) {
        ui->executeInternalCommand("cp " + args.join(' '));
    }, "复制(Windows)", "copy <源> <目标>");

    // mv / move
    registerCommand("mv", [ui](const QStringList& args) {
        if (args.size() < 2) return;
        QFile::rename(QDir(ui->currentDir).absoluteFilePath(args[0]),
                      QDir(ui->currentDir).absoluteFilePath(args[1]));
        ui->appendOutput(QString("移动: %1 → %2\n").arg(args[0], args[1]), INFO_COLOR);
    }, "移动", "mv <源> <目标>");

    registerCommand("move", [ui](const QStringList& args) {
        ui->executeInternalCommand("mv " + args.join(' '));
    }, "移动(Windows)", "move");

    // rename
    registerCommand("rename", [ui](const QStringList& args) {
        if (args.size() != 2) return;
        QFile::rename(QDir(ui->currentDir).absoluteFilePath(args[0]),
                      QDir(ui->currentDir).absoluteFilePath(args[1]));
        ui->appendOutput(QString("重命名: %1 → %2\n").arg(args[0], args[1]), INFO_COLOR);
    }, "重命名", "rename <旧> <新>");

    // tree
    registerCommand("tree", [ui](const QStringList&) {
        std::function<void(const QString&, const QString&)> show =
            [&](const QString& dirPath, const QString& prefix) {
            QDir dir(dirPath);
            QStringList items = dir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot,
                                               QDir::Name | QDir::DirsFirst);
            items.erase(std::remove_if(items.begin(), items.end(),
                [](const QString& s){ return s.startsWith('.'); }), items.end());
            for (int i = 0; i < items.size(); ++i) {
                bool last = (i == items.size() - 1);
                ui->appendOutput(prefix + (last ? "└── " : "├── "));
                ui->appendOutput(items[i] + (QFileInfo(dir.absoluteFilePath(items[i])).isDir() ? "/\n" : "\n"),
                                 QFileInfo(dir.absoluteFilePath(items[i])).isDir() ? ACCENT_COLOR : FG_COLOR);
                if (QFileInfo(dir.absoluteFilePath(items[i])).isDir())
                    show(dir.absoluteFilePath(items[i]), prefix + (last ? "    " : "│   "));
            }
        };
        ui->appendOutput(ui->currentDir + "\n", ACCENT_COLOR);
        show(ui->currentDir, "");
    }, "树形目录", "tree");

    // find
    registerCommand("find", [ui](const QStringList& args) {
        if (args.isEmpty()) { ui->appendOutput("用法: find <关键词>\n", ERROR_COLOR); return; }
        QString pattern = args[0].toLower();
        int count = 0;
        QDirIterator it(ui->currentDir, QDir::AllEntries | QDir::NoDotAndDotDot,
                        QDirIterator::Subdirectories);
        while (it.hasNext()) {
            it.next();
            if (it.fileName().toLower().contains(pattern)) {
                ui->appendOutput(QDir(ui->currentDir).relativeFilePath(it.filePath()) + "\n");
                count++;
            }
        }
        ui->appendOutput(QString("找到 %1 个结果\n").arg(count), INFO_COLOR);
    }, "查找文件", "find <关键词>");

    // grep
    registerCommand("grep", [ui](const QStringList& args) {
        if (args.size() < 2) { ui->appendOutput("用法: grep <关键词> <文件>\n", ERROR_COLOR); return; }
        QFile file(QDir(ui->currentDir).absoluteFilePath(args[1]));
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            int lineNum = 0;
            while (!in.atEnd()) {
                QString line = in.readLine();
                lineNum++;
                if (line.contains(args[0])) {
                    ui->appendOutput(QString("%1: ").arg(lineNum), INFO_COLOR);
                    ui->appendOutput(line + "\n");
                }
            }
            file.close();
        }
    }, "搜索文本", "grep <关键词> <文件>");

    // wc
    registerCommand("wc", [ui](const QStringList& args) {
        if (args.isEmpty()) return;
        QFile file(QDir(ui->currentDir).absoluteFilePath(args[0]));
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString content = file.readAll();
            file.close();
            int lines = content.count('\n');
            int words = content.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts).size();
            ui->appendOutput(QString("行:%1 词:%2 字符:%3 %4\n")
                .arg(lines).arg(words).arg(content.size()).arg(args[0]));
        }
    }, "统计", "wc <文件>");

    // head
    registerCommand("head", [ui](const QStringList& args) {
        int n = 10; QString fn;
        for (int i = 0; i < args.size(); ++i) {
            if (args[i] == "-n" && i + 1 < args.size()) n = args[++i].toInt();
            else fn = args[i];
        }
        if (fn.isEmpty()) return;
        QFile file(QDir(ui->currentDir).absoluteFilePath(fn));
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            for (int i = 0; i < n && !in.atEnd(); ++i)
                ui->appendOutput(in.readLine() + "\n");
            file.close();
        }
    }, "前N行", "head [-n N] <文件>");

    // tail
    registerCommand("tail", [ui](const QStringList& args) {
        int n = 10; QString fn;
        for (int i = 0; i < args.size(); ++i) {
            if (args[i] == "-n" && i + 1 < args.size()) n = args[++i].toInt();
            else fn = args[i];
        }
        if (fn.isEmpty()) return;
        QFile file(QDir(ui->currentDir).absoluteFilePath(fn));
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QStringList lines = QString(file.readAll()).split('\n');
            file.close();
            for (int i = qMax(0, lines.size() - n); i < lines.size(); ++i)
                ui->appendOutput(lines[i] + "\n");
        }
    }, "后N行", "tail [-n N] <文件>");

    // more / less
    registerCommand("more", [ui](const QStringList& args) {
        if (args.isEmpty()) return;
        QFile file(QDir(ui->currentDir).absoluteFilePath(args[0]));
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            ui->appendOutput(file.readAll());
            file.close();
        }
    }, "查看文件", "more <文件>");

    registerCommand("less", [ui](const QStringList& args) {
        ui->executeInternalCommand("more " + args.join(' '));
    }, "查看文件", "less <文件>");

    // diff
    registerCommand("diff", [ui](const QStringList& args) {
        if (args.size() != 2) return;
        QFile f1(QDir(ui->currentDir).absoluteFilePath(args[0]));
        QFile f2(QDir(ui->currentDir).absoluteFilePath(args[1]));
        if (f1.open(QIODevice::ReadOnly) && f2.open(QIODevice::ReadOnly)) {
            QStringList l1 = QString(f1.readAll()).split('\n');
            QStringList l2 = QString(f2.readAll()).split('\n');
            for (int i = 0; i < qMax(l1.size(), l2.size()); ++i) {
                QString a = i < l1.size() ? l1[i] : "";
                QString b = i < l2.size() ? l2[i] : "";
                if (a != b) {
                    if (i < l1.size()) ui->appendOutput("< " + a + "\n", ERROR_COLOR);
                    if (i < l2.size()) ui->appendOutput("> " + b + "\n", PROMPT_COLOR);
                }
            }
        }
    }, "对比文件", "diff <文件1> <文件2>");

    // sort
    registerCommand("sort", [ui](const QStringList& args) {
        if (args.isEmpty()) return;
        QFile file(QDir(ui->currentDir).absoluteFilePath(args.last()));
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QStringList lines = QString(file.readAll()).split('\n', Qt::SkipEmptyParts);
            file.close();
            lines.sort();
            if (args.contains("-r")) std::reverse(lines.begin(), lines.end());
            for (const QString& l : lines) ui->appendOutput(l + "\n");
        }
    }, "排序", "sort [-r] <文件>");

    // uniq
    registerCommand("uniq", [ui](const QStringList& args) {
        if (args.isEmpty()) return;
        QFile file(QDir(ui->currentDir).absoluteFilePath(args[0]));
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QStringList lines = QString(file.readAll()).split('\n', Qt::SkipEmptyParts);
            file.close();
            QSet<QString> seen;
            for (const QString& l : lines)
                if (!seen.contains(l)) { ui->appendOutput(l + "\n"); seen.insert(l); }
        }
    }, "去重", "uniq <文件>");

    // cut
    registerCommand("cut", [ui](const QStringList& args) {
        QString delim = ","; QList<int> fields; QString fn;
        for (int i = 0; i < args.size(); ++i) {
            if (args[i] == "-d" && i + 1 < args.size()) delim = args[++i];
            else if (args[i] == "-f" && i + 1 < args.size())
                for (const QString& f : args[++i].split(',')) fields.append(f.toInt() - 1);
            else fn = args[i];
        }
        if (fn.isEmpty()) return;
        QFile file(QDir(ui->currentDir).absoluteFilePath(fn));
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            while (!in.atEnd()) {
                QStringList parts = in.readLine().split(delim);
                QStringList sel;
                for (int f : fields) if (f < parts.size()) sel.append(parts[f]);
                ui->appendOutput(sel.join(delim) + "\n");
            }
            file.close();
        }
    }, "剪切字段", "cut -d<分隔符> -f<字段> <文件>");

    // stat
    registerCommand("stat", [ui](const QStringList& args) {
        if (args.isEmpty()) return;
        QFileInfo info(QDir(ui->currentDir).absoluteFilePath(args[0]));
        if (info.exists())
            ui->appendOutput(QString("大小:%1 修改:%2\n")
                .arg(info.size()).arg(info.lastModified().toString("yyyy-MM-dd hh:mm")));
    }, "文件状态", "stat <文件>");

    // file
    registerCommand("file", [ui](const QStringList& args) {
        if (args.isEmpty()) return;
        QFileInfo info(QDir(ui->currentDir).absoluteFilePath(args[0]));
        if (info.isDir()) ui->appendOutput(args[0] + ": directory\n");
        else if (info.isFile()) ui->appendOutput(QString("%1: file, %2 bytes\n").arg(args[0]).arg(info.size()));
    }, "文件类型", "file <文件>");

    // du
    registerCommand("du", [ui](const QStringList& args) {
        QString target = args.isEmpty() ? ui->currentDir : QDir(ui->currentDir).absoluteFilePath(args[0]);
        qint64 total = 0;
        QDirIterator it(target, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) { it.next(); total += it.fileInfo().size(); }
        ui->appendOutput(Utils::formatSize(total) + "\t" + target + "\n");
    }, "磁盘使用", "du [目录]");

    // df
    registerCommand("df", [ui](const QStringList&) {
        QStorageInfo storage(ui->currentDir);
        if (storage.isValid())
            ui->appendOutput(QString("总量:%1 可用:%2\n")
                .arg(Utils::formatSize(storage.bytesTotal()),
                     Utils::formatSize(storage.bytesAvailable())));
    }, "磁盘空间", "df");

    // chmod
    registerCommand("chmod", [ui](const QStringList& args) {
        if (args.size() < 2) return;
        bool ok; int mode = args[0].toInt(&ok, 8);
        if (ok) QFile::setPermissions(QDir(ui->currentDir).absoluteFilePath(args[1]), QFile::Permission(mode));
    }, "权限", "chmod <权限> <文件>");

    // which
    registerCommand("which", [ui](const QStringList& args) {
        if (args.isEmpty()) return;
        if (true) {
            ui->appendOutput(QString("%1: 内置命令\n").arg(args[0]), INFO_COLOR);
            return;
        }
        QString path = QStandardPaths::findExecutable(args[0]);
        ui->appendOutput(path.isEmpty() ? QString("未找到: %1\n").arg(args[0]) : path + "\n");
    }, "查找命令", "which <命令>");
}