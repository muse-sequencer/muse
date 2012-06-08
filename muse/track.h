//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: track.h,v 1.39.2.17 2009/12/20 05:00:35 terminator356 Exp $
//
//  (C) Copyright 1999-2004 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011 Tim E. Real (terminator356 on sourceforge)
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

#ifndef __TRACK_H__
#define __TRACK_H__

#include <QString>

#include <vector>
#include <algorithm>

#include "wave.h" // for SndFileR
#include "part.h"
#include "key.h"
#include "node.h"
#include "route.h"
#include "ctrl.h"
#include "globaldefs.h"
#include "cleftypes.h"

namespace MusECore {
class MPEventList;
class Pipeline;
class PluginI;
class SynthI;
class Xml;
class ControlEvent;

//---------------------------------------------------------
//   Track
//---------------------------------------------------------

class Track {
   public:
      enum TrackType {
         MIDI=0, DRUM, WAVE, AUDIO_OUTPUT, AUDIO_INPUT, AUDIO_GROUP,
         AUDIO_AUX, AUDIO_SOFTSYNTH
         };
      enum AssignFlags {
         ASSIGN_PROPERTIES=1, ASSIGN_PARTS=2, ASSIGN_PLUGINS=4, ASSIGN_STD_CTRLS=8, ASSIGN_PLUGIN_CTRLS=16, ASSIGN_ROUTES=32, ASSIGN_DEFAULT_ROUTES=64 };
   private:
      TrackType _type;
      QString _comment;

      PartList _parts;

      void init();
      void internal_assign(const Track&, int flags);

   protected:
      static unsigned int _soloRefCnt;
      static Track* _tmpSoloChainTrack;
      static bool _tmpSoloChainDoIns;
      static bool _tmpSoloChainNoDec;
      
      RouteList _inRoutes;
      RouteList _outRoutes;
      bool _nodeTraversed;   // Internal anti circular route traversal flag.
      int _auxRouteCount;    // Number of aux paths feeding this track.
      
      QString _name;
      bool _recordFlag;
      bool _mute;
      bool _solo;
      unsigned int _internalSolo;
      bool _off;
      int _channels;          // 1 - mono, 2 - stereo

      bool _volumeEnCtrl;
      bool _volumeEn2Ctrl;
      bool _panEnCtrl;
      bool _panEn2Ctrl;
      
      int _activity;
      int _lastActivity;
      double _meter[MAX_CHANNELS];
      double _peak[MAX_CHANNELS];

      int _y;
      int _height;            // visual height in arranger

      bool _locked;
      bool _selected;
      bool readProperties(Xml& xml, const QString& tag);
      void writeProperties(int level, Xml& xml) const;

   public:
      Track(TrackType);
      Track(const Track&, int flags);
      virtual ~Track();
      virtual void assign(const Track&, int flags);
      
      static const char* _cname[];
      
      QString comment() const         { return _comment; }
      void setComment(const QString& s) { _comment = s; }

      int y() const;
      void setY(int n)                { _y = n;         }
      virtual int height() const = 0;
      void setHeight(int n)           { _height = n;    }

      bool selected() const           { return _selected; }
      void setSelected(bool f)        { _selected = f; }
      bool locked() const             { return _locked; }
      void setLocked(bool b)          { _locked = b; }

      bool volumeControllerEnabled()  const  { return _volumeEnCtrl; }
      bool volumeControllerEnabled2() const  { return _volumeEn2Ctrl; }
      bool panControllerEnabled()     const  { return _panEnCtrl; }
      bool panControllerEnabled2()    const  { return _panEn2Ctrl; }
      void enableVolumeController(bool b)    { _volumeEnCtrl = b; }
      void enable2VolumeController(bool b)   { _volumeEn2Ctrl = b; }
      void enablePanController(bool b)       { _panEnCtrl = b; }
      void enable2PanController(bool b)      { _panEn2Ctrl = b; }
      void clearRecAutomation(bool clearList);
      
      const QString& name() const     { return _name; }
      virtual void setName(const QString& s)  { _name = s; }

      TrackType type() const          { return _type; }
      void setType(TrackType t)       { _type = t; }
      QString cname() const           { int t = type(); return QString(_cname[t]); }

