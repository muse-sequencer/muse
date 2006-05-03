//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: astrip.cpp,v 1.81 2006/04/22 13:53:17 wschweer Exp $
//
//  (C) Copyright 2000-2005 Werner Schweer (ws@seh.de)
//=========================================================

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
#include "plugin.h"

#include "awl/volknob.h"
#include "awl/panknob.h"
#include "awl/mslider.h"
#include "awl/volentry.h"
#include "awl/panentry.h"

//---------------------------------------------------------
//   heartBeat
//---------------------------------------------------------

void AudioStrip::heartBeat()
      {
      int peakHold = (config.peakHoldTime * config.guiRefresh) / 1000;
      for (int ch = 0; ch < channel; ++ch) {
            int n = track->peakTimer(ch);
            ++n;
            float f = track->peak(ch);
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
      if (val & SC_ROUTE) {
            if (pre) {
                  pre->setChecked(src->prefader());
                  }
            }
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
      rack->setEnabled(val);

      if (track->type() != Track::AUDIO_SOFTSYNTH)
            stereo->setEnabled(val);
//TD      label->setEnabled(val);
      if (solo)
            solo->setEnabled(val);
      if (mute)
            mute->setEnabled(val);
      if (iR)
            iR->setEnabled(val);
      if (oR)
            oR->setEnabled(val);
      if (off) {
            off->setChecked(track->off());
            }
      }

//---------------------------------------------------------
//   preToggled
//---------------------------------------------------------

void AudioStrip::preToggled(bool val)
      {
      audio->msgSetPrefader((AudioTrack*)track, val);
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
//   auxChanged
//---------------------------------------------------------

void AudioStrip::auxChanged(float v, int idx)
      {
      if (auxValue[idx] != v) {
            CVal val;
            val.f = v;
            song->setControllerVal(track, AC_AUX+idx, val);
            auxValue[idx] = v;
            }
      }

//---------------------------------------------------------
//   volumeChanged
//---------------------------------------------------------

void AudioStrip::volumeChanged(float v)
      {
      CVal val;
      val.f = v;
      song->setControllerVal(track, AC_VOLUME, val);
      }

//---------------------------------------------------------
//   setAux
//---------------------------------------------------------

void AudioStrip::setAux(float val, int idx)
      {
      auxKnob[idx]->setValue(val);
      auxLabel[idx]->setValue(val);
      auxValue[idx] = val;
      }

//---------------------------------------------------------
//   auxPressed
//---------------------------------------------------------

void AudioStrip::auxPressed(int n)
      {
      ((AudioTrack*)track)->startAutoRecord(AC_AUX+n);
      }

//---------------------------------------------------------
//   auxReleased
//---------------------------------------------------------

void AudioStrip::auxReleased(int n)
      {
      ((AudioTrack*)track)->stopAutoRecord(AC_AUX+n);
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

void AudioStrip::panChanged(float v)
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

void AudioStrip::setPan(float val)
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
//   addAuxKnob
//---------------------------------------------------------

Awl::VolKnob* AudioStrip::addAuxKnob(int id, Awl::VolEntry** dlabel)
      {
      QString label;

      Awl::VolKnob* knob = new Awl::VolKnob(this);
      knob->setToolTip(tr("aux send level"));
      knob->setRange(config.minSlider-0.1f, 10.0f);

      Awl::VolEntry* pl = new Awl::VolEntry(this);
      pl->setRange(config.minSlider, 10.0f);
      label.sprintf("Aux%d", id+1);

      knob->setFixedWidth(STRIP_WIDTH/2-2);
      knob->setFixedHeight(STRIP_WIDTH/2);

      if (dlabel)
            *dlabel = pl;
      pl->setFont(*config.fonts[1]);
      pl->setFrame(true);
      pl->setFixedWidth(STRIP_WIDTH/2-2);

      QLabel* plb = new QLabel(label, this);
      plb->setFont(*config.fonts[1]);
      plb->setFixedWidth(STRIP_WIDTH/2-2);
      plb->setAlignment(Qt::AlignCenter);

      QGridLayout* pangrid = new QGridLayout;
      pangrid->setSpacing(0);
      pangrid->setMargin(0);
      pangrid->addWidget(plb, 0, 0);
      pangrid->addWidget(pl, 1, 0);
      pangrid->addWidget(knob, 0, 1, 2, 1);
      layout->addLayout(pangrid);

      knob->setId(id);

      connect(knob, SIGNAL(sliderPressed(int)),  SLOT(auxPressed(int)));
      connect(knob, SIGNAL(sliderReleased(int)), SLOT(auxReleased(int)));
      connect(knob, SIGNAL(valueChanged(float, int)), SLOT(auxChanged(float, int)));
      connect(pl,   SIGNAL(valueChanged(float, int)), SLOT(auxChanged(float, int)));
      return knob;
      }

//---------------------------------------------------------
//   addPanKnob
//---------------------------------------------------------

Awl::PanKnob* AudioStrip::addPanKnob(Awl::PanEntry** dlabel)
      {
      Awl::PanKnob* knob = new Awl::PanKnob(this);
      knob->setToolTip(tr("panorama"));
      knob->setFixedSize(buttonSize.width(), entrySize.height() * 2);

      Awl::PanEntry* pl  = new Awl::PanEntry(this);
      pl->setPrecision(2);

      if (dlabel)
            *dlabel = pl;
      pl->setFont(*config.fonts[1]);
      pl->setFrame(true);

      pl->setFixedSize(entrySize);

      QLabel* plb = new QLabel;
      plb->setText(tr("Pan"));

      plb->setFont(*config.fonts[1]);
      plb->setFixedSize(entrySize);
      plb->setAlignment(Qt::AlignCenter);

      QGridLayout* pangrid = new QGridLayout;
      pangrid->setMargin(0);
      pangrid->setSpacing(0);
      pangrid->addWidget(plb, 0, 0);
      pangrid->addWidget(pl, 1, 0);
      pangrid->addWidget(knob, 0, 1, 2, 1);
      layout->addLayout(pangrid);

      connect(knob, SIGNAL(valueChanged(float,int)), SLOT(panChanged(float)));
      connect(pl,   SIGNAL(valueChanged(float,int)), SLOT(panChanged(float)));
      connect(knob, SIGNAL(sliderPressed(int)), SLOT(panPressed()));
      connect(knob, SIGNAL(sliderReleased(int)), SLOT(panReleased()));
      return knob;
      }

//---------------------------------------------------------
//   AudioStrip
//---------------------------------------------------------

AudioStrip::~AudioStrip()
      {
      }

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
      //    plugin rack
      //---------------------------------------------------

      rack = new EffectRack(this, t);
      rack->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
      rack->setFixedHeight(rack->sizeHint().height() + 2);
      layout->addWidget(rack);

      //---------------------------------------------------
      //    mono/stereo  pre/post
      //---------------------------------------------------

      QHBoxLayout* ppBox = new QHBoxLayout;
      stereo  = newStereoButton(this);
      stereo->setChecked(channel == 2);
      stereo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
      stereo->setFixedHeight(LABEL_HEIGHT);
      connect(stereo, SIGNAL(clicked(bool)), SLOT(stereoToggled(bool)));

      pre = new QToolButton;
      pre->setFont(*config.fonts[1]);
      pre->setCheckable(true);
      pre->setText(tr("Pre"));
      pre->setToolTip(tr("pre fader - post fader"));
      pre->setChecked(t->prefader());
      pre->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
      pre->setFixedHeight(LABEL_HEIGHT);
      connect(pre, SIGNAL(clicked(bool)), SLOT(preToggled(bool)));

      ppBox->addWidget(stereo);
      ppBox->addWidget(pre);
      layout->addLayout(ppBox);

      //---------------------------------------------------
      //    aux send
      //---------------------------------------------------

      int auxsSize = song->auxs()->size();
      if (t->hasAuxSend()) {
            for (int idx = 0; idx < auxsSize; ++idx) {
                  Awl::VolEntry* al;
                  Awl::VolKnob* ak = addAuxKnob(idx, &al);
                  auxKnob.push_back(ak);
                  auxLabel.push_back(al);
                  auxValue.push_back(0.0f);
                  float as = t->getController(AC_AUX+idx)->curVal().f;
                  ak->setValue(as);
                  al->setValue(as);
                  }
            }
      else if (auxsSize && _align)
            layout->addSpacing((STRIP_WIDTH/2 + 2) * auxsSize);

      //---------------------------------------------------
      //    slider, label
      //---------------------------------------------------

      slider = new Awl::MeterSlider(this);
      slider->setRange(config.minSlider-0.1f, 10.0f);
      slider->setFixedWidth(60);
      slider->setChannel(channel);
      Ctrl* ctrl = t->getController(AC_VOLUME);
      float vol = 0.0;
      if (ctrl)
            vol = ctrl->curVal().f;
      slider->setValue(vol);
      layout->addWidget(slider, 100, Qt::AlignRight);

      sl = new Awl::VolEntry(this);
      sl->setFont(*config.fonts[1]);
      sl->setSuffix(tr("dB"));
      sl->setFrame(true);
      sl->setValue(vol);

      connect(slider, SIGNAL(valueChanged(float,int)), SLOT(volumeChanged(float)));
      connect(sl,     SIGNAL(valueChanged(float,int)), SLOT(volumeChanged(float)));

      connect(slider, SIGNAL(sliderPressed(int)), SLOT(volumePressed()));
      connect(slider, SIGNAL(sliderReleased(int)), SLOT(volumeReleased()));
      connect(slider, SIGNAL(meterClicked()), SLOT(resetPeaks()));
      layout->addWidget(sl);

      //---------------------------------------------------
      //    pan, balance
      //---------------------------------------------------

      pan = addPanKnob(&panl);
      float panv = t->getController(AC_PAN)->curVal().f;
      pan->setValue(panv);
      panl->setValue(panv);

      //---------------------------------------------------
      //    mute, solo, record
      //---------------------------------------------------

      Track::TrackType type = t->type();

      QHBoxLayout* smBox1 = new QHBoxLayout(0);
      QHBoxLayout* smBox2 = new QHBoxLayout(0);

      mute  = newMuteButton(this);
      mute->setChecked(t->mute());
      mute->setFixedSize(buttonSize);
      connect(mute, SIGNAL(clicked(bool)), SLOT(muteToggled(bool)));
      connect(t, SIGNAL(muteChanged(bool)), mute, SLOT(setChecked(bool)));
      smBox2->addWidget(mute);

      solo  = newSoloButton(this);
      solo->setDisabled(true);
      solo->setFixedSize(buttonSize);
      smBox2->addWidget(solo);
      connect(solo, SIGNAL(clicked(bool)), SLOT(soloToggled(bool)));
      connect(t, SIGNAL(soloChanged(bool)), solo, SLOT(setChecked(bool)));

      off  = newOffButton(this);
      off->setFixedSize(buttonSize);
      off->setChecked(t->off());
      connect(off, SIGNAL(clicked(bool)), SLOT(offToggled(bool)));
      connect(t, SIGNAL(offChanged(bool)), this, SLOT(updateOffState()));

      smBox1->addWidget(off);

      if (track->canRecord()) {
            record = newRecordButton(this);
            record->setFixedSize(buttonSize);
            if (type == Track::AUDIO_OUTPUT)
                  record->setToolTip(tr("record downmix"));
            record->setChecked(t->recordFlag());
            connect(record, SIGNAL(clicked(bool)), SLOT(recordToggled(bool)));
            connect(t, SIGNAL(recordChanged(bool)), record, SLOT(setChecked(bool)));
            smBox1->addWidget(record);
            }
      else {
            record = 0;
            smBox1->addStretch(100);
            }

      layout->addLayout(smBox1);
      layout->addLayout(smBox2);

      //---------------------------------------------------
      //    automation read write
      //---------------------------------------------------

      addAutomationButtons();

      //---------------------------------------------------
      //    routing
      //---------------------------------------------------

      QHBoxLayout* rBox = new QHBoxLayout(0);
//      if (type != Track::AUDIO_AUX && type != Track::AUDIO_SOFTSYNTH) {
      if (type != Track::AUDIO_AUX) {
            iR = new QToolButton(this);
            iR->setFont(*config.fonts[1]);
            iR->setFixedWidth((STRIP_WIDTH-4)/2);
            iR->setText(tr("iR"));
            iR->setCheckable(false);
            iR->setToolTip(tr("input routing"));
            rBox->addWidget(iR);
            connect(iR, SIGNAL(pressed()), SLOT(iRoutePressed()));
            }
      else 
            rBox->addSpacing((STRIP_WIDTH-4)/2);
      oR = new QToolButton(this);
      oR->setFont(*config.fonts[1]);
      oR->setFixedWidth((STRIP_WIDTH-4)/2);
      oR->setText(tr("oR"));
      oR->setCheckable(false);
      oR->setToolTip(tr("output routing"));
      rBox->addWidget(oR);
      connect(oR, SIGNAL(pressed()), SLOT(oRoutePressed()));

      layout->addLayout(rBox);

      if (off) {
            updateOffState();   // init state
            }
      connect(heartBeatTimer, SIGNAL(timeout()), SLOT(heartBeat()));
      connect(song, SIGNAL(songChanged(int)), SLOT(songChanged(int)));
      connect(track, SIGNAL(controllerChanged(int)), SLOT(controllerChanged(int)));
      connect(track, SIGNAL(autoReadChanged(bool)), SLOT(autoChanged()));
      connect(track, SIGNAL(autoWriteChanged(bool)), SLOT(autoChanged()));
      autoChanged();
      }

//---------------------------------------------------------
//   addAuxPorts
//---------------------------------------------------------

static void addAuxPorts(AudioTrack* t, QMenu* lb, RouteList* r)
      {
      AuxList* al = song->auxs();
      for (iAudioAux i = al->begin(); i != al->end(); ++i) {
            Track* track = *i;
            if (t == track)
                  continue;
            QString s(track->name());
            QAction* it = lb->addAction(s); //int it = lb->insertItem(s);
            for (iRoute ir = r->begin(); ir != r->end(); ++ir) {
                  if (ir->type == 0 && ir->track == track) {
                        it->setCheckable(true); //lb->setItemChecked(it, true);
                        it->setChecked(true);
                        break;
                        }
                  }
            }
      }

//---------------------------------------------------------
//   addInPorts
//---------------------------------------------------------

static void addInPorts(AudioTrack* t, QMenu* lb, RouteList* r)
      {
      InputList* al = song->inputs();
      for (iAudioInput i = al->begin(); i != al->end(); ++i) {
            Track* track = *i;
            if (t == track)
                  continue;
            QString s(track->name());
            QAction* it = lb->addAction(s); //int it = lb->insertItem(s);
            for (iRoute ir = r->begin(); ir != r->end(); ++ir) {
                  if (ir->type == 0 && ir->track == track) {
                        it->setCheckable(true); //lb->setItemChecked(it, true);
                        it->setChecked(true);
                        break;
                        }
                  }
            }
      }

//---------------------------------------------------------
//   addOutPorts
//---------------------------------------------------------

static void addOutPorts(AudioTrack* t, QMenu* lb, RouteList* r)
      {
      OutputList* al = song->outputs();
      for (iAudioOutput i = al->begin(); i != al->end(); ++i) {
            Track* track = *i;
            if (t == track)
                  continue;
            QString s(track->name());
            QAction* it = lb->addAction(s); //int it = lb->insertItem(s);
            for (iRoute ir = r->begin(); ir != r->end(); ++ir) {
                  if (ir->type == 0 && ir->track == track) {
                        it->setCheckable(true); //lb->setItemChecked(it, true);
                        it->setChecked(true);
                        break;
                        }
                  }
            }
      }

//---------------------------------------------------------
//   addGroupPorts
//---------------------------------------------------------

static void addGroupPorts(AudioTrack* t, QMenu* lb, RouteList* r)
      {
      GroupList* al = song->groups();
      for (iAudioGroup i = al->begin(); i != al->end(); ++i) {
            Track* track = *i;
            if (t == track)
                  continue;
            QString s(track->name());
            QAction* it = lb->addAction(s); //int it = lb->insertItem(s);
            for (iRoute ir = r->begin(); ir != r->end(); ++ir) {
                  if (ir->type == 0 && ir->track == track) {
                        it->setCheckable(true); //lb->setItemChecked(it, true);
                        it->setChecked(true);
                        break;
                        }
                  }
            }
      }

//---------------------------------------------------------
//   addWavePorts
//---------------------------------------------------------

static void addWavePorts(AudioTrack* t, QMenu* lb, RouteList* r)
      {
      WaveTrackList* al = song->waves();
      for (iWaveTrack i = al->begin(); i != al->end(); ++i) {
            Track* track = *i;
            if (t == track)
                  continue;
            QString s(track->name());
            QAction* it = lb->addAction(s);             //int it = lb->insertItem(s);
            for (iRoute ir = r->begin(); ir != r->end(); ++ir) {
                  if (ir->type == 0 && ir->track == track) {
                        it->setCheckable(true);                        //lb->setItemChecked(it, true);
                        it->setChecked(true);
                        break;
                        }
                  }
            }
      }

//---------------------------------------------------------
//   addMidiOutPorts
//---------------------------------------------------------

static void addMidiOutPorts(Track* t, QMenu* lb, RouteList* r)
      {
      MidiOutPortList* al = song->midiOutPorts();
      for (iMidiOutPort i = al->begin(); i != al->end(); ++i) {
            Track* track = *i;
            if (t == track)
                  continue;
            QString s(track->name());
            QAction* it = lb->addAction(s);//int it = lb->insertItem(s);
            Route route(track, -1, Route::TRACK);
            for (iRoute ir = r->begin(); ir != r->end(); ++ir) {
                  if (*ir == route) {
                        it->setCheckable(true);//lb->setItemChecked(it, true);
                        it->setChecked(true);
                        break;
                        }
                  }
            }
      }

//---------------------------------------------------------
//   addMidiInPorts
//---------------------------------------------------------

static void addMidiInPorts(Track* t, QMenu* lb, RouteList* r)
      {
      MidiInPortList* al = song->midiInPorts();
      for (iMidiInPort i = al->begin(); i != al->end(); ++i) {
            Track* track = *i;
            if (t == track)
                  continue;
            QString s(track->name());
            QAction* it = lb->addAction(s); //int it = lb->insertItem(s);
            Route route(track, -1, Route::TRACK);
            for (iRoute ir = r->begin(); ir != r->end(); ++ir) {
                  if (*ir == route) {
                        it->setCheckable(true); //lb->setItemChecked(it, true);
                        it->setChecked(true);
                        break;
                        }
                  }
            }
      }

//---------------------------------------------------------
//   addSyntiPorts
//---------------------------------------------------------

static void addSyntiPorts(AudioTrack* t, QMenu* lb, RouteList* r)
      {
      SynthIList* al = song->syntis();
      for (iSynthI i = al->begin(); i != al->end(); ++i) {
            Track* track = *i;
            if (t == track)
                  continue;
            QString s(track->name());
            QAction* it = lb->addAction(s); //int it = lb->insertItem(s);
            Route route(track, -1, Route::SYNTIPORT);

            for (iRoute ir = r->begin(); ir != r->end(); ++ir) {
                  if (*ir == route) {
                        it->setCheckable(true);//lb->setItemChecked(it, true);
                        it->setChecked(true);
                        break;
                        }
                  }
            }
      }

//---------------------------------------------------------
//   iRoutePressed
//---------------------------------------------------------

void AudioStrip::iRoutePressed()
      {
      QMenu* pup     = new QMenu(iR);
      AudioTrack* t  = (AudioTrack*)track;
      RouteList* irl = t->inRoutes();

      switch(track->type()) {
            default:
                  delete pup;
                  return;
            case Track::AUDIO_INPUT:
                  {
                  int gid = 0;
                  for (int i = 0; i < channel; ++i) {
                        char buffer[128];
                        snprintf(buffer, 128, "%s %d", tr("Channel").toLatin1().data(), i+1);
                        pup->addSeparator()->setText(QString(buffer));

                        std::list<PortName>* ol = audioDriver->outputPorts();
                        for (std::list<PortName>::iterator ip = ol->begin(); ip != ol->end(); ++ip) {
                              QAction* id = pup->addAction(ip->name);
                              id->setData(QVariant(gid * 16 + i));
                              Route src(ip->name, i, Route::AUDIOPORT);
                              ++gid;
                              for (iRoute ir = irl->begin(); ir != irl->end(); ++ir) {
                                    if (*ir == src) {
                                          id->setCheckable(true);
                                          id->setChecked(true);
                                          break;
                                          }
                                    }
                              }
                        delete ol;
//                        if (i+1 != channel)
//                              pup->addSeparator();
                        }
                  }
                  break;
            case Track::AUDIO_OUTPUT:
                  addWavePorts(t, pup, irl);
                  addInPorts(t, pup, irl);
                  addGroupPorts(t, pup, irl);
                  addAuxPorts(t, pup, irl);
                  addSyntiPorts(t, pup, irl);
                  break;
            case Track::WAVE:
                  addInPorts(t, pup, irl);
                  break;
            case Track::AUDIO_GROUP:
                  addWavePorts(t, pup, irl);
                  addInPorts(t, pup, irl);
                  addGroupPorts(t, pup, irl);
                  addSyntiPorts(t, pup, irl);
                  break;
            case Track::AUDIO_SOFTSYNTH:
                  addMidiOutPorts(t, pup, irl);
                  addMidiInPorts(t, pup, irl);
                  break;
            }
      QAction* n = pup->exec(QCursor::pos());
      if (n != 0) {
            int id = n->data().toInt();
            QString s(n->text());
            Route srcRoute, dstRoute;

            if (track->type() == Track::AUDIO_INPUT) {
                  srcRoute = Route(s, id & 0xf, Route::AUDIOPORT);
                  dstRoute = Route(t, id & 0xf, Route::TRACK);
                  }
            else if (track->type() == Track::AUDIO_SOFTSYNTH) {
                  srcRoute = Route(s, -1, Route::TRACK);
                  dstRoute = Route(t, -1, Route::SYNTIPORT);
                  }
            else {
                  srcRoute = Route(s, -1, Route::TRACK);
                  dstRoute = Route(t, -1, Route::TRACK);
                  }
            iRoute iir = irl->begin();
            for (; iir != irl->end(); ++iir) {
                  if (*iir == srcRoute)
                        break;
                  }
            if (iir != irl->end()) {
                  // disconnect
                  audio->msgRemoveRoute(srcRoute, dstRoute);
                  }
            else {
                  // connect
                  audio->msgAddRoute(srcRoute, dstRoute);
                  }
            song->update(SC_ROUTE);
            }
      delete pup;
      iR->setDown(false);     // pup->exec() catches mouse release event
      }

//---------------------------------------------------------
//   oRoutePressed
//---------------------------------------------------------

void AudioStrip::oRoutePressed()
      {
      QMenu* pup = new QMenu(oR);
      AudioTrack* t = (AudioTrack*)track;
      RouteList* orl = t->outRoutes();

      switch(track->type()) {
            default:
                  delete pup;
                  return;
            case Track::AUDIO_OUTPUT:
                  {
                  int gid = 0;
                  for (int i = 0; i < channel; ++i) {
                        char buffer[128];
                        snprintf(buffer, 128, "%s %d", tr("Channel").toLatin1().data(), i+1);
                        pup->addSeparator()->setText(QString(buffer));

                        std::list<PortName>* ol = audioDriver->inputPorts();
                        for (std::list<PortName>::iterator ip = ol->begin(); ip != ol->end(); ++ip) {
                              QAction* action = pup->addAction(ip->name);
                              action->setData(QVariant(gid * 16 + i));
                              Route dst(ip->name, i, Route::AUDIOPORT);
                              ++gid;
                              for (iRoute ir = orl->begin(); ir != orl->end(); ++ir) {
                                    if (*ir == dst) {
                                          action->setCheckable(true);
                                          action->setChecked(true);
                                          break;
                                          }
                                    }
                              }
                        delete ol;
//                        if (i+1 != channel)
//                              pup->addSeparator();
                        }
                  }
                  break;
            case Track::AUDIO_INPUT:
                  addWavePorts(t, pup, orl);
            case Track::WAVE:
            case Track::AUDIO_GROUP:
            case Track::AUDIO_SOFTSYNTH:
                  addOutPorts(t, pup, orl);
                  addGroupPorts(t, pup, orl);
                  break;
            case Track::AUDIO_AUX:
                  addOutPorts(t, pup, orl);
                  break;
            }
      QAction* n = pup->exec(QCursor::pos());
      if (n != 0) {
            QString s(n->text());
            Route srcRoute(t, -1, track->type() == Track::AUDIO_SOFTSYNTH
               ? Route::SYNTIPORT : Route::TRACK);
            Route dstRoute(s, -1, track->type() == Track::AUDIO_OUTPUT
               ? Route::AUDIOPORT : Route::TRACK);

            if (track->type() == Track::AUDIO_OUTPUT) {
                  QVariant data = n->data();
                  srcRoute.channel = dstRoute.channel = data.toInt() & 0xf; //n & 0xf;
                  }


            // check if route src->dst exists:
            iRoute iorl = orl->begin();
            for (; iorl != orl->end(); ++iorl) {
                  if (*iorl == dstRoute)
                        break;
                  }
            if (iorl != orl->end()) {
                  // disconnect if route exists
                  audio->msgRemoveRoute(srcRoute, dstRoute);
                  }
            else {
                  // connect if route does not exist
                  audio->msgAddRoute(srcRoute, dstRoute);
                  }
            song->update(SC_ROUTE);
            }
      delete pup;
      oR->setDown(false);     // pup->exec() catches mouse release event
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
      float val = track->getController(id)->curVal().f;
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
            case AC_AUX...AC_AUX+NUM_AUX:
                  {
                  int idx = id - AC_AUX;
                  auxKnob[idx]->setValue(val);
                  auxLabel[idx]->setValue(val);
                  auxValue[idx] = val;
                  }
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
      if (((AudioTrack*)track)->hasAuxSend()) {
            int auxsSize = song->auxs()->size();
            for (int idx = 0; idx < auxsSize; ++idx) {
                  auxKnob[idx]->setEnabled(ec);
                  auxLabel[idx]->setEnabled(ec);
                  }
            }
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
