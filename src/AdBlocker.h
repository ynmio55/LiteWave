#pragma once

#include <QWebEngineUrlRequestInterceptor>
#include <QSet>
#include <QStringList>
#include <QAtomicInt>

class AdBlocker final : public QWebEngineUrlRequestInterceptor
{
    Q_OBJECT
public:
    explicit AdBlocker(QObject *parent = nullptr);
    void setEnabled(bool enabled);
    bool isEnabled() const { return enabled_; }
    void interceptRequest(QWebEngineUrlRequestInfo &info) override;

    int blockedCount() const { return blockedCount_.loadAcquire(); }
    void resetCount() { blockedCount_.storeRelease(0); emit countChanged(0); }

    bool isDomainBlocked(const QString &host) const;
    bool isPathBlocked(const QString &target) const;

    static QString cosmeticCss();
    static QString cosmeticJs();

signals:
    void countChanged(int count);

private:
    bool enabled_ = true;
    QAtomicInt blockedCount_ = 0;
    QSet<QString> blockedDomainsSet_;
    QStringList blockedPathParts_;
};

