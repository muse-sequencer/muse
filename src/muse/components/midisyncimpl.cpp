//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: midisyncimpl.cpp,v 1.1.1.1.2.4 2009/05/03 04:14:01 terminator356 Exp $
//
//  (C) Copyright 1999/2000 Werner Schweer (ws@seh.de)
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; version 2 of
//  the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
//=========================================================

#include <QCloseEvent>
#include <QInputDialog>
#include <QMessageBox>
#include <QStringList>
//#include <QTimer>
#include <QHeaderView>
//#include <QComboBox>
#include <QTreeWidget>

#include "app.h"
#include "song.h"
#include "midiport.h"
#include "midiseq.h"
#include "mididev.h"
#include "icons.h"
#include "globals.h"
#include "gconfig.h"
#include "midisyncimpl.h"
#include "driver/audiodev.h"
#include "audio.h"
#include "al/al.h"
#include "operations.h"

namespace MusEGui {

enum { DEVCOL_NO = 0, DEVCOL_NAME, DEVCOL_SYNC_TO, DEVCOL_IN, DEVCOL_TICKIN, DEVCOL_MRTIN, DEVCOL_MMCIN, DEVCOL_MTCIN, DEVCOL_MTCTYPE, 
       DEVCOL_RID, DEVCOL_RCLK, DEVCOL_RMRT, DEVCOL_RMMC, DEVCOL_RMTC, DEVCOL_RREWSTART, 
       DEVCOL_TID, DEVCOL_TCLK, DEVCOL_TMRT, DEVCOL_TMMC, DEVCOL_TMTC, /* DEVCOL_TREWSTART, */  };

//MusECore::MidiSyncInfo tmpMidiSyncPorts[MIDI_PORTS];

//---------------------------------------------------------
//   MidiSyncConfig::setToolTips
//---------------------------------------------------------

void MidiSyncConfig::setToolTips(QTreeWidgetItem *item)
{
  item->setToolTip(DEVCOL_NO, tr("Port Number"));
  item->setToolTip(DEVCOL_NAME, tr("Name of the midi device associated with"
				   " this port number"));
  item->setToolTip(DEVCOL_SYNC_TO, tr("Sync to this device. Click to select."));
  item->setToolTip(DEVCOL_IN, tr("Midi clock input detected"));
  item->setToolTip(DEVCOL_TICKIN, tr("Midi tick input detected"));
  item->setToolTip(DEVCOL_MRTIN, tr("Midi real time input detected"));
  item->setToolTip(DEVCOL_MMCIN, tr("MMC input detected"));
  item->setToolTip(DEVCOL_MTCIN, tr("MTC input detected"));
  item->setToolTip(DEVCOL_MTCTYPE, tr("Detected SMPTE format"));
  item->setToolTip(DEVCOL_RID, tr("Receive id number. 127 = Global. Double click to edit."));
  item->setToolTip(DEVCOL_RCLK, tr("Accept midi clock input"));
  item->setToolTip(DEVCOL_RMRT, tr("Accept midi real time input"));
  item->setToolTip(DEVCOL_RMMC, tr("Accept MMC input"));
  item->setToolTip(DEVCOL_RMTC, tr("Accept MTC input"));
  item->setToolTip(DEVCOL_RREWSTART, tr("Receive start rewinds before playing"));
  item->setToolTip(DEVCOL_TID, tr("Transmit id number. 127 = Global. Double click to edit."));
  item->setToolTip(DEVCOL_TCLK, tr("Send midi clock output"));
  item->setToolTip(DEVCOL_TMRT, tr("Send midi realtime output"));
  item->setToolTip(DEVCOL_TMMC, tr("Send MMC output"));
  item->setToolTip(DEVCOL_TMTC, tr("Send MTC output"));
  //item->setToolTip(DEVCOL_TREWSTART, tr("Send continue instead of start"));
}

//---------------------------------------------------------
//   MidiSyncConfig::setWhatsThis
//---------------------------------------------------------

void MidiSyncConfig::setWhatsThis(QTreeWidgetItem *item)
{
  item->setWhatsThis(DEVCOL_NO, tr("Port Number"));
  item->setWhatsThis(DEVCOL_NAME, tr("Name of the midi device associated with this port number"));
  item->setWhatsThis(DEVCOL_SYNC_TO, tr("Sync to this device. Click to select.\n"
				   "MusE will synchronize to this device's\n clock or MTC quarter-frame,\n if 'slave to external sync' is on."));
  item->setWhatsThis(DEVCOL_IN, tr("Midi clock input detected"));
  item->setWhatsThis(DEVCOL_TICKIN, tr("Midi tick input detected"));
  item->setWhatsThis(DEVCOL_MRTIN, tr("Midi realtime input detected, including\n start/stop/continue, and song position."));
  item->setWhatsThis(DEVCOL_MMCIN, tr("MMC input detected, including stop/play/deferred play, and locate."));
                                      //"Current port actually used is red. Click to force a port to be current."));
  item->setWhatsThis(DEVCOL_MTCIN, tr("MTC input detected, including forward quarter-frame sync and full-frame locate"));
  item->setWhatsThis(DEVCOL_MTCTYPE, tr("Detected SMPTE format: 24fps, 25fps, 30fps drop frame, or 30fps non-drop\n"
					"Detects format of MTC quarter and full frame, and MMC locate."));
  item->setWhatsThis(DEVCOL_RID, tr("Receive id number. 127 = global receive all, even if not global."));
  item->setWhatsThis(DEVCOL_RCLK, tr("Accept midi clock input.\nOnly one port can be used for clock sync."));
  item->setWhatsThis(DEVCOL_RMRT, tr("Accept midi realtime input, including\n start/stop/continue, and song position.\n"
				     "Non-clock events (start,stop etc) are\n accepted by ALL enabled ports.\n"
				     "This means you may have several master\n devices connected, and muse will accept\n"
				     " input from them."));
  item->setWhatsThis(DEVCOL_RMMC, tr("Accept MMC input, including stop/play/deferred play, and locate."));
  item->setWhatsThis(DEVCOL_RMTC, tr("Accept MTC input, including forward quarter-frame sync and full-frame locate.\n"
				     "Only one port can be used for sync."));
  item->setWhatsThis(DEVCOL_RREWSTART, tr("When start is received, rewind before playing.\n"
					  "Note: In some cases, such as having many\n project audio tracks, it may be impossible\n to rewind fast"
					  " enough to synchronize\n with the external device.\nManually rewinding the device before\n playing is recommended."));
  item->setWhatsThis(DEVCOL_TID, tr("Transmit id number. 127 = global transmit to all."));
  item->setWhatsThis(DEVCOL_TCLK, tr("Send midi clock output. If 'Slave to External Sync' is chosen,\n"
				     " muse can re-transmit clock to any other chosen ports."));
  item->setWhatsThis(DEVCOL_TMRT, tr("Send midi realtime output, including start/stop/continue,\n"
				     " and song position. If 'Slave to external sync' is chosen,\n"
				     " muse can re-transmit midi realtime input messages to any\n"
				     " other chosen ports. This means you may have several slave\n"
				     " devices connected, and muse can re-send realtime messages\n"
				     " to any or all of them."));
  item->setWhatsThis(DEVCOL_TMMC, tr("Send MMC output"));
  item->setWhatsThis(DEVCOL_TMTC, tr("Send MTC output"));
  //      item->setWhatsThis(DEVCOL_TREWSTART, tr("When transport is starting, send continue instead of start.\n"));
}

//---------------------------------------------------------
//   MidiSyncConfig::addDevice
//---------------------------------------------------------

void MidiSyncConfig::addDevice(QTreeWidgetItem *item, QTreeWidget *tree)
{
  setWhatsThis(item);
  tree->addTopLevelItem(item);
}

void MidiSyncLViewItem::setPort(int port)
{ 
  _port = port; 
  if(_port < 0 || port >= MusECore::MIDI_PORTS)
    return;
    
  copyFromSyncInfo(MusEGlobal::midiPorts[port].syncInfo());
}

//---------------------------------------------------------
//   MidiSyncLViewItem
//    copyFromSyncInfo
//---------------------------------------------------------

void MidiSyncLViewItem::copyFromSyncInfo(const MusECore::MidiSyncInfo &sp)
{
  _idOut         = sp.idOut();
  _idIn          = sp.idIn();
  _sendMC        = sp.MCOut();
  _sendMRT       = sp.MRTOut();
  _sendMMC       = sp.MMCOut();
  _sendMTC       = sp.MTCOut();
  _recMC         = sp.MCIn();
  _recMRT        = sp.MRTIn();
  _recMMC        = sp.MMCIn();
  _recMTC        = sp.MTCIn();
  _recRewOnStart = sp.recRewOnStart();
  //_sendContNotStart = sp.sendContNotStart();
}

//---------------------------------------------------------
//   MidiSyncLViewItem
//    copyToSyncInfo
//---------------------------------------------------------

void MidiSyncLViewItem::copyToSyncInfo(MusECore::MidiSyncInfo &sp)
{
  sp.setIdOut(_idOut);
  sp.setIdIn(_idIn);
  sp.setMCOut(_sendMC);
  sp.setMRTOut(_sendMRT);
  sp.setMMCOut(_sendMMC);
  sp.setMTCOut(_sendMTC);
  sp.setMCIn(_recMC);
  sp.setMRTIn(_recMRT);
  sp.setMMCIn(_recMMC);
  sp.setMTCIn(_recMTC);
  sp.setRecRewOnStart(_recRewOnStart);
  //sp.setSendContNotStart(_sendContNotStart);
}

//---------------------------------------------------------
//   MidiSyncConfig
//    Midi Sync Config
//---------------------------------------------------------

MidiSyncConfig::MidiSyncConfig(QWidget* parent)
  : QDialog(parent)
{
      setupUi(this);
      
      // Remember the current value, for cancelling.
      _curMidiSyncInPort = MusEGlobal::config.curMidiSyncInPort;
      
      _dirty = false;
      applyButton->setEnabled(false);
      okButton->setEnabled(false);
      
      devicesListView->setAllColumnsShowFocus(true);
      QStringList columnnames;
      columnnames << tr("Port")
		  << tr("Device Name")
          << "Sync-to"
          << "Clock"
          << "Tick"
          << "RT"
          << "MMC"
          << "MTC"
          << tr("Type")
          << "RID" // Receive
          << "Clock in" // Receive
          << "RT in" // Receive
          << "MMC in" // Receive
          << "MTC in" // Receive
          << "Rewind" // Receive
          << "TID" // Transmit
          << "Clock out" // Transmit
          << "RT out" // Transmit
          << "MMC out" // Transmit
          << "MTC out"; // Transmit
	
      devicesListView->setColumnCount(columnnames.size());
      devicesListView->setHeaderLabels(columnnames);
      setWhatsThis(devicesListView->headerItem());
      setToolTips(devicesListView->headerItem());
      devicesListView->setFocusPolicy(Qt::NoFocus);

      syncRecFilterPreset->addItem(tr("None"), MusECore::MidiSyncInfo::NONE);
      syncRecFilterPreset->addItem(tr("Tiny"), MusECore::MidiSyncInfo::TINY);
      syncRecFilterPreset->addItem(tr("Small"), MusECore::MidiSyncInfo::SMALL);
      syncRecFilterPreset->addItem(tr("Large"), MusECore::MidiSyncInfo::LARGE);
      syncRecFilterPreset->addItem(tr("Large with pre-detect"), MusECore::MidiSyncInfo::LARGE_WITH_PRE_DETECT);
      
      songChanged(-1);
      
    //connect(devicesListView, SIGNAL(pressed(QListViewItem*,const QPoint&,int)),
      //   this, SLOT(dlvClicked(QListViewItem*,const QPoint&,int)));
      connect(devicesListView, SIGNAL(itemClicked(QTreeWidgetItem*, int)),
         this, SLOT(dlvClicked(QTreeWidgetItem*, int)));
      connect(devicesListView, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)),
         this, SLOT(dlvDoubleClicked(QTreeWidgetItem*, int)));
      //connect(devicesListView, SIGNAL(itemRenamed(QListViewItem*, int, const QString&)),
      //   this, SLOT(renameOk(QListViewItem*, int, const QString&)));
      
      connect(okButton, SIGNAL(clicked()), SLOT(ok()));
      connect(applyButton, SIGNAL(clicked()), SLOT(apply()));
      connect(cancelButton, SIGNAL(clicked()), SLOT(cancel()));

      //connect(syncMode, SIGNAL(clicked(int)), SLOT(syncChanged(int)));
      connect(extSyncCheckbox, SIGNAL(clicked()), SLOT(syncChanged()));
      connect(mtcSyncType, SIGNAL(activated(int)), SLOT(syncChanged()));
      connect(syncRecFilterPreset, SIGNAL(currentIndexChanged(int)), SLOT(syncChanged()));
      connect(syncRecTempoValQuant, SIGNAL(valueChanged(double)), SLOT(syncChanged()));
      connect(syncDelaySpinBox, SIGNAL(valueChanged(int)), SLOT(syncChanged()));

      connect(extSyncCheckbox, SIGNAL(toggled(bool)), SLOT(extSyncClicked(bool)));
  
      // Done in show().
      //connect(MusEGlobal::song, SIGNAL(songChanged(MusECore::SongChangedStruct_t)), SLOT(songChanged(MusECore::SongChangedStruct_t)));
      //connect(MusEGlobal::heartBeatTimer, SIGNAL(timeout()), SLOT(heartBeat()));
}

