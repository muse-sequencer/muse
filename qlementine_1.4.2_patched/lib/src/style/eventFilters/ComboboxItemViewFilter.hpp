// SPDX-FileCopyrightText: Olivier Cléro <oclero@hotmail.com>
// SPDX-License-Identifier: MIT

#pragma once


#include <oclero/qlementine/style/QlementineStyle.hpp>
#include <oclero/qlementine/style/Delegates.hpp>

#include <QStyledItemDelegate>
#include <QEvent>
#include <QObject>
#include <QComboBox>
#include <QTreeView>
#include <QAbstractItemView>
#include <QChildEvent>
#include <QScreen>
#include <cstdio>

namespace oclero::qlementine {

/// Returns true if the delegate is a default Qt delegate (not a custom subclass).
inline bool isDefaultItemDelegate(const QAbstractItemDelegate* delegate) {
  if (!delegate)
    return true;
  const auto* meta = delegate->metaObject();
  return meta == &QStyledItemDelegate::staticMetaObject || meta == &QItemDelegate::staticMetaObject;
}

// Event filter for the item view in the QComboBox's popup.
class ComboboxItemViewFilter : public QObject {
public:
  ComboboxItemViewFilter(QComboBox* comboBox, QAbstractItemView* view)
    : QObject(view)
    , _comboBox(comboBox)
    , _view(view)
    , _initialMaxHeight(view->maximumHeight()) {
    _view->installEventFilter(this);

    if (auto* viewport = _view->viewport()) {
      viewport->installEventFilter(this);
    }

    auto* comboBoxPopup = _view->parentWidget();
    comboBoxPopup->installEventFilter(this);
    _comboBox->installEventFilter(this);

    if (const auto* treeView = qobject_cast<QTreeView*>(_view)) {
      QObject::connect(treeView, &QTreeView::expanded, this, &ComboboxItemViewFilter::fixViewGeometry);
    }
  }

protected:
  bool eventFilter(QObject* watchedObject, QEvent* evt) override {
    switch (evt->type()) {
      case QEvent::Type::ChildAdded: {
        if (watchedObject == _comboBox) {
          const auto* childEvent = static_cast<QChildEvent*>(evt);
          const auto* child = childEvent->child();
          // NOTE: was `if (child == _comboBox->view())`. That re-enters
          //  QComboBox::view() from inside a ChildAdded event that fires
          //  as a side effect of view()'s OWN lazy construction of the
          //  popup container - infinite recursion, each level allocating
          //  a fresh QComboBoxPrivateContainer, until the process runs
          //  out of memory. 'child' is already the object we need to
          //  identify - check its type directly instead.
          if (qobject_cast<const QAbstractItemView*>(child)) {
            if (auto* qlementine = qobject_cast<QlementineStyle*>(_comboBox->style())) {
              if (isDefaultItemDelegate(_comboBox->itemDelegate())) {
                _comboBox->setItemDelegate(new ComboBoxDelegate(_comboBox, *qlementine));
              }
            }
          }
        }
      } break;
      case QEvent::Type::Show:
        fixViewGeometry();
        break;
      case QEvent::Type::Resize:
        if (watchedObject == _comboBox) {
          fixViewGeometry();
        }
        break;
      // case QEvent::Type::MouseButtonPress:
      //   if (watchedObject == _view->viewport()) {
      //     // _clickPoint = _view->itemat
      //     _view->indexAt()
      //     return false;
      //   }
      //   break;
      // case QEvent::Type::MouseButtonRelease:
      //   if (watchedObject == _view->viewport()) {
      //     return true;
      //   }
      //   break;
      default:
        break;
    }
    return false;
  }

private:
  void fixViewGeometry() const {
    if (_comboBox) {
      if (auto* view = _comboBox->view()) {
        if (const auto* qlementineStyle = qobject_cast<QlementineStyle*>(_comboBox->style())) {
          const auto isTreeView = view->inherits("QTreeView");
          const auto hMargin = qlementineStyle->pixelMetric(QStyle::PM_MenuHMargin);
          const auto shadowWidth = qlementineStyle->theme().spacing;
          const auto borderWidth = qlementineStyle->theme().borderWidth;

          // Width.
          const auto absoluteMinWidth = qlementineStyle->theme().controlHeightLarge * (isTreeView ? 2 : 1);
          const auto absoluteMaxWidth = qlementineStyle->theme().controlHeightLarge * 24;
          const auto width = std::min(absoluteMaxWidth, std::max({
                                                          _comboBox->width(),
                                                          view->sizeHintForColumn(0),
                                                          absoluteMinWidth,
                                                        }))
                             + shadowWidth * 2 + hMargin * 2 + borderWidth * 2;

          // Height.
          const auto absoluteMinHeight = qlementineStyle->theme().controlHeightLarge * (isTreeView ? 5 : 1);
          const auto screen = view->screen();
          const auto viewGlobalY =
            view->mapToGlobal(QPoint(0, 0)).y(); // Don't exceed the screen height when expanding a tree view.
          const auto absoluteMaxHeight = screen != nullptr ? screen->geometry().height() - 128 - viewGlobalY
                                                           : qlementineStyle->theme().controlHeightLarge * 10;
          const auto height = std::min(absoluteMaxHeight, std::max(absoluteMinHeight, viewMinimumSizeHint().height()));
          fprintf(stderr, "QLEMENTINE DEBUG: fixViewGeometry absoluteMinHeight=%d absoluteMaxHeight=%d "
                           "-> final height=%d, view size before=%dx%d\n",
                  absoluteMinHeight, absoluteMaxHeight, height, view->width(), view->height());

          view->setFixedWidth(width);
          view->setFixedHeight(height);
          view->parentWidget()->adjustSize();
          fprintf(stderr, "QLEMENTINE DEBUG: fixViewGeometry after setFixedHeight: view size=%dx%d "
                           "viewport size=%dx%d popup size=%dx%d\n",
                  view->width(), view->height(),
                  view->viewport() ? view->viewport()->width() : -1, view->viewport() ? view->viewport()->height() : -1,
                  view->parentWidget()->width(), view->parentWidget()->height());
        }
      }
    }
  }

