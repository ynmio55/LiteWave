#include "SearchEngineManager.h"
#include <QSettings>
#include <QUrlQuery>

SearchEngineManager::SearchEngineManager(QObject *parent) : QObject(parent) {
  engines_ = {
      {"google", "Google", "https://www.google.com/search?q=%1",
       "https://suggestqueries.google.com/complete/search?client=chrome&q=%1"},
      {"duckduckgo", "DuckDuckGo", "https://duckduckgo.com/?q=%1",
       "https://ac.duckduckgo.com/ac/?q=%1&type=list"},
      {"brave", "Brave Search", "https://search.brave.com/search?q=%1",
       "https://search.brave.com/api/suggest?q=%1"},
      {"bing", "Bing", "https://www.bing.com/search?q=%1",
       "https://api.bing.com/osjson.aspx?query=%1"},
      {"litewave", "LiteWave Aggregated Search (Custom)",
       "http://localhost:8080/search?q=%1",
       "http://localhost:8080/api/suggest?q=%1"}};

  loadSettings();
}

SearchEngineManager &SearchEngineManager::instance() {
  static SearchEngineManager mgr;
  return mgr;
}

QVector<SearchEngineInfo> SearchEngineManager::availableEngines() const {
  return engines_;
}

SearchEngineInfo SearchEngineManager::currentEngine() const {
  for (const auto &eng : engines_) {
    if (eng.id == currentEngineId_) {
      return eng;
    }
  }
  return engines_.first();
}

void SearchEngineManager::setCurrentEngineId(const QString &id) {
  if (currentEngineId_ == id)
    return;
  currentEngineId_ = id;
  saveSettings();
  emit engineChanged(currentEngine());
}

QUrl SearchEngineManager::buildSearchUrl(const QString &query) const {
  const auto eng = currentEngine();
  const QString encodedQuery = QUrl::toPercentEncoding(query);
  return QUrl(eng.searchUrlTemplate.arg(encodedQuery));
}

QUrl SearchEngineManager::buildSuggestUrl(const QString &query) const {
  const auto eng = currentEngine();
  if (eng.suggestUrlTemplate.isEmpty())
    return QUrl();
  const QString encodedQuery = QUrl::toPercentEncoding(query);
  return QUrl(eng.suggestUrlTemplate.arg(encodedQuery));
}

void SearchEngineManager::addCustomEngine(const SearchEngineInfo &engine) {
  for (int i = 0; i < engines_.size(); ++i) {
    if (engines_[i].id == engine.id) {
      engines_[i] = engine;
      saveSettings();
      return;
    }
  }
  engines_.append(engine);
  saveSettings();
}

void SearchEngineManager::loadSettings() {
  QSettings settings("LiteWave", "LiteWave");
  QString id = settings.value("search/engineId").toString();
  if (id.isEmpty()) {
    const QString legacy = settings.value("searchEngine", "Google").toString().toLower();
    if (legacy == "google" || legacy == "brave" || legacy == "duckduckgo" || legacy == "bing") {
      id = legacy;
    } else {
      id = "google";
    }
  }
  currentEngineId_ = id;
}

void SearchEngineManager::saveSettings() {
  QSettings settings("LiteWave", "LiteWave");
  settings.setValue("search/engineId", currentEngineId_);
  settings.setValue("searchEngine", currentEngine().name);
}