MidiSyncConfig::~MidiSyncConfig()
{
}

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void MidiSyncConfig::songChanged(MusECore::SongChangedStruct_t flags)
{
      // Is it simply a midi controller value adjustment? Forget it. Otherwise, it's mainly midi port/device changes we want.
      if(!(flags & (SC_CONFIG | SC_MASTER | SC_TEMPO | SC_SIG | SC_TRACK_INSERTED | SC_TRACK_REMOVED | SC_TRACK_MODIFIED |
                    SC_PART_INSERTED | SC_PART_REMOVED | SC_PART_MODIFIED | SC_EVENT_INSERTED | SC_EVENT_REMOVED | SC_EVENT_MODIFIED |
                    SC_MIDI_CONTROLLER_ADD | SC_EXTERNAL_MIDI_SYNC | SC_USE_JACK_TRANSPORT)))
        return;
    
      // Reset dirty flag, since we're loading new values.
      _dirty = false;
      if(applyButton->isEnabled())
        applyButton->setEnabled(false);
      if(okButton->isEnabled())
        okButton->setEnabled(false);
      
      //for(int i = 0; i < MIDI_PORTS; ++i)
      //  tmpMidiSyncPorts[i] = midiSyncPorts[i];
      
      extSyncCheckbox->blockSignals(true);
      syncDelaySpinBox->blockSignals(true);
      extSyncCheckbox->setChecked(MusEGlobal::extSyncFlag);
      syncDelaySpinBox->setValue(MusEGlobal::syncSendFirstClockDelay);
      syncDelaySpinBox->blockSignals(false);
      extSyncCheckbox->blockSignals(false);

      int fp_idx = syncRecFilterPreset->findData(MusEGlobal::syncRecFilterPreset);
      if(fp_idx != -1)
      {
        syncRecFilterPreset->blockSignals(true);
        syncRecFilterPreset->setCurrentIndex(fp_idx);
        syncRecFilterPreset->blockSignals(false);
      }
      syncRecTempoValQuant->blockSignals(true);
      syncRecTempoValQuant->setValue(MusEGlobal::syncRecTempoValQuant);
      syncRecTempoValQuant->blockSignals(false);
      
      mtcSyncType->setCurrentIndex(MusEGlobal::mtcType);

      mtcOffH->blockSignals(true);
      mtcOffM->blockSignals(true);
      mtcOffS->blockSignals(true);
      mtcOffF->blockSignals(true);
      mtcOffSf->blockSignals(true);
      mtcOffH->setValue(MusEGlobal::mtcOffset.h());
      mtcOffM->setValue(MusEGlobal::mtcOffset.m());
      mtcOffS->setValue(MusEGlobal::mtcOffset.s());
      mtcOffF->setValue(MusEGlobal::mtcOffset.f());
      mtcOffSf->setValue(MusEGlobal::mtcOffset.sf());
      mtcOffH->blockSignals(false);
      mtcOffM->blockSignals(false);
      mtcOffS->blockSignals(false);
      mtcOffF->blockSignals(false);
      mtcOffSf->blockSignals(false);

      updateSyncInfoLV();
      
      //selectionChanged();
}

