#include "MainWindow.h"
#include "AdBlocker.h"
#include <QApplication>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>

class ShieldIntegration : public QObject {
    Q_OBJECT
private slots:
    void youtubeOnlyNativePolicy() {
        MainWindow window(nullptr, true);
        auto *shield = window.findChild<AdBlocker *>();
        QVERIFY(shield);

        const QUrl youtube("https://www.youtube.com/watch?v=integration-test");
        const QUrl otherSite("https://hubseriesshds.com/play/73017/");
        const QUrl knownAd("https://ad.doubleclick.net/litewave-shield-test.js");

        shield->setSiteAllowed(youtube, false);
        shield->setEnabled(true);

        QVERIFY(shield->isEnabledForUrl(youtube));
        QVERIFY(shield->shouldBlock(knownAd, youtube,
                                    QWebEngineUrlRequestInfo::ResourceTypeScript));

        // Shield must never intercept non-YouTube pages: compatibility first.
        QVERIFY(!shield->isEnabledForUrl(otherSite));
        QVERIFY(!shield->shouldBlock(knownAd, otherSite,
                                     QWebEngineUrlRequestInfo::ResourceTypeScript));
        QVERIFY(!shield->shouldBlockPopup(QUrl("https://example.net/"), otherSite, false));

        shield->setEnabled(false);
        QVERIFY(!shield->isEnabledForUrl(youtube));
        QVERIFY(!shield->shouldBlock(knownAd, youtube,
                                     QWebEngineUrlRequestInfo::ResourceTypeScript));

        shield->setEnabled(true);
        shield->setSiteAllowed(youtube, true);
        QVERIFY(!shield->isEnabledForUrl(youtube));
        QVERIFY(!shield->shouldBlock(knownAd, youtube,
                                     QWebEngineUrlRequestInfo::ResourceTypeScript));
        shield->setSiteAllowed(youtube, false);
    }
};

int main(int argc, char **argv) {
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QApplication app(argc, argv);
    QStandardPaths::setTestModeEnabled(true);
    QTemporaryDir settings;
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settings.path());
    ShieldIntegration test;
    return QTest::qExec(&test, argc, argv);
}

#include "ShieldIntegration.moc"
