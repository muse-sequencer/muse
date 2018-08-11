//
// C++ Interface: simplesynth
//
// Description:
//
//
// Author: Mathias Lundgren <lunar_shuttle@users.sf.net>, (C) 2004
//  Contributer: (C) Copyright 2011 Tim E. Real (terminator356 at users.sourceforge.net)
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
//
//
#ifndef SIMPLESYNTH_H
#define SIMPLESYNTH_H

#include <sndfile.h>
#include "libsynti/mess.h"
#include "common.h"
#include "common_defs.h"
#include "muse/mpevent.h"   
#include "simpledrumsgui.h"
#include "libsimpleplugin/simpler_plugin.h"

#define SS_NO_SAMPLE       0
#define SS_NO_PLUGIN       0

#define SS_PROCESS_BUFFER_SIZE SS_segmentSize
#define SS_SENDFX_BUFFER_SIZE  SS_PROCESS_BUFFER_SIZE

enum SS_ChannelState
{
   SS_CHANNEL_INACTIVE=0,
   SS_SAMPLE_PLAYING
};

enum SS_State
{
   SS_INITIALIZING=0,
   SS_LOADING_SAMPLE,
   SS_CLEARING_SAMPLE,
   SS_RUNNING
};

enum SS_SendFXState
{
   SS_SENDFX_OFF=0,
   SS_SENDFX_ON
};

struct SS_SendFx
{
   SS_SendFXState state;
   MusESimplePlugin::PluginI*       plugin;
   int            inputs;
   int            outputs;
   int            retgain_ctrlval;
   double         retgain;
   int            nrofparameters;
};

struct SS_Sample
{
   SS_Sample() { data = 0; }
   float*      data;
   int         samplerate;
   //int         bits;
   std::string filename;
   long        samples;
   long        frames;
   int         channels;
   //SF_INFO     sfinfo;
};

enum SS_ChannelRoute
{
   SS_CHN_ROUTE_MIX = 0,
   SS_CHN_ROUTE_CHN,
};

struct SS_Channel
{
   SS_ChannelState state;
   const char*     name;
   SS_Sample*      sample;
   SS_Sample*      originalSample;
   int             playoffset;
   bool            noteoff_ignore;

   double          volume;
   int             volume_ctrlval;

   double          cur_velo;
   double          gain_factor;

   int             pan;
   double          balanceFactorL;
   double          balanceFactorR;
   int             pitchInt;

   bool            channel_on;

   SS_ChannelRoute route;

   //Send fx:
   double          sendfxlevel[SS_NR_OF_SENDEFFECTS];
};

struct SS_Controller
{
   std::string name;
   int num;
   int min, max;
};

double rangeToPitch(int value);
//int pitchToRange(double pitch);

class SimpleSynth : public Mess
{
public:
   SimpleSynth(int);

   virtual ~SimpleSynth();

   virtual bool nativeGuiVisible() const;
   virtual bool hasNativeGui() const;
   virtual bool playNote(int arg1, int arg2, int arg3);
   virtual bool processEvent(const MusECore::MidiPlayEvent& arg1);
   virtual bool setController(int arg1, int arg2, int arg3);
   virtual bool sysex(int arg1, const unsigned char* arg2);
   virtual const char* getPatchName(int arg1, int arg2, bool arg3) const;
   virtual const MidiPatch* getPatchInfo(int arg1, const MidiPatch* arg2) const;
   virtual int getControllerInfo(int arg1, const char** arg2, int* arg3, int* arg4, int* arg5, int* arg6) const;
   virtual void processMessages();
   virtual void process(unsigned pos, float** data, int offset, int len);
   virtual void showNativeGui(bool arg1);
   virtual void guiHeartBeat();
   virtual void getInitData(int*, const unsigned char**);
   // This is only a kludge required to support old songs' midistates. Do not use in any new synth.
   virtual int oldMidiStateHeader(const unsigned char** data) const;
   bool init(const char* name);
   void guiSendSampleLoaded(bool success, int ch, const char* filename);
   void guiSendError(const char* errorstring);
   
   SS_State synth_state;

   static const char* synth_state_descr[];
   static const char* channel_state_descr[];

private:
   SimpleSynthGui* gui;

   byte* initBuffer;
   int initLen;
   void setupInitBuffer(int len);

   SS_Channel channels[SS_NR_OF_CHANNELS];
   SS_Controller controllers[SS_NR_OF_CONTROLLERS];
   bool setController(int channel, int id, int val, bool fromGui);
   bool loadSample(int ch_no, const char* filename);
   void parseInitData(const unsigned char* data);
   void updateVolume(int ch, int in_volume_ctrlval);
   void updatePitch(int ch, int inpitch_ctrlval);
   void updateBalance(int ch, int pan);
   void guiNotifySampleCleared(int ch);
   void guiUpdateBalance(int ch, int bal);
   void guiUpdatePitch(int ch, int bal);
   void guiUpdateRoute(int ch, int val);
   void guiUpdateVolume(int ch, int val);
   void guiUpdateNoff(int ch, bool b);
   void guiUpdateChoff(int ch, bool b);
   void guiUpdateMasterVol(int val);
   void guiUpdateFxParameter(int fxid, int param, float val);
   void guiUpdateSendFxLevel(int channel, int fxid, int level);
   // Returns true on success.
   bool initSendEffect(int sendeffectid, QString lib, QString name);
   void setSendFxLevel(int channel, int effectid, double val);
   void cleanupPlugin(int id);
   void setFxParameter(int fxid, int param, float val);
   void clearSample(int ch);

   double master_vol;
   int master_vol_ctrlval;

   //Send effects:
   SS_SendFx sendEffects[SS_NR_OF_SENDEFFECTS];
   float* sendFxLineOut[SS_NR_OF_SENDEFFECTS][2]; //stereo output (fed into LADSPA inputs),sent from the individual channels -> LADSPA fx
   float* sendFxReturn[SS_NR_OF_SENDEFFECTS][2];  //stereo inputs, from LADSPA plugins, sent from LADSPA -> SS and added to the mix
   double* processBuffer[2];
};

struct SS_SampleLoader
{
   SS_Channel*  channel;
   std::string  filename;
   int          ch_no;
   SimpleSynth* synth;
   int sampleRate;
};

void resample(SS_Sample *origSmp, SS_Sample* newSample, double pitch, int sample_rate);
static void* loadSampleThread(void*);
static pthread_mutex_t SS_LoaderMutex;

#endif