  QSize viewMinimumSizeHint() const {
    // NOTE: previously clamped `height` against _initialMaxHeight on every
    //  iteration below (both loops). _initialMaxHeight is captured once,
    //  at construction time, from view->maximumHeight() - before the
    //  popup has ever been laid out for real content, so it can be as
    //  small as ~1 row's height. Clamping every iteration against that
    //  meant the accumulated height got capped at ~1 row regardless of
    //  actual item count, so items beyond the first had no vertical space
    //  in the popup - present but neither visible nor clickable. Removed
    //  the clamp here: fixViewGeometry() (the caller) already does the
    //  real, correct bounding against actual screen geometry right after
    //  calling this (see absoluteMinHeight/absoluteMaxHeight there), so
    //  this inner clamp was both redundant and the actual bug.
    auto height = 0;
    if (const auto* treeView = qobject_cast<QTreeView*>(_view)) {
      // For a QTreeView, look at expanded rows.
      auto currentIndex = treeView->indexAt(QPoint(0, 0));
      while (currentIndex.isValid()) {
        const auto rowSizeHint = _view->sizeHintForIndex(currentIndex).height();
        height += std::max(0, rowSizeHint);
        currentIndex = treeView->indexBelow(currentIndex);
      }
    } else {
      // QListView::minimumSizeHint() doesn't give the correct minimumHeight,
      // so we have to compute it.
      const auto rowCount = _view->model()->rowCount();
      fprintf(stderr, "QLEMENTINE DEBUG: viewMinimumSizeHint rowCount=%d initialMaxHeight=%d\n", rowCount, _initialMaxHeight);
      for (auto i = 0; i < rowCount; ++i) {
        const auto rowSizeHint = _view->sizeHintForRow(i);
        fprintf(stderr, "QLEMENTINE DEBUG:   row %d sizeHint=%d\n", i, rowSizeHint);
        height += rowSizeHint;
      }
      fprintf(stderr, "QLEMENTINE DEBUG: viewMinimumSizeHint total height=%d\n", height);
    }
    // It looks like it is OK for the width, though.
    const auto width = _view->sizeHintForColumn(0);
    return { width, height };
  }

  QComboBox* _comboBox{ nullptr };
  QAbstractItemView* _view{ nullptr };
  int _initialMaxHeight{ 0 };
  QModelIndex _clickedIndex{};
};

class ComboboxFilter : public QObject {
public:
  explicit ComboboxFilter(QComboBox* comboBox)
    : QObject(comboBox)
    , _comboBox(comboBox) {
    if (const auto* view = comboBox->view()) {
      if (auto* popup = view->parentWidget()) {
        popup->installEventFilter(this);
      }
    }
  }

  bool eventFilter(QObject* watchedObject, QEvent* evt) override {
    switch (evt->type()) {
      // This is the only way we found to know when the QComboBox's view has changed.
      case QEvent::ChildAdded: {
        const auto* childEvent = static_cast<QChildEvent*>(evt);
        const auto* child = childEvent->child();
        // NOTE: see the identical fix + explanation in
        //  ComboboxItemViewFilter::eventFilter() above.
        if (qobject_cast<const QAbstractItemView*>(child)) {
          if (auto* qlementine = qobject_cast<QlementineStyle*>(_comboBox->style())) {
            if (isDefaultItemDelegate(_comboBox->itemDelegate())) {
              _comboBox->setItemDelegate(new ComboBoxDelegate(_comboBox, *qlementine));
            }
          }

          // if (const auto* treeView = qobject_cast<QTreeView*>(child)) {
          //   QObject::connect(treeView, &QTreeView::expanded, this, [this]() {
          //     // &ComboboxItemViewFilter::fixViewGeometry
          //     qDebug() << "expanded";
          //     fixViewGeometry();

          //   });
          // }
        }
      } break;
      default:
        break;
    }

    return QObject::eventFilter(watchedObject, evt);
  }

private:
  QComboBox* _comboBox{ nullptr };
};
} // namespace oclero::qlementine
