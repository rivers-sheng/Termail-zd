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

#include "config.h"
#include "commands/commands.h"

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
    QString pendingCommand;

    void appendOutput(const QString& text, const QColor& color = FG_COLOR);
    void clearScreen();
    void updatePrompt();
    void executeInternalCommand(const QString& commandLine, int depth = 0);
    void saveCustomCommands();
    QStringList getHistory() const { return history; }

private slots:
    void onCommandEntered();
    void onHistoryUp();
    void onHistoryDown();
    void onTabComplete();

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

    QStringList history;
    int historyIndex;
    int tabMatchIndex;
    bool interactiveMode;
    QProcess* interactiveProcess;
    CommandRegistry* cmdRegistry;
};

#endif