#include "MainWindow.h"
#include "AdBlocker.h"

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QDesktopServices>
#include <QDialog>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QInputDialog>
#include <QRadioButton>
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
#include <QPushButton>
#include <QProgressBar>
#include <QRegularExpression>
#include <QSettings>
#include <QShortcut>
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QStatusBar>
#include <QTabBar>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QUrlQuery>
#include <QUuid>
#include <QVBoxLayout>
#include <QWindow>
#include <QWebEngineDownloadRequest>
#include <QWebEngineFullScreenRequest>
#include <QWebEngineNewWindowRequest>
#include <QWebEnginePage>
#include <QWebEnginePermission>
#include <QWebEngineProfile>
#include <QWebEngineScript>
#include <QWebEngineScriptCollection>
#include <QWebEngineSettings>
#include <QWebEngineView>
#include <algorithm>

static QIcon createToolbarIcon(const QString &name, const QColor &color) {
  QPixmap pix(20, 20);
  pix.fill(Qt::transparent);
  QPainter p(&pix);
  p.setRenderHint(QPainter::Antialiasing);
  p.setPen(QPen(color, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  p.setBrush(Qt::NoBrush);

  if (name == "lock_https") {
    p.drawRoundedRect(4, 9, 12, 9, 2, 2);
    p.drawArc(7, 4, 6, 8, 0, 180 * 16);
    p.setBrush(color);
    p.drawEllipse(9, 12, 2, 2);
  } else if (name == "lock_http") {
    QPainterPath path;
    path.moveTo(10, 3);
    path.lineTo(18, 17);
    path.lineTo(2, 17);
    path.closeSubpath();
    p.drawPath(path);
    p.drawLine(10, 8, 10, 12);
    p.drawPoint(10, 15);
  } else if (name == "moon") {
    QPainterPath path;
    path.moveTo(14, 3);
    path.cubicTo(8, 3, 4, 7, 4, 13);
    path.cubicTo(4, 17, 7, 19, 11, 19);
    path.cubicTo(8, 16, 8, 10, 14, 3);
    path.closeSubpath();
    p.drawPath(path);
  } else if (name == "sun") {
    p.drawEllipse(6, 6, 8, 8);
    p.drawLine(10, 2, 10, 4);
    p.drawLine(10, 16, 10, 18);
    p.drawLine(2, 10, 4, 10);
    p.drawLine(16, 10, 18, 10);
  } else if (name == "globe") {
    p.drawEllipse(3, 3, 14, 14);
    p.drawLine(3, 10, 17, 10);
    p.drawArc(6, 3, 8, 14, 0, 360 * 16);
  }

  return QIcon(pix);
}

class WebPage : public QWebEnginePage {
public:
  WebPage(QWebEngineProfile *profile, MainWindow *mw, QWebEngineView *view,
          QObject *parent = nullptr)
      : QWebEnginePage(profile, parent), mw_(mw), view_(view) {}

protected:
  bool acceptNavigationRequest(const QUrl &url, NavigationType type,
                               bool isMainFrame) override {
    if (isMainFrame && mw_ && view_)
      mw_->configurePageShield(view_, url);
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
      progress_(new QProgressBar(this)), toolbar_(nullptr),
      shieldAction_(nullptr), adBlocker_(nullptr) {
  setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
  setWindowTitle("LiteWave");
  setWindowIcon(QIcon(":/icons/litewave.svg"));
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
  profile_ = privateMode_ ? new QWebEngineProfile(this) : normalProfile;
  // Keep the engine's real version and platform for capability detection.
  profile_->setHttpAcceptLanguage("th-TH,th;q=0.9,en-US;q=0.8,en;q=0.7");
  if (!privateMode_) {
    profile_->setPersistentStoragePath(storagePath);
    profile_->setCachePath(cachePath);
    profile_->setPersistentCookiesPolicy(
        QWebEngineProfile::AllowPersistentCookies);
    profile_->setHttpCacheType(QWebEngineProfile::DiskHttpCache);
    profile_->setHttpCacheMaximumSize(64 * 1024 * 1024);
  }

  // Profile request interceptor setup
  adBlocker_ =
      static_cast<AdBlocker *>(profile_->findChild<QObject *>("shield"));
  if (!adBlocker_) {
    adBlocker_ = new AdBlocker(profile_, !privateMode_);
    adBlocker_->setObjectName("shield");
    profile_->setUrlRequestInterceptor(adBlocker_);
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
  tabBar_->setIconSize(QSize(16, 16));

  tabBarLayout->addWidget(tabBar_);

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

  // Window Control on far right of Tab Bar Row (Only Close button)
  auto *closeWinBtn = new QToolButton(this);
  closeWinBtn->setObjectName("windowCloseButton");
  closeWinBtn->setText("✕");
  closeWinBtn->setToolTip("ปิดโปรแกรม (Alt+F4)");
  closeWinBtn->setFixedSize(28, 28);
  closeWinBtn->setCursor(Qt::PointingHandCursor);
  connect(closeWinBtn, &QToolButton::clicked, this, &QMainWindow::close);

  tabBarLayout->addWidget(closeWinBtn);

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
  connect(back, &QAction::triggered, this, [this] {
    if (currentView())
      currentView()->back();
  });

  auto *forward = addNavAction("→", "ถัดไป (Alt+Right)");
  connect(forward, &QAction::triggered, this, [this] {
    if (currentView())
      currentView()->forward();
  });

  auto *reload = addNavAction("↻", "รีเฟรชหน้าเว็บ (Ctrl+R)");
  connect(reload, &QAction::triggered, this, [this] { reloadCurrentView(); });

  auto *home = addNavAction("⌂", "ไปยังหน้าแรก (Alt+Home)");
  connect(home, &QAction::triggered, this, [this] {
    if (currentView())
      loadHome(currentView());
  });

  // SSL Lock Icon
  sslLabel_ = new QLabel(this);
  sslLabel_->setObjectName("sslLabel");
  sslLabel_->setStyleSheet("padding: 0 4px;");
  toolbar_->addWidget(sslLabel_);

  // Omnibox Address Bar
  urlBar_->setPlaceholderText("ค้นหาด้วย Google หรือระบุ URL...");
  urlBar_->setClearButtonEnabled(true);
  urlBar_->setMinimumHeight(32);
  toolbar_->addWidget(urlBar_);
  connect(urlBar_, &QLineEdit::returnPressed, this, &MainWindow::navigate);

  auto *bookmark = addNavAction("★", "บันทึกหน้าเว็บนี้ (Ctrl+D)");
  connect(bookmark, &QAction::triggered, this, &MainWindow::addBookmark);

  // Shield Menu & Badge
  auto *shieldMenu = new QMenu(this);
  shieldAction_ = shieldMenu->addAction("เปิด Shield");
  shieldAction_->setCheckable(true);
  shieldAction_->setChecked(adBlocker_->isEnabled());
  connect(shieldAction_, &QAction::toggled, this, &MainWindow::toggleShield);

  siteShieldAction_ = shieldMenu->addAction("ป้องกันเว็บนี้");
  siteShieldAction_->setCheckable(true);
  connect(siteShieldAction_, &QAction::toggled, this, [this](bool enabled) {
    if (!currentView())
      return;
    adBlocker_->setSiteAllowed(currentView()->url(), !enabled);
    reloadCurrentView();
  });
  shieldMenu->addSeparator();
  filterInfoAction_ = shieldMenu->addAction(adBlocker_->filterStatus());
  filterInfoAction_->setEnabled(false);
  auto *updateFilters = shieldMenu->addAction("อัปเดตรายการบล็อก…");
  connect(updateFilters, &QAction::triggered, this, [this] {
    if (adBlocker_->isUpdating())
      return;
    const auto answer = QMessageBox::question(
        this, "อัปเดต Shield",
        "ดาวน์โหลดรายการ AdAway ผ่าน HTTPS จาก GitHub?\n"
        "GitHub จะเห็น IP ของการเชื่อมต่อ แต่ไม่มีการส่งประวัติหรือ URL ที่คุณเปิด\n"
        "รายการเดิมจะยังใช้งานได้หากอัปเดตไม่สำเร็จ",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
    if (answer == QMessageBox::Yes)
      adBlocker_->updateFilters();
  });
  connect(adBlocker_, &AdBlocker::filtersUpdated, this,
          [this](bool, const QString &message) {
            statusBar()->showMessage(message, 10000);
            updateShieldBadge(adBlocker_->blockedCount());
          });

  shieldBtn_ = new QToolButton(this);
  shieldBtn_->setObjectName("shieldButton");
  shieldBtn_->setPopupMode(QToolButton::InstantPopup);
  shieldBtn_->setMenu(shieldMenu);
  shieldBtn_->setCursor(Qt::PointingHandCursor);
  toolbar_->addWidget(shieldBtn_);
  connect(shieldMenu, &QMenu::aboutToShow, this,
          [this] { updateShieldBadge(adBlocker_->blockedCount()); });
  connect(adBlocker_, &AdBlocker::configurationChanged, this,
          &MainWindow::refreshShield);

  auto *badgeTimer = new QTimer(this);
  badgeTimer->setInterval(500);
  connect(badgeTimer, &QTimer::timeout, this,
          [this] { updateShieldBadge(adBlocker_->blockedCount()); });
  badgeTimer->start();

  // Theme Toggle Button
  themeAction_ = toolbar_->addAction("");
  themeAction_->setToolTip("สลับโหมดมืด/สว่าง");
  connect(themeAction_, &QAction::triggered, this, &MainWindow::toggleTheme);

  // Main Menu Button (Brave style main menu)
  menuBtn_ = new QToolButton(this);
  menuBtn_->setObjectName("mainMenuButton");
  menuBtn_->setText("☰");
  menuBtn_->setToolTip("เมนูหลัก (LiteWave)");
  menuBtn_->setPopupMode(QToolButton::InstantPopup);
  menuBtn_->setCursor(Qt::PointingHandCursor);
  menuBtn_->setMenu(createMainMenu());
  toolbar_->addWidget(menuBtn_);

  // 2px Slim Progress Line (Row 3 right below toolbar, 0 spacing)
  progress_->setMaximumHeight(2);
  progress_->setMinimumHeight(2);
  progress_->setTextVisible(false);
  progress_->setRange(0, 100);
  progress_->hide();

  headerLayout->addWidget(toolbar_);
  headerLayout->addWidget(progress_);

  // Assemble Main Layout
  mainLayout->addWidget(headerWidget_);
  mainLayout->addWidget(tabStack_);

  connect(tabBar_, &QTabBar::currentChanged, tabStack_,
          &QStackedWidget::setCurrentIndex);
  connect(tabBar_, &QTabBar::currentChanged, this, [this](int) {
    if (currentView()) {
      updateCurrentUrl(currentView()->url());
      setWindowTitle(currentView()->title() +
                     (privateMode_ ? " — Private · LiteWave" : " — LiteWave"));
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

  setupShortcuts();
  applyTheme();
  newTab();
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event) {
  if (watched == tabBarContainer_ || watched == headerWidget_ || watched == tabBar_) {
    if (event->type() == QEvent::MouseButtonPress) {
      auto *mouseEvent = static_cast<QMouseEvent *>(event);
      if (mouseEvent->button() == Qt::LeftButton) {
        if (windowHandle()) {
          windowHandle()->startSystemMove();
          return true;
        }
        dragPosition_ =
            mouseEvent->globalPosition().toPoint() - frameGeometry().topLeft();
        return false;
      }
    } else if (event->type() == QEvent::MouseMove) {
      auto *mouseEvent = static_cast<QMouseEvent *>(event);
      if (mouseEvent->buttons() & Qt::LeftButton) {
        if (isMaximized()) {
          showNormal();
        }
        move(mouseEvent->globalPosition().toPoint() - dragPosition_);
        return true;
      }
    } else if (event->type() == QEvent::MouseButtonDblClick) {
      auto *mouseEvent = static_cast<QMouseEvent *>(event);
      if (mouseEvent->button() == Qt::LeftButton) {
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

QWebEngineView *MainWindow::currentView() const {
  return qobject_cast<QWebEngineView *>(tabStack_->currentWidget());
}

QWebEngineView *MainWindow::createView(const QUrl &url) {
  auto *view = new QWebEngineView(tabStack_);
  auto *page = new WebPage(profile_, this, view, view);
  view->setPage(page);
  configurePageShield(view, url);
  auto *s = view->settings();
  s->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
  s->setAttribute(QWebEngineSettings::LocalStorageEnabled, true);
  s->setAttribute(QWebEngineSettings::PluginsEnabled, true);
  s->setAttribute(QWebEngineSettings::DnsPrefetchEnabled, true);
  s->setAttribute(QWebEngineSettings::WebGLEnabled, true);
  s->setAttribute(QWebEngineSettings::Accelerated2dCanvasEnabled, true);
  s->setAttribute(QWebEngineSettings::AutoLoadImages, true);
  s->setAttribute(QWebEngineSettings::JavascriptCanAccessClipboard, false);
  s->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows, true);
  s->setAttribute(QWebEngineSettings::AllowWindowActivationFromJavaScript,
                  false);
  s->setAttribute(QWebEngineSettings::ScreenCaptureEnabled, true);
  s->setAttribute(QWebEngineSettings::PdfViewerEnabled, true);
  s->setAttribute(QWebEngineSettings::AllowRunningInsecureContent, false);
  s->setAttribute(QWebEngineSettings::FullScreenSupportEnabled, true);
  s->setAttribute(QWebEngineSettings::PlaybackRequiresUserGesture, false);
  const int index = tabBar_->addTab("LiteWave");
  tabStack_->addWidget(view);
  tabBar_->setCurrentIndex(index);
  tabStack_->setCurrentIndex(index);

  auto *closeBtn = new QToolButton(tabBar_);
  closeBtn->setObjectName("tabCloseButton");
  closeBtn->setText("✕");
  closeBtn->setToolTip("ปิดแท็บ (Ctrl+W)");
  closeBtn->setFixedSize(16, 16);
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
            if (adBlocker_->shouldBlockPopup(request.requestedUrl(),
                                             page->url(),
                                             request.isUserInitiated())) {
              adBlocker_->recordBlocked();
              statusBar()->showMessage(
                  "Shield บล็อกป๊อปอัป — ปิดเฉพาะเว็บนี้ได้จากเมนู Shield", 3500);
              return;
            }
            auto *newView = createView(QUrl());
            request.openIn(newView->page());
          });

  // Compatible with Qt 6.4+; never grant sensitive permissions silently.
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
              capability = "clipboard access";
              break;
            case QWebEnginePermission::PermissionType::DesktopAudioVideoCapture:
              capability = "screen capture";
              break;
            default:
              permission.deny();
              return;
            }
            const auto answer = QMessageBox::question(
                this, "Site permission",
                permission.origin().toDisplayString() + "\nAllow access to " +
                    capability + "?",
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            if (answer == QMessageBox::Yes) {
              permission.grant();
            } else {
              permission.deny();
            }
          });

  connect(
      profile_, &QWebEngineProfile::downloadRequested, view,
      [this, page](QWebEngineDownloadRequest *download) {
        // A shared profile emits to every tab: only the originating page
        // handles it.
        if (!download || download->page() != page ||
            download->state() != QWebEngineDownloadRequest::DownloadRequested)
          return;
        if (download->isSavePageDownload()) {
          download->accept(); // Keep the filename selected by Ctrl+S.
          return;
        }
        QSettings st("LiteWave", "LiteWave");
        const QString defaultDir =
            QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
        QString dir = st.value("downloadDirectory", defaultDir).toString();
        if (dir.isEmpty() || !QDir(dir).exists())
          dir = defaultDir;
        const bool askLocation = st.value("askDownloadLocation", false).toBool();

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
        rec.completed = false;
        downloadRecords_.prepend(rec);

        connect(
            download, &QWebEngineDownloadRequest::stateChanged, this,
            [this, download, name](QWebEngineDownloadRequest::DownloadState state) {
              if (state == QWebEngineDownloadRequest::DownloadCompleted) {
                statusBar()->showMessage(
                    "ดาวน์โหลดเสร็จแล้ว: " + name, 5000);
                for (auto &r : downloadRecords_) {
                  if (r.fileName == name) {
                    r.completed = true;
                    break;
                  }
                }
              } else if (state ==
                             QWebEngineDownloadRequest::DownloadCancelled ||
                         state ==
                             QWebEngineDownloadRequest::DownloadInterrupted) {
                statusBar()->showMessage("ดาวน์โหลดไม่สำเร็จ", 5000);
              }
            });
        download->accept();
        statusBar()->showMessage("กำลังดาวน์โหลด: " + name, 2500);
      });

  connect(view, &QWebEngineView::urlChanged, this, [this, view](const QUrl &u) {
    configurePageShield(view, u, false);
    if (view == currentView())
      updateCurrentUrl(u);
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
    if (view == currentView()) {
      progress_->setValue(100);
      progress_->hide();
    }
  });

  configurePageShield(view, url, false);
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
  key("Ctrl+J", [this] { showDownloadsDialog(); });
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
  auto reload = [this] { reloadCurrentView(); };
  key("Ctrl+R", reload);
  key("F5", reload);
  auto hard = [this] { reloadCurrentView(); };
  key("Ctrl+Shift+R", hard);
  key("Shift+F5", hard);
  key("Ctrl+D", [this] { addBookmark(); });
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
    bool ok = false;
    const QString q = QInputDialog::getText(this, "Find in page", "Text",
                                            QLineEdit::Normal, findQuery_, &ok);
    if (ok && currentView()) {
      findQuery_ = q;
      currentView()->findText(q);
    }
  });
  key("Ctrl+G", [this] {
    if (currentView())
      currentView()->findText(findQuery_);
  });
  key("Ctrl+Shift+G", [this] {
    if (currentView())
      currentView()->findText(findQuery_, QWebEnginePage::FindBackward);
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
    } else if (engine == "brave") {
      view->setUrl(QUrl("https://search.brave.com/search?q=" +
                        QUrl::toPercentEncoding(text)));
    } else if (engine == "duckduckgo") {
      view->setUrl(
          QUrl("https://duckduckgo.com/?q=" + QUrl::toPercentEncoding(text)));
    } else {
      openUrl("g " + text);
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

void MainWindow::navigate() { openUrl(urlBar_->text()); }

void MainWindow::openUrl(const QString &text) {
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

  // Default search engine setting (Google / Brave / DuckDuckGo)
  QSettings settings("LiteWave", "LiteWave");
  const QString engine = settings.value("searchEngine", "Google").toString();
  QString searchBase = "https://www.google.com/search?q=";
  if (engine == "Brave") {
    searchBase = "https://search.brave.com/search?q=";
  } else if (engine == "DuckDuckGo") {
    searchBase = "https://duckduckgo.com/?q=";
  }

  url = QUrl(searchBase + QUrl::toPercentEncoding(input));
  currentView()->setUrl(url);
}

void MainWindow::configurePageShield(QWebEngineView *view, const QUrl &url,
                                     bool applyNow) {
  if (!view || !adBlocker_)
    return;
  auto &scripts = view->page()->scripts();
  const auto old = scripts.find("LiteWaveShield");
  for (const auto &script : old)
    scripts.remove(script);
  QWebEngineScript script;
  script.setName("LiteWaveShield");
  script.setSourceCode(
      AdBlocker::cosmeticScript(adBlocker_->isEnabledForUrl(url)));
  script.setInjectionPoint(QWebEngineScript::DocumentCreation);
  script.setWorldId(QWebEngineScript::ApplicationWorld);
  // The top-page owns cosmetic state; subframe requests are still
  // network-filtered.
  script.setRunsOnSubFrames(false);
  scripts.insert(script);
  if (applyNow)
    view->page()->runJavaScript(script.sourceCode(),
                                QWebEngineScript::ApplicationWorld);
}

void MainWindow::refreshShield() {
  for (int i = 0; i < tabStack_->count(); ++i) {
    auto *view = qobject_cast<QWebEngineView *>(tabStack_->widget(i));
    if (view)
      configurePageShield(view, view->url(), true);
  }
  updateShieldBadge(adBlocker_->blockedCount());
}

void MainWindow::updateShieldBadge(int count) {
  if (!shieldBtn_ || !adBlocker_)
    return;
  const auto url = currentView() ? currentView()->url() : QUrl();
  const bool enabled = adBlocker_->isEnabledForUrl(url);
  const QString label = enabled ? QString("Shield · %1").arg(count)
                        : adBlocker_->isEnabled() ? "Shield · ปิดเว็บนี้"
                                                  : "Shield · ปิด";
  if (shieldBtn_->text() != label)
    shieldBtn_->setText(label);
  shieldBtn_->setToolTip("จำนวนคำขอ/ป๊อปอัปที่บล็อกในโปรไฟล์นี้ (ไม่รวมการซ่อนด้วย CSS)");
  const QSignalBlocker globalGuard(shieldAction_);
  const QSignalBlocker siteGuard(siteShieldAction_);
  shieldAction_->setChecked(adBlocker_->isEnabled());
  siteShieldAction_->setChecked(!adBlocker_->isSiteAllowed(url));
  siteShieldAction_->setEnabled(
      adBlocker_->isEnabled() &&
      (url.scheme() == "http" || url.scheme() == "https") &&
      url.host() != "litewave.home");
  siteShieldAction_->setText("ป้องกันเว็บนี้: " + url.host());
  filterInfoAction_->setText(adBlocker_->isUpdating()
                                 ? "กำลังอัปเดตรายการ…"
                                 : adBlocker_->filterStatus());
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

  if (sslLabel_) {
    const QColor col = darkMode_ ? QColor("#9ca3af") : QColor("#475569");
    if (currentUrl.scheme() == "https") {
      sslLabel_->setPixmap(createToolbarIcon("lock_https", QColor("#10b981")).pixmap(16, 16));
      sslLabel_->setToolTip("การเชื่อมต่อปลอดภัย (HTTPS)");
    } else if (currentUrl.scheme() == "http") {
      sslLabel_->setPixmap(createToolbarIcon("lock_http", QColor("#ef4444")).pixmap(16, 16));
      sslLabel_->setToolTip("การเชื่อมต่อไม่ปลอดภัย (HTTP)");
    } else {
      sslLabel_->setPixmap(createToolbarIcon("globe", col).pixmap(16, 16));
      sslLabel_->setToolTip("LiteWave Dashboard");
    }
  }
}

void MainWindow::updateTabTitle(const QString &title) {
  auto *view = qobject_cast<QWebEngineView *>(sender());
  if (!view)
    return;
  const int index = tabStack_->indexOf(view);
  const QString displayTitle =
      title.trimmed().isEmpty() ? QStringLiteral("LiteWave") : title.left(22);
  if (index >= 0) {
    tabBar_->setTabText(index, displayTitle);
    tabBar_->setTabToolTip(index, title);
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
  }
}

void MainWindow::addBookmark() {
  if (!currentView() || currentView()->url().isEmpty())
    return;
  const QString title = currentView()->title().isEmpty()
                            ? currentView()->url().host()
                            : currentView()->title();
  QSettings settings("LiteWave", "LiteWave");
  settings.setValue("bookmarks/" + currentView()->url().toString(), title);
  statusBar()->showMessage("บันทึกเว็บแล้ว: " + title, 3000);
}

void MainWindow::toggleShield(bool enabled) {
  adBlocker_->setEnabled(
      enabled); // refreshShield immediately stops CSS/observers in every tab.
  statusBar()->showMessage(enabled ? "เปิด Shield แล้ว" : "ปิด Shield ทุกส่วนแล้ว",
                           3000);
  reloadCurrentView();
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
  if (themeAction_) {
    const QColor iconCol = darkMode_ ? QColor("#f1f5f9") : QColor("#475569");
    themeAction_->setIcon(createToolbarIcon(darkMode_ ? "sun" : "moon", iconCol));
    themeAction_->setToolTip(darkMode_ ? "เปลี่ยนเป็นโหมดสว่าง" : "เปลี่ยนเป็นโหมดมืด");
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
            QTabBar::tab {
                background-color: transparent;
                color: #9e9ea0;
                border: none;
                border-top-left-radius: 8px;
                border-top-right-radius: 8px;
                padding: 6px 8px 6px 12px;
                margin-right: 2px;
                font-size: 13px;
                min-width: 120px;
                max-width: 220px;
            }
            QTabBar::tab:hover {
                background-color: #282a31;
                color: #e8eaed;
            }
            QTabBar::tab:selected {
                background-color: #2b2d35;
                color: #ffffff;
                font-weight: bold;
            }
            QToolButton#tabCloseButton {
                background: transparent;
                color: #94a3b8;
                border: none;
                border-radius: 8px;
                font-size: 11px;
                font-weight: bold;
                padding: 0px;
                margin-left: 4px;
                margin-right: 2px;
            }
            QToolButton#tabCloseButton:hover {
                background-color: #ef4444;
                color: #ffffff;
            }
            QToolButton#newTabButton {
                background-color: transparent;
                color: #9e9ea0;
                border: none;
                border-radius: 14px;
                font-size: 16px;
                font-weight: bold;
                margin-bottom: 2px;
            }
            QToolButton#newTabButton:hover {
                background-color: #2b2d35;
                color: #ffffff;
            }
            QToolButton#windowMinButton, QToolButton#windowMaxButton {
                background-color: transparent;
                color: #94a3b8;
                border: none;
                border-radius: 14px;
                font-size: 13px;
                font-weight: bold;
            }
            QToolButton#windowMinButton:hover, QToolButton#windowMaxButton:hover {
                background-color: #3b3e4a;
                color: #ffffff;
            }
            QToolButton#windowCloseButton {
                background-color: transparent;
                color: #94a3b8;
                border: none;
                border-radius: 14px;
                font-size: 13px;
                font-weight: bold;
            }
            QToolButton#windowCloseButton:hover {
                background-color: #ef4444;
                color: #ffffff;
            }
            QToolBar#mainToolbar {
                background-color: #2b2d35;
                border-top: none;
                border-bottom: 1px solid #18191d;
                padding: 4px 8px;
                spacing: 4px;
            }
            QToolBar#mainToolbar QToolButton {
                background-color: transparent;
                color: #d1d5db;
                border: none;
                border-radius: 14px;
                padding: 5px 8px;
                font-size: 14px;
                min-width: 24px;
                min-height: 24px;
            }
            QToolBar#mainToolbar QToolButton:hover {
                background-color: #3b3e4a;
                color: #ffffff;
            }
            QToolBar#mainToolbar QToolButton:pressed {
                background-color: #484b59;
            }
            QLineEdit {
                background-color: #1e2026;
                color: #f1f3f4;
                border: 1px solid #383a42;
                border-radius: 17px;
                padding: 5px 30px 5px 14px;
                font-size: 13px;
                selection-background-color: #2563eb;
            }
            QLineEdit:focus {
                border: 1px solid #ff5500;
                background-color: #16171c;
            }
            QToolButton#shieldButton {
                background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #ff5500, stop:1 #ff2a00);
                color: #ffffff;
                border: none;
                border-radius: 14px;
                padding: 4px 12px;
                font-weight: bold;
                font-size: 12px;
            }
            QToolButton#shieldButton:hover {
                background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #ff6611, stop:1 #ff3311);
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
                background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #ff5500, stop:1 #00f2fe);
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
            }
            QMenu::item {
                padding: 6px 20px;
                border-radius: 4px;
            }
            QMenu::item:selected {
                background-color: #ff5500;
                color: #ffffff;
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
            QTabBar::tab {
                background-color: transparent;
                color: #5f6368;
                border: none;
                border-top-left-radius: 8px;
                border-top-right-radius: 8px;
                padding: 6px 8px 6px 12px;
                margin-right: 2px;
                font-size: 13px;
                min-width: 120px;
                max-width: 220px;
            }
            QTabBar::tab:hover {
                background-color: #d8dadf;
                color: #202124;
            }
            QTabBar::tab:selected {
                background-color: #ffffff;
                color: #1e1e1e;
                font-weight: bold;
            }
            QToolButton#tabCloseButton {
                background: transparent;
                color: #64748b;
                border: none;
                border-radius: 8px;
                font-size: 11px;
                font-weight: bold;
                padding: 0px;
                margin-left: 4px;
                margin-right: 2px;
            }
            QToolButton#tabCloseButton:hover {
                background-color: #ef4444;
                color: #ffffff;
            }
            QToolButton#newTabButton {
                background-color: transparent;
                color: #5f6368;
                border: none;
                border-radius: 14px;
                font-size: 16px;
                font-weight: bold;
                margin-bottom: 2px;
            }
            QToolButton#newTabButton:hover {
                background-color: #ffffff;
                color: #1e1e1e;
            }
            QToolButton#windowMinButton, QToolButton#windowMaxButton {
                background-color: transparent;
                color: #5f6368;
                border: none;
                border-radius: 14px;
                font-size: 13px;
                font-weight: bold;
            }
            QToolButton#windowMinButton:hover, QToolButton#windowMaxButton:hover {
                background-color: #d8dadf;
                color: #1e1e1e;
            }
            QToolButton#windowCloseButton {
                background-color: transparent;
                color: #5f6368;
                border: none;
                border-radius: 14px;
                font-size: 13px;
                font-weight: bold;
            }
            QToolButton#windowCloseButton:hover {
                background-color: #ef4444;
                color: #ffffff;
            }
            QToolBar#mainToolbar {
                background-color: #ffffff;
                border-top: none;
                border-bottom: 1px solid #d0d2d6;
                padding: 4px 8px;
                spacing: 4px;
            }
            QToolBar#mainToolbar QToolButton {
                background-color: transparent;
                color: #4a4d52;
                border: none;
                border-radius: 14px;
                padding: 5px 8px;
                font-size: 14px;
                min-width: 24px;
                min-height: 24px;
            }
            QToolBar#mainToolbar QToolButton:hover {
                background-color: #f1f3f4;
                color: #1e1e1e;
            }
            QToolBar#mainToolbar QToolButton:pressed {
                background-color: #e8eaed;
            }
            QLineEdit {
                background-color: #f1f3f4;
                color: #202124;
                border: 1px solid #e0e2e5;
                border-radius: 17px;
                padding: 5px 30px 5px 14px;
                font-size: 13px;
                selection-background-color: #2563eb;
            }
            QLineEdit:focus {
                border: 1px solid #ff5500;
                background-color: #ffffff;
            }
            QToolButton#shieldButton {
                background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #ff5500, stop:1 #ff2a00);
                color: #ffffff;
                border: none;
                border-radius: 14px;
                padding: 4px 12px;
                font-weight: bold;
                font-size: 12px;
            }
            QToolButton#shieldButton:hover {
                background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #ff6611, stop:1 #ff3311);
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
                background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #ff5500, stop:1 #0072ff);
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
            }
            QMenu::item {
                padding: 6px 20px;
                border-radius: 4px;
            }
            QMenu::item:selected {
                background-color: #ff5500;
                color: #ffffff;
            }
        )QSS"));
  }
}

