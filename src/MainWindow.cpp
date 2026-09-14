#include "MainWindow.h"
#include "AdBlocker.h"

#include <QAction>
#include <QApplication>
#include <QIcon>
#include <QShortcut>
#include <QInputDialog>
#include <QFileDialog>
#include <QPrinter>
#include <QWebEngineDownloadRequest>
#include <QPrintDialog>
#include <QMessageBox>
#include <QUuid>
#include <algorithm>
#include <QTabBar>
#include <QToolBar>
#include <QWebEngineFullScreenRequest>
#include <QWebEngineNewWindowRequest>
#include <QJsonArray>
#include <QJsonDocument>
#include <QPointer>
#include <QWebEnginePermission>
#include <QWebEngineCertificateError>
#include <QWebEnginePage>
#include <QUrlQuery>
#include <QFileInfo>
#include <QWebEngineSettings>
#include <QLineEdit>
#include <QProgressBar>
#include <QStandardPaths>
#include <QRegularExpression>
#include <QSettings>
#include <QStatusBar>
#include <QTabWidget>
#include <QToolButton>
#include <QLabel>
#include <QWebEngineProfile>
#include <QWebEngineView>
#include <QWebEngineScript>
#include <QWebEngineScriptCollection>

class WebPage : public QWebEnginePage {
public:
    WebPage(QWebEngineProfile *profile, MainWindow *mw, QWebEngineView *view, QObject *parent = nullptr)
        : QWebEnginePage(profile, parent), mw_(mw), view_(view) {}

protected:
    QWebEnginePage *createWindow(WebWindowType type) override {
        Q_UNUSED(type);
        if (mw_) {
            auto *newView = mw_->createView(QUrl());
            return newView->page();
        }
        return nullptr;
    }

    bool acceptNavigationRequest(const QUrl &url, NavigationType type, bool isMainFrame) override {
        if (url.scheme() == "litewave") {
            if (isMainFrame && mw_ && view_) {
                MainWindow *mw = mw_;
                QPointer<QWebEngineView> v = view_;
                QMetaObject::invokeMethod(mw, [mw, v, url]{
                    if (v) mw->handleHomeNavigation(url, v);
                }, Qt::QueuedConnection);
            }
            return false;
        }
        if (url.host() == "litewave.home") {
            if (isMainFrame && mw_ && view_) {
                MainWindow *mw = mw_;
                QPointer<QWebEngineView> v = view_;
                QMetaObject::invokeMethod(mw, [mw, v]{
                    if (v) mw->loadHome(v);
                }, Qt::QueuedConnection);
            }
            return false;
        }
        return QWebEnginePage::acceptNavigationRequest(url, type, isMainFrame);
    }

private:
    MainWindow *mw_ = nullptr;
    QWebEngineView *view_ = nullptr;
};

MainWindow::MainWindow(QWidget *parent, bool privateMode)
    : QMainWindow(parent),
      urlBar_(new QLineEdit(this)),
      tabs_(new QTabWidget(this)),
      progress_(new QProgressBar(this)),
      toolbar_(nullptr),
      shieldAction_(nullptr),
      adBlocker_(nullptr)
{
    setWindowTitle("LiteWave");
    setWindowIcon(QIcon(":/icons/litewave.svg"));
    resize(1320, 840);

    privateMode_ = privateMode;
    QSettings themeSettings("LiteWave", "LiteWave");
    darkMode_ = themeSettings.value("theme/dark", false).toBool();
    const QString storagePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/Storage";
    const QString cachePath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/Cache";

    static QWebEngineProfile *normalProfile = new QWebEngineProfile("LiteWave", qApp);
    profile_ = privateMode_ ? new QWebEngineProfile(this) : normalProfile;
    // Keep the engine's real version and platform for capability detection.
    profile_->setHttpAcceptLanguage("th-TH,th;q=0.9,en-US;q=0.8,en;q=0.7");
    if (!privateMode_) {
        profile_->setPersistentStoragePath(storagePath);
        profile_->setCachePath(cachePath);
        profile_->setPersistentCookiesPolicy(QWebEngineProfile::AllowPersistentCookies);
        profile_->setHttpCacheType(QWebEngineProfile::DiskHttpCache);
    }

    // Register cosmetic CSS & YouTube ad auto-skip script
    setupUserScripts();

    // Profile request interceptor setup
    adBlocker_ = static_cast<AdBlocker *>(profile_->findChild<QObject *>("shield"));
    if (!adBlocker_) {
        adBlocker_ = new AdBlocker(profile_);
        adBlocker_->setObjectName("shield");
        profile_->setUrlRequestInterceptor(adBlocker_);
    }

    auto *toolbar = addToolBar("LiteWave");
    toolbar_ = toolbar;
    toolbar->setMovable(false);
    toolbar->setToolButtonStyle(Qt::ToolButtonTextOnly);

    auto addButton = [toolbar](const QString &text) {
        return toolbar->addAction(text);
    };

    auto *back = addButton("◀");
    back->setToolTip("ย้อนกลับ");
    connect(back, &QAction::triggered, this, [this] { if (currentView()) currentView()->back(); });

    auto *forward = addButton("▶");
    forward->setToolTip("ถัดไป");
    connect(forward, &QAction::triggered, this, [this] { if (currentView()) currentView()->forward(); });

    auto *reload = addButton("↻");
    reload->setToolTip("รีเฟรชหน้าเว็บ");
    connect(reload, &QAction::triggered, this, [this] { reloadCurrentView(); });

    auto *home = addButton("หน้าแรก");
    home->setToolTip("ไปยังหน้าแรก");
    connect(home, &QAction::triggered, this, [this] { if (currentView()) loadHome(currentView()); });

    auto *plus = addButton("+ แท็บใหม่");
    plus->setToolTip("เปิดแท็บใหม่ (Ctrl+T)");
    connect(plus, &QAction::triggered, this, &MainWindow::newTab);

    // SSL security status label
    sslLabel_ = new QLabel("🔒 ", this);
    sslLabel_->setStyleSheet("padding-left: 8px; font-size: 14px;");
    toolbar->addWidget(sslLabel_);

    urlBar_->setPlaceholderText("ค้นหา หรือระบุ URL เว็บไซต์...");
    urlBar_->setClearButtonEnabled(true);
    toolbar->addWidget(urlBar_);
    connect(urlBar_, &QLineEdit::returnPressed, this, &MainWindow::navigate);

    auto *bookmark = addButton("★");
    bookmark->setToolTip("บันทึกหน้าเว็บนี้ (Ctrl+D)");
    connect(bookmark, &QAction::triggered, this, &MainWindow::addBookmark);

    shieldAction_ = addButton("Shield: เปิด (0)");
    shieldAction_->setCheckable(true);
    shieldAction_->setChecked(true);
    connect(shieldAction_, &QAction::toggled, this, &MainWindow::toggleShield);
    connect(adBlocker_, &AdBlocker::countChanged, this, &MainWindow::updateShieldBadge);

    themeAction_ = addButton(darkMode_ ? "โหมดสว่าง" : "โหมดมืด");
    themeAction_->setToolTip("สลับโหมดมืด/สว่าง");
    connect(themeAction_, &QAction::triggered, this, &MainWindow::toggleTheme);

    auto *mediaCheck = addButton("ตรวจวิดีโอ");
    mediaCheck->setToolTip("ตรวจ codec ที่เครื่องนี้รองรับ โดยไม่เปิดเว็บไซต์ภายนอก");
    connect(mediaCheck, &QAction::triggered, this, [this] {
        if (!currentView()) return;
        QPointer<MainWindow> guard(this);
        currentView()->page()->runJavaScript(QStringLiteral(R"JS(
            (() => {
                const v = document.createElement('video');
                const types = [
                    ['H.264 / AAC (MP4)', 'video/mp4; codecs="avc1.42E01E, mp4a.40.2"'],
                    ['VP9 / Opus (WebM)', 'video/webm; codecs="vp9, opus"'],
                    ['AV1 (MP4)', 'video/mp4; codecs="av01.0.05M.08"']
                ];
                return types.map(([name, mime]) => name + ': ' +
                    (v.canPlayType(mime) || 'not supported')).join('\n') +
                    '\nMediaSource: ' + (typeof MediaSource !== 'undefined') +
                    '\nThis checks advertised support, not actual playback or DRM.';
            })()
        )JS"), [guard](const QVariant &result) {
            if (guard) QMessageBox::information(guard, "Video capability", result.toString());
        });
    });

    progress_->setMaximumWidth(120);
    progress_->setTextVisible(false);
    progress_->setRange(0, 100);
    toolbar->addWidget(progress_);

    tabs_->setTabsClosable(true);
    tabs_->setDocumentMode(true);
    tabs_->setMovable(true);
    tabs_->setTabBarAutoHide(true);
    setCentralWidget(tabs_);

    connect(tabs_, &QTabWidget::currentChanged, this, [this](int) {
        if (currentView()) {
            updateCurrentUrl(currentView()->url());
            setWindowTitle(currentView()->title() + (privateMode_ ? " — Private · LiteWave" : " — LiteWave"));
        }
    });
    connect(tabs_, &QTabWidget::tabCloseRequested, this, &MainWindow::closeTab);

    setupShortcuts();
    applyTheme();
    newTab();
}


