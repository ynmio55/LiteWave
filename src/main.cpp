#include "MainWindow.h"

#include <QApplication>
#include <QIcon>
#include <QSettings>

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName("LiteWave");
    QCoreApplication::setApplicationName("LiteWave");
    QCoreApplication::setApplicationVersion("1.0.1");

    QSettings settings("LiteWave", "LiteWave");
    bool secureDnsEnabled = settings.value("secureDnsEnabled", false).toBool();
    QString provider = settings.value("dnsProvider", "OS Default").toString();
    QString customUrl = settings.value("customDnsUrl", "").toString();

    if (secureDnsEnabled && provider != "OS Default") {
        QString templateUri;
        if (provider == "Cloudflare") {
            templateUri = "https://chrome.cloudflare-dns.com/dns-query";
        } else if (provider == "Google") {
            templateUri = "https://dns.google/dns-query{?dns}";
        } else if (provider == "Quad9") {
            templateUri = "https://dns.quad9.net/dns-query";
        } else if (provider == "AdGuard") {
            templateUri = "https://dns.adguard-dns.com/dns-query";
        } else if (provider == "Custom" && !customUrl.isEmpty()) {
            templateUri = customUrl;
        }

        if (!templateUri.isEmpty()) {
            QString existingFlags = qgetenv("QTWEBENGINE_CHROMIUM_FLAGS");
            QString dnsFlags = QString("--enable-features=DnsOverHttps --dns-over-https-templates=\"%1\" --dns-over-https-mode=secure").arg(templateUri);
            if (!existingFlags.isEmpty()) {
                dnsFlags = existingFlags + " " + dnsFlags;
            }
            qputenv("QTWEBENGINE_CHROMIUM_FLAGS", dnsFlags.toUtf8());
        }
    }

    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QApplication app(argc, argv);
    QGuiApplication::setDesktopFileName("LiteWave");
    app.setWindowIcon(QIcon(":/icons/litewave.svg"));

    MainWindow window;
    window.show();
    return app.exec();
}
