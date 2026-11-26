//=========================================================
//  MusE
//  Linux Music Editor
//  (C) Copyright 2009 Mathias Gyllengahm (lunar_shuttle@users.sf.net)
//  (C) Copyright 2019 Tim E. Real (terminator356 on users dot sourceforge dot net)
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
#include <cmath>
#include <limits>
#include <string>

#include <QApplication>
#include <QEvent>
#include <QFile>
#include <QMetaObject>
#include <QThread>
#include <QUuid>

#include "pyapi.h"
#include "globaldefs.h"
#include "song.h"
#include "tempo.h"
#include "track.h"
#include "audio.h"
#include "gconfig.h"
#include "midictrl.h"
#include "midiport.h"
#include "instruments/minstrument.h"
#include "plugin.h"
#include "midi_consts.h"
#include "app.h"
#include "globals.h"
#include "undo.h"
#include "helper.h"
#include "part.h"
#include "synth.h"

// Steals ref: PyList_SetItem, PyTuple_SetItem
using namespace std;

namespace MusECore {

PyroServerThread g_pyroServerThread;
PyObject* g_pMainModule = nullptr;
PyObject* g_pMainDictionary = nullptr;

template <typename Callable>
bool invokeOnMusEGuiThread(Callable&& operation)
{
      MusEGui::MusE* app = MusEGlobal::muse;
      if (app == nullptr)
            return false;
      if (QThread::currentThread() == app->thread())
            return operation();

      bool result = false;
      const bool invoked = QMetaObject::invokeMethod(app, [&]() { result = operation(); },
            Qt::BlockingQueuedConnection);
      return invoked && result;
}

PyObject* saveProjectFile(PyObject*, PyObject* args)
{
      const char* path;
      int overwrite;
      if (!PyArg_ParseTuple(args, "sp", &path, &overwrite))
            return nullptr;

      const bool saved = invokeOnMusEGuiThread([&]() {
            return MusEGlobal::muse->saveProjectFileForMcp(QString::fromUtf8(path), overwrite != 0);
            });
      return PyBool_FromLong(saved);
}

PyObject* closeProjectFile(PyObject*, PyObject* args)
{
      const char* savePath = "";
      int overwrite = 0;
      if (!PyArg_ParseTuple(args, "|sp", &savePath, &overwrite))
            return nullptr;

      QString savedPath;
      const bool closed = invokeOnMusEGuiThread([&]() {
            return MusEGlobal::muse->closeProjectFileForMcp(QString::fromUtf8(savePath), overwrite != 0,
                  &savedPath);
            });
      if (!closed)
            Py_RETURN_FALSE;
      const QByteArray savedPathUtf8 = savedPath.toUtf8();
      return PyUnicode_FromString(savedPathUtf8.constData());
}

PyObject* createProjectFile(PyObject*, PyObject* args)
{
      const char* path;
      const char* savePath = "";
      int saveCurrent = 1;
      int overwrite = 0;
      if (!PyArg_ParseTuple(args, "s|psp", &path, &saveCurrent, &savePath, &overwrite))
            return nullptr;

      const bool created = invokeOnMusEGuiThread([&]() {
            return MusEGlobal::muse->createProjectFileForMcp(QString::fromUtf8(path), saveCurrent != 0,
                  QString::fromUtf8(savePath), overwrite != 0);
            });
      return PyBool_FromLong(created);
}

PyObject* openProjectFile(PyObject*, PyObject* args)
{
      const char* path;
      const char* savePath = "";
      int saveCurrent = 1;
      int overwrite = 0;
      if (!PyArg_ParseTuple(args, "s|psp", &path, &saveCurrent, &savePath, &overwrite))
            return nullptr;

      const bool opened = invokeOnMusEGuiThread([&]() {
            return MusEGlobal::muse->openProjectFileForMcp(QString::fromUtf8(path), saveCurrent != 0,
                  QString::fromUtf8(savePath), overwrite != 0);
            });
      return PyBool_FromLong(opened);
}

class QPybridgeEvent : public QEvent
{
public:
      enum EventType { SONG_UPDATE=0, SONGLEN_CHANGE, SONG_POSCHANGE, SONG_SETPLAY, SONG_SETSTOP, SONG_REWIND, SONG_SETMUTE,
             SONG_SETCTRL, SONG_SETAUDIOVOL, SONG_IMPORT_PART, SONG_TOGGLE_EFFECT, SONG_ADD_TRACK, SONG_CHANGE_TRACKNAME,
             SONG_DELETE_TRACK, SONG_SET_MIDI_OUTPUT, SONG_ADD_TRACK_EFFECT, SONG_ADD_SYNTH,
             SONG_SET_TRACK_EFFECT_PARAMETER, SONG_SET_MIDI_PROGRAM };
      QPybridgeEvent( QPybridgeEvent::EventType _type, int _p1=0, int _p2=0, const SongChangedStruct_t& _sc1 = SongChangedStruct_t());
      EventType getType() { return type; }
      int getP1() { return p1; }
      int getP2() { return p2; }
      void setS1(QString in) { s1 = in; }
      void setS2(QString in) { s2 = in; }
      void setS3(QString in) { s3 = in; }
      void setS4(QString in) { s4 = in; }
      int getResult() const { return result; }
      void setResult(int value) { result = value; }
      const QString& getS1() { return s1; }
      const QString& getS2() { return s2; }
      const QString& getS3() { return s3; }
      const QString& getS4() { return s4; }
      double getD1() { return d1; }
      void setD1(double _d1) { d1 = _d1; }
      SongChangedStruct_t getSC1() { return sc1; }

private:
      EventType type;
      int p1, p2;
      int result;
      double d1;
      QString s1;
      QString s2;
      QString s3;
      QString s4;
      SongChangedStruct_t sc1;

};

//------------------------------------------------------------
QPybridgeEvent::QPybridgeEvent(QPybridgeEvent::EventType _type, int _p1, int _p2, const SongChangedStruct_t& _sc1)
      :QEvent(QEvent::User),
      type(_type),
      p1(_p1),
      p2(_p2),
      result(-1),
      sc1(_sc1)
{
}

bool sendPythonBridgeEvent(QPybridgeEvent& event)
{
      return invokeOnMusEGuiThread([&]() {
            if (MusEGlobal::song == nullptr)
                  return false;
            return QApplication::sendEvent(MusEGlobal::song, &event);
            });
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
      QPybridgeEvent event(QPybridgeEvent::SONG_SETPLAY);
      return PyBool_FromLong(sendPythonBridgeEvent(event));
}
//------------------------------------------------------------
// Stop playing
//------------------------------------------------------------
PyObject* stopPlay(PyObject*, PyObject*)
{
      //MusEGlobal::song->setStop(true);
      QPybridgeEvent event(QPybridgeEvent::SONG_SETSTOP);
      return PyBool_FromLong(sendPythonBridgeEvent(event));
}
//------------------------------------------------------------
// Rewind to start
//------------------------------------------------------------
PyObject* rewindStart(PyObject*, PyObject*)
{
      //MusEGlobal::song->rewindStart();
      QPybridgeEvent event(QPybridgeEvent::SONG_REWIND);
      return PyBool_FromLong(sendPythonBridgeEvent(event));
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
            PyObject* ptrackname = Py_BuildValue("s", track->name().toUtf8().constData());
            PyList_Append(res, ptrackname);
            Py_DECREF(ptrackname);
            }

