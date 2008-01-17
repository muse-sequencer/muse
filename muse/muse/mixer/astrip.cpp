//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2000-2006 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#include "audio.h"
#include "driver/audiodev.h"
#include "song.h"
#include "astrip.h"
#include "synth.h"
#include "rack.h"
#include "gconfig.h"
#include "muse.h"
#include "widgets/simplebutton.h"
#include "widgets/utils.h"
#include "auxplugin.h"
#include "midiinport.h"
#include "midioutport.h"

#include "awl/volknob.h"
#include "awl/panknob.h"
#include "awl/mslider.h"
#include "awl/volentry.h"
#include "awl/panentry.h"

//---------------------------------------------------------
//   AudioStrip
//    create mixer strip
//---------------------------------------------------------

AudioStrip::AudioStrip(Mixer* m, AudioTrack* t, bool align)
   : Strip(m, t, align)
      {
      iR            = 0;
      oR            = 0;
      off           = 0;
      volume        = -1.0;
      channel       = t->channels();

      //---------------------------------------------------
      //    prefader plugin rack
      //---------------------------------------------------

      rack1 = new EffectRack(this, t, true);
      rack1->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
      rack1->setFixedHeight(rack1->sizeHint().height() + 2);
      grid->addWidget(rack1, 1, 0, 1, 2);

      //---------------------------------------------------
      //    mono/stereo  pre/post
      //---------------------------------------------------

      stereo  = newStereoButton();
      stereo->setChecked(channel == 2);
      stereo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
      stereo->setFixedHeight(LABEL_HEIGHT);
      connect(stereo, SIGNAL(clicked(bool)), SLOT(stereoToggled(bool)));

      pre = new QToolButton;
      pre->setFont(config.fonts[1]);
      pre->setCheckable(true);
      pre->setText(tr("Pre"));
      pre->setToolTip(tr("pre fader - post fader"));
      pre->setChecked(t->prefader());
      pre->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
      pre->setFixedHeight(LABEL_HEIGHT);
      connect(pre, SIGNAL(clicked(bool)), SLOT(preToggled(bool)));

      grid->addWidget(stereo, 2, 0);
      grid->addWidget(pre, 2, 1);

      //---------------------------------------------------
      //    slider, label
      //---------------------------------------------------

      slider = new Awl::MeterSlider(this);
      slider->setRange(config.minSlider, config.maxSlider);
      slider->setFixedWidth(60);
      slider->setChannel(channel);
      Ctrl* ctrl = t->getController(AC_VOLUME);
      double vol = 0.0f;
      if (ctrl)
            vol = ctrl->curVal().f;
      slider->setValue(vol);
      grid->addWidget(slider, 3, 0, 1, 2); // 100, Qt::AlignRight);

      sl = new Awl::VolEntry(this);
      sl->setFont(config.fonts[1]);
      sl->setSuffix(tr("dB"));
      sl->setFrame(true);
      sl->setValue(vol);
      sl->setFixedHeight(entrySize.height());

      connect(slider, SIGNAL(valueChanged(double,int)), SLOT(volumeChanged(double)));
      connect(sl,     SIGNAL(valueChanged(double,int)), SLOT(volumeChanged(double)));

      connect(slider, SIGNAL(sliderPressed(int)), SLOT(volumePressed()));
      connect(slider, SIGNAL(sliderReleased(int)), SLOT(volumeReleased()));
      connect(slider, SIGNAL(meterClicked()), SLOT(resetPeaks()));
      grid->addWidget(sl, 4, 0, 1, 2);

      //---------------------------------------------------
      //    pan, balance
      //---------------------------------------------------

      pan = addPanKnob(&panl, 5);
      double panv = t->getController(AC_PAN)->curVal().f;
      pan->setValue(panv);
      panl->setValue(panv);

      //---------------------------------------------------
      //    postfader plugin rack
      //---------------------------------------------------

      rack2 = new EffectRack(this, t, false);
      rack2->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
      rack2->setFixedHeight(rack1->sizeHint().height() + 2);
      grid->addWidget(rack2, 7, 0, 1, 2);

      //---------------------------------------------------
      //    mute, solo, record
      //---------------------------------------------------

      Track::TrackType type = t->type();

      mute  = newMuteButton();
      mute->setChecked(t->mute());
      mute->setFixedHeight(BUTTON_HEIGHT);
      connect(mute, SIGNAL(clicked(bool)), SLOT(muteToggled(bool)));
      connect(t, SIGNAL(muteChanged(bool)), mute, SLOT(setChecked(bool)));

      solo  = newSoloButton();
      solo->setDisabled(true);
      solo->setFixedHeight(BUTTON_HEIGHT);
      connect(solo, SIGNAL(clicked(bool)), SLOT(soloToggled(bool)));
      connect(t, SIGNAL(soloChanged(bool)), solo, SLOT(setChecked(bool)));

      off  = newOffButton();
      off->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
      off->setFixedHeight(BUTTON_HEIGHT);
      off->setChecked(t->off());
      connect(off, SIGNAL(clicked(bool)), SLOT(offToggled(bool)));
      connect(t, SIGNAL(offChanged(bool)), this, SLOT(updateOffState()));

      if (track->canRecord()) {
            record = newRecordButton();
            record->setFixedHeight(BUTTON_HEIGHT);
            if (type == Track::AUDIO_OUTPUT)
                  record->setToolTip(tr("record downmix"));
            record->setChecked(t->recordFlag());
            connect(record, SIGNAL(clicked(bool)), SLOT(recordToggled(bool)));
            connect(t, SIGNAL(recordChanged(bool)), record, SLOT(setChecked(bool)));
            }
      else
            record = 0;

      grid->addWidget(off, 8, 0);
      if (record)
            grid->addWidget(record, 8, 1);
      grid->addWidget(mute, 9, 0);
      grid->addWidget(solo, 9, 1);

      //---------------------------------------------------
      //    automation read write
      //---------------------------------------------------

      addAutomationButtons(10);

      //---------------------------------------------------
      //    routing
      //---------------------------------------------------

      iR = newInRouteButton();
      grid->addWidget(iR, 12, 0);
      connect(iR->menu(), SIGNAL(aboutToShow()), SLOT(iRouteShow()));
      connect(iR->menu(), SIGNAL(aboutToHide()), SLOT(iRouteHide()));
      connect(iR->menu(), SIGNAL(triggered(QAction*)), song, SLOT(routeChanged(QAction*)));

      oR = newOutRouteButton();
      grid->addWidget(oR, 12, 1);
      connect(oR->menu(), SIGNAL(aboutToShow()), SLOT(oRouteShow()));
      connect(oR->menu(), SIGNAL(aboutToHide()), SLOT(oRouteHide()));
      connect(oR->menu(), SIGNAL(triggered(QAction*)), song, SLOT(routeChanged(QAction*)));

      if (off) {
            updateOffState();   // init state
            }
      connect(song, SIGNAL(songChanged(int)), SLOT(songChanged(int)));
      connect(track, SIGNAL(controllerChanged(int)), SLOT(controllerChanged(int)));
      connect(track, SIGNAL(autoReadChanged(bool)), SLOT(autoChanged()));
      connect(track, SIGNAL(autoWriteChanged(bool)), SLOT(autoChanged()));
      autoChanged();
      }

