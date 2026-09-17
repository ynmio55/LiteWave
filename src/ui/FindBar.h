#pragma once

#include <QWidget>
#include <QPointer>

class QLineEdit;
class QLabel;
class QToolButton;
class QWebEngineView;

class FindBar final : public QWidget {
  Q_OBJECT

public:
  explicit FindBar(QWidget *parent = nullptr);
  ~FindBar() override = default;

  void attachView(QWebEngineView *view);
  void showAndFocus(const QString &initialQuery = QString());
  void hideAndClear();
  void setDarkMode(bool dark);

protected:
  bool eventFilter(QObject *watched, QEvent *event) override;

public slots:
  void findNext();
  void findPrevious();

private slots:
  void onSearchTextChanged(const QString &text);

private:
  void performFind(bool backward);
  void updateStyle();

  QLineEdit *findInput_ = nullptr;
  QLabel *matchCountLabel_ = nullptr;
  QToolButton *prevBtn_ = nullptr;
  QToolButton *nextBtn_ = nullptr;
  QToolButton *closeBtn_ = nullptr;
  QPointer<QWebEngineView> currentView_;
  bool darkMode_ = false;
  QString lastQuery_;
};
