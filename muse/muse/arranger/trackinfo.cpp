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
      QLabel* l1 = new QLabel(tr("Transp."));
      QLabel* l2 = new QLabel(tr("Delay"));
      QLabel* l3 = new QLabel(tr("Length"));
      QLabel* l4 = new QLabel(tr("Velocity"));
      QLabel* l5 = new QLabel(tr("Compr."));

      transposition = new QSpinBox;
      transposition->setRange(-127, 127);
      transposition->setToolTip(tr("Pitch Transpose"));

      delay = new QSpinBox;
      delay->setRange(-1000, 1000);
      delay->setToolTip(tr("Track Delay (ticks)"));

      length = new QSpinBox;
      length->setRange(25, 200);
      length->setToolTip(tr("Modify Note Length"));
      length->setSuffix("%");

      velocity = new QSpinBox;
      velocity->setRange(-127, 127);
      velocity->setToolTip(tr("Modify Note Velocity"));

      compression = new QSpinBox;
      compression->setRange(25, 200);
      compression->setSuffix("%");
      compression->setToolTip(tr("Compress Note Velocity"));

      grid->addWidget(transposition, 2, 0);
      grid->addWidget(l1,  2, 1);

      grid->addWidget(delay, 3, 0);
      grid->addWidget(l2,  3, 1);

      grid->addWidget(length, 4, 0);
      grid->addWidget(l3,  4, 1);

      grid->addWidget(velocity, 5, 0);
      grid->addWidget(l4,  5, 1);

      grid->addWidget(compression, 6, 0);
      grid->addWidget(l5,  6, 1);

      //
      // Midi Channel
      //
      QLabel* label = new QLabel(tr("Midi Channel"));
      label->setLineWidth(2);
      label->setFrameStyle(QFrame::Panel | QFrame::Raised);
      grid->addWidget(label,  7, 0, 1, 2);
      channel = new QComboBox;
      grid->addWidget(channel, 8, 0, 1, 2);
      patch = new QPushButton;
      patch->setText("???");
      patch->setToolTip(tr("Instrument Patch"));
      grid->addWidget(patch,  9, 0, 1, 2);
      pop = new QMenu(patch);

      //
      // Midi Out Port
      //
      label = new QLabel(tr("Midi Out Port"));
      label->setLineWidth(2);
      label->setFrameStyle(QFrame::Panel | QFrame::Raised);
      grid->addWidget(label,  10, 0, 1, 2);
      port = new QComboBox;
      grid->addWidget(port,  11, 0, 1, 2);

      instrument = new QComboBox;
      instrument->setFixedWidth(infoWidth);
      instrument->setToolTip(tr("Midi Instrument"));
      grid->addWidget(instrument,  12, 0, 1, 2);

      connect(transposition, SIGNAL(valueChanged(int)), SLOT(transpositionChanged(int)));
      connect(velocity,      SIGNAL(valueChanged(int)), SLOT(velocityChanged(int)));
      connect(delay,         SIGNAL(valueChanged(int)), SLOT(delayChanged(int)));
      connect(length,        SIGNAL(valueChanged(int)), SLOT(lenChanged(int)));
      connect(compression,   SIGNAL(valueChanged(int)), SLOT(iKomprChanged(int)));
      connect(patch,         SIGNAL(clicked()),         SLOT(patchClicked()));
      connect(channel,       SIGNAL(activated(int)),    SLOT(channelSelected(int)));
      connect(port,          SIGNAL(activated(int)),    SLOT(portSelected(int)));
      connect(instrument,    SIGNAL(activated(int)),    SLOT(instrumentSelected(int)));
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
      transposition->setValue(((MidiTrack*)track)->transposition);
      delay->setValue(((MidiTrack*)track)->delay);
      length->setValue(((MidiTrack*)track)->len);
      velocity->setValue(((MidiTrack*)track)->velocity);
      compression->setValue(((MidiTrack*)track)->compression);

      instrument->clear();
      MidiChannel* mc = ((MidiTrack*)track)->channel();
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
            if (mc && mc->port() == *i)
                  portIndex = k + 1;
            }
      if (mc) {
            MidiOutPort* op = mc->port();
            for (int i = 0; i < MIDI_CHANNELS; ++i) {
                  MidiChannel* c = op->channel(i);
                  if (mc == c)
                        channelIndex = i + 1;
                  channel->addItem(c->name(), i);
                  }
            connect(mc, SIGNAL(controllerChanged(int)), SLOT(controllerChanged(int)));
            connect(op, SIGNAL(instrumentChanged()), SLOT(instrumentChanged()));
            channel->setCurrentIndex(channelIndex);
            port->setCurrentIndex(portIndex);
            MidiInstrument* mi = op->instrument();
            int idx = 0;
            int curIdx = 0;
            for (iMidiInstrument i = midiInstruments.begin(); i != midiInstruments.end(); ++i, ++idx) {
                  instrument->addItem((*i)->iname());
                  if ((*i)->iname() == mi->iname())
                        curIdx = idx;
                  }
            instrument->setCurrentIndex(curIdx);
            autoChanged(mc, false);             // update enable
            int val = mc->ctrlVal(CTRL_PROGRAM).i;
            patch->setText(mi->getPatchName(mc->channelNo(), val));
            }
      else {
            channel->setCurrentIndex(0);
            port->setCurrentIndex(0);
            instrument->addItem("--");
            instrument->setCurrentIndex(0);
            patch->setText("--");
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
      MidiChannel* mc = ((MidiTrack*)track)->channel();
      if (mc) {
      	Route odstRoute(mc, -1, Route::TRACK);
	      audio->msgRemoveRoute(srcRoute, odstRoute);
            }

      int channel = mc ? mc->channelNo() : 0;
      MidiOutPort* mp = song->midiOutPort(portno);
      mc = mp->channel(channel);

      Route dstRoute(mc, -1, Route::TRACK);
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
      MidiChannel* mc = ((MidiTrack*)track)->channel();
      MidiOutPort* mp = mc->port();
      if (mc) {
      	Route odstRoute(mc, -1, Route::TRACK);
	      audio->msgRemoveRoute(srcRoute, odstRoute);
            }

      mc = mp->channel(ch);
      Route dstRoute(mc, -1, Route::TRACK);
      audio->msgAddRoute(srcRoute, dstRoute);

	song->update(SC_ROUTE);
      }

