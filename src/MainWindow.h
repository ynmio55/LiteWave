#pragma once

#include <QMainWindow>
#include <QUrl>

class AdBlocker;
class QLineEdit;
class QProgressBar;
class QTabWidget;
class QWebEngineView;
class QToolBar;
class QAction;

class MainWindow final : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void navigate();
    void newTab();
    void closeTab(int index);
    void updateCurrentUrl(const QUrl &url);
    void updateTabTitle(const QString &title);
    void addBookmark();
    void toggleTheme();
    void toggleShield(bool enabled);

private:
    QLineEdit *urlBar_;
    QTabWidget *tabs_;
    QProgressBar *progress_;
    QToolBar *toolbar_;
    QAction *shieldAction_;
    AdBlocker *adBlocker_;
    bool darkMode_ = false;

    QWebEngineView *currentView() const;
    QWebEngineView *createView(const QUrl &url);
    void openUrl(const QString &text);
    void applyTheme();
    void loadHome(QWebEngineView *view);
};
