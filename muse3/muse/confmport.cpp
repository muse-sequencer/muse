//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: confmport.cpp,v 1.9.2.10 2009/12/15 03:39:58 terminator356 Exp $
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//  (C) Copyright 2015 Tim E. Real (terminator356 on sourceforge)
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

#include <list>
#ifndef _WIN32
#include <termios.h>
#endif
#include <iostream>
#include <stdio.h>

#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QMessageBox>
#include <QPixmap>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QSettings>

#include "confmport.h"
#include "app.h"
#include "icons.h"
#include "globals.h"
#include "arranger.h"
#include "midiport.h"
#include "mididev.h"
#include "xml.h"
#include "midisyncimpl.h"
#include "midifilterimpl.h"
#include "ctrlcombo.h"
#include "minstrument.h"
#include "synth.h"
#include "audio.h"
#include "midiseq.h"
#ifndef _WIN32
#include "driver/alsamidi.h"
#endif
#include "driver/jackmidi.h"
#include "audiodev.h"
#include "menutitleitem.h"
#include "utils.h"
#include "popupmenu.h"
#include "routepopup.h"
#include "operations.h"
#include "gconfig.h"

// For debugging output: Uncomment the fprintf section.
#define DEBUG_PRST_ROUTES(dev, format, args...) // fprintf(dev, format, ##args);

// Undefine if and when multiple output routes are added to midi tracks.
#define _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_

namespace MusEGlobal {
extern MusECore::SynthList synthis;
}

