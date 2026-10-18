#include "terminal.h"
#include "utils/utils.h"
#include "commands/commands.h"

#include <QApplication>
#include <QScrollBar>
#include <QDir>
#include <QStandardPaths>
#include <QDateTime>
#include <QRegularExpression>
#include <QJsonDocument>
#include <QTimer>
#include <QFileInfo>
#include <QStorageInfo>
#ifdef Q_OS_ANDROID
#include <QJniObject>
#endif

TerminalWidget::TerminalWidget(QWidget* parent)
    : QWidget(parent), historyIndex(0), currentProcess(nullptr),
      tabMatchIndex(0), interactiveMode(false), interactiveProcess(nullptr),
      currentSessionId(1), nextSessionId(2) {

    currentDir = QDir::currentPath();
#ifdef Q_OS_WIN
    username = qgetenv("USERNAME");
#else
    username = qgetenv("USER");
#endif
    if (username.isEmpty()) username = "user";
    hostname = QSysInfo::machineHostName();
    if (hostname.isEmpty()) hostname = "localhost";

    // 创建初始会话
    Session s;
    s.id = 1;
    s.name = "main";
    s.currentDir = currentDir;
    s.process = nullptr;
    s.interactiveMode = false;
    s.interactiveProcess = nullptr;
    sessions[1] = s;

    cmdRegistry = new CommandRegistry(this);
    loadCustomCommands();
    loadAliases();
    setupUI();
    initCommands();
    setupShortcuts();
    displayWelcome();
    loadFontSettings();
}

TerminalWidget::~TerminalWidget() {
    Network2Cleanup::cleanup();
    if (statusTimer) {
        statusTimer->stop();
        delete statusTimer;
    }
    if (currentProcess && currentProcess->state() != QProcess::NotRunning) {
        currentProcess->kill();
        currentProcess->waitForFinished(500);
    }
    saveCustomCommands();
    saveAliases();
    saveFontSettings();
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

    // 状态栏
    statusLabel = new QLabel(this);
    statusLabel->setFont(QFont(FONT_FAMILY, FONT_SIZE - 2));
    statusLabel->setStyleSheet(
        QString("QLabel { color: %1; background: %2; padding: 2px 6px; }")
            .arg(INFO_COLOR.name(), "#111111")
    );
    statusLabel->setFixedHeight(22);
    mainLayout->addWidget(statusLabel);

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

    // 状态栏定时刷新
    statusTimer = new QTimer(this);
    connect(statusTimer, &QTimer::timeout, this, &TerminalWidget::updateStatusBar);
    statusTimer->start(2000);
    updateStatusBar();
}

void TerminalWidget::updateStatusBar() {
    // 内存信息
    QStorageInfo storage("/data");
    if (!storage.isValid()) storage = QStorageInfo("/");
    QString memInfo = QString("磁盘:%1/%2")
        .arg(Utils::formatSize(storage.bytesAvailable()),
             Utils::formatSize(storage.bytesTotal()));

    // 会话信息
    QString sessionInfo = QString("会话:%1").arg(currentSessionId);

    // 目录
    QString dir = currentDir;
    if (dir.length() > 40) dir = "..." + dir.right(37);

    statusLabel->setText(QString(" %1  |  %2  |  %3")
        .arg(dir, memInfo, sessionInfo));
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
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) return;
    QJsonObject obj = doc.object();
    for (auto it = obj.begin(); it != obj.end(); ++it)
        if (it.value().isObject())
            customCommands[it.key()] = it.value().toObject();
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

void TerminalWidget::loadAliases() {
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
                   + "/termail/aliases.json";
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    QJsonObject obj = doc.object();
    for (auto it = obj.begin(); it != obj.end(); ++it)
        aliases[it.key()] = it.value().toString();
}

void TerminalWidget::saveAliases() {
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
                   + "/termail/aliases.json";
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        QJsonObject obj;
        for (auto it = aliases.begin(); it != aliases.end(); ++it)
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
        "  输入 'help' 查看所有命令\n"
        "  制作:rivers晟 github:jiang-sheng\n"
        "  辅助制作:deepseek chat\n"
        "  邮箱:k-jiangsheng@outlook.com\n"
        "  ©rivers晟 GNU GENERAL PUBLIC LICENSE\n"
        "  输入 'session new' 创建新会话\n"
        "  输入 'exit' 退出\n\n"
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

    // 只有在底部时才自动滚动
    if (atBottom) sb->setValue(sb->maximum());
}

void TerminalWidget::clearScreen() {
    outputArea->clear();
}

void TerminalWidget::updatePrompt() {
    QString home = QDir::homePath();
    QString path = currentDir;
    if (path.startsWith(home)) path.replace(home, "~");
    promptLabel->setText(QString("[%1] %2:%3$ ").arg(currentSessionId).arg(username, path));
}

