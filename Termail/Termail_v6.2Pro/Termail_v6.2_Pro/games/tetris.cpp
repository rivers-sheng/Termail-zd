#include "tetris.h"
#include "config.h"

#include <QPainter>
#include <QKeyEvent>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

static const int PIECES[7][4][4] = {
    // I
    {{0,0,0,0},{1,1,1,1},{0,0,0,0},{0,0,0,0}},
    // O
    {{0,0,0,0},{0,1,1,0},{0,1,1,0},{0,0,0,0}},
    // T
    {{0,0,0,0},{0,1,0,0},{1,1,1,0},{0,0,0,0}},
    // S
    {{0,0,0,0},{0,1,1,0},{1,1,0,0},{0,0,0,0}},
    // Z
    {{0,0,0,0},{1,1,0,0},{0,1,1,0},{0,0,0,0}},
    // L
    {{0,0,0,0},{1,0,0,0},{1,1,1,0},{0,0,0,0}},
    // J
    {{0,0,0,0},{0,0,1,0},{1,1,1,0},{0,0,0,0}}
};

TetrisGame::TetrisGame(QWidget* parent)
    : QDialog(parent), score(0), lines(0), level(1), gameOver(false), timer(nullptr) {

    setWindowTitle("俄罗斯方块");
    setFixedSize(COLS * CELL + 120, ROWS * CELL + 120);
    setStyleSheet("background-color: #1a1a1a;");

    board = QVector<QVector<int>>(ROWS, QVector<int>(COLS, 0));
    current = QVector<QVector<int>>(4, QVector<int>(4, 0));
    next = QVector<QVector<int>>(4, QVector<int>(4, 0));

    // 顶部信息栏
    QWidget* topBar = new QWidget(this);
    topBar->setGeometry(0, 0, width(), 40);
    topBar->setStyleSheet("background-color: #2a2a2a;");

    scoreLabel = new QLabel(topBar);
    scoreLabel->setStyleSheet("color: #00ff00; font-size: 14px; font-weight: bold;");
    scoreLabel->setGeometry(10, 10, 200, 20);

    statusLabel = new QLabel(topBar);
    statusLabel->setStyleSheet("color: #ff4444; font-size: 14px; font-weight: bold;");
    statusLabel->setGeometry(220, 10, 200, 20);

    QPushButton* exitBtn = new QPushButton("退出", topBar);
    exitBtn->setGeometry(width() - 80, 8, 70, 24);
    exitBtn->setStyleSheet("background-color: #d32f2f; color: white; border: none; border-radius: 3px;");
    connect(exitBtn, &QPushButton::clicked, this, &QDialog::close);

    // 底部按钮
    QWidget* botBar = new QWidget(this);
    botBar->setGeometry(0, ROWS * CELL + 40, width(), 50);
    botBar->setStyleSheet("background-color: #2a2a2a;");

    auto makeBtn = [botBar](const QString& text, int x) {
        QPushButton* btn = new QPushButton(text, botBar);
        btn->setGeometry(x, 8, 60, 34);
        btn->setStyleSheet("background-color: #444; color: white; border: none; border-radius: 4px; font-size: 16px;");
        return btn;
    };

    QPushButton* leftBtn = makeBtn("◀", 10);
    QPushButton* rotBtn = makeBtn("↻", 80);
    QPushButton* rightBtn = makeBtn("▶", 150);
    QPushButton* downBtn = makeBtn("▼", 220);

    connect(leftBtn, &QPushButton::clicked, this, &TetrisGame::moveLeft);
    connect(rightBtn, &QPushButton::clicked, this, &TetrisGame::moveRight);
    connect(rotBtn, &QPushButton::clicked, this, &TetrisGame::rotate);
    connect(downBtn, &QPushButton::clicked, this, &TetrisGame::drop);

    newGame();
    setFocusPolicy(Qt::StrongFocus);
}

void TetrisGame::newGame() {
    board = QVector<QVector<int>>(ROWS, QVector<int>(COLS, 0));
    score = 0;
    lines = 0;
    level = 1;
    gameOver = false;
    statusLabel->setText("");
    scoreLabel->setText("分数: 0  行: 0  等级: 1");

    // 生成下一个
    int r = QRandomGenerator::global()->bounded(7);
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            next[i][j] = PIECES[r][i][j];

    spawnPiece();

    if (timer) timer->stop();
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &TetrisGame::tick);
    timer->start(500);
    update();
}

void TetrisGame::spawnPiece() {
    current = next;
    curX = COLS / 2 - 2;
    curY = 0;
    curColor = QRandomGenerator::global()->bounded(1, 8);

    int r = QRandomGenerator::global()->bounded(7);
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            next[i][j] = PIECES[r][i][j];

    if (!valid(curX, curY, current)) {
        gameOver = true;
        timer->stop();
        statusLabel->setText("游戏结束！");
    }
}