//---------------------------------------------------------
//   heartBeat
//---------------------------------------------------------

void MidiSyncConfig::heartBeat()
{
      //inHeartBeat = true;
//  for (int i = MusECore::MIDI_PORTS-1; i >= 0; --i)
  for (int i = 0; i < devicesListView->topLevelItemCount(); ++i)
    {
      MidiSyncLViewItem* lvi = (MidiSyncLViewItem*)devicesListView->topLevelItem(i);
      int port = lvi->port();
      if(port >= 0 && port < MusECore::MIDI_PORTS)
        {
          bool sdet = MusEGlobal::midiPorts[port].syncInfo().MCSyncDetect();
          if(sdet)
          {
            if(!lvi->_inDet)
            {
              lvi->_inDet = true;
              lvi->_curDet = false;
              lvi->setIcon(DEVCOL_IN, *ledGreenSVGIcon);
            }  
          }
          else
          {
            if(lvi->_curDet || lvi->_inDet)
            {
              lvi->_curDet = false;
              lvi->_inDet = false;
              lvi->setIcon(DEVCOL_IN, *ledOffSVGIcon);
            }  
          }
          
          if(port == MusEGlobal::config.curMidiSyncInPort)
              lvi->setCheckState(DEVCOL_SYNC_TO, Qt::Checked);
//            lvi->setIcon(DEVCOL_SYNC_TO, *ledRedSVGIcon);
          else
              lvi->setCheckState(DEVCOL_SYNC_TO, Qt::Unchecked);
//            lvi->setIcon(DEVCOL_SYNC_TO, *ledOffSVGIcon);
          
          sdet = MusEGlobal::midiPorts[port].syncInfo().tickDetect();
          if(sdet)
          {
            if(!lvi->_tickDet)
            {
              lvi->_tickDet = true;
              lvi->setIcon(DEVCOL_TICKIN, *ledGreenSVGIcon);
            }  
          } 
          else
          {
            if(lvi->_tickDet)
            {
              lvi->_tickDet = false;
              lvi->setIcon(DEVCOL_TICKIN, *ledOffSVGIcon);
            }  
          }
        
          sdet = MusEGlobal::midiPorts[port].syncInfo().MRTDetect();
          if(sdet)
          {
            if(!lvi->_MRTDet)
            {
              lvi->_MRTDet = true;
              lvi->setIcon(DEVCOL_MRTIN, *ledGreenSVGIcon);
            }  
          } 
          else
          {
            if(lvi->_MRTDet)
            {
              lvi->_MRTDet = false;
              lvi->setIcon(DEVCOL_MRTIN, *ledOffSVGIcon);
            }  
          }
        
          int type = MusEGlobal::midiPorts[port].syncInfo().recMTCtype();
          sdet = MusEGlobal::midiPorts[port].syncInfo().MMCDetect();
          bool mtcdet = MusEGlobal::midiPorts[port].syncInfo().MTCDetect();
          if(sdet)
          {
            if(!lvi->_MMCDet)
            {
              lvi->_MMCDet = true;
              lvi->setIcon(DEVCOL_MMCIN, *ledGreenSVGIcon);
            }
            // MMC locate command can contain SMPTE format type. Update now.
            if(!mtcdet && lvi->_recMTCtype != type)
            {
              lvi->_recMTCtype = type;
              switch(type)
              {
                case 0:
                  lvi->setText(DEVCOL_MTCTYPE, "24");
                break;  
                case 1:
                  lvi->setText(DEVCOL_MTCTYPE, "25");
                break;  
                case 2:
                  lvi->setText(DEVCOL_MTCTYPE, "30D");
                break;  
                case 3:
                  lvi->setText(DEVCOL_MTCTYPE, "30N");
                break;  
                default:
                  lvi->setText(DEVCOL_MTCTYPE, "??");
                break;  
              }  
            }
          } 
          else
          {
            if(lvi->_MMCDet)
            {
              lvi->_MMCDet = false;
              lvi->setIcon(DEVCOL_MMCIN, *ledOffSVGIcon);
            }  
          }
          
          if(mtcdet)
          {
            if(!lvi->_MTCDet)
            {
              lvi->_MTCDet = true;
              lvi->_curMTCDet = false;
              lvi->setIcon(DEVCOL_MTCIN, *ledGreenSVGIcon);
            }
            
            if(lvi->_recMTCtype != type)
            {
              lvi->_recMTCtype = type;
              switch(type)
              {
                case 0:
                  lvi->setText(DEVCOL_MTCTYPE, "24");
                break;  
                case 1:
                  lvi->setText(DEVCOL_MTCTYPE, "25");
                break;  
                case 2:
                  lvi->setText(DEVCOL_MTCTYPE, "30D");
                break;  
                case 3:
                  lvi->setText(DEVCOL_MTCTYPE, "30N");
                break;  
                default:
                  lvi->setText(DEVCOL_MTCTYPE, "??");
                break;  
              }  
            }
          } 
          else
          {
            if(lvi->_curMTCDet || lvi->_MTCDet)
            {
              lvi->_MTCDet = false;
              lvi->_curMTCDet = false;
              lvi->setIcon(DEVCOL_MTCIN, *ledOffSVGIcon);
            }  
          }
        }
        
        //MusECore::MidiDevice* dev = lvi->device();
        //bool sdet = dev->syncInfo().MCSyncDetect();
        //if(lvi->pixmap(DEVCOL_IN) != (sdet ? *ledGreenSVGIcon : *ledOffSVGIcon)
        //  lvi->setIcon(DEVCOL_IN, QIcon( sdet ? *ledGreenSVGIcon : *ledOffSVGIcon);
        
      }
            
      //inHeartBeat = false;
}

