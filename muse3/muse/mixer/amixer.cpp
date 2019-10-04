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
#include "muse_math.h"

#include <QApplication>
#include <QMenuBar>
#include <QResizeEvent>
#include <QPaintEvent>
#include <QHBoxLayout>
#include <QCloseEvent>
#include <QMenu>
#include <QActionGroup>
#include <QAction>

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

// For debugging output: Uncomment the fprintf section.
#define DEBUG_MIXER(dev, format, args...)  // fprintf(dev, format, ##args);

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
  DEBUG_MIXER(stderr, "viewportEvent type %d\n", (int)event->type());
//   event->ignore();
  // Let it do the layout now, before we emit.
  const bool res = QScrollArea::viewportEvent(event);
  
  if(event->type() == QEvent::LayoutRequest)       
    emit layoutRequest();
         
//   return false;
//   return true;
  return res;
}

//---------------------------------------------------------
//   AudioMixer
//
//    inputs | synthis | tracks | groups | master
//---------------------------------------------------------

AudioMixerApp::AudioMixerApp(QWidget* parent, MusEGlobal::MixerConfig* c)
   : QMainWindow(parent)
{
      _preferKnobs = MusEGlobal::config.preferKnobsVsSliders;
      cfg = c;
      oldAuxsSize = 0;
      routingDialog = 0;
      setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Expanding));   // TESTING Tim
      setWindowTitle(cfg->name);
      setWindowIcon(*museIcon);

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

      showDrumTracksId->setVisible(false);
      
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
      view->setFocusPolicy(Qt::NoFocus);
      view->setContentsMargins(0, 0, 0, 0);
      //view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
      setCentralWidget(view);
      
      central = new QWidget(view);
      central->setFocusPolicy(Qt::NoFocus);
      central->setContentsMargins(0, 0, 0, 0);
      //splitter = new QSplitter(view);
      mixerLayout = new QHBoxLayout();
      central->setLayout(mixerLayout);
      mixerLayout->setSpacing(0);
      mixerLayout->setContentsMargins(0, 0, 0, 0);
      view->setWidget(central);
      //view->setWidget(splitter);
      view->setWidgetResizable(true);
      
      connect(view, SIGNAL(layoutRequest()), SLOT(setSizing()));  
      
      connect(MusEGlobal::song, SIGNAL(songChanged(MusECore::SongChangedStruct_t)), SLOT(songChanged(MusECore::SongChangedStruct_t)));
      connect(MusEGlobal::muse, SIGNAL(configChanged()), SLOT(configChanged()));
      
      initMixer();
      redrawMixer();

      central->installEventFilter(this);
      mixerLayout->installEventFilter(this);
      view ->installEventFilter(this);

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
    act = menuStrips->addAction(tr("(no hidden strips)"));
    act->setData(UNHANDLED_NUMBER);
  }
}

