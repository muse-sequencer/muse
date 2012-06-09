//
// C++ Implementation: simplesynth
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

#include "muse/midictrl.h"
#include "muse/midi.h"
//#include "libsynti/mpevent.h"
#include "muse/mpevent.h"   
//#include "common_defs.h"
#include "simpledrums.h"

#include <samplerate.h>

const char* SimpleSynth::synth_state_descr[] =
      {
      "SS_INITIALIZING",
      "SS_LOADING_SAMPLE",
      "SS_CLEARING_SAMPLE",
      "SS_RUNNING"
      };

const char* SimpleSynth::channel_state_descr[] =
      {
      "SS_CHANNEL_INACTIVE",
      "SS_SAMPLE_PLAYING"
      };

#define SWITCH_SYNTH_STATE(state)\
synth_state = state; \
if (SS_DEBUG_STATE) \
      fprintf (stderr, "SS STATE: %s\n", SimpleSynth::synth_state_descr[state]);

#define SWITCH_CHAN_STATE(ch, s)\
channels[ch].state = s; \
if (SS_DEBUG_STATE) \
      fprintf (stderr, "SS CHAN %d STATE: %s\n", ch, SimpleSynth::channel_state_descr[s]);

#define SS_CHANNEL_VOLUME_QUOT 100.0
#define SS_MASTER_VOLUME_QUOT  100.0
int SS_samplerate;

#define SS_LOG_MAX   0
#define SS_LOG_MIN -10
#define SS_LOG_OFFSET SS_LOG_MIN


//
// Map plugin parameter on domain [SS_PLUGIN_PARAM_MIN, SS_PLUGIN_PARAM_MAX] to domain [SS_LOG_MIN, SS_LOG_MAX] (log domain)
//
float SS_map_pluginparam2logdomain(int pluginparam_val)
      {
      float scale = (float) (SS_LOG_MAX - SS_LOG_MIN)/ (float) SS_PLUGIN_PARAM_MAX;
      float scaled = (float) pluginparam_val * scale;
      float mapped = scaled + SS_LOG_OFFSET;
      return mapped;
      }
//
// Map plugin parameter on domain to domain [SS_LOG_MIN, SS_LOG_MAX] to [SS_PLUGIN_PARAM_MIN, SS_PLUGIN_PARAM_MAX]  (from log-> [0,127])
// (inverse func to the above)
int SS_map_logdomain2pluginparam(float pluginparam_log)
      {
      float mapped = pluginparam_log - SS_LOG_OFFSET;
      float scale = (float) SS_PLUGIN_PARAM_MAX / (float) (SS_LOG_MAX - SS_LOG_MIN);
      int scaled  = (int) round(mapped * scale);
      return scaled;
      }

//---------------------------------------------------------
//   SimpleSynth
//---------------------------------------------------------
SimpleSynth::SimpleSynth(int sr)
      : Mess(SS_AUDIO_CHANNELS)
      {
      SS_TRACE_IN
      SS_samplerate = sr;
      SS_initPlugins();

      initBuffer  = 0;
      initLen     = 0;
      
      simplesynth_ptr = this;
      master_vol = 100.0 / SS_MASTER_VOLUME_QUOT;
      master_vol_ctrlval = 100;

      //initialize
      for (int i=0; i<SS_NR_OF_CHANNELS; i++) {
            channels[i].sample = 0;
            channels[i].playoffset = 0;
            channels[i].noteoff_ignore = false;
            channels[i].volume = (double) (100.0/SS_CHANNEL_VOLUME_QUOT );
            channels[i].volume_ctrlval = 100;
            channels[i].pan = 64;
            channels[i].balanceFactorL = 1.0;
            channels[i].balanceFactorR = 1.0;
            SWITCH_CHAN_STATE(i, SS_CHANNEL_INACTIVE);
            channels[i].channel_on = false;
            for (int j=0; j<SS_NR_OF_SENDEFFECTS; j++) {
                  channels[i].sendfxlevel[j] = 0.0;
                  }
            }

      //Process buffer:
      processBuffer[0] = new double[SS_PROCESS_BUFFER_SIZE]; //left
      processBuffer[1] = new double[SS_PROCESS_BUFFER_SIZE]; //right

      //Send effects
      for (int i=0; i<SS_NR_OF_SENDEFFECTS; i++) {
            sendFxLineOut[i][0] = new float[SS_SENDFX_BUFFER_SIZE]; //left out
            sendFxLineOut[i][1] = new float[SS_SENDFX_BUFFER_SIZE]; //right out
            sendFxReturn[i][0]  = new float[SS_SENDFX_BUFFER_SIZE]; //left in
            sendFxReturn[i][1]  = new float[SS_SENDFX_BUFFER_SIZE]; //right in
            }

      for (int i=0; i<SS_NR_OF_SENDEFFECTS; i++) {
            sendEffects[i].state       = SS_SENDFX_OFF;
            sendEffects[i].plugin      = 0;
            sendEffects[i].retgain     = 1.0;
            sendEffects[i].retgain_ctrlval = 100;
            sendEffects[i].nrofparameters = 0;
            }

      //Build controller list:
      controllers[0].name = "Master volume";
      controllers[0].num  = MusECore::CTRL_NRPN14_OFFSET;
      controllers[0].min  = 0;
      controllers[0].max  = 127;

      int i=1;
      for (int ch=0; ch<SS_NR_OF_CHANNELS; ch++) {
            QString c1 = "Channel " + QString::number(ch + 1) + " volume";
            QString c2 = "Channel " + QString::number(ch + 1) + " pan";
            QString c3 = "Channel " + QString::number(ch + 1) + " noteoff ignore";
            QString c4 = "Channel " + QString::number(ch + 1) + " on/off";
            QString c5 = "Channel " + QString::number(ch + 1) + " fx send 1";
            QString c6 = "Channel " + QString::number(ch + 1) + " fx send 2";
            QString c7 = "Channel " + QString::number(ch + 1) + " fx send 3";
            QString c8 = "Channel " + QString::number(ch + 1) + " fx send 4";
            controllers[i].name = c1.toLatin1().constData();
            controllers[i].num  = MusECore::CTRL_NRPN14_OFFSET+i;
            controllers[i].min  = 0;
            controllers[i].max  = 127;

            controllers[i+1].name = c2.toLatin1().constData();
            controllers[i+1].num  = MusECore::CTRL_NRPN14_OFFSET+i+1;
            controllers[i+1].min  = 0;
            controllers[i+1].max  = 127;

            controllers[i+2].name = c3.toLatin1().constData();
            controllers[i+2].num  = MusECore::CTRL_NRPN14_OFFSET+i+2;
            controllers[i+2].min  = 0;
            controllers[i+2].max  = 1;

            controllers[i+3].name = c4.toLatin1().constData();
            controllers[i+3].num  = MusECore::CTRL_NRPN14_OFFSET+i+3;
            controllers[i+3].min  = 0;
            controllers[i+3].max  = 1;

            controllers[i+4].name = c5.toLatin1().constData();
            controllers[i+4].num  = MusECore::CTRL_NRPN14_OFFSET+i+4;

            controllers[i+5].name = c6.toLatin1().constData();
            controllers[i+5].num  = MusECore::CTRL_NRPN14_OFFSET+i+5;

            controllers[i+6].name = c7.toLatin1().constData();
            controllers[i+6].num  = MusECore::CTRL_NRPN14_OFFSET+i+6;

            controllers[i+7].name = c8.toLatin1().constData();
            controllers[i+7].num  = MusECore::CTRL_NRPN14_OFFSET+i+7;

            controllers[i+4].min = controllers[i+5].min = controllers[i+6].min = controllers[i+7].min = 0;
            controllers[i+4].max = controllers[i+5].max = controllers[i+6].max = controllers[i+7].max = 127;

            i+=8;
            }

      for (int sfx=0; sfx<SS_NR_OF_SENDEFFECTS; sfx++) {
            QString c1 = "Sendfx " + QString::number(sfx) + " ret gain";
            QString c2 = "Sendfx " + QString::number(sfx) + " on/off";
            controllers[i].name = c1.toLatin1().constData();
            controllers[i].num  = MusECore::CTRL_NRPN14_OFFSET+i;
            controllers[i].min  = 0;
            controllers[i].max  = 127;

            controllers[i+1].name = c2.toLatin1().constData();
            controllers[i+1].num  = MusECore::CTRL_NRPN14_OFFSET+i+1;
            controllers[i+1].min  = 0;
            controllers[i+1].max  = 1;
            i+=2;
            }

      pthread_mutex_init(&SS_LoaderMutex, NULL);
      SS_TRACE_OUT
      }

//---------------------------------------------------------
//   ~SimpleSynth
//---------------------------------------------------------
SimpleSynth::~SimpleSynth()
      {
      SS_TRACE_IN

      if(gui)        
        delete gui;  // p4.0.27
        
      // Cleanup channels and samples:
      SS_DBG("Cleaning up sample data");
      for (int i=0; i<SS_NR_OF_CHANNELS; i++) {
            if (channels[i].sample) {
                  delete[] channels[i].sample->data;
                  delete channels[i].sample;
                  }
            }
      simplesynth_ptr = NULL;

      SS_DBG("Deleting pluginlist");
      //Cleanup plugins:
      for (iPlugin i = plugins.begin(); i != plugins.end(); ++i) {
            delete (*i);
            }
      plugins.clear();

      SS_DBG("Deleting sendfx buffers");
      //Delete sendfx buffers:
      for (int i=0; i<SS_NR_OF_SENDEFFECTS; i++) {
            delete[] sendFxLineOut[i][0];
            delete[] sendFxLineOut[i][1];
            delete[] sendFxReturn[i][0];
            delete[] sendFxReturn[i][1];
            }

      //processBuffer:
      SS_DBG("Deleting process buffer");
      delete[] processBuffer[0];
      delete[] processBuffer[1];
      if (initBuffer)
      {
            SS_DBG("Deleting init buffer");
            delete [] initBuffer;
      }      
      SS_TRACE_OUT
      }