      return res;
}
//------------------------------------------------------------
// Get MIDI output ports with an assigned device or synth
//------------------------------------------------------------
PyObject* getMidiOutputs(PyObject*, PyObject*)
{
      PyObject* outputs = Py_BuildValue("[]");
      for (int i = 0; i < MusECore::MIDI_PORTS; ++i) {
            if (!MusEGlobal::midiPorts[i].device())
                  continue;
            const QString& name = MusEGlobal::midiPorts[i].portname();
            const QByteArray nameBytes = name.toUtf8();
            PyObject* output = Py_BuildValue("{s:i,s:s}", "port", i, "name", nameBytes.constData());
            PyList_Append(outputs, output);
            Py_DECREF(output);
            }
      return outputs;
}
//------------------------------------------------------------
// Get the MIDI output port and channel assigned to a track
//------------------------------------------------------------
PyObject* getMidiTrackOutput(PyObject*, PyObject* args)
{
      const char* trackname;
      if (!PyArg_ParseTuple(args, "s", &trackname))
            Py_RETURN_NONE;
      int port = -1;
      int channel = -1;
      const bool found = invokeOnMusEGuiThread([&]() {
            if (MusEGlobal::song == nullptr)
                  return false;
            Track* t = MusEGlobal::song->findTrack(QString(trackname));
            if (t == nullptr || !t->isMidiTrack())
                  return false;
            MidiTrack* track = static_cast<MidiTrack*>(t);
            port = track->outPort();
            channel = track->outChannel();
            return true;
            });
      if (!found)
            Py_RETURN_NONE;
      return Py_BuildValue("{s:i,s:i}", "port", port, "channel", channel);
}
//------------------------------------------------------------
// Get available software instrument plugins
//------------------------------------------------------------
PyObject* getAvailableInstruments(PyObject*, PyObject*)
{
      PyObject* instruments = Py_BuildValue("[]");
      for (iSynthList synth = MusEGlobal::synthis.begin(); synth != MusEGlobal::synthis.end(); ++synth) {
            const QByteArray name = (*synth)->name().toUtf8();
            const QByteArray file = (*synth)->uri().isEmpty() ? (*synth)->completeBaseName().toUtf8() : QByteArray();
            const QByteArray uri = (*synth)->uri().toUtf8();
            const QByteArray label = (*synth)->label().toUtf8();
            PyObject* instrument = Py_BuildValue("{s:s,s:i,s:s,s:s,s:s}",
                  "name", name.constData(), "type", static_cast<int>((*synth)->pluginType()),
                  "file", file.constData(), "uri", uri.constData(), "label", label.constData());
            PyList_Append(instruments, instrument);
            Py_DECREF(instrument);
            }
      return instruments;
}
//------------------------------------------------------------
// Add a software instrument on the GUI thread
//------------------------------------------------------------
PyObject* addSynthTrack(PyObject*, PyObject* args)
{
      int pluginType;
      const char* file;
      const char* uri;
      const char* label;
      const char* trackname;
      if (!PyArg_ParseTuple(args, "issss", &pluginType, &file, &uri, &label, &trackname))
            return nullptr;
      if (trackname[0] == '\0')
            Py_RETURN_FALSE;

      QPybridgeEvent event(QPybridgeEvent::SONG_ADD_SYNTH, pluginType);
      event.setS1(trackname);
      event.setS2(file);
      event.setS3(uri);
      event.setS4(label);
      if (!sendPythonBridgeEvent(event) || event.getResult() < 0)
            Py_RETURN_FALSE;
      return PyLong_FromLong(event.getResult());
}
//------------------------------------------------------------
// Set the MIDI output port and channel of a track
//------------------------------------------------------------
PyObject* setMidiTrackOutput(PyObject*, PyObject* args)
{
      const char* trackname;
      int port;
      int channel;
      if (!PyArg_ParseTuple(args, "sii", &trackname, &port, &channel))
            Py_RETURN_FALSE;
      if (port < 0 || port >= MusECore::MIDI_PORTS || channel < 0 || channel >= MusECore::MUSE_MIDI_CHANNELS)
            Py_RETURN_FALSE;

      QPybridgeEvent event(QPybridgeEvent::SONG_SET_MIDI_OUTPUT, port, channel);
      event.setS1(trackname);
      return PyBool_FromLong(sendPythonBridgeEvent(event));
}
//------------------------------------------------------------
// List available MIDI programs for a routed MIDI track
//------------------------------------------------------------
PyObject* getMidiTrackPrograms(PyObject*, PyObject* args)
{
      const char* trackname;
      if (!PyArg_ParseTuple(args, "s", &trackname))
            Py_RETURN_NONE;
      Track* t = MusEGlobal::song->findTrack(QString(trackname));
      if (t == nullptr || !t->isMidiTrack())
            Py_RETURN_NONE;
      MidiTrack* track = static_cast<MidiTrack*>(t);
      const int portIndex = track->outPort();
      if (portIndex < 0 || portIndex >= MusECore::MIDI_PORTS)
            Py_RETURN_NONE;
      MidiInstrument* instrument = MusEGlobal::midiPorts[portIndex].instrument();
      if (instrument == nullptr)
            Py_RETURN_NONE;

      PyObject* programs = Py_BuildValue("[]");
      const bool drum = track->isDrumTrack();
      const QList<dumb_patchlist_entry_t> patches = instrument->getPatches(track->outChannel(), drum);
      for (const dumb_patchlist_entry_t& patch : patches) {
            const int lbank = patch.lbank < 0 ? 0xff : patch.lbank;
            const int hbank = patch.hbank < 0 ? 0xff : patch.hbank;
            const int code = (patch.prog & 0xff) | ((lbank & 0xff) << 8) | ((hbank & 0xff) << 16);
            const QByteArray name = instrument->getPatchName(track->outChannel(), code, drum, true).toUtf8();
            PyObject* program = Py_BuildValue("{s:i,s:i,s:i,s:i,s:s}",
                  "patch_code", code, "program", patch.prog, "bank_lsb", lbank, "bank_msb", hbank,
                  "name", name.constData());
            PyList_Append(programs, program);
            Py_DECREF(program);
            }
      return programs;
}
//------------------------------------------------------------
// Select a MIDI program for a routed MIDI track
//------------------------------------------------------------
PyObject* setMidiTrackProgram(PyObject*, PyObject* args)
{
      const char* trackname;
      int patchCode;
      if (!PyArg_ParseTuple(args, "si", &trackname, &patchCode) || patchCode < 0 || patchCode > 0xffffff)
            Py_RETURN_FALSE;

      QPybridgeEvent event(QPybridgeEvent::SONG_SET_MIDI_PROGRAM, patchCode);
      event.setS1(trackname);
      return PyBool_FromLong(sendPythonBridgeEvent(event));
}
//------------------------------------------------------------
// Find part by serial nr
//------------------------------------------------------------
Part* findPartBySerial(const QUuid& uuid)
{
      TrackList* tracks = MusEGlobal::song->tracks();
      for (ciTrack t = tracks->begin(); t != tracks->end(); ++t) {
            Track* track = *t;
            PartList* parts = track->parts();
            for (ciPart p = parts->begin(); p != parts->end(); p++) {
                  Part* part = p->second;
                  if (part->uuid() == uuid)
                        return part;
                  }
            }

      return nullptr;
}
//------------------------------------------------------------
// Get parts from track
//------------------------------------------------------------
PyObject* getParts(PyObject*, PyObject* args)
{
      TrackList* tracks = MusEGlobal::song->tracks();
      const char* trackname;
      if (!PyArg_ParseTuple(args, "s", &trackname)) {
            Py_RETURN_NONE;
            }

      PyObject* pyparts = Py_BuildValue("[]");
      for (ciTrack t = tracks->begin(); t != tracks->end(); ++t) {
            Track* track = *t;
            if (track->name() != trackname)
                  continue;

            if (!track->isMidiTrack()) {
                  Py_DECREF(pyparts);
                  Py_RETURN_NONE;
                  }

            PartList* parts = track->parts();
            for (ciPart p = parts->begin(); p != parts->end(); p++) {
                  Part* part = p->second;

                  MidiPart* mpart = (MidiPart*) part;
                  PyObject* pypart = PyDict_New();
                  int tick = mpart->tick();
                  int lentick = mpart->lenTick();
                  const QUuid serialnr = mpart->uuid();
                  PyObject* pstrtick = Py_BuildValue("s","tick");
                  PyObject* pitick = Py_BuildValue("i", tick);
                  PyObject* pstrid = Py_BuildValue("s","id");
//                   PyObject* pstrserial = Py_BuildValue("s", serialnr.toByteArray().constData());
                  PyObject* pstrserial = Py_BuildValue("s", serialnr.toString().toUtf8().constData());
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
                  const EventList& events = mpart->events();
                  PyObject* pyevents = Py_BuildValue("[]");
                  for (ciEvent e = events.begin(); e != events.end(); e++) {
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
                                    fprintf(stderr, "Event type not supported yet: %d\n", event.type());
                                    break;
                              }
                        PyList_Append(pyevents, pyevent);
                        Py_DECREF(pyevent);
                        }

                  // Add the event list to the pypart dictionary
                  PyObject* pystrevents = Py_BuildValue("s", "events");
                  PyDict_SetItem(pypart, pystrevents, pyevents);

                  Py_DECREF(pyevents);
                  Py_DECREF(pystrevents);

                  PyList_Append(pyparts, pypart);
                  Py_DECREF(pypart);
                  }

            return pyparts;
            }

      Py_DECREF(pyparts);
      Py_RETURN_NONE;
}

//------------------------------------------------------------
// parsePythonPart
//  get part id/serialno from python part structure
//------------------------------------------------------------
QUuid getPythonPartId(PyObject* part)
{
      if (!PyDict_Check(part)) {
            PyErr_SetString(PyExc_TypeError, "Part data must be a dictionary");
            return QUuid();
            }
      PyObject* pyid = PyDict_GetItemString(part, "id");
      if (pyid == nullptr || !PyUnicode_Check(pyid)) {
            PyErr_SetString(PyExc_ValueError, "Part data must include a string id");
            return QUuid();
            }
// For Python 3.3 and above:
#if (PY_MAJOR_VERSION >= 4) || (PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION >= 3)
            const char* bytes = PyUnicode_AsUTF8(pyid);
#else
            PyObject* p_bytes = PyUnicode_AsUTF8String(pyid);
            const char* bytes = PyBytes_AsString(p_bytes);
#endif
      if (bytes == nullptr)
            return QUuid();
      return QUuid(bytes);
}

