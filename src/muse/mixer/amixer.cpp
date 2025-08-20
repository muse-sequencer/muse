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

#include "muse_math.h"

#include <QMenuBar>
#include <QPaintEvent>
#include <QSpacerItem>
#include <QUuid>
#include <QInputDialog>
#include <QMessageBox>

#include "amixer.h"
#include "app.h"
#include "helper.h"
#include "icons.h"
#include "song.h"
#include "audio.h"
#include "astrip.h"
#include "mstrip.h"
#include "shortcuts.h"
#include "globals.h"
#include "undo.h"
#include "menutitleitem.h"

// Forwards from header:
#include <QWidget>
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QHBoxLayout>
#include <QEvent>
#include "track.h"
#include "combobox.h"
#include "knob.h"
#include "slider.h"
#include "strip.h"
#include "components/routedialog.h"

//#define __WIDTH_COMPENSATION 4

// For debugging output: Uncomment the fprintf section.
#define DEBUG_MIXER(dev, format, args...)  // fprintf(dev, format, ##args);

namespace MusEGui {

ScrollArea::ScrollArea(QWidget* parent) : QScrollArea(parent)
{

}

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

AudioMixerApp::AudioMixerApp(QWidget* parent, MusEGlobal::MixerConfig* c, bool docked)
   : QMainWindow(parent)
{
      _resizeFlag = false;
      _preferKnobs = MusEGlobal::config.preferKnobsVsSliders;
      _docked = docked;
      cfg = c;
      oldAuxsSize = 0;
      routingDialog = nullptr;
      setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Expanding));
      setWindowTitle(cfg->name);
      setWindowIcon(*museIcon);
      
      // Due to issues with certain window managers, such as maximize IGNORING
      //  our maximum size imposed on the window, maximizing is disabled.
      // Several attempts to allow maximizing were unsuccessful.
      // The window is full screen width but the scroll area is cut off at our maximum width.
      // Seems internally some layer IS respecting our maximum width imposed, cutting it off.
      // Tried end spacers, blank widgets autoFillBackground etc. No luck.
      // Might be possible but more work needed. Easier to do this:
      setWindowFlags(windowFlags() & ~Qt::WindowFlags(Qt::WindowMaximizeButtonHint));

      //cfg->displayOrder = MusEGlobal::MixerConfig::STRIPS_TRADITIONAL_VIEW;

      menuConfig = menuBar()->addMenu(tr("&Create"));
      connect(menuConfig, &QMenu::aboutToShow, [=]() { menuConfig->clear(); MusEGui::populateAddTrack(menuConfig, true); } );
      connect(menuConfig, &QMenu::triggered, [](QAction* a) { MusEGlobal::song->addNewTrack(a); } );

      menuView = menuBar()->addMenu(tr("&View"));
      menuStrips = menuView->addMenu(tr("Strips"));
      connect(menuStrips, &QMenu::aboutToShow, [this]() { stripsMenu(); } );
      connect(menuStrips, &QMenu::triggered, [this](QAction* a) { handleMenu(a); } );

      connect(menuView, &QMenu::aboutToShow, [=]() { menuViewAboutToShow(); } );

      menuView->addSeparator();

      QActionGroup* actionItems = new QActionGroup(this);
      actionItems->setExclusive(false);
      connect(actionItems, &QActionGroup::triggered, [this](QAction* a) { menuViewGroupTriggered(a); } );

      routingId = new QAction(tr("Advanced Router..."), actionItems);
      routingId->setData(ADVANCED_ROUTER);
      routingId->setCheckable(true);
      routingId->setIcon(*routerSVGIcon);

      showMidiTracksId = new QAction(tr("Show Midi Tracks"), actionItems);
//      showDrumTracksId = new QAction(tr("Show Drum Tracks"), actionItems);
      showNewDrumTracksId = new QAction(tr("Show Drum Tracks"), actionItems);
      showWaveTracksId = new QAction(tr("Show Wave Tracks"), actionItems);

      QAction *separator = new QAction(this);
      separator->setSeparator(true);
      actionItems->addAction(separator);

      showInputTracksId = new QAction(tr("Show Inputs"), actionItems);
      showOutputTracksId = new QAction(tr("Show Outputs"), actionItems);
      showGroupTracksId = new QAction(tr("Show Groups"), actionItems);
      showAuxTracksId = new QAction(tr("Show Auxs"), actionItems);
      showSyntiTracksId = new QAction(tr("Show Synthesizers"), actionItems);

//      showDrumTracksId->setVisible(false);
      
      showMidiTracksId->setCheckable(true);
