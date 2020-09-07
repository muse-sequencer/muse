//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: plugin.cpp,v 1.21.2.23 2009/12/15 22:07:12 spamatica Exp $
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011-2016 Tim E. Real (terminator356 on sourceforge)
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

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <string>
#include "muse_math.h"
#include <sys/stat.h>

#include <QGridLayout>
#include <QLabel>
#include <QWhatsThis>
#include <QToolBar>
#include <QMessageBox>
#include <QByteArray>
#include <QToolButton>
#include <QComboBox>

#include "globals.h"
#include "gconfig.h"
#include "filedialog.h"
#include "slider.h"
#include "midictrl_consts.h"
#include "plugin.h"
#include "controlfifo.h"
#include "icons.h"
#include "song.h"
#include "doublelabel.h"
#include "fastlog.h"
#include "checkbox.h"
#include "meter.h"
#include "utils.h"
#include "pluglist.h"
#include "gui.h"

#ifdef LV2_SUPPORT
#include "lv2host.h"
#endif

#ifdef VST_NATIVE_SUPPORT
#include "vst_native.h"
#endif

#include "audio.h"
#include "al/dsp.h"

#include "muse_math.h"

// NOTE: To cure circular dependencies these includes are at the bottom.
#include <QScrollArea>
#include <QShowEvent>
#include <QHideEvent>
#include <QAction>
#include <QSpinBox>
#include "xml.h"
#include "plugin_list.h"
#include "track.h"

#ifdef _WIN32
#define S_ISLNK(X) 0
#endif

// Turn on debugging messages.
//#define PLUGIN_DEBUGIN

// Turn on constant stream of debugging messages.
//#define PLUGIN_DEBUGIN_PROCESS

namespace MusEGlobal {
MusECore::PluginList plugins;
MusECore::PluginGroups plugin_groups;
QList<QString> plugin_group_names;

}

namespace MusECore {

//---------------------------------------------------------
//   ladspa2MidiControlValues
//---------------------------------------------------------

bool ladspa2MidiControlValues(const LADSPA_Descriptor* plugin, unsigned long port, int ctlnum, int* min, int* max, int* def)
{
  LADSPA_PortRangeHint range = plugin->PortRangeHints[port];
  LADSPA_PortRangeHintDescriptor desc = range.HintDescriptor;

  float fmin, fmax, fdef;
  int   imin, imax;
  float frng;

  bool hasdef = ladspaDefaultValue(plugin, port, &fdef);
  MidiController::ControllerType t = midiControllerType(ctlnum);

  #ifdef PLUGIN_DEBUGIN
  printf("ladspa2MidiControlValues: ctlnum:%d ladspa port:%lu has default?:%d default:%f\n", ctlnum, port, hasdef, fdef);
  #endif

  if(desc & LADSPA_HINT_TOGGLED)
  {
    #ifdef PLUGIN_DEBUGIN
    printf("ladspa2MidiControlValues: has LADSPA_HINT_TOGGLED\n");
    #endif

    *min = 0;
    *max = 1;
    *def = (int)lrintf(fdef);
    return hasdef;
  }

  float m = 1.0;
  if(desc & LADSPA_HINT_SAMPLE_RATE)
  {
    #ifdef PLUGIN_DEBUGIN
    printf("ladspa2MidiControlValues: has LADSPA_HINT_SAMPLE_RATE\n");
    #endif

    m = float(MusEGlobal::sampleRate);
  }

  if(desc & LADSPA_HINT_BOUNDED_BELOW)
  {
    #ifdef PLUGIN_DEBUGIN
    printf("ladspa2MidiControlValues: has LADSPA_HINT_BOUNDED_BELOW\n");
    #endif

    fmin =  range.LowerBound * m;
  }
  else
    fmin = 0.0;

  if(desc & LADSPA_HINT_BOUNDED_ABOVE)
  {
    #ifdef PLUGIN_DEBUGIN
    printf("ladspa2MidiControlValues: has LADSPA_HINT_BOUNDED_ABOVE\n");
    #endif

    fmax =  range.UpperBound * m;
  }
  else
    fmax = 1.0;

  frng = fmax - fmin;
  imin = lrintf(fmin);
  imax = lrintf(fmax);

  int ctlmn = 0;
  int ctlmx = 127;

  #ifdef PLUGIN_DEBUGIN
  printf("ladspa2MidiControlValues: port min:%f max:%f \n", fmin, fmax);
  #endif

  bool isneg = (imin < 0);
  int bias = 0;
  switch(t)
  {
    case MidiController::RPN:
    case MidiController::NRPN:
    case MidiController::Controller7:
      if(isneg)
      {
        ctlmn = -64;
        ctlmx = 63;
        bias = -64;
      }
      else
      {
        ctlmn = 0;
        ctlmx = 127;
      }
    break;
    case MidiController::Controller14:
    case MidiController::RPN14:
    case MidiController::NRPN14:
      if(isneg)
      {
        ctlmn = -8192;
        ctlmx = 8191;
        bias = -8192;
      }
      else
      {
        ctlmn = 0;
        ctlmx = 16383;
      }
    break;
    case MidiController::Program:
      ctlmn = 0;
      ctlmx = 0x3fff;     // FIXME: Really should not happen or be allowed. What to do here...
    break;
    case MidiController::Pitch:
      ctlmn = -8192;
      ctlmx = 8191;
    break;
    case MidiController::Velo:        // cannot happen
    default:
      break;
  }
  float fctlrng = float(ctlmx - ctlmn);

  // Is it an integer control?
  if(desc & LADSPA_HINT_INTEGER)
  {
    #ifdef PLUGIN_DEBUGIN
    printf("ladspa2MidiControlValues: has LADSPA_HINT_INTEGER\n");
    #endif

    // just clip the limits instead of scaling the whole range. ie fit the range into clipped space.
    if(imin < ctlmn)
      imin = ctlmn;
    if(imax > ctlmx)
      imax = ctlmx;

    *min = imin;
    *max = imax;

    *def = (int)lrintf(fdef);

    return hasdef;
  }

  // It's a floating point control, just use wide open maximum range.
  *min = ctlmn;
  *max = ctlmx;

  float normdef = fdef / frng;
  fdef = normdef * fctlrng;

  // FIXME: TODO: Incorrect... Fix this somewhat more trivial stuff later....

  *def = (int)lrintf(fdef) + bias;

  #ifdef PLUGIN_DEBUGIN
  printf("ladspa2MidiControlValues: setting default:%d\n", *def);
  #endif

  return hasdef;
}

//---------------------------------------------------------
//   midi2LadspaValue
//---------------------------------------------------------

float midi2LadspaValue(const LADSPA_Descriptor* plugin, unsigned long port, int ctlnum, int val)
{
  LADSPA_PortRangeHint range = plugin->PortRangeHints[port];
  LADSPA_PortRangeHintDescriptor desc = range.HintDescriptor;

  float fmin, fmax;
  int   imin;
  float frng;

  MidiController::ControllerType t = midiControllerType(ctlnum);

  #ifdef PLUGIN_DEBUGIN
  printf("midi2LadspaValue: ctlnum:%d ladspa port:%lu val:%d\n", ctlnum, port, val);
  #endif

  float m = 1.0;
  if(desc & LADSPA_HINT_SAMPLE_RATE)
  {
    #ifdef PLUGIN_DEBUGIN
    printf("midi2LadspaValue: has LADSPA_HINT_SAMPLE_RATE\n");
    #endif

    m = float(MusEGlobal::sampleRate);
  }

  if(desc & LADSPA_HINT_BOUNDED_BELOW)
  {
    #ifdef PLUGIN_DEBUGIN
    printf("midi2LadspaValue: has LADSPA_HINT_BOUNDED_BELOW\n");
    #endif

    fmin =  range.LowerBound * m;
  }
  else
    fmin = 0.0;

  if(desc & LADSPA_HINT_BOUNDED_ABOVE)
  {
    #ifdef PLUGIN_DEBUGIN
    printf("midi2LadspaValue: has LADSPA_HINT_BOUNDED_ABOVE\n");
    #endif

    fmax =  range.UpperBound * m;
  }
  else
    fmax = 1.0;

  frng = fmax - fmin;
  imin = lrintf(fmin);

  if(desc & LADSPA_HINT_TOGGLED)
  {
    #ifdef PLUGIN_DEBUGIN
    printf("midi2LadspaValue: has LADSPA_HINT_TOGGLED\n");
    #endif

    if(val > 0)
      return fmax;
    else
      return fmin;
  }

  int ctlmn = 0;
  int ctlmx = 127;

  #ifdef PLUGIN_DEBUGIN
  printf("midi2LadspaValue: port min:%f max:%f \n", fmin, fmax);
  #endif

  bool isneg = (imin < 0);
  int bval = val;
  int cval = val;
  switch(t)
  {
    case MidiController::RPN:
    case MidiController::NRPN:
    case MidiController::Controller7:
      if(isneg)
      {
        ctlmn = -64;
        ctlmx = 63;
        bval -= 64;
        cval -= 64;
      }
      else
      {
        ctlmn = 0;
        ctlmx = 127;
        cval -= 64;
      }
    break;
    case MidiController::Controller14:
    case MidiController::RPN14:
    case MidiController::NRPN14:
      if(isneg)
      {
        ctlmn = -8192;
        ctlmx = 8191;
        bval -= 8192;
        cval -= 8192;
      }
      else
      {
        ctlmn = 0;
        ctlmx = 16383;
        cval -= 8192;
      }
    break;
    case MidiController::Program:
      ctlmn = 0;
      ctlmx = 0xffffff;
    break;
    case MidiController::Pitch:
      ctlmn = -8192;
      ctlmx = 8191;
    break;
    case MidiController::Velo:        // cannot happen
    default:
      break;
  }
  int ctlrng = ctlmx - ctlmn;
  float fctlrng = float(ctlmx - ctlmn);

  // Is it an integer control?
  if(desc & LADSPA_HINT_INTEGER)
  {
    float ret = float(cval);
    if(ret < fmin)
      ret = fmin;
    if(ret > fmax)
      ret = fmax;
    #ifdef PLUGIN_DEBUGIN
    printf("midi2LadspaValue: has LADSPA_HINT_INTEGER returning:%f\n", ret);
    #endif

    return ret;
  }

  // Avoid divide-by-zero error below.
  if(ctlrng == 0)
    return 0.0;

  // It's a floating point control, just use wide open maximum range.
  float normval = float(bval) / fctlrng;
  float ret = normval * frng + fmin;

  #ifdef PLUGIN_DEBUGIN
  printf("midi2LadspaValue: float returning:%f\n", ret);
  #endif

  return ret;
}

//---------------------------------------------------------
//   ladspaCtrlValueType
//---------------------------------------------------------

CtrlValueType ladspaCtrlValueType(const LADSPA_Descriptor* plugin, int port)
{
  LADSPA_PortRangeHint range = plugin->PortRangeHints[port];
  LADSPA_PortRangeHintDescriptor desc = range.HintDescriptor;

  if(desc & LADSPA_HINT_INTEGER)
    return VAL_INT;
  else if(desc & LADSPA_HINT_LOGARITHMIC)
    return VAL_LOG;
  else if(desc & LADSPA_HINT_TOGGLED)
    return VAL_BOOL;
  else
    return VAL_LINEAR;
}

//---------------------------------------------------------
//   ladspaCtrlMode
//---------------------------------------------------------

CtrlList::Mode ladspaCtrlMode(const LADSPA_Descriptor* plugin, int port)
{
  LADSPA_PortRangeHint range = plugin->PortRangeHints[port];
  LADSPA_PortRangeHintDescriptor desc = range.HintDescriptor;

  if(desc & LADSPA_HINT_INTEGER)
    return CtrlList::DISCRETE;
  else if(desc & LADSPA_HINT_LOGARITHMIC)
    return CtrlList::INTERPOLATE;
  else if(desc & LADSPA_HINT_TOGGLED)
    return CtrlList::DISCRETE;
  else
    return CtrlList::INTERPOLATE;
}

// DELETETHIS 20
// Works but not needed.
/*
//---------------------------------------------------------
//   ladspa2MidiController
//---------------------------------------------------------

MidiController* ladspa2MidiController(const LADSPA_Descriptor* plugin, unsigned long port, int ctlnum)
{
  int min, max, def;

  if(!ladspa2MidiControlValues(plugin, port, ctlnum, &min, &max, &def))
    return 0;

  MidiController* mc = new MidiController(QString(plugin->PortNames[port]), ctlnum, min, max, def);

  return mc;
}
*/

//----------------------------------------------------------------------------------
//   defaultValue
//   If no default ladspa value found, still sets *def to 1.0, but returns false.
//---------------------------------------------------------------------------------

bool ladspaDefaultValue(const LADSPA_Descriptor* plugin, unsigned long port, float* val)
{
      if(port < plugin->PortCount)
      {
        LADSPA_PortRangeHint range = plugin->PortRangeHints[port];
        LADSPA_PortRangeHintDescriptor rh = range.HintDescriptor;
        float m = (rh & LADSPA_HINT_SAMPLE_RATE) ? float(MusEGlobal::sampleRate) : 1.0f;

        if (LADSPA_IS_HINT_DEFAULT_MINIMUM(rh))
        {
              *val = range.LowerBound * m;
              return true;
        }
        else if (LADSPA_IS_HINT_DEFAULT_MAXIMUM(rh))
        {
              *val = range.UpperBound*m;
              return true;
        }
        else if (LADSPA_IS_HINT_DEFAULT_LOW(rh))
        {
              if (LADSPA_IS_HINT_LOGARITHMIC(rh))
              {
                *val = expf(logf(range.LowerBound * m) * .75 +
                      logf(range.UpperBound * m) * .25);
                return true;
              }
              else
              {
                *val = range.LowerBound*.75*m + range.UpperBound*.25*m;
                return true;
              }
        }
        else if (LADSPA_IS_HINT_DEFAULT_MIDDLE(rh))
        {
              if (LADSPA_IS_HINT_LOGARITHMIC(rh))
              {
                *val = expf(logf(range.LowerBound * m) * .5 +
                      logf(range.UpperBound * m) * .5);
                return true;
              }
              else
              {
                *val = range.LowerBound*.5*m + range.UpperBound*.5*m;
                return true;
              }
        }
        else if (LADSPA_IS_HINT_DEFAULT_HIGH(rh))
        {
              if (LADSPA_IS_HINT_LOGARITHMIC(rh))
              {
                *val = expf(logf(range.LowerBound * m) * .25 +
                      logf(range.UpperBound * m) * .75);
                return true;
              }
              else
              {
                *val = range.LowerBound*.25*m + range.UpperBound*.75*m;
                return true;
              }
        }
        else if (LADSPA_IS_HINT_DEFAULT_0(rh))
        {
              *val = 0.0;
              return true;
        }
        else if (LADSPA_IS_HINT_DEFAULT_1(rh))
        {
              *val = 1.0;
              return true;
        }
        else if (LADSPA_IS_HINT_DEFAULT_100(rh))
        {
              *val = 100.0;
              return true;
        }
        else if (LADSPA_IS_HINT_DEFAULT_440(rh))
        {
              *val = 440.0;
              return true;
        }

        // No default found. Make one up...
        else if (LADSPA_IS_HINT_BOUNDED_BELOW(rh) && LADSPA_IS_HINT_BOUNDED_ABOVE(rh))
        {
          if (LADSPA_IS_HINT_LOGARITHMIC(rh))
          {
            *val = expf(logf(range.LowerBound * m) * .5 +
                  logf(range.UpperBound * m) * .5);
            return true;
          }
          else
          {
            *val = range.LowerBound*.5*m + range.UpperBound*.5*m;
            return true;
          }
        }
        else if (LADSPA_IS_HINT_BOUNDED_BELOW(rh))
        {
            *val = range.LowerBound;
            return true;
        }
        else if (LADSPA_IS_HINT_BOUNDED_ABOVE(rh))
        {
            // Hm. What to do here... Just try 0.0 or the upper bound if less than zero.
            //if(range.UpperBound > 0.0)
            //  *val = 0.0;
            //else
            //  *val = range.UpperBound * m;
            // Instead try this: Adopt an 'attenuator-like' policy, where upper is the default.
            *val = range.UpperBound * m;
            return true;
        }
      }

      // No default found. Set return value to 0.0, but return false.
      *val = 0.0;
      return false;
}

//---------------------------------------------------------
//   ladspaControlRange
//---------------------------------------------------------

void ladspaControlRange(const LADSPA_Descriptor* plugin, unsigned long port, float* min, float* max)
      {
      LADSPA_PortRangeHint range = plugin->PortRangeHints[port];
      LADSPA_PortRangeHintDescriptor desc = range.HintDescriptor;
      if (desc & LADSPA_HINT_TOGGLED) {
            *min = 0.0;
            *max = 1.0;
            return;
            }
      float m = 1.0;
      if (desc & LADSPA_HINT_SAMPLE_RATE)
            m = float(MusEGlobal::sampleRate);

      if (desc & LADSPA_HINT_BOUNDED_BELOW)
            *min =  range.LowerBound * m;
      else
            *min = 0.0;
      if (desc & LADSPA_HINT_BOUNDED_ABOVE)
            *max =  range.UpperBound * m;
      else
            *max = 1.0;
      }


//==============================================================
//   BEGIN PluginQuirks
//==============================================================

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void PluginQuirks::write(int level, Xml& xml) const
      {
      // Defaults? Nothing to save.
      if(!_fixedSpeed && !_transportAffectsAudioLatency && !_overrideReportedLatency
              && _latencyOverrideValue == 0 && _fixNativeUIScaling == NatUISCaling::GLOBAL)
        return;

      xml.tag(level++, "quirks");

      if(_fixedSpeed)
        xml.intTag(level, "fixedSpeed", _fixedSpeed);

      if(_transportAffectsAudioLatency)
        xml.intTag(level, "trnspAffAudLat", _transportAffectsAudioLatency);

      if(_overrideReportedLatency)
        xml.intTag(level, "ovrRepAudLat", _overrideReportedLatency);

      if(_latencyOverrideValue != 0)
        xml.intTag(level, "latOvrVal", _latencyOverrideValue);

      if(_fixNativeUIScaling != NatUISCaling::GLOBAL)
        xml.intTag(level, "fixNatUIScal", _fixNativeUIScaling);

      xml.etag(--level, "quirks");
      }

//---------------------------------------------------------
//   read
//    return true on error
//---------------------------------------------------------

bool PluginQuirks::read(Xml& xml)
      {
      for (;;) {
            Xml::Token token(xml.parse());
            const QString& tag(xml.s1());
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return true;
                  case Xml::TagStart:
                        if (tag == "fixedSpeed")
                              _fixedSpeed = xml.parseInt();
                        else if (tag == "trnspAffAudLat")
                              _transportAffectsAudioLatency = xml.parseInt();
                        else if (tag == "ovrRepAudLat")
                              _overrideReportedLatency = xml.parseInt();
                        else if (tag == "latOvrVal")
                              _latencyOverrideValue = xml.parseInt();
                        else if (tag == "fixNatUIScal")
                              _fixNativeUIScaling = (NatUISCaling)xml.parseInt();
                        else
                              xml.unknown("PluginQuirks");
                        break;
                  case Xml::Attribut:
                        break;
                  case Xml::TagEnd:
                        if (tag == "quirks") {
                              return false;
                              }
                        return true;
                  default:
                        break;
                  }
            }
      return true;
      }

//==============================================================
//   END PluginQuirks
//==============================================================



//---------------------------------------------------------
//   Plugin
//---------------------------------------------------------

