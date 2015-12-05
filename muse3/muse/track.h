//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: track.h,v 1.39.2.17 2009/12/20 05:00:35 terminator356 Exp $
//
//  (C) Copyright 1999-2004 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011-2013 Tim E. Real (terminator356 on sourceforge)
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
#include "mpevent.h"
#include "key.h"
#include "node.h"
#include "route.h"
#include "ctrl.h"
#include "globaldefs.h"
#include "cleftypes.h"
#include "controlfifo.h"

class QPixmap;

namespace MusECore {
class Pipeline;
class PluginI;
class SynthI;
class Xml;
struct DrumMap;
struct ControlEvent;
struct Port;
class PendingOperationList;


typedef std::vector<double> AuxSendValueList;
typedef std::vector<double>::iterator iAuxSendValue;

//---------------------------------------------------------
//   Track
//---------------------------------------------------------

class Track {
   public:
      enum TrackType {
         MIDI=0, DRUM, NEW_DRUM, WAVE, AUDIO_OUTPUT, AUDIO_INPUT, AUDIO_GROUP,
         AUDIO_AUX, AUDIO_SOFTSYNTH
         };
      enum AssignFlags {
         ASSIGN_PROPERTIES=1, ASSIGN_PARTS=2, ASSIGN_PLUGINS=4, ASSIGN_STD_CTRLS=8, ASSIGN_PLUGIN_CTRLS=16, ASSIGN_ROUTES=32, ASSIGN_DEFAULT_ROUTES=64, ASSIGN_DRUMLIST=128 };
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
      static QPixmap* trackTypeIcon(TrackType);
      QPixmap* icon() const { return trackTypeIcon(type()); }
      
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

      void clearRecAutomation(bool clearList);
      
      const QString& name() const     { return _name; }
      // setName can be overloaded to do other things like setting port names, while setNameText just sets the text.
      virtual void setName(const QString& s)  { _name = s; }
      // setNameText just sets the text, while setName can be overloaded to do other things like setting port names.
      void setNameText(const QString& s)  { _name = s; }

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
      // Number of routable inputs/outputs for each Route::RouteType.
      virtual RouteCapabilitiesStruct routeCapabilities() const;
      
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
      bool isMidiTrack() const       { return type() == MIDI || type() == DRUM || type() == NEW_DRUM; }
      bool isDrumTrack() const       { return type() == DRUM || type() == NEW_DRUM; }
      virtual bool canRecord() const { return false; }
      virtual AutomationType automationType() const    = 0;
      virtual void setAutomationType(AutomationType t) = 0;
      static void setVisible(bool) { }
      bool isVisible();

      };

//---------------------------------------------------------
//   MidiTrack
//---------------------------------------------------------

class MidiTrack : public Track {
      int _outPort;
      int _outChannel;
      bool _recEcho;              // For midi (and audio). Whether to echo incoming record events to output device.

   public:
      EventList events;     // tmp Events during midi import
      MPEventList mpevents; // tmp Events druring recording

   private:
      static bool _isVisible;
      clefTypes clefType;

      
      DrumMap* _drummap; // _drummap[foo].anote is always equal to foo
      bool* _drummap_hidden; // _drummap und _drummap_hidden will be an array[128]
      bool _drummap_tied_to_patch; //if true, changing patch also changes drummap
      bool _drummap_ordering_tied_to_patch; //if true, changing patch also changes drummap-ordering
      int drum_in_map[128];
      
      void init();
      void internal_assign(const Track&, int flags);
      void init_drummap(bool write_ordering); // function without argument in public
      void remove_ourselves_from_drum_ordering();
      void init_drum_ordering();
      
      void writeOurDrumSettings(int level, Xml& xml) const;
      void readOurDrumSettings(Xml& xml);
      //void writeOurDrumMap(int level, Xml& xml, bool full) const; //below in public:
      //void readOurDrumMap(Xml& xml, bool dont_init=false); //below in public:

   public:
      MidiTrack();
      MidiTrack(const MidiTrack&, int flags);
      virtual ~MidiTrack();

      virtual void assign(const Track&, int flags);

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

      virtual void read(Xml&);
      virtual void write(int, Xml&) const;