void AudioMixerApp::handleMenu(QAction *act)
{
  DEBUG_MIXER(stderr, "handleMenu %d\n", act->data().toInt());
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
    case MusECore::Track::AUDIO_INPUT:
      if (!cfg->showInputTracks)
        return false;
      break;
    case MusECore::Track::AUDIO_OUTPUT:
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
  DEBUG_MIXER(stderr, "redrawMixer type %d, mixerLayout count %d\n", cfg->displayOrder, mixerLayout->count());
  // empty layout
  while (mixerLayout->count() > 0) {
    mixerLayout->removeItem(mixerLayout->itemAt(0));
  }

  switch (cfg->displayOrder) {
    case MusEGlobal::MixerConfig::STRIPS_ARRANGER_VIEW:
      {
        DEBUG_MIXER(stderr, "Draw strips with arranger view\n");
        MusECore::TrackList *tl = MusEGlobal::song->tracks();
        MusECore::TrackList::iterator tli = tl->begin();
        for (; tli != tl->end(); tli++) {
          DEBUG_MIXER(stderr, "Adding strip %s\n", (*tli)->name().toLatin1().data());
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
        DEBUG_MIXER(stderr, "Draw strips with edited view\n");
        // add them back in the selected order
        StripList::iterator si = stripList.begin();
        for (; si != stripList.end(); ++si) {
            DEBUG_MIXER(stderr, "Adding strip %s\n", (*si)->getTrack()->name().toLatin1().data());
            addStripToLayoutIfVisible(*si);
        }
        DEBUG_MIXER(stderr, "mixerLayout count is now %d\n", mixerLayout->count());
      }
      break;
    case MusEGlobal::MixerConfig::STRIPS_TRADITIONAL_VIEW:
      {
        DEBUG_MIXER(stderr, "TRADITIONAL VIEW mixerLayout count is now %d\n", mixerLayout->count());
        addStripsTraditionalLayout();
      }

      break;
  }

  // Setup all tabbing for each strip, and between strips.
  setupComponentTabbing();
  
  update();
}

Strip* AudioMixerApp::findStripForTrack(StripList &sl, MusECore::Track *t)
{
  StripList::iterator si = sl.begin();
  for (;si != sl.end(); si++) {
    if ((*si)->getTrack() == t)
      return *si;
  }
  DEBUG_MIXER(stderr, "AudioMixerApp::findStripForTrack - ERROR: there was no strip for this track!\n");
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
  DEBUG_MIXER(stderr, "Recreate stripList\n");
  if (cfg->displayOrder == MusEGlobal::MixerConfig::STRIPS_ARRANGER_VIEW) {

    const int tracks_sz = MusEGlobal::song->tracks()->size();
    for (int i=0; i< stripList.size(); i++)
    {
      Strip *s2 = stripList.at(i);
      if (s2 == s) continue;

      DEBUG_MIXER(stderr, "loop loop %d %d width %d\n", s->pos().x(),s2->pos().x(), s2->width());
      if (s->pos().x()+s->width()/2 < s2->pos().x()+s2->width() // upper limit
          && s->pos().x()+s->width()/2 > s2->pos().x() ) // lower limit
      {
        // found relevant pos.
        int sTrack = MusEGlobal::song->tracks()->index(s->getTrack());
        int dTrack = MusEGlobal::song->tracks()->index(s2->getTrack());
        if (sTrack >= 0 && dTrack >= 0)   // sanity check
        {
          if (sTrack < tracks_sz && dTrack < tracks_sz)   // sanity check
            MusEGlobal::song->applyOperation(MusECore::UndoOp(MusECore::UndoOp::MoveTrack, sTrack, dTrack));
        }
      }
    }

  } else if (cfg->displayOrder == MusEGlobal::MixerConfig::STRIPS_TRADITIONAL_VIEW)
  {
    fillStripListTraditional();
    cfg->displayOrder = MusEGlobal::MixerConfig::STRIPS_EDITED_VIEW;
  }
  DEBUG_MIXER(stderr, "moveStrip %s! stripList.size = %d\n", s->getLabelText().toLatin1().data(), stripList.size());

  for (int i=0; i< stripList.size(); i++)
  {
    Strip *s2 = stripList.at(i);
    if (s2 == s) continue;

    DEBUG_MIXER(stderr, "loop loop %d %d width %d\n", s->pos().x(),s2->pos().x(), s2->width());
    if (s->pos().x()+s->width()/2 < s2->pos().x()+s2->width() // upper limit
        && s->pos().x()+s->width()/2 > s2->pos().x() ) // lower limit
    {
      DEBUG_MIXER(stderr, "got new pos: %d\n", i);
#if DEBUG_MIXER
      bool isSuccess = stripList.removeOne(s);
      DEBUG_MIXER(stderr, "Removed strip %d", isSuccess);
#else
      stripList.removeOne(s);
#endif
      stripList.insert(i,s);
      DEBUG_MIXER(stderr, "Inserted strip at %d", i);
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
  DEBUG_MIXER(stderr, "AudioMixerApp::event type:%d\n", event->type());
  
  // Let it do the layout now, before we emit.
  QMainWindow::event(event);
  
  if(event->type() == QEvent::LayoutRequest)       
    emit layoutRequest();
         
  return false;       
}
*/

void AudioMixerApp::setSizing()
{
  DEBUG_MIXER(stderr, "setSizing\n");
      int w = 0;
      
      w = mixerLayout->minimumSize().width();
      
      if(const QStyle* st = style())
      {
        st = st->proxy();
        w += 2 * st->pixelMetric(QStyle::PM_DefaultFrameWidth);
      }
      
      if(w < 40)
        w = 40;
      view->setUpdatesEnabled(false);
      setUpdatesEnabled(false);
      if(stripList.size() <= 6)
        setMinimumWidth(w);
        
      setMaximumWidth(w);

      setUpdatesEnabled(true);
      view->setUpdatesEnabled(true);

}

//---------------------------------------------------------
//   addStrip
//---------------------------------------------------------

void AudioMixerApp::addStrip(MusECore::Track* t, bool visible)
{
    DEBUG_MIXER(stderr, "addStrip\n");
    Strip* strip;

    // Make them non-embedded: Moveable, hideable, and with an expander handle.
    if (t->isMidiTrack())
          strip = new MidiStrip(central, (MusECore::MidiTrack*)t, true, false);
    else
          strip = new AudioStrip(central, (MusECore::AudioTrack*)t, true, false);

    // Broadcast changes to other selected tracks.
    strip->setBroadcastChanges(true);

    // Set focus yielding to the mixer window.
    if(MusEGlobal::config.smartFocus)
    {
      strip->setFocusYieldWidget(this);
      //strip->setFocusPolicy(Qt::WheelFocus);
    }

    connect(strip, SIGNAL(clearStripSelection()),this,SLOT(clearStripSelection()));
    connect(strip, SIGNAL(moveStrip(Strip*)),this,SLOT(moveStrip(Strip*)));

    DEBUG_MIXER(stderr, "putting new strip [%s] at end\n", t->name().toLatin1().data());
    stripList.append(strip);
    strip->setVisible(visible);
    strip->setStripVisible(visible);
}

//---------------------------------------------------------
//   clearAndDelete
//---------------------------------------------------------

void AudioMixerApp::clearAndDelete()
{
  DEBUG_MIXER(stderr, "clearAndDelete\n");
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
  DEBUG_MIXER(stderr, "initMixer %d\n", cfg->stripOrder.size());
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
      DEBUG_MIXER(stderr, "processing strip [%s][%d]\n", cfg->stripOrder.at(i).toLatin1().data(), cfg->stripVisibility.at(i));
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
  DEBUG_MIXER(stderr, "configChanged\n");
  StripList::iterator si = stripList.begin();  // Catch when fonts change, viewable tracks, etc. No full rebuild.  p4.0.45
  for (; si != stripList.end(); ++si) 
        (*si)->configChanged();
  
  // Detect when knobs are preferred.
  // TODO: Later if added: Detect when a rack component is added or removed. 
  //       Or other stuff requiring this retabbing.
  if(_preferKnobs != MusEGlobal::config.preferKnobsVsSliders)
  {
    _preferKnobs = MusEGlobal::config.preferKnobsVsSliders;
    // Now set up all tabbing on all strips.
    setupComponentTabbing();
  }
}

//---------------------------------------------------------
//   updateStripList
//---------------------------------------------------------

void AudioMixerApp::updateStripList()
{
  DEBUG_MIXER(stderr, "updateStripList stripList %d tracks %zd\n", stripList.size(), MusEGlobal::song->tracks()->size());
  
  if (stripList.size() == 0 && cfg->stripOrder.size() > 0) {
      return initMixer();
  }
      
  MusECore::TrackList *tl = MusEGlobal::song->tracks();
  // check for superfluous strips
  for (StripList::iterator si = stripList.begin(); si != stripList.end(); ) {
    MusECore::TrackList::iterator tli = tl->begin();
    bool found = false;
    for (; tli != tl->end();++tli) {
      if ((*si)->getTrack() == (*tli)) {
        found = true;
        break;
      }
    }
    if (!found) {
      DEBUG_MIXER(stderr, "Did not find track for strip %s - Removing\n", (*si)->getLabelText().toLatin1().data());
      //(*si)->deleteLater();
      delete (*si);
      si = stripList.erase(si);
    }
    else
      ++si;
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
      DEBUG_MIXER(stderr, "Did not find strip for track %s - Adding\n", (*tli)->name().toLatin1().data());
      addStrip((*tli)); // TODO: be intelligent about where strip is inserted
    }
  }
}

void AudioMixerApp::updateSelectedStrips()
{
  foreach(Strip *s, stripList)
  {
    if(MusECore::Track* t = s->getTrack())
    {
      if(s->isSelected() != t->selected())
        s->setSelected(t->selected());
    }
  }
}

QWidget* AudioMixerApp::setupComponentTabbing(QWidget* previousWidget)
{
  QWidget* prev = previousWidget;
  QLayoutItem* li;
  QWidget* widget;
  Strip* strip;
  const int cnt = mixerLayout->count();
  for(int i = 0; i < cnt; ++i)
  {
    li = mixerLayout->itemAt(i);
    if(!li)
      continue;
    widget = li->widget();
    if(!widget)
      continue;
    strip = qobject_cast<Strip*>(widget);
    if(!strip)
      continue;
    prev = strip->setupComponentTabbing(prev);
  }
  return prev;
}

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void AudioMixerApp::songChanged(MusECore::SongChangedStruct_t flags)
{
  DEBUG_MIXER(stderr, "AudioMixerApp::songChanged %llX\n", (long long)flags._flags);
  if (flags & SC_TRACK_REMOVED) {
        updateStripList();
  }
  else if (flags & SC_TRACK_INSERTED) {
        updateStripList();
  }
  DEBUG_MIXER(stderr, "songChanged action = %ld\n", (long int)flags._flags);
    
  
  // This costly to do every time. Try to filter it according to required flags.
  // The only flags which would require a redraw, which is very costly, are the following:
  if (flags & (SC_TRACK_REMOVED | SC_TRACK_INSERTED | SC_TRACK_MOVED
               //| SC_CHANNELS   // Channels due to one/two meters and peak labels can change the strip width.
               //| SC_AUX
              ))
  redrawMixer();

  StripList::iterator si = stripList.begin();
  for (; si != stripList.end(); ++si) {
        (*si)->songChanged(flags);
        }

  if(flags & SC_TRACK_SELECTION)
    updateSelectedStrips();
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
{
    cfg->showMidiTracks = v;
    redrawMixer();
}
void AudioMixerApp::showDrumTracksChanged(bool v)
{
    cfg->showDrumTracks = v;
    redrawMixer();
}
void AudioMixerApp::showNewDrumTracksChanged(bool v)
{
    cfg->showNewDrumTracks = v;
    redrawMixer();
}
void AudioMixerApp::showWaveTracksChanged(bool v)
{
    cfg->showWaveTracks = v;
    redrawMixer();
}
void AudioMixerApp::showInputTracksChanged(bool v)
{
    cfg->showInputTracks = v;
    redrawMixer();
}
void AudioMixerApp::showOutputTracksChanged(bool v)
{
    cfg->showOutputTracks = v;
    redrawMixer();
}
void AudioMixerApp::showGroupTracksChanged(bool v)
{
    cfg->showGroupTracks = v;
    redrawMixer();
}
void AudioMixerApp::showAuxTracksChanged(bool v)
{
    cfg->showAuxTracks = v;
    redrawMixer();
}
void AudioMixerApp::showSyntiTracksChanged(bool v)
{
    cfg->showSyntiTracks = v;
    redrawMixer();
}

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void AudioMixerApp::write(int level, MusECore::Xml& xml)
{
  DEBUG_MIXER(stderr, "AudioMixerApp:;write\n");
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

void AudioMixerApp::keyPressEvent(QKeyEvent *ev)
{
  bool moveEnabled=false;
  const bool shift = ev->modifiers() & Qt::ShiftModifier;
  const bool alt = ev->modifiers() & Qt::AltModifier;
  const bool ctl = ev->modifiers() & Qt::ControlModifier;
  if (ctl && alt) {
    moveEnabled=true;
  }

  switch (ev->key()) {
    case Qt::Key_Left:
      if (moveEnabled)
      {
        selectNextStrip(false, !shift);
        ev->accept();
        return;
      }
      break;

    case Qt::Key_Right:
      if (moveEnabled)
      {
        selectNextStrip(true, !shift);
        ev->accept();
        return;
      }
      break;

    default:
      break;
  }

  ev->ignore();
  return QMainWindow::keyPressEvent(ev);
}

void AudioMixerApp::clearStripSelection()
{
  foreach (Strip *s, stripList)
    s->setSelected(false);
}

void AudioMixerApp::selectNextStrip(bool isRight, bool /*clearAll*/)
{
  Strip *prev = NULL;

  for (int i = 0; i < mixerLayout->count(); i++)
  {
    QWidget *w = mixerLayout->itemAt(i)->widget();
    if (w)
    {
      if (prev && !prev->isEmbedded() && prev->isSelected() && isRight) // got it
      {
        Strip* st = static_cast<Strip*>(w);
        //if(clearAll)  // TODO
        {
          MusEGlobal::song->selectAllTracks(false);
          clearStripSelection();
        }
        st->setSelected(true);
        if(st->getTrack())
          st->getTrack()->setSelected(true);
        MusEGlobal::song->update(SC_TRACK_SELECTION);
        return;
      }
      else if( !static_cast<Strip*>(w)->isEmbedded() && static_cast<Strip*>(w)->isSelected() && prev && !prev->isEmbedded() && !isRight)
      {
        //if(clearAll) // TODO
        {
          MusEGlobal::song->selectAllTracks(false);
          clearStripSelection();
        }
        prev->setSelected(true);
        if(prev->getTrack())
          prev->getTrack()->setSelected(true);
        MusEGlobal::song->update(SC_TRACK_SELECTION);
        return;
      }
      else {
        prev = static_cast<Strip*>(w);
      }
    }
  }

  QWidget *w;
  if (isRight)
    w = mixerLayout->itemAt(0)->widget();
  else
    w = mixerLayout->itemAt(mixerLayout->count()-1)->widget();
  Strip* st = static_cast<Strip*>(w);
  if(st && !st->isEmbedded())
  {
    //if(clearAll) // TODO
    {
      MusEGlobal::song->selectAllTracks(false);
      clearStripSelection();
    }
    st->setSelected(true);
    if(st->getTrack())
      st->getTrack()->setSelected(true);
    MusEGlobal::song->update(SC_TRACK_SELECTION);
  }
}

bool AudioMixerApp::eventFilter(QObject *obj,
                             QEvent *event)
{
  DEBUG_MIXER(stderr, "eventFilter type %d\n", (int)event->type());
    QKeyEvent *keyEvent = NULL;//event data, if this is a keystroke event
    bool result = false;//return true to consume the keystroke

    if (event->type() == QEvent::KeyPress)
    {
         keyEvent = dynamic_cast<QKeyEvent*>(event);
         this->keyPressEvent(keyEvent);
         result = true;
    }//if type()

    else if (event->type() == QEvent::KeyRelease)
    {
        keyEvent = dynamic_cast<QKeyEvent*>(event);
        this->keyReleaseEvent(keyEvent);
        result = true;
    }//else if type()

    //### Standard event processing ###
    else
        result = QObject::eventFilter(obj, event);

    return result;
}//eventFilter


} // namespace MusEGui
