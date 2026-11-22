#ifndef TERMINAL_H
#define TERMINAL_H

#include <QWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStringList>
#include <QMap>
#include <QProcess>
#include <QJsonObject>
#include <QSettings>
#include <QAction>
#include <QTimer>
#include <QProgressBar>

#include "config.h"
#include "commands/commands.h"

struct Session {
    int id;
    QString name;
    QString currentDir;
    QStringList history;
    QString outputBuffer;
    QProcess* process;
    bool interactiveMode;
    QProcess* interactiveProcess;
};

class TerminalWidget : public QWidget {
    Q_OBJECT

public:
    explicit TerminalWidget(QWidget* parent = nullptr);
    ~TerminalWidget();

    QString currentDir;
    QString username;
    QString hostname;
    QString previousDir;
    QMap<QString, QString> aliases;
    QMap<QString, QJsonObject> customCommands;
    QProcess* currentProcess;
    QTextEdit* outputArea;
    QLineEdit* inputField;
    QLabel* promptLabel;
    QLabel* statusLabel;
    QString pendingCommand;

    QMap<int, Session> sessions;
    int currentSessionId;
    int nextSessionId;

    void appendOutput(const QString& text, const QColor& color = FG_COLOR);
    void clearScreen();
    void updatePrompt();
    void updateStatusBar();
    void executeInternalCommand(const QString& commandLine, int depth = 0);
    void saveCustomCommands();
    void saveAliases();
    void loadAliases();
    QStringList getHistory() const;

    void createSession(const QString& name = "");
    void switchSession(int id);
    void killSession(int id);
    void listSessions();

private slots:
    void onCommandEntered();
    void onHistoryUp();
    void onHistoryDown();
    void onTabComplete();
    void runNextCheck(); // 新增

private:
    void setupUI();
    void initCommands();
    void loadCustomCommands();
    void displayWelcome();
    void executePipeline(const QString& commandLine);
    void executeRedirect(const QString& commandLine);
    bool handleEasterEgg(const QString& command);
    void setupShortcuts();
    void loadFontSettings();
    void saveFontSettings();
    void startInteractiveProcess(const QString& command);
    void updateMemoryStatus();
    void logCommand(const QString& cmd);
    void vibrate(int ms);
    void saveHistory();
    QString suggestCommand(const QString& cmd);
    void startSelfCheck(); // 新增

    QStringList history;
    int historyIndex;
    int tabMatchIndex;
    bool interactiveMode;
    QProcess* interactiveProcess;
    QTimer* statusTimer;
    CommandRegistry* cmdRegistry;

    // 自检相关变量声明
    QStringList m_cmdList;
    int m_cmdIndex;
    QStringList m_availableCmds;
};

#endif
