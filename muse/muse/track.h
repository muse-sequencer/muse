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

#ifndef __TRACK_H__
#define __TRACK_H__

#include "driver/port.h"
#include "al/pos.h"
#include "route.h"
#include "ctrl.h"
#include "globaldefs.h"
#include "midievent.h"
#include "midififo.h"

namespace AL {
      class Xml;
      enum TType;
      };
using AL::Xml;
using AL::TType;

class DrumMap;
class MidiPipeline;
class MidiEvent;
class MPEventList;
class SynthI;
class MidiPlugin;
class MidiPluginI;
class MidiInstrument;
class PartList;
class Part;
class MidiOutPort;
class MidiInPort;
class MidiChannel;

#ifndef __APPLE__
// actually it should check for ALSA but I don't know how to do that
typedef struct snd_seq_event snd_seq_event_t;
#endif

static const int EVENT_FIFO_SIZE = 128;

//---------------------------------------------------------
//   ArrangerTrack
//---------------------------------------------------------

struct ArrangerTrack {
      QWidget* tw;      // tracklist widget
      int ctrl;
      int h;            // tmp val used by readProperties()
      Ctrl* controller;

      ArrangerTrack();
      };

typedef std::list<ArrangerTrack*> ArrangerTrackList;
typedef ArrangerTrackList::iterator iArrangerTrack;
typedef ArrangerTrackList::const_iterator ciArrangerTrack;

//---------------------------------------------------------
//   Track
//---------------------------------------------------------

class Track : public QObject {
      Q_OBJECT

   public:
      enum TrackType {
            AUDIO_OUTPUT,
            AUDIO_GROUP,
            WAVE,
            AUDIO_INPUT,
            AUDIO_SOFTSYNTH,
            MIDI,
            MIDI_OUT,
            MIDI_IN,
            MIDI_CHANNEL,
            MIDI_SYNTI,
            TRACK_TYPES
            };
      enum {
            M_AUDIO_OUTPUT    = 1 << AUDIO_OUTPUT,
            M_AUDIO_GROUP     = 1 << AUDIO_GROUP,
            M_WAVE            = 1 << WAVE,
            M_AUDIO_INPUT     = 1 << AUDIO_INPUT,
            M_AUDIO_SOFTSYNTH = 1 << AUDIO_SOFTSYNTH,
            M_MIDI            = 1 << MIDI,
            M_MIDI_OUT        = 1 << MIDI_OUT,
            M_MIDI_IN         = 1 << MIDI_IN,
            M_MIDI_CHANNEL    = 1 << MIDI_CHANNEL,
            M_MIDI_SYNTI      = 1 << MIDI_SYNTI
            };

   private:
      TrackType _type;
      QString _comment;
      PartList* _parts;
      Port _alsaPort[MAX_CHANNELS], _jackPort[MAX_CHANNELS];

      void init();

   protected:
      TType _tt;            // time type
      QString _name;
      bool _recordFlag;
      bool _mute;
      bool _solo;
      bool _off;
      bool _monitor;
      int _channels;                // 1 - mono, 2 - stereo
                                    // Note: midi out/in tracks have 
                                    // 1 channel
      CtrlRecList _recEvents;       // recorded automation events
      double _meter[MAX_CHANNELS];
      double _peak[MAX_CHANNELS];
      int _peakTimer[MAX_CHANNELS];
      bool _locked;                 // true if parts are locked to frames
      bool _selected;

      RouteList _inRoutes, _outRoutes;
      CtrlList _controller;

      bool _autoRead, _autoWrite;

      bool readProperties(QDomNode);
      void writeProperties(Xml& xml) const;

      virtual bool setMute(bool val);
      virtual bool setOff(bool val);
      virtual bool setSolo(bool val);

   signals:
      void recordChanged(bool);
      void autoReadChanged(bool);
      void autoWriteChanged(bool);
      void clChanged();
      void controllerChanged(int id);
      void selectionChanged(bool);
      void muteChanged(bool);
      void soloChanged(bool);
      void monitorChanged(bool);
      void offChanged(bool);
      void partsChanged();
      void nameChanged(const QString&);
      void routeChanged();

   private slots:
      void setAutoRead(bool);
      void setAutoWrite(bool);

   public:
      Track();
      Track(TrackType);
      virtual ~Track();

      static const char* _cname[];
      static const char* _clname[];

      QString comment() const           { return _comment; }
      void setComment(const QString& s) { _comment = s; }
      TType timeType() const            { return _tt; }
      void setTimeType(TType t)         { _tt = t; }

      QString cname() const             { return QString(_cname[_type]); }
      QString clname() const            { return QString(_clname[_type]); }

