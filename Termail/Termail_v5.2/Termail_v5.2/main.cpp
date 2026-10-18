#include <QApplication>
#include "terminal/terminal.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("Termail终端");
    app.setApplicationVersion("5.2.0");

    TerminalWidget terminal;
    terminal.show();

#ifdef Q_OS_ANDROID
    terminal.showMaximized();
#else
    terminal.resize(900, 600);
#endif

    return app.exec();
}