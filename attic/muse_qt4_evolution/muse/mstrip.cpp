//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2002-2008 by Werner Schweer and others
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

#include "midictrl.h"
#include "mstrip.h"
#include "audio.h"
#include "song.h"
#include "mixer.h"
#include "widgets/simplebutton.h"
#include "widgets/utils.h"
#include "audiodev.h"
#include "synth.h"
#include "midirack.h"
#include "midiplugin.h"
#include "midiinport.h"
#include "midioutport.h"

#include "awl/midimslider.h"
#include "awl/midimeter.h"
#include "awl/midivolentry.h"
#include "awl/midipanentry.h"
#include "awl/midipanknob.h"
#include "awl/knob.h"

enum { KNOB_PAN, KNOB_CHOR_SEND, KNOB_VAR_SEND, KNOB_REV_SEND };

//---------------------------------------------------------
//   addMidiTracks
//    input == true
//          add routes from all possible midi tracks to input
//          route list of track(channel)
//    input == false
//          add routes to all midi tracks to output route
//          list of track(channel)
//
//    Note: midi tracks do not have channels
//---------------------------------------------------------

void addMidiTracks(QMenu* menu, Track* track, int channel, bool input)
      {
      RouteList* rl = input ? track->inRoutes() : track->outRoutes();
      RouteNode a(track, channel, RouteNode::TRACK);

      MidiTrackList* tl = song->midis();
      for (iMidiTrack i = tl->begin();i != tl->end(); ++i) {
            MidiTrack* track = *i;
            QAction* action = menu->addAction(track->name());
            action->setCheckable(true);
            RouteNode b(track);
            Route r = input ? Route(b, a) : Route(a, b);
            action->setData(QVariant::fromValue(r));
            action->setChecked(rl->indexOf(r) != -1);
            }
      }

//---------------------------------------------------------
//   addMidiInPorts
//    can only be added to input route lists
//---------------------------------------------------------

void Strip::addMidiInPorts(QMenu* menu, Track* dtrack, int channel)
      {
      RouteList* rl = dtrack->inRoutes();
      RouteNode dst(dtrack, channel, RouteNode::TRACK);

      MidiInPortList* tl = song->midiInPorts();
      for (iMidiInPort i = tl->begin();i != tl->end(); ++i) {
            MidiInPort* track = *i;
            QMenu* m = menu->addMenu(track->name());
            for (int ch = 0; ch < MIDI_CHANNELS; ++ch) {
                  QAction* a = m->addAction(QString("Channel %1").arg(ch+1));
                  a->setCheckable(true);
                  RouteNode src(track, ch, RouteNode::TRACK);
                  Route r = Route(src, dst);
                  a->setData(QVariant::fromValue(r));
                  a->setChecked(rl->indexOf(r) != -1);
                  }
            }
      }

//---------------------------------------------------------
//   addMidiOutPorts
//    can only be added to output route lists
//---------------------------------------------------------

void Strip::addMidiOutPorts(QMenu* menu, Track* strack, int channel)
      {
      RouteList* rl = strack->outRoutes();
      RouteNode src(strack, channel, RouteNode::TRACK);

      MidiOutPortList* tl = song->midiOutPorts();
      for (iMidiOutPort i = tl->begin();i != tl->end(); ++i) {
            MidiOutPort* op = *i;
            QMenu* m = menu->addMenu(op->name());
            for (int ch = 0; ch < MIDI_CHANNELS; ++ch) {
                  QAction* a = m->addAction(QString("Channel %1").arg(ch+1));
                  a->setCheckable(true);
                  RouteNode dst(op, ch, RouteNode::TRACK);
                  Route r = Route(src, dst);
                  a->setData(QVariant::fromValue(r));
                  a->setChecked(rl->indexOf(r) != -1);
                  }
            }
      }

//---------------------------------------------------------
//   addSyntiPorts
//    can only be added to output route lists
//---------------------------------------------------------

static void addSyntiPorts(QMenu* menu, Track* strack, int channel)
      {
      RouteList* rl = strack->outRoutes();
      RouteNode src(strack, channel, RouteNode::TRACK);

      SynthIList* sl = song->syntis();
      for (iSynthI i = sl->begin(); i != sl->end(); ++i) {
            SynthI* sy = *i;
            QMenu* m = menu->addMenu(sy->name());
            for (int ch = 0; ch < MIDI_CHANNELS; ++ch) {
                  QAction* a = m->addAction(QString("Channel %1").arg(ch+1));
                  a->setCheckable(true);
                  RouteNode dst(sy, ch, RouteNode::TRACK);
                  Route r = Route(src, dst);
                  a->setData(QVariant::fromValue(r));
                  a->setChecked(rl->indexOf(r) != -1);
                  }
            }
      }

//---------------------------------------------------------
//   addKnob
//---------------------------------------------------------

