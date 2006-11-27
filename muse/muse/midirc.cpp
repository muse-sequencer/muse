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

#include "midirc.h"
#include "song.h"
#include "midi.h"

MidiRCList midiRCList;

//---------------------------------------------------------
//   isActive
//---------------------------------------------------------

bool MidiRCList::isActive(int action)
      {
      for (iMidiRC i = begin(); i != end(); ++i) {
            if (i->action == action)
                  return true;
            }
      return false;
      }

//---------------------------------------------------------
//   setAction
//---------------------------------------------------------

void MidiRCList::setAction(const MidiEvent& e, int action)
      {
      //
      //  TODO: check for already used events
      //
      for (iMidiRC i = begin(); i != end(); ++i) {
            if (i->action == action) {
//      printf("replace action %d: ", action);
//      i->event.dump();
                  i->event = e;
                  return;
                  }
            }
      push_back(MidiRC(e, action));
//      printf("add action %d: ", action);
      e.dump();
      }

//---------------------------------------------------------
//   emitAction
//---------------------------------------------------------

void MidiRCList::emitAction(int action) const
      {
//      printf("emit action %d\n", action);
      switch(action) {
            case RC_STOP:
                  song->setStop(true);
                  break;
            case RC_PLAY:
                  song->setPlay(true);
                  break;
            case RC_RECORD:
                  song->setRecord(true);
                  break;
            case RC_GOTO_LEFT_MARK:
                  song->setPos(0, song->lpos(), true, true, true);
                  break;
            }
      }

//---------------------------------------------------------
//   doAction
//    emit action associated with event e
//    return true if action found
//---------------------------------------------------------

bool MidiRCList::doAction(const MidiEvent& e)
      {
//      printf("MidiRCList::doAction ");
//      e.dump();
      for (iMidiRC i = begin(); i != end(); ++i) {
            if ((i->event.type()& 0xf0) == (e.type() & 0xf0)) {
                  //
                  // for note on events only compare pitch, not velocity
                  // ignore note off events (note on with velicity zero
                  //
                  if ((e.type() == ME_NOTEON)
                     && i->event.dataA() == e.dataA()
                     && e.dataB() != 0) {
                        emitAction(i->action);
                        return true;
                        }
                  //
                  // compare controller and controller value
                  // TODO: maybe we need a special option to
                  //       ignore the controller value
                  //
                  if ((e.type() == ME_CONTROLLER)
                     && i->event.dataA() == e.dataA()
                     && i->event.dataB() == e.dataB()) {
                        emitAction(i->action);
                        return true;
                        }
                  }
            }
      return false;
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void MidiRCList::read(QDomNode node)
      {
      int action = 0;
      MidiEvent event;
      for (node = node.firstChild(); !node.isNull(); node = node.nextSibling()) {
            QDomElement e = node.toElement();
            if (e.isNull())
                  continue;
            if (e.tagName() == "action") {
                  action = e.attribute("id", "0").toInt();
                  for (QDomNode n = node.firstChild(); !n.isNull(); n = n.nextSibling()) {
                        QDomElement e1 = node.toElement();
                        if (e1.isNull())
                              continue;
                        if (e1.tagName() == "noteOn") {
                              event.setType(ME_NOTEON);
                              event.setA(e1.attribute("pitch","0").toInt());
                              }
                        else if (e1.tagName() == "controller") {
                              event.setType(ME_CONTROLLER);
                              event.setA(e1.attribute("no","0").toInt());
                              event.setB(e1.attribute("val","0").toInt());
                              }
                        else
                              printf("MusE:midiRC:action: unknown tag %s\n", e1.tagName().toLatin1().data());
                        }
                  setAction(event, action);
                  }
            else
                  printf("MusE:midiRC: unknown tag %s\n", e.tagName().toLatin1().data());
            }
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void MidiRCList::write(Xml& xml)
      {
      xml.stag("midiRC");
      for (iMidiRC i = begin(); i != end(); ++i) {
            xml.stag("action id=\"%d\"", i->action);
            if (i->event.type() == ME_NOTEON)
                  xml.stag("noteOn pitch=\"%d\"", i->event.dataA());
            else if (i->event.type() == ME_CONTROLLER)
                  xml.stag("controller no=\"%d\" val=\"%d\"",
                     i->event.dataA(), i->event.dataB());
            else
                  printf("remote event type %d not supported\n", i->event.type());
            xml.etag("action");
            }
      xml.etag("midiRC");
      }

