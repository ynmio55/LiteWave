#include "AdBlocker.h"

#include <QtTest>

class ShieldTests final : public QObject
{
    Q_OBJECT

private slots:
    void standardBlocksKnownThirdPartyAdvertising();
    void standardKeepsNavigationAndUnknownResources();
    void aggressiveAddsTrackerBlocking();
    void siteExceptionRestoresCompatibility();
};

void ShieldTests::standardBlocksKnownThirdPartyAdvertising()
{
    AdBlocker blocker(nullptr, false);
    blocker.setEnabled(true);
    blocker.setMode(AdBlocker::Mode::Standard);

    QVERIFY(blocker.shouldBlock(
        QUrl("https://securepubads.g.doubleclick.net/tag/js/gpt.js"),
        QUrl("https://news.example.com/story"),
        QWebEngineUrlRequestInfo::ResourceTypeScript));

    QVERIFY(blocker.shouldBlock(
        QUrl("https://www.youtube.com/api/stats/ads?event=ad"),
        QUrl("https://www.youtube.com/watch?v=example"),
        QWebEngineUrlRequestInfo::ResourceTypeXhr));
}

void ShieldTests::standardKeepsNavigationAndUnknownResources()
{
    AdBlocker blocker(nullptr, false);

    QVERIFY(!blocker.shouldBlock(
        QUrl("https://securepubads.g.doubleclick.net/"),
        QUrl("https://news.example.com/story"),
        QWebEngineUrlRequestInfo::ResourceTypeMainFrame));

    QVERIFY(!blocker.shouldBlock(
        QUrl("https://cdn.example.com/player.js"),
        QUrl("https://news.example.com/story"),
        QWebEngineUrlRequestInfo::ResourceTypeScript));

    QVERIFY(!blocker.shouldBlock(
        QUrl("https://www.google.com/recaptcha/api.js"),
        QUrl("https://news.example.com/login"),
        QWebEngineUrlRequestInfo::ResourceTypeScript));
}

void ShieldTests::aggressiveAddsTrackerBlocking()
{
    AdBlocker blocker(nullptr, false);
    blocker.setMode(AdBlocker::Mode::Standard);

    QVERIFY(!blocker.shouldBlock(
        QUrl("https://www.google-analytics.com/analytics.js"),
        QUrl("https://news.example.com/story"),
        QWebEngineUrlRequestInfo::ResourceTypeScript));

    blocker.setMode(AdBlocker::Mode::Aggressive);
    QVERIFY(blocker.shouldBlock(
        QUrl("https://www.google-analytics.com/analytics.js"),
        QUrl("https://news.example.com/story"),
        QWebEngineUrlRequestInfo::ResourceTypeScript));
}

void ShieldTests::siteExceptionRestoresCompatibility()
{
    AdBlocker blocker(nullptr, false);
    const QUrl site("https://video.example.co.th/watch/42");
    blocker.setSiteAllowed(site, true);

    QVERIFY(blocker.isSiteAllowed(QUrl("https://cdn.example.co.th/player")));
    QVERIFY(!blocker.shouldBlock(
        QUrl("https://securepubads.g.doubleclick.net/tag/js/gpt.js"),
        site, QWebEngineUrlRequestInfo::ResourceTypeScript));

    blocker.setSiteAllowed(site, false);
    QVERIFY(blocker.shouldBlock(
        QUrl("https://securepubads.g.doubleclick.net/tag/js/gpt.js"),
        site, QWebEngineUrlRequestInfo::ResourceTypeScript));
}

QTEST_APPLESS_MAIN(ShieldTests)
#include "ShieldTests.moc"
