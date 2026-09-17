#include "SearchSuggestionPopup.h"
#include "SearchEngineManager.h"

#include <QApplication>
#include <QEvent>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMouseEvent>
#include <QPainter>
#include <QSettings>
#include <QUrlQuery>
#include <QVBoxLayout>

static QPixmap createVectorIcon(SuggestionItem::Type type, bool dark) {
  QPixmap pixmap(20, 20);
  pixmap.fill(Qt::transparent);

  QPainter p(&pixmap);
  p.setRenderHint(QPainter::Antialiasing);

  const QColor color = dark ? QColor("#94a3b8") : QColor("#64748b");
  p.setPen(QPen(color, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

  if (type == SuggestionItem::History) {
    p.drawEllipse(2, 2, 15, 15);
    p.drawLine(10, 5, 10, 10);
    p.drawLine(10, 10, 14, 10);
  } else if (type == SuggestionItem::Website) {
    p.drawEllipse(2, 2, 15, 15);
    p.drawLine(2, 10, 17, 10);
    p.drawEllipse(6, 2, 7, 15);
  } else {
    // SearchSuggestion (Magnifier)
    p.drawEllipse(3, 3, 9, 9);
    p.drawLine(10, 10, 16, 16);
  }

  return pixmap;
}

SearchSuggestionPopup::SearchSuggestionPopup(QWidget *parent)
    : QFrame(parent),
      listWidget_(new QListWidget(this)),
      nam_(new QNetworkAccessManager(this)) {
  setObjectName("SearchSuggestionPopup");
  setFocusPolicy(Qt::NoFocus);

  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(4, 6, 4, 6);
  layout->setSpacing(0);

  listWidget_->setFrameShape(QFrame::NoFrame);
  listWidget_->setFocusPolicy(Qt::NoFocus);
  listWidget_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  listWidget_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  listWidget_->setSelectionMode(QAbstractItemView::SingleSelection);
  listWidget_->setMouseTracking(true);

  layout->addWidget(listWidget_);

  connect(listWidget_, &QListWidget::itemClicked, this, &SearchSuggestionPopup::onItemClicked);

  hide();
  updateStyle();
}

SearchSuggestionPopup::~SearchSuggestionPopup() {
  if (currentReply_) {
    currentReply_->abort();
    currentReply_->deleteLater();
    currentReply_ = nullptr;
  }
}

void SearchSuggestionPopup::attachTo(QLineEdit *targetEdit) {
  if (targetEdit_) {
    targetEdit_->removeEventFilter(this);
  }
  targetEdit_ = targetEdit;
  if (targetEdit_) {
    targetEdit_->installEventFilter(this);
  }
}

void SearchSuggestionPopup::setDarkMode(bool dark) {
  darkMode_ = dark;
  updateStyle();
  if (!currentItems_.isEmpty()) {
    renderItems(currentItems_);
  }
}

void SearchSuggestionPopup::updateStyle() {
  const QString bg = darkMode_ ? "#1e293b" : "#ffffff";
  const QString border = darkMode_ ? "#334155" : "#cbd5e1";
  const QString textCol = darkMode_ ? "#f8fafc" : "#0f172a";
  const QString hoverBg = darkMode_ ? "#334155" : "#f1f5f9";
  const QString selectedBg = darkMode_ ? "#2563eb" : "#e0e7ff";
  const QString selectedText = darkMode_ ? "#ffffff" : "#1e40af";

  setStyleSheet(QString(R"CSS(
    #SearchSuggestionPopup {
      background-color: %1;
      border: 1px solid %2;
      border-top: none;
      border-bottom-left-radius: 14px;
      border-bottom-right-radius: 14px;
    }
    QListWidget {
      background: transparent;
      outline: none;
    }
    QListWidget::item {
      height: 38px;
      border-radius: 8px;
      margin: 1px 4px;
      padding: 0 8px;
      color: %3;
    }
    QListWidget::item:hover {
      background-color: %4;
    }
    QListWidget::item:selected {
      background-color: %5;
      color: %6;
    }
  )CSS").arg(bg, border, textCol, hoverBg, selectedBg, selectedText));
}

void SearchSuggestionPopup::queryChanged(const QString &rawQuery) {
  const QString query = rawQuery.trimmed();
  if (query.isEmpty()) {
    if (currentReply_) {
      currentReply_->abort();
      currentReply_->deleteLater();
      currentReply_ = nullptr;
    }
    hide();
    currentItems_.clear();
    selectedIndex_ = -1;
    return;
  }

  lastQuery_ = query;
  selectedIndex_ = -1;

  // Immediately display local history matches while live suggestions load
  auto items = getHistorySuggestions(query);
  currentItems_ = items;
  renderItems(currentItems_);

  // Fetch live suggestions asynchronously
  fetchLiveSuggestions(query);
}

QVector<SuggestionItem> SearchSuggestionPopup::getHistorySuggestions(const QString &query) {
  QVector<SuggestionItem> items;
  QSettings st("LiteWave", "LiteWave");
  const QVariantList history = st.value("history").toList();

  const QString qLower = query.toLower();
  int count = 0;

  for (const auto &var : history) {
    if (count >= 3) break;
    const QVariantMap map = var.toMap();
    const QString title = map.value("title").toString();
    const QString urlStr = map.value("url").toString();
    const QUrl url(urlStr);

    if (url.hasQuery()) {
      QUrlQuery uq(url);
      if (uq.hasQueryItem("q")) {
        const QString searchQuery = uq.queryItemValue("q");
        if (searchQuery.toLower().contains(qLower) && searchQuery.compare(query, Qt::CaseInsensitive) != 0) {
          bool already = false;
          for (const auto &it : items) {
            if (it.text.compare(searchQuery, Qt::CaseInsensitive) == 0) {
              already = true;
              break;
            }
          }
          if (!already) {
            SuggestionItem item;
            item.type = SuggestionItem::History;
            item.text = searchQuery;
            item.urlOrQuery = searchQuery;
            item.subtitle = "ประวัติการค้นหา";
            items.append(item);
            count++;
            continue;
          }
        }
      }
    }

    // Match domain or title
    if (!title.isEmpty() && title.toLower().contains(qLower)) {
      const QString host = url.host().isEmpty() ? urlStr : url.host();
      if (!host.isEmpty()) {
        bool already = false;
        for (const auto &it : items) {
          if (it.text.compare(host, Qt::CaseInsensitive) == 0) {
            already = true;
            break;
          }
        }
        if (!already) {
          SuggestionItem item;
          item.type = SuggestionItem::Website;
          item.text = host;
          item.urlOrQuery = urlStr;
          item.subtitle = title.left(40);
          items.append(item);
          count++;
        }
      }
    }
  }

  return items;
}

void SearchSuggestionPopup::fetchLiveSuggestions(const QString &query) {
  if (currentReply_) {
    currentReply_->abort();
    currentReply_->deleteLater();
    currentReply_ = nullptr;
  }

  QUrl suggestUrl = SearchEngineManager::instance().buildSuggestUrl(query);
  if (!suggestUrl.isValid() || suggestUrl.isEmpty()) {
    return;
  }

  QNetworkRequest req(suggestUrl);
  req.setHeader(QNetworkRequest::UserAgentHeader,
                "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36");
  currentReply_ = nam_->get(req);
  connect(currentReply_, &QNetworkReply::finished, this, &SearchSuggestionPopup::onReplyFinished);
}

void SearchSuggestionPopup::onReplyFinished() {
  if (!currentReply_) return;

  if (currentReply_->error() == QNetworkReply::NoError) {
    const QByteArray data = currentReply_->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);

    QStringList suggestions;
    if (doc.isArray()) {
      const QJsonArray rootArr = doc.array();
      // Google, Bing, Brave style: ["query", ["sug1", "sug2", ...]]
      if (rootArr.size() >= 2 && rootArr[1].isArray()) {
        const QJsonArray sugArr = rootArr[1].toArray();
        for (const auto &val : sugArr) {
          if (val.isString()) {
            suggestions.append(val.toString());
          }
        }
      } else if (!rootArr.isEmpty() && rootArr[0].isObject()) {
        // DuckDuckGo style: [{"phrase": "..."}, ...]
        for (const auto &val : rootArr) {
          if (val.isObject()) {
            const QString ph = val.toObject().value("phrase").toString();
            if (!ph.isEmpty()) {
              suggestions.append(ph);
            }
          }
        }
      }
    }

    QVector<SuggestionItem> merged = getHistorySuggestions(lastQuery_);
    int added = 0;
    for (const QString &sug : suggestions) {
      if (added >= 8) break;
      bool exists = false;
      for (const auto &it : merged) {
        if (it.text.compare(sug, Qt::CaseInsensitive) == 0) {
          exists = true;
          break;
        }
      }
      if (!exists) {
        SuggestionItem item;
        item.type = SuggestionItem::SearchSuggestion;
        item.text = sug;
        item.urlOrQuery = sug;
        merged.append(item);
        added++;
      }
    }

    currentItems_ = merged;
    renderItems(currentItems_);
  }

  currentReply_->deleteLater();
  currentReply_ = nullptr;
}

void SearchSuggestionPopup::renderItems(const QVector<SuggestionItem> &items) {
  listWidget_->clear();

  if (items.isEmpty()) {
    hide();
    return;
  }

  for (const auto &item : items) {
    auto *listItem = new QListWidgetItem(listWidget_);
    listItem->setData(Qt::UserRole, item.text);
    listItem->setData(Qt::UserRole + 1, item.urlOrQuery);
    listItem->setData(Qt::UserRole + 2, (item.type == SuggestionItem::Website));

    auto *rowWidget = new QWidget();
    auto *rowLayout = new QHBoxLayout(rowWidget);
    rowLayout->setContentsMargins(6, 2, 8, 2);
    rowLayout->setSpacing(10);

    auto *iconLabel = new QLabel(rowWidget);
    iconLabel->setPixmap(createVectorIcon(item.type, darkMode_));
    iconLabel->setFixedSize(20, 20);

    auto *textLabel = new QLabel(item.text, rowWidget);
    QFont font = textLabel->font();
    font.setPointSize(10);
    textLabel->setFont(font);
    textLabel->setStyleSheet(QString("color: %1;").arg(darkMode_ ? "#f8fafc" : "#0f172a"));

    rowLayout->addWidget(iconLabel);
    rowLayout->addWidget(textLabel, 1);

    if (!item.subtitle.isEmpty()) {
      auto *subLabel = new QLabel(item.subtitle, rowWidget);
      QFont subFont = subLabel->font();
      subFont.setPointSize(9);
      subLabel->setFont(subFont);
      subLabel->setStyleSheet(QString("color: %1;").arg(darkMode_ ? "#94a3b8" : "#64748b"));
      rowLayout->addWidget(subLabel);
    }

    listItem->setSizeHint(QSize(0, 38));
    listWidget_->addItem(listItem);
    listWidget_->setItemWidget(listItem, rowWidget);
  }

  const int totalRows = qMin(items.size(), 9);
  const int rowHeight = 38;
  const int calculatedHeight = (totalRows * rowHeight) + 14;

  reposition();
  resize(width(), calculatedHeight);
  show();
  raise();
}

void SearchSuggestionPopup::reposition() {
  if (!targetEdit_ || !parentWidget()) return;

  const QPoint pos = targetEdit_->mapTo(parentWidget(), QPoint(0, targetEdit_->height()));
  move(pos.x(), pos.y());
  setFixedWidth(targetEdit_->width());
}

bool SearchSuggestionPopup::handleKeyPress(int key) {
  if (!isVisible() || currentItems_.isEmpty()) {
    return false;
  }

  if (key == Qt::Key_Down) {
    selectIndex((selectedIndex_ + 1) % currentItems_.size());
    return true;
  }
  if (key == Qt::Key_Up) {
    int next = selectedIndex_ - 1;
    if (next < 0) next = currentItems_.size() - 1;
    selectIndex(next);
    return true;
  }
  if (key == Qt::Key_Return || key == Qt::Key_Enter) {
    if (selectedIndex_ >= 0 && selectedIndex_ < currentItems_.size()) {
      const auto item = currentItems_[selectedIndex_];
      hide();
      emit suggestionSelected(item.urlOrQuery, item.type == SuggestionItem::Website);
      return true;
    }
  }
  if (key == Qt::Key_Escape) {
    hide();
    return true;
  }

  return false;
}

void SearchSuggestionPopup::selectIndex(int index) {
  if (index < 0 || index >= currentItems_.size()) {
    selectedIndex_ = -1;
    listWidget_->clearSelection();
    return;
  }

  selectedIndex_ = index;
  listWidget_->setCurrentRow(selectedIndex_);

  if (targetEdit_) {
    targetEdit_->blockSignals(true);
    targetEdit_->setText(currentItems_[selectedIndex_].text);
    targetEdit_->selectAll();
    targetEdit_->blockSignals(false);
  }
}

void SearchSuggestionPopup::onItemClicked(QListWidgetItem *item) {
  if (!item) return;
  const QString urlOrQuery = item->data(Qt::UserRole + 1).toString();
  const bool isWebsite = item->data(Qt::UserRole + 2).toBool();
  hide();
  emit suggestionSelected(urlOrQuery, isWebsite);
}

bool SearchSuggestionPopup::eventFilter(QObject *watched, QEvent *event) {
  if (watched == targetEdit_) {
    if (event->type() == QEvent::FocusOut) {
      // Delay hide to allow mouse click to process
      QPoint mousePos = mapFromGlobal(QCursor::pos());
      if (!rect().contains(mousePos)) {
        hide();
      }
    } else if (event->type() == QEvent::Resize) {
      reposition();
    }
  }
  return QFrame::eventFilter(watched, event);
}
