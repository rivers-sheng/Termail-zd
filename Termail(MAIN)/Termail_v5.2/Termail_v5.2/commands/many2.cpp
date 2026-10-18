#include "commands.h"
#include "terminal/terminal.h"
#include "utils/utils.h"

void CommandRegistry::registerMany2Commands(TerminalWidget* ui) {
    // ========== 磁盘/文件系统 ==========
    registerCommand("mount", [ui](const QStringList& a){ CommandHelper::runExternal("mount", a, ui->currentDir, ui); }, "挂载文件系统", "mount");
    registerCommand("umount", [ui](const QStringList& a){ CommandHelper::runExternal("umount", a, ui->currentDir, ui); }, "卸载文件系统", "umount");
    registerCommand("fdisk", [ui](const QStringList& a){ CommandHelper::runExternal("fdisk", a, ui->currentDir, ui); }, "磁盘分区", "fdisk");
    registerCommand("mkfs", [ui](const QStringList& a){ CommandHelper::runExternal("mkfs", a, ui->currentDir, ui); }, "创建文件系统", "mkfs");
    registerCommand("mkfs.ext4", [ui](const QStringList& a){ CommandHelper::runExternal("mkfs.ext4", a, ui->currentDir, ui); }, "创建ext4", "mkfs.ext4");
    registerCommand("mkfs.ext3", [ui](const QStringList& a){ CommandHelper::runExternal("mkfs.ext3", a, ui->currentDir, ui); }, "创建ext3", "mkfs.ext3");
    registerCommand("mkfs.vfat", [ui](const QStringList& a){ CommandHelper::runExternal("mkfs.vfat", a, ui->currentDir, ui); }, "创建vfat", "mkfs.vfat");
    registerCommand("fsck", [ui](const QStringList& a){ CommandHelper::runExternal("fsck", a, ui->currentDir, ui); }, "检查文件系统", "fsck");
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
    registerCommand("sar", [ui](const QStringList& a){ CommandHelper::runExternal("sar", a, ui->currentDir, ui); }, "系统活动报告", "sar");
    registerCommand("mpstat", [ui](const QStringList& a){ CommandHelper::runExternal("mpstat", a, ui->currentDir, ui); }, "CPU统计", "mpstat");

    // ========== 硬件 ==========
    registerCommand("lspci", [ui](const QStringList& a){ CommandHelper::runExternal("lspci", a, ui->currentDir, ui); }, "PCI设备", "lspci");
    registerCommand("lsusb", [ui](const QStringList& a){ CommandHelper::runExternal("lsusb", a, ui->currentDir, ui); }, "USB设备", "lsusb");
    registerCommand("lshw", [ui](const QStringList& a){ CommandHelper::runExternal("lshw", a, ui->currentDir, ui); }, "硬件列表", "lshw");
    registerCommand("dmidecode", [ui](const QStringList& a){ CommandHelper::runExternal("dmidecode", a, ui->currentDir, ui); }, "DMI信息", "dmidecode");
    registerCommand("hdparm", [ui](const QStringList& a){ CommandHelper::runExternal("hdparm", a, ui->currentDir, ui); }, "磁盘参数", "hdparm");
    registerCommand("sdparm", [ui](const QStringList& a){ CommandHelper::runExternal("sdparm", a, ui->currentDir, ui); }, "SCSI参数", "sdparm");
    registerCommand("sensors", [ui](const QStringList& a){ CommandHelper::runExternal("sensors", a, ui->currentDir, ui); }, "温度传感器", "sensors");
    registerCommand("hwinfo", [ui](const QStringList& a){ CommandHelper::runExternal("hwinfo", a, ui->currentDir, ui); }, "硬件信息", "hwinfo");

    // ========== 进程管理 ==========
    registerCommand("htop", [ui](const QStringList& a){ CommandHelper::runExternal("htop", a, ui->currentDir, ui); }, "交互式进程查看", "htop");
    registerCommand("killall", [ui](const QStringList& a){ CommandHelper::runExternal("killall", a, ui->currentDir, ui); }, "按名杀进程", "killall");
    registerCommand("pkill", [ui](const QStringList& a){ CommandHelper::runExternal("pkill", a, ui->currentDir, ui); }, "按名杀进程", "pkill");
    registerCommand("pgrep", [ui](const QStringList& a){ CommandHelper::runExternal("pgrep", a, ui->currentDir, ui); }, "查找进程", "pgrep");
    registerCommand("nice", [ui](const QStringList& a){ CommandHelper::runExternal("nice", a, ui->currentDir, ui); }, "调整优先级", "nice");
    registerCommand("renice", [ui](const QStringList& a){ CommandHelper::runExternal("renice", a, ui->currentDir, ui); }, "调整优先级", "renice");
    registerCommand("nohup", [ui](const QStringList& a){ CommandHelper::runExternal("nohup", a, ui->currentDir, ui); }, "后台运行", "nohup");
    registerCommand("bg", [ui](const QStringList& a){ CommandHelper::runExternal("bg", a, ui->currentDir, ui); }, "后台任务", "bg");
    registerCommand("fg", [ui](const QStringList& a){ CommandHelper::runExternal("fg", a, ui->currentDir, ui); }, "前台任务", "fg");
    registerCommand("pidof", [ui](const QStringList& a){ CommandHelper::runExternal("pidof", a, ui->currentDir, ui); }, "查找PID", "pidof");

    // ========== 用户管理 ==========
    registerCommand("useradd", [ui](const QStringList& a){ CommandHelper::runExternal("useradd", a, ui->currentDir, ui); }, "添加用户", "useradd");
    registerCommand("userdel", [ui](const QStringList& a){ CommandHelper::runExternal("userdel", a, ui->currentDir, ui); }, "删除用户", "userdel");
    registerCommand("usermod", [ui](const QStringList& a){ CommandHelper::runExternal("usermod", a, ui->currentDir, ui); }, "修改用户", "usermod");
    registerCommand("groupadd", [ui](const QStringList& a){ CommandHelper::runExternal("groupadd", a, ui->currentDir, ui); }, "添加组", "groupadd");
    registerCommand("groupdel", [ui](const QStringList& a){ CommandHelper::runExternal("groupdel", a, ui->currentDir, ui); }, "删除组", "groupdel");
    registerCommand("groupmod", [ui](const QStringList& a){ CommandHelper::runExternal("groupmod", a, ui->currentDir, ui); }, "修改组", "groupmod");
    registerCommand("chown", [ui](const QStringList& a){ CommandHelper::runExternal("chown", a, ui->currentDir, ui); }, "更改所有者", "chown");
    registerCommand("chgrp", [ui](const QStringList& a){ CommandHelper::runExternal("chgrp", a, ui->currentDir, ui); }, "更改组", "chgrp");
    registerCommand("chage", [ui](const QStringList& a){ CommandHelper::runExternal("chage", a, ui->currentDir, ui); }, "密码过期", "chage");
    registerCommand("lastlog", [ui](const QStringList& a){ CommandHelper::runExternal("lastlog", a, ui->currentDir, ui); }, "登录日志", "lastlog");

    // ========== 系统服务 ==========
    registerCommand("systemctl", [ui](const QStringList& a){ CommandHelper::runExternal("systemctl", a, ui->currentDir, ui); }, "系统服务管理", "systemctl");
    registerCommand("service", [ui](const QStringList& a){ CommandHelper::runExternal("service", a, ui->currentDir, ui); }, "服务管理", "service");
    registerCommand("shutdown", [ui](const QStringList& a){ CommandHelper::runExternal("shutdown", a, ui->currentDir, ui); }, "关机", "shutdown");
    registerCommand("reboot", [ui](const QStringList& a){ CommandHelper::runExternal("reboot", a, ui->currentDir, ui); }, "重启", "reboot");
    registerCommand("poweroff", [ui](const QStringList& a){ CommandHelper::runExternal("poweroff", a, ui->currentDir, ui); }, "关机", "poweroff");
    registerCommand("halt", [ui](const QStringList& a){ CommandHelper::runExternal("halt", a, ui->currentDir, ui); }, "停机", "halt");
    registerCommand("init", [ui](const QStringList& a){ CommandHelper::runExternal("init", a, ui->currentDir, ui); }, "初始化", "init");
    registerCommand("runlevel", [ui](const QStringList& a){ CommandHelper::runExternal("runlevel", a, ui->currentDir, ui); }, "运行级别", "runlevel");
    registerCommand("dmesg", [ui](const QStringList& a){ CommandHelper::runExternal("dmesg", a, ui->currentDir, ui); }, "内核日志", "dmesg");
    registerCommand("journalctl", [ui](const QStringList& a){ CommandHelper::runExternal("journalctl", a, ui->currentDir, ui); }, "日志查看", "journalctl");
}