void MidiStrip::addKnob(int ctrl, int idx, const QString& tt, const QString& label,
   const char* slot, bool enabled, int row)
      {
      Awl::FloatEntry* dl;
      Awl::Knob* knob;

      if (idx == KNOB_PAN) {
            dl   = new Awl::MidiPanEntry(this);
            knob = new Awl::MidiPanKnob(this);
            }
      else {
            dl   = new Awl::MidiVolEntry(this);
            knob = new Awl::Knob(this);
            knob->setRange(0.0, 127.0);
            }
      knob->setId(ctrl);
      dl->setId(ctrl);
      dl->setFrame(true);

      controller[idx].knob = knob;
      knob->setToolTip(tt);
      knob->setEnabled(enabled);

      controller[idx].dl = dl;
      dl->setFixedSize(entrySize);
      dl->setEnabled(enabled);

      QLabel* lb = new QLabel(label, this);
      lb->setObjectName("knobLabel");
      controller[idx].lb = lb;
      lb->setFixedSize(entrySize);
      lb->setAlignment(Qt::AlignCenter);
      lb->setEnabled(enabled);

      grid->addWidget(lb,   row,   0);
      grid->addWidget(dl,   row+1, 0);
      grid->addWidget(knob, row,   1, 2, 1);

      connect(knob, SIGNAL(valueChanged(double,int)), slot);
      connect(dl, SIGNAL(valueChanged(double,int)), slot);
      connect(knob, SIGNAL(sliderPressed(int)), SLOT(sliderPressed(int)));
      connect(knob, SIGNAL(sliderReleased(int)), SLOT(sliderReleased(int)));
      }

//---------------------------------------------------------
//   MidiStrip
//---------------------------------------------------------

MidiStrip::MidiStrip(Mixer* m, MidiTrack* t, bool align)
   : Strip(m, t, align)
      {
      volumeTouched     = false;
      panTouched        = false;
      reverbSendTouched = false;
      variSendTouched   = false;
      chorusSendTouched = false;

      addKnob(CTRL_VARIATION_SEND, KNOB_VAR_SEND, tr("VariationSend"), tr("Var"), SLOT(ctrlChanged(double,int)), true, 1);
      addKnob(CTRL_REVERB_SEND, KNOB_REV_SEND, tr("ReverbSend"), tr("Rev"), SLOT(ctrlChanged(double,int)), true, 3);
      addKnob(CTRL_CHORUS_SEND, KNOB_CHOR_SEND, tr("ChorusSend"), tr("Cho"), SLOT(ctrlChanged(double,int)), true, 5);

      //---------------------------------------------------
      //    slider, label, meter
      //---------------------------------------------------

      slider = new Awl::MidiMeterSlider(this);
      slider->setId(CTRL_VOLUME);
      slider->setFixedWidth(40);
      grid->addWidget(slider, 7, 0, 1, 2, Qt::AlignRight);

      sl = new Awl::MidiVolEntry(this);
      sl->setId(CTRL_VOLUME);
      sl->setFixedHeight(entrySize.height());

      connect(slider, SIGNAL(valueChanged(double,int)), SLOT(ctrlChanged(double, int)));
      connect(slider, SIGNAL(sliderPressed(int)), SLOT(sliderPressed(int)));
      connect(slider, SIGNAL(sliderReleased(int)), SLOT(sliderReleased(int)));
      connect(sl,     SIGNAL(valueChanged(double,int)), SLOT(ctrlChanged(double, int)));
      grid->addWidget(sl, 8, 0, 1, 2);

      //---------------------------------------------------
      //    pan, balance
      //---------------------------------------------------

      addKnob(CTRL_PANPOT, KNOB_PAN, tr("Pan/Balance"), tr("Pan"), SLOT(ctrlChanged(double,int)), true, 9);

      //---------------------------------------------------
      //    ---   record
      //    mute, solo
      //---------------------------------------------------

      SimpleButton* monitor = newMonitorButton();
      monitor->setFixedHeight(BUTTON_HEIGHT);
      monitor->setChecked(track->monitor());
      connect(monitor, SIGNAL(clicked(bool)), SLOT(monitorToggled(bool)));
      connect(t, SIGNAL(monitorChanged(bool)), monitor, SLOT(setChecked(bool)));

      SimpleButton* record = newRecordButton();
      record->setFixedHeight(BUTTON_HEIGHT);
      record->setChecked(track->recordFlag());
      connect(record, SIGNAL(clicked(bool)), SLOT(recordToggled(bool)));
      connect(t, SIGNAL(recordChanged(bool)), record, SLOT(setChecked(bool)));

      mute  = newMuteButton();
      mute->setChecked(track->isMute());
      mute->setFixedHeight(BUTTON_HEIGHT);
      connect(mute, SIGNAL(clicked(bool)), SLOT(muteToggled(bool)));

      solo  = newSoloButton();
      solo->setFixedHeight(BUTTON_HEIGHT);
      solo->setChecked(track->solo());
      connect(solo, SIGNAL(clicked(bool)), SLOT(soloToggled(bool)));

      grid->addWidget(monitor, 11, 0);
      grid->addWidget(record, 11, 1);
      grid->addWidget(mute, 12, 0);
      grid->addWidget(solo, 12, 1);

      //---------------------------------------------------
      //    automation mode
      //---------------------------------------------------

      addAutomationButtons(13);

      //---------------------------------------------------
      //    routing
      //---------------------------------------------------

      iR = newInRouteButton();
      grid->addWidget(iR, 14, 0);
      connect(iR->menu(), SIGNAL(aboutToShow()), SLOT(iRouteShow()));
      connect(iR->menu(), SIGNAL(triggered(QAction*)), song, SLOT(routeChanged(QAction*)));

      oR = newOutRouteButton();
      grid->addWidget(oR, 14, 1);
      connect(oR->menu(), SIGNAL(aboutToShow()), SLOT(oRouteShow()));
      connect(oR->menu(), SIGNAL(triggered(QAction*)), song, SLOT(routeChanged(QAction*)));

      connect(song,  SIGNAL(songChanged(int)), SLOT(songChanged(int)));
      connect(track, SIGNAL(muteChanged(bool)), mute, SLOT(setChecked(bool)));
      connect(track, SIGNAL(soloChanged(bool)), solo, SLOT(setChecked(bool)));
      connect(track, SIGNAL(autoReadChanged(bool)),  SLOT(autoChanged()));
      connect(track, SIGNAL(autoWriteChanged(bool)), SLOT(autoChanged()));
      connect(track, SIGNAL(controllerChanged(int)), SLOT(controllerChanged(int)));
      autoChanged();
      controllerChanged(CTRL_VOLUME);
      controllerChanged(CTRL_PANPOT);
      controllerChanged(CTRL_VARIATION_SEND);
      controllerChanged(CTRL_REVERB_SEND);
      controllerChanged(CTRL_CHORUS_SEND);
      }

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void MidiStrip::songChanged(int val)
      {
      if (val & SC_TRACK_MODIFIED)
            updateLabel();
      }

