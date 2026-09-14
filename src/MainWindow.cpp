#include "MainWindow.h"
#include "AdBlocker.h"

#include <QAction>
#include <QApplication>
#include <QLineEdit>
#include <QProgressBar>
#include <QToolBar>
#include <QUrl>
#include <QWebEngineProfile>
#include <QWebEngineView>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      urlBar_(new QLineEdit(this)),
      webView_(new QWebEngineView(this))
{
    setWindowTitle("LiteWave");
    resize(1280, 820);

    auto *profile = QWebEngineProfile::defaultProfile();
    profile->setHttpUserAgent(profile->httpUserAgent() + " LiteWave/0.2");
    profile->setUrlRequestInterceptor(new AdBlocker(profile));

    auto *toolbar = addToolBar("LiteWave");
    toolbar->setMovable(false);
    toolbar->setToolButtonStyle(Qt::ToolButtonTextOnly);

    auto *back = toolbar->addAction("ย้อนกลับ");
    connect(back, &QAction::triggered, webView_, &QWebEngineView::back);

    auto *forward = toolbar->addAction("ถัดไป");
    connect(forward, &QAction::triggered, webView_, &QWebEngineView::forward);

    auto *reload = toolbar->addAction("รีเฟรช");
    connect(reload, &QAction::triggered, webView_, &QWebEngineView::reload);

    urlBar_->setPlaceholderText("ค้นหาเว็บหรือใส่ URL...");
    urlBar_->setClearButtonEnabled(true);
    toolbar->addWidget(urlBar_);
    connect(urlBar_, &QLineEdit::returnPressed, this, &MainWindow::navigate);

    auto *shield = toolbar->addAction("Shield: เปิด");
    shield->setCheckable(true);
    shield->setChecked(true);
    connect(shield, &QAction::toggled, this, [shield](bool enabled) {
        shield->setText(enabled ? "Shield: เปิด" : "Shield: ปิด");
        if (auto *interceptor = QWebEngineProfile::defaultProfile()->urlRequestInterceptor()) {
            interceptor->setParent(enabled ? QWebEngineProfile::defaultProfile() : nullptr);
        }
    });

    auto *theme = toolbar->addAction("โทนมืด");
    connect(theme, &QAction::triggered, this, &MainWindow::toggleTheme);

    auto *progress = new QProgressBar(this);
    progress->setMaximumWidth(100);
    progress->setTextVisible(false);
    toolbar->addWidget(progress);

    connect(webView_, &QWebEngineView::urlChanged, this, &MainWindow::updateUrl);
    connect(webView_, &QWebEngineView::titleChanged, this, &MainWindow::updateTitle);
    connect(webView_, &QWebEngineView::loadProgress, progress, &QProgressBar::setValue);

    setCentralWidget(webView_);

    const QString homePage = QStringLiteral(R"HTML(
<!doctype html><html lang="th"><head><meta charset="utf-8"><title>LiteWave</title>
<style>
*{box-sizing:border-box}body{margin:0;background:#f2f4f6;color:#20252b;font-family:Arial,sans-serif}
.top{height:64px;background:#fff;border-bottom:1px solid #dfe3e7;display:flex;align-items:center;padding:0 34px}
.brand{font-size:24px;font-weight:700;color:#1688c0}.wrap{max-width:920px;margin:75px auto;padding:0 28px}
h1{font-size:31px;font-weight:500;margin:0 0 8px}.desc{color:#69747e;margin:0 0 28px}
.search{display:flex;background:#fff;border:1px solid #cbd3d9;border-radius:7px;padding:5px;box-shadow:0 2px 7px #17212b12}
input{flex:1;border:0;outline:0;font-size:16px;padding:13px}.search button{border:0;border-radius:5px;padding:0 22px;background:#1688c0;color:white;font-size:15px}
h2{font-size:18px;font-weight:500;margin:38px 0 14px}.sites{display:grid;grid-template-columns:repeat(4,1fr);gap:14px}
.site{background:#fff;border:1px solid #e0e5e9;border-radius:7px;padding:22px 14px;text-align:center;cursor:pointer;min-height:105px}
.site:hover{border-color:#79b8d4}.icon{font-size:25px;color:#1688c0;margin-bottom:9px}.site span{display:block;font-size:14px}
.note{color:#69747e;font-size:13px;margin-top:30px}
</style></head><body><header class="top"><div class="brand">LiteWave</div></header>
<main class="wrap"><h1>เริ่มต้นใช้งาน</h1><p class="desc">ค้นหาเว็บได้เร็วขึ้น พร้อม Shield ช่วยลดโฆษณารบกวน</p>
<form class="search" onsubmit="go(event)"><input id="q" autofocus placeholder="ค้นหาหรือพิมพ์ที่อยู่เว็บไซต์"><button>ค้นหา</button></form>
<h2>เว็บที่ปักหมุด</h2><div class="sites">
<div class="site" onclick="openSite('https://www.google.com')"><div class="icon">G</div><span>Google</span></div>
<div class="site" onclick="openSite('https://www.youtube.com')"><div class="icon">▶</div><span>YouTube</span></div>
<div class="site" onclick="openSite('https://github.com')"><div class="icon">⌘</div><span>GitHub</span></div>
<div class="site" onclick="ask()"><div class="icon">?</div><span>AI ช่วยค้นหา</span></div>
</div><div class="note">LiteWave 0.2 · Windows + Fedora · Chrome engine + Brave-style Shield</div></main>
<script>
function openSite(u){location.href=u}function ask(){q.value='ช่วยค้นหา ';q.focus()}
function go(e){e.preventDefault();let x=q.value.trim();if(!x)return;let u=/^(https?:\\/\\/|localhost|[a-z0-9-]+\\.[a-z]{2,})/i.test(x)?(x.match(/^https?:\\/\\//i)?x:'https://'+x):'https://www.google.com/search?q='+encodeURIComponent(x);location.href=u}
</script></body></html>
)HTML");
    webView_->setHtml(homePage, QUrl("https://litewave.home/"));
}

void MainWindow::navigate()
{
    openUrl(urlBar_->text());
}

void MainWindow::openUrl(const QString &text)
{
    const QString input = text.trimmed();
    if (input.isEmpty()) return;

    QUrl url = QUrl::fromUserInput(input);
    if (!url.isValid()) return;
    if (url.scheme().isEmpty() || url.scheme() == "search")
        url = QUrl("https://www.google.com/search?q=" + QUrl::toPercentEncoding(input));
    webView_->setUrl(url);
}

void MainWindow::updateUrl(const QUrl &url)
{
    urlBar_->setText(url.toString());
    urlBar_->setCursorPosition(0);
}

void MainWindow::updateTitle(const QString &title)
{
    setWindowTitle(title.isEmpty() ? "LiteWave" : title + " - LiteWave");
}

void MainWindow::toggleTheme()
{
    darkMode_ = !darkMode_;
    applyTheme();
}

void MainWindow::applyTheme()
{
    if (darkMode_) {
        qApp->setStyleSheet("QToolBar{background:#18222d;color:#fff;border:0;padding:5px} QLineEdit{background:#273544;color:#fff;border:1px solid #506070;padding:6px;border-radius:5px} QMainWindow{background:#111820} QToolButton{color:#fff;padding:5px}");
    } else {
        qApp->setStyleSheet("");
    }
}
