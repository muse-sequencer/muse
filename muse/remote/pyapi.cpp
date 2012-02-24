//=========================================================
//  MusE
//  Linux Music Editor
//  (C) Copyright 2009 Mathias Gyllengahm (lunar_shuttle@users.sf.net)
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; version 2 of
//  the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
//=========================================================
#include <Python.h>
#include <iostream>
#include <fstream>
#include <string>
#include <pthread.h>

#include <QApplication>

#include "pyapi.h"
#include "globaldefs.h"
#include "song.h"
#include "tempo.h"
#include "track.h"
#include "audio.h"
#include "gconfig.h"
#include "midictrl.h"
#include "midiport.h"
#include "plugin.h"
#include "midi.h"
#include "app.h"

// Steals ref: PyList_SetItem, PyTuple_SetItem
using namespace std;

namespace MusECore {

static pthread_t pyapiThread;
//------------------------------------------------------------
QPybridgeEvent::QPybridgeEvent(QPybridgeEvent::EventType _type, int _p1, int _p2)
      :QEvent(QEvent::User),
      type(_type),
      p1(_p1),
      p2(_p2)
{
}
//------------------------------------------------------------
// Get current position
//------------------------------------------------------------
PyObject* getCPos(PyObject*, PyObject*)
{
      return Py_BuildValue("i", MusEGlobal::song->cpos());
}
//------------------------------------------------------------
// Get position of left locator
//------------------------------------------------------------
PyObject* getLPos(PyObject*, PyObject*)
{
      return Py_BuildValue("i", MusEGlobal::song->lpos());
}
//------------------------------------------------------------
// Get position of right locator
//------------------------------------------------------------
PyObject* getRPos(PyObject*, PyObject*)
{
      return Py_BuildValue("i", MusEGlobal::song->rpos());
}
//------------------------------------------------------------
// Start playing from current position
//------------------------------------------------------------
PyObject* startPlay(PyObject*, PyObject*)
{
      //MusEGlobal::song->setPlay(true);
      QPybridgeEvent* pyevent = new QPybridgeEvent(QPybridgeEvent::SONG_SETPLAY);
      QApplication::postEvent(MusEGlobal::song, pyevent);
      Py_INCREF(Py_None);
      return Py_None;
}
//------------------------------------------------------------
// Stop playing
//------------------------------------------------------------
PyObject* stopPlay(PyObject*, PyObject*)
{
      //MusEGlobal::song->setStop(true);
      QPybridgeEvent* pyevent = new QPybridgeEvent(QPybridgeEvent::SONG_SETSTOP);
      QApplication::postEvent(MusEGlobal::song, pyevent);
      Py_INCREF(Py_None);
      return Py_None;
}
//------------------------------------------------------------
// Rewind to start
//------------------------------------------------------------
PyObject* rewindStart(PyObject*, PyObject*)
{
      //MusEGlobal::song->rewindStart();
      QPybridgeEvent* pyevent = new QPybridgeEvent(QPybridgeEvent::SONG_REWIND);
      QApplication::postEvent(MusEGlobal::song, pyevent);
      Py_INCREF(Py_None);
      return Py_None;
}
//------------------------------------------------------------
// Get tempo at specific position
//------------------------------------------------------------
PyObject* getTempo(PyObject*, PyObject* args)
{
      int tick;
      if (!PyArg_ParseTuple(args, "i", &tick)) {
            return Py_BuildValue("i", 1000);
            }

      int tempovalue = MusEGlobal::tempomap.tempo(tick);
      return Py_BuildValue("i", tempovalue);
}
//------------------------------------------------------------
// Get track names
//------------------------------------------------------------
PyObject* getTrackNames(PyObject*, PyObject*)
{
      TrackList* tracks = MusEGlobal::song->tracks();
      PyObject* res = Py_BuildValue("[]");
      for (ciTrack t = tracks->begin(); t != tracks->end(); ++t) {
            Track* track = *t;
            PyObject* ptrackname = Py_BuildValue("s", track->name().toLatin1().constData());
            PyList_Append(res, ptrackname);
            Py_DECREF(ptrackname);
            }

      return res;
}
//------------------------------------------------------------
// Find part by serial nr
//------------------------------------------------------------
Part* findPartBySerial(int sn)
{
      TrackList* tracks = MusEGlobal::song->tracks();
      for (ciTrack t = tracks->begin(); t != tracks->end(); ++t) {
            Track* track = *t;
            PartList* parts = track->parts();
            for (ciPart p = parts->begin(); p != parts->end(); p++) {
                  Part* part = p->second;
                  if (part->sn() == sn)
                        return part;
                  }
            }

      return NULL;
}
//------------------------------------------------------------
// Get parts from track
//------------------------------------------------------------
PyObject* getParts(PyObject*, PyObject* args)
{
      TrackList* tracks = MusEGlobal::song->tracks();
      const char* trackname;
      if (!PyArg_ParseTuple(args, "s", &trackname)) {
            return NULL;
            }

      PyObject* pyparts = Py_BuildValue("[]");
      for (ciTrack t = tracks->begin(); t != tracks->end(); ++t) {
            Track* track = *t;
            if (track->name() != trackname)
                  continue;

            PartList* parts = track->parts();
            for (ciPart p = parts->begin(); p != parts->end(); p++) {
                  Part* part = p->second;

                  MidiPart* mpart = (MidiPart*) part;
                  PyObject* pypart = PyDict_New();
                  int tick = mpart->tick();
                  int lentick = mpart->lenTick();
                  int serialnr = mpart->sn();
                  PyObject* pstrtick = Py_BuildValue("s","tick");
                  PyObject* pitick = Py_BuildValue("i", tick);
                  PyObject* pstrid = Py_BuildValue("s","id");
                  PyObject* pstrserial = Py_BuildValue("i", serialnr);
                  PyObject* pstrlen = Py_BuildValue("s","len");
                  PyObject* pstrtick2 = Py_BuildValue("i", lentick);

                  PyDict_SetItem(pypart, pstrtick, pitick);
                  PyDict_SetItem(pypart, pstrid, pstrserial);
                  PyDict_SetItem(pypart, pstrlen, pstrtick2);

                  Py_DECREF(pstrtick);
                  Py_DECREF(pitick);
                  Py_DECREF(pstrid);
                  Py_DECREF(pstrserial);
                  Py_DECREF(pstrlen);
                  Py_DECREF(pstrtick2);

                  // Pack midi events into list before wrapping it all up
                  EventList* events = mpart->events();
                  PyObject* pyevents = Py_BuildValue("[]");
                  for (ciEvent e = events->begin(); e != events->end(); e++) {
                        PyObject* pyevent = PyDict_New(); // The event structure - a dictionary with keys 'type','tick','data'

                        const Event& event = e->second;
                        unsigned tick = e->first;
                        PyObject* eventdata = Py_BuildValue("[i,i,i]", event.dataA(), event.dataB(), event.dataC());
                        PyObject* pstrdata = Py_BuildValue("s", "data");
                        pstrtick = Py_BuildValue("s", "tick");
                        PyObject* pitickval = Py_BuildValue("i", tick);
                        PyDict_SetItem(pyevent, pstrdata, eventdata);
                        PyDict_SetItem(pyevent, pstrtick, pitickval);
                        Py_DECREF(eventdata);
                        Py_DECREF(pstrdata);
                        Py_DECREF(pstrtick);
                        Py_DECREF(pitickval);

                        switch(event.type()) {
                              case Note: {
                                    PyObject* pstrtype  = Py_BuildValue("s", "type");
                                    PyObject* pstrnote  = Py_BuildValue("s", "note");
                                    PyObject* pstrlen   = Py_BuildValue("s", "len");
                                    PyObject* pilentick = Py_BuildValue("i", event.lenTick());
                                    PyDict_SetItem(pyevent, pstrtype, pstrnote);
                                    PyDict_SetItem(pyevent, pstrlen, pilentick);
                                    Py_DECREF(pstrtype);
                                    Py_DECREF(pstrnote);
                                    Py_DECREF(pstrlen);
                                    Py_DECREF(pilentick);
                                    break;
                                    }
                              case Controller: {
                                    PyObject* pstrtype = Py_BuildValue("s", "type");
                                    PyObject* pstrctrl = Py_BuildValue("s", "ctrl");
                                    PyDict_SetItem(pyevent, pstrtype, pstrctrl);
                                    Py_DECREF(pstrtype);
                                    Py_DECREF(pstrctrl);
                                    break;
                                    }
                              default:
                                    printf("Event type not supported yet: %d\n", event.type());
                                    break;
                              }
                        PyList_Append(pyevents, pyevent);
                        Py_DECREF(pyevent);
                        }
                  Py_DECREF(pyevents);
                  // Add the event list to the pypart dictionary
                  PyObject* pystrevents = Py_BuildValue("s", "events");
                  PyDict_SetItem(pypart, pystrevents, pyevents);
                  Py_DECREF(pystrevents);
                  PyList_Append(pyparts, pypart);
                  Py_DECREF(pypart);
                  }

            return pyparts;
            }

      return NULL;
}

//------------------------------------------------------------
// parsePythonPart
//  get part id/serialno from python part structure
//------------------------------------------------------------
int getPythonPartId(PyObject* part)
{
      PyObject* pyid = PyDict_GetItemString(part, "id");
      int id = PyInt_AsLong(pyid);
      return id;
}

//------------------------------------------------------------
// addPyPartEventsToMusePart
//  parse events from python part structure into muse part
//------------------------------------------------------------
bool addPyPartEventsToMusePart(MidiPart* npart, PyObject* part)
{
      PyObject* events;

      if (PyDict_Check(part) == false) {
            printf("Not a dict!\n");
            return false;
            }
      PyObject* pstrevents = Py_BuildValue("s","events");
      if (PyDict_Contains(part, pstrevents) == false) {
            Py_DECREF(pstrevents);
            printf("No events in part data...\n");
            return false;
            }
      Py_DECREF(pstrevents);

      events = PyDict_GetItemString(part, "events");

      if (PyList_Check(events) == false) {
            printf("Events not a list!\n");
            return false;
            }

      //
      // Go through event list, create MusE events of them and add to new part
      //
      Py_ssize_t len = PyList_Size(events);
      for (Py_ssize_t i=0; i<len; i++) {
            PyObject* pevent = PyList_GetItem(events, i);
            if (PyDict_Check(pevent) == false) {
                  printf("Event is not a dictionary!\n");
                  return false;
                  }
            PyObject* p_etick = PyDict_GetItemString(pevent, "tick");
            PyObject* p_type = PyDict_GetItemString(pevent, "type");
            PyObject* p_len = PyDict_GetItemString(pevent, "len");
            PyObject* p_data = PyDict_GetItemString(pevent, "data"); // list

            int etick = PyInt_AsLong(p_etick);
            int elen =  PyInt_AsLong(p_len);
            string type = string(PyString_AsString(p_type));
            int data[3];

            // Traverse data list:
            for (int j=0; j<3; j++) {
                  PyObject* plItem = PyList_GetItem(p_data, j);
                  data[j] = PyInt_AsLong(plItem);
                  }
            if (type == "note" || type == "ctrl") {
                  Event event(Note);
                  event.setA(data[0]);
                  event.setB(data[1]);
                  event.setC(data[2]);
                  event.setTick(etick);
                  event.setLenTick(elen);
                  npart->events()->add(event);
                  }
            else
                  printf("Unhandled event type from python: %s\n", type.c_str());
            }

      return true;
}
//------------------------------------------------------------
// Create a new part at a particular tick and track
//------------------------------------------------------------
PyObject* createPart(PyObject*, PyObject* args)
{
      const char* trackname;
      unsigned tick, tickLen;
      PyObject* part;

      if (!PyArg_ParseTuple(args, "siiO", &trackname, &tick, &tickLen, &part)) {
            return NULL;
            }

      QString qtrackname(trackname);
      MidiTrack* track = (MidiTrack*) MusEGlobal::song->findTrack(trackname);
      if (track == NULL)
            return NULL;

      MidiPart* npart = new MidiPart(track);
      npart->setTick(tick);
      npart->setLenTick(tickLen);
      addPyPartEventsToMusePart(npart, part);

      MusEGlobal::song->addPart(npart);
      QPybridgeEvent* pyevent = new QPybridgeEvent(QPybridgeEvent::SONG_UPDATE, SC_TRACK_MODIFIED);
      QApplication::postEvent(MusEGlobal::song, pyevent);

      Py_INCREF(Py_None);
      return Py_None;
}

//------------------------------------------------------------
// Modify a particular part:
//   args: new part data, old part data is used from the part with the same id as the one sent here
//   TODO: Lots and lots of refcount stuff
//------------------------------------------------------------
PyObject* modifyPart(PyObject*, PyObject* part)
{
      int id = getPythonPartId(part);

      Part* opart = NULL;
      // Verify a part with that id actually exists, then get it
      TrackList* tracks = MusEGlobal::song->tracks();
      for (ciTrack t = tracks->begin(); t != tracks->end(); ++t) {
            Track* track = *t;
            for (ciPart p = track->parts()->begin(); p != track->parts()->end(); p++) {
                  if (p->second->sn() == id) {
                        opart = p->second;
                        break;
                        }
                  }
            }

      if (opart == NULL) {
            printf("Part doesn't exist!\n");
            return NULL;
            }

      // Remove all note and controller events from current part eventlist
      std::list< std::pair<const unsigned, Event> > elist;
      MidiPart* npart = new MidiPart((MidiTrack*)opart->track());
      npart->setTick(opart->tick());
      npart->setLenTick(opart->lenTick());
      npart->setSn(opart->sn());
       
      for (iEvent e = opart->events()->begin(); e != opart->events()->end(); e++) {
            Event& event = e->second;
            if (event.type() == Note || event.type() == Controller) 
                  continue;

            npart->events()->add(event);
            }

      addPyPartEventsToMusePart(npart, part);

      //MusEGlobal::song->startUndo();
      MusEGlobal::song->changePart(opart, npart);
      //MusEGlobal::song->endUndo(SC_TRACK_MODIFIED | SC_PART_MODIFIED | SC_PART_INSERTED); // Crash! Probably since the call ends up in Qt GUI thread from this thread

      QPybridgeEvent* pyevent = new QPybridgeEvent(QPybridgeEvent::SONG_UPDATE, SC_TRACK_MODIFIED | SC_PART_MODIFIED | SC_PART_INSERTED);
      QApplication::postEvent(MusEGlobal::song, pyevent);


      Py_INCREF(Py_None);
      return Py_None;
}
//------------------------------------------------------------
// deletePart
//  delete part by serial nr
//------------------------------------------------------------
PyObject* deletePart(PyObject*, PyObject* args)
{
      int id;
      if (!PyArg_ParseTuple(args, "i", &id)) {
            return NULL;
            }

      Part* part = findPartBySerial(id);
      if (part == NULL)
            return NULL;

      MusEGlobal::song->removePart(part);
      QPybridgeEvent* pyevent = new QPybridgeEvent(QPybridgeEvent::SONG_UPDATE, SC_TRACK_MODIFIED | SC_PART_REMOVED);
      QApplication::postEvent(MusEGlobal::song, pyevent);
      Py_INCREF(Py_None);
      return Py_None;
}

//------------------------------------------------------------
// setPos
//------------------------------------------------------------
PyObject* setPos(PyObject*, PyObject* args)
{
      int index;
      int ticks;
      if (!PyArg_ParseTuple(args, "ii", &index, &ticks)) {
            return NULL;
            }

      //MusEGlobal::song->setPos(index, ticks);
      QPybridgeEvent* pyevent = new QPybridgeEvent(QPybridgeEvent::SONG_POSCHANGE, index, ticks);
      QApplication::postEvent(MusEGlobal::song, pyevent);

      Py_INCREF(Py_None);
      return Py_None;
}


//------------------------------------------------------------
// setLen
//------------------------------------------------------------
PyObject* setSongLen(PyObject*, PyObject* args)
{
      unsigned len;

      if (!PyArg_ParseTuple(args, "i", &len)) {
            return NULL;
            }
      //MusEGlobal::song->setLen(len);// Appears to not be ok to call from python thread, we do it with event instead
      QPybridgeEvent* pyevent = new QPybridgeEvent(QPybridgeEvent::SONGLEN_CHANGE, len);
      QApplication::postEvent(MusEGlobal::song, pyevent);
      Py_INCREF(Py_None);
      return Py_None;
}
//------------------------------------------------------------
// getLen
//------------------------------------------------------------
PyObject* getSongLen(PyObject*, PyObject*)
{
      PyObject* pylen = Py_BuildValue("i", MusEGlobal::song->len());

      return pylen;
}
//------------------------------------------------------------
// getDivision
//------------------------------------------------------------
PyObject* getDivision(PyObject*, PyObject*)
{
      return Py_BuildValue("i", MusEGlobal::config.division);
}
//------------------------------------------------------------
// setTrackParameter
//------------------------------------------------------------
PyObject* setMidiTrackParameter(PyObject*, PyObject* args)
{
      const char* trackname;
      const char* paramname;
      int value;
      if(!PyArg_ParseTuple(args, "ssi", &trackname, &paramname, &value))
            return NULL;

      Track* track = MusEGlobal::song->findTrack(QString(trackname));
      if (track == NULL)
            return NULL;

      MidiTrack* mt = (MidiTrack*) track;

      QString qparamname(paramname);
      bool changed = false;
      if (qparamname == "velocity") {
            changed = true;
            mt->velocity = value;
            }
      else if (qparamname == "compression") {
            changed = true;
            mt->compression = value;
            }
      else if (qparamname == "transposition") {
            changed = true;
            mt->transposition = value;
            }
      else if (qparamname == "delay") {
            changed = true;
            mt->delay = value;
            }

      if (changed) {
            QPybridgeEvent* pyevent = new QPybridgeEvent(QPybridgeEvent::SONG_UPDATE, SC_TRACK_MODIFIED);
            QApplication::postEvent(MusEGlobal::song, pyevent);
            }

      return Py_BuildValue("b", changed); // true/false depending on whether anythin was changed
}
//------------------------------------------------------------
// Set loop
//------------------------------------------------------------
PyObject* setLoop(PyObject*, PyObject* args)
{
      bool loopFlag;
      if(!PyArg_ParseTuple(args, "b", &loopFlag))
            return NULL;

      MusEGlobal::song->setLoop(loopFlag);
      Py_INCREF(Py_None);
      return Py_None;
}
//------------------------------------------------------------
// Get loop value
//------------------------------------------------------------
PyObject* getLoop(PyObject*, PyObject*)
{
      return Py_BuildValue("b", MusEGlobal::song->getLoop());
}
//------------------------------------------------------------
// getMute trackname
//------------------------------------------------------------
PyObject* getMute(PyObject*, PyObject* args)
{
      const char* trackname;
      if (!PyArg_ParseTuple(args, "s", &trackname)) {
            return NULL;
            }

      Track* track = MusEGlobal::song->findTrack(QString(trackname));
      if (track == NULL)
            return NULL;

      return Py_BuildValue("b", track->isMute());
}
//------------------------------------------------------------
// setMute (trackname, boolean)
//------------------------------------------------------------
PyObject* setMute(PyObject*, PyObject* args)
{
      const char* trackname;
      bool muted;

      if (!PyArg_ParseTuple(args, "sb", &trackname, &muted)) {
            return NULL;
            }

      Track* track = MusEGlobal::song->findTrack(QString(trackname));
      if (track == NULL)
            return NULL;

      int mutedint = 1;
      if (muted == false)
            mutedint = 0;

      QPybridgeEvent* pyevent = new QPybridgeEvent(QPybridgeEvent::SONG_SETMUTE, mutedint);
      pyevent->setS1(trackname);
      QApplication::postEvent(MusEGlobal::song, pyevent);
      Py_INCREF(Py_None);
      return Py_None;
}
//------------------------------------------------------------
// setController
//------------------------------------------------------------
void setController(const char* trackname, int ctrltype, int ctrlval)
{
      QPybridgeEvent* pyevent = new QPybridgeEvent(QPybridgeEvent::SONG_SETCTRL, ctrltype, ctrlval);
      pyevent->setS1(trackname);
      QApplication::postEvent(MusEGlobal::song, pyevent);
}

//------------------------------------------------------------
// setMidiControllerValue
//------------------------------------------------------------
PyObject* setMidiControllerValue(PyObject*, PyObject* args)
{
      const char* trackname;
      int ctrltype;
      int value;

      if (!PyArg_ParseTuple(args, "sii", &trackname, &ctrltype, &value)) {
            return NULL;
            }

      setController(trackname, ctrltype, value);
      Py_INCREF(Py_None);
      return Py_None;
}

//------------------------------------------------------------
// getMidiControllerValue
//------------------------------------------------------------
PyObject* getMidiControllerValue(PyObject*, PyObject* args)
{
      const char* trackname;
      int ctrltype;

      if (!PyArg_ParseTuple(args, "si", &trackname, &ctrltype)) {
            return NULL;
            }

      Track* t = MusEGlobal::song->findTrack(QString(trackname));
      if (t == NULL)
            return NULL;

      if (t->isMidiTrack() == false) {
            Py_INCREF(Py_None);
            return Py_None;
            }

      MidiTrack* track = (MidiTrack*) t;
      int channel  = track->outChannel();
      int outport = track->outPort();
      MidiPort* mp = &MusEGlobal::midiPorts[outport];
      if (mp == NULL)
            return Py_BuildValue("i", -1);

      int value = mp->hwCtrlState(channel, ctrltype);
      return Py_BuildValue("i", value);
}
//------------------------------------------------------------
// setAudioTrackVolume
//------------------------------------------------------------
PyObject* setAudioTrackVolume(PyObject*, PyObject* args)
{
      const char* trackname;
      double volume = 0.0f;

      if (!PyArg_ParseTuple(args, "sd", &trackname, &volume)) {
            return NULL;
            }

      QPybridgeEvent* pyevent = new QPybridgeEvent(QPybridgeEvent::SONG_SETAUDIOVOL);
      pyevent->setD1(volume);
      pyevent->setS1(trackname);
      QApplication::postEvent(MusEGlobal::song, pyevent);
      Py_INCREF(Py_None);
      return Py_None;
}
//------------------------------------------------------------
// getAudioTrackVolume
//------------------------------------------------------------
PyObject* getAudioTrackVolume(PyObject*, PyObject* args)
{
      const char* trackname;

      if (!PyArg_ParseTuple(args, "s", &trackname)) {
            return NULL;
            }

      Track* t = MusEGlobal::song->findTrack(QString(trackname));
      if (t == NULL)
            return NULL;

      if (t->type() == Track::DRUM || t->type() == Track::MIDI)
            return NULL;

      AudioTrack* track = (AudioTrack*) t;
      return Py_BuildValue("d", track->volume());
}

//------------------------------------------------------------
// getSelectedTrack
//------------------------------------------------------------
PyObject* getSelectedTrack(PyObject*, PyObject*)
{
      TrackList* tracks = MusEGlobal::song->tracks();
      for (ciTrack t = tracks->begin(); t != tracks->end(); ++t) {
            Track* track = *t;
            if (track->selected())
                  return Py_BuildValue("s", track->name().toLatin1().constData());
            }

      Py_INCREF(Py_None);
      return Py_None;
}

//------------------------------------------------------------
// importPart
//------------------------------------------------------------
PyObject* importPart(PyObject*, PyObject* args)
{
      const char* trackname;
      const char* filename;
      int tick;

      if (!PyArg_ParseTuple(args, "ssi", &trackname, &filename, &tick)) {
            return NULL;
            }

      QPybridgeEvent* pyevent = new QPybridgeEvent(QPybridgeEvent::SONG_IMPORT_PART, tick);
      pyevent->setS1(trackname);
      pyevent->setS2(filename);
      QApplication::postEvent(MusEGlobal::song, pyevent);
      Py_INCREF(Py_None);
      return Py_None;
}
//------------------------------------------------------------
// getTrackEffects
//------------------------------------------------------------
PyObject* getTrackEffects(PyObject*, PyObject* args)
{
      const char* trackname;
      if (!PyArg_ParseTuple(args, "s", &trackname)) {
            return NULL;
            }

      Track* t = MusEGlobal::song->findTrack(QString(trackname));
      if (t == NULL)
            return NULL;

      if (t->type() != Track::WAVE)
            return NULL;

      AudioTrack* track = (AudioTrack*) t;
      PyObject* pyfxnames = Py_BuildValue("[]");
      const Pipeline* pipeline = track->efxPipe();
      for (int i = 0; i < PipelineDepth; i++) {
            QString name = pipeline->name(i);
            printf("fx %d name: %s\n", i, name.toLatin1().constData());
            PyObject* pyname = Py_BuildValue("s", name.toLatin1().constData());
            PyList_Append(pyfxnames, pyname);
            Py_DECREF(pyname);
            }

      return pyfxnames;
}
//------------------------------------------------------------
// toggleTrackEffect
//------------------------------------------------------------
PyObject* toggleTrackEffect(PyObject*, PyObject* args)
{
      const char* trackname;
      int fxid;
      bool onoff;

      if (!PyArg_ParseTuple(args, "sib", &trackname, &fxid, &onoff)) 
            return NULL;

      Track* t = MusEGlobal::song->findTrack(QString(trackname));
      if (t == NULL)
            return NULL;

      if (t->type() != Track::WAVE)
            return NULL;

      QPybridgeEvent* pyevent = new QPybridgeEvent(QPybridgeEvent::SONG_TOGGLE_EFFECT, fxid, onoff);
      pyevent->setS1(trackname);

      QApplication::postEvent(MusEGlobal::song, pyevent);
      Py_INCREF(Py_None);
      return Py_None;
}
//------------------------------------------------------------
// changeTrackName
//------------------------------------------------------------
PyObject* changeTrackName(PyObject*, PyObject* args)
{
      const char* trackname;
      const char* newname;

      if (!PyArg_ParseTuple(args, "ss", &trackname, &newname)) 
            return NULL;

      Track* t = MusEGlobal::song->findTrack(QString(trackname));
      if (t == NULL)
            return Py_BuildValue("b", false);

      QPybridgeEvent* pyevent = new QPybridgeEvent(QPybridgeEvent::SONG_CHANGE_TRACKNAME);
      pyevent->setS1(trackname);
      pyevent->setS2(newname);
      QApplication::postEvent(MusEGlobal::song, pyevent);
      QPybridgeEvent* pyevent2 = new QPybridgeEvent(QPybridgeEvent::SONG_UPDATE, SC_TRACK_MODIFIED);
      QApplication::postEvent(MusEGlobal::song, pyevent2);
      return Py_BuildValue("b", true);
}
//------------------------------------------------------------
// addMidiTrack
//------------------------------------------------------------
PyObject* addMidiTrack(PyObject*, PyObject*)
{
      QPybridgeEvent* pyevent = new QPybridgeEvent(QPybridgeEvent::SONG_ADD_TRACK, Track::MIDI);
      QApplication::postEvent(MusEGlobal::song, pyevent);
      Py_INCREF(Py_None);
      return Py_None;
}
//------------------------------------------------------------
// addWaveTrack
//------------------------------------------------------------
PyObject* addWaveTrack(PyObject*, PyObject*)
{
      QPybridgeEvent* pyevent = new QPybridgeEvent(QPybridgeEvent::SONG_ADD_TRACK, Track::WAVE);
      QApplication::postEvent(MusEGlobal::song, pyevent);
      Py_INCREF(Py_None);
      return Py_None;
}
//------------------------------------------------------------
// addInput
//------------------------------------------------------------
PyObject* addInput(PyObject*, PyObject*)
{
      QPybridgeEvent* pyevent = new QPybridgeEvent(QPybridgeEvent::SONG_ADD_TRACK, Track::AUDIO_INPUT);
      QApplication::postEvent(MusEGlobal::song, pyevent);
      Py_INCREF(Py_None);
      return Py_None;
}
//------------------------------------------------------------
// addOutput
//------------------------------------------------------------
PyObject* addOutput(PyObject*, PyObject*)
{
      QPybridgeEvent* pyevent = new QPybridgeEvent(QPybridgeEvent::SONG_ADD_TRACK, Track::AUDIO_OUTPUT);
      QApplication::postEvent(MusEGlobal::song, pyevent);
      return Py_None;
}
//------------------------------------------------------------
// addGroup
//------------------------------------------------------------
PyObject* addGroup(PyObject*, PyObject*)
{
      QPybridgeEvent* pyevent = new QPybridgeEvent(QPybridgeEvent::SONG_ADD_TRACK, Track::AUDIO_GROUP);
      QApplication::postEvent(MusEGlobal::song, pyevent);
      return Py_None;
}
//------------------------------------------------------------
// deleteTrack
//------------------------------------------------------------
PyObject* deleteTrack(PyObject*, PyObject* args)
{
      const char* trackname;

      if (!PyArg_ParseTuple(args, "s", &trackname)) 
            return NULL;

      QPybridgeEvent* pyevent = new QPybridgeEvent(QPybridgeEvent::SONG_DELETE_TRACK);
      pyevent->setS1(trackname);
      QApplication::postEvent(MusEGlobal::song, pyevent);
      return Py_None;
}
//------------------------------------------------------------
// getOutputRoute
//------------------------------------------------------------
/*
PyObject* getOutputRoute(PyObject*, PyObject* args)
{
      const char* trackname;

      if (!PyArg_ParseTuple(args, "s", &trackname)) 
            return NULL;

      Track* tt = MusEGlobal::song->findTrack(QString(trackname));
      if (tt == NULL)
            return Py_BuildValue("b", false);

      PyObject* routes = Py_BuildValue("[]");
      if (tt->type() == Track::WAVE && tt->type() == Track::AUDIO_AUX) {
            AudioTrack* t = (AudioTrack*)tt;
            RouteList* r = t->outRoutes();

            OutputList* al = MusEGlobal::song->outputs();
            for (iAudioOutput i = al->begin(); i != al->end(); ++i) {
                  Track* track = *i;
                  if (t == track)
                        continue;

                  QString s(track->name());
                  
                 // for (iRoute ir = r->begin(); ir != r->end(); ++ir) {
                 //       if (ir->type == 0 && ir->track == track) {
                 //             s += "*";
                 //             PyList_Append(routes, Py_BuildValue("s", s.toLatin1()));
                 //             break;
                 //             }
                 //       }
                 // 
                  }
            }
      else if (tt->type() == Track::AUDIO_OUTPUT) {
      }


      
      return routes;
}
*/
//------------------------------------------------------------
// Global method definitions for MusE:s Python API
//
// This is where global functions in Python is linked to their equivalent C/C++ functions
//------------------------------------------------------------
PyMethodDef g_methodDefinitions[] =
{
      { "startPlay", startPlay, METH_VARARGS, "Starts playing the song from current position" },
      { "stopPlay", stopPlay, METH_VARARGS, "Stops playback if currently playing" },
      { "rewindStart", rewindStart, METH_VARARGS, "Set current position to beginning of song" },
      { "getCPos", getCPos, METH_NOARGS, "Get current position (in ticks)" },
      { "getLPos", getLPos, METH_NOARGS, "Get position of left locator (in ticks)" },
      { "getRPos", getRPos, METH_NOARGS, "Get position of right locator (in ticks)" },
      { "setPos", setPos, METH_VARARGS, "Set position of locators or current position" },
      { "getTempo", getTempo, METH_VARARGS, "Get tempo of the song at a particular tick" },
      { "setLoop", setLoop, METH_VARARGS, "Set loop mode on/off" },
      { "getLoop", getLoop, METH_NOARGS, "Get loop value" },

      { "getTrackNames", getTrackNames, METH_VARARGS, "Get track names (which are unique)" },
      { "getParts", getParts, METH_VARARGS, "Get part data from a track" },
      { "createPart", createPart, METH_VARARGS, "Create a part" },
      { "modifyPart", modifyPart, METH_O, "Modify a particular part" },
      { "deletePart", deletePart, METH_VARARGS, "Remove part with a particular serial nr" },
      { "getSelectedTrack", getSelectedTrack, METH_NOARGS, "Get first selected track" },
      { "importPart", importPart, METH_VARARGS, "Import part file to a track at a particular position" },
      { "changeTrackName", changeTrackName, METH_VARARGS, "Change track name" },
      { "addMidiTrack", addMidiTrack, METH_NOARGS, "Add a midi track" },
      { "addWaveTrack", addWaveTrack, METH_NOARGS, "Add a wave track" },
      { "addInput", addInput, METH_NOARGS, "Add audio input" },
      { "addOutput", addOutput, METH_NOARGS, "Add audio output" },
      { "addGroup", addGroup, METH_NOARGS, "Add audio group" },
      { "deleteTrack", deleteTrack, METH_VARARGS, "Delete a track" },

      { "getTrackEffects", getTrackEffects, METH_VARARGS, "Get names of LADSPA effects on a track" },
      { "toggleTrackEffect", toggleTrackEffect, METH_VARARGS, "Toggle LADSPA effect on/off" },
      //{ "getOutputRoute", getOutputRoute, METH_VARARGS, "Get route for an audio output" },

      { "setSongLen", setSongLen, METH_VARARGS, "Set length of song (in ticks)" },
      { "getSongLen", getSongLen, METH_VARARGS, "Get length of song (in ticks)" },

      { "getMute", getMute, METH_VARARGS, "Get track mute property (if track is played or not)" },
      { "setMute", setMute, METH_VARARGS, "Set track mute property (if track should be played or not)" },
      { "setMidiControllerValue", setMidiControllerValue, METH_VARARGS, "Set midi controller value for a track" },
      { "getMidiControllerValue", getMidiControllerValue, METH_VARARGS, "Get midi controller value for a track" },
      { "setAudioTrackVolume", setAudioTrackVolume, METH_VARARGS, "Set volume on audio track/aux/output/input" },
      { "getAudioTrackVolume", getAudioTrackVolume, METH_VARARGS, "Get audio track/aux/output/input volume" },

      { "setMidiTrackParameter", setMidiTrackParameter, METH_VARARGS, "Set transposition, velocity, compression or delay on track level" },

      { "getDivision", getDivision, METH_VARARGS, "Number of ticks per 1/4 (?)" },

      {NULL, NULL, NULL, NULL}
};

/**
 * This function launches the Pyro name service, which blocks execution
 * Thus it needs its own thread
 **/
static void* pyapithreadfunc(void*)
{
      Py_Initialize();
      PyImport_AddModule("muse");
      Py_InitModule( "muse", g_methodDefinitions );

      //
      // Access the "__main__" module and its name-space dictionary.
      //

      PyObject *pMainModule     = PyImport_AddModule( "__main__" );
      PyObject *pMainDictionary = PyModule_GetDict( pMainModule );
      string launcherfilename = string(SHAREDIR) + string("/pybridge/museplauncher.py");
      printf("Initiating MusE Pybridge launcher from %s\n", launcherfilename.c_str());
      FILE* fp = fopen(launcherfilename.c_str(),"r");
      PyRun_File(fp, launcherfilename.c_str(), Py_file_input, pMainDictionary, pMainDictionary);
      fclose(fp);

      return NULL;
}

/**
 * This function currently only launches the thread. There should be some kind of check that
 * things are up and running as they are supposed to
 */
bool initPythonBridge()
{
      if (pthread_create(&pyapiThread, NULL, MusECore::pyapithreadfunc, 0)) {
            return false;
            }
      return true; // TODO: Verify that things are up and running!
}

//---------------------------------------------------------
// event 
//
//  Function in Song class, run in the Qt event thread context.
//  Handles events sent from the Python bridge subsystem
//
//  This is part of Qt:s event handling and events are fed
//  here via QApplication::postEvent since gui updates should
//  be done by Qt:s GUI thread. QApplication::postEvent is 
//  a static method, which is threadsafe. Using the song object
//  from the Python thread is dangerous when it comes to
//  operations that manipulate the gui itself (read is ok)
//---------------------------------------------------------
bool Song::event(QEvent* _e)
{
      if (_e->type() != QEvent::User)
            return false; //ignore all events except user events, which are events from Python bridge subsystem

      QPybridgeEvent* e = (QPybridgeEvent*) _e;
      switch (e->getType()) {
            case QPybridgeEvent::SONG_UPDATE:
                  this->update(e->getP1());
                  break;
            case QPybridgeEvent::SONGLEN_CHANGE:
                  this->setLen(e->getP1());
                  break;
            case QPybridgeEvent::SONG_POSCHANGE:
                  this->setPos(e->getP1(), e->getP2());
                  break;
            case QPybridgeEvent::SONG_SETPLAY:
                  this->setPlay(true);
                  break;
            case QPybridgeEvent::SONG_SETSTOP:
                  this->setStop(true);
                  break;
            case QPybridgeEvent::SONG_REWIND:
                  this->rewindStart();
                  break;
            case QPybridgeEvent::SONG_SETMUTE: {
                  Track* track = this->findTrack(e->getS1());
                  if (track == NULL)
                        return false;

                  bool muted = e->getP1() == 1;
                  track->setMute(muted);
                  this->update(SC_MUTE | SC_TRACK_MODIFIED);
                  break;
                  }
            case QPybridgeEvent::SONG_SETCTRL: {
                  Track* t = this->findTrack(e->getS1());
                  if (t == NULL)
                        return false;

                  if (t->isMidiTrack() == false)
                        return false;

                  MidiTrack* track = (MidiTrack*) t;
                  int chan  = track->outChannel();

                  int num = e->getP1();
                  int val = e->getP2();
                  int tick = MusEGlobal::song->cpos();
                  MidiPlayEvent ev(tick, track->outPort(), chan, ME_CONTROLLER, num, val);
                  MusEGlobal::audio->msgPlayMidiEvent(&ev);
                  MusEGlobal::song->update(SC_MIDI_CONTROLLER); 
                  break;
                  }
            case QPybridgeEvent::SONG_SETAUDIOVOL: {
                  Track* t = this->findTrack(e->getS1());
                  if (t == NULL)
                        return false;

                  if (t->type() == Track::DRUM || t->type() == Track::MIDI)
                        return false;

                  AudioTrack* track = (AudioTrack*) t;
                  track->setVolume(e->getD1());
                  break;
                  }
            case QPybridgeEvent::SONG_IMPORT_PART: {
                  Track* track = this->findTrack(e->getS1());
                  QString filename = e->getS2();
                  unsigned int tick = e->getP1();
                  if (track == NULL)
                        return false;

                  MusEGlobal::muse->importPartToTrack(filename, tick, track);
                  break;
                  }
            case QPybridgeEvent::SONG_TOGGLE_EFFECT: {
                  Track* t = this->findTrack(e->getS1());
                  if (t == NULL)
                        return false;

                  if (t->type() != Track::WAVE)
                        return false;

                  int fxid = e->getP1();

                  if (fxid > PipelineDepth)
                        return false;

                  int onoff = (e->getP2() == 1);

                  AudioTrack* track = (AudioTrack*) t;
                  Pipeline* pipeline = track->efxPipe();
                  pipeline->setOn(fxid, onoff);
                  break;
                  }
            case QPybridgeEvent::SONG_ADD_TRACK: {
                  MusECore::Undo operations;
                  MusEGlobal::song->addTrack(operations, (Track::TrackType)e->getP1());  // Add at end of list.
                  MusEGlobal::song->applyOperationGroup(operations);
                  break;
                  }
            case QPybridgeEvent::SONG_CHANGE_TRACKNAME: {
                  Track* t = this->findTrack(e->getS1());
                  if (t == NULL)
                        return false;
                  t->setName(e->getS2());
                  break;
                  }
            case QPybridgeEvent::SONG_DELETE_TRACK: {
                  Track* t = this->findTrack(e->getS1());
                  if (t == NULL)
                        return false;

                  MusEGlobal::audio->msgRemoveTrack(t);
                  break;
                  }
            default:
                  printf("Unknown pythonthread event received: %d\n", e->getType());
                  break;
            }


      return true;
}

} // namespace MusECore