void MainWindow::loadHome(QWebEngineView *view) {
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
* { box-sizing: border-box; margin: 0; padding: 0; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, "Helvetica Neue", Arial, sans-serif; }
body {
  background-color: %1;
  color: %3;
  min-height: 100vh;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: flex-start;
  padding: 80px 20px 40px;
}
.brand {
  font-size: 58px;
  font-weight: 800;
  letter-spacing: -1.5px;
  margin-bottom: 8px;
  user-select: none;
  color: #3b82f6;
  background: linear-gradient(135deg, #2563eb 0%, #60a5fa 60%, #93c5fd 100%);
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
  display: inline-block;
}
.subtitle {
  font-size: 14px;
  color: %4;
  margin-bottom: 32px;
  font-weight: 400;
}
.search-container {
  width: 100%;
  max-width: 740px;
  margin-bottom: 44px;
}
.search-box {
  display: flex;
  align-items: center;
  background-color: %2;
  border: 1px solid %5;
  border-radius: 28px;
  padding: 4px 8px 4px 18px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.08);
  transition: box-shadow 0.2s ease, border-color 0.2s ease;
}
.search-box:focus-within, .search-box:hover {
  box-shadow: 0 4px 18px rgba(32, 33, 36, 0.16);
  border-color: #2563eb;
}
.search-icon {
  display: flex;
  align-items: center;
  justify-content: center;
  color: %4;
  margin-right: 8px;
}
.search-box select {
  background-color: transparent;
  color: %3;
  border: none;
  font-size: 14px;
  font-weight: 500;
  outline: none;
  cursor: pointer;
  padding: 0 8px 0 0;
  border-right: 1px solid %5;
}
.search-box select option {
  background-color: %2;
  color: %3;
}
.search-box input {
  flex: 1;
  border: none;
  background: transparent;
  padding: 12px 14px;
  font-size: 16px;
  color: %3;
  outline: none;
}
.search-box button {
  background-color: #2563eb;
  color: #ffffff;
  border: none;
  border-radius: 20px;
  padding: 10px 24px;
  font-size: 14px;
  font-weight: 600;
  cursor: pointer;
  transition: background-color 0.15s ease, transform 0.1s ease;
}
.search-box button:hover {
  background-color: #1d4ed8;
}
.search-box button:active {
  transform: scale(0.98);
}
.sites-section {
  width: 100%;
  max-width: 740px;
}
.section-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  margin-bottom: 16px;
}
.section-title {
  font-size: 13px;
  font-weight: 600;
  letter-spacing: 0.5px;
  text-transform: uppercase;
  color: %4;
}
.sites-grid {
  display: grid;
  grid-template-columns: repeat(4, 1fr);
  gap: 16px;
}
.site-card {
  position: relative;
  background-color: %2;
  border: 1px solid %5;
  border-radius: 14px;
  padding: 18px 12px;
  text-align: center;
  cursor: pointer;
  text-decoration: none;
  color: %3;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: 10px;
  transition: border-color 0.15s ease, transform 0.15s ease, box-shadow 0.15s ease;
}
.site-card:hover {
  border-color: #2563eb;
  transform: translateY(-2px);
  box-shadow: 0 6px 16px rgba(0,0,0,0.08);
}
.options-btn {
  position: absolute;
  top: 8px;
  right: 8px;
  background-color: %2;
  color: %3;
  border: 1px solid %5;
  width: 26px;
  height: 26px;
  border-radius: 50%;
  cursor: pointer;
  display: flex;
  align-items: center;
  justify-content: center;
  opacity: 0.9;
  box-shadow: 0 2px 6px rgba(0,0,0,0.12);
  transition: all 0.15s ease;
  z-index: 10;
}
.site-card:hover .options-btn, .options-btn:hover {
  opacity: 1;
  background-color: #2563eb;
  color: #ffffff;
  border-color: #2563eb;
}
.card-dropdown {
  position: absolute;
  top: 38px;
  right: 8px;
  background-color: %2;
  color: %3;
  border: 1px solid %5;
  border-radius: 10px;
  padding: 4px;
  box-shadow: 0 8px 24px rgba(0,0,0,0.22);
  display: none;
  flex-direction: column;
  min-width: 130px;
  z-index: 100;
}
.card-dropdown.active {
  display: flex;
}
.dropdown-item {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 8px 12px;
  font-size: 13px;
  font-weight: 500;
  border-radius: 6px;
  color: %3;
  cursor: pointer;
  transition: background 0.12s ease;
}
.dropdown-item:hover {
  background-color: #2563eb;
  color: #ffffff;
}
.dropdown-item.danger-item:hover {
  background-color: #ef4444;
  color: #ffffff;
}
.site-icon-box {
  width: 44px;
  height: 44px;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 12px;
  background: rgba(255,255,255,0.9);
  border: 1px solid %5;
  overflow: hidden;
}
.site-favicon {
  width: 28px;
  height: 28px;
  object-fit: contain;
}
.site-icon-fallback {
  width: 36px;
  height: 36px;
  border-radius: 50%;
  background: #2563eb;
  color: #ffffff;
  font-weight: bold;
  font-size: 16px;
  display: flex;
  align-items: center;
  justify-content: center;
}
.site-name {
  font-size: 13px;
  font-weight: 500;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
  max-width: 110px;
}
.add-card {
  border: 2px dashed %5;
  background-color: transparent;
}
.add-card:hover {
  border-color: #2563eb;
  background-color: %2;
}
.add-icon-box {
  font-size: 22px;
  font-weight: bold;
  color: #2563eb;
  background: transparent;
  border: none;
}
.modal-overlay {
  position: fixed;
  top: 0; left: 0; right: 0; bottom: 0;
  background: rgba(0,0,0,0.45);
  display: none;
  align-items: center;
  justify-content: center;
  z-index: 1000;
}
.modal-overlay.active {
  display: flex;
}
.modal {
  background-color: %2;
  color: %3;
  border: 1px solid %5;
  border-radius: 16px;
  padding: 24px;
  width: 90%;
  max-width: 420px;
  box-shadow: 0 12px 36px rgba(0,0,0,0.25);
}
.modal h3 {
  font-size: 18px;
  font-weight: 600;
  margin-bottom: 16px;
}
.modal input {
  width: 100%;
  padding: 12px 16px;
  margin-bottom: 12px;
  border: 1px solid %5;
  border-radius: 8px;
  background: %1;
  color: %3;
  font-size: 14px;
  outline: none;
}
.modal input:focus {
  border-color: #2563eb;
}
.modal-buttons {
  display: flex;
  justify-content: flex-end;
  gap: 10px;
  margin-top: 8px;
}
.modal-buttons button {
  padding: 10px 20px;
  border-radius: 8px;
  font-size: 14px;
  font-weight: 600;
  cursor: pointer;
  border: none;
}
.btn-cancel {
  background: transparent;
  color: %4;
}
.btn-save {
  background: #2563eb;
  color: #fff;
}
.btn-save:hover {
  background: #1d4ed8;
}
.footer-note {
  margin-top: auto;
  padding-top: 48px;
  font-size: 13px;
  color: %4;
}
</style>
</head>
<body>

