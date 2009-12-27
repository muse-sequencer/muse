//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: amixer.cpp,v 1.49.2.5 2009/11/16 01:55:55 terminator356 Exp $
//
//  (C) Copyright 2000-2004 Werner Schweer (ws@seh.de)
//=========================================================

#include <list>
#include <cmath>

#include <qapplication.h>
#include <qmenubar.h>

#include "app.h"
#include "amixer.h"
#include "song.h"

#include "astrip.h"
#include "mstrip.h"

extern void populateAddTrack(QPopupMenu* addTrack);

typedef std::list<Strip*> StripList;
static StripList stripList;

//---------------------------------------------------------
//   AudioMixer
//
//    inputs | synthis | tracks | groups | master
//---------------------------------------------------------

AudioMixerApp::AudioMixerApp(QWidget* parent)
   : QMainWindow(parent, "mixer")
      {
      oldAuxsSize = 0;
      routingDialog = 0;
      setCaption(tr("MusE: Mixer"));

      QPopupMenu* menuConfig = new QPopupMenu(this);
      menuBar()->insertItem(tr("&Create"), menuConfig);
      populateAddTrack(menuConfig);
      
      menuView = new QPopupMenu(this);
      menuBar()->insertItem(tr("&View"), menuView);
      routingId = menuView->insertItem(tr("Routing"), this, SLOT(toggleRouteDialog()));

      view = new QScrollView(this);
      setCentralWidget(view);
      central = new QWidget(view);
      view->setResizePolicy(QScrollView::AutoOneFit);
      view->setVScrollBarMode(QScrollView::AlwaysOff);
      view->addChild(central);
      layout = new QHBoxLayout(central);
      connect(song, SIGNAL(songChanged(int)), SLOT(songChanged(int)));
      connect(muse, SIGNAL(configChanged()), SLOT(configChanged()));
      song->update();  // calls update mixer
      }

//---------------------------------------------------------
//   addStrip
//---------------------------------------------------------

void AudioMixerApp::addStrip(Track* t, int idx)
      {
      StripList::iterator si = stripList.begin();
      for (int i = 0; i < idx; ++i) {
            if (si != stripList.end())
                  ++si;
            }
      if (si != stripList.end() && (*si)->getTrack() == t)
            return;

      std::list<Strip*>::iterator nsi = si;
      ++nsi;
      if (si != stripList.end()
         && nsi != stripList.end()
         && (*nsi)->getTrack() == t) {
            layout->remove(*si);
            delete *si;
            stripList.erase(si);
            }
      else {
            Strip* strip;
            if (t->isMidiTrack())
                  strip = new MidiStrip(central, (MidiTrack*)t);
            else
                  strip = new AudioStrip(central, (AudioTrack*)t);
            layout->insertWidget(idx, strip);
            stripList.insert(si, strip);
            strip->show();  
            }
      }

//---------------------------------------------------------
//   clear
//---------------------------------------------------------

void AudioMixerApp::clear()
      {
      StripList::iterator si = stripList.begin();
      for (; si != stripList.end(); ++si) {
            layout->remove(*si);
            delete *si;
            }
      stripList.clear();
      oldAuxsSize = -1;
      }

//---------------------------------------------------------
//   updateMixer
//---------------------------------------------------------

