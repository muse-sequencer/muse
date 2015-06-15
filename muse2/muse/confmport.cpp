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
#include <termios.h>
#include <iostream>
#include <stdio.h>

#include <QMenu>
#include <QAction>
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
#include "driver/alsamidi.h"
#include "driver/jackmidi.h"
#include "audiodev.h"
#include "menutitleitem.h"
#include "utils.h"
#include "popupmenu.h"
#include "routepopup.h"

namespace MusEGlobal {
extern std::vector<MusECore::Synth*> synthis;
}

namespace MusEGui {

enum { DEVCOL_NO = 0, DEVCOL_GUI, DEVCOL_REC, DEVCOL_PLAY, DEVCOL_INSTR, DEVCOL_NAME,
       DEVCOL_INROUTES, DEVCOL_OUTROUTES, DEVCOL_DEF_IN_CHANS, DEVCOL_DEF_OUT_CHANS, DEVCOL_STATE };

// REMOVE Tim. Persistent routes. Added.
//enum { INSTCOL_NAME = 0, INSTCOL_TYPE, INSTCOL_REC, INSTCOL_PLAY, INSTCOL_GUI, INSTCOL_STATE };
enum { INSTCOL_NAME = 0, INSTCOL_TYPE, INSTCOL_STATE };
       
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
  if(no < 0 || no >= MIDI_PORTS)
    return;
  int actid = act->data().toInt();
  int allch = (1 << MIDI_CHANNELS) - 1;  
  int defch = MusEGlobal::midiPorts[no].defaultInChannels();  
  