//---------------------------------------------------------
//   heartBeat
//---------------------------------------------------------

void AudioStrip::heartBeat()
      {
      int peakHold = (config.peakHoldTime * config.guiRefresh) / 1000;
      for (int ch = 0; ch < channel; ++ch) {
            int n = track->peakTimer(ch);
            ++n;
            double f = track->peak(ch);
            if (n >= peakHold) {
            	// track->resetPeak(ch);
                  track->setPeak(ch, f * 0.7);
            	}
            track->setPeakTimer(ch, n);
            slider->setMeterVal(ch, track->meter(ch), f);
            }
      }

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void AudioStrip::songChanged(int val)
      {
      AudioTrack* src = (AudioTrack*)track;
      if (val & SC_TRACK_MODIFIED)
            updateLabel();
      if ((val & SC_ROUTE) && pre)
            pre->setChecked(src->prefader());
      if (val & SC_CHANNELS)
            updateChannels();
      }

//---------------------------------------------------------
//   offToggled
//---------------------------------------------------------

void AudioStrip::offToggled(bool val)
      {
      song->setOff(track, !val);
      }

//---------------------------------------------------------
//   updateOffState
//---------------------------------------------------------

void AudioStrip::updateOffState()
      {
      bool val = !track->off();
      slider->setEnabled(val);
      sl->setEnabled(val);
      pan->setEnabled(val);
      panl->setEnabled(val);
      pre->setEnabled(val);
      rack1->setEnabled(val);
      rack2->setEnabled(val);

      if (track->type() != Track::AUDIO_SOFTSYNTH)
            stereo->setEnabled(val);
      label->setEnabled(val);
      if (solo)
            solo->setEnabled(val);
      if (mute)
            mute->setEnabled(val);
      if (iR)
            iR->setEnabled(val);
      if (oR)
            oR->setEnabled(val);
      if (off)
            off->setChecked(track->off());
      if (ar)
            ar->setEnabled(val);
      if (aw)
            aw->setEnabled(val);
      }

