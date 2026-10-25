#ifndef CONFIG_H
#define CONFIG_H

#include <QString>
#include <QColor>
#include <QFont>

const QString APP_NAME = "Termail Games";
const QString APP_VERSION = "1.4.0";

// 配色
const QColor BG_COLOR("#000000");
const QColor FG_COLOR("#ffffff");
const QColor PROMPT_COLOR("#00ff00");
const QColor ERROR_COLOR("#ff4444");
const QColor INFO_COLOR("#888888");
const QColor ACCENT_COLOR("#00ccff");

// 字体
#ifdef Q_OS_ANDROID
#define FONT_FAMILY "monospace"
#else
#define FONT_FAMILY "Consolas"
#endif

const int FONT_SIZE = 12;

#endif