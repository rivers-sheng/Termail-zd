#ifndef CONFIG_H
#define CONFIG_H

#include <QString>
#include <QMap>
#include <QStringList>
#include <QColor>
#include <QFont>
#include <functional>

// ============ 全局常量 ============
const QString APP_NAME = "Termail终端";
const QString APP_VERSION = "4.0.0";
const QString APP_AUTHOR = "Termail Team";
const QString DEFAULT_CONFIG_PATH = "~/.termail/custom_commands.json";

// ============ 颜色主题 ============
struct ThemeColors {
    QColor bg = QColor("#1e1e1e");
    QColor fg = QColor("#ffffff");
    QColor inputBg = QColor("#2d2d2d");
    QColor outputBg = QColor("#1e1e1e");
    QColor green = QColor("#00ff00");
    QColor blue = QColor("#569cd6");
    QColor yellow = QColor("#dcdcaa");
    QColor red = QColor("#f44747");
    QColor purple = QColor("#c586c0");
    QColor cyan = QColor("#4ec9b0");
    QColor orange = QColor("#ce9178");
};

const ThemeColors THEME;

// ============ 字体配置 ============
#ifdef Q_OS_ANDROID
    const QString DEFAULT_FONT_FAMILY = "Droid Sans Mono";
    const int DEFAULT_FONT_SIZE = 10;
#else
    const QString DEFAULT_FONT_FAMILY = "Consolas";
    const int DEFAULT_FONT_SIZE = 11;
#endif

const QFont TERMINAL_FONT(DEFAULT_FONT_FAMILY, DEFAULT_FONT_SIZE);
const QFont TITLE_FONT(DEFAULT_FONT_FAMILY, DEFAULT_FONT_SIZE, QFont::Bold);

// ============ 扫雷默认配置 ============
struct MinesweeperConfig {
    int easyRows = 8, easyCols = 8, easyMines = 10;
    int mediumRows = 12, mediumCols = 12, mediumMines = 25;
    int hardRows = 16, hardCols = 16, hardMines = 40;
    int cellSizeMin = 25, cellSizeMax = 35, cellSizeDefault = 30;
};

const MinesweeperConfig MINESWEEPER_CFG;

// ============ 命令类型枚举 ============
enum class CommandType {
    BUILTIN,
    FILE_OP,
    NETWORK,
    SYSTEM,
    TEXT_PROC,
    FUN,
    CUSTOM
};

// ============ 命令函数签名 ============
using CommandFunc = std::function<void(const QStringList&)>;

// ============ 命令信息结构 ============
struct CommandInfo {
    QString name;
    QString description;
    QString usage;
    CommandType type;
};

// ============ 彩蛋触发词 ============
const QStringList EASTER_EGG_GAME = {"game", "play", "xyzzy"};
const QString EASTER_EGG_KONAMI = "上上下下左右左右ba";

#endif // CONFIG_H