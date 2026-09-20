#include "MainWindow.h"
#include "AdBlocker.h"
#include "SearchEngineManager.h"
#include "FindBar.h"
#include "SearchSuggestionPopup.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QKeyEvent>
#include <QResizeEvent>
#include <QCheckBox>
#include <QComboBox>
#include <QCompleter>
#include <QDateTime>
#include <QDesktopServices>
#include <QDialog>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPointer>
#include <QPrintDialog>
#include <QPrinter>
#include <QProgressBar>
#include <QPushButton>
#include <QProgressDialog>
#include <QProcess>
#include <QDesktopServices>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QScrollArea>
#include <QRadioButton>
#include <QRegularExpression>
#include <QSettings>
#include <QShortcut>
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QSplitter>
#include <QStandardPaths>
#include <QStatusBar>
#include <QStringListModel>
#include <QTabBar>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QUrlQuery>
#include <QUuid>
#include <QVBoxLayout>
#include <QWebEngineCertificateError>
#include <QWebEngineDownloadRequest>
#include <QWebEngineFullScreenRequest>
#include <QWebEngineHistory>
#include <QWebEngineNewWindowRequest>
#include <QWebEnginePage>
#include <QWindow>
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
#include <QWebEnginePermission>
#endif
#include <QWebEngineProfile>
#include <QWebEngineScript>
#include <QWebEngineScriptCollection>
#include <QWebEngineSettings>
#include <QWebEngineView>
#include <algorithm>

#ifndef LITEWAVE_APP_VERSION
#define LITEWAVE_APP_VERSION "1.2.1"
#endif

#ifndef LITEWAVE_GIT_COMMIT
#define LITEWAVE_GIT_COMMIT ""
#endif

static QList<int> parseVersionNumbers(const QString &str, QString *cleanedStr = nullptr) {
  QRegularExpression re(R"(\bv?(\d+)\.(\d+)(?:\.(\d+))?\b)", QRegularExpression::CaseInsensitiveOption);
  auto match = re.match(str);
  if (match.hasMatch()) {
    int major = match.captured(1).toInt();
    int minor = match.captured(2).toInt();
    int patch = match.captured(3).isEmpty() ? 0 : match.captured(3).toInt();
    if (cleanedStr) {
      *cleanedStr = QString("%1.%2.%3").arg(major).arg(minor).arg(patch);
    }
    return {major, minor, patch};
  }
  if (cleanedStr) {
    *cleanedStr = str;
  }
  return {0, 0, 0};
}

static QIcon createMenuIcon(const QString &name, const QColor &color);

