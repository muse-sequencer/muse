//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: mixer.cpp,v 1.18 2006/01/12 18:15:28 wschweer Exp $
//
//  (C) Copyright 2000-2004 Werner Schweer (ws@seh.de)
//=========================================================

#include "muse.h"
#include "mixer.h"
#include "song.h"
#include "icons.h"
#include "astrip.h"
#include "mstrip.h"
#include "routedialog.h"
#include "synth.h"

extern void populateAddTrack(QMenu* addTrack);

//---------------------------------------------------------
//   Mixer
//
//    inputs | synthis | tracks | groups | master
//---------------------------------------------------------

Mixer::Mixer(QWidget* parent, MixerConfig* c)
   : QMainWindow(parent)
      {
      mustUpdateMixer = false;
      cfg = c;
      oldAuxsSize = 0;
      routingDialog = 0;
      setWindowTitle(tr("MusE: Mixer"));
      setWindowIcon(*museIcon);

      QMenu* menuCreate = menuBar()->addMenu(tr("&Create"));
      populateAddTrack(menuCreate);

      menuView = menuBar()->addMenu(tr("&View"));
      routingAction = menuView->addAction(tr("Routing"), this, SLOT(toggleRouteDialog()));
      routingAction->setCheckable(true);

      showMidiTracksId  = menuView->addAction(tr("Show Midi Tracks"));
      showMidiOutPortId = menuView->addAction(tr("Show Midi Out Ports"));
      showMidiInPortId  = menuView->addAction(tr("Show Midi In Ports"));
      showMidiChannelId = menuView->addAction(tr("Show Midi Channels"));

      menuView->addSeparator();

      showWaveTracksId   = menuView->addAction(tr("Show Wave Tracks"));
      showOutputTracksId = menuView->addAction(tr("Show Output Tracks"));
      showGroupTracksId  = menuView->addAction(tr("Show Group Tracks"));
      showInputTracksId  = menuView->addAction(tr("Show Input Tracks"));
      showAuxTracksId    = menuView->addAction(tr("Show Aux Tracks"));
      showSyntiTracksId  = menuView->addAction(tr("Show Synthesizer"));
      connect(menuView, SIGNAL(triggered(QAction*)), SLOT(showTracksChanged(QAction*)));

      showMidiTracksId->setCheckable(true);
      showMidiInPortId->setCheckable(true);
      showMidiOutPortId->setCheckable(true);
      showMidiChannelId->setCheckable(true);
      showWaveTracksId->setCheckable(true);
      showOutputTracksId->setCheckable(true);
      showGroupTracksId->setCheckable(true);
      showInputTracksId->setCheckable(true);
      showAuxTracksId->setCheckable(true);
      showSyntiTracksId->setCheckable(true);

      QScrollArea* view = new QScrollArea;
      view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
      setCentralWidget(view);

      central = new QWidget;
      layout  = new QHBoxLayout;
      central->setLayout(layout);
      layout->setSpacing(0);
      layout->setMargin(0);
      view->setWidget(central);
      view->setWidgetResizable(true);

      connect(song, SIGNAL(songChanged(int)), SLOT(songChanged(int)));
      connect(muse, SIGNAL(configChanged()), SLOT(configChanged()));
      connect(heartBeatTimer, SIGNAL(timeout()), SLOT(heartBeat()));
      song->update();  // calls update mixer
      }

//---------------------------------------------------------
//   addStrip
//---------------------------------------------------------

void Mixer::addStrip(Track* t, int idx)
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
            layout->removeWidget(*si);
            delete *si;
            stripList.erase(si);
            }
      else {
            Strip* strip;
            switch(t->type()) {
                  case Track::MIDI_IN:
                        strip = new MidiInPortStrip(this, (MidiInPort*)t, true);
                        break;
                  case Track::MIDI_OUT:
                        strip = new MidiOutPortStrip(this, (MidiOutPort*)t, true);
                        break;
                  case Track::MIDI_CHANNEL:
                        strip = new MidiChannelStrip(this, (MidiChannel*)t, true);
                        break;
                  case Track::MIDI:
                        strip = new MidiStrip(this, (MidiTrack*)t, true);
                        break;
                  case Track::MIDI_SYNTI:
                        strip = new MidiSyntiStrip(this, (MidiSynti*)t, true);
                        break;
                  default:
                        strip = new AudioStrip(this, (AudioTrack*)t, true);
                        break;
                  }
            layout->insertWidget(idx, strip);
            stripList.insert(si, strip);
            strip->show();
            }
      }

