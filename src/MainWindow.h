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
public:
    explicit MainWindow(QWidget *parent = nullptr, bool privateMode = false);
    ~MainWindow() override;
    void loadHome(QWebEngineView *view);
    void handleHomeNavigation(const QUrl &url, QWebEngineView *view);
    void reloadCurrentView();

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
    QToolButton *shieldBtn_ = nullptr;
    QLabel *sslLabel_ = nullptr;
    AdBlocker *adBlocker_;
    bool darkMode_ = false;
    bool privateMode_ = false;
    QWebEngineProfile *profile_ = nullptr;
    QList<QUrl> closedTabs_;
    QString findQuery_;

    void setupShortcuts();
    void setupUserScripts();
    QWebEngineView *currentView() const;
    QWebEngineView *createView(const QUrl &url);
    void openUrl(const QString &text);
    void applyTheme();
};

