//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: amixer.cpp,v 1.49.2.5 2009/11/16 01:55:55 terminator356 Exp $
//
//  (C) Copyright 2000-2004 Werner Schweer (ws@seh.de)
//=========================================================

#include <list>
#include <cmath>

#include <QApplication>
#include <QMenuBar>
//Added by qt3to4:
#include <QHBoxLayout>
#include <QCloseEvent>
#include <QMenu>
#include <QActionGroup>
#include <QAction>

#include "app.h"
#include "icons.h"
#include "amixer.h"
#include "song.h"

#include "astrip.h"
#include "mstrip.h"

#include "gconfig.h"
#include "xml.h"

extern void populateAddTrack(QMenu* addTrack);

#define __WIDTH_COMPENSATION 4

//typedef std::list<Strip*> StripList;
//static StripList stripList;

//---------------------------------------------------------
//   AudioMixer
//
//    inputs | synthis | tracks | groups | master
//---------------------------------------------------------

AudioMixerApp::AudioMixerApp(QWidget* parent, MixerConfig* c)
   : QMainWindow(parent)
      {
      cfg = c;
      oldAuxsSize = 0;
      routingDialog = 0;
      setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Expanding));   // TESTING Tim
      setWindowTitle(cfg->name);
      setWindowIcon(*museIcon);

      QMenu* menuConfig = menuBar()->addMenu(tr("&Create"));
      populateAddTrack(menuConfig);
      
      QMenu* menuView = menuBar()->addMenu(tr("&View"));
      routingId = menuView->addAction(tr("Routing"), this, SLOT(toggleRouteDialog()));
      routingId->setCheckable(true);

      menuView->addSeparator();

      QActionGroup* actionItems = new QActionGroup(this);
      actionItems->setExclusive(false);
      
      showMidiTracksId = new QAction(tr("Show Midi Tracks"), actionItems);
      showDrumTracksId = new QAction(tr("Show Drum Tracks"), actionItems);
      showWaveTracksId = new QAction(tr("Show Wave Tracks"), actionItems);

      actionItems->addSeparator();

      showInputTracksId = new QAction(tr("Show Inputs"), actionItems);
      showOutputTracksId = new QAction(tr("Show Outputs"), actionItems);
      showGroupTracksId = new QAction(tr("Show Groups"), actionItems);
      showAuxTracksId = new QAction(tr("Show Auxs"), actionItems);
      showSyntiTracksId = new QAction(tr("Show Synthesizers"), actionItems);

      showMidiTracksId->setCheckable(true);
      showDrumTracksId->setCheckable(true);
      showWaveTracksId->setCheckable(true);
      showInputTracksId->setCheckable(true);
      showOutputTracksId->setCheckable(true);
      showGroupTracksId->setCheckable(true);
      showAuxTracksId->setCheckable(true);
      showSyntiTracksId->setCheckable(true);

      //connect(menuView, SIGNAL(triggered(QAction*)), SLOT(showTracksChanged(QAction*)));
      //connect(actionItems, SIGNAL(selected(QAction*)), this, SLOT(showTracksChanged(QAction*)));
      connect(showMidiTracksId, SIGNAL(triggered(bool)), SLOT(showMidiTracksChanged(bool)));
      connect(showDrumTracksId, SIGNAL(triggered(bool)), SLOT(showDrumTracksChanged(bool)));      
      connect(showWaveTracksId, SIGNAL(triggered(bool)), SLOT(showWaveTracksChanged(bool)));      
      connect(showInputTracksId, SIGNAL(triggered(bool)), SLOT(showInputTracksChanged(bool)));      
      connect(showOutputTracksId, SIGNAL(triggered(bool)), SLOT(showOutputTracksChanged(bool)));      
      connect(showGroupTracksId, SIGNAL(triggered(bool)), SLOT(showGroupTracksChanged(bool)));      
      connect(showAuxTracksId, SIGNAL(triggered(bool)), SLOT(showAuxTracksChanged(bool)));      
      connect(showSyntiTracksId, SIGNAL(triggered(bool)), SLOT(showSyntiTracksChanged(bool)));      
              
      menuView->addActions(actionItems->actions());
      
      view = new QScrollArea();
      view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
      setCentralWidget(view);
      
      central = new QWidget(view);
      layout = new QHBoxLayout();
      central->setLayout(layout);
      layout->setSpacing(0);
      layout->setMargin(0);
      view->setWidget(central);
      view->setWidgetResizable(true);
      
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
//   computeWidth
//---------------------------------------------------------
int AudioMixerApp::computeWidth()
{
      int w = 0;
      StripList::iterator si = stripList.begin();
      for (; si != stripList.end(); ++si) {
            //w += (*si)->width();
            //w += (*si)->frameGeometry().width();
            Strip* s = *si;
            //w += s->width() + 2 * (s->frameWidth() + s->lineWidth() + s->midLineWidth());
            w += s->width() + 2 * s->frameWidth();
            }
      return w;      
}

