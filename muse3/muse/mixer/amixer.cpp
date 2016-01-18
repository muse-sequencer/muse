//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: amixer.cpp,v 1.49.2.5 2009/11/16 01:55:55 terminator356 Exp $
//
//  (C) Copyright 2000-2004 Werner Schweer (ws@seh.de)
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
#include <cmath>

#include <QApplication>
#include <QMenuBar>
#include <QResizeEvent>
#include <QPaintEvent>
#include <QHBoxLayout>
#include <QCloseEvent>
#include <QMenu>
#include <QActionGroup>
#include <QAction>
#include <QThread>

#include "app.h"
#include "helper.h"
#include "icons.h"
#include "amixer.h"
#include "song.h"
#include "audio.h"

#include "astrip.h"
#include "mstrip.h"
#include "track.h"

#include "gconfig.h"
#include "xml.h"

#define __WIDTH_COMPENSATION 4

#define DEBUG_MIXER 0

//typedef std::list<Strip*> StripList;
//static StripList stripList;

namespace MusEGui {

/* 
Nov 16, 2010: After making the strips variable width, we need a way to
 set the maximum size of the main window.
 
// See help Qt4 "Window Geometry"      
// "On X11, a window does not have a frame until the window manager decorates it. 
//  This happens asynchronously at some point in time after calling QWidget::show() 
//   and the first paint event the window receives, or it does not happen at all. 
// " ...you cannot make any safe assumption about the decoration frame your window will get." 
// "X11 provides no standard or easy way to get the frame geometry once the window is decorated. 
//  Qt solves this problem with nifty heuristics and clever code that works on a wide range of 
//  window managers that exist today..."
//
            
Sequence of events when mixer is opened, and then when a strip is added:

ViewWidget::event type:68                 // Mixer opened:
Event is QEvent::ChildAdded
ViewWidget::event type:18  
ViewWidget::event type:27  
ViewWidget::event type:131 
ScrollArea::viewportEvent type:68
Event is QEvent::ChildAdded      
ViewWidget::event type:21        
ViewWidget::event type:75        
ViewWidget::event type:70        
ScrollArea::viewportEvent type:69
Event is QEvent::ChildPolished   
child width:100 frame width:100            
ViewWidget::event type:26        
ViewWidget::event type:68        
Event is QEvent::ChildAdded      
ViewWidget::event type:69        
Event is QEvent::ChildPolished   
child width:100 frame width:100            // Size is not correct yet
AudioMixerApp::updateMixer other 
ScrollArea::viewportEvent type:75
ScrollArea::viewportEvent type:70
ScrollArea::viewportEvent type:13
ScrollArea::viewportEvent type:14
ViewWidget::event type:70        
ViewWidget::event type:13        
ViewWidget::event type:14        
ViewWidget::event type:17        
ScrollArea::viewportEvent type:17
ScrollArea::viewportEvent type:26
ViewWidget::event type:67        
ScrollArea::viewportEvent type:67
ViewWidget::event type:67        
ScrollArea::viewportEvent type:14
ViewWidget::event type:14        
ScrollArea::viewportEvent type:74
ViewWidget::event type:74        
ViewWidget::event type:76        
ScrollArea::viewportEvent type:76                  // Layout request: 
Event is QEvent::LayoutRequest   
AudioMixerApp::setSizing width:75 frame width:2
ScrollArea::viewportEvent type:14                  
ViewWidget::event type:14                      
ViewWidget::event type:12                          // Paint event:
ViewWidget::paintEvent                             // By this time the size is correct.
ScrollArea::viewportEvent type:24                  // But to avoid having to do the resizing
ViewWidget::event type:24                          //  in every paint event, do it just after
ScrollArea::viewportEvent type:14                  //  the layout request, as shown above.
ViewWidget::event type:14                          // Hopefully that is a good time to do it.
ViewWidget::event type:12                      
ViewWidget::paintEvent                         
ScrollArea::viewportEvent type:25              
ViewWidget::event type:25                      

ViewWidget::event type:68                          // Strip is added:
Event is QEvent::ChildAdded
ViewWidget::event type:69
Event is QEvent::ChildPolished
child width:100 frame width:100                    // Size not correct yet.
ViewWidget::event type:70                          
AudioMixerApp::updateMixer other
ViewWidget::event type:67
ViewWidget::event type:76
ScrollArea::viewportEvent type:76
ViewWidget::event type:14
Event is QEvent::LayoutRequest
AudioMixerApp::setSizing width:75 frame width:2
AudioMixerApp::setSizing width:75 frame width:2
ViewWidget::event type:12                          // Size is correct by now. 
ViewWidget::paintEvent
*/

bool ScrollArea::viewportEvent(QEvent* event)
{
  event->ignore();
  // Let it do the layout now, before we emit.
  QScrollArea::viewportEvent(event);
  
  if(event->type() == QEvent::LayoutRequest)       
    emit layoutRequest();
         
//   return false;       
  return true;       
}

//---------------------------------------------------------
//   AudioMixer
//
//    inputs | synthis | tracks | groups | master
//---------------------------------------------------------

AudioMixerApp::AudioMixerApp(QWidget* parent, MusEGlobal::MixerConfig* c)
   : QMainWindow(parent)
{
      cfg = c;
      oldAuxsSize = 0;
      routingDialog = 0;
      setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Expanding));   // TESTING Tim
      setWindowTitle(cfg->name);
      setWindowIcon(*museIcon);
      mixerClicked=false;

      //cfg->displayOrder = MusEGlobal::MixerConfig::STRIPS_TRADITIONAL_VIEW;

      QMenu* menuConfig = menuBar()->addMenu(tr("&Create"));
      MusEGui::populateAddTrack(menuConfig,true);
      connect(menuConfig, SIGNAL(triggered(QAction *)), MusEGlobal::song, SLOT(addNewTrack(QAction *)));
      
      QMenu* menuView = menuBar()->addMenu(tr("&View"));
      menuStrips = menuView->addMenu(tr("Strips"));
      connect(menuStrips, SIGNAL(aboutToShow()), SLOT(stripsMenu()));

      routingId = menuView->addAction(tr("Routing"), this, SLOT(toggleRouteDialog()));
      routingId->setCheckable(true);

      menuView->addSeparator();

      QActionGroup* actionItems = new QActionGroup(this);
      actionItems->setExclusive(false);
      
      showMidiTracksId = new QAction(tr("Show Midi Tracks"), actionItems);
      showDrumTracksId = new QAction(tr("Show Drum Tracks"), actionItems);
      showNewDrumTracksId = new QAction(tr("Show New Style Drum Tracks"), actionItems);
      showWaveTracksId = new QAction(tr("Show Wave Tracks"), actionItems);

      QAction *separator = new QAction(this);
      separator->setSeparator(true);
      actionItems->addAction(separator);

      showInputTracksId = new QAction(tr("Show Inputs"), actionItems);
      showOutputTracksId = new QAction(tr("Show Outputs"), actionItems);
      showGroupTracksId = new QAction(tr("Show Groups"), actionItems);
      showAuxTracksId = new QAction(tr("Show Auxs"), actionItems);
      showSyntiTracksId = new QAction(tr("Show Synthesizers"), actionItems);

      showMidiTracksId->setCheckable(true);
      showDrumTracksId->setCheckable(true);
      showNewDrumTracksId->setCheckable(true);
      showWaveTracksId->setCheckable(true);
      showInputTracksId->setCheckable(true);
      showOutputTracksId->setCheckable(true);
      showGroupTracksId->setCheckable(true);
      showAuxTracksId->setCheckable(true);
      showSyntiTracksId->setCheckable(true);

      connect(showMidiTracksId, SIGNAL(triggered(bool)), SLOT(showMidiTracksChanged(bool)));
      connect(showDrumTracksId, SIGNAL(triggered(bool)), SLOT(showDrumTracksChanged(bool)));      
      connect(showNewDrumTracksId, SIGNAL(triggered(bool)), SLOT(showNewDrumTracksChanged(bool)));      
      connect(showWaveTracksId, SIGNAL(triggered(bool)), SLOT(showWaveTracksChanged(bool)));      
      connect(showInputTracksId, SIGNAL(triggered(bool)), SLOT(showInputTracksChanged(bool)));      
      connect(showOutputTracksId, SIGNAL(triggered(bool)), SLOT(showOutputTracksChanged(bool)));      
      connect(showGroupTracksId, SIGNAL(triggered(bool)), SLOT(showGroupTracksChanged(bool)));      
      connect(showAuxTracksId, SIGNAL(triggered(bool)), SLOT(showAuxTracksChanged(bool)));      
      connect(showSyntiTracksId, SIGNAL(triggered(bool)), SLOT(showSyntiTracksChanged(bool)));      
              
      menuView->addActions(actionItems->actions());
      
      ///view = new QScrollArea();
      view = new ScrollArea();
      view->setContentsMargins(0, 0, 0, 0);
      //view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
      setCentralWidget(view);
      
      central = new QWidget(view);
      central->setContentsMargins(0, 0, 0, 0);
      //splitter = new QSplitter(view);
      mixerLayout = new QHBoxLayout();
      central->setLayout(mixerLayout);
      mixerLayout->setSpacing(0);
      mixerLayout->setContentsMargins(0, 0, 0, 0);
      //layout->setSpacing(0);  // REMOVE Tim. Trackinfo. Duplicate.
      view->setWidget(central);
      //view->setWidget(splitter);
      view->setWidgetResizable(true);
      
      connect(view, SIGNAL(layoutRequest()), SLOT(setSizing()));  
      
      connect(MusEGlobal::song, SIGNAL(songChanged(MusECore::SongChangedFlags_t)), SLOT(songChanged(MusECore::SongChangedFlags_t)));
      connect(MusEGlobal::muse, SIGNAL(configChanged()), SLOT(configChanged()));
      
      initMixer();
      redrawMixer();
}

