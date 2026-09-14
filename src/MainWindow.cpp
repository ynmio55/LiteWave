#include "MainWindow.h"
#include "AdBlocker.h"

#include <QAction>
#include <QApplication>
#include <QInputDialog>
#include <QLineEdit>
#include <QProgressBar>
#include <QSettings>
#include <QTabWidget>
#include <QToolBar>
#include <QToolButton>
#include <QWebEngineProfile>
#include <QWebEngineView>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      urlBar_(new QLineEdit(this)),
      tabs_(new QTabWidget(this)),
      progress_(new QProgressBar(this)),
      shieldAction_(nullptr),
      adBlocker_(new AdBlocker(QWebEngineProfile::defaultProfile()))
{
    setWindowTitle("LiteWave");
    resize(1320, 840);

    auto *profile = QWebEngineProfile::defaultProfile();
    profile->setHttpUserAgent(profile->httpUserAgent() + " LiteWave/0.3");
    profile->setUrlRequestInterceptor(adBlocker_);

    auto *toolbar = addToolBar("LiteWave");
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

    urlBar_->setPlaceholderText("ค้นหาเว็บหรือใส่ URL...");
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
        if (currentView()) updateCurrentUrl(currentView()->url());
    });
    connect(tabs_, &QTabWidget::tabCloseRequested, this, &MainWindow::closeTab);

    newTab();
}

QWebEngineView *MainWindow::currentView() const
{
    return qobject_cast<QWebEngineView *>(tabs_->currentWidget());
}

QWebEngineView *MainWindow::createView(const QUrl &url)
{
    auto *view = new QWebEngineView(tabs_);
    const int index = tabs_->addTab(view, "LiteWave");
    tabs_->setCurrentIndex(index);

    connect(view, &QWebEngineView::urlChanged, this, &MainWindow::updateCurrentUrl);
    connect(view, &QWebEngineView::titleChanged, this, &MainWindow::updateTabTitle);
    connect(view, &QWebEngineView::loadProgress, progress_, &QProgressBar::setValue);
    connect(view, &QWebEngineView::renderProcessTerminated, this,
            [this](QWebEnginePage::RenderProcessTerminationStatus, int) {
                if (currentView()) currentView()->setHtml("<h2 style='padding:40px'>หน้าเว็บหยุดทำงาน กดรีเฟรชเพื่อลองใหม่</h2>");
            });

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
    if (tabs_->count() == 1) {
        newTab();
        tabs_->removeTab(index);
        return;
    }
    QWidget *page = tabs_->widget(index);
    tabs_->removeTab(index);
    page->deleteLater();
}

void MainWindow::navigate()
{
    openUrl(urlBar_->text());
}

void MainWindow::openUrl(const QString &text)
{
    const QString input = text.trimmed();
    if (input.isEmpty() || !currentView()) return;

    QUrl url = QUrl::fromUserInput(input);
    if (!url.isValid()) return;
    if (url.scheme().isEmpty() || url.scheme() == "search")
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
        setWindowTitle(title.isEmpty() ? "LiteWave" : title + " - LiteWave");
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
<main><h1>เริ่มต้นใช้งาน</h1><p>ค้นหาเว็บ เปิดหลายแท็บ และลดโฆษณารบกวนด้วย Shield</p>
<form class="search" onsubmit="go(event)"><input id="q" autofocus placeholder="ค้นหาหรือใส่ URL"><button>ค้นหา</button></form>
<h2>ทางลัด</h2><div class="sites">
<div class="site" onclick="openSite('https://www.google.com')"><div class="icon">G</div><span>Google</span></div>
<div class="site" onclick="openSite('https://www.youtube.com')"><div class="icon">▶</div><span>YouTube</span></div>
<div class="site" onclick="openSite('https://github.com')"><div class="icon">⌘</div><span>GitHub</span></div>
<div class="site" onclick="help()"><div class="icon">?</div><span>AI ช่วยค้นหา</span></div>
</div><div class="note">LiteWave 0.3 · Shield บล็อกตัวติดตามและโฆษณาจากโดเมนที่รู้จัก</div></main>
<script>
function openSite(u){location.href=u}function help(){q.value='ช่วยค้นหา ';q.focus()}
function go(e){e.preventDefault();let x=q.value.trim();if(!x)return;let u=/^(https?:\\/\\/|localhost|[a-z0-9-]+\\.[a-z]{2,})/i.test(x)?(x.match(/^https?:\\/\\//i)?x:'https://'+x):'https://www.google.com/search?q='+encodeURIComponent(x);location.href=u}
</script></body></html>
)HTML");
    view->setHtml(html, QUrl("https://litewave.home/"));
}
