#include "terminal_ui.h"
#include "games/minesweeper.h"
#include "customcmd/custom_commands_dialog.h"

#include <QApplication>
#include <QClipboard>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QFontDialog>
#include <QScrollBar>
#include <QDir>
#include <QStandardPaths>
#include <QDateTime>
#include <QThread>
#include <QRegularExpression>
#include <QDesktopServices>
#include <QToolBar>
#include <QToolButton>
#include <QScreen>
#include <QTimer>

// ============ 构造函数 ============
TerminalUI::TerminalUI(bool adminMode, QWidget* parent)
    : QMainWindow(parent), isAdminMode(adminMode), historyIndex(0), currentProcess(nullptr) {

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
    initCommands();
    setupMenus();
    setupUI();
    setupTheme();
    displayWelcome();

        QTimer::singleShot(100, [this]() {
        inputField->setFocus();
        menuBar->setNativeMenuBar(false);
    });
    
}
TerminalUI::~TerminalUI() {
    saveCustomCommands();
    delete cmdRegistry;
}

// ============ UI设置 ============
void TerminalUI::setupUI() {
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(5);

    // 输出区域
    outputArea = new QTextEdit(this);
    outputArea->setReadOnly(true);
    outputArea->setFont(TERMINAL_FONT);
    outputArea->setStyleSheet(
        QString("QTextEdit { background-color: %1; color: %2; border: none; }")
            .arg(THEME.outputBg.name(), THEME.fg.name())
    );
    outputArea->setUndoRedoEnabled(false);
    mainLayout->addWidget(outputArea, 1);

       // 输入区域（竖屏分行）
    QVBoxLayout* inputLayout = new QVBoxLayout();
    inputLayout->setSpacing(2);

    QHBoxLayout* promptRow = new QHBoxLayout();
    promptRow->setSpacing(0);

    promptLabel = new QLabel(this);
    promptLabel->setFont(TERMINAL_FONT);
    promptLabel->setStyleSheet(
        QString("QLabel { color: %1; background: transparent; }").arg(THEME.green.name())
    );
    updatePrompt();
    promptRow->addWidget(promptLabel);
    promptRow->addStretch();

    stopBtn = new QPushButton("⏹", this);
    stopBtn->setFixedSize(30, 30);
    stopBtn->setStyleSheet(
        "QPushButton { background-color: #d32f2f; color: white; border: none; border-radius: 3px; font-weight: bold; }"
        "QPushButton:hover { background-color: #f44336; }"
    );
    stopBtn->setToolTip("强行停止当前进程");
    promptRow->addWidget(stopBtn);
    connect(stopBtn, &QPushButton::clicked, this, &TerminalUI::onStopProcess);

    inputLayout->addLayout(promptRow);

    inputField = new QLineEdit(this);
    inputField->setFont(TERMINAL_FONT);
    inputField->setStyleSheet(
        QString("QLineEdit { background-color: %1; color: %2; border: none; padding: 4px; }")
            .arg(THEME.inputBg.name(), THEME.fg.name())
    );
    inputLayout->addWidget(inputField);

    mainLayout->addLayout(inputLayout);

    connect(inputField, &QLineEdit::returnPressed, this, &TerminalUI::onCommandEntered);
}