void AudioMixerApp::stripsMenu()
{
  menuStrips->clear();
  connect(menuStrips, SIGNAL(triggered(QAction*)), SLOT(handleMenu(QAction*)));
  QAction *act;

  act = menuStrips->addAction(tr("Traditional order"));
  act->setData(MusEGlobal::MixerConfig::STRIPS_TRADITIONAL_VIEW);
  act->setCheckable(true);
  if (cfg->displayOrder == MusEGlobal::MixerConfig::STRIPS_TRADITIONAL_VIEW)
    act->setChecked(true);

  act = menuStrips->addAction(tr("Arranger order"));
  act->setData(MusEGlobal::MixerConfig::STRIPS_ARRANGER_VIEW);
  act->setCheckable(true);
  if (cfg->displayOrder == MusEGlobal::MixerConfig::STRIPS_ARRANGER_VIEW)
    act->setChecked(true);

  act = menuStrips->addAction(tr("User order"));
  act->setData(MusEGlobal::MixerConfig::STRIPS_EDITED_VIEW);
  act->setCheckable(true);
  if (cfg->displayOrder == MusEGlobal::MixerConfig::STRIPS_EDITED_VIEW)
    act->setChecked(true);

  menuStrips->addSeparator();
  act = menuStrips->addAction(tr("Show all hidden strips"));
  act->setData(UNHIDE_STRIPS);
  menuStrips->addSeparator();

  // loop through all tracks and show the hidden ones
  int i=0,h=0;
  foreach (Strip *s, stripList) {
    if (!s->getStripVisible()){
      act = menuStrips->addAction(tr("Unhide strip: ") + s->getTrack()->name());
      act->setData(i);
      h++;
    }
    i++;
  }
  if (h==0) {
    act = menuStrips->addAction(tr("No hidden strips"));
    act->setData(UNHANDLED_NUMBER);
  }
}