//------------------------------------------------------------
// addPyPartEventsToMusePart
//  parse events from python part structure into muse part
//------------------------------------------------------------
bool addPyPartEventsToMusePart(MidiPart* npart, PyObject* part)
{
      PyObject* events;

      if (PyDict_Check(part) == false) {
            fprintf(stderr, "Not a dict!\n");
            return false;
            }
      PyObject* pstrevents = Py_BuildValue("s","events");
      if (PyDict_Contains(part, pstrevents) == false) {
            Py_DECREF(pstrevents);
            fprintf(stderr, "No events in part data...\n");
            return false;
            }
      Py_DECREF(pstrevents);

      events = PyDict_GetItemString(part, "events");

      if (PyList_Check(events) == false) {
            fprintf(stderr, "Events not a list!\n");
            return false;
            }

      //
      // Go through event list, create MusE events of them and add to new part
      //
      Py_ssize_t len = PyList_Size(events);
      for (Py_ssize_t i=0; i<len; i++) {
            PyObject* pevent = PyList_GetItem(events, i);
            if (PyDict_Check(pevent) == false) {
                  fprintf(stderr, "Event is not a dictionary!\n");
                  return false;
                  }
            PyObject* p_etick = PyDict_GetItemString(pevent, "tick");
            PyObject* p_type = PyDict_GetItemString(pevent, "type");
            PyObject* p_len = PyDict_GetItemString(pevent, "len");
            PyObject* p_data = PyDict_GetItemString(pevent, "data"); // list
            if (p_etick == nullptr || p_type == nullptr || p_data == nullptr ||
                !PyList_Check(p_data) || PyList_Size(p_data) < 3) {
                  PyErr_SetString(PyExc_ValueError, "MIDI event is missing its tick, type, or data");
                  return false;
                  }
            const long etickValue = PyLong_AsLong(p_etick);
            if (PyErr_Occurred())
                  return false;
            if (etickValue < 0 || etickValue > std::numeric_limits<int>::max()) {
                  PyErr_SetString(PyExc_ValueError, "MIDI event tick is out of range");
                  return false;
                  }

// For Python 3.3 and above:
#if (PY_MAJOR_VERSION >= 4) || (PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION >= 3)
            const char* bytes = PyUnicode_AsUTF8(p_type);
#else
            PyObject* p_bytes = PyUnicode_AsUTF8String(p_type);
            const char* bytes = PyBytes_AsString(p_bytes);
#endif
            if(bytes && bytes[0] != 0)
            {
              string type = string(bytes);
              if (type != "note" && type != "ctrl") {
                    PyErr_SetString(PyExc_ValueError, "Unsupported MIDI event type");
                    return false;
                    }
              int elen = 0;
              if (type == "note") {
                    if (p_len == nullptr) {
                          PyErr_SetString(PyExc_ValueError, "MIDI note event is missing its length");
                          return false;
                          }
                    const long lengthValue = PyLong_AsLong(p_len);
                    if (PyErr_Occurred())
                          return false;
                    if (lengthValue < 1 || lengthValue > std::numeric_limits<int>::max()) {
                          PyErr_SetString(PyExc_ValueError, "MIDI note length is out of range");
                          return false;
                          }
                    elen = static_cast<int>(lengthValue);
                    }
              int data[3];

              // Traverse data list:
              for (int j=0; j<3; j++) {
                    PyObject* plItem = PyList_GetItem(p_data, j);
                    if (!PyLong_Check(plItem)) {
                          PyErr_SetString(PyExc_TypeError, "MIDI event data must contain integers");
                          return false;
                          }
                    const long value = PyLong_AsLong(plItem);
                    if (PyErr_Occurred())
                          return false;
                    if (value < 0 || value > 127) {
                          PyErr_SetString(PyExc_ValueError, "MIDI event data values must be between 0 and 127");
                          return false;
                          }
                    data[j] = static_cast<int>(value);
                    }
              if (data[2] >= MusECore::MUSE_MIDI_CHANNELS ||
                  (type == "note" && data[1] == 0)) {
                    PyErr_SetString(PyExc_ValueError, "MIDI channel or note velocity is out of range");
                    return false;
                    }
              if (type == "note") {
                    Event event(Note);
                    event.setA(data[0]);
                    event.setB(data[1]);
                    event.setC(data[2]);
                    event.setTick(static_cast<unsigned>(etickValue));
                    event.setLenTick(elen);
                    npart->addEvent(event);
                    }
              else {
                    Event event(Controller);
                    event.setA(data[0]);
                    event.setB(data[1]);
                    event.setC(data[2]);
                    event.setTick(static_cast<unsigned>(etickValue));
                    npart->addEvent(event);
                    }
            }
            else
                  return false;
            }

      return true;
}
//------------------------------------------------------------
// Create a new part at a particular tick and track
//------------------------------------------------------------
PyObject* createPart(PyObject*, PyObject* args)
{
      const char* trackname;
      int tick, tickLen;
      PyObject* part;

      if (!PyArg_ParseTuple(args, "siiO", &trackname, &tick, &tickLen, &part)) {
            Py_RETURN_NONE;
            }
      if (tick < 0 || tickLen <= 0) {
            PyErr_SetString(PyExc_ValueError, "Part tick must be non-negative and length must be positive");
            return nullptr;
            }

      QString qtrackname(trackname);
      Track* t = MusEGlobal::song->findTrack(trackname);
      if (t == nullptr || t->isMidiTrack() == false) {
            Py_RETURN_NONE;
            }

      MidiTrack* track = (MidiTrack*)t;

      MidiPart* npart = new MidiPart(track);
      npart->setTick(static_cast<unsigned>(tick));
      npart->setLenTick(static_cast<unsigned>(tickLen));
      if (!addPyPartEventsToMusePart(npart, part)) {
            delete npart;
            if (PyErr_Occurred())
                  return nullptr;
            Py_RETURN_NONE;
            }

      MusEGlobal::song->addPart(npart);
      QPybridgeEvent* pyevent = new QPybridgeEvent(QPybridgeEvent::SONG_UPDATE, 0, 0, SC_TRACK_MODIFIED);
      QApplication::postEvent(MusEGlobal::song, pyevent);

      const QByteArray partId = npart->uuid().toString().toUtf8();
      return Py_BuildValue("s", partId.constData());
}
//------------------------------------------------------------
// Create a new Wave part at a particular tick and track
// Wave parts don't need audio files for loop flag testing —
// they just need to exist on the track with a length.
//------------------------------------------------------------
PyObject* createWavePart(PyObject*, PyObject* args)
{
      const char* trackname;
      unsigned tick, tickLen;

      if (!PyArg_ParseTuple(args, "sii", &trackname, &tick, &tickLen)) {
            Py_RETURN_NONE;
            }

      Track* t = MusEGlobal::song->findTrack(trackname);
      if (t == nullptr || t->type() != Track::WAVE) {
            Py_RETURN_NONE;
            }

      WavePart* npart = new WavePart((WaveTrack*)t);
      npart->setTick(tick);
      npart->setLenTick(tickLen);

      MusEGlobal::song->addPart(npart);
      QPybridgeEvent* pyevent = new QPybridgeEvent(QPybridgeEvent::SONG_UPDATE, 0, 0, SC_TRACK_MODIFIED);
      QApplication::postEvent(MusEGlobal::song, pyevent);

      Py_RETURN_NONE;
}