//---------------------------------------------------------
//   clear
//---------------------------------------------------------

void Mixer::clear()
      {
      QLayoutItem* i;
      while ((i = layout->takeAt(0))) {
            if (i->widget())
                  delete i->widget();
            delete i;
            }
      stripList.clear();
      oldAuxsSize = -1;
      }

//---------------------------------------------------------
//   updateMixer
//---------------------------------------------------------

void Mixer::updateMixer(int action)
      {
      showMidiTracksId->setChecked(cfg->showMidiTracks);
      showMidiInPortId->setChecked(cfg->showMidiInPorts);
      showMidiOutPortId->setChecked(cfg->showMidiOutPorts);
      showMidiChannelId->setChecked(cfg->showMidiChannels);
      showWaveTracksId->setChecked(cfg->showWaveTracks);
      showOutputTracksId->setChecked(cfg->showOutputTracks);
      showGroupTracksId->setChecked(cfg->showGroupTracks);
      showInputTracksId->setChecked(cfg->showInputTracks);
      showAuxTracksId->setChecked(cfg->showAuxTracks);
      showSyntiTracksId->setChecked(cfg->showSyntiTracks);

      int auxsSize = song->auxs()->size();
      if (action == STRIP_REMOVED && auxsSize == oldAuxsSize) {
            StripList::iterator si = stripList.begin();
            for (; si != stripList.end();) {
                  Track* track = (*si)->getTrack();
                  StripList::iterator ssi = si;
                  ++si;
                  if (song->trackExists(track))
                        continue;
                  layout->removeWidget(*ssi);
                  delete *ssi;
                  stripList.erase(ssi);
                  }
            int idx = stripList.size();
            setMaximumWidth(STRIP_WIDTH * idx);
            central->setFixedWidth(STRIP_WIDTH * idx);
            if (idx < 4)
                  setMinimumWidth(idx * STRIP_WIDTH);
            return;
            }

      clear();
      oldAuxsSize = auxsSize;

      int idx = 0;
      //---------------------------------------------------
      //  generate Input Strips
      //---------------------------------------------------

      if (cfg->showInputTracks) {
            InputList* itl = song->inputs();
            for (iAudioInput i = itl->begin(); i != itl->end(); ++i)
                  addStrip(*i, idx++);
            }

      //---------------------------------------------------
      //  Synthesizer Strips
      //---------------------------------------------------

      if (cfg->showSyntiTracks) {
            SynthIList* sl = song->syntis();
            for (iSynthI i = sl->begin(); i != sl->end(); ++i)
                  addStrip(*i, idx++);
            }

      //---------------------------------------------------
      //  generate Wave Track Strips
      //---------------------------------------------------

      if (cfg->showWaveTracks) {
            WaveTrackList* wtl = song->waves();
            for (iWaveTrack i = wtl->begin(); i != wtl->end(); ++i)
                  addStrip(*i, idx++);
            }

      //---------------------------------------------------
      //  generate Midi strips
      //---------------------------------------------------

      if (cfg->showMidiInPorts) {
            MidiInPortList* mpl = song->midiInPorts();
            for (iMidiInPort i = mpl->begin(); i != mpl->end(); ++i)
                  addStrip(*i, idx++);
            }

      if (cfg->showMidiSyntiPorts) {
            MidiSyntiList* mpl = song->midiSyntis();
            for (iMidiSynti i = mpl->begin(); i != mpl->end(); ++i)
                  addStrip(*i, idx++);
            }

      if (cfg->showMidiTracks) {
            MidiTrackList* mtl = song->midis();
            for (iMidiTrack i = mtl->begin(); i != mtl->end(); ++i)
                  addStrip(*i, idx++);
            }

      if (cfg->showMidiChannels) {
            MidiChannelList* mpl = song->midiChannel();
            for (iMidiChannel i = mpl->begin(); i != mpl->end(); ++i) {
                  MidiChannel* mc = *i;
                  //
                  // show only if used
                  //
                  if (!mc->noInRoute())
                        addStrip(mc, idx++);
                  }
            }
      if (cfg->showMidiOutPorts) {
            MidiOutPortList* mpl = song->midiOutPorts();
            for (iMidiOutPort i = mpl->begin(); i != mpl->end(); ++i)
                  addStrip(*i, idx++);
            }

      //---------------------------------------------------
      //  Groups
      //---------------------------------------------------

      if (cfg->showGroupTracks) {
            GroupList* gtl = song->groups();
            for (iAudioGroup i = gtl->begin(); i != gtl->end(); ++i)
                  addStrip(*i, idx++);
            }

      //---------------------------------------------------
      //  Aux
      //---------------------------------------------------

      if (cfg->showAuxTracks) {
            AuxList* al = song->auxs();
            for (iAudioAux i = al->begin(); i != al->end(); ++i)
                  addStrip(*i, idx++);
            }

      //---------------------------------------------------
      //    Master
      //---------------------------------------------------

      if (cfg->showOutputTracks) {
            OutputList* otl = song->outputs();
            for (iAudioOutput i = otl->begin(); i != otl->end(); ++i)
                  addStrip(*i, idx++);
            }
      setMaximumWidth(STRIP_WIDTH * idx);
      central->setFixedWidth(STRIP_WIDTH * idx);
      if (idx < 4)
            setMinimumWidth(idx * STRIP_WIDTH);
      layout->update();
      }

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void Mixer::songChanged(int flags)
      {
      int action = NO_UPDATE;
      if (flags == -1)
            action = UPDATE_ALL;
      else {
            if (flags & SC_TRACK_REMOVED)
                  action |= STRIP_REMOVED;
            if (flags & SC_TRACK_INSERTED)
                  action |= STRIP_INSERTED;
            }
      if (action != NO_UPDATE)
            updateMixer(action);
      }