Plugin::Plugin(QFileInfo* f, const LADSPA_Descriptor* d, const QString& uri,
               bool isDssi, bool isDssiSynth, bool isDssiVst, PluginFeatures_t reqFeatures)
{
  _isDssi = isDssi;
  _isDssiSynth = isDssiSynth;
  _isDssiVst = isDssiVst;
  _isLV2Plugin = false;
  _isLV2Synth = false;
  _isVstNativePlugin = false;
  _isVstNativeSynth = false;
  _requiredFeatures = reqFeatures;
  _usesTimePosition = false;


  #ifdef DSSI_SUPPORT
  dssi_descr = NULL;
  #endif

  if(f)
    fi = *f;
  _uri = uri;

  plugin = NULL;
  ladspa = NULL;
  _handle = 0;
  _references = 0;
  _instNo     = 0;
  _label = QString(d->Label);
  _name = QString(d->Name);
  _uniqueID = d->UniqueID;
  _maker = QString(d->Maker);
  _copyright = QString(d->Copyright);

  _portCount = d->PortCount;

  _inports = 0;
  _outports = 0;
  _controlInPorts = 0;
  _controlOutPorts = 0;
  for(unsigned long k = 0; k < _portCount; ++k)
  {
    LADSPA_PortDescriptor pd = d->PortDescriptors[k];
    if(pd & LADSPA_PORT_AUDIO)
    {
      if(pd & LADSPA_PORT_INPUT)
        ++_inports;
      else
      if(pd & LADSPA_PORT_OUTPUT)
        ++_outports;
    }
    else
    if(pd & LADSPA_PORT_CONTROL)
    {
      if(pd & LADSPA_PORT_INPUT)
        ++_controlInPorts;
      else
      if(pd & LADSPA_PORT_OUTPUT)
        ++_controlOutPorts;
    }
  }

  // Hack: Blacklist vst plugins in-place, configurable for now.
  if ((_inports != _outports) || (_isDssiVst && !MusEGlobal::config.vstInPlace))
        _requiredFeatures |= PluginNoInPlaceProcessing;
}

Plugin::Plugin(const MusEPlugin::PluginScanInfoStruct& info)
{
  _isDssi = false;
  _isDssiSynth = false;
  _isDssiVst = false;
  _isLV2Plugin = false;
  _isLV2Synth = false;
  _isVstNativePlugin = false;
  _isVstNativeSynth = false;
  _requiredFeatures = info._requiredFeatures;
  
  switch(info._type)
  {
    case MusEPlugin::PluginScanInfoStruct::PluginTypeLADSPA:
    break;
    
    case MusEPlugin::PluginScanInfoStruct::PluginTypeDSSI:
    {
      _isDssi = true;
      if(info._class & MusEPlugin::PluginScanInfoStruct::PluginClassInstrument)
        _isDssiSynth = true;
    }
    break;
    
    case MusEPlugin::PluginScanInfoStruct::PluginTypeDSSIVST:
    {
      _isDssi = true;
      _isDssiVst = true;
      if(info._class & MusEPlugin::PluginScanInfoStruct::PluginClassInstrument)
        _isDssiSynth = true;
    }
    break;
    
    case MusEPlugin::PluginScanInfoStruct::PluginTypeVST:
    case MusEPlugin::PluginScanInfoStruct::PluginTypeLV2:
    case MusEPlugin::PluginScanInfoStruct::PluginTypeLinuxVST:
    case MusEPlugin::PluginScanInfoStruct::PluginTypeMESS:
    case MusEPlugin::PluginScanInfoStruct::PluginTypeUnknown:
    case MusEPlugin::PluginScanInfoStruct::PluginTypeNone:
    case MusEPlugin::PluginScanInfoStruct::PluginTypeAll:
    break;
  }
  
  #ifdef DSSI_SUPPORT
  dssi_descr = NULL;
  #endif

//   fi = info._fi;
  fi = QFileInfo(PLUGIN_GET_QSTRING(info.filePath()));
  _uri = PLUGIN_GET_QSTRING(info._uri);

  plugin = NULL;
  ladspa = NULL;
  _handle = 0;
  _references = 0;
  _instNo     = 0;
  
  _label = PLUGIN_GET_QSTRING(info._label);
  _name = PLUGIN_GET_QSTRING(info._name);
  _uniqueID = info._uniqueID;
  _maker = PLUGIN_GET_QSTRING(info._maker);
  _copyright = PLUGIN_GET_QSTRING(info._copyright);

  _portCount = info._portCount;

  _inports = info._inports;
  _outports = info._outports;
  _controlInPorts = info._controlInPorts;
  _controlOutPorts = info._controlOutPorts;

  // Hack: Blacklist vst plugins in-place, configurable for now.
  if(_isDssiVst && !MusEGlobal::config.vstInPlace)
    _requiredFeatures |= PluginNoInPlaceProcessing;
  
  _usesTimePosition = info._pluginFlags & MusEPlugin::PluginScanInfoStruct::SupportsTimePosition;
}

Plugin::~Plugin()
{
  if(plugin && !isLV2Plugin() && !isVstNativePlugin())
  //  delete plugin;
    printf("Plugin::~Plugin Error: plugin is not NULL\n");
}

//---------------------------------------------------------
//   incReferences
//---------------------------------------------------------

int Plugin::incReferences(int val)
{
  #ifdef PLUGIN_DEBUGIN
  fprintf(stderr, "Plugin::incReferences _references:%d val:%d\n", _references, val);
  #endif

  int newref = _references + val;

  if(newref <= 0)
  {
    _references = 0;
    if(_handle)
    {
      #ifdef PLUGIN_DEBUGIN
      fprintf(stderr, "Plugin::incReferences no more instances, closing library\n");
      #endif

      dlclose(_handle);
    }

    _handle = 0;
    ladspa = NULL;
    plugin = NULL;
    rpIdx.clear();

    #ifdef DSSI_SUPPORT
    dssi_descr = NULL;
    #endif

    return 0;
  }

  if(_handle == 0)
  {
    _handle = dlopen(fi.filePath().toLatin1().constData(), RTLD_NOW);

    if(_handle == 0)
    {
      fprintf(stderr, "Plugin::incReferences dlopen(%s) failed: %s\n",
              fi.filePath().toLatin1().constData(), dlerror());
      return 0;
    }

    #ifdef DSSI_SUPPORT
    DSSI_Descriptor_Function dssi = (DSSI_Descriptor_Function)dlsym(_handle, "dssi_descriptor");
    if(dssi)
    {
      const DSSI_Descriptor* descr;
      for(unsigned long i = 0;; ++i)
      {
        descr = dssi(i);
        if(descr == NULL)
          break;

        QString label(descr->LADSPA_Plugin->Label);
        if(label == _label)
        {
          _isDssi = true;
          ladspa = NULL;
          dssi_descr = descr;
          plugin = descr->LADSPA_Plugin;
          break;
        }
      }
    }
    else
    #endif // DSSI_SUPPORT
    {
      LADSPA_Descriptor_Function ladspadf = (LADSPA_Descriptor_Function)dlsym(_handle, "ladspa_descriptor");
      if(ladspadf)
      {
        const LADSPA_Descriptor* descr;
        for(unsigned long i = 0;; ++i)
        {
          descr = ladspadf(i);
          if(descr == NULL)
            break;

          QString label(descr->Label);
          if(label == _label)
          {
            _isDssi = false;
            ladspa = ladspadf;
            plugin = descr;

            #ifdef DSSI_SUPPORT
            dssi_descr = NULL;
            #endif

            break;
          }
        }
      }
    }

    if(plugin != NULL)
    {
      _name = QString(plugin->Name);
      _uniqueID = plugin->UniqueID;
      _maker = QString(plugin->Maker);
      _copyright = QString(plugin->Copyright);

      _portCount = plugin->PortCount;

      _inports = 0;
      _outports = 0;
      _controlInPorts = 0;
      _controlOutPorts = 0;
      for(unsigned long k = 0; k < _portCount; ++k)
      {
        LADSPA_PortDescriptor pd = plugin->PortDescriptors[k];
        if(pd & LADSPA_PORT_AUDIO)
        {
          if(pd & LADSPA_PORT_INPUT)
            ++_inports;
          else
          if(pd & LADSPA_PORT_OUTPUT)
            ++_outports;

          rpIdx.push_back((unsigned long)-1);
        }
        else
        if(pd & LADSPA_PORT_CONTROL)
        {
          if(pd & LADSPA_PORT_INPUT)
          {
            rpIdx.push_back(_controlInPorts);
            ++_controlInPorts;
          }
          else
          if(pd & LADSPA_PORT_OUTPUT)
          {
            rpIdx.push_back((unsigned long)-1);
            ++_controlOutPorts;
          }
        }
      }

      // Hack: Blacklist vst plugins in-place, configurable for now.
      if ((_inports != _outports) || (_isDssiVst && !MusEGlobal::config.vstInPlace))
        _requiredFeatures |= PluginNoInPlaceProcessing;
    }
  }

  if(plugin == NULL)
  {
    dlclose(_handle);
    _handle = 0;
    _references = 0;
    fprintf(stderr, "Plugin::incReferences Error: %s no plugin!\n", fi.filePath().toLatin1().constData());
    return 0;
  }

  _references = newref;

  return _references;
}

//---------------------------------------------------------
//   range
//---------------------------------------------------------

void Plugin::range(unsigned long i, float* min, float* max) const
      {
      ladspaControlRange(plugin, i, min, max);
      }

//---------------------------------------------------------
//   defaultValue
//---------------------------------------------------------

double Plugin::defaultValue(unsigned long port) const
{
    float val;
    ladspaDefaultValue(plugin, port, &val);
    return val;
}

//---------------------------------------------------------
//   ctrlValueType
//---------------------------------------------------------

CtrlValueType Plugin::ctrlValueType(unsigned long i) const
      {
      return ladspaCtrlValueType(plugin, i);
      }

//---------------------------------------------------------
//   ctrlMode
//---------------------------------------------------------

CtrlList::Mode Plugin::ctrlMode(unsigned long i) const
      {
      return ladspaCtrlMode(plugin, i);
      }

void PluginGroups::shift_left(int first, int last)
{
  for (int i=first; i<=last; i++)
    replace_group(i, i-1);
}

void PluginGroups::shift_right(int first, int last)
{
  for (int i=last; i>=first; i--)
    replace_group(i,i+1);
}

void PluginGroups::erase(int index)
{
  for (PluginGroups::iterator it=begin(); it!=end(); it++)
  {
    it->remove(index);
  }
}

void PluginGroups::replace_group(int old, int now)
{
  for (PluginGroups::iterator it=begin(); it!=end(); it++)
  {
    if (it->contains(old))
    {
      it->remove(old);
      it->insert(now);
    }
  }
}

//---------------------------------------------------------
//   initPlugins
//---------------------------------------------------------

void initPlugins()
{
  const char* message = "Plugins: loadPluginLib: ";
  const MusEPlugin::PluginScanList& scan_list = MusEPlugin::pluginList;
  for(MusEPlugin::ciPluginScanList isl = scan_list.begin(); isl != scan_list.end(); ++isl)
  {
    const MusEPlugin::PluginScanInfoRef inforef = *isl;
    const MusEPlugin::PluginScanInfoStruct& info = inforef->info();
    switch(info._type)
    {
      case MusEPlugin::PluginScanInfoStruct::PluginTypeLADSPA:
      {
        if(MusEGlobal::loadPlugins)
        {
          // Make sure it doesn't already exist.
          if(const Plugin* pl = MusEGlobal::plugins.find(
            PLUGIN_GET_QSTRING(info._completeBaseName),
            PLUGIN_GET_QSTRING(info._uri),
            PLUGIN_GET_QSTRING(info._label)))
          {
            fprintf(stderr, "Ignoring LADSPA effect label:%s uri:%s path:%s duplicate of path:%s\n",
                    PLUGIN_GET_CSTRING(info._label),
                    PLUGIN_GET_CSTRING(info._uri),
                    PLUGIN_GET_CSTRING(info.filePath()),
                    pl->filePath().toLatin1().constData());
          }
          else
          {
            if(MusEGlobal::debugMsg)
              info.dump(message);
            MusEGlobal::plugins.add(info);
          }
        }
      }
      break;
      
      case MusEPlugin::PluginScanInfoStruct::PluginTypeDSSI:
      case MusEPlugin::PluginScanInfoStruct::PluginTypeDSSIVST:
      {
#ifdef DSSI_SUPPORT
        if(MusEGlobal::loadDSSI)
        {
          // Allow both effects and instruments for now.
          if(info._class & MusEPlugin::PluginScanInfoStruct::PluginClassEffect ||
             info._class & MusEPlugin::PluginScanInfoStruct::PluginClassInstrument)
          {
            // Make sure it doesn't already exist.
            if(const Plugin* pl = MusEGlobal::plugins.find(
              PLUGIN_GET_QSTRING(info._completeBaseName),
              PLUGIN_GET_QSTRING(info._uri),
              PLUGIN_GET_QSTRING(info._label)))
            {
              fprintf(stderr, "Ignoring DSSI effect label:%s uri:%s path:%s duplicate of path:%s\n",
                      PLUGIN_GET_CSTRING(info._label),
                      PLUGIN_GET_CSTRING(info._uri),
                      PLUGIN_GET_CSTRING(info.filePath()),
                      pl->filePath().toLatin1().constData());
            }
            else
            {
              if(MusEGlobal::debugMsg)
                info.dump(message);
              MusEGlobal::plugins.add(info);
            }
          }
        }
#endif
      }
      break;
      
      case MusEPlugin::PluginScanInfoStruct::PluginTypeVST:
      case MusEPlugin::PluginScanInfoStruct::PluginTypeLV2:
      case MusEPlugin::PluginScanInfoStruct::PluginTypeLinuxVST:
      case MusEPlugin::PluginScanInfoStruct::PluginTypeMESS:
      case MusEPlugin::PluginScanInfoStruct::PluginTypeUnknown:
      case MusEPlugin::PluginScanInfoStruct::PluginTypeNone:
      case MusEPlugin::PluginScanInfoStruct::PluginTypeAll:
      break;
    }
  }
}

//---------------------------------------------------------
//   find
//---------------------------------------------------------

Plugin* PluginList::find(const QString& file, const QString& uri, const QString& label) const
      {
      const bool f_empty = file.isEmpty();
      const bool u_empty = uri.isEmpty();
      const bool l_empty = label.isEmpty();
      for (ciPlugin i = begin(); i != end(); ++i) {
            if ( (!u_empty || f_empty || file  == (*i)->lib()) &&
                 (u_empty  || uri   == (*i)->uri()) &&
                 (!u_empty || l_empty || label == (*i)->label()))
                  return *i;
            }

      return 0;
      }

//---------------------------------------------------------
//   Pipeline
//---------------------------------------------------------

Pipeline::Pipeline()
   : std::vector<PluginI*>()
      {
      for(int i = 0; i < MusECore::MAX_CHANNELS; ++i)
        buffer[i] = NULL;
      initBuffers();

      for (int i = 0; i < MusECore::PipelineDepth; ++i)
            push_back(0);
      }

//---------------------------------------------------------
//   Pipeline copy constructor
//---------------------------------------------------------

Pipeline::Pipeline(const Pipeline& p, AudioTrack* t)
   : std::vector<PluginI*>()
      {
      for(int i = 0; i < MusECore::MAX_CHANNELS; ++i)
        buffer[i] = NULL;
      initBuffers();

      for(int i = 0; i < MusECore::PipelineDepth; ++i)
      {
        PluginI* pli = p[i];
        if(pli)
        {
          Plugin* pl = pli->plugin();
          if(pl)
          {
            PluginI* new_pl = new PluginI();
            if(new_pl->initPluginInstance(pl, t->channels())) {
                  fprintf(stderr, "cannot instantiate plugin <%s>\n",
                      pl->name().toLatin1().constData());
                  delete new_pl;
                  }
            else
            {
              // Assigns valid ID and track to plugin, and creates controllers for plugin.
              t->setupPlugin(new_pl, i);
              push_back(new_pl);
              continue;
            }
          }
        }
        push_back(NULL); // No plugin. Initialize with NULL.
      }
      }

//---------------------------------------------------------
//   ~Pipeline
//---------------------------------------------------------

Pipeline::~Pipeline()
      {
      removeAll();
      for (int i = 0; i < MusECore::MAX_CHANNELS; ++i)
          if(buffer[i])
            ::free(buffer[i]);
      }

void Pipeline::initBuffers()
{
  for(int i = 0; i < MusECore::MAX_CHANNELS; ++i)
  {
    if(!buffer[i])
    {
#ifdef _WIN32
      buffer[i] = (float *) _aligned_malloc(16, sizeof(float *) * MusEGlobal::segmentSize);
      if(buffer[i] == NULL)
      {
         fprintf(stderr, "ERROR: Pipeline ctor: _aligned_malloc returned error: NULL. Aborting!\n");
         abort();
      }
#else
      int rv = posix_memalign((void**)(buffer + i), 16, sizeof(float) * MusEGlobal::segmentSize);
      if(rv != 0)
      {
        fprintf(stderr, "ERROR: Pipeline ctor: posix_memalign returned error:%d. Aborting!\n", rv);
        abort();
      }
#endif
    }
  }

  for(int i = 0; i < MusECore::MAX_CHANNELS; ++i)
  {
    if(MusEGlobal::config.useDenormalBias)
    {
      for(unsigned q = 0; q < MusEGlobal::segmentSize; ++q)
        buffer[i][q] = MusEGlobal::denormalBias;
    }
    else
      memset(buffer[i], 0, sizeof(float) * MusEGlobal::segmentSize);
  }
}

//---------------------------------------------------------
//  latency
//---------------------------------------------------------

float Pipeline::latency() const
{
  float l = 0.0f;
  const PluginI* p;
  for(int i = 0; i < MusECore::PipelineDepth; ++i)
  {
    p = (*this)[i];
    if(p)
    {
// REMOVE Tim. lv2. Added. TESTING. Do we need to leave this alone for reporting?
// I think so... It seemed we do that with tracks but then those are wave and midi tracks...
#if 0
      // If the transport affects audio latency, it means we can completely correct
      //  for the latency by adjusting the transport, therefore meaning zero
      //  resulting audio latency. As far as the rest of the app knows, the plugin
      //  in this rack position has zero audio latency. Yet we still retain the
      //  original latency value in each plugin so we can use it.
      if(!p->cquirks()._transportAffectsAudioLatency)
#endif
        l+= p->latency();
    }
  }
  return l;
}

//---------------------------------------------------------
//   addScheduledControlEvent
//   track_ctrl_id is the fully qualified track audio controller number
//   Returns true if event cannot be delivered
//---------------------------------------------------------

bool Pipeline::addScheduledControlEvent(int track_ctrl_id, double val, unsigned frame)
{
  // If a track controller, or the special dssi synth controller block, just return.
  if(track_ctrl_id < AC_PLUGIN_CTL_BASE || track_ctrl_id >= (int)genACnum(MusECore::MAX_PLUGINS, 0))
    return true;
  int rack_idx = (track_ctrl_id - AC_PLUGIN_CTL_BASE) >> AC_PLUGIN_CTL_BASE_POW;
  for (int i = 0; i < MusECore::PipelineDepth; ++i)
  {
    PluginI* p = (*this)[i];
    if(p && p->id() == rack_idx)
      return p->addScheduledControlEvent(track_ctrl_id & AC_PLUGIN_CTL_ID_MASK, val, frame);
  }
  return true;
}

//---------------------------------------------------------
//   controllerEnabled
//   Returns whether automation control stream is enabled or disabled.
//   Used during automation recording to inhibit gui controls
//---------------------------------------------------------

bool Pipeline::controllerEnabled(int track_ctrl_id)
{
  // If a track controller, or the special dssi synth controller block, just return.
  if(track_ctrl_id < AC_PLUGIN_CTL_BASE || track_ctrl_id >= (int)genACnum(MusECore::MAX_PLUGINS, 0))
    return false;
  int rack_idx = (track_ctrl_id - AC_PLUGIN_CTL_BASE) >> AC_PLUGIN_CTL_BASE_POW;
  for (int i = 0; i < MusECore::PipelineDepth; ++i)
  {
    PluginI* p = (*this)[i];
    if(p && p->id() == rack_idx)
      return p->controllerEnabled(track_ctrl_id & AC_PLUGIN_CTL_ID_MASK);
  }
  return false;
}

