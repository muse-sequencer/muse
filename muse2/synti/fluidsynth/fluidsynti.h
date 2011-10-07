//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: ./synti/fluidsynth/fluidsynti.h $
//
//  Copyright (C) 1999-2011 by Werner Schweer and others
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
/*
 * MusE FLUID Synth softsynth plugin
 *
 * Copyright (C) 2004 Mathias Lundgren (lunar_shuttle@users.sourcforge.net)
 *
 * $Id: fluidsynti.h,v 1.15.2.5 2009/11/19 04:20:33 terminator356 Exp $
 *
 */

#ifndef __MUSE_FLUIDSYNTI_H__
#define __MUSE_FLUIDSYNTI_H__

#include <fluidsynth.h>
#include <pthread.h>
#include <string>

#include "fluidsynthgui.h"
#include "libsynti/mess.h"
#include "muse/debug.h"
//#include "libsynti/mpevent.h"
#include "muse/mpevent.h"   
#include "muse/midictrl.h"
#include "common_defs.h"

#define FS_DEBUG_DATA 0 //Turn on/off debug print of midi data sent to fluidsynth

typedef unsigned char byte;

struct FluidSoundFont
      {
      std::string filename;
      std::string name;
      byte extid, intid;
      };

struct FluidCtrl {
      const char* name;
      int num;
      int min, max;
      //int val;
      int initval;
      };

// NRPN-controllers:
static const int FS_GAIN            = 0 + MusECore::CTRL_NRPN14_OFFSET;
static const int FS_REVERB_ON       = 1 + MusECore::CTRL_NRPN14_OFFSET;
static const int FS_REVERB_LEVEL    = 2 + MusECore::CTRL_NRPN14_OFFSET;
static const int FS_REVERB_ROOMSIZE = 3 + MusECore::CTRL_NRPN14_OFFSET;
static const int FS_REVERB_DAMPING  = 4 + MusECore::CTRL_NRPN14_OFFSET;
static const int FS_REVERB_WIDTH    = 5 + MusECore::CTRL_NRPN14_OFFSET;
static const int FS_CHORUS_ON       = 6 + MusECore::CTRL_NRPN14_OFFSET;
static const int FS_CHORUS_NUM      = 7 + MusECore::CTRL_NRPN14_OFFSET;
static const int FS_CHORUS_TYPE     = 8 + MusECore::CTRL_NRPN14_OFFSET;
static const int FS_CHORUS_SPEED    = 9 + MusECore::CTRL_NRPN14_OFFSET;
static const int FS_CHORUS_DEPTH   = 10 + MusECore::CTRL_NRPN14_OFFSET;
static const int FS_CHORUS_LEVEL   = 11 + MusECore::CTRL_NRPN14_OFFSET;
// Added by T356
static const int FS_PITCHWHEELSENS  = 0 + MusECore::CTRL_RPN_OFFSET;

// FluidChannel is used to map different soundfonts to different fluid-channels
// This is to be able to select different presets from specific soundfonts, since
// Fluidsynth has a quite strange way of dealing with fontloading and channels
// We also need this since getFirstPatch and getNextPatch only tells us which channel is
// used, so this works as a connection between soundfonts and fluid-channels (one channel
// can only have one soundfont, but one soundfont can have many channels)

struct FluidChannel
      {
      byte font_extid, font_intid, preset, drumchannel;
      byte banknum; // hbank
      //FluidSoundFont* font;
      };

/*#include <string>
#include <list>
#include <map>
*/

class FluidSynth : public Mess {
   private:
      bool pushSoundfont (const char*, int);
      void sendSysex(int l, const unsigned char* d);
      void sendLastdir(const char*);
      void sfChannelChange(unsigned char font_id, unsigned char channel);
      void parseInitData(int n, const byte* d);

      byte* initBuffer;
      int initLen;

      byte getFontInternalIdByExtId (byte channel);

      void debug(const char* msg) { if (FS_DEBUG) printf("Debug: %s\n",msg); }
      void dumpInfo(); //Prints out debug info

      FluidChannel channels[FS_MAX_NR_OF_CHANNELS];
      std::string lastdir;
      pthread_t fontThread;
      const MidiPatch * getFirstPatch (int channel) const;
      const MidiPatch* getNextPatch (int, const MidiPatch *) const;

      //For reverb and chorus:
      double rev_size, rev_damping, rev_width, rev_level, cho_level, cho_speed, cho_depth;
      bool rev_on, cho_on;
      int cho_num, cho_type;

public:
      FluidSynth(int sr, pthread_mutex_t *_Globalsfloader_mutex);
      virtual ~FluidSynth();
      bool init(const char*);
      // This is only a kludge required to support old songs' midistates. Do not use in any new synth.
      virtual int oldMidiStateHeader(const unsigned char** data) const;
      virtual void processMessages();
      virtual void process(float**, int, int);
      virtual bool playNote(int channel, int pitch, int velo);
      virtual bool sysex(int, const unsigned char*);
      virtual bool setController(int, int, int);
      void setController(int, int , int, bool);
      virtual void getInitData(int*, const unsigned char**);
      virtual const char* getPatchName(int, int, int, bool) const;
      virtual const MidiPatch* getPatchInfo(int i, const MidiPatch* patch) const;
      virtual int getControllerInfo(int, const char**, int*, int*, int*, int*) const;
      virtual bool processEvent(const MusECore::MidiPlayEvent&);

      //virtual bool hasGui() const { return true; }
      //virtual bool guiVisible() const;
      //virtual void showGui(bool val);
      virtual bool hasNativeGui() const { return true; }
      virtual bool nativeGuiVisible() const;
      virtual void showNativeGui(bool val);

      void sendError(const char*);
      void sendSoundFontData();
      void sendChannelData();
      void rewriteChannelSettings(); //used because fluidsynth does some very nasty things when loading a font!
      bool popSoundfont (int ext_id);

      int getNextAvailableExternalId();

      fluid_synth_t* fluidsynth;
      FluidSynthGui* gui;
      pthread_mutex_t *_sfloader_mutex;
      int currentlyLoadedFonts; //To know whether or not to run the init-parameters
      std::list<FluidSoundFont> stack;
      int nrOfSoundfonts;

      void initInternal();

      static FluidCtrl fluidCtrl[];

      };

struct FS_Helper //Only used to pass parameters when calling the loading thread
      {
      FluidSynth* fptr;
      std::string filename;
      int id;
      };

// static void* fontLoadThread(void* t); // moved to the implementation file -Orcan
#endif /* __MUSE_FLUIDSYNTI_H__ */
