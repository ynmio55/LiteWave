#include "AdBlocker.h"
#include "SearchEngineManager.h"

#include <QtTest>

class ShieldTests final : public QObject
{
    Q_OBJECT

private slots:
    void standardBlocksKnownThirdPartyAdvertising();
    void standardKeepsNavigationAndUnknownResources();
    void aggressiveAddsTrackerBlocking();
    void siteExceptionRestoresCompatibility();
    void popupBlocksAreCounted();
    void bundledEasyListBlocksAdServer();
    void searchEngineManagerBuildsCorrectUrls();
    void searchEngineManagerSupportsEnginesAndFallback();
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
        QUrl("https://www.google.com/pagead/1p-user-list/12345/"),
        QUrl("https://news.example.com/story"),
        QWebEngineUrlRequestInfo::ResourceTypeScript));
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

void ShieldTests::bundledEasyListBlocksAdServer()
{
    AdBlocker blocker(nullptr, false);
    blocker.setEnabled(true);
    QVERIFY(blocker.shouldBlock(
        QUrl("https://000491b06a.com/ad.js"),
        QUrl("https://news.example.com/article"),
        QWebEngineUrlRequestInfo::ResourceTypeScript));
}

void ShieldTests::popupBlocksAreCounted()
{
    AdBlocker blocker(nullptr, false);
    QCOMPARE(blocker.blockedCount(), 0);
    blocker.recordBlockedPopup();
    QCOMPARE(blocker.blockedCount(), 1);
    blocker.resetBlockedCount();
    QCOMPARE(blocker.blockedCount(), 0);
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

void ShieldTests::searchEngineManagerBuildsCorrectUrls()
{
    SearchEngineManager &mgr = SearchEngineManager::instance();
    mgr.setCurrentEngineId("google");
    QUrl url = mgr.buildSearchUrl("litewave browser");
    QCOMPARE(url.toString(QUrl::FullyEncoded), QString("https://www.google.com/search?q=litewave%20browser"));

    mgr.setCurrentEngineId("duckduckgo");
    QUrl ddg = mgr.buildSearchUrl("privacy test");
    QCOMPARE(ddg.toString(QUrl::FullyEncoded), QString("https://duckduckgo.com/?q=privacy%20test"));

    mgr.setCurrentEngineId("brave");
    QUrl brave = mgr.buildSearchUrl("speed");
    QCOMPARE(brave.toString(QUrl::FullyEncoded), QString("https://search.brave.com/search?q=speed"));

    mgr.setCurrentEngineId("bing");
    QUrl bing = mgr.buildSearchUrl("hello world");
    QCOMPARE(bing.toString(QUrl::FullyEncoded), QString("https://www.bing.com/search?q=hello%20world"));
}

void ShieldTests::searchEngineManagerSupportsEnginesAndFallback()
{
    SearchEngineManager &mgr = SearchEngineManager::instance();
    const auto engines = mgr.availableEngines();
    QVERIFY(engines.size() >= 4);

    mgr.setCurrentEngineId("non_existent_engine_id");
    QUrl fallback = mgr.buildSearchUrl("query");
    QVERIFY(fallback.isValid());
    QVERIFY(!fallback.isEmpty());
}

QTEST_APPLESS_MAIN(ShieldTests)
#include "ShieldTests.moc"
