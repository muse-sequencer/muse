#include <QTimer>

#include "synctoolbar.h"
#include "song.h"
#include "icons.h"
#include "audio.h"
#include "audiodev.h"

#include "gconfig.h"

namespace MusEGui {


SyncToolbar::SyncToolbar(const QString &title, QWidget *parent)
    : QToolBar(title, parent)
{
    setObjectName("Sync toolbar");

    syncAction = new QAction(*externSyncOnSVGIcon, "External sync", this);
    syncAction->setCheckable(true);
    syncAction->setToolTip(tr("External sync on/off"));

    jackTransportAction = new QAction(*jackTransportOnSVGIcon, "Jack Transport", this);
    jackTransportAction->setCheckable(true);
    jackTransportAction->setToolTip(tr("Jack Transport on/off"));

    timebaseMasterAction = new QAction(*timebaseMasterOnSVGIcon, "Timebase master", this);
    timebaseMasterAction->setCheckable(true);
    timebaseMasterAction->setToolTip(
                tr("On: Timebase master\nOff: Not master\nFlash: Waiting. Another client is master. Click to force."));

    syncAction->setChecked(MusEGlobal::extSyncFlag);
    jackTransportAction->setChecked(MusEGlobal::config.useJackTransport);
    timebaseMasterAction->setChecked(MusEGlobal::timebaseMasterState);

    addAction(syncAction);
    addAction(jackTransportAction);
    addAction(timebaseMasterAction);

    blinkTimer = new QTimer(this);
    connect(blinkTimer, SIGNAL(timeout()), SLOT(timebaseBlink()));
    blinkTimer->stop();

    connect(syncAction, SIGNAL(toggled(bool)), SLOT(extSyncClicked(bool)));
    connect(jackTransportAction, SIGNAL(toggled(bool)), SLOT(useJackTransportClicked(bool)));
    connect(timebaseMasterAction, SIGNAL(toggled(bool)), SLOT(timebaseMasterClicked(bool)));

    connect(MusEGlobal::song, SIGNAL(songChanged(MusECore::SongChangedStruct_t)), this, SLOT(songChanged(MusECore::SongChangedStruct_t)));
}

//---------------------------------------------------------
//   extSyncClicked
//---------------------------------------------------------

void SyncToolbar::extSyncClicked(bool v)
{
    MusECore::PendingOperationList operations;
    operations.add(MusECore::PendingOperationItem(&MusEGlobal::extSyncFlag, v, MusECore::PendingOperationItem::SetExternalSyncFlag));
    MusEGlobal::audio->msgExecutePendingOperations(operations, true);
}

//---------------------------------------------------------
//   useJackTransportClicked
//---------------------------------------------------------

void SyncToolbar::useJackTransportClicked(bool v)
{
    if(!v && MusEGlobal::timebaseMasterState && MusEGlobal::audioDevice)
    {
        // Let the operation do this.
        //MusEGlobal::config.timebaseMaster = v;
        // Force it.
        MusEGlobal::audioDevice->setMaster(v, false);
    }

    MusECore::PendingOperationList operations;
    operations.add(MusECore::PendingOperationItem(&MusEGlobal::config.useJackTransport, v, MusECore::PendingOperationItem::SetUseJackTransport));
    MusEGlobal::audio->msgExecutePendingOperations(operations, true);
}

//---------------------------------------------------------
//   timebaseMasterClicked
//---------------------------------------------------------

void SyncToolbar::timebaseMasterClicked(bool v)
{
    //   MusECore::PendingOperationList operations;
    //   operations.add(MusECore::PendingOperationItem(&MusEGlobal::useJackTransport, v, MusECore::PendingOperationItem::SetUseJackTransport));
    //   MusEGlobal::audio->msgExecutePendingOperations(operations, true);
    if (!MusEGlobal::audioDevice)
        return;

    if (MusEGlobal::config.useJackTransport) {
        //if(MusEGlobal::timebaseMasterState != MusEGlobal::config.timebaseMaster)
        MusEGlobal::config.timebaseMaster = v;
        // Force it.
        MusEGlobal::audioDevice->setMaster(v, true);
    }
}

void SyncToolbar::songChanged(MusECore::SongChangedStruct_t flags)
{
    if (flags & SC_EXTERNAL_MIDI_SYNC) {
        QSignalBlocker blocker(syncAction);
        syncAction->setChecked(MusEGlobal::extSyncFlag);
    }

    if (flags & SC_USE_JACK_TRANSPORT) {
        QSignalBlocker blocker(jackTransportAction);
        QSignalBlocker blockerx(timebaseMasterAction);
        jackTransportAction->setEnabled(MusEGlobal::audioDevice && MusEGlobal::audioDevice->hasOwnTransport());
        jackTransportAction->setChecked(MusEGlobal::config.useJackTransport && jackTransportAction->isEnabled());
        timebaseMasterAction->setEnabled(MusEGlobal::audioDevice &&
                                         MusEGlobal::audioDevice->hasOwnTransport() &&
                                         MusEGlobal::audioDevice->hasTimebaseMaster() &&
                                         MusEGlobal::config.useJackTransport);
    }

    if (flags & SC_TIMEBASE_MASTER) {
        QSignalBlocker blocker(timebaseMasterAction);

        const bool has_master = MusEGlobal::audioDevice && MusEGlobal::audioDevice->hasTimebaseMaster();
        if (has_master && MusEGlobal::timebaseMasterState)
        {
            blinkTimer->stop();
            timebaseMasterAction->setChecked(true);
        }
        else if (has_master && MusEGlobal::config.timebaseMaster)
        {
            timebaseMasterAction->setChecked(false);
            if (timebaseMasterAction->isEnabled()) {
                blinkState = false;
                blinkTimer->start(250);
            }
        }
        else
        {
            blinkTimer->stop();
            timebaseMasterAction->setChecked(false);
        }
    }
}

void SyncToolbar::timebaseBlink()
{
    if (!timebaseMasterAction->isEnabled())
        return;

    QSignalBlocker blocker(timebaseMasterAction);
    blinkState = !blinkState;
    timebaseMasterAction->setChecked(blinkState);
}


} // namespace