//---------------------------------------------------------
//   controllerChanged
//---------------------------------------------------------

void MidiTrackInfo::controllerChanged(int id)
      {
      if (id == CTRL_PROGRAM) {
            MidiChannel* mc = ((MidiTrack*)track)->channel();
            if (mc) {
                  MidiOutPort* op = mc->port();
                  MidiInstrument* mi = op->instrument();
                  int val = mc->ctrlVal(id).i;
                  patch->setText(mi->getPatchName(mc->channelNo(), val));
                  }
            }
      }

//---------------------------------------------------------
//   instrumentChanged
//---------------------------------------------------------

void MidiTrackInfo::instrumentChanged()
      {
      MidiChannel* mc = ((MidiTrack*)track)->channel();
      if (mc) {
            MidiOutPort* op = mc->port();
            MidiInstrument* mi = op->instrument();
            int idx = 0;
            for (iMidiInstrument i = midiInstruments.begin(); i != midiInstruments.end(); ++i, ++idx) {
                  if (*i == mi) {
                        instrument->setCurrentIndex(idx);
                        break;
                        }
                  }
            }
      else {
            instrument->clear();
            instrument->setCurrentIndex(0);
            }
      }

//---------------------------------------------------------
//   autoChanged
//---------------------------------------------------------

void MidiTrackInfo::autoChanged(Track* t, bool)
      {
      MidiChannel* mc = ((MidiTrack*)track)->channel();
      if (mc != t)
            return;
      bool ar = t->autoRead();
      bool aw = t->autoWrite();
      bool en = !ar || (ar && aw);
      patch->setEnabled(en);
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
      MidiChannel* mc = ((MidiTrack*)track)->channel();
      if (!mc)
            return;

      MidiOutPort* op = mc->port();
      MidiInstrument* mi = op->instrument();
      mi->populatePatchPopup(pop, 0);

      QAction* rv = pop->exec(patch->mapToGlobal(QPoint(10,5)));
      if (rv != 0) {
            CVal cval;
            cval.i = rv->data().toInt();
            song->setControllerVal(mc, CTRL_PROGRAM, cval);
            }
      }

//---------------------------------------------------------
//   instrumentSelected
//---------------------------------------------------------

void MidiTrackInfo::instrumentSelected(int n)
      {
      MidiChannel* mc = ((MidiTrack*)track)->channel();
      if (mc == 0)
            return;
      MidiOutPort* op = mc->port();
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
      instrument = new QComboBox(this);
      instrument->setFixedWidth(infoWidth);
      instrument->setToolTip(tr("Midi Device"));
      grid->addWidget(instrument,  2, 0, 1, 2);

      grid->setRowStretch(grid->rowCount(), 100);

      connect(instrument, SIGNAL(activated(int)), SLOT(instrumentSelected(int)));
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
      instrument->clear();
      for (iMidiInstrument i = midiInstruments.begin(); i != midiInstruments.end(); ++i, ++idx) {
            instrument->addItem((*i)->iname());
            if ((*i)->iname() == mi->iname())
                  curIdx = idx;
            }
      instrument->setCurrentIndex(curIdx);
      connect(t, SIGNAL(instrumentChanged()), SLOT(instrumentChanged()));
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
                  instrument->setCurrentIndex(idx);
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
      MidiChannel* mc = (MidiChannel*)t;
      MidiOutPort* op = mc->port();
      connect(op, SIGNAL(instrumentChanged()), SLOT(instrumentChanged()));
      connect(mc, SIGNAL(controllerChanged(int)), SLOT(controllerChanged(int)));
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
      MidiChannel* mc = (MidiChannel*)track;
      MidiOutPort* op = mc->port();
      op->setInstrument(midiInstruments[n]);
      }

//---------------------------------------------------------
//   instrumentChanged
//---------------------------------------------------------

void MidiChannelInfo::instrumentChanged()
      {
      MidiChannel* mc = (MidiChannel*)track;
      MidiOutPort* op = mc->port();
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
      MidiChannel* mc = (MidiChannel*)track;
      MidiOutPort* op = mc->port();
      MidiInstrument* mi = op->instrument();
      mi->populatePatchPopup(pop, 0);

      QAction* rv = pop->exec(patch->mapToGlobal(QPoint(10,5)));
      if (rv != 0) {
            CVal cval;
            cval.i = rv->data().toInt();
            song->setControllerVal(mc, CTRL_PROGRAM, cval);
            }
      }

//---------------------------------------------------------
//   controllerChanged
//---------------------------------------------------------

void MidiChannelInfo::controllerChanged(int id)
      {
      if (id == CTRL_PROGRAM) {
            MidiChannel* mc = (MidiChannel*)track;
            MidiOutPort* op = mc->port();
            MidiInstrument* mi = op->instrument();
            int val = mc->ctrlVal(id).i;
            patch->setText(mi->getPatchName(mc->channelNo(), val));
            }
      }
