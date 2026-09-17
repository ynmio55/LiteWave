#include "FindBar.h"

#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QToolButton>
#include <QWebEngineFindTextResult>
#include <QWebEnginePage>
#include <QWebEngineView>

FindBar::FindBar(QWidget *parent) : QWidget(parent) {
  auto *layout = new QHBoxLayout(this);
  layout->setContentsMargins(10, 4, 10, 4);
  layout->setSpacing(6);

  findInput_ = new QLineEdit(this);
  findInput_->setPlaceholderText("ค้นหาในหน้านี้... (Enter ถัดไป, Shift+Enter ก่อนหน้า)");
  findInput_->setMinimumWidth(220);
  findInput_->installEventFilter(this);
  connect(findInput_, &QLineEdit::textChanged, this,
          &FindBar::onSearchTextChanged);

  matchCountLabel_ = new QLabel(this);
  matchCountLabel_->setMinimumWidth(80);
  matchCountLabel_->setAlignment(Qt::AlignCenter);

  prevBtn_ = new QToolButton(this);
  prevBtn_->setText("▲");
  prevBtn_->setToolTip("ผลลัพธ์ก่อนหน้า (Shift+Enter)");
  prevBtn_->setFixedSize(24, 24);
  connect(prevBtn_, &QToolButton::clicked, this, &FindBar::findPrevious);

  nextBtn_ = new QToolButton(this);
  nextBtn_->setText("▼");
  nextBtn_->setToolTip("ผลลัพธ์ถัดไป (Enter)");
  nextBtn_->setFixedSize(24, 24);
  connect(nextBtn_, &QToolButton::clicked, this, &FindBar::findNext);

  closeBtn_ = new QToolButton(this);
  closeBtn_->setText("✕");
  closeBtn_->setToolTip("ปิด (Esc)");
  closeBtn_->setFixedSize(24, 24);
  connect(closeBtn_, &QToolButton::clicked, this, &FindBar::hideAndClear);

  layout->addWidget(findInput_);
  layout->addWidget(matchCountLabel_);
  layout->addWidget(prevBtn_);
  layout->addWidget(nextBtn_);
  layout->addWidget(closeBtn_);

  updateStyle();
  hide();
}

void FindBar::attachView(QWebEngineView *view) {
  if (currentView_ == view)
    return;

  if (currentView_ && isVisible()) {
    currentView_->page()->findText(QString());
  }

  currentView_ = view;
  matchCountLabel_->setText(QString());

  if (isVisible() && !findInput_->text().isEmpty()) {
    onSearchTextChanged(findInput_->text());
  }
}

void FindBar::showAndFocus(const QString &initialQuery) {
  if (!initialQuery.isEmpty() && findInput_->text().isEmpty()) {
    findInput_->setText(initialQuery);
  }
  show();
  findInput_->selectAll();
  findInput_->setFocus();

  if (!findInput_->text().isEmpty()) {
    onSearchTextChanged(findInput_->text());
  }
}

void FindBar::hideAndClear() {
  hide();
  matchCountLabel_->setText(QString());
  if (currentView_) {
    currentView_->page()->findText(QString());
  }
}

void FindBar::setDarkMode(bool dark) {
  if (darkMode_ == dark)
    return;
  darkMode_ = dark;
  updateStyle();
}

void FindBar::onSearchTextChanged(const QString &text) {
  lastQuery_ = text;
  if (text.isEmpty()) {
    matchCountLabel_->setText(QString());
    if (currentView_) {
      currentView_->page()->findText(QString());
    }
    return;
  }
  performFind(false);
}

void FindBar::findNext() { performFind(false); }

void FindBar::findPrevious() { performFind(true); }

void FindBar::performFind(bool backward) {
  if (!currentView_ || lastQuery_.isEmpty()) {
    matchCountLabel_->setText(QString());
    return;
  }

  QWebEnginePage::FindFlags flags;
  if (backward) {
    flags |= QWebEnginePage::FindBackward;
  }

  currentView_->page()->findText(
      lastQuery_, flags, [this](const QWebEngineFindTextResult &result) {
        if (lastQuery_.isEmpty()) {
          matchCountLabel_->setText(QString());
          return;
        }
        const int total = result.numberOfMatches();
        const int current = result.activeMatch();
        if (total == 0) {
          matchCountLabel_->setText("ไม่พบคำ");
          matchCountLabel_->setStyleSheet("color: #ef4444; font-size: 12px; font-weight: bold;");
        } else {
          matchCountLabel_->setText(QString("%1 จาก %2").arg(current).arg(total));
          matchCountLabel_->setStyleSheet(
              darkMode_ ? "color: #94a3b8; font-size: 12px;"
                        : "color: #64748b; font-size: 12px;");
        }
      });
}

bool FindBar::eventFilter(QObject *watched, QEvent *event) {
  if (watched == findInput_ && event->type() == QEvent::KeyPress) {
    auto *keyEvent = static_cast<QKeyEvent *>(event);
    if (keyEvent->key() == Qt::Key_Escape) {
      hideAndClear();
      return true;
    }
    if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
      if (keyEvent->modifiers() & Qt::ShiftModifier) {
        findPrevious();
      } else {
        findNext();
      }
      return true;
    }
  }
  return QWidget::eventFilter(watched, event);
}

void FindBar::updateStyle() {
  const QString bg = darkMode_ ? "#1e293b" : "#f1f5f9";
  const QString border = darkMode_ ? "#334155" : "#cbd5e1";
  const QString textCol = darkMode_ ? "#f8fafc" : "#0f172a";
  const QString inputBg = darkMode_ ? "#0f172a" : "#ffffff";
  const QString btnHover = darkMode_ ? "#334155" : "#e2e8f0";

  setStyleSheet(QString(R"(
    FindBar {
      background-color: %1;
      border-bottom: 1px solid %2;
    }
    QLineEdit {
      background-color: %3;
      color: %4;
      border: 1px solid %2;
      border-radius: 6px;
      padding: 4px 8px;
      font-size: 13px;
    }
    QLineEdit:focus {
      border-color: #3b82f6;
    }
    QToolButton {
      background-color: transparent;
      color: %4;
      border: 1px solid transparent;
      border-radius: 4px;
      font-size: 12px;
    }
    QToolButton:hover {
      background-color: %5;
      border-color: %2;
    }
  )").arg(bg, border, inputBg, textCol, btnHover));
}
