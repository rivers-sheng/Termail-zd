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
#include <QAction>
#include <QKeySequence>
#include <QTimer>

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
    QTextEdit* outputArea;
    QLineEdit* inputField;
    QLabel* promptLabel;

    void appendOutput(const QString& text, const QColor& color = FG_COLOR);
    void clearScreen();
    void updatePrompt();
    void executeInternalCommand(const QString& commandLine);
    QStringList getHistory() const { return history; }

private slots:
    void onCommandEntered();
    void onHistoryUp();
    void onHistoryDown();
    void onTabComplete();

private:
    void setupUI();
    void initCommands();
    void displayWelcome();
    void setupShortcuts();

    QStringList history;
    int historyIndex;
    int tabMatchIndex;
    CommandRegistry* cmdRegistry;
};

#endif