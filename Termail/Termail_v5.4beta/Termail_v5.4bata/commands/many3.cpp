#include <QApplication>
#include "commands.h"
#include "terminal/terminal.h"
#include "utils/utils.h"

#include <QProcess>
#include <QDir>
#include <QFile>
#include <QDateTime>

void CommandRegistry::registerMany3Commands(TerminalWidget* ui) {

    // ========== 查找/文档 ==========
    registerCommand("whereis", [ui](const QStringList& a){ CommandHelper::runExternal("whereis", a, ui->currentDir, ui); }, "查找命令", "whereis");
    registerCommand("locate", [ui](const QStringList& a){ CommandHelper::runExternal("locate", a, ui->currentDir, ui); }, "快速查找", "locate");
    registerCommand("updatedb", [ui](const QStringList& a){ CommandHelper::runExternal("updatedb", a, ui->currentDir, ui); }, "更新数据库", "updatedb");
    registerCommand("man", [ui](const QStringList& a){ CommandHelper::runExternal("man", a, ui->currentDir, ui); }, "手册", "man");
    registerCommand("info", [ui](const QStringList& a){ CommandHelper::runExternal("info", a, ui->currentDir, ui); }, "信息", "info");
    registerCommand("whatis", [ui](const QStringList& a){ CommandHelper::runExternal("whatis", a, ui->currentDir, ui); }, "简介", "whatis");
    registerCommand("apropos", [ui](const QStringList& a){ CommandHelper::runExternal("apropos", a, ui->currentDir, ui); }, "搜索手册", "apropos");

    // ========== 文本处理 ==========
    registerCommand("awk", [ui](const QStringList& a){ CommandHelper::runExternal("awk", a, ui->currentDir, ui); }, "文本处理", "awk");
    registerCommand("perl", [ui](const QStringList& a){ CommandHelper::runExternal("perl", a, ui->currentDir, ui); }, "Perl", "perl");
    registerCommand("egrep", [ui](const QStringList& a){ CommandHelper::runExternal("egrep", a, ui->currentDir, ui); }, "grep", "egrep");
    registerCommand("fgrep", [ui](const QStringList& a){ CommandHelper::runExternal("fgrep", a, ui->currentDir, ui); }, "grep", "fgrep");
    registerCommand("printf", [ui](const QStringList& a){ CommandHelper::runExternal("printf", a, ui->currentDir, ui); }, "格式化输出", "printf");
    registerCommand("column", [ui](const QStringList& a){ CommandHelper::runExternal("column", a, ui->currentDir, ui); }, "列格式化", "column");
    registerCommand("expand", [ui](const QStringList& a){ CommandHelper::runExternal("expand", a, ui->currentDir, ui); }, "制表符", "expand");
    registerCommand("unexpand", [ui](const QStringList& a){ CommandHelper::runExternal("unexpand", a, ui->currentDir, ui); }, "制表符", "unexpand");
    registerCommand("od", [ui](const QStringList& a){ CommandHelper::runExternal("od", a, ui->currentDir, ui); }, "八进制", "od");
    registerCommand("hexdump", [ui](const QStringList& a){ CommandHelper::runExternal("hexdump", a, ui->currentDir, ui); }, "十六进制", "hexdump");
    registerCommand("xxd", [ui](const QStringList& a){ CommandHelper::runExternal("xxd", a, ui->currentDir, ui); }, "十六进制", "xxd");
    registerCommand("patch", [ui](const QStringList& a){ CommandHelper::runExternal("patch", a, ui->currentDir, ui); }, "补丁", "patch");
    registerCommand("comm", [ui](const QStringList& a){ CommandHelper::runExternal("comm", a, ui->currentDir, ui); }, "对比", "comm");
    registerCommand("cmp", [ui](const QStringList& a){ CommandHelper::runExternal("cmp", a, ui->currentDir, ui); }, "对比", "cmp");
    registerCommand("fold", [ui](const QStringList& a){ CommandHelper::runExternal("fold", a, ui->currentDir, ui); }, "折叠", "fold");

    // ========== 日期 ==========
    registerCommand("cal", [ui](const QStringList& a){ CommandHelper::runExternal("cal", a, ui->currentDir, ui); }, "日历", "cal");
    registerCommand("ncal", [ui](const QStringList& a){ CommandHelper::runExternal("ncal", a, ui->currentDir, ui); }, "日历", "ncal");
    registerCommand("hwclock", [ui](const QStringList& a){ CommandHelper::runExternal("hwclock", a, ui->currentDir, ui); }, "硬件时钟", "hwclock");
    registerCommand("timedatectl", [ui](const QStringList& a){ CommandHelper::runExternal("timedatectl", a, ui->currentDir, ui); }, "时间控制", "timedatectl");

    // ========== 网络 ==========
    registerCommand("ip", [ui](const QStringList& a){ CommandHelper::runExternal("ip", a, ui->currentDir, ui); }, "网络配置", "ip");
    registerCommand("ss", [ui](const QStringList& a){ CommandHelper::runExternal("ss", a, ui->currentDir, ui); }, "Socket统计", "ss");
    registerCommand("iptables", [ui](const QStringList& a){ CommandHelper::runExternal("iptables", a, ui->currentDir, ui); }, "防火墙", "iptables");
    registerCommand("nft", [ui](const QStringList& a){ CommandHelper::runExternal("nft", a, ui->currentDir, ui); }, "NFTables", "nft");
    registerCommand("ethtool", [ui](const QStringList& a){ CommandHelper::runExternal("ethtool", a, ui->currentDir, ui); }, "网卡", "ethtool");
    registerCommand("iwconfig", [ui](const QStringList& a){ CommandHelper::runExternal("iwconfig", a, ui->currentDir, ui); }, "无线", "iwconfig");
    registerCommand("iwlist", [ui](const QStringList& a){ CommandHelper::runExternal("iwlist", a, ui->currentDir, ui); }, "无线扫描", "iwlist");
    registerCommand("resolvectl", [ui](const QStringList& a){ CommandHelper::runExternal("resolvectl", a, ui->currentDir, ui); }, "DNS", "resolvectl");

    // ========== 数学/其他 ==========
    registerCommand("dc", [ui](const QStringList& a){ CommandHelper::runExternal("dc", a, ui->currentDir, ui); }, "计算器", "dc");
    registerCommand("numfmt", [ui](const QStringList& a){ CommandHelper::runExternal("numfmt", a, ui->currentDir, ui); }, "数字格式化", "numfmt");
    registerCommand("shred", [ui](const QStringList&) {
        ui->appendOutput("shred 命令在 Android 上不可用，请用 rm 删除文件\n", ERROR_COLOR);
    }, "安全删除", "shred");
    registerCommand("truncate", [ui](const QStringList& a){ CommandHelper::runExternal("truncate", a, ui->currentDir, ui); }, "截断", "truncate");
    registerCommand("mktemp", [ui](const QStringList& a){ CommandHelper::runExternal("mktemp", a, ui->currentDir, ui); }, "临时文件", "mktemp");
    registerCommand("realpath", [ui](const QStringList& a){ CommandHelper::runExternal("realpath", a, ui->currentDir, ui); }, "绝对路径", "realpath");
    registerCommand("readlink", [ui](const QStringList& a){ CommandHelper::runExternal("readlink", a, ui->currentDir, ui); }, "链接目标", "readlink");
    registerCommand("basename", [ui](const QStringList& a){ CommandHelper::runExternal("basename", a, ui->currentDir, ui); }, "文件名", "basename");
    registerCommand("dirname", [ui](const QStringList& a){ CommandHelper::runExternal("dirname", a, ui->currentDir, ui); }, "目录名", "dirname");
    registerCommand("hostid", [ui](const QStringList& a){ CommandHelper::runExternal("hostid", a, ui->currentDir, ui); }, "主机ID", "hostid");
    registerCommand("nproc", [ui](const QStringList& a){ CommandHelper::runExternal("nproc", a, ui->currentDir, ui); }, "CPU数", "nproc");
    registerCommand("getconf", [ui](const QStringList& a){ CommandHelper::runExternal("getconf", a, ui->currentDir, ui); }, "配置", "getconf");
    registerCommand("ulimit", [ui](const QStringList& a){ CommandHelper::runExternal("ulimit", a, ui->currentDir, ui); }, "资源限制", "ulimit");
    registerCommand("xargs", [ui](const QStringList& a){ CommandHelper::runExternal("xargs", a, ui->currentDir, ui); }, "参数传递", "xargs");
    registerCommand("printenv", [ui](const QStringList& a){ CommandHelper::runExternal("printenv", a, ui->currentDir, ui); }, "环境", "printenv");
    registerCommand("unset", [ui](const QStringList& a){ CommandHelper::runExternal("unset", a, ui->currentDir, ui); }, "删除变量", "unset");

    // seq 修复（步长0检测+递减）
    registerCommand("seq", [ui](const QStringList& args) {
        if (args.size() == 1) {
            for (int i = 1; i <= args[0].toInt(); ++i) ui->appendOutput(QString::number(i) + "\n");
        } else if (args.size() == 2) {
            int start = args[0].toInt(), end = args[1].toInt();
            if (start <= end) for (int i = start; i <= end; ++i) ui->appendOutput(QString::number(i) + "\n");
            else for (int i = start; i >= end; --i) ui->appendOutput(QString::number(i) + "\n");
        } else if (args.size() >= 3) {
            int start = args[0].toInt(), step = args[1].toInt(), end = args[2].toInt();
            if (step == 0) { ui->appendOutput("步长不能为0\n", ERROR_COLOR); return; }
            if (step > 0) for (int i = start; i <= end; i += step) ui->appendOutput(QString::number(i) + "\n");
            else for (int i = start; i >= end; i += step) ui->appendOutput(QString::number(i) + "\n");
        }
    }, "序列", "seq [起] [步] <止>");

    // exit / logout
    registerCommand("exit", [ui](const QStringList&) {
        ui->saveCustomCommands();
        qApp->quit();
    }, "退出", "exit");

    registerCommand("logout", [ui](const QStringList&) {
        ui->saveCustomCommands();
        qApp->quit();
    }, "注销", "logout");
}