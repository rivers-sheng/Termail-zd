#include "terminal.h"
#include "utils/utils.h"

#include <QApplication>
#include <QScrollBar>
#include <QDir>
#include <QStandardPaths>
#include <QDateTime>
#include <QRegularExpression>

TerminalWidget::TerminalWidget(QWidget* parent)
    : QWidget(parent), historyIndex(0), tabMatchIndex(0) {

    currentDir = QDir::currentPath();
#ifdef Q_OS_WIN
    username = qgetenv("USERNAME");
#else
    username = qgetenv("USER");
#endif
    if (username.isEmpty()) username = "user";
    hostname = QSysInfo::machineHostName();
    if (hostname.isEmpty()) hostname = "localhost";

    cmdRegistry = new CommandRegistry(this);
    setupUI();
    initCommands();
    setupShortcuts();
    displayWelcome();
}

TerminalWidget::~TerminalWidget() {
    delete cmdRegistry;
}

void TerminalWidget::setupUI() {
    setStyleSheet(QString("background-color: %1;").arg(BG_COLOR.name()));

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 输出区
    outputArea = new QTextEdit(this);
    outputArea->setReadOnly(true);
    outputArea->setFrameShape(QFrame::NoFrame);
    outputArea->setStyleSheet(
        QString("QTextEdit { background-color: %1; color: %2; border: none; padding: 4px; }")
            .arg(BG_COLOR.name(), FG_COLOR.name())
    );
    outputArea->setFont(QFont(FONT_FAMILY, FONT_SIZE));
    outputArea->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    outputArea->setUndoRedoEnabled(false);
    mainLayout->addWidget(outputArea, 1);

    // 输入区
    QHBoxLayout* inputLayout = new QHBoxLayout();
    inputLayout->setContentsMargins(4, 2, 4, 4);
    inputLayout->setSpacing(2);

    promptLabel = new QLabel(this);
    promptLabel->setFont(QFont(FONT_FAMILY, FONT_SIZE));
    promptLabel->setStyleSheet(QString("color: %1; background: transparent;").arg(PROMPT_COLOR.name()));
    updatePrompt();
    inputLayout->addWidget(promptLabel);

    inputField = new QLineEdit(this);
    inputField->setFont(QFont(FONT_FAMILY, FONT_SIZE));
    inputField->setStyleSheet(
        QString("QLineEdit { background-color: %1; color: %2; border: none; padding: 2px; }")
            .arg(BG_COLOR.name(), FG_COLOR.name())
    );
    inputLayout->addWidget(inputField, 1);

    mainLayout->addLayout(inputLayout);

    connect(inputField, &QLineEdit::returnPressed, this, &TerminalWidget::onCommandEntered);
    QTimer::singleShot(100, [this]() { inputField->setFocus(); });
}

void TerminalWidget::setupShortcuts() {
    QAction* clearAction = new QAction(this);
    clearAction->setShortcut(QKeySequence("Ctrl+L"));
    connect(clearAction, &QAction::triggered, this, &TerminalWidget::clearScreen);
    addAction(clearAction);

    QAction* upAction = new QAction(this);
    upAction->setShortcut(Qt::Key_Up);
    connect(upAction, &QAction::triggered, this, &TerminalWidget::onHistoryUp);
    addAction(upAction);

    QAction* downAction = new QAction(this);
    downAction->setShortcut(Qt::Key_Down);
    connect(downAction, &QAction::triggered, this, &TerminalWidget::onHistoryDown);
    addAction(downAction);
}

void TerminalWidget::initCommands() {
    cmdRegistry->registerAllBuiltins(this);
}

void TerminalWidget::displayWelcome() {
    QString welcome = QString(
        " Termail Games特别版\n"
        " 输入 'help' 查看所有命令\n"
        " 制作:rivers晟 github:jiang-sheng\n"
        " 辅助制作:deepseek chat\n"
        " 邮箱:k-jiangsheng@outlook.com\n"
        " ©rivers晟 GNU GENERAL PUBLIC LICENSE\n"
        " 输入 'exit' 退出\n"
        "  \n"
        "\n"
    ).arg(APP_VERSION);
    appendOutput(welcome, ACCENT_COLOR);
}

void TerminalWidget::appendOutput(const QString& text, const QColor& color) {
    QScrollBar* sb = outputArea->verticalScrollBar();
    bool atBottom = (sb->value() >= sb->maximum() - 10);

    outputArea->moveCursor(QTextCursor::End);
    if (color != FG_COLOR) {
        outputArea->insertHtml(QString("<span style='color:%1;'>%2</span>")
            .arg(color.name(), text.toHtmlEscaped().replace("\n", "<br>").replace("'", "&#39;")));
    } else {
        outputArea->insertPlainText(text);
    }

    if (atBottom) sb->setValue(sb->maximum());
}

void TerminalWidget::clearScreen() {
    outputArea->clear();
}

void TerminalWidget::updatePrompt() {
    QString home = QDir::homePath();
    QString path = currentDir;
    if (path.startsWith(home)) path.replace(home, "~");
    promptLabel->setText(QString("%1:%2$ ").arg(username, path));
}

void TerminalWidget::onCommandEntered() {
    QString command = inputField->text().trimmed();
    inputField->clear();

    if (command.isEmpty()) {
        appendOutput(promptLabel->text() + "\n");
        return;
    }

    appendOutput(promptLabel->text() + command + "\n");
    if (history.size() >= 1000) history.removeFirst();
    history.append(command);
    historyIndex = history.size();

    try {
        executeInternalCommand(command);
    } catch (const std::exception& e) {
        appendOutput(QString("错误: %1\n").arg(e.what()), ERROR_COLOR);
    }

    updatePrompt();
    inputField->setFocus();
}

void TerminalWidget::executeInternalCommand(const QString& commandLine) {
    QStringList parts = commandLine.trimmed().split(QRegularExpression("\\s+"));
    if (parts.isEmpty()) return;

    QString cmd = parts[0].toLower();
    QStringList args = parts.mid(1);

    if (cmdRegistry->execute(cmd, args)) return;

    // 命令未找到
    QString suggestion = cmdRegistry->suggestCommand(cmd);
    if (!suggestion.isEmpty()) {
        appendOutput(QString("命令未找到: %1\n").arg(cmd), ERROR_COLOR);
        appendOutput(QString("你是不是想输入: %1\n").arg(suggestion), INFO_COLOR);
    } else {
        appendOutput(QString("命令未找到: %1 (输入 help 查看)\n").arg(cmd), ERROR_COLOR);
    }
}

void TerminalWidget::onHistoryUp() {
    if (!history.isEmpty() && historyIndex > 0) {
        historyIndex--;
        inputField->setText(history[historyIndex]);
    }
}

void TerminalWidget::onHistoryDown() {
    if (historyIndex < history.size() - 1) {
        historyIndex++;
        inputField->setText(history[historyIndex]);
    } else {
        historyIndex = history.size();
        inputField->clear();
    }
}

void TerminalWidget::onTabComplete() {
    QString current = inputField->text();
    if (current.isEmpty()) return;

    QStringList cmds = cmdRegistry->getCommandNames();
    QStringList matches;
    for (const QString& c : cmds)
        if (c.startsWith(current, Qt::CaseInsensitive)) matches.append(c);

    if (!matches.isEmpty()) {
        inputField->setText(matches[tabMatchIndex % matches.size()]);
        tabMatchIndex++;
        if (matches.size() > 1)
            appendOutput(matches.join("  ") + "\n", INFO_COLOR);
    }
}