//------------------------------------------------------------
// Modify a particular part:
//   args: new part data, old part data is used from the part with the same id as the one sent here
//   TODO: Lots and lots of refcount stuff
//------------------------------------------------------------
PyObject* modifyPart(PyObject*, PyObject* part)
{
      const QUuid id = getPythonPartId(part);
      if (PyErr_Occurred())
            return nullptr;

      bool hasPartTick = false;
      bool hasPartLen = false;
      unsigned partTick = 0;
      unsigned partLen = 0;
      PyObject* pyTick = PyDict_GetItemString(part, "tick");
      PyObject* pyLen = PyDict_GetItemString(part, "len");
      if (pyTick) {
            if (!PyLong_Check(pyTick)) {
                  PyErr_SetString(PyExc_TypeError, "Part tick must be an integer");
                  return nullptr;
                  }
            const unsigned long value = PyLong_AsUnsignedLong(pyTick);
            if (PyErr_Occurred())
                  return nullptr;
            if (value > std::numeric_limits<int>::max()) {
                  PyErr_SetString(PyExc_ValueError, "Part tick is out of range");
                  return nullptr;
                  }
            partTick = static_cast<unsigned>(value);
            hasPartTick = true;
            }
      if (pyLen) {
            if (!PyLong_Check(pyLen)) {
                  PyErr_SetString(PyExc_TypeError, "Part length must be an integer");
                  return nullptr;
                  }
            const unsigned long value = PyLong_AsUnsignedLong(pyLen);
            if (PyErr_Occurred())
                  return nullptr;
            if (value == 0 || value > std::numeric_limits<int>::max()) {
                  PyErr_SetString(PyExc_ValueError, "Part length is out of range");
                  return nullptr;
                  }
            partLen = static_cast<unsigned>(value);
            hasPartLen = true;
            }
      MidiPart* replacement = new MidiPart(nullptr);
      if (!addPyPartEventsToMusePart(replacement, part)) {
            delete replacement;
            if (PyErr_Occurred())
                  return nullptr;
            Py_RETURN_NONE;
            }

      const bool changed = invokeOnMusEGuiThread([&]() {
            if (MusEGlobal::song == nullptr)
                  return false;
            Part* opart = findPartBySerial(id);
            if (opart == nullptr || !opart->track()->isMidiTrack())
                  return false;

            MidiPart* originalPart = static_cast<MidiPart*>(opart);
            MidiPart* updatedPart = new MidiPart(originalPart->track());
            updatedPart->setTick(hasPartTick ? partTick : opart->tick());
            updatedPart->setLenTick(hasPartLen ? partLen : opart->lenTick());
            updatedPart->setUuid(opart->uuid());

            for (ciEvent e = opart->events().begin(); e != opart->events().end(); ++e) {
                  const Event& event = e->second;
                  if (event.type() != Note && event.type() != Controller) {
                        Event copiedEvent = event;
                        updatedPart->addEvent(copiedEvent);
                        }
                  }
            for (ciEvent e = replacement->events().begin(); e != replacement->events().end(); ++e) {
                  Event copiedEvent = e->second;
                  updatedPart->addEvent(copiedEvent);
                  }

            MusEGlobal::song->changePart(opart, updatedPart);
            MusEGlobal::song->update(SC_TRACK_MODIFIED | SC_PART_MODIFIED | SC_PART_INSERTED);
            return true;
            });
      delete replacement;
      return PyBool_FromLong(changed);
}
//------------------------------------------------------------
// deletePart
//  delete part by serial nr
//------------------------------------------------------------
PyObject* deletePart(PyObject*, PyObject* args)
{
      const char* bytes;
      if (!PyArg_ParseTuple(args, "s", &bytes)) {
            Py_RETURN_NONE;
            }

      const QUuid id(bytes);
      Part* part = findPartBySerial(id);
      if (part == nullptr) {
            Py_RETURN_NONE;
            }

      MusEGlobal::song->removePart(part);
      QPybridgeEvent* pyevent = new QPybridgeEvent(QPybridgeEvent::SONG_UPDATE, 0, 0, SC_TRACK_MODIFIED | SC_PART_REMOVED);
      QApplication::postEvent(MusEGlobal::song, pyevent);
      Py_RETURN_NONE;
}
//------------------------------------------------------------
// setPartLoop
//  Set loop flag on a part by serial nr
//------------------------------------------------------------
PyObject* setPartLoop(PyObject*, PyObject* args)
{
      const char* bytes;
      int loop;
      if (!PyArg_ParseTuple(args, "si", &bytes, &loop)) {
            Py_RETURN_NONE;
            }

      const QUuid id(bytes);
      Part* part = findPartBySerial(id);
      if (part == nullptr) {
            Py_RETURN_NONE;
            }

      // Use undo operation system instead of directly calling setLoop().
      // This ensures the change is undoable and consistent with context-menu changes.
      bool newLoop = (loop != 0);
      if (part->loop() == newLoop) {
            Py_RETURN_NONE;
            }
      MusECore::Undo operations;
      operations.push_back(MusECore::UndoOp(MusECore::UndoOp::ModifyPartLoop, part, (int)part->loop(), (int)newLoop));
      MusEGlobal::song->applyOperationGroup(operations);
      QPybridgeEvent* pyevent = new QPybridgeEvent(QPybridgeEvent::SONG_UPDATE, 0, 0, SC_TRACK_MODIFIED | SC_PART_MODIFIED);
      QApplication::postEvent(MusEGlobal::song, pyevent);
      Py_RETURN_NONE;
}
//------------------------------------------------------------
// undo
//  Undo the last operation
//------------------------------------------------------------
PyObject* undo(PyObject*, PyObject*)
{
      MusEGlobal::song->undo();
      Py_RETURN_NONE;
}
//------------------------------------------------------------
// redo
//  Redo the last undone operation
//------------------------------------------------------------
PyObject* redo(PyObject*, PyObject*)
{
      MusEGlobal::song->redo();
      Py_RETURN_NONE;
}
//------------------------------------------------------------
// save_song
//  Save the current song to a file
//------------------------------------------------------------
PyObject* save_song(PyObject*, PyObject* args)
{
      const char* path;
      if (!PyArg_ParseTuple(args, "s", &path)) {
            PyErr_SetString(PyExc_ValueError, "Invalid path argument");
            return nullptr;
            }

      QFile file(path);
      if (!file.open(QIODevice::WriteOnly)) {
            PyErr_Format(PyExc_IOError, "Cannot open file for writing: %s", path);
            return nullptr;
            }
      MusECore::Xml xml(&file);
      MusEGlobal::song->write(0, xml);
      file.close();
      Py_RETURN_TRUE;
}
//------------------------------------------------------------
// load_song
//  Load a song from a file
//------------------------------------------------------------
PyObject* load_song(PyObject*, PyObject* args)
{
      const char* path;
      if (!PyArg_ParseTuple(args, "s", &path)) {
            PyErr_SetString(PyExc_ValueError, "Invalid path argument");
            return nullptr;
            }

      QFile file(path);
      if (!file.open(QIODevice::ReadOnly)) {
            PyErr_Format(PyExc_IOError, "Cannot open file for reading: %s", path);
            return nullptr;
            }
      MusECore::Xml xml(&file);
      MusEGlobal::song->read(xml);
      file.close();
      QPybridgeEvent* pyevent = new QPybridgeEvent(QPybridgeEvent::SONG_UPDATE, 0, 0);
      QApplication::postEvent(MusEGlobal::song, pyevent);
      Py_RETURN_TRUE;
}
//------------------------------------------------------------
// getPartLoop
//  Get loop flag of a part by serial nr
//------------------------------------------------------------
PyObject* getPartLoop(PyObject*, PyObject* args)
{
      const char* bytes;
      if (!PyArg_ParseTuple(args, "s", &bytes)) {
            Py_RETURN_NONE;
            }

      const QUuid id(bytes);
      Part* part = findPartBySerial(id);
      if (part == nullptr) {
            Py_RETURN_NONE;
            }

      return Py_BuildValue("b", part->loop());
}
//------------------------------------------------------------
// convertLoopToSingle
//  Convert a looping part to a single part spanning the entire loop range
//------------------------------------------------------------
PyObject* convertLoopToSingle(PyObject*, PyObject* args)
{
      const char* bytes;
      if (!PyArg_ParseTuple(args, "s", &bytes)) {
            Py_RETURN_NONE;
            }

      const QUuid id(bytes);
      Part* part = findPartBySerial(id);
      if (part == nullptr || !part->loop()) {
            Py_RETURN_NONE;
            }

      ::MusEGui::convertLoopToSingle(part);
      QPybridgeEvent* pyevent = new QPybridgeEvent(QPybridgeEvent::SONG_UPDATE, 0, 0, SC_TRACK_MODIFIED | SC_PART_MODIFIED);
      QApplication::postEvent(MusEGlobal::song, pyevent);
      Py_RETURN_NONE;
}
//------------------------------------------------------------
// convertLoopToMany
//  Convert a looping part to many individual parts
//------------------------------------------------------------
PyObject* convertLoopToMany(PyObject*, PyObject* args)
{
      const char* bytes;
      if (!PyArg_ParseTuple(args, "s", &bytes)) {
            Py_RETURN_NONE;
            }

      const QUuid id(bytes);
      Part* part = findPartBySerial(id);
      if (part == nullptr || !part->loop()) {
            Py_RETURN_NONE;
            }

      ::MusEGui::convertLoopToMany(part);
      QPybridgeEvent* pyevent = new QPybridgeEvent(QPybridgeEvent::SONG_UPDATE, 0, 0, SC_TRACK_MODIFIED | SC_PART_MODIFIED);
      QApplication::postEvent(MusEGlobal::song, pyevent);
      Py_RETURN_NONE;
}
//------------------------------------------------------------
// convertLoopToSingleNewTrack
//  Convert a looping part to a single part on a new track
//------------------------------------------------------------
PyObject* convertLoopToSingleNewTrack(PyObject*, PyObject* args)
{
      const char* bytes;
      if (!PyArg_ParseTuple(args, "s", &bytes)) {
            Py_RETURN_NONE;
            }

      const QUuid id(bytes);
      Part* part = findPartBySerial(id);
      if (part == nullptr || !part->loop()) {
            Py_RETURN_NONE;
            }

      ::MusEGui::convertLoopToSingleNewTrack(part);
      QPybridgeEvent* pyevent = new QPybridgeEvent(QPybridgeEvent::SONG_UPDATE, 0, 0, SC_TRACK_MODIFIED | SC_PART_MODIFIED);
      QApplication::postEvent(MusEGlobal::song, pyevent);
      Py_RETURN_NONE;
}
//------------------------------------------------------------
// convertLoopToManyNewTrack
//  Convert a looping part to many parts on a new track
//------------------------------------------------------------
PyObject* convertLoopToManyNewTrack(PyObject*, PyObject* args)
{
      const char* bytes;
      if (!PyArg_ParseTuple(args, "s", &bytes)) {
            Py_RETURN_NONE;
            }

      const QUuid id(bytes);
      Part* part = findPartBySerial(id);
      if (part == nullptr || !part->loop()) {
            Py_RETURN_NONE;
            }

      ::MusEGui::convertLoopToManyNewTrack(part);
      QPybridgeEvent* pyevent = new QPybridgeEvent(QPybridgeEvent::SONG_UPDATE, 0, 0, SC_TRACK_MODIFIED | SC_PART_MODIFIED);
      QApplication::postEvent(MusEGlobal::song, pyevent);
      Py_RETURN_NONE;
}

//------------------------------------------------------------
// setPos
//------------------------------------------------------------
PyObject* setPos(PyObject*, PyObject* args)
{
      int index;
      int ticks;
      if (!PyArg_ParseTuple(args, "ii", &index, &ticks))
            return nullptr;
      if (ticks < 0)
            Py_RETURN_FALSE;

      //MusEGlobal::song->setPos(index, ticks);
      QPybridgeEvent event(QPybridgeEvent::SONG_POSCHANGE, index, ticks);
      return PyBool_FromLong(sendPythonBridgeEvent(event));
}