      // routing
      RouteList* inRoutes()    { return &_inRoutes; }
      RouteList* outRoutes()   { return &_outRoutes; }
      bool noInRoute() const   { return _inRoutes.empty();  }
      bool noOutRoute() const  { return _outRoutes.empty(); }
      void writeRouting(int, Xml&) const;
      bool isCircularRoute(Track* dst);   
      int auxRefCount() const { return _auxRouteCount; }  // Number of Aux Tracks with routing paths to this track. 
      void updateAuxRoute(int refInc, Track* dst);  // Internal use. 
      
      PartList* parts()               { return &_parts; }
      const PartList* cparts() const  { return &_parts; }
      Part* findPart(unsigned tick);
      iPart addPart(Part* p);

      virtual void write(int, Xml&) const = 0;

      virtual Track* newTrack() const = 0;
      virtual Track* clone(int flags) const    = 0;

      virtual bool setRecordFlag1(bool f) = 0;
      virtual void setRecordFlag2(bool f) = 0;

      virtual Part* newPart(Part*p=0, bool clone = false) = 0;
      void dump() const;
      virtual void splitPart(Part*, int, Part*&, Part*&);

      virtual void setMute(bool val);
      virtual void setOff(bool val);
      void setInternalSolo(unsigned int val);
      virtual void setSolo(bool val) = 0;
      virtual bool isMute() const = 0;
      unsigned int internalSolo() const  { return _internalSolo; }
      bool soloMode() const              { return _soloRefCnt; }
      bool solo() const                  { return _solo;         }
      bool mute() const                  { return _mute;         }
      bool off() const                   { return _off;          }
      bool recordFlag() const            { return _recordFlag;   }

      // Internal use...
      static void clearSoloRefCounts();
      void updateSoloState();
      virtual void updateSoloStates(bool noDec) = 0;  
      virtual void updateInternalSoloStates();        

      int activity()                     { return _activity;     }
      void setActivity(int v)            { _activity = v;        }
      int lastActivity()                 { return _lastActivity; }
      void setLastActivity(int v)        { _lastActivity = v;    }
      void addActivity(int v)            { _activity += v;       }
      void resetPeaks();
      static void resetAllMeter();
      double meter(int ch) const  { return _meter[ch]; }
      double peak(int ch) const   { return _peak[ch]; }
      void resetMeter();

      bool readProperty(Xml& xml, const QString& tag);
      void setDefaultName(QString base = QString());
      int channels() const                { return _channels; }
      virtual void setChannels(int n);
      bool isMidiTrack() const       { return type() == MIDI || type() == DRUM; }
      virtual bool canRecord() const { return false; }
      virtual AutomationType automationType() const    = 0;
      virtual void setAutomationType(AutomationType t) = 0;
      static void setVisible(bool ) { }
      bool isVisible();

      };

//---------------------------------------------------------
//   MidiTrack
//---------------------------------------------------------

class MidiTrack : public Track {
      int _outPort;
      int _outChannel;
      bool _recEcho;              // For midi (and audio). Whether to echo incoming record events to output device.

      EventList* _events;     // tmp Events during midi import
      MPEventList* _mpevents; // tmp Events druring recording
      static bool _isVisible;
      clefTypes clefType;
      void internal_assign(const Track&, int flags);

   public:
      MidiTrack();
      MidiTrack(const MidiTrack&, int flags);
      virtual ~MidiTrack();

      virtual void assign(const Track&, int flags);

      void init();
      virtual AutomationType automationType() const;
      virtual void setAutomationType(AutomationType);

      // play parameter
      int transposition;
      int velocity;
      int delay;
      int len;
      int compression;

      virtual bool setRecordFlag1(bool f) { _recordFlag = f; return true;}
      virtual void setRecordFlag2(bool) {}

      EventList* events() const          { return _events; }
      MPEventList* mpevents() const      { return _mpevents; }

      virtual void read(Xml&);
      virtual void write(int, Xml&) const;

      virtual int height() const;
      
      virtual MidiTrack* newTrack() const { return new MidiTrack(); }
      virtual MidiTrack* clone(int flags) const { return new MidiTrack(*this, flags); }
      virtual Part* newPart(Part*p=0, bool clone=false);

      void setOutChannel(int i)       { _outChannel = i; }
      void setOutPort(int i)          { _outPort = i; }
      // These will transfer controller data to the new selected port and/or channel.
      void setOutChanAndUpdate(int chan);
      void setOutPortAndUpdate(int port);
      // Combines both port and channel operations.
      void setOutPortAndChannelAndUpdate(int port, int chan);
      
