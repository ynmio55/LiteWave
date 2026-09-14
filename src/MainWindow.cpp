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
#include <QWebEnginePage>
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
    const QString storagePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/Storage";
    const QString cachePath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/Cache";

    static QWebEngineProfile *normalProfile = new QWebEngineProfile("LiteWave", qApp);
    profile_ = privateMode_ ? new QWebEngineProfile(this) : normalProfile;
    profile_->setHttpUserAgent("Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/128.0.0.0 Safari/537.36");
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
    back->setToolTip("ย้อนกลับ (Alt+Left)");
    connect(back, &QAction::triggered, this, [this] { if (currentView()) currentView()->back(); });

    auto *forward = addButton("▶");
    forward->setToolTip("ถัดไป (Alt+Right)");
    connect(forward, &QAction::triggered, this, [this] { if (currentView()) currentView()->forward(); });

    auto *reload = addButton("🔄");
    reload->setToolTip("รีเฟรช (F5)");
    connect(reload, &QAction::triggered, this, [this] { if (currentView()) currentView()->reload(); });

    auto *home = addButton("🏠");
    home->setToolTip("หน้าแรก (Alt+Home)");
    connect(home, &QAction::triggered, this, [this] { if (currentView()) loadHome(currentView()); });

    auto *plus = addButton("➕ แท็บ");
    plus->setToolTip("เปิดแท็บใหม่ (Ctrl+T)");
    connect(plus, &QAction::triggered, this, &MainWindow::newTab);

    // SSL security status label
    sslLabel_ = new QLabel("🔒 ", this);
    sslLabel_->setStyleSheet("padding-left: 8px; font-size: 14px;");
    toolbar->addWidget(sslLabel_);

    urlBar_->setPlaceholderText("🔍 พิมพ์คำค้นหา หรือระบุ URL เว็บไซต์...");
    urlBar_->setClearButtonEnabled(true);
    toolbar->addWidget(urlBar_);
    connect(urlBar_, &QLineEdit::returnPressed, this, &MainWindow::navigate);

    auto *bookmark = addButton("⭐");
    bookmark->setToolTip("บันทึกหน้าเว็บนี้ (Ctrl+D)");
    connect(bookmark, &QAction::triggered, this, &MainWindow::addBookmark);

    shieldAction_ = addButton("🛡️ Shield (0)");
    shieldAction_->setCheckable(true);
    shieldAction_->setChecked(true);
    connect(shieldAction_, &QAction::toggled, this, &MainWindow::toggleShield);
    connect(adBlocker_, &AdBlocker::countChanged, this, &MainWindow::updateShieldBadge);

    auto *theme = addButton("🌙 โหมดมืด");
    theme->setToolTip("สลับโหมดมืด/สว่าง");
    connect(theme, &QAction::triggered, this, &MainWindow::toggleTheme);

    progress_->setMaximumWidth(120);
    progress_->setTextVisible(false);
    progress_->setRange(0, 100);
    toolbar->addWidget(progress_);

    tabs_->setTabsClosable(true);
    tabs_->setDocumentMode(true);
    tabs_->setMovable(true);
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
    auto *view = new QWebEngineView(profile_, tabs_);
    view->settings()->setAttribute(QWebEngineSettings::FullScreenSupportEnabled, true);
    view->settings()->setAttribute(QWebEngineSettings::PlaybackRequiresUserGesture, false);
    const int index = tabs_->addTab(view, "LiteWave");
    tabs_->setCurrentIndex(index);
    tabs_->tabBar()->setVisible(tabs_->count() > 1);

    connect(view->page(), &QWebEnginePage::fullScreenRequested, this,
            [this](QWebEngineFullScreenRequest request) {
                request.accept();
                if (request.toggleOn()) {
                    toolbar_->hide();
                    tabs_->tabBar()->hide();
                    showFullScreen();
                } else {
                    showNormal();
                    toolbar_->show();
                    tabs_->tabBar()->setVisible(tabs_->count() > 1);
                }
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
    auto reload = [this]{ if(currentView()) currentView()->reload(); };
    key("Ctrl+R",reload); key("F5",reload);
    auto hard = [this]{ if(currentView()) currentView()->triggerPageAction(QWebEnginePage::ReloadAndBypassCache); };
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
            showNormal(); toolbar_->show(); tabs_->tabBar()->setVisible(tabs_->count()>1);
        } else { toolbar_->hide(); tabs_->tabBar()->hide(); statusBar()->hide(); showFullScreen(); }
    });
    key("Escape", [this]{
        if(!isFullScreen()) return;
        if(currentView()) currentView()->triggerPageAction(QWebEnginePage::ExitFullScreen);
        showNormal(); toolbar_->show(); tabs_->tabBar()->setVisible(tabs_->count()>1);
    });
}

