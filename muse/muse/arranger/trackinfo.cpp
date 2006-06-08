//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2002-2006 by Werner Schweer and others
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

#include "arranger.h"
#include "widgets/outportcombo.h"
#include "instruments/minstrument.h"
#include "midiedit/drummap.h"
#include "midictrl.h"
#include "song.h"
#include "trackinfo.h"
#include "synth.h"
#include "tllineedit.h"
#include "audio.h"
#include "gui.h"

//---------------------------------------------------------
//   createTrackInfo
//---------------------------------------------------------

TrackInfo* Arranger::createTrackInfo()
      {
      Track::TrackType t = _curTrack->type();
      switch (t) {
            case Track::MIDI:            return new MidiTrackInfo();
            case Track::AUDIO_OUTPUT:    return new AudioOutputInfo();
            case Track::AUDIO_GROUP:     return new AudioGroupInfo();
            case Track::AUDIO_AUX:       return new AudioAuxInfo();
            case Track::WAVE:            return new WaveTrackInfo();
            case Track::AUDIO_INPUT:     return new AudioInputInfo();
            case Track::AUDIO_SOFTSYNTH: return new SynthIInfo();
            case Track::MIDI_OUT:        return new MidiOutPortInfo();
            case Track::MIDI_IN:         return new MidiInPortInfo();
            case Track::MIDI_CHANNEL:    return new MidiChannelInfo();
            case Track::MIDI_SYNTI:      return new MidiSynthIInfo();
            default:
                  printf("Arranger::createTrackInfo: type %d\n", t);
                  abort();
            }
      }

//---------------------------------------------------------
//   TrackInfo
//---------------------------------------------------------

TrackInfo::TrackInfo()
   : QWidget()
      {
      label = new QLabel;
      label->setToolTip(tr("Track Type"));
      label->setLineWidth(2);
      label->setFrameStyle(QFrame::Panel | QFrame::Raised);

      name  = new TLLineEdit("");
      name->setToolTip(tr("Track Name"));

      grid = new QGridLayout;
      grid->setMargin(0);
      grid->setSpacing(0);
      setLayout(grid);

      grid->addWidget(label, 0, 0, 1, 2);
      grid->addWidget(name,  1, 0, 1, 2);
      connect(name, SIGNAL(contentChanged(QString)), SLOT(nameChanged(QString)));
      resize(QSize(infoWidth, height()));
      }

//---------------------------------------------------------
//   nameChanged
//---------------------------------------------------------

void TrackInfo::nameChanged(QString s)
      {
      song->changeTrackName(track, s);
      name->setCursorPosition(0);
      }

//---------------------------------------------------------
//   init
//---------------------------------------------------------

void TrackInfo::init(Track* t)
      {
      track = t;
      label->setText(track->clname());
      name->setText(track->name());
      //
      //TD disconnect previous tracks
      //
      connect(track, SIGNAL(nameChanged(const QString&)), name, SLOT(setText(const QString&)));
      connect(song, SIGNAL(songChanged(int)), this, SLOT(songChanged(int)));
      }

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void TrackInfo::songChanged(int val)
      {
      if (val & SC_ROUTE)
		init(track);
      }

//---------------------------------------------------------
//   MidiTrackInfo
//---------------------------------------------------------