//---------------------------------------------------------
//   heartBeat
//---------------------------------------------------------

void MidiStrip::heartBeat()
      {
      double a = track->meter(0); // fast_log10(track->meter(0)) * .2f;
      slider->setMeterVal(a * 0.008);
      track->setMeter(0, a * 0.8);  // hack
      }

//---------------------------------------------------------
//   controllerChanged
//---------------------------------------------------------

void MidiStrip::controllerChanged(int id)
      {
      CVal cv = track->ctrlVal(id);
      double val = double(cv.i);

      switch (id) {
            case CTRL_VOLUME:
                  if (!volumeTouched)
                        slider->setValue(val);
             	sl->setValue(val);
                  break;
            case CTRL_PANPOT:
                  if (!panTouched)
                        controller[KNOB_PAN].knob->setValue(val);
              	controller[KNOB_PAN].dl->setValue(val);
                  break;
            case CTRL_VARIATION_SEND:
                  if (!variSendTouched)
                        controller[KNOB_VAR_SEND].knob->setValue(val);
             	controller[KNOB_VAR_SEND].dl->setValue(val);
                  break;
            case CTRL_REVERB_SEND:
                  if (!reverbSendTouched)
                        controller[KNOB_REV_SEND].knob->setValue(val);
             	controller[KNOB_REV_SEND].dl->setValue(val);
                  break;
            case CTRL_CHORUS_SEND:
                  if (!chorusSendTouched)
                        controller[KNOB_CHOR_SEND].knob->setValue(val);
                  controller[KNOB_CHOR_SEND].dl->setValue(val);
                  break;
            }
      }

//---------------------------------------------------------
//   ctrlChanged
//	called when user changes controller
//---------------------------------------------------------

void MidiStrip::ctrlChanged(double val, int num)
      {
      int ival = int(val);
      CVal cval;
      cval.i = ival;
      song->setControllerVal(track, num, cval);
      }

//---------------------------------------------------------
//   sliderPressed
//---------------------------------------------------------

void MidiStrip::sliderPressed(int id)
      {
      switch (id) {
            case CTRL_VOLUME:         volumeTouched = true;     break;
            case CTRL_PANPOT:         panTouched = true;        break;
            case CTRL_VARIATION_SEND: variSendTouched = true;   break;
            case CTRL_REVERB_SEND:    reverbSendTouched = true; break;
            case CTRL_CHORUS_SEND:    chorusSendTouched = true; break;
            }
      track->startAutoRecord(id);
      }

//---------------------------------------------------------
//   sliderReleased
//---------------------------------------------------------

void MidiStrip::sliderReleased(int id)
      {
      switch (id) {
            case CTRL_VOLUME:         volumeTouched = false;     break;
            case CTRL_PANPOT:         panTouched = false;        break;
            case CTRL_VARIATION_SEND: variSendTouched = false;   break;
            case CTRL_REVERB_SEND:    reverbSendTouched = false; break;
            case CTRL_CHORUS_SEND:    chorusSendTouched = false; break;
            }
      track->stopAutoRecord(id);
      }