bool TetrisGame::valid(int x, int y, const QVector<QVector<int>>& shape) {
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            if (shape[i][j]) {
                int nx = x + j, ny = y + i;
                if (nx < 0 || nx >= COLS || ny >= ROWS) return false;
                if (ny >= 0 && board[ny][nx]) return false;
            }
    return true;
}

void TetrisGame::lockPiece() {
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            if (current[i][j]) {
                int ny = curY + i;
                int nx = curX + j;
                if (ny >= 0 && ny < ROWS && nx >= 0 && nx < COLS)
                    board[ny][nx] = curColor;
            }
    clearLines();
    spawnPiece();
}

void TetrisGame::clearLines() {
    int cleared = 0;
    for (int i = ROWS - 1; i >= 0; --i) {
        bool full = true;
        for (int j = 0; j < COLS; ++j)
            if (!board[i][j]) { full = false; break; }
        if (full) {
            board.removeAt(i);
            board.insert(0, QVector<int>(COLS, 0));
            cleared++;
            i++;
        }
    }
    if (cleared > 0) {
        lines += cleared;
        score += cleared * 100 * level;
        level = lines / 10 + 1;
        scoreLabel->setText(QString("分数: %1  行: %2  等级: %3")
            .arg(score).arg(lines).arg(level));
        timer->setInterval(qMax(100, 500 - (level - 1) * 40));
    }
}

void TetrisGame::tick() {
    if (gameOver) return;
    if (valid(curX, curY + 1, current)) curY++;
    else lockPiece();
    update();
}

void TetrisGame::moveLeft() {
    if (gameOver) return;
    if (valid(curX - 1, curY, current)) { curX--; update(); }
}

void TetrisGame::moveRight() {
    if (gameOver) return;
    if (valid(curX + 1, curY, current)) { curX++; update(); }
}

void TetrisGame::rotate() {
    if (gameOver) return;
    QVector<QVector<int>> rotated(4, QVector<int>(4, 0));
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            rotated[j][3-i] = current[i][j];
    if (valid(curX, curY, rotated)) { current = rotated; update(); }
}

void TetrisGame::drop() {
    if (gameOver) return;
    while (valid(curX, curY + 1, current)) curY++;
    lockPiece();
    update();
}

void TetrisGame::keyPressEvent(QKeyEvent* event) {
    switch (event->key()) {
        case Qt::Key_Left: moveLeft(); break;
        case Qt::Key_Right: moveRight(); break;
        case Qt::Key_Up: rotate(); break;
        case Qt::Key_Down: drop(); break;
        case Qt::Key_Space: drop(); break;
        case Qt::Key_Escape: close(); break;
    }
}

void TetrisGame::drawBlock(QPainter& p, int x, int y, int color) {
    static const QColor colors[8] = {
        QColor(0,0,0), QColor(0,255,255), QColor(255,255,0),
        QColor(128,0,255), QColor(0,255,0), QColor(255,0,0),
        QColor(0,0,255), QColor(255,165,0)
    };
    p.fillRect(x, y, CELL - 1, CELL - 1, colors[color]);
    p.setPen(QColor(255, 255, 255, 50));
    p.drawRect(x, y, CELL - 1, CELL - 1);
}

void TetrisGame::drawNext(QPainter& p) {
    int startX = COLS * CELL + 20;
    int startY = 60;
    p.setPen(Qt::white);
    p.setFont(QFont(FONT_FAMILY, 11));
    p.drawText(startX, startY - 10, "下一个:");
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            if (next[i][j])
                drawBlock(p, startX + j * CELL, startY + i * CELL, 1);
}

void TetrisGame::paintEvent(QPaintEvent*) {
    QPainter p(this);
    int offsetY = 40;

    // 游戏区背景
    p.fillRect(0, offsetY, COLS * CELL, ROWS * CELL, QColor(10, 10, 10));
    p.setPen(QColor(60, 60, 60));
    for (int i = 0; i <= COLS; ++i)
        p.drawLine(i * CELL, offsetY, i * CELL, offsetY + ROWS * CELL);
    for (int i = 0; i <= ROWS; ++i)
        p.drawLine(0, offsetY + i * CELL, COLS * CELL, offsetY + i * CELL);

    // 已固定方块
    for (int i = 0; i < ROWS; ++i)
        for (int j = 0; j < COLS; ++j)
            if (board[i][j])
                drawBlock(p, j * CELL, offsetY + i * CELL, board[i][j]);

    // 当前方块
    if (!gameOver) {
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                if (current[i][j]) {
                    int ny = curY + i;
                    if (ny >= 0)
                        drawBlock(p, (curX + j) * CELL, offsetY + ny * CELL, curColor);
                }
    }

    drawNext(p);

    if (gameOver) {
        p.setPen(QColor(255, 68, 68));
        p.setFont(QFont(FONT_FAMILY, 24, QFont::Bold));
        p.drawText(rect(), Qt::AlignCenter, "游戏结束");
    }
}