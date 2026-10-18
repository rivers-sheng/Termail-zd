#ifndef TETRIS_H
#define TETRIS_H

#include <QDialog>
#include <QWidget>
#include <QTimer>
#include <QLabel>
#include <QPushButton>
#include <QGridLayout>
#include <QVector>

class TetrisGame : public QDialog {
    Q_OBJECT
public:
    explicit TetrisGame(QWidget* parent = nullptr);

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private slots:
    void tick();

private:
    void newGame();
    void spawnPiece();
    bool valid(int x, int y, const QVector<QVector<int>>& shape);
    void lockPiece();
    void clearLines();
    void moveLeft();
    void moveRight();
    void rotate();
    void drop();
    void drawBlock(QPainter& p, int x, int y, int color);
    void drawNext(QPainter& p);

    static const int COLS = 10;
    static const int ROWS = 20;
    static const int CELL = 18;

    QVector<QVector<int>> board;
    QVector<QVector<int>> current;
    QVector<QVector<int>> next;
    int curX, curY, curColor;
    int score, lines, level;
    bool gameOver;
    QTimer* timer;
    QLabel* scoreLabel;
    QLabel* statusLabel;
};

#endif