int SimpleSynth::oldMidiStateHeader(const unsigned char** data) const 
{
  static unsigned char const d[2] = {MUSE_SYNTH_SYSEX_MFG_ID, SIMPLEDRUMS_UNIQUE_ID};
  *data = &d[0];
  return 2; 
}
        
//---------------------------------------------------------
//   nativeGuiVisible
/*!
    \fn SimpleSynth::nativeGuiVisible
    \brief Tells if the gui is hidden or shown
    \return true/false if gui is shown/hidden
 */
//---------------------------------------------------------
bool SimpleSynth::nativeGuiVisible() const
      {
      SS_TRACE_IN
      bool v = gui->isVisible();
      SS_TRACE_OUT
      return v;
      }

//---------------------------------------------------------
//   hasNativeGui
/*!
    \fn SimpleSynth::hasNativeGui
    \brief Tells if the synth has a gui or not
    \return true if synth has gui, false it synth has no gui
 */
//---------------------------------------------------------
bool SimpleSynth::hasNativeGui() const
      {
      SS_TRACE_IN
      SS_TRACE_OUT
      return true;
      }

//---------------------------------------------------------
//   playNote
/*!
    \fn SimpleSynth::playNote
    \brief Triggers a note on (noteoffs are noteons with velo=0)
    \param channel midi channel
    \param pitch note pitch
    \param velo note velocity
    \return false for ok, true for not ok (not sure these are handled differently, but...)
 */
//---------------------------------------------------------
bool SimpleSynth::playNote(int /*channel*/, int pitch, int velo)
      {
      SS_TRACE_IN
      //Don't bother about channel, we're processing every playnote!
      if ((pitch >= SS_LOWEST_NOTE) && (pitch <= SS_HIGHEST_NOTE)) {
            bool noteOff = (velo == 0 ? 1 : 0);
            int ch = pitch - SS_LOWEST_NOTE;
            if(!noteOff) {
                  if (channels[ch].sample) {
                        //Turn on the white stuff:
                        channels[ch].playoffset = 0;
                        SWITCH_CHAN_STATE(ch , SS_SAMPLE_PLAYING);
                        channels[ch].cur_velo = (double) velo / 127.0;
                        channels[ch].gain_factor = channels[ch].cur_velo * channels[ch].volume;
                        if (SS_DEBUG_MIDI) {
                              printf("Playing note %d on channel %d\n", pitch, ch);
                              }
                        }
                  }
            else {
                  //Note off:
                  ///if (channels[ch].noteoff_ignore) {
                  if (!channels[ch].noteoff_ignore) {     // p4.0.27
                        if (SS_DEBUG_MIDI) {
                              printf("Note off on channel %d\n", ch);
                              }
                        SWITCH_CHAN_STATE(ch , SS_CHANNEL_INACTIVE);
                        channels[ch].playoffset = 0;
                        channels[ch].cur_velo = 0;
                        }
                  }
            }
      SS_TRACE_OUT
      return false;
      }

//---------------------------------------------------------
//   processEvent
/*!
    \fn SimpleSynth::processEvent
    \brief All events from sequencer first shows up here and are forwarded to their correct functions
    \param event The event sent from sequencer
    \return false for ok, true for not ok
 */
//---------------------------------------------------------
bool SimpleSynth::processEvent(const MusECore::MidiPlayEvent& ev)
      {
      SS_TRACE_IN
      switch(ev.type()) {
            case MusECore::ME_CONTROLLER:
                  if (SS_DEBUG_MIDI) {
                        printf("SimpleSynth::processEvent - Controller. Chan: %x dataA: %x dataB: %x\n", ev.channel(), ev.dataA(), ev.dataB());
                        for (int i=0; i< ev.len(); i++)
                              printf("%x ", ev.data()[i]);
                        }
                  setController(ev.channel(), ev.dataA(), ev.dataB(), false);
                  //return true;  // ??
                  return false;   
            case MusECore::ME_NOTEON:
                  return playNote(ev.channel(), ev.dataA(), ev.dataB());
            case MusECore::ME_NOTEOFF:
                  return playNote(ev.channel(), ev.dataA(), 0);
            case MusECore::ME_SYSEX:
                  //Debug print
                  if (SS_DEBUG_MIDI) {
                        printf("SimpleSynth::processEvent - Sysex received\n");
                        for (int i=0; i< ev.len(); i++)
                              printf("%x ", ev.data()[i]);
                        printf("\n");
                        }
                  return sysex(ev.len(), ev.data());
            }
      return false;
      SS_TRACE_OUT
      }

//---------------------------------------------------------
//   setController
/*!
    \fn SimpleSynth::setController
    \brief Called from sequencer indirectly via SimpleSynth::processEvent
    \brief when the synth is supposed to set a controller value
    \param channel channel nr
    \param id controller id
    \param val value of controller
    \return false for ok, true for not ok
 */
//---------------------------------------------------------
bool SimpleSynth::setController(int channel, int id, int val)
      {
      SS_TRACE_IN
      if (SS_DEBUG_MIDI) {
            printf("SimpleSynth::setController - received controller on channel %d, id %d value %d\n", channel, id, val);
            }

      // Channel controllers:
      if (id >= SS_FIRST_CHANNEL_CONTROLLER && id <= SS_LAST_CHANNEL_CONTROLLER ) {
            // Find out which channel we're dealing with:
            id-= SS_FIRST_CHANNEL_CONTROLLER;
            int ch = (id / SS_NR_OF_CHANNEL_CONTROLLERS);
            id = (id % SS_NR_OF_CHANNEL_CONTROLLERS);

            switch (id) {
                  case SS_CHANNEL_CTRL_VOLUME:
                        if (SS_DEBUG_MIDI)
                              printf("Received channel ctrl volume %d for channel %d\n", val, ch);
                        channels[ch].volume_ctrlval = val;
                        updateVolume(ch, val);
                        break;
                  case SS_CHANNEL_CTRL_NOFF:
                        if (SS_DEBUG_MIDI)
                              printf("Received ctrl noff %d for channel %d\n", val, ch);
                        channels[ch].noteoff_ignore = val;
                        break;
                  case SS_CHANNEL_CTRL_PAN:
                        {
                        if (SS_DEBUG_MIDI)
                              printf("Received ctrl pan %d for channel %d\n", val, ch);
                        channels[ch].pan = val;
                        updateBalance(ch, val);
                        break;
                        }
                  case SS_CHANNEL_CTRL_ONOFF:
                        {
                        if (SS_DEBUG_MIDI)
                              printf("Received ctrl onoff %d for channel %d\n", val, ch);

                        if (val == false && channels[ch].channel_on == true) {
                              SWITCH_CHAN_STATE(ch, SS_CHANNEL_INACTIVE);
                              channels[ch].channel_on = val;
                              }
                        else if (val == true && channels[ch].channel_on == false) { // if it actually _was_ off:
                              SWITCH_CHAN_STATE(ch, SS_CHANNEL_INACTIVE);
                              channels[ch].playoffset = 0;
                              channels[ch].channel_on = val;
                              }
                        break;
                        }
                  case SS_CHANNEL_SENDFX1:
                  case SS_CHANNEL_SENDFX2:
                  case SS_CHANNEL_SENDFX3:
                  case SS_CHANNEL_SENDFX4:
                        {
                        int fxid = id - SS_CHANNEL_SENDFX1;
                        channels[ch].sendfxlevel[fxid] = (double)val/127.0;
                        break;
                        }

                  default:
                        if (SS_DEBUG_MIDI)
                              printf("Unknown controller received for channel %d. id=%d\n", ch, id);
                        break;
                  }
            }
      // Master controllers:
      else if (id >= SS_FIRST_MASTER_CONTROLLER && id <= SS_LAST_MASTER_CONTROLLER) {
            if (SS_DEBUG_MIDI)
                  printf("Mastervol controller received: %d\n", id);
            master_vol_ctrlval = val;
            master_vol = (double) master_vol_ctrlval / SS_MASTER_VOLUME_QUOT;
            }
      // Emmm, this one should've been there in the beginning
      else if (id == MusECore::CTRL_VOLUME) {
            if (SS_DEBUG_MIDI) {
                  printf("Ctrl volume received: vol: %d\n", val);
                  }
            master_vol_ctrlval = val;
            master_vol = (double) master_vol_ctrlval / SS_MASTER_VOLUME_QUOT;
            //This one can't be from the gui, update gui:
            guiUpdateMasterVol(val);
            }
      // Plugin controllers:
      else if (id >= SS_FIRST_PLUGIN_CONTROLLER && id <= SS_LAST_PLUGIN_CONTROLLER) {

            int fxid = (id - SS_FIRST_PLUGIN_CONTROLLER) / SS_NR_OF_PLUGIN_CONTROLLERS;
            int cmd = (id - SS_FIRST_PLUGIN_CONTROLLER) % SS_NR_OF_PLUGIN_CONTROLLERS;

            // Plugin return-gain:
            if (cmd == SS_PLUGIN_RETURN) {
                  if (SS_DEBUG_MIDI)
                        printf("Ctrl fx retgain received: fxid: %d val: %d\n", fxid, val);
                  sendEffects[fxid].retgain_ctrlval = val;
                  sendEffects[fxid].retgain = (double) val / 75.0;
                  }
            // Plugin on/off:
            else if (cmd == SS_PLUGIN_ONOFF) {
                  if (SS_DEBUG_MIDI)
                        printf("Ctrl fx onoff received: fxid: %d val: %d\n", fxid, val);
                  sendEffects[fxid].state = (SS_SendFXState) val;
                  }
            }
      else {
            if (SS_DEBUG_MIDI)
                  printf("Unknown controller received: %d\n", id);
            }
      SS_TRACE_OUT
      return false;
      }