//---------------------------------------------------------
//   syncChanged
//    val = 1  -  Master Mode
//          0  -  Slave Mode
//---------------------------------------------------------

void MidiSyncConfig::syncChanged()
      {
      setDirty();
      }

//---------------------------------------------------------
//   extSyncChanged
//---------------------------------------------------------

void MidiSyncConfig::extSyncChanged(bool v)
      {
      extSyncCheckbox->blockSignals(true);
      extSyncCheckbox->setChecked(v);
//      if(v)
//        MusEGlobal::song->setMasterFlag(false);
      extSyncCheckbox->blockSignals(false);
      }

//---------------------------------------------------------
//   extSyncClicked
//---------------------------------------------------------

void MidiSyncConfig::extSyncClicked(bool v)
{
  MusECore::PendingOperationList operations;
  operations.add(MusECore::PendingOperationItem(&MusEGlobal::extSyncFlag, v, MusECore::PendingOperationItem::SetExternalSyncFlag));
  MusEGlobal::audio->msgExecutePendingOperations(operations, true);
}

//---------------------------------------------------------
//   ok Pressed
//---------------------------------------------------------

void MidiSyncConfig::ok()
      {
      apply();
      _dirty = false;
      if(applyButton->isEnabled())
        applyButton->setEnabled(false);
      if(okButton->isEnabled())
        okButton->setEnabled(false);
      
      close();
      }

