#include "AdBlocker.h"
#include <QUrl>

AdBlocker::AdBlocker(QObject *parent)
    : QWebEngineUrlRequestInterceptor(parent)
{
}

void AdBlocker::interceptRequest(QWebEngineUrlRequestInfo &info)
{
    const QString host = info.requestUrl().host().toLower();
    static const QStringList blocked = {
        "doubleclick.net", "googlesyndication.com", "googleadservices.com",
        "adservice.google.com", "ads.yahoo.com", "adnxs.com",
        "adform.net", "taboola.com", "outbrain.com", "popads.net",
        "exoclick.com", "scorecardresearch.com", "zedo.com"
    };

    for (const QString &domain : blocked) {
        if (host == domain || host.endsWith("." + domain)) {
            info.block(true);
            return;
        }
    }
}