//---------------------------------------------------------
//   muteToggled
//---------------------------------------------------------

void MidiStrip::muteToggled(bool val)
      {
      song->setMute(track, val);
      }

//---------------------------------------------------------
//   soloToggled
//---------------------------------------------------------

void MidiStrip::soloToggled(bool val)
      {
      song->setSolo(track, val);
      }

//---------------------------------------------------------
//   autoChanged
//---------------------------------------------------------

void MidiStrip::autoChanged()
      {
      bool ar = track->autoRead();
      bool aw = track->autoWrite();

      //  controller are enabled if
      //    autoRead is off
      //    autoRead and autoWrite are on (touch mode)

      bool ec = !ar || (ar && aw);
      for (unsigned i = 0; i < sizeof(controller)/sizeof(*controller); ++i) {
            controller[i].knob->setEnabled(ec);
            controller[i].dl->setEnabled(ec);
            }
      slider->setEnabled(ec);
      sl->setEnabled(ec);
      }

//---------------------------------------------------------
//   autoReadToggled
//---------------------------------------------------------

void MidiStrip::autoReadToggled(bool val)
      {
      song->setAutoRead(track, val);
      }

//---------------------------------------------------------
//   autoWriteToggled
//---------------------------------------------------------

void MidiStrip::autoWriteToggled(bool val)
      {
      song->setAutoWrite(track, val);
      }

//---------------------------------------------------------
//   iRouteShow
//---------------------------------------------------------

void MidiStrip::iRouteShow()
      {
      QMenu* pup = iR->menu();
      pup->clear();
      addMidiInPorts(pup, track, -1); // add midi inputs to menu
      }

//---------------------------------------------------------
//   oRouteShow
//---------------------------------------------------------

void MidiStrip::oRouteShow()
      {
      QMenu* pup = oR->menu();
      pup->clear();
      addMidiOutPorts(pup, track, -1);
      addSyntiPorts(pup, track, -1);
      }

//---------------------------------------------------------
//   monitorToggled
//---------------------------------------------------------

void MidiStrip::monitorToggled(bool val)
      {
      song->setMonitor(track, val);
      }

//---------------------------------------------------------
//   recordToggled
//---------------------------------------------------------

void MidiStrip::recordToggled(bool val)
      {
      song->setRecordFlag(track, !val);
      }

//---------------------------------------------------------
//   MidiOutPortStrip
//---------------------------------------------------------

MidiOutPortStrip::MidiOutPortStrip(Mixer* m, MidiOutPort* t, bool align)
   : Strip(m, t, align)
      {
      //---------------------------------------------------
      //    plugin rack
      //---------------------------------------------------

      MidiRack* rack = new MidiRack(this, t);
      rack->setFixedHeight(rack->sizeHint().height()+2);
      grid->addWidget(rack, 1, 0, 1, 2);


      if (_align)
            grid->setRowMinimumHeight(2, STRIP_WIDTH/2);

      volumeTouched = false;

      //---------------------------------------------------
      //    slider, label, meter
      //---------------------------------------------------

      slider = new Awl::MidiMeterSlider(this);
      slider->setId(CTRL_MASTER_VOLUME);
      slider->setRange(0.0, 1024*16.0);
      slider->setFixedWidth(40);
      grid->addWidget(slider, 3, 0, 1, 2, Qt::AlignRight);

      sl = new Awl::MidiVolEntry(this);
      sl->setId(CTRL_MASTER_VOLUME);
      sl->setMax(128 * 128 - 1);
//      sl->setFont(config.fonts[1]);
      sl->setFixedHeight(entrySize.height());

      controllerChanged(CTRL_MASTER_VOLUME);

      connect(slider, SIGNAL(valueChanged(double,int)), SLOT(ctrlChanged(double, int)));
      connect(slider, SIGNAL(sliderPressed(int)), SLOT(sliderPressed(int)));
      connect(slider, SIGNAL(sliderReleased(int)), SLOT(sliderReleased(int)));
      connect(sl,     SIGNAL(valueChanged(double,int)), SLOT(ctrlChanged(double, int)));
      grid->addWidget(sl, 4, 0, 1, 2);

      //---------------------------------------------------
      //    pan, balance
      //---------------------------------------------------

      if (_align)
            grid->setRowMinimumHeight(5, entrySize.height() * 2);

      //---------------------------------------------------
      //    sync
      //    mute, solo
      //---------------------------------------------------

      sync = newSyncButton();
      sync->setFixedHeight(BUTTON_HEIGHT);
      sync->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
      sync->setChecked(((MidiOutPort*)track)->sendSync());
      grid->addWidget(sync, 6, 0, 1, 2);
      connect(sync, SIGNAL(clicked(bool)), SLOT(syncToggled(bool)));
      connect(track, SIGNAL(sendSyncChanged(bool)), sync, SLOT(setChecked(bool)));

      mute  = newMuteButton();
      mute->setChecked(track->isMute());
      mute->setFixedHeight(BUTTON_HEIGHT);
      connect(mute, SIGNAL(clicked(bool)), SLOT(muteToggled(bool)));

      solo  = newSoloButton();
      solo->setFixedHeight(BUTTON_HEIGHT);
      solo->setChecked(track->solo());
      connect(solo, SIGNAL(clicked(bool)), SLOT(soloToggled(bool)));

      grid->addWidget(mute, 7, 0);
      grid->addWidget(solo, 7, 1);

      //---------------------------------------------------
      //    automation mode
      //---------------------------------------------------

      addAutomationButtons(8);

      //---------------------------------------------------
      //    output routing
      //---------------------------------------------------

      iR = newInRouteButton();
      grid->addWidget(iR, 9, 0);
      connect(iR->menu(), SIGNAL(aboutToShow()), SLOT(iRouteShow()));
      connect(iR->menu(), SIGNAL(triggered(QAction*)), song, SLOT(routeChanged(QAction*)));

      oR = newOutRouteButton();
      grid->addWidget(oR, 9, 1);
      connect(oR->menu(), SIGNAL(aboutToShow()), SLOT(oRouteShow()));
      connect(oR->menu(), SIGNAL(triggered(QAction*)), song, SLOT(routeChanged(QAction*)));

      connect(song,  SIGNAL(songChanged(int)), SLOT(songChanged(int)));
      connect(track, SIGNAL(muteChanged(bool)), mute, SLOT(setChecked(bool)));
      connect(track, SIGNAL(soloChanged(bool)), solo, SLOT(setChecked(bool)));
      connect(track, SIGNAL(autoReadChanged(bool)), SLOT(autoChanged()));
      connect(track, SIGNAL(autoWriteChanged(bool)), SLOT(autoChanged()));
      autoChanged();
      controllerChanged(CTRL_MASTER_VOLUME);
      }

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void MidiOutPortStrip::songChanged(int val)
      {
      if (val & SC_TRACK_MODIFIED)
            updateLabel();
      }