//---------------------------------------------------------
//   updateMixer
//---------------------------------------------------------

void AudioMixerApp::updateMixer(UpdateAction action)
      {
      //printf("AudioMixerApp::updateMixer action:%d\n", action);
      
      //name = cfg->name;
      //setCaption(name);
      setCaption(cfg->name);
      
      showMidiTracksId->setOn(cfg->showMidiTracks);
      showDrumTracksId->setOn(cfg->showDrumTracks);
      showInputTracksId->setOn(cfg->showInputTracks);
      showOutputTracksId->setOn(cfg->showOutputTracks);
      showWaveTracksId->setOn(cfg->showWaveTracks);
      showGroupTracksId->setOn(cfg->showGroupTracks);
      showAuxTracksId->setOn(cfg->showAuxTracks);
      showSyntiTracksId->setOn(cfg->showSyntiTracks);

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
                  
            // TODO: See help Qt4 "Window Geometry"      
            // "On X11, a window does not have a frame until the window manager decorates it. 
            //  This happens asynchronously at some point in time after calling QWidget::show() 
            //   and the first paint event the window receives, or it does not happen at all. 
            // " ...you cannot make any safe assumption about the decoration frame your window will get." 
            // "X11 provides no standard or easy way to get the frame geometry once the window is decorated. 
            //  Qt solves this problem with nifty heuristics and clever code that works on a wide range of 
            //  window managers that exist today..."
            //
            // I think I may be seeing these issues here... 
            //
            //setMaximumWidth(STRIP_WIDTH * stripList.size() + __WIDTH_COMPENSATION);  
///            int w = computeWidth();      
///            setMaximumWidth(w);      
///            if (stripList.size() < 8)
            //      view->setMinimumWidth(stripList.size() * STRIP_WIDTH + __WIDTH_COMPENSATION);  
///                  view->setMinimumWidth(w);
                  
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
            for (iMidiTrack i = mtl->begin(); i != mtl->end(); ++i) 
            {
              MidiTrack* mt = *i;
              if((mt->type() == Track::MIDI && cfg->showMidiTracks) || (mt->type() == Track::DRUM && cfg->showDrumTracks)) 
                addStrip(*i, idx++);
            }
      
            //setMaximumWidth(STRIP_WIDTH * stripList.size() + __WIDTH_COMPENSATION);  
///            int w = computeWidth();      
///            setMaximumWidth(w);      
///            if (stripList.size() < 8)
            //      view->setMinimumWidth(stripList.size() * STRIP_WIDTH + __WIDTH_COMPENSATION); 
///                  view->setMinimumWidth(w);
            return;
      }

      int idx = 0;
      //---------------------------------------------------
      //  generate Input Strips
      //---------------------------------------------------

      if(cfg->showInputTracks)
      {
        InputList* itl = song->inputs();
        for (iAudioInput i = itl->begin(); i != itl->end(); ++i)
            addStrip(*i, idx++);
      }
      
      //---------------------------------------------------
      //  Synthesizer Strips
      //---------------------------------------------------

      if(cfg->showSyntiTracks)
      {
        SynthIList* sl = song->syntis();
        for (iSynthI i = sl->begin(); i != sl->end(); ++i)
            addStrip(*i, idx++);
      }
      
      //---------------------------------------------------
      //  generate Wave Track Strips
      //---------------------------------------------------

      if(cfg->showWaveTracks)
      {
        WaveTrackList* wtl = song->waves();
        for (iWaveTrack i = wtl->begin(); i != wtl->end(); ++i)
            addStrip(*i, idx++);
      }
      
      //---------------------------------------------------
      //  generate Midi channel/port Strips
      //---------------------------------------------------

      MidiTrackList* mtl = song->midis();
      for (iMidiTrack i = mtl->begin(); i != mtl->end(); ++i) 
      {
        MidiTrack* mt = *i;
        if((mt->type() == Track::MIDI && cfg->showMidiTracks) || (mt->type() == Track::DRUM && cfg->showDrumTracks)) 
          addStrip(*i, idx++);
      }

      //---------------------------------------------------
      //  Groups
      //---------------------------------------------------

      if(cfg->showGroupTracks)
      {
        GroupList* gtl = song->groups();
        for (iAudioGroup i = gtl->begin(); i != gtl->end(); ++i)
            addStrip(*i, idx++);
      }
      
      //---------------------------------------------------
      //  Aux
      //---------------------------------------------------

      if(cfg->showAuxTracks)
      {
        AuxList* al = song->auxs();
        for (iAudioAux i = al->begin(); i != al->end(); ++i)
            addStrip(*i, idx++);
      }
      
      //---------------------------------------------------
      //    Master
      //---------------------------------------------------

      if(cfg->showOutputTracks)
      {
        OutputList* otl = song->outputs();
        for (iAudioOutput i = otl->begin(); i != otl->end(); ++i)
            addStrip(*i, idx++);
      }
      
      //setMaximumWidth(STRIP_WIDTH * idx + __WIDTH_COMPENSATION);     