void AudioMixerApp::handleMenu(QAction *act)
{
  if (DEBUG_MIXER)
    printf("handleMenu %d\n", act->data().toInt());
  int operation = act->data().toInt();
  if (operation >= 0) {
    stripList.at(act->data().toInt())->setStripVisible(true);
  } else if (operation ==  UNHIDE_STRIPS) {
    foreach (Strip *s, stripList) {
      s->setStripVisible(true);
    }
  } else if (operation == MusEGlobal::MixerConfig::STRIPS_TRADITIONAL_VIEW) {
    cfg->displayOrder = MusEGlobal::MixerConfig::STRIPS_TRADITIONAL_VIEW;
  } else if (operation == MusEGlobal::MixerConfig::STRIPS_ARRANGER_VIEW) {
    cfg->displayOrder = MusEGlobal::MixerConfig::STRIPS_ARRANGER_VIEW;
  } else if (operation == MusEGlobal::MixerConfig::STRIPS_EDITED_VIEW) {
    cfg->displayOrder = MusEGlobal::MixerConfig::STRIPS_EDITED_VIEW;
  }
  redrawMixer();
}

bool AudioMixerApp::stripIsVisible(Strip* s)
{
  if (!s->getStripVisible())
    return false;

  MusECore::Track *t = s->getTrack();
  switch (t->type())
  {
    case MusECore::Track::AUDIO_SOFTSYNTH:
      if (!cfg->showSyntiTracks)
        return false;
      break;
    case MusECore::Track::AUDIO_OUTPUT:
      if (!cfg->showInputTracks)
        return false;
      break;
    case MusECore::Track::AUDIO_INPUT:
      if (!cfg->showOutputTracks)
        return false;
      break;
    case MusECore::Track::AUDIO_AUX:
      if (!cfg->showAuxTracks)
        return false;
      break;
    case MusECore::Track::AUDIO_GROUP:
      if (!cfg->showGroupTracks)
        return false;
      break;
    case MusECore::Track::WAVE:
      if (!cfg->showWaveTracks)
        return false;
      break;
    case MusECore::Track::MIDI:
    case MusECore::Track::DRUM:
    case MusECore::Track::NEW_DRUM:
      if (!cfg->showMidiTracks)
        return false;
      break;

  }
  return true;
}

