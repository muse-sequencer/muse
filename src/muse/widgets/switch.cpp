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

#include "switch.h"

namespace MusEGui {

Animator::Animator(QObject* target, QObject* parent) : QVariantAnimation(parent) {
    setTargetObject(target);
}

Animator::~Animator() {
    stop();
}

QObject* Animator::targetObject() const {
    return target.data();
}

void Animator::setTargetObject(QObject* _target) {
    if (target.data() == _target)
        return;

    if (isRunning()) {
        qWarning("Animation::setTargetObject: you can't change the target of a running animation");
        return;
    }

    target = _target;
}

void Animator::updateCurrentValue(const QVariant& value) {
    Q_UNUSED(value);

    if (!target.isNull()) {
        auto update = QEvent(QEvent::StyleAnimationUpdate);
        update.setAccepted(false);
        QCoreApplication::sendEvent(target.data(), &update);
        if (!update.isAccepted())
            stop();
    }
}

void Animator::updateState(QAbstractAnimation::State newState, QAbstractAnimation::State oldState) {
    if (target.isNull() && oldState == Stopped) {
        qWarning("Animation::updateState: Changing state of an animation without target");
        return;
    }

    QVariantAnimation::updateState(newState, oldState);

    if (!endValue().isValid() && direction() == Forward) {
        qWarning("Animation::updateState (%s): starting an animation without end value", targetObject()->metaObject()->className());
    }
}

void Animator::setup(int duration, QEasingCurve easing) {
    setDuration(duration);
    setEasingCurve(easing);
}

void Animator::interpolate(const QVariant& _start, const QVariant& end) {
    setStartValue(_start);
    setEndValue(end);
    start();
}

void Animator::setCurrentValue(const QVariant& value) {
    setStartValue(value);
    setEndValue(value);
    updateCurrentValue(currentValue());
}



SelectionControl::SelectionControl(QWidget* parent) : QAbstractButton(parent) {
    setObjectName("SelectionControl");
    setCheckable(true);
}

SelectionControl::~SelectionControl() {

}

void SelectionControl::enterEvent(QEvent* e) {
    setCursor(Qt::PointingHandCursor);
    QAbstractButton::enterEvent(e);
}

Qt::CheckState SelectionControl::checkState() const {
    return isChecked() ? Qt::Checked : Qt::Unchecked;
}

void SelectionControl::checkStateSet() {
    const auto state = checkState();
    emit stateChanged(state);
    toggle(state);
}

void SelectionControl::nextCheckState() {
    QAbstractButton::nextCheckState();
    SelectionControl::checkStateSet();
}



void Switch::init(const char* name) {
//     setFont(style.font);
    setObjectName(name);
    //setMouseTracking(true);
    /* setup animations */
    thumbBrushAnimation = new Animator{ this, this };
    trackBrushAnimation = new Animator{ this, this };
    thumbPosAniamtion = new Animator{ this, this };
    thumbPosAniamtion->setup(style.thumbPosAniamtion.duration, style.thumbPosAniamtion.easing);
    trackBrushAnimation->setup(style.trackBrushAnimation.duration, style.trackBrushAnimation.easing);
    thumbBrushAnimation->setup(style.thumbBrushAnimation.duration, style.thumbBrushAnimation.easing);
    /* set init values */
    trackBrushAnimation->setStartValue(colorFromOpacity(style.trackOffBrush, style.trackOffOpacity));
    trackBrushAnimation->setEndValue(colorFromOpacity(style.trackOffBrush, style.trackOffOpacity));
    thumbBrushAnimation->setStartValue(colorFromOpacity(style.thumbOffBrush, style.thumbOffOpacity));
    thumbBrushAnimation->setEndValue(colorFromOpacity(style.thumbOffBrush, style.thumbOffOpacity));
    /* set standard palettes */
    auto p = palette();
    p.setColor(QPalette::Active, QPalette::ButtonText, style.textColor);
    p.setColor(QPalette::Disabled, QPalette::ButtonText, style.textColor);
    setPalette(p);
    setSizePolicy(QSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Fixed));
}

