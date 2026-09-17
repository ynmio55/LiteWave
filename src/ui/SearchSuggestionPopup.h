#pragma once

#include <QFrame>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class QListWidget;
class QListWidgetItem;
class QLineEdit;

struct SuggestionItem {
  enum Type {
    History,
    SearchSuggestion,
    Website
  };

  Type type = SearchSuggestion;
  QString text;
  QString urlOrQuery;
  QString subtitle;
};

class SearchSuggestionPopup : public QFrame {
  Q_OBJECT

public:
  explicit SearchSuggestionPopup(QWidget *parent = nullptr);
  ~SearchSuggestionPopup() override;

  void attachTo(QLineEdit *targetEdit);
  void setDarkMode(bool dark);
  void queryChanged(const QString &query);
  bool handleKeyPress(int key);
  void reposition();

signals:
  void suggestionSelected(const QString &text, bool isDirectUrl);

protected:
  bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
  void onReplyFinished();
  void onItemClicked(QListWidgetItem *item);

private:
  void updateStyle();
  void fetchLiveSuggestions(const QString &query);
  QVector<SuggestionItem> getHistorySuggestions(const QString &query);
  void renderItems(const QVector<SuggestionItem> &items);
  void selectIndex(int index);

  QLineEdit *targetEdit_ = nullptr;
  QListWidget *listWidget_ = nullptr;
  QNetworkAccessManager *nam_ = nullptr;
  QNetworkReply *currentReply_ = nullptr;
  QString lastQuery_;
  QVector<SuggestionItem> currentItems_;
  int selectedIndex_ = -1;
  bool darkMode_ = false;
};