<div class="brand">LiteWave</div>
<div class="subtitle">ค้นหาและท่องเว็บ · ควบคุมโฆษณาและตัวติดตามด้วย Shield</div>

<div class="search-container">
  <form class="search-box" onsubmit="return submitSearch(event)">
    <div class="search-icon">
      <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round"><circle cx="11" cy="11" r="8"></circle><line x1="21" y1="21" x2="16.65" y2="16.65"></line></svg>
    </div>
    <select id="engine" name="engine" aria-label="เครื่องมือค้นหา">
      <option value="brave" selected>Brave Search</option>
      <option value="google">Google Search</option>
      <option value="duckduckgo">DuckDuckGo</option>
    </select>
    <input id="q" name="q" type="search" autofocus placeholder="ค้นหาบน Google หรือป้อนที่อยู่เว็บไซต์..." autocomplete="off">
    <button type="submit">ค้นหา</button>
  </form>
</div>

<div class="sites-section">
  <div class="section-header">
    <div class="section-title">ทางลัดเว็บไซต์</div>
  </div>
  <div class="sites-grid" id="sitesGrid"></div>
</div>

<div class="modal-overlay" id="addModal">
  <div class="modal">
    <h3 id="modalTitle">เพิ่มทางลัดเว็บไซต์</h3>
    <input id="shortcutName" placeholder="ชื่อเว็บไซต์ (เช่น Instagram)" autocomplete="off">
    <input id="shortcutUrl" placeholder="ที่อยู่เว็บ (เช่น https://instagram.com)" autocomplete="off">
    <div class="modal-buttons">
      <button type="button" class="btn-cancel" onclick="closeModal()">ยกเลิก</button>
      <button type="button" class="btn-save" onclick="saveShortcut()">บันทึก</button>
    </div>
  </div>
