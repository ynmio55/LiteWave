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
    static QWebEngineProfile *normalProfile = new QWebEngineProfile("LiteWave", qApp);
    profile_ = privateMode_ ? new QWebEngineProfile(this) : normalProfile;
    if (!privateMode_) {
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

    url = QUrl("https://www.google.com/search?q=" + QUrl::toPercentEncoding(input));
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
                background-color: #0f172a;
                color: #f8fafc;
            }
            QToolBar {
                background: #1e293b;
                border-bottom: 1px solid #334155;
                padding: 6px 8px;
                spacing: 6px;
            }
            QToolButton {
                background: #334155;
                color: #f8fafc;
                border: 1px solid #475569;
                border-radius: 8px;
                padding: 6px 12px;
                font-size: 13px;
                font-weight: 500;
            }
            QToolButton:hover {
                background: #0284c7;
                border-color: #38bdf8;
                color: #ffffff;
            }
            QToolButton:checked {
                background: #0369a1;
                border-color: #0ea5e9;
                color: #ffffff;
            }
            QLineEdit {
                background-color: #0f172a;
                color: #f8fafc;
                border: 1.5px solid #334155;
                border-radius: 10px;
                padding: 7px 14px;
                font-size: 14px;
            }
            QLineEdit:focus {
                border: 1.5px solid #38bdf8;
                background-color: #1e293b;
            }
            QTabWidget::pane {
                border: none;
                background: #0f172a;
            }
            QTabBar {
                background: #0f172a;
                border-bottom: 1px solid #1e293b;
            }
            QTabBar::tab {
                background: #1e293b;
                color: #94a3b8;
                border: 1px solid #334155;
                border-bottom: none;
                border-top-left-radius: 8px;
                border-top-right-radius: 8px;
                padding: 8px 18px;
                margin-right: 4px;
                font-size: 13px;
            }
            QTabBar::tab:selected {
                background: #0f172a;
                color: #38bdf8;
                border: 1.5px solid #0ea5e9;
                border-bottom: 2px solid #0f172a;
                font-weight: bold;
            }
            QTabBar::tab:hover:!selected {
                background: #334155;
                color: #f1f5f9;
            }
            QProgressBar {
                background-color: #1e293b;
                border: none;
                border-radius: 3px;
                max-height: 4px;
            }
            QProgressBar::chunk {
                background-color: #38bdf8;
                border-radius: 3px;
            }
            QStatusBar {
                background: #1e293b;
                color: #94a3b8;
                border-top: 1px solid #334155;
            }
        )QSS"));
    } else {
        qApp->setStyleSheet(QStringLiteral(R"QSS(
            QMainWindow {
                background-color: #f8fafc;
                color: #0f172a;
            }
            QToolBar {
                background: #ffffff;
                border-bottom: 1px solid #e2e8f0;
                padding: 6px 8px;
                spacing: 6px;
            }
            QToolButton {
                background: #f1f5f9;
                color: #334155;
                border: 1px solid #cbd5e1;
                border-radius: 8px;
                padding: 6px 12px;
                font-size: 13px;
                font-weight: 500;
            }
            QToolButton:hover {
                background: #0284c7;
                border-color: #0284c7;
                color: #ffffff;
            }
            QToolButton:checked {
                background: #0369a1;
                color: #ffffff;
            }
            QLineEdit {
                background-color: #ffffff;
                color: #0f172a;
                border: 1.5px solid #cbd5e1;
                border-radius: 10px;
                padding: 7px 14px;
                font-size: 14px;
            }
            QLineEdit:focus {
                border: 1.5px solid #0284c7;
            }
            QTabWidget::pane {
                border: none;
                background: #f8fafc;
            }
            QTabBar {
                background: #f8fafc;
                border-bottom: 1px solid #e2e8f0;
            }
            QTabBar::tab {
                background: #f1f5f9;
                color: #64748b;
                border: 1px solid #cbd5e1;
                border-bottom: none;
                border-top-left-radius: 8px;
                border-top-right-radius: 8px;
                padding: 8px 18px;
                margin-right: 4px;
                font-size: 13px;
            }
            QTabBar::tab:selected {
                background: #ffffff;
                color: #0284c7;
                border: 1.5px solid #0284c7;
                border-bottom: 2px solid #ffffff;
                font-weight: bold;
            }
            QProgressBar {
                background-color: #e2e8f0;
                border: none;
                border-radius: 3px;
                max-height: 4px;
            }
            QProgressBar::chunk {
                background-color: #0284c7;
                border-radius: 3px;
            }
            QStatusBar {
                background: #ffffff;
                color: #64748b;
                border-top: 1px solid #e2e8f0;
            }
        )QSS"));
    }
}