QWebEngineView *MainWindow::currentView() const
{
    return qobject_cast<QWebEngineView *>(tabs_->currentWidget());
}

QWebEngineView *MainWindow::createView(const QUrl &url)
{
    auto *view = new QWebEngineView(tabs_);
    auto *page = new WebPage(profile_, this, view, view);
    view->setPage(page);
    auto *s = view->settings();
    s->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
    s->setAttribute(QWebEngineSettings::LocalStorageEnabled, true);
    s->setAttribute(QWebEngineSettings::PluginsEnabled, true);
    s->setAttribute(QWebEngineSettings::DnsPrefetchEnabled, true);
    s->setAttribute(QWebEngineSettings::WebGLEnabled, true);
    s->setAttribute(QWebEngineSettings::Accelerated2dCanvasEnabled, true);
    s->setAttribute(QWebEngineSettings::AutoLoadImages, true);
    s->setAttribute(QWebEngineSettings::JavascriptCanAccessClipboard, false);
    s->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows, true);
    s->setAttribute(QWebEngineSettings::AllowWindowActivationFromJavaScript, false);
    s->setAttribute(QWebEngineSettings::ScreenCaptureEnabled, true);
    s->setAttribute(QWebEngineSettings::PdfViewerEnabled, true);
    s->setAttribute(QWebEngineSettings::AllowRunningInsecureContent, false);
    s->setAttribute(QWebEngineSettings::FullScreenSupportEnabled, true);
    s->setAttribute(QWebEngineSettings::PlaybackRequiresUserGesture, false);
    const int index = tabs_->addTab(view, "LiteWave");
    tabs_->setCurrentIndex(index);
    tabs_->tabBar()->setVisible(tabs_->count() > 1);

    connect(view->page(), &QWebEnginePage::fullScreenRequested, this,
            [this](QWebEngineFullScreenRequest request) {
                request.accept();
                if (request.toggleOn()) {
                    toolbar_->hide();
                    tabs_->tabBar()->hide();
                    statusBar()->hide();
                    showFullScreen();
                } else {
                    showNormal();
                    toolbar_->show();
                    statusBar()->show();
                    tabs_->tabBar()->setVisible(tabs_->count() > 1);
                }
            });

    connect(page, &QWebEnginePage::newWindowRequested, this,
            [this](QWebEngineNewWindowRequest &request) {
                auto *newView = createView(QUrl());
                request.openIn(newView->page());
            });

    // Compatible with Qt 6.4+; never grant sensitive permissions silently.
    connect(page, &QWebEnginePage::featurePermissionRequested, this,
            [this, page](const QUrl &origin, QWebEnginePage::Feature feature) {
                QString capability;
                switch (feature) {
                case QWebEnginePage::MediaAudioCapture: capability = "microphone"; break;
                case QWebEnginePage::MediaVideoCapture: capability = "camera"; break;
                case QWebEnginePage::MediaAudioVideoCapture: capability = "camera and microphone"; break;
                case QWebEnginePage::Geolocation: capability = "location"; break;
                case QWebEnginePage::Notifications: capability = "notifications"; break;
                default:
                    page->setFeaturePermission(origin, feature, QWebEnginePage::PermissionDeniedByUser);
                    return;
                }
                QPointer<QWebEnginePage> guard(page);
                const auto answer = QMessageBox::question(this, "Site permission",
                    origin.toDisplayString() + "\nAllow access to " + capability + "?",
                    QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
                if (guard)
                    guard->setFeaturePermission(origin, feature,
                        answer == QMessageBox::Yes ? QWebEnginePage::PermissionGrantedByUser
                                                   : QWebEnginePage::PermissionDeniedByUser);
            });

    connect(page, &QWebEnginePage::certificateError, this,
            [](QWebEngineCertificateError certError) {
                certError.acceptCertificate();
            });

    connect(profile_, &QWebEngineProfile::downloadRequested, view,
            [this, page](QWebEngineDownloadRequest *download) {
                // A shared profile emits to every tab: only the originating page handles it.
                if (!download || download->page() != page ||
                    download->state() != QWebEngineDownloadRequest::DownloadRequested) return;
                if (download->isSavePageDownload()) {
                    download->accept(); // Keep the filename selected by Ctrl+S.
                    return;
                }
                const QString dir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
                const QString name = download->downloadFileName().isEmpty()
                    ? QStringLiteral("LiteWave-download") : download->downloadFileName();
                download->setDownloadDirectory(dir);
                download->setDownloadFileName(name);
                connect(download, &QWebEngineDownloadRequest::stateChanged, this,
                        [this, download](QWebEngineDownloadRequest::DownloadState state) {
                            if (state == QWebEngineDownloadRequest::DownloadCompleted) {
                                statusBar()->showMessage("ดาวน์โหลดเสร็จแล้ว: " + download->downloadFileName(), 5000);
                            } else if (state == QWebEngineDownloadRequest::DownloadCancelled ||
                                       state == QWebEngineDownloadRequest::DownloadInterrupted) {
                                statusBar()->showMessage("ดาวน์โหลดไม่สำเร็จ", 5000);
                            }
                        });
                download->accept();
                statusBar()->showMessage("กำลังดาวน์โหลด: " + name, 2500);
            });

    connect(view, &QWebEngineView::urlChanged, this, &MainWindow::updateCurrentUrl);
    connect(view, &QWebEngineView::titleChanged, this, &MainWindow::updateTabTitle);
    connect(view, &QWebEngineView::loadProgress, progress_, &QProgressBar::setValue);
    if (url.isValid() && !url.isEmpty())
        view->setUrl(url);
    return view;
}

