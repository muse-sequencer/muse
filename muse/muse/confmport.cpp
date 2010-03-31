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

#include <qlistview.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qlineedit.h>
#include <qcombobox.h>
#include <qlabel.h>
#include <qbuttongroup.h>
#include <stdio.h>
#include <qpopupmenu.h>
#include <qgroupbox.h>
#include <qradiobutton.h>
#include <qspinbox.h>
#include <qcheckbox.h>
#include <qsignalmapper.h>
#include <qtooltip.h>
#include <qfiledialog.h>
#include <qtoolbutton.h>

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
       DEVCOL_ROUTES, DEVCOL_STATE };

//---------------------------------------------------------
//   rbClicked
//---------------------------------------------------------

void MPConfig::rbClicked(QListViewItem* item, const QPoint&, int col)
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
      QListView* listView = item->listView();
      QPoint ppt          = listView->itemRect(item).bottomLeft();
      ppt += QPoint(listView->header()->sectionPos(col), listView->header()->height());
      ppt  = listView->mapToGlobal(ppt);

      switch (col) {
            case DEVCOL_GUI:
                  if (dev == 0)
                        break;
                  if (port->hasGui())
                        port->instrument()->showGui(!port->guiVisible());
                  break;

            case DEVCOL_REC:
                  if (dev == 0 || !(rwFlags & 2))
                        break;
                  openFlags ^= 0x2;
                  dev->setOpenFlags(openFlags);
                  midiSeq->msgSetMidiDevice(port, dev);       // reopen device
                  break;
            case DEVCOL_PLAY:
                  if (dev == 0 || !(rwFlags & 1))
                        break;
                  openFlags ^= 0x1;
                  dev->setOpenFlags(openFlags);
                  midiSeq->msgSetMidiDevice(port, dev);       // reopen device
                  break;
            case DEVCOL_ROUTES:
                  {
                    if(!checkAudioDevice())
                      return;
                      
                    if(!dev)
                      return;
                    
                    // Only Jack midi devices.
                    //MidiJackDevice* mjd = dynamic_cast<MidiJackDevice*>(dev);
                    //if(!mjd)
                    if(dev->deviceType() != MidiDevice::JACK_MIDI)  
                      return;
                    
                    if(!dev->rwFlags() & 3)
                      return;
                      
                    RouteList* rl = (dev->rwFlags() & 1) ? dev->outRoutes() : dev->inRoutes();
              
                    QPopupMenu* pup = 0;
                    int gid = 0;
                    std::list<QString> sl;
        
        _redisplay:
                    // Jack input ports if device is writable, and jack output ports if device is readable.
                    sl = (dev->rwFlags() & 1) ? audioDevice->inputPorts(true, _showAliases) : audioDevice->outputPorts(true, _showAliases);
                    
                    pup = new QPopupMenu(this);
                    pup->setCheckable(true);
                    
                    gid = 0;
                    //for (int i = 0; i < channel; ++i) 
                    //{
                      //char buffer[128];
                      //snprintf(buffer, 128, "%s %d", tr("Channel").latin1(), i+1);
                      //MenuTitleItem* titel = new MenuTitleItem(QString(buffer));
                      //pup->insertItem(titel);
  
                      pup->insertItem(tr("Show first aliases"), gid);
                      pup->setItemChecked(gid, (_showAliases == 0));
                      ++gid;
                      pup->insertItem(tr("Show second aliases"), gid);
                      pup->setItemChecked(gid, (_showAliases == 1));
                      ++gid;
                      pup->insertSeparator();
                      
                      for(std::list<QString>::iterator ip = sl.begin(); ip != sl.end(); ++ip) 
                      {
                        //int id = pup->insertItem(*ip, gid);
                        pup->insertItem(*ip, gid);
                        //Route dst(*ip, true, i);
                        Route rt(*ip, (dev->rwFlags() & 1), -1, Route::JACK_ROUTE);
                        for(iRoute ir = rl->begin(); ir != rl->end(); ++ir) 
                        {
                          if (*ir == rt) 
                          {
                            //pup->setItemChecked(id, true);
                            pup->setItemChecked(gid, true);
                            break;
                          }
                        }
                        ++gid;
                      }
                      //if (i+1 != channel)
                      //      pup->insertSeparator();
                    //}
                    
                    n = pup->exec(ppt, 0);
                    if (n != -1) 
                    {
                      if(n == 0) // Show first aliases
                      {
                        delete pup;
                        if(_showAliases == 0)
                          _showAliases = -1;
                        else  
                          _showAliases = 0;
                        goto _redisplay;   // Go back
                      }
                      else
                      if(n == 1) // Show second aliases
                      {
                        delete pup;
                        if(_showAliases == 1)
                          _showAliases = -1;
                        else  
                          _showAliases = 1;
                        goto _redisplay;   // Go back
                      }
                      
                      QString s(pup->text(n));
                      
                      if(dev->rwFlags() & 1) // Writable
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
                      if(dev->rwFlags() & 2) // Readable
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
                    }
                    delete pup;
                    //iR->setDown(false);     // pup->exec() catches mouse release event
                  
                  
                  
                  }
                  break;
            case DEVCOL_NAME:
                  {
                    QPopupMenu* pup = new QPopupMenu(this);
                    
                    pup->setCheckable(true);
                    
                    pup->insertItem(tr("Create") + QT_TR_NOOP(" Jack") + tr(" input"), 0);
                    pup->insertItem(tr("Create") + QT_TR_NOOP(" Jack") + tr(" output"), 1);
                    
                    typedef std::map<std::string, int > asmap;
                    typedef std::map<std::string, int >::iterator imap;
                    
                    asmap mapALSA;
                    asmap mapJACK;
                    asmap mapSYNTH;
                    
                    int aix = 2;
                    int jix = 0x10000000;
                    int six = 0x20000000;
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
                    if(!mapALSA.empty())
                    {
                      pup->insertSeparator();
                      pup->insertItem(new MenuTitleItem(QT_TR_NOOP("ALSA:")));
                      
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
                            
                          //pup->insertItem(QT_TR_NOOP(md->name()), idx + 3);
                          pup->insertItem(QT_TR_NOOP(md->name()), idx);
                          
                          //for(int k = 0; k < MIDI_PORTS; ++k) 
                          //{
                            //MidiDevice* dev = midiPorts[k].device();
                            //if(dev && s == dev->name()) 
                            if(md == dev) 
                            {
                              //pup->setItemEnabled(idx + 3, false);
                              //pup->setItemChecked(idx + 3, true);
                              pup->setItemChecked(idx, true);
                              //break;
                            }      
                          //}
                        }  
                      }
                    }  
                    
                    if(!mapJACK.empty())
                    {
                      pup->insertSeparator();
                      pup->insertItem(new MenuTitleItem(QT_TR_NOOP("JACK:")));
                      
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
                            
                          //pup->insertItem(QT_TR_NOOP(md->name()), idx + 3);
                          pup->insertItem(QT_TR_NOOP(md->name()), idx);
                          
                          //for(int k = 0; k < MIDI_PORTS; ++k) 
                          //{
                            //MidiDevice* dev = midiPorts[k].device();
                            //if(dev && s == dev->name()) 
                            if(md == dev) 
                            {
                              //pup->setItemEnabled(idx + 3, false);
                              //pup->setItemChecked(idx + 3, true);
                              pup->setItemChecked(idx, true);
                              //break;
                            }      
                          //}
                        }  
                      }
                    }  
                    
                    if(!mapSYNTH.empty())
                    {
                      pup->insertSeparator();
                      pup->insertItem(new MenuTitleItem(QT_TR_NOOP("SYNTH:")));
                      
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
                            
                          //pup->insertItem(QT_TR_NOOP(md->name()), idx + 3);
                          pup->insertItem(QT_TR_NOOP(md->name()), idx);
                          
                          //for(int k = 0; k < MIDI_PORTS; ++k) 
                          //{
                            //MidiDevice* dev = midiPorts[k].device();
                            //if(dev && s == dev->name()) 
                            if(md == dev) 
                            {
                              //pup->setItemEnabled(idx + 3, false);
                              //pup->setItemChecked(idx + 3, true);
                              pup->setItemChecked(idx, true);
                              //break;
                            }      
                          //}
                        }  
                      }
                    }
                    
                    n = pup->exec(ppt, 0);
                    if(n == -1)
                    {      
                      delete pup;
                      break;
                    }
                    
                    //printf("MPConfig::rbClicked n:%d\n", n);
                    
                    MidiDevice* sdev = 0;
                    if(n < 2)
                    {
                      delete pup;
                      if(n == 0)
                        sdev = MidiJackDevice::createJackMidiDevice(QString(), 2); // 2: Readable.
                      else
                      if(n == 1)
                        sdev = MidiJackDevice::createJackMidiDevice(QString(), 1); // 1:Writable.
                    }  
                    else
                    {
                      int typ = MidiDevice::ALSA_MIDI;
                      if(n >= 0x10000000)
                        typ = MidiDevice::JACK_MIDI;
                      if(n >= 0x20000000)
                        typ = MidiDevice::SYNTH_MIDI;
                      
                      sdev = midiDevices.find(pup->text(n), typ);
                      delete pup;
                      // Is it the current device? Reset it to <none>.
                      if(sdev == dev)
                        sdev = 0;
                    }    
                    
                    midiSeq->msgSetMidiDevice(port, sdev);
                    muse->changeConfig(true);     // save configuration file
                    song->update();
                  }
                  break;

            case DEVCOL_INSTR:
                  {
                  if (dev && dev->isSynti())
                        break;
                  if (instrPopup == 0)
                        instrPopup = new QPopupMenu(this);
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
                          instrPopup->insertItem((*i)->iname());
                     }
                  n = instrPopup->exec(ppt, 0);
                  if (n == -1)
                        break;
                  QString s = instrPopup->text(n);
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
                  break;
            }
      songChanged(-1);
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
                  return QHeader::tr("Port Number");
            case DEVCOL_GUI:
                  return QHeader::tr("enable gui for device");
            case DEVCOL_REC:
                  return QHeader::tr("enables reading from device");
            case DEVCOL_PLAY:
                  return QHeader::tr("enables writing to device");
            case DEVCOL_NAME:
                  return QHeader::tr("Name of the midi device associated with"
                       " this port number");
            case DEVCOL_INSTR:
                  return QHeader::tr("Instrument connected to port");
            case DEVCOL_ROUTES:
                  return QHeader::tr("Jack midi ports");
            case DEVCOL_STATE:
                  return QHeader::tr("State: result of opening the device");
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
      popup      = 0;
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
      mdevView->addColumn(tr("Routing"), 80);
      mdevView->addColumn(tr("State"));
      mdevView->setFocusPolicy(NoFocus);

      mdevView->setColumnAlignment(DEVCOL_NO, AlignHCenter);
      mdevView->setColumnAlignment(DEVCOL_GUI, AlignCenter);
      mdevView->setColumnAlignment(DEVCOL_REC, AlignCenter);
      mdevView->setColumnAlignment(DEVCOL_PLAY, AlignCenter);
      mdevView->header()->setResizeEnabled(false, DEVCOL_NO);
      mdevView->header()->setResizeEnabled(false, DEVCOL_REC);
      mdevView->header()->setResizeEnabled(false, DEVCOL_GUI);
      mdevView->setResizeMode(QListView::LastColumn);

      instanceList->setColumnAlignment(1, AlignHCenter);

      new MPWhatsThis(mdevView, mdevView->header());

      connect(mdevView, SIGNAL(pressed(QListViewItem*,const QPoint&,int)),
         this, SLOT(rbClicked(QListViewItem*,const QPoint&,int)));
      connect(song, SIGNAL(songChanged(int)), SLOT(songChanged(int)));

      connect(synthList, SIGNAL(selectionChanged()), SLOT(selectionChanged()));
      connect(instanceList, SIGNAL(selectionChanged()), SLOT(selectionChanged()));

      connect(addInstance, SIGNAL(clicked()), SLOT(addInstanceClicked()));
      connect(removeInstance, SIGNAL(clicked()), SLOT(removeInstanceClicked()));
      songChanged(0);
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
    
      mdevView->clear();
      for (int i = MIDI_PORTS-1; i >= 0; --i) {
            MidiPort* port  = &midiPorts[i];
            MidiDevice* dev = port->device();
            QString s;
            s.setNum(i+1);
            QListViewItem* item = new QListViewItem(mdevView);
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
              item->setPixmap(DEVCOL_ROUTES, *buttondownIcon);
              item->setText(DEVCOL_ROUTES, tr("routes"));
            }
            
            mdevView->insertItem(item);
            }

      QString s;
      synthList->clear();
      for (std::vector<Synth*>::iterator i = synthis.begin();
         i != synthis.end(); ++i) {
            //s = (*i)->baseName();
            //s = (*i)->name();

            QListViewItem* item = new QListViewItem(synthList);
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
            QListViewItem* iitem = new QListViewItem(instanceList);
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
      QListViewItem* item = synthList->selectedItem();
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
      QListViewItem* item = instanceList->selectedItem();
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

