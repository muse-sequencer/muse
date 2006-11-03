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

#include "muse.h"
#include "song.h"
#include "arranger/arranger.h"
#include "al/al.h"
#include "al/xml.h"
#include "midiedit/drummap.h"
#include "al/marker.h"
#include "midictrl.h"
#include "mixer/mixer.h"
#include "conf.h"
#include "midiseq.h"
#include "al/tempo.h"
#include "al/sig.h"
#include "synth.h"
#include "waveedit/waveedit.h"
#include "master/masteredit.h"
#include "midiedit/drumedit.h"
#include "midiedit/pianoroll.h"
#include "part.h"
#include "marker/markerview.h"
#include "midioutport.h"
#include "midiinport.h"
// #include "midichannel.h"

//---------------------------------------------------------
//   readPart
//---------------------------------------------------------

Part* MusE::readPart(QDomNode node)
      {
      QDomElement e = node.toElement();
      Part* part = 0;
      QString s = e.text();
      int trackIdx;
      int partIdx;
      sscanf(s.toLatin1().data(), "%d:%d", &trackIdx, &partIdx);
      TrackList* tl = song->tracks();
      Track* track = 0;
      if (trackIdx < tl->size()) {
            track = tl->at(trackIdx);
            part = track->parts()->find(partIdx);
            }
      return part;
      }

//---------------------------------------------------------
//   readToplevels
//---------------------------------------------------------

void MusE::readToplevels(QDomNode node)
      {
      PartList* pl = new PartList;

      for (;!node.isNull(); node = node.nextSibling()) {
            QDomElement e = node.toElement();
            QString tag(e.tagName());
            if (tag == "part") {
                  Part* part = readPart(node);
                  if (part)
                        pl->add(part);
                  }
            else if (tag == "PianoRoll") {
                  PianoRoll* pianoroll = new PianoRoll(pl, true);
                  connect(muse, SIGNAL(configChanged()), pianoroll, SLOT(configChanged()));
                  pianoroll->read(node);
                  pianoroll->show();
                  pl = new PartList;
                  }
            else if (tag == "DrumEdit") {
                  DrumEdit* drumEditor = new DrumEdit(pl, true);
                  connect(muse, SIGNAL(configChanged()), drumEditor, SLOT(configChanged()));
                  drumEditor->read(node);
                  drumEditor->show();
                  pl = new PartList;
                  }
            else if (tag == "ListEditor") {
//                ListEdit* listEditor = new ListEdit(0, pl);
//                listEditor->show();
//                  listEditor->readStatus(node.firstChild());
                  pl = new PartList;
                  }
            else if (tag == "MasterEdit") {
                  MasterEdit* masterEditor = new MasterEdit();
                  masterEditor->show();
                  masterEditor->read(node);
                  }
            else if (tag == "MarkerView") {
                  showMarker(true);
                  markerView->read(node);
                  }
            else if (tag == "WaveEdit") {
                  WaveEdit* waveEditor = new WaveEdit(pl, true);
                  waveEditor->show();
                  connect(muse, SIGNAL(configChanged()), waveEditor, SLOT(configChanged()));
                  waveEditor->read(node);
                  pl = new PartList;
                  }
            else
                  printf("MusE:readToplevels: unknown tag <%s>\n", e.tagName().toLatin1().data());
            }
      delete pl;
      }

//---------------------------------------------------------
//   readMarker
//---------------------------------------------------------

