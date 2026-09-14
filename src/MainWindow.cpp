#include "MainWindow.h"

#include <QAction>
#include <QLineEdit>
#include <QProgressBar>
#include <QToolBar>
#include <QUrl>
#include <QWebEngineView>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      urlBar_(new QLineEdit(this)),
      webView_(new QWebEngineView(this))
{
    setWindowTitle("LiteWave");
    resize(1200, 780);

    auto *toolbar = addToolBar("Navigation");
    toolbar->setMovable(false);

    auto *back = toolbar->addAction("←");
    back->setToolTip("ย้อนกลับ");
    connect(back, &QAction::triggered, webView_, &QWebEngineView::back);

    auto *forward = toolbar->addAction("→");
    forward->setToolTip("ไปข้างหน้า");
    connect(forward, &QAction::triggered, webView_, &QWebEngineView::forward);

    auto *reload = toolbar->addAction("รีเฟรช");
    connect(reload, &QAction::triggered, webView_, &QWebEngineView::reload);

    urlBar_->setPlaceholderText("พิมพ์ที่อยู่เว็บไซต์หรือคำค้นหา...");
    urlBar_->setClearButtonEnabled(true);
    toolbar->addWidget(urlBar_);
    connect(urlBar_, &QLineEdit::returnPressed, this, &MainWindow::navigate);

    auto *progress = new QProgressBar(this);
    progress->setMaximumWidth(120);
    progress->setTextVisible(false);
    progress->setRange(0, 100);
    toolbar->addWidget(progress);

    connect(webView_, &QWebEngineView::urlChanged, this, &MainWindow::updateUrl);
    connect(webView_, &QWebEngineView::titleChanged, this, &MainWindow::updateTitle);
    connect(webView_, &QWebEngineView::loadProgress, progress, &QProgressBar::setValue);

    setCentralWidget(webView_);

    const QString homePage = QStringLiteral(R"HTML(
<!doctype html>
<html lang="th">
<head>
<meta charset="utf-8">
<title>LiteWave</title>
<style>
*{box-sizing:border-box}
body{margin:0;background:#f3f5f7;color:#20252b;font-family:Arial,sans-serif}
.top{height:64px;background:#fff;border-bottom:1px solid #dfe3e7;display:flex;align-items:center;padding:0 34px;gap:20px}
.brand{font-size:23px;font-weight:700;color:#1688c0;letter-spacing:-.5px}
.menu{margin-left:auto;color:#69747e;font-size:14px}
main{max-width:920px;margin:70px auto;padding:0 28px}
h1{font-size:30px;font-weight:500;margin:0 0 8px;color:#20252b}
.desc{color:#69747e;margin:0 0 28px}
.search{display:flex;background:#fff;border:1px solid #cbd3d9;border-radius:7px;padding:5px;box-shadow:0 2px 7px #17212b12}
input{flex:1;border:0;outline:0;font-size:16px;padding:13px 14px;color:#20252b}
button{border:0;border-radius:5px;padding:0 22px;background:#1688c0;color:white;font-size:15px;cursor:pointer}
.section{display:flex;align-items:center;justify-content:space-between;margin:38px 0 14px}
.section h2{font-size:18px;font-weight:500;margin:0}
.add{background:transparent;color:#1688c0;border:1px solid #b7c7d1;padding:8px 13px}
.sites{display:grid;grid-template-columns:repeat(4,1fr);gap:14px}
.site{position:relative;background:#fff;border:1px solid #e0e5e9;border-radius:7px;padding:20px 14px;text-align:center;cursor:pointer;min-height:102px}
.site:hover{border-color:#79b8d4;background:#fbfdfe}
.icon{font-size:25px;color:#1688c0;margin-bottom:9px}
.site span{display:block;font-size:14px;color:#37424b}
.pin{position:absolute;right:8px;top:7px;color:#1688c0;font-size:13px}
.help{margin-top:34px;border-top:1px solid #dfe3e7;padding-top:18px;color:#69747e;font-size:14px}
.help a{color:#1688c0;text-decoration:none;margin-left:8px}
@media(max-width:650px){main{margin:35px auto}.sites{grid-template-columns:repeat(2,1fr)}.top{padding:0 18px}.menu{display:none}}
</style>
</head>
<body>
<header class="top"><div class="brand">LiteWave</div><div class="menu">เบราว์เซอร์ของคุณ · Windows + Fedora</div></header>
<main>
  <h1>เริ่มต้นใช้งาน</h1>
  <p class="desc">ค้นหาเว็บหรือเปิดเว็บไซต์ที่คุณต้องการ</p>
  <form class="search" onsubmit="go(event)">
    <input id="q" autofocus placeholder="ค้นหาหรือพิมพ์ที่อยู่เว็บไซต์">
    <button type="submit">ค้นหา</button>
  </form>
  <div class="section"><h2>เว็บที่ปักหมุด</h2><button class="add" onclick="addSite()">+ เพิ่มเว็บ</button></div>
  <div class="sites">
    <div class="site" onclick="openSite('https://www.google.com')"><div class="pin">📌</div><div class="icon">G</div><span>Google</span></div>
    <div class="site" onclick="openSite('https://www.youtube.com')"><div class="pin">📌</div><div class="icon">▶</div><span>YouTube</span></div>
    <div class="site" onclick="openSite('https://github.com')"><div class="pin">📌</div><div class="icon">⌘</div><span>GitHub</span></div>
    <div class="site" onclick="askAI()"><div class="icon">?</div><span>AI ช่วยค้นหา</span></div>
  </div>
  <div class="help">ต้องการความช่วยเหลือหรือไม่?<a href="https://github.com/ynmio55/LiteWave">ดูคู่มือ LiteWave</a></div>
</main>
<script>
function openSite(url){location.href=url}
function go(e){
  e.preventDefault();
  const q=document.getElementById('q').value.trim();
  if(!q)return;
  const looksLikeUrl=/^(https?:\\/\\/|localhost|[a-z0-9-]+\\.[a-z]{2,})/i.test(q);
  location.href=looksLikeUrl?(q.match(/^https?:\\/\\//i)?q:'https://'+q):'https://www.google.com/search?q='+encodeURIComponent(q);
}
function askAI(){
  document.getElementById('q').value='ช่วยค้นหา ';
  document.getElementById('q').focus();
}
function addSite(){
  const url=prompt('ใส่ URL เว็บไซต์ที่ต้องการปักหมุด');
  if(url) openSite(url);
}
</script>
</body>
</html>
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

    if (url.scheme().isEmpty() || url.scheme() == "search") {
        url = QUrl("https://www.google.com/search?q=" + QUrl::toPercentEncoding(input));
    }

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
