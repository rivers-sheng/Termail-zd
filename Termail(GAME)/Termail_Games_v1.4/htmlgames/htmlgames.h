#ifndef HTMLGAMES_H
#define HTMLGAMES_H

#include <QString>
#include <QList>

// HTML 游戏结构
struct HtmlGame {
    int id;
    QString name;
    QString dirName;
};

class HtmlGames {
public:
    // 获取所有游戏
    static const QList<HtmlGame>& allGames();

    // 通过编号或名字找游戏
    static const HtmlGame* findGame(const QString& input);

    // 启动游戏
    static void launch(const QString& dirName);

    // 读取游戏介绍（I.txt）
    static QString readGameInfo(const QString& dirName);

    // 判断是否为游戏名或编号
    static bool isGame(const QString& input);
};

#endif