#include "AdBlocker.h"

#include <QFile>
#include <QStringList>
#include <QTimer>
#include <QSaveFile>
#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>
#include <QRegularExpression>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrentRun>
#include <memory>

namespace {
constexpr qint64 MaxListBytes = 4 * 1024 * 1024;
QByteArray hostKey(QString host)
{
    host = host.trimmed().toLower();
    while (host.endsWith('.')) host.chop(1);
    return QUrl::toAce(host).toLower();
}
bool family(const QByteArray &host, const QByteArray &domain)
{
    return host == domain || host.endsWith(QByteArray(".") + domain);
}
bool hasFamily(QByteArray host, const QSet<QByteArray> &domains)
{
    while (!host.isEmpty()) {
        if (domains.contains(host)) return true;
        const int dot = host.indexOf('.');
        if (dot < 0) break;
        host = host.mid(dot + 1);
    }
    return false;
}
bool isVerification(const QUrl &url)
{
    const auto host = hostKey(url.host());
    return family(host, "recaptcha.net") || family(host, "hcaptcha.com") ||
        host == "challenges.cloudflare.com" ||
        ((host == "www.google.com" || host == "www.gstatic.com") &&
         (url.path().startsWith("/recaptcha/") || url.path().startsWith("/sorry/")));
}

bool isYouTubePage(const QUrl &url)
{
    const auto host = hostKey(url.host());
    return family(host, "youtube.com") || family(host, "youtube-nocookie.com") ||
           host == "youtu.be";
}

// A small registrable-domain approximation keeps CDNs such as
// static.example.com first-party to www.example.com without introducing a
// heavyweight public-suffix dependency into the request hot path.
QByteArray siteKey(const QUrl &url)
{
    const auto host = hostKey(url.host());
    const auto labels = host.split('.');
    if (labels.size() < 3)
        return host;
    const QByteArray suffix = labels[labels.size() - 2] + "." + labels.back();
    static const QSet<QByteArray> multiLabelSuffixes = {
        "co.uk", "org.uk", "ac.uk", "gov.uk", "com.au", "net.au", "org.au",
        "co.nz", "co.jp", "co.kr", "co.th", "co.in", "com.br", "com.mx",
        "com.tr", "com.sg", "com.cn", "com.tw", "com.hk", "co.za", "com.ar"
    };
    if (labels.size() >= 3 && multiLabelSuffixes.contains(suffix))
        return labels[labels.size() - 3] + "." + suffix;
    return suffix;
}

bool isThirdParty(const QUrl &request, const QUrl &firstParty)
{
    const auto requestSite = siteKey(request);
    const auto firstPartySite = siteKey(firstParty);
    return !requestSite.isEmpty() && !firstPartySite.isEmpty() &&
           requestSite != firstPartySite;
}
}