//---------------------------------------------------------
/*!
    \fn SimpleSynth::setController
 */
//---------------------------------------------------------
bool SimpleSynth::setController(int channel, int id, int val, bool /*fromGui*/)
      {
      SS_TRACE_IN
      bool ret = setController(channel, id, val); //Perhaps TODO... Separate events from the gui
      SS_TRACE_OUT
      return ret;
      }
//---------------------------------------------------------
//   sysex
/*!
    \fn SimpleSynth::sysex
    \brief Called from sequencer indirectly via SimpleSynth::processEvent
    \param len length of the sysex data
    \param data the sysex data
    \return false for ok, true for not ok
*/
//---------------------------------------------------------
bool SimpleSynth::sysex(int len, const unsigned char* d)
      {
      if(len < 3 || d[0] != MUSE_SYNTH_SYSEX_MFG_ID 
          || d[1] != SIMPLEDRUMS_UNIQUE_ID) 
      {
        if (SS_DEBUG_MIDI)
          printf("MusE SimpleDrums: Unknown sysex header\n");
        return false;
      }
      
      const unsigned char* data = d + 2;
      SS_TRACE_IN
      int cmd = data[0];
      switch (cmd) {
            case SS_SYSEX_LOAD_SAMPLE:
                  {
                  int channel = data[1];
                  //int l = data[2];
                  const char* filename = (const char*)(data+3);
                  if (SS_DEBUG_MIDI) {
                        printf("Sysex cmd: load sample, filename %s, on channel: %d\n", filename, channel);
                        }
                  loadSample(channel, filename);
                  break;
                  }
            case SS_SYSEX_CLEAR_SAMPLE:
                  {
                  int ch = data[1];
                  clearSample(ch);
                  break;
                  }

            case SS_SYSEX_INIT_DATA:
                  {
                  parseInitData(data);
                  break;
                  }

            case SS_SYSEX_LOAD_SENDEFFECT:
                  {
                  int fxid = data[1];
                  QString lib = (const char*) (data + 2);
                  QString label = (const char*) (data + lib.length() + 3);
                  if (SS_DEBUG_MIDI) {
                        printf("Sysex cmd load effect: %d %s %s\n", fxid, lib.toLatin1().constData(), label.toLatin1().constData());
                        }
                  initSendEffect(fxid, lib, label);
                  break;
                  }

            case SS_SYSEX_CLEAR_SENDEFFECT:
                  {
                  int fxid = data[1];
                  if (SS_DEBUG_MIDI) {
                        printf("Sysex cmd clear effect: %d\n", fxid);
                        }
                  sendEffects[fxid].state = SS_SENDFX_OFF;
                  cleanupPlugin(fxid);
                  sendEffects[fxid].plugin = 0;
                  break;
                  }

            case SS_SYSEX_SET_PLUGIN_PARAMETER:
                  {
                  int fxid = data[1];
                  int parameter = data[2];
                  int val = data[3];
                  // Write it to the plugin:
                  float floatval = sendEffects[fxid].plugin->convertGuiControlValue(parameter, val);
                  setFxParameter(fxid, parameter, floatval);
                  break;
                  }

            case SS_SYSEX_GET_INIT_DATA:
                  {
                  int initdata_len = 0;
                  const byte* tmp_initdata = NULL;
                  ///byte* event_data = NULL;

                  getInitData(&initdata_len, &tmp_initdata);
                  ///int totlen = initdata_len + 1;

                  ///event_data = new byte[initdata_len + 1];
                  ///event_data[0] = SS_SYSEX_SEND_INIT_DATA;
                  *((byte*)(tmp_initdata) + 1) = SS_SYSEX_SEND_INIT_DATA;    // Re-use the synth ID byte as the command byte.
                  
                  ///memcpy(event_data + 1, tmp_initdata, initdata_len);
                  ///delete[] tmp_initdata;
                  ///tmp_initdata = NULL;

                  ///MusECore::MidiPlayEvent ev(0, 0, MusECore::ME_SYSEX, event_data, totlen);
                  MusECore::MidiPlayEvent ev(0, 0, MusECore::ME_SYSEX, tmp_initdata + 1, initdata_len - 1);  // Strip MFG ID.
                  gui->writeEvent(ev);
                  ///delete[] event_data;

                  break;
                  }

            default:
                  if (SS_DEBUG_MIDI)
                        printf("Unknown sysex cmd received: %d\n", cmd);
                  break;
            }
      SS_TRACE_OUT
      return false;
      }

//---------------------------------------------------------
//   getPatchName
/*!
    \fn SimpleSynth::getPatchName
    \brief Called from host to get names of patches
    \param index - which patchnr we're about to deliver
    \return const char* with patchname
 */
//---------------------------------------------------------
const char* SimpleSynth::getPatchName(int /*index*/, int, int) const
      {
      SS_TRACE_IN
      SS_TRACE_OUT
      return 0;
      }

//---------------------------------------------------------
//   getPatchInfo
/*!
    \fn SimpleSynth::getPatchInfo
    \brief Called from host to get info about patches
    \param index - which patchnr we're about to deliver
    \param patch - if this one is 0, this is the first call, otherwise keep deliver the host patches... or something
    \return MidiPatch with patch info for host
 */
//---------------------------------------------------------
const MidiPatch* SimpleSynth::getPatchInfo(int /*index*/, const MidiPatch* /*patch*/) const
      {
      SS_TRACE_IN
      //index = 0; patch = 0;
      SS_TRACE_OUT
      return 0;
      }

//---------------------------------------------------------
//   getControllerInfo
/*!
    \fn SimpleSynth::getControllerInfo
    \brief Called from host to collect info about which controllers the synth supports
    \param index current controller number
    \param name pointer where name is stored
    \param controller int pointer where muse controller number is stored
    \param min int pointer where controller min value is stored
    \param max int pointer where controller max value is stored
    \return 0 when done, otherwise return next desired controller index
 */
//---------------------------------------------------------
int SimpleSynth::getControllerInfo(int index, const char** name, int* controller, int* min, int* max, int* initval) const
      {
      SS_TRACE_IN
      if (index >= SS_NR_OF_CONTROLLERS) {
            SS_TRACE_OUT
            return 0;
            }

      *name = controllers[index].name.c_str();
      *controller = controllers[index].num;
      *min = controllers[index].min;
      *max = controllers[index].max;

      *initval = 0;                // p4.0.27 FIXME NOTE TODO    
      
      if (SS_DEBUG_MIDI) {   
            printf("setting controller info: index %d name %s controller %d min %d max %d\n", index, *name, *controller, *min, *max);
            }
      SS_TRACE_OUT
      return (index +1);
      }

//---------------------------------------------------------
//   processMessages
//   Called from host always, even if output path is unconnected.
//---------------------------------------------------------

void SimpleSynth::processMessages()
{
      //Process messages from the gui
      while (gui->fifoSize()) {
            MusECore::MidiPlayEvent ev = gui->readEvent();
            if (ev.type() == MusECore::ME_SYSEX) {
                  sysex(ev.len(), ev.data());
                  sendEvent(ev);
                  }
            else if (ev.type() == MusECore::ME_CONTROLLER) {
                  setController(ev.channel(), ev.dataA(), ev.dataB(), true);
                  sendEvent(ev);
                  }
            else {
                  if (SS_DEBUG)
                        printf("SimpleSynth::process(): unknown event, type: %d\n", ev.type());
                  }
            }
}

//---------------------------------------------------------
//   process
/*!
    \fn SimpleSynth::process
    \brief Realtime function where the processing actually occurs
    \param channels - audio data
    \param offset - sample offset
    \param len - nr of samples to process
 */