static QIcon createToolbarIcon(const QString &name, const QColor &color) {
  QPixmap pix(20, 20);
  pix.fill(Qt::transparent);
  QPainter p(&pix);
  p.setRenderHint(QPainter::Antialiasing);
  p.setPen(QPen(color, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  p.setBrush(Qt::NoBrush);

  if (name == "lock_https") {
    // Lock positioned higher in 20x20 canvas
    p.drawRoundedRect(4, 6, 12, 9, 2, 2);
    p.drawArc(6, 1, 8, 8, 0, 180 * 16);
    p.drawLine(6, 5, 6, 6);
    p.drawLine(14, 5, 14, 6);
    p.setBrush(color);
    p.drawEllipse(9, 9, 2, 2);
  } else if (name == "lock_http") {
    QPainterPath path;
    path.moveTo(10, 1);
    path.lineTo(18, 15);
    path.lineTo(2, 15);
    path.closeSubpath();
    p.drawPath(path);
    p.drawLine(10, 6, 10, 10);
    p.drawPoint(10, 13);
  } else if (name == "moon") {
    p.setBrush(color);
    QPainterPath path;
    path.moveTo(14, 3);
    path.cubicTo(8, 3, 4, 7, 4, 13);
    path.cubicTo(4, 17, 7, 19, 11, 19);
    path.cubicTo(8, 16, 8, 10, 14, 3);
    path.closeSubpath();
    p.drawPath(path);
  } else if (name == "sun") {
    p.setPen(QPen(color, 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.drawEllipse(6, 6, 8, 8);
    p.drawLine(10, 2, 10, 4);
    p.drawLine(10, 16, 10, 18);
    p.drawLine(2, 10, 4, 10);
    p.drawLine(16, 10, 18, 10);
    p.drawLine(4, 4, 6, 6);
    p.drawLine(14, 14, 16, 16);
    p.drawLine(4, 16, 6, 14);
    p.drawLine(14, 6, 16, 4);
  } else if (name == "globe") {
    p.drawEllipse(3, 3, 14, 14);
    p.drawLine(3, 10, 17, 10);
    p.drawArc(6, 3, 8, 14, 0, 360 * 16);
  } else if (name == "shield") {
    QPainterPath path;
    path.moveTo(10, 3);
    path.lineTo(16, 5.5);
    path.lineTo(16, 10.5);
    path.cubicTo(16, 14.5, 10, 17.5, 10, 17.5);
    path.cubicTo(10, 17.5, 4, 14.5, 4, 10.5);
    path.lineTo(4, 5.5);
    path.closeSubpath();
    p.drawPath(path);
  } else if (name == "menu") {
    // Crisp modern vector hamburger icon with rounded caps
    p.setPen(QPen(color, 2.0, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(3, 5, 17, 5);
    p.drawLine(3, 10, 17, 10);
    p.drawLine(3, 15, 17, 15);
  }

  return QIcon(pix);
}

static QIcon createWindowControlIcon(const QString &name, const QColor &normalColor, const QColor &hoverColor) {
  auto makePix = [&](const QColor &c) {
    QPixmap pix(32, 32);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(Qt::NoBrush);

    if (name == "win_min") {
      p.setPen(QPen(c, 2.8, Qt::SolidLine, Qt::RoundCap));
      p.drawLine(7, 18, 25, 18);
    } else if (name == "win_max") {
      p.setPen(QPen(c, 2.4, Qt::SolidLine, Qt::SquareCap, Qt::RoundJoin));
      p.drawRoundedRect(7, 7, 18, 18, 2.0, 2.0);
    } else if (name == "win_restore") {
      p.setPen(QPen(c, 2.2, Qt::SolidLine, Qt::SquareCap, Qt::RoundJoin));
      p.drawLine(11, 6, 26, 6);
      p.drawLine(26, 6, 26, 21);
      p.drawLine(21, 21, 26, 21);
      p.drawLine(11, 6, 11, 11);
      p.drawRoundedRect(6, 11, 15, 15, 2.0, 2.0);
    } else if (name == "win_close") {
      p.setPen(QPen(c, 2.6, Qt::SolidLine, Qt::RoundCap));
      p.drawLine(8, 8, 24, 24);
      p.drawLine(24, 8, 8, 24);
    }
    pix.setDevicePixelRatio(2.0);
    return pix;
  };

  QIcon icon;
  icon.addPixmap(makePix(normalColor), QIcon::Normal);
  icon.addPixmap(makePix(hoverColor), QIcon::Active);
  return icon;
}

static bool isSameSiteOrSubdomain(const QUrl &first, const QUrl &second)
{
  const QString firstHost = first.host().toLower();
  const QString secondHost = second.host().toLower();
  if (firstHost.isEmpty() || secondHost.isEmpty())
    return false;

  return firstHost == secondHost ||
         firstHost.endsWith(QStringLiteral(".") + secondHost) ||
         secondHost.endsWith(QStringLiteral(".") + firstHost);
}

class WebPage : public QWebEnginePage {
public:
  WebPage(QWebEngineProfile *profile, MainWindow *mw, QWebEngineView *view,
          QObject *parent = nullptr)
      : QWebEnginePage(profile, parent), mw_(mw), view_(view) {}

protected:
  bool acceptNavigationRequest(const QUrl &url, NavigationType type,
                               bool isMainFrame) override {
    if (url.scheme() == "litewave") {
      if (isMainFrame && mw_ && view_) {
        MainWindow *mw = mw_;
        QPointer<QWebEngineView> v = view_;
        QMetaObject::invokeMethod(
            mw,
            [mw, v, url] {
              if (v)
                mw->handleHomeNavigation(url, v);
            },
            Qt::QueuedConnection);
      }
      return false;
    }
    if (url.host() == "litewave.home") {
      if (isMainFrame && mw_ && view_) {
        MainWindow *mw = mw_;
        QPointer<QWebEngineView> v = view_;
        QMetaObject::invokeMethod(
            mw,
            [mw, v] {
              if (v)
                mw->loadHome(v);
            },
            Qt::QueuedConnection);
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
    : QMainWindow(parent), urlBar_(new QLineEdit(this)),
      tabBar_(new QTabBar(this)), tabStack_(new QStackedWidget(this)),
      progress_(new QProgressBar(this)), toolbar_(nullptr) {
  setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
  setWindowTitle("LiteWave");
  QIcon appIcon;
  appIcon.addFile(":/icons/litewave.png");
  appIcon.addFile(":/icons/litewave.svg");
  setWindowIcon(appIcon);
  resize(1320, 840);

  privateMode_ = privateMode;
  QSettings themeSettings("LiteWave", "LiteWave");
  darkMode_ = themeSettings.value("theme/dark", false).toBool();
  const QString storagePath =
      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
      "/Storage";
  const QString cachePath =
      QStandardPaths::writableLocation(QStandardPaths::CacheLocation) +
      "/Cache";

  static QWebEngineProfile *normalProfile =
      new QWebEngineProfile("LiteWave", qApp);
  static bool profileConfigured = false;
  if (!profileConfigured) {
    profileConfigured = true;
    normalProfile->setHttpCacheType(QWebEngineProfile::DiskHttpCache);
    normalProfile->setHttpCacheMaximumSize(256 * 1024 * 1024); // 256 MB optimal cache
  }
  static AdBlocker *normalShield = new AdBlocker(qApp, true);
  profile_ = privateMode_ ? new QWebEngineProfile(this) : normalProfile;
  if (privateMode_) {
    profile_->setHttpCacheType(QWebEngineProfile::MemoryHttpCache);
    profile_->setHttpCacheMaximumSize(64 * 1024 * 1024);
  }
  adBlocker_ = privateMode_ ? new AdBlocker(this, false) : normalShield;
  profile_->setUrlRequestInterceptor(adBlocker_);

  // Register YouTube Ad-Skipper & Auto-Mute UserScript into profile
  QWebEngineScript ytSkipperScript;
  ytSkipperScript.setName(QStringLiteral("LiteWaveYouTubeAdSkipper"));
  ytSkipperScript.setSourceCode(AdBlocker::youtubeAdSkipScript());
  ytSkipperScript.setInjectionPoint(QWebEngineScript::DocumentCreation);
  ytSkipperScript.setWorldId(QWebEngineScript::MainWorld);
  ytSkipperScript.setRunsOnSubFrames(false);
  profile_->scripts()->insert(ytSkipperScript);
  // Register Google Auth & OAuth UserAgentData Bypass Script
  QWebEngineScript googleAuthScript;
  googleAuthScript.setName(QStringLiteral("LiteWaveGoogleAuthBypass"));
  googleAuthScript.setSourceCode(QStringLiteral(R"JS(
(function() {
    'use strict';
    try {
        Object.defineProperty(navigator, 'vendor', {
            get: function() { return 'Google Inc.'; },
            configurable: true
        });
    } catch(e) {}

    try {
        Object.defineProperty(navigator, 'webdriver', {
            get: function() { return false; },
            configurable: true
        });
    } catch(e) {}

    try {
        if (!window.chrome) {
            window.chrome = {};
        }
        if (!window.chrome.app) {
            window.chrome.app = {
                isInstalled: false,
                getIsInstalled: function() { return false; },
                getDetails: function() { return null; },
                installState: function() { return 'not_installed'; }
            };
        }
        if (!window.chrome.runtime) {
            window.chrome.runtime = {
                OnInstalledReason: { INSTALL: 'install', UPDATE: 'update', CHROME_UPDATE: 'chrome_update', SHARED_MODULE_UPDATE: 'shared_module_update' },
                OnRestartRequiredReason: { APP_UPDATE: 'app_update', OS_UPDATE: 'os_update', PERIODIC: 'periodic' },
                PlatformOs: { MAC: 'mac', WIN: 'win', ANDROID: 'android', CROS: 'cros', LINUX: 'linux', OPENBSD: 'openbsd' },
                connect: function() {},
                sendMessage: function() {}
            };
        }
        if (!window.chrome.csi) window.chrome.csi = function() {};
        if (!window.chrome.loadTimes) window.chrome.loadTimes = function() {};
    } catch(e) {}

    try {
        delete window.qt;
        delete window.QtWebEngine;
        delete window.qWebChannel;
    } catch(e) {}

    try {
        const mockData = {
            brands: [
                { brand: 'Chromium', version: '130' },
                { brand: 'Google Chrome', version: '130' },
                { brand: 'Not?A_Brand', version: '99' }
            ],
            mobile: false,
            platform: 'Linux',
            getHighEntropyValues: function() {
                return Promise.resolve({
                    architecture: 'x86',
                    bitness: '64',
                    brands: [
                        { brand: 'Chromium', version: '130.0.6723.69' },
                        { brand: 'Google Chrome', version: '130.0.6723.69' },
                        { brand: 'Not?A_Brand', version: '99.0.0.0' }
                    ],
                    fullVersionList: [
                        { brand: 'Chromium', version: '130.0.6723.69' },
                        { brand: 'Google Chrome', version: '130.0.6723.69' },
                        { brand: 'Not?A_Brand', version: '99.0.0.0' }
                    ],
                    mobile: false,
                    model: '',
                    platform: 'Linux',
                    platformVersion: '6.5.0',
                    uaFullVersion: '130.0.6723.69'
                });
            }
        };
        Object.defineProperty(navigator, 'userAgentData', {
            get: function() { return mockData; },
            configurable: true,
            enumerable: true
        });
    } catch(e) {}
})();
)JS"));
  googleAuthScript.setInjectionPoint(QWebEngineScript::DocumentCreation);
  googleAuthScript.setWorldId(QWebEngineScript::MainWorld);
  googleAuthScript.setRunsOnSubFrames(false);
  profile_->scripts()->insert(googleAuthScript);

  profile_->setHttpAcceptLanguage("th-TH,th;q=0.9,en-US;q=0.8,en;q=0.7");
#if defined(Q_OS_WIN)
  const QString defaultUA = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/130.0.0.0 Safari/537.36";
#elif defined(Q_OS_MAC)
  const QString defaultUA = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/130.0.0.0 Safari/537.36";
#else
  const QString defaultUA = "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/130.0.0.0 Safari/537.36";
#endif
  profile_->setHttpUserAgent(defaultUA);

  auto *webSettings = profile_->settings();
  webSettings->setAttribute(QWebEngineSettings::Accelerated2dCanvasEnabled, true);
  webSettings->setAttribute(QWebEngineSettings::WebGLEnabled, true);
  webSettings->setAttribute(QWebEngineSettings::ScrollAnimatorEnabled, true);
  webSettings->setAttribute(QWebEngineSettings::PdfViewerEnabled, true);

  if (!privateMode_) {
    profile_->setPersistentStoragePath(storagePath);
    profile_->setCachePath(cachePath);
    profile_->setPersistentCookiesPolicy(
        QWebEngineProfile::AllowPersistentCookies);
    profile_->setHttpCacheType(QWebEngineProfile::DiskHttpCache);
    profile_->setHttpCacheMaximumSize(512 * 1024 * 1024);
  }

  // Main Central Widget
  auto *centralWidget = new QWidget(this);
  auto *mainLayout = new QVBoxLayout(centralWidget);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setSpacing(0);
  setCentralWidget(centralWidget);

  headerWidget_ = new QWidget(this);
  headerWidget_->setObjectName("headerWidget");
  headerWidget_->installEventFilter(this);
  auto *headerLayout = new QVBoxLayout(headerWidget_);
  headerLayout->setContentsMargins(0, 0, 0, 0);
  headerLayout->setSpacing(0);

  // Tab Bar Container (Row 1 at the VERY TOP of the window - CSD Header)
  tabBarContainer_ = new QWidget(this);
  tabBarContainer_->setObjectName("tabBarContainer");
  tabBarContainer_->installEventFilter(this);
  tabBar_->installEventFilter(this);
  auto *tabBarLayout = new QHBoxLayout(tabBarContainer_);
  tabBarLayout->setContentsMargins(6, 4, 6, 0);
  tabBarLayout->setSpacing(4);

  tabBar_->setTabsClosable(true);
  tabBar_->setMovable(true);
  tabBar_->setDrawBase(false);
  tabBar_->setDocumentMode(true);
  tabBar_->setExpanding(false);
  // Keep the tab strip only as wide as its tabs so the + button follows the
  // last visible tab. The strip may still shrink when space runs out, at which
  // point QTabBar's scroll buttons handle overflow.
  tabBar_->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
  tabBar_->setIconSize(QSize(16, 16));
  tabBar_->setUsesScrollButtons(true);
  tabBar_->setElideMode(Qt::ElideRight);

  tabBarLayout->addWidget(tabBar_);

  // Tab search / switcher for crowded tab strips.
  auto *tabSearchBtn = new QToolButton(this);
  tabSearchBtn->setObjectName("tabSearchButton");
  tabSearchBtn->setText("⌄");
  tabSearchBtn->setToolTip("ค้นหาและสลับแท็บ");
  tabSearchBtn->setFixedSize(26, 26);
  tabSearchBtn->setCursor(Qt::PointingHandCursor);
  tabSearchBtn->setPopupMode(QToolButton::InstantPopup);
  auto *tabSearchMenu = new QMenu(tabSearchBtn);
  tabSearchBtn->setMenu(tabSearchMenu);
  connect(tabSearchMenu, &QMenu::aboutToShow, this, [this, tabSearchMenu] {
    tabSearchMenu->clear();

    auto *header = tabSearchMenu->addAction(
        QString("แท็บที่เปิดอยู่ (%1)").arg(tabBar_->count()));
    header->setEnabled(false);
    tabSearchMenu->addSeparator();

    for (int i = 0; i < tabBar_->count(); ++i) {
      auto *view = qobject_cast<QWebEngineView *>(tabStack_->widget(i));
      QString title = tabBar_->tabText(i);
      if (title.endsWith(" ·"))
        title.chop(2);
      if (title.trimmed().isEmpty())
        title = QStringLiteral("แท็บใหม่");

      auto *act = tabSearchMenu->addAction(
          view && !view->icon().isNull() ? view->icon()
                                         : QIcon(":/icons/litewave.png"),
          title);
      act->setCheckable(true);
      act->setChecked(i == tabBar_->currentIndex());
      const int tabIndex = i;
      connect(act, &QAction::triggered, this, [this, tabIndex] {
        if (tabIndex >= 0 && tabIndex < tabBar_->count())
          tabBar_->setCurrentIndex(tabIndex);
      });
    }

    if (!closedTabs_.isEmpty()) {
      tabSearchMenu->addSeparator();
      auto *reopen = tabSearchMenu->addAction("↶ เปิดแท็บที่เพิ่งปิด");
      connect(reopen, &QAction::triggered, this, [this] {
        if (closedTabs_.isEmpty())
          return;
        const QUrl u = closedTabs_.takeLast();
        if (u.isEmpty() || u.host() == "litewave.home" ||
            u.scheme() == "litewave")
          newTab();
        else
          createView(u);
      });
    }
  });
  // '+' New Tab Button right next to the tabs (ชิดแท็บ)
  auto *newTabBtn = new QToolButton(this);
  newTabBtn->setObjectName("newTabButton");
  newTabBtn->setText("+");
  newTabBtn->setToolTip("เปิดแท็บใหม่ (Ctrl+T)");
  newTabBtn->setFixedSize(26, 26);
  newTabBtn->setCursor(Qt::PointingHandCursor);
  connect(newTabBtn, &QToolButton::clicked, this, &MainWindow::newTab);
  tabBarLayout->addWidget(newTabBtn);
  tabBarLayout->addStretch(); // Keeps + button right next to the tabs!
  tabBarLayout->addWidget(tabSearchBtn);

  // Window Controls on far right of Tab Bar Row (Minimize, Maximize/Restore, Close)
  minWinBtn_ = new QToolButton(this);
  minWinBtn_->setObjectName("windowMinButton");
  minWinBtn_->setFixedSize(34, 28);
  minWinBtn_->setCursor(Qt::PointingHandCursor);
  connect(minWinBtn_, &QToolButton::clicked, this, &QMainWindow::showMinimized);

  maxWinBtn_ = new QToolButton(this);
  maxWinBtn_->setObjectName("windowMaxButton");
  maxWinBtn_->setFixedSize(34, 28);
  maxWinBtn_->setCursor(Qt::PointingHandCursor);
  connect(maxWinBtn_, &QToolButton::clicked, this, [this]() {
    if (isMaximized()) {
      showNormal();
    } else {
      showMaximized();
    }
  });

  closeWinBtn_ = new QToolButton(this);
  closeWinBtn_->setObjectName("windowCloseButton");
  closeWinBtn_->setFixedSize(34, 28);
  closeWinBtn_->setCursor(Qt::PointingHandCursor);
  connect(closeWinBtn_, &QToolButton::clicked, this, &QMainWindow::close);

  tabBarLayout->addWidget(minWinBtn_);
  tabBarLayout->addWidget(maxWinBtn_);
  tabBarLayout->addWidget(closeWinBtn_);

  headerLayout->addWidget(tabBarContainer_);

  // Main Navigation Toolbar (Row 2 right under tabs)
  toolbar_ = new QToolBar("LiteWave", this);
  toolbar_->setObjectName("mainToolbar");
  toolbar_->setMovable(false);
  toolbar_->setFloatable(false);
  toolbar_->setToolButtonStyle(Qt::ToolButtonTextOnly);

  auto addNavAction = [this](const QString &text, const QString &tooltip) {
    auto *action = toolbar_->addAction(text);
    action->setToolTip(tooltip);
    return action;
  };

  auto *back = addNavAction("←", "ย้อนกลับ (Alt+Left)");
  back->setObjectName("navBackAction");
  back->setEnabled(false);
  connect(back, &QAction::triggered, this, [this] {
    if (auto *view = currentView(); view && view->history()->canGoBack())
      view->back();
  });

  auto *forward = addNavAction("→", "ถัดไป (Alt+Right)");
  forward->setObjectName("navForwardAction");
  forward->setEnabled(false);
  connect(forward, &QAction::triggered, this, [this] {
    if (auto *view = currentView(); view && view->history()->canGoForward())
      view->forward();
  });

  auto *reload = addNavAction("↻", "รีเฟรชหน้าเว็บ (Ctrl+R)");
  connect(reload, &QAction::triggered, this, [this] { reloadCurrentView(); });

  auto *home = addNavAction("⌂", "ไปยังหน้าแรก (Alt+Home)");
  connect(home, &QAction::triggered, this, [this] {
    if (currentView())
      loadHome(currentView());
  });

  // Omnibox Address Bar with integrated leading SSL Icon
  sslAction_ = urlBar_->addAction(createToolbarIcon("globe", darkMode_ ? QColor("#9ca3af") : QColor("#64748b")),
                                  QLineEdit::LeadingPosition);
  sslAction_->setToolTip("LiteWave Dashboard");

  urlBar_->setPlaceholderText("ค้นหาเว็บหรือพิมพ์ URL");
  urlBar_->setToolTip("ค้นหาเว็บหรือพิมพ์ที่อยู่เว็บไซต์ (Ctrl+L)");
  urlBar_->setClearButtonEnabled(true);
  urlBar_->setMinimumHeight(34);
  toolbar_->addWidget(urlBar_);
  connect(urlBar_, &QLineEdit::returnPressed, this, &MainWindow::navigate);
  setupUrlBarCompleter();

  urlBar_->installEventFilter(this);
  suggestionPopup_ = new SearchSuggestionPopup(this);
  suggestionPopup_->attachTo(urlBar_);
  suggestionPopup_->setDarkMode(darkMode_);

  connect(urlBar_, &QLineEdit::textEdited, this, [this](const QString &text) {
    QSettings st("LiteWave", "LiteWave");
    if (!st.value("search/enableSuggestions", true).toBool()) {
      if (suggestionPopup_) suggestionPopup_->hide();
      return;
    }
    if (suggestionPopup_) {
      suggestionPopup_->queryChanged(text);
    }
  });

  connect(suggestionPopup_, &SearchSuggestionPopup::suggestionSelected, this,
          [this](const QString &queryOrUrl, bool isDirectUrl) {
            if (isDirectUrl) {
              openUrl(queryOrUrl);
            } else {
              const QUrl searchUrl = SearchEngineManager::instance().buildSearchUrl(queryOrUrl);
              openUrl(searchUrl.toString());
            }
          });

  bookmarkAction_ = addNavAction("☆", "บันทึกหน้าเว็บนี้ (Ctrl+D)");
  connect(bookmarkAction_, &QAction::triggered, this, &MainWindow::addBookmark);

  // Shield has one clear menu: Standard, Aggressive, Off, plus a per-site
  // compatibility escape hatch. It reports only requests actually blocked.
  shieldBtn_ = new QToolButton(this);
  shieldBtn_->setObjectName("shieldButton");
  shieldBtn_->setPopupMode(QToolButton::InstantPopup);
  shieldBtn_->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  shieldBtn_->setMinimumWidth(80);
  shieldBtn_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
  shieldBtn_->setCursor(Qt::PointingHandCursor);
  auto *shieldMenu = new QMenu(shieldBtn_);
  shieldBtn_->setMenu(shieldMenu);
  connect(shieldMenu, &QMenu::aboutToShow, this, [this, shieldMenu] {
    shieldMenu->clear();
    if (auto *oldModeGroup =
            shieldMenu->findChild<QActionGroup *>("shieldModeGroup")) {
      oldModeGroup->deleteLater();
    }
    if (!adBlocker_)
      return;

    auto *modeGroup = new QActionGroup(shieldMenu);
    modeGroup->setObjectName("shieldModeGroup");
    modeGroup->setExclusive(true);

    auto *standardAction =
        shieldMenu->addAction("Shield มาตรฐาน — บล็อกโฆษณา third-party");
    standardAction->setCheckable(true);
    standardAction->setActionGroup(modeGroup);
    standardAction->setChecked(adBlocker_->isEnabled() &&
                               adBlocker_->mode() == AdBlocker::Mode::Standard);
    connect(standardAction, &QAction::triggered, this, [this] {
      adBlocker_->setMode(AdBlocker::Mode::Standard);
      adBlocker_->setEnabled(true);
      refreshShieldUi();
      reloadCurrentView();
    });

    auto *aggressiveAction =
        shieldMenu->addAction("Shield เข้มงวด — รวม tracker (บางเว็บอาจต้องปิด)");
    aggressiveAction->setCheckable(true);
    aggressiveAction->setActionGroup(modeGroup);
    aggressiveAction->setChecked(adBlocker_->isEnabled() &&
                                 adBlocker_->mode() == AdBlocker::Mode::Aggressive);
    connect(aggressiveAction, &QAction::triggered, this, [this] {
      adBlocker_->setMode(AdBlocker::Mode::Aggressive);
      adBlocker_->setEnabled(true);
      refreshShieldUi();
      reloadCurrentView();
    });

    auto *offAction = shieldMenu->addAction("ปิด Shield ทั้งหมด");
    offAction->setCheckable(true);
    offAction->setActionGroup(modeGroup);
    offAction->setChecked(!adBlocker_->isEnabled());
    connect(offAction, &QAction::triggered, this, [this] {
      adBlocker_->setEnabled(false);
      refreshShieldUi();
      reloadCurrentView();
    });

    shieldMenu->addSeparator();
    const QUrl pageUrl = currentView() ? currentView()->url() : QUrl();
    const QString host = pageUrl.host();
    const bool canConfigureSite =
        pageUrl.scheme() == "http" || pageUrl.scheme() == "https";
    const bool siteAllowed = canConfigureSite && adBlocker_->isSiteAllowed(pageUrl);
    auto *siteAction = shieldMenu->addAction(
        siteAllowed ? "เปิด Shield สำหรับ " + host
                    : "ปิด Shield สำหรับ " + host);
    siteAction->setEnabled(canConfigureSite);
    connect(siteAction, &QAction::triggered, this, [this, pageUrl, siteAllowed] {
      adBlocker_->setSiteAllowed(pageUrl, !siteAllowed);
      refreshShieldUi();
      reloadCurrentView();
    });

    auto *resetCountAction = shieldMenu->addAction("รีเซ็ตจำนวนที่บล็อก");
    connect(resetCountAction, &QAction::triggered, this, [this] {
      adBlocker_->resetBlockedCount();
      refreshShieldUi();
    });

    shieldMenu->addSeparator();
    auto *shieldSettingsAction = shieldMenu->addAction("ตั้งค่า Shield...");
    connect(shieldSettingsAction, &QAction::triggered, this,
            &MainWindow::showSettingsDialog);
  });
  toolbar_->addWidget(shieldBtn_);

  // Downloads Button (Toolbar indicator & flyout trigger)
  downloadsBtn_ = new QToolButton(this);
  downloadsBtn_->setObjectName("downloadsButton");
  downloadsBtn_->setText("⭳");
  downloadsBtn_->setToolTip("รายการดาวน์โหลด (Ctrl+J)");
  downloadsBtn_->setCursor(Qt::PointingHandCursor);
  connect(downloadsBtn_, &QToolButton::clicked, this, &MainWindow::showDownloadPopup);
  toolbar_->addWidget(downloadsBtn_);

  // Theme Toggle Button
  themeBtn_ = new QToolButton(this);
  themeBtn_->setObjectName("themeButton");
  themeBtn_->setToolTip("สลับโหมดมืด/สว่าง");
  themeBtn_->setCursor(Qt::PointingHandCursor);
  connect(themeBtn_, &QToolButton::clicked, this, &MainWindow::toggleTheme);
  toolbar_->addWidget(themeBtn_);

  // Main Menu Button (Brave style main menu)
  menuBtn_ = new QToolButton(this);
  menuBtn_->setObjectName("mainMenuButton");
  menuBtn_->setIcon(createToolbarIcon("menu", darkMode_ ? QColor("#f1f5f9") : QColor("#475569")));
  menuBtn_->setIconSize(QSize(20, 20));
  menuBtn_->setToolTip("เมนูหลัก (LiteWave)");
  menuBtn_->setPopupMode(QToolButton::InstantPopup);
  menuBtn_->setCursor(Qt::PointingHandCursor);
  menuBtn_->setMenu(createMainMenu());
  toolbar_->addWidget(menuBtn_);

  auto *shieldRefreshTimer = new QTimer(this);
  shieldRefreshTimer->setInterval(400);
  connect(shieldRefreshTimer, &QTimer::timeout, this,
          &MainWindow::refreshShieldUi);
  shieldRefreshTimer->start();
  refreshShieldUi();

  // 2px Slim Progress Line (Row 3 right below toolbar, 0 spacing)
  progress_->setMaximumHeight(2);
  progress_->setMinimumHeight(2);
  progress_->setTextVisible(false);
  progress_->setRange(0, 100);
  progress_->hide();

  headerLayout->addWidget(toolbar_);
  headerLayout->addWidget(progress_);

  loadBookmarks();

  // Assemble Main Layout
  mainLayout->addWidget(headerWidget_);
  findBar_ = new FindBar(this);
  findBar_->setDarkMode(darkMode_);
  mainLayout->addWidget(findBar_);

  mainSplitter_ = new QSplitter(Qt::Vertical, this);
  mainSplitter_->setChildrenCollapsible(false);
  mainSplitter_->addWidget(tabStack_);

  devToolsContainer_ = new QWidget(mainSplitter_);
  devToolsContainer_->setObjectName("devToolsContainer");
  auto *devLayout = new QVBoxLayout(devToolsContainer_);
  devLayout->setContentsMargins(0, 0, 0, 0);
  devLayout->setSpacing(0);

  auto *devHeader = new QWidget(devToolsContainer_);
  devHeader->setObjectName("devToolsHeader");
  devHeader->setFixedHeight(30);
  auto *devHeaderLayout = new QHBoxLayout(devHeader);
  devHeaderLayout->setContentsMargins(10, 0, 8, 0);
  devHeaderLayout->setSpacing(8);

  devToolsTitleLabel_ = new QLabel("🛠️ เครื่องมือนักพัฒนา (DevTools)", devHeader);
  devToolsTitleLabel_->setObjectName("devToolsTitleLabel");
  devHeaderLayout->addWidget(devToolsTitleLabel_);
  devHeaderLayout->addStretch();

  auto *undockBtn = new QToolButton(devHeader);
  undockBtn->setObjectName("devToolsUndockBtn");
  undockBtn->setText("⧉");
  undockBtn->setToolTip("เปิดในหน้าต่างแยก (Undock)");
  undockBtn->setFixedSize(24, 22);
  undockBtn->setCursor(Qt::PointingHandCursor);
  connect(undockBtn, &QToolButton::clicked, this, [this] {
    openDevToolsUndocked(currentView());
  });
  devHeaderLayout->addWidget(undockBtn);

  auto *closeDevBtn = new QToolButton(devHeader);
  closeDevBtn->setObjectName("devToolsCloseBtn");
  closeDevBtn->setText("✕");
  closeDevBtn->setToolTip("ปิด DevTools (F12)");
  closeDevBtn->setFixedSize(24, 22);
  closeDevBtn->setCursor(Qt::PointingHandCursor);
  connect(closeDevBtn, &QToolButton::clicked, this, [this] {
    toggleDevTools();
  });
  devHeaderLayout->addWidget(closeDevBtn);

  devLayout->addWidget(devHeader);

  devToolsView_ = new QWebEngineView(devToolsContainer_);
  devLayout->addWidget(devToolsView_);

  mainSplitter_->addWidget(devToolsContainer_);
  devToolsContainer_->hide();

  mainLayout->addWidget(mainSplitter_);

  loadDownloadRecords();

  connect(profile_, &QWebEngineProfile::downloadRequested, this,
          &MainWindow::handleDownloadRequested);

  tabBar_->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(tabBar_, &QTabBar::customContextMenuRequested, this,
          &MainWindow::showTabContextMenu);

  connect(tabBar_, &QTabBar::currentChanged, tabStack_,
          &QStackedWidget::setCurrentIndex);
  connect(tabBar_, &QTabBar::currentChanged, this, [this](int) {
    if (currentView()) {
      wakeTab(currentView());
      tabLastActiveTime_[currentView()] = QDateTime::currentMSecsSinceEpoch();
      updateCurrentUrl(currentView()->url());
      if (auto *back = findChild<QAction *>("navBackAction"))
        back->setEnabled(currentView()->history()->canGoBack());
      if (auto *forward = findChild<QAction *>("navForwardAction"))
        forward->setEnabled(currentView()->history()->canGoForward());
      setWindowTitle(currentView()->title() +
                     (privateMode_ ? " — Private · LiteWave" : " — LiteWave"));
      if (findBar_) {
        findBar_->attachView(currentView());
      }
      if (devToolsContainer_ && devToolsContainer_->isVisible()) {
        currentView()->page()->setDevToolsPage(devToolsView_->page());
        if (devToolsTitleLabel_) {
          devToolsTitleLabel_->setText(QStringLiteral("🛠️ DevTools — %1").arg(
              currentView()->title().isEmpty() ? "LiteWave" : currentView()->title()));
        }
      }
    }
    progress_->hide();
  });
  connect(tabBar_, &QTabBar::tabCloseRequested, this, &MainWindow::closeTab);
  connect(tabBar_, &QTabBar::tabMoved, this, [this](int from, int to) {
    QWidget *w = tabStack_->widget(from);
    tabStack_->removeWidget(w);
    tabStack_->insertWidget(to, w);
    tabStack_->setCurrentIndex(to);
  });

  // Sleeping Tabs Manager: check every 60s to freeze/discard inactive background tabs.
  // Session autosave runs independently so an unexpected process/OS shutdown loses
  // at most a small amount of tab-state history.
  tabSleepTimer_ = new QTimer(this);
  connect(tabSleepTimer_, &QTimer::timeout, this, &MainWindow::checkSleepingTabs);
  tabSleepTimer_->start(60000);

  if (!privateMode_) {
    auto *sessionSaveTimer = new QTimer(this);
    sessionSaveTimer->setInterval(30000);
    connect(sessionSaveTimer, &QTimer::timeout, this, &MainWindow::saveSession);
    sessionSaveTimer->start();
  }

  setupShortcuts();
  applyTheme();

  QSettings startupSettings("LiteWave", "LiteWave");
  statusBar()->setVisible(startupSettings.value("showStatusBar", true).toBool());

  const QString startupOpt = startupSettings.value("startupOption", "home").toString();
  const bool previousCleanExit =
      startupSettings.value("session/cleanExit", true).toBool();
  if (!privateMode_) {
    // Mark the session dirty as soon as the window starts. A normal close will
    // flip it back to true; if the process/OS crashes it remains false.
    startupSettings.setValue("session/cleanExit", false);
    startupSettings.sync();
  }

  if (!privateMode_ && startupOpt == "restore") {
    const QStringList savedTabs =
        startupSettings.value("session/openTabs").toStringList();
    bool shouldRestore = !savedTabs.isEmpty();

    if (!previousCleanExit && !savedTabs.isEmpty()) {
      QMessageBox recoveryBox(this);
      recoveryBox.setWindowTitle("กู้คืนเซสชัน LiteWave");
      recoveryBox.setIcon(QMessageBox::Information);
      recoveryBox.setText("<b>LiteWave ปิดไม่สมบูรณ์ในครั้งก่อน</b>");
      recoveryBox.setInformativeText(
          QString("พบ %1 แท็บจากเซสชันก่อนหน้า ต้องการกู้คืนหรือไม่?")
              .arg(savedTabs.size()));
      auto *restoreBtn =
          recoveryBox.addButton("กู้คืนแท็บ", QMessageBox::AcceptRole);
      recoveryBox.addButton("เริ่มใหม่", QMessageBox::RejectRole);
      recoveryBox.exec();
      shouldRestore = recoveryBox.clickedButton() == restoreBtn;
    }

    if (shouldRestore)
      restoreSession();
    if (tabStack_->count() == 0)
      newTab();
  } else if (!privateMode_ && startupOpt == "custom") {
    const QString customUrl =
        startupSettings.value("customStartupUrl", "").toString().trimmed();
    if (!customUrl.isEmpty()) {
      createView(QUrl::fromUserInput(customUrl));
    } else {
      newTab();
    }
  } else {
    newTab();
  }
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event) {
  if (watched == urlBar_) {
    if (event->type() == QEvent::KeyPress) {
      auto *keyEvent = static_cast<QKeyEvent *>(event);
      if (suggestionPopup_ && suggestionPopup_->handleKeyPress(keyEvent->key())) {
        return true;
      }
    }
  }

  if (watched == tabBarContainer_ || watched == headerWidget_ ||
      watched == tabBar_) {
    if (event->type() == QEvent::MouseButtonPress) {
      auto *mouseEvent = static_cast<QMouseEvent *>(event);

      if (watched == tabBar_ && mouseEvent->button() == Qt::MiddleButton) {
        const int tabIndex = tabBar_->tabAt(mouseEvent->pos());
        if (tabIndex >= 0) {
          closeTab(tabIndex);
          return true;
        }
      }

      if (mouseEvent->button() == Qt::LeftButton) {
        if (watched == tabBar_ && tabBar_->tabAt(mouseEvent->pos()) != -1) {
          return false;
        }
        if (windowHandle()) {
          windowHandle()->startSystemMove();
          return true;
        }
      }
    } else if (event->type() == QEvent::MouseButtonDblClick) {
      auto *mouseEvent = static_cast<QMouseEvent *>(event);
      if (mouseEvent->button() == Qt::LeftButton) {
        if (watched == tabBar_ && tabBar_->tabAt(mouseEvent->pos()) != -1) {
          return false;
        }
        if (isMaximized()) {
          showNormal();
        } else {
          showMaximized();
        }
        return true;
      }
    }
  }
  return QMainWindow::eventFilter(watched, event);
}

void MainWindow::closeEvent(QCloseEvent *event) {
  saveSession();
  saveDownloadRecords();
  if (!privateMode_) {
    QSettings st("LiteWave", "LiteWave");
    st.setValue("session/cleanExit", true);
    st.sync();
  }
  QMainWindow::closeEvent(event);
}

void MainWindow::resizeEvent(QResizeEvent *event) {
  QMainWindow::resizeEvent(event);
  if (suggestionPopup_ && suggestionPopup_->isVisible()) {
    suggestionPopup_->reposition();
  }
}

void MainWindow::changeEvent(QEvent *event) {
  if (event->type() == QEvent::WindowStateChange) {
    updateWindowControls();
  }
  QMainWindow::changeEvent(event);
}

QWebEngineView *MainWindow::currentView() const {
  return qobject_cast<QWebEngineView *>(tabStack_->currentWidget());
}

QWebEngineView *MainWindow::createView(const QUrl &url) {
  auto *view = new QWebEngineView(tabStack_);
  auto *page = new WebPage(profile_, this, view, view);
  view->setPage(page);
  auto *s = view->settings();
  s->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
  s->setAttribute(QWebEngineSettings::LocalStorageEnabled, true);
  s->setAttribute(QWebEngineSettings::DnsPrefetchEnabled, true);
  s->setAttribute(QWebEngineSettings::WebGLEnabled, true);
  s->setAttribute(QWebEngineSettings::Accelerated2dCanvasEnabled, true);
  s->setAttribute(QWebEngineSettings::AutoLoadImages, true);
  s->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows, true);
  s->setAttribute(QWebEngineSettings::PdfViewerEnabled, true);
  s->setAttribute(QWebEngineSettings::AllowRunningInsecureContent, false);
  s->setAttribute(QWebEngineSettings::FullScreenSupportEnabled, true);
  s->setAttribute(QWebEngineSettings::PlaybackRequiresUserGesture, false);
  s->setAttribute(QWebEngineSettings::WebRTCPublicInterfacesOnly, false);
  s->setAttribute(QWebEngineSettings::FocusOnNavigationEnabled, true);
  s->setAttribute(QWebEngineSettings::ScrollAnimatorEnabled, false);
  s->setAttribute(QWebEngineSettings::PluginsEnabled, true);
  s->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, false);
  s->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, false);
  // Allow user-initiated Clipboard API writes such as Copy buttons on
  // GitHub/ChatGPT. Clipboard reads remain permission-gated below.
  s->setAttribute(QWebEngineSettings::JavascriptCanAccessClipboard, true);
  s->setAttribute(QWebEngineSettings::JavascriptCanPaste, false);
  s->setAttribute(QWebEngineSettings::AllowWindowActivationFromJavaScript, false);
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
  s->setAttribute(QWebEngineSettings::ScreenCaptureEnabled, true);
#endif
  const int index = tabBar_->addTab(QIcon(":/icons/litewave.png"), "LiteWave");
  tabStack_->addWidget(view);
  tabBar_->setCurrentIndex(index);
  tabStack_->setCurrentIndex(index);
  tabLastActiveTime_[view] = QDateTime::currentMSecsSinceEpoch();

  auto *closeBtn = new QToolButton(tabBar_);
  closeBtn->setObjectName("tabCloseButton");
  closeBtn->setText("✕");
  closeBtn->setToolTip("ปิดแท็บ (Ctrl+W)");
  closeBtn->setFixedSize(18, 18);
  closeBtn->setCursor(Qt::PointingHandCursor);
  connect(closeBtn, &QToolButton::clicked, this, [this, view] {
    const int idx = tabStack_->indexOf(view);
    if (idx >= 0)
      closeTab(idx);
  });
  tabBar_->setTabButton(index, QTabBar::RightSide, closeBtn);

  connect(view, &QWebEngineView::iconChanged, this,
          [this, view](const QIcon &icon) {
            const int idx = tabStack_->indexOf(view);
            if (idx >= 0 && !icon.isNull()) {
              tabBar_->setTabIcon(idx, icon);
            }
          });

  connect(view->page(), &QWebEnginePage::fullScreenRequested, this,
          [this](QWebEngineFullScreenRequest request) {
            request.accept();
            if (request.toggleOn()) {
              if (headerWidget_)
                headerWidget_->hide();
              statusBar()->hide();
              showFullScreen();
            } else {
              showNormal();
              if (headerWidget_)
                headerWidget_->show();
              statusBar()->show();
            }
          });

  connect(page, &QWebEnginePage::newWindowRequested, this,
          [this, page](QWebEngineNewWindowRequest &request) {
            const QUrl sourceUrl = page ? page->url() : QUrl();
            const QUrl targetUrl = request.requestedUrl();
            const bool shieldProtectsPage =
                adBlocker_ && adBlocker_->isEnabledForUrl(sourceUrl);
            const bool automaticPopup = !request.isUserInitiated();
            const bool aggressiveCrossSitePopup =
                shieldProtectsPage &&
                adBlocker_->mode() == AdBlocker::Mode::Aggressive &&
                targetUrl.isValid() && !isSameSiteOrSubdomain(sourceUrl, targetUrl);

            // Never create the tab first and close it later: ad networks use
            // that race to steal focus. Standard blocks scripted popups;
            // Aggressive also blocks cross-site tabs created by ad-click traps.
            if (shieldProtectsPage && (automaticPopup || aggressiveCrossSitePopup)) {
              adBlocker_->recordBlockedPopup();
              statusBar()->showMessage(
                  automaticPopup
                      ? QStringLiteral("Shield บล็อกป๊อปอัปอัตโนมัติแล้ว")
                      : QStringLiteral("Shield เข้มงวดบล็อกแท็บโฆษณาข้ามเว็บแล้ว"),
                  3500);
              refreshShieldUi();
              return;
            }

            auto *newView = createView(QUrl());
            request.openIn(newView->page());
          });

  // Qt 6.4 uses the legacy feature signal; Qt 6.6+ uses the richer
  // QWebEnginePermission API. In both cases sensitive capabilities always
  // require an explicit answer.
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
  connect(page, &QWebEnginePage::permissionRequested, this,
          [this](QWebEnginePermission permission) {
            if (permission.state() != QWebEnginePermission::State::Ask)
              return;
            QString capability;
            switch (permission.permissionType()) {
            case QWebEnginePermission::PermissionType::MediaAudioCapture:
              capability = "microphone";
              break;
            case QWebEnginePermission::PermissionType::MediaVideoCapture:
              capability = "camera";
              break;
            case QWebEnginePermission::PermissionType::MediaAudioVideoCapture:
              capability = "camera and microphone";
              break;
            case QWebEnginePermission::PermissionType::Geolocation:
              capability = "location";
              break;
            case QWebEnginePermission::PermissionType::Notifications:
              capability = "notifications";
              break;
            case QWebEnginePermission::PermissionType::ClipboardReadWrite:
              capability = "คลิปบอร์ด";
              break;
            case QWebEnginePermission::PermissionType::DesktopVideoCapture:
            case QWebEnginePermission::PermissionType::DesktopAudioVideoCapture:
              capability = "screen capture";
              break;
            default:
              permission.deny();
              return;
            }
            QMessageBox box(this);
            box.setWindowTitle("สิทธิ์เว็บไซต์");
            box.setIcon(QMessageBox::Question);
            box.setText(QString("<b>%1</b> ต้องการใช้ %2")
                            .arg(permission.origin().host().toHtmlEscaped(),
                                 capability.toHtmlEscaped()));
            box.setInformativeText(
                "อนุญาตเฉพาะเว็บไซต์ที่คุณเชื่อถือ คุณสามารถปฏิเสธได้โดยไม่กระทบแท็บอื่น");
            auto *allowBtn = box.addButton("อนุญาต", QMessageBox::AcceptRole);
            box.addButton("ไม่อนุญาต", QMessageBox::RejectRole);
            box.exec();
            if (box.clickedButton() == allowBtn)
              permission.grant();
            else
              permission.deny();
          });
#else
  connect(
      page, &QWebEnginePage::featurePermissionRequested, this,
      [this, page](const QUrl &origin, QWebEnginePage::Feature feature) {
        QString capability;
        switch (feature) {
        case QWebEnginePage::MediaAudioCapture:
          capability = "microphone";
          break;
        case QWebEnginePage::MediaVideoCapture:
          capability = "camera";
          break;
        case QWebEnginePage::MediaAudioVideoCapture:
          capability = "camera and microphone";
          break;
        case QWebEnginePage::Geolocation:
          capability = "location";
          break;
        case QWebEnginePage::Notifications:
          capability = "notifications";
          break;
        case QWebEnginePage::DesktopVideoCapture:
        case QWebEnginePage::DesktopAudioVideoCapture:
          capability = "screen capture";
          break;
        default:
          page->setFeaturePermission(origin, feature,
                                     QWebEnginePage::PermissionDeniedByUser);
          return;
        }
        QMessageBox box(this);
        box.setWindowTitle("สิทธิ์เว็บไซต์");
        box.setIcon(QMessageBox::Question);
        box.setText(QString("<b>%1</b> ต้องการใช้ %2")
                        .arg(origin.host().toHtmlEscaped(),
                             capability.toHtmlEscaped()));
        box.setInformativeText(
            "อนุญาตเฉพาะเว็บไซต์ที่คุณเชื่อถือ คุณสามารถปฏิเสธได้โดยไม่กระทบแท็บอื่น");
        auto *allowBtn = box.addButton("อนุญาต", QMessageBox::AcceptRole);
        box.addButton("ไม่อนุญาต", QMessageBox::RejectRole);
        box.exec();
        page->setFeaturePermission(
            origin, feature,
            box.clickedButton() == allowBtn
                ? QWebEnginePage::PermissionGrantedByUser
                : QWebEnginePage::PermissionDeniedByUser);
      });
#endif


  connect(page, &QWebEnginePage::recentlyAudibleChanged, this,
          [this, view](bool recentlyAudible) {
            const int idx = tabStack_->indexOf(view);
            if (idx >= 0) {
              updateTabAudioIcon(idx, recentlyAudible, view->page()->isAudioMuted());
            }
          });
  connect(page, &QWebEnginePage::audioMutedChanged, this,
          [this, view](bool muted) {
            const int idx = tabStack_->indexOf(view);
            if (idx >= 0) {
              updateTabAudioIcon(idx, view->page()->recentlyAudible(), muted);
            }
          });

  connect(page, &QWebEnginePage::renderProcessTerminated, this,
          [this, view](QWebEnginePage::RenderProcessTerminationStatus status, int exitCode) {
            if (status == QWebEnginePage::NormalTerminationStatus)
              return;
            const QString crashHtml = QString(R"HTML(
<!doctype html>
<html lang="th">
<head>
<meta charset="utf-8">
<title>แท็บขัดข้อง - LiteWave</title>
<style>
  body { font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
         background: #0f172a; color: #f8fafc; display: flex; flex-direction: column;
         align-items: center; justify-content: center; height: 90vh; margin: 0; text-align: center; }
  .sad { font-size: 64px; margin-bottom: 16px; user-select: none; }
  h1 { font-size: 28px; margin-bottom: 8px; font-weight: 700; }
  p { color: #94a3b8; max-width: 460px; line-height: 1.6; margin-bottom: 24px; font-size: 15px; }
  button { background: #2563eb; color: white; border: none; border-radius: 8px;
           padding: 10px 24px; font-size: 14px; font-weight: 600; cursor: pointer; }
  button:hover { background: #1d4ed8; }
</style>
</head>
<body>
  <div class="sad">:(</div>
  <h1>เกิดข้อผิดพลาดกับแท็บนี้</h1>
  <p>กระบวนการประมวลผลของหน้าเว็บนี้หยุดทำงานโดยไม่คาดคิด (รหัสข้อผิดพลาด: %1)</p>
  <button onclick="location.reload()">โหลดแท็บนี้ใหม่</button>
</body>
</html>
)HTML").arg(exitCode);
            view->setHtml(crashHtml);
            statusBar()->showMessage(QStringLiteral("แท็บขัดข้อง (Exit code %1)").arg(exitCode), 4000);
          });

  connect(page, &QWebEnginePage::certificateError, this,
          [this](QWebEngineCertificateError error) {
            const QString msg = QStringLiteral(
                "คำเตือนความปลอดภัยของใบรับรอง SSL\n\n"
                "เว็บไซต์: %1\n"
                "ข้อผิดพลาด: %2\n\n"
                "การเชื่อมต่อนี้อาจไม่ปลอดภัย มีบุคคลอื่นพยายามดักรับข้อมูลหรือไม่?\n\n"
                "ต้องการเพิกเฉยต่อข้อผิดพลาดนี้และเปิดหน้าเว็บต่อไปหรือไม่?")
                .arg(error.url().host(), error.description());

            const auto reply = QMessageBox::warning(
                this, QStringLiteral("คำเตือนความปลอดภัย"), msg,
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

            if (reply == QMessageBox::Yes) {
              error.acceptCertificate();
            } else {
              error.rejectCertificate();
            }
          });

  connect(view, &QWebEngineView::urlChanged, this, [this, view](const QUrl &u) {
    if (view == currentView()) {
      updateCurrentUrl(u);
      if (auto *back = findChild<QAction *>("navBackAction"))
        back->setEnabled(view->history()->canGoBack());
      if (auto *forward = findChild<QAction *>("navForwardAction"))
        forward->setEnabled(view->history()->canGoForward());
    }
  });
  connect(view, &QWebEngineView::titleChanged, this,
          &MainWindow::updateTabTitle);

  connect(view, &QWebEngineView::loadStarted, this, [this, view] {
    if (view == currentView()) {
      progress_->setValue(0);
      progress_->show();
    }
  });
  connect(view, &QWebEngineView::loadProgress, this, [this, view](int p) {
    if (view == currentView()) {
      progress_->setValue(p);
      if (p >= 100)
        progress_->hide();
      else
        progress_->show();
    }
  });
  connect(view, &QWebEngineView::loadFinished, this, [this, view](bool) {
    applyShieldCosmetics(view);
    if (view == currentView()) {
      progress_->setValue(100);
      progress_->hide();
    }
  });

  if (url.isValid() && !url.isEmpty())
    view->setUrl(url);
  return view;
}

void MainWindow::newTab() {
  auto *view = createView(QUrl());
  loadHome(view);
  if (urlBar_) {
    urlBar_->setFocus();
    urlBar_->selectAll();
  }
}

void MainWindow::closeTab(int index) {
  auto *view = qobject_cast<QWebEngineView *>(tabStack_->widget(index));
  if (!view)
    return;
  closedTabs_.append(view->url());
  if (closedTabs_.size() > 20)
    closedTabs_.removeFirst();
  tabBar_->removeTab(index);
  tabStack_->removeWidget(view);
  tabLastActiveTime_.remove(view);
  delete view;
  if (!tabBar_->count())
    newTab();
  tabBar_->setVisible(!isFullScreen());
}

MainWindow::~MainWindow() { delete takeCentralWidget(); }

void MainWindow::setupShortcuts() {
  auto key = [this](const QString &sequence, auto fn) {
    auto *shortcut = new QShortcut(QKeySequence(sequence), this);
    connect(shortcut, &QShortcut::activated, this, fn);
  };
  key("Ctrl+T", [this] { newTab(); });
  key("Ctrl+N", [this] {
    auto *w = new MainWindow(nullptr, privateMode_);
    w->setAttribute(Qt::WA_DeleteOnClose);
    w->show();
  });
  key("Ctrl+Shift+N", [] {
    auto *w = new MainWindow(nullptr, true);
    w->setAttribute(Qt::WA_DeleteOnClose);
    w->show();
  });
  key("Ctrl+J", [this] { showDownloadPopup(); });
  key("Ctrl+,", [this] { showSettingsDialog(); });
  key("Ctrl+Shift+Del", [this] { clearBrowsingDataDialog(); });
  key("Ctrl+W", [this] { closeTab(tabBar_->currentIndex()); });
  key("Ctrl+Shift+T", [this] {
    if (closedTabs_.isEmpty())
      return;
    const auto u = closedTabs_.takeLast();
    if (u.host() == "litewave.home" || u.isEmpty())
      newTab();
    else
      createView(u);
  });
  key("Ctrl+Tab", [this] {
    if (tabBar_->count() > 0)
      tabBar_->setCurrentIndex((tabBar_->currentIndex() + 1) %
                               tabBar_->count());
  });
  key("Ctrl+Shift+Tab", [this] {
    if (tabBar_->count() > 0)
      tabBar_->setCurrentIndex(
          (tabBar_->currentIndex() + tabBar_->count() - 1) % tabBar_->count());
  });
  for (int i = 1; i <= 8; ++i)
    key("Ctrl+" + QString::number(i), [this, i] {
      if (i <= tabBar_->count())
        tabBar_->setCurrentIndex(i - 1);
    });
  key("Ctrl+9", [this] {
    if (tabBar_->count() > 0)
      tabBar_->setCurrentIndex(tabBar_->count() - 1);
  });
  key("Ctrl+L", [this] {
    urlBar_->setFocus();
    urlBar_->selectAll();
  });
  key("Alt+Left", [this] {
    if (auto *view = currentView(); view && view->history()->canGoBack())
      view->back();
  });
  key("Alt+Right", [this] {
    if (auto *view = currentView(); view && view->history()->canGoForward())
      view->forward();
  });
  auto reload = [this] { reloadCurrentView(); };
  key("Ctrl+R", reload);
  key("F5", reload);
  auto hard = [this] { reloadCurrentView(); };
  key("Ctrl+Shift+R", hard);
  key("Shift+F5", hard);
  key("Ctrl+D", [this] { addBookmark(); });
  key("Ctrl+Shift+O", [this] { showSettingsDialog(6); });
  key("Ctrl+Shift+D", [this] {
    bool ok = false;
    QString name = QInputDialog::getText(this, "Bookmarks", "Folder name",
                                         QLineEdit::Normal, "Saved tabs", &ok);
    if (!ok || name.trimmed().isEmpty())
      return;
    QSettings st("LiteWave", "LiteWave");
    const QString group =
        "folders/" + QUuid::createUuid().toString(QUuid::WithoutBraces);
    st.setValue(group + "/name", name);
    QStringList urls;
    for (int i = 0; i < tabStack_->count(); ++i)
      urls << qobject_cast<QWebEngineView *>(tabStack_->widget(i))
                  ->url()
                  .toString();
    st.setValue(group + "/urls", urls);
    statusBar()->showMessage("Saved " + QString::number(urls.size()) + " tabs",
                             3000);
  });
  key("Ctrl+F", [this] {
    if (findBar_ && currentView()) {
      findBar_->attachView(currentView());
      findBar_->showAndFocus(findQuery_);
    }
  });
  key("Ctrl+G", [this] {
    if (findBar_ && currentView()) {
      findBar_->findNext();
    }
  });
  key("Ctrl+Shift+G", [this] {
    if (findBar_ && currentView()) {
      findBar_->findPrevious();
    }
  });
  key("F12", [this] {
    toggleDevTools(currentView());
  });
  key("Ctrl+Shift+I", [this] {
    toggleDevTools(currentView());
  });
  auto zoomIn = [this] {
    if (currentView())
      currentView()->setZoomFactor(
          std::min(5.0, currentView()->zoomFactor() + 0.1));
  };
  key("Ctrl++", zoomIn);
  key("Ctrl+=", zoomIn);
  key("Ctrl+-", [this] {
    if (currentView())
      currentView()->setZoomFactor(
          std::max(0.25, currentView()->zoomFactor() - 0.1));
  });
  key("Ctrl+0", [this] {
    if (currentView())
      currentView()->setZoomFactor(1.0);
  });
  key("Ctrl+S", [this] {
    if (!currentView())
      return;
    const QString path = QFileDialog::getSaveFileName(
        this, "Save page", QString(), "Web archive (*.mhtml)");
    if (!path.isEmpty())
      currentView()->page()->save(
          path, QWebEngineDownloadRequest::MimeHtmlSaveFormat);
  });
  key("Ctrl+P", [this] {
    auto *view = currentView();
    if (!view)
      return;
    auto *printer = new QPrinter(QPrinter::HighResolution);
    QPrintDialog dialog(printer, this);
    if (dialog.exec() != QDialog::Accepted) {
      delete printer;
      return;
    }
    connect(
        view, &QWebEngineView::printFinished, view,
        [printer](bool) { delete printer; }, Qt::SingleShotConnection);
    view->print(printer);
  });
  key("F11", [this] {
    if (isFullScreen()) {
      if (currentView())
        currentView()->triggerPageAction(QWebEnginePage::ExitFullScreen);
      showNormal();
      if (headerWidget_)
        headerWidget_->show();
      statusBar()->show();
    } else {
      if (headerWidget_)
        headerWidget_->hide();
      statusBar()->hide();
      showFullScreen();
    }
  });
  key("Escape", [this] {
    if (!isFullScreen())
      return;
    if (currentView())
      currentView()->triggerPageAction(QWebEnginePage::ExitFullScreen);
    showNormal();
    if (headerWidget_)
      headerWidget_->show();
    statusBar()->show();
  });
}

void MainWindow::handleHomeNavigation(const QUrl &url, QWebEngineView *view) {
  if (!view)
    return;

  const QString host = url.host().toLower();
  const QString path = url.path().toLower();

  if (url.scheme() != "litewave" && host != "litewave.home") {
    view->setUrl(url);
    return;
  }

  const QUrlQuery query(url);
  if (host == "search" || path == "/search") {
    const QString text = query.queryItemValue("q").trimmed();
    const QString engine = query.queryItemValue("engine").toLower();
    if (text.isEmpty()) {
      loadHome(view);
    } else {
      if (!engine.isEmpty()) {
        for (const auto &eng : SearchEngineManager::instance().availableEngines()) {
          if (eng.id == engine) {
            view->setUrl(QUrl(eng.searchUrlTemplate.arg(QString::fromUtf8(QUrl::toPercentEncoding(text)))));
            return;
          }
        }
      }
      view->setUrl(SearchEngineManager::instance().buildSearchUrl(text));
    }
    return;
  }

  loadHome(view);
}

void MainWindow::reloadCurrentView() {
  auto *view = currentView();
  if (!view)
    return;
  if (view->url().host() == "litewave.home" || view->url().isEmpty() ||
      view->url().toString().contains("litewave.home")) {
    loadHome(view);
  } else {
    view->reload();
  }
}

void MainWindow::navigate() {
  if (suggestionPopup_) {
    suggestionPopup_->hide();
  }
  openUrl(urlBar_->text());
}

void MainWindow::openUrl(const QString &text) {
  if (suggestionPopup_) {
    suggestionPopup_->hide();
  }
  const QString input = text.trimmed();
  if (input.isEmpty() || !currentView())
    return;

  if (input.contains("litewave.home") || input.startsWith("litewave:")) {
    loadHome(currentView());
    return;
  }

  if (input == "litewave://settings" || input == "chrome://settings") {
    showSettingsDialog();
    return;
  }

  const QRegularExpression urlPattern(
      R"(^(https?://|localhost(?::\d+)?(?:/|$)|(?:[a-zA-Z0-9-]+\.)+[a-zA-Z]{2,}(?::\d+)?(?:/|$)))",
      QRegularExpression::CaseInsensitiveOption);

  QUrl url;
  if (urlPattern.match(input).hasMatch()) {
    const QString address = input.contains("://") ? input : "https://" + input;
    url = QUrl(address);
    if (!url.isValid())
      return;
    currentView()->setUrl(url);
    return;
  }

  // Allow prefix "g " or "google " to force search on Google Search
  if (input.startsWith("g ") || input.startsWith("google ")) {
    const QString query = input.section(' ', 1).trimmed();
    if (!query.isEmpty()) {
      url = QUrl("https://www.google.com/search?q=" +
                 QUrl::toPercentEncoding(query));
      currentView()->setUrl(url);
      return;
    }
  }

  // Use configured search engine from SearchEngineManager
  url = SearchEngineManager::instance().buildSearchUrl(input);
  currentView()->setUrl(url);
}

void MainWindow::updateCurrentUrl(const QUrl &url) {
  const auto view = currentView();
  if (!view)
    return;
  const QUrl currentUrl = url.isValid() ? url : view->url();

  if (currentUrl.isEmpty() || currentUrl.host() == "litewave.home" ||
      currentUrl.scheme() == "litewave" ||
      currentUrl.toString().contains("litewave.home")) {
    urlBar_->clear();
  } else {
    urlBar_->setText(currentUrl.toString());
    urlBar_->setCursorPosition(0);
  }

  if (sslAction_) {
    const QColor col = darkMode_ ? QColor("#9ca3af") : QColor("#64748b");
    if (currentUrl.scheme() == "https") {
      sslAction_->setIcon(createToolbarIcon("lock_https", QColor("#10b981")));
      sslAction_->setToolTip("การเชื่อมต่อปลอดภัย (HTTPS)");
    } else if (currentUrl.scheme() == "http") {
      sslAction_->setIcon(createToolbarIcon("lock_http", QColor("#ef4444")));
      sslAction_->setToolTip("การเชื่อมต่อไม่ปลอดภัย (HTTP)");
    } else {
      sslAction_->setIcon(createToolbarIcon("globe", col));
      sslAction_->setToolTip("LiteWave Dashboard");
    }
  }
  updateBookmarkStarState();
}

void MainWindow::updateTabTitle(const QString &title) {
  auto *view = qobject_cast<QWebEngineView *>(sender());
  if (!view)
    return;
  const int index = tabStack_->indexOf(view);
  QString displayTitle =
      title.trimmed().isEmpty() ? QStringLiteral("LiteWave") : title.left(22);
  if (view->page() && view->page()->lifecycleState() == QWebEnginePage::LifecycleState::Discarded) {
    displayTitle += " ·";
  }
  if (index >= 0) {
    tabBar_->setTabText(index, displayTitle);
    tabBar_->setTabToolTip(index, title);
    if (view && (view->url().host() == "litewave.home" || view->url().scheme() == "litewave" || displayTitle.startsWith("LiteWave"))) {
      tabBar_->setTabIcon(index, QIcon(":/icons/litewave.png"));
    }
  }
  if (view == currentView()) {
    if (displayTitle == "LiteWave" || displayTitle.isEmpty()) {
      setWindowTitle(privateMode_ ? QStringLiteral("LiteWave (Private)")
                                  : QStringLiteral("LiteWave"));
    } else {
      setWindowTitle(displayTitle +
                     (privateMode_ ? QStringLiteral(" — Private · LiteWave")
                                   : QStringLiteral(" — LiteWave")));
    }
    if (devToolsContainer_ && devToolsContainer_->isVisible() && devToolsTitleLabel_) {
      devToolsTitleLabel_->setText(QStringLiteral("🛠️ DevTools — %1").arg(
          title.trimmed().isEmpty() ? "LiteWave" : title.trimmed()));
    }
  }
}

void MainWindow::updateBookmarkStarState() {
  if (!bookmarkAction_)
    return;
  auto *view = currentView();
  if (!view) {
    bookmarkAction_->setText("☆");
    bookmarkAction_->setToolTip("บันทึกหน้าเว็บนี้ (Ctrl+D)");
    return;
  }
  const QString currentUrlStr = view->url().toString();
  bool bookmarked = false;
  for (const auto &item : bookmarks_) {
    if (item.url == currentUrlStr) {
      bookmarked = true;
      break;
    }
  }
  bookmarkAction_->setText(bookmarked ? "★" : "☆");
  bookmarkAction_->setToolTip(bookmarked ? "แก้ไขบุ๊กมาร์กสำหรับหน้านี้ (Ctrl+D)"
                                         : "บันทึกหน้าเว็บนี้ (Ctrl+D)");
}

void MainWindow::loadBookmarks() {
  bookmarks_.clear();
  QSettings st("LiteWave", "LiteWave");

  // If already migrated or modern items key exists, load strictly from bookmarks/items
  if (st.value("bookmarks/migrated", false).toBool() || st.contains("bookmarks/items")) {
    const QVariantList list = st.value("bookmarks/items").toList();
    for (const auto &var : list) {
      const QVariantMap map = var.toMap();
      BookmarkItem item;
      item.title = map.value("title").toString();
      item.url = map.value("url").toString();
      item.addedTime = map.value("addedTime").toLongLong();
      if (!item.url.isEmpty()) {
        bookmarks_.append(item);
      }
    }
    return;
  }

  // One-time legacy bookmarks migration for very old builds
  st.beginGroup("bookmarks");
  const QStringList keys = st.allKeys();
  for (const QString &key : keys) {
    if (key == "items" || key == "migrated")
      continue;
    const QString title = st.value(key).toString();
    BookmarkItem item;
    item.title = title.isEmpty() ? key : title;
    QString url = key;
    if (url.startsWith("https:/") && !url.startsWith("https://")) {
      url.replace("https:/", "https://");
    } else if (url.startsWith("http:/") && !url.startsWith("http://")) {
      url.replace("http:/", "http://");
    }
    item.url = url;
    item.addedTime = QDateTime::currentMSecsSinceEpoch();
    bookmarks_.append(item);
  }
  st.endGroup();

  saveBookmarks();
}

void MainWindow::saveBookmarks() {
  QSettings st("LiteWave", "LiteWave");
  // Clean up any legacy single-key bookmarks to prevent ghost resurrection
  st.remove("bookmarks");

  QVariantList list;
  for (const auto &item : bookmarks_) {
    QVariantMap map;
    map["title"] = item.title;
    map["url"] = item.url;
    map["addedTime"] = item.addedTime;
    list.append(map);
  }
  st.setValue("bookmarks/items", list);
  st.setValue("bookmarks/migrated", true);
  st.sync();
}

void MainWindow::addBookmark() {
  auto *view = currentView();
  if (!view)
    return;
  QUrl url = view->url();
  if (url.isEmpty() || url.host() == "litewave.home" || url.scheme() == "litewave") {
    url = QUrl("https://litewave.home/");
  }

  const QString currentUrlStr = url.toString();
  QString currentTitle = view->title().trimmed();
  if (currentTitle.isEmpty()) {
    currentTitle = url.host().isEmpty() ? "หน้าแรก (Home)" : url.host();
  }

  int existingIndex = -1;
  for (int i = 0; i < bookmarks_.size(); ++i) {
    if (bookmarks_[i].url == currentUrlStr) {
      existingIndex = i;
      currentTitle = bookmarks_[i].title;
      break;
    }
  }

  QDialog dialog(this);
  dialog.setWindowTitle(existingIndex >= 0 ? "แก้ไขบุ๊กมาร์ก" : "เพิ่มบุ๊กมาร์ก");
  dialog.setFixedWidth(420);

  const QString bg = darkMode_ ? "#0f172a" : "#ffffff";
  const QString cardBg = darkMode_ ? "#1e293b" : "#f8fafc";
  const QString textCol = darkMode_ ? "#f8fafc" : "#0f172a";
  const QString borderCol = darkMode_ ? "rgba(255,255,255,0.12)" : "rgba(0,0,0,0.15)";
  const QString primaryCol = "#0ea5e9";

  dialog.setStyleSheet(QString(R"(
    QDialog {
      background-color: %1;
      color: %2;
      border: 1px solid %3;
      border-radius: 12px;
    }
    QLabel {
      color: %2;
      font-size: 13px;
      font-weight: 500;
    }
    QLineEdit {
      background-color: %4;
      color: %2;
      border: 1px solid %3;
      border-radius: 8px;
      padding: 8px 12px;
      font-size: 13px;
      selection-background-color: %5;
    }
    QLineEdit:focus {
      border: 1.5px solid %5;
    }
    QPushButton {
      background-color: %4;
      color: %2;
      border: 1px solid %3;
      border-radius: 8px;
      padding: 8px 16px;
      font-size: 13px;
      font-weight: 500;
    }
    QPushButton:hover {
      background-color: rgba(14, 165, 233, 0.15);
      border-color: %5;
    }
    QPushButton#primaryBtn {
      background-color: %5;
      color: #ffffff;
      border: none;
      font-weight: 600;
    }
    QPushButton#primaryBtn:hover {
      background-color: #0284c7;
    }
    QPushButton#deleteBtn {
      background-color: rgba(239, 68, 68, 0.12);
      color: #ef4444;
      border: 1px solid rgba(239, 68, 68, 0.3);
    }
    QPushButton#deleteBtn:hover {
      background-color: #ef4444;
      color: #ffffff;
    }
  )").arg(bg, textCol, borderCol, cardBg, primaryCol));

  auto *layout = new QVBoxLayout(&dialog);
  layout->setContentsMargins(20, 20, 20, 20);
  layout->setSpacing(14);

  auto *titleLabel = new QLabel(existingIndex >= 0 ? "⭐ แก้ไขบุ๊กมาร์ก" : "⭐ บันทึกบุ๊กมาร์กแล้ว", &dialog);
  titleLabel->setStyleSheet("font-size: 16px; font-weight: 700;");
  layout->addWidget(titleLabel);

  auto *nameFormLayout = new QVBoxLayout();
  nameFormLayout->setSpacing(4);
  auto *nameLbl = new QLabel("ชื่อเว็บ:", &dialog);
  auto *nameEdit = new QLineEdit(&dialog);
  nameEdit->setText(currentTitle);
  nameFormLayout->addWidget(nameLbl);
  nameFormLayout->addWidget(nameEdit);
  layout->addLayout(nameFormLayout);

  auto *urlFormLayout = new QVBoxLayout();
  urlFormLayout->setSpacing(4);
  auto *urlLbl = new QLabel("ที่อยู่ URL:", &dialog);
  auto *urlEdit = new QLineEdit(&dialog);
  urlEdit->setText(currentUrlStr);
  urlFormLayout->addWidget(urlLbl);
  urlFormLayout->addWidget(urlEdit);
  layout->addLayout(urlFormLayout);

  auto *btnLayout = new QHBoxLayout();
  btnLayout->setSpacing(8);

  QPushButton *deleteBtn = nullptr;
  if (existingIndex >= 0) {
    deleteBtn = new QPushButton("ลบบุ๊กมาร์ก", &dialog);
    deleteBtn->setObjectName("deleteBtn");
    btnLayout->addWidget(deleteBtn);
  }

  btnLayout->addStretch();

  auto *cancelBtn = new QPushButton("ยกเลิก", &dialog);
  auto *saveBtn = new QPushButton("เสร็จสิ้น", &dialog);
  saveBtn->setObjectName("primaryBtn");
  saveBtn->setDefault(true);

  btnLayout->addWidget(cancelBtn);
  btnLayout->addWidget(saveBtn);
  layout->addLayout(btnLayout);

  connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);

  if (deleteBtn) {
    connect(deleteBtn, &QPushButton::clicked, &dialog, [this, &dialog, existingIndex] {
      if (existingIndex >= 0 && existingIndex < bookmarks_.size()) {
        bookmarks_.removeAt(existingIndex);
        saveBookmarks();
        updateBookmarkStarState();
        statusBar()->showMessage("ลบบุ๊กมาร์กเรียบร้อยแล้ว", 3000);
      }
      dialog.accept();
    });
  }

  connect(saveBtn, &QPushButton::clicked, &dialog, [this, &dialog, nameEdit, urlEdit, existingIndex] {
    const QString finalTitle = nameEdit->text().trimmed();
    const QString finalUrl = urlEdit->text().trimmed();
    if (!finalUrl.isEmpty()) {
      BookmarkItem item;
      item.title = finalTitle.isEmpty() ? finalUrl : finalTitle;
      item.url = finalUrl;
      item.addedTime = QDateTime::currentMSecsSinceEpoch();
      if (existingIndex >= 0 && existingIndex < bookmarks_.size()) {
        bookmarks_[existingIndex] = item;
      } else {
        bookmarks_.prepend(item);
      }
      saveBookmarks();
      updateBookmarkStarState();
      statusBar()->showMessage("บันทึกบุ๊กมาร์กแล้ว: " + item.title, 3000);
    }
    dialog.accept();
  });

  dialog.exec();
}

void MainWindow::showBookmarksManagerDialog() {
  showSettingsDialog(6);
}

void MainWindow::refreshShieldUi() {
  if (!shieldBtn_ || !adBlocker_)
    return;

  const QUrl pageUrl = currentView() ? currentView()->url() : QUrl();
  const bool enabledForSite = adBlocker_->isEnabledForUrl(pageUrl);
  const int count = adBlocker_->blockedCount();
  const bool isDark = darkMode_;
  const QColor activeColor("#ffffff");
  const QColor inactiveColor = isDark ? QColor("#94a3b8") : QColor("#64748b");

  if (!adBlocker_->isEnabled()) {
    shieldBtn_->setIcon(createToolbarIcon("shield", inactiveColor));
    shieldBtn_->setText("ปิด");
    shieldBtn_->setProperty("active", false);
    shieldBtn_->setToolTip("Shield ปิดอยู่ — คลิกเพื่อเปิดใช้งาน");
  } else if (!enabledForSite && (pageUrl.scheme() == "http" || pageUrl.scheme() == "https")) {
    shieldBtn_->setIcon(createToolbarIcon("shield", inactiveColor));
    shieldBtn_->setText("ปิดเฉพาะเว็บ");
    shieldBtn_->setProperty("active", false);
    shieldBtn_->setToolTip("Shield ปิดสำหรับ " + pageUrl.host());
  } else if (count > 0) {
    shieldBtn_->setIcon(createToolbarIcon("shield", activeColor));
    shieldBtn_->setText(QString::number(count));
    shieldBtn_->setProperty("active", true);
    const QString mode =
        adBlocker_->mode() == AdBlocker::Mode::Aggressive ? "เข้มงวด" : "มาตรฐาน";
    shieldBtn_->setToolTip(
        QString("Shield %1 — บล็อกแล้ว %2 รายการในเซสชันนี้\nคลิกเพื่อเปลี่ยนโหมดหรือปิดเฉพาะเว็บ")
            .arg(mode)
            .arg(count));
  } else {
    // Enabled but 0 blocked on this session/page so far: keep it subtle and clean
    shieldBtn_->setIcon(createToolbarIcon("shield", inactiveColor));
    shieldBtn_->setText("Shield");
    shieldBtn_->setProperty("active", false);
    const QString mode =
        adBlocker_->mode() == AdBlocker::Mode::Aggressive ? "เข้มงวด" : "มาตรฐาน";
    shieldBtn_->setToolTip(QString("LiteWave Shield (%1) — พร้อมปกป้อง").arg(mode));
  }

  shieldBtn_->style()->unpolish(shieldBtn_);
  shieldBtn_->style()->polish(shieldBtn_);
}

void MainWindow::applyShieldCosmetics(QWebEngineView *view) {
  if (!view || !view->page() || !adBlocker_)
    return;

  const QString styleId = QStringLiteral("litewave-shield-cosmetic-style");
  const bool enabled = adBlocker_->isEnabledForUrl(view->url());

  if (!enabled) {
    view->page()->runJavaScript(
        "(function(){"
        "window.__litewave_shield_disabled = true;"
        "const s=document.getElementById('" + styleId + "');if(s)s.remove();"
        "})();",
        QWebEngineScript::ApplicationWorld);
    return;
  }

  view->page()->runJavaScript(
      "(function(){ window.__litewave_shield_disabled = false; })();",
      QWebEngineScript::ApplicationWorld);

  const QString cssJson = QString::fromUtf8(
      QJsonDocument(QJsonArray{AdBlocker::cosmeticCss()})
          .toJson(QJsonDocument::Compact));
  const QString script =
      "(function(){const id='" + styleId +
      "';let s=document.getElementById(id);"
      "if(!s){s=document.createElement('style');s.id=id;"
      "(document.head||document.documentElement).appendChild(s);}"
      "s.textContent=" + cssJson + "[0];})();";
  view->page()->runJavaScript(script, QWebEngineScript::ApplicationWorld);
}

void MainWindow::toggleTheme() {
  darkMode_ = !darkMode_;
  QSettings themeSettings("LiteWave", "LiteWave");
  themeSettings.setValue("theme/dark", darkMode_);
  applyTheme();

  // Refresh the built-in home page immediately so its colors match the browser
  // chrome.
  if (currentView() && currentView()->url().host() == "litewave.home") {
    loadHome(currentView());
  }
}

void MainWindow::applyTheme() {
  const QColor iconCol = darkMode_ ? QColor("#f1f5f9") : QColor("#475569");
  if (themeBtn_) {
    themeBtn_->setIcon(createToolbarIcon(darkMode_ ? "sun" : "moon", iconCol));
    themeBtn_->setToolTip(darkMode_ ? "เปลี่ยนเป็นโหมดสว่าง" : "เปลี่ยนเป็นโหมดมืด");
  }
  if (menuBtn_) {
    menuBtn_->setIcon(createToolbarIcon("menu", iconCol));
  }

  if (darkMode_) {
    qApp->setStyleSheet(QStringLiteral(R"QSS(
            QMainWindow {
                background-color: #1e2026;
                color: #f1f3f4;
            }
            QWidget#headerWidget, QWidget#tabBarContainer {
                background-color: #1e2026;
                border: none;
            }
            QTabBar {
                background-color: transparent;
                border: none;
                qproperty-drawBase: 0;
            }
            QTabBar::scroller {
                width: 24px;
                height: 24px;
            }
            QTabBar::left-button, QTabBar::right-button {
                width: 22px;
                height: 22px;
            }
            QTabBar::tab {
                background-color: transparent;
                color: #94a3b8;
                border: none;
                border-top-left-radius: 10px;
                border-top-right-radius: 10px;
                padding: 6px 6px 6px 14px;
                margin-right: 3px;
                margin-top: 3px;
                font-size: 13px;
                min-width: 96px;
                max-width: 220px;
            }
            QTabBar::tab:hover {
                background-color: #282a32;
                color: #e2e8f0;
            }
            QTabBar::tab:selected {
                background-color: #2b2d35;
                color: #ffffff;
                font-weight: 600;
                margin-top: 1px;
            }
            QTabBar::close-button {
                background: transparent;
                margin: 0px;
                padding: 0px;
            }
            QToolButton#tabCloseButton {
                background: transparent;
                color: #94a3b8;
                border: none;
                border-radius: 9px;
                font-size: 11px;
                font-weight: bold;
                padding: 0px;
                margin-left: 2px;
                margin-right: 6px;
                min-width: 18px;
                max-width: 18px;
                min-height: 18px;
                max-height: 18px;
            }
            QToolButton#tabCloseButton:hover {
                background-color: #ef4444;
                color: #ffffff;
                border-radius: 9px;
            }
            QToolButton#tabSearchButton, QToolButton#newTabButton {
                background-color: transparent;
                color: #9e9ea0;
                border: none;
                border-radius: 13px;
                font-size: 16px;
                font-weight: bold;
                margin-bottom: 2px;
                min-width: 26px;
                max-width: 26px;
                min-height: 26px;
                max-height: 26px;
            }
            QToolButton#tabSearchButton:hover, QToolButton#newTabButton:hover {
                background-color: #2b2d35;
                color: #ffffff;
            }
            QToolButton#windowMinButton, QToolButton#windowMaxButton {
                background-color: transparent;
                border: none;
                border-radius: 6px;
                min-width: 34px;
                max-width: 34px;
                min-height: 28px;
                max-height: 28px;
            }
            QToolButton#windowMinButton:hover, QToolButton#windowMaxButton:hover {
                background-color: #333642;
            }
            QToolButton#windowCloseButton {
                background-color: transparent;
                border: none;
                border-radius: 6px;
                min-width: 34px;
                max-width: 34px;
                min-height: 28px;
                max-height: 28px;
            }
            QToolButton#windowCloseButton:hover {
                background-color: #ef4444;
                border-radius: 6px;
            }
            QToolBar#mainToolbar {
                background-color: #2b2d35;
                border-top: none;
                border-bottom: 1px solid #23252c;
                padding: 4px 10px;
                spacing: 6px;
            }
            QToolBar#mainToolbar QToolButton {
                background-color: transparent;
                color: #cbd5e1;
                border: none;
                border-radius: 8px;
                padding: 2px;
                font-size: 15px;
                font-weight: 500;
                min-width: 32px;
                max-width: 32px;
                min-height: 32px;
                max-height: 32px;
            }
            QToolBar#mainToolbar QToolButton:hover {
                background-color: #383b46;
                color: #ffffff;
            }
            QToolBar#mainToolbar QToolButton:pressed {
                background-color: #474b58;
            }
            QToolBar#mainToolbar QToolButton::menu-indicator,
            QToolBar#mainToolbar QToolButton::menu-arrow {
                image: none;
                width: 0px;
                height: 0px;
            }
            QLineEdit {
                background-color: #1e2026;
                color: #f1f5f9;
                border: 1px solid #33363f;
                border-radius: 17px;
                padding: 4px 12px 4px 32px;
                font-size: 13px;
                selection-background-color: #2563eb;
            }
            QLineEdit:hover {
                background-color: #1a1c22;
                border: 1px solid #3f434e;
            }
            QLineEdit:focus {
                border: 1.5px solid #0ea5e9;
                background-color: #16171d;
            }
            QLineEdit QToolButton {
                background: transparent;
                border: none;
                border-radius: 9px;
                padding: 0px;
                margin: -13px 4px 0px 4px;
                min-width: 20px;
                max-width: 20px;
                min-height: 20px;
                max-height: 20px;
                qproperty-iconSize: 16px 16px;
            }
            QLineEdit QToolButton:hover {
                background-color: rgba(148, 163, 184, 0.2);
            }
            QToolBar#mainToolbar QToolButton#shieldButton {
                background-color: #23252c;
                color: #94a3b8;
                border: 1px solid #383b46;
                border-radius: 15px;
                padding: 3px 12px;
                font-weight: 600;
                font-size: 12px;
                min-width: 90px;
                max-width: 180px;
                min-height: 28px;
                max-height: 28px;
            }
            QToolBar#mainToolbar QToolButton#shieldButton:hover {
                background-color: #333642;
                color: #f1f5f9;
                border-color: #474b58;
            }
            QToolBar#mainToolbar QToolButton#shieldButton[active="true"] {
                background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #0284c7, stop:1 #0ea5e9);
                color: #ffffff;
                border: none;
            }
            QToolBar#mainToolbar QToolButton#shieldButton[active="true"]:hover {
                background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #0ea5e9, stop:1 #38bdf8);
            }
            QToolBar#mainToolbar QToolButton#shieldButton[active="false"] {
                background-color: #23252c;
                color: #64748b;
                border: 1px solid #333640;
            }
            QToolBar#mainToolbar QToolButton#downloadsButton {
                font-size: 13px;
                font-weight: 600;
                padding: 3px 8px;
                border-radius: 7px;
                background-color: transparent;
                color: #9ca3af;
                border: 1px solid transparent;
            }
            QToolBar#mainToolbar QToolButton#downloadsButton:hover {
                background-color: #2b2e3a;
                color: #f3f4f6;
            }
            QToolBar#mainToolbar QToolButton#downloadsButton[active="true"] {
                background-color: rgba(14, 165, 233, 0.2);
                color: #38bdf8;
                border: 1px solid #0284c7;
                font-weight: bold;
            }
            QToolBar#mainToolbar QToolButton#mainMenuButton {
                padding: 4px;
            }
            QStackedWidget {
                border: none;
                background-color: transparent;
            }
            QProgressBar {
                background-color: transparent;
                border: none;
                max-height: 2px;
                min-height: 2px;
                margin: 0px;
                padding: 0px;
            }
            QProgressBar::chunk {
                background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #0284c7, stop:1 #38bdf8);
            }
            QStatusBar {
                background: #1e2026;
                color: #9ca3af;
                border-top: 1px solid #282a31;
                font-size: 12px;
            }
            QMenu {
                background-color: #2b2d35;
                color: #f1f3f4;
                border: 1px solid #383a42;
                border-radius: 8px;
                padding: 6px;
                font-size: 13px;
            }
            QMenu::item {
                padding: 6px 24px 6px 32px;
                border-radius: 4px;
            }
            QMenu::icon {
                padding-left: 8px;
            }
            QMenu::item:selected {
                background-color: #0284c7;
                color: #ffffff;
            }
            QMenu::separator {
                height: 1px;
                background: #383a42;
                margin: 4px 8px;
            }
            QAbstractItemView {
                background-color: #2b2d35;
                color: #f1f3f4;
                border: 1px solid #383a42;
                border-radius: 8px;
                padding: 4px;
                selection-background-color: #0284c7;
                selection-color: #ffffff;
                outline: none;
            }
            QSplitter::handle:vertical {
                height: 4px;
                background-color: #282a31;
            }
            QSplitter::handle:vertical:hover {
                background-color: #0284c7;
            }
            QWidget#devToolsHeader {
                background-color: #1e2026;
                border-top: 1px solid #282a31;
                border-bottom: 1px solid #282a31;
            }
            QLabel#devToolsTitleLabel {
                font-size: 12px;
                font-weight: 600;
                color: #94a3b8;
            }
            QToolButton#devToolsUndockBtn, QToolButton#devToolsCloseBtn {
                background: transparent;
                border: none;
                border-radius: 4px;
                color: #94a3b8;
                font-size: 12px;
            }
            QToolButton#devToolsUndockBtn:hover, QToolButton#devToolsCloseBtn:hover {
                background-color: rgba(255, 255, 255, 0.08);
                color: #f1f5f9;
            }
        )QSS"));
  } else {
    qApp->setStyleSheet(QStringLiteral(R"QSS(
            QMainWindow {
                background-color: #e3e5e8;
                color: #1e1e1e;
            }
            QWidget#headerWidget, QWidget#tabBarContainer {
                background-color: #e3e5e8;
                border: none;
            }
            QTabBar {
                background-color: transparent;
                border: none;
                qproperty-drawBase: 0;
            }
            QTabBar::scroller {
                width: 24px;
                height: 24px;
            }
            QTabBar::left-button, QTabBar::right-button {
                width: 22px;
                height: 22px;
            }
            QTabBar::tab {
                background-color: transparent;
                color: #64748b;
                border: none;
                border-top-left-radius: 10px;
                border-top-right-radius: 10px;
                padding: 6px 6px 6px 14px;
                margin-right: 3px;
                margin-top: 3px;
                font-size: 13px;
                min-width: 96px;
                max-width: 220px;
            }
            QTabBar::tab:hover {
                background-color: #e2e8f0;
                color: #1e293b;
            }
            QTabBar::tab:selected {
                background-color: #ffffff;
                color: #0f172a;
                font-weight: 600;
                margin-top: 1px;
            }
            QTabBar::close-button {
                background: transparent;
                margin: 0px;
                padding: 0px;
            }
            QToolButton#tabCloseButton {
                background: transparent;
                color: #64748b;
                border: none;
                border-radius: 9px;
                font-size: 11px;
                font-weight: bold;
                padding: 0px;
                margin-left: 2px;
                margin-right: 6px;
                min-width: 18px;
                max-width: 18px;
                min-height: 18px;
                max-height: 18px;
            }
            QToolButton#tabCloseButton:hover {
                background-color: #ef4444;
                color: #ffffff;
                border-radius: 9px;
            }
            QToolButton#tabSearchButton, QToolButton#newTabButton {
                background-color: transparent;
                color: #64748b;
                border: none;
                border-radius: 13px;
                font-size: 16px;
                font-weight: bold;
                margin-bottom: 2px;
                min-width: 26px;
                max-width: 26px;
                min-height: 26px;
                max-height: 26px;
            }
            QToolButton#tabSearchButton:hover, QToolButton#newTabButton:hover {
                background-color: #ffffff;
                color: #1e1e1e;
            }
            QToolButton#windowMinButton, QToolButton#windowMaxButton {
                background-color: transparent;
                border: none;
                border-radius: 6px;
                min-width: 34px;
                max-width: 34px;
                min-height: 28px;
                max-height: 28px;
            }
            QToolButton#windowMinButton:hover, QToolButton#windowMaxButton:hover {
                background-color: #e2e8f0;
            }
            QToolButton#windowCloseButton {
                background-color: transparent;
                border: none;
                border-radius: 6px;
                min-width: 34px;
                max-width: 34px;
                min-height: 28px;
                max-height: 28px;
            }
            QToolButton#windowCloseButton:hover {
                background-color: #ef4444;
                border-radius: 6px;
            }
            QToolBar#mainToolbar {
                background-color: #ffffff;
                border-top: none;
                border-bottom: 1px solid #e2e8f0;
                padding: 4px 10px;
                spacing: 6px;
            }
            QToolBar#mainToolbar QToolButton {
                background-color: transparent;
                color: #475569;
                border: none;
                border-radius: 8px;
                padding: 2px;
                font-size: 15px;
                font-weight: 500;
                min-width: 32px;
                max-width: 32px;
                min-height: 32px;
                max-height: 32px;
            }
            QToolBar#mainToolbar QToolButton:hover {
                background-color: #f1f5f9;
                color: #0f172a;
            }
            QToolBar#mainToolbar QToolButton:pressed {
                background-color: #e2e8f0;
            }
            QToolBar#mainToolbar QToolButton::menu-indicator,
            QToolBar#mainToolbar QToolButton::menu-arrow {
                image: none;
                width: 0px;
                height: 0px;
            }
            QLineEdit {
                background-color: #f1f5f9;
                color: #0f172a;
                border: 1px solid #e2e8f0;
                border-radius: 17px;
                padding: 4px 12px 4px 32px;
                font-size: 13px;
                selection-background-color: #2563eb;
            }
            QLineEdit:hover {
                background-color: #eef2f6;
                border: 1px solid #cbd5e1;
            }
            QLineEdit:focus {
                border: 1.5px solid #0284c7;
                background-color: #ffffff;
            }
            QLineEdit QToolButton {
                background: transparent;
                border: none;
                border-radius: 9px;
                padding: 0px;
                margin: -13px 4px 0px 4px;
                min-width: 20px;
                max-width: 20px;
                min-height: 20px;
                max-height: 20px;
                qproperty-iconSize: 16px 16px;
            }
            QLineEdit QToolButton:hover {
                background-color: rgba(100, 116, 139, 0.2);
            }
            QToolBar#mainToolbar QToolButton#shieldButton {
                background-color: #f1f5f9;
                color: #475569;
                border: 1px solid #e2e8f0;
                border-radius: 15px;
                padding: 3px 12px;
                font-weight: 600;
                font-size: 12px;
                min-width: 90px;
                max-width: 180px;
                min-height: 28px;
                max-height: 28px;
            }
            QToolBar#mainToolbar QToolButton#shieldButton:hover {
                background-color: #e2e8f0;
                color: #0f172a;
                border-color: #cbd5e1;
            }
            QToolBar#mainToolbar QToolButton#shieldButton[active="true"] {
                background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #0284c7, stop:1 #0ea5e9);
                color: #ffffff;
                border: none;
            }
            QToolBar#mainToolbar QToolButton#shieldButton[active="true"]:hover {
                background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #0ea5e9, stop:1 #38bdf8);
            }
            QToolBar#mainToolbar QToolButton#shieldButton[active="false"] {
                background-color: #f1f5f9;
                color: #94a3b8;
                border: 1px solid #e2e8f0;
            }
            QToolBar#mainToolbar QToolButton#downloadsButton {
                font-size: 13px;
                font-weight: 600;
                padding: 3px 8px;
                border-radius: 7px;
                background-color: transparent;
                color: #64748b;
                border: 1px solid transparent;
            }
            QToolBar#mainToolbar QToolButton#downloadsButton:hover {
                background-color: #f1f5f9;
                color: #0f172a;
            }
            QToolBar#mainToolbar QToolButton#downloadsButton[active="true"] {
                background-color: rgba(14, 165, 233, 0.15);
                color: #0284c7;
                border: 1px solid #38bdf8;
                font-weight: bold;
            }
            QToolBar#mainToolbar QToolButton#mainMenuButton {
                padding: 4px;
            }
            QStackedWidget {
                border: none;
                background-color: transparent;
            }
            QProgressBar {
                background-color: transparent;
                border: none;
                max-height: 2px;
                min-height: 2px;
                margin: 0px;
                padding: 0px;
            }
            QProgressBar::chunk {
                background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #0284c7, stop:1 #38bdf8);
            }
            QStatusBar {
                background: #ffffff;
                color: #4b5563;
                border-top: 1px solid #e5e7eb;
                font-size: 12px;
            }
            QMenu {
                background-color: #ffffff;
                color: #202124;
                border: 1px solid #d0d2d6;
                border-radius: 8px;
                padding: 6px;
                font-size: 13px;
            }
            QMenu::item {
                padding: 6px 24px 6px 32px;
                border-radius: 4px;
            }
            QMenu::icon {
                padding-left: 8px;
            }
            QMenu::item:selected {
                background-color: #0284c7;
                color: #ffffff;
            }
            QMenu::separator {
                height: 1px;
                background: #e5e7eb;
                margin: 4px 8px;
            }
            QAbstractItemView {
                background-color: #ffffff;
                color: #202124;
                border: 1px solid #d0d2d6;
                border-radius: 8px;
                padding: 4px;
                selection-background-color: #0284c7;
                selection-color: #ffffff;
                outline: none;
            }
            QSplitter::handle:vertical {
                height: 4px;
                background-color: #e2e8f0;
            }
            QSplitter::handle:vertical:hover {
                background-color: #0284c7;
            }
            QWidget#devToolsHeader {
                background-color: #f8fafc;
                border-top: 1px solid #e2e8f0;
                border-bottom: 1px solid #e2e8f0;
            }
            QLabel#devToolsTitleLabel {
                font-size: 12px;
                font-weight: 600;
                color: #475569;
            }
            QToolButton#devToolsUndockBtn, QToolButton#devToolsCloseBtn {
                background: transparent;
                border: none;
                border-radius: 4px;
                color: #64748b;
                font-size: 12px;
            }
            QToolButton#devToolsUndockBtn:hover, QToolButton#devToolsCloseBtn:hover {
                background-color: rgba(0, 0, 0, 0.06);
                color: #0f172a;
            }
        )QSS"));
  }
  if (findBar_) {
    findBar_->setDarkMode(darkMode_);
  }
  if (suggestionPopup_) {
    suggestionPopup_->setDarkMode(darkMode_);
  }
  if (menuBtn_) {
    auto *oldMenu = menuBtn_->menu();
    menuBtn_->setMenu(createMainMenu());
    if (oldMenu) {
      oldMenu->deleteLater();
    }
  }
  updateWindowControls();
}