namespace MusEGui {

//---------------------------------------------------------
//   closeEvent
//---------------------------------------------------------
       
void MPConfig::closeEvent(QCloseEvent *event)
{
    apply();
    QSettings settings("MusE", "MusE-qt");
    settings.setValue("MPConfig/geometry", saveGeometry());
    QWidget::closeEvent(event);
}

//---------------------------------------------------------
//   apply
//---------------------------------------------------------

void MPConfig::apply()
{
  MusEGlobal::audio->msgInitMidiDevices(false);  // false = Don't force
}

//---------------------------------------------------------
//   okClicked
//---------------------------------------------------------

void MPConfig::okClicked()
{
  close();  
}

//---------------------------------------------------------
//   changeDefInputRoutes
//---------------------------------------------------------

void MPConfig::changeDefInputRoutes(QAction* act)
{
  QTableWidgetItem* item = mdevView->currentItem();
  if(item == 0)
    return;
  QString id = mdevView->item(item->row(), DEVCOL_NO)->text();
  int no = atoi(id.toLatin1().constData()) - 1;
  if(no < 0 || no >= MusECore::MIDI_PORTS)
    return;
  int actid = act->data().toInt();
  int allch = (1 << MusECore::MUSE_MIDI_CHANNELS) - 1;  
  int defch = MusEGlobal::midiPorts[no].defaultInChannels();  
  MusECore::PendingOperationList operations;
  
  if(actid == MusECore::MUSE_MIDI_CHANNELS + 1)  // Apply to all tracks now.
  {
    // Are there tracks, and is there a port device? 
    // Tested: Hmm, allow ports with no device since that is a valid situation.
    if(!MusEGlobal::song->midis()->empty()) // && MusEGlobal::midiPorts[no].device())  
    {
      int ret = QMessageBox::question(this, tr("Default input connections"),
                                    tr("Are you sure you want to apply to all existing midi tracks now?"),
                                    QMessageBox::Ok | QMessageBox::Cancel,
                                    QMessageBox::Cancel);
      if(ret == QMessageBox::Ok) 
      {
        MusECore::MidiTrackList* mtl = MusEGlobal::song->midis();
        for(MusECore::iMidiTrack it = mtl->begin(); it != mtl->end(); ++it)
        {
          MusECore::MidiTrack* mt = *it;
          MusECore::RouteList* rl = mt->inRoutes();
          for(MusECore::ciRoute ir = rl->begin(); ir != rl->end(); ++ir)
          {
            switch(ir->type)
            {
              case MusECore::Route::MIDI_PORT_ROUTE:
                // Remove all routes from this port to the tracks first.
                if(ir->midiPort == no)
                  operations.add(MusECore::PendingOperationItem(*ir, MusECore::Route(mt, ir->channel),
                                                                MusECore::PendingOperationItem::DeleteRoute));
              break;  
              case MusECore::Route::TRACK_ROUTE:
              case MusECore::Route::JACK_ROUTE:
              case MusECore::Route::MIDI_DEVICE_ROUTE:
              break;  
            }
            
            // All channels set or Omni? Use an Omni route:
            if(defch == -1 || defch == allch)
              operations.add(MusECore::PendingOperationItem(MusECore::Route(no), MusECore::Route(mt),
                                                            MusECore::PendingOperationItem::AddRoute));
            else
            // Add individual channels:  
            for(int ch = 0; ch < MusECore::MUSE_MIDI_CHANNELS; ++ch)
            {
              const int chbit = 1 << ch;
              if(defch & chbit)
                operations.add(MusECore::PendingOperationItem(MusECore::Route(no, ch), MusECore::Route(mt, ch),
                                                              MusECore::PendingOperationItem::AddRoute));
            }
          }
        }
        
        if(!operations.empty())
        {
          operations.add(MusECore::PendingOperationItem((MusECore::TrackList*)NULL, MusECore::PendingOperationItem::UpdateSoloStates));
          MusEGlobal::audio->msgExecutePendingOperations(operations, true);
//           MusEGlobal::song->update(SC_ROUTE);
        }
      }
    }  
  }
  else
  {
    int chbits;
    if(actid == MusECore::MUSE_MIDI_CHANNELS)              // Toggle all.
    {
      chbits = (defch == -1 || defch == allch) ? 0 : allch;
      if(act->actionGroup())
      {
        QList<QAction*> acts = act->actionGroup()->actions();
        const int sz = acts.size();
        for(int i = 0; i < sz; ++i)
        {
          QAction* a = acts.at(i);  
          if(a)
            a->setChecked(chbits);
        }
      }
    }  
    else
    {
      if(defch == -1)
        chbits = 0;
      else
        chbits = defch ^ (1 << actid);
    }
    MusEGlobal::midiPorts[no].setDefaultInChannels(chbits);
    mdevView->item(item->row(), DEVCOL_DEF_IN_CHANS)->setText(MusECore::bitmap2String(chbits));
  }  
}

//---------------------------------------------------------
//   changeDefOutputRoutes
//---------------------------------------------------------

void MPConfig::changeDefOutputRoutes(QAction* act)
{
  QTableWidgetItem* item = mdevView->currentItem();
  if(item == 0)
    return;
  QString id = mdevView->item(item->row(), DEVCOL_NO)->text();
  int no = atoi(id.toLatin1().constData()) - 1;
  if(no < 0 || no >= MusECore::MIDI_PORTS)
    return;
  int actid = act->data().toInt();
  int defch = MusEGlobal::midiPorts[no].defaultOutChannels();  
  
#ifndef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
  int allch = (1 << MusECore::MUSE_MIDI_CHANNELS) - 1;
  MusECore::PendingOperationList operations;
#endif
  
  if(actid == MusECore::MUSE_MIDI_CHANNELS + 1)  // Apply to all tracks now.
  {
    // Are there tracks, and is there a port device? 
    // Tested: Hmm, allow ports with no device since that is a valid situation.
    if(!MusEGlobal::song->midis()->empty()) // && MusEGlobal::midiPorts[no].device())
    {
      
#ifndef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
      if(!defch) // No channels selected? Just return.
        return;
#endif
        
      int ret = QMessageBox::question(this, tr("Default output connections"),
                                    tr("Are you sure you want to apply to all existing midi tracks now?"),
                                    QMessageBox::Ok | QMessageBox::Cancel,
                                    QMessageBox::Cancel);
      if(ret == QMessageBox::Ok) 
      {
        MusECore::MidiTrackList* mtl = MusEGlobal::song->midis();
        
#ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
        for(int ch = 0; ch < MusECore::MUSE_MIDI_CHANNELS; ++ch)
          if(defch & (1 << ch))
          { 
            MusECore::MidiTrack::ChangedType_t changed = MusECore::MidiTrack::NothingChanged;
            MusEGlobal::audio->msgIdle(true);
            for(MusECore::iMidiTrack it = mtl->begin(); it != mtl->end(); ++it)
            {
              // Leave drum track channel at current setting.
              if((*it)->type() == MusECore::Track::DRUM)
                changed |= (*it)->setOutPortAndUpdate(no, false);
              else
                changed |= (*it)->setOutPortAndChannelAndUpdate(no, ch, false);
            }
            MusEGlobal::audio->msgIdle(false);
            MusEGlobal::audio->msgUpdateSoloStates();
            MusEGlobal::song->update(SC_ROUTE | ((changed & MusECore::MidiTrack::DrumMapChanged) ? SC_DRUMMAP : 0));

            // Stop at the first output channel found.
            break;
          }  
#else
        for(MusECore::iMidiTrack it = mtl->begin(); it != mtl->end(); ++it)
        {
          MusECore::MidiTrack* mt = *it;
          MusECore::RouteList* rl = mt->outRoutes();
          for(MusECore::ciRoute ir = rl->begin(); ir != rl->end(); ++ir)
          {
            switch(ir->type)
            {
              case MusECore::Route::MIDI_PORT_ROUTE:
                // Remove all routes from this port to the tracks first.
                if(ir->midiPort == no)
                  operations.add(MusECore::PendingOperationItem(MusECore::Route(mt, ir->channel), *ir,
                                                                MusECore::PendingOperationItem::DeleteRoute));
              break;  
              case MusECore::Route::TRACK_ROUTE:
              case MusECore::Route::JACK_ROUTE:
              case MusECore::Route::MIDI_DEVICE_ROUTE:
              break;  
            }
            
            // All channels set or Omni? Use an Omni route:
            if(defch == -1 || defch == allch)
              operations.add(MusECore::PendingOperationItem(MusECore::Route(mt), MusECore::Route(no),
                                                            MusECore::PendingOperationItem::AddRoute));
            else
            // Add individual channels:  
            for(int ch = 0; ch < MusECore::MUSE_MIDI_CHANNELS; ++ch)
            {
              const int chbit = 1 << ch;
              if(defch & chbit)
                operations.add(MusECore::PendingOperationItem(MusECore::Route(mt, ch), MusECore::Route(no, ch),
                                                              MusECore::PendingOperationItem::AddRoute));
            }
          }
        }
        
        if(!operations.empty())
        {
          operations.add(MusECore::PendingOperationItem((MusECore::TrackList*)NULL, MusECore::PendingOperationItem::UpdateSoloStates));
          MusEGlobal::audio->msgExecutePendingOperations(operations, true);
//           MusEGlobal::song->update(SC_ROUTE);
        }        
#endif

      }
    }  
  }
  else
  {
#ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
    if(actid < MusECore::MUSE_MIDI_CHANNELS)
    {
      int chbits = 1 << actid;
      // Are we toggling off?
      //if(chbits & defch)
      if(defch == -1 || chbits & defch)
      {
        // Just clear this port's default channels.
        MusEGlobal::midiPorts[no].setDefaultOutChannels(0);
        mdevView->item(item->row(), DEVCOL_DEF_OUT_CHANS)->setText(MusECore::bitmap2String(0));
      }
      else
      {
        // Multiple out routes not supported. Make the setting exclusive to this port - exclude all other ports.
        MusECore::setPortExclusiveDefOutChan(no, chbits);
        int j = mdevView->rowCount();
        for(int i = 0; i < j; ++i)
          mdevView->item(i, DEVCOL_DEF_OUT_CHANS)->setText(MusECore::bitmap2String(i == no ? chbits : 0));
        // The group is exclusive. No need to iterate manually.
//         if(act->actionGroup())
//         {
//           QList<QAction*> acts = act->actionGroup()->actions();
//           const int sz = acts.size();
//           if(sz > actid)
//           {
//             QAction* a = acts.at(actid);
//             if(a)
//               a->setChecked(true);
//           }
//         }
      }
    }    
#else
    int chbits;
    if(actid == MusECore::MUSE_MIDI_CHANNELS)              // Toggle all.
    {
      chbits = (defch == -1 || defch == allch) ? 0 : allch;
      if(act->actionGroup())
      {
        QList<QAction*> acts = act->actionGroup()->actions();
        const int sz = acts.size();
        for(int i = 0; i < sz; ++i)
        {
          QAction* a = acts.at(i);  
          if(a)
            a->setChecked(chbits);
        }
      }
    }  
    else
    {
      if(defch == -1)
        chbits = 0;
      else
        chbits = defch ^ (1 << actid);
    }
    MusEGlobal::midiPorts[no].setDefaultOutChannels(chbits);
    mdevView->item(item->row(), DEVCOL_DEF_OUT_CHANS)->setText(MusECore::bitmap2String(chbits));
#endif

  }  
}

//---------------------------------------------------------
//   DeviceItemRenamed
//---------------------------------------------------------

void MPConfig::DeviceItemRenamed(QTableWidgetItem* item)
{
  if(item == 0)
    return;
  if(!item->data(DeviceRole).canConvert<void*>())
    return;
  MusECore::MidiDevice* md = static_cast<MusECore::MidiDevice*>(item->data(DeviceRole).value<void*>());
  
  int col = item->column();
  QTableWidgetItem* twi = item->tableWidget()->item(item->row(), INSTCOL_NAME);
  if(!twi)
  {
    fprintf(stderr, "synthesizerConfig::DeviceItemRenamed(): row:%d INSTCOL_NAME not found\n", item->row());
    return;
  }
  QString new_name = twi->text();
  // Get the original name.
  QString orig_name = md->name();
  if(new_name == orig_name)
    return;
  MusECore::iMidiDevice imd;
  for(imd = MusEGlobal::midiDevices.begin(); imd != MusEGlobal::midiDevices.end(); ++imd) {
        MusECore::MidiDevice* d = *imd;
        if(d == md || (*imd)->name() != new_name)
          continue;
        break;
        }
  switch(col)
  {
    case INSTCOL_NAME:
    {
      // Only Jack midi devices.
      if(md->deviceType() != MusECore::MidiDevice::JACK_MIDI)
        return;
      if(imd != MusEGlobal::midiDevices.end())
      {
        QMessageBox::critical(this,
            tr("MusE: bad device name"),
            tr("please choose a unique device name"),
            QMessageBox::Ok,
            Qt::NoButton,
            Qt::NoButton);
        instanceList->blockSignals(true);
        item->setText(orig_name);
        instanceList->blockSignals(false);
        return;
      }
      MusEGlobal::audio->msgIdle(true); // Make it safe to edit structures
      md->setName(new_name);
      MusEGlobal::audio->msgIdle(false);
      MusEGlobal::song->update(SC_CONFIG);
    }
    break;    
    default: 
    break;
  } 
}

//---------------------------------------------------------
//   rbClicked
//---------------------------------------------------------

void MPConfig::rbClicked(QTableWidgetItem* item)
      {
      if (item == 0)
            return;
      QString id = item->tableWidget()->item(item->row(), DEVCOL_NO)->text();
      int no = atoi(id.toLatin1().constData()) - 1;
      if (no < 0 || no >= MusECore::MIDI_PORTS)
            return;

      int n;
      MusECore::MidiPort* port      = &MusEGlobal::midiPorts[no];
      MusECore::MidiDevice* dev     = port->device();
#ifndef _USE_EXTRA_INSTANCE_COLUMNS_
      int rwFlags         = dev ? dev->rwFlags() : 0;
      int openFlags       = dev ? dev->openFlags() : 0;
#endif      
      QTableWidget* listView = item->tableWidget();
      QPoint ppt          = listView->visualItemRect(item).bottomLeft();
      int col = item->column();
      ppt += QPoint(0, listView->horizontalHeader()->height());
      ppt  = listView->mapToGlobal(ppt);
      MusECore::PendingOperationList operations;
      
      switch (col) {
        
#ifndef _USE_EXTRA_INSTANCE_COLUMNS_
            case DEVCOL_GUI:
                  if (dev == 0)
                        return;
                  if (port->hasNativeGui())
                  {
                        port->showNativeGui(!port->nativeGuiVisible());
                        item->setIcon(port->nativeGuiVisible() ? QIcon(*dotIcon) : QIcon(*dothIcon));
                  }
                  return;
            break;                    
                  
            case DEVCOL_REC:
                  if (dev == 0 || !(rwFlags & 2))
                        return;
                  openFlags ^= 0x2;
                  dev->setOpenFlags(openFlags);
                  MusEGlobal::audio->msgSetMidiDevice(port, dev);       // reopen device
                  item->setIcon(openFlags & 2 ? QIcon(*dotIcon) : QIcon(*dothIcon));
                  
                  if(dev->deviceType() == MusECore::MidiDevice::JACK_MIDI)
                  {
                    if(dev->openFlags() & 2)  
                    {
                      item->tableWidget()->item(item->row(), DEVCOL_INROUTES)->setIcon(QIcon(*buttondownIcon));
                     item->tableWidget()->item(item->row(), DEVCOL_INROUTES)->setText(tr("in"));
                    }
                    else
                    {
                      item->tableWidget()->item(item->row(), DEVCOL_INROUTES)->setIcon(QIcon());
                      item->tableWidget()->item(item->row(), DEVCOL_INROUTES)->setText("");
                    }  
                  }
                  return;
            break;                    
                  
            case DEVCOL_PLAY:
                  if (dev == 0 || !(rwFlags & 1))
                        return;
                  openFlags ^= 0x1;
                  dev->setOpenFlags(openFlags);
                  MusEGlobal::audio->msgSetMidiDevice(port, dev);       // reopen device
                  item->setIcon(openFlags & 1 ? QIcon(*dotIcon) : QIcon(*dothIcon));
                  
                  if(dev->deviceType() == MusECore::MidiDevice::JACK_MIDI)
                  {
                    if(dev->openFlags() & 1)  
                    {
                      item->tableWidget()->item(item->row(), DEVCOL_OUTROUTES)->setIcon(QIcon(*buttondownIcon));
                      item->tableWidget()->item(item->row(), DEVCOL_OUTROUTES)->setText(tr("out"));
                    }
                    else  
                    {
                      item->tableWidget()->item(item->row(), DEVCOL_OUTROUTES)->setIcon(QIcon());
                      item->tableWidget()->item(item->row(), DEVCOL_OUTROUTES)->setText("");
                    }
                  }
                  return;
            break;                    
                  
            case DEVCOL_INROUTES:  
            case DEVCOL_OUTROUTES:
                  {
                    if(!MusEGlobal::checkAudioDevice())
                      return;
                      
                    if(MusEGlobal::audioDevice->deviceType() != MusECore::AudioDevice::JACK_AUDIO)  // Only if Jack is running.
                      return;
                      
                    if(!dev)
                      return;
                    
                    // Only Jack midi devices.
                    if(dev->deviceType() != MusECore::MidiDevice::JACK_MIDI)  
                      return;
                    
                    if(!(dev->openFlags() & ((col == DEVCOL_OUTROUTES) ? 1 : 2)))    
                      return;
                      
                    RoutePopupMenu* pup = new RoutePopupMenu();
                    pup->exec(QCursor::pos(), dev, col == DEVCOL_OUTROUTES);
                    delete pup;
                  }
                  return;
            break;                    
                    
#endif  // not _USE_EXTRA_INSTANCE_COLUMNS_

            case DEVCOL_DEF_IN_CHANS:
                  {
                    PopupMenu* pup = new PopupMenu(true);
                    pup->addAction(new MenuTitleItem("Channel", pup)); 
                    QAction* act = 0;
                    int chbits = MusEGlobal::midiPorts[no].defaultInChannels();
                    QActionGroup* ag = new QActionGroup(pup);
                    ag->setExclusive(false);
                    for(int i = 0; i < MusECore::MUSE_MIDI_CHANNELS; ++i) 
                    {
                      act = ag->addAction(QString().setNum(i + 1));
                      act->setData(i);
                      act->setCheckable(true);
                      act->setChecked((1 << i) & chbits);
                    }
                    pup->addActions(ag->actions());
                    
                    act = pup->addAction(tr("Toggle all"));
                    act->setData(MusECore::MUSE_MIDI_CHANNELS);
                    
                    pup->addSeparator();
                    act = pup->addAction(tr("Change all tracks now"));
                    act->setData(MusECore::MUSE_MIDI_CHANNELS + 1);
                    // Enable only if there are tracks, and port has a device.
                    // Allow ports with no device since that is a valid situation.
                    act->setEnabled(!MusEGlobal::song->midis()->empty());
                    
                    connect(pup, SIGNAL(triggered(QAction*)), SLOT(changeDefInputRoutes(QAction*)));
                    pup->exec(QCursor::pos());
                    delete pup;
                  }
                  return;                  
            break;                    
                  
            case DEVCOL_DEF_OUT_CHANS:
                  {
                    PopupMenu* pup = new PopupMenu(true);
                    pup->addAction(new MenuTitleItem("Channel", pup)); 
                    QAction* act = 0;
                    int chbits = MusEGlobal::midiPorts[no].defaultOutChannels();
                    QActionGroup* ag = new QActionGroup(pup);
                    ag->setExclusive(true);

                    act = ag->addAction("None");
                    act->setData(0);
                    act->setCheckable(true);
                    act->setChecked(chbits == 0);

                    for(int i = 0; i < MusECore::MUSE_MIDI_CHANNELS; ++i)
                    {
                      act = ag->addAction(QString().setNum(i + 1));
                      act->setData(i);
                      act->setCheckable(true);
                      act->setChecked((1 << i) & chbits);
                    }  
                    pup->addActions(ag->actions());
                    
                    // Turn on if and when multiple output routes are supported.
#ifndef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
                    act = pup->addAction(tr("Toggle all"));
                    act->setData(MusECore::MUSE_MIDI_CHANNELS);
#endif
                    
                    pup->addSeparator();
                    act = pup->addAction(tr("Change all tracks now"));
                    act->setData(MusECore::MUSE_MIDI_CHANNELS + 1);
                    // Enable only if there are tracks, and port has a device.
                    // Allow ports with no device since that is a valid situation.
                    act->setEnabled(!MusEGlobal::song->midis()->empty());
                    
                    connect(pup, SIGNAL(triggered(QAction*)), SLOT(changeDefOutputRoutes(QAction*)));
                    pup->exec(QCursor::pos());
                    delete pup;
                  }
                  return;
            break;                    
                  
            case DEVCOL_NAME:
                  {
                    // We clicked the 'down' button.
                    PopupMenu* pup = new PopupMenu(false);
                    QAction* act;

                    // REMOVE Tim. Persistent routes. Added. Testing...
                    QMenu* ctxmenu = pup->contextMenu();
                    act = ctxmenu->addAction(tr("Remove"));
                    act->setData(0);
                    connect(ctxmenu, SIGNAL(triggered(QAction*)), SLOT(deviceContextTriggered(QAction*)));
                    
                    // Could do it this way...
                    //act = pup->addAction(tr("Create") + QT_TRANSLATE_NOOP("@default", " Jack") + tr(" input"));
                    //act = pup->addAction(tr("Create") + QT_TRANSLATE_NOOP("@default", " Jack") + tr(" output"));
                    //act = pup->addAction(tr("Create") + QT_TRANSLATE_NOOP("@default", " Jack") + tr(" combo"));
                    // ... or keep it simple and let the user click on the green lights instead.
                    act = pup->addAction(tr("Create Jack device"));
                    act->setData(0);
                    
                    typedef std::map<std::string, int > asmap;
                    typedef std::map<std::string, int >::iterator imap;
                    
#ifndef _WIN32
                    asmap mapALSA;
#endif
                    asmap mapJACK;
                    asmap mapSYNTH;
                    
                    int aix = 0x10000000;
                    int jix = 0x20000000;
                    int six = 0x30000000;
                    for(MusECore::iMidiDevice i = MusEGlobal::midiDevices.begin(); i != MusEGlobal::midiDevices.end(); ++i) 
                    {
#ifndef _WIN32
                      if((*i)->deviceType() == MusECore::MidiDevice::ALSA_MIDI)
                      {
                        mapALSA.insert( std::pair<std::string, int> (std::string((*i)->name().toLatin1().constData()), aix) );
                        ++aix;
                      }
                      else
#endif
                      if((*i)->deviceType() == MusECore::MidiDevice::JACK_MIDI)
                      {  
                        mapJACK.insert( std::pair<std::string, int> (std::string((*i)->name().toLatin1().constData()), jix) );
                        ++jix;
                      }
                      else
                      if((*i)->deviceType() == MusECore::MidiDevice::SYNTH_MIDI)
                      {
                        mapSYNTH.insert( std::pair<std::string, int> (std::string((*i)->name().toLatin1().constData()), six) );
                        ++six;  
                      }
                      else
                        fprintf(stderr, "MPConfig::rbClicked unknown midi device: %s\n", (*i)->name().toLatin1().constData());
                    }
                    
#ifndef _WIN32
                    if(!mapALSA.empty())
                    {
                      pup->addSeparator();
                      pup->addAction(new MusEGui::MenuTitleItem("ALSA:", pup));
                      
                      for(imap i = mapALSA.begin(); i != mapALSA.end(); ++i) 
                      {
                        int idx = i->second;
                        QString s(i->first.c_str());
                        MusECore::MidiDevice* md = MusEGlobal::midiDevices.find(s, MusECore::MidiDevice::ALSA_MIDI);
                        if(md)
                        {
                          if(md->deviceType() != MusECore::MidiDevice::ALSA_MIDI)  
                            continue;
                          act = pup->addAction(md->name());
                          act->setData(idx);
                          act->setCheckable(true);
                          act->setChecked(md == dev);
                        }  
                      }  
                    }
#endif
                    
                    if(!mapJACK.empty())
                    {
                      pup->addSeparator();
                      pup->addAction(new MusEGui::MenuTitleItem("JACK:", pup));
                      
                      for(imap i = mapJACK.begin(); i != mapJACK.end(); ++i) 
                      {
                        int idx = i->second;
                        QString s(i->first.c_str());
                        MusECore::MidiDevice* md = MusEGlobal::midiDevices.find(s, MusECore::MidiDevice::JACK_MIDI);
                        if(md)
                        {
                          if(md->deviceType() != MusECore::MidiDevice::JACK_MIDI)  
                            continue;
                            
                          act = pup->addAction(md->name());
                          act->setData(idx);
                          act->setCheckable(true);
                          act->setChecked(md == dev);
                        }  
                      }
                    }
                    
                    if(!mapSYNTH.empty())
                    {
                      pup->addSeparator();
                      pup->addAction(new MusEGui::MenuTitleItem("SYNTH:", pup));
                      
                      for(imap i = mapSYNTH.begin(); i != mapSYNTH.end(); ++i) 
                      {
                        int idx = i->second;
                        QString s(i->first.c_str());
                        MusECore::MidiDevice* md = MusEGlobal::midiDevices.find(s, MusECore::MidiDevice::SYNTH_MIDI);
                        if(md)
                        {
                          if(md->deviceType() != MusECore::MidiDevice::SYNTH_MIDI)  
                            continue;
                            
                          act = pup->addAction(md->name());
                          act->setData(idx);
                          act->setCheckable(true);
                          act->setChecked(md == dev);
                        }  
                      }
                    }  
                    
                    act = pup->exec(ppt);
                    if(!act)
                    {      
                      delete pup;
                      return;
                    }
                    
                    n = act->data().toInt();
                    
                    MusECore::MidiDevice* sdev = 0;
                    if(n < 0x10000000)
                    {
                      if(n <= 2)  
                      {
                        sdev = MusECore::MidiJackDevice::createJackMidiDevice(); 

                        if(sdev)
                        {
                          int of = 3;
                          switch(n)
                          {
                            case 0: of = 3; break;  
                            case 1: of = 2; break;
                            case 2: of = 1; break;
                          }  
                          sdev->setOpenFlags(of);
                        }  
                      }  
                    }  
                    else
                    {
                      int typ;
#ifndef _WIN32
                      if(n < 0x20000000)
                        typ = MusECore::MidiDevice::ALSA_MIDI;
                      else
#endif
                      if(n < 0x30000000)
                        typ = MusECore::MidiDevice::JACK_MIDI;
                      else //if(n < 0x40000000)
                        typ = MusECore::MidiDevice::SYNTH_MIDI;
                      
                      sdev = MusEGlobal::midiDevices.find(act->text(), typ);
                      // Is it the current device? Reset it to <none>.
                      if(sdev == dev)
                        sdev = 0;
                    }    

                    delete pup;
                    
                    MusECore::MidiTrackList* mtl = MusEGlobal::song->midis();
                    for(MusECore::iMidiTrack it = mtl->begin(); it != mtl->end(); ++it)
                    {
                      MusECore::MidiTrack* mt = *it;
                      MusECore::RouteList* rl = mt->inRoutes();
                      for(MusECore::iRoute ir = rl->begin(); ir != rl->end(); ++ir)
                      {
                        switch(ir->type)
                        {
                          case MusECore::Route::MIDI_PORT_ROUTE:
                            // Remove track routes from an existing port already using the selected device.
                            // Remove all track routes from this port.
                            if((sdev && ir->midiPort == sdev->midiPort()) || (ir->midiPort == no))
                              operations.add(MusECore::PendingOperationItem(*ir, MusECore::Route(mt, ir->channel),
                                                                            MusECore::PendingOperationItem::DeleteRoute));
                          break;  
                          
                          case MusECore::Route::TRACK_ROUTE:
                          case MusECore::Route::JACK_ROUTE:
                          case MusECore::Route::MIDI_DEVICE_ROUTE:
                          break;  
                        }
                      }
                      
                      rl = mt->outRoutes();
                      for(MusECore::iRoute ir = rl->begin(); ir != rl->end(); ++ir)
                      {
                        switch(ir->type)
                        {
                          case MusECore::Route::MIDI_PORT_ROUTE:
                            // Remove track routes to an existing port already using the selected device.
                            // Remove all track routes to this port.
                            if((sdev && ir->midiPort == sdev->midiPort()) || (ir->midiPort == no))
                              operations.add(MusECore::PendingOperationItem(MusECore::Route(mt, ir->channel), *ir,
                                                                            MusECore::PendingOperationItem::DeleteRoute));
                          break;  
                          
                          case MusECore::Route::TRACK_ROUTE:
                          case MusECore::Route::JACK_ROUTE:
                          case MusECore::Route::MIDI_DEVICE_ROUTE:
                          break;  
                        }
                      }
                    }
                    
                    MusEGlobal::audio->msgSetMidiDevice(port, sdev);
                    // Save settings. Use simple version - do NOT set style or stylesheet, this has nothing to do with that.
                    MusEGlobal::muse->changeConfig(true);     // save configuration file
                    
                    // Add all track routes to/from this port...
                    if(sdev)
                    {  
                      const int allch = (1 << MusECore::MUSE_MIDI_CHANNELS) - 1;  
                      const int i_chbits = MusEGlobal::midiPorts[no].defaultInChannels();
                      const int o_chbits = MusEGlobal::midiPorts[no].defaultOutChannels();
                      // Connect all the specified routes. Do not add input routes to synths.
                      if((i_chbits || o_chbits) && !sdev->isSynti())
                      {
                        for(MusECore::iMidiTrack it = mtl->begin(); it != mtl->end(); ++it)
                        {
                          MusECore::MidiTrack* mt = *it;
                          // All channels set or Omni? Use an Omni route:
                          if(i_chbits == -1 || i_chbits == allch)
                            operations.add(MusECore::PendingOperationItem(MusECore::Route(no), MusECore::Route(mt),
                                                                          MusECore::PendingOperationItem::AddRoute));
                          else
                          // Add individual channels:  
                          for(int ch = 0; ch < MusECore::MUSE_MIDI_CHANNELS; ++ch)
                          {
                            const int chbit = 1 << ch;
                            if(i_chbits & chbit)
                              operations.add(MusECore::PendingOperationItem(MusECore::Route(no, ch), MusECore::Route(mt, ch),
                                                                            MusECore::PendingOperationItem::AddRoute));
                          }
                          
#ifndef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
                          // All channels set or Omni? Use an Omni route:
                          if(o_chbits == -1 || o_chbits == allch)
                            operations.add(MusECore::PendingOperationItem(MusECore::Route(mt), MusECore::Route(no),
                                                                          MusECore::PendingOperationItem::AddRoute));
                          else
                          // Add individual channels:  
                          for(int ch = 0; ch < MusECore::MUSE_MIDI_CHANNELS; ++ch)
                          {
                            const int chbit = 1 << ch;
                            if(o_chbits & chbit)
                              operations.add(MusECore::PendingOperationItem(MusECore::Route(mt, ch), MusECore::Route(no, ch),
                                                                            MusECore::PendingOperationItem::AddRoute));
                          }
#endif
                          
                        }  
                      }
                    }
                  }
                  
                  // Do these always, regardless of operations - the device has changed.
                  operations.add(MusECore::PendingOperationItem((MusECore::TrackList*)NULL, MusECore::PendingOperationItem::UpdateSoloStates));
                  //if(!operations.empty())
                    MusEGlobal::audio->msgExecutePendingOperations(operations, true, SC_EVERYTHING);
//                   MusEGlobal::song->update();
                  
                  return;
            break;                    

            case DEVCOL_INSTR:
                  {
                  if (dev && dev->isSynti())
                        return;
                  PopupMenu* pup = new PopupMenu(false);
                  MusECore::MidiInstrument::populateInstrPopup(pup, port->instrument(), false);   
                  
                  if(pup->actions().count() == 0)
                  {
                    delete pup;
                    return;
                  }  
                  
                  QAction* act = pup->exec(ppt);
                  if(!act)
                    return;
                  
                  QString s = act->text();
                  delete pup;
                  
                  item->tableWidget()->item(item->row(), DEVCOL_INSTR)->setText(s);
                  for (MusECore::iMidiInstrument i = MusECore::midiInstruments.begin(); i
                     != MusECore::midiInstruments.end(); ++i) {
                        if ((*i)->iname() == s) {
                              MusEGlobal::audio->msgIdle(true); // Make it safe to edit structures
                              port->changeInstrument(*i);
                              MusEGlobal::audio->msgIdle(false);
                              break;
                              }
                        }
                  MusEGlobal::song->update(SC_MIDI_INSTRUMENT);
                  }
                  return;
            break;                    
            }
      }

//---------------------------------------------------------
//   MPConfig::setToolTip
//---------------------------------------------------------

void MPConfig::setToolTip(QTableWidgetItem *item, int col)
      {
      switch (col) {
            case DEVCOL_NO:     item->setToolTip(tr("Port Number")); break;
            
#ifndef _USE_EXTRA_INSTANCE_COLUMNS_
            case DEVCOL_GUI:    item->setToolTip(tr("Enable gui")); break;
            case DEVCOL_REC:    item->setToolTip(tr("Enable reading")); break;
            case DEVCOL_PLAY:   item->setToolTip(tr("Enable writing")); break;
            case DEVCOL_INROUTES:  item->setToolTip(tr("Connections from Jack Midi outputs")); break;
            case DEVCOL_OUTROUTES: item->setToolTip(tr("Connections to Jack Midi inputs")); break;
            case DEVCOL_STATE:  item->setToolTip(tr("Device state")); break;
#endif      
            
            case DEVCOL_INSTR:  item->setToolTip(tr("Port instrument")); break;
            case DEVCOL_NAME:   item->setToolTip(tr("Midi device name. Click to edit (Jack)")); break;
            case DEVCOL_DEF_IN_CHANS:   item->setToolTip(tr("Auto-connect these channels to new midi tracks")); break;
            
#ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
            case DEVCOL_DEF_OUT_CHANS:  item->setToolTip(tr("Auto-connect new midi tracks to this channel")); break;
#else
            case DEVCOL_DEF_OUT_CHANS:  item->setToolTip(tr("Auto-connect new midi tracks to these channels")); break;
#endif
            
            default: return;
            }
  }

//---------------------------------------------------------
//   MPConfig::setWhatsThis
//---------------------------------------------------------

void MPConfig::setWhatsThis(QTableWidgetItem *item, int col)
      {
      switch (col) {
            case DEVCOL_NO:
                  item->setWhatsThis(tr("Port Number")); break;
                  
#ifndef _USE_EXTRA_INSTANCE_COLUMNS_
            case DEVCOL_GUI:
                  item->setWhatsThis(tr("Enable gui for device")); break;
            case DEVCOL_REC:
                  item->setWhatsThis(tr("Enable reading from device")); break;
            case DEVCOL_PLAY:
                  item->setWhatsThis(tr("Enable writing to device")); break;
            case DEVCOL_INROUTES:
                  item->setWhatsThis(tr("Connections from Jack Midi output ports")); break;
            case DEVCOL_OUTROUTES:
                  item->setWhatsThis(tr("Connections to Jack Midi input ports")); break;
            case DEVCOL_STATE:
                  item->setWhatsThis(tr("State: result of opening the device")); break;
#endif
                  
            case DEVCOL_NAME:
                  item->setWhatsThis(tr("Name of the midi device associated with"
                                        " this port number. Click to edit Jack midi name.")); break;
            case DEVCOL_INSTR:
                  item->setWhatsThis(tr("Instrument connected to port")); break;
            case DEVCOL_DEF_IN_CHANS:
                  item->setWhatsThis(tr("Auto-connect these channels, on this port, to new midi tracks.")); break;
            case DEVCOL_DEF_OUT_CHANS:
#ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
                  item->setWhatsThis(tr("Connect new midi tracks to this channel, on this port.")); break;
#else                      
                  item->setWhatsThis(tr("Connect new midi tracks to these channels, on this port.")); break;
#endif                      
            default:
                  break;
            }
      }

//---------------------------------------------------------
//   MPConfig::setInstToolTip
//---------------------------------------------------------

void MPConfig::setInstToolTip(QTableWidgetItem *item, int col)
      {
      switch (col) {
            case INSTCOL_NAME:      item->setToolTip(tr("Midi device name")); break;
            case INSTCOL_TYPE:      item->setToolTip(tr("Midi device type")); break;
            case INSTCOL_STATE:     item->setToolTip(tr("Device state")); break;

#ifdef _USE_EXTRA_INSTANCE_COLUMNS_
            case INSTCOL_REC:       item->setToolTip(tr("Enable reading")); break;
            case INSTCOL_PLAY:      item->setToolTip(tr("Enable writing")); break;
            case INSTCOL_GUI:       item->setToolTip(tr("Enable gui")); break;
            case INSTCOL_INROUTES:  item->setToolTip(tr("Connections from Jack Midi")); break;
            case INSTCOL_OUTROUTES: item->setToolTip(tr("Connections to Jack Midi")); break;
#endif      
            
            default:
                  break;
      }
  }

//---------------------------------------------------------
//   MPConfig::setInstWhatsThis
//---------------------------------------------------------

void MPConfig::setInstWhatsThis(QTableWidgetItem *item, int col)
      {
      switch (col) {
            case INSTCOL_NAME:      item->setWhatsThis(tr("Midi device name")); break;
            case INSTCOL_TYPE:      item->setWhatsThis(tr("Midi device type")); break;
            case INSTCOL_STATE:     item->setWhatsThis(tr("Result of opening the device:\n"
                                                          "OK: Assigned to a port and in use\n"
                                                          "Closed: Unassigned to a port, or closed\n"
                                                          "R/W Error: Unable to open for read or write\n"
                                                          "Unavailable: USB midi unplugged, or external\n"
                                                          " application not running, or synth plugin\n"
                                                          " not installed etc.\n"
                                                          "(Jack Midi devices have 'unavailable ports'\n"
                                                          " in the routes columns.)\n"
                                                          "Unavailable devices or ports can be purged\n"
                                                          " with 'Remove' or with the advanced router.")); break;

#ifdef _USE_EXTRA_INSTANCE_COLUMNS_
            case INSTCOL_REC:       item->setWhatsThis(tr("Enable reading from device")); break;
            case INSTCOL_PLAY:      item->setWhatsThis(tr("Enable writing to device")); break;
            case INSTCOL_GUI:       item->setWhatsThis(tr("Enable Graphical User Interface for device")); break;
            case INSTCOL_INROUTES:  item->setWhatsThis(tr("Connections from Jack Midi ports")); break;
            case INSTCOL_OUTROUTES: item->setWhatsThis(tr("Connections to Jack Midi ports")); break;
#endif      
            
            default:
                  break;
            }
      }

//---------------------------------------------------------
//   MPConfig::addItem()
//---------------------------------------------------------

void MPConfig::addItem(int row, int col, QTableWidgetItem *item, QTableWidget *table)
      {
      setWhatsThis(item, col);
      table->setItem(row, col, item);
      }

//---------------------------------------------------------
//   MPConfig::addInstItem()
//---------------------------------------------------------

void MPConfig::addInstItem(int row, int col, QTableWidgetItem *item, QTableWidget *table)
      {
      setInstWhatsThis(item, col);
      table->setItem(row, col, item);
      }


//---------------------------------------------------------
//   MPConfig
//    Midi Port Config
//---------------------------------------------------------

MPConfig::MPConfig(QWidget* parent)
   : QDialog(parent)
      {
      setupUi(this);
      QSettings settings("MusE", "MusE-qt");
      restoreGeometry(settings.value("MPConfig/geometry").toByteArray());

      mdevView->setRowCount(MusECore::MIDI_PORTS);
      mdevView->verticalHeader()->hide();
      mdevView->setShowGrid(false);

      _showAliases = 1; // 0: Show second aliases, if available. 
      
      QStringList columnnames;
      columnnames << tr("Port")
#ifndef _USE_EXTRA_INSTANCE_COLUMNS_
		  << tr("GUI")
		  << tr("I")
		  << tr("O")
#endif                  
                  << tr("Device Name")
		  << tr("Instrument")
#ifndef _USE_EXTRA_INSTANCE_COLUMNS_
		  << tr("In routes")
		  << tr("Out routes")
#endif                  
                  << tr("Def in ch")
                  << tr("Def out ch")
#ifndef _USE_EXTRA_INSTANCE_COLUMNS_
		  << tr("State")
#endif                  
      ;
      
      mdevView->setColumnCount(columnnames.size());
      mdevView->setHorizontalHeaderLabels(columnnames);
      for (int i = 0; i < columnnames.size(); ++i) {
            setWhatsThis(mdevView->horizontalHeaderItem(i), i);
            setToolTip(mdevView->horizontalHeaderItem(i), i);
            }
      mdevView->setFocusPolicy(Qt::NoFocus);

      
      instanceList->verticalHeader()->hide();
      instanceList->setShowGrid(false);
      columnnames.clear();
      columnnames << tr("Device Name")
                  << tr("Type")
#ifdef _USE_EXTRA_INSTANCE_COLUMNS_
                  << tr("I")
                  << tr("O")
                  << tr("GUI")
                  << tr("In")
                  << tr("Out")
                  << tr("State")
#endif                  
      ;

#ifndef _WIN32
      addALSADevice->setChecked(MusEGlobal::midiSeq != NULL);
#endif

      instanceList->setColumnCount(columnnames.size());
      instanceList->setHorizontalHeaderLabels(columnnames);
      for (int i = 0; i < columnnames.size(); ++i) {
            setInstWhatsThis(instanceList->horizontalHeaderItem(i), i);
            setInstToolTip(instanceList->horizontalHeaderItem(i), i);
            }
      connect(instanceList, SIGNAL(itemPressed(QTableWidgetItem*)), SLOT(deviceItemClicked(QTableWidgetItem*)));
      connect(instanceList, SIGNAL(itemSelectionChanged()),         SLOT(deviceSelectionChanged()));
      connect(instanceList, SIGNAL(itemChanged(QTableWidgetItem*)), SLOT(DeviceItemRenamed(QTableWidgetItem*)));
      connect(addJACKDevice, SIGNAL(clicked(bool)), SLOT(addJackDeviceClicked()));
#ifndef _WIN32
      connect(addALSADevice, SIGNAL(clicked(bool)), SLOT(addAlsaDeviceClicked(bool)));
#endif
      connect(mdevView, SIGNAL(itemPressed(QTableWidgetItem*)), this, SLOT(rbClicked(QTableWidgetItem*)));
      connect(MusEGlobal::song, SIGNAL(songChanged(MusECore::SongChangedStruct_t)), SLOT(songChanged(MusECore::SongChangedStruct_t)));
      connect(synthList, SIGNAL(itemSelectionChanged()), SLOT(selectionChanged()));
      connect(addSynthDevice, SIGNAL(clicked()), SLOT(addInstanceClicked()));
      connect(synthList, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), SLOT(addInstanceClicked())); 
      connect(renameDevice, SIGNAL(clicked()), SLOT(renameInstanceClicked()));
      connect(removeDevice, SIGNAL(clicked()), SLOT(removeInstanceClicked()));
      connect(applyButton, SIGNAL(clicked()), SLOT(apply()));
      connect(okButton, SIGNAL(clicked()), SLOT(okClicked()));
      
