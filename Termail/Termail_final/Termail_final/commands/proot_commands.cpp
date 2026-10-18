#include "commands.h"
#include "terminal/terminal.h"
#include "utils/utils.h"

#include <QFile>
#include <QDir>
#include <QProcess>
#include <QFileInfo>

void CommandRegistry::registerProotCommands(TerminalWidget* ui) {

    // 辅助函数：获取当前发行版
    auto getDistro = [ui]() -> QString {
        QString marker = QDir(ui->currentDir).absoluteFilePath("proot-env/distro");
        QFile f(marker);
        if (f.open(QIODevice::ReadOnly)) {
            QString d = QString(f.readAll()).trimmed();
            f.close();
            if (!d.isEmpty()) return d;
        }
        return "alpine";
    };

    auto setDistro = [ui](const QString& name) {
        QString marker = QDir(ui->currentDir).absoluteFilePath("proot-env/distro");
        QDir().mkpath(QFileInfo(marker).absolutePath());
        QFile f(marker);
        if (f.open(QIODevice::WriteOnly)) {
            f.write(name.toUtf8());
            f.close();
        }
    };

    auto getRootfs = [ui](const QString& distro) -> QString {
        if (distro == "ubuntu")
            return QDir(ui->currentDir).absoluteFilePath("proot-env/ubuntu-rootfs");
        return QDir(ui->currentDir).absoluteFilePath("proot-env/rootfs");
    };

    // proot-setup - 安装 PRoot + Alpine
    registerCommand("proot-setup", [ui, setDistro](const QStringList&) {
        QString baseDir = QDir(ui->currentDir).absoluteFilePath("proot-env");
        QString prootPath = QDir(ui->currentDir).absoluteFilePath("proot");
        QString rootfsPath = baseDir + "/rootfs";
        QString tmpDir = QDir(ui->currentDir).absoluteFilePath("tmp");

        QDir().mkpath(tmpDir);

        if (QFile::exists(rootfsPath + "/bin")) {
            ui->appendOutput("Alpine 已安装\n", ERROR_COLOR);
            ui->appendOutput("用 proot-remove 删除后重试\n", INFO_COLOR);
            return;
        }

        ui->appendOutput("安装 PRoot + Alpine...\n", ACCENT_COLOR);

        // 复制 proot
        QFile prootRes(":/resources/proot/proot");
        if (!prootRes.open(QIODevice::ReadOnly)) {
            ui->appendOutput("proot 资源未找到\n", ERROR_COLOR);
            return;
        }
        QByteArray prootData = prootRes.readAll();
        prootRes.close();
        QFile prootDest(prootPath);
        if (!prootDest.open(QIODevice::WriteOnly)) {
            ui->appendOutput("无法写入 proot\n", ERROR_COLOR);
            return;
        }
        prootDest.write(prootData);
        prootDest.close();
        QFile::setPermissions(prootPath,
            QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner |
            QFileDevice::ReadGroup | QFileDevice::ExeGroup |
            QFileDevice::ReadOther | QFileDevice::ExeOther);
        ui->appendOutput(QString("proot 已复制 (%1 KB)\n").arg(prootData.size() / 1024), INFO_COLOR);

        // Alpine
        QDir().mkpath(baseDir);
        QString tarPath = baseDir + "/alpine.tar";
        QFile tarRes(":/resources/proot/alpine.tar");
        if (tarRes.open(QIODevice::ReadOnly)) {
            QFile tarDest(tarPath);
            if (tarDest.open(QIODevice::WriteOnly)) {
                tarDest.write(tarRes.readAll());
                tarDest.close();
            }
            tarRes.close();
        }

        QDir().mkpath(rootfsPath);
        ui->appendOutput("解压 Alpine...\n", ACCENT_COLOR);
        QProcess tarProc;
        tarProc.setWorkingDirectory(baseDir);
        QString busyboxPath = QDir(ui->currentDir).absoluteFilePath("busybox");
        if (QFile::exists(busyboxPath))
            tarProc.start(busyboxPath, QStringList() << "tar" << "-xzf" << tarPath << "-C" << rootfsPath);
        else
            tarProc.start("tar", QStringList() << "-xzf" << tarPath << "-C" << rootfsPath);
        tarProc.waitForFinished(60000);
        QFile::remove(tarPath);

        if (QFile::exists(rootfsPath + "/bin")) {
            setDistro("alpine");
            ui->appendOutput("Alpine 安装完成\n", PROMPT_COLOR);
            ui->appendOutput("输入 proot-mirror 切换镜像\n", INFO_COLOR);
            ui->appendOutput("输入 set-dns 119.29.29.29 设置DNS\n", INFO_COLOR);
            ui->appendOutput("输入 distro ubuntu 安装 Ubuntu\n", INFO_COLOR);
        } else {
            ui->appendOutput("解压失败\n", ERROR_COLOR);
        }
    }, "安装PRoot+Alpine", "proot-setup");

    // distro - 切换发行版
    registerCommand("distro", [ui, getDistro, setDistro, getRootfs](const QStringList& args) {
        QString baseDir = QDir(ui->currentDir).absoluteFilePath("proot-env");

        if (args.isEmpty() || args[0] == "list") {
            QString cur = getDistro();
            ui->appendOutput("=== 发行版 ===\n", ACCENT_COLOR);

            bool alpineInstalled = QFile::exists(baseDir + "/rootfs/bin");
            bool ubuntuInstalled = QFile::exists(baseDir + "/ubuntu-rootfs/bin");

            ui->appendOutput(QString("  [alpine] %1 %2\n")
                .arg(alpineInstalled ? "已安装" : "未安装")
                .arg(cur == "alpine" ? "← 当前" : ""),
                cur == "alpine" ? PROMPT_COLOR : FG_COLOR);

            ui->appendOutput(QString("  [ubuntu] %1 %2\n")
                .arg(ubuntuInstalled ? "已安装" : "未安装")
                .arg(cur == "ubuntu" ? "← 当前" : ""),
                cur == "ubuntu" ? PROMPT_COLOR : FG_COLOR);

            ui->appendOutput("用法: distro alpine | distro ubuntu\n", INFO_COLOR);
            return;
        }

        QString name = args[0].toLower();
        if (name != "alpine" && name != "ubuntu") {
            ui->appendOutput("未知发行版，可选: alpine / ubuntu\n", ERROR_COLOR);
            return;
        }

        QString rootfsPath = getRootfs(name);
        if (!QFile::exists(rootfsPath + "/bin")) {
            if (name == "ubuntu") {
                // 自动安装 Ubuntu
                ui->appendOutput("Ubuntu 未安装，开始解压...\n", ACCENT_COLOR);
                QString tarPath = baseDir + "/ubuntu.tar";
                QFile tarRes(":/resources/proot/ubuntu.tar");
                if (!tarRes.open(QIODevice::ReadOnly)) {
                    ui->appendOutput("Ubuntu 资源未找到\n", ERROR_COLOR);
                    return;
                }
                QFile tarDest(tarPath);
                if (!tarDest.open(QIODevice::WriteOnly)) {
                    ui->appendOutput("无法写入\n", ERROR_COLOR);
                    tarRes.close();
                    return;
                }
                QByteArray chunk;
                while (!tarRes.atEnd()) {
                    chunk = tarRes.read(1024 * 1024);
                    tarDest.write(chunk);
                }
                tarRes.close();
                tarDest.close();

                QDir().mkpath(rootfsPath);
                ui->appendOutput("解压 Ubuntu（约30秒）...\n", ACCENT_COLOR);
                QProcess tarProc;
                tarProc.setWorkingDirectory(baseDir);
                QString busyboxPath = QDir(ui->currentDir).absoluteFilePath("busybox");
                if (QFile::exists(busyboxPath))
                    tarProc.start(busyboxPath, QStringList() << "tar" << "-xzf" << tarPath << "-C" << rootfsPath);
                else
                    tarProc.start("tar", QStringList() << "-xzf" << tarPath << "-C" << rootfsPath);
                tarProc.waitForFinished(120000);
                QFile::remove(tarPath);

                if (!QFile::exists(rootfsPath + "/bin")) {
                    ui->appendOutput("Ubuntu 解压失败\n", ERROR_COLOR);
                    return;
                }
                ui->appendOutput("Ubuntu 安装完成\n", PROMPT_COLOR);
            } else {
                ui->appendOutput("该发行版未安装\n", ERROR_COLOR);
                return;
            }
        }

        setDistro(name);
        ui->appendOutput(QString("已切换到 %1\n").arg(name), PROMPT_COLOR);
    }, "切换发行版", "distro [list|alpine|ubuntu]");

    // proot-shell


    registerCommand("proot-shell", [ui, getDistro, getRootfs](const QStringList& args) {
        if (ui->currentProcess && ui->currentProcess->state() != QProcess::NotRunning) {
            ui->appendOutput("有命令正在运行，请等待或输入 stop\n", ERROR_COLOR);
            return;
        }

        QString prootPath = QDir(ui->currentDir).absoluteFilePath("proot");
        QString distro = getDistro();
        QString rootfsPath = getRootfs(distro);
        QString tmpDir = QDir(ui->currentDir).absoluteFilePath("tmp");

        if (!QFile::exists(prootPath) || !QFile::exists(rootfsPath + "/bin")) {
            ui->appendOutput("PRoot 环境未安装\n", ERROR_COLOR);
            return;
        }

        QDir().mkpath(tmpDir);
        QString cmd = args.isEmpty() ? "sh" : args.join(' ');

        QString shell = (distro == "ubuntu") ? "/bin/bash" : "/bin/sh";
        QString fullCmd = QString("PROOT_TMP_DIR=%1 %2 -r %3 -b /dev -b /proc -b /sys -w /root %4 -c \"%5\"")
            .arg(tmpDir, prootPath, rootfsPath, shell, cmd);

        ui->appendOutput(QString("[%1] %2\n").arg(distro, cmd), ACCENT_COLOR);
        CommandHelper::runExternal("sh", QStringList() << "-c" << fullCmd,
    
                                   ui->currentDir, ui, 0, true);
    }, "进入PRoot", "proot-shell [命令]");
    // proot-mirror
    registerCommand("proot-mirror", [ui, getDistro, getRootfs](const QStringList&) {
        QString prootPath = QDir(ui->currentDir).absoluteFilePath("proot");
        QString distro = getDistro();
        QString rootfsPath = getRootfs(distro);
        QString tmpDir = QDir(ui->currentDir).absoluteFilePath("tmp");
        QDir().mkpath(tmpDir);

        QString fullCmd;
        if (distro == "ubuntu") {
            ui->appendOutput("设置 Ubuntu 阿里云镜像...\n", ACCENT_COLOR);
            fullCmd = QString("PROOT_TMP_DIR=%1 %2 -r %3 -b /dev -b /proc -b /sys -w / /bin/bash -c \"echo 'deb http://mirrors.aliyun.com/ubuntu-ports/ jammy main restricted universe multiverse' > /etc/apt/sources.list; echo 'deb http://mirrors.aliyun.com/ubuntu-ports/ jammy-updates main restricted universe multiverse' >> /etc/apt/sources.list; echo nameserver 119.29.29.29 > /etc/resolv.conf\"")
                .arg(tmpDir, prootPath, rootfsPath);
        } else {
            ui->appendOutput("设置 Alpine 阿里云镜像...\n", ACCENT_COLOR);
            fullCmd = QString("PROOT_TMP_DIR=%1 %2 -r %3 -b /dev -b /proc -b /sys -w / /bin/sh -c \"echo nameserver 119.29.29.29 > /etc/resolv.conf; echo https://mirrors.aliyun.com/alpine/v3.19/main > /etc/apk/repositories; echo https://mirrors.aliyun.com/alpine/v3.19/community >> /etc/apk/repositories\"")
                .arg(tmpDir, prootPath, rootfsPath);
        }

        CommandHelper::runExternal("sh", QStringList() << "-c" << fullCmd,
                                   ui->currentDir, ui, 0, true);
        ui->appendOutput("完成\n", PROMPT_COLOR);
    }, "切换国内镜像", "proot-mirror");

    // proot-install
    registerCommand("proot-install", [ui, getDistro](const QStringList& args) {
        if (args.isEmpty()) {
            ui->appendOutput("用法: proot-install <包名>\n", ERROR_COLOR);
            return;
        }
        QString distro = getDistro();
        QString packages = args.join(' ');

        if (distro == "ubuntu") {
            ui->appendOutput(QString("Ubuntu 安装: %1\n").arg(packages), ACCENT_COLOR);
            ui->executeInternalCommand(QString("proot-shell apt update && apt install -y %1").arg(packages));
        } else {
            ui->appendOutput(QString("Alpine 安装: %1\n").arg(packages), ACCENT_COLOR);
            ui->executeInternalCommand(QString("proot-shell /sbin/apk add --no-cache %1").arg(packages));
        }
    }, "PRoot安装软件", "proot-install <包名>");

    // proot-status
    registerCommand("proot-status", [ui, getDistro](const QStringList&) {
        QString baseDir = QDir(ui->currentDir).absoluteFilePath("proot-env");
        QString prootPath = QDir(ui->currentDir).absoluteFilePath("proot");
        QString distro = getDistro();

        ui->appendOutput("=== PRoot 状态 ===\n", ACCENT_COLOR);
        ui->appendOutput(QString("proot: %1\n").arg(QFile::exists(prootPath) ? "已安装" : "未安装"));
        ui->appendOutput(QString("当前发行版: %1\n").arg(distro), PROMPT_COLOR);
        ui->appendOutput(QString("Alpine: %1\n").arg(QFile::exists(baseDir + "/rootfs/bin") ? "已安装" : "未安装"));
        ui->appendOutput(QString("Ubuntu: %1\n").arg(QFile::exists(baseDir + "/ubuntu-rootfs/bin") ? "已安装" : "未安装"));
    }, "PRoot状态", "proot-status");

    // proot-remove
    registerCommand("proot-remove", [ui](const QStringList&) {
        QDir(ui->currentDir + "/proot-env").removeRecursively();
        QFile::remove(ui->currentDir + "/proot");
        ui->appendOutput("已删除\n", INFO_COLOR);
    }, "删除PRoot环境", "proot-remove");
    // pre-position - 查看所有前置命令
    registerCommand("pre-position", [ui](const QStringList&) {
        QString output = R"(
        QString output = R"(
═══════════════════════════════════════
         Termail 前置命令清单
═══════════════════════════════════════

【PRoot 环境】
  get-busybox          安装 busybox（必须）
  proot-setup          安装 PRoot + Alpine（必须）
  proot-mirror         切换国内镜像（必须）
  set-dns <IP>         设置DNS（推荐119.29.29.29）
  proot-update         更新包索引
  distro ubuntu        安装并切换 Ubuntu
  distro alpine        切换回 Alpine
  distro list          查看发行版
  proot-status         查看 PRoot 状态

【软件安装】
  proot-install <包名>  安装软件
  Ubuntu 必装: coreutils bash apt util-linux

【多会话】
  snew [名称]          新建会话
  slist                列出会话
  ssw <编号>           切换会话
  skill <编号>         删除会话

【网络工具】
  serve [端口]         启动 HTTP 服务器
  stop-serve           关闭服务器
  netscan              扫描局域网
  mget <URL>           下载文件

【SSH】
  ssh-host add <名> <用户@主机>  添加主机
  ssh-connect <名>               快速连接
  ssh-keygen                     生成密钥
  ssh-pub                        查看公钥
  ssh-copy <用户@主机>           复制公钥
  ssh-tunnel <映射> <主机>       端口转发

【日志】
  log                  系统日志
  log-termail          程序日志
  exit-log             关闭日志

【设置】
  setfont <8-32>       设置字号
  alias 名=命令         设置别名
  addcmd <名> <命令>    添加自定义命令

【通知栏】
  notify-on            启动通知栏
  notify-off           关闭通知栏
  wakelock             切换唤醒锁

═══════════════════════════════════════
  完整安装顺序:
  1. get-busybox
  2. proot-setup
  3. proot-mirror
  4. set-dns 119.29.29.29
  5. proot-update
  6. distro ubuntu (可选)
  7. proot-install coreutils (Ubuntu必装)
═══════════════════════════════════════
)";
        ui->appendOutput(output, ACCENT_COLOR);
    }, "查看前置命令", "pre-position");
}
