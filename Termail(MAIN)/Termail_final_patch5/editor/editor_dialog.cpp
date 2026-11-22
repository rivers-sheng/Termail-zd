#include <QStringConverter>
#include <QTimer>
#include "editor_dialog.h"
#include "config.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
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
#include <QScrollBar>
#include <QResizeEvent>

EditorDialog::EditorDialog(const QString& path, QWidget* parent)
    : QDialog(parent), filePath(path), modified(false), lineNumbersVisible(false) {

    setupUI();
    loadFile();
    updateTitle();
}

EditorDialog::~EditorDialog() {}

void EditorDialog::setupUI() {
    setStyleSheet(QString("background-color: %1;").arg(BG_COLOR.name()));

    // 全屏：用 Window + Frameless，再 showFullScreen
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    setModal(false);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // ============ 顶部工具栏 ============
    QWidget* topBar = new QWidget(this);
    topBar->setStyleSheet("background-color: #2a2a2a;");
    QGridLayout* topLayout = new QGridLayout(topBar);
    topLayout->setContentsMargins(8, 4, 8, 4);
    topLayout->setSpacing(6);

    QLabel* nameLabel = new QLabel(QFileInfo(filePath).fileName(), topBar);
    nameLabel->setStyleSheet(QString("color: %1; font-size: 14px; font-weight: bold;").arg(ACCENT_COLOR.name()));
    topLayout->addWidget(nameLabel, 0, 0, 1, 7);

    searchField = new QLineEdit(topBar);
    searchField->setPlaceholderText("查找...");
    searchField->setStyleSheet("QLineEdit { background: #1a1a1a; color: white; border: 1px solid #444; padding: 4px 8px; border-radius: 3px; }");
    topLayout->addWidget(searchField, 1, 0, 1, 2);

    QString btnStyle = "QPushButton { background: #444; color: white; border: none; padding: 6px 14px; border-radius: 3px; }"
                       "QPushButton:hover { background: #555; }";

    findBtn = new QPushButton("查找", topBar);
    findBtn->setStyleSheet(btnStyle);
    connect(findBtn, &QPushButton::clicked, this, &EditorDialog::findText);
    topLayout->addWidget(findBtn, 1, 2);

    replaceBtn = new QPushButton("替换", topBar);
    replaceBtn->setStyleSheet(btnStyle);
    connect(replaceBtn, &QPushButton::clicked, this, &EditorDialog::replaceText);
    topLayout->addWidget(replaceBtn, 1, 3);

    QPushButton* linesBtn = new QPushButton("行号", topBar);
    linesBtn->setStyleSheet(btnStyle);
    connect(linesBtn, &QPushButton::clicked, this, &EditorDialog::toggleLineNumbers);
    topLayout->addWidget(linesBtn, 1, 4);

    saveBtn = new QPushButton("保存", topBar);
    saveBtn->setStyleSheet("QPushButton { background: #4CAF50; color: white; border: none; padding: 6px 14px; border-radius: 3px; } QPushButton:hover { background: #45a049; }");
    connect(saveBtn, &QPushButton::clicked, this, &EditorDialog::saveFile);
    topLayout->addWidget(saveBtn, 1, 5);

    closeBtn = new QPushButton("关闭", topBar);
    closeBtn->setStyleSheet("QPushButton { background: #d32f2f; color: white; border: none; padding: 6px 14px; border-radius: 3px; } QPushButton:hover { background: #f44336; }");
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::close);
    topLayout->addWidget(closeBtn, 1, 6);

    mainLayout->addWidget(topBar);

    // ============ 文本编辑区与行号 ============
    QWidget* editContainer = new QWidget(this);
    QHBoxLayout* editLayout = new QHBoxLayout(editContainer);
    editLayout->setContentsMargins(0, 0, 0, 0);
    editLayout->setSpacing(0);

    // 行号栏：与主编辑区用完全相同的字体和上下边距，保证行高一致
    lineNumberArea = new QPlainTextEdit(editContainer);
    lineNumberArea->setReadOnly(true);
    lineNumberArea->setFixedWidth(44);
    lineNumberArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    lineNumberArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    lineNumberArea->setFont(QFont(FONT_FAMILY, FONT_SIZE + 2));
    lineNumberArea->setStyleSheet(
        "QPlainTextEdit { background: #222; color: #666; border: none;"
        " padding: 8px 4px 8px 0px; }");
    lineNumberArea->setLineWrapMode(QPlainTextEdit::NoWrap);
    lineNumberArea->setFocusPolicy(Qt::NoFocus);
    lineNumberArea->hide();

    // 主编辑区
    textEdit = new QTextEdit(editContainer);
    textEdit->setFont(QFont(FONT_FAMILY, FONT_SIZE + 2));
    textEdit->setStyleSheet(QString("QTextEdit { background: %1; color: %2; border: none; padding: 8px; selection-background-color: #444; }").arg(BG_COLOR.name(), FG_COLOR.name()));
    textEdit->setLineWrapMode(QTextEdit::NoWrap);
    textEdit->setTabStopDistance(40);
    textEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    connect(textEdit, &QTextEdit::textChanged, [this]() {
        if (!modified) {
            modified = true;
            updateTitle();
        }
        if (lineNumbersVisible) updateLineNumbers();
    });

    // 滚动时只同步滚动条，不重建行号文本
    connect(textEdit->verticalScrollBar(), &QScrollBar::valueChanged,
            this, &EditorDialog::syncLineNumberScroll);

    editLayout->addWidget(lineNumberArea);
    editLayout->addWidget(textEdit, 1);

    mainLayout->addWidget(editContainer, 1);

    // 状态栏
    statusLabel = new QLabel(this);
    statusLabel->setStyleSheet("background: #2a2a2a; color: #888; padding: 4px 8px;");
    statusLabel->setFixedHeight(24);
    mainLayout->addWidget(statusLabel);

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
        modified = false;
    }
    if (lineNumbersVisible) updateLineNumbers();
    syncLineNumberScroll();
}

