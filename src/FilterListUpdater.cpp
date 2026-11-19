#include "FilterListUpdater.h"
#include "LiteWaveFilterEngine.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSaveFile>
#include <QSettings>
#include <QStandardPaths>
#include <QTimer>
#include <QThreadPool>

namespace {

constexpr qint64 kUpdateIntervalSecs = 4 * 24 * 60 * 60;
constexpr int kCheckIntervalMs = 6 * 60 * 60 * 1000;
constexpr qint64 kMaxListBytes = 32 * 1024 * 1024;

const char *const kFileNames[2] = {"easylist.txt", "easyprivacy.txt"};
const char *const kUrls[2] = {"https://easylist.to/easylist/easylist.txt",
                              "https://easylist.to/easylist/easyprivacy.txt"};

} // namespace

FilterListUpdater::FilterListUpdater(QObject *parent)
    : QObject(parent), network_(new QNetworkAccessManager(this)),
      timer_(new QTimer(this))
{
    timer_->setInterval(kCheckIntervalMs);
    connect(timer_, &QTimer::timeout, this, &FilterListUpdater::checkIfStale);
}

void FilterListUpdater::start()
{
    loadCached(false);
    timer_->start();
    // Give startup and the first page load the network before checking.
    QTimer::singleShot(20 * 1000, this, &FilterListUpdater::checkIfStale);
}

QString FilterListUpdater::cacheDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
           QStringLiteral("/filters");
}

bool FilterListUpdater::looksLikeFilterList(const QByteArray &data)
{
    return data.size() > 10 * 1024 && data.trimmed().startsWith("[Adblock Plus");
}

QDateTime FilterListUpdater::lastUpdated() const
{
    QSettings settings("LiteWave", "LiteWave");
    return settings.value("shield/filtersUpdatedAt").toDateTime();
}

void FilterListUpdater::checkIfStale()
{
    const QDateTime last = lastUpdated();
    if (!last.isValid() ||
        last.secsTo(QDateTime::currentDateTimeUtc()) > kUpdateIntervalSecs)
        updateNow();
}

void FilterListUpdater::updateNow()
{
    if (pending_ > 0)
        return;
    failed_ = false;
    pending_ = 2;
    for (int i = 0; i < 2; ++i) {
        downloaded_[i].clear();
        QNetworkRequest request{QUrl(QString::fromLatin1(kUrls[i]))};
        request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                             QNetworkRequest::NoLessSafeRedirectPolicy);
        request.setTransferTimeout(60 * 1000);
        QNetworkReply *reply = network_->get(request);
        connect(reply, &QNetworkReply::downloadProgress, reply,
                [reply](qint64 received, qint64) {
                    if (received > kMaxListBytes)
                        reply->abort();
                });
        connect(reply, &QNetworkReply::finished, this,
                [this, reply, i] { handleReply(reply, i); });
    }
}

void FilterListUpdater::handleReply(QNetworkReply *reply, int index)
{
    reply->deleteLater();
    const QByteArray data = reply->error() == QNetworkReply::NoError
                                ? reply->readAll()
                                : QByteArray();
    if (looksLikeFilterList(data))
        downloaded_[index] = data;
    else
        failed_ = true;

    if (--pending_ > 0)
        return;

    if (failed_) {
        emit updateFailed(reply->errorString());
        return;
    }

    QDir().mkpath(cacheDir());
    for (int i = 0; i < 2; ++i) {
        QSaveFile file(cacheDir() + '/' + QString::fromLatin1(kFileNames[i]));
        if (!file.open(QIODevice::WriteOnly) || file.write(downloaded_[i]) < 0 ||
            !file.commit()) {
            emit updateFailed(file.errorString());
            return;
        }
        downloaded_[i].clear();
    }

    QSettings settings("LiteWave", "LiteWave");
    settings.setValue("shield/filtersUpdatedAt", QDateTime::currentDateTimeUtc());
    loadCached(true);
}

void FilterListUpdater::loadCached(bool emitResult)
{
    const QString dir = cacheDir();
    QPointer<FilterListUpdater> self(this);
    QThreadPool::globalInstance()->start([dir, self, emitResult] {
        QByteArray lists[2];
        for (int i = 0; i < 2; ++i) {
            QFile file(dir + '/' + QString::fromLatin1(kFileNames[i]));
            if (file.size() <= kMaxListBytes && file.open(QIODevice::ReadOnly))
                lists[i] = file.readAll();
        }
        if (!emitResult && !looksLikeFilterList(lists[0]) && !looksLikeFilterList(lists[1]))
            return; // Nothing cached yet: keep the bundled snapshot.
        const int rules = lw_adblock_engine_load_lists(
            lists[0].isEmpty() ? nullptr : lists[0].constData(),
            lists[1].isEmpty() ? nullptr : lists[1].constData());
        QMetaObject::invokeMethod(qApp, [self, rules] {
            if (self)
                emit self->listsUpdated(rules);
        });
    });
}
