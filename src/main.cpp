#include "MainWindow.h"

#include <QApplication>
#include <QCoreApplication>
#include <QIcon>
#include <QSettings>
#include <QStringList>
#include <QUrl>
#include <QtGlobal>

#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
#include <QWebEngineGlobalSettings>
#endif

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

void saveSecureDnsStatus(QSettings &settings, const QString &status, const QString &detail)
{
    settings.setValue("secureDnsRuntimeStatus", status);
    settings.setValue("secureDnsRuntimeDetail", detail);
    settings.sync();
}

void configureSecureDns(QSettings &settings)
{
    const QString dohTemplate = dohTemplateFor(settings);

#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    QWebEngineGlobalSettings::DnsMode dnsMode;

    if (dohTemplate.isEmpty()) {
        dnsMode.secureMode = QWebEngineGlobalSettings::SecureDnsMode::SystemOnly;
        const bool applied = QWebEngineGlobalSettings::setDnsMode(dnsMode);
        saveSecureDnsStatus(
            settings, applied ? QStringLiteral("system") : QStringLiteral("error"),
            applied ? QStringLiteral("ใช้ DNS ของระบบ (ไม่ได้เปิด Secure DNS)")
                    : QStringLiteral("LiteWave ไม่สามารถตั้งค่า DNS ของระบบได้"));
        return;
    }

    // SecureOnly deliberately has no fallback to the system resolver. This is
    // the equivalent behaviour people expect after selecting a custom Secure
    // DNS provider in Chromium: DNS requests use the selected DoH resolver or
    // fail visibly, rather than silently falling back to a filtered resolver.
    dnsMode.secureMode = QWebEngineGlobalSettings::SecureDnsMode::SecureOnly;
    dnsMode.serverTemplates = QStringList{dohTemplate};

    const bool applied = QWebEngineGlobalSettings::setDnsMode(dnsMode);
    const QString provider = settings.value("dnsProvider", "Custom").toString();
    saveSecureDnsStatus(
        settings, applied ? QStringLiteral("secure-only") : QStringLiteral("error"),
        applied
            ? QStringLiteral("%1 ถูกตั้งค่าเป็น Secure DNS ของ LiteWave แล้ว (Secure-only)")
                  .arg(provider)
            : QStringLiteral("ที่อยู่ DoH ของ %1 ไม่ถูกต้อง จึงยังไม่เปิด Secure DNS")
                  .arg(provider));
#else
    // Qt introduced its real, per-WebEngine DoH resolver API in 6.6. Do not
    // pretend that a Chromium command-line flag provides the same guarantee on
    // older runtimes; show an explicit status instead.
    if (dohTemplate.isEmpty()) {
        saveSecureDnsStatus(settings, QStringLiteral("system"),
                            QStringLiteral("ใช้ DNS ของระบบ (ไม่ได้เปิด Secure DNS)"));
    } else {
        saveSecureDnsStatus(
            settings, QStringLiteral("unsupported"),
            QStringLiteral("Secure DNS ต้องใช้ Qt WebEngine 6.6 ขึ้นไป; "
                           "รุ่นนี้จึงไม่บังคับเปลี่ยน DNS แบบหลอก ๆ"));
    }
#endif
}
} // namespace

int main(int argc, char *argv[])
{
    qputenv("QTWEBENGINE_CHROMIUM_FLAGS",
            "--enable-gpu-rasterization "
            "--enable-zero-copy "
            "--ignore-gpu-blocklist "
            "--enable-features=VaapiVideoDecoder,CanvasOopRasterization "
            "--disable-blink-features=AutomationControlled");
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QApplication app(argc, argv);
    QApplication::setOrganizationName("LiteWave");
    QApplication::setApplicationName("LiteWave");
    QApplication::setApplicationVersion("1.1.0");
    QGuiApplication::setDesktopFileName("LiteWave");
    QIcon appIcon;
    appIcon.addFile(":/icons/litewave.png");
    appIcon.addFile(":/icons/litewave.svg");
    app.setWindowIcon(appIcon);

    QSettings settings("LiteWave", "LiteWave");
    // This runs before MainWindow creates the first QWebEngineProfile, which
    // is the point at which Qt WebEngine starts its resolver.
    configureSecureDns(settings);

    MainWindow window;
    window.show();
    return app.exec();
}