void AudioMixerApp::redrawMixer()
{
  if (DEBUG_MIXER)
    printf("redrawMixer type %d, mixerLayout count %d\n", cfg->displayOrder, mixerLayout->count());
  // empty layout
  while (mixerLayout->count() > 0) {
    mixerLayout->removeItem(mixerLayout->itemAt(0));
  }

  switch (cfg->displayOrder) {
    case MusEGlobal::MixerConfig::STRIPS_ARRANGER_VIEW:
      {
      if (DEBUG_MIXER)
          printf("Draw strips with arranger view\n");
        MusECore::TrackList *tl = MusEGlobal::song->tracks();
        MusECore::TrackList::iterator tli = tl->begin();
        for (; tli != tl->end(); tli++) {
          if (DEBUG_MIXER)
            printf("Adding strip %s\n", (*tli)->name().toLatin1().data());
          StripList::iterator si = stripList.begin();
          for (; si != stripList.end(); si++) {
            if((*si)->getTrack() == *tli) {
              addStripToLayoutIfVisible(*si);
            }
          }
        }
      }
      break;
    case MusEGlobal::MixerConfig::STRIPS_EDITED_VIEW:
      {
        if (DEBUG_MIXER)
          printf("Draw strips with edited view\n");
        // add them back in the selected order
        StripList::iterator si = stripList.begin();
        for (; si != stripList.end(); ++si) {
            if (DEBUG_MIXER)
              printf("Adding strip %s\n", (*si)->getTrack()->name().toLatin1().data());
            addStripToLayoutIfVisible(*si);
        }
        if (DEBUG_MIXER)
          printf("mixerLayout count is now %d\n", mixerLayout->count());
      }
      break;
    case MusEGlobal::MixerConfig::STRIPS_TRADITIONAL_VIEW:
      {
        if (DEBUG_MIXER)
          printf("TRADITIONAL VIEW mixerLayout count is now %d\n", mixerLayout->count());
        addStripsTraditionalLayout();
      }

      break;
  }

  update();
}

Strip* AudioMixerApp::findStripForTrack(StripList &sl, MusECore::Track *t)
{
  StripList::iterator si = sl.begin();
  for (;si != sl.end(); si++) {
    if ((*si)->getTrack() == t)
      return *si;
  }
  if (DEBUG_MIXER)
    printf("AudioMixerApp::findStripForTrack - ERROR: there was no strip for this track!\n");
  return NULL;
}

