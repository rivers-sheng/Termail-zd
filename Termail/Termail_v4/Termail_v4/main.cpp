#include <QApplication>
#include <QMessageBox>
#include <QDir>
#include <QStandardPaths>
#include <QProcess>
#include <QDebug>

#ifdef Q_OS_WIN
#include <windows.h>
#include <shellapi.h>
#endif

#ifdef Q_OS_UNIX
#include <unistd.h>
#include <sys/types.h>
#endif

#include "config.h"
#include "ui/terminal_ui.h"

// ============ 管理员检测 ============
bool isAdmin() {
#ifdef Q_OS_ANDROID
    // Android 上默认以管理员模式运行（无 root 区分）
    return true;
#elif defined(Q_OS_WIN)
    return IsUserAnAdmin();
#elif defined(Q_OS_UNIX)
    return (geteuid() == 0);
#else
    return false;
#endif
}

// ============ 以管理员权限重启 ============
bool runAsAdmin(int argc, char* argv[]) {
#ifdef Q_OS_WIN
    QString args;
    for (int i = 1; i < argc; ++i) {
        args += QString("\"%1\" ").arg(argv[i]);
    }
    SHELLEXECUTEINFOW sei = { sizeof(sei) };
    sei.lpVerb = L"runas";
    sei.lpFile = L"cmd.exe";
    sei.lpParameters = (L"/c \"" + QDir::toNativeSeparators(QApplication::applicationFilePath()) + " " + args + "\"").toStdWString().c_str();
    sei.nShow = SW_NORMAL;
    if (ShellExecuteExW(&sei)) {
        return true;
    }
#elif defined(Q_OS_LINUX)
    QStringList sudoArgs;
    sudoArgs << QApplication::applicationFilePath();
    for (int i = 1; i < argc; ++i) {
        sudoArgs << argv[i];
    }
    if (QProcess::execute("pkexec", sudoArgs) == 0 ||
        QProcess::execute("sudo", QStringList() << "-E" << sudoArgs) == 0) {
        return true;
    }
#elif defined(Q_OS_MACOS)
    QStringList osaArgs;
    osaArgs << "-e" << QString("do shell script \"%1\" with administrator privileges")
        .arg(QApplication::applicationFilePath());
    QProcess::execute("osascript", osaArgs);
    return true;
#endif
    return false;
}

// ============ 创建配置目录 ============
void ensureConfigDir() {
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/termail";
    QDir dir(configDir);
    if (!dir.exists()) dir.mkpath(".");
    QString defaultConfig = configDir + "/custom_commands.json";
    if (!QFile::exists(defaultConfig)) {
        QFile::copy(":/custom_commands.json", defaultConfig);
    }
}

// ============ 主函数 ============
int main(int argc, char* argv[]) {
    // Qt 高DPI支持
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

    QApplication app(argc, argv);
    app.setApplicationName(APP_NAME);
    app.setApplicationVersion(APP_VERSION);
    app.setOrganizationName("Termail");

    // 确保配置目录存在
    ensureConfigDir();

    // 管理员权限检测
    bool adminMode = isAdmin();
    QString title = APP_NAME;

    if (!adminMode) {
        QMessageBox msgBox;
        msgBox.setWindowTitle("权限请求");
        msgBox.setText("部分网络命令（如 ping、netstat 等）需要管理员权限才能完整运行。\n\n"
                       "是否以管理员权限重新启动？\n"
                       "(选择否将以受限模式运行)");
        msgBox.setIcon(QMessageBox::Question);
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::Yes);

        if (msgBox.exec() == QMessageBox::Yes) {
            if (runAsAdmin(argc, argv)) {
                return 0; // 重启成功，退出当前进程
            }
        }
        title += " [受限模式]";
    } else {
        title += " [管理员]";
    }

    // 创建并显示终端
    TerminalUI terminal(adminMode);
    terminal.setWindowTitle(title);

#ifdef Q_OS_ANDROID
    terminal.showMaximized();
#else
    terminal.resize(900, 600);
    terminal.show();
#endif

    return app.exec();
}