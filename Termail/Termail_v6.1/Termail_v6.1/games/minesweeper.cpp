#include "minesweeper.h"
#include "terminal/terminal.h"

#include <QRandomGenerator>
#include <QQueue>
#include <algorithm>

MinesweeperEngine::MinesweeperEngine(TerminalWidget* terminal)
    : ui(terminal), rows(8), cols(8), totalMines(10),
      gameOver(false), won(false), firstClick(true) {}

void MinesweeperEngine::init(int r, int c, int m) {
    rows = r; cols = c; totalMines = m;
    gameOver = false; won = false; firstClick = true;
    board = QVector<QVector<int>>(rows, QVector<int>(cols, 0));
    revealed = QVector<QVector<bool>>(rows, QVector<bool>(cols, false));
    flagged = QVector<QVector<bool>>(rows, QVector<bool>(cols, false));
    render();
}

void MinesweeperEngine::render() {
    QString output = "\n    ";
    for (int c = 0; c < cols; ++c)
        output += QString::number(c + 1).rightJustified(2) + " ";
    output += "\n   +";
    for (int c = 0; c < cols; ++c) output += "--+";
    output += "\n";

    for (int r = 0; r < rows; ++r) {
        output += QString(" %1 |").arg(QChar('A' + r));
        for (int c = 0; c < cols; ++c) {
            QString cell;
            if (flagged[r][c]) cell = "F";
            else if (!revealed[r][c]) cell = ".";
            else if (board[r][c] == -1) cell = "*";
            else if (board[r][c] == 0) cell = " ";
            else cell = QString::number(board[r][c]);
            output += " " + cell + "|";
        }
        output += "\n";
        if (r < rows - 1) {
            output += "   +";
            for (int c = 0; c < cols; ++c) output += "--+";
            output += "\n";
        }
    }
    output += "   +";
    for (int c = 0; c < cols; ++c) output += "--+";
    output += "\n\nmine A1 揭开 | flag A1 插旗 | mine new 新游戏\n";
    ui->appendOutput(output);
}

void MinesweeperEngine::placeMines(int firstRow, int firstCol) {
    QVector<QPair<int, int>> positions;
    for (int r = 0; r < rows; ++r)
        for (int c = 0; c < cols; ++c)
            if (qAbs(r - firstRow) > 1 || qAbs(c - firstCol) > 1)
                positions.append(qMakePair(r, c));

    for (int i = positions.size() - 1; i > 0; --i) {
        int j = QRandomGenerator::global()->bounded(i + 1);
        std::swap(positions[i], positions[j]);
    }

    int toPlace = qMin(totalMines, positions.size());
    for (int i = 0; i < toPlace; ++i)
        board[positions[i].first][positions[i].second] = -1;

    for (int r = 0; r < rows; ++r)
        for (int c = 0; c < cols; ++c)
            if (board[r][c] != -1) {
                int count = 0;
                for (int dr = -1; dr <= 1; ++dr)
                    for (int dc = -1; dc <= 1; ++dc) {
                        int nr = r + dr, nc = c + dc;
                        if (nr >= 0 && nr < rows && nc >= 0 && nc < cols && board[nr][nc] == -1) count++;
                    }
                board[r][c] = count;
            }
}

void MinesweeperEngine::revealCell(int row, int col) {
    if (revealed[row][col] || flagged[row][col]) return;
    revealed[row][col] = true;

    if (board[row][col] == 0) {
        QQueue<QPair<int, int>> queue;
        queue.enqueue(qMakePair(row, col));
        while (!queue.isEmpty()) {
            auto [r, c] = queue.dequeue();
            for (int dr = -1; dr <= 1; ++dr)
                for (int dc = -1; dc <= 1; ++dc) {
                    int nr = r + dr, nc = c + dc;
                    if (nr >= 0 && nr < rows && nc >= 0 && nc < cols &&
                        !revealed[nr][nc] && !flagged[nr][nc]) {
                        revealed[nr][nc] = true;
                        if (board[nr][nc] == 0) queue.enqueue(qMakePair(nr, nc));
                    }
                }
        }
    }
}

bool MinesweeperEngine::reveal(int row, int col) {
    if (gameOver || flagged[row][col]) return false;
    if (firstClick) {
        placeMines(row, col);
        firstClick = false;
    }
    if (board[row][col] == -1) {
        gameOver = true; won = false;
        for (int r = 0; r < rows; ++r)
            for (int c = 0; c < cols; ++c)
                if (board[r][c] == -1) revealed[r][c] = true;
        render();
        ui->appendOutput("游戏结束\n", ERROR_COLOR);
        return true;
    }
    revealCell(row, col);
    if (checkWin()) {
        gameOver = true; won = true;
        render();
        ui->appendOutput("恭喜胜利！\n", PROMPT_COLOR);
        return true;
    }
    render();
    return false;
}

void MinesweeperEngine::flag(int row, int col) {
    if (gameOver || revealed[row][col]) return;
    flagged[row][col] = !flagged[row][col];
    render();
}

bool MinesweeperEngine::checkWin() {
    for (int r = 0; r < rows; ++r)
        for (int c = 0; c < cols; ++c)
            if (board[r][c] != -1 && !revealed[r][c]) return false;
    return true;
}