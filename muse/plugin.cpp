//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: plugin.cpp,v 1.21.2.23 2009/12/15 22:07:12 spamatica Exp $
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
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

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <cmath>
#include <math.h>
#include <sys/stat.h>

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QCursor>
#include <QDir>
#include <QFile>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMainWindow>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QSignalMapper>
#include <QSizePolicy>
#include <QScrollArea>
#include <QSpacerItem>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QWhatsThis>

#include "globals.h"
#include "globaldefs.h"
#include "gconfig.h"
#include "filedialog.h"
#include "slider.h"
#include "midictrl.h"
#include "plugin.h"
#include "controlfifo.h"
#include "xml.h"
#include "icons.h"
#include "song.h"
#include "doublelabel.h"
#include "fastlog.h"
#include "checkbox.h"
#include "verticalmeter.h"

#include "audio.h"
#include "al/dsp.h"

#include "config.h"

// Turn on debugging messages.
//#define PLUGIN_DEBUGIN 

// Turn on constant stream of debugging messages.
//#define PLUGIN_DEBUGIN_PROCESS 

namespace MusEGlobal {
MusECore::PluginList plugins;
}

namespace MusEGui {
int PluginDialog::selectedPlugType = 0;
QStringList PluginDialog::sortItems = QStringList();
QRect PluginDialog::geometrySave = QRect();
QByteArray PluginDialog::listSave = QByteArray();
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

//---------------------------------------------------------
//   Plugin
//---------------------------------------------------------

Plugin::Plugin(QFileInfo* f, const LADSPA_Descriptor* d, bool isDssi, bool isDssiSynth)
{
  _isDssi = isDssi;
  _isDssiSynth = isDssiSynth;
  #ifdef DSSI_SUPPORT
  dssi_descr = NULL;
  #endif
  
  fi = *f;
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
  
  _inPlaceCapable = !LADSPA_IS_INPLACE_BROKEN(d->Properties);
  
  // By T356. Blacklist vst plugins in-place configurable for now. At one point they 
  //   were working with in-place here, but not now, and RJ also reported they weren't working.
  // Fixes problem with vst plugins not working or feeding back loudly.
  // I can only think of two things that made them stop working:
  // 1): I switched back from Jack-2 to Jack-1
  // 2): I changed winecfg audio to use Jack instead of ALSA.
  // Will test later...
  // Possibly the first one because under Mandriva2007.1 (Jack-1), no matter how hard I tried, 
  //  the same problem existed. It may have been when using Jack-2 with Mandriva2009 that they worked.
  // Apparently the plugins are lying about their in-place capability.
  // Quote:
  /* Property LADSPA_PROPERTY_INPLACE_BROKEN indicates that the plugin
    may cease to work correctly if the host elects to use the same data
    location for both input and output (see connect_port()). This
    should be avoided as enabling this flag makes it impossible for
    hosts to use the plugin to process audio `in-place.' */
  // Examination of all my ladspa and vst synths and effects plugins showed only one - 
  //  EnsembleLite (EnsLite VST) has the flag set, but it is a vst synth and is not involved here!
  // Yet many (all?) ladspa vst effect plugins exhibit this problem.  
  // Changed by Tim. p3.3.14
  // Hack: Special Flag required for example for control processing.
  _isDssiVst = fi.completeBaseName() == QString("dssi-vst");
  // Hack: Blacklist vst plugins in-place, configurable for now. 
  if ((_inports != _outports) || (_isDssiVst && !MusEGlobal::config.vstInPlace))
        _inPlaceCapable = false;
}

Plugin::~Plugin()
{
  if(plugin)
    delete plugin;
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
  
  if(newref == 0) 
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
      
      _inPlaceCapable = !LADSPA_IS_INPLACE_BROKEN(plugin->Properties);
      
      // Hack: Special flag required for example for control processing.
      _isDssiVst = fi.completeBaseName() == QString("dssi-vst");
      // Hack: Blacklist vst plugins in-place, configurable for now. 
      if ((_inports != _outports) || (_isDssiVst && !MusEGlobal::config.vstInPlace))
            _inPlaceCapable = false;
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

float Plugin::defaultValue(unsigned long port) const
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

//---------------------------------------------------------
//   loadPluginLib
//---------------------------------------------------------

static void loadPluginLib(QFileInfo* fi)
{
  void* handle = dlopen(fi->filePath().toAscii().constData(), RTLD_NOW);
  if (handle == 0) {
        fprintf(stderr, "dlopen(%s) failed: %s\n",
           fi->filePath().toAscii().constData(), dlerror());
        return;
        }

  #ifdef DSSI_SUPPORT
  DSSI_Descriptor_Function dssi = (DSSI_Descriptor_Function)dlsym(handle, "dssi_descriptor");
  if(dssi)
  {
    const DSSI_Descriptor* descr;
    for (unsigned long i = 0;; ++i)  
    {
      descr = dssi(i);
      if (descr == 0)
            break;
      
      // Make sure it doesn't already exist.
      if(MusEGlobal::plugins.find(fi->completeBaseName(), QString(descr->LADSPA_Plugin->Label)) != 0)
        continue;

      #ifdef PLUGIN_DEBUGIN 
      fprintf(stderr, "loadPluginLib: dssi effect name:%s inPlaceBroken:%d\n", descr->LADSPA_Plugin->Name, LADSPA_IS_INPLACE_BROKEN(descr->LADSPA_Plugin->Properties));
      #endif
    
      bool is_synth = descr->run_synth || descr->run_synth_adding 
                  || descr->run_multiple_synths || descr->run_multiple_synths_adding; 
      if(MusEGlobal::debugMsg)
        fprintf(stderr, "loadPluginLib: adding dssi effect plugin:%s name:%s label:%s synth:%d\n", 
                fi->filePath().toLatin1().constData(), 
                descr->LADSPA_Plugin->Name, descr->LADSPA_Plugin->Label,
                is_synth
                );
    
      MusEGlobal::plugins.add(fi, descr->LADSPA_Plugin, true, is_synth);
    }      
  }
  else
  #endif
  {
    LADSPA_Descriptor_Function ladspa = (LADSPA_Descriptor_Function)dlsym(handle, "ladspa_descriptor");
    if(!ladspa) 
    {
      const char *txt = dlerror();
      if(txt) 
      {
        fprintf(stderr,
              "Unable to find ladspa_descriptor() function in plugin "
              "library file \"%s\": %s.\n"
              "Are you sure this is a LADSPA plugin file?\n",
              fi->filePath().toAscii().constData(),
              txt);
      }
      dlclose(handle);
      return;
    }
    
    const LADSPA_Descriptor* descr;
    for (unsigned long i = 0;; ++i)       
    {
      descr = ladspa(i);
      if (descr == NULL)
            break;
      
      // Make sure it doesn't already exist.
      if(MusEGlobal::plugins.find(fi->completeBaseName(), QString(descr->Label)) != 0)
        continue;
        
      #ifdef PLUGIN_DEBUGIN 
      fprintf(stderr, "loadPluginLib: ladspa effect name:%s inPlaceBroken:%d\n", descr->Name, LADSPA_IS_INPLACE_BROKEN(descr->Properties));
      #endif
      
      if(MusEGlobal::debugMsg)
        fprintf(stderr, "loadPluginLib: adding ladspa plugin:%s name:%s label:%s\n", fi->filePath().toLatin1().constData(), descr->Name, descr->Label);
      MusEGlobal::plugins.add(fi, descr);
    }
  }  
  
  dlclose(handle);
}

//---------------------------------------------------------
//   loadPluginDir
//---------------------------------------------------------

static void loadPluginDir(const QString& s)
      {
      if (MusEGlobal::debugMsg)
            printf("scan ladspa plugin dir <%s>\n", s.toLatin1().constData());
      QDir pluginDir(s, QString("*.so")); // ddskrjo
      if (pluginDir.exists()) {
            QFileInfoList list = pluginDir.entryInfoList();
	    QFileInfoList::iterator it=list.begin();
            while(it != list.end()) {
                  loadPluginLib(&*it);
                  ++it;
                  }
            }
      }

//---------------------------------------------------------
//   initPlugins
//---------------------------------------------------------

void initPlugins()
      {
      loadPluginDir(MusEGlobal::museGlobalLib + QString("/plugins"));

      const char* p = 0;
      
      // Take care of DSSI plugins first...
      #ifdef DSSI_SUPPORT
      const char* dssiPath = getenv("DSSI_PATH");
      if (dssiPath == 0)
            dssiPath = "/usr/local/lib64/dssi:/usr/lib64/dssi:/usr/local/lib/dssi:/usr/lib/dssi";
      p = dssiPath;
      while (*p != '\0') {
            const char* pe = p;
            while (*pe != ':' && *pe != '\0')
                  pe++;

            int n = pe - p;
            if (n) {
                  char* buffer = new char[n + 1];
                  strncpy(buffer, p, n);
                  buffer[n] = '\0';
                  loadPluginDir(QString(buffer));
                  delete[] buffer;
                  }
            p = pe;
            if (*p == ':')
                  p++;
            }
      #endif
      
      // Now do LADSPA plugins...
      const char* ladspaPath = getenv("LADSPA_PATH");
      if (ladspaPath == 0)
            ladspaPath = "/usr/local/lib64/ladspa:/usr/lib64/ladspa:/usr/local/lib/ladspa:/usr/lib/ladspa";
      p = ladspaPath;
      
      if(MusEGlobal::debugMsg)
        fprintf(stderr, "loadPluginDir: ladspa path:%s\n", ladspaPath);
      
      while (*p != '\0') {
            const char* pe = p;
            while (*pe != ':' && *pe != '\0')
                  pe++;

            int n = pe - p;
            if (n) {
                  char* buffer = new char[n + 1];
                  strncpy(buffer, p, n);
                  buffer[n] = '\0';
                  if(MusEGlobal::debugMsg)
                    fprintf(stderr, "loadPluginDir: loading ladspa dir:%s\n", buffer);
                  
                  loadPluginDir(QString(buffer));
                  delete[] buffer;
                  }
            p = pe;
            if (*p == ':')
                  p++;
            }
      }

//---------------------------------------------------------
//   find
//---------------------------------------------------------

Plugin* PluginList::find(const QString& file, const QString& name)
      {
      for (iPlugin i = begin(); i != end(); ++i) {
            if ((file == i->lib()) && (name == i->label()))
                  return &*i;
            }

      return 0;
      }

//---------------------------------------------------------
//   Pipeline
//---------------------------------------------------------

Pipeline::Pipeline()
   : std::vector<PluginI*>()
      {
      for (int i = 0; i < MAX_CHANNELS; ++i)
            posix_memalign((void**)(buffer + i), 16, sizeof(float) * MusEGlobal::segmentSize);
      
      for (int i = 0; i < PipelineDepth; ++i)
            push_back(0);
      }

//---------------------------------------------------------
//   Pipeline copy constructor
//---------------------------------------------------------

Pipeline::Pipeline(const Pipeline& /*p*/)
   : std::vector<PluginI*>()
      {
      for (int i = 0; i < MAX_CHANNELS; ++i)
            posix_memalign((void**)(buffer + i), 16, sizeof(float) * MusEGlobal::segmentSize);
      
      // TODO: Copy plug-ins !
      for (int i = 0; i < PipelineDepth; ++i)
            push_back(0);
      }

//---------------------------------------------------------
//   ~Pipeline
//---------------------------------------------------------

Pipeline::~Pipeline()
      {
      removeAll();
      for (int i = 0; i < MAX_CHANNELS; ++i)
          if(buffer[i])
            ::free(buffer[i]);
      }

//---------------------------------------------------------
//   addScheduledControlEvent
//   track_ctrl_id is the fully qualified track audio controller number
//   Returns true if event cannot be delivered
//---------------------------------------------------------

bool Pipeline::addScheduledControlEvent(int track_ctrl_id, float val, unsigned frame) 
{
  // If a track controller, or the special dssi synth controller block, just return.
  if(track_ctrl_id < AC_PLUGIN_CTL_BASE || track_ctrl_id >= (int)genACnum(MAX_PLUGINS, 0)) 
    return true;
  int rack_idx = (track_ctrl_id - AC_PLUGIN_CTL_BASE) >> AC_PLUGIN_CTL_BASE_POW;
  for (int i = 0; i < PipelineDepth; ++i)
  {
    PluginI* p = (*this)[i];
    if(p && p->id() == rack_idx)
      return p->addScheduledControlEvent(track_ctrl_id & AC_PLUGIN_CTL_ID_MASK, val, frame);
  }
  return true;
}
      
//---------------------------------------------------------
//   controllersEnabled
//   Returns whether automation control stream is enabled or disabled. 
//   Used during automation recording to inhibit gui controls
//---------------------------------------------------------

void Pipeline::controllersEnabled(int track_ctrl_id, bool* en1, bool* en2)
{
  // If a track controller, or the special dssi synth controller block, just return.
  if(track_ctrl_id < AC_PLUGIN_CTL_BASE || track_ctrl_id >= (int)genACnum(MAX_PLUGINS, 0)) 
    return;
  int rack_idx = (track_ctrl_id - AC_PLUGIN_CTL_BASE) >> AC_PLUGIN_CTL_BASE_POW;
  for (int i = 0; i < PipelineDepth; ++i)
  {
    PluginI* p = (*this)[i];
    if(p && p->id() == rack_idx)
    {
      if(en1)
        *en1 = p->controllerEnabled(track_ctrl_id & AC_PLUGIN_CTL_ID_MASK);
      if(en2)
        *en2 = p->controllerEnabled2(track_ctrl_id & AC_PLUGIN_CTL_ID_MASK);
      return;
    }
  }
}

//---------------------------------------------------------
//   enableController
//   Enable or disable gui automation control stream. 
//   Used during automation recording to inhibit gui controls
//---------------------------------------------------------

void Pipeline::enableController(int track_ctrl_id, bool en) 
{
  // If a track controller, or the special dssi synth controller block, just return.
  if(track_ctrl_id < AC_PLUGIN_CTL_BASE || track_ctrl_id >= (int)genACnum(MAX_PLUGINS, 0)) 
    return;
  int rack_idx = (track_ctrl_id - AC_PLUGIN_CTL_BASE) >> AC_PLUGIN_CTL_BASE_POW;
  for (int i = 0; i < PipelineDepth; ++i)
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
      for (int i = 0; i < PipelineDepth; ++i)
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
      for (int i = 0; i < PipelineDepth; ++i)
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
      return QString("empty");
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
          
          if((*this)[idx])
            (*this)[idx]->setID(idx);
            
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
          
          if((*this)[idx])
            (*this)[idx]->setID(idx);
          
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

//---------------------------------------------------------
//   has_dssi_ui
//---------------------------------------------------------

bool Pipeline::has_dssi_ui(int idx) const
{
  PluginI* p = (*this)[idx];
  if(p)
    return !p->dssi_ui_filename().isEmpty();
        
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

void Pipeline::showNativeGui(int idx, bool flag)
      {
      #ifdef OSC_SUPPORT
      PluginI* p = (*this)[idx];
      if (p)
            p->oscIF().oscShowGui(flag);
      #endif      
      }

//---------------------------------------------------------
//   deleteGui
//---------------------------------------------------------

void Pipeline::deleteGui(int idx)
{
  if(idx >= PipelineDepth)
    return;
  PluginI* p = (*this)[idx];
  if(p)
    p->deleteGui();
}

//---------------------------------------------------------
//   deleteAllGuis
//---------------------------------------------------------

void Pipeline::deleteAllGuis()
{
  for(int i = 0; i < PipelineDepth; i++)
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
            return p->nativeGuiVisible();
      return false;
      }

//---------------------------------------------------------
//   apply
//   If ports is 0, just process controllers only, not audio (do not 'run').
//---------------------------------------------------------

void Pipeline::apply(unsigned long ports, unsigned long nframes, float** buffer1)
{
      bool swap = false;

      for (iPluginI ip = begin(); ip != end(); ++ip) {
            PluginI* p = *ip;
            
            if(p)
            {
              if (p->on()) 
              {
                if (p->inPlaceCapable()) 
                {
                      if (swap)
                            p->apply(nframes, ports, buffer, buffer);     
                      else
                            p->apply(nframes, ports, buffer1, buffer1);   
                }
                else 
                {
                      if (swap)
                            p->apply(nframes, ports, buffer, buffer1);    
                      else
                            p->apply(nframes, ports, buffer1, buffer);    
                      swap = !swap;
                }
              }
              else
              {
                p->apply(nframes, 0, 0, 0); // Do not process (run) audio, process controllers only.    
              }
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
//   addScheduledControlEvent
//   i is the specific index of the control input port
//   Returns true if event cannot be delivered
//---------------------------------------------------------

bool PluginIBase::addScheduledControlEvent(unsigned long i, float val, unsigned frame) 
{ 
  if(i >= parameters())
  {
    printf("PluginIBase::addScheduledControlEvent param number %lu out of range of ports:%lu\n", i, parameters());
    return true;
  }
  ControlEvent ce;
  ce.unique = false;
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
      controlPorts      = 0;
      controlOutPorts   = 0;
      //_gui              = 0;
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
      
      // remove old instances:
      deactivate();
      delete[] handle;
      instances = ni;
      handle    = new LADSPA_Handle[instances];
      for (int i = 0; i < instances; ++i) {
            handle[i] = _plugin->instantiate();
            if (handle[i] == NULL) {
                  printf("cannot instantiate instance %d\n", i);
                  return;
                  }
            }
      
      unsigned long curPort = 0;      
      unsigned long curOutPort = 0;
      unsigned long ports   = _plugin->ports();
      for (unsigned long k = 0; k < ports; ++k) 
      {
            LADSPA_PortDescriptor pd = _plugin->portd(k);
            if (pd & LADSPA_PORT_CONTROL) 
            {
                  if(pd & LADSPA_PORT_INPUT) 
                  {
                    for (int i = 0; i < instances; ++i)
                          _plugin->connectPort(handle[i], k, &controls[curPort].val);
                    controls[curPort].idx = k;
                    ++curPort;
                  }
                  else  
                  if(pd & LADSPA_PORT_OUTPUT) 
                  {
                    for (int i = 0; i < instances; ++i)
                          _plugin->connectPort(handle[i], k, &controlsOut[curOutPort].val);
                    controlsOut[curOutPort].idx = k;
                    ++curOutPort;
                  }
            }
      }
      
      activate();
}

//---------------------------------------------------------
//   setParam
//---------------------------------------------------------

void PluginI::setParam(unsigned long i, float val) 
{ 
  addScheduledControlEvent(i, val, MusEGlobal::audio->curFrame());
}     

//---------------------------------------------------------
//   defaultValue
//---------------------------------------------------------

float PluginI::defaultValue(unsigned long param) const
{
  if(param >= controlPorts)
    return 0.0;
  
  return _plugin->defaultValue(controls[param].idx);
}

LADSPA_Handle Plugin::instantiate() 
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
      
      _plugin->incReferences(1);

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
      {
        #ifdef PLUGIN_DEBUGIN 
        fprintf(stderr, "PluginI::initPluginInstance instance:%d\n", i);
        #endif
        
        handle[i] = _plugin->instantiate();
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
      
      unsigned long curPort = 0;
      unsigned long curOutPort = 0;
      for(unsigned long k = 0; k < ports; ++k) 
      {
        LADSPA_PortDescriptor pd = _plugin->portd(k);
        if(pd & LADSPA_PORT_CONTROL) 
        {
          if(pd & LADSPA_PORT_INPUT)
          {
            float val = _plugin->defaultValue(k);    
            controls[curPort].val    = val;
            controls[curPort].tmpVal = val;
            controls[curPort].enCtrl  = true;
            controls[curPort].en2Ctrl = true;
            ++curPort;
          }
          else
          if(pd & LADSPA_PORT_OUTPUT)
          {
            controlsOut[curOutPort].val     = 0.0;
            controlsOut[curOutPort].tmpVal  = 0.0;
            controlsOut[curOutPort].enCtrl  = false;
            controlsOut[curOutPort].en2Ctrl = false;
            ++curOutPort;
          }
        }
      }
      curPort = 0;
      curOutPort = 0;
      for(unsigned long k = 0; k < ports; ++k) 
      {
        LADSPA_PortDescriptor pd = _plugin->portd(k);
        if(pd & LADSPA_PORT_CONTROL) 
        {
          if(pd & LADSPA_PORT_INPUT)
          {
            for(int i = 0; i < instances; ++i)
              _plugin->connectPort(handle[i], k, &controls[curPort].val);
            controls[curPort].idx = k;
            ++curPort;
          }
          else
          if(pd & LADSPA_PORT_OUTPUT)
          {
            for(int i = 0; i < instances; ++i)
              _plugin->connectPort(handle[i], k, &controlsOut[curOutPort].val);
            controlsOut[curOutPort].idx = k;
            ++curOutPort;
          }
        }
      }
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
                        _plugin->connectPort(handle[i], k, src[port] + offset);     
                        port = (port + 1) % ports;
                        }
                  }
            }
      port = 0;
      for (int i = 0; i < instances; ++i) {
            for (unsigned long k = 0; k < _plugin->ports(); ++k) {
                  if (isAudioOut(k)) {
                        _plugin->connectPort(handle[i], k, dst[port] + offset);     
                        port = (port + 1) % ports;  // overwrite output?
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
//   setControl
//    set plugin instance controller value by name
//---------------------------------------------------------

bool PluginI::setControl(const QString& s, float val)
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
      xml.tag(level++, "plugin file=\"%s\" label=\"%s\" channel=\"%d\"",
         Xml::xmlString(_plugin->lib()).toLatin1().constData(), Xml::xmlString(_plugin->label()).toLatin1().constData(), channel);
         
      for (unsigned long i = 0; i < controlPorts; ++i) {          
            unsigned long idx = controls[i].idx;
            QString s("control name=\"%1\" val=\"%2\" /");
            xml.tag(level, s.arg(Xml::xmlString(_plugin->portName(idx)).toLatin1().constData()).arg(controls[i].tmpVal).toLatin1().constData());
            }
      if (_on == false)
            xml.intTag(level, "on", _on);
      if (guiVisible()) {
            xml.intTag(level, "gui", 1);
            xml.geometryTag(level, "geometry", _gui);
            }
      if (nativeGuiVisible())
            xml.intTag(level, "nativegui", 1);

      xml.tag(level--, "/plugin");
      }

//---------------------------------------------------------
//   loadControl
//---------------------------------------------------------

bool PluginI::loadControl(Xml& xml)
      {
      QString file;
      QString label;
      QString name("mops");
      float val = 0.0;

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
                              val = xml.s2().toFloat();     
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
                              _plugin = MusEGlobal::plugins.find(file, label);
                              
                              if (_plugin)
                              {
                                 if(initPluginInstance(_plugin, channel)) {
                                    _plugin = 0;
                                    xml.parse1();
                                    printf("Error initializing plugin instance (%s, %s)\n",
                                       file.toLatin1().constData(), label.toLatin1().constData());
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
                        else if (tag == "gui") {
                              bool flag = xml.parseInt();
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
                              if (_gui) {
                                    _gui->resize(r.size());
                                    _gui->move(r.topLeft());
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
                                    _plugin = MusEGlobal::plugins.find(file, label);
                                    if (_plugin == 0)
                                    {  
                                      printf("Warning: Plugin not found (%s, %s)\n",
                                         file.toLatin1().constData(), label.toLatin1().constData());
                                      return true;
                                    }
				    
                                    if (initPluginInstance(_plugin, channel))
                                    {  
                                      printf("Error initializing plugin instance (%s, %s)\n",
                                        file.toLatin1().constData(), label.toLatin1().constData());
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
      if (_plugin) {
            if (_gui == 0)
                    makeGui();
            _gui->setWindowTitle(titlePrefix() + name());
            if (_gui->isVisible())
                    _gui->hide();
            else
                    _gui->show();
            }
      }

void PluginI::showGui(bool flag)
      {
      if (_plugin) {
            if (flag) {
                    if (_gui == 0)
                        makeGui();
                    _gui->show();
                    }
            else {
                    if (_gui)
                        _gui->hide();
                    }
            }
      }

//---------------------------------------------------------
//   guiVisible
//---------------------------------------------------------

bool PluginI::guiVisible()
      {
      return _gui && _gui->isVisible();
      }

//---------------------------------------------------------
//   showNativeGui
//---------------------------------------------------------

void PluginI::showNativeGui()
{
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

void PluginI::showNativeGui(bool flag)
{
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

bool PluginI::nativeGuiVisible()
{
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
//   enable2AllControllers
//---------------------------------------------------------

void PluginI::enable2AllControllers(bool v)
{
  for(unsigned long i = 0; i < controlPorts; ++i) 
    controls[i].en2Ctrl = v;
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


void PluginI::apply(unsigned long n, unsigned long ports, float** bufIn, float** bufOut)
{
      unsigned long syncFrame = MusEGlobal::audio->curSyncFrame();  
      unsigned long sample = 0;
      
      // Must make this detectable for dssi vst effects.
      const bool usefixedrate = _plugin->_isDssiVst;  // Try this. (was: = true; )

      // TODO Make this number a global setting.
      // Note for dssi-vst this MUST equal audio period. It doesn't like broken-up runs (it stutters), 
      //  even with fixed sizes. Could be a Wine + Jack thing, wanting a full Jack buffer's length.
      unsigned long fixedsize = n;  // was: 2048
      
      // For now, the fixed size is clamped to the audio buffer size.
      // TODO: We could later add slower processing over several cycles -
      //  so that users can select a small audio period but a larger control period. 
      if(fixedsize > n)
        fixedsize = n;
        
      unsigned long min_per = MusEGlobal::config.minControlProcessPeriod;  
      if(min_per > n)
        min_per = n;
      
      // CtrlListList* cll = NULL;  // WIP
      AutomationType at = AUTO_OFF;
      if(_track)
      {
        at = _track->automationType();
        //cll = _track->controller();  // WIP
      }
      bool no_auto = !MusEGlobal::automation || at == AUTO_OFF;
      
      while(sample < n)
      {
        // nsamp is the number of samples the plugin->process() call will be supposed to do
        unsigned long nsamp = usefixedrate ? fixedsize : n - sample;

        //
        // Process automation control values, while also determining the maximum acceptable 
        //  size of this run. Further processing, from FIFOs for example, can lower the size 
        //  from there, but this section determines where the next highest maximum frame 
        //  absolutely needs to be for smooth playback of the controller value stream...
        //
        if(_track && _id != -1 && ports != 0) // Don't bother if not 'running'.
        {
          unsigned long frame = MusEGlobal::audio->pos().frame() + sample;
          int nextFrame;
          //double val;  // WIP
          for(unsigned long k = 0; k < controlPorts; ++k)
          {

            
#if 0  // WIP - Work in progress. Tim.    

            ciCtrlList icl = cll->find(genACnum(_id, k));
            if(icl == cll->end())
              continue;
            CtrlList* cl = icl->second;
            if(no_auto || !controls[k].enCtrl || !controls[k].en2Ctrl || cl->empty()) 
            {
              nextFrame = -1;
              val = cl->curVal();
            }
            else
            {
              ciCtrl i = cl->upper_bound(frame); // get the index after current frame
              if (i == cl->end()) { // if we are past all items just return the last value
                    --i;
                    nextFrame = -1;
                    val = i->second.val;
                    }
              else if(cl->mode() == CtrlList::DISCRETE)
              {
                if(i == cl->begin())
                {
                    nextFrame = i->second.frame;
                    val = i->second.val;
                }  
                else
                {  
                  nextFrame = i->second.frame;
                  --i;
                  val = i->second.val;
                }  
              }
              else {                  // INTERPOLATE
                if (i == cl->begin()) {
                    nextFrame = i->second.frame;
                    val = i->second.val;
                }
                else {
                    int frame2 = i->second.frame;
                    double val2 = i->second.val;
                    --i;
                    int frame1 = i->second.frame;
                    double val1   = i->second.val;

                    
                    if(val2 != val1)
                      nextFrame = 0; // Zero signifies the next frame should be determined by caller.
                    else
                      nextFrame = frame2;
                    
                    if (cl->valueType() == VAL_LOG) {
                      val1 = 20.0*fast_log10(val1);
                      if (val1 < MusEGlobal::config.minSlider)
                        val1=MusEGlobal::config.minSlider;
                      val2 = 20.0*fast_log10(val2);
                      if (val2 < MusEGlobal::config.minSlider)
                        val2=MusEGlobal::config.minSlider;
                    }

                    val2  -= val1;
                    val1 += (double(frame - frame1) * val2)/double(frame2 - frame1);
            
                    if (cl->valueType() == VAL_LOG) {
                      val1 = exp10(val1/20.0);
                    }

                    val = val1;
                  }
              }
            }
            
            controls[k].tmpVal = val;
            
            
#else            
            controls[k].tmpVal = _track->controller()->value(genACnum(_id, k), frame,
                                    no_auto || !controls[k].enCtrl || !controls[k].en2Ctrl,
                                    &nextFrame);
#endif            
            
            
#ifdef PLUGIN_DEBUGIN_PROCESS
            printf("PluginI::apply k:%lu sample:%lu frame:%lu nextFrame:%d nsamp:%lu \n", k, sample, frame, nextFrame, nsamp);
#endif
            if(MusEGlobal::audio->isPlaying() && !usefixedrate && nextFrame != -1)
            {
              // Returned value of nextFrame can be zero meaning caller replaces with some (constant) value.
              unsigned long samps = (unsigned long)nextFrame;
              if(samps > frame + min_per)
              {
                unsigned long diff = samps - frame;
                unsigned long mask = min_per-1;   // min_per must be power of 2
                samps = diff & ~mask;
                if((diff & mask) != 0)
                  samps += min_per;
              }
              else
                samps = min_per;
              
              if(samps < nsamp)
                nsamp = samps;
            }
          }
          
#ifdef PLUGIN_DEBUGIN_PROCESS
          printf("PluginI::apply sample:%lu nsamp:%lu\n", sample, nsamp);
#endif
        }
        
        //
        // Process all control ring buffer items valid for this time period...
        //
        bool found = false;
        unsigned long frame = 0; 
        unsigned long index = 0;
        unsigned long evframe; 
        while(!_controlFifo.isEmpty())
        {
          ControlEvent v = _controlFifo.peek(); 
          // The events happened in the last period or even before that. Shift into this period with + n. This will sync with audio. 
          // If the events happened even before current frame - n, make sure they are counted immediately as zero-frame.
          evframe = (syncFrame > v.frame + n) ? 0 : v.frame - syncFrame + n; 
          // Process only items in this time period. Make sure to process all
          //  subsequent items which have the same frame. 

          // Protection. Observed this condition. Why? Supposed to be linear timestamps.
          if(found && evframe < frame)
          {
            printf("PluginI::apply *** Error: evframe:%lu < frame:%lu idx:%lu val:%f unique:%d\n", 
              evframe, v.frame, v.idx, v.value, v.unique); 

            // No choice but to ignore it.
            _controlFifo.remove();               // Done with the ring buffer's item. Remove it.
            continue;
          } 
          
          // process control events up to the end of our processing cycle.
          // but stop after a control event was found (then process(),
          // then loop here again), but ensure that process() must process
          // at least min_per frames.
          if(evframe >= n                                                               // Next events are for a later period.
              || (!usefixedrate && !found && !v.unique && (evframe - sample >= nsamp))  // Next events are for a later run in this period. (Autom took prio.)
              || (found && !v.unique && (evframe - sample >= min_per))                  // Eat up events within minimum slice - they're too close.
              || (usefixedrate && found && v.unique && v.idx == index))                 // Special for dssi-vst: Fixed rate and must reply to all.
            break;
          _controlFifo.remove();               // Done with the ring buffer's item. Remove it.

          if(v.idx >= _plugin->_controlInPorts) // Sanity check
            break;

          found = true;
          frame = evframe;
          index = v.idx;

          controls[v.idx].tmpVal = v.value;
          
          // Need to update the automation value, otherwise it overwrites later with the last automation value.
          if(_track && _id != -1)
            _track->setPluginCtrlVal(genACnum(_id, v.idx), v.value);
        }

        // Now update the actual values from the temporary values...
        for(unsigned long k = 0; k < controlPorts; ++k)
          controls[k].val = controls[k].tmpVal;
        
        if(found && !usefixedrate) // If a control FIFO item was found, takes priority over automation controller stream.
          nsamp = frame - sample;  

        if(sample + nsamp >= n)    // Safety check.
          nsamp = n - sample; 
        
        // TODO: Don't allow zero-length runs. This could/should be checked in the control loop instead.
        // Note this means it is still possible to get stuck in the top loop (at least for a while).
        if(nsamp == 0)
          continue;
          
        if(ports != 0)
        {  
          connect(ports, sample, bufIn, bufOut);
        
          for(int i = 0; i < instances; ++i)
            _plugin->apply(handle[i], nsamp);
        }
        
        sample += nsamp;
      }
}

//---------------------------------------------------------
//   oscConfigure
//---------------------------------------------------------

#ifdef OSC_SUPPORT
int Plugin::oscConfigure(LADSPA_Handle handle, const char* key, const char* value)
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

int PluginI::oscConfigure(const char *key, const char *value)
      {
      if(!_plugin)
        return 0;

      // This is pretty much the simplest legal implementation of
      // configure in a DSSI host. 

      // The host has the option to remember the set of (key,value)
      // pairs associated with a particular instance, so that if it
      // wants to restore the "same" instance on another occasion it can
      // just call configure() on it for each of those pairs and so
      // restore state without any input from a GUI.  Any real-world GUI
      // host will probably want to do that.  This host doesn't have any
      // concept of restoring an instance from one run to the next, so
      // we don't bother remembering these at all. 

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
  //unsigned long cport = _plugin->port2InCtrl(port);
    
  if((int)cport == -1)
  {
    fprintf(stderr, "PluginI::oscControl: port number:%lu is not a control input\n", port);
    return 0;
  }
  
  // (From DSSI module).
  // p3.3.39 Set the DSSI control input port's value.
  // Observations: With a native DSSI synth like LessTrivialSynth, the native GUI's controls do not change the sound at all
  //  ie. they don't update the DSSI control port values themselves. 
  // Hence in response to the call to this oscControl, sent by the native GUI, it is required to that here.
///  controls[cport].val = value;
  // DSSI-VST synths however, unlike DSSI synths, DO change their OWN sound in response to their gui controls.
  // AND this function is called ! 
  // Despite the descrepency we are STILL required to update the DSSI control port values here 
  //  because dssi-vst is WAITING FOR A RESPONSE! (A CHANGE in the control port value). 
  // It will output something like "...4 events expected..." and count that number down as 4 actual control port value CHANGES
  //  are done here in response. Normally it says "...0 events expected..." when MusE is the one doing the DSSI control changes.
  // TODO: May need FIFOs on each control(!) so that the control changes get sent one per process cycle! 
  // Observed countdown not actually going to zero upon string of changes.
  // Try this ...
  /* DELETETHIS 20
  OscControlFifo* cfifo = _oscif.oscFifo(cport); 
  if(cfifo)
  {
    OscControlValue cv;
    //cv.idx = cport;
    cv.value = value;
    // Time-stamp the event. Looks like no choice but to use the (possibly slow) call to gettimeofday via timestamp(),
    //  because these are asynchronous events arriving from OSC.  timestamp() is more or less an estimate of the
    //  current frame. (This is exactly how ALSA events are treated when they arrive in our ALSA driver.) p4.0.15 Tim. 
    cv.frame = MusEGlobal::audio->timestamp();  
    if(cfifo->put(cv))
    {
      fprintf(stderr, "PluginI::oscControl: fifo overflow: in control number:%lu\n", cport);
    }
  }
  */
  ControlEvent ce;
  ce.unique = _plugin->_isDssiVst;   // Special for messages from vst gui to host - requires processing every message.
  ce.idx = cport;
  ce.value = value;
  // Time-stamp the event. This does a possibly slightly slow call to gettimeofday via timestamp().
  //  timestamp() is more or less an estimate of the current frame. (This is exactly how ALSA events 
  //  are treated when they arrive in our ALSA driver.) 
  //ce.frame = MusEGlobal::audio->timestamp();  
  // p4.0.23 timestamp() is circular, which is making it impossible to deal with 'modulo' events which 
  //  slip in 'under the wire' before processing the ring buffers. So try this linear timestamp instead:
  ce.frame = MusEGlobal::audio->curFrame();  
  if(_controlFifo.put(ce))
  {
    fprintf(stderr, "PluginI::oscControl: fifo overflow: in control number:%lu\n", cport);
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
    AutomationType at = _track->automationType();
  
    // TODO: Taken from our native gui control handlers. 
    // This may need modification or may cause problems - 
    //  we don't have the luxury of access to the dssi gui controls !
    if ((at == AUTO_WRITE) ||
        (at == AUTO_TOUCH && MusEGlobal::audio->isPlaying()))
      enableController(cport, false); //TODO maybe re-enable the ctrl soon?
      
    _track->recordAutomation(id, value);
  } 
   
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

//---------------------------------------------------------
//   PluginDialog
//    select Plugin dialog
//---------------------------------------------------------

PluginDialog::PluginDialog(QWidget* parent)
  : QDialog(parent)
      {
      setWindowTitle(tr("MusE: select plugin"));

      if(!geometrySave.isNull())
        setGeometry(geometrySave);
      
      QVBoxLayout* layout = new QVBoxLayout(this);

      pList  = new QTreeWidget(this);
      pList->setColumnCount(12);
      // "Note: In order to avoid performance issues, it is recommended that sorting 
      //   is enabled after inserting the items into the tree. Alternatively, you could 
      //   also insert the items into a list before inserting the items into the tree. "
      QStringList headerLabels;
      headerLabels << tr("Type");
      headerLabels << tr("Lib");
      headerLabels << tr("Label");
      headerLabels << tr("Name");
      headerLabels << tr("AI");
      headerLabels << tr("AO");
      headerLabels << tr("CI");
      headerLabels << tr("CO");
      headerLabels << tr("IP");
      headerLabels << tr("id");
      headerLabels << tr("Maker");
      headerLabels << tr("Copyright");

      pList->setHeaderLabels(headerLabels);

      pList->headerItem()->setToolTip(4,  tr("Audio inputs"));      
      pList->headerItem()->setToolTip(5,  tr("Audio outputs"));      
      pList->headerItem()->setToolTip(6,  tr("Control inputs"));      
      pList->headerItem()->setToolTip(7,  tr("Control outputs"));      
      pList->headerItem()->setToolTip(8,  tr("In-place capable"));      
      pList->headerItem()->setToolTip(9,  tr("ID number"));      
      
      pList->setRootIsDecorated(false);
      pList->setSelectionBehavior(QAbstractItemView::SelectRows);
      pList->setSelectionMode(QAbstractItemView::SingleSelection);
      pList->setAlternatingRowColors(true);
      pList->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
      
      layout->addWidget(pList);

      //---------------------------------------------------
      //  Ok/Cancel Buttons
      //---------------------------------------------------

      QBoxLayout* w5 = new QHBoxLayout;
      layout->addLayout(w5);

      QBoxLayout* ok_lo = new QVBoxLayout;
      w5->addLayout(ok_lo);
      
      okB     = new QPushButton(tr("Ok"), this);
      okB->setDefault(true);
      QPushButton* cancelB = new QPushButton(tr("Cancel"), this);
      okB->setFixedWidth(80);
      okB->setEnabled(false);
      cancelB->setFixedWidth(80);
      ok_lo->addWidget(okB);
      ok_lo->addSpacing(8);
      ok_lo->addWidget(cancelB);

      QGroupBox* plugSelGroup = new QGroupBox(this);
      plugSelGroup->setTitle(tr("Show plugs:"));
      plugSelGroup->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
      QGridLayout* psl = new QGridLayout;
      plugSelGroup->setLayout(psl);

      QButtonGroup* plugSel = new QButtonGroup(plugSelGroup);
      onlySM  = new QRadioButton(this);
      onlySM->setText(tr("Mono and Stereo"));
      onlySM->setCheckable(true);
      plugSel->addButton(onlySM);
      psl->addWidget(onlySM, 1, 0);
      onlyS = new QRadioButton(this);
      onlyS->setText(tr("Stereo"));
      onlyS->setCheckable(true);
      plugSel->addButton(onlyS);
      psl->addWidget(onlyS, 0, 1);
      onlyM = new QRadioButton(this);
      onlyM->setText(tr("Mono"));
      onlyM->setCheckable(true);
      plugSel->addButton(onlyM);
      psl->addWidget(onlyM, 0, 0);
      allPlug = new QRadioButton(this);
      allPlug->setText(tr("Show All"));
      allPlug->setCheckable(true);
      plugSel->addButton(allPlug);
      psl->addWidget(allPlug, 1, 1);
      plugSel->setExclusive(true);

      switch(selectedPlugType) {
            case SEL_SM:  onlySM->setChecked(true);  break;
            case SEL_S:   onlyS->setChecked(true);   break;
            case SEL_M:   onlyM->setChecked(true);   break;
            case SEL_ALL: allPlug->setChecked(true); break;
            }

      plugSelGroup->setToolTip(tr("Select which types of plugins should be visible in the list.<br>"
                             "Note that using mono plugins on stereo tracks is not a problem, two will be used in parallel.<br>"
                             "Also beware that the 'all' alternative includes plugins that may not be useful in an effect rack."));

      w5->addSpacing(8);
      w5->addWidget(plugSelGroup);
      w5->addSpacing(8);

      QBoxLayout* srch_lo = new QVBoxLayout;
      w5->addLayout(srch_lo);
      
      QLabel *sortLabel = new QLabel(this);
      sortLabel->setText(tr("Search in 'Label' and 'Name':"));
      srch_lo->addSpacing(8);
      srch_lo->addWidget(sortLabel);
      srch_lo->addSpacing(8);

      sortBox = new QComboBox(this);
      sortBox->setEditable(true);
      if (!sortItems.empty())
            sortBox->addItems(sortItems);

      sortBox->setMinimumSize(100, 10);
      srch_lo->addWidget(sortBox);
      // FIXME: Adding this makes the whole bottom hlayout expand. Would like some space between lineedit and bottom.
      //        Same thing if spacers added to group box or Ok Cancel box.
      //srch_lo->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Maximum));

      fillPlugs();
      
      pList->setSortingEnabled(true);
      
      if(listSave.isEmpty())
      {
        int sizes[] = { 80, 110, 110, 110, 30, 30, 30, 30, 30, 50, 110, 110 };
        for (int i = 0; i < 12; ++i) {
              if (sizes[i] <= 50)     // hack alert!
                    pList->header()->setResizeMode(i, QHeaderView::Fixed);
              pList->header()->resizeSection(i, sizes[i]);
        }
        pList->sortByColumn(3, Qt::AscendingOrder);
      }
      else
        pList->header()->restoreState(listSave);

      connect(pList,   SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), SLOT(accept()));
      connect(pList,   SIGNAL(itemClicked(QTreeWidgetItem*,int)), SLOT(enableOkB()));
      connect(cancelB, SIGNAL(clicked()), SLOT(reject()));
      connect(okB,     SIGNAL(clicked()), SLOT(accept()));
      connect(plugSel, SIGNAL(buttonClicked(QAbstractButton*)), SLOT(fillPlugs(QAbstractButton*)));
      connect(sortBox, SIGNAL(editTextChanged(const QString&)),SLOT(fillPlugs()));
      sortBox->setFocus();
      }

//---------------------------------------------------------
//   enableOkB
//---------------------------------------------------------

void PluginDialog::enableOkB()
      {
	okB->setEnabled(true);
      }

//---------------------------------------------------------
//   value
//---------------------------------------------------------

MusECore::Plugin* PluginDialog::value()
      {
      QTreeWidgetItem* item = pList->currentItem();
      if (item)
        return MusEGlobal::plugins.find(item->text(1), item->text(2));
      printf("plugin not found\n");
      return 0;
      }

//---------------------------------------------------------
//   saveSettings
//---------------------------------------------------------

void PluginDialog::saveSettings()
{
  if (!sortBox->currentText().isEmpty()) {
        bool found = false;
        foreach (QString item, sortItems)
            if(item == sortBox->currentText()) {
                found = true;
                break;
                }
        if(!found)        
          sortItems.push_front(sortBox->currentText());
        }

  QHeaderView* hdr = pList->header();
  if(hdr)
    listSave = hdr->saveState();

  geometrySave = geometry();      
}

//---------------------------------------------------------
//   accept
//---------------------------------------------------------

void PluginDialog::accept()
      {
      saveSettings();
      QDialog::accept();
      }

//---------------------------------------------------------
//   reject
//---------------------------------------------------------

void PluginDialog::reject()
{
      saveSettings();
      QDialog::reject();
}

//---------------------------------------------------------
//    fillPlugs
//---------------------------------------------------------

void PluginDialog::fillPlugs(QAbstractButton* ab)
      {
      if (ab == allPlug)
            selectedPlugType = SEL_ALL;
      else if (ab == onlyM)
            selectedPlugType = SEL_M;
      else if (ab == onlyS)
            selectedPlugType = SEL_S;
      else if (ab == onlySM)
            selectedPlugType = SEL_SM;
      fillPlugs();
      }

void PluginDialog::fillPlugs()
{
    QString type_name;
    pList->clear();
    for (MusECore::iPlugin i = MusEGlobal::plugins.begin(); i != MusEGlobal::plugins.end(); ++i) {
          unsigned long ai = i->inports();       
          unsigned long ao = i->outports();
          unsigned long ci = i->controlInPorts();
          unsigned long co = i->controlOutPorts();
          bool found = false;
          QString sb_txt = sortBox->currentText().toLower();
          if(sb_txt.isEmpty() || i->label().toLower().contains(sb_txt) || i->name().toLower().contains(sb_txt))
                found = true;
          
          bool addFlag = false;
          switch (selectedPlugType) {
                case SEL_SM: // stereo & mono
                      if ((ai == 1 || ai == 2) && (ao == 1 || ao ==2)) {
                            addFlag = true;
                            }
                      break;
                case SEL_S: // stereo
                      if ((ai == 1 || ai == 2) &&  ao ==2) {
                            addFlag = true;
                            }
                      break;
                case SEL_M: // mono
                      if (ai == 1  && ao == 1) {
                            addFlag = true;
                            }
                      break;
                case SEL_ALL: // all
                      addFlag = true;
                      break;
                }
          if (found && addFlag) {
                QTreeWidgetItem* item = new QTreeWidgetItem;
                if(i->isDssiSynth())
                  type_name = tr("dssi synth");
                else if(i->isDssiPlugin())
                  type_name = tr("dssi effect");
                else
                  type_name = tr("ladspa");
                item->setText(0,  type_name);
                item->setText(1,  i->lib());
                item->setText(2,  i->label());
                item->setText(3,  i->name());
                item->setText(4,  QString().setNum(ai));
                item->setText(5,  QString().setNum(ao));
                item->setText(6,  QString().setNum(ci));
                item->setText(7,  QString().setNum(co));
                item->setText(8,  QString().setNum(i->inPlaceCapable()));
                item->setText(9,  QString().setNum(i->id()));
                item->setText(10,  i->maker());
                item->setText(11, i->copyright());
                pList->addTopLevelItem(item);
                }
          }
}
  
//---------------------------------------------------------
//   getPlugin
//---------------------------------------------------------

MusECore::Plugin* PluginDialog::getPlugin(QWidget* parent)
      {
      PluginDialog* dialog = new PluginDialog(parent);
      MusECore::Plugin* p = 0;
      int rv = dialog->exec();
      if(rv)
        p = dialog->value(); 
      delete dialog;
      return p;
      }

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
      setWindowTitle(plugin->titlePrefix() + plugin->name());

      QToolBar* tools = addToolBar(tr("File Buttons"));

      QAction* fileOpen = new QAction(QIcon(*openIconS), tr("Load Preset"), this);
      connect(fileOpen, SIGNAL(triggered()), this, SLOT(load()));
      tools->addAction(fileOpen);
      
      QAction* fileSave = new QAction(QIcon(*saveIconS), tr("Save Preset"), this);
      connect(fileSave, SIGNAL(triggered()), this, SLOT(save()));
      tools->addAction(fileSave);

      tools->addAction(QWhatsThis::createAction(this));

      onOff = new QAction(QIcon(*exitIconS), tr("bypass plugin"), this);
      onOff->setCheckable(true);
      onOff->setChecked(plugin->on());
      onOff->setToolTip(tr("bypass plugin"));
      connect(onOff, SIGNAL(toggled(bool)), SLOT(bypassToggled(bool)));
      tools->addAction(onOff);

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
            QSignalMapper* mapper = new QSignalMapper(this);
            
            // FIXME: There's no unsigned for gui params. We would need to limit nobj to MAXINT.    
            // FIXME: Our MusEGui::Slider class uses doubles for values, giving some problems with float conversion.    
            
            connect(mapper, SIGNAL(mapped(int)), SLOT(guiParamChanged(int)));
            
            QSignalMapper* mapperPressed        = new QSignalMapper(this);
            QSignalMapper* mapperReleased       = new QSignalMapper(this);
            QSignalMapper* mapperContextMenuReq = new QSignalMapper(this);
            connect(mapperPressed, SIGNAL(mapped(int)), SLOT(guiParamPressed(int)));
            connect(mapperReleased, SIGNAL(mapped(int)), SLOT(guiParamReleased(int)));
            connect(mapperContextMenuReq, SIGNAL(mapped(int)), SLOT(guiContextMenuReq(int)));
            
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

                  mapper->setMapping(obj, nobj);
                  mapperPressed->setMapping(obj, nobj);
                  mapperReleased->setMapping(obj, nobj);
                  mapperContextMenuReq->setMapping(obj, nobj);
                  
                  gw[nobj].widget = (QWidget*)obj;
                  gw[nobj].param  = parameter;
                  gw[nobj].type   = -1;

                  if (strcmp(obj->metaObject()->className(), "MusEGui::Slider") == 0) {
                        gw[nobj].type = GuiWidgets::SLIDER;
                        ((Slider*)obj)->setId(nobj);
                        ((Slider*)obj)->setCursorHoming(true);
                        for(unsigned long i = 0; i < nobj; i++)             
                        {
                          if(gw[i].type == GuiWidgets::DOUBLE_LABEL && gw[i].param == parameter)
                            ((DoubleLabel*)gw[i].widget)->setSlider((Slider*)obj);
                        }
                        connect((Slider*)obj, SIGNAL(sliderMoved(double,int)), mapper, SLOT(map()));
                        connect((Slider*)obj, SIGNAL(sliderPressed(int)), SLOT(guiSliderPressed(int)));
                        connect((Slider*)obj, SIGNAL(sliderReleased(int)), SLOT(guiSliderReleased(int)));
                        connect((Slider*)obj, SIGNAL(sliderRightClicked(const QPoint &, int)), SLOT(guiSliderRightClicked(const QPoint &, int)));
                        }
                  else if (strcmp(obj->metaObject()->className(), "MusEGui::DoubleLabel") == 0) {
                        gw[nobj].type = GuiWidgets::DOUBLE_LABEL;
                        ((DoubleLabel*)obj)->setId(nobj);
                        for(unsigned long i = 0; i < nobj; i++)
                        {
                          if(gw[i].type == GuiWidgets::SLIDER && gw[i].param == parameter)
                          {
                            ((DoubleLabel*)obj)->setSlider((Slider*)gw[i].widget);
                            break;  
                          }  
                        }
                        connect((DoubleLabel*)obj, SIGNAL(valueChanged(double,int)), mapper, SLOT(map()));
                        }
                  else if (strcmp(obj->metaObject()->className(), "QCheckBox") == 0) {
                        gw[nobj].type = GuiWidgets::QCHECKBOX;
                        gw[nobj].widget->setContextMenuPolicy(Qt::CustomContextMenu);
                        connect((QCheckBox*)obj, SIGNAL(toggled(bool)), mapper, SLOT(map()));
                        connect((QCheckBox*)obj, SIGNAL(pressed()), mapperPressed, SLOT(map()));
                        connect((QCheckBox*)obj, SIGNAL(released()), mapperReleased, SLOT(map()));
                        connect((QCheckBox*)obj, SIGNAL(customContextMenuRequested(const QPoint &)), 
                                mapperContextMenuReq, SLOT(map()));
                        }
                  else if (strcmp(obj->metaObject()->className(), "QComboBox") == 0) {
                        gw[nobj].type = GuiWidgets::QCOMBOBOX;
                        gw[nobj].widget->setContextMenuPolicy(Qt::CustomContextMenu);
                        connect((QComboBox*)obj, SIGNAL(activated(int)), mapper, SLOT(map()));
                        connect((QComboBox*)obj, SIGNAL(customContextMenuRequested(const QPoint &)), 
                                mapperContextMenuReq, SLOT(map()));
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

            for (unsigned long i = 0; i < n; ++i) {       
                  QLabel* label = 0;
                  LADSPA_PortRangeHint range = plugin->range(i);
                  double lower = 0.0;     // default values
                  double upper = 1.0;
                  double dlower = lower;
                  double dupper = upper;
                  double val   = plugin->param(i);
                  double dval  = val;
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
                           Slider::None, color);
                           
                        s->setCursorHoming(true);
                        s->setId(i);
                        s->setSizeHint(200, 8);
                        s->setRange(dlower, dupper);
                        if(LADSPA_IS_HINT_INTEGER(range.HintDescriptor))
                          s->setStep(1.0);
                        s->setValue(dval);
                        params[i].actuator = s;
                        params[i].label->setSlider((Slider*)params[i].actuator);
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
                        connect(params[i].actuator, SIGNAL(sliderMoved(double,int,bool)), SLOT(sliderChanged(double,int,bool)));
                        connect(params[i].label,    SIGNAL(valueChanged(double,int)), SLOT(labelChanged(double,int)));
                        connect(params[i].actuator, SIGNAL(sliderPressed(int)), SLOT(ctrlPressed(int)));
                        connect(params[i].actuator, SIGNAL(sliderReleased(int)), SLOT(ctrlReleased(int)));
                        connect(params[i].actuator, SIGNAL(sliderRightClicked(const QPoint &, int)), SLOT(ctrlRightClicked(const QPoint &, int)));
                        }
                  else if (params[i].type == GuiParam::GUI_SWITCH){
                        connect(params[i].actuator, SIGNAL(checkboxPressed(int)), SLOT(ctrlPressed(int)));
                        connect(params[i].actuator, SIGNAL(checkboxReleased(int)), SLOT(ctrlReleased(int)));
                        connect(params[i].actuator, SIGNAL(checkboxRightClicked(const QPoint &, int)), SLOT(ctrlRightClicked(const QPoint &, int)));
                        }
                  }


            int n2  = plugin->parametersOut();
            if (n2 > 0) {
              paramsOut = new GuiParam[n2];

              int h = fm.height() - 2;
              for (int i = 0; i < n2; ++i) {
                      QLabel* label = 0;
                      LADSPA_PortRangeHint range = plugin->rangeOut(i);
                      double lower = 0.0;     // default values
                      double upper = 1.0;
                      double dlower = lower;
                      double dupper = upper;
                      double val   = plugin->paramOut(i);
                      double dval  = val;
                      paramsOut[i].hint = range.HintDescriptor;

                      getPluginConvertedValues(range, lower, upper, dlower, dupper, dval);
                      label           = new QLabel(QString(plugin->paramOutName(i)), 0);
                      paramsOut[i].type  = GuiParam::GUI_METER;
                      paramsOut[i].label = new DoubleLabel(val, lower, upper, 0);
                      paramsOut[i].label->setFrame(true);
                      paramsOut[i].label->setPrecision(2);
                      paramsOut[i].label->setId(i);

                      Meter::MeterType mType=Meter::LinMeter;
                      if(LADSPA_IS_HINT_INTEGER(range.HintDescriptor))
                        mType=Meter::DBMeter;
                      VerticalMeter* m = new VerticalMeter(this, mType);

                      m->setRange(dlower, dupper);
                      m->setVal(dval);
                      m->setFixedHeight(h);
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
  updateControls(); // FINDMICHJETZT TODO: this is not good. we have concurrent
                    // access from the audio thread (possibly writing control values)
                    // while reading them from some GUI thread. this will lead
                    // to problems if writing floats is non-atomic
}

//---------------------------------------------------------
//   ctrlPressed
//---------------------------------------------------------

void PluginGui::ctrlPressed(int param)
{
      AutomationType at = AUTO_OFF;
      MusECore::AudioTrack* track = plugin->track();
      if(track)
        at = track->automationType();
            
      if (at == AUTO_READ || at == AUTO_TOUCH || at == AUTO_WRITE)
        plugin->enableController(param, false);
      
      int id = plugin->id();
      
      if(id == -1)
        return;
        
      id = MusECore::genACnum(id, param);
      
      if(params[param].type == GuiParam::GUI_SLIDER)
      {
        double val = ((Slider*)params[param].actuator)->value();  
        if (LADSPA_IS_HINT_LOGARITHMIC(params[param].hint))
              val = pow(10.0, val/20.0);
        else if (LADSPA_IS_HINT_INTEGER(params[param].hint))
              val = rint(val);
        plugin->setParam(param, val);
        ((DoubleLabel*)params[param].label)->setValue(val);
        
        if(track)
        {
          track->setPluginCtrlVal(id, val);
          track->startAutoRecord(id, val);
        }
      }
      else if(params[param].type == GuiParam::GUI_SWITCH)
      {
        float val = (float)((CheckBox*)params[param].actuator)->isChecked();      
        plugin->setParam(param, val);
        
        if(track)
        {
          track->setPluginCtrlVal(id, val);
          track->startAutoRecord(id, val);
        }
      }
}

//---------------------------------------------------------
//   ctrlReleased
//---------------------------------------------------------

void PluginGui::ctrlReleased(int param)
{
      AutomationType at = AUTO_OFF;
      MusECore::AudioTrack* track = plugin->track();
      if(track)
        at = track->automationType();
        
      // Special for switch - don't enable controller until transport stopped.
      if ((at == AUTO_OFF) ||
          (at == AUTO_READ) ||
          (at == AUTO_TOUCH && (params[param].type != GuiParam::GUI_SWITCH ||
                                !MusEGlobal::audio->isPlaying()) ) )
        plugin->enableController(param, true);
      
      int id = plugin->id();
      if(!track || id == -1)
        return;
      id = MusECore::genACnum(id, param);
        
      if(params[param].type == GuiParam::GUI_SLIDER)
      {
        double val = ((Slider*)params[param].actuator)->value();
        if (LADSPA_IS_HINT_LOGARITHMIC(params[param].hint))
              val = pow(10.0, val/20.0);
        else if (LADSPA_IS_HINT_INTEGER(params[param].hint))
              val = rint(val);
        track->stopAutoRecord(id, val);
      }
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
//   sliderChanged
//---------------------------------------------------------

void PluginGui::sliderChanged(double val, int param, bool shift_pressed)
{
      AutomationType at = AUTO_OFF;
      MusECore::AudioTrack* track = plugin->track();
      if(track)
        at = track->automationType();
      
      if ( (at == AUTO_WRITE) ||
           (at == AUTO_TOUCH && MusEGlobal::audio->isPlaying()) )
        plugin->enableController(param, false);
      
      if (LADSPA_IS_HINT_LOGARITHMIC(params[param].hint))
            val = pow(10.0, val/20.0);
      else if (LADSPA_IS_HINT_INTEGER(params[param].hint))
            val = rint(val);
      
      if (plugin->param(param) != val) {
            plugin->setParam(param, val);
            ((DoubleLabel*)params[param].label)->setValue(val);
            }
            
      int id = plugin->id();
      if(id == -1)
        return;
      id = MusECore::genACnum(id, param);
          
      if(track)
      {
        track->setPluginCtrlVal(id, val);
        if (!shift_pressed) track->recordAutomation(id, val); //with shift, we get straight lines :)
      }  
}

//---------------------------------------------------------
//   labelChanged
//---------------------------------------------------------

void PluginGui::labelChanged(double val, int param)
{
      AutomationType at = AUTO_OFF;
      MusECore::AudioTrack* track = plugin->track();
      if(track)
        at = track->automationType();
      
      if ( (at == AUTO_WRITE) ||
           (at == AUTO_TOUCH && MusEGlobal::audio->isPlaying()) )
        plugin->enableController(param, false);
      
      double dval = val;
      if (LADSPA_IS_HINT_LOGARITHMIC(params[param].hint))
            dval = MusECore::fast_log10(val) * 20.0;
      else if (LADSPA_IS_HINT_INTEGER(params[param].hint))
            dval = rint(val);
      if (plugin->param(param) != val) {
            plugin->setParam(param, val);
            ((Slider*)params[param].actuator)->setValue(dval);
            }
      
      int id = plugin->id();
      if(id == -1)
        return;
      
      id = MusECore::genACnum(id, param);
      
      if(track)
      {
        track->setPluginCtrlVal(id, val);
        track->startAutoRecord(id, val);
      }  
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
      setWindowTitle(plugin->titlePrefix() + plugin->name());
      plugin->setOn(val);
      MusEGlobal::song->update(SC_ROUTE);
      }

//---------------------------------------------------------
//   setOn
//---------------------------------------------------------

void PluginGui::setOn(bool val)
      {
      setWindowTitle(plugin->titlePrefix() + plugin->name());
      onOff->blockSignals(true);
      onOff->setChecked(val);
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
                        gp->label->setValue(lv);
                        ((Slider*)(gp->actuator))->setValue(sv);
                        }
                  else if (gp->type == GuiParam::GUI_SWITCH) {
                        ((CheckBox*)(gp->actuator))->setChecked(int(plugin->param(i)));
                        }
                  }
            }
      else if (gw) {
            for (unsigned long i = 0; i < nobj; ++i) {      
                  QWidget* widget = gw[i].widget;
                  int type = gw[i].type;
                  unsigned long param = gw[i].param;        
                  float val = plugin->param(param);
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
                 ((VerticalMeter*)(gp->actuator))->setVal(sv);
                 gp->label->setValue(lv);

               }
             }
       }


      if (params) {
            for (unsigned long i = 0; i < plugin->parameters(); ++i) {      
                  GuiParam* gp = &params[i];
                  if (gp->type == GuiParam::GUI_SLIDER) {
                          {
                            double lv = plugin->track()->controller()->value(MusECore::genACnum(plugin->id(), i), 
                                                                             MusEGlobal::audio->curFramePos(), 
                                                                             !MusEGlobal::automation || 
                                                                             plugin->track()->automationType() == AUTO_OFF || 
                                                                             !plugin->controllerEnabled(i) || 
                                                                             !plugin->controllerEnabled2(i));                            
                            double sv = lv;
                            if (LADSPA_IS_HINT_LOGARITHMIC(params[i].hint))
                                  sv = MusECore::fast_log10(lv) * 20.0;
                            else 
                            if (LADSPA_IS_HINT_INTEGER(params[i].hint))
                            {
                                  sv = rint(lv);
                                  lv = sv;
                            }      
                            if(((Slider*)(gp->actuator))->value() != sv)
                            {
                              gp->label->blockSignals(true);
                              ((Slider*)(gp->actuator))->blockSignals(true);
                              ((Slider*)(gp->actuator))->setValue(sv);
                              gp->label->setValue(lv);
                              ((Slider*)(gp->actuator))->blockSignals(false);
                              gp->label->blockSignals(false);
                            } 
                          }
                        }
                  else if (gp->type == GuiParam::GUI_SWITCH) {
                          {
                            bool v = (int)plugin->track()->controller()->value(MusECore::genACnum(plugin->id(), i), 
                                                                             MusEGlobal::audio->curFramePos(), 
                                                                             !MusEGlobal::automation || 
                                                                             plugin->track()->automationType() == AUTO_OFF || 
                                                                             !plugin->controllerEnabled(i) || 
                                                                             !plugin->controllerEnabled2(i));                            
                            if(((CheckBox*)(gp->actuator))->isChecked() != v)
                            {
                              ((CheckBox*)(gp->actuator))->blockSignals(true);
                              ((CheckBox*)(gp->actuator))->setChecked(v);
                              ((CheckBox*)(gp->actuator))->blockSignals(false);
                            } 
                          }
                        }
                  }
            }
      else if (gw) {
            for (unsigned long i = 0; i < nobj; ++i) {    
                  QWidget* widget = gw[i].widget;
                  int type = gw[i].type;
                  unsigned long param = gw[i].param;      
                  switch(type) {
                        case GuiWidgets::SLIDER:
                              {
                                double v = plugin->track()->controller()->value(MusECore::genACnum(plugin->id(), param), 
                                                                                MusEGlobal::audio->curFramePos(), 
                                                                                !MusEGlobal::automation || 
                                                                                plugin->track()->automationType() == AUTO_OFF || 
                                                                                !plugin->controllerEnabled(param) || 
                                                                                !plugin->controllerEnabled2(param));                            
                                if(((Slider*)widget)->value() != v)
                                {
                                  ((Slider*)widget)->blockSignals(true);
                                  ((Slider*)widget)->setValue(v);
                                  ((Slider*)widget)->blockSignals(false);
                                }
                              }
                              break;
                        case GuiWidgets::DOUBLE_LABEL:
                              {
                                double v = plugin->track()->controller()->value(MusECore::genACnum(plugin->id(), param), 
                                                                                MusEGlobal::audio->curFramePos(), 
                                                                                !MusEGlobal::automation || 
                                                                                plugin->track()->automationType() == AUTO_OFF || 
                                                                                !plugin->controllerEnabled(param) || 
                                                                                !plugin->controllerEnabled2(param));                            
                                if(((DoubleLabel*)widget)->value() != v)
                                {
                                  ((DoubleLabel*)widget)->blockSignals(true);
                                  ((DoubleLabel*)widget)->setValue(v);
                                  ((DoubleLabel*)widget)->blockSignals(false);
                                }
                              }
                              break;
                        case GuiWidgets::QCHECKBOX:
                              { 
                                bool b = (bool) plugin->track()->controller()->value(MusECore::genACnum(plugin->id(), param), 
                                                                                MusEGlobal::audio->curFramePos(), 
                                                                                !MusEGlobal::automation || 
                                                                                plugin->track()->automationType() == AUTO_OFF || 
                                                                                !plugin->controllerEnabled(param) || 
                                                                                !plugin->controllerEnabled2(param));                            
                                if(((QCheckBox*)widget)->isChecked() != b)
                                {
                                  ((QCheckBox*)widget)->blockSignals(true);
                                  ((QCheckBox*)widget)->setChecked(b);
                                  ((QCheckBox*)widget)->blockSignals(false);
                                } 
                              }
                              break;
                        case GuiWidgets::QCOMBOBOX:
                              { 
                                int n = (int) plugin->track()->controller()->value(MusECore::genACnum(plugin->id(), param), 
                                                                                MusEGlobal::audio->curFramePos(), 
                                                                                !MusEGlobal::automation || 
                                                                                plugin->track()->automationType() == AUTO_OFF || 
                                                                                !plugin->controllerEnabled(param) || 
                                                                                !plugin->controllerEnabled2(param));                            
                                if(((QComboBox*)widget)->currentIndex() != n)
                                {
                                  ((QComboBox*)widget)->blockSignals(true);
                                  ((QComboBox*)widget)->setCurrentIndex(n);
                                  ((QComboBox*)widget)->blockSignals(false);
                                } 
                              }
                              break;
                        }
                  }   
            }
      }

//---------------------------------------------------------
//   guiParamChanged
//---------------------------------------------------------

void PluginGui::guiParamChanged(int idx)
{
      QWidget* w = gw[idx].widget;
      unsigned long param  = gw[idx].param;    
      int type   = gw[idx].type;

      AutomationType at = AUTO_OFF;
      MusECore::AudioTrack* track = plugin->track();
      if(track)
        at = track->automationType();
      
      if ( (at == AUTO_WRITE) ||
           (at == AUTO_TOUCH && MusEGlobal::audio->isPlaying()) )
        plugin->enableController(param, false);
      
      double val = 0.0;
      switch(type) {
            case GuiWidgets::SLIDER:
                  val = ((Slider*)w)->value();
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
            }
      
      int id = plugin->id();
      if(track && id != -1)
      {
          id = MusECore::genACnum(id, param);
          track->setPluginCtrlVal(id, val);
          switch(type) 
          {
             case GuiWidgets::DOUBLE_LABEL:
             case GuiWidgets::QCHECKBOX:
               track->startAutoRecord(id, val);
             break;
             default:
               track->recordAutomation(id, val);
             break;  
          }  
      } 
      plugin->setParam(param, val);
}

//---------------------------------------------------------
//   guiParamPressed
//---------------------------------------------------------

void PluginGui::guiParamPressed(int idx)
      {
      unsigned long param  = gw[idx].param;     

      AutomationType at = AUTO_OFF;
      MusECore::AudioTrack* track = plugin->track();
      if(track)
        at = track->automationType();
      
      if (at == AUTO_READ || at == AUTO_TOUCH || at == AUTO_WRITE)
        plugin->enableController(param, false);
      
      int id = plugin->id();
      if(!track || id == -1)
        return;
      
      id = MusECore::genACnum(id, param);
      
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

void PluginGui::guiParamReleased(int idx)
      {
      unsigned long param  = gw[idx].param;    
      int type   = gw[idx].type;
      
      AutomationType at = AUTO_OFF;
      MusECore::AudioTrack* track = plugin->track();
      if(track)
        at = track->automationType();
      
      // Special for switch - don't enable controller until transport stopped.
      if ((at == AUTO_OFF) ||
          (at == AUTO_READ) ||
          (at == AUTO_TOUCH && (type != GuiWidgets::QCHECKBOX ||
                                !MusEGlobal::audio->isPlaying()) ) )
        plugin->enableController(param, true);
      
      int id = plugin->id();
      
      if(!track || id == -1)
        return;
      
      id = MusECore::genACnum(id, param);
      
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
      }

//---------------------------------------------------------
//   guiSliderPressed
//---------------------------------------------------------

void PluginGui::guiSliderPressed(int idx)
      {
      unsigned long param  = gw[idx].param;    
      QWidget *w = gw[idx].widget;
      
      AutomationType at = AUTO_OFF;
      MusECore::AudioTrack* track = plugin->track();
      if(track)
        at = track->automationType();
      
      int id = plugin->id();
      
      if (at == AUTO_READ || at == AUTO_TOUCH || at == AUTO_WRITE)
        plugin->enableController(param, false);
      
      if(!track || id == -1)
        return;
      
      id = MusECore::genACnum(id, param);
      
      double val = ((Slider*)w)->value();
      plugin->setParam(param, val);
      
      track->setPluginCtrlVal(id, val);
      track->startAutoRecord(id, val);
      
      // Needed so that paging a slider updates a label or other buddy control.
      for (unsigned long i = 0; i < nobj; ++i) {           
            QWidget* widget = gw[i].widget;
            if (widget == w || param != gw[i].param)
                  continue;
            int type   = gw[i].type;
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
            }
      }

//---------------------------------------------------------
//   guiSliderReleased
//---------------------------------------------------------

void PluginGui::guiSliderReleased(int idx)
      {
      int param  = gw[idx].param;
      QWidget *w = gw[idx].widget;
      
      AutomationType at = AUTO_OFF;
      MusECore::AudioTrack* track = plugin->track();
      if(track)
        at = track->automationType();
      
      /* equivalent to
      if ((at == AUTO_OFF) ||
          (at == AUTO_READ) ||
          (at == AUTO_TOUCH && (type != GuiWidgets::QCHECKBOX ||    <--- this type is SLIDER != CHECKBOX -> true
                                !MusEGlobal::audio->isPlaying()) ) ) <--- above==true -> this doesn't matter */
      if (at == AUTO_OFF || at == AUTO_READ || at == AUTO_TOUCH)
        plugin->enableController(param, true);
      
      int id = plugin->id();
      
      if(!track || id == -1)
        return;
      
      id = MusECore::genACnum(id, param);
      
      double val = ((Slider*)w)->value();
      track->stopAutoRecord(id, val);
      }
    
//---------------------------------------------------------
//   guiSliderRightClicked
//---------------------------------------------------------

void PluginGui::guiSliderRightClicked(const QPoint &p, int idx)
{
  int param  = gw[idx].param;
  int id = plugin->id();
  if(id != -1)
    MusEGlobal::song->execAutomationCtlPopup(plugin->track(), p, MusECore::genACnum(id, param));
}

//---------------------------------------------------------
//   guiContextMenuReq
//---------------------------------------------------------

void PluginGui::guiContextMenuReq(int idx)
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
    return new Slider(parent, name.toLatin1().constData(), Qt::Horizontal); 

  return QUiLoader::createWidget(className, parent, name);
};

} // namespace MusEGui