void MainWindow::newTab()
{
    auto *view = createView(QUrl());
    loadHome(view);
}

void MainWindow::closeTab(int index)
{
    auto *view = qobject_cast<QWebEngineView *>(tabs_->widget(index));
    if (!view) return;
    closedTabs_.append(view->url());
    if (closedTabs_.size() > 20) closedTabs_.removeFirst();
    tabs_->removeTab(index);
    delete view;
    if (!tabs_->count()) newTab();
    tabs_->tabBar()->setVisible(!isFullScreen() && tabs_->count() > 1);
}

MainWindow::~MainWindow()
{
    delete takeCentralWidget();
}

void MainWindow::setupShortcuts()
{
    auto key = [this](const QString &sequence, auto fn) {
        auto *shortcut = new QShortcut(QKeySequence(sequence), this);
        connect(shortcut, &QShortcut::activated, this, fn);
    };
    key("Ctrl+T", [this]{ newTab(); });
    key("Ctrl+N", [this]{ auto *w = new MainWindow(nullptr, privateMode_); w->setAttribute(Qt::WA_DeleteOnClose); w->show(); });
    key("Ctrl+Shift+N", []{ auto *w = new MainWindow(nullptr, true); w->setAttribute(Qt::WA_DeleteOnClose); w->show(); });
    key("Ctrl+W", [this]{ closeTab(tabs_->currentIndex()); });
    key("Ctrl+Shift+T", [this]{
        if (closedTabs_.isEmpty()) return;
        const auto u = closedTabs_.takeLast();
        if (u.host() == "litewave.home" || u.isEmpty()) newTab(); else createView(u);
    });
    key("Ctrl+Tab", [this]{ tabs_->setCurrentIndex((tabs_->currentIndex()+1)%tabs_->count()); });
    key("Ctrl+Shift+Tab", [this]{ tabs_->setCurrentIndex((tabs_->currentIndex()+tabs_->count()-1)%tabs_->count()); });
    for (int i=1;i<=8;++i) key("Ctrl+"+QString::number(i), [this,i]{ if(i<=tabs_->count()) tabs_->setCurrentIndex(i-1); });
    key("Ctrl+9", [this]{ tabs_->setCurrentIndex(tabs_->count()-1); });
    key("Ctrl+L", [this]{ urlBar_->setFocus(); urlBar_->selectAll(); });
    auto reload = [this]{ reloadCurrentView(); };
    key("Ctrl+R",reload); key("F5",reload);
    auto hard = [this]{ reloadCurrentView(); };
    key("Ctrl+Shift+R",hard); key("Shift+F5",hard);
    key("Ctrl+D", [this]{ addBookmark(); });
    key("Ctrl+Shift+D", [this]{
        bool ok=false;
        QString name=QInputDialog::getText(this,"Bookmarks","Folder name",QLineEdit::Normal,"Saved tabs",&ok);
        if(!ok || name.trimmed().isEmpty()) return;
        QSettings st("LiteWave","LiteWave");
        const QString group="folders/"+QUuid::createUuid().toString(QUuid::WithoutBraces);
        st.setValue(group+"/name",name);
        QStringList urls;
        for(int i=0;i<tabs_->count();++i) urls << qobject_cast<QWebEngineView *>(tabs_->widget(i))->url().toString();
        st.setValue(group+"/urls",urls);
        statusBar()->showMessage("Saved "+QString::number(urls.size())+" tabs",3000);
    });
    key("Ctrl+F", [this]{
        bool ok=false;
        const QString q=QInputDialog::getText(this,"Find in page","Text",QLineEdit::Normal,findQuery_,&ok);
        if(ok && currentView()) { findQuery_=q; currentView()->findText(q); }
    });
    key("Ctrl+G", [this]{ if(currentView()) currentView()->findText(findQuery_); });
    key("Ctrl+Shift+G", [this]{ if(currentView()) currentView()->findText(findQuery_,QWebEnginePage::FindBackward); });
    auto zoomIn=[this]{ if(currentView()) currentView()->setZoomFactor(std::min(5.0,currentView()->zoomFactor()+0.1)); };
    key("Ctrl++",zoomIn); key("Ctrl+=",zoomIn);
    key("Ctrl+-", [this]{ if(currentView()) currentView()->setZoomFactor(std::max(0.25,currentView()->zoomFactor()-0.1)); });
    key("Ctrl+0", [this]{ if(currentView()) currentView()->setZoomFactor(1.0); });
    key("Ctrl+S", [this]{
        if(!currentView()) return;
        const QString path=QFileDialog::getSaveFileName(this,"Save page",QString(),"Web archive (*.mhtml)");
        if(!path.isEmpty()) currentView()->page()->save(path,QWebEngineDownloadRequest::MimeHtmlSaveFormat);
    });
    key("Ctrl+P", [this]{
        auto *view=currentView(); if(!view) return;
        auto *printer=new QPrinter(QPrinter::HighResolution);
        QPrintDialog dialog(printer,this);
        if(dialog.exec()!=QDialog::Accepted) {delete printer;return;}
        connect(view,&QWebEngineView::printFinished,view,[printer](bool){delete printer;},Qt::SingleShotConnection);
        view->print(printer);
    });
    key("F11", [this]{
        if(isFullScreen()) {
            if(currentView()) currentView()->triggerPageAction(QWebEnginePage::ExitFullScreen);
            showNormal(); toolbar_->show(); statusBar()->show(); tabs_->tabBar()->setVisible(tabs_->count()>1);
        } else { toolbar_->hide(); tabs_->tabBar()->hide(); statusBar()->hide(); showFullScreen(); }
    });
    key("Escape", [this]{
        if(!isFullScreen()) return;
        if(currentView()) currentView()->triggerPageAction(QWebEnginePage::ExitFullScreen);
        showNormal(); toolbar_->show(); statusBar()->show(); tabs_->tabBar()->setVisible(tabs_->count()>1);
    });
}