//---------------------------------------------------------
//   enableController
//   Enable or disable gui automation control stream.
//   Used during automation recording to inhibit gui controls
//---------------------------------------------------------

void Pipeline::enableController(int track_ctrl_id, bool en)
{
  // If a track controller, or the special dssi synth controller block, just return.
  if(track_ctrl_id < AC_PLUGIN_CTL_BASE || track_ctrl_id >= (int)genACnum(MusECore::MAX_PLUGINS, 0))
    return;
  int rack_idx = (track_ctrl_id - AC_PLUGIN_CTL_BASE) >> AC_PLUGIN_CTL_BASE_POW;
  for (int i = 0; i < MusECore::PipelineDepth; ++i)
  {
    PluginI* p = (*this)[i];
    if(p && p->id() == rack_idx)
    {
      p->enableController(track_ctrl_id & AC_PLUGIN_CTL_ID_MASK, en);
      return;
    }
  }
}

//---------------------------------------------------------
//   setChannels
//---------------------------------------------------------

void Pipeline::setChannels(int n)
      {
      for (int i = 0; i < MusECore::PipelineDepth; ++i)
            if ((*this)[i])
                  (*this)[i]->setChannels(n);
      }

//---------------------------------------------------------
//   insert
//    give ownership of object plugin to Pipeline
//---------------------------------------------------------

void Pipeline::insert(PluginI* plugin, int index)
      {
      remove(index);
      (*this)[index] = plugin;
      }

//---------------------------------------------------------
//   remove
//---------------------------------------------------------

void Pipeline::remove(int index)
      {
      PluginI* plugin = (*this)[index];
      if (plugin)
            delete plugin;
      (*this)[index] = 0;
      }

//---------------------------------------------------------
//   removeAll
//---------------------------------------------------------

void Pipeline::removeAll()
      {
      for (int i = 0; i < MusECore::PipelineDepth; ++i)
            remove(i);
      }

//---------------------------------------------------------
//   isOn
//---------------------------------------------------------

bool Pipeline::isOn(int idx) const
      {
      PluginI* p = (*this)[idx];
      if (p)
            return p->on();
      return false;
      }

//---------------------------------------------------------
//   setOn
//---------------------------------------------------------

void Pipeline::setOn(int idx, bool flag)
      {
      PluginI* p = (*this)[idx];
      if (p) {
            p->setOn(flag);
            if (p->gui())
                  p->gui()->setOn(flag);
            }
      }

//---------------------------------------------------------
//   label
//---------------------------------------------------------

QString Pipeline::label(int idx) const
      {
      PluginI* p = (*this)[idx];
      if (p)
            return p->label();
      return QString("");
      }

//---------------------------------------------------------
//   name
//---------------------------------------------------------

QString Pipeline::name(int idx) const
      {
      PluginI* p = (*this)[idx];
      if (p)
            return p->name();
      return "<" + QObject::tr("Slot") + " " + QString::number(idx + 1) + ">";
      }

//---------------------------------------------------------
//   uri
//---------------------------------------------------------

QString Pipeline::uri(int idx) const
      {
      PluginI* p = (*this)[idx];
      if (p)
            return p->uri();
      return QString();
      }


//---------------------------------------------------------
//   empty
//---------------------------------------------------------

bool Pipeline::empty(int idx) const
      {
      PluginI* p = (*this)[idx];
      return p == 0;
      }

//---------------------------------------------------------
//   move
//---------------------------------------------------------

void Pipeline::move(int idx, bool up)
{
      PluginI* p1 = (*this)[idx];
      if (up)
      {
            (*this)[idx]   = (*this)[idx-1];

          if((*this)[idx]) {
            (*this)[idx]->setID(idx);
          }

            (*this)[idx-1] = p1;

          if(p1)
          {
            p1->setID(idx - 1);
            if(p1->track())
              MusEGlobal::audio->msgSwapControllerIDX(p1->track(), idx, idx - 1);
            }
      }
      else
      {
            (*this)[idx]   = (*this)[idx+1];

          if((*this)[idx]) {
            (*this)[idx]->setID(idx);
          }

            (*this)[idx+1] = p1;

          if(p1)
          {
            p1->setID(idx + 1);
            if(p1->track())
              MusEGlobal::audio->msgSwapControllerIDX(p1->track(), idx, idx + 1);
            }
      }
}

//---------------------------------------------------------
//   isDssiPlugin
//---------------------------------------------------------

bool Pipeline::isDssiPlugin(int idx) const
{
  PluginI* p = (*this)[idx];
  if(p)
    return p->isDssiPlugin();

  return false;
}

bool Pipeline::isLV2Plugin(int idx) const
{
   PluginI* p = (*this)[idx];
   if(p)
     return p->isLV2Plugin();

   return false;
}

bool Pipeline::isVstNativePlugin(int idx) const
{
   PluginI* p = (*this)[idx];
   if(p)
     return p->isVstNativePlugin();

   return false;

}

//---------------------------------------------------------
//   has_dssi_ui
//---------------------------------------------------------

bool Pipeline::has_dssi_ui(int idx) const
{
  PluginI* p = (*this)[idx];
  if(p)
  {
#ifdef LV2_SUPPORT
    if(p->plugin() && p->plugin()->isLV2Plugin())
      return ((LV2PluginWrapper *)p->plugin())->hasNativeGui();
#endif

#ifdef VST_NATIVE_SUPPORT
    if(p->plugin() && p->plugin()->isVstNativePlugin())
      return ((VstNativePluginWrapper *)p->plugin())->hasNativeGui();
#endif


      return !p->dssi_ui_filename().isEmpty();
  }

  return false;
}
//---------------------------------------------------------
//   showGui
//---------------------------------------------------------

void Pipeline::showGui(int idx, bool flag)
      {
      PluginI* p = (*this)[idx];
      if (p)
            p->showGui(flag);
      }

//---------------------------------------------------------
//   showNativeGui
//---------------------------------------------------------

#if defined(LV2_SUPPORT) || defined(VST_NATIVE_SUPPORT) || defined(OSC_SUPPORT)
void Pipeline::showNativeGui(int idx, bool flag)
      {
         PluginI* p = (*this)[idx];
#ifdef LV2_SUPPORT
         if(p && p->plugin()->isLV2Plugin())
         {
            ((LV2PluginWrapper *)p->plugin())->showNativeGui(p, flag);
            return;
         }

#endif

#ifdef VST_NATIVE_SUPPORT
         if(p && p->plugin()->isVstNativePlugin())
         {
            ((VstNativePluginWrapper *)p->plugin())->showNativeGui(p, flag);
            return;
         }

#endif
      #ifdef OSC_SUPPORT
         if (p)
            p->oscIF().oscShowGui(flag);
      #endif
      }
#else // defined(LV2_SUPPORT) || defined(VST_NATIVE_SUPPORT) || defined(OSC_SUPPORT)
void Pipeline::showNativeGui(int /*idx*/, bool /*flag*/)
      {
      }
#endif // defined(LV2_SUPPORT) || defined(VST_NATIVE_SUPPORT) || defined(OSC_SUPPORT)

//---------------------------------------------------------
//   deleteGui
//---------------------------------------------------------

void Pipeline::deleteGui(int idx)
{
  if(idx >= MusECore::PipelineDepth)
    return;
  PluginI* p = (*this)[idx];
  if(p)
    p->deleteGui();
#ifdef LV2_SUPPORT
         if(p && p->plugin()->isLV2Plugin())
         {
            ((LV2PluginWrapper *)p->plugin())->showNativeGui(p, false);
         }

#endif

#ifdef VST_NATIVE_SUPPORT
         if(p && p->plugin()->isVstNativePlugin())
         {
            ((VstNativePluginWrapper *)p->plugin())->showNativeGui(p, false);
         }

#endif
}

//---------------------------------------------------------
//   deleteAllGuis
//---------------------------------------------------------

void Pipeline::deleteAllGuis()
{
  for(int i = 0; i < MusECore::PipelineDepth; i++)
    deleteGui(i);
}

//---------------------------------------------------------
//   guiVisible
//---------------------------------------------------------

bool Pipeline::guiVisible(int idx)
      {
      PluginI* p = (*this)[idx];
      if (p)
            return p->guiVisible();
      return false;
      }

//---------------------------------------------------------
//   nativeGuiVisible
//---------------------------------------------------------

bool Pipeline::nativeGuiVisible(int idx)
      {
      PluginI* p = (*this)[idx];
      if (p)
      {
#ifdef LV2_SUPPORT
         if(p->plugin()->isLV2Plugin())
            return ((LV2PluginWrapper *)p->plugin())->nativeGuiVisible(p);
#endif

#ifdef VST_NATIVE_SUPPORT
         if(p->plugin()->isVstNativePlugin())
            return ((VstNativePluginWrapper *)p->plugin())->nativeGuiVisible(p);
#endif

            return p->nativeGuiVisible();

      }
      return false;
      }

//---------------------------------------------------------
//   apply
//   If ports is 0, just process controllers only, not audio (do not 'run').
//---------------------------------------------------------

void Pipeline::apply(unsigned pos, unsigned long ports, unsigned long nframes, float** buffer1)
{
      bool swap = false;

      // Divide up the total pipeline latency to distribute latency correction
      //  among the plugins according to the latency of each plugin. Each has
      //  more correction than the next. The values are negative, meaning 'correction'.
      const int sz = size();
      float latency_corr_offsets[sz];
      float latency_corr_offset = 0.0f;
      for(int i = sz - 1; i >= 0; --i)
      {
        const PluginI* p = (*this)[i];
        if(!p)
          continue;
        const float lat = p->latency();
        // If the transport affects audio latency, it means we can completely correct
        //  for the latency by adjusting the transport, therefore meaning zero
        //  resulting audio latency. As far as the rest of the app knows, the plugin
        //  in this rack position has zero audio latency. Yet we still retain the
        //  original latency value in each plugin so we can use it.
        // Here we use a neat trick to conditionally subtract as we go, yet still 
        //  set the right transport correction offset value for each plugin.
        latency_corr_offsets[i] = latency_corr_offset - lat;
        if(!p->cquirks()._transportAffectsAudioLatency)
          latency_corr_offset -= lat;
      }

      for (int i = 0; i < sz; ++i) {
            PluginI* p = (*this)[i];
            if(!p)
              continue;

            const float corr_offset = latency_corr_offsets[i];
            if (p->on())
            {
              if (!(p->requiredFeatures() & PluginNoInPlaceProcessing))
              {
                    if (swap)
                          p->apply(pos, nframes, ports, buffer, buffer, corr_offset);
                    else
                          p->apply(pos, nframes, ports, buffer1, buffer1, corr_offset);
              }
              else
              {
                    if (swap)
                          p->apply(pos, nframes, ports, buffer, buffer1, corr_offset);
                    else
                          p->apply(pos, nframes, ports, buffer1, buffer, corr_offset);
                    swap = !swap;
              }
            }
            else
            {
              p->apply(pos, nframes, 0, 0, 0, corr_offset); // Do not process (run) audio, process controllers only.
            }
      }
      if (ports != 0 && swap)
      {
            for (unsigned long i = 0; i < ports; ++i)
                  AL::dsp->cpy(buffer1[i], buffer[i], nframes);
      }
}

//---------------------------------------------------------
//   PluginIBase
//---------------------------------------------------------

PluginIBase::PluginIBase()
{
  _gui = 0;
}

PluginIBase::~PluginIBase()
{
  if(_gui)
    delete _gui;
}

//---------------------------------------------------------
//   showGui
//---------------------------------------------------------

void PluginIBase::showGui()
{
  if(_gui == 0)
    makeGui();
  _gui->updateWindowTitle();
  if(_gui->isVisible())
    _gui->hide();
  else
    _gui->show();
}

void PluginIBase::showGui(bool flag)
{
  if(flag)
  {
    if(_gui == 0)
      makeGui();
    _gui->show();
  }
  else
  {
    if(_gui)
      _gui->hide();
  }
}

//---------------------------------------------------------
//   guiVisible
//---------------------------------------------------------

bool PluginIBase::guiVisible() const
{
  return _gui && _gui->isVisible();
}

void PluginIBase::setGeometry(int x, int y, int w, int h)
{
  _guiGeometry = QRect(x, y, w, h);
  if(_gui)
  {
    
#ifdef QT_SHOW_POS_BUG_WORKAROUND
    // Because of the bug, no matter what we must supply a position,
    //  even upon first showing...
    
    // Check sane size.
    if(w == 0)
      w = _gui->sizeHint().width();
    if(h == 0)
      h = _gui->sizeHint().height();

    // No size hint? Try minimum size.
    if(w == 0)
      w = _gui->minimumSize().width();
    if(h == 0)
      h = _gui->minimumSize().height();

    // Fallback.
    if(w == 0)
      w = 200;
    if(h == 0)
      h = 200;
    
    _gui->setGeometry(x, y, w, h);
    
#else    
    
    // If the saved geometry is valid, use it.
    // Otherwise this is probably the first time showing,
    //  so do not set a geometry - let Qt pick one 
    //  (using auto-placement and sizeHint).
    if(!(x == 0 && y == 0 && w == 0 && h == 0))
    {
      // Check sane size.
      if(w == 0)
        w = _gui->sizeHint().width();
      if(h == 0)
        h = _gui->sizeHint().height();
      
      // No size hint? Try minimum size.
      if(w == 0)
        w = _gui->minimumSize().width();
      if(h == 0)
        h = _gui->minimumSize().height();

      // Fallback.
      if(w == 0)
        w = 200;
      if(h == 0)
        h = 200;
      
      _gui->setGeometry(x, y, w, h);
    }
#endif
    
  }
}

// Returns the current geometry of the gui, or if the gui does not exist, 
//  the saved gui geometry.
void PluginIBase::getGeometry(int *x, int *y, int *w, int *h) const
{
  // If gui does not exist return the saved geometry.
  if(!_gui)
  {
    if(x) *x = _guiGeometry.x();
    if(y) *y = _guiGeometry.y();
    if(w) *w = _guiGeometry.width();
    if(h) *h = _guiGeometry.height();
    return;
  }

  // Return the actual gui geometry.
  if(x) *x = _gui->geometry().x();
  if(y) *y = _gui->geometry().y();
  if(w) *w = _gui->geometry().width();
  if(h) *h = _gui->geometry().height();
}

// Saves the current gui geometry.
void PluginIBase::saveGeometry(int x, int y, int w, int h)
{
  _guiGeometry = QRect(x, y, w, h);
}

// Returns the saved gui geometry.
void PluginIBase::savedGeometry(int *x, int *y, int *w, int *h) const
{
  if(x) *x = _guiGeometry.x();
  if(y) *y = _guiGeometry.y();
  if(w) *w = _guiGeometry.width();
  if(h) *h = _guiGeometry.height();
}


// Sets the gui's geometry. Also updates the saved geometry.
void PluginIBase::setNativeGeometry(int x, int y, int w, int h)
{ 
  _nativeGuiGeometry = QRect(x, y, w, h);
}
      
// Returns the current geometry of the gui, or if the gui does not exist, 
//  the saved gui geometry.
void PluginIBase::getNativeGeometry(int *x, int *y, int *w, int *h) const
{
  if(x) *x = 0;
  if(y) *y = 0;
  if(w) *w = 0;
  if(h) *h = 0;
}

// Saves the current gui geometry.
void PluginIBase::saveNativeGeometry(int x, int y, int w, int h)
{
  _nativeGuiGeometry = QRect(x, y, w, h);
}

// Returns the saved gui geometry.
void PluginIBase::savedNativeGeometry(int *x, int *y, int *w, int *h) const
{
  if(x) *x = _nativeGuiGeometry.x();
  if(y) *y = _nativeGuiGeometry.y();
  if(w) *w = _nativeGuiGeometry.width();
  if(h) *h = _nativeGuiGeometry.height();
}
      
//---------------------------------------------------------
//   addScheduledControlEvent
//   i is the specific index of the control input port
//   Returns true if event cannot be delivered
//---------------------------------------------------------

bool PluginIBase::addScheduledControlEvent(unsigned long i, double val, unsigned frame)
{
  if(i >= parameters())
  {
    printf("PluginIBase::addScheduledControlEvent param number %lu out of range of ports:%lu\n", i, parameters());
    return true;
  }
  ControlEvent ce;
  ce.unique = false;
  ce.fromGui = false;
  ce.idx = i;
  ce.value = val;
  // Time-stamp the event. This does a possibly slightly slow call to gettimeofday via timestamp().
  //  timestamp() is more or less an estimate of the current frame. (This is exactly how ALSA events
  //  are treated when they arrive in our ALSA driver.)
  //ce.frame = MusEGlobal::audio->timestamp();
  // p4.0.23 timestamp() is circular, which is making it impossible to deal with 'modulo' events which
  //  slip in 'under the wire' before processing the ring buffers. So try this linear timestamp instead:
  ce.frame = frame;

  if(_controlFifo.put(ce))
  {
    fprintf(stderr, "PluginIBase::addScheduledControlEvent: fifo overflow: in control number:%lu\n", i);
    return true;
  }
  return false;
}

QString PluginIBase::dssi_ui_filename() const
{
  QString libr(lib());
  if(dirPath().isEmpty() || libr.isEmpty())
    return QString();

  QString guiPath(dirPath() + "/" + libr);

  QDir guiDir(guiPath, "*", QDir::Unsorted, QDir::Files);
  if(!guiDir.exists())
    return QString();

  QStringList list = guiDir.entryList();

  QString plug(pluginLabel());
  QString lib_qt_ui;
  QString lib_any_ui;
  QString plug_qt_ui;
  QString plug_any_ui;

  for(int i = 0; i < list.count(); ++i)
  {
    QFileInfo fi(guiPath + QString("/") + list[i]);
    QString gui(fi.filePath());
    struct stat buf;
    if(stat(gui.toLatin1().constData(), &buf))
      continue;
    if(!((S_ISREG(buf.st_mode) || S_ISLNK(buf.st_mode)) &&
        (buf.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH))))
      continue;

    // FIXME: Qt::CaseInsensitive - a quick and dirty way to accept any suffix. Should be case sensitive...
    if(!libr.isEmpty())
    {
      if(lib_qt_ui.isEmpty() && list[i].contains(libr + QString("_qt"), Qt::CaseInsensitive))
        lib_qt_ui = gui;
      if(lib_any_ui.isEmpty() && list[i].contains(libr + QString('_') /*, Qt::CaseInsensitive*/))
        lib_any_ui = gui;
    }
    if(!plug.isEmpty())
    {
      if(plug_qt_ui.isEmpty() && list[i].contains(plug + QString("_qt"), Qt::CaseInsensitive))
        plug_qt_ui = gui;
      if(plug_any_ui.isEmpty() && list[i].contains(plug + QString('_') /*, Qt::CaseInsensitive*/))
        plug_any_ui = gui;
    }
  }

  // Prefer qt plugin ui
  if(!plug_qt_ui.isEmpty())
    return plug_qt_ui;
  // Prefer any plugin ui
  if(!plug_any_ui.isEmpty())
    return plug_any_ui;
  // Prefer qt lib ui
  if(!lib_qt_ui.isEmpty())
    return lib_qt_ui;
  // Prefer any lib ui
  if(!lib_any_ui.isEmpty())
    return lib_any_ui;

  // No suitable UI file found
  return QString();
};

//---------------------------------------------------------
//   PluginI
//---------------------------------------------------------