//---------------------------------------------------------
void SimpleSynth::process(float** out, int offset, int len)
      {
      /*
      //Process messages from the gui
      while (gui->fifoSize()) {
            MusECore::MidiPlayEvent ev = gui->readEvent();
            if (ev.type() == MusECore::ME_SYSEX) {
                  sysex(ev.len(), ev.data());
                  sendEvent(ev);
                  }
            else if (ev.type() == MusECore::ME_CONTROLLER) {
                  setController(ev.channel(), ev.dataA(), ev.dataB(), true);
                  sendEvent(ev);
                  }
            else {
                  if (SS_DEBUG)
                        printf("SimpleSynth::process(): unknown event, type: %d\n", ev.type());
                  }
            }
      */
      
      if (synth_state == SS_RUNNING) {

      //Temporary mix-doubles
      double out1, out2;
      //double ltemp, rtemp;
      float* data;
      // Velocity factor:
      double gain_factor;


      // Clear send-channels. Skips if fx not turned on
      for (int i=0; i<SS_NR_OF_SENDEFFECTS; i++) {
            if (sendEffects[i].state == SS_SENDFX_ON) {
                  memset(sendFxLineOut[i][0], 0, SS_SENDFX_BUFFER_SIZE * sizeof(float));
                  memset(sendFxLineOut[i][1], 0, SS_SENDFX_BUFFER_SIZE * sizeof(float));
                  }
            }


      memset(out[0] + offset, 0, len * sizeof(float));
      memset(out[1] + offset, 0, len * sizeof(float));

      //Process 1 channel at a time
      for (int ch=0; ch < SS_NR_OF_CHANNELS; ch++) {
            // If channels is turned off, skip:
            if (channels[ch].channel_on == false)
                  continue;

            //If sample isn't playing, skip:
            if (channels[ch].state == SS_SAMPLE_PLAYING) {
                  memset(processBuffer[0], 0, SS_PROCESS_BUFFER_SIZE * sizeof(double));
                  memset(processBuffer[1], 0, SS_PROCESS_BUFFER_SIZE * sizeof(double));

                  for (int i=0; i<len; i++) {
                        // Current channel sample data:
                        data = channels[ch].sample->data;
                        gain_factor = channels[ch].gain_factor;
                        // Current velocity factor:

                        if (channels[ch].sample->channels == 2) {
                              //
                              // Stereo sample:
                              //
                              // Add from sample:
                              out1 = (double) (data[channels[ch].playoffset] * gain_factor * channels[ch].balanceFactorL);
                              out2 = (double) (data[channels[ch].playoffset + 1] * gain_factor * channels[ch].balanceFactorR);
                              channels[ch].playoffset += 2;
                              }
                        else {
                              //
                              // Mono sample:
                              //
                              out1 = (double) (data[channels[ch].playoffset] * gain_factor * channels[ch].balanceFactorL);
                              out2 = (double) (data[channels[ch].playoffset] * gain_factor * channels[ch].balanceFactorR);
                              channels[ch].playoffset++;
                              }

                        processBuffer[0][i] = out1;
                        processBuffer[1][i] = out2;

                        // If send-effects tap is on, tap signal to respective lineout channel
                        for (int j=0; j<SS_NR_OF_SENDEFFECTS; j++) {
                              if (channels[ch].sendfxlevel[j] != 0.0) {
                                    //If the effect has 2 inputs (stereo in):
                                    if (sendEffects[j].inputs == 2) {
                                          sendFxLineOut[j][0][i]+= (out1 * channels[ch].sendfxlevel[j]);
                                          sendFxLineOut[j][1][i]+= (out2 * channels[ch].sendfxlevel[j]);
                                          }
                                    //If the effect is mono (1 input), only use first fxLineOut
                                    else if (sendEffects[j].inputs == 1) {
                                          sendFxLineOut[j][0][i]+= ((out1 + out2) * channels[ch].sendfxlevel[j] / 2.0);
                                          }
                                    //Effects with 0 or >2 inputs are ignored
                                    }
                              }

                        //
                        // If we've reached the last sample, set state to inactive
                        //
                        if (channels[ch].playoffset >= channels[ch].sample->samples) {
                              SWITCH_CHAN_STATE(ch, SS_CHANNEL_INACTIVE);
                              channels[ch].playoffset = 0;
                              break;
                              }
                        }
                        // Add contribution for this channel, for this frame, to final result:
                        for (int i=0; i<len; i++) {
                              out[0][i+offset]+=processBuffer[0][i];
                              out[1][i+offset]+=processBuffer[1][i];
                              }
                  }
            }
            // Do something funny with the sendies:
            for (int j=0; j<SS_NR_OF_SENDEFFECTS; j++) {
                  if (sendEffects[j].state == SS_SENDFX_ON) {
                        sendEffects[j].plugin->process(len);
                        for (int i=0; i<len; i++) {
                              //Effect has mono output:
                              if (sendEffects[j].outputs == 1) {
                                    //Add the result to both channels:
                                    out[0][i+offset]+=((sendEffects[j].retgain * sendFxReturn[j][0][i]) / 2.0);
                                    out[1][i+offset]+=((sendEffects[j].retgain * sendFxReturn[j][0][i]) / 2.0);
                                    }
                              else if (sendEffects[j].outputs == 2) {
                                    // Effect has stereo output
                                    out[0][i+offset]+=(sendEffects[j].retgain * sendFxReturn[j][0][i]);
                                    out[1][i+offset]+=(sendEffects[j].retgain * sendFxReturn[j][1][i]);
                                    }
                              }
                        }
                  }
            // Finally master gain:
            for (int i=0; i<len; i++) {
                  out[0][i+offset] = (out[0][i+offset] * master_vol);
                  out[1][i+offset] = (out[1][i+offset] * master_vol);
                  }
            }
      }

//---------------------------------------------------------
//   showNativeGui
/*!
    \fn SimpleSynth::showNativeGui
    \brief Displays or hides the gui window
    \param val true or false = gui shown or hidden
 */
//---------------------------------------------------------
void SimpleSynth::showNativeGui(bool val)
      {
      SS_TRACE_IN
      gui->setVisible(val);
      SS_TRACE_OUT
      }

//---------------------------------------------------------
/*!
    \fn SimpleSynth::init
    \brief Initializes the SimpleSynth
    \param name string set to caption in the gui dialog
    \return true if successful, false if unsuccessful
 */
//---------------------------------------------------------
bool SimpleSynth::init(const char* name)
      {
      SS_TRACE_IN
      SWITCH_SYNTH_STATE(SS_INITIALIZING);
      gui = new SimpleSynthGui();
      gui->show();
      gui->setWindowTitle(name);
      SWITCH_SYNTH_STATE(SS_RUNNING);
      SS_TRACE_OUT
      return true;
      }

//---------------------------------------------------------
// getInitBuffer
//---------------------------------------------------------
void SimpleSynth::setupInitBuffer(int len)
{
  if (len > initLen) {
        if (initBuffer)
              delete [] initBuffer;
        initBuffer = new byte[len];
        initLen = len;    
        }
}

//---------------------------------------------------------
/*!
    \fn SimpleSynth::getInitData
    \brief Data for reinitialization of SimpleSynth when loading project
    \param n - number of chars used in the data
    \param data - data that is sent as a sysex to the synth on reload of project
 */
