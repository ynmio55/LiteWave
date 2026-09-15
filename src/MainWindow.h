#pragma once

#include <QMainWindow>
#include <QUrl>
#include <QList>
#include <QPoint>

class QWebEngineProfile;
class AdBlocker;
class QLineEdit;
class QProgressBar;
class QTabBar;
class QStackedWidget;
class QWebEngineView;
class QToolBar;
class QAction;
class QLabel;
class QToolButton;
class QCompleter;

class MainWindow final : public QMainWindow
{
    Q_OBJECT
    friend class WebPage;
public:
    explicit MainWindow(QWidget *parent = nullptr, bool privateMode = false);
    ~MainWindow() override;
    QWebEngineView *createView(const QUrl &url);
    void loadHome(QWebEngineView *view);
    void handleHomeNavigation(const QUrl &url, QWebEngineView *view);
    void reloadCurrentView();
    void configurePageShield(QWebEngineView *view, const QUrl &url, bool applyNow = false);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void navigate();
    void newTab();
    void closeTab(int index);
    void updateCurrentUrl(const QUrl &url);
    void updateTabTitle(const QString &title);
    void addBookmark();
    void toggleTheme();
    void toggleShield(bool enabled);
    void updateShieldBadge(int count);

private:
    QLineEdit *urlBar_;
    QTabBar *tabBar_ = nullptr;
    QStackedWidget *tabStack_ = nullptr;
    QProgressBar *progress_;
    QToolBar *toolbar_;
    QWidget *headerWidget_ = nullptr;
    QWidget *tabBarContainer_ = nullptr;
    QToolButton *maxBtn_ = nullptr;
    QAction *shieldAction_;
    QAction *siteShieldAction_ = nullptr;
    QAction *filterInfoAction_ = nullptr;
    QToolButton *themeBtn_ = nullptr;
    QToolButton *shieldBtn_ = nullptr;
    QLabel *sslLabel_ = nullptr;
    AdBlocker *adBlocker_;
    bool darkMode_ = false;
    bool privateMode_ = false;
    QWebEngineProfile *profile_ = nullptr;
    QList<QUrl> closedTabs_;
    QString findQuery_;
    QPoint dragPosition_;

    struct DownloadRecord {
        QString fileName;
        QString path;
        qint64 totalBytes = 0;
        bool completed = false;
    };

    QList<DownloadRecord> downloadRecords_;
    QToolButton *menuBtn_ = nullptr;
    QCompleter *urlCompleter_ = nullptr;

    void setupShortcuts();
    void refreshShield();
    QWebEngineView *currentView() const;
    void openUrl(const QString &text);
    void applyTheme();
    QMenu *createMainMenu();
    void populateHistoryMenu(QMenu *menu);
    void populateBookmarksMenu(QMenu *menu);
    void addHistoryItem(const QString &title, const QUrl &url);
    void showDownloadsDialog();
    void clearBrowsingDataDialog();
    void showSettingsDialog();
    void setupUrlBarCompleter();
};
