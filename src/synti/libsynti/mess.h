//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: mess.h,v 1.3.2.3 2009/11/19 04:20:33 terminator356 Exp $
//  (C) Copyright 2001-2004 Werner Schweer (ws@seh.de)
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

#ifndef __MESS_H__
#define __MESS_H__

#define MESS_MAJOR_VERSION 1
#define MESS_MINOR_VERSION 1

#include "mpevent.h"

struct MessP;

//---------------------------------------------------------
//   MessConfig
//   Information to be passed to MESS::instantiate().
//   The plugin is free to store these values as it wishes
//    without relying on any particular libraries.
//---------------------------------------------------------

struct MessConfig {
  unsigned int _segmentSize;
  int _sampleRate;
  int _minMeterVal;
  bool _useDenormalBias;
  float _denormalBias;
  bool _leftMouseButtonCanDecrease;
  const char* _configPath;
  const char* _cachePath;
  const char* _globalLibPath;
  const char* _globalSharePath;
  const char* _userPath;
  const char* _projectPath;
  
  MessConfig();
  
  MessConfig(unsigned int segmentSize,
             int sampleRate, 
             int minMeterVal, 
             bool useDenormalBias,
             float denormalBias,
             bool leftMouseButtonCanDecrease,
             const char* configPath,
             const char* cachePath,
             const char* globalLibPath,
             const char* globalSharePath,
             const char* userPath,
             const char* projectPath);
};

//---------------------------------------------------------
//   MidiPatch
//---------------------------------------------------------

#define MP_TYPE_GM 1
#define MP_TYPE_GS 2
#define MP_TYPE_XG 4
#define MP_TYPE_LBANK 8
#define MP_TYPE_HBANK 16

struct MidiPatch {
      signed char typ;                     // 1 - GM  2 - GS  4 - XG
      signed char hbank, lbank, prog;
      const char* name;
      };

//---------------------------------------------------------
//  Mess
//    MusE experimental software synth
//    Instance virtual interface class
//   NOTICE: If implementing sysex support, be sure to make a unique ID and use   
//    it to filter out unrecognized sysexes. Headers should be constructed as:
//      MUSE_SYNTH_SYSEX_MFG_ID        The MusE SoftSynth Manufacturer ID byte (0x7C) found in midi.h 
//      0xNN                           The synth's unique ID byte
//---------------------------------------------------------

class Mess {
      MessP* d;

      int _sampleRate;
      int _channels;                // 1 - mono,  2 - stereo

   public:
      Mess(int channels);
      virtual ~Mess();

      // This is only a kludge required to support old songs' midistates. Do not use in any new synth.
      virtual int oldMidiStateHeader(const unsigned char** /*data*/) const;
      
      int channels() const;
      int sampleRate() const;
      void setSampleRate(int r);

      virtual void processMessages();
      virtual void process(unsigned pos, float** data, int numPorts, int offset, int len) = 0;

      // the synti has to (re-)implement processEvent() or provide
      // some of the next three functions:

      virtual bool processEvent(const MusECore::MidiPlayEvent&);
      virtual bool setController(int, int, int);
      virtual bool playNote(int, int, int);
      virtual bool sysex(int, const unsigned char*);

      virtual void getInitData(int* n, const unsigned char**) /*const*/; // No const: Synths may need to allocate member pointers.
      virtual int getControllerInfo(int, const char**, int*, int*, int*, int*) const;
      virtual const char* getPatchName(int, int, bool) const;
      virtual const MidiPatch* getPatchInfo(int, const MidiPatch*) const;
      // Returns true if a note name list is found for the given patch.
      // If true, name either contains the note name, or is NULL if no note name was found.
      virtual bool getNoteSampleName(bool /*drum*/, int /*channel*/, 
                                     int /*patch*/, int /*note*/, 
                                     const char** /*name*/) const;

      // synthesizer -> host communication
      void sendEvent(MusECore::MidiPlayEvent);  // called from synti
      MusECore::MidiPlayEvent receiveEvent();   // called from host
      int eventsPending() const;

      // GUI interface routines
      virtual bool hasNativeGui() const;
      virtual bool nativeGuiVisible() const;
      virtual void showNativeGui(bool);
      virtual void getNativeGeometry(int* x, int* y, int* w, int* h) const;
      virtual void setNativeGeometry(int, int, int, int);
      virtual void guiHeartBeat();
      virtual void setNativeGuiWindowTitle(const char*) const;
      };

//---------------------------------------------------------
//   MESS
//    Class descriptor
//---------------------------------------------------------

struct MESS {
      const char* name;
      const char* description;
      const char* version;
      int majorMessVersion, minorMessVersion;
      Mess* (*instantiate)(unsigned long long parentWinId, const char* name, const MessConfig* config);
      };

extern "C" {
      const MESS* mess_descriptor();
      }

typedef const MESS* (*MESS_Descriptor_Function)();

#endif