//------------------------------------------------------------
// setLen
//------------------------------------------------------------
PyObject* setSongLen(PyObject*, PyObject* args)
{
      unsigned len;

      if (!PyArg_ParseTuple(args, "i", &len)) {
            Py_RETURN_NONE;
            }
      //MusEGlobal::song->setLen(len);// Appears to not be ok to call from python thread, we do it with event instead
      QPybridgeEvent* pyevent = new QPybridgeEvent(QPybridgeEvent::SONGLEN_CHANGE, len);
      QApplication::postEvent(MusEGlobal::song, pyevent);
      Py_RETURN_NONE;
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
      if(!PyArg_ParseTuple(args, "ssi", &trackname, &paramname, &value)) {
            Py_RETURN_NONE;
            }

      Track* track = MusEGlobal::song->findTrack(QString(trackname));
      if (track == nullptr || track->isMidiTrack() == false) {
            Py_RETURN_NONE;
            }

      MidiTrack* mt = (MidiTrack*) track;

      QString qparamname(paramname);
      bool changed = false;
      int minValue = 0;
      int maxValue = 0;
      if (qparamname == "velocity") {
            minValue = MidiTrack::velocityMin;
            maxValue = MidiTrack::velocityMax;
            changed = value >= minValue && value <= maxValue;
            if (!changed)
                  return Py_BuildValue("b", false);
            mt->velocity = value;
            }
      else if (qparamname == "compression") {
            minValue = MidiTrack::compressionMin;
            maxValue = MidiTrack::compressionMax;
            changed = value >= minValue && value <= maxValue;
            if (!changed)
                  return Py_BuildValue("b", false);
            mt->compression = value;
            }
      else if (qparamname == "transposition") {
            minValue = MidiTrack::transpositionMin;
            maxValue = MidiTrack::transpositionMax;
            changed = value >= minValue && value <= maxValue;
            if (!changed)
                  return Py_BuildValue("b", false);
            mt->transposition = value;
            }
      else if (qparamname == "delay") {
            minValue = MidiTrack::delayMin;
            maxValue = MidiTrack::delayMax;
            changed = value >= minValue && value <= maxValue;
            if (!changed)
                  return Py_BuildValue("b", false);
            mt->delay = value;
            }
      else if (qparamname == "len") {
            minValue = MidiTrack::lenMin;
            maxValue = MidiTrack::lenMax;
            changed = value >= minValue && value <= maxValue;
            if (!changed)
                  return Py_BuildValue("b", false);
            mt->len = value;
            }

      if (changed) {
            QPybridgeEvent* pyevent = new QPybridgeEvent(QPybridgeEvent::SONG_UPDATE, 0, 0, SC_TRACK_MODIFIED);
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
      if(!PyArg_ParseTuple(args, "b", &loopFlag)) {
            Py_RETURN_NONE;
            }

      MusEGlobal::song->setLoop(loopFlag);
      Py_RETURN_NONE;
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
            Py_RETURN_NONE;
            }

      Track* track = MusEGlobal::song->findTrack(QString(trackname));
      if (track == nullptr) {
            Py_RETURN_NONE;
            }

      return Py_BuildValue("b", track->isMute());
}
//------------------------------------------------------------
// setMute (trackname, boolean)
//------------------------------------------------------------
PyObject* setMute(PyObject*, PyObject* args)
{
      const char* trackname;
      bool muted;

      if (!PyArg_ParseTuple(args, "sb", &trackname, &muted))
            return nullptr;

      Track* track = MusEGlobal::song->findTrack(QString(trackname));
      if (track == nullptr)
            Py_RETURN_FALSE;

      int mutedint = 1;
      if (muted == false)
            mutedint = 0;

      QPybridgeEvent event(QPybridgeEvent::SONG_SETMUTE, mutedint);
      event.setS1(trackname);
      return PyBool_FromLong(sendPythonBridgeEvent(event));
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
            Py_RETURN_NONE;
            }

      setController(trackname, ctrltype, value);
      Py_RETURN_NONE;
}