//---------------------------------------------------------
//   closeEvent
//---------------------------------------------------------

void Mixer::closeEvent(QCloseEvent* e)
      {
      emit closed();
      e->accept();
      }

//---------------------------------------------------------
//   toggleRouteDialog
//---------------------------------------------------------

void Mixer::toggleRouteDialog()
      {
      showRouteDialog(routingAction->isChecked());
      }

//---------------------------------------------------------
//   showRouteDialog
//---------------------------------------------------------

void Mixer::showRouteDialog(bool on)
      {
      if (on && routingDialog == 0) {
            routingDialog = new RouteDialog(this);
            connect(routingDialog, SIGNAL(closed()), SLOT(routingDialogClosed()));
            }
      if (routingDialog)
            routingDialog->setShown(on);
      routingAction->setChecked(on);
      }

//---------------------------------------------------------
//   routingDialogClosed
//---------------------------------------------------------

void Mixer::routingDialogClosed()
      {
      routingAction->setChecked(false);
      }

//---------------------------------------------------------
//   showTracksChanged
//---------------------------------------------------------

void Mixer::showTracksChanged(QAction* id)
      {
      bool val = id->isChecked();
      if (id == showMidiTracksId)
            cfg->showMidiTracks = val;
      else if (id == showOutputTracksId)
            cfg->showOutputTracks = val;
      else if (id == showWaveTracksId)
            cfg->showWaveTracks = val;
      else if (id == showGroupTracksId)
            cfg->showGroupTracks = val;
      else if (id == showInputTracksId)
            cfg->showInputTracks = val;
      else if (id == showAuxTracksId)
            cfg->showAuxTracks = val;
      else if (id == showSyntiTracksId)
            cfg->showSyntiTracks = val;
      else if (id == showMidiInPortId)
            cfg->showMidiInPorts = val;
      else if (id == showMidiOutPortId)
            cfg->showMidiOutPorts = val;
      else if (id == showMidiChannelId)
            cfg->showMidiChannels = val;
      updateMixer(UPDATE_ALL);
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void Mixer::write(Xml& xml, const char* name)
      {
      xml.tag("%s", name);
      xml.qrectTag("geometry",       geometry());
      xml.intTag("showMidiTracks",   cfg->showMidiTracks);
      xml.intTag("showOutputTracks", cfg->showOutputTracks);
      xml.intTag("showWaveTracks",   cfg->showWaveTracks);
      xml.intTag("showGroupTracks",  cfg->showGroupTracks);
      xml.intTag("showInputTracks",  cfg->showInputTracks);
      xml.intTag("showAuxTracks",    cfg->showAuxTracks);
      xml.intTag("showSyntiTracks",  cfg->showSyntiTracks);
      xml.intTag("showMidiInPorts",  cfg->showMidiInPorts);
      xml.intTag("showMidiOutPorts", cfg->showMidiOutPorts);
      xml.intTag("showMidiChannels", cfg->showMidiChannels);
      xml.etag("%s", name);
      }

//---------------------------------------------------------
//   heartBeat
//---------------------------------------------------------

void Mixer::heartBeat()
      {
      if (mustUpdateMixer) {
            updateMixer(STRIP_INSERTED | STRIP_REMOVED);
            mustUpdateMixer = false;
            }
      }