void PluginI::init()
      {
      _plugin           = 0;
      instances         = 0;
      handle            = 0;
      controls          = 0;
      controlsOut       = 0;
      controlsOutDummy  = 0;
      controlPorts      = 0;
      controlOutPorts   = 0;
      _audioInSilenceBuf = 0;
      _audioOutDummyBuf  = 0;
      _hasLatencyOutPort = false;
      _latencyOutPort = 0;
      _on               = true;
      initControlValues = false;
      _showNativeGuiPending = false;
      }

PluginI::PluginI()
      {
      _id = -1;
      _track = 0;

      init();
      }

//---------------------------------------------------------
//   PluginI
//---------------------------------------------------------

PluginI::~PluginI()
      {
      #ifdef OSC_SUPPORT
      _oscif.oscSetPluginI(NULL);
      #endif

      if (_plugin) {
            deactivate();
            _plugin->incReferences(-1);
            }

      if(_audioInSilenceBuf)
        free(_audioInSilenceBuf);
      if(_audioOutDummyBuf)
        free(_audioOutDummyBuf);

      if (controlsOutDummy)
            delete[] controlsOutDummy;
      if (controlsOut)
            delete[] controlsOut;
      if (controls)
            delete[] controls;
      if (handle)
            delete[] handle;
      }

//---------------------------------------------------------
//   setID
//---------------------------------------------------------

void PluginI::setID(int i)
{
  _id = i;
}

//---------------------------------------------------------
//   updateControllers
//---------------------------------------------------------

void PluginI::updateControllers()
{
  if(!_track)
    return;

  for(unsigned long i = 0; i < controlPorts; ++i)
    _track->setPluginCtrlVal(genACnum(_id, i), controls[i].val);  // TODO A faster bulk message
}

//---------------------------------------------------------
//   setChannel
//---------------------------------------------------------

void PluginI::setChannels(int c)
{
      channel = c;

      unsigned long ins = _plugin->inports();
      unsigned long outs = _plugin->outports();
      int ni = 1;
      if(outs)
        ni = c / outs;
      else
      if(ins)
        ni = c / ins;

      if(ni < 1)
        ni = 1;

      if (ni == instances)
            return;

      LADSPA_Handle* handles = new LADSPA_Handle[ni];

      if(ni > instances)
      {
        for(int i = 0; i < ni; ++i)
        {
          if(i < instances)
            // Transfer existing handle from old array to new array.
            handles[i] = handle[i];
          else
          {
            // Create a new plugin instance with handle.
            handles[i] = _plugin->instantiate(this);
            if(handles[i] == NULL)
            {
              fprintf(stderr, "PluginI::setChannels: cannot instantiate instance %d\n", i);

              // Although this is a messed up state not easy to get out of (final # of channels?), try not to assert().
              // Whoever uses these will have to check instance count or null handle, and try to gracefully fix it and allow a song save.
              for(int k = i; k < ni; ++k)
                handles[i] = NULL;
              ni = i + 1;
              //channel = ?;
              break;
            }
          }
        }
      }
      else
      {
        for(int i = 0; i < instances; ++i)
        {
          if(i < ni)
            // Transfer existing handle from old array to new array.
            handles[i] = handle[i];
          else
          {
            // Delete existing plugin instance.
            // Previously we deleted all the instances and rebuilt from scratch.
            // One side effect of this: Since a GUI is constructed only on the first handle,
            //  previously the native GUI would close when changing channels. Now it doesn't, which is good.
            _plugin->deactivate(handle[i]);
            _plugin->cleanup(handle[i]);
          }
        }
      }

      // Delete the old array, and set the new array.
      delete[] handle;
      handle = handles;

      // Connect ports:
      unsigned long curPort = 0;
      unsigned long curOutPort = 0;
      unsigned long ports = _plugin->ports();
      for(unsigned long k = 0; k < ports; ++k)
      {
        LADSPA_PortDescriptor pd = _plugin->portd(k);
        if(pd & LADSPA_PORT_CONTROL)
        {
          if(pd & LADSPA_PORT_INPUT)
          {
            for(int i = instances; i < ni; ++i)
              _plugin->connectPort(handle[i], k, &controls[curPort].val);
            controls[curPort].idx = k;
            ++curPort;
          }
          else if(pd & LADSPA_PORT_OUTPUT)
          {
            // Connect only the first instance's output controls.
            // We don't have a mechanism to display the other instances' outputs.
            _plugin->connectPort(handle[0], k, &controlsOut[curOutPort].val);
            // Connect the rest to dummy ports.
            for(int i = 1; i < ni; ++i)
              _plugin->connectPort(handle[i], k, &controlsOutDummy[curOutPort].val);
            controlsOut[curOutPort].idx = k;
            ++curOutPort;
          }
        }
      }

      // Activate new instances.
      for(int i = instances; i < ni; ++i)
        _plugin->activate(handle[i]);

      // Initialize control values.
      if(initControlValues)
      {
        for(unsigned long i = 0; i < controlPorts; ++i)
          controls[i].val = controls[i].tmpVal;
      }
      else
      {
        // get initial control values from plugin
        for(unsigned long i = 0; i < controlPorts; ++i)
          controls[i].tmpVal = controls[i].val;
      }

      // Finally, set the new number of instances.
      instances = ni;
}

//---------------------------------------------------------
//   setParam
//---------------------------------------------------------

void PluginI::setParam(unsigned long i, double val)
{
  addScheduledControlEvent(i, val, MusEGlobal::audio->curFrame());
}

//---------------------------------------------------------
//   defaultValue
//---------------------------------------------------------

double PluginI::defaultValue(unsigned long param) const
{
  if(param >= controlPorts)
    return 0.0;

  return _plugin->defaultValue(controls[param].idx);
}

void PluginI::setCustomData(const std::vector<QString>&
#if defined(LV2_SUPPORT) || defined(VST_NATIVE_SUPPORT)
  customParams
#endif
)
{
   if(_plugin == NULL)
      return;

#ifdef LV2_SUPPORT
   if(_plugin->isLV2Plugin()) //now only do it for lv2 plugs
   {

      LV2PluginWrapper *lv2Plug = static_cast<LV2PluginWrapper *>(_plugin);
      for(int i = 0; i < instances; ++i)
      {
         lv2Plug->setCustomData(handle [i], customParams);
      }
   }
#endif

#ifdef VST_NATIVE_SUPPORT
   if(_plugin->isVstNativePlugin()) //now only do it for lv2 plugs
   {

      VstNativePluginWrapper *vstPlug = static_cast<VstNativePluginWrapper *>(_plugin);
      for(int i = 0; i < instances; ++i)
      {
         vstPlug->setCustomData(handle [i], customParams);
      }
   }
#endif
}

LADSPA_Handle Plugin::instantiate(PluginI *)
{
  LADSPA_Handle h = plugin->instantiate(plugin, MusEGlobal::sampleRate);
  if(h == NULL)
  {
    fprintf(stderr, "Plugin::instantiate() Error: plugin:%s instantiate failed!\n", plugin->Label);
    return NULL;
  }

  return h;
}



//---------------------------------------------------------
//   initPluginInstance
//    return true on error
//---------------------------------------------------------

bool PluginI::initPluginInstance(Plugin* plug, int c)
      {
      channel = c;
      if(plug == 0)
      {
        printf("initPluginInstance: zero plugin\n");
        return true;
      }
      _plugin = plug;

      if (_plugin->incReferences(1)==0)
        return true;

      #ifdef OSC_SUPPORT
      _oscif.oscSetPluginI(this);
      #endif

      QString inst("-" + QString::number(_plugin->instNo()));
      _name  = _plugin->name() + inst;
      _label = _plugin->label() + inst;

      unsigned long ins = plug->inports();
      unsigned long outs = plug->outports();
      if(outs)
      {
        instances = channel / outs;
        if(instances < 1)
          instances = 1;
      }
      else
      if(ins)
      {
        instances = channel / ins;
        if(instances < 1)
          instances = 1;
      }
      else
        instances = 1;

      handle = new LADSPA_Handle[instances];
      for(int i = 0; i < instances; ++i)
        handle[i]=NULL;

      for(int i = 0; i < instances; ++i)
      {
        #ifdef PLUGIN_DEBUGIN
        fprintf(stderr, "PluginI::initPluginInstance instance:%d\n", i);
        #endif

        handle[i] = _plugin->instantiate(this);
        if(handle[i] == NULL)
          return true;
      }

      unsigned long ports = _plugin->ports();

      controlPorts = 0;
      controlOutPorts = 0;

      for(unsigned long k = 0; k < ports; ++k)
      {
        LADSPA_PortDescriptor pd = _plugin->portd(k);
        if(pd & LADSPA_PORT_CONTROL)
        {
          if(pd & LADSPA_PORT_INPUT)
            ++controlPorts;
          else
          if(pd & LADSPA_PORT_OUTPUT)
            ++controlOutPorts;
        }
      }

      controls    = new Port[controlPorts];
      controlsOut = new Port[controlOutPorts];
      controlsOutDummy = new Port[controlOutPorts];

      unsigned long curPort = 0;
      unsigned long curOutPort = 0;
      for(unsigned long k = 0; k < ports; ++k)
      {
        LADSPA_PortDescriptor pd = _plugin->portd(k);
        if(pd & LADSPA_PORT_CONTROL)
        {
          if(pd & LADSPA_PORT_INPUT)
          {
            controls[curPort].idx = k;
            double val = _plugin->defaultValue(k);
            controls[curPort].val    = val;
            controls[curPort].tmpVal = val;
            controls[curPort].enCtrl  = true;
            for(int i = 0; i < instances; ++i)
              _plugin->connectPort(handle[i], k, &controls[curPort].val);
            ++curPort;
          }
          else
          if(pd & LADSPA_PORT_OUTPUT)
          {
            const char* pname = _plugin->portName(k);
            if(pname == QString("latency") || pname == QString("_latency"))
            {
              _hasLatencyOutPort = true;
              _latencyOutPort = curOutPort;
            }
            controlsOut[curOutPort].idx = k;
            controlsOut[curOutPort].val     = 0.0;
            controlsOut[curOutPort].tmpVal  = 0.0;
            controlsOut[curOutPort].enCtrl  = false;
            // Connect only the first instance's output controls.
            // We don't have a mechanism to display the other instances' outputs.
            _plugin->connectPort(handle[0], k, &controlsOut[curOutPort].val);
            // Connect the rest to dummy ports.
            for(int i = 1; i < instances; ++i)
              _plugin->connectPort(handle[i], k, &controlsOutDummy[curOutPort].val);
            ++curOutPort;
          }
        }
      }

#ifdef _WIN32
      _audioInSilenceBuf = (float *) _aligned_malloc(16, sizeof(float *) * MusEGlobal::segmentSize);
      if(_audioInSilenceBuf == NULL)
      {
         fprintf(stderr, "ERROR: PluginI::initPluginInstance: _audioInSilenceBuf _aligned_malloc returned error: NULL. Aborting!\n");
         abort();
      }
#else
      int rv = posix_memalign((void **)&_audioInSilenceBuf, 16, sizeof(float) * MusEGlobal::segmentSize);
      if(rv != 0)
      {
          fprintf(stderr, "ERROR: PluginI::initPluginInstance: _audioInSilenceBuf posix_memalign returned error:%d. Aborting!\n", rv);
          abort();
      }
#endif
      if(MusEGlobal::config.useDenormalBias)
      {
          for(unsigned q = 0; q < MusEGlobal::segmentSize; ++q)
          {
            _audioInSilenceBuf[q] = MusEGlobal::denormalBias;
          }
      }
      else
      {
          memset(_audioInSilenceBuf, 0, sizeof(float) * MusEGlobal::segmentSize);
      }
#ifdef _WIN32
      _audioOutDummyBuf = (float *) _aligned_malloc(16, sizeof(float *) * MusEGlobal::segmentSize);
      if(_audioOutDummyBuf == NULL)
      {
         fprintf(stderr, "ERROR: PluginI::initPluginInstance: _audioOutDummyBuf _aligned_malloc returned error: NULL. Aborting!\n");
         abort();
      }
#else
      rv = posix_memalign((void **)&_audioOutDummyBuf, 16, sizeof(float) * MusEGlobal::segmentSize);
      if(rv != 0)
      {
          fprintf(stderr, "ERROR: PluginI::initPluginInstance: _audioOutDummyBuf posix_memalign returned error:%d. Aborting!\n", rv);
          abort();
      }
#endif
      activate();
      return false;
      }

//---------------------------------------------------------
//   connect
//---------------------------------------------------------

void PluginI::connect(unsigned long ports, unsigned long offset, float** src, float** dst)
      {
      unsigned long port = 0;
      for (int i = 0; i < instances; ++i) {
            for (unsigned long k = 0; k < _plugin->ports(); ++k) {
                  if (isAudioIn(k)) {
                        if(port < ports)
                          _plugin->connectPort(handle[i], k, src[port] + offset);
                        else
                          // Connect to an input silence buffer.
                          _plugin->connectPort(handle[i], k, _audioInSilenceBuf + offset);
                        ++port;
                        }
                  }
            }
      port = 0;
      for (int i = 0; i < instances; ++i) {
            for (unsigned long k = 0; k < _plugin->ports(); ++k) {
                  if (isAudioOut(k)) {
                        if(port < ports)
                          _plugin->connectPort(handle[i], k, dst[port] + offset);
                        else
                          // Connect to a dummy buffer.
                          _plugin->connectPort(handle[i], k, _audioOutDummyBuf + offset);
                        ++port;
                        }
                  }
            }
      }

//---------------------------------------------------------
//   deactivate
//---------------------------------------------------------

void PluginI::deactivate()
      {
      for (int i = 0; i < instances; ++i) {
            _plugin->deactivate(handle[i]);
            _plugin->cleanup(handle[i]);
            }
      }

//---------------------------------------------------------
//   activate
//---------------------------------------------------------

void PluginI::activate()
      {
      for (int i = 0; i < instances; ++i)
            _plugin->activate(handle[i]);
      if (initControlValues) {
            for (unsigned long i = 0; i < controlPorts; ++i) {
                  controls[i].val = controls[i].tmpVal;
                  }
            }
      else {
            // get initial control values from plugin
            for (unsigned long i = 0; i < controlPorts; ++i) {
                  controls[i].tmpVal = controls[i].val;
                  }
            }
      }

//---------------------------------------------------------
//   latency
//---------------------------------------------------------

float PluginI::latency() const
{
  // Do not report any latency if the plugin is not on.
  if(!on())
    return 0.0;
  if(cquirks()._overrideReportedLatency)
    return cquirks()._latencyOverrideValue;
  if(!hasLatencyOutPort())
    return 0.0;
  return controlsOut[latencyOutPortIndex()].val;
}


//---------------------------------------------------------
//   setControl
//    set plugin instance controller value by name
//---------------------------------------------------------

bool PluginI::setControl(const QString& s, double val)
      {
      for (unsigned long i = 0; i < controlPorts; ++i) {
            if (_plugin->portName(controls[i].idx) == s) {
                  setParam(i, val);
                  return false;
                  }
            }
      printf("PluginI:setControl(%s, %f) controller not found\n",
         s.toLatin1().constData(), val);
      return true;
      }

//---------------------------------------------------------
//   saveConfiguration
//---------------------------------------------------------

void PluginI::writeConfiguration(int level, Xml& xml)
      {
      if(_plugin->uri().isEmpty())
      {
        xml.tag(level++, "plugin file=\"%s\" label=\"%s\" channel=\"%d\"",
           Xml::xmlString(_plugin->lib()).toLatin1().constData(),
           Xml::xmlString(_plugin->label()).toLatin1().constData(), channel);
      }
      else
      {
        xml.tag(level++, "plugin uri=\"%s\" label=\"%s\" channel=\"%d\"",
           Xml::xmlString(_plugin->uri()).toLatin1().constData(),
           Xml::xmlString(_plugin->label()).toLatin1().constData(), channel);
      }

#ifdef LV2_SUPPORT
      if(_plugin != NULL && _plugin->isLV2Plugin())//save lv2 plugin state custom data before controls
      {
         LV2PluginWrapper *lv2Plug = static_cast<LV2PluginWrapper *>(_plugin);
         //for multi-instance plugins write only first instance's state
         if(instances > 0)
         {
            lv2Plug->writeConfiguration(handle [0], level, xml);
         }
      }
#endif

#ifdef VST_NATIVE_SUPPORT
      if(_plugin != NULL && _plugin->isVstNativePlugin())//save vst plugin state custom data before controls
      {
         VstNativePluginWrapper *vstPlug = static_cast<VstNativePluginWrapper *>(_plugin);
         //for multi-instance plugins write only first instance's state
         if(instances > 0)
         {
            vstPlug->writeConfiguration(handle [0], level, xml);
         }
      }
#endif
      for (unsigned long i = 0; i < controlPorts; ++i) {
            unsigned long idx = controls[i].idx;
            QString s("control name=\"%1\" val=\"%2\" /");
            xml.tag(level, s.arg(Xml::xmlString(_plugin->portName(idx)).toLatin1().constData()).arg(double(controls[i].tmpVal)).toLatin1().constData());
            }
      if (_on == false)
            xml.intTag(level, "on", _on);

      _quirks.write(level, xml);

      if(guiVisible())
        xml.intTag(level, "gui", 1);
      int x, y, w, h;
      getGeometry(&x, &y, &w, &h);
      QRect r(x, y, w, h);
      xml.qrectTag(level, "geometry", r);
      
      if (nativeGuiVisible())
            xml.intTag(level, "nativegui", 1);
      getNativeGeometry(&x, &y, &w, &h);
      QRect nr(x, y, w, h);
      xml.qrectTag(level, "nativeGeometry", nr);

      xml.etag(--level, "plugin");
      }

//---------------------------------------------------------
//   loadControl
//---------------------------------------------------------

bool PluginI::loadControl(Xml& xml)
      {
      QString file;
      QString label;
      QString name("mops");
      double val = 0.0;

      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();

            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return true;
                  case Xml::TagStart:
                        xml.unknown("PluginI-Control");
                        break;
                  case Xml::Attribut:
                        if (tag == "name")
                              name = xml.s2();
                        else if (tag == "val")
                              val = xml.s2().toDouble();
                        break;
                  case Xml::TagEnd:
                        if (tag == "control") {
                              if(_plugin)
                              {
                                bool found = false;
                                for(unsigned long i = 0; i < controlPorts; ++i)
                                {
                                  if(_plugin->portName(controls[i].idx) == name)
                                  {
                                    controls[i].val = controls[i].tmpVal = val;
                                    found = true;
                                  }
                                }
                                if(!found)
                                {
                                  printf("PluginI:loadControl(%s, %f) controller not found\n",
                                    name.toLatin1().constData(), val);
                                  return false;
                                }
                                initControlValues = true;
                              }
                              return false;
                          }
                        return true;
                  default:
                        break;
                  }
            }
      return true;
      }

//---------------------------------------------------------
//   readConfiguration
//    return true on error
//---------------------------------------------------------

