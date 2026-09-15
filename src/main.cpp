#include "MainWindow.h"
#include <QApplication>
#include <QIcon>
#include <QSettings>

int main(int argc, char *argv[])
{
    QSettings st("LiteWave", "LiteWave");
    const bool secureDns = st.value("secureDnsEnabled", true).toBool();
    const QString provider = st.value("dnsProvider", "Cloudflare").toString();
    const QString customUrl = st.value("customDnsUrl", "").toString();

    QString dohTemplate;
    if (secureDns) {
        if (provider == "Cloudflare") {
            dohTemplate = "https://chrome.cloudflare-dns.com/dns-query";
        } else if (provider == "Google") {
            dohTemplate = "https://dns.google/dns-query{?dns}";
        } else if (provider == "Quad9") {
            dohTemplate = "https://dns.quad9.net/dns-query";
        } else if (provider == "AdGuard") {
            dohTemplate = "https://dns.adguard.com/dns-query";
        } else if (provider == "Custom" && !customUrl.isEmpty()) {
            dohTemplate = customUrl;
        }
    }

    QString flags = "--ignore-gpu-blocklist --enable-gpu-rasterization --enable-zero-copy --enable-async-dns "
                    "--disable-safebrowsing --safebrowsing-disable-auto-update --disable-background-networking "
                    "--disable-component-update --disable-domain-reliability --disable-breakpad "
                    "--disable-client-side-phishing-detection --disable-hang-monitor";

    if (!dohTemplate.isEmpty()) {
        flags += " --enable-features=VaapiVideoDecoder,CanvasOopRasterization,DnsOverHttps,EncryptedClientHello,UseDnsHttpsSvcb --doh-templates=" + dohTemplate;
    } else {
        flags += " --enable-features=VaapiVideoDecoder,CanvasOopRasterization,EncryptedClientHello";
    }

    qputenv("QTWEBENGINE_CHROMIUM_FLAGS", flags.toUtf8());
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);

    QApplication app(argc, argv);
    QApplication::setApplicationName("LiteWave");
    QApplication::setApplicationVersion("0.9.0");
    QGuiApplication::setDesktopFileName("LiteWave");
    app.setWindowIcon(QIcon(":/icons/litewave.svg"));

    MainWindow window;
    window.show();
    return app.exec();
}
