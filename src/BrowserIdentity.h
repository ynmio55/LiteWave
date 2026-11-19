#pragma once

#include <QByteArray>
#include <QtWebEngineCore/qtwebenginecoreglobal.h>

// LiteWave presents itself as the Chrome release matching the Chromium that
// Qt WebEngine actually ships, so sites see a browser whose advertised
// version agrees with the features it really supports.
namespace BrowserIdentity {

inline QByteArray chromiumFullVersion()
{
    return QByteArray(qWebEngineChromiumVersion());
}

inline QByteArray chromiumMajorVersion()
{
    const QByteArray full = chromiumFullVersion();
    const int dot = full.indexOf('.');
    return dot > 0 ? full.left(dot) : full;
}

inline QByteArray platformName()
{
#if defined(Q_OS_WIN)
    return "Windows";
#elif defined(Q_OS_MAC)
    return "macOS";
#else
    return "Linux";
#endif
}

inline QByteArray userAgent()
{
#if defined(Q_OS_WIN)
    const QByteArray os = "Windows NT 10.0; Win64; x64";
#elif defined(Q_OS_MAC)
    const QByteArray os = "Macintosh; Intel Mac OS X 10_15_7";
#else
    const QByteArray os = "X11; Linux x86_64";
#endif
    return "Mozilla/5.0 (" + os + ") AppleWebKit/537.36 (KHTML, like Gecko) Chrome/" +
           chromiumMajorVersion() + ".0.0.0 Safari/537.36";
}

inline QByteArray secChUa()
{
    const QByteArray major = chromiumMajorVersion();
    return "\"Chromium\";v=\"" + major + "\", \"Google Chrome\";v=\"" + major +
           "\", \"Not?A_Brand\";v=\"99\"";
}

} // namespace BrowserIdentity
