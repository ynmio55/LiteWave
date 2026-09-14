#pragma once

#include <QMainWindow>
#include <QUrl>
#include <QList>

class QWebEngineProfile;
class AdBlocker;
class QLineEdit;
class QProgressBar;
class QTabWidget;
class QWebEngineView;
class QToolBar;
class QAction;
class QLabel;
class QToolButton;

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
    QTabWidget *tabs_;
    QProgressBar *progress_;
    QToolBar *toolbar_;
    QAction *shieldAction_;
    QAction *siteShieldAction_ = nullptr;
    QAction *filterInfoAction_ = nullptr;
    QAction *themeAction_ = nullptr;
    QToolButton *shieldBtn_ = nullptr;
    QLabel *sslLabel_ = nullptr;
    AdBlocker *adBlocker_;
    bool darkMode_ = false;
    bool privateMode_ = false;
    QWebEngineProfile *profile_ = nullptr;
    QList<QUrl> closedTabs_;
    QString findQuery_;

    void setupShortcuts();
    void refreshShield();
    QWebEngineView *currentView() const;
    void openUrl(const QString &text);
    void applyTheme();
};

