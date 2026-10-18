#include <QApplication>
#include "commands.h"
#include "terminal/terminal.h"
#include "utils/utils.h"

void CommandRegistry::registerMany3Commands(TerminalWidget* ui) {
    // ========== 查找/文档 ==========
    registerCommand("whereis", [ui](const QStringList& a){ CommandHelper::runExternal("whereis", a, ui->currentDir, ui); }, "查找命令位置", "whereis");
    registerCommand("locate", [ui](const QStringList& a){ CommandHelper::runExternal("locate", a, ui->currentDir, ui); }, "快速查找", "locate");
    registerCommand("updatedb", [ui](const QStringList& a){ CommandHelper::runExternal("updatedb", a, ui->currentDir, ui); }, "更新数据库", "updatedb");
    registerCommand("man", [ui](const QStringList& a){ CommandHelper::runExternal("man", a, ui->currentDir, ui); }, "手册页", "man");
    registerCommand("info", [ui](const QStringList& a){ CommandHelper::runExternal("info", a, ui->currentDir, ui); }, "信息页", "info");
    registerCommand("whatis", [ui](const QStringList& a){ CommandHelper::runExternal("whatis", a, ui->currentDir, ui); }, "命令简介", "whatis");
    registerCommand("apropos", [ui](const QStringList& a){ CommandHelper::runExternal("apropos", a, ui->currentDir, ui); }, "搜索手册", "apropos");

    // ========== 文本处理 ==========
    registerCommand("awk", [ui](const QStringList& a){ CommandHelper::runExternal("awk", a, ui->currentDir, ui); }, "文本处理", "awk");
    registerCommand("perl", [ui](const QStringList& a){ CommandHelper::runExternal("perl", a, ui->currentDir, ui); }, "Perl脚本", "perl");
    registerCommand("egrep", [ui](const QStringList& a){ CommandHelper::runExternal("egrep", a, ui->currentDir, ui); }, "扩展grep", "egrep");
    registerCommand("fgrep", [ui](const QStringList& a){ CommandHelper::runExternal("fgrep", a, ui->currentDir, ui); }, "固定grep", "fgrep");
    registerCommand("printf", [ui](const QStringList& a){ CommandHelper::runExternal("printf", a, ui->currentDir, ui); }, "格式化输出", "printf");
    registerCommand("column", [ui](const QStringList& a){ CommandHelper::runExternal("column", a, ui->currentDir, ui); }, "列格式化", "column");
    registerCommand("expand", [ui](const QStringList& a){ CommandHelper::runExternal("expand", a, ui->currentDir, ui); }, "展开制表符", "expand");
    registerCommand("unexpand", [ui](const QStringList& a){ CommandHelper::runExternal("unexpand", a, ui->currentDir, ui); }, "压缩制表符", "unexpand");
    registerCommand("od", [ui](const QStringList& a){ CommandHelper::runExternal("od", a, ui->currentDir, ui); }, "八进制显示", "od");
    registerCommand("hexdump", [ui](const QStringList& a){ CommandHelper::runExternal("hexdump", a, ui->currentDir, ui); }, "十六进制显示", "hexdump");
    registerCommand("xxd", [ui](const QStringList& a){ CommandHelper::runExternal("xxd", a, ui->currentDir, ui); }, "十六进制转换", "xxd");
    registerCommand("patch", [ui](const QStringList& a){ CommandHelper::runExternal("patch", a, ui->currentDir, ui); }, "补丁", "patch");
    registerCommand("comm", [ui](const QStringList& a){ CommandHelper::runExternal("comm", a, ui->currentDir, ui); }, "文件对比", "comm");
    registerCommand("cmp", [ui](const QStringList& a){ CommandHelper::runExternal("cmp", a, ui->currentDir, ui); }, "字节对比", "cmp");
    registerCommand("fold", [ui](const QStringList& a){ CommandHelper::runExternal("fold", a, ui->currentDir, ui); }, "折叠行", "fold");

    // ========== 日期/时间 ==========
    registerCommand("cal", [ui](const QStringList& a){ CommandHelper::runExternal("cal", a, ui->currentDir, ui); }, "日历", "cal");
    registerCommand("ncal", [ui](const QStringList& a){ CommandHelper::runExternal("ncal", a, ui->currentDir, ui); }, "日历", "ncal");
    registerCommand("hwclock", [ui](const QStringList& a){ CommandHelper::runExternal("hwclock", a, ui->currentDir, ui); }, "硬件时钟", "hwclock");
    registerCommand("timedatectl", [ui](const QStringList& a){ CommandHelper::runExternal("timedatectl", a, ui->currentDir, ui); }, "时间日期控制", "timedatectl");

    // ========== 网络额外 ==========
    registerCommand("ip", [ui](const QStringList& a){ CommandHelper::runExternal("ip", a, ui->currentDir, ui); }, "网络配置", "ip");
    registerCommand("ss", [ui](const QStringList& a){ CommandHelper::runExternal("ss", a, ui->currentDir, ui); }, "Socket统计", "ss");
    registerCommand("iptables", [ui](const QStringList& a){ CommandHelper::runExternal("iptables", a, ui->currentDir, ui); }, "防火墙", "iptables");
    registerCommand("nft", [ui](const QStringList& a){ CommandHelper::runExternal("nft", a, ui->currentDir, ui); }, "NFTables", "nft");
    registerCommand("ethtool", [ui](const QStringList& a){ CommandHelper::runExternal("ethtool", a, ui->currentDir, ui); }, "网卡工具", "ethtool");
    registerCommand("iwconfig", [ui](const QStringList& a){ CommandHelper::runExternal("iwconfig", a, ui->currentDir, ui); }, "无线配置", "iwconfig");
    registerCommand("iwlist", [ui](const QStringList& a){ CommandHelper::runExternal("iwlist", a, ui->currentDir, ui); }, "无线扫描", "iwlist");
    registerCommand("hostname", [ui](const QStringList& a){ ui->appendOutput(ui->hostname + "\n"); }, "主机名", "hostname");
    registerCommand("resolvectl", [ui](const QStringList& a){ CommandHelper::runExternal("resolvectl", a, ui->currentDir, ui); }, "DNS解析", "resolvectl");

    // ========== 数学/其他 ==========
    registerCommand("dc", [ui](const QStringList& a){ CommandHelper::runExternal("dc", a, ui->currentDir, ui); }, "计算器", "dc");
    registerCommand("numfmt", [ui](const QStringList& a){ CommandHelper::runExternal("numfmt", a, ui->currentDir, ui); }, "数字格式化", "numfmt");
    registerCommand("shred", [ui](const QStringList& a){ CommandHelper::runExternal("shred", a, ui->currentDir, ui); }, "安全删除", "shred");
    registerCommand("truncate", [ui](const QStringList& a){ CommandHelper::runExternal("truncate", a, ui->currentDir, ui); }, "截断文件", "truncate");
    registerCommand("mktemp", [ui](const QStringList& a){ CommandHelper::runExternal("mktemp", a, ui->currentDir, ui); }, "临时文件", "mktemp");
    registerCommand("realpath", [ui](const QStringList& a){ CommandHelper::runExternal("realpath", a, ui->currentDir, ui); }, "绝对路径", "realpath");
    registerCommand("readlink", [ui](const QStringList& a){ CommandHelper::runExternal("readlink", a, ui->currentDir, ui); }, "链接目标", "readlink");
    registerCommand("basename", [ui](const QStringList& a){ CommandHelper::runExternal("basename", a, ui->currentDir, ui); }, "文件名", "basename");
    registerCommand("dirname", [ui](const QStringList& a){ CommandHelper::runExternal("dirname", a, ui->currentDir, ui); }, "目录名", "dirname");
    registerCommand("stat", [ui](const QStringList& a){ CommandHelper::runExternal("stat", a, ui->currentDir, ui); }, "文件状态", "stat");
    registerCommand("file", [ui](const QStringList& a){ CommandHelper::runExternal("file", a, ui->currentDir, ui); }, "文件类型", "file");
    registerCommand("du", [ui](const QStringList& a){ CommandHelper::runExternal("du", a, ui->currentDir, ui); }, "磁盘使用", "du");
    registerCommand("df", [ui](const QStringList& a){ CommandHelper::runExternal("df", a, ui->currentDir, ui); }, "磁盘空间", "df");
    registerCommand("free", [ui](const QStringList& a){ CommandHelper::runExternal("free", a, ui->currentDir, ui); }, "内存", "free");
    registerCommand("uname", [ui](const QStringList& a){ CommandHelper::runExternal("uname", a, ui->currentDir, ui); }, "系统信息", "uname");
    registerCommand("uptime", [ui](const QStringList& a){ CommandHelper::runExternal("uptime", a, ui->currentDir, ui); }, "运行时间", "uptime");
    registerCommand("hostid", [ui](const QStringList& a){ CommandHelper::runExternal("hostid", a, ui->currentDir, ui); }, "主机ID", "hostid");
    registerCommand("nproc", [ui](const QStringList& a){ CommandHelper::runExternal("nproc", a, ui->currentDir, ui); }, "CPU数", "nproc");
    registerCommand("getconf", [ui](const QStringList& a){ CommandHelper::runExternal("getconf", a, ui->currentDir, ui); }, "系统配置", "getconf");
    registerCommand("ulimit", [ui](const QStringList& a){ CommandHelper::runExternal("ulimit", a, ui->currentDir, ui); }, "资源限制", "ulimit");
    registerCommand("watch", [ui](const QStringList& a){ CommandHelper::runExternal("watch", a, ui->currentDir, ui); }, "重复执行", "watch");
    registerCommand("xargs", [ui](const QStringList& a){ CommandHelper::runExternal("xargs", a, ui->currentDir, ui); }, "参数传递", "xargs");
    registerCommand("tee", [ui](const QStringList& a){ CommandHelper::runExternal("tee", a, ui->currentDir, ui); }, "分流输出", "tee");
    registerCommand("yes", [ui](const QStringList& a){ CommandHelper::runExternal("yes", a, ui->currentDir, ui); }, "重复输出", "yes");
    registerCommand("seq", [ui](const QStringList& a){ CommandHelper::runExternal("seq", a, ui->currentDir, ui); }, "序列", "seq");
    registerCommand("shuf", [ui](const QStringList& a){ CommandHelper::runExternal("shuf", a, ui->currentDir, ui); }, "随机排列", "shuf");
    registerCommand("timeout", [ui](const QStringList& a){ CommandHelper::runExternal("timeout", a, ui->currentDir, ui); }, "超时执行", "timeout");
    registerCommand("env", [ui](const QStringList& a){ CommandHelper::runExternal("env", a, ui->currentDir, ui); }, "环境变量", "env");
    registerCommand("printenv", [ui](const QStringList& a){ CommandHelper::runExternal("printenv", a, ui->currentDir, ui); }, "打印环境", "printenv");
    registerCommand("unset", [ui](const QStringList& a){ CommandHelper::runExternal("unset", a, ui->currentDir, ui); }, "删除变量", "unset");
    registerCommand("export", [ui](const QStringList& a){ CommandHelper::runExternal("export", a, ui->currentDir, ui); }, "导出变量", "export");
    registerCommand("source", [ui](const QStringList& a){ CommandHelper::runExternal("source", a, ui->currentDir, ui); }, "执行脚本", "source");
    registerCommand("alias", [ui](const QStringList& a){ CommandHelper::runExternal("alias", a, ui->currentDir, ui); }, "别名", "alias");
    registerCommand("unalias", [ui](const QStringList& a){ CommandHelper::runExternal("unalias", a, ui->currentDir, ui); }, "删除别名", "unalias");
    registerCommand("history", [ui](const QStringList& a){ CommandHelper::runExternal("history", a, ui->currentDir, ui); }, "历史", "history");
    registerCommand("clear", [ui](const QStringList& a){ ui->clearScreen(); }, "清屏", "clear");
    registerCommand("exit", [ui](const QStringList& a){ qApp->quit(); }, "退出", "exit");
    registerCommand("logout", [ui](const QStringList& a){ qApp->quit(); }, "注销", "logout");
}