void MainWindow::updateWindowControls() {
  if (!minWinBtn_ || !maxWinBtn_ || !closeWinBtn_)
    return;

  const QColor normCol = darkMode_ ? QColor("#94a3b8") : QColor("#64748b");
  const QColor hoverCol = darkMode_ ? QColor("#ffffff") : QColor("#0f172a");
  const QColor closeHoverCol = QColor("#ffffff");

  minWinBtn_->setIcon(createWindowControlIcon("win_min", normCol, hoverCol));
  minWinBtn_->setToolTip("ย่อหน้าต่าง (Ctrl+M)");

  const bool maximized = isMaximized();
  maxWinBtn_->setIcon(createWindowControlIcon(maximized ? "win_restore" : "win_max", normCol, hoverCol));
  maxWinBtn_->setToolTip(maximized ? "คืนขนาดหน้าต่าง" : "ขยายหน้าต่าง");

  closeWinBtn_->setIcon(createWindowControlIcon("win_close", normCol, closeHoverCol));
  closeWinBtn_->setToolTip("ปิดโปรแกรม (Alt+F4)");
}

void MainWindow::setupUrlBarCompleter() {
  if (urlBar_) {
    urlBar_->setCompleter(nullptr);
  }
}

void MainWindow::loadHome(QWebEngineView *view) {
  const int idx = tabStack_->indexOf(view);
  if (idx >= 0) {
    tabBar_->setTabIcon(idx, QIcon(":/icons/litewave.png"));
  }

  static QString logoBase64;
  if (logoBase64.isEmpty()) {
    QFile file(":/icons/litewave.png");
    if (file.open(QIODevice::ReadOnly)) {
      logoBase64 = QString::fromLatin1(file.readAll().toBase64());
    }
  }

  const int blockedCount = adBlocker_ ? adBlocker_->blockedCount() : 0;
  const QString ambientGrad = darkMode_
      ? "radial-gradient(circle at 50% -10%, rgba(14, 165, 233, 0.22), transparent 55%), radial-gradient(circle at 85% 15%, rgba(59, 130, 246, 0.12), transparent 45%), #090d16"
      : "radial-gradient(circle at 50% -10%, rgba(56, 189, 248, 0.22), transparent 55%), radial-gradient(circle at 85% 15%, rgba(99, 102, 241, 0.1), transparent 45%), #f8fafc";
  const QString cardBg = darkMode_ ? "rgba(30, 41, 59, 0.65)" : "rgba(255, 255, 255, 0.85)";
  const QString textCol = darkMode_ ? "#f8fafc" : "#0f172a";
  const QString subCol = darkMode_ ? "#94a3b8" : "#64748b";
  const QString borderCol = darkMode_ ? "rgba(255, 255, 255, 0.08)" : "rgba(226, 232, 240, 0.85)";

  QSettings st("LiteWave", "LiteWave");
  const bool enableSuggestions = st.value("search/enableSuggestions", true).toBool();
  const auto curEngine = SearchEngineManager::instance().currentEngine();

  auto getEngineIconSvg = [](const QString &name) -> QString {
    if (name.contains("Google", Qt::CaseInsensitive)) {
      return R"(<svg width="15" height="15" viewBox="0 0 24 24"><path fill="#4285F4" d="M23.745 12.27c0-.7-.06-1.4-.19-2.07H12v4.51h6.6c-.29 1.52-1.14 2.82-2.4 3.68v3.05h3.88c2.27-2.09 3.665-5.17 3.665-9.17z"/><path fill="#34A853" d="M12 24c3.24 0 5.95-1.08 7.93-2.91l-3.88-3.05c-1.08.72-2.45 1.16-4.05 1.16-3.12 0-5.77-2.1-6.72-4.93H1.25v3.15C3.26 21.36 7.33 24 12 24z"/><path fill="#FBBC05" d="M5.28 14.27c-.25-.72-.38-1.49-.38-2.27s.13-1.55.38-2.27V6.58H1.25C.45 8.18 0 9.99 0 12s.45 3.82 1.25 5.42l4.03-3.15z"/><path fill="#EA4335" d="M12 4.75c1.77 0 3.35.61 4.6 1.8l3.42-3.42C17.95 1.19 15.24 0 12 0 7.33 0 3.26 2.64 1.25 6.58l4.03 3.15c.95-2.83 3.6-4.93 6.72-4.93z"/></svg>)";
    }
    if (name.contains("DuckDuckGo", Qt::CaseInsensitive)) {
      return R"(<svg width="15" height="15" viewBox="0 0 24 24"><circle cx="12" cy="12" r="11" fill="#DE5833"/><circle cx="12" cy="12" r="5" fill="#FFFFFF"/><circle cx="13" cy="11" r="1.5" fill="#333333"/><path d="M7 14c2 3 8 3 10 0" stroke="#FFFFFF" stroke-width="2" stroke-linecap="round"/></svg>)";
    }
    if (name.contains("Brave", Qt::CaseInsensitive)) {
      return R"(<svg width="15" height="15" viewBox="0 0 24 24" fill="#FB542B"><path d="M12 1L3 5v6c0 5.55 3.84 10.74 9 12 5.16-1.26 9-6.45 9-12V5l-9-4zm0 4.1a3.5 3.5 0 1 1 0 7 3.5 3.5 0 0 1 0-7zm0 14.4c-3.1-1.1-5.7-4.4-6.3-8 1.8.8 4.2 1.2 6.3 1.2s4.5-.4 6.3-1.2c-.6 3.6-3.2 6.9-6.3 8z"/></svg>)";
    }
    if (name.contains("Bing", Qt::CaseInsensitive)) {
      return R"(<svg width="15" height="15" viewBox="0 0 24 24" fill="#0078D4"><path d="M5 3v18l5-2.8 5.7 3.8 3.3-2.2V9.2L13.8 6.5 10 8.8V3H5zm7.8 7.3l2.8 1.4-4.8 2.6V8.7l2 1.6z"/></svg>)";
    }
    return R"(<svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="#0284c7" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round"><circle cx="11" cy="11" r="8"></circle><line x1="21" y1="21" x2="16.65" y2="16.65"></line></svg>)";
  };

  QString curEngineShort = curEngine.name;
  if (curEngineShort.contains("Brave")) curEngineShort = "Brave";
  else if (curEngineShort.contains("Google")) curEngineShort = "Google";
  else if (curEngineShort.contains("DuckDuckGo")) curEngineShort = "DuckDuckGo";
  else if (curEngineShort.contains("Bing")) curEngineShort = "Bing";
  else if (curEngineShort.contains("LiteWave")) curEngineShort = "Custom";
  const QString curEngineName = curEngineShort.isEmpty() ? "Google" : curEngineShort;
  const QString curIconSvg = getEngineIconSvg(curEngineName);

  QString engineDropdownHtml;
  for (const auto &eng : SearchEngineManager::instance().availableEngines()) {
    const bool sel = (eng.id == curEngine.id);
    QString shortName = eng.name;
    if (shortName.contains("Brave")) shortName = "Brave";
    else if (shortName.contains("Google")) shortName = "Google";
    else if (shortName.contains("DuckDuckGo")) shortName = "DuckDuckGo";
    else if (shortName.contains("Bing")) shortName = "Bing";
    else if (shortName.contains("LiteWave")) shortName = "Custom";

    const QString iconSvg = getEngineIconSvg(shortName);
    engineDropdownHtml += QString(R"ITEM(
      <div class="engine-option%1" data-id="%2" data-name="%3" onclick="selectEngine('%2', '%3', event)">
        <span class="engine-option-icon">%4</span>
        <span class="engine-option-name">%3</span>
        <svg class="engine-check-icon" width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="3" stroke-linecap="round" stroke-linejoin="round">
          <polyline points="20 6 9 17 4 12"></polyline>
        </svg>
      </div>)ITEM")
      .arg(sel ? " selected" : "", eng.id, shortName, iconSvg);
  }

  static const char html_head[] = R"HTML_HEAD(
<!doctype html>
<html lang="th">
<head>
<meta charset="utf-8">
<title>LiteWave</title>
<link rel="icon" type="image/png" href="data:image/png;base64,%10">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<style>
html { scroll-behavior: smooth; -webkit-font-smoothing: antialiased; -moz-osx-font-smoothing: grayscale; }
* { box-sizing: border-box; margin: 0; padding: 0; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, "Helvetica Neue", Arial, sans-serif; }
body {
  background: %1;
  background-attachment: fixed;
  color: %3;
  min-height: 100vh;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: flex-start;
  gap: 28px;
  padding: 44px 20px 24px;
}
.hero-section {
  display: flex;
  flex-direction: column;
  align-items: center;
  margin-bottom: 28px;
  user-select: none;
}
.brand-badge {
  margin-bottom: 14px;
  display: flex;
  align-items: center;
  justify-content: center;
  filter: drop-shadow(0 8px 24px rgba(14, 165, 233, 0.35));
  transition: transform 0.3s cubic-bezier(0.34, 1.56, 0.64, 1);
}
.brand-badge:hover {
  transform: scale(1.08) rotate(-2deg);
}
.brand-badge-img {
  width: 84px;
  height: 84px;
  object-fit: contain;
  display: block;
}
.brand-title {
  font-size: 46px;
  font-weight: 800;
  letter-spacing: -1.2px;
  margin-bottom: 6px;
  background: linear-gradient(135deg, #0284c7 0%, #2563eb 50%, #38bdf8 100%);
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
  display: inline-block;
}
.brand-subtitle {
  font-size: 14px;
  color: %4;
  font-weight: 400;
  letter-spacing: -0.2px;
}
.search-container {
  position: relative;
  width: 100%;
  max-width: 680px;
  margin-bottom: 34px;
  z-index: 100;
}
.suggestions-box {
  position: absolute;
  top: calc(100% + 8px);
  left: 0;
  right: 0;
  background-color: %2;
  backdrop-filter: blur(20px);
  -webkit-backdrop-filter: blur(20px);
  border: 1px solid %5;
  border-radius: 18px;
  box-shadow: 0 16px 40px rgba(0,0,0,0.22);
  display: none;
  flex-direction: column;
  overflow: hidden;
  z-index: 500;
  padding: 6px 0;
}
.suggestions-box.active {
  display: flex;
}
.suggestion-item {
  display: flex;
  align-items: center;
  gap: 12px;
  padding: 10px 20px;
  font-size: 14.5px;
  color: %3;
  cursor: pointer;
  transition: background 0.12s ease;
}
.suggestion-item:hover, .suggestion-item.selected {
  background-color: #0284c7;
  color: #ffffff;
}
.suggestion-icon {
  color: %4;
  display: flex;
  align-items: center;
}
.suggestion-item:hover .suggestion-icon, .suggestion-item.selected .suggestion-icon {
  color: #ffffff;
}
.search-form {
  position: relative;
  z-index: 10;
  display: flex;
  align-items: center;
  background-color: %2;
  backdrop-filter: blur(24px);
  -webkit-backdrop-filter: blur(24px);
  border: 1px solid %5;
  border-radius: 28px;
  padding: 5px 8px 5px 12px;
  box-shadow: 0 10px 30px rgba(0,0,0,0.06), 0 1px 3px rgba(0,0,0,0.04);
  transition: all 0.25s cubic-bezier(0.4, 0, 0.2, 1);
}
.search-form:focus-within {
  border-color: #0284c7;
  box-shadow: 0 12px 36px rgba(2, 132, 199, 0.22), 0 0 0 3px rgba(14, 165, 233, 0.25);
  transform: translateY(-1px);
}
.search-engine-picker {
  position: relative;
  display: flex;
  align-items: center;
  gap: 7px;
  padding: 5px 10px 5px 8px;
  border-radius: 18px;
  background: rgba(2, 132, 199, 0.08);
  color: %3;
  cursor: pointer;
  transition: all 0.18s cubic-bezier(0.4, 0, 0.2, 1);
  user-select: none;
  flex-shrink: 0;
  border: 1px solid transparent;
}
.search-engine-picker:hover {
  background-color: rgba(2, 132, 199, 0.15);
  border-color: rgba(2, 132, 199, 0.25);
}
.search-engine-picker.active {
  background-color: rgba(2, 132, 199, 0.2);
  border-color: rgba(2, 132, 199, 0.4);
  box-shadow: 0 0 0 3px rgba(2, 132, 199, 0.18);
}
.engine-selected-icon {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 16px;
  height: 16px;
}
.engine-selected-name {
  font-size: 13px;
  font-weight: 600;
  color: inherit;
  white-space: nowrap;
}
.chevron-icon {
  color: %4;
  margin-left: 1px;
  transition: transform 0.22s cubic-bezier(0.4, 0, 0.2, 1);
}
.search-engine-picker.active .chevron-icon {
  transform: rotate(180deg);
}
.engine-dropdown-menu {
  position: absolute;
  top: calc(100% + 8px);
  left: 0;
  min-width: 175px;
  background: %2;
  backdrop-filter: blur(28px);
  -webkit-backdrop-filter: blur(28px);
  border: 1px solid %5;
  border-radius: 16px;
  box-shadow: 0 20px 45px rgba(0, 0, 0, 0.22), 0 6px 16px rgba(2, 132, 199, 0.15);
  padding: 6px;
  display: none;
  flex-direction: column;
  gap: 2px;
  z-index: 9999;
  transform-origin: top left;
  animation: engineMenuAnim 0.18s cubic-bezier(0.16, 1, 0.3, 1);
}
@keyframes engineMenuAnim {
  from { opacity: 0; transform: scale(0.93) translateY(-6px); }
  to { opacity: 1; transform: scale(1) translateY(0); }
}
.search-engine-picker.active .engine-dropdown-menu {
  display: flex;
}
.engine-option {
  display: flex;
  align-items: center;
  gap: 9px;
  padding: 8px 10px;
  border-radius: 10px;
  font-size: 13.5px;
  font-weight: 500;
  color: %3;
  cursor: pointer;
  transition: all 0.14s ease;
}
.engine-option:hover {
  background: rgba(2, 132, 199, 0.12);
  color: #0284c7;
}
.engine-option.selected {
  background: rgba(2, 132, 199, 0.16);
  color: #0284c7;
  font-weight: 600;
}
.engine-option-icon {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 16px;
  height: 16px;
  flex-shrink: 0;
}
.engine-option-name {
  flex: 1;
}
.engine-check-icon {
  opacity: 0;
  color: #0284c7;
  transition: opacity 0.15s ease;
}
.engine-option.selected .engine-check-icon {
  opacity: 1;
}
.search-divider {
  width: 1px;
  height: 22px;
  background-color: %5;
  margin: 0 8px;
  flex-shrink: 0;
}
.search-form input {
  flex: 1;
  border: none;
  background: transparent;
  padding: 10px 10px;
  font-size: 15px;
  color: %3;
  outline: none;
  min-width: 120px;
}
.search-form input::placeholder {
  color: %4;
  font-weight: 400;
}
.search-btn {
  background: linear-gradient(135deg, #0284c7 0%, #0ea5e9 100%);
  border: none;
  border-radius: 20px;
  width: 40px;
  height: 40px;
  display: flex;
  align-items: center;
  justify-content: center;
  color: #ffffff;
  cursor: pointer;
  box-shadow: 0 4px 12px rgba(2, 132, 199, 0.35);
  transition: all 0.2s ease;
  flex-shrink: 0;
}
.search-btn:hover {
  background: linear-gradient(135deg, #0369a1 0%, #0284c7 100%);
  transform: scale(1.05);
  box-shadow: 0 6px 16px rgba(2, 132, 199, 0.45);
}
.search-btn:active {
  transform: scale(0.96);
}
.section-container {
  width: 100%;
  max-width: 820px;
  position: relative;
  z-index: 1;
}
.section-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  margin-bottom: 16px;
  padding: 0 8px;
}
.section-title {
  display: flex;
  align-items: center;
  gap: 8px;
  font-size: 13.5px;
  font-weight: 600;
  color: %4;
  text-transform: uppercase;
  letter-spacing: 0.5px;
}
.sites-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(110px, 1fr));
  gap: 16px;
  width: 100%;
}
.site-card {
  position: relative;
  background-color: %2;
  backdrop-filter: blur(16px);
  -webkit-backdrop-filter: blur(16px);
  border: 1px solid %5;
  border-radius: 18px;
  padding: 16px 10px 14px;
  display: flex;
  flex-direction: column;
  align-items: center;
  cursor: pointer;
  transition: all 0.22s cubic-bezier(0.4, 0, 0.2, 1);
  box-shadow: 0 4px 12px rgba(0,0,0,0.04);
}
.site-card:hover {
  transform: translateY(-4px);
  border-color: rgba(2, 132, 199, 0.4);
  box-shadow: 0 10px 24px rgba(2, 132, 199, 0.15);
}
.site-icon-box {
  width: 52px;
  height: 52px;
  border-radius: 16px;
  display: flex;
  align-items: center;
  justify-content: center;
  margin-bottom: 10px;
  overflow: hidden;
  background: rgba(2, 132, 199, 0.08);
  box-shadow: 0 3px 8px rgba(0,0,0,0.06);
}
.site-favicon {
  width: 32px;
  height: 32px;
  border-radius: 8px;
  object-fit: contain;
}
.site-icon-fallback {
  width: 100%;
  height: 100%;
  display: flex;
  align-items: center;
  justify-content: center;
  color: #ffffff;
  font-size: 22px;
  font-weight: 700;
  text-shadow: 0 1px 2px rgba(0,0,0,0.2);
}
.site-name {
  font-size: 13px;
  font-weight: 500;
  color: %3;
  width: 100%;
  text-align: center;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}
.options-btn {
  position: absolute;
  top: 6px;
  right: 6px;
  background: transparent;
  border: none;
  color: %4;
  border-radius: 50%;
  width: 24px;
  height: 24px;
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  opacity: 0;
  transition: opacity 0.15s ease, background 0.15s ease, color 0.15s ease;
}
.site-card:hover .options-btn {
  opacity: 1;
}
.options-btn:hover {
  background: rgba(2, 132, 199, 0.15);
  color: #0284c7;
}
.card-dropdown {
  position: absolute;
  top: 32px;
  right: 6px;
  background-color: %2;
  backdrop-filter: blur(20px);
  -webkit-backdrop-filter: blur(20px);
  border: 1px solid %5;
  border-radius: 12px;
  box-shadow: 0 8px 24px rgba(0,0,0,0.18);
  display: none;
  flex-direction: column;
  z-index: 100;
  min-width: 90px;
  padding: 4px;
}
.card-dropdown.active {
  display: flex;
}
.card-dropdown-item {
  padding: 6px 12px;
  font-size: 12.5px;
  font-weight: 500;
  color: %3;
  border-radius: 6px;
  cursor: pointer;
  transition: background 0.1s ease;
}
.card-dropdown-item:hover {
  background: rgba(2, 132, 199, 0.15);
  color: #0284c7;
}
.card-dropdown-item.delete:hover {
  background: rgba(239, 68, 68, 0.15);
  color: #ef4444;
}
.add-card {
  border: 1.5px dashed %5;
  background: transparent;
}
.add-card:hover {
  border-color: #0284c7;
  background: rgba(2, 132, 199, 0.05);
}
.add-icon-box {
  background: rgba(2, 132, 199, 0.1);
  color: #0284c7;
  transition: transform 0.2s ease;
}
.add-card:hover .add-icon-box {
  transform: scale(1.1);
}
.modal-overlay {
  position: fixed;
  inset: 0;
  background: rgba(0,0,0,0.5);
  backdrop-filter: blur(8px);
  -webkit-backdrop-filter: blur(8px);
  display: none;
  align-items: center;
  justify-content: center;
  z-index: 1000;
}
.modal-overlay.active {
  display: flex;
}
.modal-card {
  background-color: %2;
  border: 1px solid %5;
  border-radius: 20px;
  width: 90%;
  max-width: 420px;
  padding: 24px;
  box-shadow: 0 24px 48px rgba(0,0,0,0.3);
}
.modal-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  margin-bottom: 20px;
}
.modal-header h3 {
  font-size: 18px;
  font-weight: 700;
  color: %3;
}
.modal-close-btn {
  background: transparent;
  border: none;
  font-size: 22px;
  color: %4;
  cursor: pointer;
  border-radius: 50%;
  width: 30px;
  height: 30px;
  display: flex;
  align-items: center;
  justify-content: center;
}
.modal-close-btn:hover {
  background: rgba(2, 132, 199, 0.1);
  color: %3;
}
.modal-body label {
  display: block;
  font-size: 13px;
  font-weight: 600;
  color: %4;
  margin-bottom: 6px;
  margin-top: 14px;
}
.modal-body label:first-child {
  margin-top: 0;
}
.modal-body input {
  width: 100%;
  padding: 10px 14px;
  border-radius: 12px;
  border: 1px solid %5;
  background: rgba(0,0,0,0.05);
  color: %3;
  font-size: 14px;
  outline: none;
  box-sizing: border-box;
}
.modal-body input:focus {
  border-color: #0284c7;
  box-shadow: 0 0 0 2px rgba(14, 165, 233, 0.2);
}
.modal-footer {
  display: flex;
  justify-content: flex-end;
  gap: 10px;
  margin-top: 24px;
}
.btn {
  padding: 8px 18px;
  border-radius: 12px;
  font-size: 13.5px;
  font-weight: 600;
  cursor: pointer;
  border: none;
  transition: all 0.15s ease;
}
.btn-secondary {
  background: rgba(0,0,0,0.08);
  color: %3;
}
.btn-secondary:hover {
  background: rgba(0,0,0,0.15);
}
.btn-primary {
  background: linear-gradient(135deg, #0284c7 0%, #0ea5e9 100%);
  color: #ffffff;
}
.btn-primary:hover {
  background: linear-gradient(135deg, #0369a1 0%, #0284c7 100%);
}
.shield-badge-container {
  margin-top: auto;
  padding-top: 16px;
  display: flex;
  justify-content: center;
  user-select: none;
}
.shield-badge {
  display: inline-flex;
  align-items: center;
  gap: 8px;
  padding: 8px 18px;
  border-radius: 24px;
  background-color: %2;
  backdrop-filter: blur(16px);
  -webkit-backdrop-filter: blur(16px);
  border: 1px solid %5;
  box-shadow: 0 4px 16px rgba(0,0,0,0.04);
  font-size: 12.5px;
  color: %4;
  transition: all 0.2s ease;
}
.shield-badge:hover {
  border-color: rgba(2, 132, 199, 0.4);
  color: %3;
  box-shadow: 0 6px 20px rgba(2, 132, 199, 0.12);
  transform: translateY(-1px);
}
.shield-badge-icon {
  color: #0284c7;
  display: flex;
  align-items: center;
}
.shield-badge strong {
  color: #0284c7;
  font-weight: 700;
}
</style>
</head>
)HTML_HEAD";

  static const char html_body[] = R"HTML_BODY(
<body>

<div class="hero-section">
  <div class="brand-badge">
    <img src="data:image/png;base64,%10" class="brand-badge-img" width="84" height="84" alt="LiteWave">
  </div>
  <div class="brand-title">LiteWave</div>
  <div class="brand-subtitle">เว็บเบราว์เซอร์ที่เร็ว ปลอดภัย และเป็นส่วนตัว</div>
</div>

<div class="search-container">
  <form class="search-form" onsubmit="return submitSearch(event)">
    <div class="search-engine-picker" id="enginePicker" onclick="toggleEngineMenu(event)" title="เลือกเครื่องมือค้นหา">
      <input type="hidden" id="engine" name="engine" value="%7">
      <div class="engine-selected-icon" id="engineCurrentIcon">%11</div>
      <span class="engine-selected-name" id="engineCurrentName">%8</span>
      <svg class="chevron-icon" width="11" height="11" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round">
        <polyline points="6 9 12 15 18 9"></polyline>
      </svg>
      <div class="engine-dropdown-menu" id="engineMenu" onclick="event.stopPropagation()">
        %12
      </div>
    </div>
    <div class="search-divider"></div>
    <input id="q" name="q" type="search" autofocus placeholder="ค้นหาด้วย %8 หรือป้อน URL..." autocomplete="off" oninput="onSearchInput(this.value)" onkeydown="onSearchKeyDown(event)">
    <button type="submit" class="search-btn" title="ค้นหา">
      <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round">
        <circle cx="11" cy="11" r="8"></circle>
        <line x1="21" y1="21" x2="16.65" y2="16.65"></line>
      </svg>
    </button>
  </form>
  <div class="suggestions-box" id="suggestionsBox"></div>
</div>

<div class="section-container">
  <div class="section-header">
    <div class="section-title">
      <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
        <polygon points="12 2 15.09 8.26 22 9.27 17 14.14 18.18 21.02 12 17.77 5.82 21.02 7 14.14 2 9.27 8.91 8.26 12 2"></polygon>
      </svg>
      <span>ทางลัดยอดนิยม</span>
    </div>
  </div>
  <div class="sites-grid" id="sitesGrid"></div>
</div>

<div class="modal-overlay" id="shortcutModal" onclick="closeModalOnOverlay(event)">
  <div class="modal-card">
    <div class="modal-header">
      <h3 id="modalTitle">เพิ่มทางลัดใหม่</h3>
      <button class="modal-close-btn" onclick="closeModal()">&times;</button>
    </div>
    <div class="modal-body">
      <label for="shortcutName">ชื่อเว็บไซต์</label>
      <input type="text" id="shortcutName" placeholder="เช่น YouTube, GitHub" />
      <label for="shortcutUrl">URL เว็บไซต์</label>
      <input type="text" id="shortcutUrl" placeholder="https://..." />
      <label for="shortcutIcon">URL โลโก้ / รูปภาพ (ไม่บังคับ - ตรวจหาอัตโนมัติ)</label>
      <div style="display:flex; gap:8px; align-items:center;">
        <input type="text" id="shortcutIcon" placeholder="https://.../logo.png หรือเลือกไฟล์" style="flex:1;" />
        <label class="btn btn-secondary" style="margin:0; padding:10px 14px; display:inline-flex; align-items:center; justify-content:center; cursor:pointer; white-space:nowrap; font-size:12px; font-weight:600; border-radius:10px; border:1px solid rgba(2,132,199,0.3);">
          เลือกไฟล์
          <input type="file" id="shortcutIconFile" accept="image/*" style="display:none;" onchange="handleIconFileUpload(event)" />
        </label>
      </div>
    </div>
    <div class="modal-footer">
      <button class="btn btn-secondary" onclick="closeModal()">ยกเลิก</button>
      <button class="btn btn-primary" onclick="saveShortcut()">บันทึก</button>
    </div>
  </div>
</div>

<div class="shield-badge-container">
  <div class="shield-badge">
    <span class="shield-badge-icon">
      <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round">
        <path d="M12 22s8-4 8-10V5l-8-3-8 3v7c0 6 8 10 8 10z"/>
      </svg>
    </span>
    <span class="shield-badge-text">Shield คุ้มครองความเป็นส่วนตัว · บล็อกตัวติดตามแล้ว <strong>%6</strong> รายการ</span>
  </div>
</div>

)HTML_BODY";

  static const char html_script1[] = R"HTML_S1(