//---------------------------------------------------------
//   preToggled
//---------------------------------------------------------

void AudioStrip::preToggled(bool val)
      {
      ((AudioTrack*)track)->setPrefader(val);
      resetPeaks();
      song->update(SC_ROUTE);
      }

//---------------------------------------------------------
//   stereoToggled
//---------------------------------------------------------

void AudioStrip::stereoToggled(bool val)
      {
      int nc = val ? 1 : 2;
      if (channel == nc)
            return;
      audio->msgSetChannels((AudioTrack*)track, nc);
      song->update(SC_CHANNELS);
      }

//---------------------------------------------------------
//   volumeChanged
//---------------------------------------------------------

void AudioStrip::volumeChanged(double v)
      {
      CVal val;
      val.f = v;
      song->setControllerVal(track, AC_VOLUME, val);
      }

//---------------------------------------------------------
//   volumePressed
//---------------------------------------------------------

void AudioStrip::volumePressed()
      {
      ((AudioTrack*)track)->startAutoRecord(AC_VOLUME);
      }

//---------------------------------------------------------
//   volumeReleased
//---------------------------------------------------------

void AudioStrip::volumeReleased()
      {
      ((AudioTrack*)track)->stopAutoRecord(AC_VOLUME);
      }

//---------------------------------------------------------
//   panChanged
//---------------------------------------------------------

void AudioStrip::panChanged(double v)
      {
      if (v != panVal) {
            CVal val;
            val.f = v;
            song->setControllerVal(track, AC_PAN, val);
            panVal = v;
            }
      }

//---------------------------------------------------------
//   setPan
//---------------------------------------------------------

void AudioStrip::setPan(double val)
      {
      if (val != panVal) {
            panVal = val;
            pan->setValue(val);
            panl->setValue(val);
            }
      }

//---------------------------------------------------------
//   panPressed
//---------------------------------------------------------

void AudioStrip::panPressed()
      {
      track->startAutoRecord(AC_PAN);
      }

//---------------------------------------------------------
//   panReleased
//---------------------------------------------------------

void AudioStrip::panReleased()
      {
      track->stopAutoRecord(AC_PAN);
      }

//---------------------------------------------------------
//   updateChannels
//---------------------------------------------------------

void AudioStrip::updateChannels()
      {
      AudioTrack* t = (AudioTrack*)track;
      int c = t->channels();
      if (c == channel)
            return;
      channel = c;
      slider->setChannel(c);
      stereo->setChecked(channel == 2);
      }

//---------------------------------------------------------
//   addPanKnob
//---------------------------------------------------------

