#pragma once

#include <QMainWindow>

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

private:
    QLineEdit *urlBar_;
    QWebEngineView *webView_;

    void openUrl(const QString &text);
};
