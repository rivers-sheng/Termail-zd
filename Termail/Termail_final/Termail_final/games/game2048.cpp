#include "game2048.h"
#include "config.h"

#include <QPainter>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QRandomGenerator>
#include <QSettings>

Game2048::Game2048(QWidget* parent)
    : QDialog(parent), score(0), gameOver(false) {

    setWindowTitle("2048");
    setFixedSize(SIZE * (CELL + GAP) + GAP, SIZE * (CELL + GAP) + GAP + 90);
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

    QPushButton* restartBtn = new QPushButton("重开", this);
    restartBtn->setGeometry(width() - 160, 8, 70, 24);
    restartBtn->setStyleSheet("background-color: #4CAF50; color: white; border: none; border-radius: 3px;");
    connect(restartBtn, &QPushButton::clicked, this, &Game2048::newGame);

    // 底部方向按钮
    auto makeBtn = [this](const QString& text, int x, int y) {
        QPushButton* btn = new QPushButton(text, this);
        btn->setGeometry(x, y, 60, 34);
        btn->setStyleSheet("background-color: #444; color: white; border: none; border-radius: 4px; font-size: 16px;");
        return btn;
    };
    int by = height() - 50;
    QPushButton* upBtn = makeBtn("▲", width()/2 - 30, by - 38);
    QPushButton* downBtn = makeBtn("▼", width()/2 - 30, by);
    QPushButton* leftBtn = makeBtn("◀", width()/2 - 90, by);
    QPushButton* rightBtn = makeBtn("▶", width()/2 + 30, by);

    connect(upBtn, &QPushButton::clicked, [this]() { if (move(0, 1)) update(); });
    connect(downBtn, &QPushButton::clicked, [this]() { if (move(0, -1)) update(); });
    connect(leftBtn, &QPushButton::clicked, [this]() { if (move(1, 0)) update(); });
    connect(rightBtn, &QPushButton::clicked, [this]() { if (move(-1, 0)) update(); });

    newGame();
    setFocusPolicy(Qt::StrongFocus);
}

void Game2048::newGame() {
    board = QVector<QVector<int>>(SIZE, QVector<int>(SIZE, 0));
    score = 0;
    gameOver = false;
    scoreLabel->setText("分数: 0");
    addRandom();
    addRandom();
    update();
}

void Game2048::addRandom() {
    QVector<QPoint> empty;
    for (int i = 0; i < SIZE; ++i)
        for (int j = 0; j < SIZE; ++j)
            if (board[i][j] == 0) empty.append(QPoint(j, i));
    if (empty.isEmpty()) return;
    QPoint p = empty[QRandomGenerator::global()->bounded(empty.size())];
    board[p.y()][p.x()] = (QRandomGenerator::global()->bounded(10) < 9) ? 2 : 4;
}

bool Game2048::move(int dx, int dy) {
    bool moved = false;
    QVector<QVector<int>> old = board;

    // 遍历方向
    if (dx != 0) {
        for (int y = 0; y < SIZE; ++y) {
            QVector<int> row;
            for (int x = 0; x < SIZE; ++x) {
                int nx = (dx > 0) ? x : SIZE - 1 - x;
                if (board[y][nx]) row.append(board[y][nx]);
            }
            QVector<int> merged;
            for (int i = 0; i < row.size(); ++i) {
                if (i + 1 < row.size() && row[i] == row[i+1]) {
                    merged.append(row[i] * 2);
                    score += row[i] * 2;
                    i++;
                } else merged.append(row[i]);
            }
            while (merged.size() < SIZE) merged.append(0);
            for (int x = 0; x < SIZE; ++x) {
                int nx = (dx > 0) ? x : SIZE - 1 - x;
                board[y][nx] = merged[x];
            }
        }
    } else if (dy != 0) {
        for (int x = 0; x < SIZE; ++x) {
            QVector<int> col;
            for (int y = 0; y < SIZE; ++y) {
                int ny = (dy > 0) ? y : SIZE - 1 - y;
                if (board[ny][x]) col.append(board[ny][x]);
            }
            QVector<int> merged;
            for (int i = 0; i < col.size(); ++i) {
                if (i + 1 < col.size() && col[i] == col[i+1]) {
                    merged.append(col[i] * 2);
                    score += col[i] * 2;
                    i++;
                } else merged.append(col[i]);
            }
            while (merged.size() < SIZE) merged.append(0);
            for (int y = 0; y < SIZE; ++y) {
                int ny = (dy > 0) ? y : SIZE - 1 - y;
                board[ny][x] = merged[y];
            }
        }
    }

    for (int i = 0; i < SIZE; ++i)
        for (int j = 0; j < SIZE; ++j)
            if (board[i][j] != old[i][j]) moved = true;

    if (moved) {
        addRandom();
        scoreLabel->setText(QString("分数: %1").arg(score));
        // 检查胜利/失败
        for (int i = 0; i < SIZE; ++i)
            for (int j = 0; j < SIZE; ++j)
                if (board[i][j] == 2048)
                    scoreLabel->setText(QString("分数: %1  🎉达成2048!").arg(score));
        if (!canMove()) {
            gameOver = true;
            scoreLabel->setText(QString("分数: %1  游戏结束").arg(score));
        }
    }
    return moved;
}