void TerminalUI::setupMenus() {
    menuBar = new QMenuBar(this);
    setMenuBar(menuBar);
    menuBar->setNativeMenuBar(false);
    menuBar->setStyleSheet(
        "QMenuBar { background: #2b2b2b; color: white; font-size: 14px; padding: 4px; }"
        "QMenuBar::item { padding: 4px 12px; border-radius: 3px; }"
        "QMenuBar::item:selected { background: #4a4a4a; }"
        "QMenu { background: #2b2b2b; color: white; border: 1px solid #555; }"
        "QMenu::item { padding: 6px 20px; }"
        "QMenu::item:selected { background: #4a4a4a; }"
    );

    // 文件
    QMenu* fileMenu = menuBar->addMenu("文件");
    QAction* a1 = fileMenu->addAction("新建窗口");
    connect(a1, &QAction::triggered, this, &TerminalUI::newWindow);
    QAction* a2 = fileMenu->addAction("打开目录");
    connect(a2, &QAction::triggered, this, &TerminalUI::openDirectory);
    fileMenu->addSeparator();
    QAction* a3 = fileMenu->addAction("退出");
    connect(a3, &QAction::triggered, this, &QMainWindow::close);

    // 编辑
    QMenu* editMenu = menuBar->addMenu("编辑");
    QAction* a4 = editMenu->addAction("复制");
    connect(a4, &QAction::triggered, this, &TerminalUI::copyText);
    QAction* a5 = editMenu->addAction("粘贴");
    connect(a5, &QAction::triggered, this, &TerminalUI::pasteText);
    QAction* a6 = editMenu->addAction("清屏");
    connect(a6, &QAction::triggered, this, &TerminalUI::clearScreen);

    // 视图
    QMenu* viewMenu = menuBar->addMenu("视图");
    QAction* a7 = viewMenu->addAction("字体设置");
    connect(a7, &QAction::triggered, this, &TerminalUI::fontSettings);

    // 工具
    QMenu* toolsMenu = menuBar->addMenu("工具");
    QAction* a8 = toolsMenu->addAction("计算器");
    connect(a8, &QAction::triggered, this, &TerminalUI::openCalculator);
    QAction* a9 = toolsMenu->addAction("自定义命令");
    connect(a9, &QAction::triggered, this, &TerminalUI::openCustomCommandsDialog);
    QAction* a10 = toolsMenu->addAction("命令历史");
    connect(a10, &QAction::triggered, this, &TerminalUI::showHistoryWindow);
    toolsMenu->addSeparator();
    QAction* a11 = toolsMenu->addAction("小游戏");
    connect(a11, &QAction::triggered, this, &TerminalUI::openMinesweeper);

    // 帮助
    QMenu* helpMenu = menuBar->addMenu("帮助");
    QAction* a12 = helpMenu->addAction("命令帮助");
    connect(a12, &QAction::triggered, this, &TerminalUI::showHelp);
    QAction* a13 = helpMenu->addAction("快捷键");
    connect(a13, &QAction::triggered, this, &TerminalUI::showShortcuts);
    QAction* a14 = helpMenu->addAction("关于");
    connect(a14, &QAction::triggered, this, &TerminalUI::showAbout);

    // 键盘快捷键
    QAction* upAction = new QAction(this);
    upAction->setShortcut(Qt::Key_Up);
    connect(upAction, &QAction::triggered, this, &TerminalUI::onHistoryUp);
    addAction(upAction);

    QAction* downAction = new QAction(this);
    downAction->setShortcut(Qt::Key_Down);
    connect(downAction, &QAction::triggered, this, &TerminalUI::onHistoryDown);
    addAction(downAction);

    QAction* tabAction = new QAction(this);
    tabAction->setShortcut(Qt::Key_Tab);
    connect(tabAction, &QAction::triggered, this, &TerminalUI::onTabComplete);
    addAction(tabAction);

        // Android 强制弹出修复
    connect(fileMenu, &QMenu::aboutToShow, fileMenu, [fileMenu](){ fileMenu->setMinimumWidth(120); });
    connect(editMenu, &QMenu::aboutToShow, editMenu, [editMenu](){ editMenu->setMinimumWidth(120); });
    connect(viewMenu, &QMenu::aboutToShow, viewMenu, [viewMenu](){ viewMenu->setMinimumWidth(120); });
    connect(toolsMenu, &QMenu::aboutToShow, toolsMenu, [toolsMenu](){ toolsMenu->setMinimumWidth(120); });
    connect(helpMenu, &QMenu::aboutToShow, helpMenu, [helpMenu](){ helpMenu->setMinimumWidth(120); });

    // 修复菜单第二次打不开：每次显示前强制重置
    QList<QMenu*> allMenus = {fileMenu, editMenu, viewMenu, toolsMenu, helpMenu};
    for (QMenu* m : allMenus) {
        QObject::connect(m, &QMenu::aboutToShow, m, [m]() {
            m->setMinimumWidth(120);
            m->setVisible(true);
            m->raise();
        });
    }
}
void TerminalUI::setupTheme() {
    setStyleSheet(
        QString("QMainWindow { background-color: %1; }").arg(THEME.bg.name())
    );
}