<script>
function submitSearch(event) {
  if (event && event.preventDefault) event.preventDefault();
  const query = document.getElementById('q').value.trim();
  if (!query) return false;
  const engine = document.getElementById('engine').value;
  window.location.href = 'litewave://search?engine=' +
    encodeURIComponent(engine) + '&q=' + encodeURIComponent(query);
  return false;
}

function toggleEngineMenu(e) {
  if (e) e.stopPropagation();
  const picker = document.getElementById('enginePicker');
  if (picker) picker.classList.toggle('active');
}

function selectEngine(id, name, e) {
  if (e) e.stopPropagation();
  const input = document.getElementById('engine');
  if (input) input.value = id;
  const nameEl = document.getElementById('engineCurrentName');
  if (nameEl) nameEl.textContent = name;

  document.querySelectorAll('.engine-option').forEach(opt => {
    if (opt.dataset.id === id) {
      opt.classList.add('selected');
      const iconSpan = opt.querySelector('.engine-option-icon');
      if (iconSpan) {
        const curIcon = document.getElementById('engineCurrentIcon');
        if (curIcon) curIcon.innerHTML = iconSpan.innerHTML;
      }
    } else {
      opt.classList.remove('selected');
    }
  });

  const picker = document.getElementById('enginePicker');
  if (picker) picker.classList.remove('active');

  const qInput = document.getElementById('q');
  if (qInput) qInput.placeholder = 'ค้นหาด้วย ' + name + ' หรือป้อน URL...';
  localStorage.setItem('litewave_preferred_engine', id);
}

window.addEventListener('DOMContentLoaded', () => {
  const pref = localStorage.getItem('litewave_preferred_engine');
  if (pref) {
    const opt = document.querySelector('.engine-option[data-id="' + pref + '"]');
    if (opt) {
      selectEngine(pref, opt.dataset.name);
    }
  }
});

const defaultList = [
  { name: 'Google', url: 'https://www.google.com' },
  { name: 'YouTube', url: 'https://www.youtube.com' },
  { name: 'GitHub', url: 'https://github.com' },
  { name: 'ChatGPT', url: 'https://chatgpt.com' },
  { name: 'Facebook', url: 'https://www.facebook.com' },
  { name: 'Wikipedia', url: 'https://www.wikipedia.org' },
  { name: 'Reddit', url: 'https://www.reddit.com' },
  { name: 'Twitch', url: 'https://twitch.tv' }
];

function getShortcuts() {
  const saved = localStorage.getItem('litewave_all_shortcuts');
  if (!saved) {
    localStorage.setItem('litewave_all_shortcuts', JSON.stringify(defaultList));
    return defaultList;
  }
  try {
    return JSON.parse(saved);
  } catch(e) {
    return defaultList;
  }
}

function saveShortcuts(list) {
  localStorage.setItem('litewave_all_shortcuts', JSON.stringify(list));
}

function getDomain(urlStr) {
  try {
    const u = new URL(urlStr.startsWith('http://') || urlStr.startsWith('https://') ? urlStr : 'https://' + urlStr);
    return u.hostname;
  } catch(e) {
    return urlStr;
  }
}

function getOrigin(urlStr) {
  try {
    const u = new URL(urlStr.startsWith('http://') || urlStr.startsWith('https://') ? urlStr : 'https://' + urlStr);
    return u.origin;
  } catch(e) {
    return '';
  }
}

function getInitialFaviconUrl(s) {
  if (s.icon && s.icon.trim()) return s.icon.trim();
  const domain = getDomain(s.url);
  return 'https://www.google.com/s2/favicons?domain=' + encodeURIComponent(domain) + '&sz=64';
}

