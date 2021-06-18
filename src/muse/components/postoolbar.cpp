#include "postoolbar.h"
#include "song.h"
#include "gconfig.h"
#include "audio.h"
#include "icons.h"

#include <QSpacerItem>
#include <QHBoxLayout>
#include <QPainter>


namespace MusEGui {


PosToolbar::PosToolbar(const QString &title, QWidget *parent)
    : QToolBar(title, parent)
{
    setObjectName("Position toolbar");

    QLabel *pixlab = new QLabel(this);
    //    QLabel *range = new QLabel(tr("Range"), this);

    int iconSize = MusEGlobal::config.iconSize;
    qreal dpr = devicePixelRatioF();
    QPixmap pix(iconSize * dpr, iconSize * dpr);
    pix.setDevicePixelRatio(dpr);
    pix.fill( Qt::transparent );

    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(MusEGlobal::config.rangeMarkerColor);
    p.setPen(MusEGlobal::config.rangeMarkerColor);

    qreal pixc = iconSize / 2;
    qreal off = 0;
    //    qreal off = iconSize / 10;
    qreal rad = iconSize / 5;
    p.drawPolygon( QVector<QPointF>{ { pixc + rad, off },
                                     { pixc - rad, off },
                                     { pixc + rad, off + 2 * rad } } );
    p.drawLine(QPointF(pixc + rad, off + 2 * rad), QPointF(pixc + rad, iconSize - 2 * off));

    pixlab->setPixmap(pix);
    addWidget(pixlab);

    markerLeft = new PosEdit(this);
    markerLeft->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
    //    markerLeft->setFocusPolicy(Qt::NoFocus);
    markerLeft->setToolTip(tr("Left marker"));
    markerLeft->setStatusTip(tr("Left marker position"));
    addWidget(markerLeft);


    pixlab = new QLabel(this);
    pix.fill( Qt::transparent );

    p.drawPolygon( QVector<QPointF>{ { pixc - rad, off },
                                     { pixc + rad, off },
                                     { pixc - rad, off + 2 * rad } } );
    p.drawLine(QPointF(pixc - rad, off + 2 * rad), QPointF(pixc - rad, iconSize - 2 * off));

    pixlab->setPixmap(pix);
    pixlab->setContentsMargins(2,0,0,0);
    addWidget(pixlab);

    markerRight = new PosEdit(this);
    markerRight->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
    //    markerRight->setFocusPolicy(Qt::NoFocus);
    markerRight->setToolTip(tr("Right marker"));
    markerRight->setStatusTip(tr("Right marker position"));
    addWidget(markerRight);

    //    QLabel *pos = new QLabel(tr("Pos"), this);
    //    pos->setIndent(2);
    //    addWidget(pos);

    p.setBrush(MusEGlobal::config.positionMarkerColor);
    p.setPen(Qt::NoPen);

    pixlab = new QLabel(this);
    pix.fill( Qt::transparent );

    p.drawPolygon( QVector<QPointF>{ { pixc - 2 * rad, off },
                                     { pixc + 2 * rad, off },
                                     { pixc, off + 2 * rad } } );
    p.setPen(MusEGlobal::config.positionMarkerColor);
    p.drawLine(QPointF(pixc, off + 2 * rad), QPointF(pixc, iconSize - 2 * off));

    pixlab->setPixmap(pix);
    pixlab->setContentsMargins(2,0,0,0);
    addWidget(pixlab);

    time = new PosEdit(this);
    time->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
    //    time->setFocusPolicy(Qt::NoFocus);
    time->setToolTip(tr("Current position"));
    time->setStatusTip(tr("Current position in bars/beats"));
    addWidget(time);

    timeSmpte = new PosEdit(this);
    timeSmpte->setSmpte(true);
    timeSmpte->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
    //    timeSmpte->setFocusPolicy(Qt::NoFocus);
    timeSmpte->setToolTip(tr("SMPTE position"));
    timeSmpte->setStatusTip(tr("Current position in SMPTE time"));
    addWidget(timeSmpte);

    toggleTickFrame = new QAction(*showFieldsSVGIcon, "Toggle ticks/frames");
    toggleTickFrame->setCheckable(true);
    toggleTickFrame->setChecked(false);
    toggleTickFrame->setToolTip(tr("Show/Hide position in ticks and audio frames"));
    toggleTickFrame->setStatusTip(tr("Show/Hide additional output fields displaying the current position in ticks and audio frames"));
    addAction(toggleTickFrame);

    posTicks = new QLabel(this);
    posTicks->setObjectName("PosTicks");
    posTicks->setToolTip(tr("Current position in ticks"));
    posTicks->setStatusTip(tr("Current position in ticks"));
    posTicks->setText("0000000000");
    posTicks->setTextFormat(Qt::PlainText);
    posTicks->setFocusPolicy(Qt::NoFocus);
    posTicks->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    posTicksAction = addWidget(posTicks);
    posTicksAction->setVisible(false);

    posFrames = new QLabel(this);
    posFrames->setObjectName("PosFrames");
    posFrames->setToolTip(tr("Current position in audio frames"));
    posFrames->setStatusTip(tr("Current position in audio frames"));
    posFrames->setText("0000000000");
    posFrames->setTextFormat(Qt::PlainText);
    posFrames->setFocusPolicy(Qt::NoFocus);
    posFrames->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    posFramesAction = addWidget(posFrames);
    posFramesAction->setVisible(false);

    slider = new QSlider;
    slider->setFocusPolicy(Qt::NoFocus);
    slider->setMinimum(0);
    slider->setMaximum(200000);
    slider->setPageStep(1000);
    slider->setValue(0);
    slider->setOrientation(Qt::Horizontal);
    slider->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
    slider->setToolTip(tr("Current position"));
    slider->setStatusTip(tr("Current position slider"));

    //    addSeparator();
    addWidget(slider);

    connect(markerLeft,  SIGNAL(valueChanged(const MusECore::Pos&)), SLOT(lposChanged(const MusECore::Pos&)));
    connect(markerRight, SIGNAL(valueChanged(const MusECore::Pos&)), SLOT(rposChanged(const MusECore::Pos&)));
    connect(time, SIGNAL(valueChanged(const MusECore::Pos&)), SLOT(cposChanged(const MusECore::Pos&)));
    connect(timeSmpte, SIGNAL(valueChanged(const MusECore::Pos&)), SLOT(cposChanged(const MusECore::Pos&)));
    connect(slider,SIGNAL(valueChanged(int)),  SLOT(cposChanged(int)));
    connect(toggleTickFrame, SIGNAL(toggled(bool)), SLOT(showTickFrameToggled(bool)));
    connect(markerLeft, &PosEdit::returnPressed, [this]() { emit returnPressed(); } );
    connect(markerLeft, &PosEdit::escapePressed, [this]() { emit escapePressed(); } );
    connect(markerRight, &PosEdit::returnPressed, [this]() { emit returnPressed(); } );
    connect(markerRight, &PosEdit::escapePressed, [this]() { emit escapePressed(); } );
    connect(time, &PosEdit::returnPressed, [this]() { emit returnPressed(); } );
    connect(time, &PosEdit::escapePressed, [this]() { emit escapePressed(); } );
    connect(timeSmpte, &PosEdit::returnPressed, [this]() { emit returnPressed(); } );
    connect(timeSmpte, &PosEdit::escapePressed, [this]() { emit escapePressed(); } );

    connect(MusEGlobal::song, SIGNAL(posChanged(int, unsigned, bool)), SLOT(setPos(int, unsigned, bool)));
    connect(MusEGlobal::song, SIGNAL(songChanged(MusECore::SongChangedStruct_t)), this, SLOT(songChanged(MusECore::SongChangedStruct_t)));
}

//---------------------------------------------------------
//   cposChanged
//---------------------------------------------------------

void PosToolbar::cposChanged(int tick)
{
    MusEGlobal::song->setPos(MusECore::Song::CPOS, tick);
}

//---------------------------------------------------------
//   cposChanged
//---------------------------------------------------------

void PosToolbar::cposChanged(const MusECore::Pos& pos)
{
    MusEGlobal::song->setPos(MusECore::Song::CPOS, pos.tick());
}

//---------------------------------------------------------
//   lposChanged
//---------------------------------------------------------

void PosToolbar::lposChanged(const MusECore::Pos& pos)
{
    MusEGlobal::song->setPos(MusECore::Song::LPOS, pos.tick());
}

//---------------------------------------------------------
//   rposChanged
//---------------------------------------------------------

void PosToolbar::rposChanged(const MusECore::Pos& pos)
{
    MusEGlobal::song->setPos(MusECore::Song::RPOS, pos.tick());
}

//---------------------------------------------------------
//   setPos
//---------------------------------------------------------

void PosToolbar::setPos(int idx, unsigned v, bool)
{
    switch (idx) {
    case 0:
        time->setValue(v);
        timeSmpte->setValue(v);
        if((unsigned) slider->value() != v) {
            slider->blockSignals(true);
            slider->setValue(v);
            slider->blockSignals(false);
        }

        if (posTicksAction->isVisible()) {
            posTicks->setText(QString::number(v).rightJustified(10, '0'));
            posFrames->setText(QString::number(MusEGlobal::audio->pos().frame()).rightJustified(10, '0'));
        }
        break;
    case 1:
        markerLeft->setValue(v);
        break;
    case 2:
        markerRight->setValue(v);
        break;
    }
}

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void PosToolbar::songChanged(MusECore::SongChangedStruct_t)
{
    slider->setRange(0, MusEGlobal::song->len());
}

void PosToolbar::showTickFrameToggled(bool checked) {
    if (checked) {
        posTicks->setText(QString::number(MusEGlobal::audio->pos().tick()).rightJustified(10, '0'));
        posFrames->setText(QString::number(MusEGlobal::audio->pos().frame()).rightJustified(10, '0'));
        posTicksAction->setVisible(true);
        posFramesAction->setVisible(true);
    } else {
        posTicksAction->setVisible(false);
        posFramesAction->setVisible(false);    }
    updateGeometry();
    update();
}



} // namespace
