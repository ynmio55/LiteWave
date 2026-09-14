#include "AdBlocker.h"
#include <QUrl>
#include <QStringList>

AdBlocker::AdBlocker(QObject *parent)
    : QWebEngineUrlRequestInterceptor(parent)
{
}

void AdBlocker::setEnabled(bool enabled)
{
    enabled_ = enabled;
}

void AdBlocker::interceptRequest(QWebEngineUrlRequestInfo &info)
{
    if (!enabled_) return;

    const QString host = info.requestUrl().host().toLower();
    static const QStringList blocked = {
        "doubleclick.net", "googlesyndication.com", "googleadservices.com",
        "adservice.google.com", "ads.yahoo.com", "adnxs.com", "adform.net",
        "taboola.com", "outbrain.com", "popads.net", "exoclick.com",
        "scorecardresearch.com", "zedo.com", "moatads.com", "criteo.com",
        "rubiconproject.com", "pubmatic.com", "openx.net", "casalemedia.com",
        "contextweb.com", "advertising.com", "turn.com", "33across.com",
        "sharethrough.com", "smartadserver.com", " lijit.com", "smaato.net",
        "inmobi.com", "appier.net", "media.net", "mgid.com", "revcontent.com",
        "adroll.com", "bidswitch.net", "adsrvr.org", "rlcdn.com",
        "demdex.net", "quantserve.com", "krxd.net", "bluekai.com",
        "hotjar.com", "mouseflow.com", "fullstory.com", "mixpanel.com",
        "app-measurement.com", "segment.io", "amplitude.com", "umeng.com",
        "scorecardresearch.com", "n.target.com", "analytics.yahoo.com"
    };

    for (const QString &domain : blocked) {
        const QString cleanDomain = domain.trimmed();
        if (host == cleanDomain || host.endsWith("." + cleanDomain)) {
            info.block(true);
            return;
        }
    }
}
