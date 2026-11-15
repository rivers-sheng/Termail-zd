#ifndef SELFCHECK_H
#define SELFCHECK_H

#include <QString>
#include <QStringList>
#include <QFile>
#include <QStandardPaths>
#include <QFileInfo>
#ifdef Q_OS_ANDROID
#include <unistd.h>
#endif

inline QString selfCheckFilePath() {
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/termail/Self-check.txt";
}

// 获取已经落盘的白名单
inline QStringList getAvailableCommands() {
    QStringList list;
    QFile f(selfCheckFilePath());
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QStringList lines = QString(f.readAll()).split('\n');
        bool inAvailable = false;
        for (const QString& line : lines) {
            QString l = line.trimmed();
            if (l == "[Available]") { inAvailable = true; continue; }
            if (l == "[Missing]") { inAvailable = false; continue; }
            if (inAvailable && !l.isEmpty()) list.append(l);
        }
        f.close();
    }
    return list;
}

// 【免测区】：纯 C++ 写的 + 系统绝对存在且必须带参的（防误杀）
inline QStringList getSafeBuiltins() {
    return QStringList() << 
        "help" << "exit" << "quit" << "clear" << "cls" << "reset" << "echo" << "date" << "time" << "whoami" << "sysinfo" << "env" << 
        "ls" << "cd" << "pwd" << "mkdir" << "rm" << "rmdir" << "cat" << "cp" << "mv" << "touch" << "tree" << "find" << "grep" << 
        "wc" << "head" << "tail" << "more" << "catmore" << "less" << "diff" << "sort" << "uniq" << "cut" << "stat" << "file" << "du" << "df" << "chmod" << "which" << "dir" << "copy" << "move" << "rename" << "type" << "del" <<
        "ip-info" << "http-get" << "port-check" << "dns-flush" << "speedtest" << "watch-net" << "process-top" << "kill-all" <<
        "tr" << "rev" << "tac" << "nl" << "paste" << "split" << "join" << "fmt" << "sed" << "hash" << "base64" << "base64-decode" << "url-encode" << "url-decode" << 
        "uuid" << "random" << "timestamp" << "random-str" << "uuid-batch" << "json-format" << "hex-view" << "diff-color" << "md5" <<
        "calc" << "bc" << "expr" << "convert" << "units" << "hex" << "bin" << "oct" << "cowsay" << "fortune" << "banner" << "figlet" << "cmatrix" << 
        "sl" << "lolcat" << "ascii" << "morse" << "prime" << "factor" << "fibonacci" << "pi" << "e" << "phi" << "pass-gen" << "game-score" << "pong" << "tetris" << "snake" << "g2048" <<
        "session" << "snew" << "slist" << "ssw" << "skill" << "session-rename" << "session-info" << "session-export" << 
        "history" << "alias" << "unalias" << "export" << "source" << "exec" << "script" << "title" << "copy" << "paste-clip" << "bell" << "notify" <<
        "open" << "share" << "mount-sd" << "brightness" << "clip" <<
        "meminfo" << "cpu-temp" << "battery" << "disk-usage" << "watch-cpu" << "watch-mem" << "watch-battery" <<
        "edit" << "nano" << "create" << "lua" << "luainfo" << "luademo" << "bb" << "cmenu" << "pre-position" << 
        "serve" << "stop-serve" << "setfont" << "addcmd" << "delcmd" << "listcmd" <<
        "config-info" << "backup" << "restore" << "export-config" << "import-config" << "reset-all" << 
        "license" << "log" << "log-termail" << "exit-log" << "stop" << "root" << "lsroot" <<
        // 系统底层的且带参的
        "ping" << "ip" << "ifconfig" << "netstat" << "route" << "arp" << "ps" << "kill" << "killall" << "top" << "free" << "uname" << "uptime" << "hostname" << "id" << "who" << "w" << "users" << "last" << "mount" << "umount" << "sync" << "reboot" << "poweroff" << "sleep" << "yes" << "toybox" << "tar" << "gzip" << "gunzip" << "xz" <<
        "vi" << "vim" << "tee" << "jobs" << "about" << "get-busybox" << "changelog";
}

// 【测试区】：只测那些真的不知道有没有的
inline QStringList getExternalCommandsToTest() {
    return QStringList() << 
        "apt" << "apt-get" << "yum" << "dnf" << "pacman" << "brew" << "ar" << "as" << 
        "autoconf" << "automake" << "awk" << "basename" << "bzip2" << "bunzip2" << "cal" << "chgrp" << 
        "chown" << "chroot" << "cmp" << "column" << "comm" << "dc" << "dd" << "dmesg" << "dmidecode" << 
        "dig" << "dirname" << "ethtool" << "fdisk" << "fgrep" << "fold" << "fsck" << "ftp" << "g++" << 
        "gcc" << "gdb" << "gprof" << "groupadd" << "groupdel" << "groups" << "hdparm" << "host" << 
        "hwclock" << "ipconfig" << "iptables" << "iwconfig" << "journalctl" << "ld" << "ldd" << "locate" << "lscpu" << "lshw" << 
        "lspci" << "lsusb" << "ltrace" << "man" << "mkfs" << "mktemp" << "nc" << "ncal" << 
        "nice" << "nm" << "nmap" << "nohup" << "nproc" << "nslookup" << "numfmt" << "objdump" << "od" << 
        "patch" << "perl" << "pgrep" << "pidof" << "pkg-config" << "pkill" << "printenv" << 
        "printf" << "psql" << "ranlib" << "rar" << "readelf" << "readlink" << "realpath" << 
        "redis" << "renice" << "rsync" << "sensors" << "service" << "sftp" << "sha1sum" << 
        "sha256sum" << "sha512sum" << "shutdown" << "shred" << "shuf" << "socat" << "ssh" << 
        "ss" << "strace" << "strings" << "strip" << "su" << "sudo" << "swapoff" << "swapon" << 
        "systemctl" << "tcpdump" << "telnet" << "traceroute" << "tracert" << "truncate" << "ulimit" << "unrar" << "unxz" << 
        "unzip" << "useradd" << "userdel" << "usermod" << "valgrind" << "vmstat" << "watch" << "whereis" << "whois" << 
        "xargs" << "xxd" << "zip" << "zcat" << "python" << "python3" << "pip" << "npm" << "java" << "javac";
}

// 【物理探测】：直接去系统目录看文件在不在，极快，绝不卡死
inline bool checkCmdExists(const QString& cmd) {
#ifdef Q_OS_ANDROID
    QStringList searchPaths = {"/system/bin", "/system/xbin", "/vendor/bin", "/sbin", "/data/adb/ksu/bin", "/data/adb/magisk"};
    for (const QString& path : searchPaths) {
        QString fullPath = path + "/" + cmd;
        if (access(fullPath.toUtf8().constData(), X_OK) == 0) {
            return true;
        }
    }
#endif
    // 桌面端兜底
    return !QStandardPaths::findExecutable(cmd).isEmpty();
}

#endif // SELFCHECK_H
