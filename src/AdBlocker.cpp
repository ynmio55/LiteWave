#include "AdBlocker.h"
#include "LiteWaveFilterEngine.h"

#include <QSettings>

namespace {

const QSet<QByteArray> &advertisingDomains()
{
    static const QSet<QByteArray> domains = {
        "doubleclick.net", "googlesyndication.com", "googleadservices.com",
        "adservice.google.com", "adnxs.com", "adsrvr.org",
        "amazon-adsystem.com", "criteo.com", "rubiconproject.com",
        "pubmatic.com", "openx.net", "casalemedia.com", "contextweb.com",
        "taboola.com", "outbrain.com", "revcontent.com", "mgid.com",
        "sharethrough.com", "smartadserver.com", "smaato.net", "adform.net",
        "advertising.com", "yieldmo.com", "spotxchange.com", "teads.tv",
        "bidswitch.net", "adroll.com", "demdex.net", "bluekai.com",
        "quantserve.com", "rlcdn.com", "trafficjunky.com", "exoclick.com",
        "juicyads.com", "popads.net", "popcash.net", "propellerads.com",
        "adsterra.com", "a-ads.com", "hilltopads.com", "clickadu.com",
        "monetag.com", "adcash.com", "ad-maven.com", "trafficstars.com",
        "tsyndicate.com", "adskeeper.com", "realsrv.com", "exosrv.com",
        "ero-advertising.com", "adxpansion.com", "clicksor.com"
    };
    return domains;
}

const QSet<QByteArray> &trackerDomains()
{
    // These are opt-in because analytics providers can occasionally be used
    // by site features. Standard mode intentionally leaves them alone.
    static const QSet<QByteArray> domains = {
        "google-analytics.com", "googletagmanager.com",
        "scorecardresearch.com", "hotjar.com", "fullstory.com",
        "mixpanel.com", "segment.io", "amplitude.com", "clarity.ms",
        "mouseflow.com", "crazyegg.com", "mathtag.com", "liveramp.com",
        "moatads.com", "doubleverify.com", "integralads.com"
    };
    return domains;
}

bool isWebUrl(const QUrl &url)
{
    return url.isValid() && (url.scheme() == "http" || url.scheme() == "https");
}

} // namespace

AdBlocker::AdBlocker(QObject *parent, bool persistent)
    : QWebEngineUrlRequestInterceptor(parent), persistent_(persistent)
{
    engine_ = lw_adblock_engine_create();

    if (!persistent_)
        return;

    QSettings settings("LiteWave", "LiteWave");
    enabled_.storeRelease(settings.value("shield/enabled", true).toBool());
    const QString configuredMode =
        settings.value("shield/mode", QStringLiteral("standard")).toString();
    mode_.storeRelease(configuredMode == "aggressive"
                           ? static_cast<int>(Mode::Aggressive)
                           : static_cast<int>(Mode::Standard));

    dntEnabled_.storeRelease(settings.value("dntEnabled", true).toBool() ? 1 : 0);

    for (const QString &site : settings.value("shield/allowedSites").toStringList()) {
        const QByteArray key = normalizedHost(site);
        if (!key.isEmpty())
            allowedSites_.insert(key);
    }
}

bool AdBlocker::isDntEnabled() const
{
    return dntEnabled_.loadAcquire() != 0;
}

void AdBlocker::setDntEnabled(bool enabled)
{
    dntEnabled_.storeRelease(enabled ? 1 : 0);
    if (persistent_) {
        QSettings settings("LiteWave", "LiteWave");
        settings.setValue("dntEnabled", enabled);
    }
}

void AdBlocker::setEnabled(bool enabled)
{
    if (isEnabled() == enabled)
        return;
    enabled_.storeRelease(enabled ? 1 : 0);
    saveSettings();
    emit configurationChanged();
}

bool AdBlocker::isEnabled() const
{
    return enabled_.loadAcquire() != 0;
}

