#pragma once

#include <QDateTime>
#include <QObject>
#include <QPointer>

class QNetworkAccessManager;
class QNetworkReply;
class QTimer;

// Keeps EasyList and EasyPrivacy fresh. Downloads land in the app data
// directory and are compiled on a worker thread, then swapped into the shared
// Shield engine without a restart. The snapshot bundled at build time remains
// the fallback whenever a download is missing or malformed.
class FilterListUpdater final : public QObject
{
    Q_OBJECT

public:
    explicit FilterListUpdater(QObject *parent = nullptr);

    // Loads previously downloaded lists, then checks for updates on a timer.
    void start();
    void updateNow();

    QDateTime lastUpdated() const;
    bool isUpdating() const { return pending_ > 0; }

signals:
    void listsUpdated(int ruleCount);
    void updateFailed(const QString &reason);

private:
    static QString cacheDir();
    static bool looksLikeFilterList(const QByteArray &data);
    void checkIfStale();
    void handleReply(QNetworkReply *reply, int index);
    void loadCached(bool emitResult);

    QNetworkAccessManager *network_ = nullptr;
    QTimer *timer_ = nullptr;
    QByteArray downloaded_[2];
    int pending_ = 0;
    bool failed_ = false;
};