AdBlocker::AdBlocker(QObject *parent, bool persistent)
    : QWebEngineUrlRequestInterceptor(parent), persistent_(persistent)
{
    // Network advertising domains that can be blocked in Standard mode when
    // they are third-party.  These are intentionally not broad wildcard URL
    // rules: the bundled hosts list supplies additional coverage.
    static const QStringList adDomains = {
        "doubleclick.net", "googlesyndication.com", "googleadservices.com",
        "adservice.google.com", "pagead2.googlesyndication.com", "ad.doubleclick.net",
        "securepubads.g.doubleclick.net", "stats.g.doubleclick.net",
        "video-stats.l.google.com", "tpc.googlesyndication.com", "ads.yahoo.com",
        "adnxs.com", "adform.net", "taboola.com", "outbrain.com", "popads.net",
        "popcash.net", "exoclick.com", "juicyads.com", "trafficjunky.com",
        "syndicatedsearch.goog", "realsrv.com", "exosrv.com", "trafficfactory.biz",
        "zedo.com", "criteo.com", "rubiconproject.com", "pubmatic.com", "openx.net",
        "casalemedia.com", "contextweb.com", "advertising.com", "turn.com", "33across.com",
        "sharethrough.com", "smartadserver.com", "lijit.com", "smaato.net",
        "inmobi.com", "appier.net", "media.net", "mgid.com", "revcontent.com",
        "adroll.com", "bidswitch.net", "adsrvr.org", "rlcdn.com", "demdex.net",
        "quantserve.com", "krxd.net", "bluekai.com", "amazon-adsystem.com",
        "adsterra.com", "propellerads.com", "a-ads.com", "hilltopads.com",
        "clickadu.com", "monetag.com", "adstriker.com", "adcash.com", "adtrue.com",
        "exponential.com", "yieldmo.com", "spotxchange.com", "teads.tv",
        "admanmedia.com", "bebi.com", "clksite.com", "coinhive.com",
        "doubleverify.com", "integralads.com", "adblade.com", "adcolony.com",
        "adriver.ru", "airpush.com", "applovin.com", "clicktale.com",
        "conversantmedia.com", "eplanning.net", "flashtalking.com", "gumgum.com",
        "indexww.com", "kargo.com", "liveramp.com", "mathtag.com", "plista.com",
        "sail-thru.com", "sharethis.com", "sizmek.com", "sovrn.com", "tapad.com",
        "unruly.co", "vungle.com", "trafficjunky.net", "tsyndicate.com",
        "adxpansion.com", "ero-advertising.com", "ad-maven.com", "ad-maven.net",
        "trafficstars.com", "adskeeper.com", "worldoftanks.asia"
    };
    // Analytics/behavioural measurement can host functional site code. Keep
    // it opt-in under Aggressive so the default does not make legitimate
    // login, checkout, or embedded web apps fail.
    static const QStringList aggressiveDomains = {
        "google-analytics.com", "googletagmanager.com", "scorecardresearch.com",
        "moatads.com", "hotjar.com", "mouseflow.com", "fullstory.com",
        "mixpanel.com", "app-measurement.com", "segment.io", "amplitude.com",
        "umeng.com", "analytics.yahoo.com", "chartbeat.com", "clarity.ms",
        "crazyegg.com"
    };

    for (const auto &domain : adDomains)
        familyHosts_.insert(hostKey(domain));
    for (const auto &domain : aggressiveDomains)
        aggressiveHosts_.insert(hostKey(domain));

    // The immutable bundled set is shared between profiles until an update.
    static const auto bundled = [] {
        QFile file(":/shield/adaway-hosts.txt");
        return file.open(QIODevice::ReadOnly) ? parseHosts(file.readAll()) : QSet<QByteArray>{};
    }();
    exactHosts_ = bundled;
    QFile cached(cacheFile());
    if (cached.size() <= MaxListBytes && cached.open(QIODevice::ReadOnly)) {
        const auto bytes = cached.readAll();
        if (validUpdate(bytes)) {
            exactHosts_ = parseHosts(bytes);
            updatedAt_ = QFileInfo(cached).lastModified();
        }
    }

    // Private windows inherit preferences, but never write exceptions/history
    // to disk.
    QSettings st("LiteWave", "LiteWave");
    enabled_.storeRelease(st.value("shield/enabled", true).toBool());
    aggressive_.storeRelease(st.value("shield/aggressive", false).toBool());
    for (const auto &host : st.value("shield/allowedHosts").toStringList())
        allowedSites_.insert(hostKey(host));
}

QString AdBlocker::cacheFile() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/Shield/adaway-hosts.txt";
}

QSet<QByteArray> AdBlocker::parseHosts(const QByteArray &data, bool *ok)
{
    if (ok) *ok = false;
    QSet<QByteArray> result;
    if (data.isEmpty() || data.size() > MaxListBytes || data.contains('\0')) return result;
    static const QRegularExpression domain(
        QStringLiteral(R"(^[a-z0-9](?:[a-z0-9-]{0,61}[a-z0-9])?(?:\.[a-z0-9](?:[a-z0-9-]{0,61}[a-z0-9])?)+$)"));
    static const QRegularExpression spaces(QStringLiteral("\\s+"));
    for (const auto &raw : data.split('\n')) {
        const auto line = QString::fromUtf8(raw).section('#', 0, 0).trimmed();
        if (line.isEmpty()) continue;
        const auto fields = line.split(spaces, Qt::SkipEmptyParts);
        if (fields.size() < 2 ||
            (fields[0] != "0.0.0.0" && fields[0] != "127.0.0.1" && fields[0] != "::1")) return {};
        for (int i = 1; i < fields.size(); ++i) {
            const auto h = fields[i].toLower();
            if (h == "localhost" || h == "localhost.localdomain" || h == "ip6-localhost" ||
                h == "ip6-loopback" || h == "0.0.0.0" || h == "broadcasthost") continue;
            if (h.size() > 253 || !domain.match(h).hasMatch() ||
                h.back().isDigit()) return {};
            result.insert(h.toLatin1());
        }
    }
    if (ok) *ok = !result.isEmpty();
    return result;
}