void MainWindow::navigate()
{
    openUrl(urlBar_->text());
}

void MainWindow::openUrl(const QString &text)
{
    const QString input = text.trimmed();
    if (input.isEmpty() || !currentView()) return;

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

    // Default search engine: DuckDuckGo (Private, fast, 100% free of Google CAPTCHA/bot checks)
    url = QUrl("https://duckduckgo.com/?q=" + QUrl::toPercentEncoding(input));
    currentView()->setUrl(url);
}

void MainWindow::setupUserScripts()
{
    if (!profile_) return;

    if (profile_->scripts()->find("LiteWaveCosmeticCss").isEmpty()) {
        QWebEngineScript cssScript;
        cssScript.setName("LiteWaveCosmeticCss");
        cssScript.setSourceCode(AdBlocker::cosmeticCss());
        cssScript.setInjectionPoint(QWebEngineScript::DocumentReady);
        cssScript.setWorldId(QWebEngineScript::MainWorld);
        cssScript.setRunsOnSubFrames(true);
        profile_->scripts()->insert(cssScript);
    }

    if (profile_->scripts()->find("LiteWaveCosmeticJs").isEmpty()) {
        QWebEngineScript jsScript;
        jsScript.setName("LiteWaveCosmeticJs");
        jsScript.setSourceCode(AdBlocker::cosmeticJs());
        jsScript.setInjectionPoint(QWebEngineScript::DocumentReady);
        jsScript.setWorldId(QWebEngineScript::MainWorld);
        jsScript.setRunsOnSubFrames(true);
        profile_->scripts()->insert(jsScript);
    }
}



