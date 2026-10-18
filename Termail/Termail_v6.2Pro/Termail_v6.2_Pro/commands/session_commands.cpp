#include "commands.h"
#include "terminal/terminal.h"
#include "utils/utils.h"

void CommandRegistry::registerSessionCommands(TerminalWidget* ui) {

    // session new [名称]
    registerCommand("session", [ui](const QStringList& args) {
        if (args.isEmpty() || args[0] == "list") {
            ui->listSessions();
            return;
        }
        QString sub = args[0].toLower();
        if (sub == "new") {
            QString name = args.size() > 1 ? args[1] : "";
            ui->createSession(name);
        } else if (sub == "switch" || sub == "s") {
            if (args.size() < 2) {
                ui->appendOutput("用法: session switch <编号>\n", ERROR_COLOR);
                return;
            }
            bool ok;
            int id = args[1].toInt(&ok);
            if (ok) ui->switchSession(id);
            else ui->appendOutput("编号必须是数字\n", ERROR_COLOR);
        } else if (sub == "kill" || sub == "k") {
            if (args.size() < 2) {
                ui->appendOutput("用法: session kill <编号>\n", ERROR_COLOR);
                return;
            }
            bool ok;
            int id = args[1].toInt(&ok);
            if (ok) ui->killSession(id);
        } else {
            ui->appendOutput("用法:\n", ERROR_COLOR);
            ui->appendOutput("  session new [名称]    - 新建会话\n", INFO_COLOR);
            ui->appendOutput("  session list          - 列出会话\n", INFO_COLOR);
            ui->appendOutput("  session switch <编号> - 切换会话\n", INFO_COLOR);
            ui->appendOutput("  session kill <编号>   - 删除会话\n", INFO_COLOR);
        }
    }, "会话管理", "session [new|list|switch|kill]");

    // 快捷命令
    registerCommand("snew", [ui](const QStringList& args) {
        ui->createSession(args.isEmpty() ? "" : args[0]);
    }, "新建会话", "snew [名称]");

    registerCommand("slist", [ui](const QStringList&) {
        ui->listSessions();
    }, "列出会话", "slist");

    registerCommand("ssw", [ui](const QStringList& args) {
        if (args.isEmpty()) return;
        bool ok;
        int id = args[0].toInt(&ok);
        if (ok) ui->switchSession(id);
    }, "切换会话", "ssw <编号>");

    registerCommand("skill", [ui](const QStringList& args) {
        if (args.isEmpty()) return;
        bool ok;
        int id = args[0].toInt(&ok);
        if (ok) ui->killSession(id);
    }, "删除会话", "skill <编号>");
}