//------------------------------------------------------------
// getMidiControllerValue
//------------------------------------------------------------
PyObject* getMidiControllerValue(PyObject*, PyObject* args)
{
      const char* trackname;
      int ctrltype;

      if (!PyArg_ParseTuple(args, "si", &trackname, &ctrltype)) {
            Py_RETURN_NONE;
            }

      Track* t = MusEGlobal::song->findTrack(QString(trackname));
      if (t == nullptr) {
            Py_RETURN_NONE;
            }

      if (t->isMidiTrack() == false) {
            Py_RETURN_NONE;
            }

      MidiTrack* track = (MidiTrack*) t;
      int channel  = track->outChannel();
      int outport = track->outPort();
      MidiPort* mp = &MusEGlobal::midiPorts[outport];
      if (mp == nullptr)
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
            Py_RETURN_NONE;
            }

      QPybridgeEvent* pyevent = new QPybridgeEvent(QPybridgeEvent::SONG_SETAUDIOVOL);
      pyevent->setD1(volume);
      pyevent->setS1(trackname);
      QApplication::postEvent(MusEGlobal::song, pyevent);
      Py_RETURN_NONE;
}
//------------------------------------------------------------
// getAudioTrackVolume
//------------------------------------------------------------
PyObject* getAudioTrackVolume(PyObject*, PyObject* args)
{
      const char* trackname;

      if (!PyArg_ParseTuple(args, "s", &trackname)) {
            Py_RETURN_NONE;
            }

      Track* t = MusEGlobal::song->findTrack(QString(trackname));
      if (t == nullptr) {
            Py_RETURN_NONE;
            }

      //if (t->type() == Track::DRUM || t->type() == Track::MIDI || t->type() == Track::NEW_DRUM)
        if (t->isMidiTrack()) { // changed by flo. should do the same thing and is better maintainable
            Py_RETURN_NONE;
            }

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
                  return Py_BuildValue("s", track->name().toUtf8().constData());
            }

      Py_RETURN_NONE;
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
            Py_RETURN_NONE;
            }

      QPybridgeEvent* pyevent = new QPybridgeEvent(QPybridgeEvent::SONG_IMPORT_PART, tick);
      pyevent->setS1(trackname);
      pyevent->setS2(filename);
      QApplication::postEvent(MusEGlobal::song, pyevent);
      Py_RETURN_NONE;
}
//------------------------------------------------------------
// getTrackEffects
//------------------------------------------------------------
PyObject* getTrackEffects(PyObject*, PyObject* args)
{
      const char* trackname;
      if (!PyArg_ParseTuple(args, "s", &trackname)) {
            Py_RETURN_NONE;
            }

      Track* t = MusEGlobal::song->findTrack(QString(trackname));
      if (t == nullptr) {
            Py_RETURN_NONE;
            }

      AudioTrack* track = dynamic_cast<AudioTrack*>(t);
      if (track == nullptr)
            Py_RETURN_NONE;
      PyObject* pyfxnames = Py_BuildValue("[]");
      const Pipeline* pipeline = track->efxPipe();
      for (int i = 0; i < MusECore::PipelineDepth; i++) {
            QString name = pipeline->name(i);
            PyObject* pyname = Py_BuildValue("s", name.toUtf8().constData());
            PyList_Append(pyfxnames, pyname);
            Py_DECREF(pyname);
            }

      return pyfxnames;
}
//------------------------------------------------------------
// Get track effect slots and states
//------------------------------------------------------------
PyObject* getTrackEffectDetails(PyObject*, PyObject* args)
{
      const char* trackname;
      if (!PyArg_ParseTuple(args, "s", &trackname))
            Py_RETURN_NONE;

      Track* t = MusEGlobal::song->findTrack(QString(trackname));
      AudioTrack* track = dynamic_cast<AudioTrack*>(t);
      if (track == nullptr)
            Py_RETURN_NONE;

      PyObject* effects = Py_BuildValue("[]");
      const Pipeline* pipeline = track->efxPipe();
      for (int i = 0; i < MusECore::PipelineDepth; ++i) {
            const QByteArray name = pipeline->name(i).toUtf8();
            const QByteArray uri = pipeline->uri(i).toUtf8();
            PyObject* effect = Py_BuildValue("{s:i,s:s,s:b,s:s}",
                  "slot", i, "name", name.constData(), "enabled", pipeline->isOn(i), "uri", uri.constData());
            PyList_Append(effects, effect);
            Py_DECREF(effect);
            }
      return effects;
}
//------------------------------------------------------------
// Get parameters for an effect in a track's rack
//------------------------------------------------------------
PyObject* getTrackEffectParameters(PyObject*, PyObject* args)
{
      const char* trackname;
      int fxid;
      if (!PyArg_ParseTuple(args, "si", &trackname, &fxid))
            Py_RETURN_NONE;

      Track* t = MusEGlobal::song->findTrack(QString(trackname));
      AudioTrack* track = dynamic_cast<AudioTrack*>(t);
      if (track == nullptr || fxid < 0 || fxid >= MusECore::PipelineDepth || track->efxPipe()->empty(fxid))
            Py_RETURN_NONE;

      PluginI* plugin = track->efxPipe()->at(fxid);
      PyObject* parameters = Py_BuildValue("[]");
      for (unsigned long i = 0; i < plugin->parameters(); ++i) {
            const char* paramName = plugin->paramName(i);
            float minimum = 0.0f;
            float maximum = 0.0f;
            plugin->range(i, &minimum, &maximum);
            const QByteArray name = paramName ? QByteArray(paramName) : QByteArray("Parameter");
            PyObject* parameter = Py_BuildValue("{s:i,s:s,s:d,s:d,s:d}",
                  "index", static_cast<int>(i), "name", name.constData(), "value", plugin->param(i),
                  "minimum", static_cast<double>(minimum), "maximum", static_cast<double>(maximum));
            PyList_Append(parameters, parameter);
            Py_DECREF(parameter);
            }
      return parameters;
}
//------------------------------------------------------------
// Set a parameter of an effect in a track's rack
//------------------------------------------------------------
PyObject* setTrackEffectParameter(PyObject*, PyObject* args)
{
      const char* trackname;
      int fxid;
      int parameter;
      double value;
      if (!PyArg_ParseTuple(args, "siid", &trackname, &fxid, &parameter, &value))
            return nullptr;
      if (!std::isfinite(value) || std::abs(value) > 1.0e9 || parameter < 0 ||
          fxid < 0 || fxid >= MusECore::PipelineDepth)
            Py_RETURN_FALSE;

      QPybridgeEvent event(QPybridgeEvent::SONG_SET_TRACK_EFFECT_PARAMETER, fxid, parameter);
      event.setS1(trackname);
      event.setD1(value);
      return PyBool_FromLong(sendPythonBridgeEvent(event));
}
//------------------------------------------------------------
// List available audio effect plugins
//------------------------------------------------------------
PyObject* getAvailableTrackEffects(PyObject*, PyObject*)
{
      PyObject* effects = Py_BuildValue("[]");
      for (iPlugin plugin = MusEGlobal::plugins.begin(); plugin != MusEGlobal::plugins.end(); ++plugin) {
            if ((*plugin)->pluginClass() != MusEPlugin::PluginClassEffect)
                  continue;
            const QByteArray name = (*plugin)->name().toUtf8();
            const QByteArray uri = (*plugin)->uri().toUtf8();
            const QByteArray file = (*plugin)->filePath().toUtf8();
            const QByteArray label = (*plugin)->label().toUtf8();
            PyObject* effect = Py_BuildValue("{s:s,s:s,s:s,s:s}",
                  "name", name.constData(), "uri", uri.constData(), "file", file.constData(), "label", label.constData());
            PyList_Append(effects, effect);
            Py_DECREF(effect);
            }
      return effects;
}
//------------------------------------------------------------
// Add an audio effect plugin to an empty rack slot
//------------------------------------------------------------
PyObject* addTrackEffect(PyObject*, PyObject* args)
{
      const char* trackname;
      const char* uri;
      const char* file;
      const char* label;
      int fxid;
      if (!PyArg_ParseTuple(args, "ssssi", &trackname, &uri, &file, &label, &fxid))
            return nullptr;
      if (fxid < 0 || fxid >= MusECore::PipelineDepth)
            Py_RETURN_FALSE;

      QPybridgeEvent event(QPybridgeEvent::SONG_ADD_TRACK_EFFECT, fxid);
      event.setS1(trackname);
      event.setS2(uri);
      event.setS3(file);
      event.setS4(label);
      return PyBool_FromLong(sendPythonBridgeEvent(event));
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
            return nullptr;

      QPybridgeEvent event(QPybridgeEvent::SONG_TOGGLE_EFFECT, fxid, onoff);
      event.setS1(trackname);
      return PyBool_FromLong(sendPythonBridgeEvent(event));
}
//------------------------------------------------------------
// changeTrackName
//------------------------------------------------------------
PyObject* changeTrackName(PyObject*, PyObject* args)
{
      const char* trackname;
      const char* newname;

      if (!PyArg_ParseTuple(args, "ss", &trackname, &newname)) {
            Py_RETURN_NONE;
            }

      Track* t = MusEGlobal::song->findTrack(QString(trackname));
      if (t == nullptr)
            return Py_BuildValue("b", false);

      QPybridgeEvent* pyevent = new QPybridgeEvent(QPybridgeEvent::SONG_CHANGE_TRACKNAME);
      pyevent->setS1(trackname);
      pyevent->setS2(newname);
      QApplication::postEvent(MusEGlobal::song, pyevent);
      QPybridgeEvent* pyevent2 = new QPybridgeEvent(QPybridgeEvent::SONG_UPDATE, 0, 0, SC_TRACK_MODIFIED);
      QApplication::postEvent(MusEGlobal::song, pyevent2);
      return Py_BuildValue("b", true);
}
//------------------------------------------------------------
// addMidiTrack
//------------------------------------------------------------
PyObject* addMidiTrack(PyObject*, PyObject* args)
{
      const char* trackname = "";
      if (!PyArg_ParseTuple(args, "|s", &trackname))
            return nullptr;
      if (trackname[0] != '\0' && MusEGlobal::song->findTrack(QString(trackname)) != nullptr)
            Py_RETURN_FALSE;
      QPybridgeEvent event(QPybridgeEvent::SONG_ADD_TRACK, Track::MIDI);
      event.setS1(trackname);
      return PyBool_FromLong(sendPythonBridgeEvent(event));
}
//------------------------------------------------------------
// addWaveTrack
//------------------------------------------------------------
PyObject* addWaveTrack(PyObject*, PyObject*)
{
      QPybridgeEvent* pyevent = new QPybridgeEvent(QPybridgeEvent::SONG_ADD_TRACK, Track::WAVE);
      QApplication::postEvent(MusEGlobal::song, pyevent);
      Py_RETURN_NONE;
}
//------------------------------------------------------------
// addInput
//------------------------------------------------------------
PyObject* addInput(PyObject*, PyObject*)
{
      QPybridgeEvent* pyevent = new QPybridgeEvent(QPybridgeEvent::SONG_ADD_TRACK, Track::AUDIO_INPUT);
      QApplication::postEvent(MusEGlobal::song, pyevent);
      Py_RETURN_NONE;
}
//------------------------------------------------------------
// addOutput
//------------------------------------------------------------
PyObject* addOutput(PyObject*, PyObject*)
{
      QPybridgeEvent* pyevent = new QPybridgeEvent(QPybridgeEvent::SONG_ADD_TRACK, Track::AUDIO_OUTPUT);
      QApplication::postEvent(MusEGlobal::song, pyevent);
      Py_RETURN_NONE;
}
//------------------------------------------------------------
// addGroup
//------------------------------------------------------------
PyObject* addGroup(PyObject*, PyObject*)
{
      QPybridgeEvent* pyevent = new QPybridgeEvent(QPybridgeEvent::SONG_ADD_TRACK, Track::AUDIO_GROUP);
      QApplication::postEvent(MusEGlobal::song, pyevent);
      Py_RETURN_NONE;
}
//------------------------------------------------------------
// deleteTrack
//------------------------------------------------------------
PyObject* deleteTrack(PyObject*, PyObject* args)
{
      const char* trackname;

      if (!PyArg_ParseTuple(args, "s", &trackname)) {
            Py_RETURN_NONE;
            }

      QPybridgeEvent* pyevent = new QPybridgeEvent(QPybridgeEvent::SONG_DELETE_TRACK);
      pyevent->setS1(trackname);
      QApplication::postEvent(MusEGlobal::song, pyevent);
      Py_RETURN_NONE;
}
//------------------------------------------------------------
// getOutputRoute
//------------------------------------------------------------
/*
PyObject* getOutputRoute(PyObject*, PyObject* args)
{
      const char* trackname;

      if (!PyArg_ParseTuple(args, "s", &trackname)) {
            Py_RETURN_NONE;
            }

      Track* tt = MusEGlobal::song->findTrack(QString(trackname));
      if (tt == nullptr)
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
                 //             PyList_Append(routes, Py_BuildValue("s", s.toUtf8()));
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
// Get user supplied Pyro nameserver host name
//------------------------------------------------------------
PyObject* getConfigPyroNSHostname(PyObject*, PyObject*)
{
      return Py_BuildValue("s", MusEGlobal::pythonBridgePyroNSHostname.toUtf8().constData());
}

//------------------------------------------------------------
// Get user supplied Pyro nameserver host port
//------------------------------------------------------------
PyObject* getConfigPyroNSPort(PyObject*, PyObject*)
{
      return Py_BuildValue("s", MusEGlobal::pythonBridgePyroNSPort.toUtf8().constData());
}

//------------------------------------------------------------
// Get user supplied Pyro daemon host name
//------------------------------------------------------------
PyObject* getConfigPyroDaemonHostname(PyObject*, PyObject*)
{
      return Py_BuildValue("s", MusEGlobal::pythonBridgePyroDaemonHostname.toUtf8().constData());
}

//------------------------------------------------------------
// Get user supplied Pyro daemon host port
//------------------------------------------------------------
PyObject* getConfigPyroDaemonPort(PyObject*, PyObject*)
{
      return Py_BuildValue("s", MusEGlobal::pythonBridgePyroDaemonPort.toUtf8().constData());
}

//------------------------------------------------------------
// Get user supplied Pyro communication timeout
//------------------------------------------------------------
PyObject* getConfigPyroCommTimeout(PyObject*, PyObject*)
{
      return Py_BuildValue("f", MusEGlobal::pythonBridgePyroCommTimeout);
}

//------------------------------------------------------------
// Check whether the server should run (True) or stop
//------------------------------------------------------------
PyObject* serverRunFlag(PyObject*, PyObject*)
{
      return Py_BuildValue("b", g_pyroServerThread.serverRunFlag());
}


//------------------------------------------------------------
// Global method definitions for MusE:s Python API
//
// This is where global functions in Python is linked to their equivalent C/C++ functions
//------------------------------------------------------------
PyMethodDef g_methodDefinitions[] =
{
      { "getConfigPyroNSHostname", getConfigPyroNSHostname, METH_NOARGS, "Get configured Pyro nameserver host name" },
      { "getConfigPyroNSPort", getConfigPyroNSPort, METH_NOARGS, "Get configured Pyro nameserver host port" },
      { "getConfigPyroDaemonHostname", getConfigPyroDaemonHostname, METH_NOARGS, "Get configured Pyro daemon host name" },
      { "getConfigPyroDaemonPort", getConfigPyroDaemonPort, METH_NOARGS, "Get configured Pyro daemon host port" },
      { "getConfigPyroCommTimeout", getConfigPyroCommTimeout, METH_NOARGS, "Get configured Pyro communication timeout" },
      { "serverRunFlag", serverRunFlag, METH_NOARGS, "Check whether the server should run (True) or stop" },

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
      { "saveProjectFile", saveProjectFile, METH_VARARGS, "Save the current project to a MusE project file" },
      { "closeProjectFile", closeProjectFile, METH_VARARGS, "Save changes and close the current project" },
      { "createProjectFile", createProjectFile, METH_VARARGS, "Create a blank MusE project from the default template" },
      { "openProjectFile", openProjectFile, METH_VARARGS, "Save changes and open a MusE project file" },
      { "getMidiOutputs", getMidiOutputs, METH_NOARGS, "Get assigned MIDI output ports" },
      { "getMidiTrackOutput", getMidiTrackOutput, METH_VARARGS, "Get MIDI output port and channel assigned to a track" },
      { "getAvailableInstruments", getAvailableInstruments, METH_NOARGS, "Get available software instrument plugins" },
      { "addSynthTrack", addSynthTrack, METH_VARARGS, "Add a software instrument track and assign a MIDI port" },
      { "setMidiTrackOutput", setMidiTrackOutput, METH_VARARGS, "Set MIDI track output port and channel" },
      { "getMidiTrackPrograms", getMidiTrackPrograms, METH_VARARGS, "Get named MIDI programs for a track's output instrument" },
      { "setMidiTrackProgram", setMidiTrackProgram, METH_VARARGS, "Select a named MIDI program on a track's output instrument" },
      { "getParts", getParts, METH_VARARGS, "Get part data from a track" },
      { "createPart", createPart, METH_VARARGS, "Create a part" },
      { "modifyPart", modifyPart, METH_O, "Modify a particular part" },
      { "deletePart", deletePart, METH_VARARGS, "Remove part with a particular serial nr" },
      { "setPartLoop", setPartLoop, METH_VARARGS, "Set loop flag on a part by serial nr" },
      { "getPartLoop", getPartLoop, METH_VARARGS, "Get loop flag of a part by serial nr" },
      { "undo", undo, METH_NOARGS, "Undo the last operation" },
      { "redo", redo, METH_NOARGS, "Redo the last undone operation" },
      { "save_song", save_song, METH_VARARGS, "Save the current song to a file" },
      { "load_song", load_song, METH_VARARGS, "Load a song from a file" },
      { "createWavePart", createWavePart, METH_VARARGS, "Create a wave part on a wave track" },
      { "convertLoopToSingle", convertLoopToSingle, METH_VARARGS, "Convert looping part to single part" },
      { "convertLoopToMany", convertLoopToMany, METH_VARARGS, "Convert looping part to many parts" },
      { "convertLoopToSingleNewTrack", convertLoopToSingleNewTrack, METH_VARARGS, "Convert looping part to single part on new track" },
      { "convertLoopToManyNewTrack", convertLoopToManyNewTrack, METH_VARARGS, "Convert looping part to many parts on new track" },
      { "getSelectedTrack", getSelectedTrack, METH_NOARGS, "Get first selected track" },
      { "importPart", importPart, METH_VARARGS, "Import part file to a track at a particular position" },
      { "changeTrackName", changeTrackName, METH_VARARGS, "Change track name" },
      { "addMidiTrack", addMidiTrack, METH_VARARGS, "Add an optionally named MIDI track" },
      { "addWaveTrack", addWaveTrack, METH_NOARGS, "Add a wave track" },
      { "addInput", addInput, METH_NOARGS, "Add audio input" },
      { "addOutput", addOutput, METH_NOARGS, "Add audio output" },
      { "addGroup", addGroup, METH_NOARGS, "Add audio group" },
      { "deleteTrack", deleteTrack, METH_VARARGS, "Delete a track" },

      { "getTrackEffects", getTrackEffects, METH_VARARGS, "Get names of effects on a track" },
      { "getTrackEffectDetails", getTrackEffectDetails, METH_VARARGS, "Get effect slots and states on a track" },
      { "getTrackEffectParameters", getTrackEffectParameters, METH_VARARGS, "Get parameters for a track effect" },
      { "setTrackEffectParameter", setTrackEffectParameter, METH_VARARGS, "Set a parameter of a track effect" },
      { "getAvailableTrackEffects", getAvailableTrackEffects, METH_NOARGS, "Get available audio effect plugins" },
      { "addTrackEffect", addTrackEffect, METH_VARARGS, "Add an effect plugin to a track" },
      { "toggleTrackEffect", toggleTrackEffect, METH_VARARGS, "Toggle an effect on or off" },
      //{ "getOutputRoute", getOutputRoute, METH_VARARGS, "Get route for an audio output" },

      { "setSongLen", setSongLen, METH_VARARGS, "Set length of song (in ticks)" },
      { "getSongLen", getSongLen, METH_VARARGS, "Get length of song (in ticks)" },

      { "getMute", getMute, METH_VARARGS, "Get track mute property (if track is played or not)" },
      { "setMute", setMute, METH_VARARGS, "Set track mute property (if track should be played or not)" },
      { "setMidiControllerValue", setMidiControllerValue, METH_VARARGS, "Set midi controller value for a track" },
      { "getMidiControllerValue", getMidiControllerValue, METH_VARARGS, "Get midi controller value for a track" },
      { "setAudioTrackVolume", setAudioTrackVolume, METH_VARARGS, "Set volume on audio track/aux/output/input" },
      { "getAudioTrackVolume", getAudioTrackVolume, METH_VARARGS, "Get audio track/aux/output/input volume" },

      { "setMidiTrackParameter", setMidiTrackParameter, METH_VARARGS, "Set transposition, velocity, compression, delay or note length on track level" },

      { "getDivision", getDivision, METH_VARARGS, "Number of ticks per 1/4 (?)" },

      {nullptr, nullptr, 0, nullptr}
};

bool PyroServerThread::initServer()
{
      _runServer = false;
      const char* mod_name = "muse";

      // Initialize Python on the main thread, then release the GIL when done.
      // The Pyro server runs in a separate QThread and acquires the GIL
      //   with PyGILState_Ensure() before calling into Python.
      Py_Initialize();

// For Python 3.5 and above:
#if (PY_MAJOR_VERSION >= 4) || (PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION >= 5)

      PyObject *pMuseModule = PyImport_AddModule(mod_name);
      PyModule_AddFunctions(pMuseModule, g_methodDefinitions);

// For Python 3.0 to 3.4:
#elif PY_MAJOR_VERSION >= 3 && PY_MINOR_VERSION < 5

      PyObject *m = PyImport_AddModule(mod_name);

      // Stupid HACK required. Tested OK. Borrowed from and based on these:
      // http://code.activestate.com/recipes/579110-add-function-to-pythons-__builtin__-module-through/
      // https://stackoverflow.com/questions/6565175/adding-new-command-to-module-through-c-api
      PyObject *po_mod_dict = PyModule_GetDict(m);
      PyObject *po_mod_name = PyUnicode_FromString(mod_name);
      PyObject *po_new_func;
      int i = 0;
      while(g_methodDefinitions[i].ml_name && g_methodDefinitions[i].ml_meth)
      {
        po_new_func = PyCFunction_NewEx(&g_methodDefinitions[i], (PyObject*)nullptr, po_mod_name);
        PyDict_SetItemString(po_mod_dict, g_methodDefinitions[i].ml_name, po_new_func);
        ++i;
      }

// For Python below 3.0:
#else

      // Life was simple, eh?
      Py_InitModule( mod_name, g_methodDefinitions );

#endif

      //
      // Access the "__main__" module and its name-space dictionary.
      //

      g_pMainModule     = PyImport_AddModule( "__main__" );
      g_pMainDictionary = PyModule_GetDict( g_pMainModule );

      // Release GIL so other threads can use Python.
      PyEval_SaveThread();

      return true;
}

/**
 * This function launches the Pyro name service, which blocks execution
 * Thus it needs its own thread
 **/