</div>

<div class="footer-note">Shield · บล็อกคำขอแล้ว %6 รายการ</div>

<script>
function submitSearch(event) {
  event.preventDefault();
  const query = document.getElementById('q').value.trim();
  if (!query) return false;
  const engine = document.getElementById('engine').value;
  window.location.href = 'litewave://search?engine=' +
    encodeURIComponent(engine) + '&q=' + encodeURIComponent(query);
  return false;
}

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
    const u = new URL(urlStr.startsWith('http') ? urlStr : 'https://' + urlStr);
    return u.hostname;
  } catch(e) {
    return urlStr;
  }
}

function getFaviconUrl(urlStr) {
  const domain = getDomain(urlStr);
  return 'https://www.google.com/s2/favicons?domain=' + encodeURIComponent(domain) + '&sz=128';
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
    card.onclick = () => window.location.href = s.url;

    const domain = getDomain(s.url);
    const favicon = getFaviconUrl(s.url);
    const initial = (s.name || 'W').charAt(0).toUpperCase();

    card.innerHTML = `
      <button class="options-btn" title="ตัวเลือก" onclick="toggleCardDropdown(event, ${idx})">
        <svg width="14" height="14" viewBox="0 0 24 24" fill="currentColor"><circle cx="12" cy="5" r="2.2"></circle><circle cx="12" cy="12" r="2.2"></circle><circle cx="12" cy="19" r="2.2"></circle></svg>
      </button>
      <div class="card-dropdown" id="dropdown-${idx}">
        <div class="dropdown-item" onclick="openEditModal(event, ${idx})">
          <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M12 20h9"></path><path d="M16.5 3.5a2.121 2.121 0 0 1 3 3L7 19l-4 1 1-4L16.5 3.5z"></path></svg>
          <span>แก้ไขทางลัด</span>
        </div>
        <div class="dropdown-item danger-item" onclick="deleteShortcut(event, ${idx})">
          <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><line x1="18" y1="6" x2="6" y2="18"></line><line x1="6" y1="6" x2="18" y2="18"></line></svg>
          <span>ลบทางลัด</span>
        </div>
      </div>
      <div class="site-icon-box">
        <img src="${favicon}" class="site-favicon" onerror="this.onerror=null; this.src='https://icon.horse/icon/${domain}'; this.onerror=function(){this.style.display='none'; this.nextElementSibling.style.display='flex';};" alt="${s.name}" />
        <div class="site-icon-fallback" style="display:none;">${initial}</div>
      </div>
      <div class="site-name">${s.name}</div>
    `;
    grid.appendChild(card);
  });

  const addBtn = document.createElement('div');
  addBtn.className = 'site-card add-card';
  addBtn.onclick = openAddModal;
  addBtn.innerHTML = `
    <div class="site-icon-box add-icon-box">+</div>
    <div class="site-name">เพิ่มทางลัด</div>
  `;
  grid.appendChild(addBtn);
}

