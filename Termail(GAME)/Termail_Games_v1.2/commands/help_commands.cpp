#include "commands.h"
#include "terminal/terminal.h"
#include "utils/utils.h"

#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QApplication>

#ifdef Q_OS_ANDROID
#include <QJniObject>
#endif

// 游戏表定义（编号 + 名字 + 目录名）
struct HtmlGame {
    int id;
    QString name;
    QString dirName;
};

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
    {29, "dou-di-zhu",    "dou-di-zhu"},
    {30, "guard-the-blue","guard-the-blue"},
};

// 通过编号或名字找游戏（先名字后编号）
static const HtmlGame* findGame(const QString& input) {
    // 按名字匹配
    for (const HtmlGame& g : g_games) {
        if (g.name.compare(input, Qt::CaseInsensitive) == 0) return &g;
    }
    // 按编号匹配
    bool isNum;
    int num = input.toInt(&isNum);
    if (isNum && num >= 1) {
        for (const HtmlGame& g : g_games) {
            if (g.id == num) return &g;
        }
    }
    return nullptr;
}

// 启动 HTML 游戏（Android WebView）
static void launchHtmlGame(const QString& dirName) {
#ifdef Q_OS_ANDROID
    QJniObject activity = QJniObject::callStaticObjectMethod(
        "org/qtproject/qt/android/QtNative", "activity",
        "()Landroid/app/Activity;");
    if (!activity.isValid()) return;

    // 构建 Intent
    QJniObject intent("android/content/Intent", "()V");
    QJniObject componentName(
        "android/content/ComponentName",
        "(Ljava/lang/String;Ljava/lang/String;)V",
        QJniObject::fromString("com.termail.spical").object(),
        QJniObject::fromString("com.termail.spical.GameWebView").object());
    intent.callObjectMethod("setComponent",
        "(Landroid/content/ComponentName;)Landroid/content/Intent;",
        componentName.object());
    intent.callObjectMethod("putExtra",
        "(Ljava/lang/String;Ljava/lang/String;)Landroid/content/Intent;",
        QJniObject::fromString("game").object(),
        QJniObject::fromString(dirName).object());
    intent.callObjectMethod("addFlags",
        "(I)Landroid/content/Intent;", (jint)0x10000000);  // FLAG_ACTIVITY_NEW_TASK

    activity.callMethod<void>("startActivity",
        "(Landroid/content/Intent;)V", intent.object());
#endif
}

// 读取 I.txt
static QString readGameInfo(const QString& dirName) {
#ifdef Q_OS_ANDROID
    QJniObject activity = QJniObject::callStaticObjectMethod(
        "org/qtproject/qt/android/QtNative", "activity",
        "()Landroid/app/Activity;");
    if (!activity.isValid()) return "暂无介绍";

    QJniObject result = QJniObject::callStaticMethod<jstring>(
        "com/termail/spical/GameHelper", "readGameInfo",
        "(Landroid/content/Context;Ljava/lang/String;)Ljava/lang/String;",
        activity.object(),
        QJniObject::fromString(dirName).object());
    if (result.isValid()) {
        QString info = QJniObject(result).toString();
        if (!info.isEmpty()) return info;
    }
    return "暂无介绍";
#else
    QFile f("android/assets/games/" + dirName + "/I.txt");
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString content = QString::fromUtf8(f.readAll());
        f.close();
        return content;
    }
    return "暂无介绍";
#endif
}

void CommandRegistry::registerHelpCommands(TerminalWidget* ui) {

    // help
    registerCommand("help", [this, ui](const QStringList& args) {
        // help <游戏名/编号>
        if (!args.isEmpty()) {
            const HtmlGame* game = findGame(args[0]);
            if (game) {
                ui->appendOutput(QString("=== %1 ===\n\n").arg(game->name), ACCENT_COLOR);
                QString info = readGameInfo(game->dirName);
                ui->appendOutput(info + "\n", FG_COLOR);
                ui->appendOutput(QString("启动: 输入 %1 或 %2\n").arg(game->name).arg(game->id), INFO_COLOR);
                return;
            }
            ui->appendOutput(QString("未找到: %1\n").arg(args[0]), ERROR_COLOR);
            return;
        }

        // 显示全部命令
        QStringList cmds = getCommandNames();
        cmds.sort();
        ui->appendOutput("=== 命令列表 ===\n\n", ACCENT_COLOR);
        int count = 0;
        for (const QString& c : cmds) {
            ui->appendOutput(QString("%1").arg(c, -16));
            if (++count % 5 == 0) ui->appendOutput("\n");
        }
        ui->appendOutput(QString("\n\n共 %1 个命令\n").arg(cmds.size()), INFO_COLOR);

        // 显示游戏列表
        ui->appendOutput("\n=== HTML 游戏列表 ===\n\n", ACCENT_COLOR);
        for (const HtmlGame& g : g_games) {
            ui->appendOutput(QString(" [%1] %2\n").arg(g.id, -3).arg(g.name), PROMPT_COLOR);
        }
        ui->appendOutput("\n输入游戏名或编号启动，输入 help <名/编号> 查看介绍\n", INFO_COLOR);
    }, "帮助", "help [游戏名/编号]");

    // games
    registerCommand("games", [ui](const QStringList&) {
        ui->appendOutput("=== HTML 游戏列表 ===\n\n", ACCENT_COLOR);
        for (const HtmlGame& g : g_games) {
            ui->appendOutput(QString(" [%1] %2\n").arg(g.id, -3).arg(g.name), PROMPT_COLOR);
        }
        ui->appendOutput("\n输入游戏名或编号启动\n", INFO_COLOR);
    }, "游戏列表", "games");

    // 为每个游戏注册：名字命令 + 编号命令
    for (const HtmlGame& g : g_games) {
        QString dirName = g.dirName;

        // 名字命令
        registerCommand(g.name, [ui, dirName](const QStringList&) {
            ui->appendOutput(QString("启动游戏: %1\n").arg(dirName), ACCENT_COLOR);
            launchHtmlGame(dirName);
        }, "HTML游戏", g.name);

        // 编号命令
        registerCommand(QString::number(g.id), [ui, dirName, g](const QStringList&) {
            ui->appendOutput(QString("启动游戏: %1\n").arg(g.name), ACCENT_COLOR);
            launchHtmlGame(dirName);
        }, "HTML游戏", QString::number(g.id));
    }

    // exit
    registerCommand("exit", [ui](const QStringList&) {
        qApp->quit();
    }, "退出", "exit");

    registerCommand("quit", [ui](const QStringList&) {
        qApp->quit();
    }, "退出", "quit");

    // clear
    registerCommand("clear", [ui](const QStringList&) { ui->clearScreen(); }, "清屏", "clear");
    registerCommand("cls", [ui](const QStringList&) { ui->clearScreen(); }, "清屏", "cls");

    // echo
    registerCommand("echo", [ui](const QStringList& args) {
        ui->appendOutput(args.join(' ') + "\n");
    }, "输出文本", "echo <文本>");

    // license
    registerCommand("license", [ui](const QStringList&) {
        QFile file(":/LICENSE/GPL.txt");
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            ui->appendOutput(file.readAll());
            file.close();
        } else {
            ui->appendOutput("GPL 许可证未找到\n", ERROR_COLOR);
        }
    }, "查看GPL许可证", "license");
}