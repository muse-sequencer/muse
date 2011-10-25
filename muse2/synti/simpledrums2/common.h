//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: ./synti/simpledrums2/common.h $
//
// Author: Mathias Lundgren <lunar_shuttle@users.sf.net>, (C) 2004
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
//
// C++ Interface: common
//
// Description:
//

#ifndef __MUSE_TESTO_COMMON_H__
#define __MUSE_TESTO_COMMON_H__

#include "muse/midictrl.h"

#define SS_VERSIONSTRING "1.0"

#define SS_DEBUG        0   
#define SS_DEBUG_INIT   0
#define SS_TRACE_FUNC   0
#define SS_DEBUG_MIDI   0
#define SS_DEBUG_LADSPA 0   
#define SS_DEBUG_STATE  0

#define SS_DBG(string) if (SS_DEBUG) fprintf(stderr, "%s:%d:%s: %s\n", __FILE__ , __LINE__ , __PRETTY_FUNCTION__, string);
#define SS_DBG2(string1, string2) if (SS_DEBUG) fprintf(stderr, "%s:%d:%s: %s: %s\n", __FILE__ , __LINE__ , __PRETTY_FUNCTION__, string1, string2);
#define SS_DBG_I(string1, int) if (SS_DEBUG) fprintf(stderr, "%s:%d:%s: %s: %d\n", __FILE__ , __LINE__ , __PRETTY_FUNCTION__, string1, int);

#define SS_TRACE_IN if (SS_TRACE_FUNC) fprintf (stderr, "->%s:%d\n", __PRETTY_FUNCTION__, __LINE__);
#define SS_TRACE_OUT if (SS_TRACE_FUNC) fprintf (stderr, "<-%s:%d\n", __PRETTY_FUNCTION__, __LINE__);
#define SS_ERROR(string) fprintf(stderr, "SimpleDrums error: %s\n", string)
#define SS_DBG_LADSPA(string1) if (SS_DEBUG_LADSPA) fprintf(stderr, "%s:%d:%s: %s\n", __FILE__ , __LINE__ , __PRETTY_FUNCTION__, string1);
#define SS_DBG_LADSPA2(string1, string2) if (SS_DEBUG_LADSPA) fprintf(stderr, "%s:%d:%s: %s: %s\n", __FILE__ , __LINE__ , __PRETTY_FUNCTION__, string1, string2);

#define SS_SYSEX_INIT_DATA_VERSION           1
#define SS_SYSEX_EFFECT_INIT_DATA_VERSION    2   // Added Jun 15 2011. Original value was SS_SYSEX_INIT_DATA_VERSION (1). p4.0.27 Tim. 

#define SS_NR_OF_CHANNELS                   16
#define SS_AUDIO_CHANNELS                    2
#define SS_NR_OF_SENDEFFECTS                 4

// Controller-related:
#define SS_CHANNEL_CTRL_VOLUME 0
#define SS_CHANNEL_CTRL_PAN    1
#define SS_CHANNEL_CTRL_NOFF   2
#define SS_CHANNEL_CTRL_ONOFF  3
#define SS_CHANNEL_SENDFX1     4
#define SS_CHANNEL_SENDFX2     5
#define SS_CHANNEL_SENDFX3     6
#define SS_CHANNEL_SENDFX4     7

#define SS_PLUGIN_RETURN       0
#define SS_PLUGIN_ONOFF        1

#define SS_NR_OF_MASTER_CONTROLLERS          1
#define SS_NR_OF_CHANNEL_CONTROLLERS         8
#define SS_NR_OF_PLUGIN_CONTROLLERS          2

#define SS_NR_OF_CONTROLLERS                 (SS_NR_OF_MASTER_CONTROLLERS + (SS_NR_OF_CHANNELS * SS_NR_OF_CHANNEL_CONTROLLERS) + (SS_NR_OF_PLUGIN_CONTROLLERS*SS_NR_OF_SENDEFFECTS))
#define SS_FIRST_MASTER_CONTROLLER           MusECore::CTRL_NRPN14_OFFSET
#define SS_FIRST_CHANNEL_CONTROLLER          (SS_FIRST_MASTER_CONTROLLER + SS_NR_OF_MASTER_CONTROLLERS)
#define SS_LAST_MASTER_CONTROLLER            (SS_FIRST_CHANNEL_CONTROLLER - 1)
#define SS_LAST_CHANNEL_CONTROLLER           (SS_FIRST_CHANNEL_CONTROLLER -1 + (SS_NR_OF_CHANNEL_CONTROLLERS * SS_NR_OF_CHANNELS))