void AudioMixerApp::fillStripListTraditional()
{
  StripList oldList = stripList;
  stripList.clear();
  MusECore::TrackList *tl = MusEGlobal::song->tracks();

  //  add Input Strips
  MusECore::TrackList::iterator tli = tl->begin();
  for (; tli != tl->end(); ++tli) {
    if ((*tli)->type() == MusECore::Track::AUDIO_INPUT)
      stripList.append(findStripForTrack(oldList,*tli));
  }

  //  Synthesizer Strips
  tli = tl->begin();
  for (; tli != tl->end(); ++tli) {
    if ((*tli)->type() == MusECore::Track::AUDIO_SOFTSYNTH)
      stripList.append(findStripForTrack(oldList,*tli));
  }

  //  generate Wave Track Strips
  tli = tl->begin();
  for (; tli != tl->end(); ++tli) {
    if ((*tli)->type() == MusECore::Track::WAVE)
      stripList.append(findStripForTrack(oldList,*tli));
  }

  //  generate Midi channel/port Strips
  tli = tl->begin();
  for (; tli != tl->end(); ++tli) {
    if ((*tli)->type() == MusECore::Track::MIDI ||
        (*tli)->type() == MusECore::Track::DRUM ||
        (*tli)->type() == MusECore::Track::NEW_DRUM)
      stripList.append(findStripForTrack(oldList,*tli));
  }

  //  Groups
  tli = tl->begin();
  for (; tli != tl->end(); ++tli) {
    if ((*tli)->type() == MusECore::Track::AUDIO_GROUP)
      stripList.append(findStripForTrack(oldList,*tli));
  }

  //  Aux
  tli = tl->begin();
  for (; tli != tl->end(); ++tli) {
    if ((*tli)->type() == MusECore::Track::AUDIO_AUX)
      stripList.append(findStripForTrack(oldList,*tli));
  }

  //    Master
  tli = tl->begin();
  for (; tli != tl->end(); ++tli) {
    if ((*tli)->type() == MusECore::Track::AUDIO_OUTPUT)
      stripList.append(findStripForTrack(oldList,*tli));
  }
}


void AudioMixerApp::moveStrip(Strip *s)
{
  mixerClicked = false;
  if (DEBUG_MIXER)
    printf("Recreate stripList\n");
  if (cfg->displayOrder == MusEGlobal::MixerConfig::STRIPS_ARRANGER_VIEW) {

    for (int i=0; i< stripList.size(); i++)
    {
      Strip *s2 = stripList.at(i);
      if (s2 == s) continue;

      if (DEBUG_MIXER)
        printf("loop loop %d %d width %d\n", s->pos().x(),s2->pos().x(), s2->width());
      if (s->pos().x()+s->width()/2 < s2->pos().x()+s2->width() // upper limit
          && s->pos().x()+s->width()/2 > s2->pos().x() ) // lower limit
      {
        // found relevant pos.
        int sTrack = MusEGlobal::song->tracks()->index(s->getTrack());
        int dTrack = MusEGlobal::song->tracks()->index(s2->getTrack());
        MusEGlobal::audio->msgMoveTrack(sTrack, dTrack);
      }
    }

  } else if (cfg->displayOrder == MusEGlobal::MixerConfig::STRIPS_TRADITIONAL_VIEW)
  {
    fillStripListTraditional();
    cfg->displayOrder = MusEGlobal::MixerConfig::STRIPS_EDITED_VIEW;
  }
  if (DEBUG_MIXER)
    printf("moveStrip %s! stripList.size = %d\n", s->getLabelText().toLatin1().data(), stripList.size());

  for (int i=0; i< stripList.size(); i++)
  {
    Strip *s2 = stripList.at(i);
    if (s2 == s) continue;

    if (DEBUG_MIXER)
      printf("loop loop %d %d width %d\n", s->pos().x(),s2->pos().x(), s2->width());
    if (s->pos().x()+s->width()/2 < s2->pos().x()+s2->width() // upper limit
        && s->pos().x()+s->width()/2 > s2->pos().x() ) // lower limit
    {
      if (DEBUG_MIXER)
        printf("got new pos: %d\n", i);
      bool isSuccess = stripList.removeOne(s);
      if (DEBUG_MIXER)
        printf("Removed strip %d", isSuccess);
      stripList.insert(i,s);
      if (DEBUG_MIXER)
        printf("Inserted strip at %d", i);
      break;
    }
  }
  redrawMixer();
  update();
}

void AudioMixerApp::addStripToLayoutIfVisible(Strip *s)
{
  if (stripIsVisible(s)) {
    s->setVisible(true);
    mixerLayout->addWidget(s);
  } else {
    s->setVisible(false);
  }
}

