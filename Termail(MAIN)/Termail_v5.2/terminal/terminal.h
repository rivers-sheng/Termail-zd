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

#include "config.h"
#include "commands/commands.h"

class TerminalWidget : public QWidget {
    Q_OBJECT

public:
    explicit TerminalWidget(QWidget* parent = nullptr);
    ~TerminalWidget();

    // 公开成员（命令需要访问）
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

    void appendOutput(const QString& text, const QColor& color = FG_COLOR);
    void clearScreen();
    void updatePrompt();
    void executeInternalCommand(const QString& commandLine);
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

    QStringList history;
    int historyIndex;
    CommandRegistry* cmdRegistry;
};

#endif