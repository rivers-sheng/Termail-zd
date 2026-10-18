#include "snake.h"
#include "config.h"

#include <QPainter>
#include <QKeyEvent>
#include <QRandomGenerator>
#include <QPushButton>
#include <QSettings>

SnakeGame::SnakeGame(QWidget* parent)
    : QDialog(parent), dirX(1), dirY(0), score(0), gameOver(false), timer(nullptr) {

    setWindowTitle("贪吃蛇");
    setFixedSize(COLS * CELL, ROWS * CELL + 180);
    setStyleSheet("background-color: #1a1a1a;");

    QLabel* topBar = new QLabel(this);
    topBar->setGeometry(0, 0, width(), 40);
    topBar->setStyleSheet("background-color: #2a2a2a;");

    scoreLabel = new QLabel("分数: 0", this);
    scoreLabel->setStyleSheet("color: #00ff00; font-size: 14px; font-weight: bold;");
    scoreLabel->setGeometry(10, 10, 200, 20);

    QPushButton* exitBtn = new QPushButton("退出", this);
    exitBtn->setGeometry(width() - 80, 8, 70, 24);
    exitBtn->setStyleSheet("background-color: #d32f2f; color: white; border: none; border-radius: 3px;");
    connect(exitBtn, &QPushButton::clicked, this, &QDialog::close);

    // 底部方向按钮
    auto makeBtn = [this](const QString& text, int x, int y) {
        QPushButton* btn = new QPushButton(text, this);
        btn->setGeometry(x, y, 80, 48);
        btn->setStyleSheet("background-color: #444; color: white; border: none; border-radius: 6px; font-size: 22px;");
        return btn;
    };
    int by = ROWS * CELL + 60;
    QPushButton* upBtn = makeBtn("▲", width()/2 - 40, by);
    QPushButton* downBtn = makeBtn("▼", width()/2 - 40, by + 56);
    QPushButton* leftBtn = makeBtn("◀", width()/2 - 120, by + 56);
    QPushButton* rightBtn = makeBtn("▶", width()/2 + 40, by + 56);

    connect(upBtn, &QPushButton::clicked, [this]() { if (dirY == 0) { dirX = 0; dirY = -1; } });
    connect(downBtn, &QPushButton::clicked, [this]() { if (dirY == 0) { dirX = 0; dirY = 1; } });
    connect(leftBtn, &QPushButton::clicked, [this]() { if (dirX == 0) { dirX = -1; dirY = 0; } });
    connect(rightBtn, &QPushButton::clicked, [this]() { if (dirX == 0) { dirX = 1; dirY = 0; } });

    newGame();
    setFocusPolicy(Qt::StrongFocus);
}

void SnakeGame::newGame() {
    snake.clear();
    snake.append(QPoint(10, 10));
    snake.append(QPoint(9, 10));
    snake.append(QPoint(8, 10));
    dirX = 1; dirY = 0;
    score = 0;
    gameOver = false;
    scoreLabel->setText("分数: 0  (2秒后开始)");
    spawnFood();

    if (timer) timer->stop();
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &SnakeGame::tick);

    QTimer::singleShot(2000, this, [this]() {
        if (!gameOver && timer) {
            timer->start(350);
            scoreLabel->setText("分数: 0");
        }
    });

    update();
}

void SnakeGame::spawnFood() {
    do {
        food = QPoint(QRandomGenerator::global()->bounded(COLS),
                      QRandomGenerator::global()->bounded(ROWS));
    } while (snake.contains(food));
}

void SnakeGame::tick() {
    if (gameOver) return;
    QPoint head = snake.first();
    QPoint newHead(head.x() + dirX, head.y() + dirY);

    if (newHead.x() < 0 || newHead.x() >= COLS ||
        newHead.y() < 0 || newHead.y() >= ROWS) {
        gameOver = true;
        timer->stop();
        update();
        return;
    }

    if (snake.contains(newHead)) {
        gameOver = true;
        timer->stop();
        update();
        return;
    }

    snake.prepend(newHead);

    if (newHead == food) {
        score += 10;
        scoreLabel->setText(QString("分数: %1").arg(score));
        spawnFood();
        // 每吃10个（100分）加速一次
        if (score % 100 == 0 && timer->interval() > 100)
            timer->setInterval(timer->interval() - 15);
    } else {
        snake.removeLast();
    }
    update();
        if (newHead.x() < 0 || newHead.x() >= COLS ||
        newHead.y() < 0 || newHead.y() >= ROWS) {
        gameOver = true;
        timer->stop();
        // 保存最高分
        QSettings settings("termail", "games");
        int high = settings.value("snake_high", 0).toInt();
        if (score > high) settings.setValue("snake_high", score);
        update();
        return;
    }
}

void SnakeGame::keyPressEvent(QKeyEvent* event) {
    switch (event->key()) {
        case Qt::Key_Up: if (dirY == 0) { dirX = 0; dirY = -1; } break;
        case Qt::Key_Down: if (dirY == 0) { dirX = 0; dirY = 1; } break;
        case Qt::Key_Left: if (dirX == 0) { dirX = -1; dirY = 0; } break;
        case Qt::Key_Right: if (dirX == 0) { dirX = 1; dirY = 0; } break;
        case Qt::Key_Escape: close(); break;
    }
}

void SnakeGame::paintEvent(QPaintEvent*) {
    if (snake.isEmpty()) return;
    QPainter p(this);
    int offsetY = 40;

    // 背景
    p.fillRect(0, offsetY, COLS * CELL, ROWS * CELL, QColor(10, 10, 10));

    // 网格
    p.setPen(QColor(30, 30, 30));
    for (int i = 0; i <= COLS; ++i)
        p.drawLine(i * CELL, offsetY, i * CELL, offsetY + ROWS * CELL);
    for (int i = 0; i <= ROWS; ++i)
        p.drawLine(0, offsetY + i * CELL, COLS * CELL, offsetY + i * CELL);

    // 食物
    p.setBrush(QColor(255, 68, 68));
    p.setPen(Qt::NoPen);
    p.drawEllipse(food.x() * CELL + 2, offsetY + food.y() * CELL + 2,
                  CELL - 4, CELL - 4);

    // 蛇
    for (int i = 0; i < snake.size(); ++i) {
        QColor c = (i == 0) ? QColor(0, 255, 0) : QColor(0, 180, 0);
        p.setBrush(c);
        p.drawRect(snake[i].x() * CELL + 1, offsetY + snake[i].y() * CELL + 1,
                   CELL - 2, CELL - 2);
    }

    if (gameOver) {
        p.setPen(QColor(255, 68, 68));
        p.setFont(QFont(FONT_FAMILY, 28, QFont::Bold));
        p.drawText(rect(), Qt::AlignCenter, "游戏结束");
    }
}