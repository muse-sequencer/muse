//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: ./synti/fluidsynth/fluidsynti.h $
//
//  Copyright (C) 1999-2011 by Werner Schweer and others
//  (C) Copyright 2016 Tim E. Real (terminator356 on users dot sourceforge dot net)
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

#include <QThread>
#include <QMutex>
#include "fluidsynthgui.h"
#include "libsynti/mess.h"
#include "muse/debug.h"
#include "mpevent.h"   
#include "midictrl_consts.h"
#include "common_defs.h"

// TODO: Try to not include this. Standalone build of plugin?
#include "config.h"
#ifdef HAVE_INSTPATCH
#include <map>
#endif

#define FS_DEBUG_DATA 0 //Turn on/off debug print of midi data sent to fluidsynth

typedef unsigned char byte;

struct FluidSoundFont
      {
      QString file_name;
      QString name;
      byte extid, intid;
      #ifdef HAVE_INSTPATCH
      std::map < int /*patch*/, std::multimap < int /* note */, std::string > > _noteSampleNameList;
      #endif
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
};

class LoadFontWorker : public QObject
{
      Q_OBJECT
  public:
      LoadFontWorker() {}
      void loadFont(void*);
  signals:
      void loadFontSignal(void*);

  private slots:
      void execLoadFont(void*);
};

class FluidSynth : public Mess {

   private:
      bool pushSoundfont (const char*, int);
      void sendSysex(int l, const unsigned char* d);
      void sendLastdir(const char*);
      void sfChannelChange(unsigned char font_id, unsigned char channel);
      void parseInitData(int n, const byte* d);

      fluid_settings_t* _settings;
      byte* initBuffer;
      int initLen;

      double chorus_speed_range_lower;

      byte getFontInternalIdByExtId (byte channel);

      void debug(const char* msg) { if (FS_DEBUG) printf("Debug: %s\n",msg); }
      void dumpInfo(); //Prints out debug info

      FluidChannel channels[FS_MAX_NR_OF_CHANNELS];
      std::string lastdir;
      QThread fontLoadThread;
      LoadFontWorker fontWorker;
      const MidiPatch * getFirstPatch (int channel) const;
      const MidiPatch* getNextPatch (int, const MidiPatch *) const;

      //For reverb and chorus:
      double rev_size, rev_damping, rev_width, rev_level, cho_level, cho_speed, cho_depth;
      bool rev_on, cho_on;
      int cho_num, cho_type;

public:
      FluidSynth(int sr, QMutex &_GlobalSfLoaderMutex);
      virtual ~FluidSynth();
      bool init(const char*);
      // This is only a kludge required to support old songs' midistates. Do not use in any new synth.
      virtual int oldMidiStateHeader(const unsigned char** data) const;
      virtual void processMessages();
      virtual void process(unsigned pos, float** ports, int numPorts, int offset, int len);
      virtual bool playNote(int channel, int pitch, int velo);
      virtual bool sysex(int, const unsigned char*);
      virtual bool setController(int, int, int);
      void setController(int, int , int, bool);
      virtual void getInitData(int*, const unsigned char**);
      virtual const char* getPatchName(int, int, bool) const;
      virtual const MidiPatch* getPatchInfo(int i, const MidiPatch* patch) const;
      virtual int getControllerInfo(int, const char**, int*, int*, int*, int*) const;
      virtual bool processEvent(const MusECore::MidiPlayEvent&);
      #ifdef HAVE_INSTPATCH
      // Returns true if a note name list is found for the given patch.
      // If true, name either contains the note name, or is NULL if no note name was found.
      virtual bool getNoteSampleName(bool drum, int channel, int patch, int note, const char** name) const;
      #endif

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
      QMutex& _sfLoaderMutex;
      int currentlyLoadedFonts; //To know whether or not to run the init-parameters
      std::list<FluidSoundFont> stack;
      int nrOfSoundfonts;

      void initInternal();

      static FluidCtrl fluidCtrl[];

      };

struct FS_Helper //Only used to pass parameters when calling the loading thread
      {
      FluidSynth* fptr;
      QString file_name;
      int id;
      };

// static void* fontLoadThread(void* t); // moved to the implementation file -Orcan
#endif /* __MUSE_FLUIDSYNTI_H__ */
