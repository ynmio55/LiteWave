#include "MainWindow.h"
#include "AdBlocker.h"
#include <QApplication>
#include <QTest>
#include <QWebEngineView>
#include <QWebEnginePage>
#include <QWebEngineScript>
#include <QWebEngineProfile>
#include <QEventLoop>
#include <QTimer>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QSettings>
#include <QTemporaryDir>
#include <QPointer>
#include <memory>

static QVariant evaluate(QWebEnginePage *page, const QString &code) {
    auto result = std::make_shared<QVariant>();
    QEventLoop loop;
    QTimer deadline;
    deadline.setSingleShot(true);
    QObject::connect(&deadline, &QTimer::timeout, &loop, &QEventLoop::quit);
    QPointer<QEventLoop> guard(&loop);
    page->runJavaScript(code, [result, guard](const QVariant &r) {
        if (guard) { *result = r; guard->quit(); }
    });
    deadline.start(5000);
    loop.exec();
    return *result;
}

class ShieldIntegration : public QObject {
    Q_OBJECT
private slots:
    void realDomAndNetwork() {
        MainWindow window(nullptr, true);
        window.show();
        auto *shield = window.findChild<AdBlocker *>();
        QVERIFY(shield);
        shield->setEnabled(true);
        auto *view = window.findChild<QWebEngineView *>();
        QVERIFY(view);
        QSignalSpy loaded(view, &QWebEngineView::loadFinished);
        const QUrl base("https://fixture.example/");
        window.configurePageShield(view, base);
        view->setHtml(R"HTML(<!doctype html><title>Shield test</title>
            <div id="ad" class="adsbygoogle">advert</div>
            <div id="normal">Keep this content</div>
            <video id="v"></video>
            <script src="https://ad.doubleclick.net/litewave-shield-test.js"></script>
            )HTML", base);
        QVERIFY(loaded.wait(15000));
        window.configurePageShield(view, base, true);
        QTRY_VERIFY(evaluate(view->page(), "getComputedStyle(document.getElementById('ad')).display").toString() == "none");
        QCOMPARE(evaluate(view->page(), "getComputedStyle(document.getElementById('normal')).display").toString(), QString("block"));
        QVERIFY(shield->blockedCount() >= 1);
        QCOMPARE(evaluate(view->page(), "document.getElementById('v').playbackRate").toDouble(), 1.0);
        QVERIFY(!evaluate(view->page(), "document.getElementById('v').muted").toBool());
        // Native config changes stop cosmetic scripts in all windows sharing this profile.
        shield->setEnabled(false);
        QTRY_VERIFY(evaluate(view->page(), "getComputedStyle(document.getElementById('ad')).display").toString() == "block");
        shield->setEnabled(true);
        QTRY_VERIFY(evaluate(view->page(), "getComputedStyle(document.getElementById('ad')).display").toString() == "none");
        shield->setSiteAllowed(base, true);
        QTRY_VERIFY(evaluate(view->page(), "getComputedStyle(document.getElementById('ad')).display").toString() == "block");
        QVERIFY(!shield->shouldBlock(QUrl("https://ad.doubleclick.net/x"), base, QWebEngineUrlRequestInfo::ResourceTypeScript));
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