//---------------------------------------------------------
//   heartBeat
//---------------------------------------------------------

void MidiOutPortStrip::heartBeat()
      {
      double a = track->meter(0); // fast_log10(track->meter(0)) * .2f;
      slider->setMeterVal(a * 0.008);
      track->setMeter(0, a * 0.8);  // hack
      }

//---------------------------------------------------------
//   ctrlChanged
//---------------------------------------------------------

void MidiOutPortStrip::ctrlChanged(double val, int num)
      {
      int ival = int(val);
      CVal cval;
      cval.i = ival;
      song->setControllerVal(track, num, cval);
      }

//---------------------------------------------------------
//   sliderPressed
//---------------------------------------------------------

void MidiOutPortStrip::sliderPressed(int id)
      {
      switch (id) {
            case CTRL_MASTER_VOLUME:  volumeTouched = true;     break;
            }
      track->startAutoRecord(id);
      }

//---------------------------------------------------------
//   sliderReleased
//---------------------------------------------------------

void MidiOutPortStrip::sliderReleased(int id)
      {
      switch (id) {
            case CTRL_MASTER_VOLUME: volumeTouched = false;     break;
            }
      track->stopAutoRecord(id);
      }

//---------------------------------------------------------
//   muteToggled
//---------------------------------------------------------

void MidiOutPortStrip::muteToggled(bool val)
      {
      song->setMute(track, val);
      }

//---------------------------------------------------------
//   soloToggled
//---------------------------------------------------------

void MidiOutPortStrip::soloToggled(bool val)
      {
      song->setSolo(track, val);
      }

//---------------------------------------------------------
//   autoChanged
//---------------------------------------------------------

void MidiOutPortStrip::autoChanged()
      {
      bool ar = track->autoRead();
      bool aw = track->autoWrite();

      //  controller are enabled if
      //    autoRead is off
      //    autoRead and autoWrite are on (touch mode)

      bool ec = !ar || (ar && aw);
      slider->setEnabled(ec);
      sl->setEnabled(ec);
      }

//---------------------------------------------------------
//   autoReadToggled
//---------------------------------------------------------

void MidiOutPortStrip::autoReadToggled(bool val)
      {
      song->setAutoRead(track, val);
      }

//---------------------------------------------------------
//   autoWriteToggled
//---------------------------------------------------------

void MidiOutPortStrip::autoWriteToggled(bool val)
      {
      song->setAutoWrite(track, val);
      }

//---------------------------------------------------------
//   controllerChanged
//---------------------------------------------------------

void MidiOutPortStrip::controllerChanged(int id)
      {
      if (id == CTRL_MASTER_VOLUME) {
            double val = double(track->ctrlVal(id).i);
      	if (!volumeTouched)
            	slider->setValue(val);
            sl->setValue(val);
            }
      }

