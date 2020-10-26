#include "rectoolbar.h"
#include "song.h"

#include <QComboBox>


namespace MusEGui {


RecToolbar::RecToolbar(const QString &title, QWidget *parent)
    : QToolBar(title, parent)
{
    setObjectName("Recording toolbar");

    recMode = new QComboBox;
    recMode->setFocusPolicy(Qt::NoFocus);
    recMode->setToolTip(tr("Record mode"));
    recMode->setStatusTip(tr("Record mode: Overdub to add new events, Replace to replace overlapping events."));
    recMode->insertItem(MusECore::Song::REC_OVERDUP, tr("Overdub"));
    recMode->insertItem(MusECore::Song::REC_REPLACE, tr("Replace"));
    recMode->setCurrentIndex(MusEGlobal::song->recMode());
    connect(recMode, SIGNAL(activated(int)), SLOT(setRecMode(int)));
    connect(MusEGlobal::song, SIGNAL(recModeChanged(int)), SLOT(setRecMode(int)));

    cycleMode = new QComboBox;
    cycleMode->setFocusPolicy(Qt::NoFocus);
    cycleMode->setToolTip(tr("Cycle record mode"));
    cycleMode->setStatusTip(tr("Cycle record mode: Normal to replace range when loop is finished, Mix to add new events, Replace to replace range on first MIDI input."));
    cycleMode->insertItem(MusECore::Song::CYCLE_NORMAL,  tr("Normal"));
    cycleMode->insertItem(MusECore::Song::CYCLE_MIX,     tr("Mix"));
    cycleMode->insertItem(MusECore::Song::CYCLE_REPLACE, tr("Replace"));
    cycleMode->setCurrentIndex(MusEGlobal::song->cycleMode());
    connect(cycleMode, SIGNAL(activated(int)), SLOT(setCycleMode(int)));
    connect(MusEGlobal::song, SIGNAL(cycleModeChanged(int)), SLOT(setCycleMode(int)));

    addWidget(recMode);
    addWidget(cycleMode);
}

//---------------------------------------------------------
//   toggleRecMode
//---------------------------------------------------------

void RecToolbar::setRecMode(int id)
{
    if (MusEGlobal::song->recMode() != id)
        MusEGlobal::song->setRecMode(id);

    if (recMode->currentIndex() != id)
        recMode->setCurrentIndex(id);
}

//---------------------------------------------------------
//   toggleCycleMode
//---------------------------------------------------------

void RecToolbar::setCycleMode(int id)
{
    if (MusEGlobal::song->cycleMode() != id)
        MusEGlobal::song->setCycleMode(id);

    if (cycleMode->currentIndex() != id)
        cycleMode->setCurrentIndex(id);
}

} // namespace