      // Backward compatibility: For reading old songs.
      void setInPortAndChannelMask(unsigned int portmask, int chanmask); 
      
      void setRecEcho(bool b)         { _recEcho = b; }
      int outPort() const             { return _outPort;     }
      int outChannel() const          { return _outChannel;  }
      bool recEcho() const            { return _recEcho; }

      virtual bool isMute() const;
      virtual void setSolo(bool val);
      virtual void updateSoloStates(bool noDec);
      virtual void updateInternalSoloStates();
      
      virtual bool canRecord() const  { return true; }
      static void setVisible(bool t) { _isVisible = t; }
      static bool visible() { return _isVisible; }
      int getFirstControllerValue(int ctrl, int def=-1);
      int getControllerChangeAtTick(unsigned tick, int ctrl, int def=-1);
      unsigned getControllerValueLifetime(unsigned tick, int ctrl); // returns the tick where this CC gets overriden by a new one
                                                                    // returns UINT_MAX for "never"

      void setClef(clefTypes i) { clefType = i; }
      clefTypes getClef() { return clefType; }
      };

//---------------------------------------------------------
//   AudioTrack
//    this track can hold audio automation data and can
//    hold tracktypes WAVE, AUDIO_GROUP, AUDIO_OUTPUT,
//    AUDIO_INPUT, AUDIO_SOFTSYNTH, AUDIO_AUX
//---------------------------------------------------------

class AudioTrack : public Track {
      bool _haveData; // Whether we have data from a previous process call during current cycle.
      
      CtrlListList _controller;
      CtrlRecList _recEvents;     // recorded automation events

      bool _prefader;               // prefader metering
      std::vector<double> _auxSend;
      void readAuxSend(Xml& xml);
      
      bool _sendMetronome;
      AutomationType _automationType;
      Pipeline* _efxPipe;
      void internal_assign(const Track&, int flags);

   protected:
      float** outBuffers;
      int _totalOutChannels;
      int _totalInChannels;
      
      unsigned bufferPos;
      virtual bool getData(unsigned, int, unsigned, float**);
      SndFileR _recFile;
      Fifo fifo;                    // fifo -> _recFile
      bool _processed;
      
   public:
      AudioTrack(TrackType t);
      
      AudioTrack(const AudioTrack&, int flags);
      virtual ~AudioTrack();

      virtual void assign(const Track&, int flags);
      
      virtual AudioTrack* clone(int flags) const = 0;
      virtual Part* newPart(Part*p=0, bool clone=false);

      virtual bool setRecordFlag1(bool f);
      virtual void setRecordFlag2(bool f);
      bool prepareRecording();

      bool processed() { return _processed; }

      void addController(CtrlList*);
      void removeController(int id);
      void swapControllerIDX(int idx1, int idx2);

      bool readProperties(Xml&, const QString&);
      void writeProperties(int, Xml&) const;

      void mapRackPluginsToControllers();
      void showPendingPluginNativeGuis();

      SndFileR recFile() const           { return _recFile; }
      void setRecFile(SndFileR sf)       { _recFile = sf;   }

      CtrlListList* controller()         { return &_controller; }

      virtual void setChannels(int n);
      virtual void setTotalOutChannels(int num);
      virtual int totalOutChannels() { return _totalOutChannels; }
      virtual void setTotalInChannels(int num);
      virtual int totalInChannels() { return _totalInChannels; }

      virtual bool isMute() const;
      virtual void setSolo(bool val);
      virtual void updateSoloStates(bool noDec);
      virtual void updateInternalSoloStates();
      
      void putFifo(int channels, unsigned long n, float** bp);

      void record();

      virtual void setMute(bool val);
      virtual void setOff(bool val);

      void setSendMetronome(bool val) { _sendMetronome = val; }
      bool sendMetronome() const { return _sendMetronome; }
      
      double volume() const;
      void setVolume(double val);
      double pan() const;
      void setPan(double val);

      bool prefader() const              { return _prefader; }
      double auxSend(int idx) const;
      void setAuxSend(int idx, double v);
      void addAuxSend(int n);

      void setPrefader(bool val);
      Pipeline* efxPipe()                { return _efxPipe;  }
      void deleteAllEfxGuis();
      void clearEfxList();
      void addPlugin(PluginI* plugin, int idx);