bool PluginI::readConfiguration(Xml& xml, bool readPreset)
      {
      QString file;
      QString label;
      QString uri;

      //custom params in xml song file , synth tag, that will be passed to new PluginI:setCustomData(Xml &) method
      //now only lv2host uses them, others simply ignore
      std::vector<QString> accumulatedCustomParams;

      if (!readPreset)
            channel = 1;

      for (;;) {
            Xml::Token token(xml.parse());
            const QString& tag(xml.s1());
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return true;
                  case Xml::TagStart:
                        if (!readPreset && _plugin == 0) {
                              _plugin = MusEGlobal::plugins.find(file, uri, label);

                              if (_plugin)
                              {
                                 if(initPluginInstance(_plugin, channel)) {
                                    _plugin = 0;
                                    xml.parse1();
                                    printf("Error initializing plugin instance (%s, %s, %s)\n",
                                       file.toLatin1().constData(),
                                       uri.toLatin1().constData(),
                                       label.toLatin1().constData());
                                    //break;      // Don't break - let it read any control tags.
                                    }
                                 }
                              }
                        if (tag == "control")
                              loadControl(xml);
                        else if (tag == "on") {
                              bool flag = xml.parseInt();
                              if (!readPreset)
                                    _on = flag;
                              }
                        else if (tag == "quirks") {
                              PluginQuirks q;
                              q.read(xml);
                              if (!readPreset)
                                _quirks = q;
                              }
                        else if (tag == "gui") {
                              const bool flag = xml.parseInt();
                              if (_plugin)
                                  showGui(flag);
                              }
                        else if (tag == "nativegui") {
                              // We can't tell OSC to show the native plugin gui
                              //  until the parent track is added to the lists.
                              // OSC needs to find the plugin in the track lists.
                              // Use this 'pending' flag so it gets done later.
                              _showNativeGuiPending = xml.parseInt();
                              }
                        else if (tag == "geometry") {
                              QRect r(readGeometry(xml, tag));
                              setGeometry(r.x(), r.y(), r.width(), r.height());
                              }
                        else if (tag == "nativeGeometry") {
                              QRect r(readGeometry(xml, tag));
                              setNativeGeometry(r.x(), r.y(), r.width(), r.height());
                              }
                        else if (tag == "customData") { //just place tag contents in accumulatedCustomParams
                              QString customData = xml.parse1();
                              if(!customData.isEmpty()){
                                 accumulatedCustomParams.push_back(customData);
                                 //now process custom data immidiatly
                                 //because it MUST be processed before plugin controls
                                 //writeConfiguration places custom data before plugin controls values
                                 setCustomData(accumulatedCustomParams);
                                 accumulatedCustomParams.clear();
                              }
                        }
                        else
                              xml.unknown("PluginI");
                        break;
                  case Xml::Attribut:
                        if (tag == "file") {
                              QString s = xml.s2();
                              if (readPreset) {
                                    if (s != plugin()->lib()) {
                                          printf("Error: Wrong preset type %s. Type must be a %s\n",
                                             s.toLatin1().constData(), plugin()->lib().toLatin1().constData());
                                          return true;
                                          }
                                    }
                              else {
                                    file = s;
                                    }
                              }
                        else if (tag == "uri") {
                              QString s = xml.s2();
                              if (readPreset) {
                                    if (s != plugin()->uri()) {
                                          printf("Error: Wrong preset uri %s. Uri must be a %s\n",
                                             s.toLatin1().constData(), plugin()->uri().toLatin1().constData());
                                          return true;
                                          }
                                    }
                              else {
                                    uri = s;
                                    }
                              }
                        else if (tag == "label") {
                              if (!readPreset)
                                    label = xml.s2();
                              }
                        else if (tag == "channel") {
                              if (!readPreset)
                                    channel = xml.s2().toInt();
                              }
                        break;
                  case Xml::TagEnd:
                        if (tag == "plugin") {
                              if (!readPreset && _plugin == 0) {
                                    _plugin = MusEGlobal::plugins.find(file, uri, label);
                                    if (_plugin == 0)
                                    {
                                      QMessageBox::warning(0,"Plugin not found!",
                                                  "Plugin: " + label + " not found, if the project is saved it will be removed from the project");
                                      fprintf(stderr, "Warning: - Plugin not found (%s, %s, %s)\n",
                                         file.toLatin1().constData(),
                                         uri.toLatin1().constData(),
                                         label.toLatin1().constData());
                                      return true;
                                    }

                                    if (initPluginInstance(_plugin, channel))
                                    {
                                      fprintf(stderr, "Error initializing plugin instance (%s, %s, %s)\n",
                                        file.toLatin1().constData(),
                                        uri.toLatin1().constData(),
                                        label.toLatin1().constData());
                                      return true;
                                    }
                              }
                              if (_gui)
                                    _gui->updateValues();
                              return false;
                              }
                        return true;
                  default:
                        break;
                  }
            }
      return true;
      }

//---------------------------------------------------------
//   showGui
//---------------------------------------------------------

void PluginI::showGui()
{
  if(_plugin)
    PluginIBase::showGui();
}

void PluginI::showGui(bool flag)
{
  if(_plugin)
    PluginIBase::showGui(flag);
}

//---------------------------------------------------------
//   showNativeGui
//---------------------------------------------------------

void PluginI::showNativeGui()
{

#ifdef LV2_SUPPORT
  if(plugin() && plugin()->isLV2Plugin())
  {
    if(((LV2PluginWrapper *)plugin())->nativeGuiVisible(this))
       ((LV2PluginWrapper *)plugin())->showNativeGui(this, false);
    else
       ((LV2PluginWrapper *)plugin())->showNativeGui(this, true);
    return;
  }
#endif

#ifdef VST_NATIVE_SUPPORT
  if(plugin() && plugin()->isVstNativePlugin())
  {
    if(((VstNativePluginWrapper *)plugin())->nativeGuiVisible(this))
       ((VstNativePluginWrapper *)plugin())->showNativeGui(this, false);
    else
       ((VstNativePluginWrapper *)plugin())->showNativeGui(this, true);
    return;
  }
#endif
  #ifdef OSC_SUPPORT
  if (_plugin)
  {
        if (_oscif.oscGuiVisible())
                _oscif.oscShowGui(false);
        else
                _oscif.oscShowGui(true);
  }
  #endif
  _showNativeGuiPending = false;
}

void PluginI::showNativeGui(
  bool
#if defined(LV2_SUPPORT) || defined(VST_NATIVE_SUPPORT) || defined(OSC_SUPPORT)
  flag
#endif
)
{
#ifdef LV2_SUPPORT
  if(plugin() && plugin()->isLV2Plugin())
  {
    ((LV2PluginWrapper *)plugin())->showNativeGui(this, flag);
    return;
  }
#endif

#ifdef VST_NATIVE_SUPPORT
  if(plugin() && plugin()->isVstNativePlugin())
  {
    ((VstNativePluginWrapper *)plugin())->showNativeGui(this, flag);
    return;
  }
#endif
  #ifdef OSC_SUPPORT
  if(_plugin)
  {
    _oscif.oscShowGui(flag);
  }
  #endif
  _showNativeGuiPending = false;
}

//---------------------------------------------------------
//   nativeGuiVisible
//---------------------------------------------------------

bool PluginI::nativeGuiVisible() const
{
#ifdef LV2_SUPPORT
    if(plugin() && plugin()->isLV2Plugin())
      return ((LV2PluginWrapper *)plugin())->nativeGuiVisible(this);
#endif
#ifdef VST_NATIVE_SUPPORT
    if(plugin() && plugin()->isVstNativePlugin())
      return ((VstNativePluginWrapper *)plugin())->nativeGuiVisible(this);
#endif
  #ifdef OSC_SUPPORT
  return _oscif.oscGuiVisible();
  #endif

  return false;
}

//---------------------------------------------------------
//   makeGui
//---------------------------------------------------------

void PluginIBase::makeGui()
      {
      _gui = new MusEGui::PluginGui(this);
      }

//---------------------------------------------------------
//   deleteGui
//---------------------------------------------------------
void PluginIBase::deleteGui()
{
  if(_gui)
  {
    delete _gui;
    _gui = 0;
  }
}

//---------------------------------------------------------
//   enableAllControllers
//---------------------------------------------------------

void PluginI::enableAllControllers(bool v)
{
  for(unsigned long i = 0; i < controlPorts; ++i)
    controls[i].enCtrl = v;
}

//---------------------------------------------------------
//   titlePrefix
//---------------------------------------------------------

QString PluginI::titlePrefix() const
{
  if (_track)
    return _track->name() + QString(": ");
  else return ":";
}

//---------------------------------------------------------
//   apply
//   If ports is 0, just process controllers only, not audio (do not 'run').
//---------------------------------------------------------

void PluginI::apply(unsigned pos, unsigned long n,
                    unsigned long ports, float** bufIn, float** bufOut, float latency_corr_offset)
{
  const unsigned long syncFrame = MusEGlobal::audio->curSyncFrame();
  unsigned long sample = 0;

  // FIXME Better support for PluginPowerOf2BlockSize, by quantizing the control period times.
  //       For now we treat it like fixed size.
  const bool usefixedrate = (requiredFeatures() & (PluginFixedBlockSize | PluginPowerOf2BlockSize | PluginCoarseBlockSize));

  // Note for dssi-vst this MUST equal audio period. It doesn't like broken-up runs (it stutters),
  //  even with fixed sizes. Could be a Wine + Jack thing, wanting a full Jack buffer's length.
  // For now, the fixed size is clamped to the audio buffer size.
  // TODO: We could later add slower processing over several cycles -
  //  so that users can select a small audio period but a larger control period.
  const unsigned long min_per = (usefixedrate || MusEGlobal::config.minControlProcessPeriod > n) ? n : MusEGlobal::config.minControlProcessPeriod;
  const unsigned long min_per_mask = min_per-1;   // min_per must be power of 2

  AutomationType at = AUTO_OFF;
  CtrlListList* cll = NULL;
  ciCtrlList icl_first;
  if(_track)
  {
    // Correction value is negative for correction.
    latency_corr_offset += _track->getLatencyInfo(false)._sourceCorrectionValue;

    at = _track->automationType();
    cll = _track->controller();
    if(_id != -1 && ports != 0)  // Don't bother if not 'running'.
      icl_first = cll->lower_bound(genACnum(_id, 0));
  }
  const bool no_auto = !MusEGlobal::automation || at == AUTO_OFF;
  const unsigned long in_ctrls = _plugin->controlInPorts();

  // Special for plugins: Deal with tmpVal. TODO: Get rid of tmpVal, maybe by using the FIFO...
  for(unsigned long k = 0; k < controlPorts; ++k)
    controls[k].val = controls[k].tmpVal;

  int cur_slice = 0;
  while(sample < n)
  {
    unsigned long nsamp = n - sample;
    const unsigned long slice_frame = pos + sample;

    // Process automation control values, while also determining the maximum acceptable
    //  size of this run. Further processing, from FIFOs for example, can lower the size
    //  from there, but this section determines where the next highest maximum frame
    //  absolutely needs to be for smooth playback of the controller value stream...
    //
    if(ports != 0)    // Don't bother if not 'running'.
    {
      ciCtrlList icl = icl_first;
      for(unsigned long k = 0; k < controlPorts; ++k)
      {
        CtrlList* cl = (cll && _id != -1 && icl != cll->end()) ? icl->second : NULL;
        CtrlInterpolate& ci = controls[k].interp;
        // Always refresh the interpolate struct at first, since things may have changed.
        // Or if the frame is outside of the interpolate range - and eStop is not true.  // FIXME TODO: Be sure these comparisons are correct.
        if(cur_slice == 0 || (!ci.eStop && MusEGlobal::audio->isPlaying() &&
            (slice_frame < (unsigned long)ci.sFrame || (ci.eFrameValid && slice_frame >= (unsigned long)ci.eFrame)) ) )
        {
          if(cl && _id != -1 && (unsigned long)cl->id() == genACnum(_id, k))
          {
            cl->getInterpolation(slice_frame, no_auto || !controls[k].enCtrl, &ci);
            if(icl != cll->end())
              ++icl;
          }
          else
          {
            // No matching controller, or end. Just copy the current value into the interpolator.
            // Keep the current icl iterator, because since they are sorted by frames,
            //  if the IDs didn't match it means we can just let k catch up with icl.
            ci.sFrame   = 0;
            ci.eFrame   = 0;
            ci.eFrameValid = false;
            ci.sVal     = controls[k].val;
            ci.eVal     = ci.sVal;
            ci.doInterp = false;
            ci.eStop    = false;
          }
        }
        else
        {
          if(ci.eStop && ci.eFrameValid && slice_frame >= (unsigned long)ci.eFrame)  // FIXME TODO: Get that comparison right.
          {
            // Clear the stop condition and set up the interp struct appropriately as an endless value.
            ci.sFrame   = 0; //ci->eFrame;
            ci.eFrame   = 0;
            ci.eFrameValid = false;
            ci.sVal     = ci.eVal;
            ci.doInterp = false;
            ci.eStop    = false;
          }
          if(cl && cll && icl != cll->end())
            ++icl;
        }

        if(!usefixedrate && MusEGlobal::audio->isPlaying())
        {
          unsigned long samps = nsamp;
          if(ci.eFrameValid)
            samps = (unsigned long)ci.eFrame - slice_frame;

          if(!ci.doInterp && samps > min_per)
          {
            samps &= ~min_per_mask;
            if((samps & min_per_mask) != 0)
              samps += min_per;
          }
          else
            samps = min_per;

          if(samps < nsamp)
            nsamp = samps;

        }

        if(ci.doInterp && cl)
          controls[k].val = cl->interpolate(MusEGlobal::audio->isPlaying() ? slice_frame : pos, ci);
        else
          controls[k].val = ci.sVal;

#ifdef LV2_SUPPORT
        if(_plugin->isLV2Plugin())
        {
           for(int i = 0; i < instances; ++i)
           {
              (reinterpret_cast<LV2PluginWrapper *>(_plugin))->setLastStateControls(handle [i], k, true, false, true, 0.0f);
           }
        }
#endif

        controls[k].tmpVal = controls[k].val;  // Special for plugins: Deal with tmpVal.

#ifdef PLUGIN_DEBUGIN_PROCESS
        printf("PluginI::apply k:%lu sample:%lu frame:%lu nextFrame:%d nsamp:%lu \n", k, sample, frame, ci.eFrame, nsamp);
#endif
      }
    }

#ifdef PLUGIN_DEBUGIN_PROCESS
    printf("PluginI::apply sample:%lu nsamp:%lu\n", sample, nsamp);
#endif

    //
    // Process all control ring buffer items valid for this time period...
    //
    bool found = false;
    unsigned long frame = 0;
    unsigned long index = 0;
    unsigned long evframe;
    // Get all control ring buffer items valid for this time period...
    while(!_controlFifo.isEmpty())
    {
      const ControlEvent& v = _controlFifo.peek();
      // The events happened in the last period or even before that. Shift into this period with + n. This will sync with audio.
      // If the events happened even before current frame - n, make sure they are counted immediately as zero-frame.
      evframe = (syncFrame > v.frame + n) ? 0 : v.frame - syncFrame + n;

      #ifdef PLUGIN_DEBUGIN_PROCESS
      fprintf(stderr, "PluginI::apply found:%d evframe:%lu frame:%lu  event frame:%lu idx:%lu val:%f unique:%d\n",
          found, evframe, frame, v.frame, v.idx, v.value, v.unique);
      #endif

      // Protection. Observed this condition. Why? Supposed to be linear timestamps.
      if(found && evframe < frame)
      {
        fprintf(stderr, 
          "PluginI::apply *** Error: Event out of order: evframe:%lu < frame:%lu idx:%lu val:%f unique:%d syncFrame:%lu nframes:%lu v.frame:%lu\n",
          evframe, frame, v.idx, v.value, v.unique, syncFrame, n, v.frame);

        // No choice but to ignore it.
        _controlFifo.remove();               // Done with the ring buffer's item. Remove it.
        continue;
      }

      if(evframe >= n                                                               // Next events are for a later period.
          || (!usefixedrate && !found && !v.unique && (evframe - sample >= nsamp))  // Next events are for a later run in this period. (Autom took prio.)
          || (found && !v.unique && (evframe - sample >= min_per))                  // Eat up events within minimum slice - they're too close.
          || (usefixedrate && found && v.unique && v.idx == index))                 // Special for dssi-vst: Fixed rate and must reply to all.
        break;
//       _controlFifo.remove();               // Done with the ring buffer's item. Remove it.

      if(v.idx >= in_ctrls) // Sanity check
      {
        _controlFifo.remove();               // Done with the ring buffer's item. Remove it.
        break;
      }

      found = true;
      frame = evframe;
      index = v.idx;

      if(ports == 0)                                              // Don't bother if not 'running'.
        controls[v.idx].val = controls[v.idx].tmpVal = v.value;   // Might as well at least update these.
      else
      {
        CtrlInterpolate* ci = &controls[v.idx].interp;
        // Tell it to stop the current ramp at this frame, when it does stop, set this value:
        ci->eFrame = frame;
        ci->eFrameValid = true;
        ci->eVal   = v.value;
        ci->eStop  = true;
      }

      // Need to update the automation value, otherwise it overwrites later with the last automation value.
      if(_track && _id != -1)
        _track->setPluginCtrlVal(genACnum(_id, v.idx), v.value);

#ifdef LV2_SUPPORT
      if(v.fromGui)
      {
         if(_plugin->isLV2Plugin())
         {
            for(int i = 0; i < instances; ++i)
            {
               (reinterpret_cast<LV2PluginWrapper *>(_plugin))->setLastStateControls(handle [i], v.idx, true, true, false, v.value);
            }
         }
      }
#endif

      _controlFifo.remove();               // Done with the ring buffer's item. Remove it.
    }

    if(found && !usefixedrate) // If a control FIFO item was found, takes priority over automation controller stream.
      nsamp = frame - sample;

    if(sample + nsamp > n)    // Safety check.
      nsamp = n - sample;

    // TODO: Don't allow zero-length runs. This could/should be checked in the control loop instead.
    // Note this means it is still possible to get stuck in the top loop (at least for a while).
    if(nsamp != 0)
    {
      if(ports != 0)     // Don't bother if not 'running'.
      {
        connect(ports, sample, bufIn, bufOut);

        for(int i = 0; i < instances; ++i)
          _plugin->apply(handle[i], nsamp, latency_corr_offset);
      }

      sample += nsamp;
    }

    ++cur_slice; // Slice is done. Moving on to any next slice now...
  }
}

//---------------------------------------------------------
//   oscConfigure
//---------------------------------------------------------

#ifdef OSC_SUPPORT
int Plugin::oscConfigure(
LADSPA_Handle
#if defined(DSSI_SUPPORT)
handle
#endif
,
const char*
#if defined(DSSI_SUPPORT) || defined(PLUGIN_DEBUGIN)
key
#endif
,
const char*
#if defined(DSSI_SUPPORT) || defined(PLUGIN_DEBUGIN)
value
#endif
)
      {
      #ifdef PLUGIN_DEBUGIN
      printf("Plugin::oscConfigure effect plugin label:%s key:%s value:%s\n", plugin->Label, key, value);
      #endif

      #ifdef DSSI_SUPPORT
      if(!dssi_descr || !dssi_descr->configure)
            return 0;

      if (!strncmp(key, DSSI_RESERVED_CONFIGURE_PREFIX,
         strlen(DSSI_RESERVED_CONFIGURE_PREFIX))) {
            fprintf(stderr, "Plugin::oscConfigure OSC: UI for plugin '%s' attempted to use reserved configure key \"%s\", ignoring\n",
               plugin->Label, key);

            return 0;
            }

      char* message = dssi_descr->configure(handle, key, value);
      if (message) {
            printf("Plugin::oscConfigure on configure '%s' '%s', plugin '%s' returned error '%s'\n",
               key, value, plugin->Label, message);

            free(message);
            }

      // also call back on UIs for plugins other than the one
      // that requested this:
      // if (n != instance->number && instances[n].uiTarget) {
      //      lo_send(instances[n].uiTarget,
      //      instances[n].ui_osc_configure_path, "ss", key, value);
      //      }

      #endif // DSSI_SUPPORT

      return 0;
}

//---------------------------------------------------------
//   oscConfigure
//---------------------------------------------------------

int PluginI::oscConfigure(
const char *
#if defined(DSSI_SUPPORT) || defined(PLUGIN_DEBUGIN)
key
#endif
,
const char *
#if defined(DSSI_SUPPORT) || defined(PLUGIN_DEBUGIN)
value
#endif
)
      {
      if(!_plugin)
        return 0;

      // "The host has the option to remember the set of (key,value)
      //   pairs associated with a particular instance, so that if it
      //   wants to restore the "same" instance on another occasion it can
      //   just call configure() on it for each of those pairs and so
      //   restore state without any input from a GUI.  Any real-world GUI
      //   host will probably want to do that."

      #ifdef PLUGIN_DEBUGIN
      printf("PluginI::oscConfigure effect plugin name:%s label:%s key:%s value:%s\n", _name.toLatin1().constData(), _label.toLatin1().constData(), key, value);
      #endif

      #ifdef DSSI_SUPPORT
      // FIXME: Don't think this is right, should probably do as example shows below.
      for(int i = 0; i < instances; ++i)
        _plugin->oscConfigure(handle[i], key, value);
      #endif // DSSI_SUPPORT

      return 0;
}