void AdBlocker::setMode(Mode mode)
{
    if (this->mode() == mode)
        return;
    mode_.storeRelease(static_cast<int>(mode));
    saveSettings();
    emit configurationChanged();
}

AdBlocker::Mode AdBlocker::mode() const
{
    return mode_.loadAcquire() == static_cast<int>(Mode::Aggressive)
               ? Mode::Aggressive
               : Mode::Standard;
}

bool AdBlocker::isSiteAllowed(const QUrl &url) const
{
    const QByteArray key = siteKey(url);
    if (key.isEmpty())
        return false;

    QReadLocker guard(&lock_);
    return allowedSites_.contains(key);
}

void AdBlocker::setSiteAllowed(const QUrl &url, bool allowed)
{
    const QByteArray key = siteKey(url);
    if (key.isEmpty())
        return;

    bool changed = false;
    {
        QWriteLocker guard(&lock_);
        if (allowed) {
            changed = !allowedSites_.contains(key);
            allowedSites_.insert(key);
        } else {
            changed = allowedSites_.remove(key);
        }
    }
    if (!changed)
        return;

    saveSettings();
    emit configurationChanged();
}

QStringList AdBlocker::allowedSites() const
{
    QReadLocker guard(&lock_);
    QStringList sites;
    sites.reserve(allowedSites_.size());
    for (const QByteArray &site : allowedSites_)
        sites.append(QString::fromLatin1(site));
    sites.sort(Qt::CaseInsensitive);
    return sites;
}

void AdBlocker::setAllowedSites(const QStringList &sites)
{
    QSet<QByteArray> normalized;
    for (const QString &site : sites) {
        const QByteArray key = normalizedHost(site);
        if (!key.isEmpty())
            normalized.insert(key);
    }

    bool changed = false;
    {
        QWriteLocker guard(&lock_);
        if (allowedSites_ != normalized) {
            allowedSites_ = normalized;
            changed = true;
        }
    }
    if (!changed)
        return;

    saveSettings();
    emit configurationChanged();
}

bool AdBlocker::isEnabledForUrl(const QUrl &url) const
{
    return isEnabled() && isWebUrl(url) && !isSiteAllowed(url);
}

AdBlocker::~AdBlocker()
{
    lw_adblock_engine_destroy(engine_);
}

bool AdBlocker::shouldBlock(
    const QUrl &request, const QUrl &firstParty,
    QWebEngineUrlRequestInfo::ResourceType resourceType) const
{
    using RequestInfo = QWebEngineUrlRequestInfo;

    if (!isEnabledForUrl(firstParty) || !isWebUrl(request))
        return false;

    // Do not interfere with typed links, redirects, OAuth returns, downloads,
    // or top-level documents. A broken site must always be recoverable by
    // simply turning Shield off for that site.
    if (resourceType == RequestInfo::ResourceTypeMainFrame ||
        resourceType == RequestInfo::ResourceTypeNavigationPreloadMainFrame)
        return false;

    if (isVerificationOrChallenge(request))
        return false;

    // Fast-path: Never block video streams, CDN assets, or Same-Site resources with Rust FFI overhead
    const QByteArray requestHost = normalizedHost(request.host());
    if (requestHost.endsWith("googlevideo.com") || requestHost.endsWith("ytimg.com") || requestHost.endsWith("ggpht.com"))
        return false;

    if (isKnownSameSiteAdEndpoint(request, firstParty))
        return true;

    // Standard mode blocks only third-party advertising infrastructure. This
    // protects video players, CDNs, sign-in, checkout, and embedded apps that
    // frequently use first-party or partner subdomains.
    const bool thirdParty = isThirdParty(request, firstParty);
    if (!thirdParty && mode() == Mode::Standard)
        return false;

    if (engine_) {
        const QByteArray requestText = request.toEncoded(QUrl::FullyEncoded);
        const QByteArray sourceText = firstParty.toEncoded(QUrl::FullyEncoded);
        const QByteArray kind = resourceTypeName(resourceType);
        if (lw_adblock_engine_should_block(engine_, requestText.constData(), sourceText.constData(), kind.constData()))
            return true;
    }

    if (!thirdParty)
        return false;

    if (matchesDomain(requestHost, advertisingDomains()))
        return true;

    return mode() == Mode::Aggressive && matchesDomain(requestHost, trackerDomains());
}

