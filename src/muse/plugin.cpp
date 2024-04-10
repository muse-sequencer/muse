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
#include <QComboBox>
#include <QGroupBox>

#include "app.h"
#include "globals.h"
#include "gconfig.h"
#include "filedialog.h"
#include "slider.h"
#include "plugin.h"
#include "controlfifo.h"
#include "icons.h"
#include "song.h"
#include "fastlog.h"
#include "checkbox.h"
#include "comboboxpi.h"
#include "meter.h"
#include "utils.h"
#include "pluglist.h"
#include "pluginsettings.h"
#include "switch.h"
#include "hex_float.h"

#ifdef LV2_SUPPORT
#include "lv2host.h"
#endif

#ifdef VST_NATIVE_SUPPORT
#include "vst_native.h"
#endif

#include "audio.h"
#include "al/dsp.h"

// Forwards from header:
#include <QScrollArea>
#include <QHideEvent>
#include <QAction>
#include "xml.h"
#include "plugin_list.h"
#include "track.h"
#include "plugin_scan.h"
#include "doublelabel.h"

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
MusECore::MissingPluginList missingPlugins;
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

  // For now we do not allow interpolation of integer or enum controllers.
  // TODO: It would require custom line drawing and corresponding hit detection.
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



// REMOVE Tim. tmp. Added.
// NOTE: Some ladspa plugins put a web address as the label (lsp ladpsa plugins).
// Although legal, the slashes are not good for folder names, and the long label is not good for display.
// This function strips away any unusual characters and everything before them, like slashes etc.
static QString stripPluginLabel(const QString& s)
{
  // The config name is empty. This will happen if an old song without the new plugin 'name' tag is loaded.
  // No choice but to compose a name from the label, which must always exist.
  const int sz = s.size();
  int last_char = -1;
  int first_char = -1;
  int i = sz;
  while(i > 0)
  {
    --i;
    const QChar c = s.at(i);
    // TODO: Check for other unusual characters?
    if(c == '/')
    {
      if(last_char != -1)
        break;
      continue;
    }

    if(last_char == -1)
      last_char = i;

    first_char = i;
  }

  QString res;

  // Nothing found? (That's an error.)
  if(last_char == -1)
    res = QString("unknown");
  else
    res = s.mid(first_char, last_char - first_char + 1);

  return res;
}


//==============================================================
//   BEGIN PluginQuirks
//==============================================================

PluginQuirks::PluginQuirks() :
  _fixedSpeed(false),
  _transportAffectsAudioLatency(false),
  _overrideReportedLatency(false),
  _latencyOverrideValue(0),
  _fixNativeUIScaling(NatUISCaling::GLOBAL)
  { }


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

bool PluginQuirks::fixNativeUIScaling() const {
    if (qApp->devicePixelRatio() <= 1.0)
        return false; // no hidpi monitor in use, no need to fix anything

    return ((_fixNativeUIScaling == NatUISCaling::GLOBAL && MusEGlobal::config.noPluginScaling)
            || _fixNativeUIScaling == NatUISCaling::ON);
}

void PluginQuirks::setFixNativeUIScaling(NatUISCaling fixScaling) { _fixNativeUIScaling = fixScaling; };

PluginQuirks::NatUISCaling PluginQuirks::getFixNativeUIScaling() const { return _fixNativeUIScaling; };

//==============================================================
//   END PluginQuirks
//==============================================================


// REMOVE Tim. tmp. Added.
//---------------------------------------------------------
//   PluginConfiguration
//---------------------------------------------------------

PluginConfiguration::PluginConfiguration()
{
  /*_type = Synth::Type::METRO_SYNTH;*/
  _guiVisible = false;
  _nativeGuiVisible = false;
  _on = false;
  _active = false;
}

PluginConfiguration::ControlConfig::ControlConfig(int ctlnum, const QString& name, float val)
{
  _ctlnum = ctlnum;
  _name = name;
  _val = val;
}

MissingPluginStruct::MissingPluginStruct()
{
  _effectInstNo = _synthInstNo = 0;
}

MissingPluginList::iterator MissingPluginList::find(const QString& file, const QString& uri, const QString& label)
{
  const bool f_empty = file.isEmpty();
  const bool u_empty = uri.isEmpty();
  const bool l_empty = label.isEmpty();
  for (iterator i = begin(); i != end(); ++i)
  {
    if ( (!u_empty || f_empty || file == (*i)._file) &&
         (u_empty || uri == (*i)._uri) &&
         (!u_empty || l_empty || label == (*i)._label))
          return i;
  }
  return end();
}

int MissingPluginList::add(const QString& file, const QString& uri, const QString& label, bool isSynth)
{
  iterator i = find(file, uri, label);
  if(i == end())
  {
    MissingPluginStruct mps;
    mps._file = file;
    mps._uri = uri;
    mps._label = label;
    push_back(mps);
    return 0;
  }
  MissingPluginStruct& mps = *i;
  if(isSynth)
    return ++mps._synthInstNo;
  else
    return ++mps._effectInstNo;
}


//---------------------------------------------------------
//   Plugin
//---------------------------------------------------------

Plugin::Plugin()
{
  _isDssi = false;
  _isDssiSynth = false;
  _isDssiVst = false;
  _isLV2Plugin = false;
  _isLV2Synth = false;
  _isVstNativePlugin = false;
  _isVstNativeSynth = false;
  _requiredFeatures = PluginNoFeatures;
  _usesTimePosition = false;
  _freewheelPortIndex = 0;
  _latencyPortIndex = 0;
  _enableOrBypassPortIndex = 0;
  _pluginBypassType = PluginBypassTypeEmulatedEnableFunction;
  _pluginLatencyReportingType = PluginLatencyTypeNone;
  _pluginFreewheelType = PluginFreewheelTypeNone;

  #ifdef DSSI_SUPPORT
  dssi_descr = nullptr;
  #endif

  plugin = nullptr;
  ladspa = nullptr;
  _handle = nullptr;
  _references = 0;
  _instNo     = 0;
  _uniqueID   = 0;

  _portCount  = 0;
  _inports = 0;
  _outports = 0;
  _controlInPorts = 0;
  _controlOutPorts = 0;
}

Plugin::Plugin(const MusEPlugin::PluginScanInfoStruct& info)
 : Plugin()
{
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
  
  fi = QFileInfo(PLUGIN_GET_QSTRING(info.filePath()));
  _uri = PLUGIN_GET_QSTRING(info._uri);

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

  _freewheelPortIndex = info._freewheelPortIdx;
  _pluginFreewheelType = info._pluginFreewheelType;

  _latencyPortIndex = info._latencyPortIdx;
  _pluginLatencyReportingType = info._pluginLatencyReportingType;

    _enableOrBypassPortIndex = info._enableOrBypassPortIdx;
  _pluginBypassType = info._pluginBypassType;
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

    _handle = nullptr;
    ladspa = nullptr;
    plugin = nullptr;
    rpIdx.clear();

    #ifdef DSSI_SUPPORT
    dssi_descr = nullptr;
    #endif

    return 0;
  }

  if(_handle == nullptr)
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
        if(descr == nullptr)
          break;

        QString label(descr->LADSPA_Plugin->Label);
        if(label == _label)
        {
          _isDssi = true;
          ladspa = nullptr;
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
          if(descr == nullptr)
            break;

          QString label(descr->Label);
          if(label == _label)
          {
            _isDssi = false;
            ladspa = ladspadf;
            plugin = descr;

            #ifdef DSSI_SUPPORT
            dssi_descr = nullptr;
            #endif

            break;
          }
        }
      }
    }

    if(plugin != nullptr)
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

  if(plugin == nullptr)
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

PluginFeatures_t Plugin::requiredFeatures() const { return _requiredFeatures; }
QString Plugin::uri() const                          { return _uri; }
QString Plugin::label() const                        { return _label; }
QString Plugin::name() const                         { return _name; }
unsigned long Plugin::id() const                     { return _uniqueID; }
QString Plugin::maker() const                        { return _maker; }
QString Plugin::copyright() const                    { return _copyright; }
QString Plugin::lib(bool complete) const             { return complete ? fi.completeBaseName() : fi.baseName(); } // ddskrjo const
QString Plugin::dirPath(bool complete) const         { return complete ? fi.absolutePath() : fi.path(); }
QString Plugin::filePath() const                     { return fi.filePath(); }
QString Plugin::fileName() const                     { return fi.fileName(); }

int Plugin::references() const                       { return _references; }
int Plugin::instNo()                                 { return _instNo++;        }

void Plugin::activate(LADSPA_Handle handle) {
      if (plugin && plugin->activate)
            plugin->activate(handle);
      }
void Plugin::deactivate(LADSPA_Handle handle) {
      if (plugin && plugin->deactivate)
            plugin->deactivate(handle);
      }
void Plugin::cleanup(LADSPA_Handle handle) {
      if (plugin && plugin->cleanup)
            plugin->cleanup(handle);
      }
void Plugin::connectPort(LADSPA_Handle handle, unsigned long port, float* value) {
      if(plugin)
        plugin->connect_port(handle, port, value);
      }

unsigned long Plugin::ports() { return _portCount; }

LADSPA_PortDescriptor Plugin::portd(unsigned long k) const {
      return plugin ? plugin->PortDescriptors[k] : 0;
      }

// This version of range does not apply any changes, such as sample rate, to the bounds.
// The information returned is verbose. See the other range() which does apply changes.
LADSPA_PortRangeHint Plugin::range(unsigned long i) const {
      // FIXME:
      //return plugin ? plugin->PortRangeHints[i] : 0; DELETETHIS
      return plugin->PortRangeHints[i];
      }

const char* Plugin::portName(unsigned long i) const {
      return plugin ? plugin->PortNames[i] : 0;
      }

unsigned long Plugin::inports() const         { return _inports; }
unsigned long Plugin::outports() const        { return _outports; }
unsigned long Plugin::controlInPorts() const  { return _controlInPorts; }
unsigned long Plugin::controlOutPorts() const { return _controlOutPorts; }

const std::vector<unsigned long>* Plugin::getRpIdx() { return &rpIdx; }


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

const CtrlVal::CtrlEnumValues* Plugin::ctrlEnumValues ( unsigned long ) const
{
    return nullptr;
}

QString Plugin::unitSymbol ( unsigned long ) const { return QString(); }
int Plugin::valueUnit ( unsigned long ) const { return -1; }

bool Plugin::usesTimePosition() const            { return _usesTimePosition; }
unsigned long Plugin::freewheelPortIndex() const { return _freewheelPortIndex; }
unsigned long Plugin::latencyPortIndex() const   { return _latencyPortIndex; }
unsigned long Plugin::enableOrBypassPortIndex() const    { return _enableOrBypassPortIndex; }
PluginLatencyReportingType Plugin::pluginLatencyReportingType() const { return _pluginLatencyReportingType; }
PluginBypassType Plugin::pluginBypassType() const { return _pluginBypassType; }
PluginFreewheelType Plugin::pluginFreewheelType() const { return _pluginFreewheelType; }
float Plugin::getPluginLatency(void* /*handle*/) { return 0.0; }

void Plugin::apply(LADSPA_Handle handle, unsigned long n, float /*latency_corr*/)
{
#ifdef DSSI_SUPPORT
  if(isDssiPlugin() && dssi_descr)
  {
    if(dssi_descr->run_synth)
    {
      dssi_descr->run_synth(handle, n, nullptr, 0);
      return;
    }
  }
#endif

  if(plugin)
    plugin->run(handle, n);
}


//---------------------------------------------------------
//   PluginGroups
//---------------------------------------------------------

QSet<int>& PluginGroups::get(QString a, QString b) { return (*this)[(QPair<QString,QString>(a,b))]; }
QSet<int>& PluginGroups::get(const Plugin *p)
  { return (*this)[(QPair<QString,QString>(p->uri().isEmpty() ? p->lib() : p->uri(), p->label()))]; }

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

PluginList::PluginList() {}

void PluginList::add(const MusEPlugin::PluginScanInfoStruct& scan_info)
{ push_back(new Plugin(scan_info)); }

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
        buffer[i] = nullptr;
      initBuffers();

      for (int i = 0; i < MusECore::PipelineDepth; ++i)
            push_back(nullptr);
      }

//---------------------------------------------------------
//   Pipeline copy constructor
//---------------------------------------------------------

Pipeline::Pipeline(const Pipeline& p, AudioTrack* t)
   : std::vector<PluginI*>()
      {
      for(int i = 0; i < MusECore::MAX_CHANNELS; ++i)
        buffer[i] = nullptr;
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
        push_back(nullptr); // No plugin. Initialize with NULL.
      }
      }

//---------------------------------------------------------
//   ~Pipeline
//---------------------------------------------------------

Pipeline::~Pipeline()
      {
      //fprintf(stderr, "~Pipeline\n");
      removeAll();
      for (int i = 0; i < MusECore::MAX_CHANNELS; ++i)
          if(buffer[i])
          {
            ::free(buffer[i]);
          }
          //else
          //{
            //fprintf(stderr, "~Pipeline: buffer[%d] is NULL !\n", i);
          //}
      }

