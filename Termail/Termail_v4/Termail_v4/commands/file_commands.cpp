#include "commands.h"
#include "ui/terminal_ui.h"

#include <QFile>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QDateTime>
#include <QTextStream>
#include <QRegularExpression>
#include <QStandardPaths>
#include <algorithm>

// ============ 文件命令注册 ============
void CommandRegistry::registerFileCommands(TerminalUI* ui) {
    // cd
    registerCommand("cd", [ui](const QStringList& args) {
        QString newDir;
        if (args.isEmpty()) {
            newDir = QDir::homePath();
        } else if (args[0] == "~") {
            newDir = QDir::homePath();
        } else if (args[0] == "-") {
            newDir = ui->previousDir.isEmpty() ? QDir::currentPath() : ui->previousDir;
        } else {
            newDir = QDir(ui->currentDir).absoluteFilePath(args[0]);
        }
        if (QDir(newDir).exists()) {
            ui->previousDir = ui->currentDir;
            ui->currentDir = newDir;
            QDir::setCurrent(ui->currentDir);
        } else {
            ui->appendOutput(QString("目录不存在: %1\n").arg(args.isEmpty() ? "~" : args[0]),
                             THEME.red.name());
        }
    }, "切换目录", "cd [目录]");

    // ls / dir
    registerCommand("ls", [ui](const QStringList& args) {
        bool showHidden = args.contains("-a") || args.contains("-la") || args.contains("-al");
        bool longFormat = args.contains("-l") || args.contains("-la") || args.contains("-al");
        QString targetDir = ui->currentDir;
        for (const QString& arg : args) {
            if (!arg.startsWith('-')) {
                targetDir = QDir(ui->currentDir).absoluteFilePath(arg);
                break;
            }
        }
        QDir dir(targetDir);
        QStringList items = dir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot,
                                           QDir::Name | QDir::DirsFirst | QDir::IgnoreCase);
        if (!showHidden) {
            items.erase(std::remove_if(items.begin(), items.end(),
                [](const QString& s) { return s.startsWith('.'); }), items.end());
        }
        for (int i = 0; i < items.size(); ++i) {
            QFileInfo info(dir.absoluteFilePath(items[i]));
            if (longFormat) {
                QString type = info.isDir() ? "d" : "-";
                QString mtime = info.lastModified().toString("yyyy-MM-dd hh:mm");
                QString color = info.isDir() ? THEME.blue.name() : THEME.fg.name();
                ui->appendOutput(QString("%1 %2 %3 ").arg(type, 8).arg(info.size()).arg(mtime));
                ui->appendOutput(QString("%1%2\n").arg(items[i], info.isDir() ? "/" : ""), color);
            } else {
                QString suffix = info.isDir() ? "/  " : "  ";
                QString color = info.isDir() ? THEME.blue.name() : THEME.fg.name();
                ui->appendOutput(items[i] + suffix, color);
                if ((i + 1) % 5 == 0) ui->appendOutput("\n");
            }
        }
        if (!longFormat) ui->appendOutput("\n");
    }, "列出目录内容", "ls [-a] [-l] [目录]");

    // 同时注册 dir
    registerCommand("dir", [ui](const QStringList& args) {
        ui->executeInternalCommand("ls " + args.join(' '));
    }, "列出目录内容", "dir");

    // pwd
    registerCommand("pwd", [ui](const QStringList&) {
        ui->appendOutput(ui->currentDir + "\n");
    }, "显示当前目录", "pwd");

    // mkdir
    registerCommand("mkdir", [ui](const QStringList& args) {
        if (args.isEmpty()) {
            ui->appendOutput("用法: mkdir <目录名>\n", THEME.red.name());
            return;
        }
        for (const QString& d : args) {
            QString path = QDir(ui->currentDir).absoluteFilePath(d);
            if (QDir().mkpath(path)) {
                ui->appendOutput(QString("创建目录: %1\n").arg(d), THEME.green.name());
            } else {
                ui->appendOutput(QString("创建失败: %1\n").arg(d), THEME.red.name());
            }
        }
    }, "创建目录", "mkdir <目录名>");

    // rmdir
    registerCommand("rmdir", [ui](const QStringList& args) {
        if (args.isEmpty()) {
            ui->appendOutput("用法: rmdir <目录名>\n", THEME.red.name());
            return;
        }
        for (const QString& d : args) {
            QString path = QDir(ui->currentDir).absoluteFilePath(d);
            QDir dir(path);
            if (dir.exists() && dir.rmdir(path)) {
                ui->appendOutput(QString("删除目录: %1\n").arg(d), THEME.green.name());
            } else {
                ui->appendOutput(QString("删除失败: %1\n").arg(d), THEME.red.name());
            }
        }
    }, "删除空目录", "rmdir <目录名>");

    // rm / del
    registerCommand("rm", [ui](const QStringList& args) {
        if (args.isEmpty()) {
            ui->appendOutput("用法: rm <文件名>\n", THEME.red.name());
            return;
        }
        for (const QString& f : args) {
            QString path = QDir(ui->currentDir).absoluteFilePath(f);
            QFileInfo info(path);
            if (info.isDir()) {
                QDir dir(path);
                if (dir.removeRecursively()) {
                    ui->appendOutput(QString("删除目录: %1\n").arg(f), THEME.green.name());
                }
            } else if (QFile::remove(path)) {
                ui->appendOutput(QString("删除文件: %1\n").arg(f), THEME.green.name());
            } else {
                ui->appendOutput(QString("删除失败: %1\n").arg(f), THEME.red.name());
            }
        }
    }, "删除文件/目录", "rm <文件名>");

    registerCommand("del", [ui](const QStringList& args) {
        ui->executeInternalCommand("rm " + args.join(' '));
    }, "删除文件", "del <文件名>");

    // touch
    registerCommand("touch", [ui](const QStringList& args) {
        if (args.isEmpty()) {
            ui->appendOutput("用法: touch <文件名>\n", THEME.red.name());
            return;
        }
        for (const QString& f : args) {
            QString path = QDir(ui->currentDir).absoluteFilePath(f);
            QFile file(path);
            if (file.open(QIODevice::WriteOnly | QIODevice::Append)) {
                file.close();
                ui->appendOutput(QString("创建文件: %1\n").arg(f), THEME.green.name());
            } else {
                QFileInfo info(path);
                QDir().mkpath(info.absolutePath());
                if (file.open(QIODevice::WriteOnly | QIODevice::Append)) {
                    file.close();
                    ui->appendOutput(QString("创建文件: %1\n").arg(f), THEME.green.name());
                }
            }
        }
    }, "创建空文件", "touch <文件名>");

    // cat / type
    registerCommand("cat", [ui](const QStringList& args) {
        if (args.isEmpty()) {
            ui->appendOutput("用法: cat <文件名>\n", THEME.red.name());
            return;
        }
        for (const QString& f : args) {
            QString path = QDir(ui->currentDir).absoluteFilePath(f);
            QFile file(path);
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream in(&file);
                ui->appendOutput(in.readAll());
                file.close();
            } else {
                ui->appendOutput(QString("'%1' 不是文件\n").arg(f), THEME.red.name());
            }
        }
    }, "查看文件内容", "cat <文件名>");

    registerCommand("type", [ui](const QStringList& args) {
        ui->executeInternalCommand("cat " + args.join(' '));
    }, "查看文件内容", "type <文件名>");

    // cp / copy
    registerCommand("cp", [ui](const QStringList& args) {
        if (args.size() < 2) {
            ui->appendOutput("用法: cp <源> <目标>\n", THEME.red.name());
            return;
        }
        QString src = QDir(ui->currentDir).absoluteFilePath(args[0]);
        QString dst = QDir(ui->currentDir).absoluteFilePath(args[1]);
        QFileInfo srcInfo(src);
        if (srcInfo.isDir()) {
            Utils::copyDir(src, dst);
        } else {
            QFile::copy(src, dst);
        }
        ui->appendOutput(QString("复制: %1 -> %2\n").arg(args[0], args[1]), THEME.green.name());
    }, "复制文件", "cp <源> <目标>");

    registerCommand("copy", [ui](const QStringList& args) {
        ui->executeInternalCommand("cp " + args.join(' '));
    }, "复制文件", "copy <源> <目标>");

    // mv / move
    registerCommand("mv", [ui](const QStringList& args) {
        if (args.size() < 2) {
            ui->appendOutput("用法: mv <源> <目标>\n", THEME.red.name());
            return;
        }
        QString src = QDir(ui->currentDir).absoluteFilePath(args[0]);
        QString dst = QDir(ui->currentDir).absoluteFilePath(args[1]);
        if (QFile::rename(src, dst)) {
            ui->appendOutput(QString("移动: %1 -> %2\n").arg(args[0], args[1]), THEME.green.name());
        } else {
            ui->appendOutput("移动失败\n", THEME.red.name());
        }
    }, "移动文件", "mv <源> <目标>");

    registerCommand("move", [ui](const QStringList& args) {
        ui->executeInternalCommand("mv " + args.join(' '));
    }, "移动文件", "move <源> <目标>");

    // rename
    registerCommand("rename", [ui](const QStringList& args) {
        if (args.size() != 2) {
            ui->appendOutput("用法: rename <旧名> <新名>\n", THEME.red.name());
            return;
        }
        QString src = QDir(ui->currentDir).absoluteFilePath(args[0]);
        QString dst = QDir(ui->currentDir).absoluteFilePath(args[1]);
        if (QFile::rename(src, dst)) {
            ui->appendOutput(QString("重命名: %1 -> %2\n").arg(args[0], args[1]), THEME.green.name());
        } else {
            ui->appendOutput("重命名失败\n", THEME.red.name());
        }
    }, "重命名文件", "rename <旧名> <新名>");

    // tree
    registerCommand("tree", [ui](const QStringList&) {
        std::function<void(const QString&, const QString&, int)> showTree =
            [&](const QString& dirPath, const QString& prefix, int depth) {
            QDir dir(dirPath);
            QStringList items = dir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot,
                                               QDir::Name | QDir::DirsFirst | QDir::IgnoreCase);
            items.erase(std::remove_if(items.begin(), items.end(),
                [](const QString& s) { return s.startsWith('.'); }), items.end());
            for (int i = 0; i < items.size(); ++i) {
                bool isLast = (i == items.size() - 1);
                QString connector = isLast ? "└── " : "├── ";
                QString newPrefix = prefix + (isLast ? "    " : "│   ");
                QString path = dir.absoluteFilePath(items[i]);
                QFileInfo info(path);
                if (info.isDir()) {
                    ui->appendOutput(prefix + connector, THEME.blue.name());
                    ui->appendOutput(items[i] + "/\n", THEME.blue.name());
                    showTree(path, newPrefix, depth + 1);
                } else {
                    ui->appendOutput(prefix + connector + items[i] + "\n");
                }
            }
        };
        ui->appendOutput(ui->currentDir + "\n", THEME.blue.name());
        showTree(ui->currentDir, "", 0);
    }, "树形显示目录", "tree");

    // find
    registerCommand("find", [ui](const QStringList& args) {
        if (args.isEmpty()) {
            ui->appendOutput("用法: find <pattern>\n", THEME.red.name());
            return;
        }
        QString pattern = args[0].toLower();
        QStringList found;
        QDirIterator it(ui->currentDir, QDir::AllEntries | QDir::NoDotAndDotDot,
                        QDirIterator::Subdirectories);
        while (it.hasNext()) {
            it.next();
            if (!it.fileName().startsWith('.') && it.fileName().toLower().contains(pattern)) {
                found.append(QDir(ui->currentDir).relativeFilePath(it.filePath()));
            }
        }
        for (const QString& item : found) {
            ui->appendOutput(item + "\n");
        }
        ui->appendOutput(QString("找到 %1 个结果\n").arg(found.size()), THEME.green.name());
    }, "查找文件", "find <pattern>");

    // grep
    registerCommand("grep", [ui](const QStringList& args) {
        if (args.size() < 2) {
            ui->appendOutput("用法: grep <pattern> <file>\n", THEME.red.name());
            return;
        }
        QString pattern = args[0];
        QString path = QDir(ui->currentDir).absoluteFilePath(args[1]);
        QFile file(path);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            int lineNum = 0;
            while (!in.atEnd()) {
                QString line = in.readLine();
                lineNum++;
                if (line.contains(pattern)) {
                    ui->appendOutput(QString("%1: ").arg(lineNum), THEME.green.name());
                    ui->appendOutput(line + "\n");
                }
            }
            file.close();
        } else {
            ui->appendOutput(QString("文件不存在: %1\n").arg(args[1]), THEME.red.name());
        }
    }, "文本搜索", "grep <pattern> <file>");

    // wc
    registerCommand("wc", [ui](const QStringList& args) {
        if (args.isEmpty()) {
            ui->appendOutput("用法: wc <文件>\n", THEME.red.name());
            return;
        }
        QString path = QDir(ui->currentDir).absoluteFilePath(args[0]);
        QFile file(path);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString content = file.readAll();
            file.close();
            int lines = content.count('\n');
            int words = content.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts).size();
            int chars = content.size();
            ui->appendOutput(QString("行: %1  词: %2  字符: %3  %4\n")
                .arg(lines).arg(words).arg(chars).arg(args[0]));
        } else {
            ui->appendOutput(QString("文件不存在: %1\n").arg(args[0]), THEME.red.name());
        }
    }, "统计行数/词数/字符数", "wc <文件>");

    // head
    registerCommand("head", [ui](const QStringList& args) {
        int n = 10;
        QString fn;
        for (int i = 0; i < args.size(); ++i) {
            if (args[i] == "-n" && i + 1 < args.size()) {
                n = args[++i].toInt();
            } else {
                fn = args[i];
            }
        }
        if (fn.isEmpty()) {
            ui->appendOutput("用法: head [-n N] <文件>\n", THEME.red.name());
            return;
        }
        QString path = QDir(ui->currentDir).absoluteFilePath(fn);
        QFile file(path);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            for (int i = 0; i < n && !in.atEnd(); ++i) {
                ui->appendOutput(in.readLine() + "\n");
            }
            file.close();
        }
    }, "显示文件前N行", "head [-n N] <文件>");

    // tail
    registerCommand("tail", [ui](const QStringList& args) {
        int n = 10;
        QString fn;
        for (int i = 0; i < args.size(); ++i) {
            if (args[i] == "-n" && i + 1 < args.size()) {
                n = args[++i].toInt();
            } else {
                fn = args[i];
            }
        }
        if (fn.isEmpty()) {
            ui->appendOutput("用法: tail [-n N] <文件>\n", THEME.red.name());
            return;
        }
        QString path = QDir(ui->currentDir).absoluteFilePath(fn);
        QFile file(path);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QStringList lines;
            QTextStream in(&file);
            while (!in.atEnd()) {
                lines.append(in.readLine());
            }
            file.close();
            for (int i = qMax(0, lines.size() - n); i < lines.size(); ++i) {
                ui->appendOutput(lines[i] + "\n");
            }
        }
    }, "显示文件后N行", "tail [-n N] <文件>");

    // more / less
    registerCommand("more", [ui](const QStringList& args) {
        if (args.isEmpty()) {
            ui->appendOutput("用法: more <文件>\n", THEME.red.name());
            return;
        }
        QString path = QDir(ui->currentDir).absoluteFilePath(args[0]);
        QFile file(path);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            ui->appendOutput(file.readAll());
            file.close();
        }
    }, "分页查看文件", "more <文件>");

    registerCommand("less", [ui](const QStringList& args) {
        ui->executeInternalCommand("more " + args.join(' '));
    }, "分页查看文件", "less <文件>");

    // diff
    registerCommand("diff", [ui](const QStringList& args) {
        if (args.size() != 2) {
            ui->appendOutput("用法: diff <文件1> <文件2>\n", THEME.red.name());
            return;
        }
        QString path1 = QDir(ui->currentDir).absoluteFilePath(args[0]);
        QString path2 = QDir(ui->currentDir).absoluteFilePath(args[1]);
        QFile f1(path1), f2(path2);
        if (f1.open(QIODevice::ReadOnly) && f2.open(QIODevice::ReadOnly)) {
            QStringList l1 = QString(f1.readAll()).split('\n');
            QStringList l2 = QString(f2.readAll()).split('\n');
            for (int i = 0; i < qMax(l1.size(), l2.size()); ++i) {
                QString a = i < l1.size() ? l1[i] : "";
                QString b = i < l2.size() ? l2[i] : "";
                if (a != b) {
                    if (i < l1.size()) ui->appendOutput(QString("< %1\n").arg(a), THEME.red.name());
                    if (i < l2.size()) ui->appendOutput(QString("> %1\n").arg(b), THEME.green.name());
                }
            }
        }
    }, "比较文件差异", "diff <文件1> <文件2>");

    // sort
    registerCommand("sort", [ui](const QStringList& args) {
        if (args.isEmpty()) return;
        bool reverse = args.contains("-r");
        QString path = QDir(ui->currentDir).absoluteFilePath(args.last());
        QFile file(path);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QStringList lines = QString(file.readAll()).split('\n', Qt::SkipEmptyParts);
            file.close();
            lines.sort();
            if (reverse) std::reverse(lines.begin(), lines.end());
            for (const QString& line : lines) {
                ui->appendOutput(line + "\n");
            }
        }
    }, "排序文件内容", "sort [-r] <文件>");

    // uniq
    registerCommand("uniq", [ui](const QStringList& args) {
        if (args.isEmpty()) {
            ui->appendOutput("用法: uniq <文件>\n", THEME.red.name());
            return;
        }
        QString path = QDir(ui->currentDir).absoluteFilePath(args[0]);
        QFile file(path);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QStringList lines = QString(file.readAll()).split('\n', Qt::SkipEmptyParts);
            file.close();
            QSet<QString> seen;
            for (const QString& line : lines) {
                if (!seen.contains(line)) {
                    ui->appendOutput(line + "\n");
                    seen.insert(line);
                }
            }
        }
    }, "去重", "uniq <文件>");

    // cut
    registerCommand("cut", [ui](const QStringList& args) {
        if (args.isEmpty()) return;
        QString delim = ",";
        QList<int> fields;
        QString fn;
        for (int i = 0; i < args.size(); ++i) {
            if (args[i] == "-d" && i + 1 < args.size()) {
                delim = args[++i];
            } else if (args[i] == "-f" && i + 1 < args.size()) {
                QStringList fparts = args[++i].split(',');
                for (const QString& fp : fparts) {
                    fields.append(fp.toInt() - 1);
                }
            } else {
                fn = args[i];
            }
        }
        if (fn.isEmpty()) return;
        QString path = QDir(ui->currentDir).absoluteFilePath(fn);
        QFile file(path);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            while (!in.atEnd()) {
                QStringList parts = in.readLine().split(delim);
                QStringList selected;
                for (int f : fields) {
                    if (f < parts.size()) selected.append(parts[f]);
                }
                ui->appendOutput(selected.join(delim) + "\n");
            }
            file.close();
        }
    }, "剪切字段", "cut -d<分隔符> -f<字段> <文件>");

    // stat
    registerCommand("stat", [ui](const QStringList& args) {
        if (args.isEmpty()) {
            ui->appendOutput("用法: stat <文件>\n", THEME.red.name());
            return;
        }
        QString path = QDir(ui->currentDir).absoluteFilePath(args[0]);
        QFileInfo info(path);
        if (info.exists()) {
            ui->appendOutput(QString("文件: %1\n大小: %2 字节\n修改: %3\n")
                .arg(args[0])
                .arg(info.size())
                .arg(info.lastModified().toString("yyyy-MM-dd hh:mm:ss")));
        } else {
            ui->appendOutput(QString("文件不存在: %1\n").arg(args[0]), THEME.red.name());
        }
    }, "查看文件状态", "stat <文件>");

    // file
    registerCommand("file", [ui](const QStringList& args) {
        if (args.isEmpty()) {
            ui->appendOutput("用法: file <文件>\n", THEME.red.name());
            return;
        }
        QString path = QDir(ui->currentDir).absoluteFilePath(args[0]);
        QFileInfo info(path);
        if (info.isDir()) {
            ui->appendOutput(QString("%1: directory\n").arg(args[0]));
        } else if (info.isFile()) {
            QString ext = info.suffix().toLower();
            QMap<QString, QString> types = {
                {"py", "Python script"}, {"txt", "ASCII text"}, {"zip", "Zip archive"},
                {"html", "HTML document"}, {"json", "JSON data"}, {"md", "Markdown document"},
                {"cpp", "C++ source"}, {"h", "C header"}, {"jpg", "JPEG image"},
                {"png", "PNG image"}, {"pdf", "PDF document"}
            };
            ui->appendOutput(QString("%1: %2, %3 bytes\n")
                .arg(args[0], types.value(ext, "data")).arg(info.size()));
        }
    }, "检测文件类型", "file <文件>");

    // du
    registerCommand("du", [ui](const QStringList& args) {
        QString target = args.isEmpty() ? ui->currentDir
                        : QDir(ui->currentDir).absoluteFilePath(args[0]);
        qint64 total = 0;
        QDirIterator it(target, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            it.next();
            total += it.fileInfo().size();
        }
        QString size;
        if (total < 1024) size = QString("%1B").arg(total);
        else if (total < 1048576) size = QString("%1K").arg(total / 1024.0, 0, 'f', 1);
        else if (total < 1073741824) size = QString("%1M").arg(total / 1048576.0, 0, 'f', 1);
        else size = QString("%1G").arg(total / 1073741824.0, 0, 'f', 1);
        ui->appendOutput(QString("%1\t%2\n").arg(size, target));
    }, "显示磁盘使用量", "du [目录]");

    // df
    registerCommand("df", [ui](const QStringList&) {
        QStorageInfo storage(ui->currentDir);
        if (storage.isValid()) {
            qint64 total = storage.bytesTotal();
            qint64 free = storage.bytesAvailable();
            qint64 used = total - free;
            ui->appendOutput(QString("总量: %1G  已用: %2G  可用: %3G\n")
                .arg(total / 1073741824.0, 0, 'f', 1)
                .arg(used / 1073741824.0, 0, 'f', 1)
                .arg(free / 1073741824.0, 0, 'f', 1));
        }
    }, "显示磁盘空间", "df");

    // ln
    registerCommand("ln", [ui](const QStringList& args) {
        if (args.contains("-s") && args.size() >= 3) {
            QString target = args[1];
            QString link = QDir(ui->currentDir).absoluteFilePath(args[2]);
            if (QFile::link(target, link)) {
                ui->appendOutput(QString("创建链接: %1 -> %2\n").arg(args[2], target), THEME.green.name());
            }
        } else {
            ui->appendOutput("用法: ln -s <目标> <链接>\n", THEME.yellow.name());
        }
    }, "创建链接", "ln -s <目标> <链接>");

    // chmod
    registerCommand("chmod", [ui](const QStringList& args) {
        if (args.size() < 2) {
            ui->appendOutput("用法: chmod <权限> <文件>\n", THEME.red.name());
            return;
        }
        QString path = QDir(ui->currentDir).absoluteFilePath(args[1]);
        bool ok;
        int mode = args[0].toInt(&ok, 8);
        if (ok && QFile::setPermissions(path, QFile::Permission(mode))) {
            ui->appendOutput(QString("权限已更改: %1\n").arg(args[1]), THEME.green.name());
        }
    }, "更改文件权限", "chmod <权限> <文件>");

    // which
    registerCommand("which", [ui](const QStringList& args) {
        if (args.isEmpty()) {
            ui->appendOutput("用法: which <命令>\n", THEME.red.name());
            return;
        }
        // 检查内置命令
        if (ui->cmdRegistry->getCommandNames().contains(args[0])) {
            ui->appendOutput(QString("%1: 内置命令\n").arg(args[0]), THEME.green.name());
        } else {
            QString path = QStandardPaths::findExecutable(args[0]);
            if (!path.isEmpty()) {
                ui->appendOutput(path + "\n");
            } else {
                ui->appendOutput(QString("未找到: %1\n").arg(args[0]), THEME.red.name());
            }
        }
    }, "查找命令位置", "which <命令>");
}