void AdBlocker::interceptRequest(QWebEngineUrlRequestInfo &info)
{
    if (dntEnabled_.loadAcquire()) {
        info.setHttpHeader("DNT", "1");
        info.setHttpHeader("Sec-GPC", "1");
    }

    const QUrl reqUrl = info.requestUrl();
    const QByteArray host = normalizedHost(reqUrl.host());

    // Only inject Client Hint headers on main frame navigation or Google/YouTube domains
    // to avoid high CPU/IPC overhead on hundreds of static sub-resource requests per page.
    if (info.resourceType() == QWebEngineUrlRequestInfo::ResourceTypeMainFrame ||
        host == "accounts.google.com" || host == "google.com" || host == "www.google.com" ||
        host.endsWith(".google.com") || host.endsWith(".youtube.com") || host == "youtube.com") {
#if defined(Q_OS_WIN)
        const QByteArray chromeUa = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/130.0.0.0 Safari/537.36";
        const QByteArray platform = "\"Windows\"";
#elif defined(Q_OS_MAC)
        const QByteArray chromeUa = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/130.0.0.0 Safari/537.36";
        const QByteArray platform = "\"macOS\"";
#else
        const QByteArray chromeUa = "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/130.0.0.0 Safari/537.36";
        const QByteArray platform = "\"Linux\"";
#endif
        info.setHttpHeader("User-Agent", chromeUa);
        info.setHttpHeader("Sec-CH-UA", "\"Chromium\";v=\"130\", \"Google Chrome\";v=\"130\", \"Not?A_Brand\";v=\"99\"");
        info.setHttpHeader("Sec-CH-UA-Mobile", "?0");
        info.setHttpHeader("Sec-CH-UA-Platform", platform);
    }

    if (!isEnabled() || !isEnabledForUrl(info.firstPartyUrl()))
        return;

    if (!shouldBlock(info.requestUrl(), info.firstPartyUrl(), info.resourceType()))
        return;

    info.block(true);
    blockedCount_.fetchAndAddRelaxed(1);
}

int AdBlocker::blockedCount() const
{
    return blockedCount_.loadAcquire();
}

void AdBlocker::resetBlockedCount()
{
    blockedCount_.storeRelease(0);
}

void AdBlocker::recordBlockedPopup()
{
    blockedCount_.fetchAndAddRelaxed(1);
}

int AdBlocker::ruleCount() const
{
    return engine_ ? lw_adblock_engine_rule_count(engine_) : advertisingDomains().size() + trackerDomains().size() + 3;
}
QByteArray AdBlocker::resourceTypeName(QWebEngineUrlRequestInfo::ResourceType type)
{
    using Type=QWebEngineUrlRequestInfo;
    switch(type){case Type::ResourceTypeScript:return "script";case Type::ResourceTypeStylesheet:return "stylesheet";case Type::ResourceTypeImage:return "image";case Type::ResourceTypeMedia:return "media";case Type::ResourceTypeSubFrame:return "subdocument";case Type::ResourceTypeXhr:return "xmlhttprequest";default:return "other";}
}