function handleFaviconError(img) {
  let step = parseInt(img.dataset.step || '0', 10);
  const domain = img.dataset.domain;
  step++;
  img.dataset.step = step.toString();

  if (step === 1) {
    img.src = 'https://icons.duckduckgo.com/ip3/' + encodeURIComponent(domain) + '.ico';
  } else if (step === 2) {
    img.src = 'https://icon.horse/icon/' + encodeURIComponent(domain);
  } else {
    img.style.display = 'none';
    const fallback = img.nextElementSibling;
    if (fallback) fallback.style.display = 'flex';
  }
}

function handleFaviconLoad(img) {
  if (img.naturalWidth <= 16 && (img.src.includes('google.com/s2') || img.src.includes('gstatic.com'))) {
    img.style.display = 'none';
    const fallback = img.nextElementSibling;
    if (fallback) fallback.style.display = 'flex';
  }
}

function handleIconFileUpload(event) {
  const file = event.target.files && event.target.files[0];
  if (!file) return;
  const reader = new FileReader();
  reader.onload = (e) => {
    document.getElementById('shortcutIcon').value = e.target.result;
  };
  reader.readAsDataURL(file);
}

const fallbackGradients = [
  'linear-gradient(135deg, #0284c7, #38bdf8)',
  'linear-gradient(135deg, #6366f1, #a855f7)',
  'linear-gradient(135deg, #ec4899, #f43f5e)',
  'linear-gradient(135deg, #10b981, #14b8a6)',
  'linear-gradient(135deg, #f59e0b, #ea580c)',
  'linear-gradient(135deg, #8b5cf6, #3b82f6)'
];

function getFallbackGradient(str) {
  let hash = 0;
  for (let i = 0; i < str.length; i++) hash = (hash * 31 + str.charCodeAt(i)) >>> 0;
  return fallbackGradients[hash % fallbackGradients.length];
}

let editIndex = -1;

function renderShortcuts() {
  const grid = document.getElementById('sitesGrid');
  if (!grid) return;
  grid.innerHTML = '';
  const shortcuts = getShortcuts();

  shortcuts.forEach((s, idx) => {
    const card = document.createElement('div');
    card.className = 'site-card';
    card.onclick = (e) => {
      if (!e.target.closest('.options-btn') && !e.target.closest('.card-dropdown')) {
        window.location.href = s.url;
      }
    };

    const domain = getDomain(s.url);
    const origin = getOrigin(s.url);
    const initialSrc = getInitialFaviconUrl(s);
    const initial = (s.name || domain || 'W').charAt(0).toUpperCase();
    const grad = getFallbackGradient(domain || s.name);

    card.innerHTML = `
      <button class="options-btn" title="ตัวเลือก" onclick="toggleCardDropdown(event, ${idx})">
        <svg width="14" height="14" viewBox="0 0 24 24" fill="currentColor">
          <circle cx="12" cy="5" r="2.2"/>
          <circle cx="12" cy="12" r="2.2"/>
          <circle cx="12" cy="19" r="2.2"/>
        </svg>
      </button>
      <div class="card-dropdown" id="dropdown-${idx}">
        <div class="card-dropdown-item" onclick="openEditModal(event, ${idx})">แก้ไข</div>
        <div class="card-dropdown-item delete" onclick="deleteShortcut(event, ${idx})">ลบ</div>
      </div>
      <div class="site-icon-box">
        <img src="${initialSrc}" class="site-favicon" data-step="0" data-domain="${domain}" data-origin="${origin}" onload="handleFaviconLoad(this)" onerror="handleFaviconError(this)" alt="${s.name}" />
        <div class="site-icon-fallback" style="display:none; background: ${grad};">${initial}</div>
      </div>
      <div class="site-name" title="${s.name}">${s.name}</div>
    `;
    grid.appendChild(card);
  });

  const addBtn = document.createElement('div');
  addBtn.className = 'site-card add-card';
  addBtn.onclick = openAddModal;
  addBtn.innerHTML = `
    <div class="site-icon-box add-icon-box">
      <svg width="22" height="22" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round">
        <line x1="12" y1="5" x2="12" y2="19"></line>
        <line x1="5" y1="12" x2="19" y2="12"></line>
      </svg>
    </div>
    <div class="site-name">เพิ่มทางลัด</div>
  `;
  grid.appendChild(addBtn);
}

function toggleCardDropdown(e, idx) {
  e.stopPropagation();
  const dropdown = document.getElementById('dropdown-' + idx);
  const wasActive = dropdown.classList.contains('active');
  document.querySelectorAll('.card-dropdown').forEach(d => d.classList.remove('active'));
  if (!wasActive) dropdown.classList.add('active');
}

document.addEventListener('click', (e) => {
  if (!e.target.closest('.card-dropdown') && !e.target.closest('.options-btn')) {
    document.querySelectorAll('.card-dropdown').forEach(d => d.classList.remove('active'));
  }
});

function openAddModal() {
  editIndex = -1;
  document.getElementById('modalTitle').textContent = 'เพิ่มทางลัดใหม่';
  document.getElementById('shortcutName').value = '';
  document.getElementById('shortcutUrl').value = '';
  document.getElementById('shortcutIcon').value = '';
  document.getElementById('shortcutModal').classList.add('active');
  document.getElementById('shortcutName').focus();
}

function openEditModal(e, idx) {
  e.stopPropagation();
  document.querySelectorAll('.card-dropdown').forEach(d => d.classList.remove('active'));
  const shortcuts = getShortcuts();
  if (idx < 0 || idx >= shortcuts.length) return;
  editIndex = idx;
  document.getElementById('modalTitle').textContent = 'แก้ไขทางลัด';
  document.getElementById('shortcutName').value = shortcuts[idx].name || '';
  document.getElementById('shortcutUrl').value = shortcuts[idx].url || '';
  document.getElementById('shortcutIcon').value = shortcuts[idx].icon || '';
  document.getElementById('shortcutModal').classList.add('active');
  document.getElementById('shortcutName').focus();
}

function closeModal() {
  document.getElementById('shortcutModal').classList.remove('active');
}

function closeModalOnOverlay(e) {
  if (e.target.id === 'shortcutModal') closeModal();
}

function saveShortcut() {
  const name = document.getElementById('shortcutName').value.trim();
  let url = document.getElementById('shortcutUrl').value.trim();
  const icon = document.getElementById('shortcutIcon').value.trim();
  if (!name || !url) return;

  if (!url.startsWith('http://') && !url.startsWith('https://')) {
    url = 'https://' + url;
  }
  const shortcuts = getShortcuts();
  const item = { name, url };
  if (icon) item.icon = icon;

  if (editIndex >= 0 && editIndex < shortcuts.length) {
    shortcuts[editIndex] = item;
  } else {
    shortcuts.push(item);
  }
  saveShortcuts(shortcuts);
  closeModal();
  renderShortcuts();
}

function deleteShortcut(e, idx) {
  e.stopPropagation();
  const shortcuts = getShortcuts();
  if (idx >= 0 && idx < shortcuts.length) {
    shortcuts.splice(idx, 1);
    saveShortcuts(shortcuts);
    renderShortcuts();
  }
}

)HTML_S1";

  static const char html_script2[] = R"HTML_S2(
const enableSearchSuggestions = %9;
let currentSuggestions = [];
let activeSuggestionIndex = -1;
let suggestDebounceTimer = null;
let currentScriptTag = null;

function onSearchInput(val) {
  const box = document.getElementById('suggestionsBox');
  if (!box) return;

  if (!enableSearchSuggestions) {
    box.classList.remove('active');
    return;
  }

  const q = val.trim();
  if (q.length === 0) {
    box.classList.remove('active');
    box.innerHTML = '';
    activeSuggestionIndex = -1;
    currentSuggestions = [];
    if (currentScriptTag) {
      currentScriptTag.remove();
      currentScriptTag = null;
    }
    return;
  }

  // Show local matching shortcuts immediately
  const shortcuts = getShortcuts().map(s => s.name).filter(item => item.toLowerCase().includes(q.toLowerCase()));
  if (shortcuts.length > 0) {
    currentSuggestions = shortcuts.slice(0, 5);
    renderSuggestions();
    box.classList.add('active');
  }

  // Live Google autocomplete query
  clearTimeout(suggestDebounceTimer);
  suggestDebounceTimer = setTimeout(() => {
    fetchGoogleSuggestions(q);
  }, 140);
}

window.handleGoogleSuggest = function(data) {
  const box = document.getElementById('suggestionsBox');
  if (!box) return;
  const inputVal = document.getElementById('q').value.trim().toLowerCase();
  if (!inputVal) {
    box.classList.remove('active');
    return;
  }

  if (data && Array.isArray(data) && Array.isArray(data[1])) {
    const liveMatches = data[1].slice(0, 7);
    const shortcuts = getShortcuts().map(s => s.name).filter(item => item.toLowerCase().includes(inputVal));
    const merged = Array.from(new Set([...liveMatches, ...shortcuts])).slice(0, 8);

    if (merged.length > 0) {
      currentSuggestions = merged;
      activeSuggestionIndex = -1;
      renderSuggestions();
      box.classList.add('active');
      return;
    }
  }
  if (currentSuggestions.length === 0) {
    box.classList.remove('active');
  }
};

function fetchGoogleSuggestions(query) {
  if (currentScriptTag) {
    currentScriptTag.remove();
    currentScriptTag = null;
  }
  currentScriptTag = document.createElement('script');
  currentScriptTag.src = 'https://suggestqueries.google.com/complete/search?client=chrome&q=' + encodeURIComponent(query) + '&callback=handleGoogleSuggest';
  currentScriptTag.onerror = function() {
    // If offline or blocked, keep local shortcut suggestions
  };
  document.body.appendChild(currentScriptTag);
}

function renderSuggestions() {
  const box = document.getElementById('suggestionsBox');
  if (!box) return;
  box.innerHTML = currentSuggestions.map((item, idx) => `
    <div class="suggestion-item ${idx === activeSuggestionIndex ? 'selected' : ''}" onclick="selectSuggestion('${item.replace(/\\/g, '\\\\').replace(/'/g, "\\'")}')">
      <div class="suggestion-icon">
        <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round">
          <circle cx="11" cy="11" r="8"></circle>
          <line x1="21" y1="21" x2="16.65" y2="16.65"></line>
        </svg>
      </div>
      <span>${item}</span>
    </div>
  `).join('');
}

function selectSuggestion(text) {
  document.getElementById('q').value = text;
  document.getElementById('suggestionsBox').classList.remove('active');
  submitSearch({ preventDefault: () => {} });
}

function onSearchKeyDown(e) {
  const box = document.getElementById('suggestionsBox');
  if (!box || !box.classList.contains('active') || currentSuggestions.length === 0) {
    return;
  }

  if (e.key === 'ArrowDown') {
    e.preventDefault();
    activeSuggestionIndex = (activeSuggestionIndex + 1) % currentSuggestions.length;
    renderSuggestions();
    document.getElementById('q').value = currentSuggestions[activeSuggestionIndex];
  } else if (e.key === 'ArrowUp') {
    e.preventDefault();
    activeSuggestionIndex = (activeSuggestionIndex - 1 + currentSuggestions.length) % currentSuggestions.length;
    renderSuggestions();
    document.getElementById('q').value = currentSuggestions[activeSuggestionIndex];
  } else if (e.key === 'Escape') {
    box.classList.remove('active');
  } else if (e.key === 'Enter') {
    if (activeSuggestionIndex >= 0 && activeSuggestionIndex < currentSuggestions.length) {
      e.preventDefault();
      selectSuggestion(currentSuggestions[activeSuggestionIndex]);
    }
  }
}

document.addEventListener('click', (e) => {
  const picker = document.getElementById('enginePicker');
  if (picker && !picker.contains(e.target)) {
    picker.classList.remove('active');
  }
  const box = document.getElementById('suggestionsBox');
  if (box && !e.target.closest('.search-container')) {
    box.classList.remove('active');
  }
});

renderShortcuts();
</script>
</body>
</html>
)HTML_S2";

  const QString templateHtml = QString::fromUtf8(html_head) +
                               QString::fromUtf8(html_body) +
                               QString::fromUtf8(html_script1) +
                               QString::fromUtf8(html_script2);

  const QString html = templateHtml
    .arg(ambientGrad, cardBg, textCol, subCol, borderCol,
         QString::number(blockedCount), curEngine.id, curEngineName,
         enableSuggestions ? "true" : "false")
    .arg(logoBase64, curIconSvg, engineDropdownHtml);

  view->setHtml(html, QUrl("https://litewave.home/"));
}

void MainWindow::addHistoryItem(const QString &title, const QUrl &url) {
  if (privateMode_ || !url.isValid() || url.isEmpty() ||
      url.scheme() == "litewave" || url.host() == "litewave.home")
    return;
  QSettings st("LiteWave", "LiteWave");
  QVariantList history = st.value("history").toList();

  if (!history.isEmpty()) {
    const auto lastMap = history.first().toMap();
    if (lastMap.value("url").toString() == url.toString())
      return;
  }

  QVariantMap item;
  item["title"] = title.isEmpty() ? url.host() : title;
  item["url"] = url.toString();
  item["time"] = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm");
  history.prepend(item);
  if (history.size() > 100)
    history.removeLast();
  st.setValue("history", history);
  setupUrlBarCompleter();
}

