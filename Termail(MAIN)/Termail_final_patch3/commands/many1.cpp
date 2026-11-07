#include "commands.h"
#include "terminal/terminal.h"
#include "utils/utils.h"

void CommandRegistry::registerMany1Commands(TerminalWidget* ui) {

    // 开发工具
    registerCommand("gdb", [ui](const QStringList& a){ CommandHelper::runExternal("gdb", a, ui->currentDir, ui); }, "调试器", "gdb");
    registerCommand("gprof", [ui](const QStringList& a){ CommandHelper::runExternal("gprof", a, ui->currentDir, ui); }, "性能分析", "gprof");
    registerCommand("strace", [ui](const QStringList& a){ CommandHelper::runExternal("strace", a, ui->currentDir, ui); }, "系统调用追踪", "strace");
    registerCommand("ltrace", [ui](const QStringList& a){ CommandHelper::runExternal("ltrace", a, ui->currentDir, ui); }, "库调用追踪", "ltrace");
    registerCommand("valgrind", [ui](const QStringList& a){ CommandHelper::runExternal("valgrind", a, ui->currentDir, ui); }, "内存检测", "valgrind");
    registerCommand("cmake", [ui](const QStringList& a){ CommandHelper::runExternal("cmake", a, ui->currentDir, ui); }, "构建工具", "cmake");
    registerCommand("automake", [ui](const QStringList& a){ CommandHelper::runExternal("automake", a, ui->currentDir, ui); }, "自动化构建", "automake");
    registerCommand("autoconf", [ui](const QStringList& a){ CommandHelper::runExternal("autoconf", a, ui->currentDir, ui); }, "配置生成", "autoconf");
    registerCommand("pkg-config", [ui](const QStringList& a){ CommandHelper::runExternal("pkg-config", a, ui->currentDir, ui); }, "包配置", "pkg-config");
    registerCommand("ldd", [ui](const QStringList& a){ CommandHelper::runExternal("ldd", a, ui->currentDir, ui); }, "依赖查看", "ldd");
    registerCommand("nm", [ui](const QStringList& a){ CommandHelper::runExternal("nm", a, ui->currentDir, ui); }, "符号表", "nm");
    registerCommand("objdump", [ui](const QStringList& a){ CommandHelper::runExternal("objdump", a, ui->currentDir, ui); }, "反汇编", "objdump");
    registerCommand("readelf", [ui](const QStringList& a){ CommandHelper::runExternal("readelf", a, ui->currentDir, ui); }, "ELF查看", "readelf");
    registerCommand("strings", [ui](const QStringList& a){ CommandHelper::runExternal("strings", a, ui->currentDir, ui); }, "字符串提取", "strings");
    registerCommand("strip", [ui](const QStringList& a){ CommandHelper::runExternal("strip", a, ui->currentDir, ui); }, "符号剥离", "strip");
    registerCommand("ar", [ui](const QStringList& a){ CommandHelper::runExternal("ar", a, ui->currentDir, ui); }, "静态库", "ar");
    registerCommand("ranlib", [ui](const QStringList& a){ CommandHelper::runExternal("ranlib", a, ui->currentDir, ui); }, "库索引", "ranlib");
    registerCommand("as", [ui](const QStringList& a){ CommandHelper::runExternal("as", a, ui->currentDir, ui); }, "汇编器", "as");
    registerCommand("ld", [ui](const QStringList& a){ CommandHelper::runExternal("ld", a, ui->currentDir, ui); }, "链接器", "ld");

    // 安全
    registerCommand("openssl", [ui](const QStringList& a){ CommandHelper::runExternal("openssl", a, ui->currentDir, ui); }, "加密工具", "openssl");
    registerCommand("gpg", [ui](const QStringList& a){ CommandHelper::runExternal("gpg", a, ui->currentDir, ui); }, "GPG加密", "gpg");
    registerCommand("md5sum", [ui](const QStringList& a){ CommandHelper::runExternal("md5sum", a, ui->currentDir, ui); }, "MD5校验", "md5sum");
    registerCommand("sha1sum", [ui](const QStringList& a){ CommandHelper::runExternal("sha1sum", a, ui->currentDir, ui); }, "SHA1校验", "sha1sum");
    registerCommand("sha256sum", [ui](const QStringList& a){ CommandHelper::runExternal("sha256sum", a, ui->currentDir, ui); }, "SHA256校验", "sha256sum");
    registerCommand("sha512sum", [ui](const QStringList& a){ CommandHelper::runExternal("sha512sum", a, ui->currentDir, ui); }, "SHA512校验", "sha512sum");
    registerCommand("chroot", [ui](const QStringList& a){ CommandHelper::runExternal("chroot", a, ui->currentDir, ui); }, "切换根目录", "chroot");
    registerCommand("sudo", [ui](const QStringList& a){ CommandHelper::runExternal("sudo", a, ui->currentDir, ui); }, "提权执行", "sudo");
    registerCommand("su", [ui](const QStringList& a){ CommandHelper::runExternal("su", a, ui->currentDir, ui); }, "切换用户", "su");
    registerCommand("passwd", [ui](const QStringList& a){ CommandHelper::runExternal("passwd", a, ui->currentDir, ui); }, "修改密码", "passwd");

    // 压缩
    registerCommand("gzip", [ui](const QStringList& a){ CommandHelper::runExternal("gzip", a, ui->currentDir, ui); }, "压缩", "gzip");
    registerCommand("gunzip", [ui](const QStringList& a){ CommandHelper::runExternal("gunzip", a, ui->currentDir, ui); }, "解压", "gunzip");
    registerCommand("bzip2", [ui](const QStringList& a){ CommandHelper::runExternal("bzip2", a, ui->currentDir, ui); }, "压缩", "bzip2");
    registerCommand("bunzip2", [ui](const QStringList& a){ CommandHelper::runExternal("bunzip2", a, ui->currentDir, ui); }, "解压", "bunzip2");
    registerCommand("xz", [ui](const QStringList& a){ CommandHelper::runExternal("xz", a, ui->currentDir, ui); }, "压缩", "xz");
    registerCommand("unxz", [ui](const QStringList& a){ CommandHelper::runExternal("unxz", a, ui->currentDir, ui); }, "解压", "unxz");
    registerCommand("7z", [ui](const QStringList& a){ CommandHelper::runExternal("7z", a, ui->currentDir, ui); }, "7zip", "7z");
    registerCommand("rar", [ui](const QStringList& a){ CommandHelper::runExternal("rar", a, ui->currentDir, ui); }, "RAR压缩", "rar");
    registerCommand("unrar", [ui](const QStringList& a){ CommandHelper::runExternal("unrar", a, ui->currentDir, ui); }, "RAR解压", "unrar");
    registerCommand("zcat", [ui](const QStringList& a){ CommandHelper::runExternal("zcat", a, ui->currentDir, ui); }, "查看压缩", "zcat");
    registerCommand("zless", [ui](const QStringList& a){ CommandHelper::runExternal("zless", a, ui->currentDir, ui); }, "分页压缩", "zless");

    // 网络额外
    registerCommand("ssh", [ui](const QStringList& a){ CommandHelper::runExternal("ssh", a, ui->currentDir, ui); }, "SSH客户端", "ssh");
    registerCommand("scp", [ui](const QStringList& a){ CommandHelper::runExternal("scp", a, ui->currentDir, ui); }, "SCP传输", "scp");
    registerCommand("rsync", [ui](const QStringList& a){ CommandHelper::runExternal("rsync", a, ui->currentDir, ui); }, "同步工具", "rsync");
    registerCommand("ftp", [ui](const QStringList& a){ CommandHelper::runExternal("ftp", a, ui->currentDir, ui); }, "FTP客户端", "ftp");
    registerCommand("sftp", [ui](const QStringList& a){ CommandHelper::runExternal("sftp", a, ui->currentDir, ui); }, "SFTP客户端", "sftp");
    registerCommand("telnet", [ui](const QStringList& a){ CommandHelper::runExternal("telnet", a, ui->currentDir, ui); }, "Telnet", "telnet");
    registerCommand("nc", [ui](const QStringList& a){ CommandHelper::runExternal("nc", a, ui->currentDir, ui); }, "Netcat", "nc");
    registerCommand("socat", [ui](const QStringList& a){ CommandHelper::runExternal("socat", a, ui->currentDir, ui); }, "Socket工具", "socat");
    registerCommand("nmap", [ui](const QStringList& a){ CommandHelper::runExternal("nmap", a, ui->currentDir, ui); }, "端口扫描", "nmap");
    registerCommand("tcpdump", [ui](const QStringList& a){ CommandHelper::runExternal("tcpdump", a, ui->currentDir, ui); }, "抓包", "tcpdump");
    registerCommand("dig", [ui](const QStringList& a){ CommandHelper::runExternal("dig", a, ui->currentDir, ui); }, "DNS查询", "dig");
    registerCommand("host", [ui](const QStringList& a){ CommandHelper::runExternal("host", a, ui->currentDir, ui); }, "DNS查询", "host");
}