// ============ 命令初始化 ============
void TerminalUI::initCommands() {
    cmdRegistry->registerAllBuiltins(this);
    // 注册自定义命令
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

void TerminalUI::loadCustomCommands() {
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
                         + "/termail/custom_commands.json";
    QFile file(configPath);
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        QJsonObject obj = doc.object();
        for (auto it = obj.begin(); it != obj.end(); ++it) {
            customCommands[it.key()] = it.value().toObject();
        }
        file.close();
    }
}

void TerminalUI::saveCustomCommands() {
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
                         + "/termail/custom_commands.json";
    QDir().mkpath(QFileInfo(configPath).absolutePath());
    QFile file(configPath);
    if (file.open(QIODevice::WriteOnly)) {
        QJsonObject obj;
        for (auto it = customCommands.begin(); it != customCommands.end(); ++it) {
            obj[it.key()] = it.value();
        }
        file.write(QJsonDocument(obj).toJson());
        file.close();
    }
}

// ============ 欢迎信息 ============
void TerminalUI::displayWelcome() {
    QString mode = isAdminMode ? "管理员模式" : "受限模式";
    QString welcome = QString(
        "     🚀 Termail终端 v%1         \n"
        "     %2                  \n"
        "     输入 'help' 查看所有命令                \n"
        "     输入 'exit' 退出终端                    \n"
        "     作者:rivers 晟 github:rivers-sheng\n"
        "     邮箱:k-jiangsheng@outlook.com\n"
    ).arg(APP_VERSION, QString("%1").arg(mode, -36, QChar(' ')));
    appendOutput(welcome, THEME.cyan.name());
    if (!isAdminMode) {
        appendOutput("⚠️ 提示: 以管理员权限运行可获得完整网络命令功能\n", THEME.yellow.name());
    }
}

// ============ 输出 ============
void TerminalUI::appendOutput(const QString& text, const QString& color) {
    QString html;
    if (color != "white" && !color.isEmpty()) {
        html = QString("<span style='color:%1;'>%2</span>").arg(color, text.toHtmlEscaped());
    } else {
        html = text.toHtmlEscaped();
    }
    html.replace("\n", "<br>");
    outputArea->moveCursor(QTextCursor::End);
    outputArea->insertHtml(html);
    QScrollBar* sb = outputArea->verticalScrollBar();
    sb->setValue(sb->maximum());
}

void TerminalUI::clearScreen() {
    outputArea->clear();
}

void TerminalUI::updatePrompt() {
    QString home = QDir::homePath();
    QString path = currentDir;
    if (path.startsWith(home)) {
        path.replace(home, "~");
    }
    QString adminChar = isAdminMode ? "#" : "$";
    promptLabel->setText(QString("%1@%2:%3%4 ").arg(username, hostname, path, adminChar));
}

// ============ 命令执行 ============
void TerminalUI::onCommandEntered() {
    QString command = inputField->text().trimmed();
    inputField->clear();

    if (command.isEmpty()) return;

    appendOutput(promptLabel->text() + command + "\n");
    history.append(command);
    historyIndex = history.size();

    // 彩蛋检测
    if (handleEasterEgg(command)) {
        updatePrompt();
        return;
    }

    try {
        executeInternalCommand(command);
    } catch (const std::exception& e) {
        appendOutput(QString("错误: %1\n").arg(e.what()), THEME.red.name());
    }

    updatePrompt();
    inputField->setFocus();
}