///      int w = computeWidth();      
///      setMaximumWidth(w);      
///      if (idx < 8)
      //      view->setMinimumWidth(idx * STRIP_WIDTH + __WIDTH_COMPENSATION); 
///            view->setMinimumWidth(w);
      }

//---------------------------------------------------------
//   configChanged
//---------------------------------------------------------

void AudioMixerApp::configChanged()    
{ 
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
      showRouteDialog(routingId->isChecked());
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
      //menuView->setItemChecked(routingId, on);
      routingId->setChecked(on);
      }

//---------------------------------------------------------
//   routingDialogClosed
//---------------------------------------------------------

void AudioMixerApp::routingDialogClosed()
      {
      routingId->setChecked(false);
      }

//---------------------------------------------------------
//   showTracksChanged
//---------------------------------------------------------

/*
void AudioMixerApp::showTracksChanged(QAction* id)
      {
      bool val = id->isOn();
      if (id == showMidiTracksId)
            cfg->showMidiTracks = val;
      else if (id == showDrumTracksId)
            cfg->showDrumTracks = val;
      else if (id == showInputTracksId)
            cfg->showInputTracks = val;
      else if (id == showOutputTracksId)
            cfg->showOutputTracks = val;
      else if (id == showWaveTracksId)
            cfg->showWaveTracks = val;
      else if (id == showGroupTracksId)
            cfg->showGroupTracks = val;
      else if (id == showAuxTracksId)
            cfg->showAuxTracks = val;
      else if (id == showSyntiTracksId)
            cfg->showSyntiTracks = val;
      updateMixer(UPDATE_ALL);
      }
*/

void AudioMixerApp::showMidiTracksChanged(bool v)
{
      cfg->showMidiTracks = v;
      updateMixer(UPDATE_ALL);
}

void AudioMixerApp::showDrumTracksChanged(bool v)
{
      cfg->showDrumTracks = v;
      updateMixer(UPDATE_ALL);
}

void AudioMixerApp::showWaveTracksChanged(bool v)
{
      cfg->showWaveTracks = v;
      updateMixer(UPDATE_ALL);
}

void AudioMixerApp::showInputTracksChanged(bool v)
{
      cfg->showInputTracks = v;
      updateMixer(UPDATE_ALL);
}

void AudioMixerApp::showOutputTracksChanged(bool v)
{
      cfg->showOutputTracks = v;
      updateMixer(UPDATE_ALL);
}

void AudioMixerApp::showGroupTracksChanged(bool v)
{
      cfg->showGroupTracks = v;
      updateMixer(UPDATE_ALL);
}

void AudioMixerApp::showAuxTracksChanged(bool v)
{
      cfg->showAuxTracks = v;
      updateMixer(UPDATE_ALL);
}

void AudioMixerApp::showSyntiTracksChanged(bool v)
{
      cfg->showSyntiTracks = v;
      updateMixer(UPDATE_ALL);
}

//---------------------------------------------------------
//   write
//---------------------------------------------------------

//void AudioMixerApp::write(Xml& xml, const char* name)
void AudioMixerApp::write(int level, Xml& xml)
//void AudioMixerApp::write(int level, Xml& xml, const char* name)
      {
      //xml.stag(QString(name));
      //xml.tag(level++, name.latin1());
      xml.tag(level++, "Mixer");
      
      xml.strTag(level, "name", cfg->name);
      
      //xml.tag("geometry",       geometry());
      xml.qrectTag(level, "geometry", geometry());
      
      xml.intTag(level, "showMidiTracks",   cfg->showMidiTracks);
      xml.intTag(level, "showDrumTracks",   cfg->showDrumTracks);
      xml.intTag(level, "showInputTracks",  cfg->showInputTracks);
      xml.intTag(level, "showOutputTracks", cfg->showOutputTracks);
      xml.intTag(level, "showWaveTracks",   cfg->showWaveTracks);
      xml.intTag(level, "showGroupTracks",  cfg->showGroupTracks);
      xml.intTag(level, "showAuxTracks",    cfg->showAuxTracks);
      xml.intTag(level, "showSyntiTracks",  cfg->showSyntiTracks);
      
      //xml.etag(name);
      //xml.etag(level, name.latin1());
      xml.etag(level, "Mixer");
      }

