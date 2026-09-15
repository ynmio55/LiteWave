#include "MainWindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    qputenv("QTWEBENGINE_CHROMIUM_FLAGS",
        "--ignore-gpu-blocklist --enable-gpu-rasterization --enable-zero-copy --enable-async-dns "
        "--disable-safebrowsing --safebrowsing-disable-auto-update --disable-background-networking "
        "--disable-component-update --disable-domain-reliability --disable-breakpad "
        "--disable-client-side-phishing-detection --disable-hang-monitor "
        "--enable-features=VaapiVideoDecoder,CanvasOopRasterization,DnsOverHttps "
        "--doh-templates=https://chrome.cloudflare-dns.com/dns-query");
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);

    QApplication app(argc, argv);
    QApplication::setApplicationName("LiteWave");
    QApplication::setApplicationVersion("0.9.0");
    QGuiApplication::setDesktopFileName("LiteWave");

    MainWindow window;
    window.show();
    return app.exec();
}
