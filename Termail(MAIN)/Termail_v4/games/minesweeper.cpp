#include "minesweeper.h"
#include "config.h"

#include <QMessageBox>
#include <QInputDialog>
#include <QDialogButtonBox>
#include <QSpinBox>
#include <QFormLayout>
#include <QTime>
#include <QRandomGenerator>
#include <QMouseEvent>
#include <QApplication>
#include <algorithm>
#include <random>

MinesweeperGame::MinesweeperGame(QWidget* parent)
    : QDialog(parent), rows(8), cols(8), totalMines(10), cellSize(30),
      gameOver(false), won(false), firstClick(true), timer(nullptr), startTime(0) {

    setWindowTitle("扫雷");
    setMinimumSize(300, 350);
    resize(500, 550);
    setStyleSheet(QString("background-color: #2b2b2b;"));

    numberColors[-1] = "#ff0000";
    numberColors[0] = "#cccccc";
    numberColors[1] = "#0000ff";
    numberColors[2] = "#008000";
    numberColors[3] = "#ff0000";
    numberColors[4] = "#000080";
    numberColors[5] = "#800000";
    numberColors[6] = "#008080";
    numberColors[7] = "#000000";
    numberColors[8] = "#808080";

    setupUI();
    initGame();
}

MinesweeperGame::~MinesweeperGame() {
    if (timer) {
        timer->stop();
    }
}

void MinesweeperGame::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    QHBoxLayout* controlLayout = new QHBoxLayout();

    QLabel* diffLabel = new QLabel("难度:", this);
    diffLabel->setStyleSheet("color: white;");
    controlLayout->addWidget(diffLabel);

    difficultyCombo = new QComboBox(this);
    difficultyCombo->addItems({"简单", "中等", "困难", "自定义"});
    difficultyCombo->setCurrentText("简单");
    difficultyCombo->setStyleSheet("background-color: #4a4a4a; color: white; padding: 2px;");
    controlLayout->addWidget(difficultyCombo);
    connect(difficultyCombo, &QComboBox::currentTextChanged, this, &MinesweeperGame::onChangeDifficulty);

    QPushButton* newGameBtn = new QPushButton("新游戏", this);
    newGameBtn->setStyleSheet("background-color: #4a4a4a; color: white; border: none; padding: 5px 15px;");
    controlLayout->addWidget(newGameBtn);
    connect(newGameBtn, &QPushButton::clicked, this, &MinesweeperGame::onNewGame);

    controlLayout->addStretch();

    timerLabel = new QLabel("时间: 0s", this);
    timerLabel->setStyleSheet("color: white;");
    controlLayout->addWidget(timerLabel);

    mineLabel = new QLabel(QString("地雷: %1").arg(totalMines), this);
    mineLabel->setStyleSheet("color: white;");
    controlLayout->addWidget(mineLabel);

    mainLayout->addLayout(controlLayout);

    gameBoard = new QWidget(this);
    mainLayout->addWidget(gameBoard, 0, Qt::AlignCenter);

    statusLabel = new QLabel("", this);
    statusLabel->setStyleSheet("color: #00ff00; font-size: 14px; font-weight: bold;");
    statusLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(statusLabel);

    setLayout(mainLayout);
}

void MinesweeperGame::initGame() {
    gameOver = false;
    won = false;
    firstClick = true;

    board = QVector<QVector<int>>(rows, QVector<int>(cols, 0));
    revealed = QVector<QVector<bool>>(rows, QVector<bool>(cols, false));
    flagged = QVector<QVector<bool>>(rows, QVector<bool>(cols, false));

    if (timer) {
        timer->stop();
    }
    timerLabel->setText("时间: 0s");
    mineLabel->setText(QString("地雷: %1").arg(totalMines));
    statusLabel->setText("");

    // 自动计算格子大小适应屏幕
    QScreen* screen = QApplication::primaryScreen();
    int maxW = screen ? screen->availableSize().width() - 60 : 800;
    int maxH = screen ? screen->availableSize().height() - 200 : 600;
    cellSize = qMin(qMin(maxW / cols, maxH / rows), 35);
    cellSize = qMax(cellSize, 20);

    // 再创建按钮
    createBoard();
    adjustSize();
    setMinimumSize(300, 350);
    setMaximumSize(1920, 1080);
}