//---------------------------------------------------------
//   oscUpdate
//---------------------------------------------------------

int PluginI::oscUpdate()
{
      #ifdef DSSI_SUPPORT
      // Send project directory.
      _oscif.oscSendConfigure(DSSI_PROJECT_DIRECTORY_KEY, MusEGlobal::museProject.toLatin1().constData());  // MusEGlobal::song->projectPath()

      /* DELETETHIS 20
      // Send current string configuration parameters.
      StringParamMap& map = synti->stringParameters();
      int i = 0;
      for(ciStringParamMap r = map.begin(); r != map.end(); ++r)
      {
        _oscIF.oscSendConfigure(r->first.c_str(), r->second.c_str());
        // Avoid overloading the GUI if there are lots and lots of params.
        if((i+1) % 50 == 0)
          usleep(300000);
        ++i;
      }

      // Send current bank and program.
      unsigned long bank, prog;
      synti->currentProg(&prog, &bank, 0);
      _oscIF.oscSendProgram(prog, bank, true); // "true" means "force"
      */

      // FIXME: TESTING FLAM: I have to put a delay because flammer hasn't opened yet.
      // How to make sure gui is ready?
      usleep(300000);

      // Send current control values.
      for(unsigned long i = 0; i < controlPorts; ++i)
      {
        _oscif.oscSendControl(controls[i].idx, controls[i].val, true /*force*/);
        // Avoid overloading the GUI if there are lots and lots of ports.
        if((i+1) % 50 == 0)
          usleep(300000);
      }
      #endif // DSSI_SUPPORT

      return 0;
}

//---------------------------------------------------------
//   oscControl
//---------------------------------------------------------

int PluginI::oscControl(unsigned long port, float value)
{
  #ifdef PLUGIN_DEBUGIN
  printf("PluginI::oscControl received oscControl port:%lu val:%f\n", port, value);
  #endif

  if(port >= _plugin->rpIdx.size())
  {
    fprintf(stderr, "PluginI::oscControl: port number:%lu is out of range of index list size:%zd\n", port, _plugin->rpIdx.size());
    return 0;
  }

  // Convert from DSSI port number to control input port index.
  unsigned long cport = _plugin->rpIdx[port];

  if((int)cport == -1)
  {
    fprintf(stderr, "PluginI::oscControl: port number:%lu is not a control input\n", port);
    return 0;
  }

  // Record automation:
  // Take care of this immediately, because we don't want the silly delay associated with
  //  processing the fifo one-at-a-time in the apply().
  // NOTE: With some vsts we don't receive control events until the user RELEASES a control.
  // So the events all arrive at once when the user releases a control.
  // That makes this pretty useless... But what the heck...
  if(_track && _id != -1)
  {
    unsigned long id = genACnum(_id, cport);
    _track->recordAutomation(id, value);
  }

  // (From DSSI module).
  // p3.3.39 Set the DSSI control input port's value.
  // Observations: With a native DSSI synth like LessTrivialSynth, the native GUI's controls do not change the sound at all
  //  ie. they don't update the DSSI control port values themselves.
  // Hence in response to the call to this oscControl, sent by the native GUI, it is required to do that here.
///  controls[cport].val = value;
  // DSSI-VST synths however, unlike DSSI synths, DO change their OWN sound in response to their gui controls.
  // AND this function is called !
  // Despite the descrepancy we are STILL required to update the DSSI control port values here
  //  because dssi-vst is WAITING FOR A RESPONSE! (A CHANGE in the control port value).
  // It will output something like "...4 events expected..." and count that number down as 4 actual control port value CHANGES
  //  are done here in response. Normally it says "...0 events expected..." when MusE is the one doing the DSSI control changes.
  // TODO: May need FIFOs on each control(!) so that the control changes get sent one per process cycle!
  // Observed countdown not actually going to zero upon string of changes.
  // Try this ...

  // Schedules a timed control change:
  ControlEvent ce;
  ce.unique = _plugin->_isDssiVst;   // Special for messages from vst gui to host - requires processing every message.
  ce.fromGui = true;                 // It came from the plugin's own GUI.
  ce.idx = cport;
  ce.value = value;
  // Don't use timestamp(), because it's circular, which is making it impossible to deal
  // with 'modulo' events which slip in 'under the wire' before processing the ring buffers.
  ce.frame = MusEGlobal::audio->curFrame();
  if(_controlFifo.put(ce))
    fprintf(stderr, "PluginI::oscControl: fifo overflow: in control number:%lu\n", cport);

  enableController(cport, false); //TODO maybe re-enable the ctrl soon?

  /* DELETETHIS 12
  const DSSI_Descriptor* dssi = synth->dssi;
  const LADSPA_Descriptor* ld = dssi->LADSPA_Plugin;

  ciMidiCtl2LadspaPort ip = synth->port2MidiCtlMap.find(cport);
  if(ip != synth->port2MidiCtlMap.end())
  {
    // TODO: TODO: Update midi MusE's midi controller knobs, sliders, boxes etc with a call to the midi port's setHwCtrlState() etc.
    // But first we need a ladspa2MidiValue() function!  ...
    //
    //
    //float val = ladspa2MidiValue(ld, i, ?, ?);

  }
  */

      return 0;
      }

#endif // OSC_SUPPORT

} // namespace MusECore

namespace MusEGui {

// TODO: We need to use .qrc files to use icons in WhatsThis bubbles. See Qt
// Resource System in Qt documentation - ORCAN
//const char* presetOpenText = "<img source=\"fileopen\"> "
//      "Click this button to load a saved <em>preset</em>.";
const char* presetOpenText = "Click this button to load a saved <em>preset</em>.";
const char* presetSaveText = "Click this button to save curent parameter "
      "settings as a <em>preset</em>.  You will be prompted for a file name.";
const char* presetBypassText = "Click this button to bypass effect unit";

//---------------------------------------------------------
//   PluginGui
//---------------------------------------------------------

PluginGui::PluginGui(MusECore::PluginIBase* p)
   : QMainWindow(0)
      {
      gw     = 0;
      params = 0;
      paramsOut = 0;
      plugin = p;
      updateWindowTitle();
      
      QToolBar* tools = addToolBar(tr("File Buttons"));
      tools->setIconSize(QSize(MusEGlobal::config.iconSize, MusEGlobal::config.iconSize));

      QAction* fileOpen = new QAction(*fileopenSVGIcon, tr("Load Preset"), this);
//       connect(fileOpen, SIGNAL(triggered()), this, SLOT(load()));
      connect(fileOpen, &QAction::triggered, [this]() { load(); } );
      tools->addAction(fileOpen);

      QAction* fileSave = new QAction(*filesaveSVGIcon, tr("Save Preset"), this);
//       connect(fileSave, SIGNAL(triggered()), this, SLOT(save()));
      connect(fileSave, &QAction::triggered, [this]() { save(); } );
      tools->addAction(fileSave);

      QAction* whatsthis = QWhatsThis::createAction(this);
      whatsthis->setIcon(*whatsthisSVGIcon);
      tools->addAction(whatsthis);

      //onOff = new QAction(QIcon(*exitIconS), tr("bypass plugin"), this);
      onOff = new QAction(*muteSVGIcon, tr("Bypass plugin"), this);
      onOff->setCheckable(true);
      onOff->setChecked(!plugin->on());
      onOff->setEnabled(plugin->hasBypass());
      onOff->setToolTip(tr("Bypass plugin"));
      connect(onOff, &QAction::toggled, [this](bool v) { bypassToggled(v); } );
      tools->addAction(onOff);

      tools->addSeparator();
      tools->addWidget(new QLabel(tr("Quirks:")));

      fixedSpeedAct= new QAction(QIcon(*fixedSpeedSVGIcon), tr("Fixed speed"), this);
      fixedSpeedAct->setCheckable(true);
      fixedSpeedAct->setChecked(plugin->cquirks()._fixedSpeed);
      fixedSpeedAct->setEnabled(plugin->usesTransportSource());
      fixedSpeedAct->setToolTip(tr("Fixed speed"));
      connect(fixedSpeedAct, &QAction::toggled, [this](bool v) { fixedSpeedToggled(v); } );
      tools->addAction(fixedSpeedAct);

      transpGovLatencyAct = new QAction(QIcon(*transportAffectsLatencySVGIcon), tr("Transport affects audio latency"), this);
      transpGovLatencyAct->setCheckable(true);
      transpGovLatencyAct->setChecked(plugin->cquirks()._transportAffectsAudioLatency);
      transpGovLatencyAct->setEnabled(plugin->usesTransportSource());
      transpGovLatencyAct->setToolTip(tr("Transport affects audio latency"));
      connect(transpGovLatencyAct, &QAction::toggled, [this](bool v) { transportGovernsLatencyToggled(v); } );
      tools->addAction(transpGovLatencyAct);

      overrideLatencyAct= new QAction(QIcon(*overrideLatencySVGIcon), tr("Override reported audio latency"), this);
      overrideLatencyAct->setCheckable(true);
      overrideLatencyAct->setChecked(plugin->cquirks()._overrideReportedLatency);
      overrideLatencyAct->setToolTip(tr("Override reported audio latency"));
      connect(overrideLatencyAct, &QAction::toggled, [this](bool v) { overrideReportedLatencyToggled(v); } );
      tools->addAction(overrideLatencyAct);

      latencyOverrideEntry = new QSpinBox();
      latencyOverrideEntry->setRange(0, 8191);
      latencyOverrideEntry->setValue(plugin->cquirks()._latencyOverrideValue);
      latencyOverrideEntry->setEnabled(plugin->cquirks()._overrideReportedLatency);
      latencyOverrideEntry->setToolTip(tr("Reported audio latency override value"));
      // Special: Need qt helper overload for these lambdas.
      connect(latencyOverrideEntry,
        QOverload<int>::of(&QSpinBox::valueChanged), [=](int v) { latencyOverrideValueChanged(v); } );
      tools->addWidget(latencyOverrideEntry);

      fixScalingTooltip[0] = tr("Revert native UI HiDPI scaling: Follow global setting");
      fixScalingTooltip[1] = tr("Revert native UI HiDPI scaling: On");
      fixScalingTooltip[2] = tr("Revert native UI HiDPI scaling: Off");
      fixNativeUIScalingTB = new QToolButton(this);
      fixNativeUIScalingTB->setIcon(*noscaleSVGIcon[plugin->cquirks()._fixNativeUIScaling]);
      fixNativeUIScalingTB->setProperty("state", plugin->cquirks()._fixNativeUIScaling);
      fixNativeUIScalingTB->setToolTip(fixScalingTooltip[plugin->cquirks()._fixNativeUIScaling]);
      connect(fixNativeUIScalingTB, &QToolButton::clicked, [this]() { fixNativeUIScalingTBClicked(); } );
      tools->addWidget(fixNativeUIScalingTB);

      // TODO: We need to use .qrc files to use icons in WhatsThis bubbles. See Qt
      // Resource System in Qt documentation - ORCAN
      fileOpen->setWhatsThis(tr(presetOpenText));
      onOff->setWhatsThis(tr(presetBypassText));
      fileSave->setWhatsThis(tr(presetSaveText));

      QString id;
      id.setNum(plugin->pluginID());
      QString name(MusEGlobal::museGlobalShare + QString("/plugins/") + id + QString(".ui"));
      QFile uifile(name);
      if (uifile.exists()) {
            //
            // construct GUI from *.ui file
            //
            PluginLoader loader;
            QFile file(uifile.fileName());
            file.open(QFile::ReadOnly);
            mw = loader.load(&file, this);
            file.close();
            setCentralWidget(mw);

            QObjectList l = mw->children();
            QObject *obj;

            nobj = 0;
            QList<QObject*>::iterator it;
            for (it = l.begin(); it != l.end(); ++it) {
                  obj = *it;
                  QByteArray ba = obj->objectName().toLatin1();
                  const char* name = ba.constData();
                  if (*name !='P')
                        continue;
                  unsigned long parameter;
                  int rv = sscanf(name, "P%lu", &parameter);
                  if(rv != 1)
                    continue;
                  ++nobj;
                  }
            it = l.begin();
            gw   = new GuiWidgets[nobj];
            nobj = 0;

            // FIXME: There's no unsigned for gui params. We would need to limit nobj to MAXINT.
            // FIXME: Our MusEGui::Slider class uses doubles for values, giving some problems with float conversion.

            DoubleLabel* dl_obj;
            QCheckBox*   cb_obj;
            QComboBox*   combobox_obj;
            unsigned long int nn;

            for (it = l.begin(); it != l.end(); ++it) {
                  obj = *it;
                  QByteArray ba = obj->objectName().toLatin1();
                  const char* name = ba.constData();
                  if (*name !='P')
                        continue;
                  unsigned long parameter;
                  int rv = sscanf(name, "P%lu", &parameter);
                if(rv != 1)
                    continue;

                  // For some reason lambdas need this local copy (nn) of nobj otherwise they fail and crash.
                  nn = nobj;
                  
                  gw[nobj].widget  = (QWidget*)obj;
                  gw[nobj].param   = parameter;
                  gw[nobj].type    = -1;
                  gw[nobj].pressed = false;

                  if (strcmp(obj->metaObject()->className(), "MusEGui::Slider") == 0) {
                        gw[nobj].type = GuiWidgets::SLIDER;
                        Slider* s = static_cast<Slider*>(obj);
                        s->setId(nobj);
                        s->setCursorHoming(true);

                        LADSPA_PortRangeHint range = plugin->range(parameter);
                        double lower = 0.0;     // default values
                        double upper = 1.0;
                        double dlower = lower;
                        double dupper = upper;
                        double val   = plugin->param(parameter);
                        double dval  = val;
                        getPluginConvertedValues(range, lower, upper, dlower, dupper, dval);

                        // TODO
                        //s->setThumbLength(1);
                        //s->setRange(MusEGlobal::config.minSlider, volSliderMax, volSliderStep);
                        //s->setScaleMaxMinor(5);
                        //s->setScale(MusEGlobal::config.minSlider-0.1, 10.0, 6.0, false);
                        //s->setScale(dlower, dupper, 1.0, false);
                        //s->setSpecialText(QString('-') + QChar(0x221e)); // The infinity character.
                        //s->setScaleBackBone(false);
                        //s->setFillThumb(false);

                        QFont fnt;
                        fnt.setFamily("Sans");
                        fnt.setPixelSize(9);
                        //fnt.setStyleStrategy(QFont::PreferBitmap);
                        fnt.setStyleStrategy(QFont::NoAntialias);
                        fnt.setHintingPreference(QFont::PreferVerticalHinting);
                        s->setFont(fnt);
                        s->setStyleSheet(MusECore::font2StyleSheetFull(fnt));
                        s->setSizeHint(200, 8);

                        for(unsigned long i = 0; i < nobj; i++)
                        {
                          if(gw[i].type == GuiWidgets::DOUBLE_LABEL && gw[i].param == parameter)
                            ((DoubleLabel*)gw[i].widget)->setSlider(s);
                        }
                        connect(s, QOverload<double, int, int>::of(&Slider::valueChanged), [=]() { guiParamChanged(nn); } );
                        connect(s, &Slider::sliderPressed, [this](double v, int i) { guiSliderPressed(v, i); } );
                        connect(s, &Slider::sliderReleased, [this](double v, int i) { guiSliderReleased(v, i); } );
                        connect(s, &Slider::sliderRightClicked, [this](const QPoint &p, int i) { guiSliderRightClicked(p, i); } );
                        }
                  else if (strcmp(obj->metaObject()->className(), "MusEGui::DoubleLabel") == 0) {
                        gw[nobj].type = GuiWidgets::DOUBLE_LABEL;
                        dl_obj = static_cast<DoubleLabel*>(obj);
                        dl_obj->setId(nobj);
                        dl_obj->setAlignment(Qt::AlignCenter);
                        for(unsigned long i = 0; i < nobj; i++)
                        {
                          if(gw[i].type == GuiWidgets::SLIDER && gw[i].param == parameter)
                          {
                            dl_obj->setSlider((Slider*)gw[i].widget);
                            break;
                          }
                        }
                        connect((DoubleLabel*)obj, &DoubleLabel::valueChanged, [this, nn]() { guiParamChanged(nn); } );
                        }
                  else if (strcmp(obj->metaObject()->className(), "QCheckBox") == 0) {
                        gw[nobj].type = GuiWidgets::QCHECKBOX;
                        gw[nobj].widget->setContextMenuPolicy(Qt::CustomContextMenu);
                        cb_obj = static_cast<QCheckBox*>(obj);
                        connect(cb_obj, &QCheckBox::toggled, [this, nn]() { guiParamChanged(nn); } );
                        connect(cb_obj, &QCheckBox::pressed, [this, nn]() { guiParamPressed(nn); } );
                        connect(cb_obj, &QCheckBox::released, [this, nn]() { guiParamReleased(nn); } );
                        connect(cb_obj, &QCheckBox::customContextMenuRequested, [this, nn]() { guiContextMenuReq(nn); } );
                        }
                  else if (strcmp(obj->metaObject()->className(), "QComboBox") == 0) {
                        gw[nobj].type = GuiWidgets::QCOMBOBOX;
                        gw[nobj].widget->setContextMenuPolicy(Qt::CustomContextMenu);
                        combobox_obj = static_cast<QComboBox*>(obj);
                        connect(combobox_obj, QOverload<int>::of(&QComboBox::activated), [=]() { guiParamChanged(nn); } );
                        connect(combobox_obj, &QComboBox::customContextMenuRequested, [this, nn]() { guiContextMenuReq(nn); } );
                        }
                  else {
                        printf("unknown widget class %s\n", obj->metaObject()->className());
                        continue;
                        }
                  ++nobj;
                  }
              updateValues(); // otherwise the GUI won't have valid data
            }
      else {
            view = new QScrollArea;
            view->setWidgetResizable(true);
            setCentralWidget(view);

            mw = new QWidget;
            QGridLayout* grid = new QGridLayout;
            grid->setSpacing(2);

            mw->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));

            unsigned long n  = plugin->parameters();
            params = new GuiParam[n];

            QFontMetrics fm = fontMetrics();
            int h           = fm.height() + 4;

            Slider* sl_obj;
            CheckBox* cb_obj;