//---------------------------------------------------------
//   cancel Pressed
//---------------------------------------------------------

void MidiSyncConfig::cancel()
      {
      // Restore the current sync port.
      MusEGlobal::config.curMidiSyncInPort = _curMidiSyncInPort;
      
      _dirty = false;
      if(applyButton->isEnabled())
        applyButton->setEnabled(false);
      if(okButton->isEnabled())
        okButton->setEnabled(false);
      
      close();
      }

//---------------------------------------------------------
//   show
//---------------------------------------------------------

void MidiSyncConfig::show()
{
  songChanged(-1);
  connect(MusEGlobal::song, SIGNAL(songChanged(MusECore::SongChangedStruct_t)), SLOT(songChanged(MusECore::SongChangedStruct_t)));
  connect(MusEGlobal::heartBeatTimer, SIGNAL(timeout()), SLOT(heartBeat()));
  QDialog::show();
}

//---------------------------------------------------------
//   closeEvent
//---------------------------------------------------------

void MidiSyncConfig::closeEvent(QCloseEvent* e)
      {
      if(_dirty)
      {
        QMessageBox::StandardButton n = QMessageBox::warning(this, tr("MusE"),
         tr("Settings have changed\n"
         "Apply sync settings?"),
         QMessageBox::Apply | QMessageBox::No | QMessageBox::Abort,
         QMessageBox::Apply);
         
        if(n == QMessageBox::Abort)
        {
          e->ignore();
          return; 
        }  
        
        if(n == QMessageBox::Apply)
          apply();
      }
      
      disconnect(MusEGlobal::heartBeatTimer, SIGNAL(timeout()), this, SLOT(heartBeat()));
      disconnect(MusEGlobal::song, SIGNAL(songChanged(MusECore::SongChangedStruct_t)), this, SLOT(songChanged(MusECore::SongChangedStruct_t)));
      
      e->accept();
      }

//---------------------------------------------------------
//   apply Pressed
//---------------------------------------------------------

void MidiSyncConfig::apply()
{
      // Protect all structures.
      if(MusEGlobal::audio && MusEGlobal::audio->isRunning())
        MusEGlobal::audio->msgIdle(true);

      MusEGlobal::syncSendFirstClockDelay = syncDelaySpinBox->value();
      
      MusEGlobal::mtcType     = mtcSyncType->currentIndex();
      // Make sure the AL namespace variables mirror our variables.
      AL::mtcType = MusEGlobal::mtcType;
      MusEGlobal::extSyncFlag = extSyncCheckbox->isChecked();

      if(syncRecFilterPreset->currentIndex() != -1)
      {
        int fp_idx = syncRecFilterPreset->itemData(syncRecFilterPreset->currentIndex()).toInt();
        if(fp_idx >= 0 && fp_idx < MusECore::MidiSyncInfo::TYPE_END)
        {
          MusEGlobal::syncRecFilterPreset = MusECore::MidiSyncInfo::SyncRecFilterPresetType(fp_idx);
          MusEGlobal::midiSyncContainer.setSyncRecFilterPreset(MusEGlobal::syncRecFilterPreset);
        }
      }
      MusEGlobal::syncRecTempoValQuant = syncRecTempoValQuant->value();
      MusEGlobal::midiSyncContainer.setRecTempoValQuant(MusEGlobal::syncRecTempoValQuant);

      MusEGlobal::mtcOffset.setH(mtcOffH->value());
      MusEGlobal::mtcOffset.setM(mtcOffM->value());
      MusEGlobal::mtcOffset.setS(mtcOffS->value());
      MusEGlobal::mtcOffset.setF(mtcOffF->value());
      MusEGlobal::mtcOffset.setSf(mtcOffSf->value());

//      for (int i = MusECore::MIDI_PORTS-1; i >= 0; --i)
      for (int i = 0; i < devicesListView->topLevelItemCount(); ++i)
      {
          MidiSyncLViewItem* lvi = (MidiSyncLViewItem*)devicesListView->topLevelItem(i);
          int port = lvi->port();
          if(port >= 0 && port < MusECore::MIDI_PORTS)
              lvi->copyToSyncInfo(MusEGlobal::midiPorts[port].syncInfo());
      }

  // Update the current value.
  _curMidiSyncInPort = MusEGlobal::config.curMidiSyncInPort;
      
  if(MusEGlobal::audio && MusEGlobal::audio->isRunning())
    MusEGlobal::audio->msgIdle(false);
  
  // Save settings. Use simple version - do NOT set style or stylesheet, this has nothing to do with that.
  //muse->changeConfig(true);

  MusEGlobal::song->update(SC_EXTERNAL_MIDI_SYNC | SC_USE_JACK_TRANSPORT);

  _dirty = false;
  if(applyButton->isEnabled())
    applyButton->setEnabled(false);
  if(okButton->isEnabled())
    okButton->setEnabled(false);
  
  // Do not call this. Causes freeze sometimes. Only will be needed if extra pollfds are used by midi seq thread.
  //midiSeq->msgUpdatePollFd();
}

//---------------------------------------------------------
//   updateSyncInfoLV
//---------------------------------------------------------

