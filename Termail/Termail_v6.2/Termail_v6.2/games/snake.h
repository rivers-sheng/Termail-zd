#ifndef SNAKE_H
#define SNAKE_H

#include <QDialog>
#include <QTimer>
#include <QLabel>
#include <QVector>
#include <QPoint>

class SnakeGame : public QDialog {
    Q_OBJECT
public:
    explicit SnakeGame(QWidget* parent = nullptr);

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private slots:
    void tick();

private:
    void newGame();
    void spawnFood();

    static const int COLS = 16;
    static const int ROWS = 22;
    static const int CELL = 16;

    QVector<QPoint> snake;
    QPoint food;
    int dirX, dirY;
    int score;
    bool gameOver;
    QTimer* timer;
    QLabel* scoreLabel;
};

#endif