#include "commands.h"
#include "terminal/terminal.h"
#include "utils/utils.h"

void CommandRegistry::registerMany2Commands(TerminalWidget* ui) {

    // 磁盘/文件系统
    registerCommand("mount", [ui](const QStringList& a){ CommandHelper::runExternal("mount", a, ui->currentDir, ui); }, "挂载", "mount");
    registerCommand("umount", [ui](const QStringList& a){ CommandHelper::runExternal("umount", a, ui->currentDir, ui); }, "卸载", "umount");
    registerCommand("fdisk", [ui](const QStringList& a){ CommandHelper::runExternal("fdisk", a, ui->currentDir, ui); }, "分区", "fdisk");
    registerCommand("mkfs", [ui](const QStringList& a){ CommandHelper::runExternal("mkfs", a, ui->currentDir, ui); }, "格式化", "mkfs");
    registerCommand("fsck", [ui](const QStringList& a){ CommandHelper::runExternal("fsck", a, ui->currentDir, ui); }, "检查修复", "fsck");
    registerCommand("dd", [ui](const QStringList& a){ CommandHelper::runExternal("dd", a, ui->currentDir, ui); }, "磁盘拷贝", "dd");
    registerCommand("sync", [ui](const QStringList& a){ CommandHelper::runExternal("sync", a, ui->currentDir, ui); }, "同步磁盘", "sync");
    registerCommand("swapon", [ui](const QStringList& a){ CommandHelper::runExternal("swapon", a, ui->currentDir, ui); }, "启用交换", "swapon");
    registerCommand("swapoff", [ui](const QStringList& a){ CommandHelper::runExternal("swapoff", a, ui->currentDir, ui); }, "禁用交换", "swapoff");
    registerCommand("losetup", [ui](const QStringList& a){ CommandHelper::runExternal("losetup", a, ui->currentDir, ui); }, "循环设备", "losetup");
    registerCommand("blkid", [ui](const QStringList& a){ CommandHelper::runExternal("blkid", a, ui->currentDir, ui); }, "块设备ID", "blkid");
    registerCommand("lsblk", [ui](const QStringList& a){ CommandHelper::runExternal("lsblk", a, ui->currentDir, ui); }, "块设备列表", "lsblk");
    registerCommand("parted", [ui](const QStringList& a){ CommandHelper::runExternal("parted", a, ui->currentDir, ui); }, "分区工具", "parted");
    registerCommand("iostat", [ui](const QStringList& a){ CommandHelper::runExternal("iostat", a, ui->currentDir, ui); }, "IO统计", "iostat");
    registerCommand("vmstat", [ui](const QStringList& a){ CommandHelper::runExternal("vmstat", a, ui->currentDir, ui); }, "虚拟内存统计", "vmstat");

    // 硬件
    registerCommand("lspci", [ui](const QStringList& a){ CommandHelper::runExternal("lspci", a, ui->currentDir, ui); }, "PCI设备", "lspci");
    registerCommand("lsusb", [ui](const QStringList& a){ CommandHelper::runExternal("lsusb", a, ui->currentDir, ui); }, "USB设备", "lsusb");
    registerCommand("lshw", [ui](const QStringList& a){ CommandHelper::runExternal("lshw", a, ui->currentDir, ui); }, "硬件列表", "lshw");
    registerCommand("dmidecode", [ui](const QStringList& a){ CommandHelper::runExternal("dmidecode", a, ui->currentDir, ui); }, "DMI信息", "dmidecode");
    registerCommand("hdparm", [ui](const QStringList& a){ CommandHelper::runExternal("hdparm", a, ui->currentDir, ui); }, "磁盘参数", "hdparm");
    registerCommand("sensors", [ui](const QStringList& a){ CommandHelper::runExternal("sensors", a, ui->currentDir, ui); }, "传感器", "sensors");

    // 进程
    registerCommand("htop", [ui](const QStringList& a){ CommandHelper::runExternal("htop", a, ui->currentDir, ui); }, "交互式进程", "htop");
    registerCommand("killall", [ui](const QStringList& a){ CommandHelper::runExternal("killall", a, ui->currentDir, ui); }, "按名杀进程", "killall");
    registerCommand("pkill", [ui](const QStringList& a){ CommandHelper::runExternal("pkill", a, ui->currentDir, ui); }, "按名杀进程", "pkill");
    registerCommand("pgrep", [ui](const QStringList& a){ CommandHelper::runExternal("pgrep", a, ui->currentDir, ui); }, "查找进程", "pgrep");
    registerCommand("nice", [ui](const QStringList& a){ CommandHelper::runExternal("nice", a, ui->currentDir, ui); }, "调整优先级", "nice");
    registerCommand("renice", [ui](const QStringList& a){ CommandHelper::runExternal("renice", a, ui->currentDir, ui); }, "调整优先级", "renice");
    registerCommand("nohup", [ui](const QStringList& a){ CommandHelper::runExternal("nohup", a, ui->currentDir, ui); }, "后台运行", "nohup");
    registerCommand("pidof", [ui](const QStringList& a){ CommandHelper::runExternal("pidof", a, ui->currentDir, ui); }, "查找PID", "pidof");

    // 用户管理
    registerCommand("useradd", [ui](const QStringList& a){ CommandHelper::runExternal("useradd", a, ui->currentDir, ui); }, "添加用户", "useradd");
    registerCommand("userdel", [ui](const QStringList& a){ CommandHelper::runExternal("userdel", a, ui->currentDir, ui); }, "删除用户", "userdel");
    registerCommand("usermod", [ui](const QStringList& a){ CommandHelper::runExternal("usermod", a, ui->currentDir, ui); }, "修改用户", "usermod");
    registerCommand("groupadd", [ui](const QStringList& a){ CommandHelper::runExternal("groupadd", a, ui->currentDir, ui); }, "添加组", "groupadd");
    registerCommand("groupdel", [ui](const QStringList& a){ CommandHelper::runExternal("groupdel", a, ui->currentDir, ui); }, "删除组", "groupdel");
    registerCommand("chown", [ui](const QStringList& a){ CommandHelper::runExternal("chown", a, ui->currentDir, ui); }, "更改所有者", "chown");
    registerCommand("chgrp", [ui](const QStringList& a){ CommandHelper::runExternal("chgrp", a, ui->currentDir, ui); }, "更改组", "chgrp");

    // 系统服务
    registerCommand("systemctl", [ui](const QStringList& a){ CommandHelper::runExternal("systemctl", a, ui->currentDir, ui); }, "服务管理", "systemctl");
    registerCommand("service", [ui](const QStringList& a){ CommandHelper::runExternal("service", a, ui->currentDir, ui); }, "服务管理", "service");
    registerCommand("shutdown", [ui](const QStringList& a){ CommandHelper::runExternal("shutdown", a, ui->currentDir, ui); }, "关机", "shutdown");
    registerCommand("reboot", [ui](const QStringList& a){ CommandHelper::runExternal("reboot", a, ui->currentDir, ui); }, "重启", "reboot");
    registerCommand("poweroff", [ui](const QStringList& a){ CommandHelper::runExternal("poweroff", a, ui->currentDir, ui); }, "关机", "poweroff");
    registerCommand("dmesg", [ui](const QStringList& a){ CommandHelper::runExternal("dmesg", a, ui->currentDir, ui); }, "内核日志", "dmesg");
    registerCommand("journalctl", [ui](const QStringList& a){ CommandHelper::runExternal("journalctl", a, ui->currentDir, ui); }, "日志查看", "journalctl");
}