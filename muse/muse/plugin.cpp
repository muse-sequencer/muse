//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: plugin.cpp,v 1.21.2.23 2009/12/15 22:07:12 spamatica Exp $
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//=========================================================

#include <qdir.h>
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <cmath>
#include <math.h>

#include <qwidget.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qsignalmapper.h>
#include <qpushbutton.h>
#include <qlistview.h>
#include <qtoolbar.h>
#include <qtoolbutton.h>
#include <qwhatsthis.h>
#include <qcheckbox.h>
#include <qtooltip.h>
//#include <qwidgetfactory.h>
#include <qfile.h>
#include <qobjectlist.h>
#include <qcombobox.h>
#include <qbuttongroup.h>
#include <qradiobutton.h>
#include <qmessagebox.h>
#include <qtimer.h>

#include "globals.h"
#include "gconfig.h"
#include "filedialog.h"
#include "slider.h"
#include "midictrl.h"
#include "plugin.h"
#include "xml.h"
#include "icons.h"
#include "song.h"
#include "doublelabel.h"
#include "fastlog.h"
#include "checkbox.h"

#include "audio.h"
#include "al/dsp.h"

#define PLUGIN_DEBUGIN 0

PluginList plugins;

/*
static const char* preset_file_pattern[] = {
      QT_TR_NOOP("Presets (*.pre *.pre.gz *.pre.bz2)"),
      QT_TR_NOOP("All Files (*)"),
      0
      };

static const char* preset_file_save_pattern[] = {
      QT_TR_NOOP("Presets (*.pre)"),
      QT_TR_NOOP("gzip compressed presets (*.pre.gz)"),
      QT_TR_NOOP("bzip2 compressed presets (*.pre.bz2)"),
      QT_TR_NOOP("All Files (*)"),
      0
      };
*/

int PluginDialog::selectedPlugType = 0;
QStringList PluginDialog::sortItems = QStringList();

//---------------------------------------------------------
//   ladspa2MidiControlValues
//---------------------------------------------------------

bool ladspa2MidiControlValues(const LADSPA_Descriptor* plugin, int port, int ctlnum, int* min, int* max, int* def)
{
  LADSPA_PortRangeHint range = plugin->PortRangeHints[port];
  LADSPA_PortRangeHintDescriptor desc = range.HintDescriptor;
  
  float fmin, fmax, fdef;
  int   imin, imax;
  float frng;
  //int idef;
  
  //ladspaControlRange(plugin, port, &fmin, &fmax);
  
  bool hasdef = ladspaDefaultValue(plugin, port, &fdef); 
  //bool isint = desc & LADSPA_HINT_INTEGER;
  MidiController::ControllerType t = midiControllerType(ctlnum);
  
  if(PLUGIN_DEBUGIN)
    printf("ladspa2MidiControlValues: ctlnum:%d ladspa port:%d has default?:%d default:%f\n", ctlnum, port, hasdef, fdef);
  
  if(desc & LADSPA_HINT_TOGGLED) 
  {
    if(PLUGIN_DEBUGIN)
      printf("ladspa2MidiControlValues: has LADSPA_HINT_TOGGLED\n");
    
    *min = 0;
    *max = 1;
    *def = (int)lrint(fdef);
    return hasdef;
  }
  
  float m = 1.0;
  if(desc & LADSPA_HINT_SAMPLE_RATE)
  {
    if(PLUGIN_DEBUGIN)
      printf("ladspa2MidiControlValues: has LADSPA_HINT_SAMPLE_RATE\n");
    
    m = float(sampleRate);
  }  
  
  if(desc & LADSPA_HINT_BOUNDED_BELOW)
  {
    if(PLUGIN_DEBUGIN)
      printf("ladspa2MidiControlValues: has LADSPA_HINT_BOUNDED_BELOW\n");
    
    fmin =  range.LowerBound * m;
  }  
  else
    fmin = 0.0;
  
  if(desc & LADSPA_HINT_BOUNDED_ABOVE)
  {  
    if(PLUGIN_DEBUGIN)
      printf("ladspa2MidiControlValues: has LADSPA_HINT_BOUNDED_ABOVE\n");
    
    fmax =  range.UpperBound * m;
  }  
  else
    fmax = 1.0;
    
  frng = fmax - fmin;
  imin = lrint(fmin);  
  imax = lrint(fmax);  
  //irng = imax - imin;

  int ctlmn = 0;
  int ctlmx = 127;
  
  if(PLUGIN_DEBUGIN)
    printf("ladspa2MidiControlValues: port min:%f max:%f \n", fmin, fmax);
  
  //bool isneg = (fmin < 0.0);
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
      //ctlmx = 0xffffff;
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
  //int ctlrng = ctlmx - ctlmn;
  float fctlrng = float(ctlmx - ctlmn);
  
  // Is it an integer control?
  if(desc & LADSPA_HINT_INTEGER)
  {
    if(PLUGIN_DEBUGIN)
      printf("ladspa2MidiControlValues: has LADSPA_HINT_INTEGER\n");
  
    // If the upper or lower limit is beyond the controller limits, just scale the whole range to fit.
    // We could get fancy by scaling only the negative or positive domain, or each one separately, but no...
    //if((imin < ctlmn) || (imax > ctlmx))
    //{
    //  float scl = float(irng) / float(fctlrng);
    //  if((ctlmn - imin) > (ctlmx - imax))
    //    scl = float(ctlmn - imin);
    //  else
    //    scl = float(ctlmx - imax);
    //}
    // No, instead just clip the limits. ie fit the range into clipped space.
    if(imin < ctlmn)
      imin = ctlmn;
    if(imax > ctlmx)
      imax = ctlmx;
      
    *min = imin;
    *max = imax;
    
    //int idef = (int)lrint(fdef);
    //if(idef < ctlmn)
    //  idef = ctlmn;
    //if(idef > ctlmx)
    //  idef = ctlmx;
    //*def = idef;
    
    *def = (int)lrint(fdef);
    
    return hasdef;
  }
  
  // It's a floating point control, just use wide open maximum range.
  *min = ctlmn;
  *max = ctlmx;
  
  float fbias = (fmin + fmax) / 2.0;
  float normbias = fbias / frng;
  float normdef = fdef / frng;
  fdef = normdef * fctlrng;
  
  // FIXME: TODO: Incorrect... Fix this somewhat more trivial stuff later....
  
  *def = (int)lrint(fdef) + bias;
 
  if(PLUGIN_DEBUGIN)
    printf("ladspa2MidiControlValues: setting default:%d\n", *def);
  
  return hasdef;
}      

//---------------------------------------------------------
//   midi2LadspaValue
//---------------------------------------------------------