void AudioMixerApp::addStripsTraditionalLayout()
{
  //  generate Input Strips
  StripList::iterator si = stripList.begin();
  for (; si != stripList.end(); ++si) {
    if ((*si)->getTrack()->type() == MusECore::Track::AUDIO_INPUT)
      addStripToLayoutIfVisible(*si);
  }

  //  Synthesizer Strips
  si = stripList.begin();
  for (; si != stripList.end(); ++si) {
    if ((*si)->getTrack()->type() == MusECore::Track::AUDIO_SOFTSYNTH)
      addStripToLayoutIfVisible(*si);
  }

  //  generate Wave Track Strips
  si = stripList.begin();
  for (; si != stripList.end(); ++si) {
    if ((*si)->getTrack()->type() == MusECore::Track::WAVE)
      addStripToLayoutIfVisible(*si);
  }

  //  generate Midi channel/port Strips
  si = stripList.begin();
  for (; si != stripList.end(); ++si) {
    if ((*si)->getTrack()->type() == MusECore::Track::MIDI ||
        (*si)->getTrack()->type() == MusECore::Track::DRUM ||
        (*si)->getTrack()->type() == MusECore::Track::NEW_DRUM)
      addStripToLayoutIfVisible(*si);
  }

  //  Groups
  si = stripList.begin();
  for (; si != stripList.end(); ++si) {
    if ((*si)->getTrack()->type() == MusECore::Track::AUDIO_GROUP)
      addStripToLayoutIfVisible(*si);
  }

  //  Aux
  si = stripList.begin();
  for (; si != stripList.end(); ++si) {
    if ((*si)->getTrack()->type() == MusECore::Track::AUDIO_AUX)
      addStripToLayoutIfVisible(*si);
  }

  //    Master
  si = stripList.begin();
  for (; si != stripList.end(); ++si) {
    if ((*si)->getTrack()->type() == MusECore::Track::AUDIO_OUTPUT)
      addStripToLayoutIfVisible(*si);
  }

}

/*
bool AudioMixerApp::event(QEvent* event)
{
  printf("AudioMixerApp::event type:%d\n", event->type());   
  
  // Let it do the layout now, before we emit.
  QMainWindow::event(event);
  
  if(event->type() == QEvent::LayoutRequest)       
    emit layoutRequest();
         
  return false;       
}
*/

void AudioMixerApp::setSizing()
{
      int w = 0;
      
      w = mixerLayout->minimumSize().width();
      
      if(const QStyle* st = style())
      {
        st = st->proxy();
        w += 2 * st->pixelMetric(QStyle::PM_DefaultFrameWidth);
      }
      
      if(w < 40)
        w = 40;
//       setMaximumWidth(w);   
      view->setUpdatesEnabled(false);
      setUpdatesEnabled(false);
      if(stripList.size() <= 6)
//         view->setMinimumWidth(w);
        setMinimumWidth(w);
        
      setMaximumWidth(w);
//       view->setMaximumWidth(w);      

      setUpdatesEnabled(true);
      view->setUpdatesEnabled(true);

}

//---------------------------------------------------------
//   addStrip
//---------------------------------------------------------

void AudioMixerApp::addStrip(MusECore::Track* t, bool visible)
{
    if (DEBUG_MIXER)
      printf("addStrip\n");
    Strip* strip;
    if (t->isMidiTrack())
          strip = new MidiStrip(central, (MusECore::MidiTrack*)t, true);
    else
          strip = new AudioStrip(central, (MusECore::AudioTrack*)t, true);

    if (DEBUG_MIXER)
      printf ("putting new strip [%s] at end\n", t->name().toLatin1().data());
    stripList.append(strip);
    strip->setVisible(visible);
    strip->setStripVisible(visible);
}

//---------------------------------------------------------
//   clearAndDelete
//---------------------------------------------------------

void AudioMixerApp::clearAndDelete()
{
  if (DEBUG_MIXER)
    printf("clearAndDelete\n");
  StripList::iterator si = stripList.begin();
  for (; si != stripList.end(); ++si)
  {
    mixerLayout->removeWidget(*si);
    //(*si)->deleteLater();
    delete (*si);
  }

  stripList.clear();
  cfg->stripOrder.clear();
  oldAuxsSize = -1;
}