MidiTrackInfo::MidiTrackInfo()
   : TrackInfo()
      {
      QWidget* midiTrackInfo = new QWidget;
      mt.setupUi(midiTrackInfo);

      QWidget* midiChannelInfo = new QWidget;
      mc.setupUi(midiChannelInfo);

      QWidget* midiPortInfo = new QWidget;
      mp.setupUi(midiPortInfo);

      grid->addWidget(midiTrackInfo, 2, 0, 1, 2);

      QLabel* label = new QLabel;
      label->setText(tr("Midi Channel"));
      label->setLineWidth(2);
      label->setFrameStyle(QFrame::Panel | QFrame::Raised);
      grid->addWidget(label, 3, 0, 1, 2);

      channel = new QComboBox;
      grid->addWidget(channel, 4, 0, 1, 2);
      grid->addWidget(midiChannelInfo, 5, 0, 1, 2);

      label = new QLabel;
      label->setText(tr("Midi Port"));
      label->setLineWidth(2);
      label->setFrameStyle(QFrame::Panel | QFrame::Raised);
      grid->addWidget(label, 6, 0, 1, 2);

      port = new QComboBox;
      grid->addWidget(port, 7, 0, 1, 2);
      grid->addWidget(midiPortInfo, 8, 0, 1, 2);

      pop = new QMenu(mc.patch);

      connect(mt.transposition, SIGNAL(valueChanged(int)), SLOT(transpositionChanged(int)));
      connect(mt.velocity,      SIGNAL(valueChanged(int)), SLOT(velocityChanged(int)));
      connect(mt.delay,         SIGNAL(valueChanged(int)), SLOT(delayChanged(int)));
      connect(mt.length,        SIGNAL(valueChanged(int)), SLOT(lenChanged(int)));
      connect(mt.compression,   SIGNAL(valueChanged(int)), SLOT(iKomprChanged(int)));
      connect(mc.patch,         SIGNAL(clicked()),         SLOT(patchClicked()));
      connect(channel,          SIGNAL(activated(int)),    SLOT(channelSelected(int)));
      connect(port,             SIGNAL(activated(int)),    SLOT(portSelected(int)));
      connect(mp.instrument,    SIGNAL(activated(int)),    SLOT(instrumentSelected(int)));
      connect(mp.deviceId,      SIGNAL(valueChanged(int)), SLOT(deviceIdChanged(int)));
      connect(song,          SIGNAL(autoReadChanged(Track*,bool)),  SLOT(autoChanged(Track*,bool)));
      connect(song,          SIGNAL(autoWriteChanged(Track*,bool)), SLOT(autoChanged(Track*,bool)));
      grid->setRowStretch(grid->rowCount(), 100);
      }

//---------------------------------------------------------
//   init
//---------------------------------------------------------

void MidiTrackInfo::init(Track* t)
      {
      TrackInfo::init(t);
      mt.transposition->setValue(((MidiTrack*)track)->transposition);
      mt.delay->setValue(((MidiTrack*)track)->delay);
      mt.length->setValue(((MidiTrack*)track)->len);
      mt.velocity->setValue(((MidiTrack*)track)->velocity);
      mt.compression->setValue(((MidiTrack*)track)->compression);

      mp.instrument->clear();
      MidiChannel* midic = ((MidiTrack*)track)->channel();
      int portIndex = 0;
      int channelIndex = 0;

      channel->clear();
      channel->addItem("---", -1);
      port->clear();
      port->addItem("---", -1);
      MidiOutPortList* opl = song->midiOutPorts();
      int k = 0;
      for (iMidiOutPort i = opl->begin(); i != opl->end(); ++i, ++k) {
      	port->addItem((*i)->name(), k);
            if (midic && midic->port() == *i)
                  portIndex = k + 1;
            }
      if (midic) {
            MidiOutPort* op = midic->port();
            for (int i = 0; i < MIDI_CHANNELS; ++i) {
                  MidiChannel* c = op->channel(i);
                  if (midic == c)
                        channelIndex = i + 1;
                  channel->addItem(c->name(), i);
                  }
            connect(midic, SIGNAL(controllerChanged(int)), SLOT(controllerChanged(int)));
            connect(op, SIGNAL(instrumentChanged()), SLOT(instrumentChanged()));
            channel->setCurrentIndex(channelIndex);
            port->setCurrentIndex(portIndex);
            MidiInstrument* mi = op->instrument();
            int idx = 0;
            int curIdx = 0;
            for (iMidiInstrument i = midiInstruments.begin(); i != midiInstruments.end(); ++i, ++idx) {
                  mp.instrument->addItem((*i)->iname());
                  if ((*i)->iname() == mi->iname())
                        curIdx = idx;
                  }
            mp.instrument->setCurrentIndex(curIdx);
            mp.deviceId->setValue(op->deviceId());
            autoChanged(midic, false);             // update enable
            int val = midic->ctrlVal(CTRL_PROGRAM).i;
            int channelno = midic->channelNo();
            mc.patch->setText(mi->getPatchName(channelno, val));
            }
      else {
            channel->setCurrentIndex(0);
            port->setCurrentIndex(0);
            mp.instrument->addItem("--");
            mp.instrument->setCurrentIndex(0);
            mc.patch->setText("--");
            }
      }

//---------------------------------------------------------
//   portSelected
//---------------------------------------------------------

void MidiTrackInfo::portSelected(int portno)
      {
      if (portno == 0)
            return;
      --portno;

	Route srcRoute(track, -1, Route::TRACK);
      MidiChannel* midic = ((MidiTrack*)track)->channel();
      if (midic) {
      	Route odstRoute(midic, -1, Route::TRACK);
	      audio->msgRemoveRoute(srcRoute, odstRoute);
            }

      int channel = midic ? midic->channelNo() : 0;
      MidiOutPort* midip = song->midiOutPort(portno);
      midic = midip->channel(channel);

      Route dstRoute(midic, -1, Route::TRACK);
      audio->msgAddRoute(srcRoute, dstRoute);

	song->update(SC_ROUTE);
      }