static QIcon createMenuIcon(const QString &name, const QColor &color) {
  QPixmap pix(20, 20);
  pix.fill(Qt::transparent);
  QPainter p(&pix);
  p.setRenderHint(QPainter::Antialiasing);
  p.setPen(QPen(color, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  p.setBrush(Qt::NoBrush);

  if (name == "new_tab") {
    // Clean plus icon
    p.drawLine(10, 4, 10, 16);
    p.drawLine(4, 10, 16, 10);
  } else if (name == "new_window") {
    // Clean browser window outline
    p.drawRoundedRect(3, 4, 14, 12, 2, 2);
    p.drawLine(3, 8, 17, 8);
    p.drawPoint(6, 6);
    p.drawPoint(9, 6);
  } else if (name == "incognito") {
    // Sleek glasses and fedora / private mask
    p.drawLine(3, 7, 17, 7);
    p.drawArc(6, 3, 8, 8, 0, 180 * 16);
    p.drawEllipse(4, 10, 5, 5);
    p.drawEllipse(11, 10, 5, 5);
    p.drawLine(9, 12, 11, 12);
  } else if (name == "history") {
    // Clock with counter-clockwise history arrow
    p.drawArc(3, 3, 14, 14, 30 * 16, 290 * 16);
    p.drawLine(10, 6, 10, 10);
    p.drawLine(10, 10, 13, 10);
    p.drawLine(11, 2, 14, 4);
    p.drawLine(14, 4, 11, 6);
  } else if (name == "bookmark") {
    // Sharp star icon
    QPolygonF star;
    star << QPointF(10, 3) << QPointF(12, 7.5) << QPointF(17, 8)
         << QPointF(13.2, 11.5) << QPointF(14.5, 16.5) << QPointF(10, 13.8)
         << QPointF(5.5, 16.5) << QPointF(6.8, 11.5) << QPointF(3, 8)
         << QPointF(8, 7.5);
    p.drawPolygon(star);
  } else if (name == "download") {
    // Arrow pointing down into open tray
    p.drawLine(10, 3, 10, 12);
    p.drawLine(6, 8.5, 10, 12.5);
    p.drawLine(14, 8.5, 10, 12.5);
    p.drawLine(3, 13, 3, 16.5);
    p.drawLine(3, 16.5, 17, 16.5);
    p.drawLine(17, 16.5, 17, 13);
  } else if (name == "trash") {
    // Clean trash can
    p.drawLine(4, 6, 16, 6);
    p.drawLine(8, 4, 12, 4);
    p.drawRoundedRect(5, 6, 10, 11, 1.5, 1.5);
    p.drawLine(8, 9, 8, 14);
    p.drawLine(12, 9, 12, 14);
  } else if (name == "page") {
    // Clean document icon
    p.drawRoundedRect(4, 3, 12, 14, 1.5, 1.5);
    p.drawLine(7, 7, 13, 7);
    p.drawLine(7, 10, 13, 10);
    p.drawLine(7, 13, 11, 13);
  } else if (name == "zoom") {
    // Magnifying glass
    p.drawEllipse(3, 3, 9, 9);
    p.drawLine(10, 10, 16, 16);
  } else if (name == "zoom_in") {
    p.drawEllipse(3, 3, 9, 9);
    p.drawLine(10, 10, 16, 16);
    p.drawLine(7.5, 5.5, 7.5, 9.5);
    p.drawLine(5.5, 7.5, 9.5, 7.5);
  } else if (name == "zoom_out") {
    p.drawEllipse(3, 3, 9, 9);
    p.drawLine(10, 10, 16, 16);
    p.drawLine(5.5, 7.5, 9.5, 7.5);
  } else if (name == "zoom_reset") {
    p.drawRoundedRect(3, 4, 14, 12, 2, 2);
    p.drawLine(10, 7, 10, 13);
  } else if (name == "fullscreen") {
    // Expand arrows
    p.drawLine(3, 7, 3, 3);
    p.drawLine(3, 3, 7, 3);
    p.drawLine(17, 7, 17, 3);
    p.drawLine(17, 3, 13, 3);
    p.drawLine(3, 13, 3, 17);
    p.drawLine(3, 17, 7, 17);
    p.drawLine(17, 13, 17, 17);
    p.drawLine(17, 17, 13, 17);
  } else if (name == "print") {
    // Modern printer
    p.drawRoundedRect(4, 7, 12, 7, 2, 2);
    p.drawLine(6, 4, 14, 4);
    p.drawLine(6, 4, 6, 7);
    p.drawLine(14, 4, 14, 7);
    p.drawRect(6, 11, 8, 5);
  } else if (name == "find") {
    // Document with search
    p.drawRoundedRect(3, 3, 10, 14, 1.5, 1.5);
    p.drawLine(6, 7, 10, 7);
    p.drawLine(6, 10, 9, 10);
    p.drawEllipse(11, 11, 5, 5);
    p.drawLine(15, 15, 18, 18);
  } else if (name == "save") {
    // Clean floppy disk / save badge
    p.drawRoundedRect(3, 3, 14, 14, 2, 2);
    p.drawRect(6, 3, 8, 5);
    p.drawRect(6, 11, 8, 6);
  } else if (name == "devtools") {
    // Code brackets < / >
    p.drawLine(7, 6, 4, 10);
    p.drawLine(4, 10, 7, 14);
    p.drawLine(13, 6, 16, 10);
    p.drawLine(16, 10, 13, 14);
    p.drawLine(11, 5, 9, 15);
  } else if (name == "theme") {
    // Half sun / half moon
    p.drawEllipse(4, 4, 12, 12);
    p.drawLine(10, 4, 10, 16);
    QPainterPath half;
    half.moveTo(10, 4);
    half.arcTo(4, 4, 12, 12, 90, 180);
    half.closeSubpath();
    p.fillPath(half, color);
  } else if (name == "settings") {
    // Precise gear / cog
    p.drawEllipse(6, 6, 8, 8);
    p.drawLine(10, 2, 10, 4);
    p.drawLine(10, 16, 10, 18);
    p.drawLine(2, 10, 4, 10);
    p.drawLine(16, 10, 18, 10);
    p.drawLine(4.5, 4.5, 6, 6);
    p.drawLine(14, 14, 15.5, 15.5);
    p.drawLine(4.5, 15.5, 6, 14);
    p.drawLine(14, 6, 15.5, 4.5);
  } else if (name == "about") {
    // Information circle (i)
    p.drawEllipse(3, 3, 14, 14);
    p.drawPoint(10, 7);
    p.drawLine(10, 9.5, 10, 13.5);
  } else if (name == "update") {
    // Sync / update circular arrow
    p.drawArc(3, 3, 14, 14, 45 * 16, 270 * 16);
    p.drawLine(10, 2, 13, 5);
    p.drawLine(13, 5, 10, 8);
  } else if (name == "feedback") {
    // Chat bubble with exclamation mark
    p.drawRoundedRect(3, 3, 14, 11, 2, 2);
    p.drawLine(6, 14, 4, 17);
    p.drawLine(4, 17, 9, 14);
    p.drawLine(10, 6, 10, 9);
    p.drawPoint(10, 11);
  } else if (name == "exit") {
    // Power / Logout icon
    p.drawArc(3, 5, 14, 12, 30 * 16, 300 * 16);
    p.drawLine(10, 2, 10, 9);
  }

  return QIcon(pix);
}

void MainWindow::populateHistoryMenu(QMenu *historyMenu) {
  historyMenu->clear();
  const QColor iconCol = darkMode_ ? QColor("#94a3b8") : QColor("#475569");
  auto *clearHist = historyMenu->addAction(createMenuIcon("trash", iconCol), "ล้างประวัติการใช้เว็บ");
  connect(clearHist, &QAction::triggered, this, [this] {
    QSettings st("LiteWave", "LiteWave");
    st.remove("history");
    statusBar()->showMessage("ล้างประวัติการใช้เว็บเรียบร้อยแล้ว", 3000);
  });
  historyMenu->addSeparator();

  QSettings st("LiteWave", "LiteWave");
  const QVariantList history = st.value("history").toList();
  if (history.isEmpty()) {
    auto *emptyItem = historyMenu->addAction("ไม่มีประวัติการใช้เว็บ");
    emptyItem->setEnabled(false);
  } else {
    int count = 0;
    for (const auto &var : history) {
      if (++count > 12)
        break;
      const QVariantMap map = var.toMap();
      QString title = map.value("title").toString().trimmed();
      const QString urlStr = map.value("url").toString();
      if (title.isEmpty()) {
        title = QUrl(urlStr).host();
      }
      QString display = title;
      if (display.length() > 38) {
        display = display.left(35) + "…";
      }
      auto *act = historyMenu->addAction(createMenuIcon("page", iconCol), display);
      act->setToolTip(title + "\n" + urlStr);
      connect(act, &QAction::triggered, this,
              [this, urlStr] { openUrl(urlStr); });
    }
  }
}

void MainWindow::populateBookmarksMenu(QMenu *bookmarksMenu) {
  bookmarksMenu->clear();
  const QColor iconCol = darkMode_ ? QColor("#94a3b8") : QColor("#475569");
  auto *addBm = bookmarksMenu->addAction(createMenuIcon("bookmark", iconCol), "บุ๊กมาร์กหน้านี้ (Ctrl+D)");
  connect(addBm, &QAction::triggered, this, &MainWindow::addBookmark);

  auto *manageBm = bookmarksMenu->addAction(createMenuIcon("settings", iconCol), "จัดการบุ๊กมาร์กในการตั้งค่า (Ctrl+Shift+O)");
  connect(manageBm, &QAction::triggered, this, [this] { showSettingsDialog(6); });

  bookmarksMenu->addSeparator();

  if (bookmarks_.isEmpty()) {
    auto *emptyItem = bookmarksMenu->addAction("ไม่มีบุ๊กมาร์กที่บันทึกไว้");
    emptyItem->setEnabled(false);
  } else {
    int count = 0;
    for (const auto &item : bookmarks_) {
      if (++count > 12)
        break;
      QString title = item.title.trimmed();
      if (title.isEmpty()) {
        title = QUrl(item.url).host();
      }
      QString display = title;
      if (display.length() > 38) {
        display = display.left(35) + "…";
      }
      auto *act = bookmarksMenu->addAction(createMenuIcon("bookmark", iconCol), display);
      act->setToolTip(item.title + "\n" + item.url);
      const QString url = item.url;
      connect(act, &QAction::triggered, this, [this, url] { openUrl(url); });
    }
    bookmarksMenu->addSeparator();
    auto *clearBm = bookmarksMenu->addAction(createMenuIcon("trash", iconCol), "ลบบุ๊กมาร์กทั้งหมด");
    connect(clearBm, &QAction::triggered, this, [this] {
      const auto ans = QMessageBox::question(
          this, "ลบบุ๊กมาร์กทั้งหมด",
          "คุณแน่ใจหรือไม่ว่าต้องการลบบุ๊กมาร์กทั้งหมด?",
          QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
      if (ans == QMessageBox::Yes) {
        bookmarks_.clear();
        saveBookmarks();
        updateBookmarkStarState();
        statusBar()->showMessage("ลบบุ๊กมาร์กทั้งหมดเรียบร้อยแล้ว", 3000);
      }
    });
  }
}

static QString formatBytes(qint64 bytes) {
  if (bytes < 0) return QStringLiteral("0 B");
  if (bytes < 1024) return QString::number(bytes) + " B";
  if (bytes < 1024 * 1024) return QString::number(bytes / 1024.0, 'f', 1) + " KB";
  if (bytes < 1024 * 1024 * 1024) return QString::number(bytes / (1024.0 * 1024.0), 'f', 1) + " MB";
  return QString::number(bytes / (1024.0 * 1024.0 * 1024.0), 'f', 2) + " GB";
}

static QString formatSpeed(double bytesPerSec) {
  if (bytesPerSec <= 0) return QStringLiteral("0 KB/s");
  if (bytesPerSec < 1024 * 1024) return QString::number(bytesPerSec / 1024.0, 'f', 1) + " KB/s";
  return QString::number(bytesPerSec / (1024.0 * 1024.0), 'f', 1) + " MB/s";
}

static QString formatEta(qint64 remainingBytes, double bytesPerSec) {
  if (bytesPerSec <= 100 || remainingBytes <= 0) return QStringLiteral("กำลังคำนวณเวลา...");
  const int totalSecs = static_cast<int>(remainingBytes / bytesPerSec);
  if (totalSecs < 60) return QString("เหลือ %1 วินาที").arg(totalSecs);
  if (totalSecs < 3600) return QString("เหลือ %1 นาที %2 วินาที").arg(totalSecs / 60).arg(totalSecs % 60);
  return QString("เหลือ %1 ชั่วโมง").arg(totalSecs / 3600);
}

void MainWindow::updateDownloadsButtonUi() {
  if (!downloadsBtn_) return;

  int activeCount = 0;
  qint64 totalRecvd = 0;
  qint64 totalSize = 0;
  double totalSpeed = 0.0;
  QString topFileName;

  for (const auto &act : activeDownloads_) {
    if (!act.completed && !act.failed && act.request && act.request->state() == QWebEngineDownloadRequest::DownloadInProgress) {
      activeCount++;
      totalRecvd += act.receivedBytes;
      if (act.totalBytes > 0) totalSize += act.totalBytes;
      totalSpeed += act.speed;
      if (topFileName.isEmpty()) topFileName = act.fileName;
    }
  }

  if (activeCount > 0) {
    downloadsBtn_->setProperty("active", true);
    downloadsBtn_->style()->unpolish(downloadsBtn_);
    downloadsBtn_->style()->polish(downloadsBtn_);

    int pct = 0;
    if (totalSize > 0) {
      pct = static_cast<int>((totalRecvd * 100) / totalSize);
    }
    downloadsBtn_->setText(QString("⭳ %1%").arg(pct));
    downloadsBtn_->setToolTip(QString("กำลังดาวน์โหลด %1 ไฟล์\nไฟล์: %2 (%3%)\nความเร็ว: %4\nขนาด: %5 / %6\n%7")
        .arg(activeCount).arg(topFileName).arg(pct)
        .arg(formatSpeed(totalSpeed)).arg(formatBytes(totalRecvd))
        .arg(totalSize > 0 ? formatBytes(totalSize) : "ไม่ทราบขนาด")
        .arg(totalSize > 0 ? formatEta(totalSize - totalRecvd, totalSpeed) : ""));

    statusBar()->showMessage(QString("กำลังดาวน์โหลด: %1 (%2%) · %3 · %4")
        .arg(topFileName).arg(pct).arg(formatSpeed(totalSpeed))
        .arg(totalSize > 0 ? formatEta(totalSize - totalRecvd, totalSpeed) : ""), 2000);
  } else {
    downloadsBtn_->setProperty("active", false);
    downloadsBtn_->style()->unpolish(downloadsBtn_);
    downloadsBtn_->style()->polish(downloadsBtn_);
    downloadsBtn_->setText("⭳");
    downloadsBtn_->setToolTip("รายการดาวน์โหลด (Ctrl+J)");
  }
}

void MainWindow::showDownloadPopup() {
  if (downloadPopup_ && downloadPopup_->isVisible()) {
    downloadPopup_->close();
    return;
  }

  const bool isDark = darkMode_;
  const QString bg = isDark ? "#0f172a" : "#ffffff";
  const QString cardBg = isDark ? "#1e293b" : "#f8fafc";
  const QString textCol = isDark ? "#f8fafc" : "#0f172a";
  const QString subCol = isDark ? "#94a3b8" : "#64748b";
  const QString borderCol = isDark ? "rgba(255,255,255,0.12)" : "rgba(0,0,0,0.12)";

  auto *popup = new QDialog(this, Qt::Popup | Qt::FramelessWindowHint);
  popup->setAttribute(Qt::WA_DeleteOnClose);
  popup->setFixedWidth(420);
  popup->setStyleSheet(QString(R"(
    QDialog {
      background-color: %1;
      border: 1px solid %4;
      border-radius: 12px;
      color: %2;
    }
    QLabel {
      color: %2;
    }
    QProgressBar {
      background-color: %3;
      border: none;
      border-radius: 4px;
      height: 7px;
      text-align: center;
    }
    QProgressBar::chunk {
      background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #0284c7, stop:1 #38bdf8);
      border-radius: 4px;
    }
    QPushButton {
      background-color: %3;
      color: %2;
      border: 1px solid %4;
      border-radius: 6px;
      padding: 5px 12px;
      font-size: 12px;
      font-weight: 500;
    }
    QPushButton:hover {
      background-color: rgba(14, 165, 233, 0.15);
      border-color: #0ea5e9;
      color: #0284c7;
    }
    QToolButton {
      background: transparent;
      border: none;
      color: %5;
      font-size: 13px;
      border-radius: 4px;
      padding: 3px;
    }
    QToolButton:hover {
      background-color: rgba(239, 68, 68, 0.15);
      color: #ef4444;
    }
    QScrollArea {
      border: none;
      background: transparent;
    }
  )").arg(bg, textCol, cardBg, borderCol, subCol));

  auto *mainLayout = new QVBoxLayout(popup);
  mainLayout->setContentsMargins(14, 12, 14, 12);
  mainLayout->setSpacing(10);

  // Header
  auto *hdrLayout = new QHBoxLayout();
  auto *titleLbl = new QLabel("📥 รายการดาวน์โหลด", popup);
  titleLbl->setStyleSheet("font-size: 14px; font-weight: bold;");
  hdrLayout->addWidget(titleLbl);
  hdrLayout->addStretch();

  auto *allBtn = new QPushButton("ดูประวัติทั้งหมด...", popup);
  allBtn->setStyleSheet("border: none; background: transparent; color: #0284c7; font-size: 12px; text-decoration: underline;");
  allBtn->setCursor(Qt::PointingHandCursor);
  connect(allBtn, &QPushButton::clicked, popup, [this, popup] {
    popup->close();
    showDownloadsDialog();
  });
  hdrLayout->addWidget(allBtn);

  auto *closeBtn = new QToolButton(popup);
  closeBtn->setText("✕");
  closeBtn->setCursor(Qt::PointingHandCursor);
  connect(closeBtn, &QToolButton::clicked, popup, &QDialog::close);
  hdrLayout->addWidget(closeBtn);
  mainLayout->addLayout(hdrLayout);

  // Divider
  auto *div = new QFrame(popup);
  div->setFrameShape(QFrame::HLine);
  div->setStyleSheet(QString("background-color: %1; max-height: 1px;").arg(borderCol));
  mainLayout->addWidget(div);

  // Scroll Area
  auto *scroll = new QScrollArea(popup);
  scroll->setWidgetResizable(true);
  scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  scroll->setMaximumHeight(360);
  auto *scrollContent = new QWidget(scroll);
  auto *itemsLayout = new QVBoxLayout(scrollContent);
  itemsLayout->setContentsMargins(0, 4, 0, 4);
  itemsLayout->setSpacing(8);

  struct ActiveUiItem {
    QPointer<QWebEngineDownloadRequest> request;
    QProgressBar *bar = nullptr;
    QLabel *subLabel = nullptr;
  };
  auto activeUis = std::make_shared<QList<ActiveUiItem>>();
  auto buildItems = std::make_shared<std::function<void()>>();

  *buildItems = [this, scrollContent, itemsLayout, cardBg, borderCol, subCol, textCol, activeUis, buildItems] {
    QLayoutItem *child;
    while ((child = itemsLayout->takeAt(0)) != nullptr) {
      if (child->widget()) child->widget()->deleteLater();
      delete child;
    }
    activeUis->clear();

    // 1. Active Downloads
    int activeAdded = 0;
    for (const auto &act : activeDownloads_) {
      if (!act.completed && !act.failed && !act.cancelled && act.request && act.request->state() == QWebEngineDownloadRequest::DownloadInProgress) {
        activeAdded++;
        auto *card = new QWidget(scrollContent);
        card->setStyleSheet(QString("background-color: %1; border: 1px solid %2; border-radius: 10px;").arg(cardBg, borderCol));
        auto *cLayout = new QVBoxLayout(card);
        cLayout->setContentsMargins(10, 9, 10, 9);
        cLayout->setSpacing(6);

        auto *row1 = new QHBoxLayout();
        row1->setContentsMargins(0, 0, 0, 0);
        auto *fNameLbl = new QLabel("⭳ " + act.fileName, card);
        fNameLbl->setStyleSheet("font-weight: 600; font-size: 13px; color: #0284c7;");
        fNameLbl->setWordWrap(true);
        row1->addWidget(fNameLbl, 1);

        auto *cancelBtn = new QToolButton(card);
        cancelBtn->setText("✕");
        cancelBtn->setToolTip("ยกเลิกการดาวน์โหลด");
        cancelBtn->setCursor(Qt::PointingHandCursor);
        cancelBtn->setStyleSheet("border: none; background: rgba(239, 68, 68, 0.1); color: #ef4444; border-radius: 4px; padding: 2px 6px; font-weight: bold; font-size: 12px;");
        auto req = act.request;
        const QString actFileName = act.fileName;
        connect(cancelBtn, &QToolButton::clicked, card, [this, req, actFileName, buildItems] {
          if (req) req->cancel();
          for (auto &a : activeDownloads_) {
            if (a.request == req) {
              a.completed = false;
              a.cancelled = true;
              a.failed = false;
              break;
            }
          }
          for (auto &r : downloadRecords_) {
            if (r.fileName == actFileName) {
              r.completed = false;
              r.cancelled = true;
              r.failed = false;
              break;
            }
          }
          saveDownloadRecords();
          updateDownloadsButtonUi();
          if (*buildItems) (*buildItems)();
        });
        row1->addWidget(cancelBtn);
        cLayout->addLayout(row1);

        auto *prog = new QProgressBar(card);
        prog->setRange(0, 100);
        const int pct = act.totalBytes > 0 ? static_cast<int>((act.receivedBytes * 100) / act.totalBytes) : 0;
        prog->setValue(pct);
        cLayout->addWidget(prog);

        auto *subLbl = new QLabel(card);
        subLbl->setStyleSheet(QString("font-size: 11.5px; color: %1;").arg(subCol));
        subLbl->setText(QString("%1 / %2 (%3%) · %4 · %5")
            .arg(formatBytes(act.receivedBytes))
            .arg(act.totalBytes > 0 ? formatBytes(act.totalBytes) : "ไม่ทราบขนาด")
            .arg(pct)
            .arg(formatSpeed(act.speed))
            .arg(act.totalBytes > 0 ? formatEta(act.totalBytes - act.receivedBytes, act.speed) : ""));
        cLayout->addWidget(subLbl);

        itemsLayout->addWidget(card);

        ActiveUiItem ui;
        ui.request = act.request;
        ui.bar = prog;
        ui.subLabel = subLbl;
        activeUis->append(ui);
      }
    }

    // 2. Recent Downloads (max 5)
    int completedAdded = 0;
    for (const auto &rec : downloadRecords_) {
      if (completedAdded >= 5) break;
      bool isActive = false;
      for (const auto &act : activeDownloads_) {
        if (!act.completed && !act.failed && !act.cancelled && act.path == rec.path) {
          isActive = true; break;
        }
      }
      if (isActive) continue;

      completedAdded++;
      auto *card = new QWidget(scrollContent);
      card->setStyleSheet(QString("background-color: %1; border: 1px solid %2; border-radius: 10px;").arg(cardBg, borderCol));
      auto *cLayout = new QVBoxLayout(card);
      cLayout->setContentsMargins(10, 9, 10, 9);
      cLayout->setSpacing(6);

      auto *topRow = new QHBoxLayout();
      topRow->setContentsMargins(0, 0, 0, 0);
      topRow->setSpacing(8);

      QString iconPrefix = "📄 ";
      if (rec.completed) iconPrefix = "✅ ";
      else if (rec.cancelled) iconPrefix = "⏹ ";
      else if (rec.failed) iconPrefix = "⚠️ ";

      auto *nameLbl = new QLabel(iconPrefix + rec.fileName, card);
      nameLbl->setStyleSheet("font-weight: 600; font-size: 13px;");
      nameLbl->setWordWrap(true);
      topRow->addWidget(nameLbl, 1);

      auto *delBtn = new QToolButton(card);
      delBtn->setText("✕");
      delBtn->setToolTip("ลบออกจากรายการนี้");
      delBtn->setCursor(Qt::PointingHandCursor);
      delBtn->setStyleSheet("border: none; background: transparent; color: #94a3b8; font-size: 12px; font-weight: bold; padding: 2px 4px;");
      const QString targetPath = rec.path;
      connect(delBtn, &QToolButton::clicked, card, [this, targetPath, buildItems] {
        for (int i = 0; i < downloadRecords_.size(); ++i) {
          if (downloadRecords_[i].path == targetPath) {
            downloadRecords_.removeAt(i);
            break;
          }
        }
        saveDownloadRecords();
        if (*buildItems) (*buildItems)();
      });
      topRow->addWidget(delBtn);
      cLayout->addLayout(topRow);

      auto *infoRow = new QHBoxLayout();
      infoRow->setContentsMargins(0, 0, 0, 0);
      infoRow->setSpacing(8);

      QLabel *statusLbl = new QLabel(card);
      if (rec.completed) {
        statusLbl->setText(QString("✓ เสร็จสิ้น · %1").arg(formatBytes(rec.totalBytes)));
        statusLbl->setStyleSheet("font-size: 12px; color: #16a34a; font-weight: 600;");
      } else if (rec.cancelled) {
        statusLbl->setText(QString("✕ ยกเลิกแล้ว%1").arg(rec.totalBytes > 0 ? (" · " + formatBytes(rec.totalBytes)) : ""));
        statusLbl->setStyleSheet("font-size: 12px; color: #64748b; font-weight: 500;");
      } else if (rec.failed) {
        statusLbl->setText("✕ ดาวน์โหลดล้มเหลว");
        statusLbl->setStyleSheet("font-size: 12px; color: #ef4444; font-weight: 500;");
      } else {
        statusLbl->setText("⟳ กำลังดำเนินการ");
        statusLbl->setStyleSheet(QString("font-size: 12px; color: %1;").arg(subCol));
      }
      infoRow->addWidget(statusLbl);
      infoRow->addStretch();

      const QString filePath = rec.path;
      if (rec.completed) {
        auto *openBtn = new QPushButton("เปิดไฟล์", card);
        openBtn->setCursor(Qt::PointingHandCursor);
        openBtn->setStyleSheet("background-color: #0284c7; color: #ffffff; border: none; border-radius: 6px; padding: 4px 10px; font-size: 11.5px; font-weight: 600;");
        connect(openBtn, &QPushButton::clicked, card, [filePath] {
          if (!filePath.isEmpty() && QFileInfo::exists(filePath)) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
          }
        });
        infoRow->addWidget(openBtn);
      }

      auto *folderBtn = new QPushButton("โฟลเดอร์", card);
      folderBtn->setCursor(Qt::PointingHandCursor);
      folderBtn->setStyleSheet(QString("background-color: %1; color: %2; border: 1px solid %3; border-radius: 6px; padding: 4px 10px; font-size: 11.5px; font-weight: 500;").arg(cardBg, textCol, borderCol));
      connect(folderBtn, &QPushButton::clicked, card, [filePath] {
        if (!filePath.isEmpty()) {
          QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(filePath).absolutePath()));
        }
      });
      infoRow->addWidget(folderBtn);
      cLayout->addLayout(infoRow);

      itemsLayout->addWidget(card);
    }

    if (activeAdded == 0 && completedAdded == 0) {
      auto *emptyLbl = new QLabel("ยังไม่มีรายการดาวน์โหลด", scrollContent);
      emptyLbl->setAlignment(Qt::AlignCenter);
      emptyLbl->setStyleSheet(QString("color: %1; padding: 24px; font-size: 13px;").arg(subCol));
      itemsLayout->addWidget(emptyLbl);
    }
    itemsLayout->addStretch();
  };

  (*buildItems)();
  scroll->setWidget(scrollContent);
  mainLayout->addWidget(scroll, 1);

  // Footer
  QSettings st("LiteWave", "LiteWave");
  const QString defaultDl = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
  const QString dlDir = st.value("downloadDirectory", defaultDl).toString();
  auto *openDlFolderBtn = new QPushButton("📁 เปิดโฟลเดอร์ดาวน์โหลด", popup);
  openDlFolderBtn->setCursor(Qt::PointingHandCursor);
  openDlFolderBtn->setStyleSheet(QString(
      "QPushButton {"
      "  background-color: transparent;"
      "  color: %1;"
      "  border: 1px solid %2;"
      "  border-radius: 8px;"
      "  padding: 8px;"
      "  font-size: 12.5px;"
      "  font-weight: 500;"
      "}"
      "QPushButton:hover {"
      "  background-color: rgba(2, 132, 199, 0.08);"
      "  border-color: #0284c7;"
      "  color: #0284c7;"
      "}").arg(textCol, borderCol));
  connect(openDlFolderBtn, &QPushButton::clicked, popup, [dlDir] {
    QDesktopServices::openUrl(QUrl::fromLocalFile(dlDir));
  });
  mainLayout->addWidget(openDlFolderBtn);

  // Timer to refresh active progress bars while popup is open
  auto *timer = new QTimer(popup);
  timer->setInterval(250);
  connect(timer, &QTimer::timeout, popup, [this, activeUis, buildItems] {
    bool hasActive = false;
    for (const auto &act : activeDownloads_) {
      if (!act.completed && !act.failed && act.request && act.request->state() == QWebEngineDownloadRequest::DownloadInProgress) {
        hasActive = true;
        for (auto &ui : *activeUis) {
          if (ui.request == act.request) {
            const int pct = act.totalBytes > 0 ? static_cast<int>((act.receivedBytes * 100) / act.totalBytes) : 0;
            if (ui.bar) ui.bar->setValue(pct);
            if (ui.subLabel) {
              ui.subLabel->setText(QString("%1 / %2 (%3%) · %4 · %5")
                  .arg(formatBytes(act.receivedBytes))
                  .arg(act.totalBytes > 0 ? formatBytes(act.totalBytes) : "ไม่ทราบขนาด")
                  .arg(pct)
                  .arg(formatSpeed(act.speed))
                  .arg(act.totalBytes > 0 ? formatEta(act.totalBytes - act.receivedBytes, act.speed) : ""));
            }
            break;
          }
        }
      }
    }
    if (!hasActive && !activeUis->isEmpty()) {
      if (*buildItems) (*buildItems)();
    }
  });
  timer->start();

  // Position popup directly under downloadsBtn_
  if (downloadsBtn_) {
    const QPoint btnGlobal = downloadsBtn_->mapToGlobal(QPoint(0, downloadsBtn_->height() + 4));
    int posX = btnGlobal.x() + downloadsBtn_->width() - popup->width();
    if (posX < 10) posX = 10;
    popup->move(posX, btnGlobal.y());
  }

  downloadPopup_ = popup;
  popup->show();
}

void MainWindow::handleDownloadRequested(QWebEngineDownloadRequest *download) {
  if (!download || download->state() != QWebEngineDownloadRequest::DownloadRequested)
    return;
  if (download->page()) {
    bool belongsToThisWindow = false;
    for (int i = 0; i < tabStack_->count(); ++i) {
      auto *view = qobject_cast<QWebEngineView *>(tabStack_->widget(i));
      if (view && view->page() == download->page()) {
        belongsToThisWindow = true;
        break;
      }
    }
    if (!belongsToThisWindow && !isActiveWindow()) {
      return;
    }
  }
  if (download->isSavePageDownload()) {
    download->accept();
    return;
  }

  QSettings st("LiteWave", "LiteWave");
  const QString defaultDir =
      QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
  QString dir = st.value("downloadDirectory", defaultDir).toString();
  if (dir.isEmpty() || !QDir(dir).exists())
    dir = defaultDir;
  const bool askLocation =
      st.value("askDownloadLocation", false).toBool();

  QString name = download->downloadFileName().isEmpty()
                     ? QStringLiteral("LiteWave-download")
                     : download->downloadFileName();

  if (askLocation) {
    const QString fullPath = QFileDialog::getSaveFileName(
        this, "บันทึกไฟล์ดาวน์โหลด", dir + "/" + name);
    if (fullPath.isEmpty()) {
      download->cancel();
      return;
    }
    QFileInfo fi(fullPath);
    dir = fi.absolutePath();
    name = fi.fileName();
  }

  download->setDownloadDirectory(dir);
  download->setDownloadFileName(name);

  DownloadRecord rec;
  rec.fileName = name;
  rec.path = dir + "/" + name;
  rec.totalBytes = download->totalBytes();
  rec.completed = false;
  rec.cancelled = false;
  rec.failed = false;
  downloadRecords_.prepend(rec);
  saveDownloadRecords();

  ActiveDownload act;
  act.request = download;
  act.fileName = name;
  act.path = dir + "/" + name;
  act.totalBytes = download->totalBytes();
  act.receivedBytes = download->receivedBytes();
  act.lastBytes = act.receivedBytes;
  act.lastTimeMs = QDateTime::currentMSecsSinceEpoch();
  act.speed = 0.0;
  act.completed = false;
  act.cancelled = false;
  act.failed = false;
  activeDownloads_.prepend(act);

  updateDownloadsButtonUi();

  connect(
      download, &QWebEngineDownloadRequest::receivedBytesChanged, this,
      [this, download, name] {
        const qint64 total = download->totalBytes();
        const qint64 recvd = download->receivedBytes();
        const qint64 now = QDateTime::currentMSecsSinceEpoch();

        if (total > 0) {
          for (auto &r : downloadRecords_) {
            if (r.fileName == name && r.totalBytes <= 0) {
              r.totalBytes = total;
              break;
            }
          }
        }

        for (auto &a : activeDownloads_) {
          if (a.request == download) {
            a.receivedBytes = recvd;
            a.totalBytes = total;
            const qint64 dt = now - a.lastTimeMs;
            if (dt >= 400) {
              const qint64 dBytes = recvd - a.lastBytes;
              if (dBytes >= 0) {
                a.speed = (dBytes * 1000.0) / dt;
              }
              a.lastBytes = recvd;
              a.lastTimeMs = now;
            }
            break;
          }
        }
        updateDownloadsButtonUi();
      });

  connect(
      download, &QWebEngineDownloadRequest::stateChanged, this,
      [this, download, name](QWebEngineDownloadRequest::DownloadState state) {
        if (state == QWebEngineDownloadRequest::DownloadCompleted) {
          statusBar()->showMessage("✓ ดาวน์โหลดเสร็จแล้ว: " + name, 6000);
          for (auto &a : activeDownloads_) {
            if (a.request == download) {
              a.completed = true;
              a.cancelled = false;
              a.failed = false;
              break;
            }
          }
          for (auto &r : downloadRecords_) {
            if (r.fileName == name) {
              r.completed = true;
              r.cancelled = false;
              r.failed = false;
              if (r.totalBytes <= 0) r.totalBytes = download->totalBytes();
              break;
            }
          }
          saveDownloadRecords();
          if (downloadsBtn_) {
            downloadsBtn_->setText("✓ เสร็จ");
            QTimer::singleShot(4000, this, [this] {
              updateDownloadsButtonUi();
            });
          }
        } else if (state == QWebEngineDownloadRequest::DownloadCancelled) {
          for (auto &a : activeDownloads_) {
            if (a.request == download) {
              a.completed = false;
              a.cancelled = true;
              a.failed = false;
              break;
            }
          }
          for (auto &r : downloadRecords_) {
            if (r.fileName == name) {
              r.completed = false;
              r.cancelled = true;
              r.failed = false;
              if (r.totalBytes <= 0) r.totalBytes = download->totalBytes();
              break;
            }
          }
          saveDownloadRecords();
          statusBar()->showMessage("✕ ยกเลิกการดาวน์โหลด: " + name, 5000);
          updateDownloadsButtonUi();
        } else if (state == QWebEngineDownloadRequest::DownloadInterrupted) {
          for (auto &a : activeDownloads_) {
            if (a.request == download) {
              a.completed = false;
              a.cancelled = false;
              a.failed = true;
              break;
            }
          }
          for (auto &r : downloadRecords_) {
            if (r.fileName == name) {
              r.completed = false;
              r.cancelled = false;
              r.failed = true;
              if (r.totalBytes <= 0) r.totalBytes = download->totalBytes();
              break;
            }
          }
          saveDownloadRecords();
          statusBar()->showMessage("✕ การดาวน์โหลดล้มเหลว: " + name, 5000);
          updateDownloadsButtonUi();
        }
      });

  download->accept();
  statusBar()->showMessage("เริ่มการดาวน์โหลด: " + name, 3000);
  showDownloadPopup();
}

void MainWindow::showDownloadsDialog() {
  auto *dialog = new QDialog(this);
  dialog->setWindowTitle("รายการดาวน์โหลด (Downloads)");
  dialog->resize(640, 420);

  const QString bg = darkMode_ ? "#0f172a" : "#ffffff";
  const QString cardBg = darkMode_ ? "#1e293b" : "#f8fafc";
  const QString itemHover = darkMode_ ? "#334155" : "#f1f5f9";
  const QString textCol = darkMode_ ? "#f8fafc" : "#0f172a";
  const QString borderCol = darkMode_ ? "rgba(255,255,255,0.12)" : "rgba(0,0,0,0.12)";
  const QString primaryCol = "#0ea5e9";

  dialog->setStyleSheet(QString(R"(
    QDialog {
      background-color: %1;
      color: %2;
    }
    QLabel {
      color: %2;
      font-size: 13px;
    }
    QListWidget {
      background-color: %3;
      color: %2;
      border: 1px solid %4;
      border-radius: 8px;
      padding: 6px;
      font-size: 13px;
    }
    QListWidget::item {
      padding: 10px 12px;
      border-bottom: 1px solid %4;
      border-radius: 6px;
      margin-bottom: 2px;
    }
    QListWidget::item:hover {
      background-color: %5;
    }
    QListWidget::item:selected {
      background-color: rgba(14, 165, 233, 0.2);
      color: %2;
    }
    QPushButton {
      background-color: %3;
      color: %2;
      border: 1px solid %4;
      border-radius: 6px;
      padding: 7px 14px;
      font-size: 13px;
      font-weight: 500;
    }
    QPushButton:hover {
      background-color: rgba(14, 165, 233, 0.15);
      border-color: %6;
    }
    QPushButton#primaryBtn {
      background-color: %6;
      color: #ffffff;
      border: none;
      font-weight: 600;
    }
    QPushButton#primaryBtn:hover {
      background-color: #0284c7;
    }
    QPushButton#dangerBtn:hover {
      background-color: rgba(239, 68, 68, 0.15);
      border-color: #ef4444;
      color: #ef4444;
    }
  )").arg(bg, textCol, cardBg, borderCol, itemHover, primaryCol));

  auto *layout = new QVBoxLayout(dialog);
  layout->setContentsMargins(18, 18, 18, 18);
  layout->setSpacing(12);

  auto *headerLbl = new QLabel("⬇️ รายการดาวน์โหลด", dialog);
  headerLbl->setStyleSheet("font-size: 16px; font-weight: 700;");
  layout->addWidget(headerLbl);

  auto *listWidget = new QListWidget(dialog);

  auto refreshList = [this, listWidget] {
    listWidget->clear();
    if (downloadRecords_.isEmpty()) {
      auto *emptyItem = new QListWidgetItem("ยังไม่มีประวัติการดาวน์โหลด", listWidget);
      emptyItem->setFlags(Qt::NoItemFlags);
    } else {
      for (const auto &rec : downloadRecords_) {
        QString statusText = rec.completed ? "✓ เสร็จสิ้น" : (rec.cancelled ? "✕ ยกเลิกแล้ว" : (rec.failed ? "✕ ล้มเหลว" : "⟳ กำลังดำเนินการ"));
        QString sizeText;
        if (rec.totalBytes > 0) {
          if (rec.totalBytes >= 1024 * 1024) {
            sizeText = QString(" (%1 MB)").arg(QString::number(rec.totalBytes / (1024.0 * 1024.0), 'f', 1));
          } else {
            sizeText = QString(" (%1 KB)").arg(rec.totalBytes / 1024);
          }
        }
        QString line = QString("%1%2\n[%3] %4").arg(rec.fileName, sizeText, statusText, rec.path);
        auto *item = new QListWidgetItem(line, listWidget);
        item->setData(Qt::UserRole, rec.path);
      }
    }
  };
  refreshList();

  layout->addWidget(listWidget);

  auto *btnLayout = new QHBoxLayout();
  btnLayout->setSpacing(8);

  auto *openFileBtn = new QPushButton("เปิดไฟล์", dialog);
  openFileBtn->setObjectName("primaryBtn");
  auto *openDirBtn = new QPushButton("เปิดโฟลเดอร์", dialog);
  auto *deleteItemBtn = new QPushButton("ลบจากรายการ", dialog);
  deleteItemBtn->setObjectName("dangerBtn");
  auto *clearAllBtn = new QPushButton("ล้างประวัติทั้งหมด", dialog);
  clearAllBtn->setObjectName("dangerBtn");
  auto *closeBtn = new QPushButton("ปิด", dialog);

  btnLayout->addWidget(openFileBtn);
  btnLayout->addWidget(openDirBtn);
  btnLayout->addWidget(deleteItemBtn);
  btnLayout->addWidget(clearAllBtn);
  btnLayout->addStretch();
  btnLayout->addWidget(closeBtn);
  layout->addLayout(btnLayout);

  auto openCurrentFile = [listWidget] {
    auto *item = listWidget->currentItem();
    if (item) {
      const QString path = item->data(Qt::UserRole).toString();
      if (!path.isEmpty() && QFileInfo::exists(path)) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
      }
    }
  };

  connect(openFileBtn, &QPushButton::clicked, dialog, openCurrentFile);
  connect(listWidget, &QListWidget::itemDoubleClicked, dialog, [openCurrentFile](QListWidgetItem *) {
    openCurrentFile();
  });

  connect(openDirBtn, &QPushButton::clicked, dialog, [listWidget] {
    auto *item = listWidget->currentItem();
    QString dir;
    if (item) {
      const QString path = item->data(Qt::UserRole).toString();
      if (!path.isEmpty()) {
        QFileInfo fi(path);
        dir = fi.absolutePath();
      }
    }
    if (dir.isEmpty() || !QDir(dir).exists()) {
      QSettings st("LiteWave", "LiteWave");
      dir = st.value("downloadDirectory",
                     QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)).toString();
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
  });

  connect(deleteItemBtn, &QPushButton::clicked, dialog, [this, listWidget, refreshList] {
    const int row = listWidget->currentRow();
    if (row >= 0 && row < downloadRecords_.size()) {
      downloadRecords_.removeAt(row);
      saveDownloadRecords();
      refreshList();
    }
  });

  connect(clearAllBtn, &QPushButton::clicked, dialog, [this, refreshList] {
    downloadRecords_.clear();
    saveDownloadRecords();
    refreshList();
  });

  connect(closeBtn, &QPushButton::clicked, dialog, &QDialog::accept);

  dialog->exec();
  dialog->deleteLater();
}