QString AdBlocker::cosmeticCss()
{
    return QStringLiteral(R"CSS(
/* LiteWave Shield: Cosmetic Ad Hiding Rules */
iframe[src*="doubleclick.net"],
iframe[src*="googlesyndication.com"],
iframe[src*="googleadservices.com"],
iframe[src*="adnxs.com"],
iframe[src*="taboola.com"],
iframe[src*="outbrain.com"],
iframe[src*="trafficjunky.com"],
iframe[src*="exoclick.com"],
iframe[src*="juicyads.com"],
.adsbygoogle,
ins.adsbygoogle,
[data-ad-client],
[data-ad-slot],
[data-google-query-id],
#player-ads,
.ytd-ad-slot-renderer,
ytd-promoted-sparkles-web-renderer,
ytd-display-ad-renderer,
ytd-statement-banner-renderer,
ytd-in-feed-ad-layout-renderer,
ytd-banner-promo-renderer-background,
.ytp-ad-overlay-container,
.ytp-ad-message-container,
#masthead-ad,
.video-ads,
.ytp-ad-module {
  display: none !important;
  visibility: hidden !important;
}
)CSS");
}

QString AdBlocker::youtubeAdSkipScript()
{
    return QStringLiteral(R"JS(
(function() {
    if (!/(^|\\.)youtube\\.com$/i.test(location.hostname)) return;
    if (window.__litewave_yt_skipper_installed) return;
    window.__litewave_yt_skipper_installed = true;

    const LW = {
        player: null,
        playerObserver: null,
        documentObserver: null,
        ticker: null,
        savedMuted: null,
        savedRate: null,
        modifiedVideo: null,
        rafPending: false
    };

    function shieldDisabled() {
        return window.__litewave_shield_disabled === true;
    }

    function sanitizePlayerPayload(payload) {
        if (shieldDisabled() || !payload || typeof payload !== 'object')
            return payload;

        // Keep this intentionally narrow. These keys are ad scheduling/payload
        // containers in YouTube player responses; do not touch streamingData,
        // captions, formats, playbackTracking, DRM or account data.
        const adKeys = [
            'adPlacements',
            'playerAds',
            'adSlots',
            'adBreakHeartbeatParams',
            'adBreakParams'
        ];

        for (const key of adKeys) {
            try {
                if (Object.prototype.hasOwnProperty.call(payload, key))
                    delete payload[key];
            } catch (e) {}
        }

        // Some response variants keep ad containers inside playerResponse-ish
        // nested objects. Walk only a shallow set of known containers instead
        // of recursively rewriting arbitrary YouTube JSON.
        const knownContainers = [
            payload.playerResponse,
            payload.response,
            payload.data
        ];
        for (const child of knownContainers) {
            if (!child || typeof child !== 'object' || child === payload)
                continue;
            for (const key of adKeys) {
                try {
                    if (Object.prototype.hasOwnProperty.call(child, key))
                        delete child[key];
                } catch (e) {}
            }
        }

        return payload;
    }

    function isPlayerApiUrl(value) {
        try {
            const raw = typeof value === 'string'
                ? value
                : (value && typeof value.url === 'string' ? value.url : '');
            if (!raw) return false;
            const u = new URL(raw, location.href);
            return /(^|\\.)youtube\\.com$/i.test(u.hostname) &&
                   u.pathname.indexOf('/youtubei/v1/player') !== -1;
        } catch (e) {
            return false;
        }
    }

    function installFetchPlayerGuard() {
        try {
            if (window.__litewave_yt_fetch_guard_installed || typeof window.fetch !== 'function')
                return;
            window.__litewave_yt_fetch_guard_installed = true;

            const originalFetch = window.fetch;
            window.fetch = function(input, init) {
                return originalFetch.apply(this, arguments).then(function(response) {
                    if (shieldDisabled() || !isPlayerApiUrl(input) || !response)
                        return response;

                    try {
                        const originalJson = response.json;
                        if (typeof originalJson === 'function') {
                            Object.defineProperty(response, 'json', {
                                configurable: true,
                                value: function() {
                                    return originalJson.call(response).then(sanitizePlayerPayload);
                                }
                            });
                        }
                    } catch (e) {}

                    try {
                        const originalText = response.text;
                        if (typeof originalText === 'function') {
                            Object.defineProperty(response, 'text', {
                                configurable: true,
                                value: function() {
                                    return originalText.call(response).then(function(text) {
                                        try {
                                            const parsed = JSON.parse(text);
                                            return JSON.stringify(sanitizePlayerPayload(parsed));
                                        } catch (e) {
                                            return text;
                                        }
                                    });
                                }
                            });
                        }
                    } catch (e) {}

                    return response;
                });
            };
        } catch (e) {}
    }

    function installInitialPlayerGuard() {
        try {
            if (window.__litewave_yt_initial_guard_installed)
                return;
            window.__litewave_yt_initial_guard_installed = true;

            let initialValue;
            try { initialValue = window.ytInitialPlayerResponse; } catch (e) {}

            Object.defineProperty(window, 'ytInitialPlayerResponse', {
                configurable: true,
                enumerable: true,
                get: function() {
                    return initialValue;
                },
                set: function(value) {
                    initialValue = sanitizePlayerPayload(value);
                }
            });

            if (initialValue && typeof initialValue === 'object')
                initialValue = sanitizePlayerPayload(initialValue);
        } catch (e) {}
    }

    function sanitizeLiveInitialPayload() {
        if (shieldDisabled()) return;
        try {
            if (window.ytInitialPlayerResponse &&
                typeof window.ytInitialPlayerResponse === 'object') {
                sanitizePlayerPayload(window.ytInitialPlayerResponse);
            }
        } catch (e) {}
    }

    // Install response guards immediately at DocumentCreation, before the
    // YouTube application starts issuing player API requests.
    installFetchPlayerGuard();
    installInitialPlayerGuard();

    function installCosmetics() {
        try {
            if (document.getElementById('litewave-youtube-shield-style')) return;
            const style = document.createElement('style');
            style.id = 'litewave-youtube-shield-style';
            style.textContent = `
                #player-ads,
                #masthead-ad,
                ytd-ad-slot-renderer,
                ytd-display-ad-renderer,
                ytd-promoted-sparkles-web-renderer,
                ytd-promoted-video-renderer,
                ytd-action-companion-ad-renderer,
                ytd-companion-slot-renderer,
                ytd-in-feed-ad-layout-renderer,
                ytd-banner-promo-renderer-background,
                .video-ads,
                .ytp-ad-module,
                .ytp-ad-overlay-container,
                .ytp-ad-overlay-slot,
                .ytp-ad-text-overlay,
                .ytp-ad-player-overlay,
                .ytp-ad-message-container {
                    display: none !important;
                    visibility: hidden !important;
                    pointer-events: none !important;
                }
            `;
            (document.head || document.documentElement).appendChild(style);
        } catch (e) {}
    }

    function currentPlayer() {
        return document.getElementById('movie_player') ||
               document.querySelector('.html5-video-player');
    }

    function currentVideo(player) {
        if (player) {
            const inside = player.querySelector && player.querySelector('video');
            if (inside) return inside;
        }
        return document.querySelector('video.html5-main-video') ||
               document.querySelector('video');
    }

    function isAdShowing(player) {
        return !!player &&
               (player.classList.contains('ad-showing') ||
                player.classList.contains('ad-interrupting'));
    }

    function rememberPlayback(video) {
        if (LW.modifiedVideo === video) return;
        restorePlayback();
        LW.modifiedVideo = video;
        try { LW.savedMuted = video.muted; } catch (e) { LW.savedMuted = null; }
        try { LW.savedRate = video.playbackRate; } catch (e) { LW.savedRate = null; }
    }

    function restorePlayback() {
        const video = LW.modifiedVideo;
        if (video) {
            try {
                if (LW.savedMuted !== null) video.muted = LW.savedMuted;
            } catch (e) {}
            try {
                if (LW.savedRate !== null && Number.isFinite(LW.savedRate) && LW.savedRate > 0)
                    video.playbackRate = LW.savedRate;
            } catch (e) {}
        }
        LW.modifiedVideo = null;
        LW.savedMuted = null;
        LW.savedRate = null;
    }

    function clickSkipButton(player) {
        const selectors = [
            '.ytp-skip-ad-button-modern',
            '.ytp-ad-skip-button-modern',
            '.ytp-ad-skip-button',
            '.ytp-ad-skip-button-slot button',
            'button.ytp-ad-skip-button',
            'button[class*="skip-ad"]'
        ];

        for (const sel of selectors) {
            const btn = (player && player.querySelector ? player.querySelector(sel) : null) ||
                        document.querySelector(sel);
            if (!btn) continue;
            try {
                if (!btn.disabled) {
                    btn.click();
                    return true;
                }
            } catch (e) {}
        }
        return false;
    }

    function accelerateAd(player, video) {
        rememberPlayback(video);

        try { video.muted = true; } catch (e) {}
        try { video.playbackRate = 16.0; } catch (e) {}

        // For unskippable creatives, jumping close to the media end is much
        // cheaper than spinning the decoder at high speed. Ignore live/unknown
        // duration media where seeking is not meaningful.
        try {
            const duration = video.duration;
            const current = video.currentTime;
            if (Number.isFinite(duration) && duration > 0.5 &&
                Number.isFinite(current) && current + 0.35 < duration) {
                video.currentTime = Math.max(current, duration - 0.15);
            }
        } catch (e) {}

        clickSkipButton(player);

        try {
            if (typeof player.skipAd === 'function')
                player.skipAd();
        } catch (e) {}
    }

    function runOnce() {
        if (shieldDisabled()) {
            restorePlayback();
            return;
        }

        installCosmetics();
        sanitizeLiveInitialPayload();

        const player = currentPlayer();
        if (!player) {
            restorePlayback();
            return;
        }

        if (LW.player !== player)
            attachPlayer(player);

        const video = currentVideo(player);
        if (!video) return;

        if (isAdShowing(player)) {
            accelerateAd(player, video);
        } else {
            restorePlayback();
        }
    }

    function scheduleRun() {
        if (LW.rafPending) return;
        LW.rafPending = true;
        requestAnimationFrame(function() {
            LW.rafPending = false;
            runOnce();
        });
    }

    function attachPlayer(player) {
        if (!player || LW.player === player) return;

        if (LW.playerObserver) {
            try { LW.playerObserver.disconnect(); } catch (e) {}
        }

        LW.player = player;
        LW.playerObserver = new MutationObserver(scheduleRun);
        try {
            LW.playerObserver.observe(player, {
                attributes: true,
                attributeFilter: ['class'],
                childList: true,
                subtree: false
            });
        } catch (e) {}

        try {
            player.addEventListener('timeupdate', function() {
                if (isAdShowing(player)) scheduleRun();
            }, { passive: true });
        } catch (e) {}

        scheduleRun();
    }

    function attachDocumentObserver() {
        if (LW.documentObserver || !document.documentElement) return;

        LW.documentObserver = new MutationObserver(function() {
            const player = currentPlayer();
            if (player && player !== LW.player)
                attachPlayer(player);
        });

        try {
            LW.documentObserver.observe(document.documentElement, {
                childList: true,
                subtree: true
            });
        } catch (e) {}
    }

    function init() {
        installCosmetics();
        attachDocumentObserver();
        const player = currentPlayer();
        if (player) attachPlayer(player);
        runOnce();

        // Low-frequency fallback for YouTube SPA/player replacements that do
        // not produce the exact mutations older versions emitted.
        if (!LW.ticker) {
            LW.ticker = setInterval(runOnce, 750);
        }
    }

    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', init, { once: true });
    } else {
        init();
    }

    window.addEventListener('yt-navigate-start', function() {
        restorePlayback();
    }, { passive: true });

    window.addEventListener('yt-navigate-finish', function() {
        LW.player = null;
        installFetchPlayerGuard();
        sanitizeLiveInitialPayload();
        init();
    }, { passive: true });

    window.addEventListener('pagehide', restorePlayback, { passive: true });
})();
)JS");
}

