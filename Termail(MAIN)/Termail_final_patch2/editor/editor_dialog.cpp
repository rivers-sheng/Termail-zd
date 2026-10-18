#include <QStringConverter>
#include <QTimer>
#include "editor_dialog.h"
#include "config.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QMessageBox>
#include <QKeyEvent>
#include <QCloseEvent>
#include <QInputDialog>
#include <QFont>
#include <QScreen>
#include <QApplication>

EditorDialog::EditorDialog(const QString& path, QWidget* parent)
    : QDialog(parent), filePath(path), modified(false), lineNumbersVisible(false) {

    setupUI();
    loadFile();
    updateTitle();
}

EditorDialog::~EditorDialog() {}

void EditorDialog::setupUI() {
    setStyleSheet(QString("background-color: %1;").arg(BG_COLOR.name()));

    // 全屏显示
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setModal(false);
    QScreen* screen = QApplication::primaryScreen();
    if (screen) {
        resize(screen->availableSize());
    }

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 顶部工具栏
    QWidget* topBar = new QWidget(this);
    topBar->setStyleSheet("background-color: #2a2a2a;");
    topBar->setFixedHeight(44);
    QHBoxLayout* topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(8, 4, 8, 4);
    topLayout->setSpacing(6);

    // 文件名显示
    QLabel* nameLabel = new QLabel(QFileInfo(filePath).fileName(), topBar);
    nameLabel->setStyleSheet(QString("color: %1; font-size: 14px; font-weight: bold;")
        .arg(ACCENT_COLOR.name()));
    topLayout->addWidget(nameLabel);

    topLayout->addStretch();

    // 查找输入
    searchField = new QLineEdit(topBar);
    searchField->setPlaceholderText("查找...");
    searchField->setStyleSheet(
        "QLineEdit { background: #1a1a1a; color: white; border: 1px solid #444; padding: 4px 8px; border-radius: 3px; }"
    );
    searchField->setFixedWidth(150);
    topLayout->addWidget(searchField);

    // 按钮
    QString btnStyle = "QPushButton { background: #444; color: white; border: none; padding: 6px 14px; border-radius: 3px; }"
                       "QPushButton:hover { background: #555; }";

    findBtn = new QPushButton("查找", topBar);
    findBtn->setStyleSheet(btnStyle);
    connect(findBtn, &QPushButton::clicked, this, &EditorDialog::findText);
    topLayout->addWidget(findBtn);

    replaceBtn = new QPushButton("替换", topBar);
    replaceBtn->setStyleSheet(btnStyle);
    connect(replaceBtn, &QPushButton::clicked, this, &EditorDialog::replaceText);
    topLayout->addWidget(replaceBtn);

    QPushButton* linesBtn = new QPushButton("行号", topBar);
    linesBtn->setStyleSheet(btnStyle);
    connect(linesBtn, &QPushButton::clicked, this, &EditorDialog::toggleLineNumbers);
    topLayout->addWidget(linesBtn);

    saveBtn = new QPushButton("保存", topBar);
    saveBtn->setStyleSheet("QPushButton { background: #4CAF50; color: white; border: none; padding: 6px 14px; border-radius: 3px; }"
                           "QPushButton:hover { background: #45a049; }");
    connect(saveBtn, &QPushButton::clicked, this, &EditorDialog::saveFile);
    topLayout->addWidget(saveBtn);

    closeBtn = new QPushButton("关闭", topBar);
    closeBtn->setStyleSheet("QPushButton { background: #d32f2f; color: white; border: none; padding: 6px 14px; border-radius: 3px; }"
                            "QPushButton:hover { background: #f44336; }");
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::close);
    topLayout->addWidget(closeBtn);

    mainLayout->addWidget(topBar);

    // 文本编辑区
    textEdit = new QTextEdit(this);
    textEdit->setFont(QFont(FONT_FAMILY, FONT_SIZE + 2));
    textEdit->setStyleSheet(
        QString("QTextEdit { background: %1; color: %2; border: none; padding: 8px; "
                "selection-background-color: #444; }")
            .arg(BG_COLOR.name(), FG_COLOR.name())
    );
    textEdit->setLineWrapMode(QTextEdit::NoWrap);
    textEdit->setTabStopDistance(40);
    connect(textEdit, &QTextEdit::textChanged, [this]() {
        if (!modified) {
            modified = true;
            updateTitle();
        }
    });
    mainLayout->addWidget(textEdit, 1);

    // 状态栏
    statusLabel = new QLabel(this);
    statusLabel->setStyleSheet("background: #2a2a2a; color: #888; padding: 4px 8px;");
    statusLabel->setFixedHeight(24);
    mainLayout->addWidget(statusLabel);

    // 定时更新状态
    QTimer* statusTimer = new QTimer(this);
    connect(statusTimer, &QTimer::timeout, [this]() {
        QTextCursor cursor = textEdit->textCursor();
        int line = cursor.blockNumber() + 1;
        int col = cursor.columnNumber() + 1;
        statusLabel->setText(QString("行 %1, 列 %2 | 字符 %3 | %4")
            .arg(line).arg(col)
            .arg(textEdit->toPlainText().size())
            .arg(modified ? "已修改" : "未修改"));
    });
    statusTimer->start(200);

    textEdit->setFocus();
}