//---------------------------------------------------------
//   channelSelected
//---------------------------------------------------------

void MidiTrackInfo::channelSelected(int ch)
      {
	if (ch == 0)
      	return;
	--ch;
	Route srcRoute(track, -1, Route::TRACK);
      MidiChannel* midic = ((MidiTrack*)track)->channel();
      MidiOutPort* midip = midic->port();
      if (midic) {
      	Route odstRoute(midic, -1, Route::TRACK);
	      audio->msgRemoveRoute(srcRoute, odstRoute);
            }

      midic = midip->channel(ch);
      Route dstRoute(midic, -1, Route::TRACK);
      audio->msgAddRoute(srcRoute, dstRoute);

	song->update(SC_ROUTE);
      }

//---------------------------------------------------------
//   controllerChanged
//---------------------------------------------------------

void MidiTrackInfo::controllerChanged(int id)
      {
      if (id == CTRL_PROGRAM) {
            MidiChannel* midic = ((MidiTrack*)track)->channel();
            if (midic) {
                  MidiOutPort* op = midic->port();
                  MidiInstrument* mi = op->instrument();
                  int val = midic->ctrlVal(id).i;
                  mc.patch->setText(mi->getPatchName(midic->channelNo(), val));
                  }
            }
      }

//---------------------------------------------------------
//   instrumentChanged
//---------------------------------------------------------

void MidiTrackInfo::instrumentChanged()
      {
      MidiChannel* midic = ((MidiTrack*)track)->channel();
      if (midic) {
            MidiOutPort* op = midic->port();
            MidiInstrument* mi = op->instrument();
            int idx = 0;
            for (iMidiInstrument i = midiInstruments.begin(); i != midiInstruments.end(); ++i, ++idx) {
                  if (*i == mi) {
                        mp.instrument->setCurrentIndex(idx);
                        break;
                        }
                  }
            }
      else {
            mp.instrument->clear();
            mp.instrument->setCurrentIndex(0);
            }
      }

//---------------------------------------------------------
//   autoChanged
//---------------------------------------------------------

void MidiTrackInfo::autoChanged(Track* t, bool)
      {
      MidiChannel* midic = ((MidiTrack*)track)->channel();
      if (midic != t)
            return;
      bool ar = t->autoRead();
      bool aw = t->autoWrite();
      bool en = !ar || (ar && aw);
      mc.patch->setEnabled(en);
      }

//---------------------------------------------------------
//   transpositionChanged
//---------------------------------------------------------

void MidiTrackInfo::transpositionChanged(int val)
      {
      ((MidiTrack*)track)->transposition = val;
      }

//---------------------------------------------------------
//   patchClicked
//---------------------------------------------------------

void MidiTrackInfo::patchClicked()
      {
      MidiChannel* midic = ((MidiTrack*)track)->channel();
      if (!midic)
            return;
      
      MidiOutPort* op = midic->port();
      MidiInstrument* mi = op->instrument();
      mi->populatePatchPopup(pop, 0);

      QAction* rv = pop->exec(mc.patch->mapToGlobal(QPoint(10,5)));
      if (rv != 0) {
            CVal cval;
            cval.i = rv->data().toInt();
            song->setControllerVal(midic, CTRL_PROGRAM, cval);
            }
      }

//---------------------------------------------------------
//   instrumentSelected
//---------------------------------------------------------

void MidiTrackInfo::instrumentSelected(int n)
      {
      MidiChannel* midic = ((MidiTrack*)track)->channel();
      if (midic == 0)
            return;
      MidiOutPort* op = midic->port();
      op->setInstrument(midiInstruments[n]);
      }

//---------------------------------------------------------
//   velocityChanged
//---------------------------------------------------------

void MidiTrackInfo::velocityChanged(int val)
      {
      ((MidiTrack*)track)->velocity = val;
      }

//---------------------------------------------------------
//   delayChanged
//---------------------------------------------------------

void MidiTrackInfo::delayChanged(int val)
      {
      ((MidiTrack*)track)->delay = val;
      }

//---------------------------------------------------------
//   lenChanged
//---------------------------------------------------------

void MidiTrackInfo::lenChanged(int val)
      {
      ((MidiTrack*)track)->len = val;
      }

//---------------------------------------------------------
//   iKomprChanged
//---------------------------------------------------------

