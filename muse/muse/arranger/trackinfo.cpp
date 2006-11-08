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
#include "midioutport.h"
#include "midiinport.h"

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
            case Track::WAVE:            return new WaveTrackInfo();
            case Track::AUDIO_INPUT:     return new AudioInputInfo();
            case Track::AUDIO_SOFTSYNTH: return new SynthIInfo();
            case Track::MIDI_OUT:        return new MidiOutPortInfo();
            case Track::MIDI_IN:         return new MidiInPortInfo();
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
      track = 0;
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
      connect(song, SIGNAL(songChanged(int)), SLOT(songChanged(int)));
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
      if (t == 0)
            return;
      label->setText(track->clname());
      name->setText(track->name());
      connect(track, SIGNAL(nameChanged(const QString&)), name, SLOT(setText(const QString&)));
      }

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void TrackInfo::songChanged(int val)
      {
      if ((val & SC_ROUTE) && track)
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

      label = new QLabel;
      label->setText(tr("Midi Port"));
      label->setLineWidth(2);
      label->setFrameStyle(QFrame::Panel | QFrame::Raised);
      grid->addWidget(label, 6, 0, 1, 2);

      port = new QComboBox;
      grid->addWidget(port, 7, 0, 1, 2);
      grid->addWidget(midiPortInfo, 8, 0, 1, 2);

      pop = new QMenu(mt.patch);

      connect(mt.transposition, SIGNAL(valueChanged(int)), SLOT(transpositionChanged(int)));
      connect(mt.velocity,      SIGNAL(valueChanged(int)), SLOT(velocityChanged(int)));
      connect(mt.delay,         SIGNAL(valueChanged(int)), SLOT(delayChanged(int)));
      connect(mt.length,        SIGNAL(valueChanged(int)), SLOT(lenChanged(int)));
      connect(mt.compression,   SIGNAL(valueChanged(int)), SLOT(iKomprChanged(int)));
      connect(mt.patch,         SIGNAL(clicked()),         SLOT(patchClicked()));
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
      mt.transposition->setValue(((MidiTrack*)track)->transposition());
      mt.delay->setValue(((MidiTrack*)track)->delay());
      mt.length->setValue(((MidiTrack*)track)->len());
      mt.velocity->setValue(((MidiTrack*)track)->velocity());
      mt.compression->setValue(((MidiTrack*)track)->compression());

      mp.instrument->clear();

      channel->clear();
      channel->addItem("---", -1);
      port->clear();
      port->addItem("---", -1);
      int portIndex = 1;
      MidiTrack* midiTrack = (MidiTrack*)track;
      
      MidiOutPortList* opl = song->midiOutPorts();

      MidiOut* mo = midiTrack->midiOut();      
      for (iMidiOutPort i = opl->begin(); i != opl->end(); ++i, ++portIndex) {
      	port->addItem((*i)->name());
            if (mo == (*i))
                  port->setCurrentIndex(portIndex);
            }
      for (int ch = 0; ch < MIDI_CHANNELS; ++ch)
            channel->addItem(QString("Channel %1").arg(ch+1), ch);
      int n = midiTrack->channelNo();
      channel->setCurrentIndex(n < 0 ? 0 : n + 1);

      connect(track, SIGNAL(controllerChanged(int)), SLOT(controllerChanged(int)));
//TODO  connect(op, SIGNAL(instrumentChanged()), SLOT(instrumentChanged()));

      MidiInstrument* mi = midiTrack->instrument();
      int idx = 0;
      int curIdx = 0;
      for (iMidiInstrument i = midiInstruments.begin(); i != midiInstruments.end(); ++i, ++idx) {
            mp.instrument->addItem((*i)->iname());
            if (mi && ((*i)->iname() == mi->iname()))
                  curIdx = idx;
            }
      mp.instrument->setCurrentIndex(curIdx);
      mp.deviceId->setValue(midiTrack->deviceId());
#if 0
            autoChanged(track, false);             // update enable
            int val = track->ctrlVal(CTRL_PROGRAM).i;
            int channelno = track->channelNo();
            mt.patch->setText(mi->getPatchName(channelno, val));
            }
      else {
            channel->setCurrentIndex(0);
            port->setCurrentIndex(0);
            mp.instrument->addItem("--");
            mp.instrument->setCurrentIndex(0);
            mt.patch->setText("--");
            }
#endif 
      }

//---------------------------------------------------------
//   portSelected
//---------------------------------------------------------

void MidiTrackInfo::portSelected(int portno)
      {
      if (portno == 0)
            return;
      --portno;
#if 0 //TODOA
	Route srcRoute(track);
      MidiChannel* midic = ((MidiTrack*)track)->channel();
      if (midic) {
      	Route odstRoute(midic);
	      audio->msgRemoveRoute(srcRoute, odstRoute);
            }

      int channel = midic ? midic->channelNo() : 0;
      MidiOutPort* midip = song->midiOutPorts()->at(portno);
      midic = midip->channel(channel);

      Route dstRoute(midic);
      audio->msgAddRoute(srcRoute, dstRoute);

	song->update(SC_ROUTE);
#endif
      }

//---------------------------------------------------------
//   channelSelected
//---------------------------------------------------------

void MidiTrackInfo::channelSelected(int ch)
      {
	if (ch == 0)
      	return;
	--ch;
#if 0 //TODOA
	Route srcRoute(track);
      MidiOut* midip = ((MidiTrack*)track)->midiOut();
      MidiOutPort* midi =  
      if (midi) {
      	Route dstRoute(midic);
	      audio->msgRemoveRoute(srcRoute, dstRoute);
            }

      midic = midip->channel(ch);
      Route dstRoute(midic);
      audio->msgAddRoute(srcRoute, dstRoute);

	song->update(SC_ROUTE);
#endif
      }