void MainWindow::clearBrowsingDataDialog() {
  const auto answer = QMessageBox::question(
      this, "ล้างข้อมูลการท่องเว็บ",
      "คุณต้องการลบข้อมูลประวัติการใช้เว็บ แคชของเบราว์เซอร์ และคุกกี้ใช่หรือไม่?",
      QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

  if (answer == QMessageBox::Yes) {
    QSettings st("LiteWave", "LiteWave");
    st.remove("history");
    closedTabs_.clear();
    if (profile_) {
      profile_->clearHttpCache();
    }
    statusBar()->showMessage("ล้างประวัติ แคช และข้อมูลการท่องเว็บเรียบร้อยแล้ว", 5000);
  }
}


QMenu *MainWindow::createMainMenu() {
  auto *menu = new QMenu(this);
  menu->setObjectName("mainAppMenu");

  const QColor iconColor = darkMode_ ? QColor("#94a3b8") : QColor("#475569");

  auto *newTabAct = menu->addAction(createMenuIcon("new_tab", iconColor), "แท็บใหม่\tCtrl+T");
  connect(newTabAct, &QAction::triggered, this, &MainWindow::newTab);

  auto *newWinAct = menu->addAction(createMenuIcon("new_window", iconColor), "หน้าต่างใหม่\tCtrl+N");
  connect(newWinAct, &QAction::triggered, this, [this] {
    auto *w = new MainWindow(nullptr, privateMode_);
    w->setAttribute(Qt::WA_DeleteOnClose);
    w->show();
  });

  auto *newPrivWinAct = menu->addAction(createMenuIcon("incognito", iconColor), "หน้าต่างส่วนตัวใหม่\tCtrl+Shift+N");
  connect(newPrivWinAct, &QAction::triggered, this, [] {
    auto *w = new MainWindow(nullptr, true);
    w->setAttribute(Qt::WA_DeleteOnClose);
    w->show();
  });

  menu->addSeparator();

  auto *historyMenu = menu->addMenu("ประวัติการใช้งาน");
  historyMenu->setIcon(createMenuIcon("history", iconColor));
  connect(historyMenu, &QMenu::aboutToShow, this,
          [this, historyMenu] { populateHistoryMenu(historyMenu); });
  populateHistoryMenu(historyMenu);

  auto *bookmarksMenu = menu->addMenu("บุ๊กมาร์ก");
  bookmarksMenu->setIcon(createMenuIcon("bookmark", iconColor));
  connect(bookmarksMenu, &QMenu::aboutToShow, this,
          [this, bookmarksMenu] { populateBookmarksMenu(bookmarksMenu); });
  populateBookmarksMenu(bookmarksMenu);

  auto *downloadsAct = menu->addAction(createMenuIcon("download", iconColor), "การดาวน์โหลด\tCtrl+J");
  downloadsAct->setShortcut(QKeySequence("Ctrl+J"));
  connect(downloadsAct, &QAction::triggered, this,
          &MainWindow::showDownloadPopup);

  auto *clearDataAct = menu->addAction(createMenuIcon("trash", iconColor), "ล้างข้อมูลการท่องเว็บ…\tCtrl+Shift+Del");
  connect(clearDataAct, &QAction::triggered, this,
          &MainWindow::clearBrowsingDataDialog);

  menu->addSeparator();

  auto *zoomMenu = menu->addMenu("ซูมหน้าเว็บ");
  zoomMenu->setIcon(createMenuIcon("zoom", iconColor));
  auto *zoomInAct = zoomMenu->addAction(createMenuIcon("zoom_in", iconColor), "ขยาย (+10%)");
  connect(zoomInAct, &QAction::triggered, this, [this] {
    if (currentView())
      currentView()->setZoomFactor(
          std::min(5.0, currentView()->zoomFactor() + 0.1));
  });
  auto *zoomOutAct = zoomMenu->addAction(createMenuIcon("zoom_out", iconColor), "ย่อ (-10%)");
  connect(zoomOutAct, &QAction::triggered, this, [this] {
    if (currentView())
      currentView()->setZoomFactor(
          std::max(0.25, currentView()->zoomFactor() - 0.1));
  });
  auto *zoomResetAct = zoomMenu->addAction(createMenuIcon("zoom_reset", iconColor), "ขนาดปกติ (100%)\tCtrl+0");
  connect(zoomResetAct, &QAction::triggered, this, [this] {
    if (currentView())
      currentView()->setZoomFactor(1.0);
  });
  auto *fullScreenAct = zoomMenu->addAction(createMenuIcon("fullscreen", iconColor), "เต็มจอ (Full Screen)\tF11");
  connect(fullScreenAct, &QAction::triggered, this, [this] {
    if (isFullScreen()) {
      showNormal();
      if (headerWidget_)
        headerWidget_->show();
      statusBar()->show();
    } else {
      if (headerWidget_)
        headerWidget_->hide();
      statusBar()->hide();
      showFullScreen();
    }
  });

  menu->addSeparator();

  auto *printAct = menu->addAction(createMenuIcon("print", iconColor), "พิมพ์…\tCtrl+P");
  connect(printAct, &QAction::triggered, this, [this] {
    if (!currentView())
      return;
    auto *printer = new QPrinter(QPrinter::HighResolution);
    QPrintDialog dialog(printer, this);
    if (dialog.exec() == QDialog::Accepted) {
      connect(
          currentView(), &QWebEngineView::printFinished, currentView(),
          [printer](bool) { delete printer; }, Qt::SingleShotConnection);
      currentView()->print(printer);
    } else {
      delete printer;
    }
  });

  auto *findAct = menu->addAction(createMenuIcon("find", iconColor), "ค้นหาในหน้าเว็บ…\tCtrl+F");
  connect(findAct, &QAction::triggered, this, [this] {
    if (findBar_ && currentView()) {
      findBar_->attachView(currentView());
      findBar_->showAndFocus(findQuery_);
    }
  });

  auto *saveAct = menu->addAction(createMenuIcon("save", iconColor), "บันทึกหน้าเว็บ…\tCtrl+S");
  connect(saveAct, &QAction::triggered, this, [this] {
    if (!currentView())
      return;
    const QString path = QFileDialog::getSaveFileName(
        this, "บันทึกหน้าเว็บ", QString(), "Web archive (*.mhtml)");
    if (!path.isEmpty())
      currentView()->page()->save(
          path, QWebEngineDownloadRequest::MimeHtmlSaveFormat);
  });

  auto *devToolsAct = menu->addAction(createMenuIcon("devtools", iconColor), "เครื่องมือนักพัฒนา (DevTools)\tF12");
  connect(devToolsAct, &QAction::triggered, this, [this] {
    openDevTools(currentView());
  });

  menu->addSeparator();

  auto *themeAct =
      menu->addAction(createMenuIcon("theme", iconColor),
                      darkMode_ ? "เปลี่ยนเป็นโหมดสว่าง" : "เปลี่ยนเป็นโหมดมืด");
  connect(themeAct, &QAction::triggered, this, &MainWindow::toggleTheme);

  auto *settingsAct = menu->addAction(createMenuIcon("settings", iconColor), "การตั้งค่า (Settings)");
  connect(settingsAct, &QAction::triggered, this,
          &MainWindow::showSettingsDialog);

  auto *updateAct = menu->addAction(createMenuIcon("update", iconColor), "ตรวจสอบการอัปเดต (Check for Updates)…");
  connect(updateAct, &QAction::triggered, this, [this] {
    checkForUpdates(false);
  });

  auto *feedbackAct = menu->addAction(createMenuIcon("feedback", iconColor), "ส่งข้อเสนอแนะ / แจ้งปัญหา (Send Feedback)…");
  connect(feedbackAct, &QAction::triggered, this, [this] {
    createView(QUrl("https://github.com/ynmio55/LiteWave/issues/new"));
  });

  auto *aboutAct = menu->addAction(createMenuIcon("about", iconColor), "เกี่ยวกับ LiteWave");
  connect(aboutAct, &QAction::triggered, this, [this] {
    QMessageBox msg(this);
    msg.setWindowTitle("เกี่ยวกับ LiteWave Browser");
    msg.setIconPixmap(QPixmap(":/icons/litewave.png").scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    msg.setText(QString("<h3>LiteWave Browser v%1</h3>").arg(LITEWAVE_APP_VERSION));
    msg.setInformativeText(
        "<p>เบราว์เซอร์ความเร็วสูง น้ำหนักเบา ปลอดภัย และใช้งานง่าย</p>"
        "<p><b>ฟีเจอร์หลัก:</b>"
        "<ul>"
        "<li>รองรับการแสดงผลทุกเว็บไซต์ ความบันเทิง วิดีโอ และสื่อได้อย่างเต็มรูปแบบ</li>"
        "<li>ระบบ Secure DNS (DNS-over-HTTPS) คุณภาพสูง</li>"
        "<li>รองรับแท็บหลายหน้าต่าง และโหมดส่วนตัว (Private Mode)</li>"
        "<li>โหมดสว่าง/มืด (Dark/Light Mode)</li>"
        "<li>ค้นหาด่วน Google/Brave/DuckDuckGo</li>"
        "</ul></p>");
    msg.exec();
  });

  auto *exitAct = menu->addAction(createMenuIcon("exit", iconColor), "ออกจากโปรแกรม\tAlt+F4");
  connect(exitAct, &QAction::triggered, this, &QWidget::close);

  return menu;
}

void MainWindow::checkForUpdates(bool silentIfUpToDate) {
  if (!updateNam_) {
    updateNam_ = new QNetworkAccessManager(this);
  }

  if (!silentIfUpToDate) {
    statusBar()->showMessage("กำลังตรวจสอบการอัปเดต...", 3000);
  }

  QNetworkRequest request(QUrl("https://api.github.com/repos/ynmio55/LiteWave/releases/latest"));
  request.setHeader(QNetworkRequest::UserAgentHeader, QString("LiteWave-Browser/%1").arg(LITEWAVE_APP_VERSION));
  request.setRawHeader("Accept", "application/vnd.github.v3+json");

  QNetworkReply *reply = updateNam_->get(request);
  connect(reply, &QNetworkReply::finished, this, [this, reply, silentIfUpToDate]() {
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
      if (!silentIfUpToDate) {
        QMessageBox::warning(this, "ตรวจสอบการอัปเดต",
                             "ไม่สามารถเชื่อมต่อกับเซิร์ฟเวอร์เพื่อตรวจสอบการอัปเดตได้\nกรุณาตรวจสอบการเชื่อมต่ออินเทอร์เน็ต");
      }
      return;
    }

    const QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
      if (!silentIfUpToDate) {
        QMessageBox::warning(this, "ตรวจสอบการอัปเดต", "ข้อมูลการอัปเดตไม่ถูกต้อง");
      }
      return;
    }

    QJsonObject root = doc.object();
    QString releaseTitle = root.value("name").toString().trimmed();
    QString tagName = root.value("tag_name").toString().trimmed();
    QString targetCommit = root.value("target_commitish").toString().trimmed();
    QJsonArray assets = root.value("assets").toArray();

    QString targetAssetUrl;
    QString targetAssetName;
    qint64 targetAssetSize = 0;

#ifdef Q_OS_WIN
    for (const auto &val : assets) {
      QJsonObject a = val.toObject();
      QString name = a.value("name").toString();
      if (name.endsWith("-Setup-Windows-x64.exe", Qt::CaseInsensitive)) {
        targetAssetUrl = a.value("browser_download_url").toString();
        targetAssetName = name;
        targetAssetSize = a.value("size").toInteger();
        break;
      }
    }
#else
    for (const auto &val : assets) {
      QJsonObject a = val.toObject();
      QString name = a.value("name").toString();
      if (name.endsWith("-Linux-x64.tar.gz", Qt::CaseInsensitive)) {
        targetAssetUrl = a.value("browser_download_url").toString();
        targetAssetName = name;
        targetAssetSize = a.value("size").toInteger();
        break;
      }
    }
#endif

    QString localVer = QString::fromUtf8(LITEWAVE_APP_VERSION).trimmed();
    QString localCommit = QString::fromUtf8(LITEWAVE_GIT_COMMIT).trimmed();

    QString remoteVerClean;
    QList<int> remoteV = parseVersionNumbers(tagName, &remoteVerClean);
    if (remoteV == QList<int>{0, 0, 0}) {
      remoteV = parseVersionNumbers(releaseTitle, &remoteVerClean);
    }
    QList<int> localV = parseVersionNumbers(localVer);

    // Check if remote version is strictly newer than local version
    bool isNewerVersion = false;
    if (remoteV[0] > localV[0]) {
      isNewerVersion = true;
    } else if (remoteV[0] == localV[0]) {
      if (remoteV[1] > localV[1]) {
        isNewerVersion = true;
      } else if (remoteV[1] == localV[1]) {
        if (remoteV[2] > localV[2]) {
          isNewerVersion = true;
        }
      }
    }

    // The compiled binary version is the source of truth. Persistent update
    // records are informational only; stale QSettings must never make an older
    // binary believe it is already updated.
    const bool sameCommit =
        !targetCommit.isEmpty() && !localCommit.isEmpty() &&
        localCommit != "dev" &&
        (targetCommit.startsWith(localCommit) ||
         localCommit.startsWith(targetCommit));

    // If the binary version is older, always offer the update when a platform
    // asset exists. Commit equality only matters when versions are already equal.
    const bool hasNewVersion =
        isNewerVersion && !targetAssetUrl.isEmpty();

    // Clean stale bookkeeping left by older updater versions.
    QSettings st("LiteWave", "LiteWave");
    const QString installedVersion =
        st.value("update/installedVersion").toString().trimmed();
    if (!installedVersion.isEmpty() && installedVersion != localVer) {
      st.remove("update/installedVersion");
      st.remove("update/installedCommit");
      st.sync();
    }

    if (!hasNewVersion) {
      if (!silentIfUpToDate) {
        QMessageBox::information(this, "ตรวจสอบการอัปเดต",
                                 QString("คุณกำลังใช้งาน LiteWave เวอร์ชันล่าสุดแล้ว (v%1)\nไม่มีการอัปเดตใหม่ในขณะนี้")
                                     .arg(localVer));
      }
      return;
    }

    double sizeMb = targetAssetSize > 0 ? (targetAssetSize / (1024.0 * 1024.0)) : 0.0;
    QString sizeStr = sizeMb > 0 ? QString(" (ขนาด: %1 MB)").arg(sizeMb, 0, 'f', 1) : "";

    QMessageBox updateBox(this);
    updateBox.setWindowTitle("พบเวอร์ชันใหม่ - LiteWave");
    updateBox.setIconPixmap(QPixmap(":/icons/litewave.png").scaled(56, 56, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    updateBox.setText(QString("<h3>มี LiteWave เวอร์ชันใหม่พร้อมติดตั้ง!</h3><p><b>%1</b>%2</p>")
                          .arg(releaseTitle.isEmpty() ? ("LiteWave v" + remoteVerClean) : releaseTitle, sizeStr));
    updateBox.setInformativeText(
        "<p>คุณต้องการดาวน์โหลดและติดตั้งการอัปเดตทันทีหรือไม่?</p>"
        "<p style='color: #64748b; font-size: 11px;'>ระบบจะดาวน์โหลดตัวติดตั้งและอัปเดตโปรแกรมให้โดยอัตโนมัติ โดยข้อมูลการใช้งานทั้งหมดจะยังคงอยู่ครบถ้วน</p>");

    auto *installBtn = updateBox.addButton("ดาวน์โหลดและอัปเดตทันที", QMessageBox::AcceptRole);
    auto *webBtn = updateBox.addButton("เปิดหน้าเว็บดาวน์โหลด", QMessageBox::ActionRole);
    updateBox.addButton("ยกเลิก", QMessageBox::RejectRole);

    updateBox.exec();

    if (updateBox.clickedButton() == installBtn) {
      downloadAndInstallUpdate(targetAssetUrl, targetAssetName, remoteVerClean, targetCommit);
    } else if (updateBox.clickedButton() == webBtn) {
      QDesktopServices::openUrl(QUrl("https://litewave.miosmooth.com"));
    }
  });
}

void MainWindow::downloadAndInstallUpdate(const QString &downloadUrl, const QString &fileName,
                                          const QString &newVersion, const QString &newCommit) {
  if (downloadUrl.isEmpty() || fileName.isEmpty()) return;

  QString savePath = QDir::tempPath() + "/" + fileName;
  auto *file = new QFile(savePath);
  if (!file->open(QIODevice::WriteOnly)) {
    delete file;
    QMessageBox::critical(this, "ข้อผิดพลาด", "ไม่สามารถสร้างไฟล์ชั่วคราวสำหรับอัปเดตได้");
    return;
  }

  auto *progress = new QProgressDialog("กำลังดาวน์โหลดตัวอัปเดต LiteWave...", "ยกเลิก", 0, 100, this);
  progress->setWindowTitle("กำลังดาวน์โหลดอัปเดต");
  progress->setWindowModality(Qt::WindowModal);
  progress->setMinimumDuration(0);
  progress->setValue(0);
  progress->show();

  QNetworkRequest request(downloadUrl);
  request.setHeader(QNetworkRequest::UserAgentHeader, QString("LiteWave-Browser/%1").arg(LITEWAVE_APP_VERSION));
  request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
  request.setAttribute(QNetworkRequest::Http2AllowedAttribute, false);

  QNetworkReply *reply = updateNam_->get(request);
  reply->setReadBufferSize(4 * 1024 * 1024);

  auto lastUpdateMs = std::make_shared<qint64>(0);
  connect(reply, &QNetworkReply::downloadProgress, this, [progress, lastUpdateMs](qint64 received, qint64 total) {
    if (total > 0) {
      qint64 now = QDateTime::currentMSecsSinceEpoch();
      int pct = static_cast<int>((received * 100) / total);
      if (now - *lastUpdateMs > 150 || pct >= 100) {
        *lastUpdateMs = now;
        progress->setValue(pct);
        double mbReceived = received / (1024.0 * 1024.0);
        double mbTotal = total / (1024.0 * 1024.0);
        progress->setLabelText(QString("กำลังดาวน์โหลด: %1 / %2 MB (%3%)")
                                   .arg(mbReceived, 0, 'f', 1)
                                   .arg(mbTotal, 0, 'f', 1)
                                   .arg(pct));
      }
    }
  });

  connect(reply, &QNetworkReply::readyRead, this, [reply, file]() {
    file->write(reply->readAll());
  });

  connect(progress, &QProgressDialog::canceled, this, [reply]() {
    reply->abort();
  });

  connect(reply, &QNetworkReply::finished, this, [this, reply, file, progress, savePath, newVersion, newCommit]() {
    progress->close();
    progress->deleteLater();
    file->flush();
    file->close();
    delete file;
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
      QFile::remove(savePath);
      if (reply->error() != QNetworkReply::OperationCanceledError) {
        QMessageBox::warning(this, "การดาวน์โหลดล้มเหลว",
                             "ไม่สามารถดาวน์โหลดไฟล์อัปเดตได้สำเร็จ กรุณาลองใหม่อีกครั้ง");
      }
      return;
    }

#ifdef Q_OS_WIN
    // Windows updates are installed in-place with the same Inno Setup AppId.
    // Do not mark the update as installed before Setup succeeds; the compiled
    // application version is the source of truth after relaunch.
    QMessageBox::information(
        this, "พร้อมอัปเดต",
        QString("ดาวน์โหลด LiteWave v%1 เรียบร้อยแล้ว\n"
                "โปรแกรมจะปิด ติดตั้งการอัปเดตอัตโนมัติ และเปิดกลับมาอีกครั้ง")
            .arg(newVersion));

    const QStringList setupArgs = {
        "/VERYSILENT",
        "/SUPPRESSMSGBOXES",
        "/NORESTART",
        "/CLOSEAPPLICATIONS",
        "/RESTARTAPPLICATIONS"
    };

    if (!QProcess::startDetached(savePath, setupArgs)) {
      QMessageBox::critical(
          this, "อัปเดตไม่สำเร็จ",
          "ไม่สามารถเปิดตัวติดตั้งอัปเดตได้ กรุณาลองใหม่อีกครั้ง");
      return;
    }

    qApp->quit();
#else
    // Linux archive updates remain manual for now.
    QSettings st("LiteWave", "LiteWave");
    if (!newVersion.isEmpty())
      st.setValue("update/downloadedVersion", newVersion);
    if (!newCommit.isEmpty())
      st.setValue("update/downloadedCommit", newCommit);
    QMessageBox::information(this, "ดาวน์โหลดเสร็จสมบูรณ์",
                             QString("ดาวน์โหลดไฟล์อัปเดตเรียบร้อยแล้ว:\n%1\n\nระบบจะเปิดโฟลเดอร์ไฟล์ให้เพื่อทำการแตกไฟล์ใช้งาน").arg(savePath));
    QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(savePath).absolutePath()));
#endif
  });
}