      double pluginCtrlVal(int ctlID) const;
      void setPluginCtrlVal(int param, double val);
      
      void readVolume(Xml& xml);

      virtual void preProcessAlways() { _processed = false; }
      virtual void  addData(unsigned samplePos, int channels, int srcStartChan, int srcChannels, unsigned frames, float** buffer);
      virtual void copyData(unsigned samplePos, int channels, int srcStartChan, int srcChannels, unsigned frames, float** buffer);
      virtual bool hasAuxSend() const { return false; }
      
      // automation
      virtual AutomationType automationType() const    { return _automationType; }
      virtual void setAutomationType(AutomationType t);
      void processAutomationEvents();
      CtrlRecList* recEvents()                         { return &_recEvents; }
      bool addScheduledControlEvent(int track_ctrl_id, float val, unsigned frame); // return true if event cannot be delivered
      void enableController(int track_ctrl_id, bool en); 
      void controllersEnabled(int track_ctrl_id, bool* en1, bool* en2) const;
      void recordAutomation(int n, double v);
      void startAutoRecord(int, double);
      void stopAutoRecord(int, double);
      void setControllerMode(int, CtrlList::Mode m); 
      void clearControllerEvents(int);
      void seekPrevACEvent(int);
      void seekNextACEvent(int);
      void eraseACEvent(int, int);
      void eraseRangeACEvents(int, int, int);
      void addACEvent(int, int, double);
      void changeACEvent(int id, int frame, int newframe, double newval);
      };

//---------------------------------------------------------
//   AudioInput
//---------------------------------------------------------

class AudioInput : public AudioTrack {
      void* jackPorts[MAX_CHANNELS];
      virtual bool getData(unsigned, int, unsigned, float**);
      static bool _isVisible;
      void internal_assign(const Track& t, int flags);
      
   public:
      AudioInput();
      AudioInput(const AudioInput&, int flags);
      virtual ~AudioInput();

      virtual void assign(const Track&, int flags);
      AudioInput* clone(int flags) const { return new AudioInput(*this, flags); }
      virtual AudioInput* newTrack() const { return new AudioInput(); }
      virtual void read(Xml&);
      virtual void write(int, Xml&) const;
      virtual void setName(const QString& s);
      void* jackPort(int channel) { return jackPorts[channel]; }
      void setJackPort(int channel, void*p) { jackPorts[channel] = p; }
      virtual void setChannels(int n);
      virtual bool hasAuxSend() const { return true; }
      static void setVisible(bool t) { _isVisible = t; }
      virtual int height() const;
      static bool visible() { return _isVisible; }
    };

//---------------------------------------------------------
//   AudioOutput
//---------------------------------------------------------

class AudioOutput : public AudioTrack {
      void* jackPorts[MAX_CHANNELS];
      float* buffer[MAX_CHANNELS];
      float* buffer1[MAX_CHANNELS];
      unsigned long _nframes;
      static bool _isVisible;
      float* _monitorBuffer[MAX_CHANNELS];
      void internal_assign(const Track& t, int flags);

   public:
      AudioOutput();
      AudioOutput(const AudioOutput&, int flags);
      virtual ~AudioOutput();

      virtual void assign(const Track&, int flags);
      AudioOutput* clone(int flags) const { return new AudioOutput(*this, flags); }
      virtual AudioOutput* newTrack() const { return new AudioOutput(); }
      virtual void read(Xml&);
      virtual void write(int, Xml&) const;
      virtual void setName(const QString& s);
      void* jackPort(int channel) { return jackPorts[channel]; }
      void setJackPort(int channel, void*p) { jackPorts[channel] = p; }
      virtual void setChannels(int n);
      void processInit(unsigned);
      void process(unsigned pos, unsigned offset, unsigned);
      void processWrite();
      void silence(unsigned);
      virtual bool canRecord() const { return true; }

      float** monitorBuffer() { return _monitorBuffer; }
      static void setVisible(bool t) { _isVisible = t; }
      static bool visible() { return _isVisible; }
      virtual int height() const;
    };

//---------------------------------------------------------
//   AudioGroup
//---------------------------------------------------------

class AudioGroup : public AudioTrack {
      static bool _isVisible;
   public:
      AudioGroup() : AudioTrack(AUDIO_GROUP) {  }
      AudioGroup(const AudioGroup& t, int flags) : AudioTrack(t, flags) { }
      