float midi2LadspaValue(const LADSPA_Descriptor* plugin, int port, int ctlnum, int val)
{
  LADSPA_PortRangeHint range = plugin->PortRangeHints[port];
  LADSPA_PortRangeHintDescriptor desc = range.HintDescriptor;
  
  float fmin, fmax;
  int   imin;
  //int imax;
  float frng;
  //int idef;
  
  //ladspaControlRange(plugin, port, &fmin, &fmax);
  
  //bool hasdef = ladspaDefaultValue(plugin, port, &fdef); 
  //bool isint = desc & LADSPA_HINT_INTEGER;
  MidiController::ControllerType t = midiControllerType(ctlnum);
  
  if(PLUGIN_DEBUGIN)
    printf("midi2LadspaValue: ctlnum:%d ladspa port:%d val:%d\n", ctlnum, port, val);
  
  float m = 1.0;
  if(desc & LADSPA_HINT_SAMPLE_RATE)
  {
    if(PLUGIN_DEBUGIN)
      printf("midi2LadspaValue: has LADSPA_HINT_SAMPLE_RATE\n");
    
    m = float(sampleRate);
  }  
  
  if(desc & LADSPA_HINT_BOUNDED_BELOW)
  {
    if(PLUGIN_DEBUGIN)
      printf("midi2LadspaValue: has LADSPA_HINT_BOUNDED_BELOW\n");
    
    fmin =  range.LowerBound * m;
  }  
  else
    fmin = 0.0;
  
  if(desc & LADSPA_HINT_BOUNDED_ABOVE)
  {  
    if(PLUGIN_DEBUGIN)
      printf("midi2LadspaValue: has LADSPA_HINT_BOUNDED_ABOVE\n");
    
    fmax =  range.UpperBound * m;
  }  
  else
    fmax = 1.0;
    
  frng = fmax - fmin;
  imin = lrint(fmin);  
  //imax = lrint(fmax);  
  //irng = imax - imin;

  if(desc & LADSPA_HINT_TOGGLED) 
  {
    if(PLUGIN_DEBUGIN)
      printf("midi2LadspaValue: has LADSPA_HINT_TOGGLED\n");
    
    if(val > 0)
      return fmax;
    else
      return fmin;
  }
  
  int ctlmn = 0;
  int ctlmx = 127;
  
  if(PLUGIN_DEBUGIN)
    printf("midi2LadspaValue: port min:%f max:%f \n", fmin, fmax);
  
  //bool isneg = (fmin < 0.0);
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
    if(PLUGIN_DEBUGIN)
      printf("midi2LadspaValue: has LADSPA_HINT_INTEGER returning:%f\n", ret);
  
    return ret;  
  }
  
  // Avoid divide-by-zero error below.
  if(ctlrng == 0)
    return 0.0;
    
  // It's a floating point control, just use wide open maximum range.
  float normval = float(bval) / fctlrng;
  //float fbias = (fmin + fmax) / 2.0;
  //float normfbias = fbias / frng;
  //float ret = (normdef + normbias) * fctlrng;
  //float normdef = fdef / frng;
  
  float ret = normval * frng + fmin;
  
  if(PLUGIN_DEBUGIN)
    printf("midi2LadspaValue: float returning:%f\n", ret);
  
  return ret;
}      