void MainWindow::handleHomeNavigation(const QUrl &url, QWebEngineView *view)
{
    if (!view) return;

    if (url.scheme() != "litewave") {
        view->setUrl(url);
        return;
    }

    const QUrlQuery query(url);
    const QString action = url.host().toLower();
    if (action == "search") {
        const QString text = query.queryItemValue("q").trimmed();
        const QString engine = query.queryItemValue("engine").toLower();
        if (text.isEmpty()) {
            loadHome(view);
        } else if (engine == "brave") {
            view->setUrl(QUrl("https://search.brave.com/search?q=" + QUrl::toPercentEncoding(text)));
        } else if (engine == "duckduckgo") {
            view->setUrl(QUrl("https://duckduckgo.com/?q=" + QUrl::toPercentEncoding(text)));
        } else {
            openUrl("g " + text);
        }
        return;
    }

    loadHome(view);
}

void MainWindow::reloadCurrentView()
{
    auto *view = currentView();
    if (!view) return;
    if (view->url().host() == "litewave.home" || view->url().isEmpty() || view->url().toString().contains("litewave.home")) {
        loadHome(view);
    } else {
        view->reload();
    }
}

void MainWindow::navigate()
{
    openUrl(urlBar_->text());
}

void MainWindow::openUrl(const QString &text)
{
    const QString input = text.trimmed();
    if (input.isEmpty() || !currentView()) return;

    if (input.contains("litewave.home") || input.startsWith("litewave:")) {
        loadHome(currentView());
        return;
    }

    const QRegularExpression urlPattern(
        R"(^(https?://|localhost(?::\d+)?(?:/|$)|(?:[a-zA-Z0-9-]+\.)+[a-zA-Z]{2,}(?::\d+)?(?:/|$)))",
        QRegularExpression::CaseInsensitiveOption);

    QUrl url;
    if (urlPattern.match(input).hasMatch()) {
        const QString address = input.contains("://") ? input : "https://" + input;
        url = QUrl(address);
        if (!url.isValid()) return;
        currentView()->setUrl(url);
        return;
    }

    // Allow prefix "g " or "google " to force search on Google Search
    if (input.startsWith("g ") || input.startsWith("google ")) {
        const QString query = input.section(' ', 1).trimmed();
        if (!query.isEmpty()) {
            url = QUrl("https://www.google.com/search?q=" + QUrl::toPercentEncoding(query));
            currentView()->setUrl(url);
            return;
        }
    }

    // Default search engine: Google. URLs and normal words are handled separately above.
    url = QUrl("https://www.google.com/search?q=" + QUrl::toPercentEncoding(input));
    currentView()->setUrl(url);
}

void MainWindow::setupUserScripts()
{
    if (!profile_) return;

    if (profile_->scripts()->find("LiteWaveCosmeticCss").isEmpty()) {
        QWebEngineScript cssScript;
        cssScript.setName("LiteWaveCosmeticCss");
        const QString cssJson = QString::fromUtf8(
            QJsonDocument(QJsonArray{AdBlocker::cosmeticCss()}).toJson(QJsonDocument::Compact));
        cssScript.setSourceCode("(() => { const s = document.createElement('style'); "
            "s.id = 'litewave-ad-style'; s.textContent = " + cssJson +
            "[0]; (document.head || document.documentElement).appendChild(s); })();");
        cssScript.setInjectionPoint(QWebEngineScript::DocumentReady);
        cssScript.setWorldId(QWebEngineScript::ApplicationWorld);
        cssScript.setRunsOnSubFrames(true);
        profile_->scripts()->insert(cssScript);
    }

    if (profile_->scripts()->find("LiteWaveCosmeticJs").isEmpty()) {
        QWebEngineScript jsScript;
        jsScript.setName("LiteWaveCosmeticJs");
        jsScript.setSourceCode(AdBlocker::cosmeticJs());
        jsScript.setInjectionPoint(QWebEngineScript::DocumentReady);
        jsScript.setWorldId(QWebEngineScript::ApplicationWorld);
        jsScript.setRunsOnSubFrames(true);
        profile_->scripts()->insert(jsScript);
    }
}

void MainWindow::updateShieldBadge(int count)
{
    if (shieldAction_) {
        const QString state = adBlocker_->isEnabled()
            ? "Shield: เปิด (" + QString::number(count) + ")"
            : "Shield: ปิด";
        shieldAction_->setText(state);
    }
}

void MainWindow::updateCurrentUrl(const QUrl &url)
{
    auto *view = qobject_cast<QWebEngineView *>(sender());
    if (!view || view != currentView()) return;
    urlBar_->setText(url.toString());
    urlBar_->setCursorPosition(0);

    if (sslLabel_) {
        if (url.scheme() == "https") {
            sslLabel_->setText("🔒 ");
            sslLabel_->setToolTip("การเชื่อมต่อปลอดภัย (HTTPS)");
        } else if (url.scheme() == "http") {
            sslLabel_->setText("⚠️ ");
            sslLabel_->setToolTip("การเชื่อมต่อไม่ปลอดภัย (HTTP)");
        } else {
            sslLabel_->setText("🌐 ");
            sslLabel_->setToolTip("LiteWave Dashboard");
        }
    }
}

void MainWindow::updateTabTitle(const QString &title)
{
    auto *view = qobject_cast<QWebEngineView *>(sender());
    if (!view) return;
    const int index = tabs_->indexOf(view);
    if (index >= 0) tabs_->setTabText(index, title.isEmpty() ? "LiteWave" : title.left(24));
    if (view == currentView())
        setWindowTitle((title.isEmpty() ? "LiteWave" : title) + (privateMode_ ? " — Private · LiteWave" : " — LiteWave"));
}

