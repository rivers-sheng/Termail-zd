#ifndef MINESWEEPER_H
#define MINESWEEPER_H

#include <QDialog>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QTimer>
#include <QVector>
#include <QPair>
#include <QMap>

#include "config.h"

class MinesweeperGame : public QDialog {
    Q_OBJECT

public:
    explicit MinesweeperGame(QWidget* parent = nullptr);
    ~MinesweeperGame();

private slots:
    void onCellLeftClick(int row, int col);
    void onCellRightClick(int row, int col);
    void onChangeDifficulty(const QString& difficulty);
    void onNewGame();
    void onCustomDifficulty();

private:
    void setupUI();
    void initGame();
    void createBoard();
    void placeMines(int firstRow, int firstCol);
    void revealCell(int row, int col);
    void revealAllMines(int hitRow, int hitCol);
    bool checkWin();
    void updateTimer();

    // UI组件
    QComboBox* difficultyCombo;
    QLabel* timerLabel;
    QLabel* mineLabel;
    QLabel* statusLabel;
    QWidget* gameBoard;
    QVector<QVector<QPushButton*>> buttons;

    // 游戏状态
    int rows;
    int cols;
    int totalMines;
    int cellSize;
    QVector<QVector<int>> board;
    QVector<QVector<bool>> revealed;
    QVector<QVector<bool>> flagged;
    bool gameOver;
    bool won;
    bool firstClick;
    QTimer* timer;
    int startTime;

    // 颜色映射
    QMap<int, QString> numberColors;
};

#endif // MINESWEEPER_H