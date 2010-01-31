//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: midisyncimpl.cpp,v 1.1.1.1.2.4 2009/05/03 04:14:01 terminator356 Exp $
//
//  (C) Copyright 1999/2000 Werner Schweer (ws@seh.de)
//=========================================================

//#include "midisyncimpl.h"
//#include "sync.h"
//#include "globals.h"

#include <qspinbox.h>
#include <qcombobox.h>
#include <qcheckbox.h>
#include <qbuttongroup.h>
#include <qpushbutton.h>
#include <qinputdialog.h>
//#include <qlistview.h>
#include <qheader.h>
#include <qtimer.h>
//#include <qwhatsthis.h>
#include <qmessagebox.h>

#include "app.h"
#include "song.h"
#include "midiport.h"
#include "midiseq.h"
#include "mididev.h"
#include "icons.h"
#include "sync.h"
#include "globals.h"
#include "midisyncimpl.h"
#include "driver/audiodev.h"
#include "audio.h"

enum { DEVCOL_NO = 0, DEVCOL_NAME, DEVCOL_IN, DEVCOL_TICKIN, DEVCOL_MMCIN, DEVCOL_MTCIN, DEVCOL_MTCTYPE, 
       DEVCOL_RID, DEVCOL_RCLK, DEVCOL_RMMC, DEVCOL_RMTC, DEVCOL_RREWSTART, 
       DEVCOL_TID, DEVCOL_TCLK, DEVCOL_TMMC, DEVCOL_TMTC, /* DEVCOL_TREWSTART, */  };

//MidiSyncInfo tmpMidiSyncPorts[MIDI_PORTS];

//---------------------------------------------------------
//   MSyncHeaderTip::maybeTip
//---------------------------------------------------------

void MSyncHeaderTip::maybeTip(const QPoint &pos)
      {
      QHeader* w  = (QHeader*)parentWidget();
      int section = w->sectionAt(pos.x());
      if (section == -1)
            return;
      QRect r(w->sectionPos(section), 0, w->sectionSize(section),
         w->height());
      QString p;
      switch (section) {
            case DEVCOL_NO:       p = QHeader::tr("Port Number"); break;
            case DEVCOL_NAME:     p = QHeader::tr("Name of the midi device associated with"
                       " this port number"); break;
            case DEVCOL_IN:       p = QHeader::tr("Midi realtime input detected"); break;
            case DEVCOL_TICKIN:   p = QHeader::tr("Midi tick input detected"); break;
            case DEVCOL_MMCIN:    p = QHeader::tr("MMC input detected"); break;
            case DEVCOL_MTCIN:    p = QHeader::tr("MTC input detected"); break;
            case DEVCOL_MTCTYPE:  p = QHeader::tr("Detected SMPTE format"); break;
            case DEVCOL_RID:      p = QHeader::tr("Receive id number. 127 = Global. Double click to edit."); break;
            case DEVCOL_RCLK:     p = QHeader::tr("Accept midi realtime input"); break;
            case DEVCOL_RMMC:     p = QHeader::tr("Accept MMC input"); break;
            case DEVCOL_RMTC:     p = QHeader::tr("Accept MTC input"); break;
            case DEVCOL_RREWSTART: p = QHeader::tr("Receiving start rewinds before playing"); break;
            case DEVCOL_TID:      p = QHeader::tr("Transmit id number. 127 = Global. Double click to edit."); break;
            case DEVCOL_TCLK:     p = QHeader::tr("Send midi realtime output"); break;
            case DEVCOL_TMMC:     p = QHeader::tr("Send MMC output"); break;
            case DEVCOL_TMTC:     p = QHeader::tr("Send MTC output"); break;
            //case DEVCOL_TREWSTART: p = QHeader::tr("Send continue instead of start"); break;
            default: return;
            }
      tip(r, p);
      }

//---------------------------------------------------------
//   MSyncWhatsThis::text
//---------------------------------------------------------

QString MSyncWhatsThis::text(const QPoint& pos)
{
      int n = header->cellAt(pos.x());
      if (n == -1)
            return QString::null;
      switch (header->mapToLogical(n)) {
            case DEVCOL_NO:
                  return QHeader::tr("Port Number");
            case DEVCOL_NAME:
                  return QHeader::tr("Name of the midi device associated with this port number");
            case DEVCOL_IN:
                  return QHeader::tr("Midi realtime input detected, including clock/start/stop/continue, and song position.\n"
                                     "Current port actually used is red. Click to force a port to be current.");
            case DEVCOL_TICKIN:
                  return QHeader::tr("Midi tick input detected");
            case DEVCOL_MMCIN:
                  return QHeader::tr("MMC input detected, including stop/play/deferred play, and locate.");
                                     //"Current port actually used is red. Click to force a port to be current.");
            case DEVCOL_MTCIN:
                  return QHeader::tr("MTC input detected, including forward quarter-frame sync and full-frame locate.\n"
                                     "Current port actually used is red. Click to force a port to be current.");
            case DEVCOL_MTCTYPE:
                  return QHeader::tr("Detected SMPTE format: 24fps, 25fps, 30fps drop frame, or 30fps non-drop\n"
                                     "Detects format of MTC quarter and full frame, and MMC locate.");
            case DEVCOL_RID:
                  return QHeader::tr("Receive id number. 127 = global receive all, even if not global.");
            case DEVCOL_RCLK:
                  return QHeader::tr("Accept midi realtime input, including clock/start/stop/continue, and song position.\n"
                                     "Only one input is used for clock. Auto-acquire: If two or more port realtime inputs\n"
                                     "are enabled, the first clock detected is used, until clock is lost, then another\n"
                                     "can take over. Non-clock events (start,stop etc) are accepted by ALL enabled ports.\n"
                                     "This means you may have several master devices connected, and muse will accept input\n"
                                     "from any, including one clock (best if each turns off its clock at stop, so muse can\n"
                                     "re-acquire the clock from another port. Click on detect indicator to force another.)");
            case DEVCOL_RMMC:
                  return QHeader::tr("Accept MMC input, including stop/play/deferred play, and locate.");
            case DEVCOL_RMTC:
                  return QHeader::tr("Accept MTC input, including forward quarter-frame sync and full-frame locate.\n"
                                     "See rmc column for more help.");
            case DEVCOL_RREWSTART:
                  return QHeader::tr("When start is received, rewind before playing. It may be impossible\n"
                                     " to rewind fast enough to synchronize with the external device.");
            case DEVCOL_TID:
                  return QHeader::tr("Transmit id number. 127 = global transmit to all.");
            case DEVCOL_TCLK:
                  return QHeader::tr("Send midi realtime output, including clock/start/stop/continue, and song position. "
                                     "If 'Slave to External Sync' is chosen, muse can re-transmit midi realtime input "
                                     " messages to any other chosen ports. This means you may have several slave devices "
                                     "connected, and muse can re-send realtime messages to any or all of them. ");
            case DEVCOL_TMMC:
                  return QHeader::tr("Send MMC output");
            case DEVCOL_TMTC:
                  return QHeader::tr("Send MTC output");
            //case DEVCOL_TREWSTART:
            //      return QHeader::tr("When transport is starting, send continue instead of start.\n");
            default:
                  break;
            }
      return QString::null;
}

