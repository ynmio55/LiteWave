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
        return QStringLiteral("https://chrome.cloudflare-dns.com/dns-query");
    if (provider == "Google")
        return QStringLiteral("https://dns.google/dns-query{?dns}");
    if (provider == "Quad9")
        return QStringLiteral("https://dns.quad9.net/dns-query");
    if (provider == "AdGuard")
        return QStringLiteral("https://dns.adguard.com/dns-query");
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

    QByteArray flags =
        "--process-per-site "
        "--ignore-certificate-errors "
        "--allow-running-insecure-content "
        "--autoplay-policy=no-user-gesture-required "
        "--disable-background-networking "
        "--disable-domain-reliability "
        "--disable-speech-api "
        "--disable-breakpad "
        "--disable-sync "
        "--disable-features=Translate,Autofill,OptimizationHints";

    if (!dohTemplate.isEmpty()) {
        flags += " --doh-templates=" + dohTemplate.toUtf8();
    }
    qputenv("QTWEBENGINE_CHROMIUM_FLAGS", flags);

    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QApplication app(argc, argv);
    QApplication::setOrganizationName("LiteWave");
    QApplication::setApplicationName("LiteWave");
    QApplication::setApplicationVersion("1.0.0");
    QGuiApplication::setDesktopFileName("LiteWave");
    app.setWindowIcon(QIcon(":/icons/litewave.svg"));

    MainWindow window;
    window.show();
    return app.exec();
}