// QRect Switch::indicatorRect() {
//     const auto w = style.indicatorMargin.left() + style.height + style.indicatorMargin.right();
//     return ltr(this) ? QRect(0, 0, w, style.height) : QRect(width() - w, 0, w, style.height);
// }
QRect Switch::indicatorRect() {
//     auto h = fontMetrics().height() + style.indicatorMargin.top() + style.indicatorMargin.bottom();
    const double th = fontMetrics().height() + /*style.indicatorMargin.top() + style.indicatorMargin.bottom()*/
      style.textMargin.top() + style.textMargin.bottom();
    auto h = qMax(th, THUMB_RADIUS * 2) + contentsMargins().top() + contentsMargins().bottom();
    const auto w = style.indicatorMargin.left() + h + style.indicatorMargin.right();
    return ltr(this) ? QRect(0, 0, w, h) : QRect(width() - w, 0, w, h);
}

// QRect Switch::textRect() {
//     const auto w = style.indicatorMargin.left() + style.height + style.indicatorMargin.right();
//     return ltr(this) ? rect().marginsRemoved(QMargins(w, 0, 0, 0)) : rect().marginsRemoved(QMargins(0, 0, w, 0));
// }
QRect Switch::textRect() {
//     auto h = fontMetrics().height() + style.indicatorMargin.top() + style.indicatorMargin.bottom() +
//       style.textMargin.top() + style.textMargin.bottom();
    const double th = fontMetrics().height() + /*style.indicatorMargin.top() + style.indicatorMargin.bottom()*/
      style.textMargin.top() + style.textMargin.bottom();
    auto h = qMax(th, THUMB_RADIUS * 2) + contentsMargins().top() + contentsMargins().bottom();
    const auto w = style.indicatorMargin.left() + h + style.indicatorMargin.right();
    return ltr(this) ? rect().marginsRemoved(QMargins(w, 0, 0, 0)) : rect().marginsRemoved(QMargins(0, 0, w, 0));
}

Switch::Switch(int id, QWidget* parent, const char* name) : SelectionControl(parent), _id(id) {
    init(name);
}

Switch::Switch(int id, const QString& text, QWidget* parent, const char* name) : Switch(id, parent, name) {
    setText(text);
}

Switch::Switch(int id, const QString& text, const QBrush& brush, QWidget* parent, const char* name) : Switch(id, text, parent, name) {
    style.thumbOnBrush = brush.color();
    style.trackOnBrush = brush.color();
}

Switch::~Switch() {

}

// QSize Switch::sizeHint() const {
//     auto h = style.height;
// // Width() is obsolete. Qt >= 5.11 use horizontalAdvance().
// #if QT_VERSION >= 0x050b00
//       auto w = style.indicatorMargin.left() + style.height + style.indicatorMargin.right() + fontMetrics().horizontalAdvance(text());
// #else
//       auto w = style.indicatorMargin.left() + style.height + style.indicatorMargin.right() + fontMetrics().width(text());
// #endif
//
//     return QSize(w, h);
// }

QSize Switch::sizeHint() const {
    const double th = fontMetrics().height() + /*style.indicatorMargin.top() + style.indicatorMargin.bottom()*/
      style.textMargin.top() + style.textMargin.bottom();
    auto h = qMax(th, THUMB_RADIUS * 2) + contentsMargins().top() + contentsMargins().bottom();
// Width() is obsolete. Qt >= 5.11 use horizontalAdvance().
#if QT_VERSION >= 0x050b00
      auto w = style.indicatorMargin.left() + h + style.indicatorMargin.right() + fontMetrics().horizontalAdvance(text());
#else
      auto w = style.indicatorMargin.left() + h + style.indicatorMargin.right() + fontMetrics().width(text());
#endif

    return QSize(w, h);
}

int Switch::id() const       { return _id; }
void Switch::setId(int i)    { _id = i; }

