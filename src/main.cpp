#include "MainWindow.h"

#include <QApplication>
#include <QIcon>

int main(int argc, char *argv[])
{
    // Do not override Chromium runtime flags: Qt WebEngine's defaults provide
    // the widest compatibility with embedded players, login providers and
    // regular websites.
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QApplication app(argc, argv);
    QApplication::setOrganizationName("LiteWave");
    QApplication::setApplicationName("LiteWave");
    QApplication::setApplicationVersion("1.0.1");
    QGuiApplication::setDesktopFileName("LiteWave");
    app.setWindowIcon(QIcon(":/icons/litewave.svg"));

    MainWindow window;
    window.show();
    return app.exec();
}
