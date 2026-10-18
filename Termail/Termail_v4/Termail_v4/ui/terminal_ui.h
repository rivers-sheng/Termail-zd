#ifndef TERMINAL_UI_H
#define TERMINAL_UI_H

#include <QMainWindow>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStringList>
#include <QMap>
#include <QProcess>
#include <QTimer>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

#include "config.h"
#include "commands/commands.h"
#include "utils/utils.h"

class TerminalUI : public QMainWindow {
    Q_OBJECT

public:
    explicit TerminalUI(bool adminMode, QWidget* parent = nullptr);
    ~TerminalUI();

    // 公开方法
    QList<QString> getHistory() const { return history; }
    void appendOutput(const QString& text, const QString& color = "white");
    void clearScreen();
    void updatePrompt();
    void executeInternalCommand(const QString& commandLine);

    // 公开成员变量
    QString currentDir;
    QString username;
    QString hostname;
    QString previousDir;
    bool isAdminMode;
    QMap<QString, QString> aliases;
    QMap<QString, QJsonObject> customCommands;
    QProcess* currentProcess;

    // 命令注册表(供which命令查询)
    CommandRegistry* cmdRegistry;

private slots:
    void onCommandEntered();
    void onHistoryUp();
    void onHistoryDown();
    void onTabComplete();
    void onStopProcess();

private:
    void setupUI();
    void setupMenus();
    void setupTheme();
    void initCommands();
    void loadCustomCommands();
    void saveCustomCommands();
    void displayWelcome();

    void executePipeline(const QString& commandLine);
    void executeRedirect(const QString& commandLine);
    bool handleEasterEgg(const QString& command);

    // 菜单动作
    void newWindow();
    void openDirectory();
    void copyText();
    void pasteText();
    void fontSettings();
    void openMinesweeper();
    void openCalculator();
    void openCustomCommandsDialog();
    void showHistoryWindow();
    void showHelp();
    void showShortcuts();
    void showAbout();

    // UI组件
    QTextEdit* outputArea;
    QLineEdit* inputField;
    QLabel* promptLabel;
    QMenuBar* menuBar;
    QPushButton* stopBtn;

    // 私有状态
    QStringList history;
    int historyIndex;
};

#endif // TERMINAL_UI_H