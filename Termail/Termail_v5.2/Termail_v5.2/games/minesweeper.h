#ifndef MINESWEEPER_H
#define MINESWEEPER_H

#include <QString>
#include <QVector>

class TerminalWidget;

class MinesweeperEngine {
public:
    MinesweeperEngine(TerminalWidget* ui);
    void init(int rows, int cols, int mines);
    void render();
    bool reveal(int row, int col);
    void flag(int row, int col);
    bool checkWin();

private:
    void placeMines(int firstRow, int firstCol);
    void revealCell(int row, int col);

    TerminalWidget* ui;
    QVector<QVector<int>> board;
    QVector<QVector<bool>> revealed;
    QVector<QVector<bool>> flagged;
    int rows, cols, totalMines;
    bool gameOver, won, firstClick;
};

#endif