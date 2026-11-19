#include "MainWindow.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QIcon>
#include <QLocalServer>
#include <QLocalSocket>
#include <QSettings>
#include <QStringList>
#include <QUrl>
#include <QtGlobal>

#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
#include <QWebEngineGlobalSettings>
#endif

namespace {
// URLs and files passed on the command line, e.g. by the desktop entry's %U
// when LiteWave is the default browser. Chromium switches are skipped.
QStringList urlsFromArguments(const QStringList &arguments)
{
    QStringList urls;
    for (int i = 1; i < arguments.size(); ++i) {
        const QString &arg = arguments.at(i);
        if (arg.isEmpty() || arg.startsWith('-'))
            continue;
        const QUrl url = QUrl::fromUserInput(arg, QDir::currentPath(),
                                             QUrl::AssumeLocalFile);
        if (url.isValid())
            urls.append(url.toString(QUrl::FullyEncoded));
    }
    return urls;
}

QString instanceServerName()
{
    return QStringLiteral("LiteWave-") +
           QString::number(qHash(QDir::homePath()), 16);
}

// Hands the URLs to an already running LiteWave. Two processes must not share
// one persistent Qt WebEngine profile, so a second launch only forwards.
bool forwardToRunningInstance(const QStringList &urls)
{
    QLocalSocket socket;
    socket.connectToServer(instanceServerName());
    if (!socket.waitForConnected(500))
        return false;
    socket.write((QStringLiteral("open\n") + urls.join('\n')).toUtf8());
    socket.waitForBytesWritten(1000);
    socket.disconnectFromServer();
    if (socket.state() != QLocalSocket::UnconnectedState)
        socket.waitForDisconnected(1000);
    return true;
}

MainWindow *targetWindow(MainWindow *fallback)
{
    if (auto *active = qobject_cast<MainWindow *>(QApplication::activeWindow());
        active && !active->isPrivateWindow())
        return active;
    for (QWidget *widget : QApplication::topLevelWidgets()) {
        auto *window = qobject_cast<MainWindow *>(widget);
        if (window && window->isVisible() && !window->isPrivateWindow())
            return window;
    }
    return fallback;
}

void openUrls(MainWindow *window, const QStringList &urls)
{
    for (const QString &url : urls)
        window->createView(QUrl(url));
}

void listenForOtherInstances(QLocalServer *server, MainWindow *primary)
{
    if (!server->listen(instanceServerName())) {
        // A crashed instance can leave a stale socket behind.
        QLocalServer::removeServer(instanceServerName());
        server->listen(instanceServerName());
    }
    QObject::connect(server, &QLocalServer::newConnection, server, [server, primary] {
        while (QLocalSocket *socket = server->nextPendingConnection()) {
            auto *buffer = new QByteArray;
            QObject::connect(socket, &QLocalSocket::readyRead, socket,
                             [socket, buffer] { buffer->append(socket->readAll()); });
            QObject::connect(socket, &QLocalSocket::disconnected, socket, [socket, buffer, primary] {
                buffer->append(socket->readAll());
                QStringList lines = QString::fromUtf8(*buffer).split('\n', Qt::SkipEmptyParts);
                delete buffer;
                socket->deleteLater();
                if (lines.isEmpty() || lines.takeFirst() != QLatin1String("open"))
                    return;

                MainWindow *window = targetWindow(primary);
                if (lines.isEmpty() && window->isVisible()) {
                    auto *fresh = new MainWindow;
                    fresh->setAttribute(Qt::WA_DeleteOnClose);
                    window = fresh;
                }
                openUrls(window, lines);
                window->show();
                window->raise();
                window->activateWindow();
            });
        }
    });
}

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
#ifdef Q_OS_LINUX
    // Prefer the desktop portal theme on Linux so file open/save dialogs use
    // the host desktop's modern native picker (GNOME/KDE) instead of Qt's
    // generic fallback. Respect an explicit user/admin override.
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORMTHEME")) {
        qputenv("QT_QPA_PLATFORMTHEME", "xdgdesktopportal");
    }
#endif

    // Keep LiteWave fast without forcing risky GPU/network flags on every machine.
    // Respect a user/admin supplied Chromium flag set so broken GPU drivers can
    // be worked around without recompiling the browser.
    if (qEnvironmentVariableIsEmpty("QTWEBENGINE_CHROMIUM_FLAGS")) {
        qputenv("QTWEBENGINE_CHROMIUM_FLAGS",
                "--enable-gpu-rasterization "
                "--enable-zero-copy "
                "--enable-quic "
                "--renderer-process-limit=8 "
                "--enable-features=ParallelDownloading,BackForwardCache "
                "--disable-blink-features=AutomationControlled");
    }
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QApplication app(argc, argv);
    QApplication::setOrganizationName("LiteWave");
    QApplication::setApplicationName("LiteWave");
    QApplication::setApplicationVersion(LITEWAVE_APP_VERSION);
    QGuiApplication::setDesktopFileName("LiteWave");
    QIcon appIcon;
    appIcon.addFile(":/icons/litewave.png");
    appIcon.addFile(":/icons/litewave.svg");
    app.setWindowIcon(appIcon);

    const QStringList startupUrls = urlsFromArguments(app.arguments());
    if (forwardToRunningInstance(startupUrls))
        return 0;

    QSettings settings("LiteWave", "LiteWave");
    // This runs before MainWindow creates the first QWebEngineProfile, which
    // is the point at which Qt WebEngine starts its resolver.
    configureSecureDns(settings);

    MainWindow window;
    QLocalServer instanceServer;
    listenForOtherInstances(&instanceServer, &window);
    openUrls(&window, startupUrls);
    window.show();
    return app.exec();
}