//---------------------------------------------------------
//void SimpleSynth::getInitData(int* n, const unsigned char** data) const
void SimpleSynth::getInitData(int* n, const unsigned char** data) 
      {
      SS_TRACE_IN
      // Calculate length of data
      // For each channel, we need to store volume, pan, noff, onoff
      int len = SS_NR_OF_CHANNEL_CONTROLLERS * SS_NR_OF_CHANNELS;
      // Sampledata: filenames len
      for (int i=0; i<SS_NR_OF_CHANNELS; i++) {
            if (channels[i].sample) {
                  int filenamelen = strlen(channels[i].sample->filename.c_str()) + 2;
                  len+=filenamelen;
                  }
            else
                  len++; //Add place for SS_NO_SAMPLE
            }
      len+=3; // 1 place for SS_SYSEX_INIT_DATA, 1 byte for master vol, 1 byte for version data

      // Effect data length
      len++; //Add place for SS_SYSEX_INIT_DATA_VERSION, as control

      for (int i=0; i<SS_NR_OF_SENDEFFECTS; i++) {
            Plugin* plugin = sendEffects[i].plugin;
            if (plugin) {
                  int namelen = plugin->lib().size() + 2;
                  int labelnamelen = plugin->label().size() + 2;
                  len+=(namelen + labelnamelen);

                  ///len+=3; //1 byte for nr of parameters, 1 byte for return gain, 1 byte for effect on/off
                  // p4.0.27 Tim.
                  len+=6; //4 bytes for nr of parameters, 1 byte for return gain, 1 byte for effect on/off
                  
                  len+=sendEffects[i].nrofparameters; // 1 byte for each parameter value
                  }
            else {
                  len++; //place for SS_NO_PLUGIN
                  }
            }

      len += 2;  // For header.

      // First, SS_SYSEX_INIT_DATA
      
      ///byte* buffer = new byte[len];
      setupInitBuffer(len);
      
      memset(initBuffer, 0, len);
      //initBuffer[0] = SS_SYSEX_INIT_DATA;
      //initBuffer[1] = SS_SYSEX_INIT_DATA_VERSION;
      initBuffer[0] = MUSE_SYNTH_SYSEX_MFG_ID;
      initBuffer[1] = SIMPLEDRUMS_UNIQUE_ID;
      initBuffer[2] = SS_SYSEX_INIT_DATA;
      initBuffer[3] = SS_SYSEX_INIT_DATA_VERSION;
      if (SS_DEBUG_INIT) {
            printf("Length of init data: %d\n", len);
            //printf("initBuffer[0] - SS_SYSEX_INIT_DATA: %d\n", SS_SYSEX_INIT_DATA);
            //printf("initBuffer[1] - SS_SYSEX_INIT_DATA_VERSION: %d\n", SS_SYSEX_INIT_DATA_VERSION);
            printf("initBuffer[2] - SS_SYSEX_INIT_DATA: %d\n", SS_SYSEX_INIT_DATA);
            printf("initBuffer[3] - SS_SYSEX_INIT_DATA_VERSION: %d\n", SS_SYSEX_INIT_DATA_VERSION);
            }
      //int i = 2;
      int i = 4;
      // All channels:
      // 0       - volume ctrlval (0-127)
      // 1       - pan (0-127)
      // 2       - noff ignore (0-1)
      // 3       - channel on/off (0-1)
      // 4 - 7   - sendfx 1-4 (0-127)
      // 8       - len of filename, n
      // 9 - 9+n - filename
      for (int ch=0; ch<SS_NR_OF_CHANNELS; ch++) {
            initBuffer[i]   = (byte) channels[ch].volume_ctrlval;
            initBuffer[i+1] = (byte) channels[ch].pan;
            initBuffer[i+2] = (byte) channels[ch].noteoff_ignore;
            initBuffer[i+3] = (byte) channels[ch].channel_on;
            initBuffer[i+4] = (byte) round(channels[ch].sendfxlevel[0] * 127.0);
            initBuffer[i+5] = (byte) round(channels[ch].sendfxlevel[1] * 127.0);
            initBuffer[i+6] = (byte) round(channels[ch].sendfxlevel[2] * 127.0);
            initBuffer[i+7] = (byte) round(channels[ch].sendfxlevel[3] * 127.0);

            if (SS_DEBUG_INIT) {
                  printf("Channel %d:\n", ch);
                  printf("initBuffer[%d] - channels[ch].volume_ctrlval = \t%d\n", i, channels[ch].volume_ctrlval);
                  printf("initBuffer[%d] - channels[ch].pan = \t\t%d\n", i+1, channels[ch].pan);
                  printf("initBuffer[%d] - channels[ch].noteoff_ignore = \t%d\n", i+2, channels[ch].noteoff_ignore );
                  printf("initBuffer[%d] - channels[ch].channel_on = \t%d\n", i+3, channels[ch].channel_on);
                  for (int j= i+4; j < i+8; j++) {
                        printf("initBuffer[%d] - channels[ch].sendfxlevel[%d]= \t%d\n", j, j-i-4, (int)round(channels[ch].sendfxlevel[j-i-4] * 127.0));
                        }
                  }
            if (channels[ch].sample) {
                  int filenamelen = strlen(channels[ch].sample->filename.c_str()) + 1;
                  initBuffer[i+8] = (byte) filenamelen;
                  memcpy((initBuffer+(i+9)), channels[ch].sample->filename.c_str(), filenamelen);
                  if (SS_DEBUG_INIT) {
                        printf("initBuffer[%d] - filenamelen: %d\n", i+8, filenamelen);
                        printf("initBuffer[%d] - initBuffer[%d] - filename: ", (i+9), (i+9) + filenamelen - 1);
                        for (int j = i+9; j< i+9+filenamelen; j++) {
                              printf("%c",initBuffer[j]);
                              }
                        printf("\n");
                        }
                  i+= (SS_NR_OF_CHANNEL_CONTROLLERS + 1 + filenamelen);
                  }
            else {
                  initBuffer[i+8] = SS_NO_SAMPLE;
                  if (SS_DEBUG_INIT) {
                        printf("initBuffer[%d]: SS_NO_SAMPLE: - %d\n", i+8, SS_NO_SAMPLE);
                        }
                  i+= (SS_NR_OF_CHANNEL_CONTROLLERS + 1);
                  }
            }
      if (SS_DEBUG_INIT) {
            printf("initBuffer[%d]: Master vol: - %d\n", i, master_vol_ctrlval);
            }
      initBuffer[i] = master_vol_ctrlval;
      *(data) = initBuffer; *n = len;
      i++;

      //Send effects:
      
      ///initBuffer[i] = SS_SYSEX_INIT_DATA_VERSION; //Just for check
      // Jun 10 2011. Bumped version up from 1 (with its own ID). p4.0.27 Tim
      initBuffer[i] = SS_SYSEX_EFFECT_INIT_DATA_VERSION;   //Just for check     
      
      if (SS_DEBUG_INIT) {
            printf("initBuffer[%d]: Control value, SS_SYSEX_EFFECT_INIT_DATA_VERSION = %d\n", i, SS_SYSEX_EFFECT_INIT_DATA_VERSION);
            }

      i++;
      for (int j=0; j<SS_NR_OF_SENDEFFECTS; j++) {
            if (sendEffects[j].plugin) {
                  int labelnamelen = sendEffects[j].plugin->label().size() + 1;
                  initBuffer[i] = labelnamelen;
                  memcpy((initBuffer+i+1), sendEffects[j].plugin->label().toLatin1().constData(), labelnamelen);
                  if (SS_DEBUG_INIT) {
                        printf("initBuffer[%d] - labelnamelen: %d\n", i, labelnamelen);
                        printf("initBuffer[%d] - initBuffer[%d] - filename: ", (i+1), (i+1) + labelnamelen - 1);
                        for (int k = i+1; k < i+1+labelnamelen; k++) {
                              printf("%c",initBuffer[k]);
                              }
                        printf("\n");
                        }

                  i+=(labelnamelen + 1);

                  int namelen = sendEffects[j].plugin->lib().size() + 1;
                  initBuffer[i] = namelen;
                  memcpy((initBuffer+i+1), sendEffects[j].plugin->lib().toLatin1().constData(), namelen);
                  if (SS_DEBUG_INIT) {
                        printf("initBuffer[%d] - libnamelen : %d\n", i, namelen);
                        printf("initBuffer[%d] - initBuffer[%d] - filename: ", (i+1), (i+1) + namelen - 1);
                        for (int k = i+1; k < i+1+namelen; k++) {
                              printf("%c",initBuffer[k]);
                              }
                        printf("\n");
                        }

                  i+=(namelen + 1);

                  ///initBuffer[i]=sendEffects[j].nrofparameters;
                  // Jun 10 2011. Changed to 32 bit. p4.0.27 Tim.
                  *((unsigned*)&initBuffer[i]) = sendEffects[j].nrofparameters;
                  if (SS_DEBUG_INIT) {
                        printf("initBuffer[%d]: sendEffects[%d].nrofparameters=%d\n", i, j, *((unsigned*)&initBuffer[i]));
                        }
                  ///i++;
                  i+=4;

                  initBuffer[i]=sendEffects[j].retgain_ctrlval;
                  if (SS_DEBUG_INIT) {
                        printf("initBuffer[%d]: sendEffects[%d].retgain_ctrlval=%d\n", i, j, initBuffer[i]);
                        }
                  i++;

                  // Jun 10 2011. This one was missing. p4.0.27 Tim.
                  initBuffer[i] = sendEffects[j].state;
                  if (SS_DEBUG_INIT) {
                        printf("initBuffer[%d]: sendEffects[%d].state=%d\n", i, j, initBuffer[i]);
                        }
                  i++;

                  for (int k=0; k<sendEffects[j].nrofparameters; k++) {
                        //TODO: Convert to 127-scale
                        initBuffer[i] = sendEffects[j].plugin->getGuiControlValue(k);
                        if (SS_DEBUG_INIT) {
                              printf("initBuffer[%d]: sendEffects[%d].parameterval[%d]=%d\n", i, j, k, initBuffer[i]);
                              }
                        i++;
                        }
                  }
            // No plugin loaded:
            else {
                  initBuffer[i] = SS_NO_PLUGIN;
                  if (SS_DEBUG_INIT) {
                        printf("initBuffer[%d]: SS_NO_PLUGIN\n", i);
                        }
                  i++;
                  }
            }

      SS_TRACE_OUT
      }


/*!
    \fn SimpleSynth::parseInitData()
 */