      //
      // called before and after use
      //    (undo/redo)
      // connects/reconnects to the outside world
      //
      void activate1();
      void activate2();
      void deactivate();

      //----------------------------------------------------------
      //   controller handling
      //----------------------------------------------------------

      CtrlList* controller()             { return &_controller; }
      const CtrlList* controller() const { return &_controller; }

      ControllerNameList* controllerNames() const;
      void addController(Ctrl*);
      void addMidiController(MidiInstrument*, int ctrl);
      void removeController(int id);
      Ctrl* getController(int id) const;

      int hwCtrlState(int ctrl) const;
      void setHwCtrlState(int ctrl, int val);

      // current value:
      CVal ctrlVal(int id)  { return getController(id)->schedVal(); }

      // editor interface:
      bool addControllerVal(int id, unsigned pos, CVal);
      void removeControllerVal(int id, unsigned pos);
      int getCtrl(int tick, int ctrl) const;

      // signal interface:
      virtual void emitControllerChanged(int id) { emit controllerChanged(id); }
      void updateController();

      // automation
      void startAutoRecord(int);
      void stopAutoRecord(int);
      CtrlRecList* recEvents()      { return &_recEvents; }
      //----------------------------------------------------------

      QColor ccolor() const;
      QPixmap* pixmap() const         { return pixmap(_type); }
      static QPixmap* pixmap(TrackType);

      bool selected() const           { return _selected; }
      void setSelected(bool f);

      bool locked() const             { return _locked; }
      void setLocked(bool b)          { _locked = b; }

      const QString name() const      { return _name; }
      virtual void setName(const QString& s);

      TrackType type() const          { return _type; }
      void setType(TrackType t)       { _type = t; }

      PartList* parts()               { return _parts; }
      const PartList* cparts() const  { return _parts; }
      Part* findPart(unsigned tick);
	void addPart(Part* p);

      virtual void write(Xml&) const = 0;
      virtual Track* newTrack() const = 0;

      void setRecordFlag(bool);
      virtual Part* newPart(Part*p=0, bool clone = false) = 0;
      void dump() const;
      virtual void splitPart(Part*, int, Part*&, Part*&);

      void setMonitor(bool val);
      virtual bool isMute() const = 0;

      bool monitor() const               { return _monitor; }
      bool solo() const                  { return _solo;         }
      bool mute() const                  { return _mute;         }
      bool off() const                   { return _off;          }
      bool recordFlag() const            { return _recordFlag;   }

      void resetMeter();
      void resetPeaks();
      static void resetAllMeter();
      double meter(int ch) const          { return _meter[ch]; }
      void addMidiMeter(int velo) {
            _meter[0] += velo/5;
            if (_meter[0] > 127.0f)
                  _meter[0] = 127.0f;
            }
      double peak(int ch) const           { return _peak[ch];      }
      void setPeak(int ch, double v)      { _peak[ch] = v;         }
      void resetPeak(int ch) {
            _peak[ch] = 0;
            _peakTimer[ch] = 0;
            }
      int peakTimer(int ch) const        { return _peakTimer[ch]; }
      void setPeakTimer(int ch, int v)   { _peakTimer[ch] = v;    }
      void setMeter(int ch, double v)     {
            _meter[ch] = v;
		if (v > _peak[ch]) {
			_peak[ch] = v;
      		_peakTimer[ch] = 0;
      		}
		}

      void setDefaultName();
      int channels() const               { return _channels; }
      virtual void setChannels(int n);
      bool isMidiTrack() const       {
            return type() == MIDI
               || type() == MIDI_IN
               || type() == MIDI_OUT
               || type() == MIDI_CHANNEL
               || type() == MIDI_SYNTI;
            }
      virtual bool canRecord() const { return false; }

      bool autoRead() const       { return _autoRead; }
      bool autoWrite() const      { return _autoWrite; }

      void partListChanged() { emit partsChanged(); }
      void updateMute()      { emit muteChanged(isMute()); }
      unsigned cpos() const;

      // routing
      RouteList* inRoutes()    { return &_inRoutes; }
      RouteList* outRoutes()   { return &_outRoutes; }
      bool noInRoute() const   { return _inRoutes.empty();  }
      bool noOutRoute() const  { return _outRoutes.empty(); }
      void writeRouting(Xml&) const;

      Port alsaPort(int channel = 0) const    { return _alsaPort[channel]; }
      Port jackPort(int channel = 0) const    { return _jackPort[channel]; }

      void setAlsaPort(const Port& port, int channel = 0) { 
            _alsaPort[channel] = port; 
            }
      void setJackPort(const Port& port, int channel = 0) { 
            _jackPort[channel] = port; 
            }