void Pipeline::initBuffers()
{
  for(int i = 0; i < MusECore::MAX_CHANNELS; ++i)
  {
    if(!buffer[i])
    {
#ifdef _WIN32
      buffer[i] = (float *) _aligned_malloc(16, sizeof(float *) * MusEGlobal::segmentSize);
      if(buffer[i] == nullptr)
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
//   isActive
//---------------------------------------------------------

bool Pipeline::isActive(int idx) const
{
      PluginI* p = (*this)[idx];
      if (p)
            return p->active();
      return false;
}

//---------------------------------------------------------
//   setActive
//---------------------------------------------------------

void Pipeline::setActive(int idx, bool flag)
{
      PluginI* p = (*this)[idx];
      if (p) {
            p->setActive(flag);
            if (p->gui())
                  p->gui()->setActive(flag);
            }
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
            return p->pluginLabel();
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

      return "<" + QObject::tr("FX slot") + " " + QString::number(idx + 1) + ">";
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

void Pipeline::move(int idx1, int idx2)
{
  PluginI* p1 = (*this)[idx1];
  (*this)[idx1] = (*this)[idx2];

  if((*this)[idx1])
    (*this)[idx1]->setID(idx1);

  (*this)[idx2] = p1;

  if(p1)
    p1->setID(idx2);
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
//   hasNativeGui
//---------------------------------------------------------

bool Pipeline::hasNativeGui(int idx) const
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

// REMOVE Tim. tmp. Changed.
// bool Pipeline::nativeGuiVisible(int idx)
//       {
//       PluginI* p = (*this)[idx];
//       if (p)
//       {
// #ifdef LV2_SUPPORT
//          if(p->plugin()->isLV2Plugin())
//             return ((LV2PluginWrapper *)p->plugin())->nativeGuiVisible(p);
// #endif
//
// #ifdef VST_NATIVE_SUPPORT
//          if(p->plugin()->isVstNativePlugin())
//             return ((VstNativePluginWrapper *)p->plugin())->nativeGuiVisible(p);
// #endif
//
//             return p->nativeGuiVisible();
//
//       }
//       return false;
//       }
bool Pipeline::nativeGuiVisible(int idx)
      {
      PluginI* p = (*this)[idx];
      if (p)
            return p->nativeGuiVisible();
      return false;
      }

void Pipeline::guiHeartBeat()
{
  for(int i = 0; i < MusECore::PipelineDepth; i++)
  {
    auto p = (*this)[i];
    if(p)
      p->guiHeartBeat();
  }
}

//---------------------------------------------------------
//   apply
//---------------------------------------------------------

void Pipeline::apply(unsigned pos, unsigned long ports, unsigned long nframes, bool wantActive, float** buffer1)
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
// REMOVE Tim. tmp. Changed.
        // if(!p)
        // If plugin is not available just continue.
        if(!p || !p->plugin())
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
// REMOVE Tim. tmp. Changed.
            // if(!p)
            // If plugin is not available just continue.
            if(!p || !p->plugin())
              continue;

            const float corr_offset = latency_corr_offsets[i];
            // If the plugin has a bypass control we let it run so it can do the pass-through,
            //  where bypass can be smoother (anti-zipper) than our simpler on/off scheme,
            //  and we manipulate the bypass control in the plugin's apply method.
            // We hope that by allowing the plugin's bypass to operate, it might consume
            //  less CPU time. Observations seem to counter that, it seems some processing
            //  still goes on even in bypass mode. But we have no choice but to let it run
            //  because that is the way it responds to UI commands, and various UI items
            //  depend on a full-length run being executed (flashing lights etc.).
            //
            // POLICY DECISION: If the track is off and there is no plugin bypass control,
            //  it's strange to do a full-length run just so it can respond to UI commands.
            // It's a waste of CPU time since the data is discarded.
            // Part of the idea of a track or plugin being off was to save CPU time.
            bool hasEnableOrBypass = false;
            switch(p->pluginBypassType())
            {
              case PluginBypassTypeEmulatedEnableFunction:
              case PluginBypassTypeEmulatedEnableController:
              break;

              case PluginBypassTypeEnableFunction:
              case PluginBypassTypeEnablePort:
              case PluginBypassTypeBypassFunction:
              case PluginBypassTypeBypassPort:
                hasEnableOrBypass = true;
              break;
            }

            // If the plugin has a REAL enable or bypass port or function, we ALWAYS do a full process
            //  so IT can do the pass-through instead of us, where it can be smoother than our simple
            //  on/off scheme (anti-zipper, crossfade etc).
            // If the plugin has no enable or bypass feature (just our own emulated), and it is bypassed,
            //  then we don't bother with a full process.
            // We manipulate the enable/bypass port/function in the plugin's apply method.
            if (wantActive && p->active() && (hasEnableOrBypass || p->on()))
            {
              if (!(p->requiredFeatures() & PluginNoInPlaceProcessing))
              {
                    if (swap)
                          p->apply(pos, nframes, ports, wantActive, buffer, buffer, corr_offset);
                    else
                          p->apply(pos, nframes, ports, wantActive, buffer1, buffer1, corr_offset);
              }
              else
              {
                    if (swap)
                          p->apply(pos, nframes, ports, wantActive, buffer, buffer1, corr_offset);
                    else
                          p->apply(pos, nframes, ports, wantActive, buffer1, buffer, corr_offset);
                    swap = !swap;
              }
            }
            else
            {
              // Apply. The plugin will see that either wantActive is false or the plugin is bypassed and has no
              //  enable/bypass feature, and will connect to dummy ports. So we don't need to pass any buffers here.
              p->apply(pos, nframes, ports, wantActive, nullptr, nullptr, corr_offset);
            }
      }
      if (wantActive && swap)
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
  _curActiveState = false;
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
// REMOVE Tim. tmp. Added.
//   _gui->updateWindowTitle();
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
      
// REMOVE Tim. tmp. Added.
const PluginQuirks& PluginIBase::cquirks() const { return _quirks; }
PluginQuirks& PluginIBase::quirks() { return _quirks; }
void PluginIBase::setQuirks(const PluginQuirks& q) { _quirks = q; }
void PluginIBase::setCustomData(const std::vector<QString> &) {/* Do nothing by default */}
MusEGui::PluginGui* PluginIBase::gui() const { return _gui; }
void PluginIBase::showNativeGui() { }
void PluginIBase::showNativeGui(bool) { }
// REMOVE Tim. tmp. Added.
void PluginIBase::closeNativeGui() { }
bool PluginIBase::nativeGuiVisible() const { return false; }
void PluginIBase::updateNativeGuiWindowTitle() { }

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

const CtrlVal::CtrlEnumValues* PluginIBase::ctrlEnumValues(unsigned long i) const {
    Q_UNUSED(i)
    return nullptr;
}

QString PluginIBase::portGroup(long unsigned int i) const {
    Q_UNUSED(i)
    return QString();
}

bool PluginIBase::ctrlIsTrigger(long unsigned int i) const {
    Q_UNUSED(i)
    return false;
}

bool PluginIBase::ctrlNotOnGui(long unsigned int i) const {
    Q_UNUSED(i)
    return false;
}

const CtrlVal::CtrlEnumValues* PluginIBase::ctrlOutEnumValues(unsigned long i) const {
    Q_UNUSED(i)
    return nullptr;
}

QString PluginIBase::portGroupOut(long unsigned int i) const {
    Q_UNUSED(i)
    return QString();
}

bool PluginIBase::ctrlOutIsTrigger(long unsigned int i) const {
    Q_UNUSED(i)
    return false;
}

bool PluginIBase::ctrlOutNotOnGui(long unsigned int i) const {
    Q_UNUSED(i)
    return false;
}

QString PluginIBase::unitSymbol(long unsigned int i) const {
    Q_UNUSED(i)
    return QString();
}

QString PluginIBase::unitSymbolOut(long unsigned int i) const {
    Q_UNUSED(i)
    return QString();
}

int PluginIBase::valueUnit(long unsigned int i) const {
    Q_UNUSED(i)
    return -1;
}

int PluginIBase::valueUnitOut(long unsigned int i) const {
    Q_UNUSED(i)
    return -1;
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

  // Note: Some ladspa plugins (lsp for ex.) put a full ip address like http://...
  // This is not good for a directory name. It ends up getting split into multiple folders: http / ... / ... /
  // FIXME: Should we extract the last piece of label text after the last '/' ?
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
      _active           = true;
      _on               = true;
// REMOVE Tim. tmp. Removed.
      // initControlValues = false;
      _showNativeGuiPending = false;
      }

PluginI::PluginI() : PluginIBase()
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
      _oscif.oscSetPluginI(nullptr);
      #endif

      if (_plugin) {
            deactivate();
            cleanup();
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

Plugin* PluginI::plugin() const       { return _plugin; }
bool PluginI::on() const              { return _on; }
void PluginI::setOn(bool val)         { _on = val; }
void PluginI::setTrack(AudioTrack* t) { _track = t; }
AudioTrack* PluginI::track() const    { return _track; }
int PluginI::id() const               { return _id; }
void PluginI::enableController(unsigned long i, bool v) { controls[i].enCtrl = v; }
bool PluginI::controllerEnabled(unsigned long i) const  { return controls[i].enCtrl; }

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
// REMOVE Tim. tmp. Added.
  // If plugin is not available just return.
  if(!plugin())
    return;

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
// REMOVE Tim. tmp. Removed.
      // channel = c;

// REMOVE Tim. tmp. Added.
      // If plugin is not available just return.
      if(!plugin())
        return;

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
            if(handles[i] == nullptr)
            {
              fprintf(stderr, "PluginI::setChannels: cannot instantiate instance %d\n", i);

              // Although this is a messed up state not easy to get out of (final # of channels?), try not to assert().
              // Whoever uses these will have to check instance count or null handle, and try to gracefully fix it and allow a song save.
              for(int k = i; k < ni; ++k)
                handles[i] = nullptr;
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

#ifdef DSSI_SUPPORT
      // Activate new instances.
      for(int i = instances; i < ni; ++i)
      {
        // Set current configuration values.
        if(isDssiPlugin() && _plugin->dssi_descr->configure)
        {
          char *rv = _plugin->dssi_descr->configure(handle[i], DSSI_PROJECT_DIRECTORY_KEY,
              MusEGlobal::museProject.toLatin1().constData()); //MusEGlobal::song->projectPath()

          if(rv)
          {
            fprintf(stderr, "MusE: Warning: plugin doesn't like project directory: \"%s\"\n", rv);
            free(rv);
          }
        }
      }
#endif

// REMOVE Tim. tmp. Removed.
      // // Initialize control values.
      // if(initControlValues)
      // {
      //   for(unsigned long i = 0; i < controlPorts; ++i)
      //     controls[i].val = controls[i].tmpVal;
      // }
      // else
      // {
      //   // get initial control values from plugin
      //   for(unsigned long i = 0; i < controlPorts; ++i)
      //     controls[i].tmpVal = controls[i].val;
      // }

      // Finally, set the new number of instances.
      instances = ni;
}

//---------------------------------------------------------
//   setParam
//---------------------------------------------------------

void PluginI::setParam(unsigned long i, double val)
{
// REMOVE Tim. tmp. Added.
  // If plugin is not available just return.
  if(!plugin())
    return;

  addScheduledControlEvent(i, val, MusEGlobal::audio->curFrame());
}

//---------------------------------------------------------
//   defaultValue
//---------------------------------------------------------

double PluginI::defaultValue(unsigned long param) const
{
// REMOVE Tim. tmp. Added.
  // If plugin is not available just return.
  if(!plugin())
    return 0.0;

  if(param >= controlPorts)
    return 0.0;

  return _plugin->defaultValue(controls[param].idx);
}

double PluginI::defaultOutValue(unsigned long param) const
{
// REMOVE Tim. tmp. Added.
  // If plugin is not available just return.
  if(!plugin())
    return 0.0;

  if(param >= controlOutPorts)
    return 0.0;

  return _plugin->defaultValue(controlsOut[param].idx);
}

//=========================================================
// If plugin is available, ask it for the values.
//=========================================================

const char* PluginI::paramName(unsigned long i) const
{ return _plugin ? _plugin->portName(controls[i].idx) : nullptr; }

const char* PluginI::paramOutName(unsigned long i) const
{ return _plugin ? _plugin->portName(controlsOut[i].idx) : nullptr; }

LADSPA_PortDescriptor PluginI::portd(unsigned long i) const
{ return _plugin ? _plugin->portd(controls[i].idx) : 0; }

LADSPA_PortDescriptor PluginI::portdOut(unsigned long i) const
{ return _plugin ? _plugin->portd(controlsOut[i].idx) : 0; }

void PluginI::range(unsigned long i, float* min, float* max) const
{ if(_plugin) _plugin->range(controls[i].idx, min, max); else {if(min) *min = 0.0f; if(max) *max = 1.0f;} }

void PluginI::rangeOut(unsigned long i, float* min, float* max) const
{ if(_plugin) _plugin->range(controlsOut[i].idx, min, max);  else {if(min) *min = 0.0f; if(max) *max = 1.0f;}}

bool PluginI::isAudioIn(unsigned long k) const
{ if(!_plugin) return false; return (_plugin->portd(k) & IS_AUDIO_IN) == IS_AUDIO_IN; }

bool PluginI::isAudioOut(unsigned long k) const
{ if(!_plugin) return false; return (_plugin->portd(k) & IS_AUDIO_OUT) == IS_AUDIO_OUT; }

LADSPA_PortRangeHint PluginI::range(unsigned long i) const
{ if(_plugin) return _plugin->range(controls[i].idx);
  return LADSPA_PortRangeHint{ 0, 0.0f, 0.0f }; }

LADSPA_PortRangeHint PluginI::rangeOut(unsigned long i) const
{ if(_plugin) return _plugin->range(controlsOut[i].idx);
  return LADSPA_PortRangeHint{ 0, 0.0f, 0.0f }; }


void PluginI::setCustomData(const std::vector<QString>&
#if defined(LV2_SUPPORT) || defined(VST_NATIVE_SUPPORT)
  customParams
#endif
)
{
   if(_plugin == nullptr)
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
  if(h == nullptr)
  {
    fprintf(stderr, "Plugin::instantiate() Error: plugin:%s instantiate failed!\n", plugin->Label);
    return nullptr;
  }

  return h;
}


// REMOVE Tim. tmp. Added.
//---------------------------------------------------------
//   createPluginI
//---------------------------------------------------------

// Static.
PluginI* PluginI::createPluginI(const PluginConfiguration &config, int channels, ConfigureOptions_t opts)
{
  Plugin *plugin = MusEGlobal::plugins.find(config._file, config._uri, config._pluginLabel);

  if(!plugin)
  {
    fprintf(stderr, "createPluginI: Warning: Plugin not found (file:%s, uri:%s, label:%s)\n",
        config._file.toLatin1().constData(),
        config._uri.toLatin1().constData(),
        config._pluginLabel.toLatin1().constData());
    // For persistence: Allow it to continue on. Initialization needs to do a few things.
  }

  PluginI *pi = new PluginI();

  // Returns true on error.
  if (pi->initPluginInstance(plugin, channels, config._name))
  {
    // For persistence: If the error was because there was no plugin available,
    //  return no error to allow it to continue loading.
    if(!plugin)
      return pi;

    fprintf(stderr, "createPluginI: Error initializing plugin instance (file:%s, uri:%s, label:%s)\n",
      config._file.toLatin1().constData(),
      config._uri.toLatin1().constData(),
      config._pluginLabel.toLatin1().constData());

    // The error was for some other reason.
    delete pi;
    // Return an error.
    return nullptr;
  }

  pi->configure(config, opts);

//   pi->setActive(config._active);
//   pi->setOn(config._on);
//   pi->setQuirks(config._quirks);
//
//   //now process custom data immediately
//   //because it MUST be processed before plugin controls
//   //writeConfiguration places custom data before plugin controls values
//   pi->setCustomData(config._accumulatedCustomParams);
//   // Done with initial custom data. Clear them.
//   //config._accumulatedCustomParams.clear();
//
//   unsigned long controlPorts = pi->parameters();
//
//   const unsigned long sz = config._initParams.size();
//   for(unsigned long i = 0; i < sz; ++i)
//   {
//     const PluginConfiguration::ControlConfig& cc = config._initParams[i];
//     // If a valid control number was stored, use it for faster lookup.
//     if(cc._ctlnum >= 0)
//     {
//       if((unsigned long)cc._ctlnum < controlPorts)
//       {
//         // TODO: Set vst params directly here, maybe even instead of controls array?
//         //       Maybe not. Process sets the vst params for us.
//
// // REMOVE Tim. tmp. Changed.
//         // controls[j].val = controls[j].tmpVal = cc._val;
//         //controls[cc._ctlnum].val = cc._val;
//         pi->putParam(cc._ctlnum, cc._val);
//       }
//       else
//       {
//         fprintf(stderr, "createPluginI(%d %s, %f) controller number out of range\n",
//           cc._ctlnum, cc._name.toLatin1().constData(), cc._val);
//         // Don't return. Let it continue.
//         //return false;
//       }
//     }
//     else
//     // A valid control number was not stored, use the name for a slow lookup.
//     {
//       bool found = false;
// //      pi->setControl();
//       for(unsigned long j = 0; j < controlPorts; ++j)
//       {
//         if(QString(pi->paramName(j)) == cc._name)
//         {
// // REMOVE Tim. tmp. Changed.
//           // controls[j].val = controls[j].tmpVal = cc._val;
//           //controls[j].val = cc._val;
//           pi->putParam(j, cc._val);
//           found = true;
//           break;
//         }
//       }
//       if(!found)
//       {
//         fprintf(stderr, "createPluginI(%s, %f) controller name not found\n",
//           cc._name.toLatin1().constData(), cc._val);
//         // Don't return. Let it continue.
//         //return false;
//       }
//     }
//
//   }
//
// // // REMOVE Tim. tmp. Removed.
// //                                 // initControlValues = true;
//
//   // Done with initial params. Clear them.
// //  config._initParams.clear();
//
//   pi->showGui(config._guiVisible);
//
//   // // We can't tell OSC to show the native plugin gui
//   // //  until the parent track is added to the lists.
//   // // OSC needs to find the plugin in the track lists.
//   // // Use this 'pending' flag so it gets done later.
//   // _showNativeGuiPending = config._nativeGuiVisible;
//
//   pi->showNativeGui(config._nativeGuiVisible);
//
//   if(pi->gui())
//     pi->gui()->updateValues();

  return pi;
}

//---------------------------------------------------------
//   initPluginInstance
//    return true on error
//---------------------------------------------------------

bool PluginI::initPluginInstance(Plugin* plug, int c, const QString& name)
      {
// REMOVE Tim. tmp. Removed.
      // channel = c;

// REMOVE Tim. tmp. Removed. Moved below.
      // if(plug == nullptr)
      // {
      //   printf("initPluginInstance: zero plugin\n");
      //   return true;
      // }

      _plugin = plug;

// REMOVE Tim. tmp. Removed. Moved below.
      // if (_plugin->incReferences(1)==0)
      //   return true;
      //
      // #ifdef OSC_SUPPORT
      // _oscif.oscSetPluginI(this);
      // #endif

// REMOVE Tim. tmp. Changed.
      // QString inst("-" + QString::number(_plugin->instNo()));
      // _name  = _plugin->name() + inst;
      // _label = _plugin->label() + inst;

      // NOTE: _plugin or _initConfig must be valid at this point.
      // We should never get here with both invalid.
      if(_plugin)
      {
        const QString inst("-" + QString::number(_plugin->instNo()));
        // If a name was given.
        if(!name.isEmpty())
          _name = name;
        else
          _name  = _plugin->name() + inst;
//        if(!label.isEmpty())
//          _label = label;
//        else
//          _label = _plugin->label() + inst;
      }
      else
      {
        const int instno = MusEGlobal::missingPlugins.add(_initConfig._file, _initConfig._uri, _initConfig._pluginLabel, false);
        const QString inst("-" + QString::number(instno));
        // If a name was given.
        if(!name.isEmpty())
        {
          _name = name;
        }
        else
        {
          if(_initConfig._name.isEmpty())
          {
            // The config name is empty. This will happen if an old song without the new plugin 'name' tag is loaded.
            // No choice but to compose a name from the label, which must always exist.
            // NOTE: Some ladspa plugins put a web address as the label (lsp ladpsa plugins).
            // Strip away any unusual characters and everything before them, like slashes etc.
            _name = stripPluginLabel(_initConfig._pluginLabel) + inst;
          }
          else
          {
            // Persistence: The config name already contains the suffix.
            _name = _initConfig._name;
          }
        }

//        if(!label.isEmpty())
//          _label = label;
//        else
//          _label = _initConfig._pluginLabel + inst;
      }

      if(!_plugin)
      {
        fprintf(stderr, "initPluginInstance: zero plugin\n");
        return true;
      }

      if (_plugin->incReferences(1)==0)
        return true;

      #ifdef OSC_SUPPORT
      _oscif.oscSetPluginI(this);
      #endif

      unsigned long ins = plug->inports();
      unsigned long outs = plug->outports();
      if(outs)
      {
// REMOVE Tim. tmp. Changed.
        // instances = channel / outs;
        instances = c / outs;
        if(instances < 1)
          instances = 1;
      }
      else
      if(ins)
      {
// REMOVE Tim. tmp. Changed.
        // instances = channel / ins;
        instances = c / ins;
        if(instances < 1)
          instances = 1;
      }
      else
        instances = 1;

      handle = new LADSPA_Handle[instances];
      for(int i = 0; i < instances; ++i)
        handle[i]=nullptr;

      for(int i = 0; i < instances; ++i)
      {
        #ifdef PLUGIN_DEBUGIN
        fprintf(stderr, "PluginI::initPluginInstance instance:%d\n", i);
        #endif

        handle[i] = _plugin->instantiate(this);
        if(handle[i] == nullptr)
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
// REMOVE Tim. tmp. Removed.
            // controls[curPort].tmpVal = val;
            controls[curPort].enCtrl  = true;
            for(int i = 0; i < instances; ++i)
              _plugin->connectPort(handle[i], k, &controls[curPort].val);
            ++curPort;
          }
          else
          if(pd & LADSPA_PORT_OUTPUT)
          {
            controlsOut[curOutPort].idx = k;
            controlsOut[curOutPort].val     = 0.0;
// REMOVE Tim. tmp. Removed.
            // controlsOut[curOutPort].tmpVal  = 0.0;
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
      if(_audioInSilenceBuf == nullptr)
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
      if(_audioOutDummyBuf == nullptr)
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

#ifdef DSSI_SUPPORT
        // Set current configuration values.
        if(isDssiPlugin() && _plugin->dssi_descr->configure)
        {
          for(int i = 0; i < instances; ++i)
          {
            char *rv = _plugin->dssi_descr->configure(handle[i], DSSI_PROJECT_DIRECTORY_KEY,
                MusEGlobal::museProject.toLatin1().constData()); //MusEGlobal::song->projectPath()

            if(rv)
            {
              fprintf(stderr, "MusE: Warning: plugin doesn't like project directory: \"%s\"\n", rv);
              free(rv);
            }
          }
        }
#endif

      return false;
      }

// REMOVE Tim. tmp. Added.
//---------------------------------------------------------
//   initPluginInstance
//    return true on error
//---------------------------------------------------------

bool PluginI::initPluginInstance(int channels, const QString& name)
{
  Plugin *plugin = MusEGlobal::plugins.find(
    _initConfig._file, _initConfig._uri, _initConfig._pluginLabel);

  // True on error. For persistence: Plugin can be null.
  if(initPluginInstance(plugin, channels, name))
    // Return true for error. For persistence: Don't clear the initial configuration members.
    return true;

//  // We are done with the initial configuration. Clear members.
//  _initConfig._accumulatedCustomParams.clear();
//  _initConfig._initParams.clear();

  // Return false for success.
  return false;
}

//---------------------------------------------------------
//   connect
//---------------------------------------------------------

void PluginI::connect(unsigned long ports, bool connectAllToDummyPorts, unsigned long offset, float** src, float** dst)
      {
// REMOVE Tim. tmp. Added.
      // If plugin is not available just return.
      if(!plugin())
        return;

      unsigned long port = 0;
      for (int i = 0; i < instances; ++i) {
            for (unsigned long k = 0; k < _plugin->ports(); ++k) {
                  if (isAudioIn(k)) {
                        if(!connectAllToDummyPorts && port < ports)
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
                        if(!connectAllToDummyPorts && port < ports)
                          _plugin->connectPort(handle[i], k, dst[port] + offset);
                        else
                          // Connect to a dummy buffer.
                          _plugin->connectPort(handle[i], k, _audioOutDummyBuf + offset);
                        ++port;
                        }
                  }
            }
      }

void PluginI::cleanup()
{
// REMOVE Tim. tmp. Added.
  // If plugin is not available just return.
  if(!plugin())
    return;

  for (int i = 0; i < instances; ++i) {
        _plugin->cleanup(handle[i]);
        }
}

//---------------------------------------------------------
//   deactivate
//---------------------------------------------------------

void PluginI::deactivate()
      {
      if(_curActiveState)
      {
        _curActiveState = false;
        for (int i = 0; i < instances; ++i) {
              _plugin->deactivate(handle[i]);
              }
      }
      }

//---------------------------------------------------------
//   activate
//---------------------------------------------------------

void PluginI::activate()
      {
      if(_curActiveState)
        return;

      for (int i = 0; i < instances; ++i)
            _plugin->activate(handle[i]);
// REMOVE Tim. tmp. Removed.
      // if (initControlValues) {
      //       for (unsigned long i = 0; i < controlPorts; ++i) {
      //             controls[i].val = controls[i].tmpVal;
      //             }
      //       }
      // else {
      //       // get initial control values from plugin
      //       for (unsigned long i = 0; i < controlPorts; ++i) {
      //             controls[i].tmpVal = controls[i].val;
      //             }
      //       }
      _curActiveState = true;
      }

//---------------------------------------------------------
//   latency
//---------------------------------------------------------

float PluginI::latency() const
{
  // Do not report any latency if the plugin is not active.
  if(!_curActiveState)
    return 0.0;

  switch(pluginBypassType())
  {
    // If the plugin has no enable/bypass feature and
    //  we are emulating it, if the plugin is bypassed
    //  do not report any latency since our emulated
    //  pass-through does not introduce any latency.
    case PluginBypassTypeEmulatedEnableFunction:
    case PluginBypassTypeEmulatedEnableController:
      if(!on())
        return 0.0;
    break;

    // Otherwise if the plugin has an enable/bypass feature,
    //  go ahead and allow the plugin to report its latency,
    //  regardless of whether it is bypassed or not.
    case PluginBypassTypeEnableFunction:
    case PluginBypassTypeEnablePort:
    case PluginBypassTypeBypassFunction:
    case PluginBypassTypeBypassPort:
    break;
  }

  if(cquirks()._overrideReportedLatency)
    return cquirks()._latencyOverrideValue;

  switch(pluginLatencyReportingType())
  {
    case PluginLatencyTypeNone:
    break;

    case PluginLatencyTypeFunction:
      // FIXME We can only deal with one instance's output for now. Just take the first instance's.
      if(handle[0])
        return _plugin->getPluginLatency(handle[0]);
    break;

    case PluginLatencyTypePort:
      if(latencyOutPortIndex() < controlOutPorts)
        return controlsOut[latencyOutPortIndex()].val;
    break;
  }
  return 0.0;
}

//=========================================================
// If plugin is available, ask it for the values.
// If plugin is not available, use the persistent settings.
//=========================================================

bool PluginI::usesTransportSource() const          { return _plugin ? _plugin->usesTimePosition() : false; };
unsigned long PluginI::latencyOutPortIndex() const { return _plugin ? _plugin->latencyPortIndex() : 0; }
unsigned long PluginI::freewheelPortIndex() const  { return _plugin ? _plugin->freewheelPortIndex() : 0; }
unsigned long PluginI::enableOrBypassPortIndex() const  { return _plugin ? _plugin->enableOrBypassPortIndex() : 0; }
PluginLatencyReportingType PluginI::pluginLatencyReportingType() const
{ return _plugin ? _plugin->pluginLatencyReportingType() : PluginLatencyTypeNone; }
PluginBypassType PluginI::pluginBypassType() const
{ return _plugin ? _plugin->pluginBypassType() : PluginBypassTypeEmulatedEnableFunction; }
PluginFreewheelType PluginI::pluginFreewheelType() const
{ return _plugin ? _plugin->pluginFreewheelType() : PluginFreewheelTypeNone; }


CtrlValueType PluginI::ctrlValueType(unsigned long i) const
{ return _plugin ? _plugin->ctrlValueType(controls[i].idx) : VAL_LINEAR; }

const CtrlVal::CtrlEnumValues* PluginI::ctrlEnumValues( unsigned long i) const
{ return _plugin ? _plugin->ctrlEnumValues(controls[i].idx) : nullptr; }

CtrlList::Mode PluginI::ctrlMode(unsigned long i) const
{ return _plugin ? _plugin->ctrlMode(controls[i].idx) : CtrlList::INTERPOLATE; }

CtrlValueType PluginI::ctrlOutValueType(unsigned long i) const
{ return _plugin ? _plugin->ctrlValueType(controlsOut[i].idx) : VAL_LINEAR; }

const CtrlVal::CtrlEnumValues* PluginI::ctrlOutEnumValues( unsigned long i) const
{ return _plugin ? _plugin->ctrlEnumValues(controlsOut[i].idx) : nullptr; }

CtrlList::Mode PluginI::ctrlOutMode(unsigned long i) const
{ return _plugin ? _plugin->ctrlMode(controlsOut[i].idx) : CtrlList::INTERPOLATE; }

// Returns a value unit string for displaying unit symbols.
QString PluginI::unitSymbol(long unsigned int i) const
{ return _plugin ? _plugin->unitSymbol(controls[i].idx) : QString(); }

QString PluginI::unitSymbolOut(long unsigned int i) const
{ return _plugin ? _plugin->unitSymbol(controlsOut[i].idx) : QString(); }

// Returns index into the global value units for displaying unit symbols.
// Can be -1 meaning no units.
int PluginI::valueUnit ( unsigned long i) const
{ return _plugin ? _plugin->valueUnit(controls[i].idx) : -1; }

int PluginI::valueUnitOut ( unsigned long i) const
{ return _plugin ? _plugin->valueUnit(controlsOut[i].idx) : -1; }


//---------------------------------------------------------
//   setControl
//    set plugin instance controller value by name
//---------------------------------------------------------

bool PluginI::setControl(const QString& s, double val)
      {
// REMOVE Tim. tmp. Added.
      // If plugin is not available just return.
      if(!plugin())
        return true;

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

// REMOVE Tim. tmp. Added.
//---------------------------------------------------------
//   getConfiguration
//---------------------------------------------------------

PluginConfiguration PluginI::getConfiguration() const
{
  // If the plugin is not available, use the persistent values.
  if(!_plugin)
    return _initConfig;

  // Plugin is available. Ask it for the values...
  PluginConfiguration conf;
  bool ctls_handled = false;

  //=============
  // Basic info
  //=============

  conf._file = _plugin->lib();
  conf._uri = _plugin->uri();
  conf._pluginLabel = _plugin->label();
  conf._name = name();
  conf._quirks = cquirks();
  conf._on = on();
  conf._active = active();
  conf._guiVisible = guiVisible();
  conf._nativeGuiVisible = nativeGuiVisible();

  int x, y, w, h;
  getGeometry(&x, &y, &w, &h);
  conf._geometry = QRect(x, y, w, h);

  getNativeGeometry(&x, &y, &w, &h);
  conf._nativeGeometry = QRect(x, y, w, h);

  //=============
  // Custom data
  //=============

#ifdef LV2_SUPPORT
  if(_plugin->isLV2Plugin())
  {
    //for multi-instance plugins write only first instance's state
    if(instances > 0)
    {
        LV2PluginWrapper *lv2Plug = static_cast<LV2PluginWrapper *>(_plugin);
        conf._accumulatedCustomParams.push_back(lv2Plug->getCustomConfiguration(handle[0]));
    }
  }
#endif

#ifdef VST_NATIVE_SUPPORT
  if(_plugin->isVstNativePlugin())
  {
    //for multi-instance plugins write only first instance's state
    if(instances > 0)
    {
        VstNativePluginWrapper *vstPlug = static_cast<VstNativePluginWrapper *>(_plugin);
        conf._accumulatedCustomParams.push_back(vstPlug->getCustomConfiguration(handle[0]));
    }
  }
#endif

  //==============================
  // Plugin controls or parameters
  //==============================

  // (Other places refer to this text. Be careful if removing or modifying it.)
  // Do not store controls or parameters if the plugin has custom configuration (vst chunks or lv2 etc).
  // The assumption is that control or parameter values would be included in the custom data.
  // This is in fact alluded to in the vst spec. It says chunks can store the whole state, but if chunks
  //  are not used then an alternative is to store the parameter values. So it does not directly state
  //  that these values ARE to be stored in the chunks, but it can be inferred although it is possible
  //  some idiot plugins might not do it. Well too bad, we're going with this.
  // One of the things this should fix is complaints that some plugins immediately show an indicator
  //  that they have been 'modified'. It's because we were sending all these parameters along with
  //  (after) the custom data.
  // TODO: Watch out for DSSI though, its concept of custom data might not include controls if I recall. (DSSI-VST ???)
  if(conf._accumulatedCustomParams.empty())
  {
    //==============================================================================================
    // Special for vst: If plugin is vst, ask the vst itself for the control values.
    // This is because at least one plugin (U-He uhbik) was seen NOT calling back the app
    //  pluginHostCallback() with audioMasterAutomate when patches were changed from the native gui.
    // All the gui controls change with patch changes, but no response - nothing at all happened.
    // So all of our local mirrored control arrays do not get updated when gui patch changes.
    // Thus our generic gui also is not updated.
    // Meanwhile other plugins DO call the callback with audioMasterAutomate when patches change
    //  from the native gui. Like TAL etc.
    //==============================================================================================
#ifdef VST_NATIVE_SUPPORT
    if(_plugin->isVstNativePlugin())
    {
      //for multi-instance plugins write only first instance's state
      if(instances > 0)
      {
        VstNativePluginWrapper_State *state = (VstNativePluginWrapper_State*)handle[0];

        // Is the getParameter() function available?
        if(state->plugin->getParameter)
        {
          for (unsigned long i = 0; i < controlPorts; ++i)
          {
            const unsigned long idx = controls[i].idx;
            conf._initParams.push_back(PluginConfiguration::ControlConfig(
              i,
              QString(_plugin->portName(idx)),
              state->plugin->getParameter(state->plugin, i)));
          }
          ctls_handled = true;
        }
      }
    }
    else
#endif // VST_NATIVE_SUPPORT

    {
      for (unsigned long i = 0; i < controlPorts; ++i)
      {
        const unsigned long idx = controls[i].idx;
        conf._initParams.push_back(PluginConfiguration::ControlConfig(
          i,
          QString(_plugin->portName(idx)),
          controls[i].val));
      }
      ctls_handled = true;
    }

    // If instance or params are not available, use the persistent values.
    if(!ctls_handled)
      conf._initParams = _initConfig._initParams;
  }

  return conf;
}

// REMOVE Tim. tmp. Added.
//---------------------------------------------------------
//   setInitialConfiguration
//---------------------------------------------------------

void PluginI::setInitialConfiguration(const PluginConfiguration& config)
{
  _initConfig = config;
}

// REMOVE Tim. tmp. Added.
//---------------------------------------------------------
//   configure
//---------------------------------------------------------

void PluginI::configure(const PluginConfiguration& config, ConfigureOptions_t opts)
{
  if(opts & ConfigActive)
    setActive(config._active);
  if(opts & ConfigOn)
    setOn(config._on);
  if(opts & ConfigQuirks)
    setQuirks(config._quirks);

  //now process custom data immediately
  //because it MUST be processed before plugin controls
  //writeConfiguration places custom data before plugin controls values
  if(opts & ConfigCustomData)
    setCustomData(config._accumulatedCustomParams);
  // Done with initial custom data. Clear them.
  //config._accumulatedCustomParams.clear();

// REMOVE Tim. tmp. Added condition.
  // Ignore parameters if there is custom data.
  // The assumption is that control or parameter values would be included in the custom data.
  // See getCustomConfiguration() for more info.
  if(config._accumulatedCustomParams.empty() && (opts & ConfigParams))
  {
    unsigned long controlPorts = parameters();

    const unsigned long sz = config._initParams.size();
    for(unsigned long i = 0; i < sz; ++i)
    {
      const PluginConfiguration::ControlConfig& cc = config._initParams[i];
      // If a valid control number was stored, use it for faster lookup.
      if(cc._ctlnum >= 0)
      {
        if((unsigned long)cc._ctlnum < controlPorts)
        {
          // TODO: Set vst params directly here, maybe even instead of controls array?
          //       Maybe not. Process sets the vst params for us.

  // REMOVE Tim. tmp. Changed.
          // controls[j].val = controls[j].tmpVal = cc._val;
          //controls[cc._ctlnum].val = cc._val;
          putParam(cc._ctlnum, cc._val);
        }
        else
        {
          fprintf(stderr, "PluginI::configure(%d %s, %f) controller number out of range\n",
            cc._ctlnum, cc._name.toLatin1().constData(), cc._val);
          // Don't return. Let it continue.
          //return false;
        }
      }
      else
      // A valid control number was not stored, use the name for a slow lookup.
      {
        bool found = false;
  //      pi->setControl();
        for(unsigned long j = 0; j < controlPorts; ++j)
        {
          if(QString(paramName(j)) == cc._name)
          {
  // REMOVE Tim. tmp. Changed.
            // controls[j].val = controls[j].tmpVal = cc._val;
            //controls[j].val = cc._val;
            putParam(j, cc._val);
            found = true;
            break;
          }
        }
        if(!found)
        {
          fprintf(stderr, "PluginI::configure(%s, %f) controller name not found\n",
            cc._name.toLatin1().constData(), cc._val);
          // Don't return. Let it continue.
          //return false;
        }
      }
    }
  }

// // REMOVE Tim. tmp. Removed.
//                                 // initControlValues = true;

  // Done with initial params. Clear them.
//  config._initParams.clear();

  if(opts & ConfigGeometry)
    setGeometry(_initConfig._geometry.x(), _initConfig._geometry.y(),
      _initConfig._geometry.width(), _initConfig._geometry.height());

  if(opts & ConfigNativeGeometry)
    setNativeGeometry(_initConfig._nativeGeometry.x(), _initConfig._nativeGeometry.y(),
      _initConfig._nativeGeometry.width(), _initConfig._nativeGeometry.height());

  if(opts & ConfigGui)
    showGui(config._guiVisible);

  if(opts & ConfigNativeGui)
  {
    if(opts & ConfigDeferNativeGui)
      // We can't tell OSC to show the native plugin gui
      //  until the parent track is added to the lists.
      // OSC needs to find the plugin in the track lists.
      // Use this 'pending' flag so it gets done later.
      _showNativeGuiPending = config._nativeGuiVisible;
    else
      showNativeGui(config._nativeGuiVisible);
  }

  if(gui())
    gui()->updateValues();
}

void PluginI::configure(ConfigureOptions_t opts)
{
  return configure(_initConfig, opts);
}

// REMOVE Tim. tmp. Changed.
// //---------------------------------------------------------
// //   saveConfiguration
// //---------------------------------------------------------
//
// void PluginI::writeConfiguration(int level, Xml& xml)
// {
// // REMOVE Tim. tmp. Changed. Channel is obsolete.
//       // if(_plugin->uri().isEmpty())
//       // {
//       //   xml.tag(level++, "plugin file=\"%s\" label=\"%s\" channel=\"%d\"",
//       //      Xml::xmlString(_plugin->lib()).toLatin1().constData(),
//       //      Xml::xmlString(_plugin->label()).toLatin1().constData(), channel);
//       // }
//       // else
//       // {
//       //   xml.tag(level++, "plugin uri=\"%s\" label=\"%s\" channel=\"%d\"",
//       //      Xml::xmlString(_plugin->uri()).toLatin1().constData(),
//       //      Xml::xmlString(_plugin->label()).toLatin1().constData(), channel);
//       // }
//
//       // If plugin is available, ask it for the values.
//       if(_plugin)
//       {
//         if(_plugin->uri().isEmpty())
//         {
//           xml.tag(level++, "plugin file=\"%s\" label=\"%s\" name=\"%s\"",
//             Xml::xmlString(_plugin->lib()).toLatin1().constData(),
//             Xml::xmlString(_plugin->label()).toLatin1().constData(),
//             Xml::xmlString(_plugin->name()).toLatin1().constData());
//         }
//         else
//         {
//           xml.tag(level++, "plugin uri=\"%s\" label=\"%s\" name=\"%s\"",
//             Xml::xmlString(_plugin->uri()).toLatin1().constData(),
//             Xml::xmlString(_plugin->label()).toLatin1().constData(),
//             Xml::xmlString(_plugin->name()).toLatin1().constData());
//         }
//       }
//       else
//       // Plugin is not available. Use the persistent values.
//       {
//         if(_initConfig._uri.isEmpty())
//         {
//           xml.tag(level++, "plugin file=\"%s\" label=\"%s\" name=\"%s\"",
//             Xml::xmlString(_initConfig._file).toLatin1().constData(),
//             Xml::xmlString(_initConfig._label).toLatin1().constData(),
//             Xml::xmlString(_initConfig._name).toLatin1().constData());
//         }
//         else
//         {
//           xml.tag(level++, "plugin uri=\"%s\" label=\"%s\" name=\"%s\"",
//             Xml::xmlString(_initConfig._uri).toLatin1().constData(),
//             Xml::xmlString(_initConfig._label).toLatin1().constData(),
//             Xml::xmlString(_initConfig._name).toLatin1().constData());
//         }
//       }
//
//       //=============================================================
//       // Save lv2 or vst plugin state custom data before controls
//       //=============================================================
//
//       // If plugin is available, ask it for the values.
//       if(_plugin)
//       {
// #ifdef LV2_SUPPORT
//         if(_plugin->isLV2Plugin())
//         {
//           LV2PluginWrapper *lv2Plug = static_cast<LV2PluginWrapper *>(_plugin);
//           //for multi-instance plugins write only first instance's state
//           if(instances > 0)
//           {
//               lv2Plug->writeConfiguration(handle [0], level, xml);
//           }
//         }
// #endif
//
// #ifdef VST_NATIVE_SUPPORT
//         if(_plugin->isVstNativePlugin())
//         {
//           VstNativePluginWrapper *vstPlug = static_cast<VstNativePluginWrapper *>(_plugin);
//           //for multi-instance plugins write only first instance's state
//           if(instances > 0)
//           {
//               vstPlug->writeConfiguration(handle [0], level, xml);
//           }
//         }
// #endif
//       }
//       else
//       // Plugin is not available. Use the persistent values.
//       {
//         const unsigned long sz = _initConfig._accumulatedCustomParams.size();
//         for(unsigned long i = 0; i < sz; ++i)
//           xml.strTag(level, "customData", _initConfig._accumulatedCustomParams[i]);
//       }
//
//
//       //==============================================================
//       // Save control values
//       //    Although custom data might already save this information,
//       //     we have no way of knowing if it does or what is saved.
//       //    Traditionally we used this with LADSPA and DSSI, which have
//       //     no control value state saving mechanism.
//       //==============================================================
//
//       bool handled = false;
//
//       // If plugin is available, ask it for the values.
//       if(_plugin)
//       {
//         //==============================================================================================
//         // Special for vst: If plugin is vst, ask the vst itself for the control values.
//         // This is because at least one plugin (U-He uhbik) was seen NOT calling back the app
//         //  pluginHostCallback() with audioMasterAutomate when patches were changed from the native gui.
//         // All the gui controls change with patch changes, but no response - nothing at all happened.
//         // So all of our local mirrored control arrays do not get updated when gui patch changes.
//         // Thus our generic gui also is not updated.
//         // Meanwhile other plugins DO call the callback with audioMasterAutomate when patches change
//         //  from the native gui. Like TAL etc.
//         //==============================================================================================
// #ifdef VST_NATIVE_SUPPORT
//         if(_plugin->isVstNativePlugin())
//         {
//           //for multi-instance plugins write only first instance's state
//           if(instances > 0)
//           {
//             VstNativePluginWrapper_State *state = (VstNativePluginWrapper_State*)handle[0];
//
//
//             // FIXME: This code is low-level ugly. Should be in the plugin or wrapper class
//             //         but there aren't any get/set param functions in either class !
//
//             // VstNativePluginWrapper *vstPlug = static_cast<VstNativePluginWrapper *>(_plugin);
//             // const int sz = state->plugin->numParams;
//             // const int ins = state->plugin->numInputs;
//             // const int outs = state->plugin->numOutputs;
//             //
//             // //const unsigned long ssz = vstPlug->controlInPorts();
//             // //for (unsigned long i = 0; i < sz; ++i)
//             // for (int i = 0; i < sz; ++i)
//             // {
//             //   const QString name = _plugin->portName(ins + outs + i);
//             //   const float val = state->plugin->getParameter(state->plugin, i);
//             // }
//
//
//             for (unsigned long i = 0; i < controlPorts; ++i)
//             {
//                   unsigned long idx = controls[i].idx;
//                   const QString s("control ctl=\"%1\" name=\"%2\" val=\"%3\" /");
//                   // Use hex value string when appropriate.
//                   xml.tag(level,
//                     s.arg(i).
//                       arg(Xml::xmlString(_plugin->portName(idx)).toLatin1().constData()).
// // REMOVE Tim. tmp. Changed.
//                       // arg(MusELib::museStringFromDouble(double(controls[i].tmpVal))).toLatin1().constData());
//                       // arg(MusELib::museStringFromDouble(double(controls[i].val))).toLatin1().constData());
//                       //arg(MusELib::museStringFromFloat(param(i))).toLatin1().constData());
//                       //arg(MusELib::museStringFromFloat(controls[i].val)).toLatin1().constData());
//                       arg(MusELib::museStringFromFloat(state->plugin->getParameter(state->plugin, i))).toLatin1().constData());
//             }
//             handled = true;
//           }
//         }
//         else
//
// // #else // VST_NATIVE_SUPPORT
// #endif
//         {
//           for (unsigned long i = 0; i < controlPorts; ++i)
//           {
//                 unsigned long idx = controls[i].idx;
//                 const QString s("control ctl=\"%1\" name=\"%2\" val=\"%3\" /");
//                 // Use hex value string when appropriate.
//                 xml.tag(level,
//                   s.arg(i).
//                     arg(Xml::xmlString(_plugin->portName(idx)).toLatin1().constData()).
//     // REMOVE Tim. tmp. Changed.
//                     // arg(MusELib::museStringFromDouble(double(controls[i].tmpVal))).toLatin1().constData());
//                     // arg(MusELib::museStringFromDouble(double(controls[i].val))).toLatin1().constData());
//                     //arg(MusELib::museStringFromFloat(param(i))).toLatin1().constData());
//                     arg(MusELib::museStringFromFloat(controls[i].val)).toLatin1().constData());
//           }
//           handled = true;
//         }
// // #endif // VST_NATIVE_SUPPORT
//
//       }
//       //else
//
//       // If plugin is not available, use the persistent values.
//       if(!handled)
//       {
//         const unsigned long sz = _initConfig._initParams.size();
//         for (unsigned long i = 0; i < sz; ++i)
//         {
//               const QString s("control ctl=\"%1\" name=\"%2\" val=\"%3\" /");
//               // Use hex value string when appropriate.
//               xml.tag(level,
//                 s.arg(_initConfig._initParams[i]._ctlnum).
//                   arg(Xml::xmlString(_initConfig._initParams[i]._name).toLatin1().constData()).
//                   arg(MusELib::museStringFromFloat(_initConfig._initParams[i]._val)).toLatin1().constData());
//         }
//       }
//
//
//       if (_active == false)
//             xml.intTag(level, "active", _active);
//
//       if (_on == false)
//             xml.intTag(level, "on", _on);
//
//       _quirks.write(level, xml);
//
//       // If plugin is available, ask it for the values.
//       if((_plugin && guiVisible()) || (!_plugin && _initConfig._guiVisible))
//         xml.intTag(level, "gui", 1);
//
//       {
//         QRect r;
//         if(_plugin)
//         {
//           int x, y, w, h;
//           getGeometry(&x, &y, &w, &h);
//           r = QRect(x, y, w, h);
//         }
//         else
//         {
//           r = _initConfig._geometry;
//         }
//         xml.qrectTag(level, "geometry", r);
//
//
//         if((_plugin && nativeGuiVisible()) || (!_plugin && _initConfig._nativeGuiVisible))
//           xml.intTag(level, "nativegui", 1);
//
//         if(_plugin)
//         {
//           int x, y, w, h;
//           getNativeGeometry(&x, &y, &w, &h);
//           r = QRect(x, y, w, h);
//         }
//         else
//         {
//           r = _initConfig._nativeGeometry;
//         }
//         xml.qrectTag(level, "nativeGeometry", r);
//       }
//
//       // if(_plugin)
//       // {
//       //   if(guiVisible())
//       //     xml.intTag(level, "gui", 1);
//       //   int x, y, w, h;
//       //   getGeometry(&x, &y, &w, &h);
//       //   QRect r(x, y, w, h);
//       //   xml.qrectTag(level, "geometry", r);
//       //
//       //   if(nativeGuiVisible())
//       //     xml.intTag(level, "nativegui", 1);
//       //   getNativeGeometry(&x, &y, &w, &h);
//       //   QRect nr(x, y, w, h);
//       //   xml.qrectTag(level, "nativeGeometry", nr);
//       // }
//       // else
//       // // Plugin is not available. Use the persistent values.
//       // {
//       //   if(_initConfig._guiVisible)
//       //     xml.intTag(level, "gui", 1);
//       //   xml.qrectTag(level, "geometry", _initConfig._geometry);
//       //   if (_initConfig._nativeGuiVisible)
//       //     xml.intTag(level, "nativegui", 1);
//       //   xml.qrectTag(level, "nativeGeometry", _initConfig._nativeGeometry);
//       // }
//
//       xml.etag(--level, "plugin");
// }

//---------------------------------------------------------
//   writeConfiguration
//---------------------------------------------------------

void PluginI::writeConfiguration(int level, Xml& xml)
{
  PluginConfiguration pc = getConfiguration();

  //==============================================
  // Save file or uri, label, and name basic info
  //==============================================

  if(pc._uri.isEmpty())
  {
    xml.tag(level++, "plugin file=\"%s\" label=\"%s\" name=\"%s\"",
      Xml::xmlString(pc._file).toLatin1().constData(),
      Xml::xmlString(pc._pluginLabel).toLatin1().constData(),
      Xml::xmlString(pc._name).toLatin1().constData());
  }
  else
  {
    xml.tag(level++, "plugin uri=\"%s\" label=\"%s\" name=\"%s\"",
      Xml::xmlString(pc._uri).toLatin1().constData(),
      Xml::xmlString(pc._pluginLabel).toLatin1().constData(),
      Xml::xmlString(pc._name).toLatin1().constData());
  }

  //=============================================================
  // Save lv2 or vst plugin state custom data before controls
  //=============================================================

  if(!pc._accumulatedCustomParams.empty())
  {
    const unsigned long sz = pc._accumulatedCustomParams.size();
    for(unsigned long i = 0; i < sz; ++i)
    {
      if(!pc._accumulatedCustomParams[i].isEmpty())
        xml.strTag(level, "customData", pc._accumulatedCustomParams[i]);
    }
  }

  //=====================
  // Save control values
  //=====================

  if(!pc._initParams.empty())
  {
    const unsigned long sz = pc._initParams.size();
    for(unsigned long i = 0; i < sz; ++i)
    {
      const PluginConfiguration::ControlConfig &cc = pc._initParams[i];
      const QString s("control ctl=\"%1\" name=\"%2\" val=\"%3\" /");
      // Use hex value string when appropriate.
      xml.tag(level,
        s.arg(cc._ctlnum).
          arg(Xml::xmlString(cc._name).toLatin1().constData()).
          arg(MusELib::museStringFromFloat(cc._val)).toLatin1().constData());
    }
  }

  //=======================
  // Save other basic info
  //=======================

  xml.intTag(level, "active", pc._active);

  xml.intTag(level, "on", pc._on);

  pc._quirks.write(level, xml);

  xml.intTag(level, "gui", pc._guiVisible);

  xml.qrectTag(level, "geometry", pc._geometry);

  xml.intTag(level, "nativegui", pc._nativeGuiVisible);

  xml.qrectTag(level, "nativeGeometry", pc._nativeGeometry);

  xml.etag(--level, "plugin");
}

//---------------------------------------------------------
//   loadControl
//---------------------------------------------------------

// REMOVE Tim. tmp. Changed.
// bool PluginI::loadControl(Xml& xml)
//       {
//       QString file;
//       QString label;
//       QString name("mops");
// // REMOVE Tim. tmp. Changed.
//       // double val = 0.0;
//       float val = 0.0f;
//       int idx = -1;
//
//       for (;;) {
//             Xml::Token token = xml.parse();
//             const QString& tag = xml.s1();
//
//             switch (token) {
//                   case Xml::Error:
//                   case Xml::End:
//                         return true;
//                   case Xml::TagStart:
//                         xml.unknown("PluginI-Control");
//                         break;
//                   case Xml::Attribut:
//                         if (tag == "name")
//                               name = xml.s2();
// // REMOVE Tim. tmp. Added.
//                         else if (tag == "idx")
//                               idx = xml.s2().toInt();
//
//                         else if (tag == "val")
//                               // Accept either decimal or hex value strings.
// // REMOVE Tim. tmp. Changed.
//                               // val = MusELib::museStringToDouble(xml.s2());
//                               val = MusELib::museStringToFloat(xml.s2());
//                         break;
//                   case Xml::TagEnd:
//                         if (tag == "control") {
//                               if(_plugin)
//                               {
//                                 bool found = false;
//                                 for(unsigned long i = 0; i < controlPorts; ++i)
//                                 {
//                                   if(_plugin->portName(controls[i].idx) == name)
//                                   {
// // REMOVE Tim. tmp. Changed.
//                                     // controls[i].val = controls[i].tmpVal = val;
//                                     controls[i].val = val;
//                                     found = true;
//                                   }
//                                 }
//                                 if(!found)
//                                 {
//                                   printf("PluginI:loadControl(%s, %f) controller not found\n",
//                                     name.toLatin1().constData(), val);
//                                   return false;
//                                 }
// // REMOVE Tim. tmp. Removed.
//                                 // initControlValues = true;
//                               }
//                               return false;
//                           }
//                         return true;
//                   default:
//                         break;
//                   }
//             }
//       return true;
//       }

bool PluginI::loadControl(Xml& xml)
      {
      QString file;
      QString label;
      QString name("mops");
      float val = 0.0f;
      int ctlnum = -1;

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
                        else if (tag == "ctl")
                              ctlnum = xml.s2().toInt();
                        else if (tag == "val")
                              // Accept either decimal or hex value strings.
                              val = MusELib::museStringToFloat(xml.s2());
                        break;
                  case Xml::TagEnd:
                        if (tag == "control") {
                              _initConfig._initParams.push_back(PluginConfiguration::ControlConfig(ctlnum, name, val));

//                               if(_plugin)
//                               {
//                                 bool found = false;
//                                 for(unsigned long i = 0; i < controlPorts; ++i)
//                                 {
//                                   if(_plugin->portName(controls[i].idx) == name)
//                                   {
// // REMOVE Tim. tmp. Changed.
//                                     // controls[i].val = controls[i].tmpVal = val;
//                                     controls[i].val = val;
//                                     found = true;
//                                   }
//                                 }
//                                 if(!found)
//                                 {
//                                   printf("PluginI:loadControl(%s, %f) controller not found\n",
//                                     name.toLatin1().constData(), val);
//                                   return false;
//                                 }
// // REMOVE Tim. tmp. Removed.
//                                 // initControlValues = true;
//                               }

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

// REMOVE Tim. tmp. Changed.
// bool PluginI::readConfiguration(Xml& xml, bool readPreset)
//       {
//       QString file;
//       QString label;
//       QString uri;
//
//       //custom params in xml song file , synth tag, that will be passed to new PluginI:setCustomData(Xml &) method
//       //now only lv2host uses them, others simply ignore
//       std::vector<QString> accumulatedCustomParams;
//
// // REMOVE Tim. tmp. Removed.
//       // if (!readPreset)
//       //       channel = 1;
//
//       for (;;) {
//             Xml::Token token(xml.parse());
//             const QString& tag(xml.s1());
//             switch (token) {
//                   case Xml::Error:
//                   case Xml::End:
//                         return true;
//                   case Xml::TagStart:
//                         if (!readPreset && _plugin == 0) {
//                               _plugin = MusEGlobal::plugins.find(file, uri, label);
//
//                               if (_plugin)
//                               {
// // REMOVE Tim. tmp. Changed.
//                                  // if(initPluginInstance(_plugin, channel)) {
//                                  if(initPluginInstance(_plugin, _track->channels())) {
//                                     _plugin = 0;
//                                     xml.parse1();
//                                     printf("Error initializing plugin instance (%s, %s, %s)\n",
//                                        file.toLatin1().constData(),
//                                        uri.toLatin1().constData(),
//                                        label.toLatin1().constData());
//                                     //break;      // Don't break - let it read any control tags.
//                                     }
//                                  }
//                               }
//                         if (tag == "control")
//                               loadControl(xml);
//                         else if (tag == "active") {
//                               bool flag = xml.parseInt();
//                               if (!readPreset)
//                                     _active = flag;
//                               }
//                         else if (tag == "on") {
//                               bool flag = xml.parseInt();
//                               if (!readPreset)
//                                     _on = flag;
//                               }
//                         else if (tag == "quirks") {
//                               PluginQuirks q;
//                               q.read(xml);
//                               if (!readPreset)
//                                 _quirks = q;
//                               }
//                         else if (tag == "gui") {
//                               const bool flag = xml.parseInt();
//                               if (_plugin)
//                                   showGui(flag);
//                               }
//                         else if (tag == "nativegui") {
//                               // We can't tell OSC to show the native plugin gui
//                               //  until the parent track is added to the lists.
//                               // OSC needs to find the plugin in the track lists.
//                               // Use this 'pending' flag so it gets done later.
//                               _showNativeGuiPending = xml.parseInt();
//                               }
//                         else if (tag == "geometry") {
//                               QRect r(readGeometry(xml, tag));
//                               setGeometry(r.x(), r.y(), r.width(), r.height());
//                               }
//                         else if (tag == "nativeGeometry") {
//                               QRect r(readGeometry(xml, tag));
//                               setNativeGeometry(r.x(), r.y(), r.width(), r.height());
//                               }
//                         else if (tag == "customData") { //just place tag contents in accumulatedCustomParams
//                               QString customData = xml.parse1();
//                               if(!customData.isEmpty()){
//                                  accumulatedCustomParams.push_back(customData);
//                                  //now process custom data immediately
//                                  //because it MUST be processed before plugin controls
//                                  //writeConfiguration places custom data before plugin controls values
//                                  setCustomData(accumulatedCustomParams);
//                                  accumulatedCustomParams.clear();
//                               }
//                         }
//                         else
//                               xml.unknown("PluginI");
//                         break;
//                   case Xml::Attribut:
//                         if (tag == "file") {
//                               QString s = xml.s2();
//                               if (readPreset) {
// // REMOVE Tim. tmp. Changed.
//                                     // if (s != plugin()->lib()) {
//                                     //       printf("Error: Wrong preset type %s. Type must be a %s\n",
//                                     //          s.toLatin1().constData(), plugin()->lib().toLatin1().constData());
//                                     //       return true;
//                                     //       }
//
//                                     // If the plugin is available, compare file with plugin file.
//                                     // If the plugin is not available, compare file with the persistent settings.
//                                     const QString plibfile = plugin() ? plugin()->lib() : _initConfig._file;
//                                     if (s != plibfile) {
//                                           fprintf(stderr, "Error: Wrong preset type %s. Type must be a %s\n",
//                                              s.toLatin1().constData(), plibfile.toLatin1().constData());
//                                           return true;
//                                           }
//                                     }
//                               else {
// // REMOVE Tim. tmp. Changed.
//                                     // file = s;
//                                     _initConfig._file = s;
//                                     }
//                               }
//                         else if (tag == "uri") {
//                               QString s = xml.s2();
//                               if (readPreset) {
// // REMOVE Tim. tmp. Changed.
//                                     // if (s != plugin()->uri()) {
//                                     //       printf("Error: Wrong preset uri %s. Uri must be a %s\n",
//                                     //          s.toLatin1().constData(), plugin()->uri().toLatin1().constData());
//                                     //       return true;
//                                     //       }
//
//                                     // If the plugin is available, compare uri with plugin uri.
//                                     // If the plugin is not available, compare uri with the persistent settings.
//                                     const QString puri = plugin() ? plugin()->uri() : _initConfig._uri;
//                                     if (s != puri) {
//                                           fprintf(stderr, "Error: Wrong preset uri %s. Uri must be a %s\n",
//                                              s.toLatin1().constData(), puri.toLatin1().constData());
//                                           return true;
//                                           }
//                                     }
//                               else {
// // REMOVE Tim. tmp. Changed.
//                                     // uri = s;
//                                     _initConfig._uri = s;
//                                     }
//                               }
//                         else if (tag == "label") {
//                               if (!readPreset)
// // REMOVE Tim. tmp. Changed.
//                                     // label = xml.s2();
//                                     _initConfig._label = xml.s2();
//                               }
// // REMOVE Tim. tmp. Removed. Obsolete.
//                         // else if (tag == "channel") {
//                         //       if (!readPreset)
//                         //             channel = xml.s2().toInt();
//                         //       }
//                         break;
//                   case Xml::TagEnd:
//                         if (tag == "plugin") {
//                               if (!readPreset && _plugin == 0) {
//                                     _plugin = MusEGlobal::plugins.find(file, uri, label);
//                                     if (_plugin == 0)
//                                     {
//                                       QMessageBox::warning(0,"Plugin not found!",
//                                                   "Plugin: " + label + " not found, if the project is saved it will be removed from the project");
//                                       fprintf(stderr, "Warning: - Plugin not found (%s, %s, %s)\n",
//                                          file.toLatin1().constData(),
//                                          uri.toLatin1().constData(),
//                                          label.toLatin1().constData());
//                                       return true;
//                                     }
//
// // REMOVE Tim. tmp. Changed.
//                                     // if (initPluginInstance(_plugin, channel))
//                                     if (initPluginInstance(_plugin, _track->channels()))
//                                     {
//                                       fprintf(stderr, "Error initializing plugin instance (%s, %s, %s)\n",
//                                         file.toLatin1().constData(),
//                                         uri.toLatin1().constData(),
//                                         label.toLatin1().constData());
//                                       return true;
//                                     }
//                               }
//                               if (_gui)
//                                     _gui->updateValues();
//                               return false;
//                               }
//                         return true;
//                   default:
//                         break;
//                   }
//             }
//       return true;
//       }

bool PluginI::readConfiguration(Xml& xml, bool readPreset, int channels)
      {
      // QString file;
      // QString label;
      // QString uri;

      // //custom params in xml song file , synth tag, that will be passed to new PluginI:setCustomData(Xml &) method
      // //now only lv2host uses them, others simply ignore
      // std::vector<QString> accumulatedCustomParams;

// REMOVE Tim. tmp. Removed.
      // if (!readPreset)
      //       channel = 1;

      for (;;) {
            Xml::Token token(xml.parse());
            const QString& tag(xml.s1());
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return true;
                  case Xml::TagStart:
//                         if (!readPreset && _plugin == 0) {
//                               _plugin = MusEGlobal::plugins.find(file, uri, label);
//
//                               if (_plugin)
//                               {
// // REMOVE Tim. tmp. Changed.
//                                  // if(initPluginInstance(_plugin, channel)) {
//                                  if(initPluginInstance(_plugin, _track->channels())) {
//                                     _plugin = 0;
//                                     xml.parse1();
//                                     printf("Error initializing plugin instance (%s, %s, %s)\n",
//                                        file.toLatin1().constData(),
//                                        uri.toLatin1().constData(),
//                                        label.toLatin1().constData());
//                                     //break;      // Don't break - let it read any control tags.
//                                     }
//                                  }
//                               }
                        if (tag == "control")
                              loadControl(xml);
                        else if (tag == "active") {
                              const bool flag = xml.parseInt();
                              if (!readPreset)
                              //       _active = flag;
                                _initConfig._active = flag;
                              }
                        else if (tag == "on") {
                              const bool flag = xml.parseInt();
                              if (!readPreset)
                              //       _on = flag;
                                _initConfig._on = flag;
                              }
                        else if (tag == "quirks") {
                              PluginQuirks q;
                              //q.read(xml);
                              //_quirks = q;
                              if(!q.read(xml) && !readPreset)
                                _initConfig._quirks = q;
                              }
                        else if (tag == "gui") {
                              const bool flag = xml.parseInt();
                              //if (_plugin)
                              //    showGui(flag);
                              if (!readPreset)
                                _initConfig._guiVisible = flag;
                              }
                        else if (tag == "nativegui") {
                              const bool flag = xml.parseInt();
//                              // We can't tell OSC to show the native plugin gui
//                              //  until the parent track is added to the lists.
//                              // OSC needs to find the plugin in the track lists.
//                              // Use this 'pending' flag so it gets done later.
//                              // _showNativeGuiPending = xml.parseInt();
                              if (!readPreset)
                                _initConfig._nativeGuiVisible = flag;
                              }
                        else if (tag == "geometry") {
                              const QRect r(readGeometry(xml, tag));
                              //setGeometry(r.x(), r.y(), r.width(), r.height());
                              if (!readPreset)
                                _initConfig._geometry = r;
                              }
                        else if (tag == "nativeGeometry") {
                              const QRect r(readGeometry(xml, tag));
                              //setNativeGeometry(r.x(), r.y(), r.width(), r.height());
                              if (!readPreset)
                                _initConfig._nativeGeometry = r;
                              }
                        else if (tag == "customData") { //just place tag contents in accumulatedCustomParams
                              const QString customData = xml.parse1();
                              if(!customData.isEmpty()){
                                 //accumulatedCustomParams.push_back(customData);
                                 //now process custom data immediately
                                 //because it MUST be processed before plugin controls
                                 //writeConfiguration places custom data before plugin controls values
                                 //setCustomData(accumulatedCustomParams);
                                 //accumulatedCustomParams.clear();
                                 _initConfig._accumulatedCustomParams.push_back(customData);
                              }
                        }
                        else
                              xml.unknown("PluginI");
                        break;
                  case Xml::Attribut:
                        if (tag == "file") {
                              QString s = xml.s2();
                              if (readPreset) {
// REMOVE Tim. tmp. Changed.
                                    // if (s != plugin()->lib()) {
                                    //       printf("Error: Wrong preset type %s. Type must be a %s\n",
                                    //          s.toLatin1().constData(), plugin()->lib().toLatin1().constData());
                                    //       return true;
                                    //       }

                                    // If the plugin is available, compare file with plugin file.
                                    // If the plugin is not available, compare file with the persistent settings.
                                    const QString plibfile = plugin() ? plugin()->lib() : _initConfig._file;
                                    if (s != plibfile) {
                                          fprintf(stderr, "Error: Wrong preset type %s. Type must be a %s\n",
                                             s.toLatin1().constData(), plibfile.toLatin1().constData());
                                          return true;
                                          }
                                    }
                              else {
// REMOVE Tim. tmp. Changed.
                                    // file = s;
                                    _initConfig._file = s;
                                    }
                              }
                        else if (tag == "uri") {
                              QString s = xml.s2();
                              if (readPreset) {
// REMOVE Tim. tmp. Changed.
                                    // if (s != plugin()->uri()) {
                                    //       printf("Error: Wrong preset uri %s. Uri must be a %s\n",
                                    //          s.toLatin1().constData(), plugin()->uri().toLatin1().constData());
                                    //       return true;
                                    //       }

                                    // If the plugin is available, compare uri with plugin uri.
                                    // If the plugin is not available, compare uri with the persistent settings.
                                    const QString puri = plugin() ? plugin()->uri() : _initConfig._uri;
                                    if (s != puri) {
                                          fprintf(stderr, "Error: Wrong preset uri %s. Uri must be a %s\n",
                                             s.toLatin1().constData(), puri.toLatin1().constData());
                                          return true;
                                          }
                                    }
                              else  {
// REMOVE Tim. tmp. Changed.
                                      // uri = s;
                                      _initConfig._uri = s;
                                    }
                              }
                        else if (tag == "label") {
                              if (!readPreset)
// REMOVE Tim. tmp. Changed.
                                    // label = xml.s2();
                                    _initConfig._pluginLabel = xml.s2();
                              }
                        else if (tag == "name") {
                              if (!readPreset)
                                    _initConfig._name = xml.s2();
                              }
// REMOVE Tim. tmp. Removed. Obsolete.
                        // else if (tag == "channel") {
                        //       if (!readPreset)
                        //             channel = xml.s2().toInt();
                        //       }
                        break;
                  case Xml::TagEnd:
                        if (tag == "plugin")
                        {
                              if (!readPreset && _plugin == nullptr)
                              {
//                                     _plugin = MusEGlobal::plugins.find(
//                                       _initConfig._file, _initConfig._uri, _initConfig._label);
//
//                                     if (_plugin == nullptr)
//                                     {
// // REMOVE Tim. tmp. Removed.
//                                       // QMessageBox::warning(0,"Plugin not found!",
//                                       //   "Plugin: " + _initConfig._label + " not found. Settings are preserved if the project is saved.");
//                                       fprintf(stderr, "Warning: - Plugin not found (%s, %s, %s)\n",
//                                          _initConfig._file.toLatin1().constData(),
//                                          _initConfig._uri.toLatin1().constData(),
//                                          _initConfig._label.toLatin1().constData());
//                                       //return true;
//                                       // Return OK to allow it to continue loading.
//                                       //return false;
//
//                                       // For persistence: Allow it to continue on. Initialization needs to do a few things.
//                                     }

// REMOVE Tim. tmp. Changed.
//                                    if (initPluginInstance(_plugin, channel))
//                                    if (initPluginInstance(_plugin, _track->channels()))
//                                    if (initPluginInstance(_initConfig, _track->channels()))
//                                    if (initPluginInstance(_track->channels()))
//                                    if (initPluginInstance(channels, _initConfig._label, _initConfig._name))
                                    if (initPluginInstance(channels))
                                    {
                                      // For persistence: Keep the custom data and parameters (don't clear them).
                                      // If the error was because there was no plugin available,
                                      //  return no error to allow it to continue loading.
                                      if(!_plugin)
                                        return false;

                                      fprintf(stderr, "Error initializing plugin instance (%s, %s, %s)\n",
                                        _initConfig._file.toLatin1().constData(),
                                        _initConfig._uri.toLatin1().constData(),
                                        _initConfig._pluginLabel.toLatin1().constData());

                                      // The error was for some other reason.
                                      // Hm, let's null that plugin pointer.
                                      _plugin = nullptr;

                                      // Return an error.
                                      return true;
                                    }
                              }

                              // In case of reading a preset but there's no plugin,
                              //  return OK to allow it to continue loading.
                              // For persistence: Keep the custom data and parameters (don't clear them).
                              if(!_plugin)
//                                return true;
                                return false;

                              // Options for configuration.
                              ConfigureOptions_t opts = readPreset ? ConfigPresetOnly : ConfigAll;
                              // Special for DSSI: Defer opening the native gui.
                              // We can't tell OSC to show the native plugin gui
                              //  until the parent track is added to the lists.
                              // OSC needs to find the plugin in the track lists.
                              // TODO: Find a way to offload this to DSSI so we don't have to worry about it here.
                              if(isDssiPlugin())
                                opts |= ConfigDeferNativeGui;

                              configure(opts);



//                               // Don't bother setting these if reading a preset.
//                               if(!readPreset)
//                               {
//                                 _active = _initConfig._active;
//                                 _on = _initConfig._on;
//                                 _quirks = _initConfig._quirks;
//                               }
//
//                               //now process custom data immediately
//                               //because it MUST be processed before plugin controls
//                               //writeConfiguration places custom data before plugin controls values
//                               setCustomData(_initConfig._accumulatedCustomParams);
//
//                               // Ignore initial parameters if there is custom data.
//                               // The assumption is that control or parameter values would be included in the custom data.
//                               // See getCustomConfiguration() for more info.
//                               if(_initConfig._accumulatedCustomParams.empty())
//                               {
//                                 const unsigned long sz = _initConfig._initParams.size();
//                                 for(unsigned long i = 0; i < sz; ++i)
//                                 {
//                                   const PluginConfiguration::ControlConfig& cc = _initConfig._initParams[i];
//                                   // If a valid control number was stored, use it for faster lookup.
//                                   if(cc._ctlnum >= 0)
//                                   {
//                                     if((unsigned long)cc._ctlnum < controlPorts)
//                                     {
//                                       // TODO: Set vst params directly here, maybe even instead of controls array?
//                                       //       Maybe not. Process sets the vst params for us.
//
// // REMOVE Tim. tmp. Changed.
// //                                      controls[j].val = controls[j].tmpVal = cc._val;
// //                                      controls[cc._ctlnum].val = cc._val;
//                                       putParam(cc._ctlnum, cc._val);
//                                     }
//                                     else
//                                     {
//                                       fprintf(stderr, "PluginI:readConfiguration(%d %s, %f) controller number out of range\n",
//                                         cc._ctlnum, cc._name.toLatin1().constData(), cc._val);
//                                       // Don't return. Let it continue.
//                                       //return false;
//                                     }
//                                   }
//                                   else
//                                   // A valid control number was not stored, use the name for a slow lookup.
//                                   {
//                                     bool found = false;
//                                     for(unsigned long j = 0; j < controlPorts; ++j)
//                                     {
// // REMOVE Tim. tmp. Changed.
// //                                      if(_plugin->portName(controls[j].idx) == cc._name)
//                                       if(QString(paramName(j)) == cc._name)
//                                       {
// // REMOVE Tim. tmp. Changed.
//                                         // controls[j].val = controls[j].tmpVal = cc._val;
//                                         controls[j].val = cc._val;
//                                         found = true;
//                                         break;
//                                       }
//                                     }
//                                     if(!found)
//                                     {
//                                       fprintf(stderr, "PluginI:readConfiguration(%s, %f) controller name not found\n",
//                                         cc._name.toLatin1().constData(), cc._val);
//                                       // Don't return. Let it continue.
//                                       //return false;
//                                     }
//                                   }
//                                 }
//                               }

// REMOVE Tim. tmp. Removed.
//                                 // initControlValues = true;

                              // Done with initial params. Clear them.
                              _initConfig._initParams.clear();
                              // Done with initial custom data. Clear them.
                              _initConfig._accumulatedCustomParams.clear();

//                               // Don't bother setting these if reading a preset.
//                               if (!readPreset)
//                               {
//                                 setGeometry(_initConfig._geometry.x(), _initConfig._geometry.y(),
//                                   _initConfig._geometry.width(), _initConfig._geometry.height());
//
//                                 setNativeGeometry(_initConfig._nativeGeometry.x(), _initConfig._nativeGeometry.y(),
//                                   _initConfig._nativeGeometry.width(), _initConfig._nativeGeometry.height());
//
//                                 showGui(_initConfig._guiVisible);
//                                 // We can't tell OSC to show the native plugin gui
//                                 //  until the parent track is added to the lists.
//                                 // OSC needs to find the plugin in the track lists.
//                                 // Use this 'pending' flag so it gets done later.
//                                 _showNativeGuiPending = _initConfig._nativeGuiVisible;
//                               }
//
//                               if (_gui)
//                                     _gui->updateValues();
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

bool PluginI::isShowNativeGuiPending() { return _showNativeGuiPending; }

// REMOVE Tim. tmp. Added.
void PluginI::closeNativeGui()
{
  #ifdef OSC_SUPPORT
  if (_plugin)
  {
    if (_oscif.isRunning())
      _oscif.oscQuitGui();
  }
  #endif
  _showNativeGuiPending = false;
}

// REMOVE Tim. tmp. Added.
void PluginI::updateNativeGuiWindowTitle()
{
#ifdef LV2_SUPPORT
    if(plugin() && plugin()->isLV2Plugin())
      ((LV2PluginWrapper *)plugin())->updateNativeGuiWindowTitle(this);
#endif
#ifdef VST_NATIVE_SUPPORT
    if(plugin() && plugin()->isVstNativePlugin())
      ((VstNativePluginWrapper *)plugin())->updateNativeGuiWindowTitle(this);
#endif

  // NOTE DSSI UIs can't be updated once they have been created.
  //      Instead, close them so that the next time they open, the text is correct.
  //      Use PluginI::closeNativeGui() for example.
}

unsigned long PluginI::parameters() const           { return controlPorts; }
unsigned long PluginI::parametersOut() const           { return controlOutPorts; }
// REMOVE Tim. tmp. Changed.
// void PluginI::putParam(unsigned long i, double val) { controls[i].val = controls[i].tmpVal = val; }
void PluginI::putParam(unsigned long i, double val) { controls[i].val = val; }
double PluginI::param(unsigned long i) const        { return controls[i].val; }
double PluginI::paramOut(unsigned long i) const        { return controlsOut[i].val; }


void PluginI::guiHeartBeat()
{
  PluginIBase::guiHeartBeat();
#ifdef OSC_SUPPORT
  // Update the DSSI UI's controls if needed, if it exists.
  if(plugin() && isDssiPlugin())
  {
    for(unsigned long i = 0; i < controlPorts; ++i)
      _oscif.oscSendControl(controls[i].idx, controls[i].val);
  }
#endif
}

//---------------------------------------------------------
//   makeGui
//---------------------------------------------------------

void PluginIBase::makeGui()
      {
      _gui = new MusEGui::PluginGui(this);
// REMOVE Tim. tmp. Added.
      updateGuiWindowTitle();
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

// REMOVE Tim. tmp. Added.
void PluginIBase::updateGuiWindowTitle() const
{
//   if(_gui)
//     _gui->updateWindowTitle();

  if(_gui)
  {
    _gui->updateWindowTitle(titlePrefix() + name() +
      (uri().isEmpty() ? QString() : QString(" : ") + uri()));
  }
}

void PluginIBase::guiHeartBeat()
{
  // Update our generic plugin UI's controls, if it exists.
  auto g = gui();
  if(g)
    g->heartBeat();
}

//---------------------------------------------------------
//   enableAllControllers
//---------------------------------------------------------

void PluginI::enableAllControllers(bool v)
{
  for(unsigned long i = 0; i < controlPorts; ++i)
    controls[i].enCtrl = v;
}

//==========================================================
// If plugin is available, ask it for the values.
// If plugin is not available, use the persistent settings.
//==========================================================

// TODO: Store the features in the file and the persistent settings, to use here?
PluginFeatures_t PluginI::requiredFeatures() const { return _plugin ? _plugin->requiredFeatures() : PluginNoFeatures; }

// TODO: Store the id in the file and the persistent settings, to use here?
unsigned long PluginI::pluginID() const { return _plugin ? _plugin->id() : 0; }

QString PluginI::pluginLabel() const    { return _plugin ? _plugin->label() : _initConfig._pluginLabel; }

// TODO: Use persistent settings here?
//QString PluginI::name() const          { return _label; }
//QString PluginI::name() const          { return _plugin ? _label : _initConfig._name; }

// TODO: Use persistent settings here?
QString PluginI::name() const           { return _name; }

QString PluginI::pluginName() const     { return _plugin ? _plugin->name() : QString(); }

QString PluginI::lib() const            { return _plugin ? _plugin->lib() : QString(); }
QString PluginI::uri() const            { return _plugin ? _plugin->uri() : _initConfig._uri; }
QString PluginI::dirPath() const        { return _plugin ? _plugin->dirPath() : QString(); }
QString PluginI::fileName() const       { return _plugin ? _plugin->fileName() : _initConfig._file; }

// TODO: Store these in the file and the persistent settings, to use here?
bool PluginI::isDssiPlugin() const { return _plugin ? _plugin->isDssiPlugin() : false; }
bool PluginI::isLV2Plugin() const { return _plugin ? _plugin->isLV2Plugin() : false; }
bool PluginI::isVstNativePlugin() const { return _plugin ? _plugin->isVstNativePlugin() : false; }

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
//---------------------------------------------------------

void PluginI::apply(unsigned pos, unsigned long n,
                    unsigned long ports, bool wantActive, float** bufIn, float** bufOut, float latency_corr_offset)
{
// REMOVE Tim. tmp. Added.
  // If plugin is not available just return.
  if(!plugin())
    return;

  const unsigned long syncFrame = MusEGlobal::audio->curSyncFrame();
  unsigned long sample = 0;
  const bool isOn = on();
  const bool isActive = active();

  const PluginBypassType bypassType = pluginBypassType();

  // Activate or deactivate the plugin now, depending on the desired track and plugin active states.
  // The two calls will do nothing if already in the desired state.
  if(wantActive && isActive)
    activate();
  else
    deactivate();

  //  Normally if the plugin is inactive or off we tell it to connect to dummy audio ports.
  //  But this can change depending on detected bypass type, below.
  bool connectToDummyAudioPorts = !_curActiveState || !isOn;
  //  Normally if the plugin is inactive or off we use a fixed controller period.
  //  But this can change depending on detected bypass type, below.
  bool usefixedrate = !_curActiveState || !isOn;
  const unsigned long fin_nsamp = n;

  // If the plugin has a REAL enable or bypass control port, we allow the plugin
  //  a full-length run so that it can handle its own enabling or bypassing.
  if(_curActiveState)
  {
    switch(bypassType)
    {
      case PluginBypassTypeEmulatedEnableController:
      break;

      case PluginBypassTypeEnablePort:
      case PluginBypassTypeBypassPort:
          connectToDummyAudioPorts = false;
          usefixedrate = false;
      break;

      case PluginBypassTypeEmulatedEnableFunction:
      break;

      case PluginBypassTypeEnableFunction:
      case PluginBypassTypeBypassFunction:
          connectToDummyAudioPorts = false;
      break;
    }
  }

  const PluginFreewheelType freewheelType = pluginFreewheelType();
  const unsigned long int freewheelIdx = freewheelPortIndex();

  // See if the features require a fixed control period.
  // FIXME Better support for PluginPowerOf2BlockSize, by quantizing the control period times.
  //       For now we treat it like fixed control period.
  if(requiredFeatures() & (PluginFixedBlockSize | PluginPowerOf2BlockSize | PluginCoarseBlockSize))
    usefixedrate = true;

  // Note for dssi-vst this MUST equal audio period. It doesn't like broken-up runs (it stutters),
  //  even with fixed sizes. Could be a Wine + Jack thing, wanting a full Jack buffer's length.
  // For now, the fixed size is clamped to the audio buffer size.
  // TODO: We could later add slower processing over several cycles -
  //  so that users can select a small audio period but a larger control period.
  const unsigned long min_per =
    (usefixedrate || MusEGlobal::config.minControlProcessPeriod > n) ? n : MusEGlobal::config.minControlProcessPeriod;
  const unsigned long min_per_mask = min_per-1;   // min_per must be power of 2

  AutomationType at = AUTO_OFF;
  CtrlListList* cll = nullptr;
  ciCtrlList icl_first;
  if(_track)
  {
    // Correction value is negative for correction.
    latency_corr_offset += _track->getLatencyInfo(false)._sourceCorrectionValue;

    at = _track->automationType();
    cll = _track->controller();
    if(_id != -1)
      icl_first = cll->lower_bound(genACnum(_id, 0));
  }
  const bool no_auto = !MusEGlobal::automation || at == AUTO_OFF;
  const unsigned long in_ctrls = _plugin->controlInPorts();

// REMOVE Tim. tmp. Changed.
  // // Special for plugins: Deal with tmpVal. TODO: Get rid of tmpVal, maybe by using the FIFO...
  // for(unsigned long k = 0; k < controlPorts; ++k)
  //   controls[k].val = controls[k].tmpVal;

  int cur_slice = 0;
  while(sample < fin_nsamp)
  {
    unsigned long slice_samps = fin_nsamp - sample;
    const unsigned long slice_frame = pos + sample;

    // Process automation control values, while also determining the maximum acceptable
    //  size of this run. Further processing, from FIFOs for example, can lower the size
    //  from there, but this section determines where the next highest maximum frame
    //  absolutely needs to be for smooth playback of the controller value stream...
    //
    {
      ciCtrlList icl = icl_first;
      for(unsigned long k = 0; k < controlPorts; ++k)
      {
        // Don't process freewheel controller. Freewheel port is a bad candidate
        //  for automation or user control, and should be hidden.
        if(freewheelType == PluginFreewheelTypePort && freewheelIdx == k)
          continue;

        CtrlList* cl = (cll && _id != -1 && icl != cll->end()) ? icl->second : nullptr;
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
          unsigned long samps = slice_samps;
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

          if(samps < slice_samps)
            slice_samps = samps;

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
              // Inform the GUI.
              (reinterpret_cast<LV2PluginWrapper *>(_plugin))->setLastStateControls(handle [i], k, true, false, true, 0.0f);
           }
        }
#endif

// REMOVE Tim. tmp. Removed.
        // controls[k].tmpVal = controls[k].val;  // Special for plugins: Deal with tmpVal.

#ifdef PLUGIN_DEBUGIN_PROCESS
        printf("PluginI::apply k:%lu sample:%lu frame:%lu nextFrame:%d slice_samps:%lu \n", k, sample, frame, ci.eFrame, slice_samps);
#endif
      }
    }

#ifdef PLUGIN_DEBUGIN_PROCESS
    printf("PluginI::apply sample:%lu slice_samps:%lu\n", sample, slice_samps);
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

      if(
         // Next events are for a later period.
         evframe >= n ||
            // Next events are for a later run in this period. (Autom took prio.)
           ((!usefixedrate && !found && !v.unique && (evframe - sample >= slice_samps))
            // Eat up events within minimum slice - they're too close.
            || (found && !v.unique && (evframe - sample >= min_per))
            // Special for dssi-vst: Fixed rate and must reply to all.
            || (usefixedrate && found && v.unique && v.idx == index)))
        break;

      if(v.idx >= in_ctrls) // Sanity check
      {
        _controlFifo.remove();               // Done with the ring buffer's item. Remove it.
        break;
      }

      found = true;
      frame = evframe;
      index = v.idx;

      // Don't process items meant for a freewheel port. Freewheel port is a bad candidate
      //  for automation or user control, and should be hidden.
      if(freewheelType == PluginFreewheelTypePort && freewheelIdx == index)
      {
        _controlFifo.remove();               // Done with the ring buffer's item. Remove it.
        continue;
      }

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
               // Don't send gui control changes back to the gui, just update the last value.
               (reinterpret_cast<LV2PluginWrapper *>(_plugin))->setLastStateControls(handle [i], v.idx, true, true, false, v.value);
            }
         }
      }
#endif

      _controlFifo.remove();               // Done with the ring buffer's item. Remove it.
    }

    if(found && !usefixedrate) // If a control FIFO item was found, takes priority over automation controller stream.
      slice_samps = frame - sample;

    if(sample + slice_samps > n)    // Safety check.
      slice_samps = n - sample;

    // TODO: Don't allow zero-length runs. This could/should be checked in the control loop instead.
    // Note this means it is still possible to get stuck in the top loop (at least for a while).
    if(slice_samps != 0)
    {
      if(_curActiveState)
      {
        connect(ports, connectToDummyAudioPorts, sample, bufIn, bufOut);
        for(int i = 0; i < instances; ++i)
          _plugin->apply(handle[i], slice_samps, latency_corr_offset);
      }

      sample += slice_samps;
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
//   oscIF
//---------------------------------------------------------

OscEffectIF& PluginI::oscIF() { return _oscif; }

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
// REMOVE Tim. tmp. Added.
      // If plugin is not available just return.
      if(!plugin())
        return 0;

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
// REMOVE Tim. tmp. Added.
  // If plugin is not available just return.
  if(!plugin())
    return 0;

  #ifdef PLUGIN_DEBUGIN
  printf("PluginI::oscControl received oscControl port:%lu val:%f\n", port, value);
  #endif

  if(port >= _plugin->rpIdx.size())
  {
    fprintf(stderr, "PluginI::oscControl: port number:%lu is out of range of index list size:%i\n", port, (int) _plugin->rpIdx.size());
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
  // Despite the discrepancy we are STILL required to update the DSSI control port values here
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
static const char* presetOpenText = "Click this button to load a saved <em>preset</em>.";
static const char* presetSaveText = "Click this button to save current parameter "
      "settings as a <em>preset</em>.  You will be prompted for a file name.";
static const char* presetBypassText = "Click this button to bypass effect unit";

//---------------------------------------------------------
//   PluginGui
//---------------------------------------------------------

PluginGui::PluginGui(MusECore::PluginIBase* p)
   : QMainWindow(nullptr)
      {
      gw     = nullptr;
      params = nullptr;
      paramsOut = nullptr;
      plugin = p;
// REMOVE Tim. tmp. Added.
//       updateWindowTitle();
      
      QToolBar* tools = addToolBar(tr("File Buttons"));
      tools->setIconSize(QSize(MusEGlobal::config.iconSize, MusEGlobal::config.iconSize));

      QAction* fileOpen = new QAction(*fileopenSVGIcon, tr("Load Preset"), this);
      connect(fileOpen, &QAction::triggered, [this]() { load(); } );
      tools->addAction(fileOpen);

      QAction* fileSave = new QAction(*filesaveasSVGIcon, tr("Save Preset"), this);
      connect(fileSave, &QAction::triggered, [this]() { save(); } );
      tools->addAction(fileSave);

      QAction* whatsthis = QWhatsThis::createAction(this);
      whatsthis->setIcon(*whatsthisSVGIcon);
      tools->addAction(whatsthis);

      tools->addSeparator();

      activeButton = new QAction(*trackOnSVGIcon, tr("Deactivate plugin"), this);
      activeButton->setCheckable(true);
      activeButton->setChecked(!plugin->active());
      activeButton->setEnabled(plugin->hasActiveButton());
      activeButton->setToolTip(tr("Deactivate plugin"));
      connect(activeButton, &QAction::toggled, [this](bool v) { activeToggled(v); } );
      tools->addAction(activeButton);

      onOff = new QAction(*muteSVGIcon, tr("Bypass plugin"), this);
      onOff->setCheckable(true);
      onOff->setChecked(!plugin->on());
      onOff->setEnabled(plugin->hasBypass());
      onOff->setToolTip(tr("Bypass plugin"));
      connect(onOff, &QAction::toggled, [this](bool v) { bypassToggled(v); } );
      tools->addAction(onOff);

      tools->addSeparator();

      QAction* settings = new QAction(*settingsSVGIcon, tr("Plugin settings"), this);
      connect(settings, &QAction::triggered, this, &PluginGui::showSettings);
      tools->addAction(settings);

      fileOpen->setWhatsThis(tr(presetOpenText));
      onOff->setWhatsThis(tr(presetBypassText));
      fileSave->setWhatsThis(tr(presetSaveText));

      QString id;
      id.setNum(plugin->pluginID());
      QString name(MusEGlobal::museGlobalShare + QString("/plugins/") + id + QString(".ui"));
      QFile uifile(name);
      if (uifile.exists())
          constructGUIFromFile(uifile);
      else
          constructGUIFromPluginMetadata();

      _configChangedMetaConn = connect(MusEGlobal::muse, &MusE::configChanged, [this]() { configChanged(); } );
// REMOVE Tim. tmp. Added.
      // _songChangedMetaConn = connect(MusEGlobal::song, &MusECore::Song::songChanged,
      //                                [this](MusECore::SongChangedStruct_t type) { songChanged(type); } );
      // updateWindowTitle();
      }

//---------------------------------------------------------
//   PluginGui
//---------------------------------------------------------

PluginGui::~PluginGui()
      {
      disconnect(_configChangedMetaConn);
// REMOVE Tim. tmp. Added.
      // disconnect(_songChangedMetaConn);

      if (gw)
            delete[] gw;
      if (params)
            delete[] params;
      if (paramsOut)
            delete[] paramsOut;
      }

//---------------------------------------------------------
// construct GUI from *.ui file
//---------------------------------------------------------

void PluginGui::constructGUIFromFile(QFile& uifile) {

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
    CheckBox*    cb_obj;
    Switch*      sw_obj;
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
          const LADSPA_PortRangeHint range = plugin->range(parameter);
          gw[nobj].hint = range.HintDescriptor;
          double lower = 0.0;     // default values
          double upper = 1.0;
          double dlower = lower;
          double dupper = upper;
          double val   = plugin->param(parameter);
          double dval  = val;
          getPluginConvertedValues(range, lower, upper, dlower, dupper, dval);

          if (strcmp(obj->metaObject()->className(), "MusEGui::Slider") == 0) {
                gw[nobj].type = GuiWidgets::SLIDER;
                Slider* s = static_cast<Slider*>(obj);
                s->setId(nobj);
                s->setCursorHoming(true);

                // TODO
                //s->setThumbLength(1);
                //s->setFillThumb(false);

                QFont fnt;
                fnt.setFamily("Sans");
                fnt.setPixelSize(9);
                //fnt.setStyleStrategy(QFont::PreferBitmap);
                fnt.setStyleStrategy(QFont::NoAntialias);
                fnt.setHintingPreference(QFont::PreferVerticalHinting);
                s->setFont(fnt);
                s->setStyleSheet(MusECore::font2StyleSheetFull(fnt));
                s->setScaleBackBone(false);
                s->setSizeHint(200, 8);

                setupControllerWidgets(
                  s, nullptr, nullptr, nullptr, 0,
                  lower, upper,
                  LADSPA_IS_HINT_INTEGER(range.HintDescriptor),
                  LADSPA_IS_HINT_LOGARITHMIC(range.HintDescriptor),
                  false, //MusEGlobal::config.preferMidiVolumeDb,
                  true,
                  0.5, 0.01, 1.0, 2, 2, 3, 20.0,
                  MusEGlobal::config.minSlider, MusEGlobal::config.minMeter,
                  // No suffix needed here.
                  QString());

                s->setValue(val);

                connect(s, QOverload<double, int, int>::of(&Slider::valueChanged), [=](double, int i, int) { guiParamChanged(i); } );
                connect(s, &Slider::sliderPressed, [this](double v, int i) { guiSliderPressed(v, i); } );
                connect(s, &Slider::sliderReleased, [this](double v, int i) { guiSliderReleased(v, i); } );
                connect(s, &Slider::sliderRightClicked, [this](const QPoint &p, int i) { guiSliderRightClicked(p, i); } );
                }
          else if (strcmp(obj->metaObject()->className(), "MusEGui::DoubleLabel") == 0) {
                gw[nobj].type = GuiWidgets::DOUBLE_LABEL;
                dl_obj = static_cast<DoubleLabel*>(obj);
                dl_obj->setId(nobj);
                dl_obj->setAlignment(Qt::AlignCenter);

                setupControllerWidgets(
                  nullptr, dl_obj, nullptr, nullptr, 0,
                  lower, upper,
                  LADSPA_IS_HINT_INTEGER(range.HintDescriptor),
                  LADSPA_IS_HINT_LOGARITHMIC(range.HintDescriptor),
                  false, //MusEGlobal::config.preferMidiVolumeDb,
                  true,
                  0.5, 0.01, 1.0, 2, 2, 3, 20.0,
                  MusEGlobal::config.minSlider, MusEGlobal::config.minMeter,
                  // Use existing suffix.
                  dl_obj->suffix());

                dl_obj->setValue(val);

                connect(dl_obj, &DoubleLabel::valueChanged, [this](double, int i) { guiParamChanged(i); } );
                }
          else if (strcmp(obj->metaObject()->className(), "MusEGui::CheckBox") == 0) {
                gw[nobj].type = GuiWidgets::CHECKBOX;
                gw[nobj].widget->setContextMenuPolicy(Qt::CustomContextMenu);
                cb_obj = static_cast<CheckBox*>(obj);
                cb_obj->setId(nobj);
                cb_obj->setChecked(val != 0.0);

                connect(cb_obj, &CheckBox::checkboxPressed, [this](int i) { guiParamPressed(i); } );
                connect(cb_obj, &CheckBox::checkboxReleased, [this](int i) { guiParamReleased(i); } );
                connect(cb_obj, &CheckBox::checkboxRightClicked, [this](const QPoint& p, int i) { guiSliderRightClicked(p, i); } );
                }
          else if (strcmp(obj->metaObject()->className(), "MusEGui::Switch") == 0) {
                gw[nobj].type = GuiWidgets::SWITCH;
                gw[nobj].widget->setContextMenuPolicy(Qt::CustomContextMenu);
                sw_obj = static_cast<Switch*>(obj);
                sw_obj->setId(nobj);
                sw_obj->setChecked(val != 0.0);

                connect(sw_obj, &Switch::toggleChanged, [this](bool, int i) { guiParamChanged(i); } );
                connect(sw_obj, &Switch::switchPressed, [this](int i) { guiParamPressed(i); } );
                connect(sw_obj, &Switch::switchReleased, [this](int i) { guiParamReleased(i); } );
                connect(sw_obj, &Switch::switchRightClicked, [this](const QPoint& p, int i) { guiSliderRightClicked(p, i); } );
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

//---------------------------------------------------------
// construct GUI from plugin's meta data
//---------------------------------------------------------

void PluginGui::constructGUIFromPluginMetadata() {

    view = new QScrollArea;
    view->setWidgetResizable(true);
    setCentralWidget(view);

    mw = new QWidget(view);
    view->setWidget(mw);

    QVBoxLayout* vbox = new QVBoxLayout(mw);
    vbox->setSizeConstraint(QLayout::SetMinAndMaxSize);

    QGroupBox* groupBox = nullptr;
    QGridLayout* grid = nullptr;


    // input ports
    unsigned long paramCnt  = plugin->parameters();
    params = new GuiParam[paramCnt];

    QFontMetrics fm = fontMetrics();
    int h           = fm.height() + 4;

    QString lastGroup;

    for (unsigned long i = 0; i < paramCnt; ++i) {

        if (!i || plugin->portGroup(i) != lastGroup) {
            if (plugin->portGroup(i).isEmpty()) {
                grid = new QGridLayout();
                grid->setColumnMinimumWidth(0, 100);
                vbox->addLayout(grid);
            } else {
                groupBox = new QGroupBox(plugin->portGroup(i));
                grid = new QGridLayout(groupBox);
                grid->setColumnMinimumWidth(0, 100);
                groupBox->setLayout(grid);
                vbox->addWidget(groupBox);
            }
            lastGroup = plugin->portGroup(i);
        }

        QLabel* label = nullptr;
        LADSPA_PortRangeHint range = plugin->range(i);
        double lower = 0.0;     // default values
        double upper = 1.0;
        double dlower = lower;
        double dupper = upper;
        double val   = plugin->param(i);
        double dval  = val;
        params[i].pressed = false;
        params[i].hint = range.HintDescriptor;
        params[i].label = nullptr;

        getPluginConvertedValues(range, lower, upper, dlower, dupper, dval);

        if (LADSPA_IS_HINT_TOGGLED(range.HintDescriptor)
            || plugin->ctrlValueType(i) == MusECore::CtrlValueType::VAL_BOOL) {

            // TODO: There should be special handling for triggers (=button),
            //   but I can't find any plugin with trigger property (kybos)
            // if (plugin->ctrlIsTrigger(i))...

// Whether to use a checkbox or a switch...
#if 1
            params[i].type = GuiParam::GUI_CHECKBOX;
            label = new QLabel(QString(plugin->paramName(i)), nullptr);
            CheckBox* cb = new CheckBox(mw, i, "param");
            cb->setId(i);
            cb->setChecked(val != 0.0);
            cb->setFixedHeight(h);
            params[i].actuator = cb;

            cb->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
            grid->addWidget(label, i, 0);
            grid->addWidget(cb, i, 1);

            connect(cb, &CheckBox::checkboxPressed, [this](int i) { switchPressed(i); } );
            connect(cb, &CheckBox::checkboxReleased, [this](int i) { switchReleased(i); } );
            connect(cb, &CheckBox::checkboxRightClicked, [this](const QPoint &p, int i) { ctrlRightClicked(p, i); } );
#else
            params[i].type = GuiParam::GUI_SWITCH;
            label = new QLabel(QString(plugin->paramName(i)), nullptr);
            Switch* sw = new Switch(i, mw, "param");
            sw->setChecked(val != 0.0);
            sw->setFixedHeight(h);
            params[i].actuator = sw;

            sw->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
            grid->addWidget(label, i, 0);
            grid->addWidget(sw, i, 1);

            connect(sw, &Switch::toggleChanged, [this](bool v, int i) { switchChanged(v, i); } );
            connect(sw, &Switch::switchPressed, [this](int i) { switchPressed(i); } );
            connect(sw, &Switch::switchReleased, [this](int i) { switchReleased(i); } );
            connect(sw, &Switch::switchRightClicked, [this](const QPoint& p, int i) { ctrlRightClicked(p, i); } );
#endif
        }
        else if (plugin->ctrlValueType(i) == MusECore::CtrlValueType::VAL_ENUM
                 && plugin->ctrlEnumValues(i)) {
            label = new QLabel(QString(plugin->paramName(i)), nullptr);
            params[i].type  = GuiParam::GUI_ENUM;
            ComboBoxPI* cmb = new ComboBoxPI(mw, i, "enum");

            int curItem = -1;
            int cnt = 0;
            for (const auto& it : *plugin->ctrlEnumValues(i)) {
                cmb->addItem(it.second, it.first);
                if (curItem == -1 && it.first == static_cast<float>(val))
                    curItem = cnt;
                cnt++;
            }
            cmb->setCurrentIndex(curItem);

            params[i].actuator = cmb;

            grid->addWidget(label, i, 0);
            grid->addWidget(params[i].actuator, i, 1, 1, 2);

            connect(cmb, QOverload<int>::of(&ComboBoxPI::currentIndexChanged), [=]() { comboChanged(i); } );
            connect(cmb, &ComboBoxPI::rightClicked, [this](const QPoint &p, int i) { ctrlRightClicked(p, i); } );

        }
        else {
            label           = new QLabel(QString(plugin->paramName(i)), nullptr);
            params[i].type  = GuiParam::GUI_SLIDER;
            params[i].label = new DoubleLabel(nullptr);
            params[i].label->setFrame(true);
            params[i].label->setAlignment(Qt::AlignCenter);
            params[i].label->setId(i);

            // Let sliders all have different but unique colors
            // Some prime number magic
            uint j = i+1;
            uint c1 = j * 211  % 256;
            uint c2 = j * j * 137  % 256;
            uint c3 = j * j * j * 43  % 256;
            QColor color(c1, c2, c3);

            Slider* s = new Slider(
              0, "param", Qt::Horizontal, Slider::ScaleRightOrBottom, 8, color, ScaleDraw::TextHighlightNone);

            // TODO
            //s->setThumbLength(1);
            //s->setFillThumb(false);

            QFont fnt;
            fnt.setFamily("Sans");
            fnt.setPixelSize(9);
            fnt.setStyleStrategy(QFont::NoAntialias);
            fnt.setHintingPreference(QFont::PreferVerticalHinting);
            s->setFont(fnt);
            s->setStyleSheet(MusECore::font2StyleSheetFull(fnt));

            s->setScaleBackBone(false);
            s->setCursorHoming(true);
            s->setId(i);
            s->setSizeHint(150, 8);

            setupControllerWidgets(
              s, params[i].label, nullptr, nullptr, 0,
              lower, upper,
              LADSPA_IS_HINT_INTEGER(range.HintDescriptor),
              LADSPA_IS_HINT_LOGARITHMIC(range.HintDescriptor),
              false,
              true,
              0.5, 0.01, 1.0, 2, 2, 3, 20.0,
              MusEGlobal::config.minSlider, MusEGlobal::config.minMeter,
              plugin->unitSymbol(i));

            s->setValue(val);
            params[i].label->setValue(val);

            params[i].actuator = s;

            label->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
            params[i].label->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
            params[i].actuator->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed));
            grid->addWidget(label, i, 0);
            grid->addWidget(params[i].label,    i, 1);
            grid->addWidget(params[i].actuator, i, 2);

            connect(s, QOverload<double, int, int>::of(&Slider::valueChanged),
                    [=](double v, int id, int scroll_mode) { sliderChanged(v, id, scroll_mode); } );
            connect(params[i].label, &DoubleLabel::valueChanged, [this](double v, int i) { labelChanged(v, i); } );
            connect(s, &Slider::sliderPressed, [this](double v, int i) { sliderPressed(v, i); } );
            connect(s, &Slider::sliderReleased, [this](double v, int i) { sliderReleased(v, i); } );
            connect(s, &Slider::sliderRightClicked, [this](const QPoint &p, int i) { ctrlRightClicked(p, i); } );
        }

        if (plugin->ctrlNotOnGui(i)) {
            params[i].actuator->setVisible(false);
            label->setVisible(false);
            if (params[i].label)
                params[i].label->setVisible(false);
        }
    }


    paramCnt  = plugin->parametersOut();
    if (paramCnt > 0) {
        paramsOut = new GuiParam[paramCnt];

        groupBox = new QGroupBox(tr("Output controls"));
        grid = new QGridLayout(groupBox);
        groupBox->setLayout(grid);
        vbox->addWidget(groupBox);

        for (unsigned long i = 0; i < paramCnt; ++i) {
            QLabel* label = nullptr;
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
            label           = new QLabel(QString(plugin->paramOutName(i)), nullptr);
            paramsOut[i].type  = GuiParam::GUI_METER;
            paramsOut[i].textLabel = new DoubleText(nullptr);
            paramsOut[i].textLabel->setTextFormat(Qt::PlainText);
            paramsOut[i].textLabel->setFrameShape(QFrame::Box);
            paramsOut[i].textLabel->setAlignment(Qt::AlignCenter);
            paramsOut[i].textLabel->setId(i);
            paramsOut[i].textLabel->setValue(val);

            Meter* m = new Meter(this);
            m->setOrientation(Qt::Horizontal);
            m->setVUSizeHint(QSize(60, 8));
            m->setFrame(true, Qt::darkGray);
            m->setScaleDist(2);
            m->setTextHighlightMode(ScaleDraw::TextHighlightNone);

            m->setRefreshRate(MusEGlobal::config.guiRefresh);
            m->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
            m->setScaleBackBone(false);
            m->setPrimaryColor(MusEGlobal::config.audioMeterPrimaryColor);

            setupControllerWidgets(
              nullptr, nullptr, paramsOut[i].textLabel, &m, 1,
              lower, upper,
              LADSPA_IS_HINT_INTEGER(range.HintDescriptor),
              LADSPA_IS_HINT_LOGARITHMIC(range.HintDescriptor),
              false, //MusEGlobal::config.preferMidiVolumeDb,
              true,
              0.5, 0.01, 1.0, 2, 2, 3, 20.0,
              MusEGlobal::config.minSlider, MusEGlobal::config.minMeter,
              plugin->unitSymbolOut(i));

            m->setVal(val, val, false);


            QFont fnt;
            fnt.setFamily("Sans");
            fnt.setPixelSize(9);
            fnt.setStyleStrategy(QFont::NoAntialias);
            fnt.setHintingPreference(QFont::PreferVerticalHinting);
            m->setFont(fnt);
            m->setStyleSheet(MusECore::font2StyleSheetFull(fnt));

            paramsOut[i].actuator = m;
            label->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
            paramsOut[i].textLabel->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
            grid->addWidget(label, i, 0);
            grid->addWidget(paramsOut[i].textLabel, i, 1);
            grid->addWidget(paramsOut[i].actuator, i, 2);
        }
    }

    vbox->addStretch(0);
    mw->setLayout(vbox);
}

// REMOVE Tim. tmp. Added.
void PluginGui::updateWindowTitle(const QString& title)
{
  // if(plugin)
  //   setWindowTitle(plugin->titlePrefix() + plugin->name() +
  //     (plugin->uri().isEmpty() ? QString() : QString(" : ") + plugin->uri()));
  setWindowTitle(title);
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
        if (lower <= 0.0)
        {
          dlower = MusEGlobal::config.minSlider;
        }
        else
        {
          dlower = muse_val2db(lower);
        }
        dupper = muse_val2db(upper);
        dval  = muse_val2db(dval);
        }

}

//---------------------------------------------------------
//   configChanged
//---------------------------------------------------------

void PluginGui::configChanged()
{
    if (paramsOut) {
        for (unsigned long i = 0; i < plugin->parametersOut(); ++i) {
            GuiParam* gp = &paramsOut[i];
            const int hint = gp->hint;

            const LADSPA_PortRangeHint range = plugin->rangeOut(i);
            double lower = 0.0;     // default values
            double upper = 32768.0; // Many latency outs have no hints so set this arbitrarily high
            double dlower = lower;
            double dupper = upper;
            const double val   = plugin->paramOut(i);
            double dval  = val;

            getPluginConvertedValues(range, lower, upper, dlower, dupper, dval);

            if (gp->type == GuiParam::GUI_METER) {

                setupControllerWidgets(
                  nullptr, nullptr, paramsOut[i].textLabel, ((Meter**)(&gp->actuator)), 1,
                  lower, upper,
                  LADSPA_IS_HINT_INTEGER(hint),
                  LADSPA_IS_HINT_LOGARITHMIC(hint),
                  false,
                  true,
                  0.5, 0.01, 1.0, 2, 2, 3, 20.0,
                  MusEGlobal::config.minSlider, MusEGlobal::config.minMeter,
                  plugin->unitSymbolOut(i));

                ((Meter*)(gp->actuator))->blockSignals(true);
                paramsOut[i].textLabel->blockSignals(true);

                ((Meter*)(gp->actuator))->setVal(val, val, false);
                ((Meter*)(gp->actuator))->setRefreshRate(MusEGlobal::config.guiRefresh);
                paramsOut[i].textLabel->setValue(val);

                ((Meter*)(gp->actuator))->blockSignals(false);
                paramsOut[i].textLabel->blockSignals(false);
            }
        }
    }

    if (params) {
        for (unsigned long i = 0; i < plugin->parameters(); ++i) {
            GuiParam* gp = &params[i];
            const int hint = gp->hint;

            const LADSPA_PortRangeHint range = plugin->range(i);
            double lower = 0.0;     // default values
            double upper = 1.0;
            double dlower = lower;
            double dupper = upper;
            const double val   = plugin->param(i);
            double dval  = val;

            getPluginConvertedValues(range, lower, upper, dlower, dupper, dval);

            if (gp->type == GuiParam::GUI_SLIDER)
            {

                setupControllerWidgets(
                  ((Slider*)(gp->actuator)), gp->label, nullptr, nullptr, 0,
                  lower, upper,
                  LADSPA_IS_HINT_INTEGER(hint),
                  LADSPA_IS_HINT_LOGARITHMIC(hint),
                  false,
                  true,
                  0.5, 0.01, 1.0, 2, 2, 3, 20.0,
                  MusEGlobal::config.minSlider, MusEGlobal::config.minMeter,
                  plugin->unitSymbol(i));

                gp->actuator->blockSignals(true);
                gp->label->blockSignals(true);

                ((Slider*)(gp->actuator))->setValue(val);
                gp->label->setValue(val);

                gp->actuator->blockSignals(false);
                gp->label->blockSignals(false);
            }
        }
    }
    else if (gw) {
        for (unsigned long i = 0; i < nobj; ++i) {
            QWidget* widget = gw[i].widget;
            const int type = gw[i].type;
            const int hint = gw[i].hint;
            const unsigned long param = gw[i].param;
            const LADSPA_PortRangeHint range = plugin->range(param);
            double lower = 0.0;     // default values
            double upper = 1.0;
            double dlower = lower;
            double dupper = upper;
            const double val = plugin->param(param);
            double dval  = val;

            getPluginConvertedValues(range, lower, upper, dlower, dupper, dval);

            switch(type) {
            case GuiWidgets::SLIDER:
            {

                setupControllerWidgets(
                  ((Slider*)widget), nullptr, nullptr, nullptr, 0,
                  lower, upper,
                  LADSPA_IS_HINT_INTEGER(hint),
                  LADSPA_IS_HINT_LOGARITHMIC(hint),
                  false,
                  true,
                  0.5, 0.01, 1.0, 2, 2, 3, 20.0,
                  MusEGlobal::config.minSlider, MusEGlobal::config.minMeter,
                  // No suffix needed here.
                  QString());

                ((Slider*)widget)->blockSignals(true);
                ((Slider*)widget)->setValue(val);
                ((Slider*)widget)->blockSignals(false);
            }
            break;
            case GuiWidgets::DOUBLE_LABEL:
            {

                setupControllerWidgets(
                  nullptr, ((DoubleLabel*)widget), nullptr, nullptr, 0,
                  lower, upper,
                  LADSPA_IS_HINT_INTEGER(hint),
                  LADSPA_IS_HINT_LOGARITHMIC(hint),
                  false,
                  true,
                  0.5, 0.01, 1.0, 2, 2, 3, 20.0,
                  MusEGlobal::config.minSlider, MusEGlobal::config.minMeter,
                  // Use existing suffix.
                  ((DoubleLabel*)widget)->suffix());

                ((DoubleLabel*)widget)->blockSignals(true);
                ((DoubleLabel*)widget)->setValue(val);
                ((DoubleLabel*)widget)->blockSignals(false);
            }
            break;
            }
        }
    }
}

// REMOVE Tim. tmp. Added.
// //---------------------------------------------------------
// //   songChanged
// //---------------------------------------------------------
//
// void PluginGui::songChanged(MusECore::SongChangedStruct_t type)
// {
//   // Catch when the track name changes or track is moved or the rack position changes.
//   if(type & (SC_TRACK_MODIFIED | SC_TRACK_MOVED | SC_RACK))
//     updateWindowTitle();
// }

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

void PluginGui::sliderPressed(double val, int param)
{
      params[param].pressed = true;
      MusECore::AudioTrack* track = plugin->track();
      int id = plugin->id();
      if(id != -1)
      {
        id = MusECore::genACnum(id, param);
        if(track)
        {
          track->startAutoRecord(id, val);
          track->setPluginCtrlVal(id, val);
        }
      }
      plugin->enableController(param, false);
}

//---------------------------------------------------------
//   ctrlReleased
//---------------------------------------------------------

void PluginGui::sliderReleased(double val, int param)
{
    MusECore::AutomationType at = MusECore::AUTO_OFF;
    MusECore::AudioTrack* track = plugin->track();
    if(track)
      at = track->automationType();

    int id = plugin->id();
    if(track && id != -1)
    {
      id = MusECore::genACnum(id, param);
      track->stopAutoRecord(id, val);
    }

    if (at == MusECore::AUTO_OFF || (at == MusECore::AUTO_READ && MusEGlobal::audio->isPlaying()) ||
        at == MusECore::AUTO_TOUCH)
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
    MusEGlobal::song->execAutomationCtlPopup(plugin->track(), p,
                                             MusECore::MidiAudioCtrlStruct::AudioControl, MusECore::genACnum(id, param));
}

//---------------------------------------------------------
//   switchPressed
//---------------------------------------------------------

void PluginGui::switchPressed(int param)
{
    params[param].pressed = true;
    MusECore::AudioTrack* track = plugin->track();
    int id = plugin->id();
    if (track && id != -1)
    {
        id = MusECore::genACnum(id, param);
        double val = 0.0;
        switch(params[param].type)
        {
          case GuiParam::GUI_CHECKBOX:
            val = (double)((CheckBox*)params[param].actuator)->isChecked();
          break;
          case GuiParam::GUI_SWITCH:
            val = (double)((Switch*)params[param].actuator)->isChecked();
          break;
        }
        track->startAutoRecord(id, val);
        track->setPluginCtrlVal(id, val);
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

      // don't enable controller until transport stopped.
      if ((at == MusECore::AUTO_OFF) || (at == MusECore::AUTO_READ && MusEGlobal::audio->isPlaying()) ||
          (at == MusECore::AUTO_TOUCH && !MusEGlobal::audio->isPlaying()) )
        plugin->enableController(param, true);

      int id = plugin->id();
      if(track && id != -1)
      {
        id = MusECore::genACnum(id, param);
        double val = 0.0;
        switch(params[param].type)
        {
          case GuiParam::GUI_CHECKBOX:
            val = (double)((CheckBox*)params[param].actuator)->isChecked();
          break;
          case GuiParam::GUI_SWITCH:
            val = (double)((Switch*)params[param].actuator)->isChecked();
          break;
        }
        track->stopAutoRecord(id, val);
      }

      params[param].pressed = false;
}

//---------------------------------------------------------
//   sliderChanged
//---------------------------------------------------------

void PluginGui::sliderChanged(double val, int param, int scrollMode)
{
      MusECore::AudioTrack* track = plugin->track();
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
//   switchChanged
//---------------------------------------------------------

void PluginGui::switchChanged(bool val, int param)
{
      MusECore::AudioTrack* track = plugin->track();
      int id = plugin->id();
      if(track && id != -1)
      {
        id = MusECore::genACnum(id, param);
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
//   comboChanged
//---------------------------------------------------------

void PluginGui::comboChanged(unsigned long param)
{
    MusECore::AudioTrack* track = plugin->track();

    ComboBoxPI *c = static_cast<ComboBoxPI*>(params[param].actuator);
    double val = rint( c->currentData().toDouble() );
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
      // Note: Some ladspa plugins (lsp for ex.) put a full ip address like http://... for the label.
      // This is not good for a directory name. It ends up getting split into multiple folders: http / ... / ... /
      // (But still, the file gets saved OK.)
      // Strip away any unusual characters and everything before them, like slashes etc.
      s += MusECore::stripPluginLabel(plugin->pluginLabel());
      s += "/";

      QString fn = getOpenFileName(s, MusEGlobal::preset_file_pattern,
         this, tr("MusE: load preset"), nullptr);
      if (fn.isEmpty())
            return;
      bool popenFlag;
      FILE* f = fileOpen(this, fn, QString(".pre"), "r", popenFlag, true);
      if (f == nullptr)
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
      // Note: Some ladspa plugins (lsp for ex.) put a full ip address like http://... for the label.
      // This is not good for a directory name. It ends up getting split into multiple folders: http / ... / ... /
      // (But still, the file gets saved OK.)
      // Strip away any unusual characters and everything before them, like slashes etc.
      s += MusECore::stripPluginLabel(plugin->pluginLabel());
      s += "/";

      QString fn = getSaveFileName(s, MusEGlobal::preset_file_save_pattern, this,
        tr("MusE: Save preset"));
      if (fn.isEmpty())
            return;
      bool popenFlag;
      FILE* f = fileOpen(this, fn, QString(".pre"), "w", popenFlag, false, true);
      if (f == nullptr)
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
//   activeToggled
//---------------------------------------------------------

void PluginGui::activeToggled(bool val)
      {
// REMOVE Tim. tmp. Added.
//       updateWindowTitle();
      plugin->setActive(!val);
      MusEGlobal::song->update(SC_ROUTE);
      }

//---------------------------------------------------------
//   bypassToggled
//---------------------------------------------------------

void PluginGui::bypassToggled(bool val)
      {
// REMOVE Tim. tmp. Added.
//       updateWindowTitle();
      plugin->setOn(!val);
      MusEGlobal::song->update(SC_ROUTE);
      }

void PluginGui::showSettings()
{
    PluginSettings settingsDialog(plugin, MusEGlobal::config.noPluginScaling, this);
    settingsDialog.setWindowTitle(tr("Plugin Settings"));
    settingsDialog.exec();
}

//---------------------------------------------------------
//   setActive
//---------------------------------------------------------

void PluginGui::setActive(bool val)
{
// REMOVE Tim. tmp. Added.
//       updateWindowTitle();
      activeButton->blockSignals(true);
      activeButton->setChecked(!val);
      activeButton->blockSignals(false);
}

//---------------------------------------------------------
//   setOn
//---------------------------------------------------------

void PluginGui::setOn(bool val)
      {
// REMOVE Tim. tmp. Added.
//       updateWindowTitle();
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
            const double sv = plugin->param(i);
            if (gp->type == GuiParam::GUI_SLIDER) {
                gp->label->blockSignals(true);
                gp->actuator->blockSignals(true);
                gp->label->setValue(sv);
                ((Slider*)(gp->actuator))->setValue(sv);
                gp->label->blockSignals(false);
                gp->actuator->blockSignals(false);
            }
            else if (gp->type == GuiParam::GUI_CHECKBOX) {
                gp->actuator->blockSignals(true);
                ((CheckBox*)(gp->actuator))->setChecked(int(sv));
                gp->actuator->blockSignals(false);
            }
            else if (gp->type == GuiParam::GUI_SWITCH) {
                gp->actuator->blockSignals(true);
                ((Switch*)(gp->actuator))->setChecked(int(sv));
                gp->actuator->blockSignals(false);
            }
            else if (gp->type == GuiParam::GUI_ENUM) {
                const float sv = static_cast<float>(plugin->param(i));
                ComboBoxPI *c = static_cast<ComboBoxPI*>(gp->actuator);
                const int idx = c->findData(sv);
                gp->actuator->blockSignals(true);
                c->setCurrentIndex(idx);
                gp->actuator->blockSignals(false);
            }
        }
    }
    else if (gw) {
        for (unsigned long i = 0; i < nobj; ++i) {
            QWidget* widget = gw[i].widget;
            const int type = gw[i].type;
            const unsigned long param = gw[i].param;
            const double val = plugin->param(param);
            widget->blockSignals(true);
            switch(type) {
            case GuiWidgets::SLIDER:
                ((Slider*)widget)->setValue(val);    // Note conversion to double
                break;
            case GuiWidgets::DOUBLE_LABEL:
                ((DoubleLabel*)widget)->setValue(val);   // Note conversion to double
                break;
            case GuiWidgets::CHECKBOX:
                ((CheckBox*)widget)->setChecked(int(val));
                break;
            case GuiWidgets::SWITCH:
                ((Switch*)widget)->setChecked(int(val));
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
                const double lv = plugin->paramOut(i);
                ((Meter*)(gp->actuator))->setVal(lv, lv, false);
                gp->textLabel->setValue(lv);
            }
        }
    }

    if (params) {
        for (unsigned long i = 0; i < plugin->parameters(); ++i) {
            GuiParam* gp = &params[i];
            const double v = plugin->track()->controller()->value(MusECore::genACnum(plugin->id(), i),
                                                            MusEGlobal::audio->curFramePos(),
                                                            !MusEGlobal::automation ||
                                                            plugin->track()->automationType() == MusECore::AUTO_OFF ||
                                                            !plugin->controllerEnabled(i));
            if (gp->type == GuiParam::GUI_SLIDER) {
                gp->label->blockSignals(true);
                gp->actuator->blockSignals(true);
                ((Slider*)(gp->actuator))->setValue(v);
                gp->label->setValue(v);
                gp->actuator->blockSignals(false);
                gp->label->blockSignals(false);
            }
            else if (gp->type == GuiParam::GUI_CHECKBOX) {
                if(gp->pressed) // Inhibit the controller stream if control is currently pressed.
                    continue;
                const bool b = (int)v;
                if(((CheckBox*)(gp->actuator))->isChecked() != b)
                {
                    gp->actuator->blockSignals(true);
                    ((CheckBox*)(gp->actuator))->setChecked(b);
                    gp->actuator->blockSignals(false);
                }
            }
            else if (gp->type == GuiParam::GUI_SWITCH) {
                if(gp->pressed) // Inhibit the controller stream if control is currently pressed.
                    continue;
                const bool b = (int)v;
                if(((Switch*)(gp->actuator))->isChecked() != b)
                {
                    gp->actuator->blockSignals(true);
                    ((Switch*)(gp->actuator))->setChecked(b);
                    gp->actuator->blockSignals(false);
                }
            }
            else if (gp->type == GuiParam::GUI_ENUM) {
                if(gp->pressed) // Inhibit the controller stream if control is currently pressed.
                    continue;
                const float sv = static_cast<float>(v);
                ComboBoxPI *c = static_cast<ComboBoxPI*>(gp->actuator);
                if (c->currentData().toFloat() != sv)
                {
                    int idx = c->findData(sv);
                    gp->actuator->blockSignals(true);
                    c->setCurrentIndex(idx);
                    gp->actuator->blockSignals(false);
                }
            }
        }
    }
    else if (gw) {
        for (unsigned long i = 0; i < nobj; ++i) {
            QWidget* widget = gw[i].widget;
            const int type = gw[i].type;
            const unsigned long param = gw[i].param;
            const double v = plugin->track()->controller()->value(MusECore::genACnum(plugin->id(), param),
                                                            MusEGlobal::audio->curFramePos(),
                                                            !MusEGlobal::automation ||
                                                            plugin->track()->automationType() == MusECore::AUTO_OFF ||
                                                            !plugin->controllerEnabled(param));
            widget->blockSignals(true);
            switch(type) {
            case GuiWidgets::SLIDER:
                ((Slider*)widget)->setValue(v);
            break;
            case GuiWidgets::DOUBLE_LABEL:
                ((DoubleLabel*)widget)->setValue(v);
            break;
            case GuiWidgets::CHECKBOX:
            {
                if(gw[i].pressed) // Inhibit the controller stream if control is currently pressed.
                    continue;
                const bool b = (bool)v;
                if(((CheckBox*)widget)->isChecked() != b)
                    ((CheckBox*)widget)->setChecked(b);
            }
            break;
            case GuiWidgets::SWITCH:
            {
                if(gw[i].pressed) // Inhibit the controller stream if control is currently pressed.
                    continue;
                const bool b = (bool)v;
                if(((Switch*)widget)->isChecked() != b)
                    ((Switch*)widget)->setChecked(b);
            }
                break;
            case GuiWidgets::QCOMBOBOX:
            {
                if(gw[i].pressed) // Inhibit the controller stream if control is currently pressed.
                    continue;
                const int n = (int)v;
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
      const unsigned long param  = gw[idx].param;
      const int type   = gw[idx].type;

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
            case GuiWidgets::CHECKBOX:
                  val = double(((CheckBox*)w)->isChecked());
                  break;
            case GuiWidgets::SWITCH:
                  val = double(((Switch*)w)->isChecked());
                  break;
            case GuiWidgets::QCOMBOBOX:
                  val = double(((QComboBox*)w)->currentIndex());
                  break;
            }

      int id = plugin->id();
      if(track && id != -1)
      {
          id = MusECore::genACnum(id, param);
          switch(type)
          {
             case GuiWidgets::DOUBLE_LABEL:
             case GuiWidgets::CHECKBOX:
             case GuiWidgets::SWITCH:
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
      QWidget *w = gw[idx].widget;
      int type   = gw[idx].type;

      MusECore::AudioTrack* track = plugin->track();
      int id = plugin->id();
      if(track && id != -1)
      {
        id = MusECore::genACnum(id, param);
        // NOTE: For this to be of any use, the freeverb gui 2142.ui
        //  would have to be used, and changed to use CheckBox and ComboBox
        //  instead of QCheckBox and QComboBox, since both of those would
        //  need customization (Ex. QCheckBox doesn't check on click). RECHECK: Qt4 it does?
        switch(type) {
              case GuiWidgets::CHECKBOX:
              {
                      const double val = (double)((CheckBox*)w)->isChecked();
                      track->startAutoRecord(id, val);
                      track->setPluginCtrlVal(id, val);
                    break;
              }
              case GuiWidgets::SWITCH:
              {
                      const double val = (double)((Switch*)w)->isChecked();
                      track->startAutoRecord(id, val);
                      track->setPluginCtrlVal(id, val);
                    break;
              }
  //             case GuiWidgets::QCOMBOBOX:
  //             {
  //                     const double val = (double)((QComboBox*)w)->currentIndex();
  //                     track->startAutoRecord(id, val);
  //                     track->setPluginCtrlVal(id, val);
  //                   break;
  //             }
              }

        }
        plugin->enableController(param, false);
      }

//---------------------------------------------------------
//   guiParamReleased
//---------------------------------------------------------

void PluginGui::guiParamReleased(unsigned long int idx)
      {
      unsigned long param  = gw[idx].param;
      QWidget *w = gw[idx].widget;
      int type   = gw[idx].type;

      MusECore::AutomationType at = MusECore::AUTO_OFF;
      MusECore::AudioTrack* track = plugin->track();
      if(track)
        at = track->automationType();

      // Special for switch - don't enable controller until transport stopped.
      if ((at == MusECore::AUTO_OFF) || (at == MusECore::AUTO_READ && MusEGlobal::audio->isPlaying()) ||
          (at == MusECore::AUTO_TOUCH && (type != GuiWidgets::CHECKBOX ||
                                !MusEGlobal::audio->isPlaying()) ) )
        plugin->enableController(param, true);

      int id = plugin->id();
      if(track && id != -1)
      {
        id = MusECore::genACnum(id, param);
        // NOTE: For this to be of any use, the freeverb gui 2142.ui
        //  would have to be used, and changed to use CheckBox and ComboBox
        //  instead of QCheckBox and QComboBox, since both of those would
        //  need customization (Ex. QCheckBox doesn't check on click).  // RECHECK Qt4 it does?

        switch(type) {
              case GuiWidgets::CHECKBOX:
              {
                    const double val = (double)((CheckBox*)w)->isChecked();
                    track->stopAutoRecord(id, val);
                    break;
              }
              case GuiWidgets::SWITCH:
              {
                    const double val = (double)((Switch*)w)->isChecked();
                    track->stopAutoRecord(id, val);
                    break;
              }
  //             case GuiWidgets::QCOMBOBOX:
  //             {
  //                   const double val = (double)((QComboBox*)w)->currentIndex();
  //                   track->stopAutoRecord(id, val);
  //                   break;
  //             }
              }
      }

      gw[idx].pressed = false;
      }

//---------------------------------------------------------
//   guiSliderPressed
//---------------------------------------------------------

void PluginGui::guiSliderPressed(double val, unsigned long int idx)
{
      gw[idx].pressed = true;
      const unsigned long param  = gw[idx].param;
      MusECore::AudioTrack* track = plugin->track();
      int id = plugin->id();
      if(track && id != -1)
      {
        id = MusECore::genACnum(id, param);
        track->startAutoRecord(id, val);
        track->setPluginCtrlVal(id, val);
      }
      plugin->enableController(param, false);
}

//---------------------------------------------------------
//   guiSliderReleased
//---------------------------------------------------------

void PluginGui::guiSliderReleased(double val, unsigned long int idx)
      {
      const int param  = gw[idx].param;
      MusECore::AutomationType at = MusECore::AUTO_OFF;
      MusECore::AudioTrack* track = plugin->track();
      if(track)
        at = track->automationType();

      int id = plugin->id();

      if(track && id != -1)
      {
        id = MusECore::genACnum(id, param);
        track->stopAutoRecord(id, val);
      }

      if (at == MusECore::AUTO_OFF || (at == MusECore::AUTO_READ && MusEGlobal::audio->isPlaying()) ||
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
    MusEGlobal::song->execAutomationCtlPopup(plugin->track(), p,
                                             MusECore::MidiAudioCtrlStruct::AudioControl, MusECore::genACnum(id, param));
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

PluginLoader::PluginLoader(QObject * parent) : QUiLoader(parent) {}

QWidget* PluginLoader::createWidget(const QString & className, QWidget * parent, const QString & name)
{
  if(className == QString("MusEGui::DoubleLabel"))
    return new DoubleLabel(parent, name.toLatin1().constData());
  if(className == QString("MusEGui::Slider"))
    return new Slider(parent, name.toLatin1().constData(), Qt::Horizontal,
      Slider::ScaleRightOrBottom, 8, QColor(), ScaleDraw::TextHighlightNone);
  if(className == QString("MusEGui::CheckBox"))
    return new CheckBox(parent, -1, name.toLatin1().constData());
  if(className == QString("MusEGui::Switch"))
    return new Switch(-1, parent, name.toLatin1().constData());

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
                        MusECore::Xml::Token tok = xml.parse();
                        if (tok == MusECore::Xml::Error || tok == MusECore::Xml::End)
                            break;

                        const QString& tg = xml.s1();
                        switch (tok)
                        {
                            case MusECore::Xml::TagStart:
                                if (tg == "lib")
                                {
                                    lib=xml.parse1();
                                    read_lib=true;
                                }
                                else if (tg == "label")
                                {
                                    label=xml.parse1();
                                    read_label=true;
                                }
                                else if (tg == "group")
                                    groups.insert(xml.parseInt());
                                else
                                    xml.unknown("readPluginGroupMap");
                                break;

                            case MusECore::Xml::TagEnd:
                                if (tg == "entry")
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
