#include "postoolbar.h"
#include "song.h"

#include <QLabel>
#include <QSpacerItem>
#include <QHBoxLayout>


namespace MusEGui {


PosToolbar::PosToolbar(const QString &title, QWidget *parent)
    : QToolBar(title, parent)
{
    setObjectName("Position toolbar");

    QLabel *range = new QLabel(tr("Range"), this);
    addWidget(range);

    markerLeft = new PosEdit(this);
    markerLeft->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
    markerLeft->setFocusPolicy(Qt::NoFocus);
    markerLeft->setToolTip(tr("Left marker"));
    markerLeft->setStatusTip(tr("Left marker position"));
    markerRight = new PosEdit(this);
    markerRight->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
    markerRight->setFocusPolicy(Qt::NoFocus);
    markerRight->setToolTip(tr("Right marker"));
    markerRight->setStatusTip(tr("Right marker position"));

    addWidget(markerLeft);
    addWidget(markerRight);

    QLabel *pos = new QLabel(tr("Pos"), this);
    pos->setIndent(2);
    addWidget(pos);

    time = new PosEdit(this);
    time->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
    time->setFocusPolicy(Qt::NoFocus);
    time->setStatusTip(tr("Current position in bars/beats"));

    timeSmpte = new PosEdit(this);
    timeSmpte->setSmpte(true);
    timeSmpte->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
    timeSmpte->setFocusPolicy(Qt::NoFocus);
    timeSmpte->setToolTip(tr("SMPTE position"));
    timeSmpte->setStatusTip(tr("Current position in SMPTE time"));

    addWidget(time);
    addWidget(timeSmpte);

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

    addSeparator();
    addWidget(slider);

    connect(markerLeft,  SIGNAL(valueChanged(const MusECore::Pos&)), SLOT(lposChanged(const MusECore::Pos&)));
    connect(markerRight, SIGNAL(valueChanged(const MusECore::Pos&)), SLOT(rposChanged(const MusECore::Pos&)));
    connect(time, SIGNAL(valueChanged(const MusECore::Pos&)), SLOT(cposChanged(const MusECore::Pos&)));
    connect(timeSmpte, SIGNAL(valueChanged(const MusECore::Pos&)), SLOT(cposChanged(const MusECore::Pos&)));
    connect(slider,SIGNAL(valueChanged(int)),  SLOT(cposChanged(int)));

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


} // namespace
