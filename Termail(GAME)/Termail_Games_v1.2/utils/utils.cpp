#include "utils.h"
#include "config.h"

#include <QStandardPaths>
#include <QFileInfo>
#include <QTextStream>
#include <QStringConverter>

QString Utils::formatSize(qint64 b) {
    if (b < 1024) return QString("%1B").arg(b);
    if (b < 1048576) return QString("%1K").arg(b / 1024.0, 0, 'f', 1);
    if (b < 1073741824) return QString("%1M").arg(b / 1048576.0, 0, 'f', 1);
    return QString("%1G").arg(b / 1073741824.0, 0, 'f', 1);
}

QString Utils::readTextFile(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }
    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    QString content = in.readAll();
    file.close();
    return content;
}

bool Utils::writeTextFile(const QString& path, const QString& content) {
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << content;
    file.close();
    return true;
}

QString Utils::configDir() {
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/termail_games";
}