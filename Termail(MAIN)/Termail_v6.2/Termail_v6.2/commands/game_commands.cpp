#include "commands.h"
#include "terminal/terminal.h"
#include "games/tetris.h"
#include "games/snake.h"
#include "games/game2048.h"

void CommandRegistry::registerGameCommands(TerminalWidget* ui) {

    registerCommand("tetris", [ui](const QStringList&) {
        ui->appendOutput("启动俄罗斯方块...\n", ACCENT_COLOR);
        TetrisGame* g = new TetrisGame(ui);
        g->setAttribute(Qt::WA_DeleteOnClose);
        g->show();
    }, "俄罗斯方块", "tetris");

    registerCommand("snake", [ui](const QStringList&) {
        ui->appendOutput("启动贪吃蛇...\n", ACCENT_COLOR);
        SnakeGame* g = new SnakeGame(ui);
        g->setAttribute(Qt::WA_DeleteOnClose);
        g->show();
    }, "贪吃蛇", "snake");

    registerCommand("g2048", [ui](const QStringList&) {
        ui->appendOutput("启动2048...\n", ACCENT_COLOR);
        Game2048* g = new Game2048(ui);
        g->setAttribute(Qt::WA_DeleteOnClose);
        g->show();
    }, "2048", "g2048");

    // 游戏菜单
    registerCommand("games", [ui](const QStringList&) {
        ui->appendOutput("=== 游戏 ===\n", ACCENT_COLOR);
        ui->appendOutput("  tetris  - 俄罗斯方块\n", PROMPT_COLOR);
        ui->appendOutput("  snake   - 贪吃蛇\n", PROMPT_COLOR);
        ui->appendOutput("  g2048   - 2048\n", PROMPT_COLOR);
    }, "游戏菜单", "games");
}