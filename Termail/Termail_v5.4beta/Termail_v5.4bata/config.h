#ifndef CONFIG_H
#define CONFIG_H

#include <QString>
#include <QColor>
#include <QFont>
#include <QFontDatabase>

const QString APP_NAME = "Termail终端";
const QString APP_VERSION = "5.4.0";

const QColor BG_COLOR("#000000");
const QColor FG_COLOR("#ffffff");
const QColor PROMPT_COLOR("#00ff00");
const QColor ERROR_COLOR("#ff4444");
const QColor INFO_COLOR("#888888");
const QColor ACCENT_COLOR("#00ccff");

#ifdef Q_OS_ANDROID
inline QString getFontFamily() {
    QStringList candidates = {"Droid Sans Mono", "monospace", "Courier New"};
    for (const QString& f : candidates) {
        if (QFontDatabase::families().contains(f)) return f;
    }
    return "monospace";
}
#define FONT_FAMILY getFontFamily()
#else
#define FONT_FAMILY "Consolas"
#endif

const int FONT_SIZE = 12;

const int MINE_EASY_ROWS = 8, MINE_EASY_COLS = 8, MINE_EASY_MINES = 10;
const int MINE_MED_ROWS = 12, MINE_MED_COLS = 12, MINE_MED_MINES = 25;
const int MINE_HARD_ROWS = 16, MINE_HARD_COLS = 16, MINE_HARD_MINES = 40;

#endif