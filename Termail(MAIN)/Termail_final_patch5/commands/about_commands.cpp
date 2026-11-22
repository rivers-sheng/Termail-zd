#include "commands.h"
#include "terminal/terminal.h"
#include "config.h"

#include <QRandomGenerator>
#include <QDateTime>

void CommandRegistry::registerAboutCommands(TerminalWidget* ui) {

    // welcome - 重新显示欢迎信息
    registerCommand("welcome", [ui](const QStringList&) {
        QString welcome = QString(
            "\n"
            "  ╔═══════════════════════════════════════╗\n"
            "  ║      Termail终端 v%1          ║\n"
            "  ╚═══════════════════════════════════════╝\n"
            "\n"
            "  制作: rivers晟\n"
            "  GitHub: rivers-sheng\n"
            "  邮箱: k-jiangsheng@outlook.com\n"
            "  许可: GNU AFFERO GENERAL PUBLIC LICENSE\n"
            "\n"
            "  输入 help 查看命令\n"
            "  输入 about 关于软件\n"
            "  输入 tips 使用技巧\n"
            "\n"
        ).arg(APP_VERSION);
        ui->appendOutput(welcome, ACCENT_COLOR);
    }, "欢迎信息", "welcome");

    // about - 关于
    registerCommand("about", [ui](const QStringList&) {
        QString info = QString(
            "\n"
            "  ══════════════════════════════════════\n"
            "           关于 Termail\n"
            "  ══════════════════════════════════════\n"
            "\n"
            "  名称:    Termail终端\n"
            "  版本:    %1\n"
            "  类型:    跨平台终端模拟器\n"
            "  框架:    Qt6 + C++17\n"
            "  平台:    Android / Linux / Windows\n"
            "\n"
            "  ─────── 作者信息 ───────\n"
            "  制作:    rivers晟\n"
            "  GitHub:  rivers-sheng\n"
            "  邮箱:    k-jiangsheng@outlook.com\n"
            "\n"
            "  ─────── 功能 ───────\n"
            "  • 200+ 内置命令\n"
            "  • 内置 toybox 扩展\n"
            "  • SSH 密钥管理\n"
            "  • 多会话\n"
            "  • HTTP 多用户服务器\n"
            "  • Lua 脚本引擎\n"
            "\n"
            "  ─────── 许可 ───────\n"
            "  GNU AFFERO GENERAL PUBLIC LICENSE\n"
            "  输入 license 查看完整许可\n"
            "\n"
            "  ══════════════════════════════════════\n"
        ).arg(APP_VERSION);
        ui->appendOutput(info, ACCENT_COLOR);
    }, "关于", "about");

    // changelog - 更新日志
    registerCommand("changelog", [ui](const QStringList&) {
        QString log = QString(
            "\n"
            "  ══════════════════════════════════════\n"
            "  更新日志-见https://github.com/rivers-sheng/Termail-zd\n"
            "  ══════════════════════════════════════\n"
        ).arg(QDateTime::currentDateTime().toString("yyyy-MM-dd"));
        ui->appendOutput(log, ACCENT_COLOR);
    }, "更新日志", "changelog");

    // tips - 随机使用技巧
    registerCommand("tips", [ui](const QStringList&) {
        QStringList tips = {
            "输入 help <命令> 查看具体用法",
            "Ctrl+L 快速清屏",
            "↑ ↓ 浏览命令历史",
            "Tab 自动补全命令和路径",
            "输入 session new 创建多会话",
            "输入 pre-position 查看前置命令",
            "输入 bb <命令> 调用系统 toybox 工具",
            "输入 serve 启动 HTTP 服务器",
            "输入 tetris / snake / g2048 玩游戏",
            "输入 tips 查看其他技巧",
            "输入 export-config 导出配置",
            "输入 backup 备份所有设置",
            "输入 alias ll='ls -la' 设置别名",
            "输入 log 查看系统日志",
            "输入 ssh-keygen 生成SSH密钥",
            "长按输出区可选中复制",
            "输入 stop 停止运行中的命令",
            "输入 cmenu 查看扩展命令菜单",
            "输入 config-info 查看配置",
            "输入 watch-mem 查看内存占用",
            "输入 battery 查看电量",
            "输入 speedtest 测试网速",
            "输入 pass-gen 生成随机密码",
            "输入 timestamp 时间戳转换",
            "输入 lang en/zh切换中英文",
        };

        int idx = QRandomGenerator::global()->bounded(tips.size());
        ui->appendOutput("💡 使用技巧:\n", ACCENT_COLOR);
        ui->appendOutput("  " + tips[idx] + "\n", PROMPT_COLOR);
        ui->appendOutput("\n再输入 tips 查看其他技巧\n", INFO_COLOR);
    }, "使用技巧", "tips");

    // version - 版本信息
    registerCommand("version", [ui](const QStringList&) {
        ui->appendOutput(QString("Termail终端 %1\n").arg(APP_VERSION), ACCENT_COLOR);
        ui->appendOutput("Qt: " QT_VERSION_STR "\n", INFO_COLOR);
        ui->appendOutput("编译: " __DATE__ " " __TIME__ "\n", INFO_COLOR);
    }, "版本", "version");
}