void Song::readMarker(QDomNode node)
      {
      AL::Marker m;
      m.read(node);
      _markerList->add(m);
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void Song::read(QDomNode node)
      {
      cloneList.clear();
      for (; !node.isNull(); node = node.nextSibling()) {
            QDomElement e = node.toElement();
            QString tag(e.tagName());
            int i = e.text().toInt();
            if (tag == "configuration")
                  ; // readConfiguration(node.firstChild());
            else if (tag == "master")
                  setMasterFlag(i);
            else if (tag == "loop")
                  setLoop(i);
            else if (tag == "punchin")
                  setPunchin(i);
            else if (tag == "punchout")
                  setPunchout(i);
            else if (tag == "record")
                  ;	// setRecord(i);
            else if (tag == "solo")
                  soloFlag = i;
            else if (tag == "recmode")
                  _recMode = i;
            else if (tag == "cycle")
                  _cycleMode = i;
            else if (tag == "click")
                  setClick(i);
            else if (tag == "quantize")
                  _quantize = i;
            else if (tag == "len")
                  _len = i;
            else if (tag == "tempolist")
                  AL::tempomap.read(node);
            else if (tag == "siglist")
                  AL::sigmap.read(node.firstChild());
            else if (tag == "miditrack") {
                  MidiTrack* track = new MidiTrack();
                  track->read(node.firstChild());
                  insertTrack0(track, -1);
                  }
            else if (tag == "drumtrack") {
                  MidiTrack* track = new MidiTrack();
                  //TODO track->setUseDrumMap(true);
                  track->read(node.firstChild());
                  insertTrack0(track, -1);
                  }
            else if (tag == "wavetrack") {
                  WaveTrack* track = new WaveTrack();
                  track->read(node.firstChild());
                  insertTrack0(track, -1);
                  }
            else if (tag == "AudioInput") {
                  AudioInput* track = new AudioInput();
                  track->read(node.firstChild());
                  insertTrack0(track,-1);
                  }
            else if (tag == "AudioOutput") {
                  AudioOutput* track = new AudioOutput();
                  track->read(node.firstChild());
                  insertTrack0(track,-1);
                  }
            else if (tag == "AudioGroup") {
                  AudioGroup* track = new AudioGroup();
                  track->read(node.firstChild());
                  insertTrack0(track,-1);
                  }
            else if (tag == "SynthI") {
                  SynthI* track = new SynthI();
                  track->read(node.firstChild());
                  // insertTrack(track, -1);
                  }
            else if (tag == "MidiOutPort") {
                  MidiOutPort* track = new MidiOutPort();
                  track->read(node.firstChild());
                  insertTrack0(track, -1);
                  }
            else if (tag == "MidiInPort") {
                  MidiInPort* track = new MidiInPort();
                  track->read(node.firstChild());
                  insertTrack0(track, -1);
                  }
            else if (tag == "MidiSynti") {
                  MidiSynti* track = new MidiSynti();
                  track->read(node.firstChild());
                  // insertTrack0(track, -1);
                  }
            else if (tag == "arranger")
                  muse->arranger->readStatus(node.firstChild());
            else if (tag == "Route")
                  readRoute(node);
            else if (tag == "marker")
                  readMarker(node);
            else if (tag == "globalPitchShift")
                  _globalPitchShift = i;
            else if (tag == "cpos") {
                  int pos = i;
                  Pos p(pos, AL::TICKS);
                  setPos(Song::CPOS, p, false, false, false);
                  }
            else if (tag == "lpos") {
                  int pos = i;
                  Pos p(pos, AL::TICKS);
                  setPos(Song::LPOS, p, false, false, false);
                  }
            else if (tag == "rpos") {
                  int pos = i;
                  Pos p(pos, AL::TICKS);
                  setPos(Song::RPOS, p, false, false, false);
                  }
            else if (tag == "Pianoroll")
                  PianoRoll::readConfiguration(node);
            else if (tag == "DrumEdit")
                  DrumEdit::readConfiguration(node);
            else if (tag == "comment")
                  _comment = e.text();
            else if (tag == "createDate")
                  _createDate = QDateTime::fromString(e.text(), Qt::ISODate);
            else if (tag == "LenInSec")
                  ;
            else
                  printf("MusE:Song: unknown tag %s\n", tag.toLatin1().data());
            }
      dirty = false;
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void Song::write(Xml& xml) const
      {
      xml.tag("song");
      xml.strTag("comment", _comment);
      xml.strTag("createDate", _createDate.toString(Qt::ISODate));
      int n = AL::tempomap.tick2frame(_len);
      xml.intTag("LenInSec", n / AL::sampleRate);

      xml.intTag("cpos", cpos());
      xml.intTag("rpos", rpos());
      xml.intTag("lpos", lpos());
      xml.intTag("master", _masterFlag);
      if (loopFlag)
            xml.intTag("loop", loopFlag);
      if (punchinFlag)
            xml.intTag("punchin", punchinFlag);
      if (punchoutFlag)
            xml.intTag("punchout", punchoutFlag);
      if (soloFlag)
            xml.intTag("solo", soloFlag);
      if (_recMode != REC_OVERDUP)
            xml.intTag("recmode", _recMode);
      if (_cycleMode != CYCLE_NORMAL)
            xml.intTag("cycle", _cycleMode);
      if (_click)
            xml.intTag("click", _click);
      if (_quantize)
            xml.intTag("quantize", _quantize);
      xml.intTag("len", _len);

      if (_globalPitchShift)
            xml.intTag("globalPitchShift", _globalPitchShift);

      cloneList.clear();

      // write tracks
      for (ciTrack i = _tracks.begin(); i != _tracks.end(); ++i) {
            // midi channels are saved as part of midi out ports
            if ((*i)->type() != Track::MIDI_CHANNEL)
                  (*i)->write(xml);
            }

      // write routing
      for (ciTrack i = _tracks.begin(); i != _tracks.end(); ++i)
            (*i)->writeRouting(xml);
      muse->arranger->writeStatus(xml);

      AL::tempomap.write(xml);
      AL::sigmap.write(xml);
      _markerList->write(xml);

      xml.etag("song");
      }

//---------------------------------------------------------
//   write
//    write song
//---------------------------------------------------------

void MusE::write(Xml& xml) const
      {
      xml.header();

      xml.tag("muse version=\"2.1\"");
      writeConfiguration(xml);

      song->write(xml);

      xml.tag("toplevels");
      foreach(QWidget* w, QApplication::topLevelWidgets()) {
            if (!w->isVisible())
                  continue;
            if (strcmp("DrumEdit", w->metaObject()->className()) == 0)
                  ((TopWin*)w)->write(xml);
            else if (strcmp("PianoRoll", w->metaObject()->className()) == 0)
                  ((TopWin*)w)->write(xml);
            else if (strcmp("MasterEdit", w->metaObject()->className()) == 0)
                  ((TopWin*)w)->write(xml);
            else if (strcmp("WaveEdit", w->metaObject()->className()) == 0)
                  ((TopWin*)w)->write(xml);
            else if (strcmp("ListEdit", w->metaObject()->className()) == 0)
                  ((TopWin*)w)->write(xml);
            else if (strcmp("MarkerView", w->metaObject()->className()) == 0)
                  ((TopWin*)w)->write(xml);
            else if (strcmp("Mixer", w->metaObject()->className()) == 0)
                  ;
            else if (strcmp("Transport", w->metaObject()->className()) == 0)
                  ;
            else if (strcmp("MusE", w->metaObject()->className()) == 0)
                  ;
            else if (strcmp("QDesktopWidget", w->metaObject()->className()) == 0)
                  ;
            else
                  printf("TopLevel <%s>\n", w->metaObject()->className());
            }
      xml.etag("toplevels");
      xml.etag("muse");
      }