void MainWindow::addBookmark()
{
    if (!currentView() || currentView()->url().isEmpty()) return;
    const QString title = currentView()->title().isEmpty() ? currentView()->url().host() : currentView()->title();
    QSettings settings("LiteWave", "LiteWave");
    settings.setValue("bookmarks/" + currentView()->url().toString(), title);
    statusBar()->showMessage("บันทึกเว็บแล้ว: " + title, 3000);
}

void MainWindow::toggleShield(bool enabled)
{
    adBlocker_->setEnabled(enabled);
    updateShieldBadge(adBlocker_->blockedCount());
    statusBar()->showMessage(enabled ? "เปิดการบล็อกโฆษณาแล้ว" : "ปิดการบล็อกโฆษณาแล้ว", 2500);
    if (currentView()) currentView()->reload();
}

void MainWindow::toggleTheme()
{
    darkMode_ = !darkMode_;
    QSettings themeSettings("LiteWave", "LiteWave");
    themeSettings.setValue("theme/dark", darkMode_);
    applyTheme();

    // Refresh the built-in home page immediately so its colors match the browser chrome.
    if (currentView() && currentView()->url().host() == "litewave.home") {
        loadHome(currentView());
    }
}

void MainWindow::applyTheme()
{
    if (themeAction_) {
        themeAction_->setText(darkMode_ ? "โหมดสว่าง" : "โหมดมืด");
        themeAction_->setToolTip(darkMode_ ? "เปลี่ยนเป็นโหมดสว่าง" : "เปลี่ยนเป็นโหมดมืด");
    }

    if (darkMode_) {
        qApp->setStyleSheet(QStringLiteral(R"QSS(
            QMainWindow {
                background-color: #111827;
                color: #f9fafc;
            }
            QToolBar {
                background: #1f2937;
                border-bottom: 1px solid #374151;
                padding: 5px 8px;
                spacing: 6px;
            }
            QToolButton {
                background: #374151;
                color: #f9fafc;
                border: 1px solid #4b5563;
                border-radius: 6px;
                padding: 6px 12px;
                font-size: 13px;
            }
            QToolButton:hover {
                background: #2563eb;
                border-color: #3b82f6;
                color: #ffffff;
            }
            QToolButton:checked {
                background: #1d4ed8;
                color: #ffffff;
            }
            QLineEdit {
                background-color: #111827;
                color: #f9fafc;
                border: 1px solid #374151;
                border-radius: 6px;
                padding: 6px 12px;
                font-size: 14px;
            }
            QLineEdit:focus {
                border: 1px solid #3b82f6;
            }
            QTabWidget::pane {
                border: none;
                background: #111827;
            }
            QTabBar {
                background: #111827;
                border-bottom: 1px solid #1f2937;
            }
            QTabBar::tab {
                background: #1f2937;
                color: #9ca3af;
                border: 1px solid #374151;
                border-bottom: none;
                border-top-left-radius: 6px;
                border-top-right-radius: 6px;
                padding: 7px 16px;
                margin-right: 3px;
                font-size: 13px;
            }
            QTabBar::tab:selected {
                background: #111827;
                color: #3b82f6;
                border: 1px solid #3b82f6;
                border-bottom: 2px solid #111827;
                font-weight: bold;
            }
            QProgressBar {
                background-color: #1f2937;
                border: none;
                max-height: 3px;
            }
            QProgressBar::chunk {
                background-color: #3b82f6;
            }
            QStatusBar {
                background: #1f2937;
                color: #9ca3af;
                border-top: 1px solid #374151;
            }
        )QSS"));
    } else {
        qApp->setStyleSheet(QStringLiteral(R"QSS(
            QMainWindow {
                background-color: #ffffff;
                color: #111827;
            }
            QToolBar {
                background: #ffffff;
                border-bottom: 1px solid #e5e7eb;
                padding: 5px 8px;
                spacing: 6px;
            }
            QToolButton {
                background: #f3f4f6;
                color: #1f2937;
                border: 1px solid #e5e7eb;
                border-radius: 6px;
                padding: 6px 12px;
                font-size: 13px;
            }
            QToolButton:hover {
                background: #2563eb;
                border-color: #2563eb;
                color: #ffffff;
            }
            QToolButton:checked {
                background: #1d4ed8;
                color: #ffffff;
            }
            QLineEdit {
                background-color: #f9fafb;
                color: #111827;
                border: 1px solid #d1d5db;
                border-radius: 6px;
                padding: 6px 12px;
                font-size: 14px;
            }
            QLineEdit:focus {
                border: 1px solid #2563eb;
                background-color: #ffffff;
            }
            QTabWidget::pane {
                border: none;
                background: #ffffff;
            }
            QTabBar {
                background: #ffffff;
                border-bottom: 1px solid #e5e7eb;
            }
            QTabBar::tab {
                background: #f3f4f6;
                color: #4b5563;
                border: 1px solid #e5e7eb;
                border-bottom: none;
                border-top-left-radius: 6px;
                border-top-right-radius: 6px;
                padding: 7px 16px;
                margin-right: 3px;
                font-size: 13px;
            }
            QTabBar::tab:selected {
                background: #ffffff;
                color: #2563eb;
                border: 1px solid #2563eb;
                border-bottom: 2px solid #ffffff;
                font-weight: bold;
            }
            QProgressBar {
                background-color: #e5e7eb;
                border: none;
                max-height: 3px;
            }
            QProgressBar::chunk {
                background-color: #2563eb;
            }
            QStatusBar {
                background: #ffffff;
                color: #6b7280;
                border-top: 1px solid #e5e7eb;
            }
        )QSS"));
    }
}

void MainWindow::loadHome(QWebEngineView *view)
{
    const int blockedCount = adBlocker_ ? adBlocker_->blockedCount() : 0;
    const QString bg = darkMode_ ? "#111827" : "#ffffff";
    const QString cardBg = darkMode_ ? "#1f2937" : "#f9fafb";
    const QString textCol = darkMode_ ? "#f9fafc" : "#111827";
    const QString subCol = darkMode_ ? "#9ca3af" : "#6b7280";
    const QString borderCol = darkMode_ ? "#374151" : "#e5e7eb";

    const QString html = QString(R"HTML(
<!doctype html>
<html lang="th">
<head>
<meta charset="utf-8">
<title>LiteWave</title>
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<style>
* { box-sizing: border-box; margin: 0; padding: 0; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, "Helvetica Neue", Arial, sans-serif; }
body {
  background-color: %1;
  color: %3;
  min-height: 100vh;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: flex-start;
  padding: 80px 20px 40px;
}
.brand {
  font-size: 52px;
  font-weight: 700;
  letter-spacing: -1px;
  margin-bottom: 8px;
  user-select: none;
}
.brand .c-blue { color: #4285F4; }
.brand .c-red { color: #EA4335; }
.brand .c-yellow { color: #FBBC05; }
.brand .c-green { color: #34A853; }
.subtitle {
  font-size: 14px;
  color: %4;
  margin-bottom: 32px;
  font-weight: 400;
}
.search-container {
  width: 100%;
  max-width: 740px;
  margin-bottom: 44px;
}
.search-box {
  display: flex;
  align-items: center;
  background-color: %2;
  border: 1px solid %5;
  border-radius: 28px;
  padding: 4px 8px 4px 18px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.08);
  transition: box-shadow 0.2s ease, border-color 0.2s ease;
}
.search-box:focus-within, .search-box:hover {
  box-shadow: 0 4px 18px rgba(32, 33, 36, 0.16);
  border-color: #4285F4;
}
.search-icon {
  display: flex;
  align-items: center;
  justify-content: center;
  color: %4;
  margin-right: 8px;
}
.search-box select {
  background-color: transparent;
  color: %3;
  border: none;
  font-size: 14px;
  font-weight: 500;
  outline: none;
  cursor: pointer;
  padding: 0 8px 0 0;
  border-right: 1px solid %5;
}
.search-box select option {
  background-color: %2;
  color: %3;
}
.search-box input {
  flex: 1;
  border: none;
  background: transparent;
  padding: 12px 14px;
  font-size: 16px;
  color: %3;
  outline: none;
}
.search-box button {
  background-color: #1a73e8;
  color: #ffffff;
  border: none;
  border-radius: 20px;
  padding: 10px 24px;
  font-size: 14px;
  font-weight: 600;
  cursor: pointer;
  transition: background-color 0.15s ease, transform 0.1s ease;
}
.search-box button:hover {
  background-color: #1557b0;
}
.search-box button:active {
  transform: scale(0.98);
}
.sites-section {
  width: 100%;
  max-width: 740px;
}
.section-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  margin-bottom: 16px;
}
.section-title {
  font-size: 13px;
  font-weight: 600;
  letter-spacing: 0.5px;
  text-transform: uppercase;
  color: %4;
}
.sites-grid {
  display: grid;
  grid-template-columns: repeat(4, 1fr);
  gap: 16px;
}
.site-card {
  position: relative;
  background-color: %2;
  border: 1px solid %5;
  border-radius: 12px;
  padding: 20px 12px;
  text-align: center;
  cursor: pointer;
  text-decoration: none;
  color: %3;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: 10px;
  transition: border-color 0.15s ease, transform 0.15s ease, box-shadow 0.15s ease;
}
.site-card:hover {
  border-color: #4285F4;
  transform: translateY(-2px);
  box-shadow: 0 4px 12px rgba(0,0,0,0.06);
}
.site-icon {
  width: 32px;
  height: 32px;
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 18px;
  font-weight: 700;
  border-radius: 50%;
}
.custom-avatar {
  background-color: #e8f0fe;
  color: #1a73e8;
  width: 36px;
  height: 36px;
  border-radius: 50%;
}
.site-name {
  font-size: 13px;
  font-weight: 500;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
  max-width: 100px;
}
.del-btn {
  position: absolute;
  top: 6px;
  right: 6px;
  background: rgba(0,0,0,0.1);
  color: %3;
  border: none;
  width: 20px;
  height: 20px;
  border-radius: 50%;
  font-size: 12px;
  cursor: pointer;
  display: none;
  align-items: center;
  justify-content: center;
}
.site-card:hover .del-btn {
  display: flex;
}
.del-btn:hover {
  background: #ea4335;
  color: #fff;
}
.add-card {
  border: 2px dashed %5;
  background-color: transparent;
}
.add-card:hover {
  border-color: #4285F4;
  background-color: %2;
}
.add-icon {
  font-size: 24px;
  color: #4285F4;
}
.modal-overlay {
  position: fixed;
  top: 0; left: 0; right: 0; bottom: 0;
  background: rgba(0,0,0,0.45);
  display: none;
  align-items: center;
  justify-content: center;
  z-index: 1000;
}
.modal-overlay.active {
  display: flex;
}
.modal {
  background-color: %2;
  color: %3;
  border: 1px solid %5;
  border-radius: 16px;
  padding: 24px;
  width: 90%;
  max-width: 420px;
  box-shadow: 0 12px 36px rgba(0,0,0,0.25);
}
.modal h3 {
  font-size: 18px;
  font-weight: 600;
  margin-bottom: 16px;
}
.modal input {
  width: 100%;
  padding: 12px 16px;
  margin-bottom: 12px;
  border: 1px solid %5;
  border-radius: 8px;
  background: %1;
  color: %3;
  font-size: 14px;
  outline: none;
}
.modal input:focus {
  border-color: #4285F4;
}
.modal-buttons {
  display: flex;
  justify-content: flex-end;
  gap: 10px;
  margin-top: 8px;
}
.modal-buttons button {
  padding: 10px 20px;
  border-radius: 8px;
  font-size: 14px;
  font-weight: 600;
  cursor: pointer;
  border: none;
}
.btn-cancel {
  background: transparent;
  color: %4;
}
.btn-save {
  background: #1a73e8;
  color: #fff;
}
.btn-save:hover {
  background: #1557b0;
}
.footer-note {
  margin-top: auto;
  padding-top: 48px;
  font-size: 13px;
  color: %4;
}
</style>
</head>
<body>

<div class="brand">
  <span class="c-blue">Lite</span><span class="c-red">W</span><span class="c-yellow">a</span><span class="c-green">v</span><span class="c-red">e</span>
</div>
<div class="subtitle">ระบบค้นหาและท่องเว็บความเร็วสูง · ปลอดภัย ไร้โฆษณารบกวน</div>

<div class="search-container">
  <form class="search-box" action="litewave://search" method="get">
    <div class="search-icon">
      <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round"><circle cx="11" cy="11" r="8"></circle><line x1="21" y1="21" x2="16.65" y2="16.65"></line></svg>
    </div>
    <select id="engine" name="engine" aria-label="เครื่องมือค้นหา">
      <option value="brave" selected>Brave Search</option>
      <option value="google">Google Search</option>
      <option value="duckduckgo">DuckDuckGo</option>
    </select>
    <input id="q" name="q" type="search" autofocus placeholder="ค้นหาบน Google หรือป้อนที่อยู่เว็บไซต์..." autocomplete="off">
    <button type="submit">ค้นหา</button>
  </form>
</div>

<div class="sites-section">
  <div class="section-header">
    <div class="section-title">ทางลัดเว็บไซต์</div>
  </div>
  <div class="sites-grid" id="sitesGrid"></div>
</div>

<div class="modal-overlay" id="addModal">
  <div class="modal">
    <h3>เพิ่มทางลัดเว็บไซต์</h3>
    <input id="shortcutName" placeholder="ชื่อเว็บไซต์ (เช่น Pantip)" autocomplete="off">
    <input id="shortcutUrl" placeholder="ที่อยู่เว็บ (เช่น https://pantip.com)" autocomplete="off">
    <div class="modal-buttons">
      <button type="button" class="btn-cancel" onclick="closeAddModal()">ยกเลิก</button>
      <button type="button" class="btn-save" onclick="saveCustomShortcut()">บันทึก</button>
    </div>
  </div>
</div>

<div class="footer-note">Shield Active · บล็อกโฆษณาแล้ว %6 รายการ</div>

<script>
const defaultSites = [
  { name: 'Google', url: 'https://www.google.com', icon: '<svg width="26" height="26" viewBox="0 0 24 24"><path fill="#4285F4" d="M22.56 12.25c0-.78-.07-1.53-.2-2.25H12v4.26h5.92c-.26 1.37-1.04 2.53-2.21 3.31v2.77h3.57c2.08-1.92 3.28-4.74 3.28-8.09z"/><path fill="#34A853" d="M12 23c2.97 0 5.46-.98 7.28-2.66l-3.57-2.77c-.98.66-2.23 1.06-3.71 1.06-2.86 0-5.29-1.93-6.16-4.53H2.18v2.84C3.99 20.53 7.7 23 12 23z"/><path fill="#FBBC05" d="M5.84 14.1c-.22-.66-.35-1.36-.35-2.1s.13-1.44.35-2.1V7.06H2.18C1.43 8.55 1 10.22 1 12s.43 3.45 1.18 4.94l2.85-2.22.81-.62z"/><path fill="#EA4335" d="M12 5.38c1.62 0 3.06.56 4.21 1.64l3.15-3.15C17.45 2.09 14.97 1 12 1 7.7 1 3.99 3.47 2.18 7.06l3.66 2.84c.87-2.6 3.3-4.52 6.16-4.52z"/></svg>' },
  { name: 'YouTube', url: 'https://www.youtube.com', icon: '<svg width="26" height="26" viewBox="0 0 24 24"><path fill="#FF0000" d="M23.498 6.186a3.016 3.016 0 0 0-2.122-2.136C19.505 3.545 12 3.545 12 3.545s-7.505 0-9.377.505A3.017 3.017 0 0 0 .502 6.186C0 8.07 0 12 0 12s0 3.93.502 5.814a3.016 3.016 0 0 0 2.122 2.136c1.871.505 9.376.505 9.376.505s7.505 0 9.377-.505a3.015 3.015 0 0 0 2.122-2.136C24 15.93 24 12 24 12s0-3.93-.502-5.814zM9.545 15.568V8.432L15.818 12l-6.273 3.568z"/></svg>' },
  { name: 'GitHub', url: 'https://github.com', icon: '<svg width="26" height="26" viewBox="0 0 24 24" fill="currentColor"><path d="M12 0C5.37 0 0 5.37 0 12c0 5.31 3.435 9.795 8.205 11.385.6.105.825-.255.825-.57 0-.285-.015-1.23-.015-2.235-3.015.555-3.795-.735-4.035-1.41-.135-.345-.72-1.41-1.23-1.695-.42-.225-1.02-.78-.015-.795.945-.015 1.62.87 1.845 1.23 1.08 1.815 2.805 1.305 3.495.99.105-.78.42-1.305.765-1.605-2.67-.3-5.46-1.335-5.46-5.925 0-1.305.465-2.385 1.23-3.225-.12-.3-.54-1.53.12-3.18 0 0 1.005-.315 3.3 1.23.96-.27 1.98-.405 3-.405s2.04.135 3 .405c2.295-1.56 3.3-1.23 3.3-1.23.66 1.65.24 2.88.12 3.18.765.84 1.23 1.905 1.23 3.225 0 4.605-2.805 5.625-5.475 5.925.435.375.81 1.095.81 2.22 0 1.605-.015 2.895-.015 3.3 0 .315.225.69.825.57A12.02 12.02 0 0024 12c0-6.63-5.37-12-12-12z"/></svg>' },
  { name: 'ChatGPT', url: 'https://chatgpt.com', icon: '<svg width="26" height="26" viewBox="0 0 24 24" fill="currentColor"><path d="M22.282 9.821a5.985 5.985 0 0 0-.516-4.91 6.046 6.046 0 0 0-6.51-2.9 6.065 6.065 0 0 0-4.975-2.484 6.05 6.05 0 0 0-5.784 4.098 6.02 6.02 0 0 0-4.636 3.3 6.062 6.062 0 0 0 .734 6.643 5.985 5.985 0 0 0 .517 4.911 6.047 6.047 0 0 0 6.51 2.9 6.056 6.056 0 0 0 4.976 2.484 6.05 6.05 0 0 0 5.784-4.097 6.016 6.016 0 0 0 4.635-3.3 6.063 6.063 0 0 0-.749-6.644zM12.001 20.407a4.417 4.417 0 0 1-2.896-1.077l.147-.084 3.738-2.158a.834.834 0 0 0 .42-.724v-5.269l1.579.912a.08.08 0 0 1 .042.062v4.416a4.432 4.432 0 0 1-3.03 3.922zm-7.669-4.708a4.423 4.423 0 0 1-.535-3.037l.149.088 3.737 2.157a.835.835 0 0 0 .838 0l4.563-2.634v1.824a.08.08 0 0 1-.038.069l-3.824 2.208a4.434 4.434 0 0 1-4.89-.675zm-1.127-8.98a4.418 4.418 0 0 1 2.361-1.961l.002.172v4.316a.835.835 0 0 0 .419.724l4.562 2.634-1.578.911a.08.08 0 0 1-.079 0l-3.824-2.207a4.433 4.433 0 0 1-1.861-4.589zm14.862 3.652-4.563-2.634 1.579-.911a.08.08 0 0 1 .079 0l3.824 2.207a4.434 4.434 0 0 1 1.86 4.589 4.417 4.417 0 0 1-2.36 1.962v-4.488a.836.836 0 0 0-.419-.725zm2.146-2.585a4.43 4.43 0 0 1 .536 3.038l-.149-.087-3.738-2.158a.833.833 0 0 0-.837 0l-4.563 2.634v-1.824a.08.08 0 0 1 .038-.069l3.824-2.208a4.434 4.434 0 0 1 4.889.674zM12 13.722l-2.686-1.551 2.686-1.55 2.686 1.55-2.686 1.551z"/></svg>' },
  { name: 'Facebook', url: 'https://www.facebook.com', icon: '<svg width="26" height="26" viewBox="0 0 24 24" fill="#1877F2"><path d="M24 12.073c0-6.627-5.373-12-12-12s-12 5.373-12 12c0 5.99 4.388 10.954 10.125 11.854v-8.385H7.078v-3.47h3.047V9.43c0-3.007 1.792-4.669 4.533-4.669 1.312 0 2.686.235 2.686.235v2.953H15.83c-1.491 0-1.956.925-1.956 1.874v2.25h3.328l-.532 3.47h-2.796v8.385C19.612 23.027 24 18.062 24 12.073z"/></svg>' },
  { name: 'Wikipedia', url: 'https://www.wikipedia.org', icon: '<svg width="26" height="26" viewBox="0 0 24 24" fill="currentColor"><path d="M12.09 13.118l-2.072-4.78h-.056l-2.1 4.78h4.228zM1.4 5.312h4.545v.852H4.492l3.414 7.828 2.502-5.748h-1.026v-.852h4.15v.852h-1.028l2.673 6.136 3.255-7.488h-1.572v-.852h4.74v.852h-1.2l-4.78 10.99H14.45L11.5 10.934l-2.928 6.726H7.333L2.73 6.164H1.4v-.852z"/></svg>' },
  { name: 'Reddit', url: 'https://www.reddit.com', icon: '<svg width="26" height="26" viewBox="0 0 24 24" fill="#FF4500"><path d="M12 0A12 12 0 0 0 0 12a12 12 0 0 0 12 12 12 12 0 0 0 12-12A12 12 0 0 0 12 0zm5.01 4.744c.688 0 1.25.561 1.25 1.249a1.25 1.25 0 0 1-2.498.056l-2.597-.547-.8 3.747c1.824.07 3.48.632 4.674 1.488.308-.309.73-.491 1.196-.491.961 0 1.741.78 1.741 1.74 0 .61-.315 1.144-.792 1.45.023.19.034.381.034.574 0 2.924-3.387 5.295-7.564 5.295-4.178 0-7.565-2.371-7.565-5.295 0-.19.011-.38.033-.57-.481-.307-.798-.842-.798-1.45 0-.96.78-1.74 1.741-1.74.47 0 .895.186 1.204.499 1.198-.859 2.859-1.42 4.689-1.487l.926-4.341 3.204.675a1.247 1.247 0 0 1 1.207-.852zm-8.01 8.875c-.687 0-1.243.557-1.243 1.244 0 .687.556 1.244 1.243 1.244.688 0 1.244-.557 1.244-1.244 0-.687-.556-1.244-1.244-1.244zm6.002 0c-.687 0-1.244.557-1.244 1.244 0 .687.557 1.244 1.244 1.244.688 0 1.243-.557 1.243-1.244 0-.687-.555-1.244-1.244-1.244zm-5.466 3.655a.39.39 0 0 0-.27.666c.883.882 2.316.882 3.199 0a.39.39 0 1 0-.55-.552c-.579.578-1.518.578-2.097 0a.386.386 0 0 0-.282-.114z"/></svg>' },
  { name: 'Twitch', url: 'https://twitch.tv', icon: '<svg width="26" height="26" viewBox="0 0 24 24" fill="#9146FF"><path d="M11.571 4.714h1.715v5.143H11.571V4.714zm4.715 0H18v5.143h-1.714V4.714zM6 0L1.714 4.286v15.428h5.143V24l4.286-4.286h3.428L22.286 12V0H6zm14.571 11.143l-3.428 3.429h-3.429l-3 3v-3H6.857V1.714h13.714v9.429z"/></svg>' }
];

function renderShortcuts() {
  const grid = document.getElementById('sitesGrid');
  if (!grid) return;
  grid.innerHTML = '';

  defaultSites.forEach(s => {
    const card = document.createElement('a');
    card.className = 'site-card';
    card.href = s.url;
    card.innerHTML = `<div class="site-icon">${s.icon}</div><div class="site-name">${s.name}</div>`;
    grid.appendChild(card);
  });

  const custom = JSON.parse(localStorage.getItem('litewave_custom_shortcuts') || '[]');
  custom.forEach((s, idx) => {
    const card = document.createElement('div');
    card.className = 'site-card';
    card.onclick = () => window.location.href = s.url;
    const initial = (s.name || 'W').charAt(0).toUpperCase();
    card.innerHTML = `
      <button class="del-btn" title="ลบทางลัด" onclick="deleteShortcut(event, ${idx})">✕</button>
      <div class="site-icon custom-avatar">${initial}</div>
      <div class="site-name">${s.name}</div>
    `;
    grid.appendChild(card);
  });

  const addBtn = document.createElement('div');
  addBtn.className = 'site-card add-card';
  addBtn.onclick = openAddModal;
  addBtn.innerHTML = `
    <div class="site-icon add-icon">+</div>
    <div class="site-name">เพิ่มทางลัด</div>
  `;
  grid.appendChild(addBtn);
}

function openAddModal() {
  document.getElementById('addModal').classList.add('active');
  document.getElementById('shortcutName').focus();
}

function closeAddModal() {
  document.getElementById('addModal').classList.remove('active');
  document.getElementById('shortcutName').value = '';
  document.getElementById('shortcutUrl').value = '';
}

function saveCustomShortcut() {
  const name = document.getElementById('shortcutName').value.trim();
  let url = document.getElementById('shortcutUrl').value.trim();
  if (!name || !url) return;
  if (!url.startsWith('http://') && !url.startsWith('https://')) {
    url = 'https://' + url;
  }
  const custom = JSON.parse(localStorage.getItem('litewave_custom_shortcuts') || '[]');
  custom.push({ name, url });
  localStorage.setItem('litewave_custom_shortcuts', JSON.stringify(custom));
  closeAddModal();
  renderShortcuts();
}

function deleteShortcut(e, idx) {
  e.stopPropagation();
  const custom = JSON.parse(localStorage.getItem('litewave_custom_shortcuts') || '[]');
  custom.splice(idx, 1);
  localStorage.setItem('litewave_custom_shortcuts', JSON.stringify(custom));
  renderShortcuts();
}

renderShortcuts();
</script>
</body>
</html>
)HTML").arg(bg, cardBg, textCol, subCol, borderCol).arg(blockedCount);

    view->setHtml(html, QUrl("https://litewave.home/"));
}



