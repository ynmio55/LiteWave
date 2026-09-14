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
body{margin:0;background:linear-gradient(135deg,#08111f,#12345b);color:#fff;font-family:Arial,sans-serif;height:100vh;display:grid;place-items:center}
.card{text-align:center;width:min(680px,90vw)}
.logo{font-size:64px;font-weight:800;letter-spacing:-4px;color:#62d8ff}
.sub{color:#b9d5ea;font-size:18px;margin:8px 0 34px}
.search{display:flex;gap:10px;background:#fff;padding:8px;border-radius:16px;box-shadow:0 12px 35px #0005}
input{flex:1;border:0;outline:0;font-size:17px;padding:12px;border-radius:10px}
button{border:0;border-radius:10px;padding:0 24px;background:#159bd7;color:white;font-size:16px;cursor:pointer}
.links{margin-top:28px;display:flex;justify-content:center;gap:22px}
a{color:#9fe8ff;text-decoration:none}
small{display:block;margin-top:50px;color:#81a7c0}
</style>
</head>
<body>
<div class="card">
  <div class="logo">LiteWave</div>
  <div class="sub">เบราว์เซอร์เรียบง่าย เบา และเป็นของเราเอง</div>
  <form class="search" onsubmit="go(event)">
    <input id="q" autofocus placeholder="ค้นหาเว็บหรือใส่ URL...">
    <button type="submit">ค้นหา</button>
  </form>
  <div class="links">
    <a href="https://www.google.com">Google</a>
    <a href="https://www.youtube.com">YouTube</a>
    <a href="https://github.com">GitHub</a>
  </div>
  <small>LiteWave Browser · Windows + Fedora</small>
</div>
<script>
function go(e){
  e.preventDefault();
  const q=document.getElementById('q').value.trim();
  if(!q)return;
  const looksLikeUrl=/^(https?:\\/\\/|localhost|[a-z0-9-]+\\.[a-z]{2,})/i.test(q);
  location.href=looksLikeUrl?(q.match(/^https?:\\/\\//i)?q:'https://'+q):'https://www.google.com/search?q='+encodeURIComponent(q);
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