            for (unsigned long i = 0; i < n; ++i) {
                  QLabel* label = 0;
                  LADSPA_PortRangeHint range = plugin->range(i);
                  double lower = 0.0;     // default values
                  double upper = 1.0;
                  double dlower = lower;
                  double dupper = upper;
                  double val   = plugin->param(i);
                  double dval  = val;
                  params[i].pressed = false;
                  params[i].hint = range.HintDescriptor;

                  getPluginConvertedValues(range, lower, upper, dlower, dupper, dval);

                  if (LADSPA_IS_HINT_TOGGLED(range.HintDescriptor)) {
                        params[i].type = GuiParam::GUI_SWITCH;
                        CheckBox* cb = new CheckBox(mw, i, "param");
                        cb->setId(i);
                        cb->setText(QString(plugin->paramName(i)));
                        cb->setChecked(plugin->param(i) != 0.0);
                        cb->setFixedHeight(h);
                        params[i].actuator = cb;
                        }
                  else {
                        label           = new QLabel(QString(plugin->paramName(i)), 0);
                        params[i].type  = GuiParam::GUI_SLIDER;
                        params[i].label = new DoubleLabel(val, lower, upper, 0);
                        params[i].label->setFrame(true);
                        params[i].label->setAlignment(Qt::AlignCenter);
                        params[i].label->setPrecision(2);
                        params[i].label->setId(i);

                        // Let sliders all have different but unique colors
                        // Some prime number magic
                        uint j = i+1;
                        uint c1 = j * 211  % 256;
                        uint c2 = j * j * 137  % 256;
                        uint c3 = j * j * j * 43  % 256;
                        QColor color(c1, c2, c3);

                        Slider* s = new Slider(0, "param", Qt::Horizontal,
                           Slider::InsideHorizontal, 8, color, ScaleDraw::TextHighlightSplitAndShadow);

                        // TODO
                        //s->setThumbLength(1);
                        //s->setRange(MusEGlobal::config.minSlider, volSliderMax, volSliderStep);
                        //s->setScaleMaxMinor(5);
                        //s->setScale(MusEGlobal::config.minSlider-0.1, 10.0, 6.0, false);
                        //s->setScale(dlower, dupper, 1.0, false);
                        //s->setSpecialText(QString('-') + QChar(0x221e)); // The infinity character.
                        //s->setScaleBackBone(false);
                        //s->setFillThumb(false);

                        QFont fnt;
                        fnt.setFamily("Sans");
                        fnt.setPixelSize(9);
                        //fnt.setStyleStrategy(QFont::PreferBitmap);
                        fnt.setStyleStrategy(QFont::NoAntialias);
                        fnt.setHintingPreference(QFont::PreferVerticalHinting);
                        s->setFont(fnt);
                        s->setStyleSheet(MusECore::font2StyleSheetFull(fnt));

                        s->setCursorHoming(true);
                        s->setId(i);
                        s->setSizeHint(200, 8);
                        s->setRange(dlower, dupper);
                        if(LADSPA_IS_HINT_INTEGER(range.HintDescriptor))
                          s->setStep(1.0);
                        s->setValue(dval);
                        params[i].actuator = s;
                        params[i].label->setSlider(s);
                        }
                  params[i].actuator->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed));
                  if (params[i].type == GuiParam::GUI_SLIDER) {
                        label->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
                        params[i].label->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
                        grid->addWidget(label, i, 0);
                        grid->addWidget(params[i].label,    i, 1);
                        grid->addWidget(params[i].actuator, i, 2);
                        }
                  else if (params[i].type == GuiParam::GUI_SWITCH) {
                        grid->addWidget(params[i].actuator, i, 0, 1, 3);
                        }
                  if (params[i].type == GuiParam::GUI_SLIDER) {
                        sl_obj = static_cast<Slider*>(params[i].actuator);
                        connect(sl_obj, QOverload<double, int, int>::of(&Slider::valueChanged),
                                [=](double v, int id, int scroll_mode) { sliderChanged(v, id, scroll_mode); } );
                        connect(params[i].label, &DoubleLabel::valueChanged, [this](double v, int i) { labelChanged(v, i); } );
                        connect(sl_obj, &Slider::sliderPressed, [this](double v, int i) { ctrlPressed(v, i); } );
                        connect(sl_obj, &Slider::sliderReleased, [this](double v, int i) { ctrlReleased(v, i); } );
                        connect(sl_obj, &Slider::sliderRightClicked, [this](const QPoint &p, int i) { ctrlRightClicked(p, i); } );
                        }
                  else if (params[i].type == GuiParam::GUI_SWITCH){
                        cb_obj = (CheckBox*)params[i].actuator;
                        connect(cb_obj, &CheckBox::checkboxPressed, [this](int i) { switchPressed(i); } );
                        connect(cb_obj, &CheckBox::checkboxReleased, [this](int i) { switchReleased(i); } );
                        connect(cb_obj, &CheckBox::checkboxRightClicked, [this](const QPoint &p, int i) { ctrlRightClicked(p, i); } );
                        }
                  }


            int n2  = plugin->parametersOut();
            if (n2 > 0) {
              paramsOut = new GuiParam[n2];

              for (int i = 0; i < n2; ++i) {
                      QLabel* label = 0;
                      LADSPA_PortRangeHint range = plugin->rangeOut(i);
                      double lower = 0.0;     // default values
                      double upper = 32768.0; // Many latency outs have no hints so set this arbitrarily high
                      double dlower = lower;
                      double dupper = upper;
                      double val   = plugin->paramOut(i);
                      double dval  = val;
                      paramsOut[i].pressed = false;
                      paramsOut[i].hint = range.HintDescriptor;

                      getPluginConvertedValues(range, lower, upper, dlower, dupper, dval);
                      label           = new QLabel(QString(plugin->paramOutName(i)), 0);
                      paramsOut[i].type  = GuiParam::GUI_METER;
                      paramsOut[i].label = new DoubleLabel(val, lower, upper, 0);
                      paramsOut[i].label->setFrame(true);
                      paramsOut[i].label->setAlignment(Qt::AlignCenter);
                      paramsOut[i].label->setPrecision(2);
                      paramsOut[i].label->setId(i);

                      Meter::MeterType mType=Meter::LinMeter;
                      //if(LADSPA_IS_HINT_INTEGER(range.HintDescriptor))
                      if(LADSPA_IS_HINT_LOGARITHMIC(range.HintDescriptor))
                        mType=Meter::DBMeter;
                      Meter* m = new Meter(this, 
                                           mType, 
                                           Qt::Horizontal, 
                                           dlower, dupper,
                                           Meter::InsideHorizontal); //, ScaleDraw::TextHighlightNone);
                      m->setRefreshRate(MusEGlobal::config.guiRefresh);
                      m->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Maximum);
                      m->setVal(dval, dval, false);
                      m->setScaleBackBone(false);
                      m->setPrimaryColor(MusEGlobal::config.audioMeterPrimaryColor);

                      QFont fnt;
                      fnt.setFamily("Sans");
                      fnt.setPixelSize(9);
                      //fnt.setStyleStrategy(QFont::PreferBitmap);
                      fnt.setStyleStrategy(QFont::NoAntialias);
                      fnt.setHintingPreference(QFont::PreferVerticalHinting);
                      m->setFont(fnt);
                      m->setStyleSheet(MusECore::font2StyleSheetFull(fnt));

                      paramsOut[i].actuator = m;
                      label->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
                      paramsOut[i].label->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
                      grid->addWidget(label, n+i, 0);
                      grid->addWidget(paramsOut[i].label,    n+i, 1);
                      grid->addWidget(paramsOut[i].actuator, n+i, 2);
              }
            }


            grid->setColumnStretch(2, 10);
            mw->setLayout(grid);
            view->setWidget(mw);
            }
      connect(MusEGlobal::heartBeatTimer, SIGNAL(timeout()), SLOT(heartBeat()));
      }

//---------------------------------------------------------
//   PluginGui
//---------------------------------------------------------

PluginGui::~PluginGui()
      {
      if (gw)
            delete[] gw;
      if (params)
            delete[] params;
      if (paramsOut)
            delete[] paramsOut;
      }

void PluginGui::updateWindowTitle()
{
  if(plugin)
    setWindowTitle(plugin->titlePrefix() + plugin->name() +
      (plugin->uri().isEmpty() ? QString() : QString(" : ") + plugin->uri()));
}

void PluginGui::hideEvent(QHideEvent *e)
{
  if(plugin)
    plugin->saveGeometry(geometry().x(), geometry().y(), geometry().width(), geometry().height());
  
  e->ignore();
  QMainWindow::hideEvent(e);
}
      
