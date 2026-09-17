#pragma once

#include <QMainWindow>
#include <QUrl>
#include <QList>
#include <QPoint>
#include <QPointer>

class QWebEngineProfile;
class QWebEngineDownloadRequest;
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
class AdBlocker;
class FindBar;
class SearchSuggestionPopup;
class QCloseEvent;
class QResizeEvent;

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

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void changeEvent(QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private slots:
    void navigate();
    void newTab();
    void closeTab(int index);
    void updateCurrentUrl(const QUrl &url);
    void updateTabTitle(const QString &title);
    void addBookmark();
    void toggleTheme();

private:
    QLineEdit *urlBar_;
    QTabBar *tabBar_ = nullptr;
    QStackedWidget *tabStack_ = nullptr;
    QProgressBar *progress_;
    QToolBar *toolbar_;
    QWidget *headerWidget_ = nullptr;
    QWidget *tabBarContainer_ = nullptr;
    QToolButton *minWinBtn_ = nullptr;
    QToolButton *maxWinBtn_ = nullptr;
    QToolButton *closeWinBtn_ = nullptr;
    QToolButton *themeBtn_ = nullptr;
    QToolButton *shieldBtn_ = nullptr;
    QAction *sslAction_ = nullptr;
    QLabel *sslLabel_ = nullptr;
    bool darkMode_ = false;
    bool privateMode_ = false;
    QWebEngineProfile *profile_ = nullptr;
    AdBlocker *adBlocker_ = nullptr;
    QList<QUrl> closedTabs_;
    QString findQuery_;
    QPoint dragPosition_;

    struct DownloadRecord {
        QString fileName;
        QString path;
        qint64 totalBytes = 0;
        bool completed = false;
    };

    struct ActiveDownload {
        QPointer<QWebEngineDownloadRequest> request;
        QString fileName;
        QString path;
        qint64 totalBytes = 0;
        qint64 receivedBytes = 0;
        qint64 lastBytes = 0;
        qint64 lastTimeMs = 0;
        double speed = 0.0;
        bool completed = false;
        bool failed = false;
    };

    struct BookmarkItem {
        QString title;
        QString url;
        qint64 addedTime = 0;
    };

    QList<DownloadRecord> downloadRecords_;
    QList<ActiveDownload> activeDownloads_;
    QList<BookmarkItem> bookmarks_;
    QAction *bookmarkAction_ = nullptr;
    QToolButton *downloadsBtn_ = nullptr;
    QPointer<QWidget> downloadPopup_;
    QToolButton *menuBtn_ = nullptr;
    QCompleter *urlCompleter_ = nullptr;
    FindBar *findBar_ = nullptr;
    SearchSuggestionPopup *suggestionPopup_ = nullptr;

    void updateDownloadsButtonUi();
    void showDownloadPopup();

    void setupShortcuts();
    QWebEngineView *currentView() const;
    void openUrl(const QString &text);
    void applyTheme();
    void updateWindowControls();
    QMenu *createMainMenu();
    void populateHistoryMenu(QMenu *menu);
    void populateBookmarksMenu(QMenu *menu);
    void addHistoryItem(const QString &title, const QUrl &url);
    void loadBookmarks();
    void saveBookmarks();
    void showBookmarksManagerDialog();
    void updateBookmarkStarState();
    void handleDownloadRequested(QWebEngineDownloadRequest *download);
    void showDownloadsDialog();
    void clearBrowsingDataDialog();
    void showSettingsDialog(int initialPage = 0);
    void setupUrlBarCompleter();
    void refreshShieldUi();
    void applyShieldCosmetics(QWebEngineView *view);
    void showTabContextMenu(const QPoint &pos);
    void updateTabAudioIcon(int index, bool audible, bool muted);
    void openDevTools(QWebEngineView *targetView);
    void saveSession();
    void restoreSession();
    void loadDownloadRecords();
    void saveDownloadRecords();
};