//---------------------------------------------------------
//   initMixer
//---------------------------------------------------------

void AudioMixerApp::initMixer()
{
  if (DEBUG_MIXER)
    printf("initMixer %d\n", cfg->stripOrder.size());
  setWindowTitle(cfg->name);
  //clearAndDelete();

  showMidiTracksId->setChecked(cfg->showMidiTracks);
  showDrumTracksId->setChecked(cfg->showDrumTracks);
  showNewDrumTracksId->setChecked(cfg->showNewDrumTracks);
  showInputTracksId->setChecked(cfg->showInputTracks);
  showOutputTracksId->setChecked(cfg->showOutputTracks);
  showWaveTracksId->setChecked(cfg->showWaveTracks);
  showGroupTracksId->setChecked(cfg->showGroupTracks);
  showAuxTracksId->setChecked(cfg->showAuxTracks);
  showSyntiTracksId->setChecked(cfg->showSyntiTracks);

  int auxsSize = MusEGlobal::song->auxs()->size();
  oldAuxsSize = auxsSize;
  MusECore::TrackList *tl = MusEGlobal::song->tracks();
  MusECore::TrackList::iterator tli = tl->begin();

  if (cfg->stripOrder.size() > 0) {
    for (int i=0; i < cfg->stripOrder.size(); i++) {
      MusECore::TrackList::iterator tli = tl->begin();
      if (DEBUG_MIXER)
        printf ("processing strip [%s][%d]\n", cfg->stripOrder.at(i).toLatin1().data(), cfg->stripVisibility.at(i));
      for (;tli != tl->end(); tli++) {
        if ((*tli)->name() == cfg->stripOrder.at(i)) {
          addStrip(*tli, cfg->stripVisibility.at(i));
          break;
        }
      }
    }
  }
  else {
    for (;tli != tl->end(); tli++) {
      addStrip(*tli);
    }
  }
}

//---------------------------------------------------------
//   configChanged
//---------------------------------------------------------

void AudioMixerApp::configChanged()    
{ 
  StripList::iterator si = stripList.begin();  // Catch when fonts change, viewable tracks, etc. No full rebuild.  p4.0.45
  for (; si != stripList.end(); ++si) 
        (*si)->configChanged();
}

//---------------------------------------------------------
//   updateStripList
//---------------------------------------------------------

