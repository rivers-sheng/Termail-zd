#ifndef UTILS_H
#define UTILS_H

#include <QString>
#include <QStringList>
#include <QFile>
#include <QDir>

class TerminalWidget;

class Utils {
public:
    // 格式化文件大小
    static QString formatSize(qint64 bytes);

    // 读取文本文件（UTF-8）
    static QString readTextFile(const QString& path);

    // 写入文本文件（UTF-8）
    static bool writeTextFile(const QString& path, const QString& content);

    // 配置文件目录
    static QString configDir();
};

#endif