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
#include <QToolBar>
#include <QToolButton>
#include <QWebEngineProfile>
#include <QWebEngineView>

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
    // Each profile owns its interceptor. Normal windows share Shield state.
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

    auto *back = addButton("ย้อนกลับ");
    connect(back, &QAction::triggered, this, [this] { if (currentView()) currentView()->back(); });

    auto *forward = addButton("ถัดไป");
    connect(forward, &QAction::triggered, this, [this] { if (currentView()) currentView()->forward(); });

    auto *reload = addButton("รีเฟรช");
    connect(reload, &QAction::triggered, this, [this] { if (currentView()) currentView()->reload(); });

    auto *home = addButton("หน้าแรก");
    connect(home, &QAction::triggered, this, [this] { if (currentView()) loadHome(currentView()); });

    auto *plus = addButton("+ แท็บ");
    connect(plus, &QAction::triggered, this, &MainWindow::newTab);

    urlBar_->setPlaceholderText("พิมพ์คำค้นหา หรือใส่ที่อยู่เว็บไซต์");
    urlBar_->setClearButtonEnabled(true);
    toolbar->addWidget(urlBar_);
    connect(urlBar_, &QLineEdit::returnPressed, this, &MainWindow::navigate);

    auto *bookmark = addButton("★ บันทึก");
    connect(bookmark, &QAction::triggered, this, &MainWindow::addBookmark);

    shieldAction_ = addButton("Shield: เปิด");
    shieldAction_->setCheckable(true);
    shieldAction_->setChecked(true);
    connect(shieldAction_, &QAction::toggled, this, &MainWindow::toggleShield);

    auto *theme = addButton("โทนมืด");
    connect(theme, &QAction::triggered, this, &MainWindow::toggleTheme);

    progress_->setMaximumWidth(110);
    progress_->setTextVisible(false);
    progress_->setRange(0, 100);
    toolbar->addWidget(progress_);

    tabs_->setTabsClosable(true);
    tabs_->setDocumentMode(true);
    tabs_->setMovable(true);
    setCentralWidget(tabs_);

    connect(tabs_, &QTabWidget::currentChanged, this, [this](int) {
        if (currentView()) { urlBar_->setText(currentView()->url().toString()); setWindowTitle(currentView()->title() + (privateMode_ ? " — Private · LiteWave" : " — LiteWave")); }
    });
    connect(tabs_, &QTabWidget::tabCloseRequested, this, &MainWindow::closeTab);

    setupShortcuts();
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

void MainWindow::updateCurrentUrl(const QUrl &url)
{
    auto *view = qobject_cast<QWebEngineView *>(sender());
    if (!view || view != currentView()) return;
    urlBar_->setText(url.toString());
    urlBar_->setCursorPosition(0);
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
    shieldAction_->setText(enabled ? "Shield: เปิด" : "Shield: ปิด");
    statusBar()->showMessage(enabled ? "เปิดการบล็อกโฆษณา" : "ปิดการบล็อกโฆษณา", 2500);
}

void MainWindow::toggleTheme()
{
    darkMode_ = !darkMode_;
    applyTheme();
}

void MainWindow::applyTheme()
{
    qApp->setStyleSheet(darkMode_
        ? "QMainWindow{background:#121820} QToolBar{background:#1b2633;color:#fff;border:0;padding:5px} QToolButton{color:#fff;padding:6px} QLineEdit{background:#293849;color:#fff;border:1px solid #587083;padding:7px;border-radius:5px} QTabBar::tab{background:#263443;color:#ddd;padding:8px 18px} QTabBar::tab:selected{background:#1688c0;color:#fff}"
        : "");
}

void MainWindow::loadHome(QWebEngineView *view)
{
    const QString html = QStringLiteral(R"HTML(
<!doctype html><html lang="th"><head><meta charset="utf-8"><title>LiteWave</title>
<style>
*{box-sizing:border-box}body{margin:0;background:#f4f6f8;color:#20252b;font-family:Arial,sans-serif}
header{height:70px;background:#fff;border-bottom:1px solid #e1e5e8;display:flex;align-items:center;padding:0 42px}
.brand{font-size:25px;font-weight:700;color:#1688c0}.tag{margin-left:auto;color:#75818b;font-size:13px}
main{max-width:980px;margin:75px auto;padding:0 28px}h1{font-weight:500;font-size:32px;margin:0 0 9px}
p{color:#69747e;margin:0 0 30px}.search{display:flex;background:#fff;border:1px solid #cdd6dc;border-radius:8px;padding:6px;box-shadow:0 3px 12px #1a2a3814}
input{flex:1;border:0;outline:0;padding:14px;font-size:16px}.search button{border:0;background:#1688c0;color:#fff;border-radius:6px;padding:0 25px;font-size:15px}
h2{font-size:18px;font-weight:500;margin:42px 0 15px}.sites{display:grid;grid-template-columns:repeat(4,1fr);gap:16px}
.site{background:#fff;border:1px solid #e1e6ea;border-radius:8px;text-align:center;padding:24px 12px;min-height:112px;cursor:pointer}.site:hover{border-color:#1688c0}
.icon{font-size:28px;color:#1688c0;margin-bottom:10px}.site span{font-size:14px}.note{color:#84919a;font-size:13px;margin-top:34px}
</style></head><body><header><div class="brand">LiteWave</div><div class="tag">เร็วขึ้น · เป็นส่วนตัวขึ้น · ควบคุมได้มากขึ้น</div></header>
<main><h1>เริ่มต้นใช้งาน</h1><p>พิมพ์คำค้นหาได้ทันที เปิดหลายแท็บ และลดโฆษณารบกวนด้วย Shield</p>
<form class="search" onsubmit="go(event)"><input id="q" autofocus placeholder="พิมพ์สิ่งที่ต้องการค้นหา เช่น ข่าว F1 หรือ YouTube"><button>ค้นหา</button></form>
<h2>ทางลัด</h2><div class="sites">
<div class="site" onclick="openSite('https://www.google.com')"><div class="icon">G</div><span>Google</span></div>
<div class="site" onclick="openSite('https://www.youtube.com')"><div class="icon">▶</div><span>YouTube</span></div>
<div class="site" onclick="openSite('https://github.com')"><div class="icon">⌘</div><span>GitHub</span></div>
<div class="site" onclick="help()"><div class="icon">?</div><span>AI ช่วยค้นหา</span></div>
</div><div class="note">LiteWave 0.6 · Google Search + Shield บล็อกตัวติดตามและโฆษณาจากโดเมนที่รู้จัก</div></main>
<script>
function openSite(u){window.location.href=u}
function help(){document.getElementById('q').value='ช่วยค้นหา ';document.getElementById('q').focus()}
function go(e){
  e.preventDefault();
  const x=document.getElementById('q').value.trim();
  if(!x)return;
  const isUrl=x.indexOf('://')>0 || x.startsWith('www.') || (x.indexOf('.')>0 && x.indexOf(' ')<0);
  window.location.href=isUrl?(x.indexOf('://')>0?x:'https://'+x):'https://www.google.com/search?q='+encodeURIComponent(x);
}
</script></body></html>
)HTML");
    view->setHtml(html, QUrl("https://litewave.home/"));
}
