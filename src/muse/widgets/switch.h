/*
 * This is nearly complete Material design Switch widget implementation in qtwidgets module.
 * More info: https://material.io/design/components/selection-controls.html#switches
 * Copyright (C) 2018-2020 Iman Ahmadvand
 *
 * This is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * It is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
*/

/*
 * Modified for MusE.
 */

#ifndef SWITCH_H
#define SWITCH_H

#include <QtWidgets>
#include "switch_style.h"

namespace MusEGui {

class Animator final : public QVariantAnimation {
    Q_OBJECT
    Q_PROPERTY(QObject* targetObject READ targetObject WRITE setTargetObject)

  public:
    Animator(QObject* target, QObject* parent = nullptr);
    ~Animator() override;

    QObject* targetObject() const;
    void setTargetObject(QObject* target);

    inline bool isRunning() const {
        return state() == Running;
    }

  public slots:
    void setup(int duration, QEasingCurve easing = QEasingCurve::Linear);
    void interpolate(const QVariant& start, const QVariant& end);
    void setCurrentValue(const QVariant&);

  protected:
    void updateCurrentValue(const QVariant& value) override final;
    void updateState(QAbstractAnimation::State newState, QAbstractAnimation::State oldState) override final;

  private:
    QPointer<QObject> target;
};

class SelectionControl : public QAbstractButton {
    Q_OBJECT

  public:
    explicit SelectionControl(QWidget* parent = nullptr);
    ~SelectionControl() override;

    Qt::CheckState checkState() const;

  Q_SIGNALS:
    void stateChanged(int);

  protected:
    void enterEvent(QEnterEvent*) override;
    void checkStateSet() override;
    void nextCheckState() override;
    virtual void toggle(Qt::CheckState state) = 0;
};

class Switch final : public SelectionControl {
    Q_OBJECT

    static constexpr auto CORNER_RADIUS = 3.0;
//     static constexpr auto THUMB_RADIUS = 14.5;
    static constexpr auto THUMB_RADIUS = 10.0;
    static constexpr auto SHADOW_ELEVATION = 2.0;

    Q_PROPERTY( int id READ id WRITE setId )
    int _id;

  Q_SIGNALS:
    void toggleChanged(bool, int);
    void switchPressed(int);
    void switchReleased(int);
    void switchRightClicked(const QPoint &, int);

  public:
    explicit Switch(int id, QWidget* parent = nullptr, const char* name = 0);
    Switch(int id, const QString& text, QWidget* parent = nullptr, const char* name = 0);
    Switch(int id, const QString& text, const QBrush&, QWidget* parent = nullptr, const char* name = 0);
    ~Switch() override;

    QSize sizeHint() const override final;

    int id() const;
    void setId(int i);

  protected:
    void paintEvent(QPaintEvent*) override final;
    void resizeEvent(QResizeEvent*) override final;
    void toggle(Qt::CheckState) override final;
    void mousePressEvent(QMouseEvent *e) override final;
    void mouseReleaseEvent(QMouseEvent *e) override final;
    void mouseMoveEvent(QMouseEvent *e) override final;

    void init(const char* name = 0);
    QRect indicatorRect();
    QRect textRect();

    static inline QColor colorFromOpacity(const QColor& c, qreal opacity) {
        return QColor(c.red(), c.green(), c.blue(), qRound(opacity * 255.0));
    }
    static inline bool ltr(QWidget* w) {
        if (nullptr != w)
            return w->layoutDirection() == Qt::LeftToRight;

        return false;
    }

  private:
    SwitchStyle::Switch style;
    QPixmap shadowPixmap;
    QPointer<Animator> thumbBrushAnimation;
    QPointer<Animator> trackBrushAnimation;
    QPointer<Animator> thumbPosAniamtion;
};

} // namespace MusEGui

#endif // SWITCH_H