// Works but not needed.
/*
//---------------------------------------------------------
//   ladspa2MidiController
//---------------------------------------------------------

MidiController* ladspa2MidiController(const LADSPA_Descriptor* plugin, int port, int ctlnum)
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

//float ladspaDefaultValue(const LADSPA_Descriptor* plugin, int k)
bool ladspaDefaultValue(const LADSPA_Descriptor* plugin, int port, float* val)
{
      LADSPA_PortRangeHint range = plugin->PortRangeHints[port];
      LADSPA_PortRangeHintDescriptor rh = range.HintDescriptor;
//      bool isLog = LADSPA_IS_HINT_LOGARITHMIC(rh);
      //double val = 1.0;
      float m = (rh & LADSPA_HINT_SAMPLE_RATE) ? float(sampleRate) : 1.0f;
      if (LADSPA_IS_HINT_DEFAULT_MINIMUM(rh)) 
      {
        *val = range.LowerBound * m;
        return true;
      }
      else if (LADSPA_IS_HINT_DEFAULT_LOW(rh)) 
      {
            if (LADSPA_IS_HINT_LOGARITHMIC(rh))
            {
              *val = exp(fast_log10(range.LowerBound * m) * .75 +
                     log(range.UpperBound * m) * .25);
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
              *val = exp(log(range.LowerBound * m) * .5 +
                     log10(range.UpperBound * m) * .5);
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
              *val = exp(log(range.LowerBound * m) * .25 +
                     log(range.UpperBound * m) * .75);
              return true;
            }         
            else
            {
              *val = range.LowerBound*.25*m + range.UpperBound*.75*m;
              return true;
            }      
      }
      else if (LADSPA_IS_HINT_DEFAULT_MAXIMUM(rh)) 
      {
            *val = range.UpperBound*m;
            return true;
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
      
      // No default found. Set return value to 1.0, but return false.
      *val = 1.0;
      return false;
}

//---------------------------------------------------------
//   ladspaControlRange
//---------------------------------------------------------

void ladspaControlRange(const LADSPA_Descriptor* plugin, int i, float* min, float* max) 
      {
      LADSPA_PortRangeHint range = plugin->PortRangeHints[i];
      LADSPA_PortRangeHintDescriptor desc = range.HintDescriptor;
      if (desc & LADSPA_HINT_TOGGLED) {
            *min = 0.0;
            *max = 1.0;
            return;
            }
      float m = 1.0;
      if (desc & LADSPA_HINT_SAMPLE_RATE)
            m = float(sampleRate);

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

Plugin::Plugin(QFileInfo* f,
   LADSPA_Descriptor_Function df, const LADSPA_Descriptor* d, bool ip)
   : fi(*f), ladspa(df), plugin(d)
      {
      _inPlaceCapable = ip;
      _inports = 0;
      _outports = 0;
      for (unsigned k = 0; k < d->PortCount; ++k) {
            LADSPA_PortDescriptor pd = d->PortDescriptors[k];
            if (pd &  LADSPA_PORT_CONTROL)
                  continue;
            if (pd &  LADSPA_PORT_INPUT)
                  ++_inports;
            else
                  ++_outports;
            }
      
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
      if ((_inports != _outports) || (f->baseName(true) == QString("dssi-vst") && !config.vstInPlace))
            _inPlaceCapable = false;
      
      _references = 0;
      _instNo     = 0;
      }

//---------------------------------------------------------
//   range
//---------------------------------------------------------

void Plugin::range(int i, float* min, float* max) const
      {
      LADSPA_PortRangeHint range = plugin->PortRangeHints[i];
      LADSPA_PortRangeHintDescriptor desc = range.HintDescriptor;
      if (desc & LADSPA_HINT_TOGGLED) {
            *min = 0.0;
            *max = 1.0;
            return;
            }
      float m = 1.0;
      if (desc & LADSPA_HINT_SAMPLE_RATE)
            m = float(sampleRate);

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
//   defaultValue
//---------------------------------------------------------

double Plugin::defaultValue(unsigned int port) const
{
    if(port >= plugin->PortCount) 
      return 0.0;
      
    LADSPA_PortRangeHint range = plugin->PortRangeHints[port];
    LADSPA_PortRangeHintDescriptor rh = range.HintDescriptor;
    double val = 1.0;
    if (LADSPA_IS_HINT_DEFAULT_MINIMUM(rh))
          val = range.LowerBound;
    else if (LADSPA_IS_HINT_DEFAULT_LOW(rh))
          if (LADSPA_IS_HINT_LOGARITHMIC(range.HintDescriptor))
                val = exp(fast_log10(range.LowerBound) * .75 +
                    log(range.UpperBound) * .25);
          else
                val = range.LowerBound*.75 + range.UpperBound*.25;
    else if (LADSPA_IS_HINT_DEFAULT_MIDDLE(rh))
          if (LADSPA_IS_HINT_LOGARITHMIC(range.HintDescriptor))
                val = exp(log(range.LowerBound) * .5 +
                    log(range.UpperBound) * .5);
          else
                val = range.LowerBound*.5 + range.UpperBound*.5;
    else if (LADSPA_IS_HINT_DEFAULT_HIGH(rh))
          if (LADSPA_IS_HINT_LOGARITHMIC(range.HintDescriptor))
                val = exp(log(range.LowerBound) * .25 +
                    log(range.UpperBound) * .75);
          else
                val = range.LowerBound*.25 + range.UpperBound*.75;
    else if (LADSPA_IS_HINT_DEFAULT_MAXIMUM(rh))
          val = range.UpperBound;
    else if (LADSPA_IS_HINT_DEFAULT_0(rh))
          val = 0.0;
    else if (LADSPA_IS_HINT_DEFAULT_1(rh))
          val = 1.0;
    else if (LADSPA_IS_HINT_DEFAULT_100(rh))
          val = 100.0;
    else if (LADSPA_IS_HINT_DEFAULT_440(rh))
          val = 440.0;
          
    return val;      
}

//---------------------------------------------------------
//   loadPluginLib
//---------------------------------------------------------

static void loadPluginLib(QFileInfo* fi)
      {
      void* handle = dlopen(fi->filePath().ascii(), RTLD_NOW);
      if (handle == 0) {
            fprintf(stderr, "dlopen(%s) failed: %s\n",
              fi->filePath().ascii(), dlerror());
            return;
            }
      LADSPA_Descriptor_Function ladspa = (LADSPA_Descriptor_Function)dlsym(handle, "ladspa_descriptor");

      if (!ladspa) {
            const char *txt = dlerror();
            if (txt) {
                  fprintf(stderr,
                        "Unable to find ladspa_descriptor() function in plugin "
                        "library file \"%s\": %s.\n"
                        "Are you sure this is a LADSPA plugin file?\n",
                        fi->filePath().ascii(),
                        txt);
                  }
                  dlclose(handle);
                  return;
            }
      const LADSPA_Descriptor* descr;
      for (int i = 0;; ++i) {
            descr = ladspa(i);
            if (descr == NULL)
                  break;
            
            // Make sure it doesn't already exist.
            if(plugins.find(fi->baseName(true), QString(descr->Label)) != 0)
              continue;
            
            LADSPA_Properties properties = descr->Properties;
            int ai = 0;
            int ao = 0;
            for (unsigned k = 0; k < descr->PortCount; ++k) {
                  LADSPA_PortDescriptor pd = descr->PortDescriptors[k];
                  if (pd &  LADSPA_PORT_CONTROL)
                        continue;
                  if (pd &  LADSPA_PORT_INPUT)
                        ++ai;
                  else
                        ++ao;
                  }
            bool inPlaceBroken = LADSPA_IS_INPLACE_BROKEN(properties);
            
            plugins.add(fi, ladspa, descr, !inPlaceBroken);
            }
      }

//---------------------------------------------------------
//   loadPluginDir
//---------------------------------------------------------

static void loadPluginDir(const QString& s)
      {
      if (debugMsg)
            printf("scan ladspa plugin dir <%s>\n", s.latin1());
      QDir pluginDir(s, QString("*.so"), QDir::Files);
      if (pluginDir.exists()) {
            const QFileInfoList* list = pluginDir.entryInfoList();
            QFileInfoListIterator it(*list);
            QFileInfo* fi;
            while((fi = it.current())) {
                  loadPluginLib(fi);
                  ++it;
                  }
            }
      }

//---------------------------------------------------------
//   initPlugins
//---------------------------------------------------------

void initPlugins()
      {
      loadPluginDir(museGlobalLib + QString("/plugins"));

      const char* ladspaPath = getenv("LADSPA_PATH");
      if (ladspaPath == 0)
            ladspaPath = "/usr/local/lib64/ladspa:/usr/lib64/ladspa:/usr/local/lib/ladspa:/usr/lib/ladspa";

      const char* p = ladspaPath;
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
      //printf("Plugin <%s> not found\n", name.ascii());
      return 0;
      }

//---------------------------------------------------------
//   Pipeline
//---------------------------------------------------------

Pipeline::Pipeline()
   : std::vector<PluginI*>()
      {
      // Added by Tim. p3.3.15
      for (int i = 0; i < MAX_CHANNELS; ++i)
            posix_memalign((void**)(buffer + i), 16, sizeof(float) * segmentSize);
      
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
            ::free(buffer[i]);
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
              audio->msgSwapControllerIDX(p1->track(), idx, idx - 1);
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
              audio->msgSwapControllerIDX(p1->track(), idx, idx + 1);
            }
      }
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
//   apply
//---------------------------------------------------------

void Pipeline::apply(int ports, unsigned long nframes, float** buffer1)
{
      // prepare a second set of buffers in case a plugin is not
      // capable of inPlace processing

      // Removed by Tim. p3.3.15
      //float* buffer2[ports];
      //float data[nframes * ports];
      //for (int i = 0; i < ports; ++i)
      //      buffer2[i] = data + i * nframes;

      bool swap = false;

      for (iPluginI ip = begin(); ip != end(); ++ip) {
            PluginI* p = *ip;
            if (p && p->on()) {
                  if (p->inPlaceCapable()) 
                  {
                        if (swap)
                              //p->connect(ports, buffer2, buffer2);
                              p->connect(ports, buffer, buffer);
                        else
                              p->connect(ports, buffer1, buffer1);
                  }
                  else 
                  {
                        if (swap)
                              //p->connect(ports, buffer2, buffer1);
                              p->connect(ports, buffer, buffer1);
                        else
                              //p->connect(ports, buffer1, buffer2);
                              p->connect(ports, buffer1, buffer);
                        swap = !swap;
                  }
                  p->apply(nframes);
                  }
            }
      if (swap) 
      {
            for (int i = 0; i < ports; ++i)
                  //memcpy(buffer1[i], buffer2[i], sizeof(float) * nframes);
                  //memcpy(buffer1[i], buffer[i], sizeof(float) * nframes);
                  AL::dsp->cpy(buffer1[i], buffer[i], nframes);
      }
}

//---------------------------------------------------------
//   PluginI
//---------------------------------------------------------

void PluginI::init()
      {
      _plugin           = 0;
      instances         = 0;
      handle            = 0;
      controls          = 0;
      controlPorts      = 0;
      _gui              = 0;
      _on               = true;
      initControlValues = false;
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
      if (_plugin) {
            deactivate();
            _plugin->incReferences(-1);
            }
      if (_gui)
            delete _gui;
      if (controls)
            delete controls;
      if (handle)
            delete handle;
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
  for(int i = 0; i < controlPorts; ++i) 
    audio->msgSetPluginCtrlVal(this, genACnum(_id, i), controls[i].val);
}
  
//---------------------------------------------------------
//   valueType
//---------------------------------------------------------

CtrlValueType PluginI::valueType() const
      {
      return VAL_LINEAR;
      }

//---------------------------------------------------------
//   setChannel
//---------------------------------------------------------

void PluginI::setChannels(int c)
      {
      if (channel == c)
            return;
      int ni = c / _plugin->outports();
      if (ni == 0)
            ni = 1;
      if (ni == instances)
            return;
      channel = c;

      // remove old instances:
      deactivate();
      delete handle;
      instances = ni;
      handle    = new LADSPA_Handle[instances];
      for (int i = 0; i < instances; ++i) {
            handle[i] = _plugin->instantiate();
            if (handle[i] == 0) {
                  printf("cannot instantiate instance %d\n", i);
                  return;
                  }
            }
      int curPort = 0;
      int ports   = _plugin->ports();
      for (int k = 0; k < ports; ++k) {
            LADSPA_PortDescriptor pd = _plugin->portd(k);
            if (pd & LADSPA_PORT_CONTROL) {
                  for (int i = 0; i < instances; ++i)
                        _plugin->connectPort(handle[i], k, &controls[curPort].val);
                  controls[curPort].idx = k;
                  ++curPort;
                  }
            }
      activate();
      }

//---------------------------------------------------------
//   defaultValue
//---------------------------------------------------------

double PluginI::defaultValue(unsigned int param) const
{
//#warning controlPorts should really be unsigned
  if(param >= (unsigned)controlPorts)
    return 0.0;
  
  return _plugin->defaultValue(controls[param].idx);
}

//---------------------------------------------------------
//   initPluginInstance
//    return true on error
//---------------------------------------------------------

bool PluginI::initPluginInstance(Plugin* plug, int c)
      {
      channel = c;
      if (plug == 0) {
            printf("initPluginInstance: zero plugin\n");
            return true;
            }
      _plugin = plug;
      _plugin->incReferences(1);

      QString inst("-" + QString::number(_plugin->instNo()));
      _name  = _plugin->name() + inst;
      _label = _plugin->label() + inst;

      instances = channel/plug->outports();
      if (instances < 1)
            instances = 1;
      handle = new LADSPA_Handle[instances];
      for (int i = 0; i < instances; ++i) {
            handle[i] = _plugin->instantiate();
            if (handle[i] == 0)
                  return true;
            }

      controlPorts = 0;
      int ports    = _plugin->ports();

      for (int k = 0; k < ports; ++k) {
            LADSPA_PortDescriptor pd = _plugin->portd(k);
            if (pd & LADSPA_PORT_CONTROL)
                  ++controlPorts;
            }
      controls = new Port[controlPorts];
      int i = 0;
      for (int k = 0; k < ports; ++k) {
            LADSPA_PortDescriptor pd = _plugin->portd(k);
            if (pd & LADSPA_PORT_CONTROL) {
                  double val = _plugin->defaultValue(k);
                  controls[i].val    = val;
                  controls[i].tmpVal = val;
                  controls[i].enCtrl  = true;
                  controls[i].en2Ctrl = true;
                  ++i;
                  }
            }
      int curPort = 0;
      for (int k = 0; k < ports; ++k) {
            LADSPA_PortDescriptor pd = _plugin->portd(k);
            if (pd & LADSPA_PORT_CONTROL) {
                  for (int i = 0; i < instances; ++i)
                        _plugin->connectPort(handle[i], k, &controls[curPort].val);
                  controls[curPort].idx = k;
                  ++curPort;
                  }
            }
      activate();
      return false;
      }

//---------------------------------------------------------
//   connect
//---------------------------------------------------------

void PluginI::connect(int ports, float** src, float** dst)
      {
      int port = 0;
      for (int i = 0; i < instances; ++i) {
            for (int k = 0; k < _plugin->ports(); ++k) {
                  if (isAudioIn(k)) {
                        _plugin->connectPort(handle[i], k, src[port]);
                        port = (port + 1) % ports;
                        }
                  }
            }
      port = 0;
      for (int i = 0; i < instances; ++i) {
            for (int k = 0; k < _plugin->ports(); ++k) {
                  if (isAudioOut(k)) {
                        _plugin->connectPort(handle[i], k, dst[port]);
                        port = (port + 1) % ports;  // overwrite output?
//                        ++port;
//                        if (port >= ports) {
//                              return;
//                              }
                        }
                  }
            }
      }

//---------------------------------------------------------
//   setControl
//    set plugin instance controller value by name
//---------------------------------------------------------

bool PluginI::setControl(const QString& s, double val)
      {
      for (int i = 0; i < controlPorts; ++i) {
            if (_plugin->portName(controls[i].idx) == s) {
                  controls[i].val = controls[i].tmpVal = val;
                  return false;
                  }
            }
      printf("PluginI:setControl(%s, %f) controller not found\n",
         s.latin1(), val);
      return true;
      }

//---------------------------------------------------------
//   saveConfiguration
//---------------------------------------------------------

void PluginI::writeConfiguration(int level, Xml& xml)
      {
      xml.tag(level++, "plugin file=\"%s\" label=\"%s\" channel=\"%d\"",
         _plugin->lib().latin1(), _plugin->label().latin1(), instances * _plugin->inports());
      for (int i = 0; i < controlPorts; ++i) {
            int idx = controls[i].idx;
            QString s("control name=\"%1\" val=\"%2\" /");
            xml.tag(level, s.arg(_plugin->portName(idx)).arg(controls[i].tmpVal).latin1());
            }
      if (_on == false)
            xml.intTag(level, "on", _on);
      if (guiVisible()) {
            xml.intTag(level, "gui", 1);
            xml.geometryTag(level, "geometry", _gui);
            }
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
                              if (setControl(name, val)) {
                                    return false;
                                    }
                              initControlValues = true;
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
            instances = 1;

      for (;;) {
            Xml::Token token(xml.parse());
            const QString& tag(xml.s1());
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return true;
                  case Xml::TagStart:
                        if (!readPreset && _plugin == 0) {
                              _plugin = plugins.find(file, label);
                              if (_plugin && initPluginInstance(_plugin, instances)) {
                                    _plugin = 0;
                                    xml.parse1();
                                    break;
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
                              showGui(flag);
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
                                             s.latin1(), plugin()->lib().latin1());
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
                                    instances = xml.s2().toInt();
                              }
                        break;
                  case Xml::TagEnd:
                        if (tag == "plugin") {
                              if (!readPreset && _plugin == 0) {
                                    _plugin = plugins.find(file, label);
                                    if (_plugin == 0)
                                          return true;
                                    if (initPluginInstance(_plugin, instances))
                                          return true;
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
//   makeGui
//---------------------------------------------------------

void PluginI::makeGui()
      {
      _gui = new PluginGui(this);
      }

//---------------------------------------------------------
//   deleteGui
//---------------------------------------------------------
void PluginI::deleteGui()
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
  for(int i = 0; i < controlPorts; ++i) 
    controls[i].enCtrl = v;
}

//---------------------------------------------------------
//   enable2AllControllers
//---------------------------------------------------------

void PluginI::enable2AllControllers(bool v)
{
  for(int i = 0; i < controlPorts; ++i) 
    controls[i].en2Ctrl = v;
}

//---------------------------------------------------------
//   apply
//---------------------------------------------------------

void PluginI::apply(int n)
      {
      if(automation && _track && _track->automationType() != AUTO_OFF && _id != -1)
      {
        for(int i = 0; i < controlPorts; ++i)
        {
          if( controls[i].enCtrl && controls[i].en2Ctrl )
            controls[i].tmpVal = _track->pluginCtrlVal(genACnum(_id, i));
        }  
      }      
      
      for (int i = 0; i < controlPorts; ++i)
            controls[i].val = controls[i].tmpVal;
      for (int i = 0; i < instances; ++i)
            _plugin->apply(handle[i], n);
      }

//---------------------------------------------------------
//   PluginDialog
//    select Plugin dialog
//---------------------------------------------------------

PluginDialog::PluginDialog(QWidget* parent, const char* name, bool modal)
  : QDialog(parent, name, modal)
      {
      setCaption(tr("MusE: select plugin"));
      QVBoxLayout* layout = new QVBoxLayout(this);

      pList  = new QListView(this);
      pList->setAllColumnsShowFocus(true);
      pList->addColumn(tr("Lib"),   110);
      pList->addColumn(tr("Label"), 110);
      pList->addColumn(tr("Name"),  200);
      pList->addColumn(tr("AI"),    30);
      pList->addColumn(tr("AO"),    30);
      pList->addColumn(tr("CI"),    30);
      pList->addColumn(tr("CO"),    30);
      pList->addColumn(tr("IP"),    30);
      pList->addColumn(tr("id"),    40);
      pList->addColumn(tr("Maker"), 110);
      pList->addColumn(tr("Copyright"), 110);
      pList->setColumnWidthMode(1, QListView::Maximum);

      layout->addWidget(pList);

      //---------------------------------------------------
      //  Ok/Cancel Buttons
      //---------------------------------------------------

      QBoxLayout* w5 = new QHBoxLayout;
      layout->addLayout(w5);

      QPushButton* okB     = new QPushButton(tr("Ok"), this);
      okB->setDefault(true);
      QPushButton* cancelB = new QPushButton(tr("Cancel"), this);
      okB->setFixedWidth(80);
      cancelB->setFixedWidth(80);
      w5->addWidget(okB);
      w5->addSpacing(12);
      w5->addWidget(cancelB);

      QButtonGroup* plugSel = new QButtonGroup(4, Qt::Horizontal, this, "Show plugs:");
      plugSel->setTitle("Show plugs:");
      QRadioButton* onlySM = new QRadioButton(plugSel, "Mono and Stereo");
      onlySM->setText( "Mono and Stereo");
      QRadioButton* onlyS = new QRadioButton(plugSel, "Stereo");
      onlyS->setText( "Stereo");
      QRadioButton* onlyM = new QRadioButton(plugSel, "Mono");
      onlyM->setText( "Mono");
      QRadioButton* allPlug = new QRadioButton(plugSel, "Show all");
      allPlug->setText( "Show All");

      plugSel->setRadioButtonExclusive(true);
      plugSel->setButton(selectedPlugType);

      QToolTip::add(plugSel, tr("Select which types of plugins should be visible in the list.<br>"
                             "Note that using mono plugins on stereo tracks is not a problem, two will be used in parallel.<br>"
                             "Also beware that the 'all' alternative includes plugins that probably are not usable by MusE."));

      onlySM->setCaption(tr("Stereo and Mono"));
      onlyS->setCaption(tr("Stereo"));
      onlyM->setCaption(tr("Mono"));
      allPlug->setCaption(tr("All"));

      w5->addSpacing(12);
      w5->addWidget(plugSel);
      w5->addSpacing(12);
      
      QLabel *sortLabel = new QLabel( this, "Search in 'Label' and 'Name':" );
      sortLabel->setText( "Search in 'Label' and 'Name':" );
      w5->addWidget(sortLabel);
      w5->addSpacing(2);
      
      sortBox = new QComboBox( true, this, "sort" );
      if (!sortItems.empty())
          sortBox->insertStringList(sortItems);
      
      sortBox->setMinimumSize( 100, 10);
      w5->addWidget(sortBox);
      w5->addStretch(-1);
      
      if (sortBox->currentText())
          fillPlugs(sortBox->currentText());
      else
          fillPlugs(selectedPlugType);
      

      connect(pList,   SIGNAL(doubleClicked(QListViewItem*)), SLOT(accept()));
      connect(cancelB, SIGNAL(clicked()), SLOT(reject()));
      connect(okB,     SIGNAL(clicked()), SLOT(accept()));
      connect(plugSel, SIGNAL(clicked(int)), SLOT(fillPlugs(int)));
      connect(sortBox, SIGNAL(textChanged(const QString&)),SLOT(fillPlugs(const QString&)));
      sortBox->setFocus();
      }

//---------------------------------------------------------
//   value
//---------------------------------------------------------

Plugin* PluginDialog::value()
      {
      QListViewItem* item = pList->selectedItem();
      if (item)
            return plugins.find(item->text(0), item->text(1));
      return 0;
      }

//---------------------------------------------------------
//   accept
//---------------------------------------------------------

void PluginDialog::accept()
      {
      if (!sortBox->currentText().isEmpty()) {
          if (sortItems.find(sortBox->currentText()) == sortItems.end())
              sortItems.push_front(sortBox->currentText());
      }
      QDialog::accept();
      }

//---------------------------------------------------------
//    fillPlugs
//---------------------------------------------------------

void PluginDialog::fillPlugs(int nbr)
      {
      pList->clear();
      for (iPlugin i = plugins.begin(); i != plugins.end(); ++i) {
        int ai = 0;
        int ao = 0;
        int ci = 0;
        int co = 0;
        for (int k = 0; k < i->ports(); ++k) {
          LADSPA_PortDescriptor pd = i->portd(k);
          if (pd &  LADSPA_PORT_CONTROL) {
              if (pd &  LADSPA_PORT_INPUT)
                  ++ci;
              else
                  ++co;
                  }
          else {
              if (pd &  LADSPA_PORT_INPUT)
                  ++ai;
              else
                  ++ao;
                  }
              }
    
          bool addFlag = false;
          switch(nbr)
              {
              case 0: // stereo & mono
                if ((ai == 1 || ai == 2) && (ao == 1 || ao ==2))
                  {
                  addFlag = true;
                  }
                break;
              case 1: // stereo
                if ((ai == 1 || ai == 2) &&  ao ==2)
                  {
                  addFlag = true;
                  }
                break;
              case 2: // mono
                if (ai == 1  && ao == 1)
                  {
                  addFlag = true;
                  }
                break;
              case 3: // all
                  addFlag = true;
                break;
              }
          if (addFlag)
              {
              QListViewItem* item = new QListViewItem(pList,
              i->lib(),
              i->label(),
              i->name(),
              QString().setNum(ai),
              QString().setNum(ao),
              QString().setNum(ci),
              QString().setNum(co),
              QString().setNum(i->inPlaceCapable())
              );
              item->setText(8, QString().setNum(i->id()));
              item->setText(9, i->maker());
              item->setText(10, i->copyright());
            }
          }
        selectedPlugType = nbr;
        }

void PluginDialog::fillPlugs(const QString &sortValue)
      {
      pList->clear();
      for (iPlugin i = plugins.begin(); i != plugins.end(); ++i) {
        int ai = 0;
        int ao = 0;
        int ci = 0;
        int co = 0;
        for (int k = 0; k < i->ports(); ++k) {
          LADSPA_PortDescriptor pd = i->portd(k);
          if (pd &  LADSPA_PORT_CONTROL) {
              if (pd &  LADSPA_PORT_INPUT)
                  ++ci;
              else
                  ++co;
                  }
          else {
              if (pd &  LADSPA_PORT_INPUT)
                  ++ai;
              else
                  ++ao;
                  }
              }
    
          bool addFlag = false;
          
          if (i->label().lower().contains(sortValue.lower()))
                addFlag = true;
          else if (i->name().lower().contains(sortValue.lower()))
                addFlag = true;
          if (addFlag)
              {
              QListViewItem* item = new QListViewItem(pList,
              i->lib(),
              i->label(),
              i->name(),
              QString().setNum(ai),
              QString().setNum(ao),
              QString().setNum(ci),
              QString().setNum(co),
              QString().setNum(i->inPlaceCapable())
              );
              item->setText(8, QString().setNum(i->id()));
              item->setText(9, i->maker());
              item->setText(10, i->copyright());
              }
          }
      }

//---------------------------------------------------------
//   getPlugin
//---------------------------------------------------------

Plugin* PluginDialog::getPlugin(QWidget* parent)
      {
      PluginDialog* dialog = new PluginDialog(parent);
      if (dialog->exec())
            return dialog->value();
      return 0;
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
            for (int i = 0; i < controlPorts; ++i) {
                  controls[i].val = controls[i].tmpVal;
                  }
            }
      else {
            //
            // get initial control values from plugin
            //
            for (int i = 0; i < controlPorts; ++i) {
                  controls[i].tmpVal = controls[i].val;
                  }
            }
      }

const char* presetOpenText = "<img source=\"fileopen\"> "
      "Click this button to load a saved <em>preset</em>.";
const char* presetSaveText = "Click this button to save curent parameter "
      "settings as a <em>preset</em>.  You will be prompted for a file name.";
const char* presetBypassText = "Click this button to bypass effect unit";

//---------------------------------------------------------
//   PluginGui
//---------------------------------------------------------

PluginGui::PluginGui(PluginI* p)
   : QMainWindow(0)
      {
      gw     = 0;
      params = 0;
      plugin = p;
      setCaption(plugin->name());

      QToolBar* tools = new QToolBar(tr("File Buttons"), this);
      QToolButton* fileOpen = new QToolButton(
         QIconSet(*openIconS, *openIcon),
         tr("Load Preset"),
     QString::null, this, SLOT(load()),
     tools, "load preset" );

      QToolButton* fileSave = new QToolButton(
         QIconSet(*saveIconS, *saveIcon),
         tr("Save Preset"),
     QString::null,
     this, SLOT(save()),
     tools, "save preset");

      QWhatsThis::whatsThisButton(tools);

      onOff = new QToolButton(tools, "bypass");
      onOff->setIconSet(*exitIconS);
      onOff->setToggleButton(true);
      onOff->setOn(plugin->on());
      QToolTip::add(onOff, tr("bypass plugin"));
      connect(onOff, SIGNAL(toggled(bool)), SLOT(bypassToggled(bool)));

      QWhatsThis::add(fileOpen, tr(presetOpenText));
      QWhatsThis::add(onOff, tr(presetBypassText));
      QMimeSourceFactory::defaultFactory()->setPixmap(QString("fileopen"), *openIcon );
      QWhatsThis::add(fileSave, tr(presetSaveText));

      QString id;
      id.setNum(plugin->plugin()->id());
      QString name(museGlobalShare + QString("/plugins/") + id + QString(".ui"));
      QFile uifile(name);
      if (uifile.exists()) {
            //
            // construct GUI from *.ui file
            //
            mw = QWidgetFactory::create(uifile.name(), 0, this);
            setCentralWidget(mw);

            const QObjectList* l = mw->children();
            QObject *obj;

            nobj = 0;
            QObjectListIt it(*l);
            while ((obj = it.current()) != 0) {
                  ++it;
                  const char* name = obj->name();
                  if (*name !='P')
                        continue;
                  int parameter = -1;
                  sscanf(name, "P%d", &parameter);
                  if (parameter == -1)
                        continue;
                  ++nobj;
                  }
            it.toFirst();
            gw   = new GuiWidgets[nobj];
            nobj = 0;
            QSignalMapper* mapper = new QSignalMapper(this, "pluginGuiMapper");
            connect(mapper, SIGNAL(mapped(int)), SLOT(guiParamChanged(int)));
            
            QSignalMapper* mapperPressed = new QSignalMapper(this, "pluginGuiMapperPressed");
            QSignalMapper* mapperReleased = new QSignalMapper(this, "pluginGuiMapperReleased");
            connect(mapperPressed, SIGNAL(mapped(int)), SLOT(guiParamPressed(int)));
            connect(mapperReleased, SIGNAL(mapped(int)), SLOT(guiParamReleased(int)));
            
            while ((obj = it.current()) != 0) {
                  ++it;
                  const char* name = obj->name();
                  if (*name !='P')
                        continue;
                  int parameter = -1;
                  sscanf(name, "P%d", &parameter);
                  if (parameter == -1)
                        continue;

                  mapper->setMapping(obj, nobj);
                  mapperPressed->setMapping(obj, nobj);
                  mapperReleased->setMapping(obj, nobj);
                  
                  gw[nobj].widget = (QWidget*)obj;
                  gw[nobj].param  = parameter;
                  gw[nobj].type   = -1;

                  if (strcmp(obj->className(), "Slider") == 0) {
                        gw[nobj].type = GuiWidgets::SLIDER;
                        ((Slider*)obj)->setId(nobj);
                        ((Slider*)obj)->setCursorHoming(true);
                        for(int i = 0; i < nobj; i++)
                        {
                          if(gw[i].type == GuiWidgets::DOUBLE_LABEL && gw[i].param == parameter)
                            ((DoubleLabel*)gw[i].widget)->setSlider((Slider*)obj);
                        }
                        connect(obj, SIGNAL(sliderMoved(double,int)), mapper, SLOT(map()));
                        connect(obj, SIGNAL(sliderPressed(int)), SLOT(guiSliderPressed(int)));
                        connect(obj, SIGNAL(sliderReleased(int)), SLOT(guiSliderReleased(int)));
                        connect(obj, SIGNAL(sliderRightClicked(const QPoint &, int)), SLOT(guiSliderRightClicked(const QPoint &, int)));
                        }
                  else if (strcmp(obj->className(), "DoubleLabel") == 0) {
                        gw[nobj].type = GuiWidgets::DOUBLE_LABEL;
                        ((DoubleLabel*)obj)->setId(nobj);
                        for(int i = 0; i < nobj; i++)
                        {
                          if(gw[i].type == GuiWidgets::SLIDER && gw[i].param == parameter)
                          {
                            ((DoubleLabel*)obj)->setSlider((Slider*)gw[i].widget);
                            break;  
                          }  
                        }
                        connect(obj, SIGNAL(valueChanged(double,int)), mapper, SLOT(map()));
                        }
                  else if (strcmp(obj->className(), "QCheckBox") == 0) {
                        gw[nobj].type = GuiWidgets::QCHECKBOX;
                        connect(obj, SIGNAL(toggled(bool)), mapper, SLOT(map()));
                        connect(obj, SIGNAL(pressed()), mapperPressed, SLOT(map()));
                        connect(obj, SIGNAL(released()), mapperReleased, SLOT(map()));
                        }
                  else if (strcmp(obj->className(), "QComboBox") == 0) {
                        gw[nobj].type = GuiWidgets::QCOMBOBOX;
                        connect(obj, SIGNAL(activated(int)), mapper, SLOT(map()));
                        }
                  else {
                        printf("unknown widget class %s\n", obj->className());
                        continue;
                        }
                  ++nobj;
                  }
              updateValues(); // otherwise the GUI won't have valid data
            }
      else {
            mw = new QWidget(this);
            setCentralWidget(mw);
            QGridLayout* grid = new QGridLayout(mw);
            grid->setSpacing(2);

            int n  = plugin->parameters();
            params = new GuiParam[n];

            resize(280, n*20+30);

            //int style       = Slider::BgTrough | Slider::BgSlot;
            QFontMetrics fm = fontMetrics();
            int h           = fm.height() + 4;

            for (int i = 0; i < n; ++i) {
                  QLabel* label = 0;
                  LADSPA_PortRangeHint range = plugin->range(i);
                  double lower = 0.0;     // default values
                  double upper = 1.0;
                  double dlower = lower;
                  double dupper = upper;
                  double val   = plugin->param(i);
                  double dval  = val;
                  params[i].hint = range.HintDescriptor;

                  if (LADSPA_IS_HINT_BOUNDED_BELOW(range.HintDescriptor)) {
                        dlower = lower = range.LowerBound;
                        }
                  if (LADSPA_IS_HINT_BOUNDED_ABOVE(range.HintDescriptor)) {
                        dupper = upper = range.UpperBound;
                        }
                  if (LADSPA_IS_HINT_SAMPLE_RATE(range.HintDescriptor)) {
                        lower *= sampleRate;
                        upper *= sampleRate;
                        dlower = lower;
                        dupper = upper;
                        }
                  if (LADSPA_IS_HINT_LOGARITHMIC(range.HintDescriptor)) {
                        if (lower == 0.0)
                              lower = 0.001;
                        dlower = fast_log10(lower)*20.0;
                        dupper = fast_log10(upper)*20.0;
                        dval  = fast_log10(val) * 20.0;
                        }
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
                        label           = new QLabel(QString(plugin->paramName(i)), mw);
                        params[i].type  = GuiParam::GUI_SLIDER;
                        params[i].label = new DoubleLabel(val, lower, upper, mw);
                        params[i].label->setFrame(true);
                        params[i].label->setPrecision(2);
                        params[i].label->setId(i);

                        //params[i].label->setMargin(2);
                        //params[i].label->setFixedHeight(h);

                        Slider* s = new Slider(mw, "param", Slider::Horizontal,
                           Slider::None); //, style);
                           
                        s->setCursorHoming(true);
                        s->setId(i);
                        //s->setFixedHeight(h);
                        s->setRange(dlower, dupper);
                        if(LADSPA_IS_HINT_INTEGER(range.HintDescriptor))
                          s->setStep(1.0);
                        s->setValue(dval);
                        params[i].actuator = s;
                        params[i].label->setSlider((Slider*)params[i].actuator);
                        }
                  //params[i].actuator->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum));
                  params[i].actuator->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
                  if (params[i].type == GuiParam::GUI_SLIDER) {
                        //label->setFixedHeight(20);
                        //label->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum));
                        label->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
                        //params[i].label->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum));
                        params[i].label->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
                        grid->addWidget(label, i, 0);
                        grid->addWidget(params[i].label,    i, 1);
                        grid->addWidget(params[i].actuator, i, 2);
                        }
                  else if (params[i].type == GuiParam::GUI_SWITCH) {
                        grid->addMultiCellWidget(params[i].actuator, i, i, 0, 2);
                        }
                  if (params[i].type == GuiParam::GUI_SLIDER) {
                        connect(params[i].actuator, SIGNAL(sliderMoved(double,int)), SLOT(sliderChanged(double,int)));
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
            grid->setColStretch(2, 10);
            }
      connect(heartBeatTimer, SIGNAL(timeout()), SLOT(heartBeat()));
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

void PluginGui::ctrlPressed(int param)
{
      AutomationType at = AUTO_OFF;
      AudioTrack* track = 0;
      if(plugin->track())
      {
        track = plugin->track();
        at = track->automationType();
      }  
            
      if(at != AUTO_OFF)
        plugin->enableController(param, false);
      
      int id = plugin->id();
      
      if(id == -1)
        return;
        
      id = genACnum(id, param);
      
      if(params[param].type == GuiParam::GUI_SLIDER)
      {
        double val = ((Slider*)params[param].actuator)->value();
        if (LADSPA_IS_HINT_LOGARITHMIC(params[param].hint))
              val = pow(10.0, val/20.0);
        else if (LADSPA_IS_HINT_INTEGER(params[param].hint))
              val = rint(val);
        plugin->setParam(param, val);
        ((DoubleLabel*)params[param].label)->setValue(val);
        audio->msgSetPluginCtrlVal(((PluginI*)plugin), id, val);
        if(track)
          track->startAutoRecord(id, val);
      }       
      else if(params[param].type == GuiParam::GUI_SWITCH)
      {
        double val = (double)((CheckBox*)params[param].actuator)->isChecked();
        plugin->setParam(param, val);
        audio->msgSetPluginCtrlVal(((PluginI*)plugin), id, val);
        if(track)
          track->startAutoRecord(id, val);
      }       
}

//---------------------------------------------------------
//   ctrlReleased
//---------------------------------------------------------

void PluginGui::ctrlReleased(int param)
{
      AutomationType at = AUTO_OFF;
      AudioTrack* track = 0;
      if(plugin->track())
      {
        track = plugin->track();
        at = track->automationType();
      }  
        
      // Special for switch - don't enable controller until transport stopped.
      if(at != AUTO_WRITE && ((params[param].type != GuiParam::GUI_SWITCH
          || !audio->isPlaying()
          || at != AUTO_TOUCH) || (!audio->isPlaying() && at == AUTO_TOUCH)) )
        plugin->enableController(param, true);
      
      int id = plugin->id();
      if(!track || id == -1)
        return;
      id = genACnum(id, param);
        
      if(params[param].type == GuiParam::GUI_SLIDER)
      {
        double val = ((Slider*)params[param].actuator)->value();
        if (LADSPA_IS_HINT_LOGARITHMIC(params[param].hint))
              val = pow(10.0, val/20.0);
        else if (LADSPA_IS_HINT_INTEGER(params[param].hint))
              val = rint(val);
        track->stopAutoRecord(id, val);
      }       
      //else if(params[param].type == GuiParam::GUI_SWITCH)
      //{
        //double val = (double)((CheckBox*)params[param].actuator)->isChecked();
        // No concept of 'untouching' a checkbox. Remain 'touched' until stop.
        //plugin->track()->stopAutoRecord(genACnum(plugin->id(), param), val);
      //}       
}

//---------------------------------------------------------
//   ctrlRightClicked
//---------------------------------------------------------

void PluginGui::ctrlRightClicked(const QPoint &p, int param)
{
  int id = plugin->id();
  if(id != -1)
    song->execAutomationCtlPopup((AudioTrack*)plugin->track(), p, genACnum(id, param));
}

//---------------------------------------------------------
//   sliderChanged
//---------------------------------------------------------

void PluginGui::sliderChanged(double val, int param)
      {
      AutomationType at = AUTO_OFF;
      AudioTrack* track = 0;
      if(plugin->track())
      {
        track = plugin->track();
        at = track->automationType();
      }  
      
      if(at == AUTO_WRITE || (audio->isPlaying() && at == AUTO_TOUCH))
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
      id = genACnum(id, param);
          
      audio->msgSetPluginCtrlVal(((PluginI*)plugin), id, val);
          
      if(track)
        track->recordAutomation(id, val);
      }

//---------------------------------------------------------
//   labelChanged
//---------------------------------------------------------

void PluginGui::labelChanged(double val, int param)
      {
      AutomationType at = AUTO_OFF;
      AudioTrack* track = 0;
      if(plugin->track())
      {
        track = plugin->track();
        at = track->automationType();
      }  
      
      if(at == AUTO_WRITE || (audio->isPlaying() && at == AUTO_TOUCH))
        plugin->enableController(param, false);
      
      double dval = val;
      if (LADSPA_IS_HINT_LOGARITHMIC(params[param].hint))
            dval = fast_log10(val) * 20.0;
      else if (LADSPA_IS_HINT_INTEGER(params[param].hint))
            dval = rint(val);
      if (plugin->param(param) != val) {
            plugin->setParam(param, val);
            ((Slider*)params[param].actuator)->setValue(dval);
            }
      
      int id = plugin->id();
      if(id == -1)
        return;
      
      id = genACnum(id, param);
      audio->msgSetPluginCtrlVal(((PluginI*)plugin), id, val);
      if(track)
        track->startAutoRecord(id, val);
      }

//---------------------------------------------------------
//   load
//---------------------------------------------------------

void PluginGui::load()
      {
      QString s("presets/plugins/");
      s += plugin->plugin()->label();
      s += "/";

      QString fn = getOpenFileName(s, preset_file_pattern,
         this, tr("MusE: load preset"), 0);
      if (fn.isEmpty())
            return;
      bool popenFlag;
      FILE* f = fileOpen(this, fn, QString(".pre"), "r", popenFlag, true);
      if (f == 0)
            return;

      Xml xml(f);
      int mode = 0;
      for (;;) {
            Xml::Token token = xml.parse();
            QString tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::TagStart:
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
                  case Xml::Attribut:
                        break;
                  case Xml::TagEnd:
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
      s += plugin->plugin()->label();
      s += "/";

      //QString fn = getSaveFileName(s, preset_file_pattern, this,
      QString fn = getSaveFileName(s, preset_file_save_pattern, this,
        tr("MusE: save preset"));
      if (fn.isEmpty())
            return;
      bool popenFlag;
      FILE* f = fileOpen(this, fn, QString(".pre"), "w", popenFlag, false, true);
      if (f == 0)
            return;
      Xml xml(f);
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
      plugin->setOn(val);
      song->update(SC_ROUTE);
      }

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void PluginGui::setOn(bool val)
      {
      onOff->blockSignals(true);
      onOff->setOn(val);
      onOff->blockSignals(false);
      }

//---------------------------------------------------------
//   updateValues
//---------------------------------------------------------

void PluginGui::updateValues()
      {
      if (params) {
            for (int i = 0; i < plugin->parameters(); ++i) {
                  GuiParam* gp = &params[i];
                  if (gp->type == GuiParam::GUI_SLIDER) {
                        double lv = plugin->param(i);
                        double sv = lv;
                        if (LADSPA_IS_HINT_LOGARITHMIC(params[i].hint))
                              sv = fast_log10(lv) * 20.0;
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
            for (int i = 0; i < nobj; ++i) {
                  QWidget* widget = gw[i].widget;
                  int type = gw[i].type;
                  int param = gw[i].param;
                  double val = plugin->param(param);
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
                              ((QComboBox*)widget)->setCurrentItem(int(val));
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
      if(!automation || !plugin->track() || plugin->id() == -1)
        return;
      AutomationType at = plugin->track()->automationType();
      if(at == AUTO_OFF)
        return;
      if (params) {
            for (int i = 0; i < plugin->parameters(); ++i) {
                  GuiParam* gp = &params[i];
                  if (gp->type == GuiParam::GUI_SLIDER) {
                        if( plugin->controllerEnabled(i) && plugin->controllerEnabled2(i) )
                          {
                            double lv = plugin->track()->pluginCtrlVal(genACnum(plugin->id(), i));
                            double sv = lv;
                            if (LADSPA_IS_HINT_LOGARITHMIC(params[i].hint))
                                  sv = fast_log10(lv) * 20.0;
                            else 
                            if (LADSPA_IS_HINT_INTEGER(params[i].hint))
                            {
                                  sv = rint(lv);
                                  lv = sv;
                            }      
                            if(((Slider*)(gp->actuator))->value() != sv)
                            {
                              // Added by Tim. p3.3.6
                              //printf("PluginGui::updateControls slider\n");
                              
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
                        if( plugin->controllerEnabled(i) && plugin->controllerEnabled2(i) )
                          {
                            bool v = (int)plugin->track()->pluginCtrlVal(genACnum(plugin->id(), i));
                            if(((CheckBox*)(gp->actuator))->isChecked() != v)
                            {
                              // Added by Tim. p3.3.6
                              //printf("PluginGui::updateControls switch\n");
                              
                              ((CheckBox*)(gp->actuator))->blockSignals(true);
                              ((CheckBox*)(gp->actuator))->setChecked(v);
                              ((CheckBox*)(gp->actuator))->blockSignals(false);
                            } 
                          }
                        }
                  }
            }
      else if (gw) {
            for (int i = 0; i < nobj; ++i) {
                  QWidget* widget = gw[i].widget;
                  int type = gw[i].type;
                  int param = gw[i].param;
                  switch(type) {
                        case GuiWidgets::SLIDER:
                              if( plugin->controllerEnabled(param) && plugin->controllerEnabled2(param) )
                              {
                                double v = plugin->track()->pluginCtrlVal(genACnum(plugin->id(), param));
                                if(((Slider*)widget)->value() != v)
                                {
                                  // Added by Tim. p3.3.6
                                  //printf("PluginGui::updateControls slider\n");
                              
                                  ((Slider*)widget)->blockSignals(true);
                                  ((Slider*)widget)->setValue(v);
                                  ((Slider*)widget)->blockSignals(false);
                                }
                              }
                              break;
                        case GuiWidgets::DOUBLE_LABEL:
                              if( plugin->controllerEnabled(param) && plugin->controllerEnabled2(param) )
                              {
                                double v = plugin->track()->pluginCtrlVal(genACnum(plugin->id(), param));
                                if(((DoubleLabel*)widget)->value() != v)
                                {
                                  // Added by Tim. p3.3.6
                                  //printf("PluginGui::updateControls label\n");
                              
                                  ((DoubleLabel*)widget)->blockSignals(true);
                                  ((DoubleLabel*)widget)->setValue(v);
                                  ((DoubleLabel*)widget)->blockSignals(false);
                                }
                              }
                              break;
                        case GuiWidgets::QCHECKBOX:
                              if( plugin->controllerEnabled(param) && plugin->controllerEnabled2(param) )
                              { 
                                bool b = (bool) plugin->track()->pluginCtrlVal(genACnum(plugin->id(), param));
                                if(((QCheckBox*)widget)->isChecked() != b)
                                {
                                  // Added by Tim. p3.3.6
                                  //printf("PluginGui::updateControls checkbox\n");
                              
                                  ((QCheckBox*)widget)->blockSignals(true);
                                  ((QCheckBox*)widget)->setChecked(b);
                                  ((QCheckBox*)widget)->blockSignals(false);
                                } 
                              }
                              break;
                        case GuiWidgets::QCOMBOBOX:
                              if( plugin->controllerEnabled(param) && plugin->controllerEnabled2(param) )
                              { 
                                int n = (int) plugin->track()->pluginCtrlVal(genACnum(plugin->id(), param));
                                if(((QComboBox*)widget)->currentItem() != n)
                                {
                                  // Added by Tim. p3.3.6
                                  //printf("PluginGui::updateControls combobox\n");
                              
                                  ((QComboBox*)widget)->blockSignals(true);
                                  ((QComboBox*)widget)->setCurrentItem(n);
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
      int param  = gw[idx].param;
      int type   = gw[idx].type;

      AutomationType at = AUTO_OFF;
      AudioTrack* track = 0;
      if(plugin->track())
      {
        track = plugin->track();
        at = track->automationType();
      }  
      
      if(at == AUTO_WRITE || (audio->isPlaying() && at == AUTO_TOUCH))
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
                  val = double(((QComboBox*)w)->currentItem());
                  break;
            }

      for (int i = 0; i < nobj; ++i) {
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
                        ((QComboBox*)widget)->setCurrentItem(int(val));
                        break;
                  }
            }
      
      int id = plugin->id();
      if(id != -1)
        {
          id = genACnum(id, param);
          audio->msgSetPluginCtrlVal(((PluginI*)plugin), id, val);
          if(track)
          {
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
        } 
      plugin->setParam(param, val);
      }

//---------------------------------------------------------
//   guiParamPressed
//---------------------------------------------------------

void PluginGui::guiParamPressed(int idx)
      {
      int param  = gw[idx].param;

      AutomationType at = AUTO_OFF;
      AudioTrack* track = 0;
      if(plugin->track())
      {
        track = plugin->track();
        at = track->automationType();
      }  
      
      if(at != AUTO_OFF)
        plugin->enableController(param, false);
      
      int id = plugin->id();
      if(!track || id == -1)
        return;
      
      id = genACnum(id, param);
      
      // NOTE: For this to be of any use, the freeverb gui 2142.ui
      //  would have to be used, and changed to use CheckBox and ComboBox
      //  instead of QCheckBox and QComboBox, since both of those would
      //  need customization (Ex. QCheckBox doesn't check on click).
      /*
      switch(type) {
            case GuiWidgets::QCHECKBOX:
                    double val = (double)((CheckBox*)w)->isChecked();
                    track->startAutoRecord(id, val);
                  break;
            case GuiWidgets::QCOMBOBOX:
                    double val = (double)((ComboBox*)w)->currentItem();
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
      int param  = gw[idx].param;
      int type   = gw[idx].type;
      
      AutomationType at = AUTO_OFF;
      AudioTrack* track = 0;
      if(plugin->track())
      {
        track = plugin->track();
        at = track->automationType();
      }  
      
      // Special for switch - don't enable controller until transport stopped.
      if(at != AUTO_WRITE && (type != GuiWidgets::QCHECKBOX
          || !audio->isPlaying()
          || at != AUTO_TOUCH))
        plugin->enableController(param, true);
      
      int id = plugin->id();
      
      if(!track || id == -1)
        return;
      
      id = genACnum(id, param);
      
      // NOTE: For this to be of any use, the freeverb gui 2142.ui
      //  would have to be used, and changed to use CheckBox and ComboBox
      //  instead of QCheckBox and QComboBox, since both of those would
      //  need customization (Ex. QCheckBox doesn't check on click).
      /*
      switch(type) {
            case GuiWidgets::QCHECKBOX:
                    double val = (double)((CheckBox*)w)->isChecked();
                    track->stopAutoRecord(id, param);
                  break;
            case GuiWidgets::QCOMBOBOX:
                    double val = (double)((ComboBox*)w)->currentItem();
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
      int param  = gw[idx].param;
      QWidget *w = gw[idx].widget;
      
      AutomationType at = AUTO_OFF;
      AudioTrack* track = 0;
      if(plugin->track())
      {
        track = plugin->track();
        at = track->automationType();
      }  
      
      int id = plugin->id();
      
      if(at == AUTO_WRITE || (at == AUTO_READ || at == AUTO_TOUCH))
        plugin->enableController(param, false);
      
      if(!track || id == -1)
        return;
      
      id = genACnum(id, param);
      
      double val = ((Slider*)w)->value();
      plugin->setParam(param, val);
      audio->msgSetPluginCtrlVal(((PluginI*)plugin), id, val);
      track->startAutoRecord(id, val);
      
      // Needed so that paging a slider updates a label or other buddy control.
      for (int i = 0; i < nobj; ++i) {
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
                        ((QComboBox*)widget)->setCurrentItem(int(val));
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
      AudioTrack* track = 0;
      if(plugin->track())
      {
        track = plugin->track();
        at = track->automationType();
      }  
      
      if(at != AUTO_WRITE || (!audio->isPlaying() && at == AUTO_TOUCH))
        plugin->enableController(param, true);
      
      int id = plugin->id();
      
      if(!track || id == -1)
        return;
      
      id = genACnum(id, param);
      
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
    song->execAutomationCtlPopup((AudioTrack*)plugin->track(), p, genACnum(id, param));
}

//---------------------------------------------------------
//   PluginWidgetFactory
//---------------------------------------------------------

QWidget* PluginWidgetFactory::createWidget(const QString& className, QWidget* parent, const char* name) const
{
  if(className == "DoubleLabel")
    return new DoubleLabel(parent, name); 
  if(className == "Slider")
    return new Slider(parent, name); 
  return 0;  
};