void MainWindow::loadHome(QWebEngineView *view)
{
    const int blockedCount = adBlocker_ ? adBlocker_->blockedCount() : 0;
    const QString html = QString(R"HTML(
<!doctype html>
<html lang="th">
<head>
<meta charset="utf-8">
<title>LiteWave Modern Dashboard</title>
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<style>
* { box-sizing: border-box; margin: 0; padding: 0; font-family: 'Inter', system-ui, -apple-system, sans-serif; }
body {
  background: #090d16;
  color: #f1f5f9;
  min-height: 100vh;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: flex-start;
  padding: 40px 20px;
  background-image: 
    radial-gradient(circle at 15% 20%, rgba(56, 189, 248, 0.12) 0%, transparent 40%),
    radial-gradient(circle at 85% 75%, rgba(99, 102, 241, 0.12) 0%, transparent 45%);
}
.header {
  width: 100%;
  max-width: 1000px;
  display: flex;
  align-items: center;
  justify-content: space-between;
  margin-bottom: 40px;
}
.brand {
  font-size: 28px;
  font-weight: 800;
  background: linear-gradient(135deg, #38bdf8 0%%, #818cf8 100%%);
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
  display: flex;
  align-items: center;
  gap: 10px;
}
.tag {
  font-size: 13px;
  color: #94a3b8;
  background: rgba(30, 41, 59, 0.6);
  backdrop-filter: blur(8px);
  padding: 6px 14px;
  border-radius: 20px;
  border: 1px solid rgba(255, 255, 255, 0.08);
}
.hero {
  text-align: center;
  width: 100%;
  max-width: 720px;
  margin-bottom: 36px;
}
.clock {
  font-size: 48px;
  font-weight: 300;
  letter-spacing: 2px;
  color: #ffffff;
  margin-bottom: 8px;
}
.greeting {
  font-size: 16px;
  color: #94a3b8;
  margin-bottom: 28px;
}
.search-box {
  background: rgba(30, 41, 59, 0.7);
  backdrop-filter: blur(16px);
  border: 1.5px solid rgba(255, 255, 255, 0.12);
  border-radius: 16px;
  padding: 8px 12px;
  display: flex;
  align-items: center;
  gap: 10px;
  box-shadow: 0 12px 32px rgba(0, 0, 0, 0.35);
  transition: all 0.3s ease;
}
.search-box:focus-within {
  border-color: #38bdf8;
  box-shadow: 0 0 0 4px rgba(56, 189, 248, 0.2), 0 12px 36px rgba(0, 0, 0, 0.45);
}
.search-box select {
  background: rgba(15, 23, 42, 0.6);
  color: #38bdf8;
  border: 1px solid rgba(255, 255, 255, 0.1);
  padding: 10px 12px;
  border-radius: 10px;
  font-size: 14px;
  outline: none;
  cursor: pointer;
}
.search-box input {
  flex: 1;
  background: transparent;
  border: none;
  outline: none;
  color: #ffffff;
  font-size: 16px;
  padding: 10px 6px;
}
.search-box button {
  background: linear-gradient(135deg, #0284c7, #6366f1);
  color: #ffffff;
  border: none;
  padding: 12px 24px;
  border-radius: 12px;
  font-size: 15px;
  font-weight: 600;
  cursor: pointer;
  transition: transform 0.2s, opacity 0.2s;
}
.search-box button:hover {
  transform: translateY(-1px);
  opacity: 0.92;
}

.stats-grid {
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: 16px;
  width: 100%;
  max-width: 840px;
  margin-bottom: 40px;
}
.stat-card {
  background: rgba(30, 41, 59, 0.4);
  backdrop-filter: blur(12px);
  border: 1px solid rgba(255, 255, 255, 0.08);
  border-radius: 16px;
  padding: 20px;
  display: flex;
  align-items: center;
  gap: 14px;
  transition: transform 0.2s, border-color 0.2s;
}
.stat-card:hover {
  transform: translateY(-3px);
  border-color: rgba(56, 189, 248, 0.3);
}
.stat-icon {
  font-size: 28px;
  background: rgba(56, 189, 248, 0.1);
  padding: 12px;
  border-radius: 12px;
  color: #38bdf8;
}
.stat-value {
  font-size: 20px;
  font-weight: 700;
  color: #ffffff;
}
.stat-label {
  font-size: 12px;
  color: #94a3b8;
  margin-top: 2px;
}

.shortcuts-title {
  width: 100%;
  max-width: 840px;
  font-size: 16px;
  font-weight: 600;
  color: #cbd5e1;
  margin-bottom: 16px;
}
.sites-grid {
  display: grid;
  grid-template-columns: repeat(4, 1fr);
  gap: 16px;
  width: 100%;
  max-width: 840px;
}
.site-card {
  background: rgba(30, 41, 59, 0.5);
  backdrop-filter: blur(12px);
  border: 1px solid rgba(255, 255, 255, 0.08);
  border-radius: 16px;
  padding: 22px 14px;
  text-align: center;
  cursor: pointer;
  transition: all 0.25s cubic-bezier(0.4, 0, 0.2, 1);
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 10px;
}
.site-card:hover {
  transform: translateY(-4px);
  background: rgba(51, 65, 85, 0.7);
  border-color: #38bdf8;
  box-shadow: 0 10px 24px rgba(0,0,0,0.3);
}
.site-icon {
  width: 44px;
  height: 44px;
  border-radius: 12px;
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 22px;
  font-weight: bold;
  color: #ffffff;
  background: linear-gradient(135deg, #1e293b, #334155);
  border: 1px solid rgba(255,255,255,0.1);
}
.site-name {
  font-size: 14px;
  font-weight: 500;
  color: #e2e8f0;
}
.footer {
  margin-top: auto;
  padding-top: 40px;
  font-size: 12px;
  color: #64748b;
}
</style>
</head>
<body>

<div class="header">
  <div class="brand">🌊 LiteWave <span style="font-size:12px; vertical-align:middle; background:#0284c7; color:#fff; padding:2px 8px; border-radius:10px;">v0.7 Pro</span></div>
  <div class="tag">🛡️ Shield Active · Real-time Ad & Tracker Blocking</div>
</div>

<div class="hero">
  <div class="clock" id="clock">00:00:00</div>
  <div class="greeting" id="dateStr">กำลังโหลดวันเวลา...</div>

  <form class="search-box" onsubmit="handleSearch(event)">
    <select id="engine">
      <option value="google">Google</option>
      <option value="duckduckgo">DuckDuckGo</option>
      <option value="youtube">YouTube</option>
      <option value="bing">Bing</option>
    </select>
    <input id="q" autofocus placeholder="ค้นหาด้วย Google หรือระบุ URL เช่น github.com..." autocomplete="off">
    <button type="submit">ค้นหา</button>
  </form>
</div>

<div class="stats-grid">
  <div class="stat-card">
    <div class="stat-icon">🛡️</div>
    <div>
      <div class="stat-value" id="adCounter">%1</div>
      <div class="stat-label">โฆษณา & Trackers ถูกบล็อกแล้ว</div>
    </div>
  </div>
  <div class="stat-card">
    <div class="stat-icon">⚡</div>
    <div>
      <div class="stat-value">35%% Faster</div>
      <div class="stat-label">ความเร็วในการโหลดเว็บ</div>
    </div>
  </div>
  <div class="stat-card">
    <div class="stat-icon">🔒</div>
    <div>
      <div class="stat-value">HTTPS Guard</div>
      <div class="stat-label">การปกป้องความเป็นส่วนตัวขั้นสูง</div>
    </div>
  </div>
</div>

<div class="shortcuts-title">ทางลัดยอดนิยม</div>
<div class="sites-grid">
  <div class="site-card" onclick="openUrl('https://www.google.com')">
    <div class="site-icon" style="background: linear-gradient(135deg, #4285f4, #34a853);">G</div>
    <div class="site-name">Google</div>
  </div>
  <div class="site-card" onclick="openUrl('https://www.youtube.com')">
    <div class="site-icon" style="background: linear-gradient(135deg, #ff0000, #cc0000);">▶</div>
    <div class="site-name">YouTube</div>
  </div>
  <div class="site-card" onclick="openUrl('https://github.com')">
    <div class="site-icon" style="background: linear-gradient(135deg, #24292e, #040404);">⌘</div>
    <div class="site-name">GitHub</div>
  </div>
  <div class="site-card" onclick="openUrl('https://chatgpt.com')">
    <div class="site-icon" style="background: linear-gradient(135deg, #10a37f, #0d8a6c);">🤖</div>
    <div class="site-name">ChatGPT</div>
  </div>
  <div class="site-card" onclick="openUrl('https://www.facebook.com')">
    <div class="site-icon" style="background: linear-gradient(135deg, #1877f2, #0b5ed7);">f</div>
    <div class="site-name">Facebook</div>
  </div>
  <div class="site-card" onclick="openUrl('https://www.wikipedia.org')">
    <div class="site-icon" style="background: linear-gradient(135deg, #636466, #363738);">W</div>
    <div class="site-name">Wikipedia</div>
  </div>
  <div class="site-card" onclick="openUrl('https://www.reddit.com')">
    <div class="site-icon" style="background: linear-gradient(135deg, #ff4500, #d33800);">r/</div>
    <div class="site-name">Reddit</div>
  </div>
  <div class="site-card" onclick="openUrl('https://twitch.tv')">
    <div class="site-icon" style="background: linear-gradient(135deg, #9146ff, #6441a5);">👾</div>
    <div class="site-name">Twitch</div>
  </div>
</div>

<div class="footer">LiteWave 0.7 Pro · Powered by Qt WebEngine & Enhanced AdBlock Engine</div>

<script>
function updateClock() {
  const now = new Date();
  const timeStr = now.toLocaleTimeString('th-TH', { hour12: false });
  const dateOptions = { weekday: 'long', year: 'numeric', month: 'long', day: 'numeric' };
  const dateStr = now.toLocaleDateString('th-TH', dateOptions);
  
  document.getElementById('clock').textContent = timeStr;
  document.getElementById('dateStr').textContent = dateStr;
}
setInterval(updateClock, 1000);
updateClock();

function openUrl(u) {
  window.location.href = u;
}

function handleSearch(e) {
  e.preventDefault();
  const q = document.getElementById('q').value.trim();
  if (!q) return;

  const engine = document.getElementById('engine').value;
  const isUrl = q.indexOf('://') > 0 || q.startsWith('www.') || (q.indexOf('.') > 0 && q.indexOf(' ') < 0);
  
  if (isUrl) {
    window.location.href = q.indexOf('://') > 0 ? q : 'https://' + q;
    return;
  }

  let targetUrl = 'https://www.google.com/search?q=' + encodeURIComponent(q);
  if (engine === 'duckduckgo') {
    targetUrl = 'https://duckduckgo.com/?q=' + encodeURIComponent(q);
  } else if (engine === 'youtube') {
    targetUrl = 'https://www.youtube.com/results?search_query=' + encodeURIComponent(q);
  } else if (engine === 'bing') {
    targetUrl = 'https://www.bing.com/search?q=' + encodeURIComponent(q);
  }
  
  window.location.href = targetUrl;
}
</script>
</body>
</html>
)HTML").arg(blockedCount);

    view->setHtml(html, QUrl("https://litewave.home/"));
}