Awl::PanKnob* AudioStrip::addPanKnob(Awl::PanEntry** dlabel, int row)
      {
      Awl::PanKnob* knob = new Awl::PanKnob(this);
      knob->setToolTip(tr("panorama"));
      knob->setFixedHeight(entrySize.height() * 2);

      Awl::PanEntry* pl  = new Awl::PanEntry(this);
      pl->setPrecision(2);

      if (dlabel)
            *dlabel = pl;
      pl->setFont(config.fonts[1]);
      pl->setFrame(true);

      pl->setFixedSize(entrySize);

      QLabel* plb = new QLabel;
      plb->setText(tr("Pan"));

      plb->setFont(config.fonts[1]);
      plb->setFixedSize(entrySize);
      plb->setAlignment(Qt::AlignCenter);

      grid->addWidget(plb,  row, 0);
      grid->addWidget(pl,   row+1, 0);
      grid->addWidget(knob, row, 1, 2, 1);

      connect(knob, SIGNAL(valueChanged(double,int)), SLOT(panChanged(double)));
      connect(pl,   SIGNAL(valueChanged(double,int)), SLOT(panChanged(double)));
      connect(knob, SIGNAL(sliderPressed(int)), SLOT(panPressed()));
      connect(knob, SIGNAL(sliderReleased(int)), SLOT(panReleased()));
      return knob;
      }

//---------------------------------------------------------
//   addAuxPorts
//---------------------------------------------------------

static void addAuxPorts(AudioTrack* track, QMenu* lb, const RouteList& rl)
      {
      QList<AuxPluginIF*> pre  = track->preAux();
      QList<AuxPluginIF*> post = track->postAux();
      foreach (AuxPluginIF* p, pre) {
            QString s = p->pluginInstance()->name();
            QAction* a = lb->addAction(p->pluginInstance()->name());
            a->setCheckable(true);
//            Route route(RouteNode(p), RouteNode(track));
            Route route = Route(RouteNode(p), RouteNode(track));
            a->setData(QVariant::fromValue(route));
            a->setChecked(rl.indexOf(route) != -1);
            }
      foreach(AuxPluginIF* p, post) {
            QAction* a = lb->addAction(p->pluginInstance()->name());
            a->setCheckable(true);
//            Route route(RouteNode(p), RouteNode(track));  // does not compile?!
            Route route = Route(RouteNode(p), RouteNode(track));
            a->setData(QVariant::fromValue(route));
            a->setChecked(rl.indexOf(route) != -1);
            }
      }

//---------------------------------------------------------
//   addInPorts
//---------------------------------------------------------

static void addInPorts(AudioTrack* t, QMenu* lb, const RouteList& rl, bool input)
      {
      RouteNode a(t);

      InputList* al = song->inputs();
      for (iAudioInput i = al->begin(); i != al->end(); ++i) {
            AudioTrack* track = (AudioTrack*)*i;
            if (t == track)
                  continue;
            QAction* it = lb->addAction(track->name());
            it->setCheckable(true);
            RouteNode b(track);
            Route route = input ? Route(b, a) : Route(a, b);
            it->setData(QVariant::fromValue(route));
            it->setChecked(rl.indexOf(route) != -1);
            if (input)
                  addAuxPorts(track, lb, rl);
            }
      }

//---------------------------------------------------------
//   addOutPorts
//---------------------------------------------------------

static void addOutPorts(AudioTrack* t, QMenu* lb, const RouteList& rl)
      {
      OutputList* al = song->outputs();
      for (iAudioOutput i = al->begin(); i != al->end(); ++i) {
            Track* track = *i;
            if (t == track)
                  continue;
            QAction* it = lb->addAction(track->name());
            it->setCheckable(true);
            Route route = Route(RouteNode(t), RouteNode(track));
            it->setData(QVariant::fromValue(route));
            it->setChecked(rl.indexOf(route) != -1);
            }
      }

//---------------------------------------------------------
//   addGroupPorts
//---------------------------------------------------------

static void addGroupPorts(AudioTrack* t, QMenu* lb, const RouteList& rl)
      {
      GroupList* al = song->groups();
      for (iAudioGroup i = al->begin(); i != al->end(); ++i) {
            AudioTrack* track = (AudioTrack*)*i;
            if (t == track)
                  continue;
            QAction* it = lb->addAction(track->name());
            it->setCheckable(true);
            Route route = Route(RouteNode(t), RouteNode(track));
            it->setData(QVariant::fromValue(route));
            it->setChecked(rl.indexOf(route) != -1);
            }
      }