      songChanged(SC_CONFIG);  
      }

  
MPConfig::~MPConfig()
{
}
  
//---------------------------------------------------------
//   selectionChanged
//---------------------------------------------------------

void MPConfig::selectionChanged()
      {
      addSynthDevice->setEnabled(synthList->selectedItems().isEmpty() ? false : (bool)synthList->currentItem());
      }

//---------------------------------------------------------
//   deviceSelectionChanged
//---------------------------------------------------------

void MPConfig::deviceSelectionChanged()
{
  DEBUG_PRST_ROUTES(stderr, "synthesizerConfig::deviceSelectionChanged() currentItem:%p\n", instanceList->currentItem());
  bool can_remove = false;
  bool can_rename = false;

  int rowSelCount = 0;
  const int sz = instanceList->rowCount();
  for(int i = 0; i < sz; ++i)
  {
    QTableWidgetItem* item = instanceList->item(i, INSTCOL_NAME);
    if(!item || !item->data(DeviceRole).canConvert<void*>() || !item->isSelected())
      continue;

    MusECore::MidiDevice* md = static_cast<MusECore::MidiDevice*>(item->data(DeviceRole).value<void*>());
    if(!md)
      continue;

    ++rowSelCount;

    switch(md->deviceType())
    {
      // TODO: For now, don't allow creating/removing/renaming them until we decide on addressing strategy.
#ifndef _WIN32
      case MusECore::MidiDevice::ALSA_MIDI:
        // Allow removing ('purging') an unavailable ALSA device.
        if(md->isAddressUnknown())
          can_remove = true;
      break;
#endif

      case MusECore::MidiDevice::JACK_MIDI:
        can_remove = true;
        can_rename = true;
      break;

      case MusECore::MidiDevice::SYNTH_MIDI:
        can_remove = true;
      break;
    }

    // Optimize: No need to check further.
    if(can_rename && can_remove && rowSelCount >= 2)
      break;
  }
  
  // Only one rename at a time, for now...
  renameDevice->setEnabled(can_rename && rowSelCount == 1);
  removeDevice->setEnabled(can_remove);
}

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void MPConfig::songChanged(MusECore::SongChangedStruct_t flags)
      {
      if(!(flags._flags & (SC_CONFIG | SC_TRACK_INSERTED | SC_TRACK_REMOVED | SC_TRACK_MODIFIED | SC_MIDI_INSTRUMENT)))
        return;
    
#ifndef _WIN32
      addALSADevice->blockSignals(true);
      addALSADevice->setChecked(MusEGlobal::midiSeq != NULL);
      addALSADevice->blockSignals(false);
#endif

      // Get currently selected index...
      int no = -1;
      QTableWidgetItem* sitem = mdevView->currentItem();
      if(sitem)
      {
        QString id = sitem->tableWidget()->item(sitem->row(), DEVCOL_NO)->text();
        no = atoi(id.toLatin1().constData()) - 1;
        if(no < 0 || no >= MusECore::MIDI_PORTS)
          no = -1;
      }
      
      sitem = 0;
      mdevView->blockSignals(true);
      mdevView->clearContents();
      int defochs = 0;
      for (int i = MusECore::MIDI_PORTS-1; i >= 0; --i) 
      {
            mdevView->blockSignals(true); // otherwise itemChanged() is triggered and bad things happen.
            MusECore::MidiPort* port  = &MusEGlobal::midiPorts[i];
            MusECore::MidiDevice* dev = port->device();
            QString s;
            s.setNum(i+1);
            QTableWidgetItem* itemno = new QTableWidgetItem(s);
            addItem(i, DEVCOL_NO, itemno, mdevView);
            itemno->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            itemno->setFlags(Qt::ItemIsEnabled);
            
#ifndef _USE_EXTRA_INSTANCE_COLUMNS_
            QTableWidgetItem* itemstate = new QTableWidgetItem(port->state());
            addItem(i, DEVCOL_STATE, itemstate, mdevView);
            itemstate->setFlags(Qt::ItemIsEnabled);
#endif
            
            QTableWidgetItem* iteminstr = new QTableWidgetItem(port->instrument() ?
                           port->instrument()->iname() :
                           tr("<unknown>"));
            addItem(i, DEVCOL_INSTR, iteminstr, mdevView);
            iteminstr->setFlags(Qt::ItemIsEnabled);
            
            QTableWidgetItem* itemname = new QTableWidgetItem;
            addItem(i, DEVCOL_NAME, itemname, mdevView);
            itemname->setFlags(Qt::ItemIsEnabled);
            
#ifndef _USE_EXTRA_INSTANCE_COLUMNS_
            QTableWidgetItem* itemgui = new QTableWidgetItem;
            addItem(i, DEVCOL_GUI, itemgui, mdevView);
            itemgui->setTextAlignment(Qt::AlignCenter);
            itemgui->setFlags(Qt::ItemIsEnabled);
            QTableWidgetItem* itemrec = new QTableWidgetItem;
            addItem(i, DEVCOL_REC, itemrec, mdevView);
            itemrec->setTextAlignment(Qt::AlignCenter);
            itemrec->setFlags(Qt::ItemIsEnabled);
            QTableWidgetItem* itemplay = new QTableWidgetItem;
            addItem(i, DEVCOL_PLAY, itemplay, mdevView);
            itemplay->setTextAlignment(Qt::AlignCenter);
            itemplay->setFlags(Qt::ItemIsEnabled);
            QTableWidgetItem* itemout = new QTableWidgetItem;
            addItem(i, DEVCOL_OUTROUTES, itemout, mdevView);
            itemout->setFlags(Qt::ItemIsEnabled);
            QTableWidgetItem* itemin = new QTableWidgetItem;
            addItem(i, DEVCOL_INROUTES, itemin, mdevView);
            itemin->setFlags(Qt::ItemIsEnabled);
#endif
            
            // Ignore synth devices. Default input routes make no sense for them (right now).
            QTableWidgetItem* itemdefin = new QTableWidgetItem((dev && dev->isSynti()) ? 
                                               QString() : MusECore::bitmap2String(port->defaultInChannels()));
            addItem(i, DEVCOL_DEF_IN_CHANS, itemdefin, mdevView);
            // Enabled: Use editor (not good). Disabled: Use pop-up menu.
            #if 0
            itemdefin->setFlags((dev && dev->isSynti()) ? Qt::NoItemFlags : Qt::ItemIsEditable | Qt::ItemIsEnabled);
            # else
            if(dev && dev->isSynti())
              itemdefin->setFlags(Qt::NoItemFlags);
            else
            {
              itemdefin->setFlags(Qt::ItemIsEnabled);
              itemdefin->setIcon(QIcon(*buttondownIcon));
            }  
            #endif
            
#ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
            QTableWidgetItem* itemdefout = new QTableWidgetItem(MusECore::bitmap2String(0));
            defochs = port->defaultOutChannels();
            if(defochs)
            {
              for(int ch = 0; ch < MusECore::MUSE_MIDI_CHANNELS; ++ch)
              {
                if(defochs & (1 << ch))
                {
                  itemdefout->setText(QString().setNum(ch + 1));
                  break;
                }
              }
            }  
            addItem(i, DEVCOL_DEF_OUT_CHANS, itemdefout, mdevView);
            itemdefout->setFlags(Qt::ItemIsEnabled);
            itemdefout->setIcon(QIcon(*buttondownIcon));
#else
            QTableWidgetItem* itemdefout = new QTableWidgetItem(MusECore::bitmap2String(port->defaultOutChannels()));
            addItem(i, DEVCOL_DEF_OUT_CHANS, itemdefout, mdevView);
            itemdefout->setFlags(Qt::ItemIsEditable | Qt::ItemIsEnabled);
#endif
            
            if(dev && dev->isSynti()) //make deleted audio softsynths not show in ports dialog
            {
               MusECore::AudioTrack *_track = static_cast<MusECore::AudioTrack *>(static_cast<MusECore::SynthI *>(dev));
               MusECore::TrackList* tl = MusEGlobal::song->tracks();
               if(tl->find(_track) == tl->end())
               {
                  for(int __col = 0; __col  < mdevView->columnCount(); ++__col)
                  {
                     mdevView->item(i, __col)->setFlags(Qt::NoItemFlags);
                  }
               }
            }
            mdevView->blockSignals(false);

            if (dev) {
	          itemname->setText(dev->name());

#ifndef _USE_EXTRA_INSTANCE_COLUMNS_
                  if (dev->rwFlags() & 0x2)
                       itemrec->setIcon(dev->openFlags() & 2 ? QIcon(*dotIcon) : QIcon(*dothIcon));
                  else
                       itemrec->setIcon(QIcon(QPixmap()));
                  if (dev->rwFlags() & 0x1)
                       itemplay->setIcon( dev->openFlags() & 1 ? QIcon(*dotIcon) : QIcon(*dothIcon));
                  else
                       itemplay->setIcon(QIcon(QPixmap()));
#endif                  
                  }
            else {
                  itemname->setText(tr("<none>"));
#ifndef _USE_EXTRA_INSTANCE_COLUMNS_
                  itemgui->setIcon(QIcon(*dothIcon));
                  itemrec->setIcon(QIcon(QPixmap()));
                  itemplay->setIcon(QIcon(QPixmap()));
#endif                  
                  }
                  
#ifndef _USE_EXTRA_INSTANCE_COLUMNS_
            if (port->hasNativeGui())
                  itemgui->setIcon(port->nativeGuiVisible() ? QIcon(*dotIcon) : QIcon(*dothIcon));
            else
                  itemgui->setIcon(QIcon(QPixmap()));
            
            if(dev && dev->deviceType() == MusECore::MidiDevice::JACK_MIDI)
            {
              if(dev->rwFlags() & 1)  
              {
                if(dev->openFlags() & 1)  
                {
                  itemout->setIcon(QIcon(*buttondownIcon));
                  itemout->setText(tr("out"));
                }  
              }  
              if(dev->rwFlags() & 2)  
              {
                if(dev->openFlags() & 2)  
                {
                  itemin->setIcon(QIcon(*buttondownIcon));
                  itemin->setText(tr("in"));
                }  
              }  
            }
#endif                  

            if (!(dev && dev->isSynti()))
                  iteminstr->setIcon(QIcon(*buttondownIcon));

            itemname->setIcon(QIcon(*buttondownIcon));

            if(i == no) sitem = itemno;
      }
      if(sitem)
         mdevView->setCurrentItem(sitem);
      mdevView->blockSignals(false);
      
      QString s;
      synthList->blockSignals(true);
      synthList->clear();
      for (std::vector<MusECore::Synth*>::iterator i = MusEGlobal::synthis.begin();
         i != MusEGlobal::synthis.end(); ++i) {
            QTreeWidgetItem* item = new QTreeWidgetItem(synthList);
            item->setText(0, QString((*i)->baseName()));
            item->setText(1, MusECore::synthType2String((*i)->synthType()));
            s.setNum((*i)->instances());
            item->setText(2, s);
            item->setText(3, QString((*i)->name()));
            
            item->setText(4, QString((*i)->version()));
            item->setText(5, QString((*i)->description()));
            }
      synthList->blockSignals(false);
      instanceList->blockSignals(true);
      instanceList->clearContents();
      instanceList->setRowCount(MusEGlobal::midiDevices.size());
      int row_cnt = 0;
      for (MusECore::iMidiDevice imd = MusEGlobal::midiDevices.begin(); imd != MusEGlobal::midiDevices.end(); ++imd) {
            MusECore::MidiDevice* md = *imd;
            MusECore::SynthI* synth = 0;
            if(md->isSynti())
              synth = static_cast<MusECore::SynthI*>(md);
            QTableWidgetItem* iitem = new QTableWidgetItem(md->name());
            iitem->setData(DeviceRole, QVariant::fromValue<void*>(md));
            // Is it a Jack midi device? Allow renaming.
            if(md->deviceType() == MusECore::MidiDevice::JACK_MIDI)
              iitem->setFlags(Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
            else
              iitem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
            addInstItem(row_cnt, INSTCOL_NAME, iitem, instanceList);
            iitem = new QTableWidgetItem(md->deviceTypeString());
            iitem->setData(DeviceRole, QVariant::fromValue<void*>(md));
            iitem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
            addInstItem(row_cnt, INSTCOL_TYPE, iitem, instanceList);

#ifdef _USE_EXTRA_INSTANCE_COLUMNS_
            iitem = new QTableWidgetItem;
            iitem->setData(DeviceRole, QVariant::fromValue<void*>(md));
            iitem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
            iitem->setTextAlignment(Qt::AlignCenter);
            addInstItem(row_cnt, INSTCOL_REC, iitem, instanceList);
            if(md->rwFlags() & 0x2)
              iitem->setIcon(md->openFlags() & 2 ? QIcon(*dotIcon) : QIcon(*dothIcon));
            else
              iitem->setIcon(QIcon(QPixmap()));
            
            iitem = new QTableWidgetItem;
            iitem->setData(DeviceRole, QVariant::fromValue<void*>(md));
            iitem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
            iitem->setTextAlignment(Qt::AlignCenter);
            addInstItem(row_cnt, INSTCOL_PLAY, iitem, instanceList);
            if(md->rwFlags() & 0x1)
              iitem->setIcon(md->openFlags() & 1 ? QIcon(*dotIcon) : QIcon(*dothIcon));
            else
              iitem->setIcon(QIcon(QPixmap()));
            
            iitem = new QTableWidgetItem;
            iitem->setData(DeviceRole, QVariant::fromValue<void*>(md));
            iitem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
            iitem->setTextAlignment(Qt::AlignCenter);
            addInstItem(row_cnt, INSTCOL_GUI, iitem, instanceList);
            if(synth && synth->hasNativeGui())
              iitem->setIcon(synth->nativeGuiVisible() ? QIcon(*dotIcon) : QIcon(*dothIcon));
            else
              iitem->setIcon(QIcon(QPixmap()));

            QTableWidgetItem* or_item = new QTableWidgetItem;
            or_item->setData(DeviceRole, QVariant::fromValue<void*>(md));
            or_item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
            QTableWidgetItem* ir_item = new QTableWidgetItem;
            ir_item->setData(DeviceRole, QVariant::fromValue<void*>(md));
            ir_item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
            if(md->deviceType() == MusECore::MidiDevice::JACK_MIDI)
            {
              if(md->rwFlags() & 1)  
                or_item->setIcon(QIcon(*routesMidiOutIcon));
              else  
                or_item->setIcon(QIcon());
              
              if(md->rwFlags() & 2)  
                ir_item->setIcon(QIcon(*routesMidiInIcon));
              else
                ir_item->setIcon(QIcon());
            }
            addInstItem(row_cnt, INSTCOL_OUTROUTES, or_item, instanceList);
            addInstItem(row_cnt, INSTCOL_INROUTES, ir_item, instanceList);
            
            iitem = new QTableWidgetItem(md->state());
            iitem->setData(DeviceRole, QVariant::fromValue<void*>(md));
            iitem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
            iitem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            addInstItem(row_cnt, INSTCOL_STATE, iitem, instanceList);
#endif            

            ++row_cnt;
            }
      instanceList->blockSignals(false);

      instanceList->resizeColumnToContents(INSTCOL_NAME);
      instanceList->resizeColumnToContents(INSTCOL_TYPE);
      instanceList->resizeColumnToContents(INSTCOL_STATE); 
      
#ifdef _USE_EXTRA_INSTANCE_COLUMNS_
      instanceList->resizeColumnToContents(INSTCOL_REC);
      instanceList->resizeColumnToContents(INSTCOL_PLAY);
      instanceList->resizeColumnToContents(INSTCOL_GUI);
      instanceList->resizeColumnToContents(INSTCOL_OUTROUTES);
      instanceList->resizeColumnToContents(INSTCOL_INROUTES);
      instanceList->horizontalHeader()->setSectionResizeMode(INSTCOL_REC, QHeaderView::Fixed);
      instanceList->horizontalHeader()->setSectionResizeMode(INSTCOL_PLAY, QHeaderView::Fixed);
      instanceList->horizontalHeader()->setSectionResizeMode(INSTCOL_GUI, QHeaderView::Fixed);
      instanceList->horizontalHeader()->setSectionResizeMode(INSTCOL_OUTROUTES, QHeaderView::Fixed);
      instanceList->horizontalHeader()->setSectionResizeMode(INSTCOL_INROUTES, QHeaderView::Fixed);
#endif
      
      //instanceList->horizontalHeader()->setStretchLastSection( false );
      instanceList->horizontalHeader()->setSectionResizeMode(INSTCOL_STATE, QHeaderView::Stretch);
      instanceList->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
      deviceSelectionChanged();
      
      synthList->resizeColumnToContents(1);
      mdevView->resizeColumnsToContents();
      mdevView->horizontalHeader()->setSectionResizeMode(DEVCOL_NO ,QHeaderView::Fixed);
      
#ifndef _USE_EXTRA_INSTANCE_COLUMNS_
      mdevView->horizontalHeader()->setSectionResizeMode(DEVCOL_REC ,QHeaderView::Fixed);
      mdevView->horizontalHeader()->setSectionResizeMode(DEVCOL_PLAY ,QHeaderView::Fixed);
      mdevView->horizontalHeader()->setSectionResizeMode(DEVCOL_GUI ,QHeaderView::Fixed);
#endif
      
      mdevView->horizontalHeader()->setStretchLastSection( true );
      selectionChanged();
      }

//---------------------------------------------------------
//   addInstanceClicked
//---------------------------------------------------------

void MPConfig::addInstanceClicked()
      {
      QTreeWidgetItem* item = synthList->currentItem();
      if (item == 0)
            return;
      // Add at end of list.
      MusECore::SynthI *si = MusEGlobal::song->createSynthI(item->text(0), 
                                                            item->text(3), 
                                                            MusECore::string2SynthType(item->text(1))); 
      if(!si)
        return;

      // add instance last in midi device list
      for (int i = 0; i < MusECore::MIDI_PORTS; ++i) {
            MusECore::MidiPort* port  = &MusEGlobal::midiPorts[i];
            MusECore::MidiDevice* dev = port->device();
            if (dev==0) {
                  MusEGlobal::audio->msgSetMidiDevice(port, si);
                  // Save settings. Use simple version - do NOT set style or stylesheet, this has nothing to do with that.
                  MusEGlobal::muse->changeConfig(true);     // save configuration file
                  MusEGlobal::song->update();
                  break;
                  }
            }
      }

//---------------------------------------------------------
//   removeInstanceClicked
//---------------------------------------------------------

void MPConfig::removeInstanceClicked()
{
  const int sz = instanceList->rowCount();
  if(sz == 0)
    return;

  bool doupd = false;

  // Two passes: One for synths and one for all others (so far).

  //
  // Others:
  //
  bool isIdle = false;
  for(int i = 0; i < sz; ++i)
  {
    QTableWidgetItem* item = instanceList->item(i, INSTCOL_NAME);
    if(!item || !item->data(DeviceRole).canConvert<void*>() || !item->isSelected())
      continue;
    MusECore::MidiDevice* md = static_cast<MusECore::MidiDevice*>(item->data(DeviceRole).value<void*>());
    if(!md)
      continue;

    switch(md->deviceType())
    {
      // TODO: For now, don't allow creating/removing/renaming them until we decide on addressing strategy.
#ifndef _WIN32
      case MusECore::MidiDevice::ALSA_MIDI:
        // Allow removing ('purging') an unavailable ALSA device.
        if(!md->isAddressUnknown())
          break;
#endif
      // Fall through.
      case MusECore::MidiDevice::JACK_MIDI:
        if(!isIdle)
        {
          MusEGlobal::audio->msgIdle(true); // Make it safe to edit structures
          isIdle = true;
        }
        if(md->midiPort() != -1)
          MusEGlobal::midiPorts[md->midiPort()].setMidiDevice(0);
        //MusEGlobal::midiDevices.erase(imd);
        MusEGlobal::midiDevices.remove(md);
      break;

      case MusECore::MidiDevice::SYNTH_MIDI:
      break;
    }
  }
  if(isIdle)
  {
    MusEGlobal::audio->msgIdle(false);
    // Defer update until end, otherwise instanceList is wiped and
    //  rebuilt upon songChanged so next section won't work!
    doupd = true;
  }

  //
  // Synths:
  //
  MusECore::Undo operations;
  for(int i = 0; i < sz; ++i)
  {
    QTableWidgetItem* item = instanceList->item(i, INSTCOL_NAME);
    if(!item || !item->data(DeviceRole).canConvert<void*>() || !item->isSelected())
      continue;
    MusECore::MidiDevice* md = static_cast<MusECore::MidiDevice*>(item->data(DeviceRole).value<void*>());
    if(!md)
      continue;

    switch(md->deviceType())
    {
#ifndef _WIN32
      case MusECore::MidiDevice::ALSA_MIDI:
#endif
      case MusECore::MidiDevice::JACK_MIDI:
      break;

      case MusECore::MidiDevice::SYNTH_MIDI:
      {
        MusECore::SynthI* s = dynamic_cast<MusECore::SynthI*>(md);
        if(s)
          operations.push_back(MusECore::UndoOp(MusECore::UndoOp::DeleteTrack, MusEGlobal::song->tracks()->index(s), s));
      }
      break;
    }
  }
  if(!operations.empty())
    MusEGlobal::song->applyOperationGroup(operations);

  if(doupd)
    MusEGlobal::song->update(SC_CONFIG);
}

//---------------------------------------------------------
//   renameInstanceClicked
//---------------------------------------------------------

void MPConfig::renameInstanceClicked()
{
  QTableWidgetItem* item = instanceList->currentItem();
  if(!item)
    return;
  item = instanceList->item(item->row(), INSTCOL_NAME);
  if(!item)
    return;
  // FIXME: How to know if the table is already in edit mode? The useful state() method is protected,
  //         and there don't appear to be any signals we can use.
  if(item->flags().testFlag(Qt::ItemIsEditable) && item->flags().testFlag(Qt::ItemIsEnabled))
    instanceList->editItem(item);
}

//---------------------------------------------------------
//   deviceItemClicked
//---------------------------------------------------------

void MPConfig::deviceItemClicked(QTableWidgetItem* item)
{
      if(!item)
        return;
      const int col = item->column();
            
#ifdef _USE_EXTRA_INSTANCE_COLUMNS_
      if(!item->data(DeviceRole).canConvert<void*>())
        return;
      MusECore::MidiDevice* md = static_cast<MusECore::MidiDevice*>(item->data(DeviceRole).value<void*>());
      MusECore::SynthI* synth = 0;
      if(md->isSynti())
        synth = static_cast<MusECore::SynthI*>(md);
      int rwFlags   = md->rwFlags();
      int openFlags = md->openFlags();
#endif        
      
      switch(col)
      {
        
#ifdef _USE_EXTRA_INSTANCE_COLUMNS_
        case INSTCOL_REC:
                  if(!(rwFlags & 2))
                        return;
                  openFlags ^= 0x2;
                  MusEGlobal::audio->msgIdle(true);  // Make it safe to edit structures
                  md->setOpenFlags(openFlags);
                  if(md->midiPort() != -1)
                    MusEGlobal::midiPorts[md->midiPort()].setMidiDevice(md); // reopen device // FIXME: This causes jack crash with R+W Jack midi device
                  MusEGlobal::audio->msgIdle(false);
                  item->setIcon(openFlags & 2 ? QIcon(*dotIcon) : QIcon(*dothIcon));
                  return;
        case INSTCOL_PLAY:
                  if(!(rwFlags & 1))
                        return;
                  openFlags ^= 0x1;
                  MusEGlobal::audio->msgIdle(true);  // Make it safe to edit structures
                  md->setOpenFlags(openFlags);
                  if(md->midiPort() != -1)
                    MusEGlobal::midiPorts[md->midiPort()].setMidiDevice(md); // reopen device FIXME: This causes jack crash with R+W Jack midi device
                  MusEGlobal::audio->msgIdle(false);
                  item->setIcon(openFlags & 1 ? QIcon(*dotIcon) : QIcon(*dothIcon));
                  return;
        case INSTCOL_GUI:
                  if(synth && synth->hasNativeGui())
                  {
                    synth->showNativeGui(!synth->nativeGuiVisible());
                    item->setIcon(synth->nativeGuiVisible() ? QIcon(*dotIcon) : QIcon(*dothIcon));
                  }
                  return;
                  
        case INSTCOL_INROUTES:
        case INSTCOL_OUTROUTES:
                  {
                    if(!MusEGlobal::checkAudioDevice())
                      return;
                      
                    if(MusEGlobal::audioDevice->deviceType() != MusECore::AudioDevice::JACK_AUDIO)  // Only if Jack is running.
                      return;
                      
                    if(!md)
                      return;
                    
                    // Only Jack midi devices.
                    if(md->deviceType() != MusECore::MidiDevice::JACK_MIDI)  
                      return;
                    
                    if(!(md->rwFlags() & ((col == INSTCOL_OUTROUTES) ? 1 : 2)))    
                      return;
                      
                    RoutePopupMenu* pup = new RoutePopupMenu();
                    pup->exec(QCursor::pos(), md, col == INSTCOL_OUTROUTES);
                    delete pup;
                  }
                  return;
                  
#endif  
                  
      }
}
      
//---------------------------------------------------------
//   addJackDeviceClicked
//---------------------------------------------------------

void MPConfig::addJackDeviceClicked()
{
  MusEGlobal::audio->msgIdle(true); // Make it safe to edit structures
  // This automatically adds the device to the midiDevices list.
  MusECore::MidiDevice* md = MusECore::MidiJackDevice::createJackMidiDevice(); 
  if(md)
    md->setOpenFlags(3); // Start with open read + write.
  MusEGlobal::audio->msgIdle(false);
  if(md)
    MusEGlobal::song->update(SC_CONFIG);
}

#ifndef _WIN32
//---------------------------------------------------------
//   addAlsaDeviceClicked
//---------------------------------------------------------

void MPConfig::addAlsaDeviceClicked(bool v)
{
  MusEGlobal::audio->msgIdle(true); // Make it safe to edit structures

  MusEGlobal::config.enableAlsaMidiDriver = v;
  // Save settings. Use simple version - do NOT set style or stylesheet, this has nothing to do with that.
  //MusEGlobal::muse->changeConfig(true);    // Save settings? No, wait till close.

  if(v)
  {
    // Initialize the ALSA driver. This will automatically initialize the sequencer thread if necessary.
    MusECore::initMidiAlsa();

    if(MusEGlobal::midiSeq)
    {
      // Now start the sequencer if necessary. Prio unused, set in start.
      MusEGlobal::midiSeq->start(0);
      // Update the timer poll file descriptors.
      MusEGlobal::midiSeq->msgUpdatePollFd();
    }

    MusEGlobal::audio->msgIdle(false);

    // Scan for any changes in ALSA. FIXME: Note there's another another idle here !
    MusECore::alsaScanMidiPorts();

    // Inform the rest of the app's gui.
    MusEGlobal::song->update(SC_CONFIG);
  }
  else
  {
    // Exit ALSA midi.
    MusECore::exitMidiAlsa();
    MusEGlobal::audio->msgIdle(false);

    // Scan for any changes in ALSA. FIXME: Note there's another another idle here !
    MusECore::alsaScanMidiPorts();

    if(MusEGlobal::midiSeq)
    {
      MusEGlobal::audio->msgIdle(true); // Make it safe to edit structures

      MusEGlobal::midiSeq->msgUpdatePollFd();
      MusEGlobal::midiSeq->stop(true);

      MusECore::exitMidiSequencer();

      MusEGlobal::audio->msgIdle(false);
    }

    // Inform the rest of the app's gui.
    MusEGlobal::song->update(SC_CONFIG);
  }
}
#endif

//---------------------------------------------------------
//   beforeDeviceContextShow
//---------------------------------------------------------

void MPConfig::beforeDeviceContextShow(PopupMenu* /*menu*/, QAction* /*menuAction*/, QMenu* /*ctxMenu*/)
{
  DEBUG_PRST_ROUTES(stderr, "MPConfig::beforeDeviceContextShow\n");
}
      
//---------------------------------------------------------
//   deviceContextTriggered
//---------------------------------------------------------

void MPConfig::deviceContextTriggered(QAction* act)
{
  DEBUG_PRST_ROUTES(stderr, "MPConfig::deviceRemoveTriggered:%s\n", act->text().toLatin1().constData());
  if(act)
  {
// TODO: Work in progress.
//     
//     PopupMenu* menu = 0;
//     QAction* action = 0;
//     QVariant var_val = 0;
//     
//     menu = act->data().value<PopupMenuContextData>().menu();
//     action = act->data().value<PopupMenuContextData>().action();
//     var_val = act->data().value<PopupMenuContextData>().varValue();
// 
//     DEBUG_PRST_ROUTES(stderr, " menu:%p action:%p var:%x md:%p\n", menu, action, var_val.toInt(), MusEGlobal::midiDevices.find(action->text());
  }
}

//---------------------------------------------------------
//   configMidiPorts
//---------------------------------------------------------

void MusE::configMidiPorts()
      {
      if (!midiPortConfig) {
         midiPortConfig = new MusEGui::MPConfig(this);
      }
         midiPortConfig->show();
         midiPortConfig->raise();
         midiPortConfig->activateWindow();
      }

} // namespace MusEGui