bool AdBlocker::validUpdate(const QByteArray &data)
{
    bool ok = false;
    const auto hosts = parseHosts(data, &ok);
    return ok && hosts.size() >= 1000 && hosts.size() <= 100000 &&
        data.startsWith("# AdAway default blocklist");
}

void AdBlocker::saveSettings() const
{
    if (!persistent_) return;
    QStringList hosts;
    { QReadLocker guard(&lock_); for (const auto &host : allowedSites_) hosts << QString::fromLatin1(host); }
    QSettings st("LiteWave", "LiteWave");
    st.setValue("shield/enabled", isEnabled());
    st.setValue("shield/aggressive", isAggressive());
    st.setValue("shield/allowedHosts", hosts);
}

void AdBlocker::setEnabled(bool enabled)
{
    if (isEnabled() == enabled) return;
    enabled_.storeRelease(enabled);
    saveSettings();
    emit configurationChanged();
}

void AdBlocker::setAggressive(bool enabled)
{
    if (isAggressive() == enabled) return;
    aggressive_.storeRelease(enabled);
    saveSettings();
    emit configurationChanged();
}

bool AdBlocker::isSiteAllowed(const QUrl &url) const
{
    QReadLocker guard(&lock_);
    return allowedSites_.contains(hostKey(url.host()));
}

bool AdBlocker::isEnabledForUrl(const QUrl &url) const
{
    // LiteWave Shield is deliberately scoped to YouTube. Other websites get
    // no request interception, popup blocking, or cosmetic filtering so their
    // video hosts, login flows, and embedded apps remain untouched.
    return isEnabled() && isYouTubePage(url) && !isSiteAllowed(url);
}

void AdBlocker::setSiteAllowed(const QUrl &url, bool allowed)
{
    const auto host = hostKey(url.host());
    if (host.isEmpty() || (url.scheme() != "http" && url.scheme() != "https")) return;
    {
        QWriteLocker guard(&lock_);
        if (allowed) allowedSites_.insert(host); else allowedSites_.remove(host);
    }
    saveSettings();
    emit configurationChanged();
}

bool AdBlocker::isDomainBlocked(const QString &host) const
{
    const auto key = hostKey(host);
    QReadLocker guard(&lock_);
    // Hosts entries are exact: do not silently expand them to entire services.
    return exactHosts_.contains(key) || hasFamily(key, familyHosts_) ||
           hasFamily(key, aggressiveHosts_);
}

bool AdBlocker::shouldBlock(const QUrl &request, const QUrl &firstParty,
                           QWebEngineUrlRequestInfo::ResourceType type) const
{
    using Info = QWebEngineUrlRequestInfo;
    if (!isEnabledForUrl(firstParty) || !request.isValid())
        return false;
    if (request.scheme() != "http" && request.scheme() != "https" &&
        request.scheme() != "ws" && request.scheme() != "wss")
        return false;

    // This guard is redundant with isEnabledForUrl but makes the request
    // boundary explicit for callers/tests.
    if (!isYouTubePage(firstParty))
        return false;

    // Never blank an explicit navigation, download, OAuth return, age check,
    // or consent page.  This is the most important compatibility boundary.
    if (type == Info::ResourceTypeMainFrame ||
        type == Info::ResourceTypeNavigationPreloadMainFrame)
        return false;
    if (isVerification(request))
        return false;

    const auto host = hostKey(request.host());
    const auto path = request.path();
    // URL rules are narrowly scoped; a query string alone never triggers a
    // block. They work even when an ad request is same-site.
    if (family(host, "youtube.com")) {
        return path.startsWith("/api/stats/ads") || path.startsWith("/pagead/") ||
               path == "/get_midroll_info";
    }
    if (host == "www.google.com")
        return path.startsWith("/pagead/") || path.startsWith("/ads/ga-audiences");

    const bool thirdParty = isThirdParty(request, firstParty);
    bool matchedHostRule = false;
    bool matchedAggressiveRule = false;
    {
        QReadLocker guard(&lock_);
        matchedHostRule = exactHosts_.contains(host) || hasFamily(host, familyHosts_);
        matchedAggressiveRule = hasFamily(host, aggressiveHosts_);
    }

    if (matchedAggressiveRule && !isAggressive())
        return false;
    // Standard mode is third-party only. This lets a site load its own CDNs,
    // login provider assets, media player, and checkout scripts.
    return (matchedHostRule || matchedAggressiveRule) &&
           (thirdParty || isAggressive());
}