void TerminalUI::executeInternalCommand(const QString& commandLine) {
    // 管道处理
    if (commandLine.contains('|')) {
        executePipeline(commandLine);
        return;
    }

    // 重定向处理
    if (commandLine.contains('>')) {
        executeRedirect(commandLine);
        return;
    }

    QStringList parts = commandLine.trimmed().split(QRegularExpression("\\s+"));
    if (parts.isEmpty()) return;

    QString cmd = parts[0].toLower();
    QStringList args = parts.mid(1);

    // 别名展开
    if (aliases.contains(cmd)) {
        QString expanded = aliases[cmd] + " " + args.join(' ');
        parts = expanded.trimmed().split(QRegularExpression("\\s+"));
        cmd = parts[0].toLower();
        args = parts.mid(1);
    }

    // 执行内置命令
    if (cmdRegistry->execute(cmd, args)) {
        return;
    }

    // 执行外部命令
    Utils::runSystemCommand(commandLine, currentDir, this);
}

void TerminalUI::executePipeline(const QString& commandLine) {
    QStringList commands = commandLine.split('|');
    QString prevOutput;
    QString lastError;

    for (int i = 0; i < commands.size(); ++i) {
        QString cmd = commands[i].trimmed();
        QProcess process;
        process.setWorkingDirectory(currentDir);
        if (i == 0) {
            process.start("sh", QStringList() << "-c" << cmd);
        } else {
            process.start("sh", QStringList() << "-c" << cmd);
            process.write(prevOutput.toUtf8());
            process.closeWriteChannel();
        }
        process.waitForFinished(10000);
        prevOutput = process.readAllStandardOutput();
        if (i == commands.size() - 1) {
            lastError = process.readAllStandardError();
        }
    }

    if (!prevOutput.isEmpty()) {
        appendOutput(prevOutput);
    }
    if (!lastError.isEmpty()) {
        appendOutput(lastError, THEME.red.name());
    }
}

void TerminalUI::executeRedirect(const QString& commandLine) {
    bool append = commandLine.contains(">>");
    QStringList parts = commandLine.split(append ? ">>" : ">");
    if (parts.size() < 2) return;

    QString cmd = parts[0].trimmed();
    QString filename = parts[1].trimmed();
    QString filePath = QDir(currentDir).absoluteFilePath(filename);

    QProcess process;
    process.setWorkingDirectory(currentDir);
    process.start("sh", QStringList() << "-c" << cmd);
    process.waitForFinished(10000);

    QIODevice::OpenMode mode = append ? QIODevice::Append : QIODevice::WriteOnly;
    QFile file(filePath);
    if (file.open(mode | QIODevice::Text)) {
        file.write(process.readAllStandardOutput());
        file.close();
    }

    QString stderr = process.readAllStandardError();
    if (!stderr.isEmpty()) {
        appendOutput(stderr, THEME.red.name());
    }
}

bool TerminalUI::handleEasterEgg(const QString& command) {
    QString lower = command.toLower().trimmed();
    if (EASTER_EGG_GAME.contains(lower)) {
        appendOutput("启动扫雷游戏...\n", THEME.yellow.name());
        openMinesweeper();
        return true;
    }
    if (lower == EASTER_EGG_KONAMI) {
        appendOutput("🎮 科乐美秘技！\n", THEME.yellow.name());
        openMinesweeper();
        return true;
    }
    return false;
}

// ============ 历史导航 ============
void TerminalUI::onHistoryUp() {
    if (!history.isEmpty() && historyIndex > 0) {
        historyIndex--;
        inputField->setText(history[historyIndex]);
    }
}

void TerminalUI::onHistoryDown() {
    if (historyIndex < history.size() - 1) {
        historyIndex++;
        inputField->setText(history[historyIndex]);
    } else {
        historyIndex = history.size();
        inputField->clear();
    }
}

void TerminalUI::onTabComplete() {
    QString current = inputField->text();
    if (current.isEmpty()) return;

    // 命令补全
    QStringList allCommands = cmdRegistry->getCommandNames();
    for (const QString& key : customCommands.keys()) {
        allCommands.append(key);
    }
    QStringList matches;
    for (const QString& cmd : allCommands) {
        if (cmd.startsWith(current)) {
            matches.append(cmd);
        }
    }
    if (!matches.isEmpty()) {
        inputField->setText(matches.first());
        return;
    }

    // 文件路径补全
    QDir dir(currentDir);
    QStringList files = dir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot);
    for (const QString& f : files) {
        if (f.startsWith(current)) {
            QString completion = f;
            if (QFileInfo(dir.absoluteFilePath(f)).isDir()) {
                completion += "/";
            }
            inputField->setText(completion);
            return;
        }
    }
}

