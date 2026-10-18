#include "htmlgames.h"
#include "config.h"

#include <QFile>
#include <QDir>

#ifdef Q_OS_ANDROID
#include <QJniObject>
#endif

static const QList<HtmlGame> g_games = {
    {1,  "2048",          "2048"},
    {2,  "24-points",     "24-points"},
    {3,  "brick-breaker", "brick-breaker"},
    {4,  "connect",       "connect"},
    {5,  "dodgeball",     "dodgeball"},
    {6,  "dwarfs",        "Dwarfs"},
    {7,  "fabric-brush",  "Fabric-Brush"},
    {8,  "hit-airplane",  "hit-airplane"},
    {9,  "huarong-path",  "huarong-path"},
    {10, "memory-cards",  "memory-cards"},
    {11, "minesweeper",   "minesweeper"},
    {12, "radius-raid",   "Radius-Raid"},
    {13, "snack",         "snack"},
    {14, "stroke",        "stroke"},
    {15, "sudoku",        "sudoku"},
    {16, "tetris",        "tetris"},
    {17, "tic-tac-toe",   "tic-tac-toe"},
    {18, "xx142-b2",      "xx142-b2"},
    {19, "chinese-chess", "chinese-chess"},
    {20, "cutting-fruit", "cutting-fruit"},
    {21, "fireworks",     "fireworks"},
    {22, "flappy-bird",   "flappy-bird"},
    {23, "go",            "go"},
    {24, "gomoku-connect","gomoku-connect"},
    {25, "jump-jump",     "jump-jump"},
    {26, "mining-world",  "mining-world"},
    {27, "othello",       "othello"},
    {28, "whack-a-mole",  "whack-a-mole"},
};

const QList<HtmlGame>& HtmlGames::allGames() {
    return g_games;
}

const HtmlGame* HtmlGames::findGame(const QString& input) {
    bool isNum;
    int num = input.toInt(&isNum);
    if (isNum) {
        for (const HtmlGame& g : g_games)
            if (g.id == num) return &g;
    } else {
        for (const HtmlGame& g : g_games)
            if (g.name.compare(input, Qt::CaseInsensitive) == 0) return &g;
    }
    return nullptr;
}

bool HtmlGames::isGame(const QString& input) {
    return findGame(input) != nullptr;
}

void HtmlGames::launch(const QString& dirName) {
#ifdef Q_OS_ANDROID
    QJniObject activity = QJniObject::callStaticObjectMethod(
        "org/qtproject/qt/android/QtNative", "activity",
        "()Landroid/app/Activity;");
    if (!activity.isValid()) return;

    QJniObject intent("com/termail/spical/GameWebView",
        "(Landroid/content/Context;)V", activity.object());
    intent.callObjectMethod("putExtra",
        "(Ljava/lang/String;Ljava/lang/String;)Landroid/content/Intent;",
        QJniObject::fromString("game").object(),
        QJniObject::fromString(dirName).object());
    activity.callMethod<void>("startActivity",
        "(Landroid/content/Intent;)V", intent.object());
#else
    Q_UNUSED(dirName);
#endif
}

QString HtmlGames::readGameInfo(const QString& dirName) {
#ifdef Q_OS_ANDROID
    QJniObject activity = QJniObject::callStaticObjectMethod(
        "org/qtproject/qt/android/QtNative", "activity",
        "()Landroid/app/Activity;");
    if (!activity.isValid()) return "暂无介绍";

    QJniObject result = QJniObject::callStaticObjectMethod(
        "com/termail/spical/GameHelper", "readGameInfo",
        "(Landroid/content/Context;Ljava/lang/String;)Ljava/lang/String;",
        activity.object(),
        QJniObject::fromString(dirName).object());
    if (result.isValid()) {
        QString info = result.toString();
        if (!info.isEmpty()) return info;
    }
#else
    // 桌面版直接从文件读（调试用）
    QFile f("android/assets/games/" + dirName + "/I.txt");
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString content = QString::fromUtf8(f.readAll());
        f.close();
        return content;
    }
#endif
    return "暂无介绍";
}