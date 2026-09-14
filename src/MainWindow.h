#pragma once

#include <QMainWindow>

class AdBlocker;
class QLineEdit;
class QWebEngineView;

class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void navigate();
    void updateUrl(const QUrl &url);
    void updateTitle(const QString &title);
    void toggleTheme();

private:
    QLineEdit *urlBar_;
    QWebEngineView *webView_;
    AdBlocker *adBlocker_;
    bool darkMode_ = false;

    void openUrl(const QString &text);
    void applyTheme();
};