void AudioMixerApp::updateStripList()
{
  if (DEBUG_MIXER)
    printf("updateStripList stripList %d tracks %zd\n", stripList.size(), MusEGlobal::song->tracks()->size());
  
  if (stripList.size() == 0 && cfg->stripOrder.size() > 0) {
      return initMixer();
  }
      
  MusECore::TrackList *tl = MusEGlobal::song->tracks();
  // check for superfluous strips
  StripList::iterator si = stripList.begin();
  for (; si != stripList.end(); ++si) {
    MusECore::TrackList::iterator tli = tl->begin();
    bool found = false;
    for (; tli != tl->end();++tli) {
      if ((*si)->getTrack() == (*tli)) {
        found = true;
        break;
      }
    }
    if (!found) {
      if (DEBUG_MIXER)
        printf("Did not find track for strip %s - Removing\n", (*si)->getLabelText().toLatin1().data());
      (*si)->deleteLater();
      si = stripList.erase(si);
    }
  }

  // check for new tracks
  MusECore::TrackList::iterator tli = tl->begin();
  for (; tli != tl->end();++tli) {
    bool found = false;
    StripList::iterator si = stripList.begin();
    for (; si != stripList.end(); ++si) {
      if ((*si)->getTrack() == (*tli)) {
        found = true;
        break;
      }
    }
    if (!found) {
      if (DEBUG_MIXER)
        printf("Did not find strip for track %s - Adding\n", (*tli)->name().toLatin1().data());
      addStrip((*tli)); // TODO: be intelligent about where strip is inserted
    }
  }
}

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void AudioMixerApp::songChanged(MusECore::SongChangedFlags_t flags)
{
  if (DEBUG_MIXER)
    printf("AudioMixerApp::songChanged %llX\n", (long long)flags);
  // Is it simply a midi controller value adjustment? Forget it.
  if(flags == SC_MIDI_CONTROLLER)
    return;

  UpdateAction action = NO_UPDATE;

  if (flags == -1) {
        action = UPDATE_ALL;
  }
  else if (flags & SC_TRACK_REMOVED) {
        action = STRIP_REMOVED;
        updateStripList();
  }
  else if (flags & SC_TRACK_INSERTED) {
        action = STRIP_INSERTED;
        updateStripList();
  }
  else if (flags & SC_MIDI_TRACK_PROP) {
        action = UPDATE_MIDI;
  }

  if (DEBUG_MIXER)
    printf("songChanged action = %d\n", action);
  redrawMixer();

  if (action != UPDATE_ALL)
  {

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
            routingDialog = new MusEGui::RouteDialog(this);
            connect(routingDialog, SIGNAL(closed()), SLOT(routingDialogClosed()));
            }
      if (routingDialog)
            routingDialog->setVisible(on);
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
//   show hide track groups
//---------------------------------------------------------

void AudioMixerApp::showMidiTracksChanged(bool v)
{      cfg->showMidiTracks = v;}
void AudioMixerApp::showDrumTracksChanged(bool v)
{      cfg->showDrumTracks = v;}
void AudioMixerApp::showNewDrumTracksChanged(bool v)
{      cfg->showNewDrumTracks = v;}
void AudioMixerApp::showWaveTracksChanged(bool v)
{      cfg->showWaveTracks = v;}
void AudioMixerApp::showInputTracksChanged(bool v)
{      cfg->showInputTracks = v;}
void AudioMixerApp::showOutputTracksChanged(bool v)
{      cfg->showOutputTracks = v;}
void AudioMixerApp::showGroupTracksChanged(bool v)
{      cfg->showGroupTracks = v;}
void AudioMixerApp::showAuxTracksChanged(bool v)
{      cfg->showAuxTracks = v;}
void AudioMixerApp::showSyntiTracksChanged(bool v)
{      cfg->showSyntiTracks = v; }

//---------------------------------------------------------
//   mouse events
//---------------------------------------------------------

void AudioMixerApp::mousePressEvent(QMouseEvent* ev)
{
  if (DEBUG_MIXER)
    printf("mixer mouse press event! %d\n", (int)ev->button());
  mixerClicked = true;
  QMainWindow::mousePressEvent(ev);
}
void AudioMixerApp::mouseReleaseEvent(QMouseEvent* ev)
{
  if (DEBUG_MIXER)
    printf("mixer mouse release event! %d\n",(int)ev->button());
  mixerClicked = false;
  QMainWindow::mouseReleaseEvent(ev);
}


//---------------------------------------------------------
//   write
//---------------------------------------------------------

void AudioMixerApp::write(int level, MusECore::Xml& xml)
{
  if (DEBUG_MIXER)
    printf("AudioMixerApp:;write\n");
  xml.tag(level++, "Mixer");

  xml.strTag(level, "name", cfg->name);

  xml.qrectTag(level, "geometry", geometry());

  xml.intTag(level, "showMidiTracks",   cfg->showMidiTracks);
  xml.intTag(level, "showDrumTracks",   cfg->showDrumTracks);
  xml.intTag(level, "showNewDrumTracks",   cfg->showNewDrumTracks);
  xml.intTag(level, "showInputTracks",  cfg->showInputTracks);
  xml.intTag(level, "showOutputTracks", cfg->showOutputTracks);
  xml.intTag(level, "showWaveTracks",   cfg->showWaveTracks);
  xml.intTag(level, "showGroupTracks",  cfg->showGroupTracks);
  xml.intTag(level, "showAuxTracks",    cfg->showAuxTracks);
  xml.intTag(level, "showSyntiTracks",  cfg->showSyntiTracks);

  xml.intTag(level, "displayOrder", cfg->displayOrder);

  // specific to store made to song file - this is not part of MixerConfig::write
  StripList::iterator si = stripList.begin();
  for (; si != stripList.end(); ++si) {
    xml.strTag(level, "StripName", (*si)->getTrack()->name());
    xml.intTag(level, "StripVisible", (*si)->getStripVisible());
  }

  xml.etag(level, "Mixer");
  }

} // namespace MusEGui