void MinesweeperGame::createBoard() {
    QLayout* layout = gameBoard->layout();
    if (layout) {
        QLayoutItem* item;
        while ((item = layout->takeAt(0)) != nullptr) {
            delete item->widget();
            delete item;
        }
        delete layout;
    }

    QGridLayout* gridLayout = new QGridLayout(gameBoard);
    gridLayout->setSpacing(1);
    gridLayout->setContentsMargins(0, 0, 0, 0);

    buttons = QVector<QVector<QPushButton*>>(rows, QVector<QPushButton*>(cols, nullptr));

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            QPushButton* btn = new QPushButton(this);
            btn->setFixedSize(cellSize, cellSize);
            btn->setStyleSheet(
                "QPushButton { background-color: #4a4a4a; color: white; font-weight: bold; border: 1px solid #555; }"
                "QPushButton:hover { background-color: #5a5a5a; }"
                "QPushButton:pressed { background-color: #3a3a3a; }"
            );

            int row = r, col = c;

            QTimer* pressTimer = new QTimer(btn);
            pressTimer->setSingleShot(true);
            pressTimer->setProperty("row", row);
            pressTimer->setProperty("col", col);
            bool* isLongPress = new bool(false);
            pressTimer->setProperty("isLong", QVariant::fromValue(reinterpret_cast<quintptr>(isLongPress)));

            QObject::connect(btn, &QPushButton::pressed, [pressTimer, isLongPress]() {
                *isLongPress = false;
                pressTimer->start(500);
            });

            QObject::connect(pressTimer, &QTimer::timeout, [this, pressTimer, isLongPress]() {
                *isLongPress = true;
                int r = pressTimer->property("row").toInt();
                int c = pressTimer->property("col").toInt();
                onCellRightClick(r, c);
            });

            QObject::connect(btn, &QPushButton::released, [this, row, col, pressTimer, isLongPress]() {
                pressTimer->stop();
                if (!(*isLongPress)) {
                    onCellLeftClick(row, col);
                }
            });

            gridLayout->addWidget(btn, r, c);
            buttons[r][c] = btn;
        }
    }

    gameBoard->setLayout(gridLayout);
}

void MinesweeperGame::placeMines(int firstRow, int firstCol) {
    QVector<QPair<int, int>> positions;
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            if (qAbs(r - firstRow) > 1 || qAbs(c - firstCol) > 1) {
                positions.append(qMakePair(r, c));
            }
        }
    }

    std::shuffle(positions.begin(), positions.end(), std::mt19937(std::random_device()()));
    int minesToPlace = qMin(totalMines, positions.size());
    for (int i = 0; i < minesToPlace; ++i) {
        board[positions[i].first][positions[i].second] = -1;
    }

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            if (board[r][c] != -1) {
                int count = 0;
                for (int dr = -1; dr <= 1; ++dr) {
                    for (int dc = -1; dc <= 1; ++dc) {
                        int nr = r + dr, nc = c + dc;
                        if (nr >= 0 && nr < rows && nc >= 0 && nc < cols && board[nr][nc] == -1) {
                            count++;
                        }
                    }
                }
                board[r][c] = count;
            }
        }
    }

    startTime = QTime::currentTime().secsTo(QTime::currentTime());
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MinesweeperGame::updateTimer);
    timer->start(1000);
}

void MinesweeperGame::updateTimer() {
    if (!gameOver) {
        int elapsed = QTime::currentTime().secsTo(QTime::currentTime()) - startTime;
        timerLabel->setText(QString("时间: %1s").arg(qAbs(elapsed)));
    }
}

void MinesweeperGame::onCellLeftClick(int row, int col) {
    if (gameOver || flagged[row][col]) return;

    if (firstClick) {
        placeMines(row, col);
        firstClick = false;
    }

    if (board[row][col] == -1) {
        revealAllMines(row, col);
        gameOver = true;
        won = false;
        statusLabel->setText("💥 游戏结束！");
        statusLabel->setStyleSheet("color: red; font-size: 14px; font-weight: bold;");
        if (timer) timer->stop();
        return;
    }

    revealCell(row, col);
    checkWin();
}

void MinesweeperGame::onCellRightClick(int row, int col) {
    if (gameOver || revealed[row][col]) return;

    flagged[row][col] = !flagged[row][col];
    if (flagged[row][col]) {
        buttons[row][col]->setText("🚩");
        buttons[row][col]->setStyleSheet(
            "QPushButton { background-color: #4a4a4a; font-size: 12px; border: 1px solid #555; }"
        );
    } else {
        buttons[row][col]->setText("");
        buttons[row][col]->setStyleSheet(
            "QPushButton { background-color: #4a4a4a; color: white; font-weight: bold; border: 1px solid #555; }"
        );
    }

    int flagCount = 0;
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            if (flagged[r][c]) flagCount++;
        }
    }
    mineLabel->setText(QString("地雷: %1").arg(totalMines - flagCount));
}