void SimpleSynth::parseInitData(const unsigned char* data)
      {
      SS_TRACE_IN
      //int len = strlen((const char*)data);
      if (SS_DEBUG_INIT) {
            printf("buffer[1], SS_SYSEX_INIT_DATA_VERSION=%d\n", *(data+1));
            }
      const byte* ptr = data+2;
      for (int ch=0; ch<SS_NR_OF_CHANNELS; ch++) {
               channels[ch].volume_ctrlval = (byte) *(ptr);

               if (SS_DEBUG_INIT) {
                     printf("Channel %d:\n", ch);
                     //willyfoobar-2011-02-13
                     // ptr-data might be long long on 64 bit machine --> cast to "long"
                     // *(ptr+...) has type byte --> use %hhd as format item
                     /* old code  
                     printf("buffer[%ld] - channels[ch].volume_ctrlval = \t%d\n", ptr-data, *ptr);
                     printf("buffer[%ld] - channels[ch].pan = \t\t%d\n", ptr-data+1, *(ptr+1));
                     printf("buffer[%ld] - channels[ch].noteoff_ignore = \t%d\n", ptr-data+2, *(ptr+2));
                     printf("buffer[%ld] - channels[ch].channel_on = \t%d\n", ptr-data+3, *(ptr+3));
                     */
                     printf("buffer[%ld] - channels[ch].volume_ctrlval = \t%hhd\n", long(ptr-data), *ptr);
                     printf("buffer[%ld] - channels[ch].pan = \t\t%hhd\n", long(ptr-data+1), *(ptr+1));
                     printf("buffer[%ld] - channels[ch].noteoff_ignore = \t%hhd\n", long(ptr-data+2), *(ptr+2));
                     printf("buffer[%ld] - channels[ch].channel_on = \t%hhd\n", long(ptr-data+3), *(ptr+3));
                     }
               updateVolume(ch, *(ptr));
               guiUpdateVolume(ch, *(ptr));

               channels[ch].pan = *(ptr+1);
               updateBalance(ch, *(ptr+1));
               guiUpdateBalance(ch, *(ptr+1));

               channels[ch].noteoff_ignore = *(ptr+2);
               guiUpdateNoff(ch, *(ptr+2));

               channels[ch].channel_on = *(ptr+3);
               guiUpdateChoff(ch, *(ptr+3));

               ptr+=4;

               for (int i=0; i<4; i++) {
                     channels[ch].sendfxlevel[i] = (float) (*(ptr)/127.0);
                     guiUpdateSendFxLevel(ch, i, *(ptr));
                     ptr++;
                     }

               bool hasSample = *(ptr);
               ptr++;

               channels[ch].sample = 0;
               channels[ch].playoffset = 0;
               SWITCH_CHAN_STATE(ch, SS_CHANNEL_INACTIVE);
               if (SS_DEBUG_INIT) {
                     printf("parseInitData: channel %d, volume: %f pan: %d bfL %f bfR %f chON %d s1: %f s2: %f s3: %f s4: %f\n",
                              ch,
                              channels[ch].volume,
                              channels[ch].pan,
                              channels[ch].balanceFactorL,
                              channels[ch].balanceFactorR,
                              channels[ch].channel_on,
                              channels[ch].sendfxlevel[0],
                              channels[ch].sendfxlevel[1],
                              channels[ch].sendfxlevel[2],
                              channels[ch].sendfxlevel[3]
                              );
                     }
               if (hasSample) {
                     std::string filenametmp = (const char*) ptr;
                     ptr+= strlen(filenametmp.c_str()) + 1;
                     //printf("We should load %s\n", filenametmp.c_str());
                     loadSample(ch, filenametmp.c_str());
                     }
               else {
                     //Clear sample
                     clearSample(ch);
                     guiNotifySampleCleared(ch);
                     }
               }
      //Master vol:
      master_vol_ctrlval = *(ptr);
      master_vol = (double) master_vol_ctrlval / SS_MASTER_VOLUME_QUOT;
      guiUpdateMasterVol(master_vol_ctrlval);
      if (SS_DEBUG_INIT) {
                  printf("Master vol: %d\n", master_vol_ctrlval);
                  }
      ptr++;

      // Effects:
      ///if (*(ptr) != SS_SYSEX_INIT_DATA_VERSION) {
      int effver = *(ptr);
      if (effver < 1 || effver > SS_SYSEX_EFFECT_INIT_DATA_VERSION) {
            //if (SS_DEBUG_INIT)
              fprintf(stderr, "Error loading init data - effect init version is from future or too old. Skipping...\n");
            SS_TRACE_OUT
            return;
            }
      
      ptr++;

      for (int i=0; i<SS_NR_OF_SENDEFFECTS; i++) {
          if (SS_DEBUG_INIT){
                  //wilyfoobar-2011-02-13
                  // arg2 :pointer diifference might be 64 bit (long long)  on 64 bit machine,  this requires cast to long
                  // arg4: *ptr is type byte, not int, this requires format %hhd
                  //old code //printf("buffer[%ld] - sendeffect[%d], labelnamelen=%d\n", ptr-data, i, *ptr);
                  printf("buffer[%ld] - sendeffect[%d], labelnamelen=%hhd\n", long(ptr-data), i, *ptr);
                  }
            int labelnamelen = *(ptr);

            if (labelnamelen != SS_NO_PLUGIN) {
                  ptr++;
                  std::string labelnametmp = (const char*) ptr;
                  ptr+= labelnamelen;

                  //int libnamelen = *(ptr);
                  ptr++;
                  std::string libnametmp = (const char*) ptr;
                  ptr+= strlen(libnametmp.c_str()) + 1;


                  initSendEffect(i, libnametmp.c_str(), labelnametmp.c_str());
                  //initSendEffect(0, "cmt", "freeverb3");

                  ///byte params = *(ptr);
                  unsigned params = (effver < 2) ? *(ptr) : *((unsigned*)ptr);   // p4.0.27
                  ptr+= (effver < 2) ? 1 : 4;
                  
                  ///byte retgain = *(ptr+1);
                  ///ptr+=2;
                  byte retgain = *(ptr);                                     // p4.0.27
                  ptr++;

                  sendEffects[i].nrofparameters = params;

                  sendEffects[i].retgain_ctrlval = retgain;
                  sendEffects[i].retgain = retgain;
                  sendEffects[i].retgain = (double) retgain/ 75.0;
                  MusECore::MidiPlayEvent ev(0, 0, 0, MusECore::ME_CONTROLLER, SS_PLUGIN_RETURNLEVEL_CONTROLLER(i), retgain);
                  gui->writeEvent(ev);

                  // Jun 10 2011. This one was missing. p4.0.27 Tim.
                  if(effver >= 2)
                  {
                    if (SS_DEBUG_INIT)
                        printf("buffer[%ld] - sendeffect[%d] state=%d\n", long(ptr-data), i, *(ptr));
                    sendEffects[i].state = (SS_SendFXState) *(ptr);
                    MusECore::MidiPlayEvent ev(0, 0, 0, MusECore::ME_CONTROLLER, SS_PLUGIN_ONOFF_CONTROLLER(i), sendEffects[i].state);
                    gui->writeEvent(ev);
                    ptr++;
                  }  
                  
                  ///for (int j=0; j<params; j++) {
                  for (unsigned j=0; j<params; j++) {
                      if (SS_DEBUG_INIT){
                          //wilyfoobar-2011-02-13
                          // arg2 :pointer diifference might be 64 bit (long long)  on 64 bit machine,  this requires cast to long
                          // arg5: *ptr is type byte, not int, this requires format %hhd
                          //old code//printf("buffer[%ld] - sendeffect[%d], parameter[%d]=%d\n", long(ptr-data, i, j, *ptr);
                          printf("buffer[%ld] - sendeffect[%d], parameter[%d]=%hhd\n", long(ptr-data), i, j, *ptr);
                          }
                        setFxParameter(i, j, sendEffects[i].plugin->convertGuiControlValue(j, *(ptr)));
                        ptr++;
                        }
                  }
            else {
                  if (sendEffects[i].plugin)
                        cleanupPlugin(i);
                  ptr++;
                  }
            }

      SS_TRACE_OUT
      }

/*!
    \fn SimpleSynth::loadSample(int chno, const char* filename)
 */
bool SimpleSynth::loadSample(int chno, const char* filename)
      {
      SS_TRACE_IN
      SS_Channel* ch = &channels[chno];

      // Thread stuff:
      SS_SampleLoader* loader = new SS_SampleLoader;
      loader->channel = ch;
      loader->filename = std::string(filename);
      loader->ch_no   = chno;
      if (SS_DEBUG) {
            printf("Loader filename is: %s\n", filename);
            }
      pthread_t sampleThread;
      pthread_attr_t* attributes = (pthread_attr_t*) malloc(sizeof(pthread_attr_t));
      pthread_attr_init(attributes);
      pthread_attr_setdetachstate(attributes, PTHREAD_CREATE_DETACHED);
      if (pthread_create(&sampleThread, attributes, ::loadSampleThread, (void*) loader)) {
            perror("creating thread failed:");
            pthread_attr_destroy(attributes);
            delete loader;
            return false;
            }

      pthread_attr_destroy(attributes);
      SS_TRACE_OUT
      return true;
      }

/*!
    \fn loadSampleThread(void* p)
    \brief Since process needs to respond withing a certain time, loading of samples need to be done in a separate thread
 */
static void* loadSampleThread(void* p)
      {
      SS_TRACE_IN
      pthread_mutex_lock(&SS_LoaderMutex);

      // Crit section:
      SS_State prevState = synth_state;
      SWITCH_SYNTH_STATE(SS_LOADING_SAMPLE);
      SS_SampleLoader* loader = (SS_SampleLoader*) p;
      SS_Channel* ch = loader->channel;
      int ch_no      = loader->ch_no;

      if (ch->sample) {
            delete[] ch->sample->data;
            delete ch->sample;
            }
      ch->sample = new SS_Sample;
      SS_Sample* smp = ch->sample;

      SNDFILE* sf;
      const char* filename = loader->filename.c_str();
      SF_INFO sfi;

      if (SS_DEBUG)
            printf("loadSampleThread: filename = %s\n", filename);

      sf = sf_open(filename, SFM_READ, &sfi);
      if (sf == 0) {
            fprintf(stderr,"Error opening file: %s\n", filename);
            SWITCH_SYNTH_STATE(prevState);
            simplesynth_ptr->guiSendSampleLoaded(false, loader->ch_no, filename);
            delete ch->sample; ch->sample = 0;
            delete loader;
            pthread_mutex_unlock(&SS_LoaderMutex);
            SS_TRACE_OUT
            pthread_exit(0);
            }

      //Print some info:
      if (SS_DEBUG) {
            printf("Sample info:\n");
            printf("Frames: \t%ld\n", (long) sfi.frames);
            printf("Channels: \t%d\n", sfi.channels);
            printf("Samplerate: \t%d\n", sfi.samplerate);
            }

      //
      // Allocate and read the thingie
      //

      // If current samplerate is the same as MusE's:
      if (SS_samplerate == sfi.samplerate) {
            smp->data = new float[sfi.channels * sfi.frames];
            sf_count_t n = sf_readf_float(sf, smp->data, sfi.frames);
            smp->frames = sfi.frames;
            smp->samples = (n * sfi.channels);
            smp->channels = sfi.channels;
            if (SS_DEBUG) {
                  printf("%ld frames read\n", (long) n);
                  }
            }
      else  // otherwise, resample:
      {
            smp->channels = sfi.channels;
            // Get new nr of frames:
            double srcratio = (double) SS_samplerate/ (double) sfi.samplerate;
            smp->frames = (long) floor(((double) sfi.frames * srcratio));
            smp->frames = (sfi.channels == 1 ? smp->frames * 2 : smp->frames ); // Double nr of new frames if mono->stereo
            smp->samples = smp->frames * smp->channels;

            if (SS_DEBUG) {
                  //wilyfoobar-2011-02-13
                  // arg2 :sfi.frames is of type sf_count_t (== 64 bit)  (long long)  
                  // this requires format %lld (twice 'l' in format string (arg1)
                  // old code//printf("Resampling from %ld frames to %ld frames - srcration: %lf\n", sfi.frames, smp->frames, srcratio);
                  //printf("Resampling from %lld frames to %ld frames - srcration: %lf\n", sfi.frames, smp->frames, srcratio);
                  // Changed by Tim. Just avoid the hassle for now. Need to determine 32/64 bit and provide two different printf lines.
                  printf("Resampling to %ld frames - srcration: %lf\n", smp->frames, srcratio);
                  printf("Nr of new samples: %ld\n", smp->samples);
                  }

            // Read to temporary:
            float temp[sfi.frames * sfi.channels];
            int frames_read = sf_readf_float(sf, temp, sfi.frames);
            if (frames_read != sfi.frames) {
                  fprintf(stderr,"Error reading sample %s\n", filename);
                  simplesynth_ptr->guiSendSampleLoaded(false, loader->ch_no, filename);
                  sf_close(sf);
                  SWITCH_SYNTH_STATE(prevState);
                  delete ch->sample; ch->sample = 0;
                  delete loader;
                  pthread_mutex_unlock(&SS_LoaderMutex);
                  pthread_exit(0);
                  SS_TRACE_OUT
                  }

            // Allocate mem for the new one
            smp->data = new float[smp->frames * smp->channels];
            memset(smp->data, 0, sizeof(float)* smp->frames * smp->channels);

            // libsamplerate & co (secret rabbits in the code!)
            SRC_DATA srcdata;
            srcdata.data_in  = temp;
            srcdata.data_out = smp->data;
            srcdata.input_frames  = sfi.frames;
            srcdata.output_frames = smp->frames;
            srcdata.src_ratio = (double) SS_samplerate / (double) sfi.samplerate;

            if (SS_DEBUG) {
                  printf("Converting sample....\n");
                  }

            if (src_simple(&srcdata, SRC_SINC_BEST_QUALITY, sfi.channels)) {
                  SS_ERROR("Error when resampling, ignoring current sample");
                  //TODO: deallocate and stuff
                  }
            else if (SS_DEBUG) {
                  printf("Sample converted. %ld input frames used, %ld output frames generated\n",
                           srcdata.input_frames_used,
                           srcdata.output_frames_gen);
                  }
         }
      //Just close the dam thing
      sf_close(sf);
      SWITCH_SYNTH_STATE(prevState);
      ch->sample->filename = loader->filename;
      simplesynth_ptr->guiSendSampleLoaded(true, ch_no, filename);
      delete loader;
      pthread_mutex_unlock(&SS_LoaderMutex);
      SS_TRACE_OUT
      pthread_exit(0);
      }