/*
//---------------------------------------------------------
//   MidiSyncLViewItem
//    setDevice
//---------------------------------------------------------

void MidiSyncLViewItem::setDevice(MidiDevice* d)
{ 
  _device = d; 
  if(_device)
    _syncInfo.copyParams(_device->syncInfo());
}
*/

//---------------------------------------------------------
//   MidiSyncLViewItem
//    setPort
//---------------------------------------------------------

void MidiSyncLViewItem::setPort(int port)
{ 
  _port = port; 
  if(_port < 0 || port > MIDI_PORTS)
    return;
    
  //_syncInfo.copyParams(midiPorts[port].syncInfo());
  copyFromSyncInfo(midiPorts[port].syncInfo());
}

//---------------------------------------------------------
//   MidiSyncLViewItem
//    copyFromSyncInfo
//---------------------------------------------------------

void MidiSyncLViewItem::copyFromSyncInfo(const MidiSyncInfo &sp)
{
  _idOut         = sp.idOut();
  _idIn          = sp.idIn();
  _sendMC        = sp.MCOut();
  _sendMMC       = sp.MMCOut();
  _sendMTC       = sp.MTCOut();
  _recMC         = sp.MCIn();
  _recMMC        = sp.MMCIn();
  _recMTC        = sp.MTCIn();
  _recRewOnStart = sp.recRewOnStart();
  //_sendContNotStart = sp.sendContNotStart();
}

//---------------------------------------------------------
//   MidiSyncLViewItem
//    copyToSyncInfo
//---------------------------------------------------------

void MidiSyncLViewItem::copyToSyncInfo(MidiSyncInfo &sp)
{
  sp.setIdOut(_idOut);
  sp.setIdIn(_idIn);
  sp.setMCOut(_sendMC);
  sp.setMMCOut(_sendMMC);
  sp.setMTCOut(_sendMTC);
  sp.setMCIn(_recMC);
  sp.setMMCIn(_recMMC);
  sp.setMTCIn(_recMTC);
  sp.setRecRewOnStart(_recRewOnStart);
  //sp.setSendContNotStart(_sendContNotStart);
}

//---------------------------------------------------------
//   MidiSyncConfig
//    Midi Sync Config
//---------------------------------------------------------

