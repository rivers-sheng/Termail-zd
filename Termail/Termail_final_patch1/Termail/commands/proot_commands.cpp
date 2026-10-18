#include "commands.h"
#include "terminal/terminal.h"
#include "utils/utils.h"

#include <QFile>
#include <QDir>
#include <QProcess>
#include <QFileInfo>

void CommandRegistry::registerProotCommands(TerminalWidget* ui) {

    // proot-setup - 安装 PRoot 环境
    registerCommand("proot-setup", [ui](const QStringList&) {
        QString baseDir = QDir(ui->currentDir).absoluteFilePath("proot-env");
        QString prootPath = QDir(ui->currentDir).absoluteFilePath("proot");
        QString rootfsPath = baseDir + "/rootfs";
        QString tmpDir = QDir(ui->currentDir).absoluteFilePath("tmp");

        QDir().mkpath(tmpDir);

        if (QFile::exists(rootfsPath + "/bin")) {
            ui->appendOutput("PRoot 已安装\n", ERROR_COLOR);
            ui->appendOutput("用 proot-remove 删除后重试\n", INFO_COLOR);
            return;
        }

        ui->appendOutput("正在安装 PRoot 环境...\n", ACCENT_COLOR);

        // 1. 检查 busybox 是否可用
        QString busyboxPath = QDir(ui->currentDir).absoluteFilePath("busybox");
        if (!QFile::exists(busyboxPath)) {
            ui->appendOutput("未找到 busybox，请先运行 get-busybox\n", ERROR_COLOR);
            return;
        }

        // 2. 复制 proot
        QFile prootRes(":/resources/proot/proot");
        if (!prootRes.open(QIODevice::ReadOnly)) {
            ui->appendOutput("proot 资源未找到\n", ERROR_COLOR);
            return;
        }
        QByteArray prootData = prootRes.readAll();
        prootRes.close();

        if (prootData.isEmpty()) {
            ui->appendOutput("proot 资源为空\n", ERROR_COLOR);
            return;
        }

        QFile prootDest(prootPath);
        if (!prootDest.open(QIODevice::WriteOnly)) {
            ui->appendOutput(QString("无法写入 proot: %1\n").arg(prootPath), ERROR_COLOR);
            return;
        }
        prootDest.write(prootData);
        prootDest.close();

        if (!QFile::setPermissions(prootPath,
            QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner |
            QFileDevice::ReadGroup | QFileDevice::ExeGroup |
            QFileDevice::ReadOther | QFileDevice::ExeOther)) {
            ui->appendOutput("proot 权限设置失败\n", ERROR_COLOR);
            QFile::remove(prootPath);
            return;
        }
        ui->appendOutput(QString("proot 已复制 (%1 KB)\n")
            .arg(prootData.size() / 1024), INFO_COLOR);

        // 3. 复制 Alpine rootfs
        QDir().mkpath(baseDir);
        QString tarPath = baseDir + "/alpine.tar";

        QFile tarRes(":/resources/proot/alpine.tar");
        if (!tarRes.open(QIODevice::ReadOnly)) {
            ui->appendOutput("Alpine rootfs 资源未找到\n", ERROR_COLOR);
            QFile::remove(prootPath);
            return;
        }
        QByteArray tarData = tarRes.readAll();
        tarRes.close();

        if (tarData.isEmpty()) {
            ui->appendOutput("Alpine rootfs 资源为空\n", ERROR_COLOR);
            QFile::remove(prootPath);
            return;
        }

        QFile tarDest(tarPath);
        if (!tarDest.open(QIODevice::WriteOnly)) {
            ui->appendOutput("无法写入压缩包\n", ERROR_COLOR);
            QFile::remove(prootPath);
            return;
        }
        tarDest.write(tarData);
        tarDest.close();
        ui->appendOutput(QString("rootfs 已复制 (%1 KB)\n")
            .arg(tarData.size() / 1024), INFO_COLOR);

        // 4. 解压
        QDir().mkpath(rootfsPath);
        ui->appendOutput("解压中...\n", ACCENT_COLOR);

        QProcess tarProc;
        tarProc.setWorkingDirectory(baseDir);
        tarProc.start(busyboxPath, QStringList() << "tar" << "-xzf" << tarPath << "-C" << rootfsPath);

        if (!tarProc.waitForStarted(5000)) {
            ui->appendOutput("无法启动解压进程\n", ERROR_COLOR);
            QFile::remove(tarPath);
            QFile::remove(prootPath);
            return;
        }

        if (!tarProc.waitForFinished(120000)) {
            ui->appendOutput("解压超时\n", ERROR_COLOR);
            tarProc.kill();
            tarProc.waitForFinished(2000);
            QFile::remove(tarPath);
            QDir(rootfsPath).removeRecursively();
            QFile::remove(prootPath);
            return;
        }

        QFile::remove(tarPath);

        // 5. 验证解压结果
        if (QFile::exists(rootfsPath + "/bin") && QFile::exists(rootfsPath + "/bin/busybox")) {
            ui->appendOutput("PRoot 环境安装完成！\n", PROMPT_COLOR);
            ui->appendOutput(QString("位置: %1\n").arg(rootfsPath));
            ui->appendOutput("输入 proot-mirror 切换国内镜像\n", INFO_COLOR);
            ui->appendOutput("输入 proot-install gcc 安装编译器\n", INFO_COLOR);
        } else {
            ui->appendOutput("解压失败，已回滚\n", ERROR_COLOR);
            QDir(rootfsPath).removeRecursively();
            QFile::remove(prootPath);
        }
    }, "安装PRoot环境", "proot-setup");

    // proot-shell - 进入 PRoot 执行命令
    registerCommand("proot-shell", [ui](const QStringList& args) {
        // 防止多实例冲突
        if (ui->currentProcess && ui->currentProcess->state() != QProcess::NotRunning) {
            ui->appendOutput("有命令正在运行，请等待或输入 stop\n", ERROR_COLOR);
            return;
        }

        QString prootPath = QDir(ui->currentDir).absoluteFilePath("proot");
        QString rootfsPath = QDir(ui->currentDir).absoluteFilePath("proot-env/rootfs");
        QString tmpDir = QDir(ui->currentDir).absoluteFilePath("tmp");

        if (!QFile::exists(prootPath)) {
            ui->appendOutput("proot 未找到，请运行 proot-setup\n", ERROR_COLOR);
            return;
        }
        if (!QFile::exists(rootfsPath + "/bin")) {
            ui->appendOutput("rootfs 未找到，请运行 proot-setup\n", ERROR_COLOR);
            return;
        }

        QDir().mkpath(tmpDir);

        QString cmd = args.isEmpty() ? "sh" : args.join(' ');
        QString fullCmd = QString("PROOT_TMP_DIR=%1 %2 -r %3 -b /dev -b /proc -b /sys -w / /bin/busybox sh -c \"%4\"")
            .arg(tmpDir, prootPath, rootfsPath, cmd);

        ui->appendOutput(QString("proot: %1\n").arg(cmd), ACCENT_COLOR);
        CommandHelper::runExternal("sh", QStringList() << "-c" << fullCmd,
                                   ui->currentDir, ui, 0, true);
    }, "进入PRoot", "proot-shell [命令]");

    // proot-mirror - 切换国内镜像
    registerCommand("proot-mirror", [ui](const QStringList&) {
        QString prootPath = QDir(ui->currentDir).absoluteFilePath("proot");
        QString rootfsPath = QDir(ui->currentDir).absoluteFilePath("proot-env/rootfs");
        QString tmpDir = QDir(ui->currentDir).absoluteFilePath("tmp");
        QDir().mkpath(tmpDir);

        ui->appendOutput("设置DNS + 阿里云镜像...\n", ACCENT_COLOR);

        QString fullCmd = QString("PROOT_TMP_DIR=%1 %2 -r %3 -b /dev -b /proc -b /sys -w / /bin/busybox sh -c \"echo nameserver 119.29.29.29 > /etc/resolv.conf; echo https://mirrors.aliyun.com/alpine/v3.19/main > /etc/apk/repositories; echo https://mirrors.aliyun.com/alpine/v3.19/community >> /etc/apk/repositories\"")
            .arg(tmpDir, prootPath, rootfsPath);

        CommandHelper::runExternal("sh", QStringList() << "-c" << fullCmd,
                                   ui->currentDir, ui, 0, true);
        ui->appendOutput("完成\n", PROMPT_COLOR);
        ui->appendOutput("输入 proot-update 更新索引\n", INFO_COLOR);
    }, "切换国内镜像", "proot-mirror");

    // proot-update - 更新索引
    registerCommand("proot-update", [ui](const QStringList&) {
        ui->appendOutput("更新 Alpine 包索引...\n", ACCENT_COLOR);
        ui->executeInternalCommand("proot-shell /sbin/apk update");
    }, "更新包索引", "proot-update");

    // proot-install - 安装软件
    registerCommand("proot-install", [ui](const QStringList& args) {
        if (args.isEmpty()) {
            ui->appendOutput("用法: proot-install <包名>\n", ERROR_COLOR);
            ui->appendOutput("如: proot-install gcc\n", INFO_COLOR);
            return;
        }
        QString packages = args.join(' ');
        ui->appendOutput(QString("安装中: %1\n").arg(packages), ACCENT_COLOR);
        ui->executeInternalCommand(QString("proot-shell /sbin/apk add --no-cache %1").arg(packages));
    }, "PRoot安装软件", "proot-install <包名>");

    // proot-status - 状态
    registerCommand("proot-status", [ui](const QStringList&) {
        QString baseDir = QDir(ui->currentDir).absoluteFilePath("proot-env");
        QString rootfsPath = baseDir + "/rootfs";
        QString prootPath = QDir(ui->currentDir).absoluteFilePath("proot");

        ui->appendOutput("=== PRoot 状态 ===\n", ACCENT_COLOR);
        ui->appendOutput(QString("proot: %1\n")
            .arg(QFile::exists(prootPath) ? "已安装" : "未安装"),
            QFile::exists(prootPath) ? PROMPT_COLOR : ERROR_COLOR);
        ui->appendOutput(QString("Alpine: %1\n")
            .arg(QFile::exists(rootfsPath + "/bin") ? "已安装" : "未安装"),
            QFile::exists(rootfsPath + "/bin") ? PROMPT_COLOR : ERROR_COLOR);
    }, "PRoot状态", "proot-status");

    // proot-remove - 删除环境
    registerCommand("proot-remove", [ui](const QStringList&) {
        QDir(ui->currentDir + "/proot-env").removeRecursively();
        QFile::remove(ui->currentDir + "/proot");
        ui->appendOutput("已删除\n", INFO_COLOR);
    }, "删除PRoot环境", "proot-remove");

    // distro - 发行版信息
    registerCommand("distro", [ui](const QStringList& args) {
        QString baseDir = QDir(ui->currentDir).absoluteFilePath("proot-env");
        bool installed = QFile::exists(baseDir + "/rootfs/bin");

        if (args.isEmpty() || args[0] == "list") {
            ui->appendOutput("=== 发行版 ===\n", ACCENT_COLOR);
            ui->appendOutput(QString("  Alpine %1\n")
                .arg(installed ? "已安装" : "未安装"),
                installed ? PROMPT_COLOR : FG_COLOR);
            return;
        }

        QString name = args[0].toLower();
        if (name != "alpine") {
            ui->appendOutput("仅支持 Alpine\n", ERROR_COLOR);
            return;
        }

        if (!installed) {
            ui->appendOutput("Alpine 未安装，请运行 proot-setup\n", ERROR_COLOR);
            return;
        }

        ui->appendOutput("当前发行版: Alpine\n", PROMPT_COLOR);
    }, "发行版信息", "distro");

    // set-dns - 设置DNS
    registerCommand("set-dns", [ui](const QStringList& args) {
        QString prootPath = QDir(ui->currentDir).absoluteFilePath("proot");
        QString rootfsPath = QDir(ui->currentDir).absoluteFilePath("proot-env/rootfs");
        QString tmpDir = QDir(ui->currentDir).absoluteFilePath("tmp");

        if (!QFile::exists(prootPath) || !QFile::exists(rootfsPath + "/bin")) {
            ui->appendOutput("请先运行 proot-setup\n", ERROR_COLOR);
            return;
        }

        QDir().mkpath(tmpDir);

        if (args.isEmpty()) {
            ui->appendOutput("=== 当前 DNS ===\n", ACCENT_COLOR);
            QString showCmd = QString("PROOT_TMP_DIR=%1 %2 -r %3 -b /dev -b /proc -b /sys -w / /bin/busybox sh -c \"cat /etc/resolv.conf\"")
                .arg(tmpDir, prootPath, rootfsPath);
            QProcess p;
            p.start("sh", QStringList() << "-c" << showCmd);
            p.waitForFinished(5000);
            QString current = p.readAllStandardOutput();
            if (!current.isEmpty()) ui->appendOutput(current, INFO_COLOR);
            else ui->appendOutput("(无DNS配置)\n", INFO_COLOR);

            ui->appendOutput("\n=== 推荐 DNS ===\n", ACCENT_COLOR);
            ui->appendOutput("  119.29.29.29    腾讯（推荐）\n", PROMPT_COLOR);
            ui->appendOutput("  223.5.5.5       阿里云\n", FG_COLOR);
            ui->appendOutput("  114.114.114.114 114DNS\n", FG_COLOR);
            ui->appendOutput("\n用法: set-dns <IP> [备用IP]\n", INFO_COLOR);
            return;
        }

        QString dns1 = args[0];
        QString dns2 = args.size() > 1 ? args[1] : "";

        QRegularExpression ipRegex("^\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}$");
        if (!ipRegex.match(dns1).hasMatch()) {
            ui->appendOutput("DNS 格式错误，应为 x.x.x.x\n", ERROR_COLOR);
            return;
        }

        QString confContent = QString("nameserver %1").arg(dns1);
        if (!dns2.isEmpty() && ipRegex.match(dns2).hasMatch())
            confContent += QString("\\nnameserver %1").arg(dns2);

        QString setCmd = QString("PROOT_TMP_DIR=%1 %2 -r %3 -b /dev -b /proc -b /sys -w / /bin/busybox sh -c \"printf '%4\\n' > /etc/resolv.conf\"")
            .arg(tmpDir, prootPath, rootfsPath, confContent);

        QProcess p;
        p.start("sh", QStringList() << "-c" << setCmd);
        p.waitForFinished(10000);

        ui->appendOutput(QString("DNS 已设置: %1").arg(dns1), PROMPT_COLOR);
        if (!dns2.isEmpty()) ui->appendOutput(QString(" / %1").arg(dns2), PROMPT_COLOR);
        ui->appendOutput("\n", PROMPT_COLOR);
    }, "设置DNS", "set-dns [IP] [备用IP]");

    // pre-position - 前置命令清单
    registerCommand("pre-position", [ui](const QStringList&) {
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
  proot-status         查看 PRoot 状态
  distro               查看发行版

【软件安装】
  proot-install <包名>  安装软件
  常用: gcc python3 git vim nano

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

【日志】
  log                  系统日志
  log-termail          程序日志
  exit-log             关闭日志

【设置】
  setfont <8-32>       设置字号
  alias 名=命令         设置别名
  addcmd <名> <命令>    添加自定义命令

═══════════════════════════════════════
  完整安装顺序:
  1. get-busybox
  2. proot-setup
  3. proot-mirror
  4. set-dns 119.29.29.29
  5. proot-update
  6. proot-install gcc
═══════════════════════════════════════
)";
        ui->appendOutput(output, ACCENT_COLOR);
    }, "前置命令", "pre-position");
}