void EditorDialog::toggleLineNumbers() {
    lineNumbersVisible = !lineNumbersVisible;
    lineNumberArea->setVisible(lineNumbersVisible);
    if (lineNumbersVisible) {
        updateLineNumbers();
        syncLineNumberScroll();
    }
    statusLabel->setText(lineNumbersVisible ? "行号: 开" : "行号: 关");
}

void EditorDialog::updateLineNumbers() {
    int lineCount = textEdit->document()->blockCount();
    QString numbers;
    numbers.reserve(lineCount * 4);
    for (int i = 1; i <= lineCount; ++i) {
        numbers += QString::number(i) + "\n";
    }
    lineNumberArea->setPlainText(numbers);
}

void EditorDialog::syncLineNumberScroll() {
    if (!lineNumbersVisible) return;
    // 行号区滚动条被关闭，直接操作其 viewport 的滚动
    lineNumberArea->verticalScrollBar()->setValue(
        textEdit->verticalScrollBar()->value());
}

void EditorDialog::resizeEvent(QResizeEvent* event) {
    QDialog::resizeEvent(event);
    syncLineNumberScroll();
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
    QString replaceText = QInputDialog::getText(this, "替换", "替换为:", QLineEdit::Normal, "", &ok);
    if (!ok) return;

    QString content = textEdit->toPlainText();
    int count = content.count(findText);
    content.replace(findText, replaceText);
    textEdit->setPlainText(content);
    statusLabel->setText(QString("替换 %1 处").arg(count));
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
    if (event->key() == Qt::Key_S && event->modifiers() & Qt::ControlModifier) { saveFile(); return; }
    if (event->key() == Qt::Key_F && event->modifiers() & Qt::ControlModifier) { searchField->setFocus(); searchField->selectAll(); return; }
    if (event->key() == Qt::Key_H && event->modifiers() & Qt::ControlModifier) { replaceText(); return; }
    if (event->key() == Qt::Key_Escape) { close(); return; }
    QDialog::keyPressEvent(event);
}

void EditorDialog::closeEvent(QCloseEvent* event) {
    if (maybeSave()) {
        event->accept();
    } else {
        event->ignore();
    }
}