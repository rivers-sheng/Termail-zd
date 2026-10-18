#include "terminal.h"
#include "utils/utils.h"

#include <QApplication>
#include <QClipboard>
#include <QScrollBar>
#include <QDir>
#include <QStandardPaths>
#include <QDateTime>
#include <QThread>
#include <QRegularExpression>
#include <QJsonDocument>
#include <QKeyEvent>
#include <QTimer>

TerminalWidget::TerminalWidget(QWidget* parent)
    : QWidget(parent), historyIndex(0), currentProcess(nullptr) {

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
    loadCustomCommands();
    setupUI();
    initCommands();
    setupShortcuts();
    displayWelcome();
}

TerminalWidget::~TerminalWidget() {
    saveCustomCommands();
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
    mainLayout->addWidget(outputArea, 1);

    // 输入行：提示符 + 输入框
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

    QTimer::singleShot(100, [this]() {
    inputField->setFocus();
    });
}

void TerminalWidget::setupShortcuts() {
    // Ctrl+L 清屏
    QAction* clearAction = new QAction(this);
    clearAction->setShortcut(QKeySequence("Ctrl+L"));
    connect(clearAction, &QAction::triggered, this, &TerminalWidget::clearScreen);
    addAction(clearAction);

    // 历史导航
    QAction* upAction = new QAction(this);
    upAction->setShortcut(Qt::Key_Up);
    connect(upAction, &QAction::triggered, this, &TerminalWidget::onHistoryUp);
    addAction(upAction);

    QAction* downAction = new QAction(this);
    downAction->setShortcut(Qt::Key_Down);
    connect(downAction, &QAction::triggered, this, &TerminalWidget::onHistoryDown);
    addAction(downAction);

    // Tab 补全
    QAction* tabAction = new QAction(this);
    tabAction->setShortcut(Qt::Key_Tab);
    connect(tabAction, &QAction::triggered, this, &TerminalWidget::onTabComplete);
    addAction(tabAction);
}

void TerminalWidget::initCommands() {
    cmdRegistry->registerAllBuiltins(this);
    for (auto it = customCommands.begin(); it != customCommands.end(); ++it) {
        QString name = it.key();
        QString cmd = it.value()["command"].toString();
        cmdRegistry->registerCommand(name, [this, cmd](const QStringList& args) {
            QString expanded = cmd;
            for (int i = 0; i < args.size(); ++i) {
                expanded.replace(QString("${%1}").arg(i + 1), args[i]);
                expanded.replace(QString("${%1}").arg(i), args[i]);
            }
            expanded.replace("${*}", args.join(' '));
            expanded.replace("$@", args.join(' '));
            expanded.replace("$#", QString::number(args.size()));
            Utils::runSystemCommand(expanded, currentDir, this);
        });
    }
}

void TerminalWidget::loadCustomCommands() {
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
                   + "/termail/custom_commands.json";
    QFile file(path);
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        QJsonObject obj = doc.object();
        for (auto it = obj.begin(); it != obj.end(); ++it)
            customCommands[it.key()] = it.value().toObject();
        file.close();
    }
}

void TerminalWidget::saveCustomCommands() {
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
                   + "/termail/custom_commands.json";
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        QJsonObject obj;
        for (auto it = customCommands.begin(); it != customCommands.end(); ++it)
            obj[it.key()] = it.value();
        file.write(QJsonDocument(obj).toJson());
        file.close();
    }
}

void TerminalWidget::displayWelcome() {
    QString welcome = QString(
        "\n"
        "  Termail终端 v%1\n"
        "  输入 'help' 查看所有命令 输入 'get-busybox' 获得更多linux指令 \n"
        "  输入 'exit' 退出\n"
        "制作:rivers晟 github:jiang-sheng\n"
        "邮箱:k-jiangsheng@outlook.com\n"
        "©rivers晟 GNU GENERAL PUBLIC LICENSE\n"
        "\n"
    ).arg(APP_VERSION);
    appendOutput(welcome, ACCENT_COLOR);
}