void PyroServerThread::run()
{
      if(!g_pMainDictionary)
        return;
  
      _runServer = true;

      string launcherfilename = string(SHAREDIR) + string("/pybridge/museplauncher.py");
      fprintf(stderr, "Initiating MusE Pybridge launcher from %s\n", launcherfilename.c_str());
      FILE* fp = fopen(launcherfilename.c_str(),"r");
      if(!fp)
      {
        fprintf(stderr, "MusE Pybridge open launcher file failed\n");
      }
      else
      {
        fprintf(stderr, "About to acquire GIL\n");
        PyGILState_STATE state = PyGILState_Ensure();
        fprintf(stderr, "GIL acquired\n");

        PyObject* sysPath = PySys_GetObject("path");
        PyObject* bridgePath = PyUnicode_FromString((string(SHAREDIR) + string("/pybridge")).c_str());
        if (sysPath && bridgePath && PyList_Check(sysPath)) {
            const int pathPresent = PySequence_Contains(sysPath, bridgePath);
            if (pathPresent == 0)
                PyList_Insert(sysPath, 0, bridgePath);
            else if (pathPresent < 0)
                PyErr_Clear();
        }
        Py_XDECREF(bridgePath);

        PyObject* p_res = PyRun_File(fp, launcherfilename.c_str(), Py_file_input, g_pMainDictionary, g_pMainDictionary);
        if(p_res == nullptr)
        {
            fprintf(stderr, "MusE Pybridge initialization failed\n");
            PyErr_Print();
        }
        fclose(fp);

        PyGILState_Release(state);

        fprintf(stderr, "MusE Pybridge finished\n");
      }
}

void PyroServerThread::stop()
{
  // Flag the server to stop, when it eventually gets around to reading this...
  _runServer = false;
}

/**
 * This function currently only launches the thread. There should be some kind of check that
 * things are up and running as they are supposed to
 */
bool startPythonBridge()
{
  g_pyroServerThread.initServer();
  g_pyroServerThread.start();
  return true;
}

