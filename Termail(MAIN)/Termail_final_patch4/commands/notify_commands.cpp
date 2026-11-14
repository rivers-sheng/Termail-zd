#include "commands.h"
#include "terminal/terminal.h"

#ifdef Q_OS_ANDROID
#include <QJniObject>
#endif

void CommandRegistry::registerNotifyCommands(TerminalWidget* ui) {

    // notify-on - 启动通知栏
    registerCommand("notify-on", [ui](const QStringList&) {
#ifdef Q_OS_ANDROID
        QJniObject activity = QJniObject::callStaticObjectMethod(
            "org/qtproject/qt/android/QtNative", "activity",
            "()Landroid/app/Activity;");
        if (!activity.isValid()) {
            ui->appendOutput("无法获取 Activity\n", ERROR_COLOR);
            return;
        }
        QJniObject::callStaticMethod<void>(
            "com/termail/terminal/TermailService", "start",
            "(Landroid/content/Context;)V", activity.object());
        ui->appendOutput("通知栏已启动\n", PROMPT_COLOR);
#else
        ui->appendOutput("仅 Android 支持\n", ERROR_COLOR);
#endif
    }, "启动通知栏", "notify-on");

    // notify-off - 关闭通知栏
    registerCommand("notify-off", [ui](const QStringList&) {
#ifdef Q_OS_ANDROID
        QJniObject activity = QJniObject::callStaticObjectMethod(
            "org/qtproject/qt/android/QtNative", "activity",
            "()Landroid/app/Activity;");
        if (activity.isValid()) {
            QJniObject::callStaticMethod<void>(
                "com/termail/terminal/TermailService", "stop",
                "(Landroid/content/Context;)V", activity.object());
        }
        ui->appendOutput("通知栏已关闭\n", INFO_COLOR);
#else
        ui->appendOutput("仅 Android 支持\n", ERROR_COLOR);
#endif
    }, "关闭通知栏", "notify-off");

    // wakelock - 切换唤醒锁
    registerCommand("wakelock", [ui](const QStringList& args) {
#ifdef Q_OS_ANDROID
        QJniObject activity = QJniObject::callStaticObjectMethod(
            "org/qtproject/qt/android/QtNative", "activity",
            "()Landroid/app/Activity;");
        if (!activity.isValid()) {
            ui->appendOutput("无法获取 Activity\n", ERROR_COLOR);
            return;
        }

        if (!args.isEmpty() && args[0] == "status") {
            jboolean on = QJniObject::callStaticMethod<jboolean>(
                "com/termail/terminal/TermailService", "isWakeLockOn", "()Z");
            ui->appendOutput(QString("唤醒锁: %1\n").arg(on ? "开" : "关"), PROMPT_COLOR);
            return;
        }

        QJniObject::callStaticMethod<void>(
            "com/termail/terminal/TermailService", "toggleWakeLock",
            "(Landroid/content/Context;)V", activity.object());
        ui->appendOutput("唤醒锁已切换\n", INFO_COLOR);
#else
        ui->appendOutput("仅 Android 支持\n", ERROR_COLOR);
#endif
    }, "切换唤醒锁", "wakelock [status]");

    // notify-status - 通知栏状态
    registerCommand("notify-status", [ui](const QStringList&) {
#ifdef Q_OS_ANDROID
        QJniObject activity = QJniObject::callStaticObjectMethod(
            "org/qtproject/qt/android/QtNative", "activity",
            "()Landroid/app/Activity;");
        if (!activity.isValid()) return;
        jboolean on = QJniObject::callStaticMethod<jboolean>(
            "com/termail/terminal/TermailService", "isWakeLockOn", "()Z");
        ui->appendOutput(QString("唤醒锁: %1\n").arg(on ? "开" : "关"), PROMPT_COLOR);
#else
        ui->appendOutput("仅 Android 支持\n", ERROR_COLOR);
#endif
    }, "通知栏状态", "notify-status");
}