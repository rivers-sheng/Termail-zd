#ifndef CONFIG_H
#define CONFIG_H

#include <QString>
#include <QColor>
#include <QFont>

const QString APP_NAME = "Termail";
const QString APP_VERSION = "6.5.0";

const QColor BG_COLOR("#000000");
const QColor FG_COLOR("#ffffff");
const QColor PROMPT_COLOR("#00ff00");
const QColor ERROR_COLOR("#ff4444");
const QColor INFO_COLOR("#888888");
const QColor ACCENT_COLOR("#00ccff");

#ifdef Q_OS_ANDROID
#define FONT_FAMILY "monospace"
#else
#define FONT_FAMILY "Consolas"
#endif

const int FONT_SIZE = 12;

const int MINE_EASY_ROWS = 8, MINE_EASY_COLS = 8, MINE_EASY_MINES = 10;
const int MINE_MED_ROWS = 12, MINE_MED_COLS = 12, MINE_MED_MINES = 25;
const int MINE_HARD_ROWS = 16, MINE_HARD_COLS = 16, MINE_HARD_MINES = 40;

#endif
