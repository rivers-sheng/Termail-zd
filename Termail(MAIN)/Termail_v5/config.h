#ifndef CONFIG_H
#define CONFIG_H

#include <QString>
#include <QColor>
#include <QFont>

const QString APP_NAME = "Termail终端";
const QString APP_VERSION = "5.0.0";

// 终端配色
const QColor BG_COLOR("#000000");
const QColor FG_COLOR("#ffffff");
const QColor PROMPT_COLOR("#00ff00");
const QColor ERROR_COLOR("#ff4444");
const QColor INFO_COLOR("#888888");
const QColor ACCENT_COLOR("#00ccff");

// 字体
#ifdef Q_OS_ANDROID
const QString FONT_FAMILY = "Droid Sans Mono";
#else
const QString FONT_FAMILY = "Consolas";
#endif
const int FONT_SIZE = 12;

// 扫雷默认
const int MINE_EASY_ROWS = 8, MINE_EASY_COLS = 8, MINE_EASY_MINES = 10;
const int MINE_MED_ROWS = 12, MINE_MED_COLS = 12, MINE_MED_MINES = 25;
const int MINE_HARD_ROWS = 16, MINE_HARD_COLS = 16, MINE_HARD_MINES = 40;

#endif