void MidiSyncConfig::updateSyncInfoLV()
      {
      devicesListView->clear();
      for(int i = 0; i < MusECore::MIDI_PORTS; ++i) 
      {
            MusECore::MidiPort* port  = &MusEGlobal::midiPorts[i];
            MusECore::MidiDevice* dev = port->device();
            // Don't show if it is a synthesizer device.
            // Hmm, some synths might support transport commands or even sync?
            // If anything, the DSSI or VST synths just might... 
            // TODO: Must test to see if it screws any of them up, especially clock out.
            // Also, if we do this, we must prevent such messages from reaching
            //  those ports at several other places in the code.
            //if(dev && dev->isSynti())
            //  continue;

            if (!dev)
                continue;
              
            QString s;
            s.setNum(i+1);
            MidiSyncLViewItem* lvi = new MidiSyncLViewItem(devicesListView);
            lvi->setPort(i); // setPort will copy parameters.
            MusECore::MidiSyncInfo& portsi = port->syncInfo();
            
            lvi->setText(DEVCOL_NO, s);
            
            if (dev) 
                  lvi->setText(DEVCOL_NAME, dev->name());
            else 
                  lvi->setText(DEVCOL_NAME, tr("<none>"));
            
            if(portsi.MCSyncDetect())
            {
              {
                lvi->_curDet = false;
                lvi->_inDet = true;
                lvi->setIcon(DEVCOL_IN, *ledGreenSVGIcon);
              }
            }
            else
            {
              lvi->_curDet = false;
              lvi->_inDet = false;
              lvi->setIcon(DEVCOL_IN, *ledOffSVGIcon);
            }
            
            if(i == MusEGlobal::config.curMidiSyncInPort)
              lvi->setCheckState(DEVCOL_SYNC_TO, Qt::Checked);
//            lvi->setIcon(DEVCOL_SYNC_TO, *ledRedSVGIcon);
            else
              lvi->setCheckState(DEVCOL_SYNC_TO, Qt::Unchecked);
//            lvi->setIcon(DEVCOL_SYNC_TO, *ledOffSVGIcon);
            
            if(portsi.tickDetect())
            {
              lvi->_tickDet = true;
              lvi->setIcon(DEVCOL_TICKIN, *ledGreenSVGIcon);
            }
            else
            {
              lvi->_tickDet = false;
              lvi->setIcon(DEVCOL_TICKIN, *ledOffSVGIcon);
            }

            if(portsi.MRTDetect())
            {
              lvi->_MRTDet = true;
              lvi->setIcon(DEVCOL_MRTIN, *ledGreenSVGIcon);
            }
            else
            {
              lvi->_MRTDet = false;
              lvi->setIcon(DEVCOL_MRTIN, *ledOffSVGIcon);
            }

            if(portsi.MMCDetect())
            {
              lvi->_MMCDet = true;
              lvi->setIcon(DEVCOL_MMCIN, *ledGreenSVGIcon);
              // MMC locate command can have SMPTE format bits...
              if(lvi->_recMTCtype != portsi.recMTCtype())
              {
                switch(portsi.recMTCtype())
                {
                  case 0:
                    lvi->setText(DEVCOL_MTCTYPE, "24");
                  break;  
                  case 1:
                    lvi->setText(DEVCOL_MTCTYPE, "25");
                  break;  
                  case 2:
                    lvi->setText(DEVCOL_MTCTYPE, "30D");
                  break;  
                  case 3:
                    lvi->setText(DEVCOL_MTCTYPE, "30N");
                  break;  
                  default:
                    lvi->setText(DEVCOL_MTCTYPE, "??");
                  break;  
                }  
              }  
            }
            else
            {
              lvi->_MMCDet = false;
              lvi->setIcon(DEVCOL_MMCIN, *ledOffSVGIcon);
            }

            if(portsi.MTCDetect())
            {
              {
                lvi->_curMTCDet = false;
                lvi->_MTCDet = true;
                lvi->setIcon(DEVCOL_MTCIN, *ledGreenSVGIcon);
              }
                
              if(lvi->_recMTCtype != portsi.recMTCtype())
              {
                switch(portsi.recMTCtype())
                {
                  case 0:
                    lvi->setText(DEVCOL_MTCTYPE, "24");
                  break;  
                  case 1:
                    lvi->setText(DEVCOL_MTCTYPE, "25");
                  break;  
                  case 2:
                    lvi->setText(DEVCOL_MTCTYPE, "30D");
                  break;  
                  case 3:
                    lvi->setText(DEVCOL_MTCTYPE, "30N");
                  break;  
                  default:
                    lvi->setText(DEVCOL_MTCTYPE, "??");
                  break;  
                }  
              }  
            }
            else
            {
              lvi->_curMTCDet = false;
              lvi->_MTCDet = false;
              lvi->setIcon(DEVCOL_MTCIN, *ledOffSVGIcon);
              //lvi->setText(DEVCOL_MTCTYPE, "--");
            }

            lvi->setText(DEVCOL_RID,    QString().setNum(lvi->_idIn) );
            lvi->setCheckState(DEVCOL_RCLK, lvi->_recMC ? Qt::Checked : Qt::Unchecked);
            lvi->setCheckState(DEVCOL_RMRT, lvi->_recMRT ? Qt::Checked : Qt::Unchecked);
            lvi->setCheckState(DEVCOL_RMMC, lvi->_recMMC ? Qt::Checked : Qt::Unchecked);
            lvi->setCheckState(DEVCOL_RMTC, lvi->_recMTC ? Qt::Checked : Qt::Unchecked);
            lvi->setCheckState(DEVCOL_RREWSTART, lvi->_recRewOnStart ? Qt::Checked : Qt::Unchecked);
//            lvi->setIcon(DEVCOL_RCLK, lvi->_recMC ? *ledGreenSVGIcon : *ledOffSVGIcon);
//            lvi->setIcon(DEVCOL_RMRT, lvi->_recMRT ? *ledGreenSVGIcon : *ledOffSVGIcon);
//            lvi->setIcon(DEVCOL_RMMC, lvi->_recMMC ? *ledGreenSVGIcon : *ledOffSVGIcon);
//            lvi->setIcon(DEVCOL_RMTC, lvi->_recMTC ? *ledGreenSVGIcon : *ledOffSVGIcon);
//            lvi->setIcon(DEVCOL_RREWSTART, lvi->_recRewOnStart ? *ledGreenSVGIcon : *ledOffSVGIcon);
            
            lvi->setText(DEVCOL_TID,          QString().setNum(lvi->_idOut) );
            lvi->setCheckState(DEVCOL_TCLK, lvi->_sendMC ? Qt::Checked : Qt::Unchecked);
            lvi->setCheckState(DEVCOL_TMRT, lvi->_sendMRT ? Qt::Checked : Qt::Unchecked);
            lvi->setCheckState(DEVCOL_TMMC, lvi->_sendMMC ? Qt::Checked : Qt::Unchecked);
            lvi->setCheckState(DEVCOL_TMTC, lvi->_sendMTC ? Qt::Checked : Qt::Unchecked);
//            lvi->setIcon(DEVCOL_TCLK, lvi->_sendMC ? *ledGreenSVGIcon : *ledOffSVGIcon);
//            lvi->setIcon(DEVCOL_TMRT, lvi->_sendMRT ? *ledGreenSVGIcon : *ledOffSVGIcon);
//            lvi->setIcon(DEVCOL_TMMC, lvi->_sendMMC ? *ledGreenSVGIcon : *ledOffSVGIcon);
//            lvi->setIcon(DEVCOL_TMTC, lvi->_sendMTC ? *ledGreenSVGIcon : *ledOffSVGIcon);
            //lvi->setIcon(DEVCOL_TREWSTART, QIcon(  lvi->_sendContNotStart ? *ledGreenSVGIcon : *ledOffSVGIcon);
            
            addDevice(lvi, devicesListView);
      }
	    devicesListView->resizeColumnToContents(DEVCOL_NO);
	    //devicesListView->resizeColumnToContents(DEVCOL_NAME);
	    devicesListView->header()->resizeSection(DEVCOL_NAME, 120);
	    devicesListView->resizeColumnToContents(DEVCOL_SYNC_TO);
	    devicesListView->resizeColumnToContents(DEVCOL_IN);
	    devicesListView->resizeColumnToContents(DEVCOL_TICKIN);
	    devicesListView->resizeColumnToContents(DEVCOL_MRTIN);
	    devicesListView->resizeColumnToContents(DEVCOL_MMCIN);
	    devicesListView->resizeColumnToContents(DEVCOL_MTCIN);
	    devicesListView->resizeColumnToContents(DEVCOL_MTCTYPE);
	    devicesListView->resizeColumnToContents(DEVCOL_RID);	    
	    devicesListView->resizeColumnToContents(DEVCOL_RCLK);
	    devicesListView->resizeColumnToContents(DEVCOL_RMRT);
	    devicesListView->resizeColumnToContents(DEVCOL_RMMC);
	    devicesListView->resizeColumnToContents(DEVCOL_RMTC);
	    devicesListView->resizeColumnToContents(DEVCOL_RREWSTART);
	    devicesListView->resizeColumnToContents(DEVCOL_TID);
	    devicesListView->resizeColumnToContents(DEVCOL_TCLK);
	    devicesListView->resizeColumnToContents(DEVCOL_TMRT);
	    devicesListView->resizeColumnToContents(DEVCOL_TMMC);
	    devicesListView->resizeColumnToContents(DEVCOL_TMTC);

     devicesListView->header()->setSectionResizeMode(DEVCOL_NO, QHeaderView::Fixed);
     devicesListView->header()->setSectionResizeMode(DEVCOL_SYNC_TO, QHeaderView::Fixed);
     devicesListView->header()->setSectionResizeMode(DEVCOL_IN, QHeaderView::Fixed);
     devicesListView->header()->setSectionResizeMode(DEVCOL_TICKIN, QHeaderView::Fixed);
     devicesListView->header()->setSectionResizeMode(DEVCOL_MRTIN, QHeaderView::Fixed);
     devicesListView->header()->setSectionResizeMode(DEVCOL_MMCIN, QHeaderView::Fixed);
     devicesListView->header()->setSectionResizeMode(DEVCOL_MTCIN, QHeaderView::Fixed);
     devicesListView->header()->setSectionResizeMode(DEVCOL_RCLK, QHeaderView::Fixed);
     devicesListView->header()->setSectionResizeMode(DEVCOL_RMRT, QHeaderView::Fixed);
     devicesListView->header()->setSectionResizeMode(DEVCOL_RMMC, QHeaderView::Fixed);
     devicesListView->header()->setSectionResizeMode(DEVCOL_RMTC, QHeaderView::Fixed);
     devicesListView->header()->setSectionResizeMode(DEVCOL_RMTC, QHeaderView::Fixed);
     devicesListView->header()->setSectionResizeMode(DEVCOL_RREWSTART, QHeaderView::Fixed);
     devicesListView->header()->setSectionResizeMode(DEVCOL_TCLK, QHeaderView::Fixed);
     devicesListView->header()->setSectionResizeMode(DEVCOL_TMRT, QHeaderView::Fixed);
     devicesListView->header()->setSectionResizeMode(DEVCOL_TMMC, QHeaderView::Fixed);

      }