void Switch::paintEvent(QPaintEvent*) {
    /* for desktop usage we do not need Radial reaction */

    QPainter p(this);

    const auto _indicatorRect = indicatorRect();
    const auto _textRect = textRect();
    auto trackMargin = style.indicatorMargin;
    trackMargin.setTop(trackMargin.top() + 2);
    trackMargin.setBottom(trackMargin.bottom() + 2);
    QRectF trackRect = _indicatorRect.marginsRemoved(trackMargin);
//     const auto thumbRadius = (height() - style.indicatorMargin.top() - style.indicatorMargin.bottom()) / 2.0;

    if (isEnabled()) {
        p.setOpacity(1.0);
        p.setPen(Qt::NoPen);
        /* draw track */
        p.setBrush(trackBrushAnimation->currentValue().value<QColor>());
        p.setRenderHint(QPainter::Antialiasing, true);
        p.drawRoundedRect(trackRect, CORNER_RADIUS, CORNER_RADIUS);
        p.setRenderHint(QPainter::Antialiasing, false);
        /* draw thumb */
        trackRect.setX(trackRect.x() - trackMargin.left() - trackMargin.right() - 2 + thumbPosAniamtion->currentValue().toInt());
        auto thumbRect = trackRect;

        if (!shadowPixmap.isNull())
            p.drawPixmap(thumbRect.center() - QPointF(THUMB_RADIUS, THUMB_RADIUS - 1.0), shadowPixmap);
//             p.drawPixmap(thumbRect.center() - QPointF(thumbRadius, thumbRadius - 1.0), shadowPixmap);

        p.setBrush(thumbBrushAnimation->currentValue().value<QColor>());
        p.setRenderHint(QPainter::Antialiasing, true);
        //        qDebug() << thumbRect << thumbPosAniamtion->currentValue();
        p.drawEllipse(thumbRect.center(), THUMB_RADIUS - SHADOW_ELEVATION - 1.0, THUMB_RADIUS - SHADOW_ELEVATION - 1.0);
//         p.drawEllipse(thumbRect.center(), thumbRadius - SHADOW_ELEVATION - 1.0, thumbRadius - SHADOW_ELEVATION - 1.0);
        p.setRenderHint(QPainter::Antialiasing, false);

        /* draw text */
        if (text().isEmpty())
            return;

        p.setOpacity(1.0);
        p.setPen(palette().color(QPalette::Active, QPalette::ButtonText));
        p.setFont(font());
        p.drawText(_textRect, Qt::AlignLeft | Qt::AlignVCenter, text());
    } else {
        p.setOpacity(style.trackDisabledOpacity);
        p.setPen(Qt::NoPen);
        // draw track
        p.setBrush(style.trackDisabled);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.drawRoundedRect(trackRect, CORNER_RADIUS, CORNER_RADIUS);
        p.setRenderHint(QPainter::Antialiasing, false);
        // draw thumb
        p.setOpacity(1.0);
        if (!isChecked())
            trackRect.setX(trackRect.x() - trackMargin.left() - trackMargin.right() - 2);
        else
            trackRect.setX(trackRect.x() + trackMargin.left() + trackMargin.right() + 2);
        auto thumbRect = trackRect;

        if (!shadowPixmap.isNull())
            p.drawPixmap(thumbRect.center() - QPointF(THUMB_RADIUS, THUMB_RADIUS - 1.0), shadowPixmap);
//             p.drawPixmap(thumbRect.center() - QPointF(thumbRadius, thumbRadius - 1.0), shadowPixmap);

        p.setOpacity(1.0);
        p.setBrush(style.thumbDisabled);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.drawEllipse(thumbRect.center(), THUMB_RADIUS - SHADOW_ELEVATION - 1.0, THUMB_RADIUS - SHADOW_ELEVATION - 1.0);
//         p.drawEllipse(thumbRect.center(), thumbRadius - SHADOW_ELEVATION - 1.0, thumbRadius - SHADOW_ELEVATION - 1.0);

        /* draw text */
        if (text().isEmpty())
            return;

        p.setOpacity(style.disabledTextOpacity);
        p.setPen(palette().color(QPalette::Disabled, QPalette::ButtonText));
        p.setFont(font());
        p.drawText(_textRect, Qt::AlignLeft | Qt::AlignVCenter, text());
    }
}

