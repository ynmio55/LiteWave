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

    const QUrl request = info.requestUrl();
    const QString host = request.host().toLower();
    const QString target = (request.path() + "?" + request.query()).toLower();

    static const QStringList blockedDomains = {
        "doubleclick.net", "googlesyndication.com", "googleadservices.com",
        "adservice.google.com", "ads.yahoo.com", "adnxs.com", "adform.net",
        "taboola.com", "outbrain.com", "popads.net", "exoclick.com",
        "scorecardresearch.com", "zedo.com", "moatads.com", "criteo.com",
        "rubiconproject.com", "pubmatic.com", "openx.net", "casalemedia.com",
        "contextweb.com", "advertising.com", "turn.com", "33across.com",
        "sharethrough.com", "smartadserver.com", "lijit.com", "smaato.net",
        "inmobi.com", "appier.net", "media.net", "mgid.com", "revcontent.com",
        "adroll.com", "bidswitch.net", "adsrvr.org", "rlcdn.com",
        "demdex.net", "quantserve.com", "krxd.net", "bluekai.com",
        "hotjar.com", "mouseflow.com", "fullstory.com", "mixpanel.com",
        "app-measurement.com", "segment.io", "amplitude.com", "umeng.com",
        "analytics.yahoo.com"
    };

    for (const QString &domain : blockedDomains) {
        if (host == domain || host.endsWith("." + domain)) {
            info.block(true);
            return;
        }
    }

    // Common ad/tracker URL patterns, including several YouTube ad endpoints.
    static const QStringList blockedPathParts = {
        "/pagead/", "/pagead2.", "/adsystem/", "/adservice/",
        "/advertising/", "/advertisement/", "/ads/", "/adserver/",
        "/prebid/", "/bidrequest", "/tracking/", "/tracker/",
        "/telemetry/", "/doubleclick/", "/api/stats/ads",
        "googlesyndication", "googleadservices", "ad_click"
    };

    for (const QString &part : blockedPathParts) {
        if (target.contains(part)) {
            info.block(true);
            return;
        }
    }
}