//---------------------------------------------------------
//   dlvClicked
//---------------------------------------------------------

void MidiSyncConfig::dlvClicked(QTreeWidgetItem* item, int col)
{
      if (item == nullptr)
            return;
      
      MidiSyncLViewItem* lvi = (MidiSyncLViewItem*)item;
      int no = lvi->port();
      if (no < 0 || no >= MusECore::MIDI_PORTS)
        return;
      //MusECore::MidiDevice* dev = lvi->device();
      // Does the device really exist?
      //if(midiDevices.find(dev) == midiDevices.end())
      //  return;
      
      switch (col) 
      {
            case DEVCOL_NO:
                  break;
            case DEVCOL_NAME:
                  break;
            case DEVCOL_SYNC_TO:
                  if(no != MusEGlobal::config.curMidiSyncInPort)
                  {
                    // Turn off the current sync port's light.
                    MidiSyncLViewItem* prev_lvi = 
                      (MidiSyncLViewItem*)devicesListView->topLevelItem(MusEGlobal::config.curMidiSyncInPort);
                    if(prev_lvi)
                      prev_lvi->setCheckState(DEVCOL_SYNC_TO, Qt::Unchecked);
//                    prev_lvi->setIcon(DEVCOL_SYNC_TO, *ledOffSVGIcon);
                    
                    // Set the current sync port and turn on the port's light.
                    MusEGlobal::config.curMidiSyncInPort = no;
                    lvi->setCheckState(DEVCOL_SYNC_TO, Qt::Checked);
//                    lvi->setIcon(DEVCOL_SYNC_TO, *ledRedSVGIcon);
                    setDirty();
                  }  
                  break;
            case DEVCOL_IN:
                  break;
            case DEVCOL_TICKIN:
                  break;
            case DEVCOL_MMCIN:
                  break;
            case DEVCOL_MTCIN:
                  break;
            case DEVCOL_MTCTYPE:
                  break;
            case DEVCOL_RID:
                  break;
            case DEVCOL_RCLK:
                  lvi->_recMC = (lvi->_recMC ? false : true);
                  lvi->setCheckState(DEVCOL_RCLK, lvi->_recMC ? Qt::Checked : Qt::Unchecked);
                  setDirty();
                  break;
            case DEVCOL_RMRT:
                  lvi->_recMRT = (lvi->_recMRT ? false : true);
                  lvi->setCheckState(DEVCOL_RMRT, lvi->_recMRT ? Qt::Checked : Qt::Unchecked);
                  setDirty();
                  break;
            case DEVCOL_RMMC:
                  lvi->_recMMC = (lvi->_recMMC ? false : true);
                  lvi->setCheckState(DEVCOL_RMMC, lvi->_recMMC ? Qt::Checked : Qt::Unchecked);
                  setDirty();
                  break;
            case DEVCOL_RMTC:
                  lvi->_recMTC = (lvi->_recMTC ? false : true);
                  lvi->setCheckState(DEVCOL_RMTC, lvi->_recMTC ? Qt::Checked : Qt::Unchecked);
                  setDirty();
                  break;
            case DEVCOL_RREWSTART:
                  lvi->_recRewOnStart = (lvi->_recRewOnStart ? false : true);
                  lvi->setCheckState(DEVCOL_RREWSTART, lvi->_recRewOnStart ? Qt::Checked : Qt::Unchecked);
                  setDirty();
                  break;
            case DEVCOL_TID:
                  break;
            case DEVCOL_TCLK:
                  lvi->_sendMC = (lvi->_sendMC ? false : true);
                  lvi->setCheckState(DEVCOL_TCLK, lvi->_sendMC ? Qt::Checked : Qt::Unchecked);
                  setDirty();
                  break;
            case DEVCOL_TMRT:
                  lvi->_sendMRT = (lvi->_sendMRT ? false : true);
                  lvi->setCheckState(DEVCOL_TMRT, lvi->_sendMRT ? Qt::Checked : Qt::Unchecked);
                  setDirty();
                  break;
            case DEVCOL_TMMC:
                  lvi->_sendMMC = (lvi->_sendMMC ? false : true);
                  lvi->setCheckState(DEVCOL_TMMC, lvi->_sendMMC ? Qt::Checked : Qt::Unchecked);
                  setDirty();
                  break;
            case DEVCOL_TMTC:
                  lvi->_sendMTC = (lvi->_sendMTC ? false : true);
                  lvi->setCheckState(DEVCOL_TMTC, lvi->_sendMTC ? Qt::Checked : Qt::Unchecked);
                  setDirty();
                  break;
            //case DEVCOL_TREWSTART:
            //      lvi->_sendContNotStart = (lvi->_sendContNotStart ? false : true);
            //      lvi->setIcon(DEVCOL_TREWSTART, QIcon( lvi->_sendContNotStart ? *ledGreenSVGIcon : *ledOffSVGIcon);
            //      setDirty();
            //      break;
      }
      //songChanged(-1);
}