      virtual int height() const;
      
      virtual MidiTrack* newTrack() const { return new MidiTrack(); }
      virtual MidiTrack* clone(int flags) const { return new MidiTrack(*this, flags); }
      virtual Part* newPart(Part*p=0, bool clone=false);

      // Number of routable inputs/outputs for each Route::RouteType.
      virtual RouteCapabilitiesStruct routeCapabilities() const;
      
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
      
      DrumMap* drummap() { return _drummap; }
      bool* drummap_hidden() { return _drummap_hidden; }
      int map_drum_in(int enote) { return drum_in_map[enote]; }
      void update_drum_in_map();

      void init_drummap() { init_drummap(false); } // function with argument in private
      
      bool auto_update_drummap();
      void set_drummap_tied_to_patch(bool);
      bool drummap_tied_to_patch() { return _drummap_tied_to_patch; }
      void set_drummap_ordering_tied_to_patch(bool);
      bool drummap_ordering_tied_to_patch() { return _drummap_ordering_tied_to_patch; }

      //void writeOurDrumSettings(int level, Xml& xml) const; // above in private:
      //void readOurDrumSettings(Xml& xml); // above in private:
      void writeOurDrumMap(int level, Xml& xml, bool full) const;
      void readOurDrumMap(Xml& xml, QString tag, bool dont_init=false, bool compatibility=false);
      };

//---------------------------------------------------------
//   AudioTrack
//    this track can hold audio automation data and can
//    hold tracktypes WAVE, AUDIO_GROUP, AUDIO_OUTPUT,
//    AUDIO_INPUT, AUDIO_SOFTSYNTH, AUDIO_AUX
//---------------------------------------------------------

class AudioTrack : public Track {
      bool _haveData; // Whether we have data from a previous process call during current cycle.
      
      CtrlListList _controller;   // Holds all controllers including internal, plugin and synth.
      ControlFifo _controlFifo;   // For internal controllers like volume and pan. Plugins/synths have their own.
      CtrlRecList _recEvents;     // recorded automation events

      unsigned long _controlPorts;
      Port* _controls;             // For internal controllers like volume and pan. Plugins/synths have their own.

      float _curVolume;
      float _curVol1;
      float _curVol2;
      
      bool _prefader;               // prefader metering
      AuxSendValueList _auxSend;
      void readAuxSend(Xml& xml);
      int recFileNumber;
      
      bool _sendMetronome;
      AutomationType _automationType;
      Pipeline* _efxPipe;
      double _gain;

      void initBuffers();
      void internal_assign(const Track&, int flags);
      void processTrackCtrls(unsigned pos, int trackChans, unsigned nframes, float** buffer);

   protected:
      // Cached audio data for all channels. If prefader is not on, the first two channels
      //  have volume and pan applied if track is stereo, or the first channel has just
      //  volume applied if track is mono.
      float** outBuffers;
      // Extra cached audio data.
      float** outBuffersExtraMix;
      // Just all zeros all the time, so we don't have to clear for silence.
      float*  audioInSilenceBuf;
      // Just a place to connect all unused audio outputs.
      float*  audioOutDummyBuf;

      // These two are not the same as the number of track channels which is always either 1 (mono) or 2 (stereo):
      // Total number of output channels.
      int _totalOutChannels;
      // Total number of input channels.
      int _totalInChannels;
      
      virtual bool getData(unsigned, int, unsigned, float**);
      SndFileR _recFile;
      Fifo fifo;                    // fifo -> _recFile
      SndFileR _recFilePreview;  //for live wave update while recording
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
      SndFileR recFilePreview() const           { return _recFilePreview; }
      void setRecFile(SndFileR sf)       { _recFile = sf; if(!sf){_recFilePreview = 0;}else{_recFilePreview = new MusECore::SndFile(sf.path());}  }

      CtrlListList* controller()         { return &_controller; }
      // For setting/getting the _controls 'port' values.
      unsigned long parameters() const { return _controlPorts; }
      void setParam(unsigned long i, float val); 
      float param(unsigned long i) const;

