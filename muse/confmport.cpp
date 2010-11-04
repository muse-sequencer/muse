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

#include <q3listview.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qlineedit.h>
#include <qcombobox.h>
#include <qlabel.h>
#include <QButtonGroup>
#include <QMenu>
//Added by qt3to4:
#include <QPixmap>
#include <stdio.h>
//#include <q3popupmenu.h>
#include <q3groupbox.h>
#include <qradiobutton.h>
#include <qspinbox.h>
#include <qcheckbox.h>
#include <qsignalmapper.h>
#include <qtooltip.h>
#include <q3filedialog.h>
#include <qtoolbutton.h>
#include <qmessagebox.h>
#include <qpoint.h>

#include "confmport.h"
#include "app.h"
#include "icons.h"
#include "globals.h"
#include "transport.h"
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

extern std::vector<Synth*> synthis;

enum { DEVCOL_NO = 0, DEVCOL_GUI, DEVCOL_REC, DEVCOL_PLAY, DEVCOL_INSTR, DEVCOL_NAME,
       //DEVCOL_STATE };
       //DEVCOL_ROUTES, DEVCOL_STATE };
       DEVCOL_INROUTES, DEVCOL_OUTROUTES, DEVCOL_STATE };  // p3.3.55

//---------------------------------------------------------
//   mdevViewItemRenamed
//---------------------------------------------------------