function toggleCardDropdown(e, idx) {
  e.stopPropagation();
  const allDropdowns = document.querySelectorAll('.card-dropdown');
  allDropdowns.forEach((d, i) => {
    if (i !== idx) d.classList.remove('active');
  });
  const menu = document.getElementById('dropdown-' + idx);
  if (menu) menu.classList.toggle('active');
}

document.addEventListener('click', () => {
  const allDropdowns = document.querySelectorAll('.card-dropdown');
  allDropdowns.forEach(d => d.classList.remove('active'));
});

function openAddModal() {
  editIndex = -1;
  document.getElementById('modalTitle').innerText = 'เพิ่มทางลัดเว็บไซต์';
  document.getElementById('shortcutName').value = '';
  document.getElementById('shortcutUrl').value = '';
  document.getElementById('addModal').classList.add('active');
  document.getElementById('shortcutName').focus();
}

function openEditModal(e, idx) {
  e.stopPropagation();
  editIndex = idx;
  const shortcuts = getShortcuts();
  const item = shortcuts[idx];
  if (!item) return;
  document.getElementById('modalTitle').innerText = 'แก้ไขทางลัดเว็บไซต์';
  document.getElementById('shortcutName').value = item.name;
  document.getElementById('shortcutUrl').value = item.url;
  document.getElementById('addModal').classList.add('active');
  document.getElementById('shortcutName').focus();
}