bool stopPythonBridge()
{
  g_pyroServerThread.stop();
  // Be sure to set this to something higher than what the daemon will use,
  //  so that the thread does not timeout before the daemon does. Say, * 1.5.
  // In units of ms.
  return g_pyroServerThread.wait(1500.0 * MusEGlobal::pythonBridgePyroCommTimeout);
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
                  this->update(e->getSC1());
                  break;
            case QPybridgeEvent::SONGLEN_CHANGE:
                  this->setLen(e->getP1());
                  break;
            case QPybridgeEvent::SONG_POSCHANGE:
                  this->setPos(POSTYPE(e->getP1()), e->getP2());
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
                  if (track == nullptr)
                        return false;

                  bool muted = e->getP1() == 1;
                  // No undo.
                  // Operation is undoable but do not start/end undo.
                  MusEGlobal::song->applyOperation(MusECore::UndoOp(
                    MusECore::UndoOp::SetTrackMute, track, muted, double(0), double(0), double(0), double(0)),
                                                   MusECore::Song::OperationUndoableUpdate);
                  break;
                  }
            case QPybridgeEvent::SONG_SETCTRL: {
                  Track* t = this->findTrack(e->getS1());
                  if (t == nullptr)
                        return false;

                  if (t->isMidiTrack() == false)
                        return false;

                  MidiTrack* track = (MidiTrack*) t;
                  int chan  = track->outChannel();

                  int num = e->getP1();
                  int val = e->getP2();
                  // Schedule for immediate playback.
                  MidiPlayEvent ev(0, track->outPort(), chan, ME_CONTROLLER, num, val);
                  MusEGlobal::audio->msgPlayMidiEvent(&ev);
                  break;
                  }
            case QPybridgeEvent::SONG_SETAUDIOVOL: {
                  Track* t = this->findTrack(e->getS1());
                  if (t == nullptr)
                        return false;

                  if (t->isMidiTrack()) // changed by flo. is better maintainable
                  //if (t->type() == Track::DRUM || t->type() == Track::NEW_DRUM || t->type() == Track::MIDI)
                        return false;

                  AudioTrack* track = (AudioTrack*) t;
                  track->setVolume(e->getD1());
                  break;
                  }
            case QPybridgeEvent::SONG_IMPORT_PART: {
                  Track* track = this->findTrack(e->getS1());
                  QString filename = e->getS2();
                  unsigned int tick = e->getP1();
                  if (track == nullptr)
                        return false;

                  MusEGlobal::muse->importPartToTrack(filename, tick, track);
                  break;
                  }
            case QPybridgeEvent::SONG_TOGGLE_EFFECT: {
                  Track* t = this->findTrack(e->getS1());
                  if (t == nullptr)
                        return false;

                  AudioTrack* track = dynamic_cast<AudioTrack*>(t);
                  if (track == nullptr)
                        return false;

                  int fxid = e->getP1();

                  if (fxid < 0 || fxid >= PipelineDepth || track->efxPipe()->empty(fxid))
                        return false;

                  int onoff = (e->getP2() == 1);
                  Pipeline* pipeline = track->efxPipe();
                  pipeline->setOn(fxid, onoff);
                  break;
                  }
            case QPybridgeEvent::SONG_SET_MIDI_OUTPUT: {
                  Track* t = this->findTrack(e->getS1());
                  const int port = e->getP1();
                  if (t == nullptr || !t->isMidiTrack() || MusEGlobal::audio == nullptr ||
                      port < 0 || port >= MusECore::MIDI_PORTS ||
                      e->getP2() < 0 || e->getP2() >= MusECore::MUSE_MIDI_CHANNELS ||
                      !MusEGlobal::midiPorts[port].device())
                        return false;

                  MidiTrack* track = static_cast<MidiTrack*>(t);
                  MusEGlobal::audio->msgIdle(true);
                  const MidiTrack::ChangedType_t changed = track->setOutPortAndChannelAndUpdate(
                        e->getP1(), e->getP2(), false);
                  MusEGlobal::audio->msgIdle(false);
                  MusEGlobal::audio->msgUpdateSoloStates();
                  this->update(SC_ROUTE | ((changed & MidiTrack::DrumMapChanged) ? SC_DRUMMAP : 0));
                  break;
                  }
            case QPybridgeEvent::SONG_SET_MIDI_PROGRAM: {
                  Track* t = this->findTrack(e->getS1());
                  if (t == nullptr || !t->isMidiTrack() || MusEGlobal::audio == nullptr)
                        return false;
                  MidiTrack* track = static_cast<MidiTrack*>(t);
                  const int port = track->outPort();
                  if (port < 0 || port >= MusECore::MIDI_PORTS || !MusEGlobal::midiPorts[port].device())
                        return false;
                  const int patchCode = e->getP1();
                  if (patchCode < 0 || patchCode > 0xffffff)
                        return false;
                  MidiInstrument* instrument = MusEGlobal::midiPorts[port].instrument();
                  if (instrument == nullptr)
                        return false;
                  bool foundPatch = false;
                  const bool drum = track->isDrumTrack();
                  for (const dumb_patchlist_entry_t& patch : instrument->getPatches(track->outChannel(), drum)) {
                        const int lbank = patch.lbank < 0 ? 0xff : patch.lbank;
                        const int hbank = patch.hbank < 0 ? 0xff : patch.hbank;
                        const int code = (patch.prog & 0xff) | ((lbank & 0xff) << 8) | ((hbank & 0xff) << 16);
                        if (code == patchCode) {
                              foundPatch = true;
                              break;
                              }
                        }
                  if (!foundPatch)
                        return false;
                  const int hbank = (patchCode >> 16) & 0xff;
                  const int lbank = (patchCode >> 8) & 0xff;
                  const int channel = track->outChannel();
                  if (hbank != 0xff) {
                        MidiPlayEvent bankMsb(0, port, channel, ME_CONTROLLER, 0, hbank);
                        MusEGlobal::audio->msgPlayMidiEvent(&bankMsb);
                        }
                  if (lbank != 0xff) {
                        MidiPlayEvent bankLsb(0, port, channel, ME_CONTROLLER, 32, lbank);
                        MusEGlobal::audio->msgPlayMidiEvent(&bankLsb);
                        }
                  MidiPlayEvent program(0, port, channel, ME_PROGRAM, patchCode & 0xff, 0);
                  MusEGlobal::audio->msgPlayMidiEvent(&program);
                  break;
                  }
            case QPybridgeEvent::SONG_ADD_TRACK_EFFECT: {
                  Track* t = this->findTrack(e->getS1());
                  AudioTrack* track = dynamic_cast<AudioTrack*>(t);
                  const int fxid = e->getP1();
                  if (track == nullptr || fxid < 0 || fxid >= PipelineDepth || !track->efxPipe()->empty(fxid))
                        return false;

                  Plugin* plugin = MusEGlobal::plugins.find(
                        MusEPlugin::PluginTypesAll, e->getS3(), e->getS2(), e->getS4());
                  if (plugin == nullptr || plugin->pluginClass() != MusEPlugin::PluginClassEffect)
                        return false;

                  PluginI* pluginI = new PluginI();
                  if (pluginI->initPluginInstance(plugin, track->channels())) {
                        delete pluginI;
                        return false;
                        }
                  MusEGlobal::song->applyOperation(UndoOp(UndoOp::ChangeRackEffectPlugin, track, pluginI, fxid));
                  break;
                  }
            case QPybridgeEvent::SONG_SET_TRACK_EFFECT_PARAMETER: {
                  Track* t = this->findTrack(e->getS1());
                  AudioTrack* track = dynamic_cast<AudioTrack*>(t);
                  const int fxid = e->getP1();
                  const int parameter = e->getP2();
                  if (track == nullptr || MusEGlobal::audio == nullptr || fxid < 0 || fxid >= PipelineDepth ||
                      track->efxPipe()->empty(fxid))
                        return false;
                  PluginI* plugin = track->efxPipe()->at(fxid);
                  if (parameter < 0 || static_cast<unsigned long>(parameter) >= plugin->parameters())
                        return false;
                  const double value = e->getD1();
                  float minimum = 0.0f;
                  float maximum = 0.0f;
                  plugin->range(static_cast<unsigned long>(parameter), &minimum, &maximum);
                  if (!std::isfinite(value) || std::abs(value) > 1.0e9 || !std::isfinite(minimum) ||
                      !std::isfinite(maximum) || minimum > maximum || value < minimum || value > maximum)
                        return false;
                  plugin->setParam(static_cast<unsigned long>(parameter), value);
                  plugin->enableController(static_cast<unsigned long>(parameter), false);
                  break;
                  }
            case QPybridgeEvent::SONG_ADD_TRACK: {
                  if (!e->getS1().isEmpty() && this->findTrack(e->getS1()) != nullptr)
                        return false;
                  Track* track = MusEGlobal::song->addTrack((Track::TrackType)e->getP1());  // Add at end of list.
                  if (track == nullptr)
                        return false;
                  if (!e->getS1().isEmpty())
                        track->setName(e->getS1());
                  break;
                  }
            case QPybridgeEvent::SONG_ADD_SYNTH: {
                  if (this->findTrack(e->getS1()) != nullptr || MusEGlobal::audio == nullptr)
                        return false;
                  Synth* synthInfo = MusEGlobal::synthis.find(
                        MusEPlugin::PluginTypesAll, e->getS2(), e->getS3(), e->getS4());
                  if (synthInfo == nullptr || synthInfo->pluginType() != MusEPlugin::PluginType(e->getP1()))
                        return false;
                  int freeMidiPort = -1;
                  for (int i = 0; i < MusECore::MIDI_PORTS; ++i) {
                        if (MusEGlobal::midiPorts[i].device() == nullptr) {
                              freeMidiPort = i;
                              break;
                              }
                        }
                  if (freeMidiPort < 0)
                        return false;
                  SynthI* synth = this->createSynthI(MusEPlugin::PluginType(e->getP1()),
                        e->getS2(), e->getS3(), e->getS4());
                  if (synth == nullptr)
                        return false;
                  synth->setName(e->getS1());
                  MidiPort* port = &MusEGlobal::midiPorts[freeMidiPort];
                  if (port->device() != nullptr) {
                        MusEGlobal::song->applyOperation(
                              UndoOp(UndoOp::DeleteTrack, MusEGlobal::song->tracks()->index(synth), synth));
                        return false;
                        }
                  MusEGlobal::audio->msgSetMidiDevice(port, synth, synth);
                  this->update();
                  e->setResult(freeMidiPort);
                  break;
                  }
            case QPybridgeEvent::SONG_CHANGE_TRACKNAME: {
                  Track* t = this->findTrack(e->getS1());
                  if (t == nullptr)
                        return false;
                  t->setName(e->getS2());
                  break;
                  }
            case QPybridgeEvent::SONG_DELETE_TRACK: {
                  Track* t = this->findTrack(e->getS1());
                  if (t == nullptr)
                        return false;

                  MusEGlobal::song->applyOperation(UndoOp(UndoOp::DeleteTrack, MusEGlobal::song->tracks()->index(t), t));
                  break;
                  }
            default:
                  fprintf(stderr, "Unknown pythonthread event received: %d\n", e->getType());
                  break;
            }


      return true;
}

} // namespace MusECore