void MidiTrackInfo::iKomprChanged(int val)
      {
      ((MidiTrack*)track)->compression = val;
      }

//---------------------------------------------------------
//   deviceIdChanged
//---------------------------------------------------------

void MidiTrackInfo::deviceIdChanged(int val)
      {
      MidiChannel* midic = ((MidiTrack*)track)->channel();
      if (midic == 0)
            return;
      MidiOutPort* op = midic->port();
      op->setDeviceId(val);
      }

//---------------------------------------------------------
//   AudioOutputInfo
//---------------------------------------------------------

AudioOutputInfo::AudioOutputInfo()
   : TrackInfo()
      {
      grid->setRowStretch(grid->rowCount(), 100);
      }

//---------------------------------------------------------
//   AudioInputInfo
//---------------------------------------------------------

AudioInputInfo::AudioInputInfo()
   : TrackInfo()
      {
      grid->setRowStretch(grid->rowCount(), 100);
      }

//---------------------------------------------------------
//   AudioGroupInfo
//---------------------------------------------------------

AudioGroupInfo::AudioGroupInfo()
   : TrackInfo()
      {
      grid->setRowStretch(grid->rowCount(), 100);
      }

//---------------------------------------------------------
//   AudioAuxInfo
//---------------------------------------------------------

AudioAuxInfo::AudioAuxInfo()
   : TrackInfo()
      {
      grid->setRowStretch(grid->rowCount(), 100);
      }

//---------------------------------------------------------
//   WaveTrackInfo
//---------------------------------------------------------

WaveTrackInfo::WaveTrackInfo()
   : TrackInfo()
      {
      grid->setRowStretch(grid->rowCount(), 100);
      }

//---------------------------------------------------------
//   SynthIInfo
//---------------------------------------------------------

SynthIInfo::SynthIInfo()
   : TrackInfo()
      {
      grid->setRowStretch(grid->rowCount(), 100);
      }

//---------------------------------------------------------
//   MidiSynthIInfo
//---------------------------------------------------------

MidiSynthIInfo::MidiSynthIInfo()
   : TrackInfo()
      {
      grid->setRowStretch(grid->rowCount(), 100);
      }

//---------------------------------------------------------
//   MidiOutPortInfo
//---------------------------------------------------------

MidiOutPortInfo::MidiOutPortInfo()
   : TrackInfo()
      {
      QWidget* midiPortInfo = new QWidget;
      mp.setupUi(midiPortInfo);
      grid->addWidget(midiPortInfo, 2, 0, 1, 2);

      grid->setRowStretch(grid->rowCount(), 100);

      connect(mp.instrument, SIGNAL(activated(int)), SLOT(instrumentSelected(int)));
      connect(mp.deviceId,   SIGNAL(valueChanged(int)), SLOT(deviceIdChanged(int)));
      }

//---------------------------------------------------------
//   init
//---------------------------------------------------------

void MidiOutPortInfo::init(Track* t)
      {
      TrackInfo::init(t);

      MidiOutPort* op = (MidiOutPort*)track;
      MidiInstrument* mi = op->instrument();
      int idx = 0;
      int curIdx = 0;
      mp.instrument->clear();
      for (iMidiInstrument i = midiInstruments.begin(); i != midiInstruments.end(); ++i, ++idx) {
            mp.instrument->addItem((*i)->iname());
            if ((*i)->iname() == mi->iname())
                  curIdx = idx;
            }
      mp.instrument->setCurrentIndex(curIdx);
      mp.deviceId->setValue(op->deviceId());
      connect(t, SIGNAL(instrumentChanged()), SLOT(instrumentChanged()));
      }

//---------------------------------------------------------
//   deviceIdChanged
//---------------------------------------------------------

void MidiOutPortInfo::deviceIdChanged(int val)
      {
      ((MidiOutPort*)track)->setDeviceId(val);
      }

//---------------------------------------------------------
//   instrumentChanged
//---------------------------------------------------------

void MidiOutPortInfo::instrumentChanged()
      {
      MidiOutPort* op = (MidiOutPort*)track;
      MidiInstrument* mi = op->instrument();
      int idx = 0;
      for (iMidiInstrument i = midiInstruments.begin(); i != midiInstruments.end(); ++i, ++idx) {
            if (*i == mi) {
                  mp.instrument->setCurrentIndex(idx);
                  break;
                  }
            }
      }

//---------------------------------------------------------
//   instrumentSelected
//---------------------------------------------------------