//---------------------------------------------------------
//   iRouteShow
//---------------------------------------------------------

void MidiOutPortStrip::iRouteShow()
      {
      QMenu* pup = iR->menu();
      pup->clear();

      for (int ch = 0; ch < MIDI_CHANNELS; ++ch) {
            QMenu* m = pup->addMenu(QString("Channel %1").arg(ch+1));
            addMidiTracks(m, track, ch, true);
            addMidiInPorts(m, track, ch);
            }
      }

//---------------------------------------------------------
//   oRouteShow
//---------------------------------------------------------

void MidiOutPortStrip::oRouteShow()
      {
      QMenu* pup = oR->menu();
      pup->clear();
      RouteList* orl = track->outRoutes();

      QList<PortName> ol;
      //
      // add JACK midi ports to list
      //
      ol = audioDriver->inputPorts(true);
      foreach (PortName ip, ol) {
            QAction* oa = pup->addAction(ip.name);
            oa->setCheckable(true);
            RouteNode dst(ip.port, RouteNode::JACKMIDIPORT);
            Route r = Route(RouteNode(track), dst);
            oa->setData(QVariant::fromValue(r));
            oa->setChecked(orl->indexOf(r) != -1);
            }
      }

//---------------------------------------------------------
//   syncToggled
//---------------------------------------------------------

void MidiOutPortStrip::syncToggled(bool val) const
      {
      ((MidiOutPort*)track)->setSendSync(val);
      }

//---------------------------------------------------------
//   MidiInPortStrip
//---------------------------------------------------------

MidiInPortStrip::MidiInPortStrip(Mixer* m, MidiInPort* t, bool align)
   : Strip(m, t, align)
      {
      //---------------------------------------------------
      //    plugin rack
      //---------------------------------------------------

      MidiRack* rack = new MidiRack(this, t);
      rack->setFixedHeight(rack->sizeHint().height()+2);
      grid->addWidget(rack, 1, 0, 1, 2);

      //---------------------------------------------------
      //    input activity
      //---------------------------------------------------

      grid->setRowStretch(2, 100);

      QGridLayout* ag = new QGridLayout;
      ag->setMargin(4);
      ag->setSpacing(1);
      QSvgRenderer sr;
      QPainter painter;

      sr.load(QString(":/xpm/activeon.svg"));
      QSize aSize(sr.defaultSize());
      activityOn = new QPixmap(aSize);
      activityOn->fill(Qt::transparent);
      painter.begin(activityOn);
      sr.render(&painter);
      painter.end();

      sr.load(QString(":/xpm/activeoff.svg"));
      activityOff = new QPixmap(aSize);
      activityOff->fill(Qt::transparent);
      painter.begin(activityOff);
      sr.render(&painter);
      painter.end();

      for (int ch = MIDI_CHANNELS-1; ch >= 0; --ch) {
            QLabel* l = new QLabel(QString("%1").arg(ch+1));
            l->setObjectName("midiChannelLabel");
            ag->addWidget(l, ch, 0, Qt::AlignCenter);
            channelActivity[ch] = new QLabel;
            ag->addWidget(channelActivity[ch], ch, 1, Qt::AlignCenter);
            channelActivity[ch]->setPixmap(*activityOff);
            activity[ch] = 0;
            }
      grid->addLayout(ag, 3, 0, 2, Qt::AlignHCenter);

      if (_align)
            grid->setRowMinimumHeight(4, BUTTON_HEIGHT);

      //---------------------------------------------------
      //    mute, solo
      //    or
      //    record, mixdownfile
      //---------------------------------------------------

      mute  = newMuteButton();
      mute->setChecked(track->isMute());
      mute->setFixedHeight(BUTTON_HEIGHT);
      connect(mute, SIGNAL(clicked(bool)), SLOT(muteToggled(bool)));

      solo  = newSoloButton();
      solo->setFixedHeight(BUTTON_HEIGHT);
      solo->setChecked(track->solo());
      connect(solo, SIGNAL(clicked(bool)), SLOT(soloToggled(bool)));

      grid->addWidget(mute, 5, 0);
      grid->addWidget(solo, 5, 1);

      //---------------------------------------------------
      //    output routing
      //---------------------------------------------------

      if (_align)
            grid->setRowMinimumHeight(6, BUTTON_HEIGHT);

      iR = newInRouteButton();
      connect(iR->menu(), SIGNAL(aboutToShow()), SLOT(iRouteShow()));
      connect(iR->menu(), SIGNAL(triggered(QAction*)), song, SLOT(routeChanged(QAction*)));

      oR = newOutRouteButton();
      connect(oR->menu(), SIGNAL(aboutToShow()), SLOT(oRouteShow()));
      connect(oR->menu(), SIGNAL(triggered(QAction*)), song, SLOT(routeChanged(QAction*)));

      grid->addWidget(iR, 7, 0);
      grid->addWidget(oR, 7, 1);

      connect(song,  SIGNAL(songChanged(int)), SLOT(songChanged(int)));
      connect(track, SIGNAL(muteChanged(bool)), mute, SLOT(setChecked(bool)));
      connect(track, SIGNAL(soloChanged(bool)), solo, SLOT(setChecked(bool)));
      }

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void MidiInPortStrip::songChanged(int val)
      {
      if (val & SC_TRACK_MODIFIED)
            updateLabel();
      }

