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
      webView_(new QWebEngineView(this)),
      adBlocker_(new AdBlocker(QWebEngineProfile::defaultProfile()))
{
    setWindowTitle("LiteWave");
    resize(1280, 820);

    auto *profile = QWebEngineProfile::defaultProfile();
    profile->setHttpUserAgent(profile->httpUserAgent() + " LiteWave/0.2");
    profile->setUrlRequestInterceptor(adBlocker_);

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
    connect(shield, &QAction::toggled, this, [this, shield](bool enabled) {
        adBlocker_->setEnabled(enabled);
        shield->setText(enabled ? "Shield: เปิด" : "Shield: ปิด");
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
    webView_->setUrl(QUrl("https://www.google.com"));
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