#define SS_FIRST_PLUGIN_CONTROLLER           (SS_LAST_CHANNEL_CONTROLLER + 1)
#define SS_LAST_PLUGIN_CONTROLLER            (SS_FIRST_PLUGIN_CONTROLLER -1 + SS_NR_OF_SENDEFFECTS*SS_NR_OF_PLUGIN_CONTROLLERS)

#define SS_MASTER_CTRL_VOLUME                SS_FIRST_MASTER_CONTROLLER

#define SS_CHANNEL_VOLUME_CONTROLLER(int)    (SS_FIRST_CHANNEL_CONTROLLER + (SS_NR_OF_CHANNEL_CONTROLLERS * int) + SS_CHANNEL_CTRL_VOLUME)
#define SS_CHANNEL_PAN_CONTROLLER(int)       (SS_FIRST_CHANNEL_CONTROLLER + (SS_NR_OF_CHANNEL_CONTROLLERS * int) + SS_CHANNEL_CTRL_PAN)
#define SS_CHANNEL_NOFF_CONTROLLER(int)      (SS_FIRST_CHANNEL_CONTROLLER + (SS_NR_OF_CHANNEL_CONTROLLERS * int) + SS_CHANNEL_CTRL_NOFF)
#define SS_CHANNEL_ONOFF_CONTROLLER(int)     (SS_FIRST_CHANNEL_CONTROLLER + (SS_NR_OF_CHANNEL_CONTROLLERS * int) + SS_CHANNEL_CTRL_ONOFF)
#define SS_CHANNEL_SENDFX_CONTROLLER(int1,int2) (SS_FIRST_CHANNEL_CONTROLLER + (SS_NR_OF_CHANNEL_CONTROLLERS * int1) + SS_CHANNEL_SENDFX1 + int2)

#define SS_PLUGIN_RETURNLEVEL_CONTROLLER(int) (SS_FIRST_PLUGIN_CONTROLLER + (int * SS_NR_OF_PLUGIN_CONTROLLERS))
#define SS_PLUGIN_ONOFF_CONTROLLER(int) (SS_FIRST_PLUGIN_CONTROLLER + (int * SS_NR_OF_PLUGIN_CONTROLLERS) + 1)

#define SS_LOWEST_NOTE                       36
#define SS_HIGHEST_NOTE                      (SS_LOWEST_NOTE + SS_NR_OF_CHANNELS)

#define SS_PLUGIN_PARAM_MIN                  0
#define SS_PLUGIN_PARAM_MAX                127

typedef unsigned char byte;

enum {
      SS_SYSEX_LOAD_SAMPLE = 0,   // gui -> synth: tell synth to load sample
      SS_SYSEX_INIT_DATA,         // synth reinitialization, the position of this (1) in the enum must not be changed since this value is written into proj file
      SS_SYSEX_LOAD_SAMPLE_OK,    // synth -> gui: tell gui sample loaded OK
      SS_SYSEX_LOAD_SAMPLE_ERROR, // synth -> gui: tell gui sample ! loaded OK
      SS_SYSEX_CLEAR_SAMPLE,       // gui -> synth: tell synth to clear sample
      SS_SYSEX_CLEAR_SAMPLE_OK,    // synth->gui: confirm sample cleared OK
      SS_SYSEX_LOAD_SENDEFFECT,   // gui -> synth: tell synth to load laspa-effect
      SS_SYSEX_LOAD_SENDEFFECT_OK,// synth->gui: plugin loaded ok
      SS_SYSEX_LOAD_SENDEFFECT_ERROR, // synth->gui: plugin _not_ loaded ok
      SS_SYSEX_CLEAR_SENDEFFECT,  // gui->synth: clear plugin
      SS_SYSEX_CLEAR_SENDEFFECT_OK,// synth->gui: plugin cleared
      SS_SYSEX_SET_PLUGIN_PARAMETER, //gui->synth: set plugin parameter
      SS_SYSEX_SET_PLUGIN_PARAMETER_OK, // synth->gui: set plugin parameter (update gui)
      SS_SYSEX_ERRORMSG,           // synth -> gui: general error message from synth
      SS_SYSEX_GET_INIT_DATA,      // gui->synth: request init data
      SS_SYSEX_SEND_INIT_DATA      // synth->gui: give gui init data
      };

extern int SS_samplerate;
extern float SS_map_pluginparam2logdomain(int pluginparam_val);
extern int SS_map_logdomain2pluginparam(float pluginparam_log);
#endif

