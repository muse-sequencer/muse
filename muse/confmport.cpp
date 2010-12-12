//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: confmport.cpp,v 1.9.2.10 2009/12/15 03:39:58 terminator356 Exp $
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//=========================================================

#include <list>
#include <termios.h>
#include <iostream>
#include <stdio.h>

#include <QMenu>
#include <QMessageBox>
#include <QPixmap>
#include <QTableWidget>
#include <QTableWidgetItem>

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

extern std::vector<Synth*> synthis;

enum { DEVCOL_NO = 0, DEVCOL_GUI, DEVCOL_REC, DEVCOL_PLAY, DEVCOL_INSTR, DEVCOL_NAME,
       //DEVCOL_STATE };
       //DEVCOL_ROUTES, DEVCOL_STATE };
       //DEVCOL_INROUTES, DEVCOL_OUTROUTES, DEVCOL_STATE };  // p3.3.55
       DEVCOL_INROUTES, DEVCOL_OUTROUTES, DEVCOL_DEF_IN_CHANS, DEVCOL_DEF_OUT_CHANS, DEVCOL_STATE };  

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
    case DEVCOL_DEF_IN_CHANS:
    {
      QString id = item->tableWidget()->item(item->row(), DEVCOL_NO)->text();
      int no = atoi(id.toLatin1().constData()) - 1;
      if(no < 0 || no >= MIDI_PORTS)
        return;
      midiPorts[no].setDefaultInChannels(((1 << MIDI_CHANNELS) - 1) & string2bitmap(s));
      song->update();
    }
    break;    
    case DEVCOL_DEF_OUT_CHANS:
    {
      QString id = item->tableWidget()->item(item->row(), DEVCOL_NO)->text();
      int no = atoi(id.toLatin1().constData()) - 1;
      if(no < 0 || no >= MIDI_PORTS)
        return;
      midiPorts[no].setDefaultOutChannels(((1 << MIDI_CHANNELS) - 1) & string2bitmap(s));
      song->update();
    }
    break;    
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
      //printf("MPConfig::rbClicked            cpt x:%d y:%d\n", cpt.x(), cpt.y());
      //printf("MPConfig::rbClicked        new cpt x:%d y:%d\n", cpt.x(), cpt.y());
      //printf("MPConfig::rbClicked new mapped cpt x:%d y:%d\n", cpt.x(), cpt.y());
      QPoint ppt          = listView->visualItemRect(item).bottomLeft();
      QPoint mousepos     = QCursor::pos();
      //printf("MPConfig::rbClicked            ppt x:%d y:%d\n", ppt.x(), ppt.y());
      int col = item->column();
      ppt += QPoint(0, listView->horizontalHeader()->height());
      //printf("MPConfig::rbClicked        new ppt x:%d y:%d\n", ppt.x(), ppt.y());
      ppt  = listView->mapToGlobal(ppt);
      //printf("MPConfig::rbClicked new mapped ppt x:%d y:%d\n", ppt.x(), ppt.y());

      switch (col) {
            case DEVCOL_GUI:
                  if (dev == 0)
                        //break;
                        return;
                  if (port->hasGui())
                  {
                        port->instrument()->showGui(!port->guiVisible());
                        item->setIcon(port->guiVisible() ? QIcon(*dotIcon) : QIcon(*dothIcon));
                  }
                  //break;
                  return;
                  
            case DEVCOL_REC:
                  if (dev == 0 || !(rwFlags & 2))
                        //break;
                        return;
                  openFlags ^= 0x2;
                  dev->setOpenFlags(openFlags);
                  midiSeq->msgSetMidiDevice(port, dev);       // reopen device
                  item->setIcon(openFlags & 2 ? QIcon(*dotIcon) : QIcon(*dothIcon));
                  
                  // p3.3.55
                  if(dev->deviceType() == MidiDevice::JACK_MIDI)
                  {
                    if(dev->openFlags() & 2)  
                    {
                      //item->setPixmap(DEVCOL_INROUTES, *buttondownIcon);
		      item->tableWidget()->item(item->row(), DEVCOL_INROUTES)->setText(tr("in"));
		    }
                    else
                    {
                      //item->setPixmap(DEVCOL_INROUTES, *buttondownIcon);
		      item->tableWidget()->item(item->row(), DEVCOL_INROUTES)->setText("");
                    }  
                  }
            
                  //break;
                  return;
                  
            case DEVCOL_PLAY:
                  if (dev == 0 || !(rwFlags & 1))
                        //break;
                        return;
                  openFlags ^= 0x1;
                  dev->setOpenFlags(openFlags);
                  midiSeq->msgSetMidiDevice(port, dev);       // reopen device
                  item->setIcon(openFlags & 1 ? QIcon(*dotIcon) : QIcon(*dothIcon));
                  
                  // p3.3.55
                  if(dev->deviceType() == MidiDevice::JACK_MIDI)
                  {
                    if(dev->openFlags() & 1)  
                    {
                      //item->setPixmap(DEVCOL_OUTROUTES, *buttondownIcon);
		      item->tableWidget()->item(item->row(), DEVCOL_OUTROUTES)->setText(tr("out"));
                    }
                    else  
                    {
                      //item->setPixmap(DEVCOL_OUTROUTES, *buttondownIcon);
		      item->tableWidget()->item(item->row(), DEVCOL_OUTROUTES)->setText("");
                    }
                  }
            
                  //break;
                  return;
                  
            //case DEVCOL_ROUTES:
            case DEVCOL_INROUTES:  // p3.3.55
            case DEVCOL_OUTROUTES:
                  {
                    if(!checkAudioDevice())
                      return;
                      
                    if(audioDevice->deviceType() != AudioDevice::JACK_AUDIO)  // p3.3.52  Only if Jack is running.
                      return;
                      
                    if(!dev)
                      return;
                    
                    // Only Jack midi devices.
                    //MidiJackDevice* mjd = dynamic_cast<MidiJackDevice*>(dev);
                    //if(!mjd)
                    if(dev->deviceType() != MidiDevice::JACK_MIDI)  
                      return;
                    
                    //if(!(dev->rwFlags() & 3))
                    //if(!(dev->rwFlags() & ((col == DEVCOL_OUTROUTES) ? 1 : 2)))    // p3.3.55
                    if(!(dev->openFlags() & ((col == DEVCOL_OUTROUTES) ? 1 : 2)))    
                      return;
                      
                    //RouteList* rl = (dev->rwFlags() & 1) ? dev->outRoutes() : dev->inRoutes();
                    RouteList* rl = (col == DEVCOL_OUTROUTES) ? dev->outRoutes() : dev->inRoutes();   // p3.3.55
                    QMenu* pup = 0;
                    int gid = 0;
                    std::list<QString> sl;
                    pup = new QMenu(this);
                    
        _redisplay:
                    pup->clear();
                    gid = 0;
                    
                    // Jack input ports if device is writable, and jack output ports if device is readable.
                    //sl = (dev->rwFlags() & 1) ? audioDevice->inputPorts(true, _showAliases) : audioDevice->outputPorts(true, _showAliases);
                    // p3.3.55
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
                        Route rt(*ip, (col == DEVCOL_OUTROUTES), -1, Route::JACK_ROUTE);   // p3.3.55
                        for(iRoute ir = rl->begin(); ir != rl->end(); ++ir) 
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
                      
                      //if(dev->rwFlags() & 1) // Writable
                      if(col == DEVCOL_OUTROUTES) // Writable  p3.3.55
                      {
                        Route srcRoute(dev, -1);
                        Route dstRoute(s, true, -1, Route::JACK_ROUTE);
            
                        iRoute iir = rl->begin();
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
                      //if(col == DEVCOL_INROUTES) // Readable    p3.3.55
                      {
                        Route srcRoute(s, false, -1, Route::JACK_ROUTE);
                        Route dstRoute(dev, -1);
            
                        iRoute iir = rl->begin();
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
                      
                      // p3.3.46
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
            case DEVCOL_DEF_OUT_CHANS:
                  {
                  }
                  //break;
                  return;
                  
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
                        pup->addAction(new MenuTitleItem(QT_TRANSLATE_NOOP("@default", "ALSA:"), pup));
                        
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
                        pup->addAction(new MenuTitleItem(QT_TRANSLATE_NOOP("@default", "SYNTH:"), pup));
                        
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
                        pup->addAction(new MenuTitleItem(QT_TRANSLATE_NOOP("@default", "JACK:"), pup));
                        
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
                        if(n <= 2)  // p3.3.55
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
                      muse->changeConfig(true);     // save configuration file
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
                  
                  QAction* act = instrPopup->exec(ppt, 0);
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
            case DEVCOL_DEF_IN_CHANS:   item->setToolTip(tr("Connect these to new midi tracks")); break;
            case DEVCOL_DEF_OUT_CHANS:  item->setToolTip(tr("Connect new midi tracks to this (first listed only)")); break;
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
                  item->setWhatsThis(tr("Connect these channels, on this port, to new midi tracks.\n"
                                        "Example:\n"
                                        " 1 2 3    channel 1 2 and 3\n"
                                        " 1-3      same\n"
                                        " 1-3 5    channel 1 2 3 and 5\n"
                                        " all      all channels\n"
                                        " none     no channels")); break;
            case DEVCOL_DEF_OUT_CHANS:
                  item->setWhatsThis(tr("Connect new midi tracks to these channels, on this port.\n"
                                        "See default in channels.\n"
                                        "NOTE: Currently only one output port and channel supported (first found)")); break;
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
      mdevView->setRowCount(MIDI_PORTS);
      mdevView->verticalHeader()->hide();
      mdevView->setSelectionMode(QAbstractItemView::SingleSelection);
      mdevView->setShowGrid(false);

      //popup      = 0;
      instrPopup = 0;
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
      songChanged(0);
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
      if(flags == SC_MIDI_CONTROLLER)
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
            QTableWidgetItem* itemdefin = new QTableWidgetItem(bitmap2String(port->defaultInChannels()));
            addItem(i, DEVCOL_DEF_IN_CHANS, itemdefin, mdevView);
            itemdefin->setFlags(Qt::ItemIsEditable | Qt::ItemIsEnabled);
            QTableWidgetItem* itemdefout = new QTableWidgetItem(bitmap2String(port->defaultOutChannels()));
            addItem(i, DEVCOL_DEF_OUT_CHANS, itemdefout, mdevView);
            itemdefout->setFlags(Qt::ItemIsEditable | Qt::ItemIsEnabled);
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
            if (port->hasGui()) {
                  itemgui->setIcon(port->guiVisible() ? QIcon(*dotIcon) : QIcon(*dothIcon));
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
              
              // p3.3.55
              if(dev->rwFlags() & 1)  
              //if(dev->openFlags() & 1)  
              {
		itemout->setIcon(QIcon(*buttondownIcon));
                if(dev->openFlags() & 1)  
		  itemout->setText(tr("out"));
              }  
              if(dev->rwFlags() & 2)  
              //if(dev->openFlags() & 2)  
              {
		itemin->setIcon(QIcon(*buttondownIcon));
                if(dev->openFlags() & 2)  
		  itemin->setText(tr("in"));
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
      //SynthI *si = song->createSynthI(item->text(2));
      SynthI *si = song->createSynthI(item->text(0), item->text(2));
      if(!si)
        return;

      // add instance last in midi device list
      for (int i = 0; i < MIDI_PORTS; ++i) {
            MidiPort* port  = &midiPorts[i];
            MidiDevice* dev = port->device();
            if (dev==0) {
                  midiSeq->msgSetMidiDevice(port, si);
                  muse->changeConfig(true);     // save configuration file
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

//---------------------------------------------------------
//   configMidiPorts
//---------------------------------------------------------

void MusE::configMidiPorts()
      {
      if (!midiPortConfig)
	midiPortConfig = new MPConfig(0);
     if (midiPortConfig->isVisible()) {
         midiPortConfig->raise();
         midiPortConfig->activateWindow();
         }
      else
            midiPortConfig->show();
      }