void TerminalWidget::onCommandEntered() {
    QString command = inputField->text().trimmed();
    inputField->clear();

    if (interactiveMode && interactiveProcess) {
        interactiveProcess->write((command + "\n").toUtf8());
        appendOutput(command + "\n");
        return;
    }

    if (command.isEmpty()) {
        appendOutput(promptLabel->text() + "\n");
        return;
    }

        // 记录命令到日志
    logCommand(command);

    appendOutput(promptLabel->text() + command + "\n");
    if (history.size() >= 1000) history.removeFirst();
    history.append(command);
    historyIndex = history.size();

    if (handleEasterEgg(command)) {
        updatePrompt();
        return;
    }

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
    updateStatusBar();
    inputField->setFocus();

    #ifdef Q_OS_ANDROID
    vibrate(50);
    #endif
}

void TerminalWidget::executeInternalCommand(const QString& commandLine, int depth) {
    if (depth > 10) {
        appendOutput("递归深度超限\n", ERROR_COLOR);
        return;
    }

    // 高危命令确认
    QStringList dangerous = {
        "rm -rf", "rm -fr", "dd if=", "dd of=", "mkfs", "fdisk",
        "shred", "poweroff", "reboot", "shutdown", "halt",
        "wipefs", "mkswap", "swapoff", "chmod 777", "chown root"
    };
    for (const QString& d : dangerous) {
        if (commandLine.startsWith(d)) {
            appendOutput(QString("高危命令: %1\n").arg(commandLine), ERROR_COLOR);
            appendOutput("输入 confirm 执行\n", ERROR_COLOR);
            pendingCommand = commandLine;
            return;
        }
    }

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
        for (const QChar& ch : expandedLine)
            if (ch == '"' || ch == '\'') inQ = !inQ;
        if (expandedLine.contains('|') && !inQ) { executePipeline(expandedLine); return; }
        if (expandedLine.contains('>') && !inQ) { executeRedirect(expandedLine); return; }
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

    // 命令未找到，给出建议
    QString suggestion = suggestCommand(cmd);
    if (!suggestion.isEmpty()) {
        appendOutput(QString("命令未找到: %1\n").arg(cmd), ERROR_COLOR);
        appendOutput(QString("你是不是想输入: %1\n").arg(suggestion), INFO_COLOR);
    } else {
        appendOutput(QString("命令未找到: %1 (输入 help 查看)\n").arg(cmd), ERROR_COLOR);
    }
    Utils::runSystemCommand(expandedLine, currentDir, this);
}

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
            process.waitForBytesWritten(5000);
            process.closeWriteChannel();
        }
        process.waitForFinished(-1);
        prevOutput = process.readAllStandardOutput();
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
            p.waitForFinished(-1);
            pipeOutput = p.readAllStandardOutput();
        }
        stdoutText = pipeOutput;
    } else {
        QProcess process;
        process.setWorkingDirectory(currentDir);
        process.start("sh", QStringList() << "-c" << cmd);
        process.waitForFinished(-1);
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
    Q_UNUSED(command);
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
    if (current.contains(' ')) {
        QStringList parts = current.split(' ');
        QString pathPart = parts.last();
        QString dirPath = currentDir;
        QString prefix = "";
        if (pathPart.contains('/')) {
            int slashIdx = pathPart.lastIndexOf('/');
            dirPath = QDir(currentDir).absoluteFilePath(pathPart.left(slashIdx + 1));
            prefix = pathPart.mid(slashIdx + 1);
        } else prefix = pathPart;
        QDir dir(dirPath);
        QStringList files = dir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot);
        QStringList matches;
        for (const QString& f : files)
            if (f.startsWith(prefix, Qt::CaseInsensitive)) matches.append(f);
        if (!matches.isEmpty()) {
            QString base = pathPart.contains('/') ? pathPart.left(pathPart.lastIndexOf('/') + 1) : "";
            QString completion = matches.first();
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
        inputField->setText(matches[tabMatchIndex % matches.size()]);
        tabMatchIndex++;
        if (matches.size() > 1) appendOutput(matches.join("  ") + "\n", INFO_COLOR);
    }
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
        appendOutput("\n[会话结束]\n", INFO_COLOR);
        updatePrompt();
    });
    interactiveProcess->start("sh", QStringList() << "-c" << command);
    appendOutput("[交互模式，exit 退出]\n", ACCENT_COLOR);
}

QStringList TerminalWidget::getHistory() const { return history; }

QString TerminalWidget::suggestCommand(const QString& cmd) {
    QStringList all = cmdRegistry->getCommandNames();
    for (const QString& c : all) {
        if (c.length() == cmd.length()) {
            int diff = 0;
            for (int i = 0; i < c.length(); ++i)
                if (c[i] != cmd[i]) diff++;
            if (diff == 1) return c;
        }
        if (c.startsWith(cmd) && cmd.length() >= 2 && c != cmd) return c;
    }
    return "";
}