void AudioMixerApp::updateMixer(UpdateAction action)
      {
      int auxsSize = song->auxs()->size();
      if ((action == UPDATE_ALL) || (auxsSize != oldAuxsSize)) {
            clear();
            oldAuxsSize = auxsSize;
            }
      else if (action == STRIP_REMOVED) 
      {
            StripList::iterator si = stripList.begin();
            for (; si != stripList.end();) {
                  Track* track = (*si)->getTrack();
                  TrackList* tl = song->tracks();
                  iTrack it;
                  for (it = tl->begin(); it != tl->end(); ++it) {
                        if (*it == track)
                              break;
                        }
                  StripList::iterator ssi = si;
                  ++si;
                  if (it != tl->end())
                        continue;
                  layout->remove(*ssi);
                  delete *ssi;
                  stripList.erase(ssi);
                  }
            setMaximumWidth(STRIP_WIDTH * stripList.size());
            // Added by Tim. p3.3.7
            if (stripList.size() < 8)
                  view->setMinimumWidth(stripList.size() * STRIP_WIDTH);
                  
            return;
      }
      // Added by Tim. p3.3.7
      else if (action == UPDATE_MIDI) 
      {
            int i = 0;
            int idx = -1;
            StripList::iterator si = stripList.begin();
            for (; si != stripList.end(); ++i) 
            {
                  Track* track = (*si)->getTrack();
                  if(!track->isMidiTrack())
                  {
                    ++si;
                    continue;
                  }
                  
                  if(idx == -1)
                    idx = i;
                     
                  StripList::iterator ssi = si;
                  ++si;
                  layout->remove(*ssi);
                  delete *ssi;
                  stripList.erase(ssi);
            }
            
            if(idx == -1)
              idx = 0;
              
            //---------------------------------------------------
            //  generate Midi channel/port Strips
            //---------------------------------------------------
      
            MidiTrackList* mtl = song->midis();
            int ports[MIDI_PORTS];
            memset(ports, 0, MIDI_PORTS * sizeof(int));
            for (iMidiTrack i = mtl->begin(); i != mtl->end(); ++i) {
                  MidiTrack* track = *i;
                  int port = track->outPort();
                  int channel = track->outChannel();
                  if ((ports[port] & (1 << channel)) == 0) {
                        addStrip(*i, idx++);
                        ports[port] |= 1 << channel;
                        }
                  }
      
            setMaximumWidth(STRIP_WIDTH * stripList.size());
            if (stripList.size() < 8)
                  view->setMinimumWidth(stripList.size() * STRIP_WIDTH);
            return;
      }

      int idx = 0;
      //---------------------------------------------------
      //  generate Input Strips
      //---------------------------------------------------

      InputList* itl = song->inputs();
      for (iAudioInput i = itl->begin(); i != itl->end(); ++i)
            addStrip(*i, idx++);

      //---------------------------------------------------
      //  Synthesizer Strips
      //---------------------------------------------------

      SynthIList* sl = song->syntis();
      for (iSynthI i = sl->begin(); i != sl->end(); ++i)
            addStrip(*i, idx++);

      //---------------------------------------------------
      //  generate Wave Track Strips
      //---------------------------------------------------

      WaveTrackList* wtl = song->waves();
      for (iWaveTrack i = wtl->begin(); i != wtl->end(); ++i)
            addStrip(*i, idx++);

      //---------------------------------------------------
      //  generate Midi channel/port Strips
      //---------------------------------------------------

      MidiTrackList* mtl = song->midis();
      int ports[MIDI_PORTS];
      memset(ports, 0, MIDI_PORTS * sizeof(int));
      for (iMidiTrack i = mtl->begin(); i != mtl->end(); ++i) {
            MidiTrack* track = *i;
            int port = track->outPort();
            int channel = track->outChannel();
            if ((ports[port] & (1 << channel)) == 0) {
                  addStrip(*i, idx++);
                  ports[port] |= 1 << channel;
                  }
            }

      //---------------------------------------------------
      //  Groups
      //---------------------------------------------------

      GroupList* gtl = song->groups();
      for (iAudioGroup i = gtl->begin(); i != gtl->end(); ++i)
            addStrip(*i, idx++);

      //---------------------------------------------------
      //  Aux
      //---------------------------------------------------

      AuxList* al = song->auxs();
      for (iAudioAux i = al->begin(); i != al->end(); ++i)
            addStrip(*i, idx++);

      //---------------------------------------------------
      //    Master
      //---------------------------------------------------

      OutputList* otl = song->outputs();
      for (iAudioOutput i = otl->begin(); i != otl->end(); ++i)
            addStrip(*i, idx++);

      setMaximumWidth(STRIP_WIDTH * idx);
      if (idx < 8)
            view->setMinimumWidth(idx * STRIP_WIDTH);
      }

//---------------------------------------------------------
//   configChanged
//---------------------------------------------------------

void AudioMixerApp::configChanged()    
{ 
  // Added by Tim. p3.3.6
  //printf("AudioMixerApp::configChanged\n");
      
  songChanged(SC_CONFIG); 
}

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void AudioMixerApp::songChanged(int flags)
      {
      // Is it simply a midi controller value adjustment? Forget it.
      if(flags == SC_MIDI_CONTROLLER)
        return;
    
// printf("  =======AudioMixer::songChanged %x\n", flags);
      UpdateAction action = NO_UPDATE;
      if (flags == -1)
            action = UPDATE_ALL;
      else if (flags & SC_TRACK_REMOVED)
            action = STRIP_REMOVED;
      else if (flags & SC_TRACK_INSERTED)
            action = STRIP_INSERTED;
      else if (flags & SC_MIDI_CHANNEL)
            action = UPDATE_MIDI;
      if (action != NO_UPDATE)
            updateMixer(action);
      if (action != UPDATE_ALL) {
            StripList::iterator si = stripList.begin();
            for (; si != stripList.end(); ++si) {
                  (*si)->songChanged(flags);
                  }
            }
      }

//---------------------------------------------------------
//   closeEvent
//---------------------------------------------------------

void AudioMixerApp::closeEvent(QCloseEvent* e)
      {
      emit closed();
      e->accept();
      }

//---------------------------------------------------------
//   toggleRouteDialog
//---------------------------------------------------------

void AudioMixerApp::toggleRouteDialog()
      {
      showRouteDialog(!menuView->isItemChecked(routingId));
      }

//---------------------------------------------------------
//   showRouteDialog
//---------------------------------------------------------

void AudioMixerApp::showRouteDialog(bool on)
      {
      if (on && routingDialog == 0) {
            routingDialog = new RouteDialog(this);
            connect(routingDialog, SIGNAL(closed()), SLOT(routingDialogClosed()));
            }
      if (routingDialog)
            routingDialog->setShown(on);
      menuView->setItemChecked(routingId, on);
      }

//---------------------------------------------------------
//   routingDialogClosed
//---------------------------------------------------------

void AudioMixerApp::routingDialogClosed()
      {
      menuView->setItemChecked(routingId, false);
      }

