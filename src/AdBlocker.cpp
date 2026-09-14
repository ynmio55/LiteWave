#include "AdBlocker.h"
#include <QUrl>
#include <QStringList>

AdBlocker::AdBlocker(QObject *parent)
    : QWebEngineUrlRequestInterceptor(parent)
{
    static const QStringList initialDomains = {
        "doubleclick.net", "googlesyndication.com", "googleadservices.com",
        "adservice.google.com", "pagead2.googlesyndication.com", "ad.doubleclick.net",
        "securepubads.g.doubleclick.net", "stats.g.doubleclick.net", "video-stats.l.google.com",
        "tpc.googlesyndication.com", "ads.yahoo.com", "adnxs.com", "adform.net",
        "taboola.com", "outbrain.com", "popads.net", "popcash.net", "exoclick.com",
        "scorecardresearch.com", "zedo.com", "moatads.com", "criteo.com",
        "rubiconproject.com", "pubmatic.com", "openx.net", "casalemedia.com",
        "contextweb.com", "advertising.com", "turn.com", "33across.com",
        "sharethrough.com", "smartadserver.com", "lijit.com", "smaato.net",
        "inmobi.com", "appier.net", "media.net", "mgid.com", "revcontent.com",
        "adroll.com", "bidswitch.net", "adsrvr.org", "rlcdn.com",
        "demdex.net", "quantserve.com", "krxd.net", "bluekai.com",
        "hotjar.com", "mouseflow.com", "fullstory.com", "mixpanel.com",
        "app-measurement.com", "segment.io", "amplitude.com", "umeng.com",
        "analytics.yahoo.com", "amazon-adsystem.com", "adsterra.com",
        "propellerads.com", "a-ads.com", "trafficjunky.com", "juicyads.com",
        "hilltopads.com", "clickadu.com", "monetag.com", "adstriker.com",
        "adcash.com", "adtrue.com", "exponential.com", "yieldmo.com",
        "spotxchange.com", "teads.tv", "admanmedia.com", "bebi.com",
        "clksite.com", "coinhive.com", "luckyorange.com", "crazyegg.com",
        "mc.yandex.ru", "clarity.ms", "newrelic.com", "bugsnag.com",
        "sentry.io", "doubleverify.com", "integralads.com", "adblade.com",
        "adcolony.com", "adriver.ru", "airpush.com", "applovin.com",
        "chartbeat.com", "clicktale.com", "conversantmedia.com", "eplanning.net",
        "flashtalking.com", "gumgum.com", "indexww.com", "kargo.com",
        "liveramp.com", "mathtag.com", "onesignal.com", "onesignal.io",
        "optimizely.com", "plista.com", "sail-thru.com", "sharethis.com",
        "sizmek.com", "sovrn.com", "tapad.com", "unruly.co", "vungle.com"
    };

    for (const QString &domain : initialDomains) {
        blockedDomainsSet_.insert(domain.toLower());
    }

    blockedPathParts_ = {
        "/pagead/", "/pagead2.", "/adsystem/", "/adservice/",
        "/advertising/", "/advertisement/", "/ads/", "/adserver/",
        "/prebid/", "/bidrequest", "/tracking/", "/tracker/",
        "/telemetry/", "/doubleclick/", "/api/stats/ads",
        "googlesyndication", "googleadservices", "ad_click",
        "/gampad/", "/get_ad", "/ad_status", "ytp-ad-module",
        "/pcs/activeview", "/api/stats/qoe"
    };
}

void AdBlocker::setEnabled(bool enabled)
{
    enabled_ = enabled;
}

bool AdBlocker::isDomainBlocked(const QString &host) const
{
    if (host.isEmpty()) return false;
    if (blockedDomainsSet_.contains(host)) return true;

    int pos = host.indexOf('.');
    while (pos != -1) {
        const QString sub = host.mid(pos + 1);
        if (blockedDomainsSet_.contains(sub)) return true;
        pos = host.indexOf('.', pos + 1);
    }
    return false;
}

bool AdBlocker::isPathBlocked(const QString &target) const
{
    for (const QString &part : blockedPathParts_) {
        if (target.contains(part)) return true;
    }
    return false;
}

void AdBlocker::interceptRequest(QWebEngineUrlRequestInfo &info)
{
    if (!enabled_) return;

    const QUrl request = info.requestUrl();
    const QString host = request.host().toLower();
    const QString target = (request.path() + "?" + request.query()).toLower();

    // Never block reCAPTCHA or Google verification URLs
    if (host.contains("recaptcha") || target.contains("recaptcha") || target.contains("sorry/index")) {
        return;
    }

    if (isDomainBlocked(host) || isPathBlocked(target)) {
        info.block(true);
        const int count = blockedCount_.fetchAndAddAcquire(1) + 1;
        emit countChanged(count);
    }
}

QString AdBlocker::cosmeticCss()
{
    return QStringLiteral(R"CSS(
        iframe[src*="doubleclick"], iframe[src*="googlesyndication"], iframe[src*="adservice"], iframe[src*="adsystem"],
        div[class*="ad-box"], div[class*="ad-container"], div[class*="sponsored-post"], div[class*="ad-banner"],
        .adsbygoogle, .a-ad, .ad_wrapper, .ad-unit, .ad-slot, .ad-card, .ad-placement, .ad_box, .ad_frame,
        [id*="google_ads"], [id*="div-gpt-ad"], [class*="google-auto-placed"],
        ytd-promoted-sparkles-web-renderer, ytd-display-ad-renderer, ytd-statement-banner-renderer,
        ytd-in-feed-ad-layout-renderer, ytd-banner-promo-renderer, .ytd-action-companion-ad-renderer,
        #player-ads, .video-ads, .ytp-ad-module, .ytp-ad-overlay-container, .ytp-ad-message-container,
        ytd-ad-slot-renderer, ytd-promoted-video-renderer, .ytp-ad-button, .ytp-ad-text
        { display: none !important; visibility: hidden !important; width: 0px !important; height: 0px !important; pointer-events: none !important; opacity: 0 !important; }
    )CSS");
}

QString AdBlocker::cosmeticJs()
{
    return QStringLiteral(R"JS(
        (function() {
            function cleanAds() {
                try {
                    var video = document.querySelector('video');
                    var adModule = document.querySelector('.ad-interrupting, .html5-ad-state, .ytp-ad-player-overlay, .ytp-ad-module');
                    if (video && adModule) {
                        if (!isNaN(video.duration) && video.duration > 0) {
                            video.currentTime = video.duration - 0.1;
                        }
                        video.playbackRate = 16.0;
                        video.muted = true;
                    }
                    var skipBtns = document.querySelectorAll('.ytp-ad-skip-button, .ytp-ad-skip-button-modern, .ytp-ad-overlay-close-button, .ytp-ad-skip-button-slot, .ytp-ad-skip-button-container');
                    skipBtns.forEach(function(btn) { if (btn) btn.click(); });

                    var adNodes = document.querySelectorAll('ytd-promoted-sparkles-web-renderer, ytd-display-ad-renderer, #player-ads, .ytd-in-feed-ad-layout-renderer, .adsbygoogle, [id*="google_ads"]');
                    adNodes.forEach(function(node) {
                        if (node && node.parentNode) { node.parentNode.removeChild(node); }
                    });
                } catch(e) {}
            }
            setInterval(cleanAds, 500);
            document.addEventListener('DOMContentLoaded', cleanAds);
        })();
    )JS");
}