void EditorDialog::loadFile() {
    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        in.setEncoding(QStringConverter::Utf8);
        textEdit->setPlainText(in.readAll());
        file.close();
        modified = false;
    } else {
        // 文件不存在，新建
        modified = false;
    }
}

void EditorDialog::saveFile() {
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out.setEncoding(QStringConverter::Utf8);
        out << textEdit->toPlainText();
        file.close();
        modified = false;
        updateTitle();
        statusLabel->setText("已保存");
    } else {
        QMessageBox::warning(this, "错误", "无法保存文件");
    }
}

void EditorDialog::findText() {
    QString text = searchField->text();
    if (text.isEmpty()) return;
    if (!textEdit->find(text)) {
        statusLabel->setText("未找到: " + text);
    } else {
        statusLabel->setText("找到: " + text);
    }
}

void EditorDialog::replaceText() {
    QString findText = searchField->text();
    if (findText.isEmpty()) return;

    bool ok;
    QString replaceText = QInputDialog::getText(this, "替换",
        "替换为:", QLineEdit::Normal, "", &ok);
    if (!ok) return;

    QString content = textEdit->toPlainText();
    int count = content.count(findText);
    content.replace(findText, replaceText);
    textEdit->setPlainText(content);
    statusLabel->setText(QString("替换 %1 处").arg(count));
}

void EditorDialog::toggleLineNumbers() {
    lineNumbersVisible = !lineNumbersVisible;
    // 简化处理：仅切换显示
    updateLineNumbers();
}

void EditorDialog::updateLineNumbers() {
    // 简化版：忽略行号显示（QTextEdit 原生不支持）
    statusLabel->setText(lineNumbersVisible ? "行号: 开" : "行号: 关");
}

void EditorDialog::updateTitle() {
    QString title = QFileInfo(filePath).fileName();
    if (modified) title += " *";
    setWindowTitle(title);
}

bool EditorDialog::maybeSave() {
    if (!modified) return true;

    QMessageBox::StandardButton ret = QMessageBox::question(this, "保存",
        "文件已修改，是否保存？",
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

    if (ret == QMessageBox::Save) {
        saveFile();
        return true;
    } else if (ret == QMessageBox::Cancel) {
        return false;
    }
    return true;
}

void EditorDialog::keyPressEvent(QKeyEvent* event) {
    // Ctrl+S 保存
    if (event->key() == Qt::Key_S && event->modifiers() & Qt::ControlModifier) {
        saveFile();
        return;
    }
    // Ctrl+F 查找
    if (event->key() == Qt::Key_F && event->modifiers() & Qt::ControlModifier) {
        searchField->setFocus();
        searchField->selectAll();
        return;
    }
    // Ctrl+H 替换
    if (event->key() == Qt::Key_H && event->modifiers() & Qt::ControlModifier) {
        replaceText();
        return;
    }
    // ESC 关闭
    if (event->key() == Qt::Key_Escape) {
        close();
        return;
    }
    QDialog::keyPressEvent(event);
}

void EditorDialog::closeEvent(QCloseEvent* event) {
    if (maybeSave()) {
        event->accept();
    } else {
        event->ignore();
    }
}