static QIcon createCategoryIcon(const QString &name, const QColor &color) {
  QPixmap pix(24, 24);
  pix.fill(Qt::transparent);
  QPainter p(&pix);
  p.setRenderHint(QPainter::Antialiasing);
  p.setPen(QPen(color, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  p.setBrush(Qt::NoBrush);

  if (name == "appearance") {
    p.drawEllipse(3, 3, 18, 18);
    p.setBrush(color);
    p.drawEllipse(8, 8, 2, 2);
    p.drawEllipse(13, 7, 2, 2);
    p.drawEllipse(16, 11, 2, 2);
  } else if (name == "search") {
    p.drawEllipse(4, 4, 11, 11);
    p.drawLine(13, 13, 19, 19);
  } else if (name == "startup") {
    p.drawArc(4, 5, 16, 16, 30 * 16, 300 * 16);
    p.drawLine(12, 2, 12, 11);
  } else if (name == "shield") {
    QPainterPath path;
    path.moveTo(12, 3);
    path.lineTo(20, 6);
    path.lineTo(20, 13);
    path.cubicTo(20, 18, 12, 21, 12, 21);
    path.cubicTo(12, 21, 4, 18, 4, 13);
    path.lineTo(4, 6);
    path.closeSubpath();
    p.drawPath(path);
  } else if (name == "privacy") {
    p.drawRoundedRect(5, 10, 14, 11, 2, 2);
    p.drawArc(8, 4, 8, 10, 0, 180 * 16);
  } else if (name == "bookmark") {
    QPolygonF star;
    star << QPointF(12, 4) << QPointF(14.5, 9.5) << QPointF(20, 10.2)
         << QPointF(16, 14.1) << QPointF(17, 19.8) << QPointF(12, 17)
         << QPointF(7, 19.8) << QPointF(8, 14.1) << QPointF(4, 10.2)
         << QPointF(9.5, 9.5);
    p.drawPolygon(star);
  } else if (name == "downloads") {
    p.drawLine(12, 4, 12, 15);
    p.drawLine(8, 11, 12, 15);
    p.drawLine(16, 11, 12, 15);
    p.drawLine(4, 19, 20, 19);
  } else if (name == "reset") {
    p.drawArc(4, 4, 16, 16, 45 * 16, 270 * 16);
    p.drawLine(14, 2, 18, 5);
    p.drawLine(14, 8, 18, 5);
  }

  return QIcon(pix);
}

void MainWindow::showSettingsDialog(int initialPage) {
  QDialog dialog(this);
  dialog.setWindowTitle("การตั้งค่า LiteWave (Settings)");
  dialog.setMinimumSize(780, 560);

  QSettings st("LiteWave", "LiteWave");
  const bool isDark = darkMode_;
  const QString dialogBg = isDark ? "#111827" : "#f8fafc";
  const QString cardBg = isDark ? "#1f2937" : "#ffffff";
  const QString textColor = isDark ? "#f9fafc" : "#0f172a";
  const QString subTextColor = isDark ? "#9ca3af" : "#64748b";
  const QString borderColor = isDark ? "#374151" : "#e2e8f0";
  const QColor iconColor = isDark ? QColor("#38bdf8") : QColor("#0284c7");

  dialog.setStyleSheet(QString(R"(
    QDialog {
      background-color: %1;
      color: %2;
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
    }
    QGroupBox {
      font-size: 14px;
      font-weight: bold;
      color: %2;
      background-color: %3;
      border: 1px solid %4;
      border-radius: 10px;
      margin-top: 8px;
      padding: 16px;
    }
    QGroupBox::title {
      subcontrol-origin: margin;
      left: 12px;
      padding: 0 6px;
    }
    QLabel {
      color: %2;
      font-size: 13px;
    }
    QCheckBox, QRadioButton {
      color: %2;
      font-size: 13px;
      spacing: 8px;
    }
    QLineEdit, QComboBox {
      background-color: %1;
      color: %2;
      border: 1px solid %4;
      border-radius: 6px;
      padding: 6px 10px;
      font-size: 13px;
    }
    QLineEdit:focus, QComboBox:focus {
      border: 1px solid #0ea5e9;
    }
    QPushButton {
      background-color: %3;
      color: %2;
      border: 1px solid %4;
      border-radius: 6px;
      padding: 6px 14px;
      font-size: 13px;
      font-weight: 500;
    }
    QPushButton:hover {
      border-color: #0284c7;
      color: #0284c7;
    }
  )")
                           .arg(dialogBg, textColor, cardBg, borderColor));

  auto *mainLayout = new QVBoxLayout(&dialog);
  mainLayout->setContentsMargins(20, 20, 20, 20);
  mainLayout->setSpacing(16);

  auto *headerLabel = new QLabel("การตั้งค่า LiteWave", &dialog);
  headerLabel->setStyleSheet(
      "font-size: 20px; font-weight: bold; letter-spacing: -0.5px;");
  mainLayout->addWidget(headerLabel);

  auto *contentLayout = new QHBoxLayout();
  contentLayout->setSpacing(20);

  auto *sidebar = new QListWidget(&dialog);
  sidebar->setFixedWidth(210);
  sidebar->setIconSize(QSize(20, 20));
  sidebar->setStyleSheet(
      QString(R"(
    QListWidget {
      background-color: %1;
      border: 1px solid %2;
      border-radius: 10px;
      padding: 6px;
      font-size: 13px;
    }
    QListWidget::item {
      padding: 10px 12px;
      border-radius: 6px;
      margin-bottom: 2px;
      color: %3;
    }
    QListWidget::item:hover {
      background-color: %4;
    }
    QListWidget::item:selected {
      background-color: #0284c7;
      color: #ffffff;
      font-weight: bold;
    }
  )")
          .arg(cardBg, borderColor, textColor, isDark ? "#374151" : "#f1f5f9"));

  auto addSidebarItem = [&](const QString &title, const QString &iconName) {
    auto *item = new QListWidgetItem(createCategoryIcon(iconName, iconColor),
                                     title, sidebar);
    return item;
  };

  addSidebarItem("การแสดงผล", "appearance");
  addSidebarItem("เครื่องมือค้นหา", "search");
  addSidebarItem("เมื่อเริ่มต้นทำงาน", "startup");
  addSidebarItem("ความเป็นส่วนตัว", "privacy");
  addSidebarItem("Shield", "shield");
  addSidebarItem("Secure DNS", "privacy");
  addSidebarItem("บุ๊กมาร์ก", "bookmark");
  addSidebarItem("การดาวน์โหลด", "downloads");
  addSidebarItem("รีเซ็ตการตั้งค่า", "reset");

  auto *stacked = new QStackedWidget(&dialog);

  // ---------------- Page 0: Appearance ----------------
  auto *page0 = new QWidget();
  auto *p0Layout = new QVBoxLayout(page0);
  p0Layout->setSpacing(16);
  p0Layout->setContentsMargins(0, 0, 0, 0);

  auto *grpTheme = new QGroupBox("การแสดงผลและธีม", page0);
  auto *grpThemeLayout = new QVBoxLayout(grpTheme);
  grpThemeLayout->setSpacing(12);

  auto *darkThemeBox = new QCheckBox("เปิดใช้งานโหมดสีมืด (Dark Theme)", grpTheme);
  darkThemeBox->setChecked(darkMode_);

  auto *showStatusBarBox =
      new QCheckBox("แสดงแถบสถานะด้านล่าง (Status Bar)", grpTheme);
  showStatusBarBox->setChecked(statusBar()->isVisible());

  grpThemeLayout->addWidget(darkThemeBox);
  grpThemeLayout->addWidget(showStatusBarBox);
  p0Layout->addWidget(grpTheme);
  p0Layout->addStretch();
  stacked->addWidget(page0);

  // ---------------- Page 1: Search Engine ----------------
  auto *page1 = new QWidget();
  auto *p1Layout = new QVBoxLayout(page1);
  p1Layout->setSpacing(16);
  p1Layout->setContentsMargins(0, 0, 0, 0);

  auto *grpSearch = new QGroupBox("เครื่องมือค้นหาเริ่มต้น", page1);
  auto *grpSearchLayout = new QVBoxLayout(grpSearch);
  grpSearchLayout->setSpacing(10);

  auto *lblSearch = new QLabel(
      "เครื่องมือค้นหาที่จะใช้เมื่อระบุคำค้นหาในช่องที่อยู่ (Address Bar):", grpSearch);
  auto *searchCombo = new QComboBox(grpSearch);
  const auto availableEngines = SearchEngineManager::instance().availableEngines();
  const auto curEngine = SearchEngineManager::instance().currentEngine();
  int activeIdx = 0;
  for (int i = 0; i < availableEngines.size(); ++i) {
    searchCombo->addItem(availableEngines[i].name, availableEngines[i].id);
    if (availableEngines[i].id == curEngine.id)
      activeIdx = i;
  }
  searchCombo->setCurrentIndex(activeIdx);

  grpSearchLayout->addWidget(lblSearch);
  grpSearchLayout->addWidget(searchCombo);

  auto *enableSuggestionsBox = new QCheckBox(
      "แสดงรายการคำแนะนำและประวัติการค้นหาขณะพิมพ์ (Search Suggestions Dropdown)", grpSearch);
  enableSuggestionsBox->setChecked(st.value("search/enableSuggestions", true).toBool());
  grpSearchLayout->addWidget(enableSuggestionsBox);
  p1Layout->addWidget(grpSearch);
  p1Layout->addStretch();
  stacked->addWidget(page1);

  // ---------------- Page 2: On Startup ----------------
  auto *page2 = new QWidget();
  auto *p2Layout = new QVBoxLayout(page2);
  p2Layout->setSpacing(16);
  p2Layout->setContentsMargins(0, 0, 0, 0);

  auto *grpStartup = new QGroupBox("เมื่อเปิดโปรแกรมเบราว์เซอร์", page2);
  auto *grpStartupLayout = new QVBoxLayout(grpStartup);
  grpStartupLayout->setSpacing(10);

  auto *rbHome =
      new QRadioButton("เปิดหน้าเริ่มต้นแท็บใหม่ (LiteWave Home Page)", grpStartup);
  auto *rbRestore =
      new QRadioButton("เปิดแท็บเดิมที่ค้างไว้จากการใช้งานครั้งล่าสุด (Continue where you left off)", grpStartup);
  auto *rbCustom =
      new QRadioButton("เปิดหน้าเว็บที่กำหนดเฉพาะ (Custom URL):", grpStartup);

  auto *customUrlEdit = new QLineEdit(grpStartup);
  customUrlEdit->setPlaceholderText("https://example.com");
  customUrlEdit->setText(st.value("customStartupUrl", "").toString());

  const QString startupOpt = st.value("startupOption", "home").toString();
  if (startupOpt == "restore") {
    rbRestore->setChecked(true);
    customUrlEdit->setEnabled(false);
  } else if (startupOpt == "custom") {
    rbCustom->setChecked(true);
    customUrlEdit->setEnabled(true);
  } else {
    rbHome->setChecked(true);
    customUrlEdit->setEnabled(false);
  }
  connect(rbCustom, &QRadioButton::toggled, customUrlEdit,
          &QLineEdit::setEnabled);

  grpStartupLayout->addWidget(rbHome);
  grpStartupLayout->addWidget(rbRestore);
  grpStartupLayout->addWidget(rbCustom);
  grpStartupLayout->addWidget(customUrlEdit);
  p2Layout->addWidget(grpStartup);
  p2Layout->addStretch();
  stacked->addWidget(page2);

  // ---------------- Page 3: Privacy Settings ----------------
  auto *page3 = new QWidget();
  auto *p3Layout = new QVBoxLayout(page3);
  p3Layout->setSpacing(16);
  p3Layout->setContentsMargins(0, 0, 0, 0);

  auto *grpPrivacy = new QGroupBox("ความเป็นส่วนตัวและการท่องเว็บ", page3);
  auto *grpPrivacyLayout = new QVBoxLayout(grpPrivacy);
  grpPrivacyLayout->setSpacing(12);

  auto *dntBox =
      new QCheckBox("ส่งคำขอ Do Not Track (DNT) ไปยังทุกเว็บไซต์", grpPrivacy);
  dntBox->setChecked(st.value("dntEnabled", true).toBool());

  auto *clearDataBtn =
      new QPushButton("ล้างข้อมูลการท่องเว็บ ประวัติ แคช และคุกกี้...", grpPrivacy);
  connect(clearDataBtn, &QPushButton::clicked, &dialog,
          [this] { clearBrowsingDataDialog(); });

  grpPrivacyLayout->addWidget(dntBox);
  grpPrivacyLayout->addWidget(clearDataBtn);
  p3Layout->addWidget(grpPrivacy);
  p3Layout->addStretch();
  stacked->addWidget(page3);

  // ---------------- Page 4: LiteWave Shield ----------------
  auto *shieldPage = new QWidget();
  auto *shieldLayout = new QVBoxLayout(shieldPage);
  shieldLayout->setSpacing(16);
  shieldLayout->setContentsMargins(0, 0, 0, 0);

  auto *shieldGroup = new QGroupBox("LiteWave Shield", shieldPage);
  auto *shieldGroupLayout = new QVBoxLayout(shieldGroup);
  shieldGroupLayout->setSpacing(10);

  const bool shieldEnabled = adBlocker_ && adBlocker_->isEnabled();
  const AdBlocker::Mode shieldMode =
      adBlocker_ ? adBlocker_->mode() : AdBlocker::Mode::Standard;
  auto *shieldEnabledBox = new QCheckBox(
      "เปิด Shield — บล็อกโฆษณาและคำขอติดตามที่รู้จัก", shieldGroup);
  shieldEnabledBox->setChecked(shieldEnabled);

  auto *standardShieldRadio = new QRadioButton(
      "มาตรฐาน (แนะนำ): บล็อกเฉพาะโฆษณา third-party ที่รู้จัก", shieldGroup);
  auto *aggressiveShieldRadio = new QRadioButton(
      "เข้มงวด: เพิ่มการบล็อก tracker — บางเว็บอาจต้องปิด Shield เฉพาะเว็บ",
      shieldGroup);
  standardShieldRadio->setChecked(shieldMode == AdBlocker::Mode::Standard);
  aggressiveShieldRadio->setChecked(shieldMode == AdBlocker::Mode::Aggressive);
  standardShieldRadio->setEnabled(shieldEnabled);
  aggressiveShieldRadio->setEnabled(shieldEnabled);
  connect(shieldEnabledBox, &QCheckBox::toggled, standardShieldRadio,
          &QWidget::setEnabled);
  connect(shieldEnabledBox, &QCheckBox::toggled, aggressiveShieldRadio,
          &QWidget::setEnabled);

  auto *shieldDescription = new QLabel(
      "Shield ไม่บล็อกการเปิดหน้าเว็บหลัก, CAPTCHA, หน้าเข้าสู่ระบบ หรือ "
      "คำขอจากเว็บที่คุณยกเว้นไว้ เพื่อให้วิดีโอและเว็บแอปมีโอกาสทำงานได้ปกติ",
      shieldGroup);
  shieldDescription->setWordWrap(true);
  shieldDescription->setStyleSheet(
      QString("color: %1; font-size: 12px;").arg(subTextColor));

  auto *ruleStatus = new QLabel(
      QString("ชุดกฎที่ฝังมากับ LiteWave: %1 กฎเครือข่าย")
          .arg(adBlocker_ ? adBlocker_->ruleCount() : 0),
      shieldGroup);
  ruleStatus->setStyleSheet(
      QString("color: %1; font-size: 12px; font-weight: 600;")
          .arg(isDark ? "#86efac" : "#15803d"));

  shieldGroupLayout->addWidget(shieldEnabledBox);
  shieldGroupLayout->addWidget(standardShieldRadio);
  shieldGroupLayout->addWidget(aggressiveShieldRadio);
  shieldGroupLayout->addWidget(shieldDescription);
  shieldGroupLayout->addWidget(ruleStatus);
  shieldLayout->addWidget(shieldGroup);

  auto *exceptionsGroup = new QGroupBox(
      "เว็บไซต์ที่ปิด Shield ไว้ (Allowlist)", shieldPage);
  auto *exceptionsLayout = new QVBoxLayout(exceptionsGroup);
  auto *shieldAllowedList = new QListWidget(exceptionsGroup);
  shieldAllowedList->setMinimumHeight(105);
  const QStringList existingAllowedSites =
      adBlocker_ ? adBlocker_->allowedSites() : QStringList{};
  for (const QString &site : existingAllowedSites) {
    auto *item = new QListWidgetItem(site, shieldAllowedList);
    item->setData(Qt::UserRole, site);
  }
  if (shieldAllowedList->count() == 0) {
    auto *emptyItem = new QListWidgetItem("ยังไม่มีเว็บไซต์ที่ยกเว้น", shieldAllowedList);
    emptyItem->setFlags(Qt::NoItemFlags);
    emptyItem->setForeground(QColor(subTextColor));
  }

  auto *removeAllowedSiteBtn =
      new QPushButton("นำเว็บไซต์ที่เลือกออกจากรายการ", exceptionsGroup);
  connect(removeAllowedSiteBtn, &QPushButton::clicked, &dialog,
          [shieldAllowedList] {
            auto *item = shieldAllowedList->currentItem();
            if (!item || !item->data(Qt::UserRole).isValid())
              return;
            delete shieldAllowedList->takeItem(shieldAllowedList->row(item));
          });

  auto *exceptionsHint = new QLabel(
      "ถ้าเว็บใดมีปัญหา ให้กด Shield บนแถบด้านบน แล้วเลือก “ปิด Shield สำหรับเว็บนี้”",
      exceptionsGroup);
  exceptionsHint->setWordWrap(true);
  exceptionsHint->setStyleSheet(
      QString("color: %1; font-size: 12px;").arg(subTextColor));
  exceptionsLayout->addWidget(shieldAllowedList);
  exceptionsLayout->addWidget(removeAllowedSiteBtn);
  exceptionsLayout->addWidget(exceptionsHint);
  shieldLayout->addWidget(exceptionsGroup);
  shieldLayout->addStretch();
  stacked->addWidget(shieldPage);

  // ---------------- Page 5: Privacy & Secure DNS ----------------
  auto *page4 = new QWidget();
  auto *p4Layout = new QVBoxLayout(page4);
  p4Layout->setSpacing(16);
  p4Layout->setContentsMargins(0, 0, 0, 0);

  auto *grpDns = new QGroupBox("Secure DNS (DNS-over-HTTPS)", page4);
  auto *grpDnsLayout = new QVBoxLayout(grpDns);
  grpDnsLayout->setSpacing(10);

  auto *secureDnsBox =
      new QCheckBox("เปิดใช้งาน Secure DNS (DNS-over-HTTPS)", grpDns);
  secureDnsBox->setChecked(st.value("secureDnsEnabled", false).toBool());

  auto *lblDnsDesc = new QLabel(
      "เมื่อเลือกผู้ให้บริการ LiteWave จะใช้ DNS-over-HTTPS ของผู้ให้บริการนั้นโดยตรง "
      "(Secure-only) ไม่ย้อนกลับไปใช้ DNS ของระบบ จึงเหมาะกับเครือข่ายที่ DNS "
      "ของระบบบล็อกเว็บไซต์บางแห่ง",
      grpDns);
  lblDnsDesc->setWordWrap(true);
  lblDnsDesc->setStyleSheet(
      QString("color: %1; font-size: 12px; margin-bottom: 6px;")
          .arg(subTextColor));

  auto *lblProvider = new QLabel("ผู้ให้บริการ DNS (Select DNS Provider):", grpDns);
  auto *dnsCombo = new QComboBox(grpDns);
  dnsCombo->addItem("OS Default (ตามการตั้งค่าของระบบ)", "OS Default");
  dnsCombo->addItem("Cloudflare (1.1.1.1 / 1.0.0.1)", "Cloudflare");
  dnsCombo->addItem("Google Public DNS (8.8.8.8 / 8.8.4.4)", "Google");
  dnsCombo->addItem("Quad9 (9.9.9.9)", "Quad9");
  dnsCombo->addItem("AdGuard DNS (บล็อกโฆษณาและความเป็นส่วนตัว)", "AdGuard");
  dnsCombo->addItem("กำหนด DoH Server URL เอง (Custom)", "Custom");

  const QString curProvider = st.value("dnsProvider", "OS Default").toString();
  int pIdx = dnsCombo->findData(curProvider);
  if (pIdx < 0)
    pIdx = 0;
  // Avoid a confusing "enabled" state that still means OS DNS. The first
  // opt-in starts with Cloudflare; users can still choose any provider.
  if (secureDnsBox->isChecked() &&
      dnsCombo->itemData(pIdx).toString() == "OS Default")
    pIdx = dnsCombo->findData("Cloudflare");
  dnsCombo->setCurrentIndex(pIdx);

  auto *customDnsEdit = new QLineEdit(grpDns);
  customDnsEdit->setPlaceholderText("https://example.com/dns-query");
  customDnsEdit->setText(st.value("customDnsUrl", "").toString());
  customDnsEdit->setVisible(curProvider == "Custom");

  connect(dnsCombo, &QComboBox::currentIndexChanged, &dialog,
          [dnsCombo, customDnsEdit] {
            customDnsEdit->setVisible(dnsCombo->currentData().toString() ==
                                      "Custom");
          });

  const QString dnsRuntimeStatus =
      st.value("secureDnsRuntimeStatus", "unknown").toString();
  const QString dnsRuntimeDetail =
      st.value("secureDnsRuntimeDetail").toString();
  const bool dnsActive = dnsRuntimeStatus == "secure-only";
  const bool dnsProblem =
      dnsRuntimeStatus == "error" || dnsRuntimeStatus == "unsupported";
  const QString dnsStatusText =
      dnsRuntimeStatus == "unknown"
          ? QStringLiteral("สถานะ: จะตรวจการตั้งค่าเมื่อเปิด LiteWave ครั้งถัดไป")
          : QStringLiteral("สถานะของการเปิดครั้งล่าสุด: %1").arg(dnsRuntimeDetail);
  auto *lblDnsStatus = new QLabel(dnsStatusText, grpDns);
  lblDnsStatus->setWordWrap(true);
  lblDnsStatus->setStyleSheet(
      QString("color: %1; font-size: 12px; font-weight: 600;")
          .arg(dnsActive ? "#16a34a" : (dnsProblem ? "#ef4444" : subTextColor)));

  auto *lblDnsNote = new QLabel(
      "การเปลี่ยน Secure DNS จะมีผลหลังปิด LiteWave ทุกหน้าต่างแล้วเปิดใหม่",
      grpDns);
  lblDnsNote->setStyleSheet(
      "color: #f59e0b; font-size: 11px; margin-top: 4px;");

  grpDnsLayout->addWidget(secureDnsBox);
  grpDnsLayout->addWidget(lblDnsDesc);
  grpDnsLayout->addWidget(lblDnsStatus);
  grpDnsLayout->addWidget(lblProvider);
  grpDnsLayout->addWidget(dnsCombo);
  grpDnsLayout->addWidget(customDnsEdit);
  grpDnsLayout->addWidget(lblDnsNote);
  p4Layout->addWidget(grpDns);
  p4Layout->addStretch();
  stacked->addWidget(page4);

  // ---------------- Page 6: Bookmarks ----------------
  auto *pageBm = new QWidget();
  auto *pBmLayout = new QVBoxLayout(pageBm);
  pBmLayout->setSpacing(14);
  pBmLayout->setContentsMargins(0, 0, 0, 0);

  auto *grpBm = new QGroupBox("จัดการบุ๊กมาร์ก (Bookmarks)", pageBm);
  auto *grpBmLayout = new QVBoxLayout(grpBm);
  grpBmLayout->setSpacing(10);

  auto *bmSearchEdit = new QLineEdit(grpBm);
  bmSearchEdit->setPlaceholderText("🔍 ค้นหาบุ๊กมาร์ก (ชื่อ หรือ URL)...");
  grpBmLayout->addWidget(bmSearchEdit);

  auto *bmListWidget = new QListWidget(grpBm);
  bmListWidget->setStyleSheet(QString(R"(
    QListWidget {
      background-color: %1;
      border: 1px solid %2;
      border-radius: 8px;
      padding: 6px;
    }
    QListWidget::item {
      padding: 8px 10px;
      border-bottom: 1px solid %2;
      border-radius: 6px;
      margin-bottom: 2px;
      color: %3;
    }
    QListWidget::item:hover {
      background-color: %4;
    }
    QListWidget::item:selected {
      background-color: rgba(14, 165, 233, 0.25);
      color: %3;
    }
  )").arg(dialogBg, borderColor, textColor, isDark ? "#374151" : "#f1f5f9"));

  auto populateBmList = [this, bmListWidget, bmSearchEdit] {
    bmListWidget->clear();
    const QString filter = bmSearchEdit->text().trimmed().toLower();
    bool found = false;
    for (int i = 0; i < bookmarks_.size(); ++i) {
      const auto &bm = bookmarks_[i];
      if (!filter.isEmpty() && !bm.title.toLower().contains(filter) && !bm.url.toLower().contains(filter)) {
        continue;
      }
      found = true;
      auto *item = new QListWidgetItem(QString("⭐ %1\n    %2").arg(bm.title, bm.url), bmListWidget);
      item->setData(Qt::UserRole, bm.url);
      item->setData(Qt::UserRole + 1, i);
    }
    if (!found) {
      auto *emptyItem = new QListWidgetItem(bookmarks_.isEmpty() ? "ยังไม่มีบุ๊กมาร์กที่บันทึกไว้" : "ไม่พบบุ๊กมาร์กที่ตรงกับคำค้น", bmListWidget);
      emptyItem->setFlags(Qt::NoItemFlags);
    }
  };

  populateBmList();
  connect(bmSearchEdit, &QLineEdit::textChanged, pageBm, populateBmList);
  grpBmLayout->addWidget(bmListWidget, 1);

  auto *bmBtnLayout = new QHBoxLayout();
  bmBtnLayout->setSpacing(8);

  auto *bmOpenBtn = new QPushButton("เปิด", grpBm);
  bmOpenBtn->setStyleSheet("background-color: #0284c7; color: white; font-weight: bold; border: none; padding: 6px 14px; border-radius: 6px;");
  auto *bmOpenTabBtn = new QPushButton("เปิดในแท็บใหม่", grpBm);
  auto *bmEditBtn = new QPushButton("แก้ไข...", grpBm);
  auto *bmDeleteBtn = new QPushButton("ลบ", grpBm);
  bmDeleteBtn->setStyleSheet("color: #ef4444; border-color: #ef4444;");
  auto *bmClearAllBtn = new QPushButton("ลบทั้งหมด", grpBm);
  bmClearAllBtn->setStyleSheet("color: #ef4444; border-color: #ef4444;");

  bmBtnLayout->addWidget(bmOpenBtn);
  bmBtnLayout->addWidget(bmOpenTabBtn);
  bmBtnLayout->addWidget(bmEditBtn);
  bmBtnLayout->addWidget(bmDeleteBtn);
  bmBtnLayout->addWidget(bmClearAllBtn);
  bmBtnLayout->addStretch();
  grpBmLayout->addLayout(bmBtnLayout);

  auto openCurrentBm = [this, &dialog, bmListWidget] {
    auto *item = bmListWidget->currentItem();
    if (item && item->data(Qt::UserRole).isValid()) {
      const QString url = item->data(Qt::UserRole).toString();
      if (!url.isEmpty()) {
        openUrl(url);
        dialog.accept();
      }
    }
  };

  connect(bmOpenBtn, &QPushButton::clicked, &dialog, openCurrentBm);
  connect(bmListWidget, &QListWidget::itemDoubleClicked, &dialog, [openCurrentBm](QListWidgetItem *) {
    openCurrentBm();
  });

  connect(bmOpenTabBtn, &QPushButton::clicked, &dialog, [this, bmListWidget] {
    auto *item = bmListWidget->currentItem();
    if (item && item->data(Qt::UserRole).isValid()) {
      const QString url = item->data(Qt::UserRole).toString();
      if (!url.isEmpty()) {
        createView(QUrl(url));
      }
    }
  });

  connect(bmEditBtn, &QPushButton::clicked, &dialog, [this, bmListWidget, populateBmList] {
    auto *item = bmListWidget->currentItem();
    if (!item || !item->data(Qt::UserRole).isValid())
      return;
    const int idx = item->data(Qt::UserRole + 1).toInt();
    if (idx < 0 || idx >= bookmarks_.size())
      return;

    bool ok = false;
    const QString newTitle = QInputDialog::getText(this, "แก้ไขบุ๊กมาร์ก", "ชื่อเว็บ:", QLineEdit::Normal, bookmarks_[idx].title, &ok);
    if (!ok) return;
    const QString newUrl = QInputDialog::getText(this, "แก้ไขบุ๊กมาร์ก", "URL:", QLineEdit::Normal, bookmarks_[idx].url, &ok);
    if (!ok || newUrl.trimmed().isEmpty()) return;

    bookmarks_[idx].title = newTitle.trimmed().isEmpty() ? newUrl.trimmed() : newTitle.trimmed();
    bookmarks_[idx].url = newUrl.trimmed();
    saveBookmarks();
    updateBookmarkStarState();
    populateBmList();
  });

  connect(bmDeleteBtn, &QPushButton::clicked, &dialog, [this, bmListWidget, populateBmList] {
    auto *item = bmListWidget->currentItem();
    if (item && item->data(Qt::UserRole).isValid()) {
      const int idx = item->data(Qt::UserRole + 1).toInt();
      if (idx >= 0 && idx < bookmarks_.size()) {
        bookmarks_.removeAt(idx);
        saveBookmarks();
        updateBookmarkStarState();
        populateBmList();
      }
    }
  });

  connect(bmClearAllBtn, &QPushButton::clicked, &dialog, [this, populateBmList] {
    if (bookmarks_.isEmpty())
      return;
    const auto ans = QMessageBox::question(this, "ลบทั้งหมด", "คุณแน่ใจหรือไม่ว่าต้องการลบบุ๊กมาร์กทั้งหมด?", QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (ans == QMessageBox::Yes) {
      bookmarks_.clear();
      saveBookmarks();
      updateBookmarkStarState();
      populateBmList();
    }
  });

  pBmLayout->addWidget(grpBm);
  stacked->addWidget(pageBm);

  // ---------------- Page 5: Downloads ----------------
  auto *page5 = new QWidget();
  auto *p5Layout = new QVBoxLayout(page5);
  p5Layout->setSpacing(16);
  p5Layout->setContentsMargins(0, 0, 0, 0);

  auto *grpDownloads = new QGroupBox("ตั้งค่าการดาวน์โหลด", page5);
  auto *grpDownloadsLayout = new QVBoxLayout(grpDownloads);
  grpDownloadsLayout->setSpacing(10);

  auto *lblDir = new QLabel("โฟลเดอร์สำหรับจัดเก็บไฟล์ดาวน์โหลด:", grpDownloads);
  auto *dirHLayout = new QHBoxLayout();
  auto *downloadPathEdit = new QLineEdit(grpDownloads);
  const QString defaultDl =
      QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
  downloadPathEdit->setText(
      st.value("downloadDirectory", defaultDl).toString());

  auto *browseBtn = new QPushButton("เรียกดู...", grpDownloads);
  connect(browseBtn, &QPushButton::clicked, &dialog, [this, downloadPathEdit] {
    const QString sel = QFileDialog::getExistingDirectory(
        this, "เลือกโฟลเดอร์ดาวน์โหลด", downloadPathEdit->text());
    if (!sel.isEmpty()) {
      downloadPathEdit->setText(sel);
    }
  });

  dirHLayout->addWidget(downloadPathEdit);
  dirHLayout->addWidget(browseBtn);

  auto *askDownloadBox =
      new QCheckBox("ถามสถานที่บันทึกทุกครั้งก่อนดาวน์โหลด", grpDownloads);
  askDownloadBox->setChecked(st.value("askDownloadLocation", false).toBool());

  grpDownloadsLayout->addWidget(lblDir);
  grpDownloadsLayout->addLayout(dirHLayout);
  grpDownloadsLayout->addWidget(askDownloadBox);
  p5Layout->addWidget(grpDownloads);
  p5Layout->addStretch();
  stacked->addWidget(page5);

  // ---------------- Page 6: Reset Settings ----------------
  auto *page6 = new QWidget();
  auto *p6Layout = new QVBoxLayout(page6);
  p6Layout->setSpacing(16);
  p6Layout->setContentsMargins(0, 0, 0, 0);

  auto *grpReset = new QGroupBox("รีเซ็ตการตั้งค่าเบราว์เซอร์", page6);
  auto *grpResetLayout = new QVBoxLayout(grpReset);
  grpResetLayout->setSpacing(12);

  auto *lblResetMsg =
      new QLabel("คืนค่าการตั้งค่าเบราว์เซอร์ LiteWave ทั้งหมดกลับเป็นค่าเริ่มต้น", grpReset);
  auto *resetAllBtn =
      new QPushButton("คืนค่าการตั้งค่าทั้งหมด (Reset All Settings)", grpReset);
  resetAllBtn->setStyleSheet(
      "background-color: #ef4444; color: white; font-weight: bold; border: "
      "none; padding: 8px 16px;");

  connect(resetAllBtn, &QPushButton::clicked, &dialog, [this, &dialog] {
    const auto res = QMessageBox::warning(
        &dialog, "ยืนยันการคืนค่า", "คุณแน่ใจหรือว่าต้องการคืนค่าการตั้งค่าทั้งหมดเป็นค่าเริ่มต้น?",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (res == QMessageBox::Yes) {
      QSettings stReset("LiteWave", "LiteWave");
      stReset.clear();
      if (darkMode_)
        toggleTheme();
      statusBar()->show();
      if (adBlocker_) {
        adBlocker_->setEnabled(true);
        adBlocker_->setMode(AdBlocker::Mode::Standard);
        adBlocker_->setAllowedSites({});
        adBlocker_->setDntEnabled(true);
      }
      SearchEngineManager::instance().setCurrentEngineId("google");
      for (int i = 0; i < tabStack_->count(); ++i) {
        if (auto *v = qobject_cast<QWebEngineView *>(tabStack_->widget(i))) {
          if (v->url().scheme() == "litewave" || v->url().host() == "litewave.home") {
            loadHome(v);
          }
        }
      }
      QMessageBox::information(&dialog, "สำเร็จ", "คืนค่าการตั้งค่าทั้งหมดเรียบร้อยแล้ว");
      dialog.accept();
    }
  });

  grpResetLayout->addWidget(lblResetMsg);
  grpResetLayout->addWidget(resetAllBtn);
  p6Layout->addWidget(grpReset);
  p6Layout->addStretch();
  stacked->addWidget(page6);

  connect(sidebar, &QListWidget::currentRowChanged, stacked,
          &QStackedWidget::setCurrentIndex);
  const int targetRow = (initialPage >= 0 && initialPage < sidebar->count()) ? initialPage : 0;
  sidebar->setCurrentRow(targetRow);

  contentLayout->addWidget(sidebar);
  contentLayout->addWidget(stacked, 1);
  mainLayout->addLayout(contentLayout);

  auto *btnLayout = new QHBoxLayout();
  btnLayout->addStretch();
  auto *saveBtn = new QPushButton("ตกลง", &dialog);
  saveBtn->setDefault(true);
  saveBtn->setStyleSheet(
      "background-color: #0284c7; color: white; font-weight: bold; border: "
      "none; padding: 7px 20px; border-radius: 6px;");
  auto *cancelBtn = new QPushButton("ยกเลิก", &dialog);

  btnLayout->addWidget(saveBtn);
  btnLayout->addWidget(cancelBtn);
  mainLayout->addLayout(btnLayout);

  connect(saveBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
  connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);

  if (dialog.exec() == QDialog::Accepted) {
    if (darkThemeBox->isChecked() != darkMode_) {
      toggleTheme();
    }
    statusBar()->setVisible(showStatusBarBox->isChecked());
    st.setValue("showStatusBar", showStatusBarBox->isChecked());

    const QString oldEngineId = SearchEngineManager::instance().currentEngine().id;
    const QString newEngineId = searchCombo->currentData().toString();
    const bool engineChanged = (oldEngineId != newEngineId);
    SearchEngineManager::instance().setCurrentEngineId(newEngineId);
    st.setValue("search/enableSuggestions", enableSuggestionsBox->isChecked());
    setupUrlBarCompleter();

    if (rbRestore->isChecked()) {
      st.setValue("startupOption", "restore");
    } else if (rbCustom->isChecked()) {
      st.setValue("startupOption", "custom");
      st.setValue("customStartupUrl", customUrlEdit->text().trimmed());
    } else {
      st.setValue("startupOption", "home");
    }

    st.setValue("dntEnabled", dntBox->isChecked());
    if (adBlocker_) {
      adBlocker_->setDntEnabled(dntBox->isChecked());
    }

    if (engineChanged) {
      for (int i = 0; i < tabStack_->count(); ++i) {
        if (auto *v = qobject_cast<QWebEngineView *>(tabStack_->widget(i))) {
          if (v->url().scheme() == "litewave" || v->url().host() == "litewave.home") {
            loadHome(v);
          }
        }
      }
    }

    if (adBlocker_) {
      const bool shieldWasEnabled = adBlocker_->isEnabled();
      const AdBlocker::Mode shieldWasMode = adBlocker_->mode();
      const QStringList previousAllowedSites = adBlocker_->allowedSites();
      QStringList selectedAllowedSites;
      for (int i = 0; i < shieldAllowedList->count(); ++i) {
        const auto *item = shieldAllowedList->item(i);
        if (item && item->data(Qt::UserRole).isValid())
          selectedAllowedSites.append(item->data(Qt::UserRole).toString());
      }
      selectedAllowedSites.sort(Qt::CaseInsensitive);

      adBlocker_->setEnabled(shieldEnabledBox->isChecked());
      adBlocker_->setMode(aggressiveShieldRadio->isChecked()
                              ? AdBlocker::Mode::Aggressive
                              : AdBlocker::Mode::Standard);
      adBlocker_->setAllowedSites(selectedAllowedSites);

      QStringList normalizedPrevious = previousAllowedSites;
      normalizedPrevious.sort(Qt::CaseInsensitive);
      const bool shieldChanged =
          shieldWasEnabled != adBlocker_->isEnabled() ||
          shieldWasMode != adBlocker_->mode() ||
          normalizedPrevious != selectedAllowedSites;
      if (shieldChanged) {
        refreshShieldUi();
        reloadCurrentView();
      }
    }

    const bool secureDnsChanged =
        st.value("secureDnsEnabled", false).toBool() != secureDnsBox->isChecked() ||
        st.value("dnsProvider", "OS Default").toString() !=
            dnsCombo->currentData().toString() ||
        st.value("customDnsUrl", "").toString() != customDnsEdit->text().trimmed();
    st.setValue("secureDnsEnabled", secureDnsBox->isChecked());
    st.setValue("dnsProvider", dnsCombo->currentData().toString());
    st.setValue("customDnsUrl", customDnsEdit->text().trimmed());

    st.setValue("downloadDirectory", downloadPathEdit->text().trimmed());
    st.setValue("askDownloadLocation", askDownloadBox->isChecked());

    if (secureDnsChanged) {
      QMessageBox::information(
          this, "ต้องเปิด LiteWave ใหม่",
          "บันทึก Secure DNS แล้ว\n\n"
          "เพื่อให้ LiteWave ใช้ DNS-over-HTTPS จริง ให้ปิด LiteWave ทุกหน้าต่าง "
          "แล้วเปิดโปรแกรมใหม่หนึ่งครั้ง");
    }
    statusBar()->showMessage("บันทึกการตั้งค่าเรียบร้อยแล้ว", 3000);
  }
}

void MainWindow::showTabContextMenu(const QPoint &pos) {
  const int index = tabBar_->tabAt(pos);
  if (index < 0)
    return;

  auto *view = qobject_cast<QWebEngineView *>(tabStack_->widget(index));
  if (!view)
    return;

  QMenu menu(this);
  auto *reloadAct = menu.addAction("↻ รีโหลดแท็บ");
  connect(reloadAct, &QAction::triggered, this, [view] { view->reload(); });

  auto *duplicateAct = menu.addAction("⎘ ทำซ้ำแท็บ");
  connect(duplicateAct, &QAction::triggered, this, [this, view] {
    createView(view->url());
  });

  const bool isMuted = view->page()->isAudioMuted();
  auto *muteAct = menu.addAction(isMuted ? "🔊 เปิดเสียงแท็บ" : "🔇 ปิดเสียงแท็บ");
  connect(muteAct, &QAction::triggered, this, [view, isMuted] {
    view->page()->setAudioMuted(!isMuted);
  });

  auto *inspectAct = menu.addAction("🛠️ ตรวจสอบองค์ประกอบ (DevTools)");
  connect(inspectAct, &QAction::triggered, this, [this, view] {
    openDevTools(view);
  });

  menu.addSeparator();

  auto *closeAct = menu.addAction("✕ ปิดแท็บ");
  connect(closeAct, &QAction::triggered, this, [this, index] {
    closeTab(index);
  });

  auto *closeOthersAct = menu.addAction("ปิดแท็บอื่นๆ ทั้งหมด");
  connect(closeOthersAct, &QAction::triggered, this, [this, view] {
    for (int i = tabStack_->count() - 1; i >= 0; --i) {
      if (tabStack_->widget(i) != view) {
        closeTab(i);
      }
    }
  });

  auto *closeRightAct = menu.addAction("ปิดแท็บทางด้านขวา");
  connect(closeRightAct, &QAction::triggered, this, [this, index] {
    for (int i = tabStack_->count() - 1; i > index; --i) {
      closeTab(i);
    }
  });

  if (!closedTabs_.isEmpty()) {
    menu.addSeparator();
    auto *reopenAct = menu.addAction("เปิดแท็บที่เพิ่งปิด (Ctrl+Shift+T)");
    connect(reopenAct, &QAction::triggered, this, [this] {
      const auto u = closedTabs_.takeLast();
      if (u.host() == "litewave.home" || u.isEmpty())
        newTab();
      else
        createView(u);
    });
  }

  menu.exec(tabBar_->mapToGlobal(pos));
}

void MainWindow::updateTabAudioIcon(int index, bool audible, bool muted) {
  if (index < 0 || index >= tabBar_->count())
    return;
  auto *view = qobject_cast<QWebEngineView *>(tabStack_->widget(index));
  if (!view)
    return;

  if (muted) {
    tabBar_->setTabToolTip(index, view->title() + " [ปิดเสียงอยู่]");
  } else if (audible) {
    tabBar_->setTabToolTip(index, view->title() + " [กำลังเล่นเสียง]");
  } else {
    tabBar_->setTabToolTip(index, view->title());
  }
}

void MainWindow::toggleDevTools(QWebEngineView *targetView) {
  if (!targetView)
    targetView = currentView();
  if (!targetView || !targetView->page() || !devToolsContainer_ || !devToolsView_)
    return;

  if (devToolsContainer_->isVisible()) {
    devToolsContainer_->hide();
    targetView->page()->setDevToolsPage(nullptr);
  } else {
    targetView->page()->setDevToolsPage(devToolsView_->page());
    devToolsContainer_->show();
    if (devToolsTitleLabel_) {
      devToolsTitleLabel_->setText(QStringLiteral("🛠️ DevTools — %1").arg(
          targetView->title().isEmpty() ? "LiteWave" : targetView->title()));
    }
    if (mainSplitter_) {
      const int totalHeight = mainSplitter_->height();
      const int devHeight = std::max(200, std::min(450, totalHeight * 38 / 100));
      mainSplitter_->setSizes({std::max(100, totalHeight - devHeight), devHeight});
    }
  }
}

void MainWindow::openDevTools(QWebEngineView *targetView) {
  toggleDevTools(targetView);
}

void MainWindow::openDevToolsUndocked(QWebEngineView *targetView) {
  if (!targetView)
    targetView = currentView();
  if (!targetView || !targetView->page())
    return;

  if (devToolsContainer_ && devToolsContainer_->isVisible()) {
    devToolsContainer_->hide();
    targetView->page()->setDevToolsPage(nullptr);
  }

  auto *devWindow = new QMainWindow(this);
  devWindow->setWindowTitle(QStringLiteral("LiteWave DevTools — %1").arg(
      targetView->title().isEmpty() ? "LiteWave" : targetView->title()));
  devWindow->resize(960, 640);
  devWindow->setAttribute(Qt::WA_DeleteOnClose);

  auto *devView = new QWebEngineView(devWindow);
  devWindow->setCentralWidget(devView);
  targetView->page()->setDevToolsPage(devView->page());

  connect(devWindow, &QObject::destroyed, targetView, [targetView] {
    if (targetView && targetView->page()) {
      targetView->page()->setDevToolsPage(nullptr);
    }
  });

  devWindow->show();
}

void MainWindow::saveSession() {
  if (privateMode_ || !tabStack_)
    return;

  QSettings st("LiteWave", "LiteWave");
  QStringList urls;
  urls.reserve(tabStack_->count());

  for (int i = 0; i < tabStack_->count(); ++i) {
    auto *view = qobject_cast<QWebEngineView *>(tabStack_->widget(i));
    if (!view)
      continue;

    const QUrl url = view->url();
    if (url.isEmpty() || url.toString() == "about:blank")
      continue;

    // Preserve LiteWave home tabs as an explicit internal URL so the restored
    // tab order matches what the user actually had open.
    if (url.scheme() == "litewave" || url.host() == "litewave.home") {
      urls.append(QStringLiteral("litewave://home"));
    } else if (url.scheme() == "http" || url.scheme() == "https" ||
               url.scheme() == "file") {
      urls.append(url.toString(QUrl::FullyEncoded));
    }
  }

  st.setValue("session/openTabs", urls);
  const int savedTabCount = static_cast<int>(urls.size());
  st.setValue("session/activeIndex",
              savedTabCount > 0
                  ? std::clamp(tabStack_->currentIndex(), 0, savedTabCount - 1)
                  : 0);
  st.setValue("session/savedAtMs", QDateTime::currentMSecsSinceEpoch());
  st.sync();
}

void MainWindow::restoreSession() {
  if (privateMode_)
    return;

  QSettings st("LiteWave", "LiteWave");
  const QString startup = st.value("startupOption", "home").toString();
  if (startup != "restore")
    return;

  const QStringList urls = st.value("session/openTabs").toStringList();
  if (urls.isEmpty())
    return;

  // Avoid pathological startup if a damaged settings file contains thousands
  // of entries. 50 restored tabs is still generous for a desktop browser.
  const int restoreCount =
      std::min(static_cast<int>(urls.size()), 50);
  for (int i = 0; i < restoreCount; ++i) {
    const QString raw = urls.at(i).trimmed();
    if (raw.isEmpty())
      continue;

    if (raw == QStringLiteral("litewave://home")) {
      newTab();
      continue;
    }

    const QUrl url(raw);
    if (url.isValid() &&
        (url.scheme() == "http" || url.scheme() == "https" ||
         url.scheme() == "file")) {
      createView(url);
    }
  }

  if (tabStack_->count() > 0) {
    const int requestedIndex = st.value("session/activeIndex", 0).toInt();
    const int activeIndex =
        std::clamp(requestedIndex, 0, tabStack_->count() - 1);
    tabBar_->setCurrentIndex(activeIndex);
    tabStack_->setCurrentIndex(activeIndex);
  }
}

void MainWindow::loadDownloadRecords() {
  QSettings st("LiteWave", "LiteWave");
  const QVariantList list = st.value("downloads/history").toList();
  downloadRecords_.clear();
  for (const auto &item : list) {
    const QVariantMap map = item.toMap();
    DownloadRecord rec;
    rec.fileName = map.value("fileName").toString();
    rec.path = map.value("path").toString();
    rec.totalBytes = map.value("totalBytes").toLongLong();
    rec.completed = map.value("completed", false).toBool();
    rec.cancelled = map.value("cancelled", false).toBool();
    rec.failed = map.value("failed", false).toBool();
    if (!rec.completed && !rec.cancelled && !rec.failed) {
      rec.cancelled = true;
    }
    if (!rec.fileName.isEmpty()) {
      downloadRecords_.append(rec);
    }
  }
}

void MainWindow::saveDownloadRecords() {
  if (privateMode_)
    return;
  QSettings st("LiteWave", "LiteWave");
  QVariantList list;
  const qsizetype maxItems = std::min<qsizetype>(downloadRecords_.size(), 50);
  for (qsizetype i = 0; i < maxItems; ++i) {
    const auto &rec = downloadRecords_.at(i);
    QVariantMap map;
    map["fileName"] = rec.fileName;
    map["path"] = rec.path;
    map["totalBytes"] = rec.totalBytes;
    map["completed"] = rec.completed;
    map["cancelled"] = rec.cancelled;
    map["failed"] = rec.failed;
    list.append(map);
  }
  st.setValue("downloads/history", list);
  st.sync();
}

void MainWindow::checkSleepingTabs() {
  const qint64 now = QDateTime::currentMSecsSinceEpoch();
  const int currentIdx = tabStack_->currentIndex();

  QSettings perfSettings("LiteWave", "LiteWave");
  const int freezeMinutes =
      std::clamp(perfSettings.value("performance/tabFreezeMinutes", 10).toInt(),
                 2, 120);
  const int discardMinutes =
      std::clamp(perfSettings.value("performance/tabDiscardMinutes", 30).toInt(),
                 freezeMinutes + 1, 240);
  for (int i = 0; i < tabStack_->count(); ++i) {
    if (i == currentIdx)
      continue;
    auto *view = qobject_cast<QWebEngineView *>(tabStack_->widget(i));
    if (!view || !view->page())
      continue;

    // Never sleep tabs currently playing media or audio (e.g. YouTube, Spotify, podcasts)
    if (view->page()->recentlyAudible())
      continue;

    // Never interrupt active page loads
    if (view->page()->isLoading())
      continue;

    const QUrl u = view->url();
    if (u.isEmpty() || u.toString() == "about:blank")
      continue;

    const qint64 lastActive = tabLastActiveTime_.value(view, now);
    const qint64 idleSeconds = (now - lastActive) / 1000;

    // Freeze first to stop background timers without throwing away the page.
    // Discard only after a longer idle period so forms/web-app state is less
    // likely to be interrupted during normal tab switching.
    if (idleSeconds >= static_cast<qint64>(discardMinutes) * 60) {
      if (view->page()->lifecycleState() != QWebEnginePage::LifecycleState::Discarded) {
        view->page()->setLifecycleState(QWebEnginePage::LifecycleState::Discarded);
        const QString cur = tabBar_->tabText(i);
        if (!cur.endsWith(" ·")) {
          tabBar_->setTabText(i, cur + " ·");
        }
        tabBar_->setTabToolTip(
            i, cur + " — แท็บพักการทำงานเพื่อประหยัด RAM; คลิกเพื่อเปิดต่อ");
      }
    } else if (idleSeconds >= static_cast<qint64>(freezeMinutes) * 60) {
      if (view->page()->lifecycleState() == QWebEnginePage::LifecycleState::Active) {
        view->page()->setLifecycleState(QWebEnginePage::LifecycleState::Frozen);
      }
    }
  }
}

void MainWindow::wakeTab(QWebEngineView *view) {
  if (!view || !view->page())
    return;
  if (view->page()->lifecycleState() != QWebEnginePage::LifecycleState::Active) {
    view->page()->setLifecycleState(QWebEnginePage::LifecycleState::Active);
    const int idx = tabStack_->indexOf(view);
    if (idx >= 0) {
      QString text = tabBar_->tabText(idx);
      if (text.endsWith(" ·")) {
        text.chop(2);
        tabBar_->setTabText(idx, text);
      }
      tabBar_->setTabToolTip(idx, text);
    }
  }
}