//static Mess* instantiate(int sr, const char* name)
static Mess* instantiate(int sr, QWidget*, QString* /*projectPathPtr*/, const char* name)
      {
      printf("SimpleSynth sampleRate %d\n", sr);
      SimpleSynth* synth = new SimpleSynth(sr);
      if (!synth->init(name)) {
            delete synth;
            synth = 0;
            }
      return synth;
      }


/*!
    \fn SimpleSynth::updateBalance(int pan)
 */
void SimpleSynth::updateBalance(int ch, int val)
      {
      SS_TRACE_IN
      channels[ch].pan = val;

      // Balance:
      channels[ch].balanceFactorL = 1.0;
      channels[ch].balanceFactorR = 1.0;
      double offset = 0;
      int dev = val - 64;
      offset = (double) dev / 64.0;
      if (offset < 0) {
            channels[ch].balanceFactorR = 1.0 + offset;
            }
      else {
            channels[ch].balanceFactorL = 1.0 - offset;
            }

      if (SS_DEBUG_MIDI)
            printf("balanceFactorL %f balanceFactorR %f\n", channels[ch].balanceFactorL, channels[ch].balanceFactorR);
      SS_TRACE_OUT
      }


/*!
    \fn SimpleSynth::updateVolume(int invol_ctrlval)
 */
void SimpleSynth::updateVolume(int ch, int invol_ctrlval)
      {
      SS_TRACE_IN
      channels[ch].volume = (double)invol_ctrlval/ (double) SS_CHANNEL_VOLUME_QUOT;
      channels[ch].volume_ctrlval = invol_ctrlval;
      SS_TRACE_OUT
      }


/*!
    \fn SimpleSynth::guiUpdateBalance(int ch, int bal)
 */
void SimpleSynth::guiUpdateBalance(int ch, int bal)
      {
      SS_TRACE_IN
      MusECore::MidiPlayEvent ev(0, 0, ch, MusECore::ME_CONTROLLER, SS_CHANNEL_PAN_CONTROLLER(ch), bal);
      gui->writeEvent(ev);
      SS_TRACE_OUT
      }


/*!
    \fn SimpleSynth::guiUpdateVolume(int ch, int val)
 */
void SimpleSynth::guiUpdateVolume(int ch, int val)
      {
      SS_TRACE_IN
      MusECore::MidiPlayEvent ev(0, 0, 0, MusECore::ME_CONTROLLER, SS_CHANNEL_VOLUME_CONTROLLER(ch), val);
      gui->writeEvent(ev);
      SS_TRACE_OUT
      }


/*!
    \fn SimpleSynth::guiUpdateNoff(bool b)
 */
void SimpleSynth::guiUpdateNoff(int ch, bool b)
      {
      SS_TRACE_IN
      MusECore::MidiPlayEvent ev(0, 0, 0, MusECore::ME_CONTROLLER, SS_CHANNEL_NOFF_CONTROLLER(ch), b);
      gui->writeEvent(ev);
      SS_TRACE_OUT
      }


/*!
    \fn SimpleSynth::guiUpdateChoff(int ch, bool b)
 */
void SimpleSynth::guiUpdateChoff(int ch, bool b)
      {
      SS_TRACE_IN
      MusECore::MidiPlayEvent ev(0, 0, 0, MusECore::ME_CONTROLLER, SS_CHANNEL_ONOFF_CONTROLLER(ch), b);
      gui->writeEvent(ev);
      SS_TRACE_OUT
      }


/*!
    \fn SimpleSynth::guiUpdateMasterVol(int val)
 */
void SimpleSynth::guiUpdateMasterVol(int val)
      {
      SS_TRACE_IN
      MusECore::MidiPlayEvent ev(0, 0, 0, MusECore::ME_CONTROLLER, SS_MASTER_CTRL_VOLUME, val);
      gui->writeEvent(ev);
      SS_TRACE_OUT
      }

/*!
    \fn SimpleSynth::guiUpdateSendFxLevel(int fxid, int level)
 */
void SimpleSynth::guiUpdateSendFxLevel(int channel, int fxid, int level)
      {
      SS_TRACE_IN
      MusECore::MidiPlayEvent ev(0, 0, 0, MusECore::ME_CONTROLLER, SS_CHANNEL_SENDFX_CONTROLLER(channel, fxid), level);
      gui->writeEvent(ev);
      SS_TRACE_OUT
      }


/*!
    \fn SimpleSynth::guiSendSampleLoaded(int ch, const char* filename)
 */
void SimpleSynth::guiSendSampleLoaded(bool success, int ch, const char* filename)
      {
      SS_TRACE_IN
      int len = strlen(filename) + 3; //2 + filenamelen + 1;
      //int len = strlen(filename) + 3 + 2; //2 + filenamelen + 1, + 2 for header;
      byte out[len];

      //out[0] = MUSE_SYNTH_SYSEX_MFG_ID;
      //out[1] = SIMPLEDRUMS_UNIQUE_ID;
      if (success) {
            out[0] = SS_SYSEX_LOAD_SAMPLE_OK;
            //out[2] = SS_SYSEX_LOAD_SAMPLE_OK;
            }
      else {
            out[0] = SS_SYSEX_LOAD_SAMPLE_ERROR;
            //out[2] = SS_SYSEX_LOAD_SAMPLE_ERROR;
            }
      out[1] = ch;
      //out[3] = ch;
      memcpy(out+2, filename, strlen(filename)+1);
      //memcpy(out+4, filename, strlen(filename)+1);
      MusECore::MidiPlayEvent ev(0, 0, MusECore::ME_SYSEX, out, len);
      gui->writeEvent(ev);
      SS_TRACE_OUT
      }


/*!
    \fn SimpleSynth::guiSendError(const char* errorstring)
 */
void SimpleSynth::guiSendError(const char* errorstring)
      {
      SS_TRACE_IN
      byte out[strlen(errorstring)+2];
      //byte out[strlen(errorstring)+4];
      //out[0] = MUSE_SYNTH_SYSEX_MFG_ID;
      //out[1] = SIMPLEDRUMS_UNIQUE_ID;
      out[0] = SS_SYSEX_ERRORMSG;
      //out[2] = SS_SYSEX_ERRORMSG;
      memcpy(out+1, errorstring, strlen(errorstring) +1);
      //memcpy(out+3, errorstring, strlen(errorstring) +1);
      SS_TRACE_OUT
      }

extern "C"
      {
      static MESS descriptor = {
            "SimpleSynth",
            "SimpleSynth by Mathias Lundgren",   // (lunar_shuttle@users.sf.net)",
            "0.1",      //Version string
            MESS_MAJOR_VERSION, MESS_MINOR_VERSION,
            instantiate,
            };
      // We must compile with -fvisibility=hidden to avoid namespace
      // conflicts with global variables.
      // Only visible symbol is "mess_descriptor".
      // (TODO: all plugins should be compiled this way)

      __attribute__ ((visibility("default")))
      const MESS* mess_descriptor() { return &descriptor; }
      }


/*!
    \fn SimpleSynth::initSendEffect(int sendeffectid, QString lib, QString name)
 */
