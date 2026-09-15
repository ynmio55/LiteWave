#include "MainWindow.h"

#include <QApplication>
#include <QIcon>
#include <QSettings>
#include <QUrl>

namespace {
QString dohTemplateFor(const QSettings &settings)
{
    if (!settings.value("secureDnsEnabled", false).toBool())
        return {};

    const QString provider = settings.value("dnsProvider", "OS Default").toString();
    if (provider == "Cloudflare")
        return QStringLiteral("https://chrome.cloudflare-dns.com/dns-query{?dns}");
    if (provider == "Google")
        return QStringLiteral("https://dns.google/dns-query{?dns}");
    if (provider == "Quad9")
        return QStringLiteral("https://dns.quad9.net/dns-query{?dns}");
    if (provider == "AdGuard")
        return QStringLiteral("https://dns.adguard-dns.com/dns-query{?dns}");
    if (provider == "Custom") {
        const QUrl custom(settings.value("customDnsUrl").toString().trimmed());
        if (custom.isValid() && custom.scheme() == "https" && !custom.host().isEmpty())
            return custom.toString(QUrl::FullyEncoded);
    }
    return {};
}
} // namespace

int main(int argc, char *argv[])
{
    QSettings settings("LiteWave", "LiteWave");
    const QString dohTemplate = dohTemplateFor(settings);

    // Keep Chromium defaults for all normal browsing. DoH is the single,
    // explicit opt-in override and is applied only before WebEngine starts.
    if (!dohTemplate.isEmpty()) {
        qputenv("QTWEBENGINE_CHROMIUM_FLAGS",
                QByteArray("--enable-features=DnsOverHttps --doh-templates=") +
                    dohTemplate.toUtf8());
    }

    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QApplication app(argc, argv);
    QApplication::setOrganizationName("LiteWave");
    QApplication::setApplicationName("LiteWave");
    QApplication::setApplicationVersion("1.0.2");
    QGuiApplication::setDesktopFileName("LiteWave");
    app.setWindowIcon(QIcon(":/icons/litewave.svg"));

    MainWindow window;
    window.show();
    return app.exec();
}
