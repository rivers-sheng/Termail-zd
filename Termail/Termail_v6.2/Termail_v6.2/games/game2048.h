#ifndef GAME2048_H
#define GAME2048_H

#include <QDialog>
#include <QVector>
#include <QLabel>
#include <QPushButton>
#include <QPoint>

class Game2048 : public QDialog {
    Q_OBJECT
public:
    explicit Game2048(QWidget* parent = nullptr);

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    void newGame();
    void addRandom();
    bool move(int dx, int dy);
    bool canMove();
    QColor tileColor(int value);
    QColor textColor(int value);

    static const int SIZE = 4;
    static const int CELL = 70;
    static const int GAP = 6;

    QVector<QVector<int>> board;
    int score;
    bool gameOver;
    QLabel* scoreLabel;
    QPoint pressPos;
};

#endif