bool SimpleSynth::initSendEffect(int id, QString lib, QString name)
      {
      SS_TRACE_IN
      bool success = false;
      if (sendEffects[id].plugin) {
            //Cleanup if one was already there:
            cleanupPlugin(id);
            }
      sendEffects[id].plugin  = (LadspaPlugin*) plugins.find(lib, name);
      LadspaPlugin* plugin = sendEffects[id].plugin;
      if (plugin) { //We found one

            sendEffects[id].inputs  = plugin->inports();
            sendEffects[id].outputs = plugin->outports();

            if (plugin->instantiate()) {
                  SS_DBG2("Plugin instantiated", name.toLatin1().constData());
                  SS_DBG_I("Parameters", plugin->parameter());
                  SS_DBG_I("No of inputs", plugin->inports());
                  SS_DBG_I("No of outputs",plugin->outports());
                  SS_DBG_I("Inplace-capable", plugin->inPlaceCapable());

                  // Connect inputs/outputs:
                  // If single output/input, only use first channel in sendFxLineOut/sendFxReturn
                  SS_DBG("Connecting ports...");
                  plugin->connectInport(0, sendFxLineOut[id][0]);
                  if (plugin->inports() == 2)
                        plugin->connectInport(1, sendFxLineOut[id][1]);
                  else if (plugin->inports() > 2) {
                        fprintf(stderr, "Plugin has more than 2 inputs, not supported\n");
                        }

                  plugin->connectOutport(0, sendFxReturn[id][0]);
                  if (plugin->outports() == 2)
                        plugin->connectOutport(1, sendFxReturn[id][1]);
                  else if (plugin->outports() > 2) {
                        fprintf(stderr, "Plugin has more than 2 outputs, not supported\n");
                        }
                  SS_DBG("Ports connected");
                  if (plugin->start()) {
                        sendEffects[id].state = SS_SENDFX_ON;
                        success = true;

                        int n = plugin->parameter();
                        sendEffects[id].nrofparameters = n;

                        // This is not nice, but freeverb doesn't want to play until some values are set:
                        if (name == "freeverb3") {
                              setFxParameter(id, 2, 0.5);
                              setFxParameter(id, 3, 0.5);
                              setFxParameter(id, 4, 0.5);
                              guiUpdateFxParameter(id, 2, 0.5);
                              guiUpdateFxParameter(id, 3, 0.5);
                              guiUpdateFxParameter(id, 4, 0.5);
                              }
                        }
                  //TODO: cleanup if failed
                  }
            }
      
      //Notify gui
      ///int len = 3;
      int len = 2 + 4;  // Char is not enough for many plugins. Was causing crash. Changed to 32 bits. p4.0.27 Tim.
      //int len = 5;  
      byte out[len];
      out[0] = SS_SYSEX_LOAD_SENDEFFECT_OK;
      out[1] = id;
      //out[0] = MUSE_SYNTH_SYSEX_MFG_ID;
      //out[1] = SIMPLEDRUMS_UNIQUE_ID;
      //out[2] = SS_SYSEX_LOAD_SENDEFFECT_OK;
      //out[3] = id;
      int j=0;
      for (iPlugin i = plugins.begin(); i!=plugins.end(); i++, j++) {
            if ((*i)->lib() == plugin->lib() && (*i)->label() == plugin->label()) {
                  ///out[2] = j;
                  //out[4] = j;
                  *((unsigned*)(out + 2)) = j;
                  MusECore::MidiPlayEvent ev(0, 0, MusECore::ME_SYSEX, out, len);
                  gui->writeEvent(ev);
                  }
            }

      if (!success) {
            QString errorString = "Error loading plugin \"" + plugin->label() + "\"";
            guiSendError(errorString.toLatin1().constData());
            }
      SS_TRACE_OUT
      return success;
      }


/*!
    \fn SimpleSynth::setSendFxLevel(int channel, int effectid, double val)
 */
void SimpleSynth::setSendFxLevel(int channel, int effectid, double val)
      {
      SS_TRACE_IN
      channels[channel].sendfxlevel[effectid] = val;
      SS_TRACE_OUT
      }


/*!
    \fn SimpleSynth::cleanupPlugin(int id)
 */
void SimpleSynth::cleanupPlugin(int id)
      {
      SS_TRACE_IN
      LadspaPlugin* plugin = sendEffects[id].plugin;
      plugin->stop();
      SS_DBG2("Stopped fx", plugin->label().toLatin1().constData());
      sendEffects[id].nrofparameters = 0;
      sendEffects[id].state = SS_SENDFX_OFF;
      sendEffects[id].plugin = 0;

      byte d[2];
      //byte d[4];
      d[0] = SS_SYSEX_CLEAR_SENDEFFECT_OK;
      d[1] = id;
      //d[0] = MUSE_SYNTH_SYSEX_MFG_ID;
      //d[1] = SIMPLEDRUMS_UNIQUE_ID;
      //d[2] = SS_SYSEX_CLEAR_SENDEFFECT_OK;
      //d[3] = id;
      MusECore::MidiPlayEvent ev(0, 0, MusECore::ME_SYSEX, d, 2);
      //MusECore::MidiPlayEvent ev(0, 0, MusECore::ME_SYSEX, d, 4);
      gui->writeEvent(ev);
      SS_TRACE_OUT
      }


/*!
    \fn SimpleSynth::setFxParameter(int fxid, int param, float val)
    \brief Set fx-parameter on plugin and notify gui
 */
void SimpleSynth::setFxParameter(int fxid, int param, float val)
      {
      SS_TRACE_IN
      LadspaPlugin* plugin = sendEffects[fxid].plugin;
      if (SS_DEBUG_LADSPA) {
            printf("Setting fx parameter: %f\n", val);
            }
      plugin->setParam(param, val);
      //sendEffects[fxid].parameter[param] = val;
      //guiUpdateFxParameter(fxid, param, val);
      SS_TRACE_OUT
      }



/*!
    \fn SimpleSynth::guiUpdateFxParameter(int fxid, int param, float val)
    \brief Notify gui of changed fx-parameter
 */
void SimpleSynth::guiUpdateFxParameter(int fxid, int param, float val)
      {
      SS_TRACE_IN
      LadspaPlugin* plugin = sendEffects[fxid].plugin;
      float min, max;
      plugin->range(param, &min, &max);
      //offset:
      val-= min;

      int intval = plugin->getGuiControlValue(param);
      /*if (plugin->isLog(param)) {
            intval = SS_map_logdomain2pluginparam(logf(val/(max - min) + min));
            }
      else if (plugin->isBool(param)) {
            intval = (int) val;
            }
      else {
            float scale = SS_PLUGIN_PARAM_MAX / (max - min);
            intval = (int) ((val - min) * scale);
            }*/
      if (SS_DEBUG_MIDI) {
            printf("Updating gui, fx parameter. fxid=%d, param=%d val=%d\n", fxid, param, intval);
            }

      byte d[4];
      //byte d[6];
      d[0] = SS_SYSEX_SET_PLUGIN_PARAMETER_OK;
      d[1] = fxid;
      d[2] = param;
      d[3] = intval;
      //d[0] = MUSE_SYNTH_SYSEX_MFG_ID;
      //d[1] = SIMPLEDRUMS_UNIQUE_ID;
      //d[2] = SS_SYSEX_SET_PLUGIN_PARAMETER_OK;
      //d[3] = fxid;
      //d[4] = param;
      //d[5] = intval;
      MusECore::MidiPlayEvent ev(0, 0, MusECore::ME_SYSEX, d, 4);
      //MusECore::MidiPlayEvent ev(0, 0, MusECore::ME_SYSEX, d, 6);
      gui->writeEvent(ev);
      SS_TRACE_OUT
      }


/*!
    \fn SimpleSynth::clearSample(int ch)
    \brief Clears a sample (actually clears a channel)
 */
void SimpleSynth::clearSample(int ch)
      {
      SS_TRACE_IN
      if (channels[ch].sample) {
            if (SS_DEBUG)
                  printf("Clearing sample on channel %d\n", ch);
            SS_State prevstate = synth_state;
            SWITCH_CHAN_STATE(ch, SS_CHANNEL_INACTIVE);
            SWITCH_SYNTH_STATE(SS_CLEARING_SAMPLE);
            if (channels[ch].sample->data) {
                  delete[] channels[ch].sample->data;
                  channels[ch].sample->data = 0;
                  }
            if (channels[ch].sample) {
                  delete channels[ch].sample;
                  channels[ch].sample = 0;
                  }
            SWITCH_SYNTH_STATE(prevstate);
            guiNotifySampleCleared(ch);
            if (SS_DEBUG) {
                  printf("Clear sample - sample cleared on channel %d\n", ch);
                  }
            }
      SS_TRACE_OUT
      }


/*!
    \fn SimpleSynth::guiNotifySampleCleared(int ch)
 */
void SimpleSynth::guiNotifySampleCleared(int ch)
      {
      SS_TRACE_IN
      byte d[2];
      //byte d[4];
      d[0] = SS_SYSEX_CLEAR_SAMPLE_OK;
      d[1] = (byte) ch;
      //d[0] = MUSE_SYNTH_SYSEX_MFG_ID;
      //d[1] = SIMPLEDRUMS_UNIQUE_ID;
      //d[2] = SS_SYSEX_CLEAR_SAMPLE_OK;
      //d[3] = (byte) ch;
      MusECore::MidiPlayEvent ev(0, 0, MusECore::ME_SYSEX, d, 2);
      //MusECore::MidiPlayEvent ev(0, 0, MusECore::ME_SYSEX, d, 4);
      gui->writeEvent(ev);
      SS_TRACE_OUT
      }
