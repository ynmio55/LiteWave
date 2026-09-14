#include "MainWindow.h"

#include <QAction>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLineEdit>
#include <QProgressBar>
#include <QToolBar>
#include <QUrl>
#include <QWebEngineView>
#include <QWidget>

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