//      showDrumTracksId->setCheckable(true);
      showNewDrumTracksId->setCheckable(true);
      showWaveTracksId->setCheckable(true);
      showInputTracksId->setCheckable(true);
      showOutputTracksId->setCheckable(true);
      showGroupTracksId->setCheckable(true);
      showAuxTracksId->setCheckable(true);
      showSyntiTracksId->setCheckable(true);

      showMidiTracksId->setData(SHOW_MIDI_TRACKS);
      showNewDrumTracksId->setData(SHOW_DRUM_TRACKS);
      showWaveTracksId->setData(SHOW_WAVE_TRACKS);
      showInputTracksId->setData(SHOW_INPUT_TRACKS);
      showOutputTracksId->setData(SHOW_OUTPUT_TRACKS);
      showGroupTracksId->setData(SHOW_GROUP_TRACKS);
      showAuxTracksId->setData(SHOW_AUX_TRACKS);
      showSyntiTracksId->setData(SHOW_SYNTH_TRACKS);

      actionItems->addAction(new MenuTitleItem(tr("Configuration"), this));

      knobsVsSlidersId = new QAction(tr("Prefer Knobs, Not Sliders"), actionItems);
      knobsVsSlidersId->setData(KNOBS_VS_SLIDERS);
      knobsVsSlidersId->setCheckable(true);

      showValuesId = new QAction(tr("Show Values in Controls"), actionItems);
      showValuesId->setData(SHOW_VALUES_IN_CONTROLS);
      showValuesId->setCheckable(true);

      showMidiDbsId = new QAction(tr("Prefer Midi Volume As Decibels"), actionItems);
      showMidiDbsId->setData(SHOW_MIDI_DBS);
      showMidiDbsId->setCheckable(true);

      monOnRecArmId = new QAction(tr("Monitor on Record-arm Automatically"), actionItems);
      monOnRecArmId->setData(MON_ON_REC_ARM);
      monOnRecArmId->setCheckable(true);

      momentaryMuteId = new QAction(tr("Momentary Mute"), actionItems);
      momentaryMuteId->setData(MOMENTARY_MUTE);
      momentaryMuteId->setCheckable(true);

      momentarySoloId = new QAction(tr("Momentary Solo"), actionItems);
      momentarySoloId->setData(MOMENTARY_SOLO);
      momentarySoloId->setCheckable(true);

      // Add the group actions so far.
      menuView->addActions(actionItems->actions());

      menuView->addSeparator();

      // Add the number of visible effects menu.
      menuAudEffRackVisibleItems = new QMenu(tr("Visible Audio Effects"));
      audEffRackVisibleGroup = new QActionGroup(this);
      audEffRackVisibleGroup->setExclusive(true);
      for(int i = 0; i <= MusECore::PipelineDepth; ++i)
      {
        QAction* act = new QAction(QString::number(i), audEffRackVisibleGroup);
        act->setData(int(AUD_EFF_RACK_VIS_ITEMS_BASE - i));
        act->setCheckable(true);
      }
      menuAudEffRackVisibleItems->addActions(audEffRackVisibleGroup->actions());
      connect(menuAudEffRackVisibleItems, &QMenu::aboutToShow, [=]() { menuAudEffRackVisItemsAboutToShow(); } );
      connect(audEffRackVisibleGroup, &QActionGroup::triggered, [this](QAction* a) { audEffRackVisItemsTriggered(a); } );
      menuView->addMenu(menuAudEffRackVisibleItems);

      // Add the "Actions" title.
      menuView->addAction(new MenuTitleItem(tr("Actions"), this));

      // Add the change track name action to the action group.
      changeTrackNameId = new QAction(tr("Change Track Name..."), actionItems);
      changeTrackNameId->setData(CHANGE_TRACK_NAME);
      changeTrackNameId->setCheckable(false);

      // Add the change track name action to the menu.
      menuView->addAction(changeTrackNameId);

      menuView->addActions(MusEGlobal::undoRedo->actions());

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

      // Ensure the rightmost item is always a spacer.
      // Although this does not have any effect because we disabled the maximize button, just in case
      //  the maximize button is ever re-added and/or the maximum width imposed on the mixer window
      //  is ever removed, keep this - it will be required. No harm so far in leaving it in.
      //      QSpacerItem* right_spacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding);
      //      mixerLayout->addSpacerItem(right_spacer);

      mixerLayout->addStretch(1);
      DEBUG_MIXER(stderr, "AudioMixerApp: mixerLayout count %d\n", mixerLayout->count());
      if (!_docked)
          connect(view, SIGNAL(layoutRequest()), SLOT(setSizing()));
      // FIXME: Neither of these two replacement functor version work. What's wrong here?
      //connect(view, &ScrollArea::layoutRequest, [this]() { setSizing(); } );
      //connect(view, QOverload<>::of(&ScrollArea::layoutRequest), [=]() { setSizing(); } );
      
      _songChangedMetaConn = connect(MusEGlobal::song, &MusECore::Song::songChanged, [this](MusECore::SongChangedStruct_t f) { songChanged(f); } );
      _configChangedMetaConn = connect(MusEGlobal::muse, &MusEGui::MusE::configChanged, [this]() { configChanged(); } );

      initMixer();
      updateStripList();
      updateSelectedStrips();
      redrawMixer();

      central->installEventFilter(this);
      mixerLayout->installEventFilter(this);
      view ->installEventFilter(this);
}

AudioMixerApp::~AudioMixerApp()
{
  disconnect(_songChangedMetaConn);
  disconnect(_configChangedMetaConn);
}