void MinesweeperGame::revealCell(int row, int col) {
    if (revealed[row][col] || flagged[row][col]) return;

    revealed[row][col] = true;

    if (board[row][col] == 0) {
        buttons[row][col]->setText("");
        buttons[row][col]->setStyleSheet(
            "QPushButton { background-color: #cccccc; border: 1px solid #999; }"
        );

        QVector<QPair<int, int>> queue;
        queue.append(qMakePair(row, col));

        while (!queue.isEmpty()) {
            auto [r, c] = queue.takeFirst();
            for (int dr = -1; dr <= 1; ++dr) {
                for (int dc = -1; dc <= 1; ++dc) {
                    int nr = r + dr, nc = c + dc;
                    if (nr >= 0 && nr < rows && nc >= 0 && nc < cols &&
                        !revealed[nr][nc] && !flagged[nr][nc]) {
                        revealed[nr][nc] = true;
                        if (board[nr][nc] == 0) {
                            buttons[nr][nc]->setText("");
                            buttons[nr][nc]->setStyleSheet(
                                "QPushButton { background-color: #cccccc; border: 1px solid #999; }"
                            );
                            queue.append(qMakePair(nr, nc));
                        } else {
                            QString color = numberColors.value(board[nr][nc], "black");
                            buttons[nr][nc]->setText(QString::number(board[nr][nc]));
                            buttons[nr][nc]->setStyleSheet(
                                QString("QPushButton { background-color: #cccccc; color: %1; font-weight: bold; border: 1px solid #999; }").arg(color)
                            );
                        }
                    }
                }
            }
        }
    } else {
        QString color = numberColors.value(board[row][col], "black");
        buttons[row][col]->setText(QString::number(board[row][col]));
        buttons[row][col]->setStyleSheet(
            QString("QPushButton { background-color: #cccccc; color: %1; font-weight: bold; border: 1px solid #999; }").arg(color)
        );
    }
}

void MinesweeperGame::revealAllMines(int hitRow, int hitCol) {
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            if (board[r][c] == -1) {
                if (r == hitRow && c == hitCol) {
                    buttons[r][c]->setText("💥");
                    buttons[r][c]->setStyleSheet(
                        "QPushButton { background-color: red; font-size: 14px; border: 1px solid #999; }"
                    );
                } else {
                    buttons[r][c]->setText("💣");
                    buttons[r][c]->setStyleSheet(
                        "QPushButton { background-color: #ffcccc; font-size: 14px; border: 1px solid #999; }"
                    );
                }
            }
        }
    }
}

bool MinesweeperGame::checkWin() {
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            if (board[r][c] != -1 && !revealed[r][c]) {
                return false;
            }
        }
    }

    gameOver = true;
    won = true;
    statusLabel->setText("🎉 恭喜你赢了！");
    statusLabel->setStyleSheet("color: #00ff00; font-size: 14px; font-weight: bold;");
    if (timer) timer->stop();

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            if (board[r][c] == -1) {
                buttons[r][c]->setText("🚩");
                buttons[r][c]->setStyleSheet(
                    "QPushButton { background-color: #4a4a4a; font-size: 12px; border: 1px solid #555; }"
                );
            }
        }
    }

    return true;
}

void MinesweeperGame::onChangeDifficulty(const QString& difficulty) {
    if (difficulty == "简单") {
        rows = 8; cols = 8; totalMines = 10;
    } else if (difficulty == "中等") {
        rows = 12; cols = 12; totalMines = 25;
    } else if (difficulty == "困难") {
        rows = 16; cols = 16; totalMines = 40;
    } else if (difficulty == "自定义") {
        onCustomDifficulty();
        return;
    }
    cellSize = qMax(25, qMin(35, 400 / qMax(rows, cols)));
    initGame();
}

void MinesweeperGame::onNewGame() {
    initGame();
}

void MinesweeperGame::onCustomDifficulty() {
    QDialog* dlg = new QDialog(this);
    dlg->setWindowTitle("自定义难度");
    dlg->setStyleSheet("background-color: #2b2b2b;");

    QFormLayout* form = new QFormLayout(dlg);

    QSpinBox* rowsSpin = new QSpinBox(dlg);
    rowsSpin->setRange(5, 30);
    rowsSpin->setValue(rows);
    rowsSpin->setStyleSheet("background-color: #1e1e1e; color: white;");
    form->addRow(new QLabel("行数:", dlg), rowsSpin);

    QSpinBox* colsSpin = new QSpinBox(dlg);
    colsSpin->setRange(5, 30);
    colsSpin->setValue(cols);
    colsSpin->setStyleSheet("background-color: #1e1e1e; color: white;");
    form->addRow(new QLabel("列数:", dlg), colsSpin);

    QSpinBox* minesSpin = new QSpinBox(dlg);
    minesSpin->setRange(1, 500);
    minesSpin->setValue(totalMines);
    minesSpin->setStyleSheet("background-color: #1e1e1e; color: white;");
    form->addRow(new QLabel("地雷数:", dlg), minesSpin);

    for (int i = 0; i < form->rowCount(); ++i) {
        QLabel* label = qobject_cast<QLabel*>(form->itemAt(i, QFormLayout::LabelRole)->widget());
        if (label) label->setStyleSheet("color: white;");
    }

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, dlg);
    buttons->setStyleSheet("QPushButton { background-color: #4a4a4a; color: white; padding: 5px 15px; }");
    form->addRow(buttons);

    connect(buttons, &QDialogButtonBox::accepted, [dlg, rowsSpin, colsSpin, minesSpin, this]() {
        rows = rowsSpin->value();
        cols = colsSpin->value();
        totalMines = qMin(minesSpin->value(), rows * cols - 1);
        cellSize = qMax(25, qMin(35, 400 / qMax(rows, cols)));
        dlg->accept();
        initGame();
    });
    connect(buttons, &QDialogButtonBox::rejected, dlg, &QDialog::reject);

    dlg->exec();
    dlg->deleteLater();
}