void TerminalWidget::appendOutput(const QString& text, const QColor& color) {
    outputArea->moveCursor(QTextCursor::End);
    if (color != FG_COLOR) {
        outputArea->insertHtml(QString("<span style='color:%1;'>%2</span>")
            .arg(color.name(), text.toHtmlEscaped().replace("\n", "<br>")));
    } else {
        outputArea->insertPlainText(text);
    }
    QScrollBar* sb = outputArea->verticalScrollBar();
    sb->setValue(sb->maximum());
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
    if (command.isEmpty()) return;

    appendOutput(promptLabel->text() + command + "\n");
    history.append(command);
    historyIndex = history.size();

    if (handleEasterEgg(command)) {
        updatePrompt();
        return;
    }

    try {
        executeInternalCommand(command);
    } catch (const std::exception& e) {
        appendOutput(QString("错误: %1\n").arg(e.what()), ERROR_COLOR);
    }

    updatePrompt();
    inputField->setFocus();
}

void TerminalWidget::executeInternalCommand(const QString& commandLine) {
    if (commandLine.contains('|')) { executePipeline(commandLine); return; }
    if (commandLine.contains('>')) { executeRedirect(commandLine); return; }

    QStringList parts = commandLine.trimmed().split(QRegularExpression("\\s+"));
    if (parts.isEmpty()) return;

    QString cmd = parts[0].toLower();
    QStringList args = parts.mid(1);

    if (aliases.contains(cmd)) {
        QString expanded = aliases[cmd] + " " + args.join(' ');
        parts = expanded.trimmed().split(QRegularExpression("\\s+"));
        cmd = parts[0].toLower();
        args = parts.mid(1);
    }

    if (cmdRegistry->execute(cmd, args)) return;
    Utils::runSystemCommand(commandLine, currentDir, this);
}

void TerminalWidget::executePipeline(const QString& commandLine) {
    QStringList commands = commandLine.split('|');
    QString prevOutput;
    for (int i = 0; i < commands.size(); ++i) {
        QProcess process;
        process.setWorkingDirectory(currentDir);
        process.start("sh", QStringList() << "-c" << commands[i].trimmed());
        if (i > 0) { process.write(prevOutput.toUtf8()); process.closeWriteChannel(); }
        process.waitForFinished(10000);
        prevOutput = process.readAllStandardOutput();
        if (i == commands.size() - 1) {
            QString err = process.readAllStandardError();
            if (!err.isEmpty()) appendOutput(err, ERROR_COLOR);
        }
    }
    if (!prevOutput.isEmpty()) appendOutput(prevOutput);
}

void TerminalWidget::executeRedirect(const QString& commandLine) {
    bool append = commandLine.contains(">>");
    QStringList parts = commandLine.split(append ? ">>" : ">");
    if (parts.size() < 2) return;
    QProcess process;
    process.setWorkingDirectory(currentDir);
    process.start("sh", QStringList() << "-c" << parts[0].trimmed());
    process.waitForFinished(10000);
    QIODevice::OpenMode mode = append ? QIODevice::Append : QIODevice::WriteOnly;
    QFile file(QDir(currentDir).absoluteFilePath(parts[1].trimmed()));
    if (file.open(mode | QIODevice::Text)) {
        file.write(process.readAllStandardOutput());
        file.close();
    }
}

bool TerminalWidget::handleEasterEgg(const QString& command) {
    QString lower = command.toLower().trimmed();
    if (lower == "game" || lower == "play" || lower == "xyzzy"
        || lower == "上上下下左右左右ba") {
        appendOutput("启动扫雷...\n", ACCENT_COLOR);
        executeInternalCommand("mine");
        return true;
    }
    return false;
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
    for (const QString& c : customCommands.keys()) cmds.append(c);
    for (const QString& c : cmds) {
        if (c.startsWith(current)) { inputField->setText(c); return; }
    }
}