MidiSyncConfig::MidiSyncConfig(QWidget* parent, const char* name)
  : MidiSyncConfigBase(parent, name)
{
      _synctooltip = 0;
      
      _dirty = false;
      applyButton->setEnabled(false);
      
      //inHeartBeat = true;
      
      //for(int i = 0; i < MIDI_PORTS; ++i)
      //  tmpMidiSyncPorts[i] = midiSyncPorts[i];
        
      //bool ext = extSyncFlag.value();
      //syncMode->setButton(int(ext));
      //syncChanged(ext);
//      extSyncCheckbox->setChecked(extSyncFlag.value());

//      dstDevId->setValue(txDeviceId);
//      srcDevId->setValue(rxDeviceId);
//      srcSyncPort->setValue(rxSyncPort + 1);
//      dstSyncPort->setValue(txSyncPort + 1);

//      mtcSync->setChecked(genMTCSync);
//      mcSync->setChecked(genMCSync);
//      midiMachineControl->setChecked(genMMC);

//      acceptMTCCheckbox->setChecked(acceptMTC);
      //acceptMTCCheckbox->setChecked(false);
//      acceptMCCheckbox->setChecked(acceptMC);
//      acceptMMCCheckbox->setChecked(acceptMMC);

//      mtcSyncType->setCurrentItem(mtcType);

//      mtcOffH->setValue(mtcOffset.h());
//      mtcOffM->setValue(mtcOffset.m());
//      mtcOffS->setValue(mtcOffset.s());
//      mtcOffF->setValue(mtcOffset.f());
//      mtcOffSf->setValue(mtcOffset.sf());

      
      
      
      devicesListView->setSorting(-1);
      devicesListView->setAllColumnsShowFocus(true);
      devicesListView->addColumn(tr("Port"));
      devicesListView->addColumn(tr("Device Name"), 120);
      devicesListView->addColumn(tr("i"));
      devicesListView->addColumn(tr("t"));
      devicesListView->addColumn(tr("m"));
      devicesListView->addColumn(tr("c"));
      devicesListView->addColumn(tr("type"));
      devicesListView->addColumn(tr("rid"));  // Receive
      devicesListView->addColumn(tr("rmc")); // Receive
      devicesListView->addColumn(tr("rmmc")); // Receive
      devicesListView->addColumn(tr("rmtc")); // Receive
      devicesListView->addColumn(tr("rrs")); // Receive
      devicesListView->addColumn(tr("tid"));  // Transmit
      devicesListView->addColumn(tr("tmc")); // Transmit
      devicesListView->addColumn(tr("tmmc")); // Transmit
      devicesListView->addColumn(tr("tmtc")); // Transmit
      //devicesListView->addColumn(tr("trs")); // Transmit
      devicesListView->setFocusPolicy(NoFocus);

      devicesListView->setColumnAlignment(DEVCOL_NO, AlignHCenter);
      devicesListView->setColumnAlignment(DEVCOL_IN, AlignCenter);
      devicesListView->setColumnAlignment(DEVCOL_TICKIN, AlignCenter);
      devicesListView->setColumnAlignment(DEVCOL_MMCIN, AlignCenter);
      devicesListView->setColumnAlignment(DEVCOL_MTCIN, AlignCenter);
      //devicesListView->setColumnAlignment(DEVCOL_MTCTYPE, AlignCenter);
      //devicesListView->setColumnAlignment(DEVCOL_RID, AlignCenter);
      devicesListView->setColumnAlignment(DEVCOL_RCLK, AlignCenter);
      devicesListView->setColumnAlignment(DEVCOL_RMMC, AlignCenter);
      devicesListView->setColumnAlignment(DEVCOL_RMTC, AlignCenter);
      devicesListView->setColumnAlignment(DEVCOL_RREWSTART, AlignCenter);
      //devicesListView->setColumnAlignment(DEVCOL_TID, AlignCenter);
      devicesListView->setColumnAlignment(DEVCOL_TCLK, AlignCenter);
      devicesListView->setColumnAlignment(DEVCOL_TMMC, AlignCenter);
      devicesListView->setColumnAlignment(DEVCOL_TMTC, AlignCenter);
      //devicesListView->setColumnAlignment(DEVCOL_TREWSTART, AlignCenter);
      devicesListView->header()->setResizeEnabled(false, DEVCOL_NO);
      devicesListView->header()->setResizeEnabled(false, DEVCOL_IN);
      devicesListView->header()->setResizeEnabled(false, DEVCOL_TICKIN);
      devicesListView->header()->setResizeEnabled(false, DEVCOL_MTCIN);
      devicesListView->header()->setResizeEnabled(false, DEVCOL_RCLK);
      devicesListView->header()->setResizeEnabled(false, DEVCOL_RMMC);
      devicesListView->header()->setResizeEnabled(false, DEVCOL_RMTC);
      devicesListView->header()->setResizeEnabled(false, DEVCOL_RMTC);
      devicesListView->header()->setResizeEnabled(false, DEVCOL_RREWSTART);
      devicesListView->header()->setResizeEnabled(false, DEVCOL_TCLK);
      devicesListView->header()->setResizeEnabled(false, DEVCOL_TMMC);
      //devicesListView->header()->setResizeEnabled(false, DEVCOL_TREWSTART);
      //devicesListView->setResizeMode(QListView::LastColumn);
      devicesListView->setResizeMode(QListView::NoColumn);


      new MSyncWhatsThis(devicesListView, devicesListView->header());
      _synctooltip = new MSyncHeaderTip(devicesListView->header());
      //MSyncHeaderTip::add(devicesListView->header(), QString("Midi sync ports"));

//      updateSyncInfoLV();
      
      songChanged(-1);
      
    //connect(devicesListView, SIGNAL(pressed(QListViewItem*,const QPoint&,int)),
      //   this, SLOT(dlvClicked(QListViewItem*,const QPoint&,int)));
      connect(devicesListView, SIGNAL(mouseButtonClicked(int, QListViewItem*,const QPoint&, int)),
         this, SLOT(dlvClicked(int, QListViewItem*,const QPoint&, int)));
      connect(devicesListView, SIGNAL(doubleClicked(QListViewItem*,const QPoint&,int)),
         this, SLOT(dlvDoubleClicked(QListViewItem*,const QPoint&,int)));
      //connect(devicesListView, SIGNAL(itemRenamed(QListViewItem*, int, const QString&)),
      //   this, SLOT(renameOk(QListViewItem*, int, const QString&)));
      
      connect(okButton, SIGNAL(clicked()), SLOT(ok()));
      connect(applyButton, SIGNAL(clicked()), SLOT(apply()));
      connect(cancelButton, SIGNAL(clicked()), SLOT(cancel()));

      //connect(syncMode, SIGNAL(clicked(int)), SLOT(syncChanged(int)));
      connect(extSyncCheckbox, SIGNAL(clicked()), SLOT(syncChanged()));
      connect(mtcSyncType, SIGNAL(activated(int)), SLOT(syncChanged()));
      connect(useJackTransportCheckbox, SIGNAL(clicked()), SLOT(syncChanged()));
      connect(jackTransportMasterCheckbox, SIGNAL(clicked()), SLOT(syncChanged()));
      connect(&extSyncFlag, SIGNAL(valueChanged(bool)), SLOT(extSyncChanged(bool)));
      connect(syncDelaySpinBox, SIGNAL(valueChanged(int)), SLOT(syncChanged()));
  
      // Done in show().
      //connect(song, SIGNAL(songChanged(int)), SLOT(songChanged(int)));
      //connect(heartBeatTimer, SIGNAL(timeout()), SLOT(heartBeat()));
      
      //inHeartBeat = false;
}