//---------------------------------------------------------
//   dlvDoubleClicked
//---------------------------------------------------------

void MidiSyncConfig::dlvDoubleClicked(QTreeWidgetItem* item, int col)
{
      if(!item)
        return;
      
      MidiSyncLViewItem* lvi = (MidiSyncLViewItem*)item;
      
      bool ok = false;
      if(col == DEVCOL_RID)
      {
        int val = lvi->_idIn;
        int newval = QInputDialog::getInt(this, "Muse: Sync info" , "Enter new id number (127 = all):", val, 0, 127, 1, &ok);
        if(ok)
        {
          lvi->_idIn = newval;
          lvi->setText(DEVCOL_RID, QString().setNum(newval)); 
        }  
      }  
      else
      if(col == DEVCOL_TID)
      {
        int val = lvi->_idOut;
        int newval = QInputDialog::getInt(this, "Muse: Sync info" , "Enter new id number (127 = global):", val, 0, 127, 1, &ok);
        if(ok)
        {
          lvi->_idOut = newval;
          lvi->setText(DEVCOL_TID, QString().setNum(newval)); 
        }  
      }  
      
      if(ok)
        setDirty();
}

//---------------------------------------------------------
//   MidiSyncConfig::setDirty
//---------------------------------------------------------

void MidiSyncConfig::setDirty()
{
  _dirty = true;
  if(!applyButton->isEnabled())
    applyButton->setEnabled(true);
  if(!okButton->isEnabled())
    okButton->setEnabled(true);
}

} // namespace MusEGui