// ============ 菜单动作 ============
void TerminalUI::newWindow() {
#ifdef Q_OS_ANDROID
    appendOutput("Android 不支持多窗口\n", THEME.yellow.name());
#else
    TerminalUI* newTerm = new TerminalUI(isAdminMode);
    newTerm->setWindowTitle(windowTitle());
    newTerm->resize(900, 600);
    newTerm->show();
#endif
}

void TerminalUI::openDirectory() {
    QString dir = QFileDialog::getExistingDirectory(this, "选择目录", currentDir);
    if (!dir.isEmpty()) {
        previousDir = currentDir;
        currentDir = dir;
        QDir::setCurrent(currentDir);
        updatePrompt();
        appendOutput(QString("切换到目录: %1\n").arg(dir), THEME.green.name());
    }
}

void TerminalUI::copyText() {
    QString selected = outputArea->textCursor().selectedText();
    if (!selected.isEmpty()) {
        QApplication::clipboard()->setText(selected);
    }
}

void TerminalUI::pasteText() {
    QString text = QApplication::clipboard()->text();
    if (!text.isEmpty()) {
        inputField->insert(text);
    }
}

void TerminalUI::fontSettings() {
    bool ok;
    QFont font = QFontDialog::getFont(&ok, TERMINAL_FONT, this, "字体设置");
    if (ok) {
        outputArea->setFont(font);
        promptLabel->setFont(font);
        inputField->setFont(font);
    }
}

void TerminalUI::openMinesweeper() {
    appendOutput("启动扫雷游戏...\n", THEME.yellow.name());
    MinesweeperGame* game = new MinesweeperGame(this);
    game->setAttribute(Qt::WA_DeleteOnClose);
    game->show();
}

void TerminalUI::openCalculator() {
    QDialog* calc = new QDialog(this);
    calc->setWindowTitle("计算器");
    calc->setFixedSize(300, 380);
    calc->setStyleSheet(QString("background-color: %1;").arg(THEME.bg.name()));

    QVBoxLayout* mainLayout = new QVBoxLayout(calc);
    QLineEdit* display = new QLineEdit(calc);
    display->setReadOnly(true);
    display->setAlignment(Qt::AlignRight);
    display->setFont(QFont(TERMINAL_FONT.family(), 18));
    display->setStyleSheet(QString("background-color: %1; color: %2; border: 1px solid #555; padding: 8px;")
        .arg(THEME.inputBg.name(), THEME.fg.name()));
    display->setText("0");
    mainLayout->addWidget(display);

    QGridLayout* grid = new QGridLayout();
    QStringList buttons = {"C","(",")","/","7","8","9","*","4","5","6","-","1","2","3","+","0",".","=",""};
    QString btnStyle = "QPushButton{background:#4a4a4a;color:#fff;font-size:16px;border:1px solid #555;border-radius:4px}";
    QString opStyle = "QPushButton{background:#ff9800;color:#fff;font-size:16px;border:1px solid #555;border-radius:4px}";
    QString eqStyle = "QPushButton{background:#4caf50;color:#fff;font-size:16px;border:1px solid #555;border-radius:4px}";
    QStringList ops = {"+","-","*","/"};
    double* cur = new double(0);
    QString* op = new QString("");
    bool* clr = new bool(false);

    for (int i = 0; i < buttons.size(); ++i) {
        QString t = buttons[i];
        if (t.isEmpty()) continue;
        QPushButton* btn = new QPushButton(t, calc);
        if (t == "=") btn->setStyleSheet(eqStyle);
        else if (ops.contains(t)) btn->setStyleSheet(opStyle);
        else btn->setStyleSheet(btnStyle);
        grid->addWidget(btn, i/4, i%4);

        QObject::connect(btn, &QPushButton::clicked, [display, t, cur, op, clr, ops]() {
            QString c = display->text();
            if (t == "C") { display->setText("0"); *cur=0; *op=""; *clr=false; }
            else if (t == "=") {
                double r = *cur, right = c.toDouble();
                if (*op=="+") r+=right; else if (*op=="-") r-=right;
                else if (*op=="*") r*=right; else if (*op=="/") r=(right!=0)?r/right:0;
                else r=right;
                display->setText(QString::number(r,'g',12));
                *cur=r; *op=""; *clr=true;
            } else if (ops.contains(t)) {
                *cur=c.toDouble(); *op=t; *clr=true;
            } else {
                if (*clr||c=="0"){display->setText(t);*clr=false;}
                else display->setText(c+t);
            }
        });
    }
    mainLayout->addLayout(grid);
    calc->setLayout(mainLayout);
    calc->setAttribute(Qt::WA_DeleteOnClose);
    calc->show();
}

