#pragma once

#include <QObject>
#include <QString>
#include <QUrl>
#include <QVector>

struct SearchEngineInfo {
  QString id;
  QString name;
  QString searchUrlTemplate;   // e.g. "https://www.google.com/search?q=%1"
  QString suggestUrlTemplate;  // e.g. "https://suggestqueries.google.com/complete/search?client=chrome&q=%1"
};

class SearchEngineManager : public QObject {
  Q_OBJECT

public:
  explicit SearchEngineManager(QObject *parent = nullptr);

  static SearchEngineManager &instance();

  QVector<SearchEngineInfo> availableEngines() const;
  SearchEngineInfo currentEngine() const;
  void setCurrentEngineId(const QString &id);

  QUrl buildSearchUrl(const QString &query) const;
  QUrl buildSuggestUrl(const QString &query) const;

  void addCustomEngine(const SearchEngineInfo &engine);

signals:
  void engineChanged(const SearchEngineInfo &engine);

private:
  void loadSettings();
  void saveSettings();

  QVector<SearchEngineInfo> engines_;
  QString currentEngineId_;
};
