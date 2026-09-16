#pragma once

#include <QAtomicInt>
#include <QReadWriteLock>
#include <QSet>
#include <QStringList>
#include <QUrl>
#include <QWebEngineUrlRequestInterceptor>
#include <QWebEngineUrlRequestInfo>

struct LiteWaveAdblockEngine;

// LiteWave Shield deliberately uses a small, auditable network-rule set.
// It only blocks known advertising endpoints, and it never blocks main-frame
// navigation, verification pages, or a site explicitly allowed by the user.
class AdBlocker final : public QWebEngineUrlRequestInterceptor
{
    Q_OBJECT

public:
    enum class Mode {
        Standard,
        Aggressive
    };

    explicit AdBlocker(QObject *parent = nullptr, bool persistent = true);

    void setEnabled(bool enabled);
    bool isEnabled() const;

    void setMode(Mode mode);
    Mode mode() const;

    bool isSiteAllowed(const QUrl &url) const;
    void setSiteAllowed(const QUrl &url, bool allowed);
    QStringList allowedSites() const;
    void setAllowedSites(const QStringList &sites);

    bool isEnabledForUrl(const QUrl &url) const;

    // Kept public for deterministic unit tests. The hot path does no network,
    // disk, JavaScript, QSettings, or UI work.
    bool shouldBlock(const QUrl &request, const QUrl &firstParty,
                     QWebEngineUrlRequestInfo::ResourceType resourceType) const;
    void interceptRequest(QWebEngineUrlRequestInfo &info) override;

    int blockedCount() const;
    void resetBlockedCount();
    void recordBlockedPopup();
    int ruleCount() const;

    // Cosmetic rules only hide elements that identify a known ad endpoint or
    // an explicit ad slot. Network matching remains the source of truth.
    static QString cosmeticCss();

signals:
    void configurationChanged();

private:
    static QByteArray normalizedHost(const QString &host);
    static QByteArray siteKey(const QUrl &url);
    static bool matchesDomain(const QByteArray &host, const QSet<QByteArray> &domains);
    static bool isThirdParty(const QUrl &request, const QUrl &firstParty);
    static bool isVerificationOrChallenge(const QUrl &url);
    static QByteArray resourceTypeName(QWebEngineUrlRequestInfo::ResourceType resourceType);
    static bool isKnownSameSiteAdEndpoint(const QUrl &request,
                                          const QUrl &firstParty);

    void saveSettings() const;

    QAtomicInt enabled_ = 1;
    QAtomicInt mode_ = static_cast<int>(Mode::Standard);
    QAtomicInt blockedCount_ = 0;
    mutable QReadWriteLock lock_;
    QSet<QByteArray> allowedSites_;
    bool persistent_ = true;
    LiteWaveAdblockEngine *engine_ = nullptr;
};
