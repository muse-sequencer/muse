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
#include "arranger.h"
#include "al/al.h"
#include "al/xml.h"
#include "midiedit/drummap.h"
#include "al/marker.h"
#include "midictrl.h"
#include "conf.h"
#include "al/tempo.h"
#include "al/sig.h"
#include "synth.h"
#include "waveedit/waveedit.h"
#include "master/masteredit.h"
#include "midiedit/drumedit.h"
#include "midiedit/pianoroll.h"
#include "part.h"
#include "marker/markerview.h"
#include "liste/listedit.h"

using namespace AL;

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
      Track* track = song->tracks()->value(trackIdx);
      if (track) {
            part = track->parts()->find(partIdx);
            if (part == 0) {
                  printf("MusE::readPart(): part %d(%d)  not found in track <%s>\n",
                     partIdx, track->parts()->size(), track->name().toLatin1().data());
                  }
            }
      else {
            printf("MusE::readPart(): trackIdx >= tl->size %d > %d\n",
               trackIdx , tl->size());
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
                  else
                        printf("part not found\n");
                  }
            else if (tag == "PianoRoll") {
                  PianoRoll* pianoroll = new PianoRoll(pl, true);
//                  connect(muse, SIGNAL(configChanged()), pianoroll, SLOT(configChanged()));
                  pianoroll->read(node);
                  pianoroll->show();
                  pl = new PartList;
                  }
            else if (tag == "DrumEdit") {
                  DrumEdit* drumEditor = new DrumEdit(pl, true);
//                  connect(muse, SIGNAL(configChanged()), drumEditor, SLOT(configChanged()));
                  drumEditor->read(node);
                  drumEditor->show();
                  pl = new PartList;
                  }
            else if (tag == "ListEdit") {
                  listEditor = new ListEdit(0);
                  listEditor->show();
                  listEditor->read(node);
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
                  waveEditor->read(node);
                  waveEditor->show();
                  connect(muse, SIGNAL(configChanged()), waveEditor, SLOT(configChanged()));
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
      xml.stag("song");
      xml.tag("comment", _comment);
      xml.tag("createDate", _createDate.toString(Qt::ISODate));
      int n = AL::tempomap.tick2frame(_len);
      xml.tag("LenInSec", n / AL::sampleRate);

      xml.tag("cpos", cpos());
      xml.tag("rpos", rpos());
      xml.tag("lpos", lpos());
      xml.tag("master", _masterFlag);
      if (loopFlag)
            xml.tag("loop", loopFlag);
      if (punchinFlag)
            xml.tag("punchin", punchinFlag);
      if (punchoutFlag)
            xml.tag("punchout", punchoutFlag);
      if (soloFlag)
            xml.tag("solo", soloFlag);
      if (_recMode != REC_OVERDUP)
            xml.tag("recmode", _recMode);
      if (_cycleMode != CYCLE_NORMAL)
            xml.tag("cycle", _cycleMode);
      if (_click)
            xml.tag("click", _click);
      if (_quantize)
            xml.tag("quantize", _quantize);
      xml.tag("len", _len);

      if (_globalPitchShift)
            xml.tag("globalPitchShift", _globalPitchShift);

      cloneList.clear();

      // write tracks
      for (ciTrack i = _tracks.begin(); i != _tracks.end(); ++i)
            (*i)->write(xml);

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

      xml.stag("muse version=\"3.0\"");
      writeConfiguration(xml);

      song->write(xml);

      xml.stag("toplevels");
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

//---------------------------------------------------------
//   read20
//    read old file versions (muse < 1.0)
//---------------------------------------------------------

void Song::read20(QDomNode node)
      {
      printf("Warning: importing old muse file version\n");

      for (QDomNode n = node.firstChild(); !n.isNull(); n = n.nextSibling()) {
            QDomElement e = n.toElement();
            if (e.isNull())
                  continue;
            QString t(e.tagName());
            if (t == "configuration")
                  readConfiguration(node.firstChild());
            else if (t == "song") {
                  for (QDomNode n1 = n.firstChild(); !n1.isNull(); n1 = n1.nextSibling()) {
                        QDomElement e = n1.toElement();
                        if (e.isNull())
                              continue;
                        QString t(e.tagName());
                        int i = e.text().toInt();
                        if (t == "automation")
                              ;
                        else if (t == "cpos") {
                              int pos = i;
                              Pos p(pos, AL::TICKS);
                              setPos(Song::CPOS, p, false, false, false);
                              }
                        else if (t == "rpos") {
                              int pos = i;
                              Pos p(pos, AL::TICKS);
                              setPos(Song::RPOS, p, false, false, false);
                              }
                        else if (t == "lpos") {
                              int pos = i;
                              Pos p(pos, AL::TICKS);
                              setPos(Song::LPOS, p, false, false, false);
                              }
                        else if (t == "master")
                              setMasterFlag(i);
                        else if (t == "loop")
                              setLoop(i);
                        else if (t == "punchin")
                              setPunchin(i);
                        else if (t == "punchout")
                              setPunchout(i);
                        else if (t == "record")
                              ;
                        else if (t == "solo")
                              soloFlag = i;
                        else if (t == "type")
                              ;
                        else if (t == "recmode")
                              _recMode = i;
                        else if (t == "cycle")
                              _cycleMode = i;
                        else if (t == "click")
                              setClick(i);
                        else if (t == "quantize")
                              _quantize = i;
                        else if (t == "len")
                              _len = i;
                        else if (t == "follow")
                              ;
                        else if (t == "drummap")
                              ;
                        else if (t == "siglist")
                              AL::sigmap.read(node.firstChild());
                        else if (t == "tempolist")
                              AL::tempomap.read(node);
                        else if (t == "Route")
                              ;
                        else if (t == "AudioAux") {
                              AudioGroup* track = new AudioGroup();
                              track->read(n1.firstChild());
                              insertTrack0(track,-1);
                              }
                        else if (t == "AudioInput") {
                              AudioInput* track = new AudioInput();
                              track->read(n1.firstChild());
                              insertTrack0(track,-1);
                              }
                        else if (t == "AudioGroup") {
                              AudioGroup* track = new AudioGroup();
                              track->read(n1.firstChild());
                              insertTrack0(track,-1);
                              }
                        else if (t == "AudioOutput") {
                              AudioOutput* track = new AudioOutput();
                              track->read(n1.firstChild());
                              insertTrack0(track,-1);
                              }
                        else if (t == "wavetrack") {
                              MidiTrack* track = new MidiTrack();
                              track->read(n1.firstChild());
                              insertTrack0(track, -1);
                              }
                        else if (t == "drumtrack") {
                              MidiTrack* track = new MidiTrack();
                              track->read(n1.firstChild());
                              insertTrack0(track, -1);
                              }
                        else if (t == "miditrack") {
                              MidiTrack* track = new MidiTrack();
                              track->read(n1.firstChild());
                              insertTrack0(track, -1);
                              }
                        else
                              domError(n1);
                        }
                  }
            else if (t == "toplevels") {
                  }
            else
                  domError(n);
            }
      }