function closeModal() {
  document.getElementById('addModal').classList.remove('active');
}

function saveShortcut() {
  const name = document.getElementById('shortcutName').value.trim();
  let url = document.getElementById('shortcutUrl').value.trim();
  if (!name || !url) return;
  if (!url.startsWith('http://') && !url.startsWith('https://')) {
    url = 'https://' + url;
  }

  const shortcuts = getShortcuts();
  if (editIndex >= 0 && editIndex < shortcuts.length) {
    shortcuts[editIndex] = { name, url };
  } else {
    shortcuts.push({ name, url });
  }
  saveShortcuts(shortcuts);
  closeModal();
  renderShortcuts();
}

function deleteShortcut(e, idx) {
  e.stopPropagation();
  const shortcuts = getShortcuts();
  shortcuts.splice(idx, 1);
  saveShortcuts(shortcuts);
  renderShortcuts();
}

renderShortcuts();
</script>
</body>
</html>
)HTML")
                           .arg(bg, cardBg, textCol, subCol, borderCol)
                           .arg(blockedCount);

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
}

void MainWindow::populateHistoryMenu(QMenu *historyMenu) {
  historyMenu->clear();
  auto *clearHist = historyMenu->addAction("🗑️ ล้างประวัติการใช้เว็บ");
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
      if (++count > 15)
        break;
      const QVariantMap map = var.toMap();
      const QString title = map.value("title").toString();
      const QString urlStr = map.value("url").toString();
      auto *act = historyMenu->addAction(title + " — " + urlStr);
      connect(act, &QAction::triggered, this, [this, urlStr] {
        openUrl(urlStr);
      });
    }
  }
}

void MainWindow::populateBookmarksMenu(QMenu *bookmarksMenu) {
  bookmarksMenu->clear();
  auto *addBm = bookmarksMenu->addAction("★ บันทึกแท็บนี้ (Ctrl+D)");
  connect(addBm, &QAction::triggered, this, &MainWindow::addBookmark);

  auto *clearBm = bookmarksMenu->addAction("🗑️ ลบบุ๊กมาร์กทั้งหมด");
  connect(clearBm, &QAction::triggered, this, [this] {
    QSettings st("LiteWave", "LiteWave");
    st.remove("bookmarks");
    statusBar()->showMessage("ลบบุ๊กมาร์กทั้งหมดเรียบร้อยแล้ว", 3000);
  });
  bookmarksMenu->addSeparator();

  QSettings st("LiteWave", "LiteWave");
  st.beginGroup("bookmarks");
  const QStringList keys = st.allKeys();
  if (keys.isEmpty()) {
    auto *emptyItem = bookmarksMenu->addAction("ไม่มีบุ๊กมาร์กที่บันทึกไว้");
    emptyItem->setEnabled(false);
  } else {
    for (const QString &key : keys) {
      const QString title = st.value(key).toString();
      auto *act = bookmarksMenu->addAction(title + " — " + key);
      connect(act, &QAction::triggered, this, [this, key] {
        openUrl(key);
      });
    }
  }
  st.endGroup();
}

