//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id: dssihost.cpp,v 1.15.2.16 2009/12/15 03:39:58 terminator356 Exp $
//
//  Copyright (C) 1999-2011 by Werner Schweer and others
//  (C) Copyright 2011 Tim E. Real (terminator356 on sourceforge)
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License
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
//=============================================================================

#include "config.h"
#ifdef DSSI_SUPPORT

// Turn on debugging messages
//#define DSSI_DEBUG 
// Turn on constant flow of process debugging messages
//#define DSSI_DEBUG_PROCESS 

// Support vst state saving/loading with vst chunks. 
//#define DSSI_VST_CHUNK_SUPPORT    

#include <string.h>
#include <signal.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <QDir>
#include <QFileInfo>

#include "dssihost.h"
#include "synth.h"
#include "audio.h"
#include "jackaudio.h"
#include "midi.h"
#include "midiport.h"
#include "stringparam.h"
#include "plugin.h"
#include "controlfifo.h"
#include "xml.h"
#include "song.h"
#include "ctrl.h"

#include "app.h"
#include "globals.h"
#include "globaldefs.h"
#include "gconfig.h"
#include "popupmenu.h"

namespace MusECore {

//---------------------------------------------------------
//   scanDSSILib
//---------------------------------------------------------

static void scanDSSILib(QFileInfo& fi) // ddskrjo removed const for argument
      {
      void* handle = dlopen(fi.filePath().toLatin1().constData(), RTLD_NOW);
      
      if (handle == 0) {
            fprintf(stderr, "scanDSSILib: dlopen(%s) failed: %s\n",
              fi.filePath().toLatin1().constData(), dlerror());
              
            return;
            }
      DSSI_Descriptor_Function dssi = (DSSI_Descriptor_Function)dlsym(handle, "dssi_descriptor");

      if (!dssi) 
      {
        dlclose(handle);
        return;
      }
      else
      {
        for (int i = 0;; ++i) 
        {
          const DSSI_Descriptor* descr;
          
          descr = dssi(i);
          if (descr == 0)
                break;
          
          #ifdef DSSI_DEBUG 
          fprintf(stderr, "scanDSSILib: name:%s inPlaceBroken:%d\n", descr->LADSPA_Plugin->Name, LADSPA_IS_INPLACE_BROKEN(descr->LADSPA_Plugin->Properties));
          #endif
          
          // Listing synths only while excluding effect plugins:
          // Do the exact opposite of what dssi-vst.cpp does for listing ladspa plugins.
          // That way we cover all bases - effect plugins and synths. 
          // Non-synths will show up in the ladspa effect dialog, while synths will show up here...
          // There should be nothing left out...
          // TIP: Until we add programs to plugins, comment these four checks to load dssi effects as synths, in order to have programs. 
          if(descr->run_synth ||                  
            descr->run_synth_adding ||
            descr->run_multiple_synths ||
            descr->run_multiple_synths_adding) 
          {
            const QString label(descr->LADSPA_Plugin->Label);
            
            // Make sure it doesn't already exist.
            std::vector<Synth*>::iterator is;
            for(is = MusEGlobal::synthis.begin(); is != MusEGlobal::synthis.end(); ++is)
            {
              Synth* s = *is;
              if(s->name() == label && s->baseName() == fi.completeBaseName())
                break;
            }
            if(is != MusEGlobal::synthis.end())
              continue;

            DssiSynth* s = new DssiSynth(fi, descr);
            
            if(MusEGlobal::debugMsg)
            {
              fprintf(stderr, "scanDSSILib: name:%s listname:%s lib:%s listlib:%s\n", 
                      label.toLatin1().constData(), s->name().toLatin1().constData(), fi.completeBaseName().toLatin1().constData(), s->baseName().toLatin1().constData());
              int ai = 0, ao = 0, ci = 0, co = 0;
              for(unsigned long pt = 0; pt < descr->LADSPA_Plugin->PortCount; ++pt)
              {
                LADSPA_PortDescriptor pd = descr->LADSPA_Plugin->PortDescriptors[pt];
                if(LADSPA_IS_PORT_INPUT(pd) && LADSPA_IS_PORT_AUDIO(pd))
                  ai++;
                else  
                if(LADSPA_IS_PORT_OUTPUT(pd) && LADSPA_IS_PORT_AUDIO(pd))
                  ao++;
                else  
                if(LADSPA_IS_PORT_INPUT(pd) && LADSPA_IS_PORT_CONTROL(pd))
                  ci++;
                else  
                if(LADSPA_IS_PORT_OUTPUT(pd) && LADSPA_IS_PORT_CONTROL(pd))
                  co++;
              }  
              fprintf(stderr, "  audio ins:%d outs:%d control ins:%d outs:%d\n", ai, ao, ci, co);
            }
            
            MusEGlobal::synthis.push_back(s);
          }
        }
      }  
      dlclose(handle);
      }

//---------------------------------------------------------
//   scanVstDir
//---------------------------------------------------------

static void scanDSSIDir(QString& s) // ddskrjo removed const for argument
{
      if(MusEGlobal::debugMsg)
        printf("scanDSSIDir: scan DSSI plugin dir <%s>\n", s.toLatin1().constData());

#ifdef __APPLE__
      QDir pluginDir(s, QString("*.dylib"), QDir::Unsorted, QDir::Files);
#else
      QDir pluginDir(s, QString("*.so"), QDir::Unsorted, QDir::Files);
#endif
      if(!pluginDir.exists())
        return;

      QStringList list = pluginDir.entryList();
      for(int i = 0; i < list.count(); ++i) 
      {
        if(MusEGlobal::debugMsg)
          printf("scanDSSIDir: found %s\n", (s + QString("/") + list[i]).toLatin1().constData());

        QFileInfo fi(s + QString("/") + list[i]);
        scanDSSILib(fi);
      }
}

//---------------------------------------------------------
//   initDSSI
//---------------------------------------------------------

void initDSSI()
      {
      const char* dssiPath = getenv("DSSI_PATH");
      if (dssiPath == 0)
            dssiPath = "/usr/local/lib64/dssi:/usr/lib64/dssi:/usr/local/lib/dssi:/usr/lib/dssi";

      const char* p = dssiPath;
      while (*p != '\0') {
            const char* pe = p;
            while (*pe != ':' && *pe != '\0')
                  pe++;

            int n = pe - p;
            if (n) {
                  char* buffer = new char[n + 1];
                  strncpy(buffer, p, n);
                  buffer[n] = '\0';
                  QString tmpStr(buffer);
                  scanDSSIDir(tmpStr);
                  delete[] buffer;
                  }
            p = pe;
            if (*p == ':')
                  p++;
            }
      }

//---------------------------------------------------------
//   DssiSynth
//   Synth.label   =  plug.Label 
//   Synth.descr   =  plug.Name
//   Synth.maker   =  plug.maker 
//   Synth.version =  nil (no such field in ladspa, maybe try copyright instead)
//---------------------------------------------------------

DssiSynth::DssiSynth(QFileInfo& fi, const DSSI_Descriptor* d) : // ddskrjo removed const from QFileInfo
  Synth(fi, QString(d->LADSPA_Plugin->Label), QString(d->LADSPA_Plugin->Name), QString(d->LADSPA_Plugin->Maker), QString()) 
{
  df = 0;
  handle = 0;
  dssi = 0;
  _hasGui = false;
  
  const LADSPA_Descriptor* descr = d->LADSPA_Plugin;
  
  _portCount = descr->PortCount;
  
  _inports = 0;
  _outports = 0;
  _controlInPorts = 0;
  _controlOutPorts = 0;
  for(unsigned long k = 0; k < _portCount; ++k) 
  {
    LADSPA_PortDescriptor pd = descr->PortDescriptors[k];
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
  
  _inPlaceCapable = !LADSPA_IS_INPLACE_BROKEN(descr->Properties);
  
  // Hack: Special flag required for example for control processing.
  _isDssiVst = fi.completeBaseName() == QString("dssi-vst");
  // Hack: Blacklist vst plugins in-place, configurable for now. 
  if ((_inports != _outports) || (_isDssiVst && !MusEGlobal::config.vstInPlace))
        _inPlaceCapable = false;
}

DssiSynth::~DssiSynth() 
{ 
  if(dssi)
    delete dssi;
}

//---------------------------------------------------------
//   createSIF
//---------------------------------------------------------

SynthIF* DssiSynth::createSIF(SynthI* synti)
{
      if (_instances == 0) 
      {
        handle = dlopen(info.filePath().toLatin1().constData(), RTLD_NOW);
        if (handle == 0) 
        {
              fprintf(stderr, "DssiSynth::createSIF dlopen(%s) failed: %s\n",
                info.filePath().toLatin1().constData(), dlerror());
              return 0;
        }
        df = (DSSI_Descriptor_Function)dlsym(handle, "dssi_descriptor");

        if (!df) {
              const char *txt = dlerror();
              fprintf(stderr,
                  "Unable to find dssi_descriptor() function in plugin "
                  "library file \"%s\": %s.\n"
                  "Are you sure this is a DSSI plugin file?\n",
                  info.filePath().toLatin1().constData(),
                  txt ? txt : "?");
              dlclose(handle);
              handle = 0;
              return 0;
              }
        for (int i = 0;; ++i) 
        {
          dssi = df(i);
          if (dssi == 0)
            break;
          QString label(dssi->LADSPA_Plugin->Label);
          if (label == _name)
            break;
        }

        if(dssi != 0)
        {
          _inports    = 0;
          _outports   = 0;
          _controlInPorts = 0;
          _controlOutPorts = 0;
          iIdx.clear(); 
          oIdx.clear(); 
          rpIdx.clear();
          iUsedIdx.clear();
          midiCtl2PortMap.clear();
          port2MidiCtlMap.clear();
          
          const LADSPA_Descriptor* descr = dssi->LADSPA_Plugin;
          
          _portCount = descr->PortCount;
          
          for (unsigned long k = 0; k < _portCount; ++k) 
          {
            LADSPA_PortDescriptor pd = descr->PortDescriptors[k];
            
            #ifdef DSSI_DEBUG 
            printf("DssiSynth::createSIF ladspa plugin Port:%lu Name:%s descriptor:%x\n", k, descr->PortNames[k], pd);
            #endif
            
            if (LADSPA_IS_PORT_AUDIO(pd)) 
            {
              if (LADSPA_IS_PORT_INPUT(pd)) 
              {
                ++_inports;
                iIdx.push_back(k);
                iUsedIdx.push_back(false); // Start out with all false.
              }
              else if (LADSPA_IS_PORT_OUTPUT(pd)) 
              {
                ++_outports;
                oIdx.push_back(k);
              }
              
              rpIdx.push_back((unsigned long)-1);
            }
            else if (LADSPA_IS_PORT_CONTROL(pd)) 
            {
              if (LADSPA_IS_PORT_INPUT(pd)) 
              {
                rpIdx.push_back(_controlInPorts);
                ++_controlInPorts;
              }
              else if (LADSPA_IS_PORT_OUTPUT(pd))
              {
                rpIdx.push_back((unsigned long)-1);
                ++_controlOutPorts;
              }
            }
          }
          
          _inPlaceCapable = !LADSPA_IS_INPLACE_BROKEN(descr->Properties);
          // Hack: Special flag required for example for control processing.
          _isDssiVst = info.completeBaseName() == QString("dssi-vst");
          // Hack: Blacklist vst plugins in-place, configurable for now. 
          if((_inports != _outports) || (_isDssiVst && !MusEGlobal::config.vstInPlace))
            _inPlaceCapable = false;
        }  
      }  
      
      if (dssi == 0) 
      {
        fprintf(stderr, "cannot find DSSI synti %s\n", _name.toLatin1().constData());
        dlclose(handle);
        handle = 0;
        df     = 0;
        return 0;
      }
      
      DssiSynthIF* sif = new DssiSynthIF(synti);
      ++_instances;
      sif->init(this);

      QString guiPath(info.path() + "/" + info.baseName());
      QDir guiDir(guiPath, "*", QDir::Unsorted, QDir::Files);
      _hasGui = guiDir.exists();
      
      return sif;
}

//---------------------------------------------------------
//   guiVisible
//---------------------------------------------------------

bool DssiSynthIF::nativeGuiVisible() const
      {
      #ifdef OSC_SUPPORT
      return _oscif.oscGuiVisible();
      #endif
      return false;
      }

bool DssiSynthIF::guiVisible() const
      {
      return _gui && _gui->isVisible();
      }

//---------------------------------------------------------
//   showNativeGui
//---------------------------------------------------------

void DssiSynthIF::showNativeGui(bool v)
      {
      #ifdef OSC_SUPPORT
      
      #ifdef DSSI_DEBUG 
      printf("DssiSynthIF::showNativeGui(): v:%d visible:%d\n", v, guiVisible());
      #endif
      
      _oscif.oscShowGui(v);
      
      #endif // OSC_SUPPORT
      }

//---------------------------------------------------------
//   showGui
//---------------------------------------------------------

void DssiSynthIF::showGui(bool v)
{
    if (v) {
            if (_gui == 0)
                makeGui();
            _gui->show();
            }
    else {
            if (_gui)
                _gui->hide();
            }
}

//---------------------------------------------------------
//   receiveEvent
//---------------------------------------------------------

MusECore::MidiPlayEvent DssiSynthIF::receiveEvent()
      {
      return MusECore::MidiPlayEvent();
      }

//---------------------------------------------------------
//   init
//---------------------------------------------------------

bool DssiSynthIF::init(DssiSynth* s)
      {
      #ifdef DSSI_DEBUG 
      printf("DssiSynthIF::init\n");
      #endif
      
      synth = s;
      const DSSI_Descriptor* dssi = synth->dssi;
      const LADSPA_Descriptor* ld = dssi->LADSPA_Plugin;
      handle = ld->instantiate(ld, MusEGlobal::sampleRate);

      #ifdef OSC_SUPPORT
      _oscif.oscSetSynthIF(this);
      #endif
      
      queryPrograms();

      int inports = synth->_inports;
      if(inports != 0)
      {
        posix_memalign((void**)&audioInSilenceBuf, 16, sizeof(float) * MusEGlobal::segmentSize);
        memset(audioInSilenceBuf, 0, sizeof(float) * MusEGlobal::segmentSize);
        
        audioInBuffers = new float*[inports];
        for(int k = 0; k < inports; ++k)
        {
          posix_memalign((void**)&audioInBuffers[k], 16, sizeof(float) * MusEGlobal::segmentSize);
          memset(audioInBuffers[k], 0, sizeof(float) * MusEGlobal::segmentSize);
          ld->connect_port(handle, synth->iIdx[k], audioInBuffers[k]);
        }  
      }
      
      int outports = synth->_outports;
      if(outports != 0)
      {
        audioOutBuffers = new float*[outports];
        for(int k = 0; k < outports; ++k)
        {
          posix_memalign((void**)&audioOutBuffers[k], 16, sizeof(float) * MusEGlobal::segmentSize);
          memset(audioOutBuffers[k], 0, sizeof(float) * MusEGlobal::segmentSize);
          ld->connect_port(handle, synth->oIdx[k], audioOutBuffers[k]);
        }  
      }
      
      int controlPorts = synth->_controlInPorts;
      int controlOutPorts = synth->_controlOutPorts;
      
      if(controlPorts != 0)
        controls = new Port[controlPorts];
      else
        controls = 0;
          
      if(controlOutPorts != 0)
        controlsOut = new Port[controlOutPorts];
      else
        controlsOut = 0;

      synth->midiCtl2PortMap.clear();
      synth->port2MidiCtlMap.clear();
                
      int cip = 0;
      int cop = 0;
      for (unsigned long k = 0; k < synth->_portCount; ++k) 
      {
        LADSPA_PortDescriptor pd = ld->PortDescriptors[k];
        
        #ifdef DSSI_DEBUG 
        printf("DssiSynth::init ladspa plugin Port:%lu Name:%s descriptor:%x\n", k, ld->PortNames[k], pd);
        #endif
        
        if (LADSPA_IS_PORT_CONTROL(pd)) 
        {
          if (LADSPA_IS_PORT_INPUT(pd)) 
          {
            controls[cip].idx = k;    
            float val;
            ladspaDefaultValue(ld, k, &val);
            controls[cip].val    = val;
            controls[cip].tmpVal = val;
            controls[cip].enCtrl  = true;
            controls[cip].en2Ctrl = true;
            
            #ifdef DSSI_DEBUG 
            printf("DssiSynthIF::init control port:%d port idx:%lu name:%s\n", cip, k, ld->PortNames[k]);
            #endif
            
            // This code is duplicated in ::getControllerInfo()
            
            
            int ctlnum = DSSI_NONE;
            if(dssi->get_midi_controller_for_port)
              ctlnum = dssi->get_midi_controller_for_port(handle, k);
            
            // No controller number? Try to give it a unique one...
            if(ctlnum == DSSI_NONE)
            {
              // FIXME: Be more careful. Must make sure to pick numbers not already chosen or which WILL BE chosen.
              // Simple but flawed solution: Start them at 0x60000 + 0x2000 = 0x62000. Max NRPN number is 0x3fff.
              // TODO: Update: Actually we want to try to use CC Controller7 controllers if possible (or a choice) because what if
              //  the user's controller hardware doesn't support RPN?
              // If CC Controller7 is chosen we must make sure to use only non-common numbers. An already limited range
              //  of 127 now becomes narrower. See the cool document midi-controllers.txt in the DSSI source for a 
              //  nice roundup of numbers and how to choose them and how they relate to synths and DSSI synths etc. !
              ctlnum = MusECore::CTRL_NRPN14_OFFSET + 0x2000 + cip; 
            }
            else
            {
              int c = ctlnum;
              // Can be both CC and NRPN! Prefer CC over NRPN.
              if(DSSI_IS_CC(ctlnum))
              {
                #ifdef DSSI_DEBUG 
                printf("DssiSynthIF::init is CC control\n");
                #endif
                
                ctlnum = DSSI_CC_NUMBER(c);
                #ifdef DSSI_DEBUG 
                if(DSSI_IS_NRPN(ctlnum))
                  printf("DssiSynthIF::init is also NRPN control. Using CC.\n");
                #endif  
              }
              else
              if(DSSI_IS_NRPN(ctlnum))
              {
                #ifdef DSSI_DEBUG 
                printf("DssiSynthIF::init  is NRPN control\n");
                #endif
                
                ctlnum = DSSI_NRPN_NUMBER(c) + MusECore::CTRL_NRPN14_OFFSET;
              }  
                
            }
            
            #ifdef DSSI_DEBUG 
            printf("DssiSynthIF::init inserting to midiCtl2PortMap: ctlnum:%d k:%d\n", ctlnum, cip);
            #endif
            
            // We have a controller number! Insert it and the DSSI port number into both maps.
            synth->midiCtl2PortMap.insert(std::pair<int, int>(ctlnum, cip));
            synth->port2MidiCtlMap.insert(std::pair<int, int>(cip, ctlnum));
            
            // Support a special block for dssi synth ladspa controllers. 
            // Put the ID at a special block after plugins (far after).
            int id = genACnum(MAX_PLUGINS, cip);
            const char* name = ld->PortNames[k];
            float min, max;
            ladspaControlRange(ld, k, &min, &max);
            CtrlList* cl;
            CtrlListList* cll = ((MusECore::AudioTrack*)synti)->controller();
            iCtrlList icl = cll->find(id);
            if (icl == cll->end())
            {
              cl = new CtrlList(id);
              cll->add(cl);
              cl->setCurVal(controls[cip].val);
            }
            else 
            {
              cl = icl->second;
              controls[cip].val = cl->curVal();
            }
            cl->setRange(min, max);
            cl->setName(QString(name));
            cl->setValueType(ladspaCtrlValueType(ld, k));
            cl->setMode(ladspaCtrlMode(ld, k));
            
            ld->connect_port(handle, k, &controls[cip].val);
            
            ++cip;
          }
          else if (LADSPA_IS_PORT_OUTPUT(pd))
          {
            controlsOut[cop].idx = k;
            controlsOut[cop].val    = 0.0;
            controlsOut[cop].tmpVal = 0.0;
            controlsOut[cop].enCtrl  = false;
            controlsOut[cop].en2Ctrl = false;

            #ifdef DSSI_DEBUG 
            printf("DssiSynthIF::init control output port:%d port idx:%lu name:%s\n", cop, k, ld->PortNames[k]);
            #endif
            
            //  Control outs are not handled but still must be connected to something.
            ld->connect_port(handle, k, &controlsOut[cop].val);
            
            ++cop;
          }
        }
      }
          
      if (ld->activate)
            ld->activate(handle);

      // Set current configuration values.
      if(dssi->configure) 
      {
        char *rv = dssi->configure(handle, DSSI_PROJECT_DIRECTORY_KEY,
            MusEGlobal::museProject.toLatin1().constData()); //MusEGlobal::song->projectPath()
        
        if(rv)
        {
          fprintf(stderr, "MusE: Warning: plugin doesn't like project directory: \"%s\"\n", rv);
          free(rv);
        }          
        
        for(ciStringParamMap r = synti->_stringParamMap.begin(); r != synti->_stringParamMap.end(); ++r) 
        {
          rv = 0;
          rv = dssi->configure(handle, r->first.c_str(), r->second.c_str());
          if(rv)
          {
            fprintf(stderr, "MusE: Warning: plugin config key: %s value: %s \"%s\"\n", r->first.c_str(), r->second.c_str(), rv);
            free(rv);
          }  
        }
      }
            
      // Set current program.
      if(dssi->select_program)
        doSelectProgram(handle, synti->_curBankL, synti->_curProgram);
      
      //
      // For stored initial control values, let SynthI::initInstance() take care of that via ::setParameter().
      //
        
      return true;
      }

//---------------------------------------------------------
//   DssiSynthIF
//---------------------------------------------------------

DssiSynthIF::DssiSynthIF(SynthI* s)
   : SynthIF(s)
      {
      #ifdef DSSI_DEBUG 
      printf("DssiSynthIF::DssiSynthIF\n");
      #endif
      synth = 0;
      handle = NULL;
      controls = 0;
      controlsOut = 0;
      audioInBuffers = 0;
      audioInSilenceBuf = 0;
      audioOutBuffers = 0;
      }

//---------------------------------------------------------
//   ~DssiSynthIF
//---------------------------------------------------------

DssiSynthIF::~DssiSynthIF()
{
      #ifdef DSSI_DEBUG 
      printf("DssiSynthIF::~DssiSynthIF\n");
      #endif

      #ifdef OSC_SUPPORT
      _oscif.oscSetSynthIF(NULL);
      #endif
      
      if(synth)
      {
        #ifdef DSSI_DEBUG 
        printf("DssiSynthIF::~DssiSynthIF synth:%p\n", synth);
        #endif
        
        if(synth->dssi)
        {
          #ifdef DSSI_DEBUG 
          printf("DssiSynthIF::~DssiSynthIF synth->dssi:%p\n", synth->dssi);
          #endif
       
          if(synth->dssi->LADSPA_Plugin)
          {
            #ifdef DSSI_DEBUG 
            printf("DssiSynthIF::~DssiSynthIFsynth->dssi->LADSPA_Plugin:%p\n", synth->dssi->LADSPA_Plugin);
            #endif
          }
        }
      }
      
      if(synth && synth->dssi && synth->dssi->LADSPA_Plugin)
      {
        const DSSI_Descriptor* dssi = synth->dssi;
        const LADSPA_Descriptor* descr = dssi->LADSPA_Plugin;

        #ifdef DSSI_DEBUG 
        printf("DssiSynthIF::~DssiSynthIF checking cleanup function exists\n");
        #endif
        
        if(descr->cleanup)
        {
          #ifdef DSSI_DEBUG 
          printf("DssiSynthIF::~DssiSynthIF calling cleanup function\n");
          #endif
            
          descr->cleanup(handle);
        }    
      }
      if(audioInBuffers)
      {
        for(unsigned long i = 0; i < synth->_inports; ++i) 
        {
          if(audioInBuffers[i])
            free(audioInBuffers[i]);
        }
        delete[] audioInBuffers;
      }  
      
      if(audioInSilenceBuf)
        free(audioInSilenceBuf);
      
      if(audioOutBuffers)
      {
        for(unsigned long i = 0; i < synth->_outports; ++i) 
        {
          if(audioOutBuffers[i])
            free(audioOutBuffers[i]);
        }
        delete[] audioOutBuffers;
      }  
      
      if(controls)
        delete[] controls;
        
      if(controlsOut)
        delete[] controlsOut;
}

int DssiSynthIF::oldMidiStateHeader(const unsigned char** data) const 
{
  static unsigned char const d[2] = {MUSE_SYNTH_SYSEX_MFG_ID, DSSI_SYNTH_UNIQUE_ID};
  *data = &d[0];
  return 2; 
}
        
//---------------------------------------------------------
//   getParameter
//---------------------------------------------------------

float DssiSynthIF::getParameter(unsigned long n) const
{
  if(n >= synth->_controlInPorts)
  {
    printf("DssiSynthIF::getParameter param number %lu out of range of ports:%lu\n", n, synth->_controlInPorts);
    return 0.0;
  }
  
  if(!controls)
    return 0.0;
  
  return controls[n].val;
}
//---------------------------------------------------------
//   getParameter
//---------------------------------------------------------

float DssiSynthIF::getParameterOut(unsigned long n) const
{
  if(n >= synth->_controlOutPorts)
  {
    printf("DssiSynthIF::getParameterOut param number %lu out of range of ports:%lu\n", n, synth->_controlOutPorts);
    return 0.0;
  }

  if(!controlsOut)
    return 0.0;

  return controlsOut[n].val;
}

//---------------------------------------------------------
//   setParameter
//---------------------------------------------------------

void DssiSynthIF::setParameter(unsigned long n, float v)
{
  addScheduledControlEvent(n, v, MusEGlobal::audio->curFrame());   
}

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void DssiSynthIF::write(int level, Xml& xml) const
{
#ifdef DSSI_VST_CHUNK_SUPPORT
      if(synth->dssi->getCustomData)
      {
        //---------------------------------------------
        // dump current state of synth
        //---------------------------------------------
        printf("dumping DSSI custom data! %p\n", synth->dssi->getCustomData);
  
        // this is only needed and supported if
        // we are talking to a VST plugin at the other end.
        std::string name = synth->dssi->LADSPA_Plugin->Name;
        if ((name.length()> 4) && name.substr(name.length() - 4) == " VST")
        {
          printf("is vst plugin, commencing data dump, apiversion=%d!\n", synth->dssi->DSSI_API_Version);
          unsigned long len = 0;
          void* p = 0;
          synth->dssi->getCustomData(handle,&p, &len);
          if (len) {
                xml.tag(level++, "midistate version=\"%d\"", SYNTH_MIDI_STATE_SAVE_VERSION);         
                xml.nput(level++, "<event type=\"%d\"", Sysex);
                xml.nput(" datalen=\"%d\">\n", len+9 /* 9 = 2 bytes header + "VSTSAVE"*/);
                xml.nput(level, "");
                xml.nput("%02x %02x ", (char)MUSE_SYNTH_SYSEX_MFG_ID, (char)DSSI_SYNTH_UNIQUE_ID);   // Wrap in a proper header
                xml.nput("56 53 54 53 41 56 45 "); // embed a save marker "string 'VSTSAVE'
                for (long unsigned int i = 0; i < len; ++i) {
                      if (i && (((i+9) % 16) == 0)) {
                            xml.nput("\n");
                            xml.nput(level, "");
                            }
                      xml.nput("%02x ", ((char*)(p))[i] & 0xff);
                      }
                xml.nput("\n");
                xml.tag(level--, "/event");
                xml.etag(level--, "midistate");
                }
        }        
      }
#else
      printf("support for vst chunks not compiled in!\n");
#endif
      
      // DELETETHIS 97 ???
      /*
      // p3.3.39 Store the state of current program and bank and all input control values, but only if VSTSAVE above didn't do it already! 
      // TODO: Not quite good enough, we would want to store all controls for EACH program, not just the current one. 
      // Need to modify controls array to be inside a program array and act as a cache when the user changes a control on a particular program.
      if(!vstsaved)
      {
        if(synth->_controlInPorts)
        {
          // TODO: Hmm, what if these sizes change (platform etc.)? Hard code? Not good - need to store complete value.
          const int fs = sizeof(float);
          const int uls = sizeof(unsigned long);
          
          // Data length: Version major and minor bytes, bank + program, and controllers. 
          const unsigned long len = 2 + 2 * uls + synth->_controlInPorts * fs; 
          
          unsigned long prog = _curBank; 
          unsigned long bnk = _curProgram;
          
          xml.tag(level++, "midistate");
          xml.nput(level++, "<event type=\"%d\"", Sysex);
          xml.nput(" datalen=\"%d\">\n", len+9); //  "PARAMSAVE" length + data length.
          xml.nput(level, "");
          xml.nput("50 41 52 41 4d 53 41 56 45 "); // Embed a save marker string "PARAMSAVE".
          
          unsigned long i = 9;
          
          // Store PARAMSAVE version major...
          char uc = DSSI_PARAMSAVE_VERSION_MAJOR;
          if(i && ((i % 16) == 0)) 
          {
            xml.nput("\n");
            xml.nput(level, "");
          }
          xml.nput("%02x ", uc & 0xff);
          ++i;
          
          // Store PARAMSAVE version minor...
          uc = DSSI_PARAMSAVE_VERSION_MINOR;
          if(i && ((i % 16) == 0)) 
          {
            xml.nput("\n");
            xml.nput(level, "");
          }
          xml.nput("%02x ", uc & 0xff);
          ++i;
          
          // Store bank...
          void* p = &bnk;
          for(int j = 0; j < uls; ++j)
          {
            if(i && ((i % 16) == 0)) 
            {
              xml.nput("\n");
              xml.nput(level, "");
            }
            xml.nput("%02x ", ((char*)(p))[j] & 0xff);
            ++i;
          }  
          
          // Store program...
          p = &prog;
          for(int j = 0; j < uls; ++j)
          {
            if(i && ((i % 16) == 0)) 
            {
              xml.nput("\n");
              xml.nput(level, "");
            }
            xml.nput("%02x ", ((char*)(p))[j] & 0xff);
            ++i;
          }  
          
          // Store controls...
          for(unsigned long c = 0; c < synth->_controlInPorts; ++c)
          {
            float v = controls[c].val;
            p = &v;
            for(int j = 0; j < fs; ++j)
            {
              if(i && ((i % 16) == 0)) 
              {
                xml.nput("\n");
                xml.nput(level, "");
              }
              xml.nput("%02x ", ((char*)(p))[j] & 0xff);
              ++i;
            }  
          }
          xml.nput("\n");
          xml.tag(level--, "/event");
          xml.etag(level--, "midistate");
        }
      }
      */
      
      // Store controls as parameters...
      for(unsigned long c = 0; c < synth->_controlInPorts; ++c)
      {
        float f = controls[c].val;
        xml.floatTag(level, "param", f);
      }  
}

//---------------------------------------------------------
//   preProcessAlways
//---------------------------------------------------------

void DssiSynthIF::preProcessAlways()
{

}

//---------------------------------------------------------
//   processEvent
//   Return true if event pointer filled.
//--------------------------------------------------------

bool DssiSynthIF::processEvent(const MusECore::MidiPlayEvent& e, snd_seq_event_t* event)
{
  const DSSI_Descriptor* dssi = synth->dssi;
  
  int chn = e.channel();
  int a   = e.dataA();
  int b   = e.dataB();
  
  int len = e.len();
  char ca[len + 2];
  
  ca[0] = 0xF0;
  memcpy(ca + 1, (const char*)e.data(), len);
  ca[len + 1] = 0xF7;

  len += 2;

  #ifdef DSSI_DEBUG 
  fprintf(stderr, "DssiSynthIF::processEvent midi event type:%d chn:%d a:%d b:%d\n", e.type(), chn, a, b);
  #endif
  
  switch(e.type()) 
  {
    case MusECore::ME_NOTEON:
      #ifdef DSSI_DEBUG 
      fprintf(stderr, "DssiSynthIF::processEvent midi event is MusECore::ME_NOTEON\n");
      #endif
          
      snd_seq_ev_clear(event); 
      event->queue = SND_SEQ_QUEUE_DIRECT;
      if(b)
        snd_seq_ev_set_noteon(event, chn, a, b);
      else
        snd_seq_ev_set_noteoff(event, chn, a, 0);
    break;
    case MusECore::ME_NOTEOFF:
      #ifdef DSSI_DEBUG 
      fprintf(stderr, "DssiSynthIF::processEvent midi event is MusECore::ME_NOTEOFF\n");
      #endif
          
      snd_seq_ev_clear(event); 
      event->queue = SND_SEQ_QUEUE_DIRECT;
      snd_seq_ev_set_noteoff(event, chn, a, 0);
    break;
    case MusECore::ME_PROGRAM:
    {
      #ifdef DSSI_DEBUG 
      fprintf(stderr, "DssiSynthIF::processEvent midi event is MusECore::ME_PROGRAM\n");
      #endif
      
      int bank = (a >> 8) & 0xff;
      int prog = a & 0xff;
      synti->_curBankH = 0;
      synti->_curBankL = bank;
      synti->_curProgram = prog;
      
      if(dssi->select_program)
        doSelectProgram(handle, bank, prog);

      // Event pointer not filled. Return false.
      return false;
    }    
    break;
    case MusECore::ME_CONTROLLER:
    {
      #ifdef DSSI_DEBUG 
      fprintf(stderr, "DssiSynthIF::processEvent midi event is MusECore::ME_CONTROLLER\n");
      #endif
      
      if((a == 0) || (a == 32))
        return false;
        
      if(a == MusECore::CTRL_PROGRAM) 
      {
        #ifdef DSSI_DEBUG 
        fprintf(stderr, "DssiSynthIF::processEvent midi event is MusECore::ME_CONTROLLER, dataA is MusECore::CTRL_PROGRAM\n");
        #endif
        
        int bank = (b >> 8) & 0xff;
        int prog = b & 0xff;
        
        synti->_curBankH = 0;
        synti->_curBankL = bank;
        synti->_curProgram = prog;
        
        if(dssi->select_program)
          doSelectProgram(handle, bank, prog);

        // Event pointer not filled. Return false.
        return false;
      }
          
      if(a == MusECore::CTRL_PITCH) 
      {
        #ifdef DSSI_DEBUG 
        fprintf(stderr, "DssiSynthIF::processEvent midi event is MusECore::ME_CONTROLLER, dataA is MusECore::CTRL_PITCH\n");
        #endif
        
        snd_seq_ev_clear(event); 
        event->queue = SND_SEQ_QUEUE_DIRECT;
        snd_seq_ev_set_pitchbend(event, chn, b);
        // Event pointer filled. Return true.
        return true;
      }
          
      const LADSPA_Descriptor* ld = dssi->LADSPA_Plugin;
      
      MusECore::ciMidiCtl2LadspaPort ip = synth->midiCtl2PortMap.find(a);
      // Is it just a regular midi controller, not mapped to a LADSPA port (either by the plugin or by us)?
      // NOTE: There's no way to tell which of these controllers is supported by the plugin.
      // For example sustain footpedal or pitch bend may be supported, but not mapped to any LADSPA port.
      if(ip == synth->midiCtl2PortMap.end())
      {
        int ctlnum = a;
        if(MusECore::midiControllerType(a) != MusECore::MidiController::Controller7)
          return false;   // Event pointer not filled. Return false.
        else  
        {
                #ifdef DSSI_DEBUG 
                fprintf(stderr, "DssiSynthIF::processEvent non-ladspa midi event is Controller7. Current dataA:%d\n", a);
                #endif  
                a &= 0x7f;
                ctlnum = DSSI_CC_NUMBER(ctlnum);
        }
        
        // Fill the event.
        #ifdef DSSI_DEBUG 
        printf("DssiSynthIF::processEvent non-ladspa filling midi event chn:%d dataA:%d dataB:%d\n", chn, a, b);
        #endif
        snd_seq_ev_clear(event); 
        event->queue = SND_SEQ_QUEUE_DIRECT;
        snd_seq_ev_set_controller(event, chn, a, b);
        return true;
      }
      
      unsigned long k = ip->second;
      unsigned long i = controls[k].idx;
      int ctlnum = DSSI_NONE;
      if(dssi->get_midi_controller_for_port)
        ctlnum = dssi->get_midi_controller_for_port(handle, i);
        
      // No midi controller for the ladspa port? Send to ladspa control.
      if(ctlnum == DSSI_NONE)
      {
        // Sanity check.
        if(k > synth->_controlInPorts)
          return false;
          
        // Simple but flawed solution: Start them at 0x60000 + 0x2000 = 0x62000. Max NRPN number is 0x3fff.
        ctlnum = k + (MusECore::CTRL_NRPN14_OFFSET + 0x2000);
      }  
      else
      {
        #ifdef DSSI_DEBUG 
        printf("DssiSynthIF::processEvent plugin requests DSSI-style ctlnum:%x(h) %d(d) be mapped to control port:%lu...\n", ctlnum, ctlnum, i);
        #endif
        
        int c = ctlnum;
        // Can be both CC and NRPN! Prefer CC over NRPN.
        if(DSSI_IS_CC(ctlnum))
        {
          ctlnum = DSSI_CC_NUMBER(c);
          
          #ifdef DSSI_DEBUG 
          printf("DssiSynthIF::processEvent is CC ctlnum:%d\n", ctlnum);
          #endif
          
          #ifdef DSSI_DEBUG 
          if(DSSI_IS_NRPN(ctlnum))
            printf("DssiSynthIF::processEvent is also NRPN control. Using CC.\n");
          #endif  
        }
        else
        if(DSSI_IS_NRPN(ctlnum))
        {
          ctlnum = DSSI_NRPN_NUMBER(c) + MusECore::CTRL_NRPN14_OFFSET;
          
          #ifdef DSSI_DEBUG 
          printf("DssiSynthIF::processEvent is NRPN ctlnum:%x(h) %d(d)\n", ctlnum, ctlnum);
          #endif
        }  
      
      }
      
      float val = midi2LadspaValue(ld, i, ctlnum, b); 
      
      #ifdef DSSI_DEBUG 
      fprintf(stderr, "DssiSynthIF::processEvent control port:%lu port:%lu dataA:%d Converting val from:%d to ladspa:%f\n", i, k, a, b, val);
      #endif
      
      // Set the ladspa port value.
      controls[k].val = val;
      
      // Need to update the automation value, otherwise it overwrites later with the last automation value.
      if(id() != -1)
        // We're in the audio thread context: no need to send a message, just modify directly.
        synti->setPluginCtrlVal(genACnum(id(), k), val);
      
      // Since we absorbed the message as a ladspa control change, return false - the event is not filled.
      return false;
    }
    break;
    case MusECore::ME_PITCHBEND:
      snd_seq_ev_clear(event); 
      event->queue = SND_SEQ_QUEUE_DIRECT;
      snd_seq_ev_set_pitchbend(event, chn, a);
    break;
    case MusECore::ME_AFTERTOUCH:
      snd_seq_ev_clear(event); 
      event->queue = SND_SEQ_QUEUE_DIRECT;
      snd_seq_ev_set_chanpress(event, chn, a);
    break;
    case MusECore::ME_SYSEX: 
      {
        #ifdef DSSI_DEBUG 
        fprintf(stderr, "DssiSynthIF::processEvent midi event is MusECore::ME_SYSEX\n");
        #endif
        
        const unsigned char* data = e.data();
        if(e.len() >= 2)
        {
          if(data[0] == MUSE_SYNTH_SYSEX_MFG_ID)
          {
            if(data[1] == DSSI_SYNTH_UNIQUE_ID)
            {
              if(e.len() >= 9)
              {
                if (QString((const char*)(data + 2)).startsWith("VSTSAVE")) {
#ifdef DSSI_VST_CHUNK_SUPPORT
                  if(dssi->setCustomData)
                  {
                    printf("loading chunk from sysex %s!\n", data+9);
                    dssi->setCustomData(handle, (unsigned char*)(data+9) /* len of str*/,e.len()-9);
                  } 
#else
                  printf("support for vst chunks not compiled in!\n");
#endif
                  // Event not filled.
                  return false;
                }  
              }  
            }  
          }  
        }
        
        // DELETETHIS, 50 clean it up or fix it?
        /*
        // p3.3.39 Read the state of current bank and program and all input control values.
        // TODO: Needs to be better. See write().
        //else 
        if (QString((const char*)e.data()).startsWith("PARAMSAVE")) 
        {
          #ifdef DSSI_DEBUG 
          fprintf(stderr, "DssiSynthIF::processEvent midi event is MusECore::ME_SYSEX PARAMSAVE\n");
          #endif
          
          unsigned long dlen = e.len() - 9; // Minus "PARAMSAVE"
          if(dlen > 0)
          {
            //if(dlen < 2 * sizeof(unsigned long))
            if(dlen < (2 + 2 * sizeof(unsigned long))) // Version major and minor bytes, bank and program.
              printf("DssiSynthIF::processEvent Error: PARAMSAVE data length does not include at least version major and minor, bank and program!\n");
            else
            {
              // Not required, yet.
              //char vmaj = *((char*)(e.data() + 9));  // After "PARAMSAVE"
              //char vmin = *((char*)(e.data() + 10));
              
              unsigned long* const ulp = (unsigned long*)(e.data() + 11);  // After "PARAMSAVE" + version major and minor.
              // TODO: TODO: Set plugin bank and program.
              _curBank = ulp[0];
              _curProgram = ulp[1];
              
              dlen -= (2 + 2 * sizeof(unsigned long)); // After the version major and minor, bank and program.
              
              if(dlen > 0)
              {
                if((dlen % sizeof(float)) != 0)
                  printf("DssiSynthIF::processEvent Error: PARAMSAVE float data length not integral multiple of float size!\n");
                else
                {
                  const unsigned long n = dlen / sizeof(float);
                  if(n != synth->_controlInPorts)
                    printf("DssiSynthIF::processEvent Warning: PARAMSAVE number of floats:%lu != number of controls:%lu\n", n, synth->_controlInPorts);
                  
                  // Point to location after "PARAMSAVE", version major and minor, bank and progam.
                  float* const fp = (float*)(e.data() + 9 + 2 + 2 * sizeof(unsigned long)); 
                  
                  for(unsigned long i = 0; i < synth->_controlInPorts && i < n; ++i)
                  {
                    const float v = fp[i];
                    controls[i].val = v;
                  }
                }
              }  
            }  
          }  
          // Event not filled.
          return false;
        }
        */
        //else
        {
          // NOTE: There is a limit on the size of a sysex. Got this: 
          // "DssiSynthIF::processEvent midi event is MusECore::ME_SYSEX"
          // "WARNING: MIDI event of type ? decoded to 367 bytes, discarding"
          // That might be ALSA doing that.
          snd_seq_ev_clear(event); 
          event->queue = SND_SEQ_QUEUE_DIRECT;
          snd_seq_ev_set_sysex(event, len,
            (unsigned char*)ca);
        }
      }  
    break;
    default:
      if(MusEGlobal::debugMsg)
        fprintf(stderr, "DssiSynthIF::processEvent midi event unknown type:%d\n", e.type());
      // Event not filled.
      return false;
    break;
  }
  
  return true;
}

//---------------------------------------------------------
//   getData
//---------------------------------------------------------

MusECore::iMPEvent DssiSynthIF::getData(MusECore::MidiPort* /*mp*/, MusECore::MPEventList* el, MusECore::iMPEvent start_event, unsigned pos, int ports, unsigned nframes, float** buffer)
{
  // We may not be using nevents all at once - this will be just the maximum. 
  unsigned long nevents = el->size() + synti->eventFifo.getSize(); 
  snd_seq_event_t events[nevents];
  
  int frameOffset = MusEGlobal::audio->getFrameOffset();
  unsigned long syncFrame = MusEGlobal::audio->curSyncFrame();  
  
  #ifdef DSSI_DEBUG_PROCESS 
  fprintf(stderr, "DssiSynthIF::getData: pos:%u ports:%d nframes:%u syncFrame:%lu nevents:%lu\n", pos, ports, nframes, syncFrame, nevents);
  #endif
          
  // All ports must be connected to something!
  unsigned long nop, k;
  
  nop = ((unsigned long) ports) > synth->_outports ? synth->_outports : ((unsigned long) ports);
  
  const DSSI_Descriptor* dssi = synth->dssi;
  const LADSPA_Descriptor* descr = dssi->LADSPA_Plugin;
  unsigned long sample = 0;
  
  // To remember the last retrieved value of each AudioTrack controller. 
  //float prev_ctrl_values[synth->_controlInPorts];
  
  // NOTE Tested: Variable run-lengths worked superbly for LADSPA and DSSI synths. But DSSI-VST definitely 
  //  does NOT like changing sample run length. It crashes the plugin and Wine (but MusE keeps running!). 
  // Furthermore, it resizes the shared memory (mmap, remap) upon each run length DIFFERENT from the last. 
  // And all of this done through client-server communications. It doesn't seem designed for this technique.
  //
  // So we could support an alternate technique: A fixed control processing rate, in number of samples. 
  //
  // Allow user to choose either a fixed rate or these 'packets' for LADSPA and DSSI plugins/synths, 
  //  but make fixed-rate MANDATORY for DSSI-VST plugins and synths.
  // 
  // Or K.I.S.S - Just use fixed rates only, but allow it to be changed. I'm worried about libraries and
  //  plugins other than DSSI-VST. What if they need the fixed-rate, too?
  // How to tell, and manage it all...?
  // But this 'packet' method sure seems to work nicely so far, so we'll throw it in...
  //
  // Must make this detectable for dssi vst synths, just like the plugins' in-place blacklist.
  const bool usefixedrate = synth->_isDssiVst;  // Try this. (was: true)
  // TODO Make this number a global setting.
  // Note for dssi-vst this MUST equal MusEGlobal::audio period. It doesn't like broken-up runs (it stutters), 
  //  even with fixed sizes. Could be a Wine + Jack thing, wanting a full Jack buffer's length.
  unsigned long fixedsize = nframes;  // was: 2048
  
  // For now, the fixed size is clamped to the MusEGlobal::audio buffer size.
  // TODO: We could later add slower processing over several cycles -
  //  so that users can select a small MusEGlobal::audio period but a larger control period. 
  if(fixedsize > nframes)
    fixedsize = nframes;
  
  unsigned long min_per = MusEGlobal::config.minControlProcessPeriod;  // Must be power of 2 !
  if(min_per > nframes)
    min_per = nframes;
  
  #ifdef DSSI_DEBUG_PROCESS 
  fprintf(stderr, "DssiSynthIF::getData: Handling inputs...\n");
  #endif
          
  // Handle inputs...
  if(!((MusECore::AudioTrack*)synti)->noInRoute()) 
  {
    RouteList* irl = ((MusECore::AudioTrack*)synti)->inRoutes();
    iRoute i = irl->begin();
    if(!i->track->isMidiTrack())
    {
      int ch     = i->channel       == -1 ? 0 : i->channel;
      int remch  = i->remoteChannel == -1 ? 0 : i->remoteChannel;
      int chs    = i->channels      == -1 ? 0 : i->channels;
      
      if((unsigned)ch < synth->_inports && (unsigned)(ch + chs) <= synth->_inports)
      {  
        int h = remch + chs;
        for(int j = remch; j < h; ++j)
          synth->iUsedIdx[j] = true;
        
        ((MusECore::AudioTrack*)i->track)->copyData(pos, chs, ch, -1, nframes, &audioInBuffers[remch]);
      }
    }  
  
    ++i;
    for(; i != irl->end(); ++i)
    {
      if(i->track->isMidiTrack())
        continue;

      int ch     = i->channel       == -1 ? 0 : i->channel;
      int remch  = i->remoteChannel == -1 ? 0 : i->remoteChannel;
      int chs    = i->channels      == -1 ? 0 : i->channels;
      
      if((unsigned)ch < synth->_inports && (unsigned)(ch + chs) <= synth->_inports)
      {  
        bool u1 = synth->iUsedIdx[remch];
        if(chs >= 2) 
        {
          bool u2 = synth->iUsedIdx[remch + 1];
          if(u1 && u2)
            ((MusECore::AudioTrack*)i->track)->addData(pos, chs, ch, -1, nframes, &audioInBuffers[remch]);
          else
          if(!u1 && !u2)
            ((MusECore::AudioTrack*)i->track)->copyData(pos, chs, ch, -1, nframes, &audioInBuffers[remch]);
          else 
          {  
            if(u1) 
              ((MusECore::AudioTrack*)i->track)->addData(pos, 1, ch, 1, nframes, &audioInBuffers[remch]);
            else
              ((MusECore::AudioTrack*)i->track)->copyData(pos, 1, ch, 1, nframes, &audioInBuffers[remch]);

            if(u2)  
              ((MusECore::AudioTrack*)i->track)->addData(pos, 1, ch + 1, 1, nframes, &audioInBuffers[remch + 1]);
            else
              ((MusECore::AudioTrack*)i->track)->copyData(pos, 1, ch + 1, 1, nframes, &audioInBuffers[remch + 1]);
          }  
        }
        else
        {
            if(u1) 
              ((MusECore::AudioTrack*)i->track)->addData(pos, 1, ch, -1, nframes, &audioInBuffers[remch]);
            else
              ((MusECore::AudioTrack*)i->track)->copyData(pos, 1, ch, -1, nframes, &audioInBuffers[remch]);
        }
          
        int h = remch + chs;
        for(int j = remch; j < h; ++j)
          synth->iUsedIdx[j] = true;
      }
    }
  }  
  
  #ifdef DSSI_DEBUG_PROCESS 
  fprintf(stderr, "DssiSynthIF::getData: Processing automation control values...\n");
  #endif
    
  while(sample < nframes)
  {
    unsigned long nsamp = usefixedrate ? fixedsize : nframes - sample;

    //
    // Process automation control values, while also determining the maximum acceptable 
    //  size of this run. Further processing, from FIFOs for example, can lower the size 
    //  from there, but this section determines where the next highest maximum frame 
    //  absolutely needs to be for smooth playback of the controller value stream...
    //
    if(id() != -1)
    {
      unsigned long frame = pos + sample;
      AutomationType at = AUTO_OFF;
      at = synti->automationType();
      bool no_auto = !MusEGlobal::automation || at == AUTO_OFF;
      AudioTrack* track = (static_cast<AudioTrack*>(synti));
      int nextFrame;
      for(unsigned long k = 0; k < synth->_controlInPorts; ++k)
      {  
        controls[k].val = track->controller()->value(genACnum(id(), k), frame,
                                no_auto || !controls[k].enCtrl || !controls[k].en2Ctrl,
                                &nextFrame);
#ifdef DSSI_DEBUG_PROCESS
        printf("DssiSynthIF::getData k:%lu sample:%lu frame:%lu nextFrame:%d nsamp:%lu \n", k, sample, frame, nextFrame, nsamp);
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
#ifdef DSSI_DEBUG
      printf("DssiSynthIF::getData sample:%lu nsamp:%lu\n", sample, nsamp);
#endif
    }
    
    bool found = false;
    unsigned long frame = 0; 
    unsigned long index = 0;
    unsigned long evframe; 
    // Get all control ring buffer items valid for this time period...
    while(!_controlFifo.isEmpty())
    {
      ControlEvent v = _controlFifo.peek(); 
      // The events happened in the last period or even before that. Shift into this period with + n. This will sync with audio. 
      // If the events happened even before current frame - n, make sure they are counted immediately as zero-frame.
      evframe = (syncFrame > v.frame + nframes) ? 0 : v.frame - syncFrame + nframes; 

      #ifdef DSSI_DEBUG 
      fprintf(stderr, "DssiSynthIF::getData found:%d evframe:%lu frame:%lu  event frame:%lu idx:%lu val:%f unique:%d\n", 
          found, evframe, frame, v.frame, v.idx, v.value, v.unique);
      #endif

      // Protection. Observed this condition. Why? Supposed to be linear timestamps.
      if(found && evframe < frame)
      {
        printf("DssiSynthIF::getData *** Error: evframe:%lu < frame:%lu event: frame:%lu idx:%lu val:%f unique:%d\n", 
          evframe, frame, v.frame, v.idx, v.value, v.unique); 

        // No choice but to ignore it.
        _controlFifo.remove();               // Done with the ring buffer's item. Remove it.
        continue;
      }    
      
      if(evframe >= nframes                                                        // Next events are for a later period.
         || (!usefixedrate && !found && !v.unique && (evframe - sample >= nsamp))  // Next events are for a later run in this period. (Autom took prio.)
         || (found && !v.unique && (evframe - sample >= min_per))                  // Eat up events within minimum slice - they're too close.
         || (usefixedrate && found && v.unique && v.idx == index))                 // Special for dssi-vst: Fixed rate and must reply to all.
        break;
      _controlFifo.remove();               // Done with the ring buffer's item. Remove it.

      if(v.idx >= synth->_controlInPorts) // Sanity check.
        break;
      found = true;
      frame = evframe;
      index = v.idx;
      // Set the ladspa control port value.
      controls[v.idx].val = v.value;
      
      // Need to update the automation value, otherwise it overwrites later with the last automation value.
      if(id() != -1)
        synti->setPluginCtrlVal(genACnum(id(), v.idx), v.value);
    }
    
    if(found && !usefixedrate)  // If a control FIFO item was found, takes priority over automation controller stream.
      nsamp = frame - sample;
    
    if(sample + nsamp >= nframes)         // Safety check.
      nsamp = nframes - sample; 
    
    // TODO: Don't allow zero-length runs. This could/should be checked in the control loop instead.
    // Note this means it is still possible to get stuck in the top loop (at least for a while).
    if(nsamp == 0)
      continue;
      
    nevents = 0;
    // Process event list events...
    for(; start_event != el->end(); ++start_event) 
    {
      #ifdef DSSI_DEBUG 
      fprintf(stderr, "DssiSynthIF::getData eventlist event time:%d pos:%u sample:%lu nsamp:%lu frameOffset:%d\n", start_event->time(), pos, sample, nsamp, frameOffset);
      #endif

      if(start_event->time() >= (pos + sample + nsamp + frameOffset))  // frameOffset? Test again...
      {
        #ifdef DSSI_DEBUG 
        fprintf(stderr, " event is for future:%lu, breaking loop now\n", start_event->time() - frameOffset - pos - sample);
        #endif
        break;
      }
      
      // Update hardware state so knobs and boxes are updated. Optimize to avoid re-setting existing values.
      // Same code as in MidiPort::sendEvent()
      if(synti->midiPort() != -1)
      {
        MusECore::MidiPort* mp = &MusEGlobal::midiPorts[synti->midiPort()];
        if(start_event->type() == MusECore::ME_CONTROLLER) 
        {
          int da = start_event->dataA();
          int db = start_event->dataB();
          db = mp->limitValToInstrCtlRange(da, db);
          if(!mp->setHwCtrlState(start_event->channel(), da, db))
            continue;
        }
        else if(start_event->type() == MusECore::ME_PITCHBEND) 
        {
          int da = mp->limitValToInstrCtlRange(MusECore::CTRL_PITCH, start_event->dataA());
          if(!mp->setHwCtrlState(start_event->channel(), MusECore::CTRL_PITCH, da))
            continue;
        }
        else if(start_event->type() == MusECore::ME_PROGRAM) 
        {
          if(!mp->setHwCtrlState(start_event->channel(), MusECore::CTRL_PROGRAM, start_event->dataA()))
            continue;
        }
      }
          
      // Returns false if the event was not filled. It was handled, but some other way.
      if(processEvent(*start_event, &events[nevents]))
      {
        // Time-stamp the event.   
        int ft = start_event->time() - frameOffset - pos - sample;
        if(ft < 0)
          ft = 0;

        if (ft >= int(nsamp)) 
        {
            printf("DssiSynthIF::getData: eventlist event time:%d out of range. pos:%d offset:%d ft:%d sample:%lu nsamp:%lu\n", start_event->time(), pos, frameOffset, ft, sample, nsamp);
            ft = nsamp - 1;
        }
      
        #ifdef DSSI_DEBUG 
        fprintf(stderr, "DssiSynthIF::getData eventlist: ft:%d current nevents:%lu\n", ft, nevents);
        #endif
      
        // "Each event is timestamped relative to the start of the block, (mis)using the ALSA "tick time" field as a frame count. 
        //  The host is responsible for ensuring that events with differing timestamps are already ordered by time."  -  From dssi.h
        events[nevents].time.tick = ft;
        
        ++nevents;
      }
    }
    
    // Now process putEvent events...
    while(!synti->eventFifo.isEmpty()) 
    {
      MusECore::MidiPlayEvent e = synti->eventFifo.peek();  
      
      #ifdef DSSI_DEBUG 
      fprintf(stderr, "DssiSynthIF::getData eventFifo event time:%d\n", e.time());
      #endif
      
      if(e.time() >= (pos + sample + nsamp + frameOffset))  
        break;
      
      synti->eventFifo.remove();    // Done with ring buffer's event. Remove it.
      // Returns false if the event was not filled. It was handled, but some other way.
      if(processEvent(e, &events[nevents]))
      {
        // Time-stamp the event.   
        int ft = e.time() - frameOffset - pos  - sample;
        if(ft < 0)
          ft = 0;
        if (ft >= int(nsamp)) 
        {
            printf("DssiSynthIF::getData: eventFifo event time:%d out of range. pos:%d offset:%d ft:%d sample:%lu nsamp:%lu\n", e.time(), pos, frameOffset, ft, sample, nsamp);
            ft = nsamp - 1;
        }
        // "Each event is timestamped relative to the start of the block, (mis)using the ALSA "tick time" field as a frame count. 
        //  The host is responsible for ensuring that events with differing timestamps are already ordered by time."  -  From dssi.h
        events[nevents].time.tick = ft;
        
        ++nevents;
      }  
    }
    
    #ifdef DSSI_DEBUG_PROCESS 
    fprintf(stderr, "DssiSynthIF::getData: Connecting and running. sample:%lu nsamp:%lu nevents:%lu\n", sample, nsamp, nevents);
    #endif
          
    k = 0;
    // Connect the given buffers directly to the ports, up to a max of synth ports.
    for(; k < nop; ++k)
      descr->connect_port(handle, synth->oIdx[k], buffer[k] + sample);
    // Connect the remaining ports to some local buffers (not used yet).
    for(; k < synth->_outports; ++k)
      descr->connect_port(handle, synth->oIdx[k], audioOutBuffers[k] + sample);
    // Connect all inputs either to some local buffers, or a silence buffer. 
    for(k = 0; k < synth->_inports; ++k)
    {  
      if(synth->iUsedIdx[k])
      {
        synth->iUsedIdx[k] = false; // Reset
        descr->connect_port(handle, synth->iIdx[k], audioInBuffers[k] + sample);
      }
      else
      {
        descr->connect_port(handle, synth->iIdx[k], audioInSilenceBuf + sample);
      }  
    }
    
    // Run the synth for a period of time. This processes events and gets/fills our local buffers...
    if(synth->dssi->run_synth)
    {
      synth->dssi->run_synth(handle, nsamp, events, nevents);
    }  
    else if (synth->dssi->run_multiple_synths) 
    {
      snd_seq_event_t* ev = events;
      synth->dssi->run_multiple_synths(1, &handle, nsamp, &ev, &nevents);
    }
    // TIP: Until we add programs to plugins, uncomment these four checks to load dssi effects as synths, in order to have programs. 
    //else 
    //if(synth->dssi->LADSPA_Plugin->run)         
    //{
    //  synth->dssi->LADSPA_Plugin->run(handle, nsamp);     
    //}
    
    sample += nsamp;
  }
  
  return start_event;
}

//---------------------------------------------------------
//   putEvent
//---------------------------------------------------------

bool DssiSynthIF::putEvent(const MusECore::MidiPlayEvent& ev)
      {
      #ifdef DSSI_DEBUG 
      fprintf(stderr, "DssiSynthIF::putEvent midi event time:%d chn:%d a:%d b:%d\n", ev.time(), ev.channel(), ev.dataA(), ev.dataB());
      #endif
      if (MusEGlobal::midiOutputTrace)
            ev.dump();
      return synti->eventFifo.put(ev);
      }

//---------------------------------------------------------
//   incInstances
//---------------------------------------------------------

void DssiSynth::incInstances(int val)
{
      _instances += val;
      if (_instances == 0) 
      {
            if (handle)
            {
              #ifdef DSSI_DEBUG 
              fprintf(stderr, "DssiSynth::incInstances no more instances, closing library\n");
              #endif
              
              dlclose(handle);
            }
            handle = 0;
            dssi = NULL;
            df   = NULL;
            iIdx.clear(); 
            oIdx.clear(); 
            rpIdx.clear();
            iUsedIdx.clear();
            midiCtl2PortMap.clear();
            port2MidiCtlMap.clear();
      }
}

//---------------------------------------------------------
//   initGui
//---------------------------------------------------------
bool DssiSynthIF::initGui()
{
      #ifdef OSC_SUPPORT
      return _oscif.oscInitGui();
      #endif
      return true;
}

//---------------------------------------------------------
//   guiHeartBeat
//---------------------------------------------------------

void DssiSynthIF::guiHeartBeat()
{
  #ifdef OSC_SUPPORT
  // Update the gui's program if needed.
  _oscif.oscSendProgram(synti->_curProgram, synti->_curBankL);
  
  // Update the gui's controls if needed.
  unsigned long ports = synth->_controlInPorts;

  for(unsigned long i = 0; i < ports; ++i)
    _oscif.oscSendControl(controls[i].idx, controls[i].val);
  #endif
}

#ifdef OSC_SUPPORT
//---------------------------------------------------------
//   oscUpdate
//---------------------------------------------------------

int DssiSynthIF::oscUpdate()
{
      // Send project directory.
      _oscif.oscSendConfigure(DSSI_PROJECT_DIRECTORY_KEY, MusEGlobal::museProject.toLatin1().constData());  // MusEGlobal::song->projectPath()
      
      // Send current string configuration parameters.
      int i = 0;
      for(ciStringParamMap r = synti->_stringParamMap.begin(); r != synti->_stringParamMap.end(); ++r) 
      {
        _oscif.oscSendConfigure(r->first.c_str(), r->second.c_str());
        // Avoid overloading the GUI if there are lots and lots of params. 
        if((i+1) % 50 == 0)
          usleep(300000);
        ++i;      
      }  
      
      // Send current bank and program.
      _oscif.oscSendProgram(synti->_curProgram, synti->_curBankL, true /*force*/);
      
      // Send current control values.
      unsigned long ports = synth->_controlInPorts;
      for(unsigned long i = 0; i < ports; ++i) 
      {
        _oscif.oscSendControl(controls[i].idx, controls[i].val, true /*force*/);
        // Avoid overloading the GUI if there are lots and lots of ports. 
        if((i+1) % 50 == 0)
          usleep(300000);
      }
      
      return 0;
}

//---------------------------------------------------------
//   oscProgram
//---------------------------------------------------------

int DssiSynthIF::oscProgram(unsigned long program, unsigned long bank)
      {
      int ch      = 0;        // TODO: ??
      int port    = synti->midiPort();        
      
      synti->_curBankH = 0;
      synti->_curBankL = bank;
      synti->_curProgram = program;
      
      bank    &= 0xff;
      program &= 0xff;
      
      if(port != -1)
      {
        MusECore::MidiPlayEvent event(0, port, ch, MusECore::ME_PROGRAM, (bank << 8) + program, 0);
      
        #ifdef DSSI_DEBUG 
        fprintf(stderr, "DssiSynthIF::oscProgram midi event chn:%d a:%d b:%d\n", event.channel(), event.dataA(), event.dataB());
        #endif
        
        MusEGlobal::midiPorts[port].sendEvent(event);
      }
      
      //synti->playMidiEvent(&event); // TODO DELETETHIS 7 hasn't changed since r462
      //
      //MidiDevice* md = dynamic_cast<MidiDevice*>(synti);
      //if(md)
      //  md->putEvent(event);
      //
      //synti->putEvent(event); 
      
      return 0;
      }

//---------------------------------------------------------
//   oscControl
//---------------------------------------------------------

int DssiSynthIF::oscControl(unsigned long port, float value)
{
  #ifdef DSSI_DEBUG 
  printf("DssiSynthIF::oscControl received oscControl port:%lu val:%f\n", port, value);    
  #endif
  
  if(port >= synth->rpIdx.size())
  {
    fprintf(stderr, "DssiSynthIF::oscControl: port number:%lu is out of range of index list size:%zd\n", port, synth->rpIdx.size());
    return 0;
  }
  
  // Convert from DSSI port number to control input port index.
  unsigned long cport = synth->rpIdx[port];
  
  if((int)cport == -1)
  {
    fprintf(stderr, "DssiSynthIF::oscControl: port number:%lu is not a control input\n", port);
    return 0;
  }
  
  // DELETETHIS????: is the below still correct? of so, then keep it of course!
  // p3.3.39 Set the DSSI control input port's value.
  // Observations: With a native DSSI synth like LessTrivialSynth, the native GUI's controls do not change the sound at all
  //  ie. they don't update the DSSI control port values themselves. 
  // Hence in response to the call to this oscControl, sent by the native GUI, it is required to that here.
///  controls[cport].val = value; DELETETHIS
  // DSSI-VST synths however, unlike DSSI synths, DO change their OWN sound in response to their gui controls.
  // AND this function is called. 
  // Despite the descrepency we are STILL required to update the DSSI control port values here 
  //  because dssi-vst is WAITING FOR A RESPONSE. (A CHANGE in the control port value). 
  // It will output something like "...4 events expected..." and count that number down as 4 actual control port value CHANGES
  //  are done here in response. Normally it says "...0 events expected..." when MusE is the one doing the DSSI control changes.
  //
  // NOTE: NOTE: This line in RemoteVSTServer::setParameter(int p, float v) in dssi-vst-server.cpp :
  //
  //  " if (tv.tv_sec > m_lastGuiComms.tv_sec + 10) "
  //
  //  explains an observation that after ten seconds, the server automatically clears the expected number to 0.
  // You can't send any 'new' values until either you a): send all the expected events or b): wait ten seconds.
  // (Because the server simply ignores the 'expected' messages.)
  //
  // Well, at least here are the fifos. Try this ...
  // DELETETHIS 20 pretty old as well
  /*
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
      fprintf(stderr, "DssiSynthIF::oscControl: fifo overflow: in control number:%lu\n", cport);
    }
  }
  */
  // p4.0.21
  ControlEvent ce;
  ce.unique = synth->_isDssiVst;    // Special for messages from vst gui to host - requires processing every message.
  ce.idx = cport;
  ce.value = value;

  ce.frame = MusEGlobal::audio->curFrame();
  // don't use timestamp(), because it's circular, which is making it impossible to deal
  // with 'modulo' events which slip in 'under the wire' before processing the ring buffers.

  
  if(_controlFifo.put(ce))
  {
    fprintf(stderr, "DssiSynthIF::oscControl: fifo overflow: in control number:%lu\n", cport);
  }
  
  // Record automation:
  // Take care of this immediately, because we don't want the silly delay associated with 
  //  processing the fifo one-at-a-time in the apply().
  // NOTE: With some vsts we don't receive control events until the user RELEASES a control. 
  // So the events all arrive at once when the user releases a control.
  // That makes this pretty useless... But what the heck...
  if(id() != -1)
  {
    unsigned long pid = genACnum(id(), cport);
    AutomationType at = synti->automationType();
  
    // TODO: Taken from our native gui control handlers. 
    // This may need modification or may cause problems - 
    //  we don't have the luxury of access to the dssi gui controls !
    if ((at == AUTO_WRITE) || 
        (at == AUTO_TOUCH && MusEGlobal::audio->isPlaying()))
      enableController(cport, false); //TODO maybe re-enable the ctrl soon?
      
    synti->recordAutomation(pid, value);
  } 
   
  return 0;
}

//---------------------------------------------------------
//   oscMidi
//---------------------------------------------------------

int DssiSynthIF::oscMidi(int a, int b, int c)
      {
      if (a == MusECore::ME_NOTEOFF) {
            a = MusECore::ME_NOTEON;
            c = 0;
            }
      int channel = 0;        // TODO: ??
      int port    = synti->midiPort();        
      
      if(port != -1)
      {
        MusECore::MidiPlayEvent event(0, port, channel, a, b, c);
      
        #ifdef DSSI_DEBUG   
        printf("DssiSynthIF::oscMidi midi event chn:%d a:%d b:%d\n", event.channel(), event.dataA(), event.dataB());  
        #endif
        
        MusEGlobal::midiPorts[port].sendEvent(event);
      }
      
      return 0;
      }

//---------------------------------------------------------
//   oscConfigure
//---------------------------------------------------------

int DssiSynthIF::oscConfigure(const char *key, const char *value)
      {
      //"This is pretty much the simplest legal implementation of
      // configure in a DSSI host. 
      // The host has the option to remember the set of (key,value)
      // pairs associated with a particular instance, so that if it
      // wants to restore the "same" instance on another occasion it can
      // just call configure() on it for each of those pairs and so
      // restore state without any input from a GUI.  Any real-world GUI
      // host will probably want to do that.  This host doesn't have any
      // concept of restoring an instance from one run to the next, so
      // we don't bother remembering these at all." 

      #ifdef DSSI_DEBUG 
      printf("DssiSynthIF::oscConfigure synth name:%s key:%s value:%s\n", synti->name().toLatin1().constData(), key, value);
      #endif
      
      // Add or modify the configuration map item.
      synti->_stringParamMap.set(key, value);
      
      if (!strncmp(key, DSSI_RESERVED_CONFIGURE_PREFIX,
         strlen(DSSI_RESERVED_CONFIGURE_PREFIX))) {
            fprintf(stderr, "MusE: OSC: UI for plugin '%s' attempted to use reserved configure key \"%s\", ignoring\n",
               synti->name().toLatin1().constData(), key);
            return 0;
            }

      if (!synth->dssi->configure)
            return 0;

      char* message = synth->dssi->configure(handle, key, value);
      if (message) {
            printf("MusE: on configure '%s' '%s', plugin '%s' returned error '%s'\n",
               key, value, synti->name().toLatin1().constData(), message);
            free(message);
            }

      // DELETETHIS 6 ???
      // "also call back on UIs for plugins other than the one
      //  that requested this:"
      // if (n != instance->number && instances[n].uiTarget) {
      //      lo_send(instances[n].uiTarget,
      //      instances[n].ui_osc_configure_path, "ss", key, value);
      //      }

      // configure invalidates bank and program information, so
      //  we should do this again now: 
      queryPrograms();
      return 0;
      }
#endif // OSC_SUPPORT

//---------------------------------------------------------
//   queryPrograms
//---------------------------------------------------------

void DssiSynthIF::queryPrograms()
      {
      for (std::vector<DSSI_Program_Descriptor>::const_iterator i = programs.begin();
         i != programs.end(); ++i) {
            free((void*)(i->Name));
            }
      programs.clear();

      if (!synth->dssi->get_program)
            return;

      for (int i = 0;; ++i) {
            const DSSI_Program_Descriptor* pd = synth->dssi->get_program(handle, i);
            if (pd == 0)
                  break;
            DSSI_Program_Descriptor d;
            d.Name    = strdup(pd->Name);
            d.Program = pd->Program;
            d.Bank    = pd->Bank;
            programs.push_back(d);
            }
      }

void DssiSynthIF::doSelectProgram(LADSPA_Handle handle, int bank, int prog)
{
  const DSSI_Descriptor* dssi = synth->dssi;
  dssi->select_program(handle, bank, prog);
  
  // Need to update the automation value, otherwise it overwrites later with the last automation value.
  //   "A plugin is permitted to re-write the values of its input control ports when select_program is called.  
  //    The host should re-read the input control port values and update its own records appropriately.  
  //    (This is the only circumstance in which a DSSI plugin is allowed to modify its own input ports.)"   From dssi.h
  if(id() != -1)
  {
    for(unsigned long k = 0; k < synth->_controlInPorts; ++k)
    {  
      // We're in the audio thread context: no need to send a message, just modify directly.
      synti->setPluginCtrlVal(genACnum(id(), k), controls[k].val);
    }
  }
}

//---------------------------------------------------------
//   getPatchName
//---------------------------------------------------------

const char* DssiSynthIF::getPatchName(int /*chan*/, int prog, MType /*type*/, bool /*drum*/)
      {
      unsigned program = prog & 0x7f;
      int lbank   = (prog >> 8) & 0xff;
      int hbank   = (prog >> 16) & 0xff;

      if (lbank == 0xff)
            lbank = 0;
      if (hbank == 0xff)
            hbank = 0;
      unsigned bank = (hbank << 8) + lbank;

      for (std::vector<DSSI_Program_Descriptor>::const_iterator i = programs.begin();
         i != programs.end(); ++i) {
            if (i->Bank == bank && i->Program ==program)
                  return i->Name;
            }
      return "?";
      }

//---------------------------------------------------------
//   populatePatchPopup
//---------------------------------------------------------

void DssiSynthIF::populatePatchPopup(MusEGui::PopupMenu* menu, int /*ch*/, MType /*type*/, bool /*drum*/)
      {
      // The plugin can change the programs, patches etc.
      // So make sure we're up to date by calling queryPrograms.
      queryPrograms();
      
      menu->clear();

      for (std::vector<DSSI_Program_Descriptor>::const_iterator i = programs.begin();
         i != programs.end(); ++i) {
            int bank = i->Bank;
            int prog = i->Program;
            int id   = (bank << 16) + prog;
            
            QAction *act = menu->addAction(QString(i->Name));
            act->setData(id);
            }
      }

int DssiSynthIF::getControllerInfo(int id, const char** name, int* ctrl, int* min, int* max, int* initval)
{
  int controlPorts = synth->_controlInPorts;
  if(id >= controlPorts)
    return 0;

  const DSSI_Descriptor* dssi = synth->dssi;
  const LADSPA_Descriptor* ld = dssi->LADSPA_Plugin;
  
  unsigned long i = controls[id].idx;   
  
  #ifdef DSSI_DEBUG 
  printf("DssiSynthIF::getControllerInfo control port:%d port idx:%lu name:%s\n", id, i, ld->PortNames[i]);
  #endif
  
  int ctlnum = DSSI_NONE;
  if(dssi->get_midi_controller_for_port)
    ctlnum = dssi->get_midi_controller_for_port(handle, i);
  
  
  // No controller number? Give it one.
  if(ctlnum == DSSI_NONE)
  {
    // Simple but flawed solution: Start them at 0x60000 + 0x2000 = 0x62000. Max NRPN number is 0x3fff.
    ctlnum = MusECore::CTRL_NRPN14_OFFSET + 0x2000 + id;
  }
  else
  {
    #ifdef DSSI_DEBUG 
    printf("DssiSynthIF::getControllerInfo ctlnum:%d\n", ctlnum);
    #endif
     
    int c = ctlnum;
    // Can be both CC and NRPN! Prefer CC over NRPN.
    if(DSSI_IS_CC(ctlnum))
    {
      #ifdef DSSI_DEBUG 
      printf("DssiSynthIF::getControllerInfo is CC control\n");
      #endif
      
      ctlnum = DSSI_CC_NUMBER(c);
      
      #ifdef DSSI_DEBUG 
      if(DSSI_IS_NRPN(ctlnum))
        printf("DssiSynthIF::getControllerInfo is also NRPN control. Using CC.\n");
      #endif  
    }
    else
    if(DSSI_IS_NRPN(ctlnum))
    {
      #ifdef DSSI_DEBUG 
      printf("DssiSynthIF::getControllerInfo is NRPN control\n");
      #endif
      
      ctlnum = DSSI_NRPN_NUMBER(c) + MusECore::CTRL_NRPN14_OFFSET;
    }  
  }
  
  int def = MusECore::CTRL_VAL_UNKNOWN;
  if(ladspa2MidiControlValues(ld, i, ctlnum, min, max, &def))
    *initval = def;
  else
    *initval = MusECore::CTRL_VAL_UNKNOWN;
    
  #ifdef DSSI_DEBUG 
  printf("DssiSynthIF::getControllerInfo passed ctlnum:%d min:%d max:%d initval:%d\n", ctlnum, *min, *max, *initval);
  #endif
  
  *ctrl = ctlnum;
  *name =  ld->PortNames[i];
  return ++id;
}

int DssiSynthIF::channels() const 
{ 
    return ((int)synth->_outports) > MAX_CHANNELS ? MAX_CHANNELS : ((int)synth->_outports) ;
}

int DssiSynthIF::totalOutChannels() const 
{ 
  return synth->_outports; 
}

int DssiSynthIF::totalInChannels() const 
{ 
  return synth->_inports; 
}

//--------------------------------
// Methods for PluginIBase:
//--------------------------------

bool DssiSynthIF::on() const                                 { return true; }  // Synth is not part of a rack plugin chain. Always on.
void DssiSynthIF::setOn(bool /*val*/)                        { }   
unsigned long DssiSynthIF::pluginID()                        { return (synth && synth->dssi) ? synth->dssi->LADSPA_Plugin->UniqueID : 0; }   
int DssiSynthIF::id()                                        { return MAX_PLUGINS; } // Set for special block reserved for dssi synth. p4.0.20
QString DssiSynthIF::pluginLabel() const                     { return (synth && synth->dssi) ? QString(synth->dssi->LADSPA_Plugin->Label) : QString(); } 
QString DssiSynthIF::name() const                            { return synti->name(); }
QString DssiSynthIF::lib() const                             { return synth ? synth->completeBaseName() : QString(); }
QString DssiSynthIF::dirPath() const                         { return synth ? synth->absolutePath() : QString(); }
QString DssiSynthIF::fileName() const                        { return synth ? synth->fileName() : QString(); }
QString DssiSynthIF::titlePrefix() const                     { return QString(); }
MusECore::AudioTrack* DssiSynthIF::track()                   { return (MusECore::AudioTrack*)synti; }
void DssiSynthIF::enableController(unsigned long i, bool v)  { controls[i].enCtrl = v; } 
bool DssiSynthIF::controllerEnabled(unsigned long i) const   { return controls[i].enCtrl; }  
void DssiSynthIF::enable2Controller(unsigned long i, bool v) { controls[i].en2Ctrl = v; }     
bool DssiSynthIF::controllerEnabled2(unsigned long i) const  { return controls[i].en2Ctrl; }   
void DssiSynthIF::enableAllControllers(bool v)               
{ 
  if(!synth)
    return;
  for(unsigned long i = 0; i < synth->_controlInPorts; ++i) 
    controls[i].enCtrl = v; 
}
void DssiSynthIF::enable2AllControllers(bool v)
{
  if(!synth)
    return;
  for(unsigned long i = 0; i < synth->_controlInPorts; ++i)
    controls[i].en2Ctrl = v; 
}

void DssiSynthIF::updateControllers()                        { }
void DssiSynthIF::writeConfiguration(int /*level*/, Xml& /*xml*/)        { }
bool DssiSynthIF::readConfiguration(Xml& /*xml*/, bool /*readPreset*/) { return false; }

unsigned long DssiSynthIF::parameters() const                { return synth ? synth->_controlInPorts : 0; }
unsigned long DssiSynthIF::parametersOut() const             { return synth ? synth->_controlOutPorts : 0; }
void DssiSynthIF::setParam(unsigned long i, float val)  { setParameter(i, val); }
float DssiSynthIF::param(unsigned long i) const              { return getParameter(i); }
float DssiSynthIF::paramOut(unsigned long i) const           { return getParameterOut(i); }
const char* DssiSynthIF::paramName(unsigned long i)          { return (synth && synth->dssi) ? synth->dssi->LADSPA_Plugin->PortNames[controls[i].idx] : 0; }
const char* DssiSynthIF::paramOutName(unsigned long i)       { return (synth && synth->dssi) ? synth->dssi->LADSPA_Plugin->PortNames[controlsOut[i].idx] : 0; }
LADSPA_PortRangeHint DssiSynthIF::range(unsigned long i)     { return synth->dssi->LADSPA_Plugin->PortRangeHints[controls[i].idx]; }
LADSPA_PortRangeHint DssiSynthIF::rangeOut(unsigned long i)  { return synth->dssi->LADSPA_Plugin->PortRangeHints[controlsOut[i].idx]; }
CtrlValueType DssiSynthIF::ctrlValueType(unsigned long i) const { return ladspaCtrlValueType(synth->dssi->LADSPA_Plugin, controls[i].idx); }
CtrlList::Mode DssiSynthIF::ctrlMode(unsigned long i) const     { return ladspaCtrlMode(synth->dssi->LADSPA_Plugin, controls[i].idx); };

} // namespace MusECore

#else //DSSI_SUPPORT
namespace MusECore {
void initDSSI() {}
}
#endif

