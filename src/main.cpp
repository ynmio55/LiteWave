#include "MainWindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    qputenv("QTWEBENGINE_CHROMIUM_FLAGS", "--ignore-gpu-blocklist --enable-gpu-rasterization --enable-zero-copy --enable-async-dns --enable-features=VaapiVideoDecoder,CanvasOopRasterization,DnsOverHttps --doh-templates=https://chrome.cloudflare-dns.com/dns-query --disable-safebrowsing --safebrowsing-disable-auto-update");
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);

    QApplication app(argc, argv);
    QApplication::setApplicationName("LiteWave");
    QApplication::setApplicationVersion("0.9.0");
    QGuiApplication::setDesktopFileName("LiteWave");

    MainWindow window;
    window.show();
    return app.exec();
}
