// SPDX-FileCopyrightText: Olivier Cléro <oclero@hotmail.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <QWidget>

#include <oclero/qlementine/utils/BadgeUtils.hpp>

namespace oclero::qlementine {
/// A QWidget that displays a badge indicating Status (Error, etc.).
class StatusBadgeWidget : public QWidget {
  Q_OBJECT

public:
  explicit StatusBadgeWidget(QWidget* parent = nullptr);
  StatusBadgeWidget(StatusBadge badge, QWidget* parent = nullptr);
  StatusBadgeWidget(StatusBadge badge, StatusBadgeSize badgeSize, QWidget* parent = nullptr);

  StatusBadge badge() const;
  Q_SLOT void setBadge(StatusBadge badge);

  StatusBadgeSize badgeSize() const;
  Q_SLOT void setBadgeSize(StatusBadgeSize size);

  QSize sizeHint() const override;

Q_SIGNALS:
  void badgeChanged();
  void badgeSizeChanged();

protected:
  void paintEvent(QPaintEvent* e) override;

private:
  StatusBadgeSize _badgeSize{ StatusBadgeSize::Medium };
  StatusBadge _badge{ StatusBadge::Info };
};
} // namespace oclero::qlementine