//---------------------------------------------------------
//   heartBeat
//---------------------------------------------------------

void MidiInPortStrip::heartBeat()
      {
      for (int i = 0; i < MIDI_CHANNELS; ++i) {
            bool isActive = inport()->checkActivity(i);
            if (activity[i] != isActive) {
                  channelActivity[i]->setPixmap(isActive ? *activityOn : *activityOff);
                  activity[i] = isActive;
                  }
            }
      }

//---------------------------------------------------------
//   muteToggled
//---------------------------------------------------------

void MidiInPortStrip::muteToggled(bool val)
      {
      song->setMute(track, val);
      }

//---------------------------------------------------------
//   soloToggled
//---------------------------------------------------------

void MidiInPortStrip::soloToggled(bool val)
      {
      song->setSolo(track, val);
      }

//---------------------------------------------------------
//   iRouteShow
//---------------------------------------------------------

void MidiInPortStrip::iRouteShow()
      {
      QMenu* pup = iR->menu();
      pup->clear();

      RouteList* irl = track->inRoutes();

      QList<PortName> ol;
      //
      // add JACK midi ports to list
      //
      ol = audioDriver->outputPorts(true);
      foreach (PortName ip, ol) {
            QAction* action = pup->addAction(ip.name);
            action->setCheckable(true);
            RouteNode src(ip.port, RouteNode::JACKMIDIPORT);
            Route r = Route(src, RouteNode(track));
            action->setData(QVariant::fromValue(r));
            action->setChecked(irl->indexOf(r) != -1);
            }
      }

//---------------------------------------------------------
//   oRouteShow
//---------------------------------------------------------

void MidiInPortStrip::oRouteShow()
      {
      QMenu* pup = oR->menu();
      pup->clear();

      for (int ch = 0; ch < MIDI_CHANNELS; ++ch) {
            QMenu* m = pup->addMenu(QString("Channel %1").arg(ch+1));
            addMidiTracks(m, track, ch, false);
            addSyntiPorts(m, track, ch);
            addMidiOutPorts(m, track, ch);
            }
      }

//---------------------------------------------------------
//   MidiSyntiStrip
//---------------------------------------------------------

MidiSyntiStrip::MidiSyntiStrip(Mixer* m, MidiSynti* t, bool align)
   : Strip(m, t, align)
      {
      if (_align)
            grid->setRowMinimumHeight(1, STRIP_WIDTH/2 * 3);

      volumeTouched = false;

      //---------------------------------------------------
      //    slider, label, meter
      //---------------------------------------------------

      slider = new Awl::MidiMeterSlider(this);
      slider->setId(CTRL_MASTER_VOLUME);
      slider->setRange(0.0, 1024*16.0);
      slider->setFixedWidth(40);
      grid->addWidget(slider, 2, 0, 1, 2, Qt::AlignRight);

      sl = new Awl::MidiVolEntry(this);
      sl->setId(CTRL_MASTER_VOLUME);
//      sl->setFont(config.fonts[1]);
      sl->setFixedHeight(entrySize.height());

      connect(slider, SIGNAL(valueChanged(double,int)), SLOT(ctrlChanged(double, int)));
      connect(slider, SIGNAL(sliderPressed(int)), SLOT(sliderPressed(int)));
      connect(slider, SIGNAL(sliderReleased(int)), SLOT(sliderReleased(int)));
      connect(sl,     SIGNAL(valueChanged(double,int)), SLOT(ctrlChanged(double, int)));
      grid->addWidget(sl, 3, 0, 1, 2);

      //---------------------------------------------------
      //    pan, balance
      //---------------------------------------------------

      if (_align)
            grid->setRowMinimumHeight(4, STRIP_WIDTH);

      //---------------------------------------------------
      //    sync
      //    mute, solo
      //---------------------------------------------------

      mute  = newMuteButton();
      mute->setChecked(track->isMute());
      mute->setFixedHeight(BUTTON_HEIGHT);
      connect(mute, SIGNAL(clicked(bool)), SLOT(muteToggled(bool)));

      solo  = newSoloButton();
      solo->setFixedHeight(BUTTON_HEIGHT);
      solo->setChecked(track->solo());
      connect(solo, SIGNAL(clicked(bool)), SLOT(soloToggled(bool)));

      grid->addWidget(mute, 5, 0);
      grid->addWidget(solo, 5, 1);

      //---------------------------------------------------
      //    automation mode
      //---------------------------------------------------

      addAutomationButtons(6);

      //---------------------------------------------------
      //    output routing
      //---------------------------------------------------

      iR = newInRouteButton();
      grid->addWidget(iR, 7, 0);
      connect(iR->menu(), SIGNAL(aboutToShow()), SLOT(iRouteShow()));
      connect(iR->menu(), SIGNAL(triggered(QAction*)), song, SLOT(routeChanged(QAction*)));

      oR = newOutRouteButton();
      grid->addWidget(oR, 7, 1);
      connect(oR->menu(), SIGNAL(aboutToShow()), SLOT(oRouteShow()));
      connect(oR->menu(), SIGNAL(triggered(QAction*)), song, SLOT(routeChanged(QAction*)));

      connect(song,  SIGNAL(songChanged(int)), SLOT(songChanged(int)));
      connect(track, SIGNAL(muteChanged(bool)), mute, SLOT(setChecked(bool)));
      connect(track, SIGNAL(soloChanged(bool)), solo, SLOT(setChecked(bool)));
      connect(track, SIGNAL(autoReadChanged(bool)), SLOT(autoChanged()));
      connect(track, SIGNAL(autoWriteChanged(bool)), SLOT(autoChanged()));
      autoChanged();
      }

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void MidiSyntiStrip::songChanged(int val)
      {
      if (val & SC_TRACK_MODIFIED)
            updateLabel();
      }