      struct ArrangerTrack arrangerTrack;
      ArrangerTrackList subtracks;

      friend class Song;
      };

//---------------------------------------------------------
//   MidiTrackBase
//---------------------------------------------------------

class MidiTrackBase : public Track {
      Q_OBJECT

      MidiPipeline* _pipeline;

   public:
      MidiTrackBase(TrackType t);
      virtual ~MidiTrackBase();

      bool readProperties(QDomNode);
      void writeProperties(Xml&) const;

      MidiPipeline* pipeline()      { return _pipeline;  }
      void addPlugin(MidiPluginI* plugin, int idx);
      MidiPluginI* plugin(int idx) const;

      virtual void getEvents(unsigned /*from*/, unsigned /*to*/, int /*channel*/, MPEventList* /*dst*/) {}
      };

//---------------------------------------------------------
//   TrackList
//---------------------------------------------------------

template<class T> class tracklist : public std::vector<Track*> {
      typedef std::vector<Track*> vlist;

   public:
      class iterator : public vlist::iterator {
         public:
            iterator() : vlist::iterator() {}
            iterator(vlist::iterator i) : vlist::iterator(i) {}

            T operator*() {
                  return (T)(**((vlist::iterator*)this));
                  }
            iterator operator++(int) {
                  return iterator ((*(vlist::iterator*)this).operator++(0));
                  }
            iterator& operator++() {
                  return (iterator&) ((*(vlist::iterator*)this).operator++());
                  }
            };

      class const_iterator : public vlist::const_iterator {
         public:
            const_iterator() : vlist::const_iterator() {}
            const_iterator(vlist::const_iterator i) : vlist::const_iterator(i) {}
            const_iterator(vlist::iterator i) : vlist::const_iterator(i) {}

            const T operator*() const {
                  return (T)(**((vlist::const_iterator*)this));
                  }
            };

      class reverse_iterator : public vlist::reverse_iterator {
         public:
            reverse_iterator() : vlist::reverse_iterator() {}
            reverse_iterator(vlist::reverse_iterator i) : vlist::reverse_iterator(i) {}

            T operator*() {
                  return (T)(**((vlist::reverse_iterator*)this));
                  }
            };

      tracklist() : vlist() {}
      virtual ~tracklist() {}

      void push_back(T v)             { vlist::push_back(v); }
      iterator begin()                { return vlist::begin(); }
      iterator end()                  { return vlist::end(); }
      const_iterator begin() const    { return vlist::begin(); }
      const_iterator end() const      { return vlist::end(); }
      reverse_iterator rbegin()       { return vlist::rbegin(); }
      reverse_iterator rend()         { return vlist::rend(); }
      T& back() const                 { return (T&)(vlist::back()); }
      T& front() const                { return (T&)(vlist::front()); }
      iterator find(const Track* t)       {
            return std::find(begin(), end(), t);
            }
      const_iterator find(const Track* t) const {
            return std::find(begin(), end(), t);
            }
      unsigned index(const Track* t) const {
            unsigned n = 0;
            for (vlist::const_iterator i = begin(); i != end(); ++i, ++n) {
                  if (*i == t)
                        return n;
                  }
            return -1;
            }
      T index(int k) const       { return (T)((*this)[k]); }

      iterator index2iterator(int k) {
            if ((unsigned)k >= size())
                  return end();
            return begin() + k;
            }
      void erase(Track* t)           { vlist::erase(find(t)); }

      void clearDelete() {
            for (vlist::iterator i = begin(); i != end(); ++i)
                  delete *i;
            vlist::clear();
            }
      void erase(vlist::iterator i) { vlist::erase(i); }
      void replace(Track* ot, Track* nt) {
            for (vlist::iterator i = begin(); i != end(); ++i) {
                  if (*i == ot) {
                        *i = nt;
                        return;
                        }
                  }
            }
      };

typedef tracklist<Track*> TrackList;
typedef TrackList::iterator iTrack;
typedef TrackList::reverse_iterator irTrack;
typedef TrackList::const_iterator ciTrack;

typedef tracklist<MidiInPort*>::iterator iMidiInPort;
typedef tracklist<MidiInPort*>::const_iterator ciMidiInPort;
typedef tracklist<MidiInPort*> MidiInPortList;

typedef tracklist<MidiOutPort*>::iterator iMidiOutPort;
typedef tracklist<MidiOutPort*>::const_iterator ciMidiOutPort;
typedef tracklist<MidiOutPort*> MidiOutPortList;

typedef tracklist<MidiChannel*>::iterator iMidiChannel;
typedef tracklist<MidiChannel*>::const_iterator ciMidiChannel;
typedef tracklist<MidiChannel*> MidiChannelList;

#endif