//---------------------------------------------------------
//   addWavePorts
//---------------------------------------------------------

static void addWavePorts(AudioTrack* t, QMenu* lb, const RouteList& rl, bool input)
      {
      RouteNode a(t);
      WaveTrackList* al = song->waves();
      for (iWaveTrack i = al->begin(); i != al->end(); ++i) {
            AudioTrack* track = (AudioTrack*)*i;
            if (t == track)         // cannot connect to itself
                  continue;
            QAction* it = lb->addAction(track->name());
            it->setCheckable(true);
            RouteNode b(track);
            Route route = input ? Route(b, a) : Route(a, b);
            it->setData(QVariant::fromValue(route));
            it->setChecked(rl.indexOf(route) != -1);
            if (input)
                  addAuxPorts(track, lb, rl);
            }
      }

//---------------------------------------------------------
//   addSyntiPorts
//---------------------------------------------------------

static void addSyntiPorts(AudioTrack* t, QMenu* lb, const RouteList& rl)
      {
      SynthIList* al = song->syntis();
      for (iSynthI i = al->begin(); i != al->end(); ++i) {
            AudioTrack* track = (AudioTrack*)*i;
            if (t == track)
                  continue;
            QAction* it = lb->addAction(track->name());
            it->setCheckable(true);
            RouteNode a(t);
            RouteNode b(track);
            Route route = Route(b, a);
            it->setData(QVariant::fromValue(route));
            it->setChecked(rl.indexOf(route) != -1);
            }
      }

//---------------------------------------------------------
//   iRouteShow
//---------------------------------------------------------

extern void addMidiTracks(QMenu* menu, Track* track, int channel, bool input);
extern void addMidiInPorts(QMenu* menu, Track* dtrack, int channel);

void AudioStrip::iRouteShow()
      {
      QMenu* pup = iR->menu();
      pup->clear();
      AudioTrack* t  = (AudioTrack*)track;
      RouteList* irl = t->inRoutes();

      switch(track->type()) {
            default:
                  return;
            case Track::AUDIO_INPUT:
                  {
                  for (int i = 0; i < channel; ++i) {
                        pup->addSeparator()->setText(QString(tr("Channel %1")).arg(i+1));
                        QList<PortName> ol = audioDriver->outputPorts(false);
                        foreach (PortName ip, ol) {
                              QAction* id = pup->addAction(ip.name);
                              id->setCheckable(true);
                              RouteNode src(ip.port, -1, RouteNode::AUDIOPORT);
                              RouteNode dst(t, i, RouteNode::TRACK);
                              Route route = Route(src, dst);
                              id->setData(QVariant::fromValue(route));
                              id->setChecked(irl->indexOf(route) != -1);
                              }
                        }
                  }
                  break;
            case Track::AUDIO_OUTPUT:
                  addWavePorts(t, pup, *irl, true);
                  addInPorts(t, pup, *irl, true);
                  addGroupPorts(t, pup, *irl);
                  addSyntiPorts(t, pup, *irl);
                  break;
            case Track::WAVE:
                  addInPorts(t, pup, *irl, true);
                  break;
            case Track::AUDIO_GROUP:
                  addWavePorts(t, pup, *irl, true);
                  addInPorts(t, pup, *irl, true);
                  addGroupPorts(t, pup, *irl);
                  addSyntiPorts(t, pup, *irl);
                  break;
            case Track::AUDIO_SOFTSYNTH:
                  pup->addSeparator()->setText(tr("MidiChannel"));
                  for (int ch = 0; ch < MIDI_CHANNELS; ++ch) {
                        QMenu* m = pup->addMenu(QString("Channel %1").arg(ch+1));
                        addMidiTracks(m, track, ch, true);
                        addMidiInPorts(m, track, ch);
                        }
                  break;
            }
      }

//---------------------------------------------------------
//   iRouteHide
//---------------------------------------------------------

