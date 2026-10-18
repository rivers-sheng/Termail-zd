#include <QApplication>
#include <QDir>
#include <QFile>
#include <QDateTime>
#include <QStandardPaths>
#include <exception>

#include "terminal/terminal.h"
#include "config.h"

#ifdef Q_OS_ANDROID
#include <QJniObject>
#endif

// 崩溃处理
void crashHandler() {
    QString logDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(logDir);
    QString crashFile = logDir + "/crash.log";
    QFile f(crashFile);
    if (f.open(QIODevice::Append | QIODevice::Text)) {
        f.write(QString("[%1] 程序异常退出\n")
            .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")).toUtf8());
        f.close();
    }
}

// 处理从其他应用分享的文本
void handleSharedText(TerminalWidget& terminal) {
#ifdef Q_OS_ANDROID
    QJniObject activity = QJniObject::callStaticObjectMethod(
        "org/qtproject/qt/android/QtNative", "activity",
        "()Landroid/app/Activity;");
    if (!activity.isValid()) return;

    QJniObject intent = activity.callObjectMethod(
        "getIntent", "()Landroid/content/Intent;");
    if (!intent.isValid()) return;

    QJniObject action = intent.callObjectMethod(
        "getAction", "()Ljava/lang/String;");
    if (!action.isValid()) return;

    QString act = action.toString();
    if (act == "android.intent.action.SEND") {
        QJniObject extraText = intent.callObjectMethod(
            "getStringExtra", "(Ljava/lang/String;)Ljava/lang/String;",
            QJniObject::fromString("android.intent.extra.TEXT").object());
        if (extraText.isValid()) {
            QString text = extraText.toString();
            if (!text.isEmpty()) {
                terminal.appendOutput("\n[收到分享内容]\n", ACCENT_COLOR);
                terminal.appendOutput(text + "\n");
            }
        }
    }
#endif
}

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("Termail");
    app.setApplicationVersion("6.4.0");
    app.setOrganizationName("Termail");

    std::set_terminate(crashHandler);

    TerminalWidget terminal;

#ifdef Q_OS_ANDROID
    terminal.showMaximized();

    // 启动通知栏服务
    QJniObject activity = QJniObject::callStaticObjectMethod(
        "org/qtproject/qt/android/QtNative", "activity",
        "()Landroid/app/Activity;");
    if (activity.isValid()) {
        QJniObject::callStaticMethod<void>(
            "com/termail/terminal/TermailService", "start",
            "(Landroid/content/Context;)V", activity.object());
    }
#else
    terminal.resize(900, 600);
    terminal.show();
#endif

    // 处理外部分享
    handleSharedText(terminal);

    return app.exec();
}