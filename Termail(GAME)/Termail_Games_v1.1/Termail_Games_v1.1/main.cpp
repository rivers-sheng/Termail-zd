#include <QApplication>
#include "terminal/terminal.h"
#include "config.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("Termail Games");
    app.setApplicationVersion("1.1.0");
    app.setOrganizationName("Termail");

    TerminalWidget terminal;

#ifdef Q_OS_ANDROID
    terminal.showMaximized();
#else
    terminal.resize(900, 600);
    terminal.show();
#endif

    return app.exec();
}