void AudioMixerApp::stripsMenu()
{
  menuStrips->clear();

  QAction *act;
  QActionGroup *ag = new QActionGroup(this);
  ag->setExclusive(true);

  act = ag->addAction(tr("Traditional Order"));
  act->setData(MusEGlobal::MixerConfig::STRIPS_TRADITIONAL_VIEW);
  act->setCheckable(true);
  if (cfg->displayOrder == MusEGlobal::MixerConfig::STRIPS_TRADITIONAL_VIEW)
    act->setChecked(true);

  act = ag->addAction(tr("Arranger Order"));
  act->setData(MusEGlobal::MixerConfig::STRIPS_ARRANGER_VIEW);
  act->setCheckable(true);
  if (cfg->displayOrder == MusEGlobal::MixerConfig::STRIPS_ARRANGER_VIEW)
    act->setChecked(true);

  act = ag->addAction(tr("User Order"));
  act->setData(MusEGlobal::MixerConfig::STRIPS_EDITED_VIEW);
  act->setCheckable(true);
  if (cfg->displayOrder == MusEGlobal::MixerConfig::STRIPS_EDITED_VIEW)
    act->setChecked(true);

  menuStrips->addActions(ag->actions());

  menuStrips->addSeparator();

  hideStripId = menuStrips->addAction(tr("Hide Selected Strips"));
  hideStripId->setData(HIDE_STRIPS);
  hideStripId->setEnabled(false);
  for(StripList::const_iterator isl = stripList.cbegin(); isl != stripList.cend(); ++isl)
  {
    const Strip* strip = *isl;
    if(strip && !strip->isEmbedded() && strip->isSelected())
    {
      hideStripId->setEnabled(true);
      break;
    }
  }

  act = menuStrips->addAction(tr("Show All Hidden Strips"));
  act->setData(UNHIDE_STRIPS);
  menuStrips->addSeparator();

  // loop through all tracks and show the hidden ones
  int i=0,h=0;
  foreach (Strip *s, stripList) {
    if (s && !s->getStripVisible()){
      act = menuStrips->addAction(tr("Unhide Strip: ") + s->getTrack()->name());
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
    Strip* s = stripList.at(act->data().toInt());
    if(s)
    {
      s->setStripVisible(true);
      stripVisibleChanged(s, true);
    }
  } else if (operation ==  UNHIDE_STRIPS) {
    foreach (Strip *s, stripList) {
      if(s && !s->isVisible())
      {
        s->setStripVisible(true);
        stripVisibleChanged(s, true);
      }
    }
  } else if (operation == HIDE_STRIPS) {
    foreach (Strip *s, stripList) {
      if(s && s->isSelected() && s->isVisible())
      {
        s->setStripVisible(false);
        stripVisibleChanged(s, false);
      }
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
  if (!s || !s->getStripVisible())
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
  const int lc = mixerLayout->count();
  if(lc > 0)
  {
    // Count down from second last item. Last item should be a spacer or blank widget.
    for (int i = lc - 2; i >= 0; --i)
    {
      // Remove only widgets from the layout, not spacers/stretchers etc.
      QLayoutItem* li = mixerLayout->itemAt(i);
      if(!li)
        continue;
      QWidget* w = li->widget();
      if(!w)
        continue;
      mixerLayout->takeAt(i);
    }
  }
  DEBUG_MIXER(stderr, "redrawMixer type %d, after emptying: mixerLayout count %d\n", cfg->displayOrder, mixerLayout->count());

  switch (cfg->displayOrder) {
    case MusEGlobal::MixerConfig::STRIPS_ARRANGER_VIEW:
      {
        DEBUG_MIXER(stderr, "Draw strips with arranger view\n");
        MusECore::TrackList *tl = MusEGlobal::song->tracks();
        MusECore::TrackList::iterator tli = tl->begin();
        for (; tli != tl->end(); tli++) {
          DEBUG_MIXER(stderr, "Adding strip %s\n", (*tli)->name().toLocal8Bit().data());
          StripList::iterator si = stripList.begin();
          for (; si != stripList.end(); si++) {
            Strip *s = *si;
            // s might be null due to calling stripList.resize() in addStrip()!
            if(s && s->getTrack() == *tli) {
              addStripToLayoutIfVisible(s);
            }
          }
        }
        DEBUG_MIXER(stderr, "redrawMixer after draw with arranger view: mixerLayout count:%d\n", mixerLayout->count());
      }
      break;
    case MusEGlobal::MixerConfig::STRIPS_EDITED_VIEW:
      {
        DEBUG_MIXER(stderr, "Draw strips with edited view\n");
        // add them back in the selected order
        StripList::iterator si = stripList.begin();
        for (; si != stripList.end(); ++si) {
            Strip *s = *si;
            // s might be null due to calling stripList.resize() in addStrip()!
            if(s)
            {
              DEBUG_MIXER(stderr, "Adding strip %s\n", s->getTrack()->name().toLocal8Bit().data());
              addStripToLayoutIfVisible(s);
            }
        }
        DEBUG_MIXER(stderr, "redrawMixer after draw with edited view: mixerLayout count:%d\n", mixerLayout->count());
      }
      break;
    case MusEGlobal::MixerConfig::STRIPS_TRADITIONAL_VIEW:
      {
        DEBUG_MIXER(stderr, "TRADITIONAL VIEW mixerLayout count is now %d\n", mixerLayout->count());
        addStripsTraditionalLayout();
        DEBUG_MIXER(stderr, "redrawMixer after draw with traditional view: mixerLayout count:%d\n", mixerLayout->count());
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
    Strip *s = *si;
    // s might be null due to calling stripList.resize() in addStrip()!
    if (s && s->getTrack() == t)
      return s;
  }
  DEBUG_MIXER(stderr, "AudioMixerApp::findStripForTrack - ERROR: there was no strip for this track!\n");
  return nullptr;
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
        (*tli)->type() == MusECore::Track::DRUM)
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
  if(!s)
    return;

  DEBUG_MIXER(stderr, "Recreate stripList\n");
  if (cfg->displayOrder == MusEGlobal::MixerConfig::STRIPS_ARRANGER_VIEW) {

    const int tracks_sz = MusEGlobal::song->tracks()->size();
    for (int i=0; i< stripList.size(); i++)
    {
      Strip *s2 = stripList.at(i);
      if (!s2 || s2 == s) continue;

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
  DEBUG_MIXER(stderr, "moveStrip %s! stripList.size = %d\n", s->getLabelText().toLocal8Bit().data(), stripList.size());

  for (int i=0; i< stripList.size(); i++)
  {
    Strip *s2 = stripList.at(i);
    // Ignore the given strip, or invisible strips.
    if(!s2 || s2 == s || !s2->getStripVisible()) continue;

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
      // Required for Qt6!
      if(i > stripList.size())
        stripList.resize(i + 1);
      stripList.insert(i,s);
      DEBUG_MIXER(stderr, "Inserted strip at %d", i);
      // Move the corresponding config list item.
      moveConfig(s, i);
      break;
    }
  }
  redrawMixer();
  update();
}

void AudioMixerApp::addStripToLayoutIfVisible(Strip *s)
{
  if(!s)
    return;

  if (stripIsVisible(s)) {
    s->setVisible(true);
    stripVisibleChanged(s, true);
    // Ensure that if a right spacer or blank widget item exits, we insert
    //  at that position, otherwise just append.
    const int lc = mixerLayout->count();
    if(lc == 0) {
      DEBUG_MIXER(stderr, "addStripToLayoutIfVisible: before addWidget(): mixerLayout count:%d\n", mixerLayout->count());
      mixerLayout->addWidget(s/*, 0, Qt::AlignLeft*/);
      DEBUG_MIXER(stderr, "addStripToLayoutIfVisible: after addWidget(): mixerLayout count:%d\n", mixerLayout->count());
    }
    else {
      DEBUG_MIXER(stderr, "addStripToLayoutIfVisible: before insertWidget(): mixerLayout count:%d\n", mixerLayout->count());
      mixerLayout->insertWidget(lc - 1, s/*, 0, Qt::AlignLeft*/);
      DEBUG_MIXER(stderr, "addStripToLayoutIfVisible: after insertWidget(): mixerLayout count:%d\n", mixerLayout->count());
    }

  } else {
    DEBUG_MIXER(stderr, "addStripToLayoutIfVisible: strip is not visible: mixerLayout count:%d\n", mixerLayout->count());
    s->setVisible(false);
    stripVisibleChanged(s, false);
  }
}

void AudioMixerApp::addStripsTraditionalLayout()
{
  //  generate Input Strips
  StripList::iterator si = stripList.begin();
  for (; si != stripList.end(); ++si) {
    Strip *s = *si;
    // s might be null due to calling stripList.resize() in addStrip()!
    if (s && s->getTrack()->type() == MusECore::Track::AUDIO_INPUT)
      addStripToLayoutIfVisible(s);
  }

  //  Synthesizer Strips
  si = stripList.begin();
  for (; si != stripList.end(); ++si) {
    Strip *s = *si;
    // s might be null due to calling stripList.resize() in addStrip()!
    if (s && s->getTrack()->type() == MusECore::Track::AUDIO_SOFTSYNTH)
      addStripToLayoutIfVisible(s);
  }

  //  generate Wave Track Strips
  si = stripList.begin();
  for (; si != stripList.end(); ++si) {
    Strip *s = *si;
    // s might be null due to calling stripList.resize() in addStrip()!
    if (s && s->getTrack()->type() == MusECore::Track::WAVE)
      addStripToLayoutIfVisible(s);
  }

  //  generate Midi channel/port Strips
  si = stripList.begin();
  for (; si != stripList.end(); ++si) {
    Strip *s = *si;
    // s might be null due to calling stripList.resize() in addStrip()!
    if (s && (s->getTrack()->type() == MusECore::Track::MIDI ||
        (s)->getTrack()->type() == MusECore::Track::DRUM))
      addStripToLayoutIfVisible(s);
  }

  //  Groups
  si = stripList.begin();
  for (; si != stripList.end(); ++si) {
    Strip *s = *si;
    // s might be null due to calling stripList.resize() in addStrip()!
    if (s && s->getTrack()->type() == MusECore::Track::AUDIO_GROUP)
      addStripToLayoutIfVisible(s);
  }

  //  Aux
  si = stripList.begin();
  for (; si != stripList.end(); ++si) {
    Strip *s = *si;
    // s might be null due to calling stripList.resize() in addStrip()!
    if (s && s->getTrack()->type() == MusECore::Track::AUDIO_AUX)
      addStripToLayoutIfVisible(s);
  }

  //    Master
  si = stripList.begin();
  for (; si != stripList.end(); ++si) {
    Strip *s = *si;
    // s might be null due to calling stripList.resize() in addStrip()!
    if (s && s->getTrack()->type() == MusECore::Track::AUDIO_OUTPUT)
      addStripToLayoutIfVisible(s);
  }
}

void AudioMixerApp::stripVisibleChanged(Strip* s, bool v)
{
  if(!s)
    return;

  const MusECore::Track* t = s->getTrack();
  const QUuid uuid = t->uuid();
  if (!cfg->stripConfigList.empty()) {
    const int sz = cfg->stripConfigList.size();
    for (int i=0; i < sz; i++) {
      MusEGlobal::StripConfig& sc = cfg->stripConfigList[i];
      DEBUG_MIXER(stderr, "stripVisibleChanged() processing strip [%s][%d]\n", sc._uuid.toString().toLocal8Bit().constData(), sc._visible);
      if(!sc.isNull() && sc._uuid == uuid) {
          sc._visible = v;
          return;
        }
      }
    }
  fprintf(stderr, "stripVisibleChanged() StripConfig not found [%s]\n", uuid.toString().toLocal8Bit().constData());
}

void AudioMixerApp::stripUserWidthChanged(Strip* s, int w)
{
  if(!s)
    return;

  const MusECore::Track* t = s->getTrack();
  const QUuid uuid = t->uuid();
  if (!cfg->stripConfigList.empty()) {
    const int sz = cfg->stripConfigList.size();
    for (int i=0; i < sz; i++) {
      MusEGlobal::StripConfig& sc = cfg->stripConfigList[i];
      DEBUG_MIXER(stderr, "stripUserWidthChanged() processing strip [%s][%d]\n", sc._uuid.toString().toLocal8Bit().constData(), sc._width);
      if(!sc.isNull() && sc._uuid == uuid) {
          sc._width = w;
          return;
        }
      }
    }
  fprintf(stderr, "stripUserWidthChanged() StripConfig not found [%s]\n", uuid.toString().toLocal8Bit().constData());
}

void AudioMixerApp::menuViewAboutToShow()
{
//   for(StripList::iterator isl = stripList.begin(); isl != stripList.end(); ++isl)
//   {
//     Strip* strip = *isl;
//     if(!strip)
//       continue;
//     if(strip->isEmbedded())
//       continue;
//     if(strip->isSelected())
//     {
//       strip->setStripVisible(false);
//       strip->setVisible(false);
//       stripVisibleChanged(strip, false);
//     }
//   }

  knobsVsSlidersId->setChecked(MusEGlobal::config.preferKnobsVsSliders);
  showValuesId->setChecked(MusEGlobal::config.showControlValues);
  showMidiDbsId->setChecked(MusEGlobal::config.preferMidiVolumeDb);
  monOnRecArmId->setChecked(MusEGlobal::config.monitorOnRecord);
  momentaryMuteId->setChecked(MusEGlobal::config.momentaryMute);
  momentarySoloId->setChecked(MusEGlobal::config.momentarySolo);

  // Check that there is only one strip selected.
  int numsel = 0;
  for(StripList::const_iterator isl = stripList.cbegin(); isl != stripList.cend(); ++isl)
  {
    const Strip* strip = *isl;
    if(!strip)
      continue;
    if(/*!strip->isEmbedded() &&*/ strip->isSelected())
    {
      ++numsel;
      if(numsel > 1)
        break;
    }
  }
  changeTrackNameId->setEnabled(numsel == 1);
}

void AudioMixerApp::menuAudEffRackVisItemsAboutToShow()
{
  QList<QAction*> acts = audEffRackVisibleGroup->actions();
  foreach (QAction *act, acts)
  {
    if(-(act->data().toInt() - AUD_EFF_RACK_VIS_ITEMS_BASE) == MusEGlobal::config.audioEffectsRackVisibleItems)
    {
      act->setChecked(true);
      break;
    }
  }
}

void AudioMixerApp::menuViewGroupTriggered(QAction* act)
{
  if(!act)
    return;
  const int num = act->data().toInt();
  const bool checked = act->isChecked();
  switch(num)
  {
    case KNOBS_VS_SLIDERS:
      if(MusEGlobal::config.preferKnobsVsSliders != checked)
      {
        MusEGlobal::config.preferKnobsVsSliders = checked;
        MusEGlobal::muse->changeConfig(true); // Save settings immediately, and use simple version.
      }
    break;
    case SHOW_VALUES_IN_CONTROLS:
      if(MusEGlobal::config.showControlValues != checked)
      {
        MusEGlobal::config.showControlValues = checked;
        MusEGlobal::muse->changeConfig(true); // Save settings immediately, and use simple version.
      }
    break;
    case SHOW_MIDI_DBS:
      if(MusEGlobal::config.preferMidiVolumeDb != checked)
      {
        MusEGlobal::config.preferMidiVolumeDb = checked;
        MusEGlobal::muse->changeConfig(true); // Save settings immediately, and use simple version.
      }
    break;
    case MON_ON_REC_ARM:
      if(MusEGlobal::config.monitorOnRecord != checked)
      {
        MusEGlobal::config.monitorOnRecord = checked;
        MusEGlobal::muse->changeConfig(true); // Save settings immediately, and use simple version.
      }
    break;
    case MOMENTARY_MUTE:
      if(MusEGlobal::config.momentaryMute != checked)
      {
        MusEGlobal::config.momentaryMute = checked;
        MusEGlobal::muse->changeConfig(true); // Save settings immediately, and use simple version.
      }
    break;
    case MOMENTARY_SOLO:
      if(MusEGlobal::config.momentarySolo != checked)
      {
        MusEGlobal::config.momentarySolo = checked;
        MusEGlobal::muse->changeConfig(true); // Save settings immediately, and use simple version.
      }
    break;
    case CHANGE_TRACK_NAME:
      changeTrackNameTriggered();
    break;
    case ADVANCED_ROUTER:
      toggleRouteDialog();
    break;


    case SHOW_MIDI_TRACKS:
      showMidiTracksChanged(checked);
    break;

    case SHOW_DRUM_TRACKS:
      showNewDrumTracksChanged(checked);
    break;

    case SHOW_WAVE_TRACKS:
      showWaveTracksChanged(checked);
    break;

    case SHOW_INPUT_TRACKS:
      showInputTracksChanged(checked);
    break;

    case SHOW_OUTPUT_TRACKS:
      showOutputTracksChanged(checked);
    break;

    case SHOW_GROUP_TRACKS:
      showGroupTracksChanged(checked);
    break;

    case SHOW_AUX_TRACKS:
      showAuxTracksChanged(checked);
    break;

    case SHOW_SYNTH_TRACKS:
      showSyntiTracksChanged(checked);
    break;
  }
}

void AudioMixerApp::audEffRackVisItemsTriggered(QAction* act)
{
  if(!act)
    return;
  const int num = -(act->data().toInt() - AUD_EFF_RACK_VIS_ITEMS_BASE);
  if(num >= 0 && num <= MusECore::PipelineDepth)
  {
    MusEGlobal::config.audioEffectsRackVisibleItems = num;
    MusEGlobal::muse->changeConfig(true); // Save settings immediately, and use simple version.
  }
}

void AudioMixerApp::changeTrackNameTriggered()
{
  // Check that there is only one strip selected.
  MusECore::Track* track = nullptr;
  int numsel = 0;
  for(StripList::const_iterator isl = stripList.cbegin(); isl != stripList.cend(); ++isl)
  {
    const Strip* strip = *isl;
    if(!strip)
      continue;
    if(/*!strip->isEmbedded() &&*/ strip->isSelected())
    {
      ++numsel;
      if(numsel > 1)
        break;
      track = strip->getTrack();
    }
  }
  if(numsel == 1 && track)
    changeTrackName(track);
}

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

      const QFontMetrics fm = fontMetrics();
      const int totalMenuW = fm.horizontalAdvance(menuConfig->title() + menuView->title()) +
        fm.horizontalAdvance('0') * 4 + 6;
      if(w < totalMenuW)
        w = totalMenuW;
      view->setUpdatesEnabled(false);
      setUpdatesEnabled(false);
      if(stripList.size() <= 6)
        setMinimumWidth(w);

      // Hack flag to prevent overwriting the config geometry when resizing...
      _resizeFlag = true;
      // At this point the width may be UNEXPANDABLE and may be small !
      // This line forces the maximum width, to at least ALLOW expansion ...
      setMaximumWidth(w);
      // ... and now, this line restores the desired size.
      if(size() != cfg->geometry.size())
        resize(cfg->geometry.size());
      // Now reset the flag in case it wasn't already by the resize handler...
      _resizeFlag = false;
      
      setUpdatesEnabled(true);
      view->setUpdatesEnabled(true);
}

//---------------------------------------------------------
//   addStrip
//---------------------------------------------------------

void AudioMixerApp::addStrip(const MusECore::Track* t, const MusEGlobal::StripConfig& sc, int insert_pos)
{
    DEBUG_MIXER(stderr, "addStrip\n");
    Strip* strip;

    // Make them non-embedded: Moveable, hideable, and with an expander handle.
    if (t->isMidiTrack())
    {
          strip = new MidiStrip(central, (MusECore::MidiTrack*)t, true, false, _docked);
          DEBUG_MIXER(stderr, "AudioMixerApp::addStrip: MidiStrip%p\n", strip);
    }
    else
    {
          strip = new AudioStrip(central, (MusECore::AudioTrack*)t, true, false, _docked);
          DEBUG_MIXER(stderr, "AudioMixerApp::addStrip: AudioStrip%p\n", strip);
    }

    // Broadcast changes to other selected tracks.
    strip->setBroadcastChanges(true);

    // Set focus yielding to the mixer window.
    strip->setFocusYieldWidget(this);

    connect(strip, &Strip::clearStripSelection, [this]() { clearStripSelection(); } );
    connect(strip, &Strip::moveStrip, [this](Strip* s) { moveStrip(s); } );
    connect(strip, &Strip::visibleChanged, [this](Strip* s, bool v) { stripVisibleChanged(s, v); } );
    connect(strip, &Strip::userWidthChanged, [this](Strip* s, int w) { stripUserWidthChanged(s, w); } );

    // No insert position given?
    if(insert_pos == -1)
    {
      DEBUG_MIXER(stderr, "putting new strip [%s] at end\n", t->name().toLocal8Bit().data());
      stripList.append(strip);
    }
    else
    {
      DEBUG_MIXER(stderr, "inserting new strip [%s] at %d\n", t->name().toLocal8Bit().data(), insert_pos);
      // FIXME: In Qt5 this appears to automatically grow the list if the insert position is beyond the end, without error.
      //        In Qt6 QList is quite changed. That condition is now considered an error.
      //        Neither help system mentions this.
      //        Immediate crash here because size was 11 and pos was 34!
      // Required for Qt6!
      if(insert_pos > stripList.size())
        stripList.resize(insert_pos + 1);
      stripList.insert(insert_pos, strip);
    }

    strip->setVisible(sc._visible);
    strip->setStripVisible(sc._visible);
    if(sc._width >= 0)
      strip->setUserWidth(sc._width);

    // Create a strip config now if no strip config exists yet for this strip (sc is default, with invalid sn).
    if(sc.isNull())
    {
      const MusEGlobal::StripConfig scl(t->uuid(), strip->getStripVisible(), strip->userWidth());
      cfg->stripConfigList.append(scl);
    }
}

//---------------------------------------------------------
//   clearAndDelete
//---------------------------------------------------------

void AudioMixerApp::clearAndDelete()
{
  DEBUG_MIXER(stderr, "clearAndDelete\n");
  // Remove and delete only strip widgets from the layout, not spacers/stretchers etc.
//  StripList::iterator si = stripList.begin();
//  for (; si != stripList.end(); ++si)
  DEBUG_MIXER(stderr, "AudioMixerApp::clearAndDelete(): Before: mixerLayout count %d\n", mixerLayout->count());
  for (auto& si : stripList)
  {
    // s might be null due to calling stripList.resize() in addStrip()!
    if(!si)
      continue;
    mixerLayout->removeWidget(si);
    //(*si)->deleteLater();
    delete si;
  }
  DEBUG_MIXER(stderr, "AudioMixerApp::clearAndDelete(): After: mixerLayout count %d\n", mixerLayout->count());

  cfg->stripConfigList.clear();
  stripList.clear();
  // Obsolete. Support old files.
  cfg->stripOrder.clear();
  oldAuxsSize = -1;
}

//---------------------------------------------------------
//   initMixer
//---------------------------------------------------------

void AudioMixerApp::initMixer()
{
  DEBUG_MIXER(stderr, "initMixer %d\n", cfg->stripOrder.empty() ? cfg->stripConfigList.size() : cfg->stripOrder.size());
  setWindowTitle(cfg->name);
  //clearAndDelete();

  showMidiTracksId->setChecked(cfg->showMidiTracks);
//  showDrumTracksId->setChecked(cfg->showDrumTracks);
  showNewDrumTracksId->setChecked(cfg->showNewDrumTracks);
  showInputTracksId->setChecked(cfg->showInputTracks);
  showOutputTracksId->setChecked(cfg->showOutputTracks);
  showWaveTracksId->setChecked(cfg->showWaveTracks);
  showGroupTracksId->setChecked(cfg->showGroupTracks);
  showAuxTracksId->setChecked(cfg->showAuxTracks);
  showSyntiTracksId->setChecked(cfg->showSyntiTracks);

  int auxsSize = MusEGlobal::song->auxs()->size();
  oldAuxsSize = auxsSize;
  const MusECore::TrackList *tl = MusEGlobal::song->tracks();

  // NOTE: stripOrder string list is obsolete. Must support old songs.
  if (!cfg->stripOrder.empty()) {
    const int sz = cfg->stripOrder.size();
    for (int i=0; i < sz; i++) {
      DEBUG_MIXER(stderr, "processing strip [%s][%d]\n", cfg->stripOrder.at(i).toLocal8Bit().data(), cfg->stripVisibility.at(i));
      for (const auto& tli : *tl) {
        if (tli->name() == cfg->stripOrder.at(i)) {
          MusEGlobal::StripConfig sc;
          sc._visible = cfg->stripVisibility.at(i);
          addStrip(tli, sc);
          break;
        }
      }
    }
  }
  // NOTE: stripConfigList is the replacement for obsolete stripOrder string list.
  else if (!cfg->stripConfigList.empty()) {
      const int sz = cfg->stripConfigList.size();
      for (int i=0; i < sz; i++) {
          const MusEGlobal::StripConfig& sc = cfg->stripConfigList.at(i);
          DEBUG_MIXER(stderr, "processing strip #:%d [%s][%d]\n", i, sc._uuid.toString().toLocal8Bit().constData(), sc._visible);
          if(!sc._deleted && !sc.isNull()) {
              const MusECore::Track* t = tl->findUuid(sc._uuid);
              if (t)
                  addStrip(t, sc);
          }
      }
  }
  else {
    for (const auto& tli : *tl)
      addStrip(tli);
  }
}

//---------------------------------------------------------
//   configChanged
//---------------------------------------------------------

void AudioMixerApp::configChanged()    
{ 
  DEBUG_MIXER(stderr, "configChanged\n");
  StripList::iterator si = stripList.begin();  // Catch when fonts change, viewable tracks, etc. No full rebuild.
  for (; si != stripList.end(); ++si) 
  {
    Strip *s = *si;
    // s might be null due to calling stripList.resize() in addStrip()!
    if(!s)
      continue;
    s->configChanged();
  }
  
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

bool AudioMixerApp::updateStripList()
{
  DEBUG_MIXER(stderr, "updateStripList stripList %d tracks %zd\n", stripList.size(), MusEGlobal::song->tracks()->size());
  
  if (stripList.empty() &&
      // Obsolete. Support old files.
      !cfg->stripOrder.empty())
      {
        initMixer();
        return true;
      }

  bool changed = false;

  const MusECore::TrackList *tl = MusEGlobal::song->tracks();
  // check for superfluous strips
  for (StripList::iterator si = stripList.begin(); si != stripList.end(); ) {
    Strip *s = *si;
    // s might be null due to calling stripList.resize() in addStrip()!
    if (s && !tl->contains(s->getTrack())) {
      DEBUG_MIXER(stderr, "Did not find track for strip %s - Removing\n", (*si)->getLabelText().toLocal8Bit().data());
      //(*si)->deleteLater();
      delete (*si);
      si = stripList.erase(si);
      changed = true;
    }
    else
      ++si;
  }

  // Mark deleted tracks' strip configs as deleted.
  // Do NOT remove them. They must hang around so undeleted tracks can find them again (below).
  const int config_sz = cfg->stripConfigList.size();
  for (int i = 0; i < config_sz; ++i )
  {
    MusEGlobal::StripConfig& sc = cfg->stripConfigList[i];
    if(!sc._deleted && tl->indexOfUuid(sc._uuid) < 0)
      sc._deleted = true;
  }

  // check for new tracks
  for (MusECore::ciTrack tli = tl->cbegin(); tli != tl->end();++tli) {
    const MusECore::Track* track = *tli;
    const QUuid uuid = track->uuid();
    StripList::const_iterator si = stripList.cbegin();
    for (; si != stripList.cend(); ++si) {
      const Strip *s = *si;
      // s might be null due to calling stripList.resize() in addStrip()!
      if (s && s->getTrack() == track) {
        break;
      }
    }
    if (si == stripList.cend()) {
      DEBUG_MIXER(stderr, "Did not find strip for track %s - Adding\n", track->name().toLocal8Bit().data());
      // Check if there's an existing strip config for this track.
      int idx = 0;
      int i = 0;
      for(; i < config_sz; ++i)
      {
        MusEGlobal::StripConfig& sc = cfg->stripConfigList[i];
        if(!sc.isNull() && sc._uuid == uuid)
        {
          // Be sure to mark the strip config as undeleted (in use).
          sc._deleted = false;
          // Insert a new strip at the index, with the given config.
          // FIXME: In Qt5 this appears to automatically grow the list if the insert position is beyond the end, without error.
          //        In Qt6 QList is quite changed. That condition is now considered an error.
          //        Neither help system mentions this.
          //        Immediate crash in addStrip because stripList size was 11 and pos was 34! Fixed now for Qt6 in addStrip.
          addStrip(track, sc, idx);
          changed = true;
          break;
        }
        if(!sc._deleted)
          ++idx;
      }
      // No config found? Append a new strip with a default config.
      if(i == config_sz)
      {
        addStrip(track);
        changed = true;
      }
    }
  }
  return changed;
}

void AudioMixerApp::updateSelectedStrips()
{
  for (Strip *s : stripList)
  {
    if(!s)
      continue;
    if(MusECore::Track* t = s->getTrack())
    {
      if(s->isSelected() != t->selected())
        s->setSelected(t->selected());
    }
  }
}

void AudioMixerApp::moveConfig(const Strip* s, int new_pos)
{
  if(!s)
    return;

  if(cfg->stripConfigList.empty())
    return;
  const MusECore::Track* track = s->getTrack();
  if(!track)
    return;
  const QUuid uuid = track->uuid();

  // The config list may also contain configs marked as 'deleted'.
  // Get the 'real' new_pos index, skipping over them.
  const int config_sz = cfg->stripConfigList.size();
  int new_pos_idx = -1;
  int s_idx = -1;
  int j = 0;
  for (int i = 0; i < config_sz; ++i)
  {
    const MusEGlobal::StripConfig& sc = cfg->stripConfigList.at(i);
    // Only 'count' configs that are not deleted.
    if(!sc._deleted)
    {
      if(new_pos_idx == -1 && j == new_pos)
        new_pos_idx = i;
      ++j;
    }
    // While we're at it, remember the index of the given strip.
    if(s_idx == -1 && sc._uuid == uuid)
      s_idx = i;
    // If we have both pieces of information we're done.
    if(new_pos_idx != -1 && s_idx != -1)
      break;
  }

  // Nothing to do?
  if(new_pos_idx == -1 || s_idx == -1 || new_pos_idx == s_idx)
    return;
  
  // To avoid keeping a copy of the item if doing REMOVE then INSERT, do INSERT then REMOVE.
  // QList::swap() is available but is too new at 5.13.

  // Pre-increment the new pos index if removing an item would affect it.
  if(new_pos_idx > s_idx)
    ++new_pos_idx;

  // Required for Qt6!
  if(new_pos_idx > stripList.size())
    stripList.resize(new_pos_idx + 1);
  cfg->stripConfigList.insert(new_pos_idx, cfg->stripConfigList.at(s_idx));

  // Pre-increment the original pos index if removing an item would affect it.
  if(s_idx > new_pos_idx)
    ++s_idx;

  cfg->stripConfigList.removeAt(s_idx);
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
  bool changed = false;

  if (flags & (SC_TRACK_INSERTED | SC_TRACK_REMOVED))
  {
    if(updateStripList())
      changed = true;
  }

  // Moving a track is not detected by updateStripList(). Do it here :-)
  if (flags & SC_TRACK_MOVED)
    changed = true;
  
  // Redrawing is costly to do every time. If no strips were added, removed, or moved,
  //  then there should be no reason to redraw the mixer.
  if(changed)
  {
    if (flags &
      // The only flags which would require a redraw, which is very costly, are the following:
      (SC_TRACK_REMOVED | SC_TRACK_INSERTED | SC_TRACK_MOVED
      //| SC_CHANNELS   // Channels due to one/two meters and peak labels can change the strip width.
      //| SC_AUX
      ))
    redrawMixer();
  }

  StripList::iterator si = stripList.begin();
  for (; si != stripList.end(); ++si) {
        Strip *s = *si;
        if(!s)
          continue;
        // s might be null due to calling stripList.resize() in addStrip()!
        s->songChanged(flags);
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
      if (on && routingDialog == nullptr) {
            routingDialog = new MusEGui::RouteDialog(this);
            connect(routingDialog, &RouteDialog::closed, [this]() { routingDialogClosed(); } );
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
//void AudioMixerApp::showDrumTracksChanged(bool v)
//{
//    cfg->showDrumTracks = v;
//    redrawMixer();
//}
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

void AudioMixerApp::keyPressEvent(QKeyEvent *ev)
{
  // Set to accept by default.
  ev->accept();

  const int kb_code = ev->key() | ev->modifiers();

  if(kb_code == MusEGui::shortcuts[MusEGui::SHRT_MIXER_SELECT_STRIP_LEFT].key)
  {
        selectNextStrip(false);
        return;
  }
  else if(kb_code == MusEGui::shortcuts[MusEGui::SHRT_MIXER_SELECT_STRIP_RIGHT].key)
  {
    selectNextStrip(true);
    return;
  }

  ev->ignore();
  return QMainWindow::keyPressEvent(ev);
}

void AudioMixerApp::resizeEvent(QResizeEvent* e)
{
  e->ignore();
  QMainWindow::resizeEvent(e);
  // Make sure to update the config size.
  // Hack flag to prevent overwriting the config geometry when resizing.
  // Do NOT overwrite if we set the flag before calling.
  if(!_resizeFlag)
    cfg->geometry.setSize(e->size());
}

void AudioMixerApp::moveEvent(QMoveEvent* e)
{
  e->ignore();
  QMainWindow::moveEvent(e);
  // Make sure to update the config geometry position.
  cfg->geometry.setTopLeft(e->pos());
}

void AudioMixerApp::clearStripSelection()
{
  foreach (Strip *s, stripList)
  {
    if(!s)
      continue;
    s->setSelected(false);
  }
}

void AudioMixerApp::selectNextStrip(bool isRight)
{
  Strip *prev = nullptr;

  for (int i = 0; i < mixerLayout->count(); i++)
  {
    QWidget *w = mixerLayout->itemAt(i)->widget();
    if (w)
    {
      if (prev && !prev->isEmbedded() && prev->isSelected() && isRight) // got it
      {
        Strip* st = static_cast<Strip*>(w);
        MusEGlobal::song->selectAllTracks(false);
        clearStripSelection();
        st->setSelected(true);
        if(st->getTrack())
          st->getTrack()->setSelected(true);
        MusEGlobal::song->update(SC_TRACK_SELECTION);
        return;
      }
      else if( !static_cast<Strip*>(w)->isEmbedded() && static_cast<Strip*>(w)->isSelected() && prev && !prev->isEmbedded() && !isRight)
      {
        MusEGlobal::song->selectAllTracks(false);
        clearStripSelection();
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
    MusEGlobal::song->selectAllTracks(false);
    clearStripSelection();
    st->setSelected(true);
    if(st->getTrack())
      st->getTrack()->setSelected(true);
    MusEGlobal::song->update(SC_TRACK_SELECTION);
  }
}

bool AudioMixerApp::eventFilter(QObject *obj, QEvent *event)
{
  //DEBUG_MIXER(stderr, "eventFilter type %d\n", (int)event->type());
    if (event->type() == QEvent::KeyPress)
    {
         this->keyPressEvent(static_cast<QKeyEvent*>(event));
         return true;
    }//if type()

    else if (event->type() == QEvent::KeyRelease)
    {
        this->keyReleaseEvent(static_cast<QKeyEvent*>(event));
        return  true;
    }//else if type()

    //### Standard event processing ###
    else
        return QMainWindow::eventFilter(obj, event);
}//eventFilter

void AudioMixerApp::changeTrackName(MusECore::Track* track)
{
  if(!track)
    return;

  const QString oldname = track->name();

  QInputDialog dlg(this);
  dlg.setWindowTitle(tr("Track Name"));
  dlg.setLabelText(tr("Enter track name:"));
  dlg.setTextValue(oldname);
  // set standard font size explicitly, otherwise the font is too small (inherited from mixer strip)
  dlg.setStyleSheet("font-size:" + QString::number(MusEGlobal::config.fonts[0].pointSize()) + "pt");

  const int res = dlg.exec();
  if(res == QDialog::Rejected)
    return;

  const QString newname = dlg.textValue();

  if(newname == oldname)
    return;

  MusECore::TrackList* tl = MusEGlobal::song->tracks();
  for (MusECore::iTrack i = tl->begin(); i != tl->end(); ++i)
  {
    if ((*i)->name() == newname)
    {
      QMessageBox msg(this);
      msg.setWindowTitle(tr("MusE: bad trackname"));
      msg.setText(tr("The track name is already used."));
      msg.setInformativeText(tr("Do you really want to use the name again?"));
      msg.setIcon(QMessageBox::Warning);
      msg.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
      int ret = msg.exec();
      if(ret != QMessageBox::Yes)
        return;
      break;
    }
  }

  MusEGlobal::song->applyOperation(
    MusECore::UndoOp(MusECore::UndoOp::ModifyTrackName, track, oldname, newname));
}

} // namespace MusEGui
