# Termail v5.2.0 更新日志

## 更新
- 版本号从 5.1.0 升级至 5.2.0，应用名改为 `Termail_v5.2`
- 命令体系大幅重组，新增大量外部命令封装，`help` 命令说明表同步扩充
- Android `versionCode` 更新为 7，应用标签更新为 `Termail_v5.2`

## 新增
- 新增源文件 `commands/many1.cpp`：开发工具类命令（gdb、cmake、strace、valgrind、ldd、objdump 等）、安全类命令（openssl、gpg、sudo、su 等）、压缩类命令（gzip、xz、7z、rar 等）、网络类命令（ssh、scp、rsync、nc、nmap、tcpdump 等）
- 新增源文件 `commands/many2.cpp`：磁盘/文件系统命令（mount、fdisk、mkfs、dd、lsblk 等）、硬件信息命令（lspci、lsusb、sensors 等）、进程管理命令（htop、killall、pgrep 等）、用户管理命令（useradd、chown 等）、系统服务命令（systemctl、shutdown、reboot 等）
- 新增源文件 `commands/many3.cpp`：查找文档命令（whereis、locate、man 等）、文本处理命令（awk、perl、hexdump 等）、日期时间命令（cal、timedatectl 等）、网络配置命令（ip、ss、iptables、ethtool 等）及其他系统命令（xargs、nproc、realpath 等）
- 新增源文件 `commands/log_commands.cpp`：
  - `log` 命令：全屏系统日志窗口，支持过滤、`/命令` 执行、与程序日志切换
  - `log-termail` 命令：Termail 程序日志窗口，支持写入 `termail.log` 及命令执行
  - `exit-log` 命令：关闭已打开的日志窗口
- `help` 命令说明表扩充，覆盖新增的众多命令

## 删除
- 部分命令注册分类在代码中的原有注册方式被拆分到新文件中，原 `registerNetworkCommands` 等部分命令改为外部调用实现