MidiSyncConfig::~MidiSyncConfig()
{
  delete _synctooltip;
}

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void MidiSyncConfig::songChanged(int flags)
{
      // Is it simply a midi controller value adjustment? Forget it. Otherwise, it's mainly midi port/device changes we want.
      if(flags == SC_MIDI_CONTROLLER || 
         !(flags & (SC_CONFIG | SC_MASTER | SC_TEMPO | SC_SIG | SC_TRACK_INSERTED | SC_TRACK_REMOVED | SC_TRACK_MODIFIED |
                    SC_PART_INSERTED | SC_PART_REMOVED | SC_PART_MODIFIED | SC_EVENT_INSERTED | SC_EVENT_REMOVED | SC_EVENT_MODIFIED |
                    SC_MIDI_CONTROLLER_ADD)))
        return;
    
      // Reset dirty flag, since we're loading new values.
      _dirty = false;
      if(applyButton->isEnabled())
        applyButton->setEnabled(false);
      
      //for(int i = 0; i < MIDI_PORTS; ++i)
      //  tmpMidiSyncPorts[i] = midiSyncPorts[i];
      
      extSyncCheckbox->blockSignals(true);
      useJackTransportCheckbox->blockSignals(true);
      jackTransportMasterCheckbox->blockSignals(true);
      syncDelaySpinBox->blockSignals(true);
      extSyncCheckbox->setChecked(extSyncFlag.value());
      useJackTransportCheckbox->setChecked(useJackTransport);
      jackTransportMasterCheckbox->setChecked(jackTransportMaster);
      //jackTransportMasterCheckbox->setEnabled(useJackTransport);
      syncDelaySpinBox->setValue(syncSendFirstClockDelay);
      syncDelaySpinBox->blockSignals(false);
      jackTransportMasterCheckbox->blockSignals(false);
      useJackTransportCheckbox->blockSignals(false);
      extSyncCheckbox->blockSignals(false);
      
      mtcSyncType->setCurrentItem(mtcType);

      mtcOffH->blockSignals(true);
      mtcOffM->blockSignals(true);
      mtcOffS->blockSignals(true);
      mtcOffF->blockSignals(true);
      mtcOffSf->blockSignals(true);
      mtcOffH->setValue(mtcOffset.h());
      mtcOffM->setValue(mtcOffset.m());
      mtcOffS->setValue(mtcOffset.s());
      mtcOffF->setValue(mtcOffset.f());
      mtcOffSf->setValue(mtcOffset.sf());
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
      
      MidiSyncLViewItem* lvi = (MidiSyncLViewItem*)devicesListView->firstChild();
      while(lvi) 
      {
        int port = lvi->port();
        if(port >= 0 && port < MIDI_PORTS)
        {
          bool sdet = midiPorts[port].syncInfo().MCSyncDetect();
          if(sdet)
          {
            if(port == curMidiSyncInPort)
            {
              if(!lvi->_curDet)
              {
                // Added by Tim. p3.3.6
                //printf("MidiSyncConfig::heartBeat setting current red icon\n");
            
                lvi->_curDet = true;
                lvi->_inDet = false;
                lvi->setPixmap(DEVCOL_IN, *record1_Icon);
              }  
            }
            else
            if(!lvi->_inDet)
            {
              // Added by Tim. p3.3.6
              //printf("MidiSyncConfig::heartBeat setting non-current green icon\n");
          
              lvi->_inDet = true;
              lvi->_curDet = false;
              lvi->setPixmap(DEVCOL_IN, *dotIcon);
            }  
          }
          else
          {
            if(lvi->_curDet || lvi->_inDet)
            {
              // Added by Tim. p3.3.6
              //printf("MidiSyncConfig::heartBeat setting off icon\n");
          
              lvi->_curDet = false;
              lvi->_inDet = false;
              lvi->setPixmap(DEVCOL_IN, *dothIcon);
            }  
          }
          
          sdet = midiPorts[port].syncInfo().tickDetect();
          if(sdet)
          {
            if(!lvi->_tickDet)
            {
              // Added by Tim. p3.3.6
              //printf("MidiSyncConfig::heartBeat setting tick on icon\n");
          
              lvi->_tickDet = true;
              lvi->setPixmap(DEVCOL_TICKIN, *dotIcon);
            }  
          } 
          else
          {
            if(lvi->_tickDet)
            {
              // Added by Tim. p3.3.6
              //printf("MidiSyncConfig::heartBeat setting tick off icon\n");
          
              lvi->_tickDet = false;
              lvi->setPixmap(DEVCOL_TICKIN, *dothIcon);
            }  
          }
        
          int type = midiPorts[port].syncInfo().recMTCtype();
          sdet = midiPorts[port].syncInfo().MMCDetect();
          bool mtcdet = midiPorts[port].syncInfo().MTCDetect();
          if(sdet)
          {
            if(!lvi->_MMCDet)
            {
              // Added by Tim. p3.3.6
              //printf("MidiSyncConfig::heartBeat setting MMC on icon\n");
          
              lvi->_MMCDet = true;
              lvi->setPixmap(DEVCOL_MMCIN, *dotIcon);
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
              // Added by Tim. p3.3.6
              //printf("MidiSyncConfig::heartBeat setting MMC off icon\n");
          
              lvi->_MMCDet = false;
              lvi->setPixmap(DEVCOL_MMCIN, *dothIcon);
            }  
          }
          
          if(mtcdet)
          {
            if(port == curMidiSyncInPort)
            {
              if(!lvi->_curMTCDet)
              {
                // Added by Tim. p3.3.6
                //printf("MidiSyncConfig::heartBeat setting current red icon\n");
            
                lvi->_curMTCDet = true;
                lvi->_MTCDet = false;
                lvi->setPixmap(DEVCOL_MTCIN, *record1_Icon);
              }  
            }
            else
            if(!lvi->_MTCDet)
            {
              // Added by Tim. p3.3.6
              //printf("MidiSyncConfig::heartBeat setting MTC on icon\n");
          
              lvi->_MTCDet = true;
              lvi->_curMTCDet = false;
              lvi->setPixmap(DEVCOL_MTCIN, *dotIcon);
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
              // Added by Tim. p3.3.6
              //printf("MidiSyncConfig::heartBeat setting MTC off icon\n");
          
              lvi->_MTCDet = false;
              lvi->_curMTCDet = false;
              lvi->setPixmap(DEVCOL_MTCIN, *dothIcon);
            }  
          }
        }
        
        //MidiDevice* dev = lvi->device();
        //bool sdet = dev->syncInfo().MCSyncDetect();
        //if(lvi->pixmap(DEVCOL_IN) != (sdet ? *dotIcon : *dothIcon))
        //  lvi->setPixmap(DEVCOL_IN, sdet ? *dotIcon : *dothIcon);
        
        lvi = (MidiSyncLViewItem*)lvi->nextSibling();
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
      
      //jackTransportMasterCheckbox->setEnabled(useJackTransport);
      
      //acceptMTCCheckbox->setEnabled(val);
//      acceptMTCCheckbox->setEnabled(false);
//      acceptMCCheckbox->setEnabled(val);
//      acceptMMCCheckbox->setEnabled(val);
      }

//---------------------------------------------------------
//   extSyncChanged
//---------------------------------------------------------

void MidiSyncConfig::extSyncChanged(bool v)
      {
      extSyncCheckbox->blockSignals(true);
      extSyncCheckbox->setChecked(v);
//      if(v)
//        song->setMasterFlag(false);
      extSyncCheckbox->blockSignals(false);
      }

//---------------------------------------------------------
//   ok Pressed
//---------------------------------------------------------

void MidiSyncConfig::ok()
      {
      apply();
      cancel();
      }

//---------------------------------------------------------
//   cancel Pressed
//---------------------------------------------------------

void MidiSyncConfig::cancel()
      {
      _dirty = false;
      if(applyButton->isEnabled())
        applyButton->setEnabled(false);
      
      close(false);
      }

//---------------------------------------------------------
//   show
//---------------------------------------------------------

void MidiSyncConfig::show()
{
  songChanged(-1);
  connect(song, SIGNAL(songChanged(int)), SLOT(songChanged(int)));
  connect(heartBeatTimer, SIGNAL(timeout()), SLOT(heartBeat()));
  QDialog::show();
}

//---------------------------------------------------------
//   closeEvent
//---------------------------------------------------------

void MidiSyncConfig::closeEvent(QCloseEvent* e)
      {
      if(_dirty)
      {
        int n = QMessageBox::warning(this, tr("MusE"),
         tr("Settings have changed\n"
         "Apply sync settings?"),
         tr("&Apply"), tr("&No"), tr("&Abort"), 0, 2);
         
        if(n == 2)
        {
          e->ignore();
          return; 
        }  
        
        if(n == 0)
          apply();
      }
      
      //emit deleted((unsigned long)this);
      
      disconnect(heartBeatTimer, SIGNAL(timeout()), this, SLOT(heartBeat()));
      disconnect(song, SIGNAL(songChanged(int)), this, SLOT(songChanged(int)));
      
      e->accept();
      }

//---------------------------------------------------------
//   apply Pressed
//---------------------------------------------------------

void MidiSyncConfig::apply()
{
//      txDeviceId  = dstDevId->value();
//      rxDeviceId  = srcDevId->value();
//      rxSyncPort  = srcSyncPort->value() - 1;
//      txSyncPort  = dstSyncPort->value() - 1;

//      genMTCSync  = mtcSync->isChecked();
//      genMCSync   = mcSync->isChecked();
//      genMMC      = midiMachineControl->isChecked();

      syncSendFirstClockDelay = syncDelaySpinBox->value();
      
      mtcType     = mtcSyncType->currentItem();
      //extSyncFlag.setValue(syncMode->id(syncMode->selected()));
      //extSyncFlag.blockSignals(true);
      extSyncFlag.setValue(extSyncCheckbox->isChecked());
//      if(extSyncFlag.value())
//        song->setMasterFlag(false);
      //extSyncFlag.blockSignals(false);
      useJackTransport = useJackTransportCheckbox->isChecked();
//      if(useJackTransport)
        jackTransportMaster = jackTransportMasterCheckbox->isChecked();
//      else  
//        jackTransportMaster = false;
//      jackTransportMasterCheckbox->setEnabled(useJackTransport);
      if(audioDevice)
        audioDevice->setMaster(jackTransportMaster);      

      mtcOffset.setH(mtcOffH->value());
      mtcOffset.setM(mtcOffM->value());
      mtcOffset.setS(mtcOffS->value());
      mtcOffset.setF(mtcOffF->value());
      mtcOffset.setSf(mtcOffSf->value());

//      acceptMC  = acceptMCCheckbox->isChecked();
//      acceptMMC = acceptMMCCheckbox->isChecked();
//      acceptMTC = acceptMTCCheckbox->isChecked();
      
      MidiSyncLViewItem* lvi = (MidiSyncLViewItem*)devicesListView->firstChild();
      while(lvi) 
      {
        //MidiDevice* dev = lvi->device();
        // Does the device really exist?
        //if(midiDevices.find(dev) != midiDevices.end())
        //  dev->syncInfo().copyParams(lvi->syncInfo());
        int port = lvi->port();
        if(port >= 0 && port < MIDI_PORTS)
          //midiPorts[port].syncInfo().copyParams(lvi->syncInfo());
          lvi->copyToSyncInfo(midiPorts[port].syncInfo());
        
        lvi = (MidiSyncLViewItem*)lvi->nextSibling();
      }
  
  //muse->changeConfig(true);    // save settings
  
  _dirty = false;
  if(applyButton->isEnabled())
    applyButton->setEnabled(false);
  
  // Do not call this. Causes freeze sometimes. Only will be needed if extra pollfds are used by midi seq thread.
  //midiSeq->msgUpdatePollFd();
}

//---------------------------------------------------------
//   updateSyncInfoLV
//---------------------------------------------------------

void MidiSyncConfig::updateSyncInfoLV()
      {
      devicesListView->clear();
      for(int i = MIDI_PORTS-1; i >= 0; --i) 
      {
            MidiPort* port  = &midiPorts[i];
            MidiDevice* dev = port->device();
            // p3.3.31
            // Don't show if it is a synthesizer device.
            // Hmm, some synths might support transport commands or even sync?
            // If anything, the DSSI or VST synths just might... 
            // TODO: Must test to see if it screws any of them up, especially clock out.
            // Also, if we do this, we must prevent such messages from reaching
            //  those ports at several other places in the code.
            //if(dev && dev->isSynti())
            //  continue;
              
            QString s;
            s.setNum(i+1);
            MidiSyncLViewItem* lvi = new MidiSyncLViewItem(devicesListView);
            lvi->setPort(i); // setPort will copy parameters.
            //MidiSyncInfo& si = lvi->syncInfo();
            //si.copyParams(port->syncInfo());
            //lvi.copyFromSyncInfo(port->syncInfo());
            MidiSyncInfo& portsi = port->syncInfo();
            
            lvi->setText(DEVCOL_NO, s);
            
            if (dev) 
                  lvi->setText(DEVCOL_NAME, dev->name());
            else 
                  lvi->setText(DEVCOL_NAME, tr("<none>"));
            
            if(portsi.MCSyncDetect())
            {
              if(i == curMidiSyncInPort)
              {
                lvi->_curDet = true;
                lvi->_inDet = false;
                lvi->setPixmap(DEVCOL_IN, *record1_Icon);
              }
              else
              {
                lvi->_curDet = false;
                lvi->_inDet = true;
                lvi->setPixmap(DEVCOL_IN, *dotIcon);
              }
            }
            else
            {
              lvi->_curDet = false;
              lvi->_inDet = false;
              lvi->setPixmap(DEVCOL_IN, *dothIcon);
            }
            
            if(portsi.tickDetect())
            {
              lvi->_tickDet = true;
              lvi->setPixmap(DEVCOL_TICKIN, *dotIcon);
            }
            else
            {
              lvi->_tickDet = false;
              lvi->setPixmap(DEVCOL_TICKIN, *dothIcon);
            }
            
            if(portsi.MMCDetect())
            {
              lvi->_MMCDet = true;
              lvi->setPixmap(DEVCOL_MMCIN, *dotIcon);
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
              lvi->setPixmap(DEVCOL_MMCIN, *dothIcon);
            }
            
            if(portsi.MTCDetect())
            {
              if(i == curMidiSyncInPort)
              {
                lvi->_curMTCDet = true;
                lvi->_MTCDet = false;
                lvi->setPixmap(DEVCOL_MTCIN, *record1_Icon);
              }
              else
              {
                lvi->_curMTCDet = false;
                lvi->_MTCDet = true;
                lvi->setPixmap(DEVCOL_MTCIN, *dotIcon);
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
              lvi->setPixmap(DEVCOL_MTCIN, *dothIcon);
              //lvi->setText(DEVCOL_MTCTYPE, "--");
            }
            
            //lvi->setText(DEVCOL_RID,    QString().setNum(si.idIn()) );
            //lvi->setRenameEnabled(DEVCOL_RID, true);
            //lvi->setPixmap(DEVCOL_RCLK, si.MCIn() ? *dotIcon : *dothIcon);
            //lvi->setPixmap(DEVCOL_RMMC, si.MMCIn() ? *dotIcon : *dothIcon);
            //lvi->setPixmap(DEVCOL_RMTC, si.MTCIn() ? *dotIcon : *dothIcon);
            lvi->setText(DEVCOL_RID,    QString().setNum(lvi->_idIn) );
            lvi->setPixmap(DEVCOL_RCLK, lvi->_recMC ? *dotIcon : *dothIcon);
            lvi->setPixmap(DEVCOL_RMMC, lvi->_recMMC ? *dotIcon : *dothIcon);
            lvi->setPixmap(DEVCOL_RMTC, lvi->_recMTC ? *dotIcon : *dothIcon);
            lvi->setPixmap(DEVCOL_RREWSTART, lvi->_recRewOnStart ? *dotIcon : *dothIcon);
            
            //lvi->setText(DEVCOL_TID,    QString().setNum(si.idOut()) );
            //lvi->setRenameEnabled(DEVCOL_TID, true);
            //lvi->setPixmap(DEVCOL_TCLK, si.MCOut() ? *dotIcon : *dothIcon);
            //lvi->setPixmap(DEVCOL_TMMC, si.MMCOut() ? *dotIcon : *dothIcon);
            //lvi->setPixmap(DEVCOL_TMTC, si.MTCOut() ? *dotIcon : *dothIcon);
            lvi->setText(DEVCOL_TID,          QString().setNum(lvi->_idOut) );
            lvi->setPixmap(DEVCOL_TCLK,       lvi->_sendMC ? *dotIcon : *dothIcon);
            lvi->setPixmap(DEVCOL_TMMC,       lvi->_sendMMC ? *dotIcon : *dothIcon);
            lvi->setPixmap(DEVCOL_TMTC,       lvi->_sendMTC ? *dotIcon : *dothIcon);
            //lvi->setPixmap(DEVCOL_TREWSTART,  lvi->_sendContNotStart ? *dotIcon : *dothIcon);
            
            devicesListView->insertItem(lvi);
      }
      
      /*
      for(iMidiDevice id = midiDevices.begin(); id != midiDevices.end(); ++id) 
      {
            MidiDevice* dev = *id;
      
            //MidiPort* port  = &midiPorts[i];
            //MidiDevice* dev = port->device();
            MidiSyncLViewItem* lvi = new MidiSyncLViewItem(devicesListView);
            //lvi->setPort(i);
            // setDevice will copy parameters.
            lvi->setDevice(dev);
            MidiSyncInfo& si = lvi->syncInfo();
            //si.copyParams(dev->syncInfo());
            
            lvi->setText(DEVCOL_NAME, dev->name());
            
            lvi->setPixmap(DEVCOL_IN,   si.MCSyncDetect() ? *dotIcon : *dothIcon);
            
            lvi->setText(DEVCOL_RID,    QString().setNum(si.idIn()) );
            lvi->setPixmap(DEVCOL_RCLK, si.MCIn() ? *dotIcon : *dothIcon);
            lvi->setPixmap(DEVCOL_RMMC, si.MMCIn() ? *dotIcon : *dothIcon);
            lvi->setPixmap(DEVCOL_RMTC, si.MTCIn() ? *dotIcon : *dothIcon);
            
            lvi->setText(DEVCOL_TID,    QString().setNum(si.idOut()) );
            lvi->setPixmap(DEVCOL_TCLK, si.MCOut() ? *dotIcon : *dothIcon);
            lvi->setPixmap(DEVCOL_TMMC, si.MMCOut() ? *dotIcon : *dothIcon);
            lvi->setPixmap(DEVCOL_TMTC, si.MTCOut() ? *dotIcon : *dothIcon);
            
            devicesListView->insertItem(lvi);
      }
      */

      }


//---------------------------------------------------------
//   dlvClicked
//---------------------------------------------------------

//void MidiSyncConfig::dlvClicked(QListViewItem* item, const QPoint&, int col)
void MidiSyncConfig::dlvClicked(int /*button*/, QListViewItem* item, const QPoint&, int col)
{
      if (item == 0)
            return;
      
      MidiSyncLViewItem* lvi = (MidiSyncLViewItem*)item;
      int no = lvi->port();
      if (no < 0 || no >= MIDI_PORTS)
        return;
      //MidiDevice* dev = lvi->device();
      // Does the device really exist?
      //if(midiDevices.find(dev) == midiDevices.end())
      //  return;
      
      //int n;
      //MidiPort* port      = &midiPorts[no];
      //MidiDevice* dev     = port->device();
      //int rwFlags         = dev ? dev->rwFlags() : 0;
      //int openFlags       = dev ? dev->openFlags() : 0;
      //MidiSyncInfo& si    = lvi->syncInfo();
      //MidiSyncInfo& portsi  = midiPorts[no].syncInfo();

      switch (col) 
      {
            case DEVCOL_NO:
                  break;
            case DEVCOL_NAME:
                  break;
            case DEVCOL_IN:
                  // If this is not the current midi sync in port, and sync in from this port is enabled,
                  //  and sync is in fact detected on this port, allow the user to force this port to now be the
                  //  current sync in port. 
                  //if(no != curMidiSyncInPort && si.MCIn() && midiPorts[no].syncInfo().MCSyncDetect())
                  //if(no != curMidiSyncInPort && lvi->_recMC && midiPorts[no].syncInfo().MCSyncDetect())
                  if(no != curMidiSyncInPort)
                  {
                    if(lvi->_recMC && midiPorts[no].syncInfo().MCSyncDetect())
                    {
                      curMidiSyncInPort = no;
                      lvi->setPixmap(DEVCOL_IN, *record1_Icon);
                    }  
                    if(lvi->_recMTC && midiPorts[no].syncInfo().MTCDetect())
                    {
                      curMidiSyncInPort = no;
                      lvi->setPixmap(DEVCOL_MTCIN, *record1_Icon);
                    }  
                  }  
                  break;
            case DEVCOL_TICKIN:
                  break;
            case DEVCOL_MMCIN:
                  break;
            case DEVCOL_MTCIN:
                  // If this is not the current midi sync in port, and sync in from this port is enabled,
                  //  and sync is in fact detected on this port, allow the user to force this port to now be the
                  //  current sync in port. 
                  //if(no != curMidiSyncInPort && si.MTCIn() && midiPorts[no].syncInfo().MTCDetect())
                  //if(no != curMidiSyncInPort && lvi->_recMTC && midiPorts[no].syncInfo().MTCDetect())
                  if(no != curMidiSyncInPort)
                  {
                    if(lvi->_recMTC && midiPorts[no].syncInfo().MTCDetect())
                    {
                      curMidiSyncInPort = no;
                      lvi->setPixmap(DEVCOL_MTCIN, *record1_Icon);
                    }  
                    if(lvi->_recMC && midiPorts[no].syncInfo().MCSyncDetect())
                    {
                      curMidiSyncInPort = no;
                      lvi->setPixmap(DEVCOL_IN, *record1_Icon);
                    }  
                  }  
                  break;
            case DEVCOL_MTCTYPE:
                  break;
            case DEVCOL_RID:
                  break;
            case DEVCOL_RCLK:
                  //si.setMCIn(si.MCIn() ? false : true);
                  //lvi->setPixmap(DEVCOL_RCLK, si.MCIn() ? *dotIcon : *dothIcon);
                  lvi->_recMC = (lvi->_recMC ? false : true);
                  lvi->setPixmap(DEVCOL_RCLK, lvi->_recMC ? *dotIcon : *dothIcon);
                  setDirty();
                  break;
            case DEVCOL_RMMC:
                  //si.setMMCIn(si.MMCIn() ? false : true);
                  //lvi->setPixmap(DEVCOL_RMMC, si.MMCIn() ? *dotIcon : *dothIcon);
                  lvi->_recMMC = (lvi->_recMMC ? false : true);
                  lvi->setPixmap(DEVCOL_RMMC, lvi->_recMMC ? *dotIcon : *dothIcon);
                  setDirty();
                  break;
            case DEVCOL_RMTC:
                  //si.setMTCIn(si.MTCIn() ? false : true);
                  //lvi->setPixmap(DEVCOL_RMTC, si.MTCIn() ? *dotIcon : *dothIcon);
                  lvi->_recMTC = (lvi->_recMTC ? false : true);
                  lvi->setPixmap(DEVCOL_RMTC, lvi->_recMTC ? *dotIcon : *dothIcon);
                  setDirty();
                  break;
            case DEVCOL_RREWSTART:
                  lvi->_recRewOnStart = (lvi->_recRewOnStart ? false : true);
                  lvi->setPixmap(DEVCOL_RREWSTART, lvi->_recRewOnStart ? *dotIcon : *dothIcon);
                  setDirty();
                  break;
            case DEVCOL_TID:
                  break;
            case DEVCOL_TCLK:
                  //si.setMCOut(si.MCOut() ? false : true);
                  //lvi->setPixmap(DEVCOL_TCLK, si.MCOut() ? *dotIcon : *dothIcon);
                  lvi->_sendMC = (lvi->_sendMC ? false : true);
                  lvi->setPixmap(DEVCOL_TCLK, lvi->_sendMC ? *dotIcon : *dothIcon);
                  setDirty();
                  break;
            case DEVCOL_TMMC:
                  //si.setMMCOut(si.MMCOut() ? false : true);
                  //lvi->setPixmap(DEVCOL_TMMC, si.MMCOut() ? *dotIcon : *dothIcon);
                  lvi->_sendMMC = (lvi->_sendMMC ? false : true);
                  lvi->setPixmap(DEVCOL_TMMC, lvi->_sendMMC ? *dotIcon : *dothIcon);
                  setDirty();
                  break;
            case DEVCOL_TMTC:
                  //si.setMTCOut(si.MTCOut() ? false : true);
                  //lvi->setPixmap(DEVCOL_TMTC, si.MTCOut() ? *dotIcon : *dothIcon);
                  lvi->_sendMTC = (lvi->_sendMTC ? false : true);
                  lvi->setPixmap(DEVCOL_TMTC, lvi->_sendMTC ? *dotIcon : *dothIcon);
                  setDirty();
                  break;
            //case DEVCOL_TREWSTART:
            //      lvi->_sendContNotStart = (lvi->_sendContNotStart ? false : true);
            //      lvi->setPixmap(DEVCOL_TREWSTART, lvi->_sendContNotStart ? *dotIcon : *dothIcon);
            //      setDirty();
            //      break;
      }
      //songChanged(-1);
}

//---------------------------------------------------------
//   dlvDoubleClicked
//---------------------------------------------------------

void MidiSyncConfig::dlvDoubleClicked(QListViewItem* item, const QPoint&, int col)
{
      if(!item)
        return;
      
      MidiSyncLViewItem* lvi = (MidiSyncLViewItem*)item;
      
      //if(col == DEVCOL_RID)
      //  lvi->startRename(DEVCOL_RID); 
      //else
      //if(col == DEVCOL_TID)
      //  lvi->startRename(DEVCOL_TID); 
      
      bool ok = false;
      if(col == DEVCOL_RID)
      {
        //int val = lvi->syncInfo().idIn();
        int val = lvi->_idIn;
        int newval = QInputDialog::getInteger("Muse: Sync info" , "Enter new id number (127 = all):", val, 0, 127, 1, &ok, this);
        if(ok)
        {
          //lvi->syncInfo().setIdIn(newval);
          lvi->_idIn = newval;
          lvi->setText(DEVCOL_RID, QString().setNum(newval)); 
        }  
      }  
      else
      if(col == DEVCOL_TID)
      {
        //int val = lvi->syncInfo().idOut();
        int val = lvi->_idOut;
        int newval = QInputDialog::getInteger("Muse: Sync info" , "Enter new id number (127 = global):", val, 0, 127, 1, &ok, this);
        if(ok)
        {
          //lvi->syncInfo().setIdOut(newval);
          lvi->_idOut = newval;
          lvi->setText(DEVCOL_TID, QString().setNum(newval)); 
        }  
      }  
      
      if(ok)
        setDirty();
}

/*
//---------------------------------------------------------
//   renameOk
//---------------------------------------------------------
//void MidiSyncConfig::renameOk(QListViewItem* item, int col)
void MidiSyncConfig::renameOk(QListViewItem* item, int col, const QString & text)
{
      if(!item)
        return;
      
      MidiSyncLViewItem* lvi = (MidiSyncLViewItem*)item;
      QString t = text;
      bool ok;
      int id = text.toInt(&ok);
      if(!ok)
      {
        lvi->setText(t);
        return;
      }  
      if(col == DEVCOL_RID)
      {
        //lvi->syncInfo().setIdIn(id);
        lvi->_idIn = id;
        setDirty();
      }  
      else
      if(col == DEVCOL_TID)
      {
        //lvi->syncInfo().setIdOut(id);
        lvi->_idOut = id;
        setDirty();
      }  
}
*/

//---------------------------------------------------------
//   MidiSyncConfig::setDirty
//---------------------------------------------------------

void MidiSyncConfig::setDirty()
{
  _dirty = true;
  if(!applyButton->isEnabled())
    applyButton->setEnabled(true);
}