      AudioGroup* clone(int flags) const { return new AudioGroup(*this, flags); }
      virtual AudioGroup* newTrack() const { return new AudioGroup(); }
      virtual void read(Xml&);
      virtual void write(int, Xml&) const;
      virtual bool hasAuxSend() const { return true; }
      static  void setVisible(bool t) { _isVisible = t; }
      virtual int height() const;
      static bool visible() { return _isVisible; }
    };

//---------------------------------------------------------
//   AudioAux
//---------------------------------------------------------

class AudioAux : public AudioTrack {
      float* buffer[MAX_CHANNELS];
      static bool _isVisible;
   public:
      AudioAux();
      AudioAux(const AudioAux& t, int flags);
      
      AudioAux* clone(int flags) const { return new AudioAux(*this, flags); }
      ~AudioAux();
      virtual AudioAux* newTrack() const { return new AudioAux(); }
      virtual void read(Xml&);
      virtual void write(int, Xml&) const;
      virtual bool getData(unsigned, int, unsigned, float**);
      virtual void setChannels(int n);
      float** sendBuffer() { return buffer; }
      static  void setVisible(bool t) { _isVisible = t; }
      virtual int height() const;
      static bool visible() { return _isVisible; }
    };


//---------------------------------------------------------
//   WaveTrack
//---------------------------------------------------------

class WaveTrack : public AudioTrack {
      Fifo _prefetchFifo;  // prefetch Fifo
      static bool _isVisible;

   public:

      WaveTrack() : AudioTrack(Track::WAVE) {  }
      WaveTrack(const WaveTrack& wt, int flags) : AudioTrack(wt, flags) {}

      virtual WaveTrack* clone(int flags) const    { return new WaveTrack(*this, flags); }
      virtual WaveTrack* newTrack() const { return new WaveTrack(); }
      virtual Part* newPart(Part*p=0, bool clone=false);

      virtual void read(Xml&);
      virtual void write(int, Xml&) const;

      virtual void fetchData(unsigned pos, unsigned frames, float** bp, bool doSeek);
      
      virtual bool getData(unsigned, int ch, unsigned, float** bp);

      void clearPrefetchFifo()      { _prefetchFifo.clear(); }
      Fifo* prefetchFifo()          { return &_prefetchFifo; }
      virtual void setChannels(int n);
      virtual bool hasAuxSend() const { return true; }
      bool canEnableRecord() const;
      virtual bool canRecord() const { return true; }
      static void setVisible(bool t) { _isVisible = t; }
      virtual int height() const;
      static bool visible() { return _isVisible; }
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
      T index(int k) const           { return (*this)[k]; }
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
typedef TrackList::const_iterator ciTrack;

typedef tracklist<MidiTrack*>::iterator iMidiTrack;
typedef tracklist<MidiTrack*>::const_iterator ciMidiTrack;
typedef tracklist<MidiTrack*> MidiTrackList;

typedef tracklist<WaveTrack*>::iterator iWaveTrack;
typedef tracklist<WaveTrack*>::const_iterator ciWaveTrack;
typedef tracklist<WaveTrack*> WaveTrackList;

typedef tracklist<AudioInput*>::iterator iAudioInput;
typedef tracklist<AudioInput*>::const_iterator ciAudioInput;
typedef tracklist<AudioInput*> InputList;

typedef tracklist<AudioOutput*>::iterator iAudioOutput;
typedef tracklist<AudioOutput*>::const_iterator ciAudioOutput;
typedef tracklist<AudioOutput*> OutputList;

typedef tracklist<AudioGroup*>::iterator iAudioGroup;
typedef tracklist<AudioGroup*>::const_iterator ciAudioGroup;
typedef tracklist<AudioGroup*> GroupList;

typedef tracklist<AudioAux*>::iterator iAudioAux;
typedef tracklist<AudioAux*>::const_iterator ciAudioAux;
typedef tracklist<AudioAux*> AuxList;

typedef tracklist<SynthI*>::iterator iSynthI;
typedef tracklist<SynthI*>::const_iterator ciSynthI;
typedef tracklist<SynthI*> SynthIList;

extern void addPortCtrlEvents(MidiTrack* t);
extern void removePortCtrlEvents(MidiTrack* t);

} // namespace MusECore

#endif