void AudioStrip::iRouteHide()
      {
      // dont leave the menu if SHIFT is pressed; this allows
      // for fast selecting of more than one input source
      //
      if (qApp->keyboardModifiers() & Qt::ShiftModifier) {
            iR->menu()->show();
            }
      }

//---------------------------------------------------------
//   oRouteHide
//---------------------------------------------------------

void AudioStrip::oRouteHide()
      {
      // dont leave the menu if SHIFT is pressed; this allows
      // for fast selecting of more than one input source
      //
      if (qApp->keyboardModifiers() & Qt::ShiftModifier) {
            oR->menu()->show();
            }
      }

//---------------------------------------------------------
//   oRouteShow
//---------------------------------------------------------

void AudioStrip::oRouteShow()
      {
      QMenu* pup = oR->menu();
      pup->clear();
      AudioTrack* t = (AudioTrack*)track;
      RouteList* orl = t->outRoutes();

      switch(track->type()) {
            default:
                  return;
            case Track::AUDIO_OUTPUT:
                  {
                  for (int i = 0; i < channel; ++i) {
                        char buffer[128];
                        snprintf(buffer, 128, "%s %d", tr("Channel").toLatin1().data(), i+1);
                        pup->addSeparator()->setText(QString(buffer));

                        QList<PortName> ol = audioDriver->inputPorts(false);
                        foreach (PortName ip, ol) {
                              QAction* action = pup->addAction(ip.name);
                              action->setCheckable(true);
                              RouteNode src(t, i, RouteNode::TRACK);
                              RouteNode dst(ip.port, -1, RouteNode::AUDIOPORT);
                              Route r = Route(src, dst);
                              action->setData(QVariant::fromValue(r));
                              action->setChecked(orl->indexOf(r) != -1);
                              }
                        }
                  }
                  break;
            case Track::AUDIO_INPUT:
                  addWavePorts(t, pup, *orl, false);
            case Track::WAVE:
            case Track::AUDIO_GROUP:
            case Track::AUDIO_SOFTSYNTH:
                  addOutPorts(t, pup, *orl);
                  addGroupPorts(t, pup, *orl);
                  break;
            }
      }

//---------------------------------------------------------
//   muteToggled
//---------------------------------------------------------

void AudioStrip::muteToggled(bool val)
      {
      song->setMute(track, val);
      }

//---------------------------------------------------------
//   soloToggled
//---------------------------------------------------------

void AudioStrip::soloToggled(bool val)
      {
      song->setSolo(track, val);
      }

//---------------------------------------------------------
//   recordToggled
//---------------------------------------------------------

void AudioStrip::recordToggled(bool val)
      {
      song->setRecordFlag(track, !val);
      }

//---------------------------------------------------------
//   controllerChanged
//---------------------------------------------------------

void AudioStrip::controllerChanged(int id)
      {
      double val = track->getController(id)->curVal().f;
      switch (id) {
            case AC_VOLUME:
                  slider->setValue(val);
                  sl->setValue(val);
                  break;
            case AC_PAN:
                  pan->setValue(val);
                  panl->setValue(val);
                  panVal = val;
                  break;
            case AC_MUTE:
                  break;
            default:
                  break;
            }
      }

//---------------------------------------------------------
//   autoChanged
//---------------------------------------------------------

void AudioStrip::autoChanged()
      {
      bool ar = track->autoRead();
      bool aw = track->autoWrite();

      //  controller are enabled if
      //    autoRead is off
      //    autoRead and autoWrite are on (touch mode)

      bool ec = !ar || (ar && aw);
      slider->setEnabled(ec);
      sl->setEnabled(ec);
      pan->setEnabled(ec);
      panl->setEnabled(ec);
      }

//---------------------------------------------------------
//   autoReadToggled
//---------------------------------------------------------

void AudioStrip::autoReadToggled(bool val)
      {
      song->setAutoRead(track, val);
      }

//---------------------------------------------------------
//   autoWriteToggled
//---------------------------------------------------------

void AudioStrip::autoWriteToggled(bool val)
      {
      song->setAutoWrite(track, val);
      }

