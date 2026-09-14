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
        "juicyads.com", "trafficjunky.com", "syndicatedsearch.goog", "realsrv.com",
        "exosrv.com", "trafficfactory.biz",
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
        "propellerads.com", "a-ads.com",
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
        "/pagead/js/", "/pagead/conversion/", "/pagead/gen_204",
        "/pagead2.googlesyndication", "/adservice.google",
        "/gampad/ads", "/pcs/activeview", "/doubleclick/pagead"
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
    // Standardize User-Agent header to latest stable Windows 10 Chrome
    info.setHttpHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/128.0.0.0 Safari/537.36");
    info.setHttpHeader("Sec-CH-UA", "\"Chromium\";v=\"128\", \"Not;A=Brand\";v=\"24\", \"Google Chrome\";v=\"128\"");
    info.setHttpHeader("Sec-CH-UA-Mobile", "?0");
    info.setHttpHeader("Sec-CH-UA-Platform", "\"Windows\"");
    info.setHttpHeader("Accept-Language", "th-TH,th;q=0.9,en-US;q=0.8,en;q=0.7");

    if (!enabled_) return;

    const QUrl request = info.requestUrl();
    const QString host = request.host().toLower();
    const QString target = (request.path() + "?" + request.query()).toLower();

    // Never block reCAPTCHA, hCaptcha, or Google verification URLs
    if (host.contains("recaptcha") || target.contains("recaptcha") ||
        host.contains("hcaptcha") || target.contains("sorry/index")) {
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
        iframe[src*="exoclick"], iframe[src*="juicyads"], iframe[src*="popads"], iframe[src*="popcash"], iframe[src*="adsterra"],
        iframe[src*="trafficjunky"], iframe[src*="propellerads"],
        .adsbygoogle, .a-ad, [id*="google_ads"], [id*="div-gpt-ad"], [class*="google-auto-placed"],
        ytd-promoted-sparkles-web-renderer, ytd-display-ad-renderer, ytd-statement-banner-renderer,
        ytd-in-feed-ad-layout-renderer, ytd-banner-promo-renderer, .ytd-action-companion-ad-renderer,
        #player-ads, .ytp-ad-overlay-container, .ytp-ad-message-container,
        ytd-ad-slot-renderer, ytd-promoted-video-renderer, .ytp-ad-button, .ytp-ad-text,
        .popunder, [class*="popunder"], [id*="popunder"], [class*="ad-box"], [id*="ad-box"]
        { display: none !important; visibility: hidden !important; width: 0px !important; height: 0px !important; pointer-events: none !important; opacity: 0 !important; }
    )CSS");
}

QString AdBlocker::cosmeticJs()
{
    return QStringLiteral(R"JS(
(() => {
    try {
        Object.defineProperty(navigator, 'webdriver', { get: () => undefined });
    } catch(e) {}

    const host = location.hostname;

    // Fast Ad-Skipper for YouTube
    if (host === 'youtube.com' || host.endsWith('.youtube.com')) {
        setInterval(() => {
            if (document.hidden) return;
            try {
                const video = document.querySelector('video');
                const adPlaying = document.querySelector('.ad-interrupting, .ad-showing, .ytp-ad-player-overlay');
                if (video && adPlaying) {
                    if (!isNaN(video.duration) && video.duration > 0 && isFinite(video.duration)) {
                        video.currentTime = video.duration - 0.1;
                    }
                    video.playbackRate = 16.0;
                    video.muted = true;
                }
                const skipBtns = document.querySelectorAll('.ytp-ad-skip-button, .ytp-ad-skip-button-modern, .ytp-ad-overlay-close-button, .ytp-ad-skip-button-slot, .ytp-ad-skip-button-container');
                skipBtns.forEach(btn => { if (btn && typeof btn.click === 'function') btn.click(); });

                const adNodes = document.querySelectorAll('ytd-promoted-sparkles-web-renderer, ytd-display-ad-renderer, #player-ads, .ytd-in-feed-ad-layout-renderer, .adsbygoogle, [id*="google_ads"]');
                adNodes.forEach(node => {
                    if (node && node.parentNode) { node.parentNode.removeChild(node); }
                });
            } catch(e) {}
        }, 500);
    }
})();
)JS");
}