void MPConfig::mdevViewItemRenamed(Q3ListViewItem* item, int col, const QString& s)
{
  //printf("MPConfig::mdevViewItemRenamed col:%d txt:%s\n", col, s.latin1());
  if(item == 0)
    return;
  switch(col)
  {
    case DEVCOL_NAME:
    {
      QString id = item->text(DEVCOL_NO);
      int no = atoi(id.latin1()) - 1;
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
      printf("MPConfig::mdevViewItemRenamed unknown column clicked col:%d txt:%s\n", col, s.latin1());
    break;
  } 
}

//---------------------------------------------------------
//   rbClicked
//---------------------------------------------------------

void MPConfig::rbClicked(Q3ListViewItem* item, const QPoint& cpt, int col)
      {
      if (item == 0)
            return;
      QString id = item->text(DEVCOL_NO);
      int no = atoi(id.latin1()) - 1;
      if (no < 0 || no >= MIDI_PORTS)
            return;

      int n;
      MidiPort* port      = &midiPorts[no];
      MidiDevice* dev     = port->device();
      int rwFlags         = dev ? dev->rwFlags() : 0;
      int openFlags       = dev ? dev->openFlags() : 0;
      Q3ListView* listView = item->listView();
      //printf("MPConfig::rbClicked            cpt x:%d y:%d\n", cpt.x(), cpt.y());
      //printf("MPConfig::rbClicked        new cpt x:%d y:%d\n", cpt.x(), cpt.y());
      //printf("MPConfig::rbClicked new mapped cpt x:%d y:%d\n", cpt.x(), cpt.y());
      QPoint ppt          = listView->itemRect(item).bottomLeft();
      //printf("MPConfig::rbClicked            ppt x:%d y:%d\n", ppt.x(), ppt.y());
      ppt += QPoint(listView->header()->sectionPos(col), listView->header()->height());
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
                        item->setPixmap(DEVCOL_GUI, port->guiVisible() ? *dotIcon : *dothIcon);
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
                  item->setPixmap(DEVCOL_REC, openFlags & 2 ? *dotIcon : *dothIcon);
                  
                  // p3.3.55
                  if(dev->deviceType() == MidiDevice::JACK_MIDI)
                  {
                    if(dev->openFlags() & 2)  
                    {
                      //item->setPixmap(DEVCOL_INROUTES, *buttondownIcon);
                      item->setText(DEVCOL_INROUTES, tr("in"));
                    }
                    else
                    {
                      //item->setPixmap(DEVCOL_INROUTES, *buttondownIcon);
                      item->setText(DEVCOL_INROUTES, "");
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
                  item->setPixmap(DEVCOL_PLAY, openFlags & 1 ? *dotIcon : *dothIcon);
                  
                  // p3.3.55
                  if(dev->deviceType() == MidiDevice::JACK_MIDI)
                  {
                    if(dev->openFlags() & 1)  
                    {
                      //item->setPixmap(DEVCOL_OUTROUTES, *buttondownIcon);
                      item->setText(DEVCOL_OUTROUTES, tr("out"));
                    }
                    else  
                    {
                      //item->setPixmap(DEVCOL_OUTROUTES, *buttondownIcon);
                      item->setText(DEVCOL_OUTROUTES, "");
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
                      //snprintf(buffer, 128, "%s %d", tr("Channel").latin1(), i+1);
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
                  
            case DEVCOL_NAME:
                  {
                    //printf("MPConfig::rbClicked DEVCOL_NAME\n");
                    
                    // Did we click in the text area?
                    if((cpt.x() - ppt.x()) > buttondownIcon->width())
                    {
                      //printf("MPConfig::rbClicked starting item rename... enabled?:%d\n", item->renameEnabled(DEVCOL_NAME));
                      // Start the renaming of the cell...
                      if(item->renameEnabled(DEVCOL_NAME))
                        item->startRename(DEVCOL_NAME);
                        
                      return;
                    }
                    else
                    // We clicked the 'down' button.
                    {
                      QMenu* pup = new QMenu(this);
                      
                      QAction* act;
                      
                      // Could do it this way...
                      //act = pup->addAction(tr("Create") + QT_TR_NOOP(" Jack") + tr(" input"));
                      //act = pup->addAction(tr("Create") + QT_TR_NOOP(" Jack") + tr(" output"));
                      //act = pup->addAction(tr("Create") + QT_TR_NOOP(" Jack") + tr(" combo"));
                      // ... or keep it simple and let the user click on the green lights instead.
                      act = pup->addAction(tr("Create") + QT_TR_NOOP(" Jack") + tr(" device"));
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
                          //mapALSA.insert( std::pair<std::string, int> (std::string(devALSA->name().lower().latin1()), ii) );
                          mapALSA.insert( std::pair<std::string, int> (std::string((*i)->name().latin1()), aix) );
                          ++aix;
                        }  
                        else
                        if((*i)->deviceType() == MidiDevice::JACK_MIDI)
                        {  
                          //devJACK = dynamic_cast<MidiJackDevice*>(*i);
                          //if(devJACK)
                            //mapJACK.insert( std::pair<std::string, int> (std::string(devJACK->name().lower().latin1()), ii) );
                          mapJACK.insert( std::pair<std::string, int> (std::string((*i)->name().latin1()), jix) );
                          ++jix;
                        }
                        else
                        if((*i)->deviceType() == MidiDevice::SYNTH_MIDI)
                        {
                          mapSYNTH.insert( std::pair<std::string, int> (std::string((*i)->name().latin1()), six) );
                          ++six;  
                        }
                        else
                          printf("MPConfig::rbClicked unknown midi device: %s\n", (*i)->name().latin1());
                      }
                      
                      //int sz = midiDevices.size();
                      //if(!mapALSA.empty())
                      {
                        pup->addSeparator();
                        pup->addAction(new MenuTitleItem(QT_TR_NOOP("ALSA:"), pup));
                        
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
                              
                            act = pup->addAction(QT_TR_NOOP(md->name()));
                            act->setData(idx);
                            act->setCheckable(true);
                            act->setChecked(md == dev);
                          }  
                        }
                      }  
                      
                      if(!mapSYNTH.empty())
                      {
                        pup->addSeparator();
                        pup->addAction(new MenuTitleItem(QT_TR_NOOP("SYNTH:"), pup));
                        
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
                              
                            act = pup->addAction(QT_TR_NOOP(md->name()));
                            act->setData(idx);
                            act->setCheckable(true);
                            act->setChecked(md == dev);
                          }  
                        }
                      }  
                      
                      //if(!mapJACK.empty())
                      {
                        pup->addSeparator();
                        pup->addAction(new MenuTitleItem(QT_TR_NOOP("JACK:"), pup));
                        
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
                              
                            act = pup->addAction(QT_TR_NOOP(md->name()));
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
                  item->setText(DEVCOL_INSTR, s);
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
//   MPHeaderTip::maybeTip
//---------------------------------------------------------

void MPHeaderTip::maybeTip(const QPoint &pos)
      {
#if 0 // ddskrjo
      Q3Header* w  = (Q3Header*)parentWidget();
      int section = w->sectionAt(pos.x());
      if (section == -1)
            return;
      QRect r(w->sectionPos(section), 0, w->sectionSize(section),
         w->height());
      QString p;
      switch (section) {
            case DEVCOL_NO:     p = Q3Header::tr("Port Number"); break;
            case DEVCOL_GUI:    p = Q3Header::tr("Enable gui"); break;
            case DEVCOL_REC:    p = Q3Header::tr("Enable reading"); break;
            case DEVCOL_PLAY:   p = Q3Header::tr("Enable writing"); break;
            case DEVCOL_INSTR:  p = Q3Header::tr("Port instrument"); break;
            case DEVCOL_NAME:   p = Q3Header::tr("Midi device name. Click to edit (Jack)"); break;
            //case DEVCOL_ROUTES: p = Q3Header::tr("Jack midi ports"); break;
            case DEVCOL_INROUTES:  p = Q3Header::tr("Connections from Jack Midi outputs"); break;
            case DEVCOL_OUTROUTES: p = Q3Header::tr("Connections to Jack Midi inputs"); break;
            case DEVCOL_STATE:  p = Q3Header::tr("Device state"); break;
            default: return;
            }
      tip(r, p);
#endif
  }

//---------------------------------------------------------
//   MPWhatsThis::text
//---------------------------------------------------------

QString MPWhatsThis::text(const QPoint& pos)
      {
      int n = header->cellAt(pos.x());
      if (n == -1)
            return QString::null;
      switch (header->mapToLogical(n)) {
            case DEVCOL_NO:
                  return Q3Header::tr("Port Number");
            case DEVCOL_GUI:
                  return Q3Header::tr("Enable gui for device");
            case DEVCOL_REC:
                  return Q3Header::tr("Enable reading from device");
            case DEVCOL_PLAY:
                  return Q3Header::tr("Enable writing to device");
            case DEVCOL_NAME:
                  return Q3Header::tr("Name of the midi device associated with"
                       " this port number. Click to edit Jack midi name.");
            case DEVCOL_INSTR:
                  return Q3Header::tr("Instrument connected to port");
            //case DEVCOL_ROUTES:
            //      return Q3Header::tr("Jack midi ports");
            case DEVCOL_INROUTES:
                  return Q3Header::tr("Connections from Jack Midi output ports");
            case DEVCOL_OUTROUTES:
                  return Q3Header::tr("Connections to Jack Midi input ports");
            case DEVCOL_STATE:
                  return Q3Header::tr("State: result of opening the device");
            default:
                  break;
            }
      return QString::null;
      }

//---------------------------------------------------------
//   MPConfig
//    Midi Port Config
//---------------------------------------------------------

MPConfig::MPConfig(QWidget* parent, char* name)
   : SynthConfigBase(parent, name)
      {
      _mptooltip = 0;
      //popup      = 0;
      instrPopup = 0;
      _showAliases = -1; // 0: Show first aliases, if available. Nah, stick with -1: none at first.
      
      mdevView->setSorting(-1);
      mdevView->setAllColumnsShowFocus(true);
      mdevView->addColumn(tr("Port"));
      mdevView->addColumn(tr("GUI"));
      mdevView->addColumn(tr("I"));
      mdevView->addColumn(tr("O"));
      mdevView->addColumn(tr("Instrument"), 120);
      mdevView->addColumn(tr("Device Name"), 120);
      //mdevView->addColumn(tr("Routing"), 80);
      mdevView->addColumn(tr("In routes"), 80);
      mdevView->addColumn(tr("Out routes"), 80);
      mdevView->addColumn(tr("State"));
      mdevView->setFocusPolicy(Qt::NoFocus);

      mdevView->setColumnAlignment(DEVCOL_NO, Qt::AlignHCenter);
      mdevView->setColumnAlignment(DEVCOL_GUI, Qt::AlignCenter);
      mdevView->setColumnAlignment(DEVCOL_REC, Qt::AlignCenter);
      mdevView->setColumnAlignment(DEVCOL_PLAY, Qt::AlignCenter);
      mdevView->header()->setResizeEnabled(false, DEVCOL_NO);
      mdevView->header()->setResizeEnabled(false, DEVCOL_REC);
      mdevView->header()->setResizeEnabled(false, DEVCOL_GUI);
      mdevView->setResizeMode(Q3ListView::LastColumn);

      instanceList->setColumnAlignment(1, Qt::AlignHCenter);

      new MPWhatsThis(mdevView, mdevView->header());
      _mptooltip = new MPHeaderTip(mdevView->header());

      connect(mdevView, SIGNAL(pressed(Q3ListViewItem*,const QPoint&,int)),
         this, SLOT(rbClicked(Q3ListViewItem*,const QPoint&,int)));
      connect(mdevView, SIGNAL(itemRenamed(Q3ListViewItem*,int,const QString&)),
         this, SLOT(mdevViewItemRenamed(Q3ListViewItem*,int,const QString&)));
      connect(song, SIGNAL(songChanged(int)), SLOT(songChanged(int)));

      connect(synthList, SIGNAL(selectionChanged()), SLOT(selectionChanged()));
      connect(instanceList, SIGNAL(selectionChanged()), SLOT(selectionChanged()));

      connect(addInstance, SIGNAL(clicked()), SLOT(addInstanceClicked()));
      connect(removeInstance, SIGNAL(clicked()), SLOT(removeInstanceClicked()));
      songChanged(0);
      }

  
MPConfig::~MPConfig()
{
  delete _mptooltip;
}
  
//---------------------------------------------------------
//   selectionChanged
//---------------------------------------------------------

void MPConfig::selectionChanged()
      {
      addInstance->setEnabled(synthList->selectedItem());
      removeInstance->setEnabled(instanceList->selectedItem());
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
      Q3ListViewItem* sitem = mdevView->selectedItem();
      if(sitem)
      {
        QString id = sitem->text(DEVCOL_NO);
        no = atoi(id.latin1()) - 1;
        if(no < 0 || no >= MIDI_PORTS)
          no = -1;
      }
      
      sitem = 0;
      mdevView->clear();
      for (int i = MIDI_PORTS-1; i >= 0; --i) 
      {
            MidiPort* port  = &midiPorts[i];
            MidiDevice* dev = port->device();
            QString s;
            s.setNum(i+1);
            Q3ListViewItem* item = new Q3ListViewItem(mdevView);
            item->setText(DEVCOL_NO, s);
            item->setText(DEVCOL_STATE, port->state());
            if (port->instrument())
                  item->setText(DEVCOL_INSTR,  port->instrument()->iname() );
            else
                  item->setText(DEVCOL_INSTR, tr("<unknown>"));
            if (dev) {
                  item->setText(DEVCOL_NAME, dev->name());
                  // Is it a Jack midi device? Allow renaming.
                  //if(dynamic_cast<MidiJackDevice*>(dev))
                  if(dev->deviceType() == MidiDevice::JACK_MIDI)
                    item->setRenameEnabled(DEVCOL_NAME, true);
                    
                  if (dev->rwFlags() & 0x2)
                        item->setPixmap(DEVCOL_REC, dev->openFlags() & 2 ? *dotIcon : *dothIcon);
                  else
                        item->setPixmap(DEVCOL_REC, QPixmap());
                  if (dev->rwFlags() & 0x1)
                        item->setPixmap(DEVCOL_PLAY, dev->openFlags() & 1 ? *dotIcon : *dothIcon);
                  else
                        item->setPixmap(DEVCOL_PLAY, QPixmap());
                  }
            else {
                  item->setText(DEVCOL_NAME, tr("<none>"));
                  item->setPixmap(DEVCOL_GUI, *dothIcon);
                  item->setPixmap(DEVCOL_REC, QPixmap());
                  item->setPixmap(DEVCOL_PLAY, QPixmap());
                  }
            if (port->hasGui()) {
                  item->setPixmap(DEVCOL_GUI, port->guiVisible() ? *dotIcon : *dothIcon);
                  }
            else {
                  item->setPixmap(DEVCOL_GUI, QPixmap());
                  }
            if (!(dev && dev->isSynti()))
                  item->setPixmap(DEVCOL_INSTR, *buttondownIcon);
            item->setPixmap(DEVCOL_NAME, *buttondownIcon);
            
            //if(dev && dynamic_cast<MidiJackDevice*>(dev))
            if(dev && dev->deviceType() == MidiDevice::JACK_MIDI)
            {
              //item->setPixmap(DEVCOL_ROUTES, *buttondownIcon);
              //item->setText(DEVCOL_ROUTES, tr("routes"));
              
              // p3.3.55
              if(dev->rwFlags() & 1)  
              //if(dev->openFlags() & 1)  
              {
                item->setPixmap(DEVCOL_OUTROUTES, *buttondownIcon);
                if(dev->openFlags() & 1)  
                  item->setText(DEVCOL_OUTROUTES, tr("out"));
              }  
              if(dev->rwFlags() & 2)  
              //if(dev->openFlags() & 2)  
              {
                item->setPixmap(DEVCOL_INROUTES, *buttondownIcon);
                if(dev->openFlags() & 2)  
                  item->setText(DEVCOL_INROUTES, tr("in"));
              }  
            }
            
            mdevView->insertItem(item);
            if(i == no)
              sitem = item;
      }
      if(sitem)
      {
        mdevView->setSelected(sitem, true);
        mdevView->ensureItemVisible(sitem);
      }
      
      QString s;
      synthList->clear();
      for (std::vector<Synth*>::iterator i = synthis.begin();
         i != synthis.end(); ++i) {
            //s = (*i)->baseName();
            //s = (*i)->name();

            Q3ListViewItem* item = new Q3ListViewItem(synthList);
            //item->setText(0, s);
            item->setText(0, QString((*i)->baseName()));
            s.setNum((*i)->instances());
            item->setText(1, s);
            //item->setText(2, QString((*i)->baseName()));
            item->setText(2, QString((*i)->name()));
            
            item->setText(3, QString((*i)->version()));
            item->setText(4, QString((*i)->description()));
            }
      instanceList->clear();
      SynthIList* sl = song->syntis();
      for (iSynthI si = sl->begin(); si != sl->end(); ++si) {
            Q3ListViewItem* iitem = new Q3ListViewItem(instanceList);
            iitem->setText(0, (*si)->name());
            if ((*si)->midiPort() == -1)
                  s = tr("<none>");
            else
                  s.setNum((*si)->midiPort() + 1);
            iitem->setText(1, s);
            }
      selectionChanged();
      }

//---------------------------------------------------------
//   addInstanceClicked
//---------------------------------------------------------

void MPConfig::addInstanceClicked()
      {
      Q3ListViewItem* item = synthList->selectedItem();
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
      Q3ListViewItem* item = instanceList->selectedItem();
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
	midiPortConfig = new MPConfig(0, (char*) "midiPortConfig");
     if (midiPortConfig->isVisible()) {
         midiPortConfig->raise();
         midiPortConfig->setActiveWindow();
         }
      else
            midiPortConfig->show();
      }