QByteArray AdBlocker::normalizedHost(const QString &host)
{
    QString normalized = host.trimmed().toLower();
    while (normalized.endsWith('.'))
        normalized.chop(1);
    return QUrl::toAce(normalized).toLower();
}

QByteArray AdBlocker::siteKey(const QUrl &url)
{
    const QByteArray host = normalizedHost(url.host());
    const QList<QByteArray> labels = host.split('.');
    if (labels.size() < 3)
        return host;

    const QByteArray twoPartSuffix =
        labels.at(labels.size() - 2) + "." + labels.last();
    static const QSet<QByteArray> multiPartSuffixes = {
        "co.uk", "org.uk", "ac.uk", "gov.uk", "com.au", "net.au", "org.au",
        "co.nz", "co.jp", "co.kr", "co.th", "co.in", "com.br", "com.mx",
        "com.tr", "com.sg", "com.cn", "com.tw", "com.hk", "co.za", "com.ar"
    };
    if (labels.size() >= 3 && multiPartSuffixes.contains(twoPartSuffix))
        return labels.at(labels.size() - 3) + "." + twoPartSuffix;
    return twoPartSuffix;
}

bool AdBlocker::matchesDomain(const QByteArray &host,
                              const QSet<QByteArray> &domains)
{
    if (host.isEmpty())
        return false;

    QByteArray candidate = host;
    while (!candidate.isEmpty()) {
        if (domains.contains(candidate))
            return true;
        const int dot = candidate.indexOf('.');
        if (dot < 0)
            break;
        candidate = candidate.mid(dot + 1);
    }
    return false;
}

