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
#include <QStorageInfo>
#include <algorithm>
#include <functional>

void CommandRegistry::registerFileCommands(TerminalWidget* ui) {

    registerCommand("cd", [ui](const QStringList& args) {
        QString newDir;
        if (args.isEmpty()) newDir = QDir::homePath();
        else if (args[0] == "~") newDir = QDir::homePath();
        else if (args[0] == "-") newDir = ui->previousDir.isEmpty() ? QDir::currentPath() : ui->previousDir;
        else newDir = QDir::cleanPath(QDir(ui->currentDir).absoluteFilePath(args[0]));
        if (QDir(newDir).exists()) {
            ui->previousDir = ui->currentDir;
            ui->currentDir = newDir;
            QDir::setCurrent(newDir);
        } else {
            ui->appendOutput(QString("目录不存在: %1\n").arg(args[0]), ERROR_COLOR);
        }
    }, "切换目录", "cd [目录]");

    registerCommand("ls", [ui](const QStringList& args) {
        bool showHidden = args.contains("-a") || args.contains("-la") || args.contains("-al");
        bool longFormat = args.contains("-l") || args.contains("-la") || args.contains("-al");
        QString target = ui->currentDir;
        for (const QString& a : args)
            if (!a.startsWith('-')) { target = QDir(ui->currentDir).absoluteFilePath(a); break; }
        QDir dir(target);
        QDir::Filters filters = QDir::AllEntries | QDir::NoDotAndDotDot;
        if (showHidden) filters |= QDir::Hidden;
        QStringList items = dir.entryList(filters, QDir::Name | QDir::DirsFirst | QDir::IgnoreCase);
        for (const QString& item : items) {
            QFileInfo info(dir.absoluteFilePath(item));
            QColor itemColor = FG_COLOR;
            if (info.isDir()) itemColor = ACCENT_COLOR;
            else if (info.isExecutable()) itemColor = PROMPT_COLOR;
            else if (info.isSymLink()) itemColor = QColor("#00ffff");
            else if (item.startsWith('.')) itemColor = INFO_COLOR;

            if (longFormat) {
                QString mtime = info.lastModified().toString("yyyy-MM-dd hh:mm");
                QString perm = info.isDir() ? "d" : (info.isSymLink() ? "l" : "-");
                ui->appendOutput(QString("%1 %2 %3 ").arg(perm)
                    .arg(info.size(), 8).arg(mtime));
                ui->appendOutput(QString("%1%2\n").arg(item, info.isDir() ? "/" : ""), itemColor);
            } else {
                ui->appendOutput(item + (info.isDir() ? "/  " : "  "), itemColor);
            }
        }
        if (!longFormat) ui->appendOutput("\n");
    }, "列出目录", "ls [-a] [-l]");

    registerCommand("dir", [ui](const QStringList& args) {
        ui->executeInternalCommand("ls " + args.join(' '));
    }, "列出目录", "dir");

    registerCommand("pwd", [ui](const QStringList&) {
        ui->appendOutput(ui->currentDir + "\n");
    }, "当前目录", "pwd");

    registerCommand("mkdir", [ui](const QStringList& args) {
        if (args.isEmpty()) return;
        for (const QString& d : args) {
            if (QDir().mkpath(QDir(ui->currentDir).absoluteFilePath(d)))
                ui->appendOutput(QString("创建: %1\n").arg(d), INFO_COLOR);
            else ui->appendOutput(QString("失败: %1\n").arg(d), ERROR_COLOR);
        }
    }, "创建目录", "mkdir <目录>");

    registerCommand("rmdir", [ui](const QStringList& args) {
        for (const QString& d : args) {
            if (QDir().rmdir(QDir(ui->currentDir).absoluteFilePath(d)))
                ui->appendOutput(QString("删除: %1\n").arg(d), INFO_COLOR);
            else ui->appendOutput(QString("删除失败(目录非空?): %1\n").arg(d), ERROR_COLOR);
        }
    }, "删除空目录", "rmdir <目录>");

    registerCommand("rm", [ui](const QStringList& args) {
        if (args.isEmpty()) { ui->appendOutput("用法: rm [-r] <文件>\n", ERROR_COLOR); return; }
        QStringList files;
        bool recursive = false;
        for (const QString& a : args) {
            if (a == "-r" || a == "-rf" || a == "-fr") recursive = true;
            else if (a == "-f") continue;
            else files.append(a);
        }
        for (const QString& f : files) {
            QString path = QDir::cleanPath(QDir(ui->currentDir).absoluteFilePath(f));
            QStringList dangerous = {"/", "/sdcard", "/storage", "/data", "/system"};
            if (dangerous.contains(path)) {
                ui->appendOutput(QString("拒绝删除: %1\n").arg(path), ERROR_COLOR);
                continue;
            }
            QFileInfo info(path);
            if (info.isDir()) {
                if (recursive) {
                    QDir(path).removeRecursively();
                    ui->appendOutput(QString("删除: %1\n").arg(f), INFO_COLOR);
                } else {
                    ui->appendOutput(QString("%1 是目录，需要 -r\n").arg(f), ERROR_COLOR);
                }
            } else {
                if (QFile::remove(path)) ui->appendOutput(QString("删除: %1\n").arg(f), INFO_COLOR);
            }
        }
    }, "删除文件", "rm [-r] <文件>");

    registerCommand("del", [ui](const QStringList& args) {
        ui->executeInternalCommand("rm " + args.join(' '));
    }, "删除", "del");

    registerCommand("touch", [ui](const QStringList& args) {
        for (const QString& f : args) {
            QFile file(QDir(ui->currentDir).absoluteFilePath(f));
            if (file.open(QIODevice::WriteOnly | QIODevice::Append)) file.close();
        }
    }, "创建空文件", "touch <文件>");

    registerCommand("cat", [ui](const QStringList& args) {
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
    }, "查看文件", "type");

    registerCommand("cp", [ui](const QStringList& args) {
        if (args.size() < 2) return;
        QString src = QDir(ui->currentDir).absoluteFilePath(args[0]);
        QString dst = QDir(ui->currentDir).absoluteFilePath(args[1]);
        if (QFileInfo(src).isDir()) Utils::copyDir(src, dst);
        else QFile::copy(src, dst);
    }, "复制", "cp");

    registerCommand("copy", [ui](const QStringList& args) {
        ui->executeInternalCommand("cp " + args.join(' '));
    }, "复制", "copy");

    registerCommand("mv", [ui](const QStringList& args) {
        if (args.size() < 2) return;
        QFile::rename(QDir(ui->currentDir).absoluteFilePath(args[0]),
                      QDir(ui->currentDir).absoluteFilePath(args[1]));
    }, "移动", "mv");

    registerCommand("move", [ui](const QStringList& args) {
        ui->executeInternalCommand("mv " + args.join(' '));
    }, "移动", "move");

    registerCommand("rename", [ui](const QStringList& args) {
        if (args.size() != 2) return;
        QFile::rename(QDir(ui->currentDir).absoluteFilePath(args[0]),
                      QDir(ui->currentDir).absoluteFilePath(args[1]));
    }, "重命名", "rename");

    registerCommand("tree", [ui](const QStringList&) {
        std::function<void(const QString&, const QString&, int)> show =
            [&](const QString& dirPath, const QString& prefix, int depth) {
            if (depth > 3) return;
            QDir dir(dirPath);
            QStringList items = dir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot,
                                               QDir::Name | QDir::DirsFirst);
            items.erase(std::remove_if(items.begin(), items.end(),
                [](const QString& s){ return s.startsWith('.'); }), items.end());
            for (int i = 0; i < items.size(); ++i) {
                bool last = (i == items.size() - 1);
                bool isDir = QFileInfo(dir.absoluteFilePath(items[i])).isDir();
                ui->appendOutput(prefix + (last ? "└── " : "├── "));
                ui->appendOutput(items[i] + (isDir ? "/\n" : "\n"),
                                 isDir ? ACCENT_COLOR : FG_COLOR);
                if (isDir) show(dir.absoluteFilePath(items[i]), prefix + (last ? "    " : "│   "), depth + 1);
            }
        };
        ui->appendOutput(ui->currentDir + "\n", ACCENT_COLOR);
        show(ui->currentDir, "", 0);
    }, "树形目录", "tree");

    registerCommand("find", [ui](const QStringList& args) {
        if (args.isEmpty()) { ui->appendOutput("用法: find <关键词>\n", ERROR_COLOR); return; }
        QString pattern = args[0].toLower();
        int maxDepth = 3;
        int count = 0;
        QDirIterator it(ui->currentDir, QDir::AllEntries | QDir::NoDotAndDotDot,
                        QDirIterator::Subdirectories);
        while (it.hasNext() && count < 100) {
            it.next();
            if (it.fileName().toLower().contains(pattern)) {
                ui->appendOutput(QDir(ui->currentDir).relativeFilePath(it.filePath()) + "\n");
                count++;
            }
        }
        ui->appendOutput(QString("找到 %1 个结果\n").arg(count), INFO_COLOR);
    }, "查找文件", "find <关键词>");

    registerCommand("grep", [ui](const QStringList& args) {
        if (args.size() < 2) return;
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
    }, "搜索文本", "grep");

    registerCommand("wc", [ui](const QStringList& args) {
        if (args.isEmpty()) return;
        QFile file(QDir(ui->currentDir).absoluteFilePath(args[0]));
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString content = file.readAll();
            file.close();
            int lines = content.count('\n');
            int words = content.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts).size();
            ui->appendOutput(QString("行:%1 词:%2 字符:%3\n").arg(lines).arg(words).arg(content.size()));
        }
    }, "统计", "wc");

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
            for (int i = 0; i < n && !in.atEnd(); ++i) ui->appendOutput(in.readLine() + "\n");
            file.close();
        }
    }, "前N行", "head");

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
    }, "后N行", "tail");

    registerCommand("more", [ui](const QStringList& args) {
        if (args.isEmpty()) return;
        QFile file(QDir(ui->currentDir).absoluteFilePath(args[0]));
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            ui->appendOutput(file.readAll());
            file.close();
        }
    }, "分页查看", "more");

    registerCommand("less", [ui](const QStringList& args) {
        ui->executeInternalCommand("more " + args.join(' '));
    }, "分页查看", "less");

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
    }, "对比", "diff");

    registerCommand("sort", [ui](const QStringList& args) {
        if (args.isEmpty()) return;
        QFile file(QDir(ui->currentDir).absoluteFilePath(args.last()));
        if (file.open(QIODevice::ReadOnly)) {
            QStringList lines = QString(file.readAll()).split('\n', Qt::SkipEmptyParts);
            file.close();
            lines.sort();
            if (args.contains("-r")) std::reverse(lines.begin(), lines.end());
            for (const QString& l : lines) ui->appendOutput(l + "\n");
        }
    }, "排序", "sort");

    registerCommand("uniq", [ui](const QStringList& args) {
        if (args.isEmpty()) return;
        QFile file(QDir(ui->currentDir).absoluteFilePath(args[0]));
        if (file.open(QIODevice::ReadOnly)) {
            QStringList lines = QString(file.readAll()).split('\n', Qt::SkipEmptyParts);
            file.close();
            QSet<QString> seen;
            for (const QString& l : lines)
                if (!seen.contains(l)) { ui->appendOutput(l + "\n"); seen.insert(l); }
        }
    }, "去重", "uniq");

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
    }, "剪切", "cut");

    registerCommand("stat", [ui](const QStringList& args) {
        if (args.isEmpty()) return;
        QFileInfo info(QDir(ui->currentDir).absoluteFilePath(args[0]));
        if (info.exists())
            ui->appendOutput(QString("大小:%1 修改:%2\n").arg(info.size())
                .arg(info.lastModified().toString("yyyy-MM-dd hh:mm")));
    }, "状态", "stat");

    registerCommand("file", [ui](const QStringList& args) {
        if (args.isEmpty()) return;
        QFileInfo info(QDir(ui->currentDir).absoluteFilePath(args[0]));
        if (info.isDir()) ui->appendOutput(args[0] + ": directory\n");
        else if (info.isFile()) ui->appendOutput(QString("%1: file, %2 bytes\n").arg(args[0]).arg(info.size()));
    }, "类型", "file");

    registerCommand("du", [ui](const QStringList& args) {
        QString target = args.isEmpty() ? ui->currentDir : QDir(ui->currentDir).absoluteFilePath(args[0]);
        qint64 total = 0;
        QDirIterator it(target, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) { it.next(); total += it.fileInfo().size(); }
        ui->appendOutput(Utils::formatSize(total) + "\t" + target + "\n");
    }, "磁盘使用", "du");

    registerCommand("df", [ui](const QStringList& args) {
        QString target = ui->currentDir;
        if (!args.isEmpty()) target = QDir(ui->currentDir).absoluteFilePath(args[0]);
        QStorageInfo storage(target);
        if (storage.isValid())
            ui->appendOutput(QString("总量:%1 可用:%2\n")
                .arg(Utils::formatSize(storage.bytesTotal()), Utils::formatSize(storage.bytesAvailable())));
    }, "磁盘空间", "df");

    registerCommand("chmod", [ui](const QStringList& args) {
        if (args.size() < 2) return;
        bool ok; int mode = args[0].toInt(&ok, 8);
        if (!ok) return;
        QFile::Permissions perms;
        if (mode & 0400) perms |= QFileDevice::ReadOwner;
        if (mode & 0200) perms |= QFileDevice::WriteOwner;
        if (mode & 0100) perms |= QFileDevice::ExeOwner;
        if (mode & 0040) perms |= QFileDevice::ReadGroup;
        if (mode & 0020) perms |= QFileDevice::WriteGroup;
        if (mode & 0010) perms |= QFileDevice::ExeGroup;
        if (mode & 0004) perms |= QFileDevice::ReadOther;
        if (mode & 0002) perms |= QFileDevice::WriteOther;
        if (mode & 0001) perms |= QFileDevice::ExeOther;
        QFile::setPermissions(QDir(ui->currentDir).absoluteFilePath(args[1]), perms);
    }, "权限", "chmod");

    registerCommand("which", [ui](const QStringList& args) {
        if (args.isEmpty()) return;
        QString path = QStandardPaths::findExecutable(args[0]);
        ui->appendOutput(path.isEmpty() ? QString("未找到: %1\n").arg(args[0]) : path + "\n");
    }, "查找命令", "which");
}