void MainWindow::showDownloadsDialog() {
  auto *dialog = new QDialog(this);
  dialog->setWindowTitle("รายการดาวน์โหลด (Downloads)");
  dialog->resize(520, 360);

  auto *layout = new QVBoxLayout(dialog);
  auto *listWidget = new QListWidget(dialog);

  if (downloadRecords_.isEmpty()) {
    listWidget->addItem("ยังไม่มีประวัติการดาวน์โหลด");
  } else {
    for (const auto &rec : downloadRecords_) {
      QString text = rec.fileName + (rec.completed ? " — [เสร็จสิ้น]" : " — [กำลังดาวน์โหลด/ยกเลิก]");
      auto *item = new QListWidgetItem(text, listWidget);
      item->setData(Qt::UserRole, rec.path);
    }
  }

  layout->addWidget(listWidget);

  auto *btnLayout = new QHBoxLayout();
  auto *openFileBtn = new QPushButton("เปิดไฟล์", dialog);
  auto *openDirBtn = new QPushButton("เปิดโฟลเดอร์ดาวน์โหลด", dialog);
  auto *closeBtn = new QPushButton("ปิด", dialog);

  btnLayout->addWidget(openFileBtn);
  btnLayout->addWidget(openDirBtn);
  btnLayout->addWidget(closeBtn);
  layout->addLayout(btnLayout);

  connect(openFileBtn, &QPushButton::clicked, dialog, [listWidget] {
    auto *item = listWidget->currentItem();
    if (item) {
      const QString path = item->data(Qt::UserRole).toString();
      if (!path.isEmpty() && QFileInfo::exists(path)) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
      }
    }
  });

  connect(openDirBtn, &QPushButton::clicked, dialog, [] {
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
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

  auto *newTabAct = menu->addAction("➕ แท็บใหม่\tCtrl+T");
  connect(newTabAct, &QAction::triggered, this, &MainWindow::newTab);

  auto *newWinAct = menu->addAction("🗔 หน้าต่างใหม่\tCtrl+N");
  connect(newWinAct, &QAction::triggered, this, [this] {
    auto *w = new MainWindow(nullptr, privateMode_);
    w->setAttribute(Qt::WA_DeleteOnClose);
    w->show();
  });

  auto *newPrivWinAct = menu->addAction("🕵️ หน้าต่างส่วนตัวใหม่\tCtrl+Shift+N");
  connect(newPrivWinAct, &QAction::triggered, this, [] {
    auto *w = new MainWindow(nullptr, true);
    w->setAttribute(Qt::WA_DeleteOnClose);
    w->show();
  });

  menu->addSeparator();

  auto *historyMenu = menu->addMenu("📜 ประวัติการใช้งาน");
  connect(historyMenu, &QMenu::aboutToShow, this, [this, historyMenu] {
    populateHistoryMenu(historyMenu);
  });
  populateHistoryMenu(historyMenu);

  auto *bookmarksMenu = menu->addMenu("★ บุ๊กมาร์ก");
  connect(bookmarksMenu, &QMenu::aboutToShow, this, [this, bookmarksMenu] {
    populateBookmarksMenu(bookmarksMenu);
  });
  populateBookmarksMenu(bookmarksMenu);

  auto *downloadsAct = menu->addAction("⬇️ การดาวน์โหลด\tCtrl+J");
  connect(downloadsAct, &QAction::triggered, this, &MainWindow::showDownloadsDialog);

  auto *clearDataAct = menu->addAction("🗑️ ล้างข้อมูลการท่องเว็บ…\tCtrl+Shift+Del");
  connect(clearDataAct, &QAction::triggered, this, &MainWindow::clearBrowsingDataDialog);

  menu->addSeparator();

  auto *zoomMenu = menu->addMenu("🔍 ซูมหน้าเว็บ");
  auto *zoomInAct = zoomMenu->addAction("➕ ขยาย (+10%)");
  connect(zoomInAct, &QAction::triggered, this, [this] {
    if (currentView())
      currentView()->setZoomFactor(std::min(5.0, currentView()->zoomFactor() + 0.1));
  });
  auto *zoomOutAct = zoomMenu->addAction("➖ ย่อ (-10%)");
  connect(zoomOutAct, &QAction::triggered, this, [this] {
    if (currentView())
      currentView()->setZoomFactor(std::max(0.25, currentView()->zoomFactor() - 0.1));
  });
  auto *zoomResetAct = zoomMenu->addAction("🎯 ขนาดปกติ (100%)\tCtrl+0");
  connect(zoomResetAct, &QAction::triggered, this, [this] {
    if (currentView())
      currentView()->setZoomFactor(1.0);
  });
  auto *fullScreenAct = zoomMenu->addAction("⛶ เต็มจอ (Full Screen)\tF11");
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

  auto *printAct = menu->addAction("🖨️ พิมพ์…\tCtrl+P");
  connect(printAct, &QAction::triggered, this, [this] {
    if (!currentView())
      return;
    auto *printer = new QPrinter(QPrinter::HighResolution);
    QPrintDialog dialog(printer, this);
    if (dialog.exec() == QDialog::Accepted) {
      connect(currentView(), &QWebEngineView::printFinished, currentView(),
              [printer](bool) { delete printer; }, Qt::SingleShotConnection);
      currentView()->print(printer);
    } else {
      delete printer;
    }
  });

  auto *findAct = menu->addAction("🔎 ค้นหาในหน้าเว็บ…\tCtrl+F");
  connect(findAct, &QAction::triggered, this, [this] {
    bool ok = false;
    const QString q = QInputDialog::getText(
        this, "ค้นหาในหน้าเว็บ", "ข้อความที่ต้องการค้นหา:", QLineEdit::Normal, findQuery_, &ok);
    if (ok && currentView()) {
      findQuery_ = q;
      currentView()->findText(q);
    }
  });

  auto *saveAct = menu->addAction("💾 บันทึกหน้าเว็บ…\tCtrl+S");
  connect(saveAct, &QAction::triggered, this, [this] {
    if (!currentView())
      return;
    const QString path = QFileDialog::getSaveFileName(
        this, "บันทึกหน้าเว็บ", QString(), "Web archive (*.mhtml)");
    if (!path.isEmpty())
      currentView()->page()->save(path, QWebEngineDownloadRequest::MimeHtmlSaveFormat);
  });

  menu->addSeparator();

  auto *themeAct = menu->addAction(darkMode_ ? "เปลี่ยนเป็นโหมดสว่าง" : "เปลี่ยนเป็นโหมดมืด");
  connect(themeAct, &QAction::triggered, this, &MainWindow::toggleTheme);

  auto *settingsAct = menu->addAction("การตั้งค่า (Settings)");
  connect(settingsAct, &QAction::triggered, this, &MainWindow::showSettingsDialog);

  auto *aboutAct = menu->addAction("เกี่ยวกับ LiteWave");
  connect(aboutAct, &QAction::triggered, this, [this] {
    QMessageBox::about(
        this, "เกี่ยวกับ LiteWave Browser",
        "<h3>LiteWave Browser v1.0</h3>"
        "<p>เบราว์เซอร์ความเร็วสูง น้ำหนักเบา ปลอดภัย และใช้งานง่าย</p>"
        "<p><b>ฟีเจอร์หลัก:</b>"
        "<ul>"
        "<li>ระบบ Shield บล็อกโฆษณาและตัวสะกดรอยอัตโนมัติ</li>"
        "<li>ระบบ Secure DNS (DNS-over-HTTPS) คุณภาพสูง</li>"
        "<li>รองรับแท็บหลายหน้าต่าง และโหมดส่วนตัว (Private Mode)</li>"
        "<li>โหมดสว่าง/มืด (Dark/Light Mode)</li>"
        "<li>ค้นหาด่วน Google/Brave/DuckDuckGo</li>"
        "</ul></p>");
  });

  auto *exitAct = menu->addAction("ออกจากโปรแกรม\tAlt+F4");
  connect(exitAct, &QAction::triggered, this, &QWidget::close);

  return menu;
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

void MainWindow::showSettingsDialog() {
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
  const QColor iconColor = isDark ? QColor("#ff6611") : QColor("#ff5500");

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
      border: 1px solid #ff5500;
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
      border-color: #ff5500;
      color: #ff5500;
    }
  )").arg(dialogBg, textColor, cardBg, borderColor));

  auto *mainLayout = new QVBoxLayout(&dialog);
  mainLayout->setContentsMargins(20, 20, 20, 20);
  mainLayout->setSpacing(16);

  auto *headerLabel = new QLabel("การตั้งค่า LiteWave", &dialog);
  headerLabel->setStyleSheet("font-size: 20px; font-weight: bold; letter-spacing: -0.5px;");
  mainLayout->addWidget(headerLabel);

  auto *contentLayout = new QHBoxLayout();
  contentLayout->setSpacing(20);

  auto *sidebar = new QListWidget(&dialog);
  sidebar->setFixedWidth(210);
  sidebar->setIconSize(QSize(20, 20));
  sidebar->setStyleSheet(QString(R"(
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
      background-color: #ff5500;
      color: #ffffff;
      font-weight: bold;
    }
  )").arg(cardBg, borderColor, textColor, isDark ? "#374151" : "#f1f5f9"));

  auto addSidebarItem = [&](const QString &title, const QString &iconName) {
    auto *item = new QListWidgetItem(createCategoryIcon(iconName, iconColor), title, sidebar);
    return item;
  };

  addSidebarItem("การแสดงผล", "appearance");
  addSidebarItem("เครื่องมือค้นหา", "search");
  addSidebarItem("เมื่อเริ่มต้นทำงาน", "startup");
  addSidebarItem("LiteWave Shield", "shield");
  addSidebarItem("ความเป็นส่วนตัว & DNS", "privacy");
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

  auto *showStatusBarBox = new QCheckBox("แสดงแถบสถานะด้านล่าง (Status Bar)", grpTheme);
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

  auto *lblSearch = new QLabel("เครื่องมือค้นหาที่จะใช้เมื่อระบุคำค้นหาในช่องที่อยู่ (Address Bar):", grpSearch);
  auto *searchCombo = new QComboBox(grpSearch);
  searchCombo->addItem("Google (www.google.com)", "Google");
  searchCombo->addItem("Brave Search (search.brave.com)", "Brave");
  searchCombo->addItem("DuckDuckGo (duckduckgo.com)", "DuckDuckGo");
  searchCombo->addItem("Bing (www.bing.com)", "Bing");

  const QString curEngine = st.value("searchEngine", "Google").toString();
  int idx = searchCombo->findData(curEngine);
  if (idx < 0) idx = 0;
  searchCombo->setCurrentIndex(idx);

  grpSearchLayout->addWidget(lblSearch);
  grpSearchLayout->addWidget(searchCombo);
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

  auto *rbHome = new QRadioButton("เปิดหน้าเริ่มต้นแท็บใหม่ (LiteWave Home Page)", grpStartup);
  auto *rbCustom = new QRadioButton("เปิดหน้าเว็บที่กำหนดเฉพาะ (Custom URL):", grpStartup);

  auto *customUrlEdit = new QLineEdit(grpStartup);
  customUrlEdit->setPlaceholderText("https://example.com");
  customUrlEdit->setText(st.value("customStartupUrl", "").toString());

  const QString startupOpt = st.value("startupOption", "home").toString();
  if (startupOpt == "custom") {
    rbCustom->setChecked(true);
    customUrlEdit->setEnabled(true);
  } else {
    rbHome->setChecked(true);
    customUrlEdit->setEnabled(false);
  }
  connect(rbCustom, &QRadioButton::toggled, customUrlEdit, &QLineEdit::setEnabled);

  grpStartupLayout->addWidget(rbHome);
  grpStartupLayout->addWidget(rbCustom);
  grpStartupLayout->addWidget(customUrlEdit);
  p2Layout->addWidget(grpStartup);
  p2Layout->addStretch();
  stacked->addWidget(page2);

  // ---------------- Page 3: LiteWave Shield ----------------
  auto *page3 = new QWidget();
  auto *p3Layout = new QVBoxLayout(page3);
  p3Layout->setSpacing(16);
  p3Layout->setContentsMargins(0, 0, 0, 0);

  auto *grpShield = new QGroupBox("ความปลอดภัย LiteWave Shield", page3);
  auto *grpShieldLayout = new QVBoxLayout(grpShield);
  grpShieldLayout->setSpacing(12);

  auto *shieldBox = new QCheckBox("เปิดใช้งาน LiteWave Shield (บล็อกโฆษณาและตัวสะกดรอยอัตโนมัติ)", grpShield);
  shieldBox->setChecked(adBlocker_ ? adBlocker_->isEnabled() : true);

  auto *dntBox = new QCheckBox("ส่งคำขอ Do Not Track (DNT) ไปยังทุกเว็บไซต์", grpShield);
  dntBox->setChecked(st.value("dntEnabled", true).toBool());

  auto *clearDataBtn = new QPushButton("ล้างข้อมูลการท่องเว็บ ประวัติ แคช และคุกกี้...", grpShield);
  connect(clearDataBtn, &QPushButton::clicked, &dialog, [this] {
    clearBrowsingDataDialog();
  });

  grpShieldLayout->addWidget(shieldBox);
  grpShieldLayout->addWidget(dntBox);
  grpShieldLayout->addWidget(clearDataBtn);
  p3Layout->addWidget(grpShield);
  p3Layout->addStretch();
  stacked->addWidget(page3);

  // ---------------- Page 4: Privacy & Secure DNS ----------------
  auto *page4 = new QWidget();
  auto *p4Layout = new QVBoxLayout(page4);
  p4Layout->setSpacing(16);
  p4Layout->setContentsMargins(0, 0, 0, 0);

  auto *grpDns = new QGroupBox("Secure DNS (DNS-over-HTTPS)", page4);
  auto *grpDnsLayout = new QVBoxLayout(grpDns);
  grpDnsLayout->setSpacing(10);

  auto *secureDnsBox = new QCheckBox("เปิดใช้งาน Secure DNS (DNS-over-HTTPS)", grpDns);
  secureDnsBox->setChecked(st.value("secureDnsEnabled", true).toBool());

  auto *lblDnsDesc = new QLabel(
      "เข้ารหัสคำขอค้นหาชื่อโดเมน (DNS) เพื่อป้องกันการสะกดรอย เพิ่มความปลอดภัยและความเร็วในการท่องเว็บ",
      grpDns);
  lblDnsDesc->setWordWrap(true);
  lblDnsDesc->setStyleSheet(QString("color: %1; font-size: 12px; margin-bottom: 6px;").arg(subTextColor));

  auto *lblProvider = new QLabel("ผู้ให้บริการ DNS (Select DNS Provider):", grpDns);
  auto *dnsCombo = new QComboBox(grpDns);
  dnsCombo->addItem("OS Default (ตามการตั้งค่าของระบบ)", "OS Default");
  dnsCombo->addItem("Cloudflare (1.1.1.1 / 1.0.0.1)", "Cloudflare");
  dnsCombo->addItem("Google Public DNS (8.8.8.8 / 8.8.4.4)", "Google");
  dnsCombo->addItem("Quad9 (9.9.9.9)", "Quad9");
  dnsCombo->addItem("AdGuard DNS (บล็อกโฆษณาและความเป็นส่วนตัว)", "AdGuard");
  dnsCombo->addItem("กำหนด DoH Server URL เอง (Custom)", "Custom");

  const QString curProvider = st.value("dnsProvider", "Cloudflare").toString();
  int pIdx = dnsCombo->findData(curProvider);
  if (pIdx < 0) pIdx = 1;
  dnsCombo->setCurrentIndex(pIdx);

  auto *customDnsEdit = new QLineEdit(grpDns);
  customDnsEdit->setPlaceholderText("https://example.com/dns-query");
  customDnsEdit->setText(st.value("customDnsUrl", "").toString());
  customDnsEdit->setVisible(curProvider == "Custom");

  connect(dnsCombo, &QComboBox::currentIndexChanged, &dialog, [dnsCombo, customDnsEdit] {
    customDnsEdit->setVisible(dnsCombo->currentData().toString() == "Custom");
  });

  auto *lblDnsNote = new QLabel("หมายเหตุ: การเปลี่ยน Secure DNS จะมีผลสมบูรณ์เมื่อเปิดเบราว์เซอร์ใหม่ครั้งถัดไป", grpDns);
  lblDnsNote->setStyleSheet("color: #f59e0b; font-size: 11px; margin-top: 4px;");

  grpDnsLayout->addWidget(secureDnsBox);
  grpDnsLayout->addWidget(lblDnsDesc);
  grpDnsLayout->addWidget(lblProvider);
  grpDnsLayout->addWidget(dnsCombo);
  grpDnsLayout->addWidget(customDnsEdit);
  grpDnsLayout->addWidget(lblDnsNote);
  p4Layout->addWidget(grpDns);
  p4Layout->addStretch();
  stacked->addWidget(page4);

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
  const QString defaultDl = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
  downloadPathEdit->setText(st.value("downloadDirectory", defaultDl).toString());

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

  auto *askDownloadBox = new QCheckBox("ถามสถานที่บันทึกทุกครั้งก่อนดาวน์โหลด", grpDownloads);
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

  auto *lblResetMsg = new QLabel("คืนค่าการตั้งค่าเบราว์เซอร์ LiteWave ทั้งหมดกลับเป็นค่าเริ่มต้น", grpReset);
  auto *resetAllBtn = new QPushButton("คืนค่าการตั้งค่าทั้งหมด (Reset All Settings)", grpReset);
  resetAllBtn->setStyleSheet("background-color: #ef4444; color: white; font-weight: bold; border: none; padding: 8px 16px;");

  connect(resetAllBtn, &QPushButton::clicked, &dialog, [this, &dialog] {
    const auto res = QMessageBox::warning(
        &dialog, "ยืนยันการคืนค่า",
        "คุณแน่ใจหรือว่าต้องการคืนค่าการตั้งค่าทั้งหมดเป็นค่าเริ่มต้น?",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (res == QMessageBox::Yes) {
      QSettings stReset("LiteWave", "LiteWave");
      stReset.clear();
      if (darkMode_) toggleTheme();
      if (adBlocker_) adBlocker_->setEnabled(true);
      statusBar()->show();
      QMessageBox::information(&dialog, "สำเร็จ", "คืนค่าการตั้งค่าทั้งหมดเรียบร้อยแล้ว");
      dialog.accept();
    }
  });

  grpResetLayout->addWidget(lblResetMsg);
  grpResetLayout->addWidget(resetAllBtn);
  p6Layout->addWidget(grpReset);
  p6Layout->addStretch();
  stacked->addWidget(page6);

  connect(sidebar, &QListWidget::currentRowChanged, stacked, &QStackedWidget::setCurrentIndex);
  sidebar->setCurrentRow(0);

  contentLayout->addWidget(sidebar);
  contentLayout->addWidget(stacked, 1);
  mainLayout->addLayout(contentLayout);

  auto *btnLayout = new QHBoxLayout();
  btnLayout->addStretch();
  auto *saveBtn = new QPushButton("ตกลง", &dialog);
  saveBtn->setDefault(true);
  saveBtn->setStyleSheet("background-color: #ff5500; color: white; font-weight: bold; border: none; padding: 7px 20px; border-radius: 6px;");
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

    st.setValue("searchEngine", searchCombo->currentData().toString());

    if (rbCustom->isChecked()) {
      st.setValue("startupOption", "custom");
      st.setValue("customStartupUrl", customUrlEdit->text().trimmed());
    } else {
      st.setValue("startupOption", "home");
    }

    if (adBlocker_) {
      adBlocker_->setEnabled(shieldBox->isChecked());
      st.setValue("shieldEnabled", shieldBox->isChecked());
      refreshShield();
    }
    st.setValue("dntEnabled", dntBox->isChecked());

    st.setValue("secureDnsEnabled", secureDnsBox->isChecked());
    st.setValue("dnsProvider", dnsCombo->currentData().toString());
    st.setValue("customDnsUrl", customDnsEdit->text().trimmed());

    st.setValue("downloadDirectory", downloadPathEdit->text().trimmed());
    st.setValue("askDownloadLocation", askDownloadBox->isChecked());

    statusBar()->showMessage("บันทึกการตั้งค่าเรียบร้อยแล้ว", 3000);
  }
}