bool AdBlocker::isThirdParty(const QUrl &request, const QUrl &firstParty)
{
    const QByteArray requestSite = siteKey(request);
    const QByteArray firstPartySite = siteKey(firstParty);
    return !requestSite.isEmpty() && !firstPartySite.isEmpty() &&
           requestSite != firstPartySite;
}

bool AdBlocker::isVerificationOrChallenge(const QUrl &url)
{
    const QByteArray host = normalizedHost(url.host());
    if (matchesDomain(host, QSet<QByteArray>{"recaptcha.net", "hcaptcha.com", "accounts.google.com"}))
        return true;
    if (host == "challenges.cloudflare.com")
        return true;
    if ((host == "www.google.com" || host == "www.gstatic.com" || host == "accounts.google.com") &&
        (url.path().startsWith("/recaptcha/") ||
         url.path().startsWith("/sorry/") ||
         url.path().startsWith("/v3/signin/") ||
         url.path().startsWith("/ServiceLogin") ||
         url.path().startsWith("/o/oauth2/")))
        return true;
    return false;
}

bool AdBlocker::isKnownSameSiteAdEndpoint(const QUrl &request,
                                          const QUrl &firstParty)
{
    const QByteArray firstPartyHost = normalizedHost(firstParty.host());
    const QByteArray requestHost = normalizedHost(request.host());
    const QString path = request.path();

    const bool youtubePage = matchesDomain(firstPartyHost,
                                           QSet<QByteArray>{"youtube.com",
                                                            "youtube-nocookie.com"});
    if (youtubePage && matchesDomain(requestHost, QSet<QByteArray>{"youtube.com"})) {
        return false;
    }

    if ((requestHost == "www.google.com" || requestHost == "google.com") &&
        (path.startsWith("/pagead/") || path.startsWith("/ads/ga-audiences")))
        return true;

    return false;
}

void AdBlocker::saveSettings() const
{
    if (!persistent_)
        return;

    QSettings settings("LiteWave", "LiteWave");
    settings.setValue("shield/enabled", isEnabled());
    settings.setValue("shield/mode",
                      mode() == Mode::Aggressive ? "aggressive" : "standard");
    settings.setValue("shield/allowedSites", allowedSites());
}
