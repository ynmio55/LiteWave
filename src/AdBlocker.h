#pragma once

#include <QWebEngineUrlRequestInterceptor>
#include <QWebEngineUrlRequestInfo>
#include <QAtomicInt>
#include <QReadWriteLock>
#include <QSet>
#include <QByteArray>
#include <QDateTime>
#include <QUrl>

class QNetworkAccessManager;

class AdBlocker final : public QWebEngineUrlRequestInterceptor
{
    Q_OBJECT
public:
    explicit AdBlocker(QObject *parent = nullptr, bool persistent = true);
    void setEnabled(bool enabled);
    bool isEnabled() const { return enabled_.loadAcquire() != 0; }
    // Standard keeps web apps compatible; Aggressive also blocks first-party
    // tracker endpoints and unsolicited third-party popups.
    void setAggressive(bool enabled);
    bool isAggressive() const { return aggressive_.loadAcquire() != 0; }
    bool isEnabledForUrl(const QUrl &url) const;
    bool isSiteAllowed(const QUrl &url) const;
    void setSiteAllowed(const QUrl &url, bool allowed);
    void interceptRequest(QWebEngineUrlRequestInfo &info) override;
    bool shouldBlock(const QUrl &request, const QUrl &firstParty,
                     QWebEngineUrlRequestInfo::ResourceType type) const;
    bool shouldBlockPopup(const QUrl &request, const QUrl &opener,
                          bool userInitiated) const;
    bool isDomainBlocked(const QString &host) const;
    int blockedCount() const { return blockedCount_.loadAcquire(); }
    void recordBlocked() { blockedCount_.fetchAndAddRelaxed(1); }
    void resetCount() { blockedCount_.storeRelease(0); }
    int ruleCount() const;
    bool isUpdating() const { return updating_; }
    QString filterStatus() const;
    void updateFilters();

    // Hosts syntax only. Unsupported/malformed input rejects the entire update.
    static QSet<QByteArray> parseHosts(const QByteArray &data, bool *ok = nullptr);
    static bool validUpdate(const QByteArray &data);
    static QString cosmeticScript(bool enabled);

signals:
    void configurationChanged();
    void filtersUpdated(bool success, const QString &message);

private:
    void saveSettings() const;
    QString cacheFile() const;
    QAtomicInt enabled_ = 1;
    QAtomicInt aggressive_ = 0;
    QAtomicInt blockedCount_ = 0;
    mutable QReadWriteLock lock_;
    QSet<QByteArray> exactHosts_;
    QSet<QByteArray> familyHosts_;
    QSet<QByteArray> aggressiveHosts_;
    QSet<QByteArray> allowedSites_;
    bool persistent_ = true;
    bool updating_ = false; // GUI thread only
    QDateTime updatedAt_;
    QNetworkAccessManager *network_ = nullptr;
};