      virtual void setChannels(int n);
      virtual void setTotalOutChannels(int num);
      virtual int totalOutChannels() const { return _totalOutChannels; }
      virtual void setTotalInChannels(int num);
      virtual int totalInChannels() const { return _totalInChannels; }
      // Number of routable inputs/outputs for each Route::RouteType.
      virtual RouteCapabilitiesStruct routeCapabilities() const;
      // Number of required processing buffers.
      virtual int totalProcessBuffers() const { return (channels() == 1) ? 1 : totalOutChannels(); }

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
      double gain() const;
      void setGain(double val);

      bool prefader() const              { return _prefader; }
      double auxSend(int idx) const;
      void setAuxSend(int idx, double v);
      void addAuxSend(int n);
      void addAuxSendOperation(int n, PendingOperationList& ops);

      void setPrefader(bool val);
      Pipeline* efxPipe()                { return _efxPipe;  }
      void deleteAllEfxGuis();
      void clearEfxList();
      // Removes any existing plugin and inserts plugin into effects rack, and calls setupPlugin.
      void addPlugin(PluginI* plugin, int idx);
      // Assigns valid ID and track to plugin, and creates controllers for plugin.
      void setupPlugin(PluginI* plugin, int idx);

      double pluginCtrlVal(int ctlID) const;
      void setPluginCtrlVal(int param, double val);
      
      void readVolume(Xml& xml);

      virtual void preProcessAlways() { _processed = false; }
      virtual void  addData(unsigned samplePos, int dstStartChan, int dstChannels, int srcStartChan, int srcChannels, unsigned frames, float** buffer);
      virtual void copyData(unsigned samplePos, int dstStartChan, int dstChannels, int srcStartChan, int srcChannels, unsigned frames, float** buffer, bool add=false);
      virtual bool hasAuxSend() const { return false; }
      
      // automation
      virtual AutomationType automationType() const    { return _automationType; }
      virtual void setAutomationType(AutomationType t);
      void processAutomationEvents();
      CtrlRecList* recEvents()                         { return &_recEvents; }
      bool addScheduledControlEvent(int track_ctrl_id, float val, unsigned frame); // return true if event cannot be delivered
      void enableController(int track_ctrl_id, bool en);
      bool controllerEnabled(int track_ctrl_id) const;
      // Enable all track and plugin controllers, and synth controllers if applicable.
      void enableAllControllers();
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
      const AuxSendValueList &getAuxSendValueList() { return _auxSend; }
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
      // Number of routable inputs/outputs for each Route::RouteType.
      virtual RouteCapabilitiesStruct routeCapabilities() const;
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
      // Number of routable inputs/outputs for each Route::RouteType.
      virtual RouteCapabilitiesStruct routeCapabilities() const;
      void processInit(unsigned);
      void process(unsigned pos, unsigned offset, unsigned);
      void processWrite();
      void silence(unsigned);
      virtual bool canRecord() const { return true; }

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
      int _index;
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
      // Number of routable inputs/outputs for each Route::RouteType.
      virtual RouteCapabilitiesStruct routeCapabilities() const {
                RouteCapabilitiesStruct s = AudioTrack::routeCapabilities();
                s._trackChannels._inRoutable = false;
                s._trackChannels._inChannels = 0;
                return s; }
      float** sendBuffer() { return buffer; }
      static  void setVisible(bool t) { _isVisible = t; }
      virtual int height() const;
      static bool visible() { return _isVisible; }
      virtual QString auxName();
      virtual int index() { return _index; }
    };


//---------------------------------------------------------
//   WaveTrack
//---------------------------------------------------------

class WaveTrack : public AudioTrack {
      Fifo _prefetchFifo;  // prefetch Fifo
      static bool _isVisible;

      void internal_assign(const Track&, int flags);
      
   public:

      WaveTrack();
      WaveTrack(const WaveTrack& wt, int flags);

      virtual void assign(const Track&, int flags);
      
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
      bool contains(const Track* t) const {
            return std::find(begin(), end(), t) != end();
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
extern void addPortCtrlEvents(Track* track, PendingOperationList& ops);
extern void removePortCtrlEvents(Track* track, PendingOperationList& ops);
} // namespace MusECore

#endif