// ============ 会话管理 ============

void TerminalWidget::createSession(const QString& name) {
    int id = nextSessionId++;
    Session s;
    s.id = id;
    s.name = name.isEmpty() ? QString("session%1").arg(id) : name;
    s.currentDir = currentDir;
    s.process = nullptr;
    s.interactiveMode = false;
    s.interactiveProcess = nullptr;
    sessions[id] = s;

    appendOutput(QString("会话 %1 (%2) 已创建\n").arg(id).arg(s.name), PROMPT_COLOR);
    switchSession(id);
}

void TerminalWidget::switchSession(int id) {
    if (!sessions.contains(id)) {
        appendOutput(QString("会话 %1 不存在\n").arg(id), ERROR_COLOR);
        return;
    }

    // 杀掉当前会话的进程
    if (currentProcess && currentProcess->state() != QProcess::NotRunning) {
        currentProcess->kill();
        currentProcess->waitForFinished(500);
        currentProcess = nullptr;
    }
    if (interactiveProcess && interactiveProcess->state() != QProcess::NotRunning) {
        interactiveProcess->kill();
        interactiveProcess->waitForFinished(500);
        interactiveProcess = nullptr;
        interactiveMode = false;
    }

    // 保存当前会话状态
    if (sessions.contains(currentSessionId)) {
        sessions[currentSessionId].currentDir = currentDir;
        sessions[currentSessionId].history = history;
    }

    currentSessionId = id;
    Session& s = sessions[id];
    currentDir = s.currentDir;
    history = s.history;
    historyIndex = history.size();

    // 清屏并显示会话信息
    outputArea->clear();
    appendOutput(QString("切换到会话 %1 (%2)\n").arg(id).arg(s.name), ACCENT_COLOR);
    appendOutput(QString("目录: %1\n\n").arg(currentDir), INFO_COLOR);

    updatePrompt();
    updateStatusBar();
}

void TerminalWidget::killSession(int id) {
    if (id == currentSessionId) {
        appendOutput("不能删除当前会话\n", ERROR_COLOR);
        return;
    }
    if (!sessions.contains(id)) {
        appendOutput(QString("会话 %1 不存在\n").arg(id), ERROR_COLOR);
        return;
    }
    sessions.remove(id);
    appendOutput(QString("会话 %1 已删除\n").arg(id), INFO_COLOR);
}

void TerminalWidget::listSessions() {
    appendOutput("=== 会话列表 ===\n", ACCENT_COLOR);
    for (auto it = sessions.begin(); it != sessions.end(); ++it) {
        QString marker = (it.key() == currentSessionId) ? " *" : "  ";
        appendOutput(QString("%1 [%2] %3  %4\n")
            .arg(marker).arg(it.key()).arg(it.value().name, it.value().currentDir),
            it.key() == currentSessionId ? PROMPT_COLOR : FG_COLOR);
    }
}

void TerminalWidget::logCommand(const QString& cmd) {
    QString logPath = QDir(currentDir).absoluteFilePath(".termail_cmd.log");
    QFile f(logPath);
    if (f.open(QIODevice::Append | QIODevice::Text)) {
        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
        f.write(QString("[%1] [会话%2] %3\n")
            .arg(timestamp).arg(currentSessionId).arg(cmd).toUtf8());
        f.close();

        // 日志超过1MB时截断
        if (QFileInfo(logPath).size() > 1024 * 1024) {
            QFile old(logPath);
            if (old.open(QIODevice::ReadOnly)) {
                QByteArray data = old.readAll();
                old.close();
                QFile newF(logPath);
                if (newF.open(QIODevice::WriteOnly)) {
                    newF.write(data.right(512 * 1024));
                    newF.close();
                }
            }
        }
    }
}

void TerminalWidget::saveHistory() {
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
                   + "/termail/history.dat";
    QFile f(path);
    if (f.open(QIODevice::WriteOnly)) {
        QByteArray data;
        QDataStream out(&data, QIODevice::WriteOnly);
        out << history;
        // 简单XOR
        for (int i = 0; i < data.size(); ++i)
            data[i] = data[i] ^ 0x5A;
        f.write(data);
        f.close();
    }
}

void TerminalWidget::vibrate(int ms) {
#ifdef Q_OS_ANDROID
    QJniObject activity = QJniObject::callStaticObjectMethod(
        "org/qtproject/qt/android/QtNative", "activity",
        "()Landroid/app/Activity;");
    if (activity.isValid()) {
        QJniObject vibrator = activity.callObjectMethod(
            "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;",
            QJniObject::fromString("vibrator").object());
        if (vibrator.isValid()) {
            vibrator.callMethod<jboolean>("vibrate", "(J)Z", (jlong)ms);
        }
    }
#endif
}