void Switch::resizeEvent(QResizeEvent* e) {
//     const auto thumbRadius = (height() - style.indicatorMargin.top() - style.indicatorMargin.bottom()) / 2.0;
    shadowPixmap = SwitchStyle::drawShadowEllipse(THUMB_RADIUS, SHADOW_ELEVATION, QColor(0, 0, 0, 70));
//     shadowPixmap = SwitchStyle::drawShadowEllipse(thumbRadius, SHADOW_ELEVATION, QColor(0, 0, 0, 70));
    SelectionControl::resizeEvent(e);
}

void Switch::toggle(Qt::CheckState state) {
    if (state == Qt::Checked) {
        const QVariant posEnd = (style.indicatorMargin.left() + style.indicatorMargin.right() + 2) * 2;
        const QVariant thumbEnd = colorFromOpacity(style.thumbOnBrush, style.thumbOnOpacity);
        const QVariant trackEnd = colorFromOpacity(style.trackOnBrush, style.trackOnOpacity);

        if (!isVisible()) {
            thumbPosAniamtion->setCurrentValue(posEnd);
            thumbBrushAnimation->setCurrentValue(thumbEnd);
            trackBrushAnimation->setCurrentValue(trackEnd);
        } else {
            thumbPosAniamtion->interpolate(0, posEnd);
            thumbBrushAnimation->interpolate(colorFromOpacity(style.thumbOffBrush, style.thumbOffOpacity), thumbEnd);
            trackBrushAnimation->interpolate(colorFromOpacity(style.trackOffBrush, style.trackOffOpacity), trackEnd);
        }
    } else { // Qt::Unchecked
        const QVariant posEnd = 0;
        const QVariant thumbEnd = colorFromOpacity(style.thumbOffBrush, style.thumbOffOpacity);
        const QVariant trackEnd = colorFromOpacity(style.trackOffBrush, style.trackOffOpacity);

        if (!isVisible()) {
            thumbPosAniamtion->setCurrentValue(posEnd);
            thumbBrushAnimation->setCurrentValue(thumbEnd);
            trackBrushAnimation->setCurrentValue(trackEnd);
        } else {
            thumbPosAniamtion->interpolate(thumbPosAniamtion->currentValue().toInt(), posEnd);
            thumbBrushAnimation->interpolate(colorFromOpacity(style.thumbOnBrush, style.thumbOnOpacity), thumbEnd);
            trackBrushAnimation->interpolate(colorFromOpacity(style.trackOnBrush, style.trackOnOpacity), trackEnd);
        }
    }
}

void Switch::mousePressEvent(QMouseEvent *e)
{
  if(e->button() == Qt::RightButton)
    emit switchRightClicked(e->globalPos(), _id);
  else
  {
    const QRect r = indicatorRect();
    const int x1 = r.x() + r.width() / 2;
    const int x2 = r.x() + r.width();
    const QPoint p = e->pos();
    if((p.x() < x1 && isChecked()) || (p.x() >= x1 && p.x() < x2 && !isChecked()))
      setChecked(!isChecked());
    emit switchPressed(_id);
  }
}

void Switch::mouseReleaseEvent(QMouseEvent *e)
{
  if(e->button() == Qt::RightButton)
    return;

  emit switchReleased(_id);
}

void Switch::mouseMoveEvent(QMouseEvent *e)
{
  if(e->buttons() & (Qt::LeftButton | Qt::MiddleButton))
  {
    const QRect r = indicatorRect();
    const int x1 = r.x() + r.width() / 2;
    const int x2 = r.x() + r.width();
    const QPoint p = e->pos();
    if((p.x() < x1 && isChecked()) || (p.x() >= x1 && p.x() < x2 && !isChecked()))
    {
      setChecked(!isChecked());
      emit toggleChanged(isChecked(), _id);
    }
  }
}

} // namespace MusEGui
