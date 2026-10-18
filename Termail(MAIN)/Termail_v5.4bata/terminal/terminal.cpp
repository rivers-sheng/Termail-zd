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
    : QWidget(parent), historyIndex(0), currentProcess(nullptr), 
      interactiveProcess(nullptr), interactiveMode(false) {

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
    loadFontSettings();
}

TerminalWidget::~TerminalWidget() {
    saveCustomCommands();
    saveFontSettings();
    delete cmdRegistry;
}

void TerminalWidget::setupUI() {
    setStyleSheet(QString("background-color: %1;").arg(BG_COLOR.name()));

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

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
    for (auto it = customCommands.begin(); it != customCommands.end(); ++it) {
        QString name = it.key();
        if (cmdRegistry->getCommandNames().contains(name)) continue;
        QString cmd = it.value()["command"].toString();
        cmdRegistry->registerCommand(name, [this, cmd](const QStringList& args) {
            QString expanded = cmd;
            for (int i = 0; i < args.size(); ++i)
                expanded.replace(QString("${%1}").arg(i), args[i]);
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
    if (!file.open(QIODevice::ReadOnly)) return;
    
    QByteArray data = file.readAll();
    file.close();
    
    if (data.isEmpty()) return;
    
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        // JSON 损坏，备份后清空
        QFile::copy(path, path + ".bak");
        QFile::remove(path);
        return;
    }
    
    if (!doc.isObject()) return;
    
    QJsonObject obj = doc.object();
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        if (it.value().isObject()) {
            customCommands[it.key()] = it.value().toObject();
        }
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

void TerminalWidget::loadFontSettings() {
    QSettings settings("termail", "terminal");
    int size = settings.value("font_size", FONT_SIZE).toInt();
    if (size >= 8 && size <= 32) {
        QFont f(FONT_FAMILY, size);
        outputArea->setFont(f);
        inputField->setFont(f);
        promptLabel->setFont(f);
    }
}

void TerminalWidget::saveFontSettings() {
    QSettings settings("termail", "terminal");
    settings.setValue("font_size", outputArea->font().pointSize());
}

void TerminalWidget::displayWelcome() {
    QString welcome = QString(
        "\n  Termail终端 v%1\n"
        "  输入 'help' 查看所有命令 输入 'get-busybox' 使用更多linux指令\n"
        "  制作:rivers晟 github:rivers-sheng\n"
        "  邮箱:k-jiangsheng@outlook.com\n"
        "  ©rivers晟 GNU GENERAL PUBLIC LICENSE\n"
        "  输入 'exit' 退出\n\n"
    ).arg(APP_VERSION);
    appendOutput(welcome, ACCENT_COLOR);
}

void TerminalWidget::appendOutput(const QString& text, const QColor& color) {
    outputArea->moveCursor(QTextCursor::End);
    if (color != FG_COLOR) {
        outputArea->insertHtml(QString("<span style='color:%1;'>%2</span>")
            .arg(color.name(), text.toHtmlEscaped().replace("\n", "<br>").replace("'", "&#39;")));
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

    // 交互模式：把输入发给进程
    if (interactiveMode && interactiveProcess) {
        interactiveProcess->write((command + "\n").toUtf8());
        appendOutput(command + "\n");
        return;
    }

    if (command.isEmpty()) {
        appendOutput(promptLabel->text() + "\n");
        return;
    }

    appendOutput(promptLabel->text() + command + "\n");
    if (history.size() >= 1000) history.removeFirst();
    history.append(command);
    historyIndex = history.size();

    if (handleEasterEgg(command)) {
        updatePrompt();
        return;
    }

    // 检测 ssh 命令
    if (command.startsWith("ssh ") || command == "ssh") {
        startInteractiveProcess(command);
        return;
    }

    try {
        executeInternalCommand(command, 0);
    } catch (const std::exception& e) {
        appendOutput(QString("错误: %1\n").arg(e.what()), ERROR_COLOR);
    }

    updatePrompt();
    inputField->setFocus();
}

void TerminalWidget::startInteractiveProcess(const QString& command) {
    interactiveProcess = new QProcess(this);
    interactiveProcess->setWorkingDirectory(currentDir);
    interactiveMode = true;
    
    connect(interactiveProcess, &QProcess::readyReadStandardOutput, [this]() {
        appendOutput(interactiveProcess->readAllStandardOutput());
    });
    connect(interactiveProcess, &QProcess::readyReadStandardError, [this]() {
        appendOutput(interactiveProcess->readAllStandardError(), ERROR_COLOR);
    });
    connect(interactiveProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this](int, QProcess::ExitStatus) {
        interactiveMode = false;
        interactiveProcess->deleteLater();
        interactiveProcess = nullptr;
        appendOutput("\n[SSH会话结束]\n", INFO_COLOR);
        updatePrompt();
    });
    
    interactiveProcess->start("sh", QStringList() << "-c" << command);
    appendOutput("[进入交互模式，输入 exit 退出]\n", ACCENT_COLOR);
    updatePrompt();
}

// 修复 #二：recursionDepth 作为参数传递，只在递归时递增
void TerminalWidget::executeInternalCommand(const QString& commandLine, int depth) {
    if (depth > 10) {
        appendOutput("递归深度超限\n", ERROR_COLOR);
        return;
    }

    // ========== 高危命令确认 ==========
    QStringList dangerous = {"rm -rf", "dd", "mkfs", "fdisk", "shred",
                              "poweroff", "reboot", "shutdown", "su ", "sudo "};
    for (const QString& d : dangerous) {
        if (commandLine.startsWith(d)) {
            appendOutput(QString("⚠️ 高危命令: %1\n").arg(commandLine), ERROR_COLOR);
            appendOutput("输入 confirm 执行，其他取消\n", ERROR_COLOR);
            pendingCommand = commandLine;
            return;
        }
    }
    // ========== 检测结束 ==========

    if (commandLine.contains("&&") || commandLine.contains("||") || commandLine.contains(';')) {
        QStringList parts = commandLine.split(QRegularExpression("(&&|\\|\\||;)"));
        for (const QString& part : parts) {
            QString p = part.trimmed();
            if (!p.isEmpty()) executeInternalCommand(p, depth + 1);
        }
        return;
    }

    bool inQuote = false;
    for (const QChar& ch : commandLine) {
        if (ch == '"' || ch == '\'') inQuote = !inQuote;
    }

    if (commandLine.contains('>') && !inQuote) {
        executeRedirect(commandLine);
        return;
    }
    if (commandLine.contains('|') && !inQuote) {
        executePipeline(commandLine);
        return;
    }

    QStringList parts = commandLine.trimmed().split(QRegularExpression("\\s+"));
    if (parts.isEmpty()) return;

    QString cmd = parts[0].toLower();
    QStringList args = parts.mid(1);

    // 别名展开
    int aliasDepth = 0;
    QString expandedLine = commandLine;
    while (aliases.contains(cmd) && aliasDepth < 5) {
        QString expanded = aliases[cmd] + " " + args.join(' ');
        if (expanded.trimmed() == expandedLine.trimmed()) {
            appendOutput("别名自引用\n", ERROR_COLOR);
            return;
        }
        expandedLine = expanded.trimmed();
        bool inQ = false;
        for (const QChar& ch : expandedLine) {
            if (ch == '"' || ch == '\'') inQ = !inQ;
        }
        if (expandedLine.contains('|') && !inQ) {
            executePipeline(expandedLine);
            return;
        }
        if (expandedLine.contains('>') && !inQ) {
            executeRedirect(expandedLine);
            return;
        }
        parts = expandedLine.split(QRegularExpression("\\s+"));
        cmd = parts[0].toLower();
        args = parts.mid(1);
        aliasDepth++;
    }
    if (aliasDepth >= 5) {
        appendOutput("别名递归过深\n", ERROR_COLOR);
        return;
    }

    if (cmdRegistry->execute(cmd, args)) return;
    Utils::runSystemCommand(expandedLine, currentDir, this);
}

// 修复 #三/#四：管道写入等待+所有stderr收集
void TerminalWidget::executePipeline(const QString& commandLine) {
    QStringList commands = commandLine.split('|');
    QString prevOutput;
    QString allErrors;

    for (int i = 0; i < commands.size(); ++i) {
        QProcess process;
        process.setWorkingDirectory(currentDir);
        process.start("sh", QStringList() << "-c" << commands[i].trimmed());
        if (i > 0) {
            QByteArray data = prevOutput.toUtf8();
            if (data.size() > 10 * 1024 * 1024) data = data.left(10 * 1024 * 1024);
            process.write(data);
            process.waitForBytesWritten(5000); // 修复：等待写完
            process.closeWriteChannel();
        }
        process.waitForFinished(10000);
        prevOutput = process.readAllStandardOutput();
        // 收集所有stderr
        QString err = process.readAllStandardError();
        if (!err.isEmpty()) allErrors += err;
    }
    if (!prevOutput.isEmpty()) {
        if (prevOutput.size() > 100000) prevOutput = prevOutput.left(100000) + "\n...截断";
        appendOutput(prevOutput);
    }
    if (!allErrors.isEmpty()) appendOutput(allErrors, ERROR_COLOR);
}

void TerminalWidget::executeRedirect(const QString& commandLine) {
    bool append = commandLine.contains(">>");
    QString sep = append ? ">>" : ">";
    QStringList parts = commandLine.split(sep);
    if (parts.size() < 2 || parts[1].trimmed().isEmpty()) {
        appendOutput("缺少重定向文件名\n", ERROR_COLOR);
        return;
    }

    QString cmd = parts[0].trimmed();
    QString filename = parts[1].trimmed().remove('"').remove('\'');
    QString filePath = QDir(currentDir).absoluteFilePath(filename);

    if (!append && QFile::exists(filePath)) {
        appendOutput(QString("文件 %1 已存在，将覆盖\n").arg(filename), INFO_COLOR);
    }

    QString stdoutText;
    if (cmd.contains('|')) {
        QStringList pipeParts = cmd.split('|');
        QString pipeOutput;
        for (int i = 0; i < pipeParts.size(); ++i) {
            QProcess p;
            p.setWorkingDirectory(currentDir);
            p.start("sh", QStringList() << "-c" << pipeParts[i].trimmed());
            if (i > 0) {
                p.write(pipeOutput.toUtf8());
                p.waitForBytesWritten(5000);
                p.closeWriteChannel();
            }
            p.waitForFinished(10000);
            pipeOutput = p.readAllStandardOutput();
        }
        stdoutText = pipeOutput;
    } else {
        QProcess process;
        process.setWorkingDirectory(currentDir);
        process.start("sh", QStringList() << "-c" << cmd);
        process.waitForFinished(10000);
        stdoutText = process.readAllStandardOutput();
    }

    QIODevice::OpenMode mode = append ? QIODevice::Append : QIODevice::WriteOnly;
    QFile file(filePath);
    if (file.open(mode | QIODevice::Text)) {
        file.write(stdoutText.toUtf8());
        file.close();
    } else {
        appendOutput("无法写入文件\n", ERROR_COLOR);
    }
}

bool TerminalWidget::handleEasterEgg(const QString& command) {
    QString lower = command.toLower().trimmed();
    if (lower == "game" || lower == "play" || lower == "xyzzy"
        || lower == "上上下下左右左右ba" || lower == "上上下下左右左右BA") {
        appendOutput("启动扫雷...\n", ACCENT_COLOR);
        executeInternalCommand("mine", 0);
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

// 修复 #七/#八：目录补全加/，命令补全循环切换
void TerminalWidget::onTabComplete() {
    QString current = inputField->text();
    if (current.isEmpty()) return;

    if (current.contains(' ')) {
        QStringList parts = current.split(' ');
        QString pathPart = parts.last();
        QString dirPath = currentDir;
        QString prefix = "";
        if (pathPart.contains('/')) {
            int slashIdx = pathPart.lastIndexOf('/');
            dirPath = QDir(currentDir).absoluteFilePath(pathPart.left(slashIdx + 1));
            prefix = pathPart.mid(slashIdx + 1);
        } else {
            prefix = pathPart;
        }
        QDir dir(dirPath);
        QStringList files = dir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot);
        QStringList matches;
        for (const QString& f : files)
            if (f.startsWith(prefix, Qt::CaseInsensitive)) matches.append(f);
        if (!matches.isEmpty()) {
            QString base = pathPart.contains('/') ? pathPart.left(pathPart.lastIndexOf('/') + 1) : "";
            QString completion = matches.first();
            // 修复 #七：目录加/
            if (QFileInfo(dir.absoluteFilePath(completion)).isDir()) completion += "/";
            parts.last() = base + completion;
            inputField->setText(parts.join(' '));
        }
        return;
    }

    QStringList cmds = cmdRegistry->getCommandNames();
    for (const QString& c : customCommands.keys()) cmds.append(c);
    QStringList matches;
    for (const QString& c : cmds)
        if (c.startsWith(current, Qt::CaseInsensitive)) matches.append(c);
    if (!matches.isEmpty()) {
        // 修复 #八：循环切换
        inputField->setText(matches[tabMatchIndex % matches.size()]);
        tabMatchIndex++;
        if (matches.size() > 1)
            appendOutput(matches.join("  ") + "\n", INFO_COLOR);
    }
}