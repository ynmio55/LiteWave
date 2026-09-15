#include "AdBlocker.h"
#include <QTest>
#include <QFile>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QSettings>
#include <memory>

using Info = QWebEngineUrlRequestInfo;

class ShieldTests : public QObject {
    Q_OBJECT
    QTemporaryDir settingsDir;
    std::unique_ptr<AdBlocker> shield;
    const QUrl site{"https://example.org/article"};
private slots:
    void initTestCase() {
        QVERIFY(settingsDir.isValid());
        QStandardPaths::setTestModeEnabled(true);
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsDir.path());
    }
    void init() {
        shield = std::make_unique<AdBlocker>(nullptr, false);
        shield->setEnabled(true);
    }
    void bundledList() {
        QFile file(":/shield/adaway-hosts.txt");
        QVERIFY(file.open(QIODevice::ReadOnly));
        const auto data = file.readAll();
        bool ok = false;
        const auto hosts = AdBlocker::parseHosts(data, &ok);
        QVERIFY(ok);
        QVERIFY(hosts.size() >= 6000);
        QVERIFY(AdBlocker::validUpdate(data));
        QVERIFY(shield->ruleCount() >= 6000);
    }
    void domainBoundaries() {
        QVERIFY(shield->isDomainBlocked("ad.doubleclick.net"));
        QVERIFY(shield->isDomainBlocked("AD.DOUBLECLICK.NET."));
        QVERIFY(!shield->isDomainBlocked("doubleclick.net.example.org"));
        QVERIFY(!shield->isDomainBlocked("notdoubleclick.net"));
        QVERIFY(shield->isDomainBlocked("analytics.163.com"));
        QVERIFY(!shield->isDomainBlocked("www.163.com"));
        QVERIFY(!shield->isDomainBlocked("www.pornhub.com"));
        QVERIFY(!shield->isDomainBlocked("www.youtube.com"));
        QVERIFY(!shield->isDomainBlocked("rr1.googlevideo.com"));
    }
    void resources_data() {
        QTest::addColumn<int>("type");
        for (const int type : {int(Info::ResourceTypeScript), int(Info::ResourceTypeImage),
             int(Info::ResourceTypeSubFrame), int(Info::ResourceTypeXhr),
             int(Info::ResourceTypeMedia), int(Info::ResourceTypeWebSocket),
             int(Info::ResourceTypePing)}) {
            QTest::newRow(qPrintable(QString::number(type))) << type;
        }
    }
    void resources() {
        QFETCH(int, type);
        QVERIFY(shield->shouldBlock(QUrl("https://ad.doubleclick.net/pixel"), site, Info::ResourceType(type)));
    }
    void navigationIsNotBlanked() {
        QVERIFY(!shield->shouldBlock(QUrl("https://ad.doubleclick.net"), site, Info::ResourceTypeMainFrame));
        QVERIFY(!shield->shouldBlock(QUrl("data:text/html,hello"), site, Info::ResourceTypeSubFrame));
        QVERIFY(!shield->shouldBlock(QUrl("file:///tmp/ads.html"), site, Info::ResourceTypeSubFrame));
    }
    void scopedPathsAndQueries() {
        QVERIFY(shield->shouldBlock(QUrl("https://www.youtube.com/api/stats/ads?x=1"), site, Info::ResourceTypeXhr));
        QVERIFY(!shield->shouldBlock(QUrl("https://example.org/api/stats/ads"), site, Info::ResourceTypeXhr));
        QVERIFY(!shield->shouldBlock(QUrl("https://example.org/article?q=/pagead/js/"), site, Info::ResourceTypeXhr));
        QVERIFY(shield->shouldBlock(QUrl("https://ad.doubleclick.net/a?recaptcha=true"), site, Info::ResourceTypeScript));
        QVERIFY(!shield->shouldBlock(QUrl("https://www.google.com/recaptcha/api.js"), site, Info::ResourceTypeScript));
        QVERIFY(!shield->shouldBlock(QUrl("https://challenges.cloudflare.com/turnstile/v0/api.js"), site, Info::ResourceTypeScript));
        QVERIFY(!shield->shouldBlock(QUrl("https://js.hcaptcha.com/1/api.js"), site, Info::ResourceTypeScript));
    }
    void allToggleAndSiteExceptions() {
        const QUrl ad("https://ad.doubleclick.net/script");
        shield->setEnabled(false);
        QVERIFY(!shield->shouldBlock(ad, site, Info::ResourceTypeScript));
        shield->setEnabled(true);
        shield->setSiteAllowed(site, true);
        QVERIFY(!shield->isEnabledForUrl(site));
        QVERIFY(!shield->shouldBlock(ad, site, Info::ResourceTypeScript));
        QVERIFY(shield->shouldBlock(ad, QUrl("https://example.org.evil.test/"), Info::ResourceTypeScript));
        shield->setSiteAllowed(site, false);
        QVERIFY(shield->shouldBlock(ad, site, Info::ResourceTypeScript));
    }
    void popupPolicy() {
        // Standard permits benign login/payment popups instead of blocking every
        // window.open() call. Known ad domains remain blocked.
        QVERIFY(!shield->shouldBlockPopup(QUrl("https://example.net/"), site, false));
        QVERIFY(shield->shouldBlockPopup(QUrl("https://popads.net/"), site, true));
        QVERIFY(!shield->shouldBlockPopup(QUrl("https://accounts.google.com/"), site, true));
        shield->setAggressive(true);
        QVERIFY(shield->shouldBlockPopup(QUrl("https://example.net/"), site, false));
        shield->setSiteAllowed(site, true);
        QVERIFY(!shield->shouldBlockPopup(QUrl("https://example.net/"), site, false));
    }
    void standardAndAggressiveModes() {
        const QUrl app("https://shop.example/");
        const QUrl tagManager("https://www.googletagmanager.com/gtm.js");
        QVERIFY(!shield->isAggressive());
        QVERIFY(!shield->shouldBlock(tagManager, app, Info::ResourceTypeScript));
        shield->setAggressive(true);
        QVERIFY(shield->shouldBlock(tagManager, app, Info::ResourceTypeScript));
        // Explicit YouTube ad endpoints stay blocked even in compatibility mode.
        shield->setAggressive(false);
        QVERIFY(shield->shouldBlock(
            QUrl("https://www.youtube.com/pagead/adview"), app,
            Info::ResourceTypeXhr));
    }
    void parserAndUpdateSafety() {
        bool ok = false;
        auto hosts = AdBlocker::parseHosts("# comment\n0.0.0.0 ads.example\n127.0.0.1 ads.example # duplicate\n::1 localhost\n", &ok);
        QVERIFY(ok);
        QCOMPARE(hosts.size(), 1);
        for (const auto &bad : {QByteArray("<html>network error</html>"),
             QByteArray("0.0.0.0 bad_domain.test"), QByteArray("0.0.0.0 test.*"),
             QByteArray("0.0.0.0 https://ads.example"),
             QByteArray("127.0.0.1 good.example\nscript.inject()"),
             QByteArray("||ads.example^$script")}) {
            ok = true;
            QVERIFY(AdBlocker::parseHosts(bad, &ok).isEmpty());
            QVERIFY(!ok);
            QVERIFY(!AdBlocker::validUpdate(bad));
        }
        QVERIFY(!AdBlocker::validUpdate("# AdAway default blocklist\n127.0.0.1 tiny.example\n"));
        QVERIFY(!AdBlocker::validUpdate(QByteArray(4 * 1024 * 1024 + 1, 'a')));
    }
    void privateSettingsStayPrivate() {
        QSettings before("LiteWave", "LiteWave");
        const auto keys = before.allKeys();
        shield->setSiteAllowed(QUrl("https://private.example/"), true);
        shield->setEnabled(false);
        QSettings after("LiteWave", "LiteWave");
        QCOMPARE(after.allKeys(), keys);
        AdBlocker anotherPrivate(nullptr, false);
        QVERIFY(!anotherPrivate.isSiteAllowed(QUrl("https://private.example/")));
    }
    void persistentSettings() {
        {
            AdBlocker normal(nullptr, true);
            normal.setSiteAllowed(QUrl("https://saved.example"), true);
            normal.setEnabled(false);
        }
        AdBlocker reopened(nullptr, true);
        QVERIFY(reopened.isSiteAllowed(QUrl("https://saved.example")));
        QVERIFY(!reopened.isEnabled());
        QSettings("LiteWave", "LiteWave").clear();
    }
};
QTEST_GUILESS_MAIN(ShieldTests)
#include "AdBlockerTests.moc"