void TerminalUI::openCustomCommandsDialog() {
    CustomCommandsDialog* dialog = new CustomCommandsDialog(this, this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}

void TerminalUI::showHistoryWindow() {
    QStringList reversed = history;
    QString text;
    int start = qMax(0, history.size() - 100);
    for (int i = start; i < history.size(); ++i) {
        text += QString("%1  %2\n").arg(i + 1, 5).arg(history[i]);
    }

    QDialog* dlg = new QDialog(this);
    dlg->setWindowTitle("命令历史");
    dlg->resize(600, 400);
    dlg->setStyleSheet(QString("background-color: %1;").arg(THEME.bg.name()));

    QTextEdit* textEdit = new QTextEdit(dlg);
    textEdit->setReadOnly(true);
    textEdit->setFont(TERMINAL_FONT);
    textEdit->setStyleSheet(QString("color: %1; background-color: %2;").arg(THEME.fg.name(), THEME.outputBg.name()));
    textEdit->setPlainText(text);

    QVBoxLayout* layout = new QVBoxLayout(dlg);
    layout->addWidget(textEdit);
    dlg->setLayout(layout);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->show();
}

void TerminalUI::showHelp() {
    QString help = R"(
🌐 真实网络命令:
  ping, curl, wget, ifconfig, ipconfig, netstat, nslookup, tracert, route, arp, whois

⚙️ 进程管理:
  ps, top, tasklist, taskkill, kill, jobs

📁 文件操作:
  ls, cd, pwd, mkdir, rmdir, rm, touch, cat, cp, mv, rename, head, tail, more, diff, sort, uniq, cut, stat, file, du, df, ln, chmod, which, find, grep, wc, tree

📝 文本处理:
  echo, tr, rev, tac, nl, paste, split, join, fmt, sed

🔧 其他:
  help, exit, clear, date, whoami, env, sysinfo, python, pip, git, calc, bc, random, cowsay, fortune, prime, factor, fibonacci, pi, alias, history, hash, base64, uuid, sleep, watch
)";
    QMessageBox::information(this, "命令帮助", help.trimmed());
}

void TerminalUI::showShortcuts() {
    QMessageBox::information(this, "快捷键",
        "Enter: 执行命令\n"
        "Up/Down: 历史\n"
        "Tab: 自动完成\n"
        "| : 管道\n"
        "> : 重定向\n"
        "game: 扫雷游戏\n"
        "Ctrl+L: 清屏\n"
        "Ctrl+N: 新建窗口");
}

void TerminalUI::showAbout() {
    QMessageBox::about(this, "关于",
        QString("Termail终端 v%1\n\n"
                "支持真实系统命令\n"
                "300+ 命令 | 图形界面 | 扫雷彩蛋\n\n"
                "框架: Qt6 Widgets\n"
                "作者:rivers 晟 github:rivers-sheng\n"
                "邮箱:k-jiangsheng@outlook.com\n"
                "平台: Windows / Linux / Android")
            .arg(APP_VERSION));
}

void TerminalUI::onStopProcess() {
    if (currentProcess && currentProcess->state() != QProcess::NotRunning) {
        currentProcess->kill();
        currentProcess = nullptr;
        appendOutput("\n[已强行停止]\n", THEME.red.name());
    }
}


