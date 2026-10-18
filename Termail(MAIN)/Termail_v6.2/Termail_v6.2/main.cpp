#include <QApplication>
#include "terminal/terminal.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("Termail_v6.2");
    app.setApplicationVersion("6.2.0");

    TerminalWidget terminal;

#ifdef Q_OS_ANDROID
    terminal.showMaximized();
#else
    terminal.resize(900, 600);
    terminal.show();
#endif

    return app.exec();
}