//---------------------------------------------------------
//   controllerChanged
//---------------------------------------------------------

void MidiTrackInfo::controllerChanged(int id)
      {
      if (id == CTRL_PROGRAM) {
#if 0 //TODOA
            MidiOut* op = ((MidiTrack*)track)->midiOut();
            if (op) {
                  MidiInstrument* mi = op->instrument();
                  int val = midic->ctrlVal(id).i;
                  mt.patch->setText(mi->getPatchName(midic->channelNo(), val));
                  }
#endif
            }
      }

//---------------------------------------------------------
//   instrumentChanged
//---------------------------------------------------------

void MidiTrackInfo::instrumentChanged()
      {
#if 0 //TODOA
      MidiChannel* midic = ((MidiTrack*)track)->channel();
      if (midic) {
            MidiOut* op = midic->port();
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
#endif
      }

//---------------------------------------------------------
//   autoChanged
//---------------------------------------------------------

void MidiTrackInfo::autoChanged(Track* t, bool)
      {
//      MidiChannel* midic = ((MidiTrack*)track)->channel();
//      if (midic != t)
//            return;
      bool ar = t->autoRead();
      bool aw = t->autoWrite();
      bool en = !ar || (ar && aw);
      mt.patch->setEnabled(en);
      }

//---------------------------------------------------------
//   transpositionChanged
//---------------------------------------------------------

void MidiTrackInfo::transpositionChanged(int val)
      {
      ((MidiTrack*)track)->setTransposition(val);
      }

//---------------------------------------------------------
//   patchClicked
//---------------------------------------------------------

void MidiTrackInfo::patchClicked()
      {
      MidiOut* op = ((MidiTrack*)track)->midiOut();
      if (op == 0)
            return;
      MidiInstrument* mi = op->instrument();
      if (mi == 0)
            return;
      mi->populatePatchPopup(pop, 0);

      QAction* rv = pop->exec(mt.patch->mapToGlobal(QPoint(10,5)));
      if (rv != 0) {
            CVal cval;
            cval.i = rv->data().toInt();
            song->setControllerVal(track, CTRL_PROGRAM, cval);
            }
      }

//---------------------------------------------------------
//   instrumentSelected
//---------------------------------------------------------

void MidiTrackInfo::instrumentSelected(int n)
      {
      MidiOut* op = ((MidiTrack*)track)->midiOut();
      if (op == 0)
            return;
      op->setInstrument(midiInstruments[n]);
      }

//---------------------------------------------------------
//   velocityChanged
//---------------------------------------------------------

void MidiTrackInfo::velocityChanged(int val)
      {
      ((MidiTrack*)track)->setVelocity(val);
      }

//---------------------------------------------------------
//   delayChanged
//---------------------------------------------------------

void MidiTrackInfo::delayChanged(int val)
      {
      ((MidiTrack*)track)->setDelay(val);
      }

//---------------------------------------------------------
//   lenChanged
//---------------------------------------------------------

void MidiTrackInfo::lenChanged(int val)
      {
      ((MidiTrack*)track)->setLen(val);
      }

//---------------------------------------------------------
//   iKomprChanged
//---------------------------------------------------------

void MidiTrackInfo::iKomprChanged(int val)
      {
      ((MidiTrack*)track)->setCompression(val);
      }

//---------------------------------------------------------
//   deviceIdChanged
//---------------------------------------------------------

void MidiTrackInfo::deviceIdChanged(int val)
      {
      ((MidiTrack*)track)->setDeviceId(val);
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

#if 0
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
      MidiOut* op = midic->port();
//TODO      connect(op, SIGNAL(instrumentChanged()), SLOT(instrumentChanged()));
      connect(midic, SIGNAL(controllerChanged(int)), SLOT(controllerChanged(int)));
      portName->setText(op->track->name());
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
      int val = midic->ctrlVal(CTRL_PROGRAM).i;
      patch->setText(mi->getPatchName(midic->channelNo(), val));
      //
      // instrument type cannot be changed for software
      // synthesizer
      //
      instrument->setEnabled(op->track->type() != Track::AUDIO_SOFTSYNTH);
      }

//---------------------------------------------------------
//   instrumentSelected
//---------------------------------------------------------

void MidiChannelInfo::instrumentSelected(int n)
      {
      MidiChannel* midic = (MidiChannel*)track;
      MidiOut* op = midic->port();
      op->setInstrument(midiInstruments[n]);
      }

//---------------------------------------------------------
//   instrumentChanged
//---------------------------------------------------------

void MidiChannelInfo::instrumentChanged()
      {
      MidiChannel* midic = (MidiChannel*)track;
      MidiOut* op = midic->port();
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
      MidiOut* op = midic->port();
      MidiInstrument* mi = op->instrument();
      mi->populatePatchPopup(pop, 0);

      QAction* rv = pop->exec(patch->mapToGlobal(QPoint(10,5)));
      if (rv != 0) {
            CVal cval;
            cval.i = rv->data().toInt();
            song->setControllerVal(track, CTRL_PROGRAM, cval);
            }
      }

//---------------------------------------------------------
//   controllerChanged
//---------------------------------------------------------

void MidiChannelInfo::controllerChanged(int id)
      {
      if (id == CTRL_PROGRAM) {
            MidiChannel* midic = (MidiChannel*)track;
            MidiOut* op = midic->port();
            MidiInstrument* mi = op->instrument();
            int val = midic->ctrlVal(id).i;
            patch->setText(mi->getPatchName(midic->channelNo(), val));
            }
      }
#endif