void MainWindow::updateShieldBadge(int count)
{
    if (shieldAction_) {
        const QString state = adBlocker_->isEnabled()
            ? "🛡️ Shield (" + QString::number(count) + ")"
            : "🛡️ Shield (ปิด)";
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
}

void MainWindow::toggleTheme()
{
    darkMode_ = !darkMode_;
    applyTheme();
}

void MainWindow::applyTheme()
{
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
* { box-sizing: border-box; margin: 0; padding: 0; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; }
body {
  background-color: %1;
  color: %3;
  min-height: 100vh;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: flex-start;
  padding: 60px 20px 30px;
}
.brand {
  font-size: 32px;
  font-weight: 700;
  color: #2563eb;
  margin-bottom: 8px;
}
.subtitle {
  font-size: 14px;
  color: %4;
  margin-bottom: 36px;
}
.search-container {
  width: 100%%;
  max-width: 640px;
  margin-bottom: 48px;
}
.search-box {
  display: flex;
  background-color: %2;
  border: 1px solid %5;
  border-radius: 8px;
  padding: 4px;
}
.search-box input {
  flex: 1;
  border: none;
  background: transparent;
  padding: 12px 16px;
  font-size: 16px;
  color: %3;
  outline: none;
}
.search-box button {
  background-color: #2563eb;
  color: #ffffff;
  border: none;
  border-radius: 6px;
  padding: 0 24px;
  font-size: 15px;
  font-weight: 500;
  cursor: pointer;
}
.search-box button:hover {
  background-color: #1d4ed8;
}
.sites-section {
  width: 100%%;
  max-width: 640px;
}
.section-title {
  font-size: 14px;
  font-weight: 600;
  color: %4;
  margin-bottom: 16px;
}
.sites-grid {
  display: grid;
  grid-template-columns: repeat(4, 1fr);
  gap: 16px;
}
.site-card {
  background-color: %2;
  border: 1px solid %5;
  border-radius: 8px;
  padding: 18px 12px;
  text-align: center;
  cursor: pointer;
  text-decoration: none;
  color: %3;
  transition: border-color 0.15s ease;
}
.site-card:hover {
  border-color: #2563eb;
}
.site-icon {
  font-size: 24px;
  margin-bottom: 8px;
}
.site-name {
  font-size: 13px;
  font-weight: 500;
}
.footer-note {
  margin-top: auto;
  padding-top: 40px;
  font-size: 13px;
  color: %4;
}
</style>
</head>
<body>

<div class="brand">LiteWave</div>
<div class="subtitle">เบราว์เซอร์ที่เร็ว ปลอดภัย และไร้โฆษณารบกวน</div>

<div class="search-container">
  <form class="search-box" onsubmit="go(event)">
    <input id="q" autofocus placeholder="พิมพ์สิ่งที่ต้องการค้นหา หรือระบุที่อยู่เว็บไซต์..." autocomplete="off">
    <button type="submit">ค้นหา</button>
  </form>
</div>

<div class="sites-section">
  <div class="section-title">ทางลัดเว็บไซต์</div>
  <div class="sites-grid">
    <div class="site-card" onclick="openSite('https://www.google.com')">
      <div class="site-icon">🌐</div>
      <div class="site-name">Google</div>
    </div>
    <div class="site-card" onclick="openSite('https://www.youtube.com')">
      <div class="site-icon">▶️</div>
      <div class="site-name">YouTube</div>
    </div>
    <div class="site-card" onclick="openSite('https://github.com')">
      <div class="site-icon">💻</div>
      <div class="site-name">GitHub</div>
    </div>
    <div class="site-card" onclick="openSite('https://chatgpt.com')">
      <div class="site-icon">🤖</div>
      <div class="site-name">ChatGPT</div>
    </div>
    <div class="site-card" onclick="openSite('https://www.facebook.com')">
      <div class="site-icon">📘</div>
      <div class="site-name">Facebook</div>
    </div>
    <div class="site-card" onclick="openSite('https://www.wikipedia.org')">
      <div class="site-icon">📖</div>
      <div class="site-name">Wikipedia</div>
    </div>
    <div class="site-card" onclick="openSite('https://www.reddit.com')">
      <div class="site-icon">🔴</div>
      <div class="site-name">Reddit</div>
    </div>
    <div class="site-card" onclick="openSite('https://twitch.tv')">
      <div class="site-icon">👾</div>
      <div class="site-name">Twitch</div>
    </div>
  </div>
</div>

<div class="footer-note">🛡️ Shield Active · บล็อกโฆษณาแล้ว %6 รายการ</div>

<script>
function openSite(u) {
  window.location.href = u;
}
function go(e) {
  e.preventDefault();
  const x = document.getElementById('q').value.trim();
  if (!x) return;
  const isUrl = x.indexOf('://') > 0 || x.startsWith('www.') || (x.indexOf('.') > 0 && x.indexOf(' ') < 0);
  window.location.href = isUrl ? (x.indexOf('://') > 0 ? x : 'https://' + x) : 'https://duckduckgo.com/?q=' + encodeURIComponent(x);
}
</script>
</body>
</html>
)HTML").arg(bg, cardBg, textCol, subCol, borderCol).arg(blockedCount);

    view->setHtml(html, QUrl("https://litewave.home/"));
}