bool AdBlocker::shouldBlockPopup(const QUrl &request, const QUrl &opener,
                                bool userInitiated) const
{
    if (!isEnabledForUrl(opener) || !request.isValid())
        return false;
    if (shouldBlock(request, opener,
                    QWebEngineUrlRequestInfo::ResourceTypeSubFrame))
        return true;

    // Login/payment flows often open a window without a direct click. Do not
    // block them in Standard mode; Aggressive can still stop generic
    // unsolicited third-party popups.
    return !userInitiated && isAggressive() && isThirdParty(request, opener);
}

void AdBlocker::interceptRequest(QWebEngineUrlRequestInfo &info)
{
    // No disk/network I/O, JavaScript execution, profile calls or per-request UI signals.
    if (shouldBlock(info.requestUrl(), info.firstPartyUrl(), info.resourceType())) {
        info.block(true);
        recordBlocked();
    }
}

int AdBlocker::ruleCount() const
{
    QReadLocker guard(&lock_);
    return exactHosts_.size() + familyHosts_.size() + aggressiveHosts_.size();
}

QString AdBlocker::filterStatus() const
{
    return QString("LiteWave Shield (%1): %2 rules | %3")
        .arg(isAggressive() ? "Aggressive" : "Standard")
        .arg(ruleCount())
        .arg(updatedAt_.isValid()
                 ? updatedAt_.toLocalTime().toString("yyyy-MM-dd HH:mm")
                 : "bundled 2026-09-14");
}

void AdBlocker::updateFilters()
{
    if (updating_) return;
    updating_ = true;
    if (!network_) network_ = new QNetworkAccessManager(this);
    QNetworkRequest req(QUrl("https://raw.githubusercontent.com/AdAway/adaway.github.io/master/hosts.txt"));
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    req.setTransferTimeout(15000);
    auto *reply = network_->get(req);
    QTimer::singleShot(20000, reply, [reply] { if (reply->isRunning()) reply->abort(); });
    reply->setReadBufferSize(MaxListBytes + 1);
    auto bytes = std::make_shared<QByteArray>();
    connect(reply, &QIODevice::readyRead, this, [reply, bytes] {
        bytes->append(reply->readAll());
        if (bytes->size() > MaxListBytes) reply->abort();
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply, bytes] {
        bytes->append(reply->readAll());
        const bool received = reply->error() == QNetworkReply::NoError &&
            reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 200 &&
            bytes->size() <= MaxListBytes;
        reply->deleteLater();
        if (!received) {
            updating_ = false;
            emit filtersUpdated(false, "อัปเดตไม่สำเร็จ ยังคงใช้รายการเดิม");
            return;
        }
        auto *watcher = new QFutureWatcher<QSet<QByteArray>>(this);
        connect(watcher, &QFutureWatcher<QSet<QByteArray>>::finished, this, [this, watcher, bytes] {
            auto hosts = watcher->result();
            watcher->deleteLater();
            if (hosts.isEmpty()) {
                updating_ = false;
                emit filtersUpdated(false, "รายการไม่ถูกต้อง ยังคงใช้รายการเดิม");
                return;
            }
            if (persistent_) {
                QDir().mkpath(QFileInfo(cacheFile()).absolutePath());
                QSaveFile file(cacheFile());
                if (!file.open(QIODevice::WriteOnly) || file.write(*bytes) != bytes->size() || !file.commit()) {
                    updating_ = false;
                    emit filtersUpdated(false, "บันทึกรายการไม่ได้ ยังคงใช้รายการเดิม");
                    return;
                }
            }
            { QWriteLocker guard(&lock_); exactHosts_.swap(hosts); }
            updatedAt_ = QDateTime::currentDateTimeUtc();
            updating_ = false;
            emit filtersUpdated(true, "อัปเดตรายการแล้ว — " + filterStatus());
        });
        watcher->setFuture(QtConcurrent::run([bytes] {
            return validUpdate(*bytes) ? parseHosts(*bytes) : QSet<QByteArray>{};
        }));
    });
}

QString AdBlocker::cosmeticScript(bool enabled)
{
    QFile file(":/shield/shield.js");
    if (!file.open(QIODevice::ReadOnly)) return {};
    return QStringLiteral("globalThis.__litewaveShieldEnabled = ") +
        (enabled ? "true;\n" : "false;\n") + QString::fromUtf8(file.readAll());
}