bool Game2048::canMove() {
    for (int i = 0; i < SIZE; ++i)
        for (int j = 0; j < SIZE; ++j) {
            if (board[i][j] == 0) return true;
            if (j + 1 < SIZE && board[i][j] == board[i][j+1]) return true;
            if (i + 1 < SIZE && board[i][j] == board[i+1][j]) return true;
        }
    return false;
}

QColor Game2048::tileColor(int value) {
    switch (value) {
        case 0: return QColor(50, 50, 50);
        case 2: return QColor(238, 228, 218);
        case 4: return QColor(237, 224, 200);
        case 8: return QColor(242, 177, 121);
        case 16: return QColor(245, 149, 99);
        case 32: return QColor(246, 124, 95);
        case 64: return QColor(246, 94, 59);
        case 128: return QColor(237, 207, 114);
        case 256: return QColor(237, 204, 97);
        case 512: return QColor(237, 200, 80);
        case 1024: return QColor(237, 197, 63);
        case 2048: return QColor(237, 194, 46);
        default: return QColor(60, 58, 50);
    }
}

QColor Game2048::textColor(int value) {
    return value <= 4 ? QColor(119, 110, 101) : QColor(249, 246, 242);
}

void Game2048::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int offsetY = 40;
    // 背景
    p.fillRect(0, offsetY, width(), height() - offsetY, QColor(187, 173, 160));

    for (int i = 0; i < SIZE; ++i) {
        for (int j = 0; j < SIZE; ++j) {
            int x = GAP + j * (CELL + GAP);
            int y = offsetY + GAP + i * (CELL + GAP);
            int val = board[i][j];

            p.setBrush(tileColor(val));
            p.setPen(Qt::NoPen);
            p.drawRoundedRect(x, y, CELL, CELL, 6, 6);

            if (val > 0) {
                p.setPen(textColor(val));
                int fontSize = val < 100 ? 28 : (val < 1000 ? 24 : 20);
                p.setFont(QFont("Arial", fontSize, QFont::Bold));
                p.drawText(QRect(x, y, CELL, CELL), Qt::AlignCenter, QString::number(val));
            }
        }
    }

    if (gameOver) {
        p.setPen(QColor(255, 68, 68));
        p.setFont(QFont(FONT_FAMILY, 24, QFont::Bold));
        p.drawText(rect(), Qt::AlignCenter, "游戏结束");
    }
}

void Game2048::keyPressEvent(QKeyEvent* event) {
    switch (event->key()) {
        case Qt::Key_Up: if (move(0, 1)) update(); break;
        case Qt::Key_Down: if (move(0, -1)) update(); break;
        case Qt::Key_Left: if (move(1, 0)) update(); break;
        case Qt::Key_Right: if (move(-1, 0)) update(); break;
        case Qt::Key_Escape: close(); break;
    }
}
void Game2048::mousePressEvent(QMouseEvent* event) {
    pressPos = event->pos();
}

void Game2048::mouseReleaseEvent(QMouseEvent* event) {
    QPoint diff = event->pos() - pressPos;
    if (qAbs(diff.x()) < 50 && qAbs(diff.y()) < 50) return;

    if (qAbs(diff.x()) > qAbs(diff.y())) {
        if (diff.x() > 0) { if (move(-1, 0)) update(); }
        else { if (move(1, 0)) update(); }
    } else {
        if (diff.y() > 0) { if (move(0, -1)) update(); }
        else { if (move(0, 1)) update(); }
    }

        if (!canMove()) {
        gameOver = true;
        QSettings settings("termail", "games");
        int high = settings.value("g2048_high", 0).toInt();
        if (score > high) settings.setValue("g2048_high", score);
        scoreLabel->setText(QString("分数: %1  游戏结束").arg(score));
        }
}