void MidiOutPortInfo::instrumentSelected(int n)
      {
      MidiOutPort* op = (MidiOutPort*)track;
      int idx = 0;
      for (iMidiInstrument i = midiInstruments.begin(); i != midiInstruments.end(); ++i, ++idx) {
            if (idx == n) {
                  op->setInstrument(*i);
                  break;
                  }
            }
      }

//---------------------------------------------------------
//   MidiInPortInfo
//---------------------------------------------------------

MidiInPortInfo::MidiInPortInfo()
   : TrackInfo()
      {
      grid->setRowStretch(grid->rowCount(), 100);
      }

//---------------------------------------------------------
//   MidiChannelInfo
//---------------------------------------------------------

MidiChannelInfo::MidiChannelInfo()
   : TrackInfo()
      {
      patch = new QPushButton;
      patch->setText("???");
      patch->setToolTip(tr("Instrument Patch"));
      grid->addWidget(patch,  2, 0, 1, 2);
      pop = new QMenu(patch);
      //
      // Midi Out Port
      //
      QLabel* label = new QLabel(tr("Midi Out Port"));
      label->setLineWidth(2);
      label->setFrameStyle(QFrame::Panel | QFrame::Raised);
      grid->addWidget(label,  3, 0, 1, 2);
      portName = new TLLineEdit("??");
      grid->addWidget(portName,  4, 0, 1, 2);

      instrument = new QComboBox;
      instrument->setFixedWidth(infoWidth);
      instrument->setToolTip(tr("Midi Instrument"));
      grid->addWidget(instrument,  5, 0, 1, 2);

      connect(instrument, SIGNAL(activated(int)), SLOT(instrumentSelected(int)));
      connect(patch,      SIGNAL(clicked()),      SLOT(patchClicked()));

      grid->setRowStretch(grid->rowCount(), 100);
      }

//---------------------------------------------------------
//   init
//---------------------------------------------------------

void MidiChannelInfo::init(Track* t)
      {
      TrackInfo::init(t);
      MidiChannel* midic = (MidiChannel*)t;
      MidiOutPort* op = midic->port();
      connect(op, SIGNAL(instrumentChanged()), SLOT(instrumentChanged()));
      connect(midic, SIGNAL(controllerChanged(int)), SLOT(controllerChanged(int)));
      portName->setText(op->name());
      MidiInstrument* mi = op->instrument();
      int idx = 0;
      int curIdx = 0;
      for (iMidiInstrument i = midiInstruments.begin(); i != midiInstruments.end(); ++i, ++idx) {
            instrument->addItem((*i)->iname());
            if ((*i)->iname() == mi->iname())
                  curIdx = idx;
            }
      instrument->setCurrentIndex(curIdx);
      }

//---------------------------------------------------------
//   instrumentSelected
//---------------------------------------------------------

void MidiChannelInfo::instrumentSelected(int n)
      {
      MidiChannel* midic = (MidiChannel*)track;
      MidiOutPort* op = midic->port();
      op->setInstrument(midiInstruments[n]);
      }

//---------------------------------------------------------
//   instrumentChanged
//---------------------------------------------------------

void MidiChannelInfo::instrumentChanged()
      {
      MidiChannel* midic = (MidiChannel*)track;
      MidiOutPort* op = midic->port();
      MidiInstrument* mi = op->instrument();
      int idx = 0;
      for (iMidiInstrument i = midiInstruments.begin(); i != midiInstruments.end(); ++i, ++idx) {
            if (*i == mi) {
                  instrument->setCurrentIndex(idx);
                  break;
                  }
            }
      }

//---------------------------------------------------------
//   patchClicked
//---------------------------------------------------------

void MidiChannelInfo::patchClicked()
      {
      MidiChannel* midic = (MidiChannel*)track;
      MidiOutPort* op = midic->port();
      MidiInstrument* mi = op->instrument();
      mi->populatePatchPopup(pop, 0);

      QAction* rv = pop->exec(patch->mapToGlobal(QPoint(10,5)));
      if (rv != 0) {
            CVal cval;
            cval.i = rv->data().toInt();
            song->setControllerVal(midic, CTRL_PROGRAM, cval);
            }
      }

//---------------------------------------------------------
//   controllerChanged
//---------------------------------------------------------

void MidiChannelInfo::controllerChanged(int id)
      {
      if (id == CTRL_PROGRAM) {
            MidiChannel* midic = (MidiChannel*)track;
            MidiOutPort* op = midic->port();
            MidiInstrument* mi = op->instrument();
            int val = midic->ctrlVal(id).i;
            patch->setText(mi->getPatchName(midic->channelNo(), val));
            }
      }
