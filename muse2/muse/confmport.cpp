//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: confmport.cpp,v 1.9.2.10 2009/12/15 03:39:58 terminator356 Exp $
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
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

extern std::vector<Synth*> synthis;

enum { DEVCOL_NO = 0, DEVCOL_GUI, DEVCOL_REC, DEVCOL_PLAY, DEVCOL_INSTR, DEVCOL_NAME,
       DEVCOL_INROUTES, DEVCOL_OUTROUTES, DEVCOL_DEF_IN_CHANS, DEVCOL_DEF_OUT_CHANS, DEVCOL_STATE };  

//---------------------------------------------------------
//   closeEvent
//---------------------------------------------------------
void MPConfig::closeEvent(QCloseEvent *event)
{
    QSettings settings("MusE", "MusE-qt");
    settings.setValue("MPConfig/geometry", saveGeometry());
    QWidget::closeEvent(event);
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
  int defch = midiPorts[no].defaultInChannels();  
  
  if(actid == MIDI_CHANNELS + 1)  // Apply to all tracks now.
  {
    // Are there tracks, and is there a port device? 
    // Tested: Hmm, allow ports with no device since that is a valid situation.
    if(!song->midis()->empty()) // && midiPorts[no].device())  
    {
      int ret = QMessageBox::question(this, tr("Default input connections"),
                                    tr("Are you sure you want to apply to all existing midi tracks now?"),
                                    QMessageBox::Ok | QMessageBox::Cancel,
                                    QMessageBox::Cancel);
      if(ret == QMessageBox::Ok) 
      {
        MidiTrackList* mtl = song->midis();
        for(iMidiTrack it = mtl->begin(); it != mtl->end(); ++it)
        {
          // Remove all routes from this port to the tracks first.
          audio->msgRemoveRoute(Route(no, allch), Route(*it, allch));
          if(defch)
            audio->msgAddRoute(Route(no, defch), Route(*it, defch));
        }  
        //audio->msgUpdateSoloStates();
        song->update(SC_ROUTE);                    
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
    midiPorts[no].setDefaultInChannels(chbits);
    mdevView->item(item->row(), DEVCOL_DEF_IN_CHANS)->setText(MusEUtil::bitmap2String(chbits));
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
  int defch = midiPorts[no].defaultOutChannels();  
  // Turn on if and when multiple output routes are supported.
  #if 0
  int allch = (1 << MIDI_CHANNELS) - 1;  
  #endif
  
  if(actid == MIDI_CHANNELS + 1)  // Apply to all tracks now.
  {
    // Are there tracks, and is there a port device? 
    // Tested: Hmm, allow ports with no device since that is a valid situation.
    if(!song->midis()->empty()) // && midiPorts[no].device())
    {
      int ret = QMessageBox::question(this, tr("Default output connections"),
                                    tr("Are you sure you want to apply to all existing midi tracks now?"),
                                    QMessageBox::Ok | QMessageBox::Cancel,
                                    QMessageBox::Cancel);
      if(ret == QMessageBox::Ok) 
      {
        MidiTrackList* mtl = song->midis();
        // Turn on if and when multiple output routes are supported.
        #if 0
        for(iMidiTrack it = mtl->begin(); it != mtl->end(); ++it)
        {
          // Remove all routes from this port to the tracks first.
          audio->msgRemoveRoute(Route(no, allch), Route(*it, allch));
          if(defch)
            audio->msgAddRoute(Route(no, defch), Route(*it, defch));
        }  
        audio->msgUpdateSoloStates();
        song->update(SC_ROUTE);                    
        #else
        int ch = 0;
        for( ; ch < MIDI_CHANNELS; ++ch)
          if(defch & (1 << ch)) break;
           
        audio->msgIdle(true);
        for(iMidiTrack it = mtl->begin(); it != mtl->end(); ++it)
        {
          // Leave drum track channel at current setting.
          if((*it)->type() == Track::DRUM)
            (*it)->setOutPortAndUpdate(no);
          else
            (*it)->setOutPortAndChannelAndUpdate(no, ch);
        }  
        audio->msgIdle(false);
        audio->msgUpdateSoloStates();
        song->update(SC_MIDI_TRACK_PROP);                    
        #endif
      }
    }  
  }
  else
  {
    #if 0          // Turn on if and when multiple output routes are supported.
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
    midiPorts[no].setDefaultOutChannels(chbits);
    mdevView->item(item->row(), DEVCOL_DEF_OUT_CHANS)->setText(MusEUtil::bitmap2String(chbits));
    #else
    if(actid < MIDI_CHANNELS)
    {
      int chbits = 1 << actid;
      // Multiple out routes not supported. Make the setting exclusive to this port - exclude all other ports.
      setPortExclusiveDefOutChan(no, chbits);
      int j = mdevView->rowCount();
      for(int i = 0; i < j; ++i)
        mdevView->item(i, DEVCOL_DEF_OUT_CHANS)->setText(MusEUtil::bitmap2String(i == no ? chbits : 0));
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
    #endif
  }  
}

//---------------------------------------------------------
//   mdevViewItemRenamed
//---------------------------------------------------------

void MPConfig::mdevViewItemRenamed(QTableWidgetItem* item)
{
  int col = item->column();
  QString s = item->text();
  //printf("MPConfig::mdevViewItemRenamed col:%d txt:%s\n", col, s.toLatin1().constData());
  if(item == 0)
    return;
  switch(col)
  {
    // Enabled: Use editor (Not good - only responds if text changed. We need to respond always).
    // Disabled: Use pop-up menu.
    #if 0
    case DEVCOL_DEF_IN_CHANS:
    {
      QString id = item->tableWidget()->item(item->row(), DEVCOL_NO)->text();
      int no = atoi(id.toLatin1().constData()) - 1;
      if(no < 0 || no >= MIDI_PORTS)
        return;
      int allch = (1 << MIDI_CHANNELS) - 1;  
      int ch = allch & string2bitmap(s);  
      midiPorts[no].setDefaultInChannels(ch);
      
      if(!song->midis()->empty() && midiPorts[no].device())  // Only if there are tracks, and device is valid. 
      {
        int ret = QMessageBox::question(this, tr("Default input connections"),
                                      tr("Setting will apply to new midi tracks.\n"
                                          "Do you want to apply to all existing midi tracks now?"),
                                      QMessageBox::Yes | QMessageBox::No,
                                      QMessageBox::No);
        if(ret == QMessageBox::Yes) 
        {
          MidiTrackList* mtl = song->midis();
          for(iMidiTrack it = mtl->begin(); it != mtl->end(); ++it)
          {
            // Remove all routes from this port to the tracks first.
            audio->msgRemoveRoute(Route(no, allch), Route(*it, allch));
            if(ch)
              audio->msgAddRoute(Route(no, ch), Route(*it, ch));
          }  
        }  
      }
      song->update();
    }
    break;    
    #endif
    
    // Enabled: Use editor (Not good - only responds if text changed. We need to respond always).
    // Disabled: Use pop-up menu.
    // Only turn on if and when multiple output routes are supported.
    #if 0
    case DEVCOL_DEF_OUT_CHANS:
    {
      QString id = item->tableWidget()->item(item->row(), DEVCOL_NO)->text();
      int no = atoi(id.toLatin1().constData()) - 1;
      if(no < 0 || no >= MIDI_PORTS)
        return;
      int allch = (1 << MIDI_CHANNELS) - 1;  
      int ch = allch & string2bitmap(s);  
      midiPorts[no].setDefaultOutChannels(ch);
      
      if(!song->midis()->empty() && midiPorts[no].device())  // Only if there are tracks, and device is valid. 
      {
        int ret = QMessageBox::question(this, tr("Default output connections"),
                                      tr("Setting will apply to new midi tracks.\n"
                                          "Do you want to apply to all existing midi tracks now?"),
                                      QMessageBox::Yes | QMessageBox::No,
                                      QMessageBox::No);
        if(ret == QMessageBox::Yes) 
        {
          MidiTrackList* mtl = song->midis();
          for(iMidiTrack it = mtl->begin(); it != mtl->end(); ++it)
          {
            // Remove all routes from the tracks to this port first.
            audio->msgRemoveRoute(Route(*it, allch), Route(no, allch));
            if(ch)
              audio->msgAddRoute(Route(*it, ch), Route(no, ch));
          }  
        }  
      }
      song->update();
    }
    break;    
    # endif
    
    case DEVCOL_NAME:
    {
      QString id = item->tableWidget()->item(item->row(), DEVCOL_NO)->text();
      int no = atoi(id.toLatin1().constData()) - 1;
      if(no < 0 || no >= MIDI_PORTS)
        return;

      MidiPort* port      = &midiPorts[no];
      MidiDevice* dev     = port->device();
      // Only Jack midi devices.
      if(!dev || dev->deviceType() != MidiDevice::JACK_MIDI)
        return;
      if(dev->name() == s)
        return;  
        
      if(midiDevices.find(s))
      {
        QMessageBox::critical(this,
            tr("MusE: bad device name"),
            tr("please choose a unique device name"),
            QMessageBox::Ok,
            Qt::NoButton,
            Qt::NoButton);
        songChanged(-1);
        return;
      }
      dev->setName(s);
      song->update();
    }
    break;    
    default: 
      //printf("MPConfig::mdevViewItemRenamed unknown column clicked col:%d txt:%s\n", col, s.toLatin1().constData());
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
      MidiPort* port      = &midiPorts[no];
      MidiDevice* dev     = port->device();
      int rwFlags         = dev ? dev->rwFlags() : 0;
      int openFlags       = dev ? dev->openFlags() : 0;
      QTableWidget* listView = item->tableWidget();
      QPoint ppt          = listView->visualItemRect(item).bottomLeft();
      QPoint mousepos     = QCursor::pos();
      int col = item->column();
      ppt += QPoint(0, listView->horizontalHeader()->height());
      ppt  = listView->mapToGlobal(ppt);

      switch (col) {
            case DEVCOL_GUI:
                  if (dev == 0)
                        return;
                  //if (port->hasGui())
                  if (port->hasNativeGui())
                  {
                        //bool v = port->nativeGuiVisible()
                        //port->instrument()->showGui(!port->guiVisible());
                        port->instrument()->showNativeGui(!port->nativeGuiVisible());
                        //port->instrument()->showNativeGui(!v);
                        //item->setIcon(port->guiVisible() ? QIcon(*dotIcon) : QIcon(*dothIcon));
                        item->setIcon(port->nativeGuiVisible() ? QIcon(*dotIcon) : QIcon(*dothIcon));
                        //item->setIcon(!v ? QIcon(*dotIcon) : QIcon(*dothIcon));
                  }
                  return;
                  
            case DEVCOL_REC:
                  if (dev == 0 || !(rwFlags & 2))
                        return;
                  openFlags ^= 0x2;
                  dev->setOpenFlags(openFlags);
                  midiSeq->msgSetMidiDevice(port, dev);       // reopen device
                  item->setIcon(openFlags & 2 ? QIcon(*dotIcon) : QIcon(*dothIcon));
                  
                  if(dev->deviceType() == MidiDevice::JACK_MIDI)
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
                  
            case DEVCOL_PLAY:
                  if (dev == 0 || !(rwFlags & 1))
                        return;
                  openFlags ^= 0x1;
                  dev->setOpenFlags(openFlags);
                  midiSeq->msgSetMidiDevice(port, dev);       // reopen device
                  item->setIcon(openFlags & 1 ? QIcon(*dotIcon) : QIcon(*dothIcon));
                  
                  if(dev->deviceType() == MidiDevice::JACK_MIDI)
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
                  
            case DEVCOL_INROUTES:  
            case DEVCOL_OUTROUTES:
                  {
                    if(!MusEGlobal::checkAudioDevice())
                      return;
                      
                    if(audioDevice->deviceType() != AudioDevice::JACK_AUDIO)  // Only if Jack is running.
                      return;
                      
                    if(!dev)
                      return;
                    
                    // Only Jack midi devices.
                    //MidiJackDevice* mjd = dynamic_cast<MidiJackDevice*>(dev);
                    //if(!mjd)
                    if(dev->deviceType() != MidiDevice::JACK_MIDI)  
                      return;
                    
                    //if(!(dev->rwFlags() & ((col == DEVCOL_OUTROUTES) ? 1 : 2)))    
                    if(!(dev->openFlags() & ((col == DEVCOL_OUTROUTES) ? 1 : 2)))    
                      return;
                      
                    //RouteList* rl = (dev->rwFlags() & 1) ? dev->outRoutes() : dev->inRoutes();
                    RouteList* rl = (col == DEVCOL_OUTROUTES) ? dev->outRoutes() : dev->inRoutes();   
                    QMenu* pup = 0;
                    int gid = 0;
                    std::list<QString> sl;
                    pup = new QMenu(this);
                    
        _redisplay:
                    pup->clear();
                    gid = 0;
                    
                    // Jack input ports if device is writable, and jack output ports if device is readable.
                    //sl = (dev->rwFlags() & 1) ? audioDevice->inputPorts(true, _showAliases) : audioDevice->outputPorts(true, _showAliases);
                    sl = (col == DEVCOL_OUTROUTES) ? audioDevice->inputPorts(true, _showAliases) : audioDevice->outputPorts(true, _showAliases);
                    
                    //for (int i = 0; i < channel; ++i) 
                    //{
                      //char buffer[128];
                      //snprintf(buffer, 128, "%s %d", tr("Channel").toLatin1().constData(), i+1);
                      //MenuTitleItem* titel = new MenuTitleItem(QString(buffer));
                      //pup->insertItem(titel);
  
                      QAction* act;
                      
                      act = pup->addAction(tr("Show first aliases"));
                      act->setData(gid);
                      act->setCheckable(true);
                      act->setChecked(_showAliases == 0);
                      ++gid;
                      
                      act = pup->addAction(tr("Show second aliases"));
                      act->setData(gid);
                      act->setCheckable(true);
                      act->setChecked(_showAliases == 1);
                      ++gid;
                      
                      pup->addSeparator();
                      for(std::list<QString>::iterator ip = sl.begin(); ip != sl.end(); ++ip) 
                      {
                        act = pup->addAction(*ip);
                        act->setData(gid);
                        act->setCheckable(true);
                        
                        //Route dst(*ip, true, i);
                        //Route rt(*ip, (dev->rwFlags() & 1), -1, Route::JACK_ROUTE);
                        Route rt(*ip, (col == DEVCOL_OUTROUTES), -1, Route::JACK_ROUTE);   
                        for(ciRoute ir = rl->begin(); ir != rl->end(); ++ir) 
                        {
                          if (*ir == rt) 
                          {
                            act->setChecked(true);
                            break;
                          }
                        }
                        ++gid;
                      }
                      //if (i+1 != channel)
                      //      pup->insertSeparator();
                    //}
                    
                    act = pup->exec(ppt);
                    if(act)
                    {
                      n = act->data().toInt();
                      if(n == 0) // Show first aliases
                      {
                        //delete pup;
                        if(_showAliases == 0)
                          _showAliases = -1;
                        else  
                          _showAliases = 0;
                        goto _redisplay;   // Go back
                      }
                      else
                      if(n == 1) // Show second aliases
                      {
                        //delete pup;
                        if(_showAliases == 1)
                          _showAliases = -1;
                        else  
                          _showAliases = 1;
                        goto _redisplay;   // Go back
                      }
                      
                      QString s(act->text());
                      
                      //if(dev->rwFlags() & 1) // Writeable
                      if(col == DEVCOL_OUTROUTES) // Writeable  
                      {
                        Route srcRoute(dev, -1);
                        Route dstRoute(s, true, -1, Route::JACK_ROUTE);
            
                        ciRoute iir = rl->begin();
                        for(; iir != rl->end(); ++iir) 
                        {
                          if(*iir == dstRoute)
                            break;
                        }
                        if(iir != rl->end()) 
                          // disconnect
                          audio->msgRemoveRoute(srcRoute, dstRoute);
                        else 
                          // connect
                          audio->msgAddRoute(srcRoute, dstRoute);
                      }
                      else
                      //if(dev->rwFlags() & 2) // Readable
                      //if(col == DEVCOL_INROUTES) // Readable    
                      {
                        Route srcRoute(s, false, -1, Route::JACK_ROUTE);
                        Route dstRoute(dev, -1);
            
                        ciRoute iir = rl->begin();
                        for(; iir != rl->end(); ++iir) 
                        {
                          if(*iir == srcRoute)
                            break;
                        }
                        if(iir != rl->end()) 
                          // disconnect
                          audio->msgRemoveRoute(srcRoute, dstRoute);
                        else 
                          // connect
                          audio->msgAddRoute(srcRoute, dstRoute);
                      }  
                      
                      audio->msgUpdateSoloStates();
                      song->update(SC_ROUTE);
                      
                      //delete pup;
                      // FIXME:
                      // Routes can't be re-read until the message sent from msgAddRoute1() 
                      //  has had time to be sent and actually affected the routes.
                      ///goto _redisplay;   // Go back
                    }  
                    delete pup;
                    //iR->setDown(false);     // pup->exec() catches mouse release event
                  }
                  //break;
                  return;
                  
            case DEVCOL_DEF_IN_CHANS:
                  // Enabled: Use editor (Not good - only responds if text changed. We need to respond always).
                  // Disabled: Use pop-up menu.
                  #if 0
                  return;
                  #else
                  {
                    defpup = new MusEWidget::PopupMenu(this, true);
                    defpup->addAction(new MusEWidget::MenuTitleItem("Channel", defpup)); 
                    QAction* act = 0;
                    int chbits = midiPorts[no].defaultInChannels();
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
                    act->setEnabled(!song->midis()->empty());  // && midiPorts[no].device());
                    
                    connect(defpup, SIGNAL(triggered(QAction*)), SLOT(changeDefInputRoutes(QAction*)));
                    //connect(defpup, SIGNAL(aboutToHide()), MusEGlobal::muse, SLOT(routingPopupMenuAboutToHide()));
                    //defpup->popup(QCursor::pos());
                    defpup->exec(QCursor::pos());
                    delete defpup;
                    defpup = 0;
                  }
                  return;                  
                  #endif
                  
            case DEVCOL_DEF_OUT_CHANS:
                  // Enabled: Use editor (Not good - only responds if text changed. We need to respond always).
                  // Disabled: Use pop-up menu.
                  // Only turn on if and when multiple output routes are supported.
                  #if 0
                  return;
                  #else
                  {
                    defpup = new MusEWidget::PopupMenu(this, true);
                    defpup->addAction(new MusEWidget::MenuTitleItem("Channel", defpup)); 
                    QAction* act = 0;
                    int chbits = midiPorts[no].defaultOutChannels();
                    for(int i = 0; i < MIDI_CHANNELS; ++i) 
                    {
                      act = defpup->addAction(QString().setNum(i + 1));
                      act->setData(i);
                      act->setCheckable(true);
                      act->setChecked((1 << i) & chbits);
                    }  
                    
                    // Turn on if and when multiple output routes are supported.
                    #if 0
                    act = defpup->addAction(tr("Toggle all"));
                    act->setData(MIDI_CHANNELS);
                    #endif
                    
                    defpup->addSeparator();
                    act = defpup->addAction(tr("Change all tracks now"));
                    act->setData(MIDI_CHANNELS + 1);
                    // Enable only if there are tracks, and port has a device.
                    // Tested: Hmm, allow ports with no device since that is a valid situation.
                    act->setEnabled(!song->midis()->empty());  // && midiPorts[no].device());
                    
                    connect(defpup, SIGNAL(triggered(QAction*)), SLOT(changeDefOutputRoutes(QAction*)));
                    //connect(defpup, SIGNAL(aboutToHide()), MusEGlobal::muse, SLOT(routingPopupMenuAboutToHide()));
                    //defpup->popup(QCursor::pos());
                    defpup->exec(QCursor::pos());
                    delete defpup;
                    defpup = 0;
                  }
                  return;
                  #endif
                  
            case DEVCOL_NAME:
                  {
                    //printf("MPConfig::rbClicked DEVCOL_NAME\n");
                    
                    // Did we click in the text area?
                    if((mousepos.x() - ppt.x()) > buttondownIcon->width())
                    {
                      //printf("MPConfig::rbClicked starting item rename... enabled?:%d\n", item->renameEnabled(DEVCOL_NAME));
                      // Start the renaming of the cell...
		      QModelIndex current = item->tableWidget()->currentIndex();
		      if (item->flags() & Qt::ItemIsEditable)
			item->tableWidget()->edit(current.sibling(current.row(), DEVCOL_NAME));
                        
                      return;
                    }
                    else
                    // We clicked the 'down' button.
                    {
                      QMenu* pup = new QMenu(this);
                      
                      QAction* act;
                      
                      // Could do it this way...
                      //act = pup->addAction(tr("Create") + QT_TRANSLATE_NOOP("@default", " Jack") + tr(" input"));
                      //act = pup->addAction(tr("Create") + QT_TRANSLATE_NOOP("@default", " Jack") + tr(" output"));
                      //act = pup->addAction(tr("Create") + QT_TRANSLATE_NOOP("@default", " Jack") + tr(" combo"));
                      // ... or keep it simple and let the user click on the green lights instead.
                      act = pup->addAction(tr("Create") + QT_TRANSLATE_NOOP("@default", " Jack") + tr(" device"));
                      act->setData(0);
                      
                      typedef std::map<std::string, int > asmap;
                      typedef std::map<std::string, int >::iterator imap;
                      
                      asmap mapALSA;
                      asmap mapJACK;
                      asmap mapSYNTH;
                      
                      int aix = 0x10000000;
                      int jix = 0x20000000;
                      int six = 0x30000000;
                      for(iMidiDevice i = midiDevices.begin(); i != midiDevices.end(); ++i) 
                      {
                        //devALSA = dynamic_cast<MidiAlsaDevice*>(*i);
                        //if(devALSA)
                        if((*i)->deviceType() == MidiDevice::ALSA_MIDI)
                        {
                          //mapALSA.insert( std::pair<std::string, int> (std::string(devALSA->name().lower().toLatin1().constData()), ii) );
                          mapALSA.insert( std::pair<std::string, int> (std::string((*i)->name().toLatin1().constData()), aix) );
                          ++aix;
                        }  
                        else
                        if((*i)->deviceType() == MidiDevice::JACK_MIDI)
                        {  
                          //devJACK = dynamic_cast<MidiJackDevice*>(*i);
                          //if(devJACK)
                            //mapJACK.insert( std::pair<std::string, int> (std::string(devJACK->name().lower().toLatin1().constData()), ii) );
                          mapJACK.insert( std::pair<std::string, int> (std::string((*i)->name().toLatin1().constData()), jix) );
                          ++jix;
                        }
                        else
                        if((*i)->deviceType() == MidiDevice::SYNTH_MIDI)
                        {
                          mapSYNTH.insert( std::pair<std::string, int> (std::string((*i)->name().toLatin1().constData()), six) );
                          ++six;  
                        }
                        else
                          printf("MPConfig::rbClicked unknown midi device: %s\n", (*i)->name().toLatin1().constData());
                      }
                      
                      //int sz = midiDevices.size();
                      //if(!mapALSA.empty())
                      {
                        pup->addSeparator();
                        pup->addAction(new MusEWidget::MenuTitleItem(QT_TRANSLATE_NOOP("@default", "ALSA:"), pup));
                        
                        for(imap i = mapALSA.begin(); i != mapALSA.end(); ++i) 
                        {
                          int idx = i->second;
                          //if(idx > sz)           // Sanity check
                          //  continue;
                          QString s(i->first.c_str());
                          MidiDevice* md = midiDevices.find(s, MidiDevice::ALSA_MIDI);
                          if(md)
                          {
                            //if(!dynamic_cast<MidiAlsaDevice*>(md))
                            if(md->deviceType() != MidiDevice::ALSA_MIDI)  
                              continue;
                              
                            act = pup->addAction(QT_TRANSLATE_NOOP("@default", md->name()));
                            act->setData(idx);
                            act->setCheckable(true);
                            act->setChecked(md == dev);
                          }  
                        }
                      }  
                      
                      if(!mapSYNTH.empty())
                      {
                        pup->addSeparator();
                        pup->addAction(new MusEWidget::MenuTitleItem(QT_TRANSLATE_NOOP("@default", "SYNTH:"), pup));
                        
                        for(imap i = mapSYNTH.begin(); i != mapSYNTH.end(); ++i) 
                        {
                          int idx = i->second;
                          //if(idx > sz)           
                          //  continue;
                          QString s(i->first.c_str());
                          MidiDevice* md = midiDevices.find(s, MidiDevice::SYNTH_MIDI);
                          if(md)
                          {
                            //if(!dynamic_cast<MidiJackDevice*>(md))
                            if(md->deviceType() != MidiDevice::SYNTH_MIDI)  
                              continue;
                              
                            act = pup->addAction(QT_TRANSLATE_NOOP("@default", md->name()));
                            act->setData(idx);
                            act->setCheckable(true);
                            act->setChecked(md == dev);
                          }  
                        }
                      }  
                      
                      //if(!mapJACK.empty())
                      {
                        pup->addSeparator();
                        pup->addAction(new MusEWidget::MenuTitleItem(QT_TRANSLATE_NOOP("@default", "JACK:"), pup));
                        
                        for(imap i = mapJACK.begin(); i != mapJACK.end(); ++i) 
                        {
                          int idx = i->second;
                          //if(idx > sz)           
                          //  continue;
                          QString s(i->first.c_str());
                          MidiDevice* md = midiDevices.find(s, MidiDevice::JACK_MIDI);
                          if(md)
                          {
                            //if(!dynamic_cast<MidiJackDevice*>(md))
                            if(md->deviceType() != MidiDevice::JACK_MIDI)  
                              continue;
                              
                            act = pup->addAction(QT_TRANSLATE_NOOP("@default", md->name()));
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
                        //break;
                        return;
                      }
                      
                      n = act->data().toInt();
                      //printf("MPConfig::rbClicked n:%d\n", n);
                      
                      MidiDevice* sdev = 0;
                      if(n < 0x10000000)
                      {
                        delete pup;
                        if(n <= 2)  
                        {
                          sdev = MidiJackDevice::createJackMidiDevice(); 

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
                          typ = MidiDevice::ALSA_MIDI;
                        else  
                        if(n < 0x30000000)
                          typ = MidiDevice::JACK_MIDI;
                        else  
                        //if(n < 0x40000000)
                          typ = MidiDevice::SYNTH_MIDI;
                        
                        sdev = midiDevices.find(act->text(), typ);
                        delete pup;
                        // Is it the current device? Reset it to <none>.
                        if(sdev == dev)
                          sdev = 0;
                      }    

                      midiSeq->msgSetMidiDevice(port, sdev);
		      MusEGlobal::muse->changeConfig(true);     // save configuration file
                      song->update();
                    }  
                  }
                  //break;
                  return;

            case DEVCOL_INSTR:
                  {
                  if (dev && dev->isSynti())
                        //break;
                        return;
                  if (instrPopup == 0)
                        instrPopup = new QMenu(this);
                  instrPopup->clear();
                  for (iMidiInstrument i = midiInstruments.begin(); i
                     != midiInstruments.end(); ++i) 
                     {
                        // By T356.
                        // Do not list synths. Although it is possible to assign a synth
                        //  as an instrument to a non-synth device, we should not allow this.
                        // (One reason is that the 'show gui' column is then enabled, which
                        //  makes no sense for a non-synth device).
                        SynthI* si = dynamic_cast<SynthI*>(*i);
                        if(!si)
                          instrPopup->addAction((*i)->iname());
                     }
                  
                  QAction* act = instrPopup->exec(ppt);
                  if(!act)
                    //break;
                    return;
                  QString s = act->text();
		  item->tableWidget()->item(item->row(), DEVCOL_INSTR)->setText(s);
                  for (iMidiInstrument i = midiInstruments.begin(); i
                     != midiInstruments.end(); ++i) {
                        if ((*i)->iname() == s) {
                              port->setInstrument(*i);
                              break;
                              }
                        }
                  song->update();
                  }
                  //break;
                  return;
            }
      //songChanged(-1);
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
            //case DEVCOL_ROUTES: item->setToolTip(tr("Jack midi ports")); break;
            case DEVCOL_INROUTES:  item->setToolTip(tr("Connections from Jack Midi outputs")); break;
            case DEVCOL_OUTROUTES: item->setToolTip(tr("Connections to Jack Midi inputs")); break;
            case DEVCOL_DEF_IN_CHANS:   item->setToolTip(tr("Auto-connect these channels to new midi tracks")); break;
            // Turn on if and when multiple output routes are supported.
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
            //case DEVCOL_ROUTES:
            //      item->setWhatsThis(tr("Jack midi ports")); break;
            case DEVCOL_INROUTES:
                  item->setWhatsThis(tr("Connections from Jack Midi output ports")); break;
            case DEVCOL_OUTROUTES:
                  item->setWhatsThis(tr("Connections to Jack Midi input ports")); break;
            case DEVCOL_DEF_IN_CHANS:
                  //item->setWhatsThis(tr("Auto-connect these channels, on this port, to new midi tracks.\n"
                  //                      "Example:\n"
                  //                      " 1 2 3    channel 1 2 and 3\n"
                  //                      " 1-3      same\n"
                  //                      " 1-3 5    channel 1 2 3 and 5\n"
                  //                      " all      all channels\n"
                  //                      " none     no channels")); break;
                  item->setWhatsThis(tr("Auto-connect these channels, on this port, to new midi tracks.")); break;
            case DEVCOL_DEF_OUT_CHANS:
                  // Turn on if and when multiple output routes are supported.
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
      mdevView->setSelectionMode(QAbstractItemView::SingleSelection);
      mdevView->setShowGrid(false);

      //popup      = 0;
      instrPopup = 0;
      defpup = 0;
      _showAliases = -1; // 0: Show first aliases, if available. Nah, stick with -1: none at first.
      
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
      mdevView->setFocusPolicy(Qt::NoFocus);

      connect(mdevView, SIGNAL(itemPressed(QTableWidgetItem*)),
         this, SLOT(rbClicked(QTableWidgetItem*)));
      connect(mdevView, SIGNAL(itemChanged(QTableWidgetItem*)),
         this, SLOT(mdevViewItemRenamed(QTableWidgetItem*)));
      connect(song, SIGNAL(songChanged(int)), SLOT(songChanged(int)));

      connect(synthList, SIGNAL(itemSelectionChanged()), SLOT(selectionChanged()));
      connect(instanceList, SIGNAL(itemSelectionChanged()), SLOT(selectionChanged()));

      connect(addInstance, SIGNAL(clicked()), SLOT(addInstanceClicked()));
      connect(synthList, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), SLOT(addInstanceClicked())); 
      connect(removeInstance, SIGNAL(clicked()), SLOT(removeInstanceClicked()));
      connect(instanceList,  SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), SLOT(removeInstanceClicked()));
      //songChanged(0);
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
      addInstance->setEnabled(synthList->currentItem());
      removeInstance->setEnabled(instanceList->currentItem());
      }

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void MPConfig::songChanged(int flags)
      {
      // Is it simply a midi controller value adjustment? Forget it.
      //if(flags == SC_MIDI_CONTROLLER)
      //  return;
      // No need for anything but this, yet.
      if(!(flags & SC_CONFIG))
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
            MidiPort* port  = &midiPorts[i];
            MidiDevice* dev = port->device();
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
            //QTableWidgetItem* itemdefin = new QTableWidgetItem(MusEUtil::bitmap2String(port->defaultInChannels()));
            // Ignore synth devices. Default input routes make no sense for them (right now).
            QTableWidgetItem* itemdefin = new QTableWidgetItem((dev && dev->isSynti()) ? 
                                               QString() : MusEUtil::bitmap2String(port->defaultInChannels()));
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
            
            // Turn on if and when multiple output routes are supported.
            #if 0
            QTableWidgetItem* itemdefout = new QTableWidgetItem(MusEUtil::bitmap2String(port->defaultOutChannels()));
            addItem(i, DEVCOL_DEF_OUT_CHANS, itemdefout, mdevView);
            itemdefout->setFlags(Qt::ItemIsEditable | Qt::ItemIsEnabled);
            #else
            //QTableWidgetItem* itemdefout = new QTableWidgetItem(QString("--"));
            QTableWidgetItem* itemdefout = new QTableWidgetItem(MusEUtil::bitmap2String(0));
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
            
            mdevView->blockSignals(false);

            if (dev) {
	          itemname->setText(dev->name());

                  // Is it a Jack midi device? Allow renaming.
                  //if(dynamic_cast<MidiJackDevice*>(dev))
                  if (dev->deviceType() == MidiDevice::JACK_MIDI)
                       itemname->setFlags(Qt::ItemIsEditable | Qt::ItemIsEnabled);
                    
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
            //if (port->hasGui()) {
            if (port->hasNativeGui()) {
                  //itemgui->setIcon(port->guiVisible() ? QIcon(*dotIcon) : QIcon(*dothIcon));
                  itemgui->setIcon(port->nativeGuiVisible() ? QIcon(*dotIcon) : QIcon(*dothIcon));
                  }
            else {
	      itemgui->setIcon(QIcon(QPixmap()));
                  }
            if (!(dev && dev->isSynti()))
	      iteminstr->setIcon(QIcon(*buttondownIcon));
	    itemname->setIcon(QIcon(*buttondownIcon));


            //if(dev && dynamic_cast<MidiJackDevice*>(dev))
            if(dev && dev->deviceType() == MidiDevice::JACK_MIDI)
            {
              //item->setPixmap(DEVCOL_ROUTES, *buttondownIcon);
              //item->setText(DEVCOL_ROUTES, tr("routes"));
              
              if(dev->rwFlags() & 1)  
              //if(dev->openFlags() & 1)  
              {
                if(dev->openFlags() & 1)  
                {
                  itemout->setIcon(QIcon(*buttondownIcon));
		  itemout->setText(tr("out"));
                }  
              }  
              if(dev->rwFlags() & 2)  
              //if(dev->openFlags() & 2)  
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
      for (std::vector<Synth*>::iterator i = synthis.begin();
         i != synthis.end(); ++i) {
            //s = (*i)->baseName();
            //s = (*i)->name();

            QTreeWidgetItem* item = new QTreeWidgetItem(synthList);
            //item->setText(0, s);
            item->setText(0, QString((*i)->baseName()));
            s.setNum((*i)->instances());
            item->setText(1, s);
	    item->setTextAlignment(1, Qt::AlignHCenter);
            //item->setText(2, QString((*i)->baseName()));
            item->setText(2, QString((*i)->name()));
            
            item->setText(3, QString((*i)->version()));
            item->setText(4, QString((*i)->description()));
            }
      instanceList->clear();
      SynthIList* sl = song->syntis();
      for (iSynthI si = sl->begin(); si != sl->end(); ++si) {
            QTreeWidgetItem* iitem = new QTreeWidgetItem(instanceList);
            iitem->setText(0, (*si)->name());
            if ((*si)->midiPort() == -1)
                  s = tr("<none>");
            else
                  s.setNum((*si)->midiPort() + 1);
            iitem->setText(1, s);
	    iitem->setTextAlignment(1, Qt::AlignHCenter);
            }
      synthList->resizeColumnToContents(1);
      mdevView->resizeColumnsToContents();
      mdevView->horizontalHeader()->setResizeMode(DEVCOL_NO ,QHeaderView::Fixed);
      mdevView->horizontalHeader()->setResizeMode(DEVCOL_REC ,QHeaderView::Fixed);
      mdevView->horizontalHeader()->setResizeMode(DEVCOL_PLAY ,QHeaderView::Fixed);
      mdevView->horizontalHeader()->setResizeMode(DEVCOL_GUI ,QHeaderView::Fixed);
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
      SynthI *si = song->createSynthI(item->text(0), item->text(2)); // Add at end of list.
      if(!si)
        return;

      // add instance last in midi device list
      for (int i = 0; i < MIDI_PORTS; ++i) {
            MidiPort* port  = &midiPorts[i];
            MidiDevice* dev = port->device();
            if (dev==0) {
                  midiSeq->msgSetMidiDevice(port, si);
		  MusEGlobal::muse->changeConfig(true);     // save configuration file
                  song->update();
                  break;
                  }
            }
      }

//---------------------------------------------------------
//   removeInstanceClicked
//---------------------------------------------------------

void MPConfig::removeInstanceClicked()
      {
      QTreeWidgetItem* item = instanceList->currentItem();
      if (item == 0)
            return;
      SynthIList* sl = song->syntis();
      iSynthI ii;
      for (ii = sl->begin(); ii != sl->end(); ++ii) {
            if ((*ii)->iname() == item->text(0))
                  break;
            }
      if (ii == sl->end()) {
            printf("synthesizerConfig::removeInstanceClicked(): synthi not found\n");
            return;
            }
      audio->msgRemoveTrack(*ii);
      }

namespace MusEApp {

//---------------------------------------------------------
//   configMidiPorts
//---------------------------------------------------------

void MusE::configMidiPorts()
      {
      if (!midiPortConfig) {
         midiPortConfig = new MPConfig(this);
      }
         midiPortConfig->show();
         midiPortConfig->raise();
         midiPortConfig->activateWindow();
      }

} // namespace MusEApp