void PluginGui::showEvent(QShowEvent *e)
{
  int x = 0, y = 0, w = 0, h = 0;
  if(plugin)
    plugin->savedGeometry(&x, &y, &w, &h);
  
#ifdef QT_SHOW_POS_BUG_WORKAROUND
  // Because of the bug, no matter what we must supply a position,
  //  even upon first showing...
  
  // Check sane size.
  if(w == 0)
    w = sizeHint().width();
  if(h == 0)
    h = sizeHint().height();

  // No size hint? Try minimum size.
  if(w == 0)
    w = minimumSize().width();
  if(h == 0)
    h = minimumSize().height();

  // Fallback.
  if(w == 0)
    w = 200;
  if(h == 0)
    h = 200;
  
  setGeometry(x, y, w, h);
  
#else    
  
  // If the saved geometry is valid, use it.
  // Otherwise this is probably the first time showing,
  //  so do not set a geometry - let Qt pick one 
  //  (using auto-placement and sizeHint).
  if(!(x == 0 && y == 0 && w == 0 && h == 0))
  {
    // Check sane size.
    if(w == 0)
      w = sizeHint().width();
    if(h == 0)
      h = sizeHint().height();
    
    // No size hint? Try minimum size.
    if(w == 0)
      w = minimumSize().width();
    if(h == 0)
      h = minimumSize().height();

    // Fallback.
    if(w == 0)
      w = 200;
    if(h == 0)
      h = 200;
    
    setGeometry(x, y, w, h);
  }
#endif
    
  // Convenience: If the window was minimized, restore it.
  if(isMinimized())
    setWindowState((windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
  
  e->ignore();
  QMainWindow::showEvent(e);
}
    
void PluginGui::getPluginConvertedValues(LADSPA_PortRangeHint range,
                          double &lower, double &upper, double &dlower, double &dupper, double &dval)
{
  if (LADSPA_IS_HINT_BOUNDED_BELOW(range.HintDescriptor)) {
        dlower = lower = range.LowerBound;
        }
  if (LADSPA_IS_HINT_BOUNDED_ABOVE(range.HintDescriptor)) {
        dupper = upper = range.UpperBound;
        }
  if (LADSPA_IS_HINT_SAMPLE_RATE(range.HintDescriptor)) {
        lower *= MusEGlobal::sampleRate;
        upper *= MusEGlobal::sampleRate;
        dlower = lower;
        dupper = upper;
        }
  if (LADSPA_IS_HINT_LOGARITHMIC(range.HintDescriptor)) {
        if (lower == 0.0)
              lower = 0.001;
        dlower = MusECore::fast_log10(lower)*20.0;
        dupper = MusECore::fast_log10(upper)*20.0;
        dval  = MusECore::fast_log10(dval) * 20.0;
        }

}

//---------------------------------------------------------
//   heartBeat
//---------------------------------------------------------

void PluginGui::heartBeat()
{
  updateControls();
}

//---------------------------------------------------------
//   ctrlPressed
//---------------------------------------------------------

void PluginGui::ctrlPressed(double /*val*/, int param)
{
      params[param].pressed = true;
      MusECore::AudioTrack* track = plugin->track();
      int id = plugin->id();
      if(id != -1)
      {
        id = MusECore::genACnum(id, param);
        if(params[param].type == GuiParam::GUI_SLIDER)
        {
          double val = ((Slider*)params[param].actuator)->value();
          if (LADSPA_IS_HINT_LOGARITHMIC(params[param].hint))
                val = muse_db2val(val);
          else if (LADSPA_IS_HINT_INTEGER(params[param].hint))
                val = rint(val);
          params[param].label->blockSignals(true);
          params[param].label->setValue(val);
          params[param].label->blockSignals(false);
          if(track)
          {
            track->startAutoRecord(id, val);
            track->setPluginCtrlVal(id, val);
          }
        }
        else if(params[param].type == GuiParam::GUI_SWITCH)
        {
          float val = (float)((CheckBox*)params[param].actuator)->isChecked();
          if(track)
          {
            track->startAutoRecord(id, val);
            track->setPluginCtrlVal(id, val);
          }
        }
      }
      plugin->enableController(param, false);
}

//---------------------------------------------------------
//   ctrlReleased
//---------------------------------------------------------

void PluginGui::ctrlReleased(double /*val*/, int param)
{
      MusECore::AutomationType at = MusECore::AUTO_OFF;
      MusECore::AudioTrack* track = plugin->track();
      if(track)
        at = track->automationType();

      int id = plugin->id();
      if(track && id != -1)
      {
        id = MusECore::genACnum(id, param);
        if(params[param].type == GuiParam::GUI_SLIDER)
        {
          double val = ((Slider*)params[param].actuator)->value();
          if (LADSPA_IS_HINT_LOGARITHMIC(params[param].hint))
                val = muse_db2val(val);
          else if (LADSPA_IS_HINT_INTEGER(params[param].hint))
                val = rint(val);
          track->stopAutoRecord(id, val);
        }
      }

      // Special for switch - don't enable controller until transport stopped.
      if ((at == MusECore::AUTO_OFF) ||
          (at == MusECore::AUTO_TOUCH && (params[param].type != GuiParam::GUI_SWITCH ||
                                !MusEGlobal::audio->isPlaying()) ) )
        plugin->enableController(param, true);

      params[param].pressed = false;
}

//---------------------------------------------------------
//   ctrlRightClicked
//---------------------------------------------------------

void PluginGui::ctrlRightClicked(const QPoint &p, int param)
{
  int id = plugin->id();
  if(id != -1)
    MusEGlobal::song->execAutomationCtlPopup(plugin->track(), p, MusECore::genACnum(id, param));
}

//---------------------------------------------------------
//   switchPressed
//---------------------------------------------------------

void PluginGui::switchPressed(int param)
{
      params[param].pressed = true;
      MusECore::AudioTrack* track = plugin->track();
      int id = plugin->id();
      if(id != -1)
      {
        id = MusECore::genACnum(id, param);
        if(params[param].type == GuiParam::GUI_SWITCH)
        {
          float val = (float)((CheckBox*)params[param].actuator)->isChecked();
          if(track)
          {
            track->startAutoRecord(id, val);
            track->setPluginCtrlVal(id, val);
          }
        }
      }
      plugin->enableController(param, false);
}

//---------------------------------------------------------
//   switchReleased
//---------------------------------------------------------

void PluginGui::switchReleased(int param)
{
      MusECore::AutomationType at = MusECore::AUTO_OFF;
      MusECore::AudioTrack* track = plugin->track();
      if(track)
        at = track->automationType();

      // Special for switch - don't enable controller until transport stopped.
      if ((at == MusECore::AUTO_OFF) ||
          (at == MusECore::AUTO_TOUCH && (params[param].type != GuiParam::GUI_SWITCH ||
                                !MusEGlobal::audio->isPlaying()) ) )
        plugin->enableController(param, true);

      params[param].pressed = false;
}

//---------------------------------------------------------
//   sliderChanged
//---------------------------------------------------------

void PluginGui::sliderChanged(double val, int param, int scrollMode)
{
      MusECore::AudioTrack* track = plugin->track();

      if (LADSPA_IS_HINT_LOGARITHMIC(params[param].hint))
            val = muse_db2val(val);
      else if (LADSPA_IS_HINT_INTEGER(params[param].hint))
            val = rint(val);

      params[param].label->blockSignals(true);
      params[param].label->setValue(val);
      params[param].label->blockSignals(false);
      int id = plugin->id();
      if(track && id != -1)
      {
        id = MusECore::genACnum(id, param);
        // Hack: Be sure to ignore in ScrDirect mode since we get both pressed AND changed signals.
        // ScrDirect mode is one-time only on press with modifier.
        if(scrollMode != SliderBase::ScrDirect)
          track->recordAutomation(id, val);
      }
      plugin->setParam(param, val);  // Schedules a timed control change.
      plugin->enableController(param, false);
}

//---------------------------------------------------------
//   labelChanged
//---------------------------------------------------------

void PluginGui::labelChanged(double val, int param)
{
      MusECore::AudioTrack* track = plugin->track();

      double dval = val;
      if (LADSPA_IS_HINT_LOGARITHMIC(params[param].hint))
            dval = MusECore::fast_log10(val) * 20.0;
      else if (LADSPA_IS_HINT_INTEGER(params[param].hint))
            dval = rint(val);
      params[param].actuator->blockSignals(true);
      ((Slider*)params[param].actuator)->setValue(dval);
      params[param].actuator->blockSignals(false);
      int id = plugin->id();
      if(track && id != -1)
      {
        id = MusECore::genACnum(id, param);
        track->startAutoRecord(id, val);
      }
      plugin->setParam(param, val);  // Schedules a timed control change.
      plugin->enableController(param, false);
}

//---------------------------------------------------------
//   load
//---------------------------------------------------------

void PluginGui::load()
      {
      QString s("presets/plugins/");
      s += plugin->pluginLabel();
      s += "/";

      QString fn = getOpenFileName(s, MusEGlobal::preset_file_pattern,
         this, tr("MusE: load preset"), 0);
      if (fn.isEmpty())
            return;
      bool popenFlag;
      FILE* f = fileOpen(this, fn, QString(".pre"), "r", popenFlag, true);
      if (f == 0)
            return;

      MusECore::Xml xml(f);
      int mode = 0;
      for (;;) {
            MusECore::Xml::Token token = xml.parse();
            QString tag = xml.s1();
            switch (token) {
                  case MusECore::Xml::Error:
                  case MusECore::Xml::End:
                        return;
                  case MusECore::Xml::TagStart:
                        if (mode == 0 && tag == "muse")
                              mode = 1;
                        else if (mode == 1 && tag == "plugin") {

                              if(plugin->readConfiguration(xml, true))
                              {
                                QMessageBox::critical(this, QString("MusE"),
                                  tr("Error reading preset. Might not be right type for this plugin"));
                                goto ende;
                              }

                              mode = 0;
                              }
                        else
                              xml.unknown("PluginGui");
                        break;
                  case MusECore::Xml::Attribut:
                        break;
                  case MusECore::Xml::TagEnd:
                        if (!mode && tag == "muse")
                        {
                              plugin->updateControllers();
                              goto ende;
                        }
                  default:
                        break;
                  }
            }
ende:
      if (popenFlag)
            pclose(f);
      else
            fclose(f);
      }

//---------------------------------------------------------
//   save
//---------------------------------------------------------

void PluginGui::save()
      {
      QString s("presets/plugins/");
      s += plugin->pluginLabel();
      s += "/";

      QString fn = getSaveFileName(s, MusEGlobal::preset_file_save_pattern, this,
        tr("MusE: save preset"));
      if (fn.isEmpty())
            return;
      bool popenFlag;
      FILE* f = fileOpen(this, fn, QString(".pre"), "w", popenFlag, false, true);
      if (f == 0)
            return;
      MusECore::Xml xml(f);
      xml.header();
      xml.tag(0, "muse version=\"1.0\"");
      plugin->writeConfiguration(1, xml);
      xml.tag(1, "/muse");

      if (popenFlag)
            pclose(f);
      else
            fclose(f);
      }

//---------------------------------------------------------
//   bypassToggled
//---------------------------------------------------------

void PluginGui::bypassToggled(bool val)
      {
      updateWindowTitle();
      plugin->setOn(!val);
      MusEGlobal::song->update(SC_ROUTE);
      }

void PluginGui::transportGovernsLatencyToggled(bool v)
{
  // TODO Make a safe audio-synced operation?
  plugin->quirks()._transportAffectsAudioLatency = v;
  MusEGlobal::song->update(SC_ROUTE);
}

void PluginGui::fixedSpeedToggled(bool v)
{
  // TODO Make a safe audio-synced operation?
  plugin->quirks()._fixedSpeed = v;
  MusEGlobal::song->update(SC_ROUTE);
}

void PluginGui::overrideReportedLatencyToggled(bool v)
{
  // TODO Make a safe audio-synced operation?
  plugin->quirks()._overrideReportedLatency = v;
  latencyOverrideEntry->setEnabled(v);
  MusEGlobal::song->update(SC_ROUTE);
}

void PluginGui::latencyOverrideValueChanged(int v)
{
  // TODO Make a safe audio-synced operation?
  plugin->quirks()._latencyOverrideValue = v;
  MusEGlobal::song->update(SC_ROUTE);
}

void PluginGui::fixNativeUIScalingTBClicked()
{
    int state = fixNativeUIScalingTB->property("state").toInt();
    state = (state == 2) ? 0 : state + 1;
    fixNativeUIScalingTB->setToolTip(fixScalingTooltip[state]);
    fixNativeUIScalingTB->setIcon(*noscaleSVGIcon[state]);
    fixNativeUIScalingTB->setProperty("state", state);
    plugin->quirks()._fixNativeUIScaling = (MusECore::PluginQuirks::NatUISCaling)state;
}

//---------------------------------------------------------
//   setOn
//---------------------------------------------------------

void PluginGui::setOn(bool val)
      {
      updateWindowTitle();
      onOff->blockSignals(true);
      onOff->setChecked(!val);
      onOff->blockSignals(false);
      }

//---------------------------------------------------------
//   updateValues
//---------------------------------------------------------

void PluginGui::updateValues()
      {
      if (params) {
            for (unsigned long i = 0; i < plugin->parameters(); ++i) {
                  GuiParam* gp = &params[i];
                  if (gp->type == GuiParam::GUI_SLIDER) {
                        double lv = plugin->param(i);
                        double sv = lv;
                        if (LADSPA_IS_HINT_LOGARITHMIC(params[i].hint))
                              sv = MusECore::fast_log10(lv) * 20.0;
                        else if (LADSPA_IS_HINT_INTEGER(params[i].hint))
                        {
                              sv = rint(lv);
                              lv = sv;
                        }
                        gp->label->blockSignals(true);
                        gp->actuator->blockSignals(true);
                        gp->label->setValue(lv);
                        ((Slider*)(gp->actuator))->setValue(sv);
                        gp->label->blockSignals(false);
                        gp->actuator->blockSignals(false);
                        }
                  else if (gp->type == GuiParam::GUI_SWITCH) {
                        gp->actuator->blockSignals(true);
                        ((CheckBox*)(gp->actuator))->setChecked(int(plugin->param(i)));
                        gp->actuator->blockSignals(false);
                        }
                  }
            }
      else if (gw) {
            for (unsigned long i = 0; i < nobj; ++i) {
                  QWidget* widget = gw[i].widget;
                  int type = gw[i].type;
                  unsigned long param = gw[i].param;
                  double val = plugin->param(param);
                  widget->blockSignals(true);
                  switch(type) {
                        case GuiWidgets::SLIDER:
                              ((Slider*)widget)->setValue(val);    // Note conversion to double
                              break;
                        case GuiWidgets::DOUBLE_LABEL:
                              ((DoubleLabel*)widget)->setValue(val);   // Note conversion to double
                              break;
                        case GuiWidgets::QCHECKBOX:
                              ((QCheckBox*)widget)->setChecked(int(val));
                              break;
                        case GuiWidgets::QCOMBOBOX:
                              ((QComboBox*)widget)->setCurrentIndex(int(val));
                              break;
                        }
                  widget->blockSignals(false);
                  }
            }
      }

//---------------------------------------------------------
//   updateControls
//---------------------------------------------------------

void PluginGui::updateControls()
      {
       if (!plugin->track() || plugin->id() == -1)
         return;

       // update outputs

       if (paramsOut) {
         for (unsigned long i = 0; i < plugin->parametersOut(); ++i) {
               GuiParam* gp = &paramsOut[i];
               if (gp->type == GuiParam::GUI_METER) {
                 double lv = plugin->paramOut(i);
                 double sv = lv;
                 if (LADSPA_IS_HINT_LOGARITHMIC(params[i].hint))
                       sv = MusECore::fast_log10(lv) * 20.0;
                 else if (LADSPA_IS_HINT_INTEGER(params[i].hint))
                 {
                       sv = rint(lv);
                       lv = sv;
                 }
                 ((Meter*)(gp->actuator))->setVal(sv, sv, false);
                 gp->label->setValue(lv);

               }
             }
       }


      if (params) {
            for (unsigned long i = 0; i < plugin->parameters(); ++i) {
                    GuiParam* gp = &params[i];
                    if(gp->pressed) // Inhibit the controller stream if control is currently pressed.
                      continue;
                    double v = plugin->track()->controller()->value(MusECore::genACnum(plugin->id(), i),
                                                                    MusEGlobal::audio->curFramePos(),
                                                                    !MusEGlobal::automation ||
                                                                    plugin->track()->automationType() == MusECore::AUTO_OFF ||
                                                                    !plugin->controllerEnabled(i));
                    if (gp->type == GuiParam::GUI_SLIDER) {
                            {
                              double sv = v;
                              if (LADSPA_IS_HINT_LOGARITHMIC(params[i].hint))
                                    sv = MusECore::fast_log10(v) * 20.0;
                              else
                              if (LADSPA_IS_HINT_INTEGER(params[i].hint))
                              {
                                    sv = rint(v);
                                    v = sv;
                              }
                              if(((Slider*)(gp->actuator))->value() != sv)
                              {
                                gp->label->blockSignals(true);
                                gp->actuator->blockSignals(true);
                                ((Slider*)(gp->actuator))->setValue(sv);
                                gp->label->setValue(v);
                                gp->actuator->blockSignals(false);
                                gp->label->blockSignals(false);
                              }
                            }
                          }
                    else if (gp->type == GuiParam::GUI_SWITCH) {
                            {
                              bool b = (int)v;
                              if(((CheckBox*)(gp->actuator))->isChecked() != b)
                              {
                                gp->actuator->blockSignals(true);
                                ((CheckBox*)(gp->actuator))->setChecked(b);
                                gp->actuator->blockSignals(false);
                              }
                            }
                          }
               }
            }
      else if (gw) {
            for (unsigned long i = 0; i < nobj; ++i) {
                  if(gw[i].pressed) // Inhibit the controller stream if control is currently pressed.
                    continue;
                  QWidget* widget = gw[i].widget;
                  int type = gw[i].type;
                  unsigned long param = gw[i].param;
                  double v = plugin->track()->controller()->value(MusECore::genACnum(plugin->id(), param),
                                                                  MusEGlobal::audio->curFramePos(),
                                                                  !MusEGlobal::automation ||
                                                                  plugin->track()->automationType() == MusECore::AUTO_OFF ||
                                                                  !plugin->controllerEnabled(param));
                  widget->blockSignals(true);
                  switch(type) {
                        case GuiWidgets::SLIDER:
                              {
                                if(((Slider*)widget)->value() != v)
                                  ((Slider*)widget)->setValue(v);
                              }
                              break;
                        case GuiWidgets::DOUBLE_LABEL:
                              {
                                if(((DoubleLabel*)widget)->value() != v)
                                  ((DoubleLabel*)widget)->setValue(v);
                              }
                              break;
                        case GuiWidgets::QCHECKBOX:
                              {
                                bool b = (bool)v;
                                if(((QCheckBox*)widget)->isChecked() != b)
                                  ((QCheckBox*)widget)->setChecked(b);
                              }
                              break;
                        case GuiWidgets::QCOMBOBOX:
                              {
                                int n = (int)v;
                                if(((QComboBox*)widget)->currentIndex() != n)
                                  ((QComboBox*)widget)->setCurrentIndex(n);
                              }
                              break;
                        }
                  widget->blockSignals(false);
                  }
            }
      }

//---------------------------------------------------------
//   guiParamChanged
//---------------------------------------------------------

void PluginGui::guiParamChanged(unsigned long int idx)
{
      QWidget* w = gw[idx].widget;
      unsigned long param  = gw[idx].param;
      int type   = gw[idx].type;

      MusECore::AudioTrack* track = plugin->track();

      double val = 0.0;
      bool ignoreRecAutomation = false;
      switch(type) {
            case GuiWidgets::SLIDER:
                  val = ((Slider*)w)->value();
                  // Hack: Be sure to ignore in ScrDirect mode since we get both pressed AND changed signals.
                  // ScrDirect mode is one-time only on press with modifier.
                  if(((Slider*)w)->scrollMode() == Slider::ScrDirect)
                    ignoreRecAutomation = true;
                  break;
            case GuiWidgets::DOUBLE_LABEL:
                  val = ((DoubleLabel*)w)->value();
                  break;
            case GuiWidgets::QCHECKBOX:
                  val = double(((QCheckBox*)w)->isChecked());
                  break;
            case GuiWidgets::QCOMBOBOX:
                  val = double(((QComboBox*)w)->currentIndex());
                  break;
            }

      for (unsigned long i = 0; i < nobj; ++i) {
            QWidget* widget = gw[i].widget;
            if (widget == w || param != gw[i].param)
                  continue;
            int type   = gw[i].type;
            widget->blockSignals(true);
            switch(type) {
                  case GuiWidgets::SLIDER:
                        ((Slider*)widget)->setValue(val);
                        break;
                  case GuiWidgets::DOUBLE_LABEL:
                        ((DoubleLabel*)widget)->setValue(val);
                        break;
                  case GuiWidgets::QCHECKBOX:
                        ((QCheckBox*)widget)->setChecked(int(val));
                        break;
                  case GuiWidgets::QCOMBOBOX:
                        ((QComboBox*)widget)->setCurrentIndex(int(val));
                        break;
                  }
            widget->blockSignals(false);
            }

      int id = plugin->id();
      if(track && id != -1)
      {
          id = MusECore::genACnum(id, param);
          switch(type)
          {
             case GuiWidgets::DOUBLE_LABEL:
             case GuiWidgets::QCHECKBOX:
               track->startAutoRecord(id, val);
             break;
             default:
               if(!ignoreRecAutomation)
                 track->recordAutomation(id, val);
             break;
          }
      }

      plugin->setParam(param, val);  // Schedules a timed control change.
      plugin->enableController(param, false);
}

//---------------------------------------------------------
//   guiParamPressed
//---------------------------------------------------------

void PluginGui::guiParamPressed(unsigned long int idx)
      {
      gw[idx].pressed = true;
      unsigned long param  = gw[idx].param;
      plugin->enableController(param, false);

      //MusECore::AudioTrack* track = plugin->track();
      //int id = plugin->id();
      //if(!track || id == -1)
      //  return;
      //id = MusECore::genACnum(id, param);
      // NOTE: For this to be of any use, the freeverb gui 2142.ui
      //  would have to be used, and changed to use CheckBox and ComboBox
      //  instead of QCheckBox and QComboBox, since both of those would
      //  need customization (Ex. QCheckBox doesn't check on click). RECHECK: Qt4 it does?
      /*
      switch(type) {
            case GuiWidgets::QCHECKBOX:
                    double val = (double)((CheckBox*)w)->isChecked();
                    track->startAutoRecord(id, val);
                  break;
            case GuiWidgets::QCOMBOBOX:
                    double val = (double)((ComboBox*)w)->currentIndex();
                    track->startAutoRecord(id, val);
                  break;
            }
      */
      }

//---------------------------------------------------------
//   guiParamReleased
//---------------------------------------------------------

void PluginGui::guiParamReleased(unsigned long int idx)
      {
      unsigned long param  = gw[idx].param;
      int type   = gw[idx].type;

      MusECore::AutomationType at = MusECore::AUTO_OFF;
      MusECore::AudioTrack* track = plugin->track();
      if(track)
        at = track->automationType();

      // Special for switch - don't enable controller until transport stopped.
      if ((at == MusECore::AUTO_OFF) ||
          (at == MusECore::AUTO_TOUCH && (type != GuiWidgets::QCHECKBOX ||
                                !MusEGlobal::audio->isPlaying()) ) )
        plugin->enableController(param, true);

      //int id = plugin->id();
      //if(!track || id == -1)
      //  return;
      //id = MusECore::genACnum(id, param);
      // NOTE: For this to be of any use, the freeverb gui 2142.ui
      //  would have to be used, and changed to use CheckBox and ComboBox
      //  instead of QCheckBox and QComboBox, since both of those would
      //  need customization (Ex. QCheckBox doesn't check on click).  // RECHECK Qt4 it does?
      /*
      switch(type) {
            case GuiWidgets::QCHECKBOX:
                    double val = (double)((CheckBox*)w)->isChecked();
                    track->stopAutoRecord(id, param);
                  break;
            case GuiWidgets::QCOMBOBOX:
                    double val = (double)((ComboBox*)w)->currentIndex();
                    track->stopAutoRecord(id, param);
                  break;
            }
      */

      gw[idx].pressed = false;
      }

//---------------------------------------------------------
//   guiSliderPressed
//---------------------------------------------------------

void PluginGui::guiSliderPressed(double /*val*/, unsigned long int idx)
{
      gw[idx].pressed = true;
      unsigned long param  = gw[idx].param;
      QWidget *w = gw[idx].widget;
      MusECore::AudioTrack* track = plugin->track();
      int id = plugin->id();
      if(track && id != -1)
      {
        id = MusECore::genACnum(id, param);
        double val = ((Slider*)w)->value();
        track->startAutoRecord(id, val);
        // Needed so that paging a slider updates a label or other buddy control.
        for (unsigned long i = 0; i < nobj; ++i) {
              QWidget* widget = gw[i].widget;
              if (widget == w || param != gw[i].param)
                    continue;
              int type   = gw[i].type;
              widget->blockSignals(true);
              switch(type) {
                    case GuiWidgets::SLIDER:
                          ((Slider*)widget)->setValue(val);
                          break;
                    case GuiWidgets::DOUBLE_LABEL:
                          ((DoubleLabel*)widget)->setValue(val);
                          break;
                    case GuiWidgets::QCHECKBOX:
                          ((QCheckBox*)widget)->setChecked(int(val));
                          break;
                    case GuiWidgets::QCOMBOBOX:
                          ((QComboBox*)widget)->setCurrentIndex(int(val));
                          break;
                    }
              widget->blockSignals(false);
              }
        track->setPluginCtrlVal(id, val);
      }
      plugin->enableController(param, false);
}

//---------------------------------------------------------
//   guiSliderReleased
//---------------------------------------------------------

void PluginGui::guiSliderReleased(double /*val*/, unsigned long int idx)
      {
      int param  = gw[idx].param;
      QWidget *w = gw[idx].widget;

      MusECore::AutomationType at = MusECore::AUTO_OFF;
      MusECore::AudioTrack* track = plugin->track();
      if(track)
        at = track->automationType();

      int id = plugin->id();

      if(track && id != -1)
      {
        id = MusECore::genACnum(id, param);

        double val = ((Slider*)w)->value();
        track->stopAutoRecord(id, val);
      }

      if (at == MusECore::AUTO_OFF ||
          at == MusECore::AUTO_TOUCH)
        plugin->enableController(param, true);

      gw[idx].pressed = false;
      }

//---------------------------------------------------------
//   guiSliderRightClicked
//---------------------------------------------------------

void PluginGui::guiSliderRightClicked(const QPoint &p, unsigned long int idx)
{
  int param  = gw[idx].param;
  int id = plugin->id();
  if(id != -1)
    MusEGlobal::song->execAutomationCtlPopup(plugin->track(), p, MusECore::genACnum(id, param));
}

//---------------------------------------------------------
//   guiContextMenuReq
//---------------------------------------------------------

void PluginGui::guiContextMenuReq(unsigned long int idx)
{
  guiSliderRightClicked(QCursor().pos(), idx);
}

//---------------------------------------------------------
//   PluginLoader
//---------------------------------------------------------
QWidget* PluginLoader::createWidget(const QString & className, QWidget * parent, const QString & name)
{
  if(className == QString("MusEGui::DoubleLabel"))
    return new DoubleLabel(parent, name.toLatin1().constData());
  if(className == QString("MusEGui::Slider"))
    return new Slider(parent, name.toLatin1().constData(), Qt::Horizontal, Slider::InsideHorizontal, 8, QColor(), ScaleDraw::TextHighlightSplitAndShadow);

  return QUiLoader::createWidget(className, parent, name);
}

} // namespace MusEGui


namespace MusEGlobal {

static void writePluginGroupNames(int level, MusECore::Xml& xml)
{
  xml.tag(level++, "group_names");

  for (QList<QString>::iterator it=plugin_group_names.begin(); it!=plugin_group_names.end(); it++)
    xml.strTag(level, "name", *it);

  xml.etag(--level, "group_names");
}

static void writePluginGroupMap(int level, MusECore::Xml& xml)
{
  using MusECore::PluginGroups;

  xml.tag(level++, "group_map");

  for (PluginGroups::iterator it=plugin_groups.begin(); it!=plugin_groups.end(); it++)
        if (!it.value().empty())
        {
            xml.tag(level++, "entry");

            xml.strTag(level, "lib", it.key().first);
            xml.strTag(level, "label", it.key().second);

            for (QSet<int>::iterator it2=it.value().begin(); it2!=it.value().end(); it2++)
                xml.intTag(level, "group", *it2);

            xml.etag(--level, "entry");
        }

  xml.etag(--level, "group_map");
}

void writePluginGroupConfiguration(int level, MusECore::Xml& xml)
{
  xml.tag(level++, "plugin_groups");

  writePluginGroupNames(level, xml);
  writePluginGroupMap(level, xml);

  xml.etag(--level, "plugin_groups");
}

static void readPluginGroupNames(MusECore::Xml& xml)
{
    plugin_group_names.clear();

    for (;;)
    {
        MusECore::Xml::Token token = xml.parse();
        if (token == MusECore::Xml::Error || token == MusECore::Xml::End)
            break;

        const QString& tag = xml.s1();
        switch (token)
        {
            case MusECore::Xml::TagStart:
                if (tag=="name")
                    plugin_group_names.append(xml.parse1());
                else
                    xml.unknown("readPluginGroupNames");
                break;

            case MusECore::Xml::TagEnd:
                if (tag == "group_names")
                    return;

            default:
                break;
        }
    }
}

static void readPluginGroupMap(MusECore::Xml& xml)
{
    plugin_groups.clear();

    for (;;)
    {
        MusECore::Xml::Token token = xml.parse();
        if (token == MusECore::Xml::Error || token == MusECore::Xml::End)
            break;

        const QString& tag = xml.s1();
        switch (token)
        {
            case MusECore::Xml::TagStart:
                if (tag=="entry")
                {
                    QString lib;
                    QString label;
                    QSet<int> groups;
                    bool read_lib=false, read_label=false;

                    for (;;)
                    {
                        MusECore::Xml::Token token = xml.parse();
                        if (token == MusECore::Xml::Error || token == MusECore::Xml::End)
                            break;

                        const QString& tag = xml.s1();
                        switch (token)
                        {
                            case MusECore::Xml::TagStart:
                                if (tag=="lib")
                                {
                                    lib=xml.parse1();
                                    read_lib=true;
                                }
                                else if (tag=="label")
                                {
                                    label=xml.parse1();
                                    read_label=true;
                                }
                                else if (tag=="group")
                                    groups.insert(xml.parseInt());
                                else
                                    xml.unknown("readPluginGroupMap");
                                break;

                            case MusECore::Xml::TagEnd:
                                if (tag == "entry")
                                    goto done_reading_entry;

                            default:
                                break;
                        }
                    }

done_reading_entry:

                    if (read_lib && read_label)
                        plugin_groups.get(lib,label)=groups;
                    else
                        fprintf(stderr,"ERROR: plugin group map entry without lib or label!\n");
                }
                else
                    xml.unknown("readPluginGroupMap");
                break;

            case MusECore::Xml::TagEnd:
                if (tag == "group_map")
                    return;

            default:
                break;
        }
    }
}

void readPluginGroupConfiguration(MusECore::Xml& xml)
{
    for (;;)
    {
        MusECore::Xml::Token token = xml.parse();
        if (token == MusECore::Xml::Error || token == MusECore::Xml::End)
            break;

        const QString& tag = xml.s1();
        switch (token)
        {
            case MusECore::Xml::TagStart:
                if (tag=="group_names")
                    readPluginGroupNames(xml);
                else if (tag=="group_map")
                    readPluginGroupMap(xml);
                else
                    xml.unknown("readPluginGroupConfiguration");
                break;

            case MusECore::Xml::TagEnd:
                if (tag == "plugin_groups")
                    return;

            default:
                break;
        }
    }
}

} // namespace MusEGlobal