  if(actid == MIDI_CHANNELS + 1)  // Apply to all tracks now.
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
          // Remove all routes from this port to the tracks first.
          MusEGlobal::audio->msgRemoveRoute(MusECore::Route(no, allch), MusECore::Route(*it, allch));
          if(defch)
            MusEGlobal::audio->msgAddRoute(MusECore::Route(no, defch), MusECore::Route(*it, defch));
        }  
        MusEGlobal::song->update(SC_ROUTE);                    
      }
    }  
  }
  else
  {
    int chbits;
    if(actid == MIDI_CHANNELS)              // Toggle all.
    {
      chbits = (defch == allch) ? 0 : allch;
      if(defpup)
        for(int i = 0; i < MIDI_CHANNELS; ++i)
        {
          QAction* act = defpup->findActionFromData(i);  
          if(act)
            act->setChecked(chbits);
        }    
    }  
    else
      chbits = defch ^ (1 << actid);
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
  if(no < 0 || no >= MIDI_PORTS)
    return;
  int actid = act->data().toInt();
  int defch = MusEGlobal::midiPorts[no].defaultOutChannels();  
  // Turn on if and when multiple output routes are supported. DELETETHIS??
  #if 0
  int allch = (1 << MIDI_CHANNELS) - 1;
  #endif
  
  if(actid == MIDI_CHANNELS + 1)  // Apply to all tracks now.
  {
    // Are there tracks, and is there a port device? 
    // Tested: Hmm, allow ports with no device since that is a valid situation.
    if(!MusEGlobal::song->midis()->empty()) // && MusEGlobal::midiPorts[no].device())
    {
      // Turn off if and when multiple output routes are supported.
      #if 1
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
        // Turn on if and when multiple output routes are supported. DELETETHIS??
        #if 0
        for(MusECore::iMidiTrack it = mtl->begin(); it != mtl->end(); ++it)
        {
          // Remove all routes from this port to the tracks first.
          MusEGlobal::audio->msgRemoveRoute(MusECore::Route(no, allch), MusECore::Route(*it, allch));
          if(defch)
            MusEGlobal::audio->msgAddRoute(MusECore::Route(no, defch), MusECore::Route(*it, defch));
        }  
        MusEGlobal::audio->msgUpdateSoloStates();
        MusEGlobal::song->update(SC_ROUTE);                    
        #else
        for(int ch = 0; ch < MIDI_CHANNELS; ++ch)
          if(defch & (1 << ch))
          { 
            MusEGlobal::audio->msgIdle(true);
            for(MusECore::iMidiTrack it = mtl->begin(); it != mtl->end(); ++it)
            {
              // Leave drum track channel at current setting.
              if((*it)->type() == MusECore::Track::DRUM)
                (*it)->setOutPortAndUpdate(no);
              else
                (*it)->setOutPortAndChannelAndUpdate(no, ch);
            }  
            MusEGlobal::audio->msgIdle(false);
            MusEGlobal::audio->msgUpdateSoloStates();
            MusEGlobal::song->update(SC_MIDI_TRACK_PROP);                    

            // Stop at the first output channel found.
            break;
          }  
        #endif
      }
    }  
  }
  else
  {
    #if 0          // Turn on if and when multiple output routes are supported. DELETETHIS??
    int chbits;
    if(actid == MIDI_CHANNELS)              // Toggle all.
    {
      chbits = (defch == allch) ? 0 : allch;
      if(defpup)
        for(int i = 0; i < MIDI_CHANNELS; ++i)
        {
          QAction* act = defpup->findActionFromData(i);  
          if(act)
            act->setChecked(chbits);
        }    
    }  
    else
      chbits = defch ^ (1 << actid);
    MusEGlobal::midiPorts[no].setDefaultOutChannels(chbits);
    mdevView->item(item->row(), DEVCOL_DEF_OUT_CHANS)->setText(MusECore::bitmap2String(chbits));
    #else
    if(actid < MIDI_CHANNELS)
    {
      int chbits = 1 << actid;
      // Are we toggling off?
      if(chbits & defch)
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
        if(defpup)
        {
          QAction* a;
          for(int i = 0; i < MIDI_CHANNELS; ++i)
          {
            a = defpup->findActionFromData(i);  
            if(a)
              a->setChecked(i == actid);
          }  
        }  
      }
    }    
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
  int col = item->column();
  //QString s = item->text();
  QTableWidgetItem* twi = item->tableWidget()->item(item->row(), INSTCOL_NAME);
  if(!twi)
  {
    fprintf(stderr, "synthesizerConfig::DeviceItemRenamed(): row:%d INSTCOL_NAME not found\n", item->row());
    return;
  }
  QString new_name = twi->text();
  // Get the original name.
  QString orig_name = twi->data(Qt::UserRole).toString();
  if(new_name == orig_name)
    return;
  
  twi = item->tableWidget()->item(item->row(), INSTCOL_TYPE);  
  if(!twi)
  {
    fprintf(stderr, "synthesizerConfig::DeviceItemRenamed(): row:%d INSTCOL_TYPE not found\n", item->row());
    return;
  }
  QString type = twi->text();
  //QString name = item->tableWidget()->item(item->row(), INSTCOL_NAME)->text();
  //QString type = item->tableWidget()->item(item->row(), INSTCOL_TYPE)->text();
  
  
  MusECore::MidiDevice* md = 0;
  MusECore::MidiDevice* newname_md = 0;
  for(MusECore::iMidiDevice imd = MusEGlobal::midiDevices.begin(); imd != MusEGlobal::midiDevices.end(); ++imd) {
        if((*imd)->deviceTypeString() == type) {
          if(!md && (*imd)->name() == orig_name)
            md = *imd;
          if(!newname_md && (*imd)->name() == new_name)
            newname_md = *imd;
          if(md && newname_md)
            break;
          }
        }
  //if (imd == MusEGlobal::midiDevices.end()) {  // REMOVE Tim. Persistent routes. Changed.
  if (!md) {
        fprintf(stderr, "synthesizerConfig::DeviceItemRenamed(): device not found\n");
        return;
        }
  switch(col)
  {
    // REMOVE Tim. Persistent routes. Changed.
    //case DEVCOL_NAME:
    case INSTCOL_NAME:
    {
      // REMOVE Tim. Persistent routes. Changed.
      //QString id = item->tableWidget()->item(item->row(), DEVCOL_NO)->text();
      //int no = atoi(id.toLatin1().constData()) - 1;
      //if(no < 0 || no >= MIDI_PORTS)
      //  return;
      //MusECore::MidiPort* port      = &MusEGlobal::midiPorts[no];
      //MusECore::MidiDevice* dev     = port->device();
      // Only Jack midi devices.
      //if(!dev || dev->deviceType() != MusECore::MidiDevice::JACK_MIDI)
      //  return;
      //if(dev->name() == s)
      //  return;  
        
      // Only Jack midi devices.
      if(md->deviceType() != MusECore::MidiDevice::JACK_MIDI)
        return;
      // REMOVE Tim. Persistent routes. Removed.
      //if(md->name() == s)
      //  return;  
      
      //if(MusEGlobal::midiDevices.find(s))
      if(newname_md)
      {
        QMessageBox::critical(this,
            tr("MusE: bad device name"),
            tr("please choose a unique device name"),
            QMessageBox::Ok,
            Qt::NoButton,
            Qt::NoButton);
        //songChanged(-1);
        instanceList->blockSignals(true);
        item->setText(orig_name);
        instanceList->blockSignals(false);
        return;
      }
      //dev->setName(s);
      MusEGlobal::audio->msgIdle(true); // Make it safe to edit structures
      md->setName(new_name);
      MusEGlobal::audio->msgIdle(false);
      // Save the new name for the next time this itemChanged handler is called.
      //item->setData(Qt::UserRole, md->name()); // Not necessary, the update clears and fills the table.

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
      if (no < 0 || no >= MIDI_PORTS)
            return;

      int n;
      MusECore::MidiPort* port      = &MusEGlobal::midiPorts[no];
      MusECore::MidiDevice* dev     = port->device();
      int rwFlags         = dev ? dev->rwFlags() : 0;
      int openFlags       = dev ? dev->openFlags() : 0;
      QTableWidget* listView = item->tableWidget();
      QPoint ppt          = listView->visualItemRect(item).bottomLeft();
      //QPoint mousepos     = QCursor::pos();  // REMOVE Tim. Persistent routes. Removed.
      int col = item->column();
      ppt += QPoint(0, listView->horizontalHeader()->height());
      ppt  = listView->mapToGlobal(ppt);

      switch (col) {
            case DEVCOL_GUI:
                  if (dev == 0)
                        return;
                  // REMOVE Tim. Persistent routes. Changed.
                  if (port->hasNativeGui())
                  //if(dev->hasNativeGui())
                  {
                        // REMOVE Tim. Persistent routes. Changed.
                        //port->instrument()->showNativeGui(!port->nativeGuiVisible());
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
                  MusEGlobal::midiSeq->msgSetMidiDevice(port, dev);       // reopen device
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
                  MusEGlobal::midiSeq->msgSetMidiDevice(port, dev);       // reopen device
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
                      
                    // REMOVE Tim. Persistent routes. Added.
                    RoutePopupMenu* pup = new RoutePopupMenu();
                    pup->exec(QCursor::pos(), dev, col == DEVCOL_OUTROUTES);
                    delete pup;
                  }
                  return;
            break;                    
                    
                    
// REMOVE Tim. Persistent routes. Removed.
//                     MusECore::RouteList* rl = (col == DEVCOL_OUTROUTES) ? dev->outRoutes() : dev->inRoutes();   
//                     QMenu* pup = 0;
//                     int gid = 0;
//                     std::list<QString> sl;
//                     pup = new QMenu(this);
//                     
//         _redisplay:
//                     pup->clear();
//                     gid = 0;
//                     
//                     // Jack input ports if device is writable, and jack output ports if device is readable.
//                     sl = (col == DEVCOL_OUTROUTES) ? MusEGlobal::audioDevice->inputPorts(true, _showAliases) : MusEGlobal::audioDevice->outputPorts(true, _showAliases);
//                     
//                     QAction* act;
//                     
//                     act = pup->addAction(tr("Show first aliases"));
//                     act->setData(gid);
//                     act->setCheckable(true);
//                     act->setChecked(_showAliases == 0);
//                     ++gid;
//                     
//                     act = pup->addAction(tr("Show second aliases"));
//                     act->setData(gid);
//                     act->setCheckable(true);
//                     act->setChecked(_showAliases == 1);
//                     ++gid;
//                     
//                     pup->addSeparator();
//                     for(std::list<QString>::iterator ip = sl.begin(); ip != sl.end(); ++ip) 
//                     {
//                       act = pup->addAction(*ip);
//                       act->setData(gid);
//                       act->setCheckable(true);
//                       
//                       MusECore::Route rt(*ip, (col == DEVCOL_OUTROUTES), -1, MusECore::Route::JACK_ROUTE);   
//                       for(MusECore::ciRoute ir = rl->begin(); ir != rl->end(); ++ir) 
//                       {
//                         if (*ir == rt) 
//                         {
//                           act->setChecked(true);
//                           break;
//                         }
//                       }
//                       ++gid;
//                     }
//                     
//                     act = pup->exec(ppt);
//                     if(act)
//                     {
//                       n = act->data().toInt();
//                       if(n == 0) // Show first aliases
//                       {
//                         if(_showAliases == 0)
//                           _showAliases = -1;
//                         else  
//                           _showAliases = 0;
//                         goto _redisplay;   // Go back
//                       }
//                       else
//                       if(n == 1) // Show second aliases
//                       {
//                         if(_showAliases == 1)
//                           _showAliases = -1;
//                         else  
//                           _showAliases = 1;
//                         goto _redisplay;   // Go back
//                       }
//                       
//                       QString s(act->text());
//                       
//                       if(col == DEVCOL_OUTROUTES) // Writeable  
//                       {
//                         MusECore::Route srcRoute(dev, -1);
//                         MusECore::Route dstRoute(s, true, -1, MusECore::Route::JACK_ROUTE);
//             
//                         MusECore::ciRoute iir = rl->begin();
//                         for(; iir != rl->end(); ++iir) 
//                         {
//                           if(*iir == dstRoute)
//                             break;
//                         }
//                         if(iir != rl->end()) 
//                           // disconnect
//                           MusEGlobal::audio->msgRemoveRoute(srcRoute, dstRoute);
//                         else 
//                           // connect
//                           MusEGlobal::audio->msgAddRoute(srcRoute, dstRoute);
//                       }
//                       else
//                       {
//                         MusECore::Route srcRoute(s, false, -1, MusECore::Route::JACK_ROUTE);
//                         MusECore::Route dstRoute(dev, -1);
//             
//                         MusECore::ciRoute iir = rl->begin();
//                         for(; iir != rl->end(); ++iir) 
//                         {
//                           if(*iir == srcRoute)
//                             break;
//                         }
//                         if(iir != rl->end()) 
//                           // disconnect
//                           MusEGlobal::audio->msgRemoveRoute(srcRoute, dstRoute);
//                         else 
//                           // connect
//                           MusEGlobal::audio->msgAddRoute(srcRoute, dstRoute);
//                       }  
//                       
//                       MusEGlobal::audio->msgUpdateSoloStates();
//                       MusEGlobal::song->update(SC_ROUTE);
//                       
//                       // FIXME:
//                       // Routes can't be re-read until the message sent from msgAddRoute1() 
//                       //  has had time to be sent and actually affected the routes.
//                       //goto _redisplay;   // Go back
//                     }  
//                     delete pup;
//                   }
//                   return;
                  
            case DEVCOL_DEF_IN_CHANS:
                  {
                    defpup = new MusEGui::PopupMenu(this, true);
                    defpup->addAction(new MusEGui::MenuTitleItem("Channel", defpup)); 
                    QAction* act = 0;
                    int chbits = MusEGlobal::midiPorts[no].defaultInChannels();
                    for(int i = 0; i < MIDI_CHANNELS; ++i) 
                    {
                      act = defpup->addAction(QString().setNum(i + 1));
                      act->setData(i);
                      act->setCheckable(true);
                      act->setChecked((1 << i) & chbits);
                    }
                    
                    act = defpup->addAction(tr("Toggle all"));
                    act->setData(MIDI_CHANNELS);
                    
                    defpup->addSeparator();
                    act = defpup->addAction(tr("Change all tracks now"));
                    act->setData(MIDI_CHANNELS + 1);
                    // Enable only if there are tracks, and port has a device.
                    // Tested: Hmm, allow ports with no device since that is a valid situation.
                    act->setEnabled(!MusEGlobal::song->midis()->empty());  // && MusEGlobal::midiPorts[no].device()); DELETETHIS
                    
                    connect(defpup, SIGNAL(triggered(QAction*)), SLOT(changeDefInputRoutes(QAction*)));
                    defpup->exec(QCursor::pos());
                    delete defpup;
                    defpup = 0;
                  }
                  return;                  
            break;                    
                  
            case DEVCOL_DEF_OUT_CHANS:
                  {
                    defpup = new MusEGui::PopupMenu(this, true);
                    defpup->addAction(new MusEGui::MenuTitleItem("Channel", defpup)); 
                    QAction* act = 0;
                    int chbits = MusEGlobal::midiPorts[no].defaultOutChannels();
                    for(int i = 0; i < MIDI_CHANNELS; ++i) 
                    {
                      act = defpup->addAction(QString().setNum(i + 1));
                      act->setData(i);
                      act->setCheckable(true);
                      act->setChecked((1 << i) & chbits);
                    }  
                    
                    // Turn on if and when multiple output routes are supported. DELETETHIS?
                    #if 0
                    act = defpup->addAction(tr("Toggle all"));
                    act->setData(MIDI_CHANNELS);
                    #endif
                    
                    defpup->addSeparator();
                    act = defpup->addAction(tr("Change all tracks now"));
                    act->setData(MIDI_CHANNELS + 1);
                    // Enable only if there are tracks, and port has a device.
                    // Tested: Hmm, allow ports with no device since that is a valid situation.
                    act->setEnabled(!MusEGlobal::song->midis()->empty());  // && MusEGlobal::midiPorts[no].device());
                    
                    connect(defpup, SIGNAL(triggered(QAction*)), SLOT(changeDefOutputRoutes(QAction*)));
                    defpup->exec(QCursor::pos());
                    delete defpup;
                    defpup = 0;
                  }
                  return;
            break;                    
                  
            case DEVCOL_NAME:
                  {
                    // REMOVE Tim. Persistent routes. Removed.
//                     // Did we click in the text area?
//                     if((mousepos.x() - ppt.x()) > buttondownIcon->width())
//                     {
//                       // Start the renaming of the cell...
//                       QModelIndex current = item->tableWidget()->currentIndex();
//                       if (item->flags() & Qt::ItemIsEditable)
//                         item->tableWidget()->edit(current.sibling(current.row(), DEVCOL_NAME));
//                         
//                       return;
//                     }
//                     else
                    // We clicked the 'down' button.
                    {
                      //QMenu* pup = new QMenu(this);   // REMOVE Tim. Persistent routes. Changed.
                      PopupMenu* pup = new PopupMenu(this);
                      
                      QAction* act;

                      // REMOVE Tim. Persistent routes. Added.
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
                      
                      asmap mapALSA;
                      asmap mapJACK;
                      asmap mapSYNTH;
                      
                      int aix = 0x10000000;
                      int jix = 0x20000000;
                      int six = 0x30000000;
                      for(MusECore::iMidiDevice i = MusEGlobal::midiDevices.begin(); i != MusEGlobal::midiDevices.end(); ++i) 
                      {
                        if((*i)->deviceType() == MusECore::MidiDevice::ALSA_MIDI)
                        {
                          mapALSA.insert( std::pair<std::string, int> (std::string((*i)->name().toLatin1().constData()), aix) );
                          ++aix;
                        }  
                        else
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
                          printf("MPConfig::rbClicked unknown midi device: %s\n", (*i)->name().toLatin1().constData());
                      }
                      
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
                        delete pup;
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
                        if(n < 0x20000000)
                          typ = MusECore::MidiDevice::ALSA_MIDI;
                        else if(n < 0x30000000)
                          typ = MusECore::MidiDevice::JACK_MIDI;
                        else //if(n < 0x40000000)
                          typ = MusECore::MidiDevice::SYNTH_MIDI;
                        
                        sdev = MusEGlobal::midiDevices.find(act->text(), typ);
                        delete pup;
                        // Is it the current device? Reset it to <none>.
                        if(sdev == dev)
                          sdev = 0;
                      }    

                      int allch = (1 << MIDI_CHANNELS) - 1;  
                      MusECore::MidiTrackList* mtl = MusEGlobal::song->midis();

                      // Remove track routes to/from an existing port already using the selected device...
                      if(sdev) 
                      {
                        for(int i = 0; i < MIDI_PORTS; ++i) 
                        {
                          if(MusEGlobal::midiPorts[i].device() == sdev) 
                          {
                            for(MusECore::iMidiTrack it = mtl->begin(); it != mtl->end(); ++it)
                              MusEGlobal::audio->msgRemoveRoute(MusECore::Route(i, allch), MusECore::Route(*it, allch));

                            // Turn on if and when multiple output routes are supported. DELETETHIS?
                        #if 0
                            for(MusECore::iMidiTrack it = mtl->begin(); it != mtl->end(); ++it)
                              MusEGlobal::audio->msgRemoveRoute(MusECore::Route(no, allch), MusECore::Route(*it, allch));
                        #endif
                        
                            break;
                          }
                        }
                      }
                      
                      // Remove all track routes to/from this port...
                      for(MusECore::iMidiTrack it = mtl->begin(); it != mtl->end(); ++it)
                        // Remove all routes from this port to the tracks.
                        MusEGlobal::audio->msgRemoveRoute(MusECore::Route(no, allch), MusECore::Route(*it, allch));
                      // Turn on if and when multiple output routes are supported. DELETETHIS?
                  #if 0
                      for(MusECore::iMidiTrack it = mtl->begin(); it != mtl->end(); ++it)
                        MusEGlobal::audio->msgRemoveRoute(MusECore::Route(no, allch), MusECore::Route(*it, allch));
                  #endif
                      
                      // REMOVE Tim. Persistent routes. Added.
//                       for(MusECore::iMidiDevice imd = MusEGlobal::midiDevices.begin(); imd != MusEGlobal::midiDevices.end(); ++imd)
//                       {
//                         MusECore::MidiDevice* md = *imd;
//                         fprintf(stderr, "MidiDevice name:%s\nIn ", md->name().toLatin1().constData());
//                         for(MusECore::iRoute ir = md->inRoutes()->begin(); ir != md->inRoutes()->end(); ++ir)
//                           (*ir).dump();
//                         fprintf(stderr, "\n");
//                         fprintf(stderr, "Out ");
//                         for(MusECore::iRoute ir = md->outRoutes()->begin(); ir != md->outRoutes()->end(); ++ir)
//                           (*ir).dump();
//                         fprintf(stderr, "\n");
//                       }
                      
                      MusEGlobal::midiSeq->msgSetMidiDevice(port, sdev);
                      MusEGlobal::muse->changeConfig(true);     // save configuration file
                      
                      // Add all track routes to/from this port...
                      if(sdev)
                      {  
                        int chbits = MusEGlobal::midiPorts[no].defaultInChannels();
                        // Do not add input routes to synths.
                        if(!sdev->isSynti())  
                        {
                          for(MusECore::iMidiTrack it = mtl->begin(); it != mtl->end(); ++it)
                          {
                            // Connect all the specified routes.
                            if(chbits)
                              MusEGlobal::audio->msgAddRoute(MusECore::Route(no, chbits), MusECore::Route(*it, chbits));
                          }  
                        }
//                        chbits = MusEGlobal::midiPorts[no].defaultOutChannels();
                        // Turn on if and when multiple output routes are supported. DELETETHIS?
                    #if 0
                        for(MusECore::iMidiTrack it = mtl->begin(); it != mtl->end(); ++it)
                        {
                          // Connect all the specified routes.
                          if(chbits)
                            MusEGlobal::audio->msgAddRoute(MusECore::Route(no, chbits), MusECore::Route(*it, chbits));
                        }  
                    #else
// REMOVE Tim.                    
//                         for(int ch = 0; ch < MIDI_CHANNELS; ++ch)
//                           if(chbits & (1 << ch)) 
//                           {    
//                             MusEGlobal::audio->msgIdle(true);
//                             for(MusECore::iMidiTrack it = mtl->begin(); it != mtl->end(); ++it)
//                             {
//                               // We are only interested in tracks which use this port being changed now.
//                               if((*it)->outPort() != no)
//                                 continue;
//                               // Leave drum track channel at current setting.  // REMOVE Tim.
//                               //if((*it)->type() == MusECore::Track::DRUM)
//                               //  (*it)->setOutPortAndUpdate(no);
//                               //else
//                               //  (*it)->setOutPortAndChannelAndUpdate(no, ch);
//                               (*it)->setOutPortAndUpdate(no);
//                             }  
//                             MusEGlobal::audio->msgIdle(false);
//                             // Stop at the first output channel found.
//                             break;
//                           }   
                    #endif
                      }
                      
                      MusEGlobal::audio->msgUpdateSoloStates();
                      MusEGlobal::song->update();
                    }  
                    
                    // REMOVE Tim. Persistent routes. Added.
//                     for(MusECore::iMidiDevice imd = MusEGlobal::midiDevices.begin(); imd != MusEGlobal::midiDevices.end(); ++imd)
//                     {
//                       MusECore::MidiDevice* md = *imd;
//                       fprintf(stderr, "MidiDevice name:%s\nIn ", md->name().toLatin1().constData());
//                       for(MusECore::iRoute ir = md->inRoutes()->begin(); ir != md->inRoutes()->end(); ++ir)
//                         (*ir).dump();
//                       fprintf(stderr, "\n");
//                       fprintf(stderr, "Out ");
//                       for(MusECore::iRoute ir = md->outRoutes()->begin(); ir != md->outRoutes()->end(); ++ir)
//                         (*ir).dump();
//                       fprintf(stderr, "\n");
//                     }
                    
                  }
                  return;
            break;                    

            case DEVCOL_INSTR:
                  {
                  if (dev && dev->isSynti())
                        return;
                  if (instrPopup == 0)
                        instrPopup = new PopupMenu(this);
                  MusECore::MidiInstrument::populateInstrPopup(instrPopup, port->instrument(), false);   
                  
                  QAction* act = instrPopup->exec(ppt);
                  if(!act)
                    return;
                  QString s = act->text();
                  item->tableWidget()->item(item->row(), DEVCOL_INSTR)->setText(s);
                  for (MusECore::iMidiInstrument i = MusECore::midiInstruments.begin(); i
                     != MusECore::midiInstruments.end(); ++i) {
                        if ((*i)->iname() == s) {
                              port->setInstrument(*i);
                              break;
                              }
                        }
                  MusEGlobal::song->update();
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
            case DEVCOL_GUI:    item->setToolTip(tr("Enable gui")); break;
            case DEVCOL_REC:    item->setToolTip(tr("Enable reading")); break;
            case DEVCOL_PLAY:   item->setToolTip(tr("Enable writing")); break;
            case DEVCOL_INSTR:  item->setToolTip(tr("Port instrument")); break;
            case DEVCOL_NAME:   item->setToolTip(tr("Midi device name. Click to edit (Jack)")); break;
            case DEVCOL_INROUTES:  item->setToolTip(tr("Connections from Jack Midi outputs")); break;
            case DEVCOL_OUTROUTES: item->setToolTip(tr("Connections to Jack Midi inputs")); break;
            case DEVCOL_DEF_IN_CHANS:   item->setToolTip(tr("Auto-connect these channels to new midi tracks")); break;
            // Turn on if and when multiple output routes are supported. DELETETHIS?
            #if 0
            case DEVCOL_DEF_OUT_CHANS:  item->setToolTip(tr("Auto-connect new midi tracks to these channels")); break;
            #else
            case DEVCOL_DEF_OUT_CHANS:  item->setToolTip(tr("Auto-connect new midi tracks to this channel")); break;
            #endif
            case DEVCOL_STATE:  item->setToolTip(tr("Device state")); break;
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
            case DEVCOL_GUI:
                  item->setWhatsThis(tr("Enable gui for device")); break;
            case DEVCOL_REC:
                  item->setWhatsThis(tr("Enable reading from device")); break;
            case DEVCOL_PLAY:
                  item->setWhatsThis(tr("Enable writing to device")); break;
            case DEVCOL_NAME:
                  item->setWhatsThis(tr("Name of the midi device associated with"
                                        " this port number. Click to edit Jack midi name.")); break;
            case DEVCOL_INSTR:
                  item->setWhatsThis(tr("Instrument connected to port")); break;
            case DEVCOL_INROUTES:
                  item->setWhatsThis(tr("Connections from Jack Midi output ports")); break;
            case DEVCOL_OUTROUTES:
                  item->setWhatsThis(tr("Connections to Jack Midi input ports")); break;
            case DEVCOL_DEF_IN_CHANS:
                  item->setWhatsThis(tr("Auto-connect these channels, on this port, to new midi tracks.")); break;
            case DEVCOL_DEF_OUT_CHANS:
                  // Turn on if and when multiple output routes are supported. DELETETHIS?
                  #if 0
                  item->setWhatsThis(tr("Connect new midi tracks to these channels, on this port.")); break;
                  #else                      
                  item->setWhatsThis(tr("Connect new midi tracks to this channel, on this port.")); break;
                  #endif                      
            case DEVCOL_STATE:
                  item->setWhatsThis(tr("State: result of opening the device")); break;
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
//   MPConfig
//    Midi Port Config
//---------------------------------------------------------

MPConfig::MPConfig(QWidget* parent)
   : QDialog(parent)
      {
      setupUi(this);
      QSettings settings("MusE", "MusE-qt");
      restoreGeometry(settings.value("MPConfig/geometry").toByteArray());

      mdevView->setRowCount(MIDI_PORTS);
      mdevView->verticalHeader()->hide();
      //mdevView->setSelectionMode(QAbstractItemView::SingleSelection);
      //mdevView->setSelectionBehavior(QAbstractItemView::SelectRows);// REMOVE Tim. Persistent routes. Added.
      mdevView->setShowGrid(false);

      instrPopup = 0;
      defpup = 0;
      _showAliases = 1; // 0: Show second aliases, if available. 
      
      QStringList columnnames;
      columnnames << tr("Port")
		  << tr("GUI")
		  << tr("I")
		  << tr("O")
		  << tr("Instrument")
		  << tr("Device Name")
		  << tr("In routes")
		  << tr("Out routes")
                  << tr("Def in ch")
                  << tr("Def out ch")
		  << tr("State");

      mdevView->setColumnCount(columnnames.size());
      mdevView->setHorizontalHeaderLabels(columnnames);
      for (int i = 0; i < columnnames.size(); ++i) {
            setWhatsThis(mdevView->horizontalHeaderItem(i), i);
            setToolTip(mdevView->horizontalHeaderItem(i), i);
            }
      // REMOVE Tim. Persistent routes. Removed.
      mdevView->setFocusPolicy(Qt::NoFocus);

      
// REMOVE Tim. Persistent routes. Added.
      //instanceList->setRowCount(MIDI_PORTS);
      instanceList->verticalHeader()->hide();
      //instanceList->setSelectionMode(QAbstractItemView::SingleSelection);
      instanceList->setShowGrid(false);
      columnnames.clear();
      columnnames << tr("Device Name")
                  << tr("Type")
                  //<< tr("I")
                  //<< tr("O")
                  //<< tr("GUI")
                  << tr("State");
      instanceList->setColumnCount(columnnames.size());
      //instanceList->setHeaderLabels(columnnames);
      instanceList->setHorizontalHeaderLabels(columnnames);
      for (int i = 0; i < columnnames.size(); ++i) {
            //setWhatsThis(instanceList->horizontalHeaderItem(i), i);
            //setToolTip(instanceList->horizontalHeaderItem(i), i);
            }
      //instanceList->setFocusPolicy(Qt::NoFocus);
      connect(instanceList, SIGNAL(itemPressed(QTableWidgetItem*)), SLOT(deviceItemClicked(QTableWidgetItem*)));
      connect(instanceList, SIGNAL(itemSelectionChanged()),         SLOT(deviceSelectionChanged()));
      connect(instanceList, SIGNAL(itemChanged(QTableWidgetItem*)), SLOT(DeviceItemRenamed(QTableWidgetItem*)));
      
      connect(addJACKDevice, SIGNAL(clicked(bool)), SLOT(addJackDeviceClicked()));
      connect(addALSADevice, SIGNAL(clicked(bool)), SLOT(addAlsaDeviceClicked()));
      
      connect(mdevView, SIGNAL(itemPressed(QTableWidgetItem*)),
         this, SLOT(rbClicked(QTableWidgetItem*)));
      // REMOVE Tim. Persistent routes. Removed.
      //connect(mdevView, SIGNAL(itemChanged(QTableWidgetItem*)),
      //   this, SLOT(mdevViewItemRenamed(QTableWidgetItem*)));
      connect(MusEGlobal::song, SIGNAL(songChanged(MusECore::SongChangedFlags_t)), SLOT(songChanged(MusECore::SongChangedFlags_t)));

      connect(synthList, SIGNAL(itemSelectionChanged()), SLOT(selectionChanged()));
      // REMOVE Tim. Persistent routes. Removed.
      //connect(instanceList, SIGNAL(itemSelectionChanged()), SLOT(selectionChanged()));

      connect(addSynthDevice, SIGNAL(clicked()), SLOT(addInstanceClicked()));
      connect(synthList, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), SLOT(addInstanceClicked())); 
      connect(removeDevice, SIGNAL(clicked()), SLOT(removeInstanceClicked()));
      // REMOVE Tim. Persistent routes. Removed.
      //connect(instanceList,  SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), SLOT(removeInstanceClicked()));
      
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
      // REMOVE Tim. Persistent routes. Changed.
      //addSynthDevice->setEnabled(synthList->currentItem());
      addSynthDevice->setEnabled(synthList->selectedItems().isEmpty() ? false : (bool)synthList->currentItem());
      // REMOVE Tim. Persistent routes. Removed.
      //removeDevice->setEnabled(instanceList->currentItem());
      }

// REMOVE Tim. Persistent routes. Added.
//---------------------------------------------------------
//   deviceSelectionChanged
//---------------------------------------------------------

void MPConfig::deviceSelectionChanged()
{
  fprintf(stderr, "synthesizerConfig::deviceSelectionChanged() currentItem:%p\n", instanceList->currentItem()); // REMOVE Tim. Persistent routes. Added.
  //removeDevice->setEnabled(instanceList->selectedItems().isEmpty() ? false : (bool)instanceList->currentItem());
  //removeDevice->setEnabled((bool)instanceList->currentItem());
  QTableWidgetItem* item = instanceList->currentItem();
  if(item == 0)
  {
    removeDevice->setEnabled(false);
    return;
  }
  // REMOVE Tim. Persistent routes. Removed.
//       MusECore::SynthIList* sl = MusEGlobal::song->syntis();
//       MusECore::iSynthI ii;
//       for (ii = sl->begin(); ii != sl->end(); ++ii) {
//             if( (*ii)->iname() == item->text(0) && 
//                  MusECore::synthType2String((*ii)->synth()->synthType()) == item->text(1) )
//               break;
//             }
//       if (ii == sl->end()) {
//             printf("synthesizerConfig::removeInstanceClicked(): synthi not found\n");
//             return;
//             }
//       MusEGlobal::audio->msgRemoveTrack(*ii);
  
  // REMOVE Tim. Persistent routes. Added.
  //MusECore::SynthIList* sl = MusEGlobal::song->syntis();
  QString name = item->tableWidget()->item(item->row(), INSTCOL_NAME)->text();
  QString type = item->tableWidget()->item(item->row(), INSTCOL_TYPE)->text();
  
  MusECore::MidiDevice* md = 0;
  MusECore::iMidiDevice imd;
  for (imd = MusEGlobal::midiDevices.begin(); imd != MusEGlobal::midiDevices.end(); ++imd) {
        md = *imd;
        if(md->name() == name && md->deviceTypeString() == type)
          break;
        }
  if (imd == MusEGlobal::midiDevices.end()) {
        fprintf(stderr, "synthesizerConfig::deviceSelectionChanged(): device not found\n");
        return;
        }
        
  // Is it an ALSA midi device? 
  // TODO: For now, don't allow creating/removing/renaming them until we decide on addressing strategy.
  if(md->deviceType() == MusECore::MidiDevice::ALSA_MIDI)
  {
    snd_seq_addr_t* addr = static_cast<snd_seq_addr_t*>(md->inClientPort());
    // Allow removing ('purging') an unavailable ALSA device.
    if(addr->client == SND_SEQ_ADDRESS_UNKNOWN || addr->port == SND_SEQ_ADDRESS_UNKNOWN)
      removeDevice->setEnabled(true);
    else
      removeDevice->setEnabled(false);
    return;
  }
  removeDevice->setEnabled(true);
}

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void MPConfig::songChanged(MusECore::SongChangedFlags_t flags)
      {
      if(!(flags & (SC_CONFIG | SC_TRACK_INSERTED | SC_TRACK_REMOVED | SC_TRACK_MODIFIED)))
        return;
    
      // Get currently selected index...
      int no = -1;
      QTableWidgetItem* sitem = mdevView->currentItem();
      if(sitem)
      {
        QString id = sitem->tableWidget()->item(sitem->row(), DEVCOL_NO)->text();
        no = atoi(id.toLatin1().constData()) - 1;
        if(no < 0 || no >= MIDI_PORTS)
          no = -1;
      }
      
      sitem = 0;
      mdevView->clearContents();
      int defochs = 0;
      for (int i = MIDI_PORTS-1; i >= 0; --i) 
      {
            mdevView->blockSignals(true); // otherwise itemChanged() is triggered and bad things happen.
            MusECore::MidiPort* port  = &MusEGlobal::midiPorts[i];
            MusECore::MidiDevice* dev = port->device();
            QString s;
            s.setNum(i+1);
            QTableWidgetItem* itemno = new QTableWidgetItem(s);
            addItem(i, DEVCOL_NO, itemno, mdevView);
            itemno->setTextAlignment(Qt::AlignHCenter);
            itemno->setFlags(Qt::ItemIsEnabled);
            QTableWidgetItem* itemstate = new QTableWidgetItem(port->state());
            addItem(i, DEVCOL_STATE, itemstate, mdevView);
            itemstate->setFlags(Qt::ItemIsEnabled);
            QTableWidgetItem* iteminstr = new QTableWidgetItem(port->instrument() ?
                           port->instrument()->iname() :
                           tr("<unknown>"));
            addItem(i, DEVCOL_INSTR, iteminstr, mdevView);
            iteminstr->setFlags(Qt::ItemIsEnabled);
            QTableWidgetItem* itemname = new QTableWidgetItem;
            addItem(i, DEVCOL_NAME, itemname, mdevView);
            itemname->setFlags(Qt::ItemIsEnabled);
            QTableWidgetItem* itemgui = new QTableWidgetItem;
            addItem(i, DEVCOL_GUI, itemgui, mdevView);
            itemgui->setTextAlignment(Qt::AlignHCenter);
            itemgui->setFlags(Qt::ItemIsEnabled);
            QTableWidgetItem* itemrec = new QTableWidgetItem;
            addItem(i, DEVCOL_REC, itemrec, mdevView);
            itemrec->setTextAlignment(Qt::AlignHCenter);
            itemrec->setFlags(Qt::ItemIsEnabled);
            QTableWidgetItem* itemplay = new QTableWidgetItem;
            addItem(i, DEVCOL_PLAY, itemplay, mdevView);
            itemplay->setTextAlignment(Qt::AlignHCenter);
            itemplay->setFlags(Qt::ItemIsEnabled);
            QTableWidgetItem* itemout = new QTableWidgetItem;
            addItem(i, DEVCOL_OUTROUTES, itemout, mdevView);
            itemout->setFlags(Qt::ItemIsEnabled);
            QTableWidgetItem* itemin = new QTableWidgetItem;
            addItem(i, DEVCOL_INROUTES, itemin, mdevView);
            itemin->setFlags(Qt::ItemIsEnabled);
            // Ignore synth devices. Default input routes make no sense for them (right now).
            QTableWidgetItem* itemdefin = new QTableWidgetItem((dev && dev->isSynti()) ? 
                                               QString() : MusECore::bitmap2String(port->defaultInChannels()));
            addItem(i, DEVCOL_DEF_IN_CHANS, itemdefin, mdevView);
            // Enabled: Use editor (not good). Disabled: Use pop-up menu. DELETETHIS
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
            
            // Turn on if and when multiple output routes are supported. DELETETHIS?
            #if 0
            QTableWidgetItem* itemdefout = new QTableWidgetItem(MusECore::bitmap2String(port->defaultOutChannels()));
            addItem(i, DEVCOL_DEF_OUT_CHANS, itemdefout, mdevView);
            itemdefout->setFlags(Qt::ItemIsEditable | Qt::ItemIsEnabled);
            #else
            QTableWidgetItem* itemdefout = new QTableWidgetItem(MusECore::bitmap2String(0));
            defochs = port->defaultOutChannels();
            if(defochs)
            {
              for(int ch = 0; ch < MIDI_CHANNELS; ++ch)
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

                  // REMOVE Tim. Persistent routes. Removed.
//                   // Is it a Jack midi device? Allow renaming.
//                   if (dev->deviceType() == MusECore::MidiDevice::JACK_MIDI)
//                        itemname->setFlags(Qt::ItemIsEditable | Qt::ItemIsEnabled);
                    
                  if (dev->rwFlags() & 0x2)
                       itemrec->setIcon(dev->openFlags() & 2 ? QIcon(*dotIcon) : QIcon(*dothIcon));
                  else
                       itemrec->setIcon(QIcon(QPixmap()));
                  if (dev->rwFlags() & 0x1)
                       itemplay->setIcon( dev->openFlags() & 1 ? QIcon(*dotIcon) : QIcon(*dothIcon));
                  else
                       itemplay->setIcon(QIcon(QPixmap()));
                  }
            else {
                  itemname->setText(tr("<none>"));
                  itemgui->setIcon(QIcon(*dothIcon));
                  itemrec->setIcon(QIcon(QPixmap()));
                  itemplay->setIcon(QIcon(QPixmap()));
                  }
            if (port->hasNativeGui())
                  itemgui->setIcon(port->nativeGuiVisible() ? QIcon(*dotIcon) : QIcon(*dothIcon));
            else
                  itemgui->setIcon(QIcon(QPixmap()));

            if (!(dev && dev->isSynti()))
                  iteminstr->setIcon(QIcon(*buttondownIcon));

            itemname->setIcon(QIcon(*buttondownIcon));


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
            
            if(i == no) sitem = itemno;
      }
      if(sitem)
         mdevView->setCurrentItem(sitem);
      
      QString s;
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
// REMOVE Tim. Persistent routes. Removed.
//       instanceList->clear();
//       MusECore::SynthIList* sl = MusEGlobal::song->syntis();
//       for (MusECore::iSynthI si = sl->begin(); si != sl->end(); ++si) {
//             QTreeWidgetItem* iitem = new QTreeWidgetItem(instanceList);
//             iitem->setText(0, (*si)->name());
//             iitem->setText(1, MusECore::synthType2String((*si)->synth()->synthType()));
//             if ((*si)->midiPort() == -1)
//                   s = tr("<none>");
//             else
//                   s.setNum((*si)->midiPort() + 1);
//             iitem->setText(2, s);
//             }
            
// REMOVE Tim. Persistent routes. Added.
      instanceList->clearContents();
      instanceList->setRowCount(MusEGlobal::midiDevices.size());
      int row_cnt = 0;
      for (MusECore::iMidiDevice imd = MusEGlobal::midiDevices.begin(); imd != MusEGlobal::midiDevices.end(); ++imd) {
            MusECore::MidiDevice* md = *imd;
            //QTreeWidgetItem* iitem = new QTreeWidgetItem(instanceList);
            
            QTableWidgetItem* iitem = new QTableWidgetItem(md->name());
            // We need the original name for the itemChanged handler, which also updates this string after changing.
            iitem->setData(Qt::UserRole, md->name());
            // Is it a Jack midi device? Allow renaming.
            if(md->deviceType() == MusECore::MidiDevice::JACK_MIDI)
              iitem->setFlags(Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
            else
              iitem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
            instanceList->setItem(row_cnt, INSTCOL_NAME, iitem);
            iitem = new QTableWidgetItem(md->deviceTypeString());
            iitem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
            instanceList->setItem(row_cnt, INSTCOL_TYPE, iitem);

//             iitem->setText(INSTCOL_NAME, md->name());
//             iitem->setText(INSTCOL_TYPE, md->deviceTypeString());
//             if(md->hasNativeGui())
//               iitem->setIcon(INSTCOL_GUI, md->nativeGuiVisible() ? QIcon(*dotIcon) : QIcon(*dothIcon));
//             else
//               iitem->setIcon(INSTCOL_GUI, QIcon(QPixmap()));
//             //iitem->setText(INSTCOL_STATE, md->());
//             if(md->rwFlags() & 0x2)
//               iitem->setIcon(INSTCOL_REC, md->openFlags() & 2 ? QIcon(*dotIcon) : QIcon(*dothIcon));
//             else
//               iitem->setIcon(INSTCOL_REC, QIcon(QPixmap()));
//             if(md->rwFlags() & 0x1)
//               iitem->setIcon(INSTCOL_PLAY, md->openFlags() & 1 ? QIcon(*dotIcon) : QIcon(*dothIcon));
//             else
//               iitem->setIcon(INSTCOL_PLAY, QIcon(QPixmap()));
//             iitem->setTextAlignment(INSTCOL_GUI, Qt::AlignHCenter);
//             iitem->setTextAlignment(INSTCOL_REC, Qt::AlignHCenter);
//             iitem->setTextAlignment(INSTCOL_PLAY, Qt::AlignHCenter);
            ++row_cnt;
            }
            
      instanceList->resizeColumnToContents(INSTCOL_NAME);
      instanceList->resizeColumnToContents(INSTCOL_TYPE);
      instanceList->resizeColumnToContents(INSTCOL_STATE);
//       instanceList->resizeColumnToContents(INSTCOL_REC);
//       instanceList->resizeColumnToContents(INSTCOL_PLAY);
//       instanceList->resizeColumnToContents(INSTCOL_GUI);
//       instanceList->header()->setSectionResizeMode(INSTCOL_REC ,QHeaderView::Fixed);
//       instanceList->header()->setSectionResizeMode(INSTCOL_PLAY ,QHeaderView::Fixed);
//       instanceList->header()->setSectionResizeMode(INSTCOL_GUI ,QHeaderView::Fixed);
      //instanceList->header()->setStretchLastSection( true );
      //instanceList->header()->setDefaultAlignment(Qt::AlignHCenter);
      instanceList->horizontalHeader()->setStretchLastSection( true );
      instanceList->horizontalHeader()->setDefaultAlignment(Qt::AlignHCenter);
      deviceSelectionChanged();
      
      synthList->resizeColumnToContents(1);
      mdevView->resizeColumnsToContents();
      mdevView->horizontalHeader()->setSectionResizeMode(DEVCOL_NO ,QHeaderView::Fixed);
      mdevView->horizontalHeader()->setSectionResizeMode(DEVCOL_REC ,QHeaderView::Fixed);
      mdevView->horizontalHeader()->setSectionResizeMode(DEVCOL_PLAY ,QHeaderView::Fixed);
      mdevView->horizontalHeader()->setSectionResizeMode(DEVCOL_GUI ,QHeaderView::Fixed);
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
      for (int i = 0; i < MIDI_PORTS; ++i) {
            MusECore::MidiPort* port  = &MusEGlobal::midiPorts[i];
            MusECore::MidiDevice* dev = port->device();
            if (dev==0) {
                  MusEGlobal::midiSeq->msgSetMidiDevice(port, si);
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
      //QTreeWidgetItem* item = instanceList->currentItem();
      QTableWidgetItem* item = instanceList->currentItem();
      if (item == 0)
            return;
      // REMOVE Tim. Persistent routes. Removed.
//       MusECore::SynthIList* sl = MusEGlobal::song->syntis();
//       MusECore::iSynthI ii;
//       for (ii = sl->begin(); ii != sl->end(); ++ii) {
//             if( (*ii)->iname() == item->text(0) && 
//                  MusECore::synthType2String((*ii)->synth()->synthType()) == item->text(1) )
//               break;
//             }
//       if (ii == sl->end()) {
//             printf("synthesizerConfig::removeInstanceClicked(): synthi not found\n");
//             return;
//             }
//       MusEGlobal::audio->msgRemoveTrack(*ii);
      
      // REMOVE Tim. Persistent routes. Added.
      //MusECore::SynthIList* sl = MusEGlobal::song->syntis();
      QString name = item->tableWidget()->item(item->row(), INSTCOL_NAME)->text();
      QString type = item->tableWidget()->item(item->row(), INSTCOL_TYPE)->text();
      
      MusECore::MidiDevice* md = 0;
      MusECore::iMidiDevice imd;
      for (imd = MusEGlobal::midiDevices.begin(); imd != MusEGlobal::midiDevices.end(); ++imd) {
            md = *imd;
            if(md->name() == name && md->deviceTypeString() == type)
              break;
            }
      if (imd == MusEGlobal::midiDevices.end()) {
            fprintf(stderr, "synthesizerConfig::removeInstanceClicked(): device not found\n");
            return;
            }
            
      // Is it an ALSA midi device? 
      // TODO: For now, don't allow creating/removing/renaming them until we decide on addressing strategy.
      if(md->deviceType() == MusECore::MidiDevice::ALSA_MIDI)
      {
        snd_seq_addr_t* addr = static_cast<snd_seq_addr_t*>(md->inClientPort());
        // Allow removing ('purging') an unavailable ALSA device.
        if(addr->client != SND_SEQ_ADDRESS_UNKNOWN && addr->port != SND_SEQ_ADDRESS_UNKNOWN)
          return;
      }
      
      MusECore::SynthI* s = dynamic_cast<MusECore::SynthI*>(md);
      if(s)
        MusEGlobal::audio->msgRemoveTrack(s);
      else
      {
        MusEGlobal::audio->msgIdle(true); // Make it safe to edit structures
        if(md->midiPort() != -1)
          MusEGlobal::midiPorts[md->midiPort()].setMidiDevice(0);
        MusEGlobal::midiDevices.erase(imd);
        MusEGlobal::audio->msgIdle(false);
        MusEGlobal::song->update(SC_CONFIG);
      }
      
      }

// REMOVE Tim. Persistent routes. Added.
//---------------------------------------------------------
//   deviceItemClicked
//---------------------------------------------------------

//void MPConfig::deviceItemClicked(QTreeWidgetItem* item, int col)
void MPConfig::deviceItemClicked(QTableWidgetItem* item)
{
      if(!item)
        return;
      int col = item->column();
      QString name = item->tableWidget()->item(item->row(), INSTCOL_NAME)->text();
      QString type = item->tableWidget()->item(item->row(), INSTCOL_TYPE)->text();
      MusECore::iMidiDevice imd;
      for (imd = MusEGlobal::midiDevices.begin(); imd != MusEGlobal::midiDevices.end(); ++imd) {
            MusECore::MidiDevice* md = *imd;
            if(md->name() == name && md->deviceTypeString() == type)
              break;
            }
      if (imd == MusEGlobal::midiDevices.end()) {
            fprintf(stderr, "synthesizerConfig::deviceItemClicked(): device not found\n");
            return;
            }
      //MusECore::MidiDevice* md = *imd;
      //int rwFlags   = md->rwFlags();
      //int openFlags = md->openFlags();
      switch(col)
      {
//         case INSTCOL_REC:
//                   if(!(rwFlags & 2))
//                         return;
//                   openFlags ^= 0x2;
//                   MusEGlobal::audio->msgIdle(true);  // Make it safe to edit structures
//                   md->setOpenFlags(openFlags);
//                   if(md->midiPort() != -1)
//                     MusEGlobal::midiPorts[md->midiPort()].setMidiDevice(md); // reopen device
//                   MusEGlobal::audio->msgIdle(false);
//                   item->setIcon(INSTCOL_REC, openFlags & 2 ? QIcon(*dotIcon) : QIcon(*dothIcon));
//                   
// //                   if(md->deviceType() == MusECore::MidiDevice::JACK_MIDI)
// //                   {
// //                     if(md->openFlags() & 2)  
// //                     {
// //                       //item->tableWidget()->item(item->row(), DEVCOL_INROUTES)->setIcon(QIcon(*buttondownIcon));
// //                       //item->tableWidget()->item(item->row(), DEVCOL_INROUTES)->setText(tr("in"));
// //                     }
// //                     else
// //                     {
// //                       //item->tableWidget()->item(item->row(), DEVCOL_INROUTES)->setIcon(QIcon());
// //                       //item->tableWidget()->item(item->row(), DEVCOL_INROUTES)->setText("");
// //                     }  
// //                   }
//                   return;
//         case INSTCOL_PLAY:
//                   if(!(rwFlags & 1))
//                         return;
//                   openFlags ^= 0x1;
//                   MusEGlobal::audio->msgIdle(true);  // Make it safe to edit structures
//                   md->setOpenFlags(openFlags);
//                   if(md->midiPort() != -1)
//                     MusEGlobal::midiPorts[md->midiPort()].setMidiDevice(md); // reopen device
//                   MusEGlobal::audio->msgIdle(false);
//                   item->setIcon(INSTCOL_PLAY, openFlags & 1 ? QIcon(*dotIcon) : QIcon(*dothIcon));
//                   
// //                   if(md->deviceType() == MusECore::MidiDevice::JACK_MIDI)
// //                   {
// //                     if(md->openFlags() & 1)  
// //                     {
// //                       //item->tableWidget()->item(item->row(), DEVCOL_OUTROUTES)->setIcon(QIcon(*buttondownIcon));
// //                       //item->tableWidget()->item(item->row(), DEVCOL_OUTROUTES)->setText(tr("out"));
// //                     }
// //                     else  
// //                     {
// //                       //item->tableWidget()->item(item->row(), DEVCOL_OUTROUTES)->setIcon(QIcon());
// //                       //item->tableWidget()->item(item->row(), DEVCOL_OUTROUTES)->setText("");
// //                     }
// //                   }
//                   return;
//         case INSTCOL_GUI:
//                   if(md->hasNativeGui())
//                   {
//                     md->showNativeGui(!md->nativeGuiVisible());
//                     item->setIcon(INSTCOL_GUI, md->nativeGuiVisible() ? QIcon(*dotIcon) : QIcon(*dothIcon));
//                   }
//                   return;
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

//---------------------------------------------------------
//   addAlsaDeviceClicked
//---------------------------------------------------------

void MPConfig::addAlsaDeviceClicked()
{
  
}
      
//---------------------------------------------------------
//   beforeDeviceContextShow
//---------------------------------------------------------

void MPConfig::beforeDeviceContextShow(PopupMenu* /*menu*/, QAction* menuAction, QMenu* /*ctxMenu*/)
{
  fprintf(stderr, "MPConfig::beforeDeviceContextShow:%s\n", menuAction->text().toLatin1().constData());
}
      
//---------------------------------------------------------
//   deviceContextTriggered
//---------------------------------------------------------

void MPConfig::deviceContextTriggered(QAction* act)
{
  fprintf(stderr, "MPConfig::deviceRemoveTriggered:%s\n", act->text().toLatin1().constData());
  if(act)
  {
 
    
    PopupMenu* menu = 0;
    QAction* action = 0;
    QVariant var_val = 0;
    
    menu = act->data().value<PopupMenuContextData>().menu();
    action = act->data().value<PopupMenuContextData>().action();
    var_val = act->data().value<PopupMenuContextData>().varValue();

    MusECore::MidiDevice* md = MusEGlobal::midiDevices.find(action->text());
    
    fprintf(stderr, " menu:%p action:%p var:%x md:%p\n", menu, action, var_val.toInt(), md);
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