//---------------------------------------------------------
//   heartBeat
//---------------------------------------------------------

void MidiSyntiStrip::heartBeat()
      {
      double a = track->meter(0); // fast_log10(track->meter(0)) * .2f;
      slider->setMeterVal(a * 0.008);
      track->setMeter(0, a * 0.8);  // hack
      }

//---------------------------------------------------------
//   ctrlChanged
//---------------------------------------------------------

void MidiSyntiStrip::ctrlChanged(double val, int num)
      {
      int ival = int(val);
      CVal cval;
      cval.i = ival;
      song->setControllerVal(track, num, cval);
      }

//---------------------------------------------------------
//   sliderPressed
//---------------------------------------------------------

void MidiSyntiStrip::sliderPressed(int id)
      {
      switch (id) {
            case CTRL_MASTER_VOLUME:  volumeTouched = true;     break;
            }
      track->startAutoRecord(id);
      }

//---------------------------------------------------------
//   sliderReleased
//---------------------------------------------------------

void MidiSyntiStrip::sliderReleased(int id)
      {
      switch (id) {
            case CTRL_MASTER_VOLUME: volumeTouched = false;     break;
            }
      track->stopAutoRecord(id);
      }

//---------------------------------------------------------
//   muteToggled
//---------------------------------------------------------

void MidiSyntiStrip::muteToggled(bool val)
      {
      song->setMute(track, val);
      }

//---------------------------------------------------------
//   soloToggled
//---------------------------------------------------------

void MidiSyntiStrip::soloToggled(bool val)
      {
      song->setSolo(track, val);
      }

//---------------------------------------------------------
//   autoChanged
//---------------------------------------------------------

void MidiSyntiStrip::autoChanged()
      {
      bool ar = track->autoRead();
      bool aw = track->autoWrite();

      //  controller are enabled if
      //    autoRead is off
      //    autoRead and autoWrite are on (touch mode)

      bool ec = !ar || (ar && aw);
      slider->setEnabled(ec);
      sl->setEnabled(ec);
      }

//---------------------------------------------------------
//   autoReadToggled
//---------------------------------------------------------

void MidiSyntiStrip::autoReadToggled(bool val)
      {
      song->setAutoRead(track, val);
      }

//---------------------------------------------------------
//   autoWriteToggled
//---------------------------------------------------------

void MidiSyntiStrip::autoWriteToggled(bool val)
      {
      song->setAutoWrite(track, val);
      }

//---------------------------------------------------------
//   controllerChanged
//---------------------------------------------------------

void MidiSyntiStrip::controllerChanged(int id)
      {
      if (id == CTRL_MASTER_VOLUME) {
            double val = double(track->ctrlVal(id).i);
      	if (!volumeTouched)
            	slider->setValue(val);
            sl->setValue(val);
            }
      }

//---------------------------------------------------------
//   oRouteShow
//---------------------------------------------------------

void MidiSyntiStrip::oRouteShow()
      {
      QMenu* pup = oR->menu();
      pup->clear();
      addMidiOutPorts(pup, track, -1);
      }

//---------------------------------------------------------
//   iRouteShow
//---------------------------------------------------------

void MidiSyntiStrip::iRouteShow()
      {
      QMenu* pup = oR->menu();
      pup->clear();

      MidiOutPort* t = (MidiOutPort*)track;

      for (int ch = 0; ch < MIDI_CHANNELS; ++ch) {
            QMenu* m = pup->addMenu(QString("Channel %1").arg(ch+1));
            addMidiTracks(m, t, ch, false);
            addMidiInPorts(m, t, ch);
            }
      }
