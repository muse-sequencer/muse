//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id: dssihost.cpp,v 1.15.2.16 2009/12/15 03:39:58 terminator356 Exp $
//
//  Copyright (C) 1999-2011 by Werner Schweer and others
//  (C) Copyright 2011-2016 Tim E. Real (terminator356 on sourceforge)
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

#include <string>
#include <string.h>
#include <signal.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>

#include <QDir>
#include <QFileInfo>

#include "dssihost.h"
#include "synth.h"
#include "audio.h"
#include "jackaudio.h"
#include "midi_consts.h"
#include "midiport.h"
#include "minstrument.h"
#include "stringparam.h"
#include "plugin.h"
#include "controlfifo.h"
#include "xml.h"
#include "song.h"
#include "ctrl.h"

#include "app.h"
#include "globals.h"
#include "gconfig.h"
#include "popupmenu.h"
#include "lock_free_buffer.h"
#include "pluglist.h"

namespace MusECore {

//---------------------------------------------------------
//   initDSSI
//---------------------------------------------------------

void initDSSI()
{
  const MusEPlugin::PluginScanList& scan_list = MusEPlugin::pluginList;
  for(MusEPlugin::ciPluginScanList isl = scan_list.begin(); isl != scan_list.end(); ++isl)
  {
    const MusEPlugin::PluginScanInfoRef inforef = *isl;
    const MusEPlugin::PluginScanInfoStruct& info = inforef->info();
    switch(info._type)
    {
      case MusEPlugin::PluginScanInfoStruct::PluginTypeDSSI:
      case MusEPlugin::PluginScanInfoStruct::PluginTypeDSSIVST:
      {
#ifdef DSSI_SUPPORT
        if(MusEGlobal::loadDSSI)
        {
          // For now we allow effects as a synth track. Until we allow programs (and midi) in the effect rack.
          if(info._class & MusEPlugin::PluginScanInfoStruct::PluginClassEffect ||
            info._class & MusEPlugin::PluginScanInfoStruct::PluginClassInstrument)
          {
            // Make sure it doesn't already exist.
            if(const Synth* sy = MusEGlobal::synthis.find(
               PLUGIN_GET_QSTRING(info._completeBaseName),
               PLUGIN_GET_QSTRING(info._uri),
               PLUGIN_GET_QSTRING(info._label)))
            {
              fprintf(stderr, "Ignoring DSSI synth label:%s uri:%s path:%s duplicate of path:%s\n",
                      PLUGIN_GET_CSTRING(info._label),
                      PLUGIN_GET_CSTRING(info._uri),
                      PLUGIN_GET_CSTRING(info.filePath()),
                      sy->filePath().toLatin1().constData());
            }
            else
            {
              DssiSynth* s = new DssiSynth(info);
              MusEGlobal::synthis.push_back(s);
            }
          }
        }
#endif
      }
      break;
      
      case MusEPlugin::PluginScanInfoStruct::PluginTypeLADSPA:
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
//   DssiSynth
//   Synth.name    =  plug.Label (In LADSPA and DSSI this is the more important unique string)
//   Synth.descr   =  plug.Name  (In LADSPA and DSSI this is the less important name string)
//   Synth.maker   =  plug.maker 
//   Synth.version =  nil (no such field in ladspa, maybe try copyright instead)
//---------------------------------------------------------

DssiSynth::DssiSynth(const MusEPlugin::PluginScanInfoStruct& info) 
 : Synth(info), handle(nullptr), dssi(nullptr), df(nullptr)

{
  _isDssiVst = info._type == MusEPlugin::PluginScanInfoStruct::PluginTypeDSSIVST;
  
  _hasGui = info._pluginFlags & MusEPlugin::PluginScanInfoStruct::HasGui;

  _portCount = info._portCount;
  
  _inports = info._inports;
  _outports = info._outports;
  _controlInPorts = info._controlInPorts;
  _controlOutPorts = info._controlOutPorts;

  // Hack: Blacklist vst plugins in-place, configurable for now. 
  if(_isDssiVst && !MusEGlobal::config.vstInPlace)
    _requiredFeatures |= PluginNoInPlaceProcessing;
}

DssiSynth::~DssiSynth() 
{ 
  if(dssi)
  //  delete dssi;
    printf("DssiSynth::~DssiSynth Error: dssi descriptor is not NULL\n");
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
          
          // Hack: Blacklist vst plugins in-place, configurable for now. 
          if((_inports != _outports) || (_isDssiVst && !MusEGlobal::config.vstInPlace))
            _requiredFeatures |= PluginNoInPlaceProcessing;
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

//---------------------------------------------------------
//   showNativeGui
//---------------------------------------------------------

void DssiSynthIF::showNativeGui(bool
#if defined(OSC_SUPPORT)
v
#endif
)
      {
      #ifdef OSC_SUPPORT
      
      #ifdef DSSI_DEBUG 
      printf("DssiSynthIF::showNativeGui(): v:%d visible:%d\n", v, guiVisible());
      #endif
      
      _oscif.oscShowGui(v);
      
      #endif // OSC_SUPPORT
      }

//---------------------------------------------------------
//   receiveEvent
//---------------------------------------------------------

MidiPlayEvent DssiSynthIF::receiveEvent()
      {
      return MidiPlayEvent();
      }

//---------------------------------------------------------
//   init
//---------------------------------------------------------

bool DssiSynthIF::init(DssiSynth* s)
      {
      #ifdef DSSI_DEBUG 
      printf("DssiSynthIF::init\n");
      #endif
      
      _synth = s;
      const DSSI_Descriptor* dssi = _synth->dssi;
      const LADSPA_Descriptor* ld = dssi->LADSPA_Plugin;
      _handle = ld->instantiate(ld, MusEGlobal::sampleRate);

      #ifdef OSC_SUPPORT
      _oscif.oscSetSynthIF(this);
      #endif
      
      queryPrograms();

      int inports = _synth->_inports;
      if(inports != 0)
      {
        int rv = posix_memalign((void**)&_audioInSilenceBuf, 16, sizeof(float) * MusEGlobal::segmentSize);
        if(rv != 0)
        {
          fprintf(stderr, "ERROR: DssiSynthIF::init: posix_memalign returned error:%d. Aborting!\n", rv);
          abort();
        }
        if(MusEGlobal::config.useDenormalBias)
        {
          for(unsigned q = 0; q < MusEGlobal::segmentSize; ++q)
            _audioInSilenceBuf[q] = MusEGlobal::denormalBias;
        }
        else
          memset(_audioInSilenceBuf, 0, sizeof(float) * MusEGlobal::segmentSize);
        
        _audioInBuffers = new float*[inports];
        for(int k = 0; k < inports; ++k)
        {
          int rv = posix_memalign((void**)&_audioInBuffers[k], 16, sizeof(float) * MusEGlobal::segmentSize);
          if(rv != 0)
          {
            fprintf(stderr, "ERROR: DssiSynthIF::init: posix_memalign returned error:%d. Aborting!\n", rv);
            abort();
          }
          if(MusEGlobal::config.useDenormalBias)
          {
            for(unsigned q = 0; q < MusEGlobal::segmentSize; ++q)
              _audioInBuffers[k][q] = MusEGlobal::denormalBias;
          }
          else
            memset(_audioInBuffers[k], 0, sizeof(float) * MusEGlobal::segmentSize);
          ld->connect_port(_handle, _synth->iIdx[k], _audioInBuffers[k]);
        }  
      }
      
      int outports = _synth->_outports;
      if(outports != 0)
      {
        _audioOutBuffers = new float*[outports];
        for(int k = 0; k < outports; ++k)
        {
          int rv = posix_memalign((void**)&_audioOutBuffers[k], 16, sizeof(float) * MusEGlobal::segmentSize);
          if(rv != 0)
          {
            fprintf(stderr, "ERROR: DssiSynthIF::init: posix_memalign returned error:%d. Aborting!\n", rv);
            abort();
          }
          if(MusEGlobal::config.useDenormalBias)
          {
            for(unsigned q = 0; q < MusEGlobal::segmentSize; ++q)
              _audioOutBuffers[k][q] = MusEGlobal::denormalBias;
          }
          else
            memset(_audioOutBuffers[k], 0, sizeof(float) * MusEGlobal::segmentSize);
          ld->connect_port(_handle, _synth->oIdx[k], _audioOutBuffers[k]);
        }  
      }
      
      int controlPorts = _synth->_controlInPorts;
      int controlOutPorts = _synth->_controlOutPorts;
      
      if(controlPorts != 0)
        _controls = new Port[controlPorts];
      else
        _controls = 0;
          
      if(controlOutPorts != 0)
        _controlsOut = new Port[controlOutPorts];
      else
        _controlsOut = 0;

      _synth->midiCtl2PortMap.clear();
      _synth->port2MidiCtlMap.clear();
                
      unsigned long int cip = 0;
      unsigned long int cop = 0;
      for (unsigned long k = 0; k < _synth->_portCount; ++k)
      {
        LADSPA_PortDescriptor pd = ld->PortDescriptors[k];
        
        #ifdef DSSI_DEBUG 
        printf("DssiSynth::init ladspa plugin Port:%lu Name:%s descriptor:%x\n", k, ld->PortNames[k], pd);
        #endif
        
        if (LADSPA_IS_PORT_CONTROL(pd)) 
        {
          if (LADSPA_IS_PORT_INPUT(pd)) 
          {
            _controls[cip].idx = k;
            float val;
            ladspaDefaultValue(ld, k, &val);
            _controls[cip].val    = val;
            _controls[cip].tmpVal = val;
            _controls[cip].enCtrl  = true;
            
            #ifdef DSSI_DEBUG 
            printf("DssiSynthIF::init control port:%d port idx:%lu name:%s\n", cip, k, ld->PortNames[k]);
            #endif
            
            // This code is duplicated in ::getControllerInfo()
            
            
            int ctlnum = DSSI_NONE;
            if(dssi->get_midi_controller_for_port)
              ctlnum = dssi->get_midi_controller_for_port(_handle, k);
            
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
              ctlnum = CTRL_NRPN14_OFFSET + 0x2000 + cip;
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
                
                ctlnum = DSSI_NRPN_NUMBER(c) + CTRL_NRPN14_OFFSET;
              }  
                
            }
            
            #ifdef DSSI_DEBUG 
            printf("DssiSynthIF::init inserting to midiCtl2PortMap: ctlnum:%d k:%d\n", ctlnum, cip);
            #endif
            
            // We have a controller number! Insert it and the DSSI port number into both maps.
            _synth->midiCtl2PortMap.insert(MidiCtl2LadspaPortInsertPair(ctlnum, cip));
            _synth->port2MidiCtlMap.insert(MidiCtl2LadspaPortInsertPair(cip, ctlnum));

            // Support a special block for dssi synth ladspa controllers. 
            // Put the ID at a special block after plugins (far after).
            int id = genACnum(MusECore::MAX_PLUGINS, cip);
            const char* name = ld->PortNames[k];
            float min, max;
            ladspaControlRange(ld, k, &min, &max);
            CtrlList* cl;
            CtrlListList* cll = track()->controller();
            iCtrlList icl = cll->find(id);
            if (icl == cll->end())
            {
              cl = new CtrlList(id);
              cll->add(cl);
              cl->setCurVal(_controls[cip].val);
            }
            else 
            {
              cl = icl->second;
              _controls[cip].val = cl->curVal();
            }
            cl->setRange(min, max);
            cl->setName(QString(name));
            cl->setValueType(ladspaCtrlValueType(ld, k));
            cl->setMode(ladspaCtrlMode(ld, k));
            // Set the value units index.
            cl->setValueUnit(valueUnit(cip));

            ld->connect_port(_handle, k, &_controls[cip].val);
            
            ++cip;
          }
          else if (LADSPA_IS_PORT_OUTPUT(pd))
          {
            _controlsOut[cop].idx = k;
            _controlsOut[cop].val    = 0.0;
            _controlsOut[cop].tmpVal = 0.0;
            _controlsOut[cop].enCtrl  = false;

            #ifdef DSSI_DEBUG 
            printf("DssiSynthIF::init control output port:%d port idx:%lu name:%s\n", cop, k, ld->PortNames[k]);
            #endif
            
            //  Control outs are not handled but still must be connected to something.
            ld->connect_port(_handle, k, &_controlsOut[cop].val);
            
            ++cop;
          }
        }
      }

      activate();

      // Set current configuration values.
      if(dssi->configure) 
      {
        char *rv = dssi->configure(_handle, DSSI_PROJECT_DIRECTORY_KEY,
            MusEGlobal::museProject.toLatin1().constData()); //MusEGlobal::song->projectPath()
        
        if(rv)
        {
          fprintf(stderr, "MusE: Warning: plugin doesn't like project directory: \"%s\"\n", rv);
          free(rv);
        }          
        
        for(ciStringParamMap r = synti->_initConfig._stringParamMap.begin(); r != synti->_initConfig._stringParamMap.end(); ++r) 
        {
          rv = 0;
          rv = dssi->configure(_handle, r->first.c_str(), r->second.c_str());
          if(rv)
          {
            fprintf(stderr, "MusE: Warning: plugin config key: %s value: %s \"%s\"\n", r->first.c_str(), r->second.c_str(), rv);
            free(rv);
          }  
        }
      }
            
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
      _synth = 0;
      _handle = nullptr;
      _controls = 0;
      _controlsOut = 0;
      _audioInBuffers = 0;
      _audioInSilenceBuf = 0;
      _audioOutBuffers = 0;
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
      
      if(_synth)
      {
        #ifdef DSSI_DEBUG 
        printf("DssiSynthIF::~DssiSynthIF synth:%p\n", synth);
        #endif
        
        if(_synth->dssi)
        {
          #ifdef DSSI_DEBUG 
          printf("DssiSynthIF::~DssiSynthIF synth->dssi:%p\n", synth->dssi);
          #endif
       
          if(_synth->dssi->LADSPA_Plugin)
          {
            #ifdef DSSI_DEBUG 
            printf("DssiSynthIF::~DssiSynthIFsynth->dssi->LADSPA_Plugin:%p\n", synth->dssi->LADSPA_Plugin);
            #endif
          }
        }
      }
      
      if(_synth && _synth->dssi && _synth->dssi->LADSPA_Plugin)
      {
        const DSSI_Descriptor* dssi = _synth->dssi;
        const LADSPA_Descriptor* descr = dssi->LADSPA_Plugin;

        #ifdef DSSI_DEBUG 
        printf("DssiSynthIF::~DssiSynthIF checking cleanup function exists\n");
        #endif
        
        if(descr->cleanup)
        {
          #ifdef DSSI_DEBUG 
          printf("DssiSynthIF::~DssiSynthIF calling cleanup function\n");
          #endif
            
          descr->cleanup(_handle);
        }    
      }
      if(_audioInBuffers)
      {
        for(unsigned long i = 0; i < _synth->_inports; ++i)
        {
          if(_audioInBuffers[i])
            free(_audioInBuffers[i]);
        }
        delete[] _audioInBuffers;
      }  
      
      if(_audioInSilenceBuf)
        free(_audioInSilenceBuf);
      
      if(_audioOutBuffers)
      {
        for(unsigned long i = 0; i < _synth->_outports; ++i)
        {
          if(_audioOutBuffers[i])
            free(_audioOutBuffers[i]);
        }
        delete[] _audioOutBuffers;
      }  
      
      if(_controls)
        delete[] _controls;
        
      if(_controlsOut)
        delete[] _controlsOut;
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

double DssiSynthIF::getParameter(unsigned long n) const
{
  if(n >= _synth->_controlInPorts)
  {
    printf("DssiSynthIF::getParameter param number %lu out of range of ports:%lu\n", n, _synth->_controlInPorts);
    return 0.0;
  }
  
  if(!_controls)
    return 0.0;
  
  return _controls[n].val;
}
//---------------------------------------------------------
//   getParameter
//---------------------------------------------------------

double DssiSynthIF::getParameterOut(unsigned long n) const
{
  if(n >= _synth->_controlOutPorts)
  {
    printf("DssiSynthIF::getParameterOut param number %lu out of range of ports:%lu\n", n, _synth->_controlOutPorts);
    return 0.0;
  }

  if(!_controlsOut)
    return 0.0;

  return _controlsOut[n].val;
}

//---------------------------------------------------------
//   setParameter
//---------------------------------------------------------

void DssiSynthIF::setParameter(unsigned long n, double v)
{
  addScheduledControlEvent(n, v, MusEGlobal::audio->curFrame());   
}

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void DssiSynthIF::write(int level, Xml& xml) const
{
#ifdef DSSI_VST_CHUNK_SUPPORT
      if(_synth->dssi->getCustomData)
      {
        //---------------------------------------------
        // dump current state of synth
        //---------------------------------------------
        printf("dumping DSSI custom data! %p\n", _synth->dssi->getCustomData);
  
        // this is only needed and supported if
        // we are talking to a VST plugin at the other end.
        std::string name = _synth->dssi->LADSPA_Plugin->Name;
        if ((name.length()> 4) && name.substr(name.length() - 4) == " VST")
        {
          printf("is vst plugin, commencing data dump, apiversion=%d!\n", _synth->dssi->DSSI_API_Version);
          unsigned long len = 0;
          void* p = 0;
          _synth->dssi->getCustomData(_handle,&p, &len);
          if (len) {
                xml.tag(level++, " version=\"%d\"", SYNTH_MIDI_STATE_SAVE_VERSION);         
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
      for(unsigned long c = 0; c < _synth->_controlInPorts; ++c)
        xml.doubleTag(level, "param", _controls[c].val);
}

//---------------------------------------------------------
//   processEvent
//   Return true if event pointer filled.
//--------------------------------------------------------

bool DssiSynthIF::processEvent(const MidiPlayEvent& e, snd_seq_event_t* event)
{
  const DSSI_Descriptor* dssi = _synth->dssi;
  
  int chn = e.channel();
  int a   = e.dataA();
  int b   = e.dataB();

  #ifdef DSSI_DEBUG 
  fprintf(stderr, "DssiSynthIF::processEvent midi event type:%d chn:%d a:%d b:%d\n", e.type(), chn, a, b);
  #endif
  
  const MidiInstrument::NoteOffMode nom = synti->noteOffMode();
  
  switch(e.type()) 
  {
    case ME_NOTEON:
      #ifdef DSSI_DEBUG 
      fprintf(stderr, "DssiSynthIF::processEvent midi event is ME_NOTEON\n");
      #endif
          
      snd_seq_ev_clear(event); 
      event->queue = SND_SEQ_QUEUE_DIRECT;
      
      if(b == 0)
      {
        // Handle zero-velocity note ons. Technically this is an error because internal midi paths
        //  are now all 'note-off' without zero-vel note ons - they're converted to note offs.
        // Nothing should be setting a Note type Event's on velocity to zero.
        // But just in case... If we get this warning, it means there is still code to change.
        fprintf(stderr, "DssiSynthIF::processEvent: Warning: Zero-vel note on: time:%d type:%d (ME_NOTEON) ch:%d A:%d B:%d\n", e.time(), e.type(), chn, a, b);  
        switch(nom)
        {
          // Instrument uses note offs. Convert to zero-vel note off.
          case MidiInstrument::NoteOffAll:
            //if(MusEGlobal::midiOutputTrace)
            //  fprintf(stderr, "MidiOut: DSSI: Following event will be converted to zero-velocity note off:\n");
            snd_seq_ev_set_noteoff(event, chn, a, 0);
          break;
          
          // Instrument uses no note offs at all. Send as-is.
          case MidiInstrument::NoteOffNone:
          // Instrument converts all note offs to zero-vel note ons. Send as-is.
          case MidiInstrument::NoteOffConvertToZVNoteOn:
            snd_seq_ev_set_noteon(event, chn, a, b);
          break;
        }
      }
      else
        snd_seq_ev_set_noteon(event, chn, a, b);
      
      
      
    break;
    case ME_NOTEOFF:
      #ifdef DSSI_DEBUG 
      fprintf(stderr, "DssiSynthIF::processEvent midi event is ME_NOTEOFF\n");
      #endif
          
      snd_seq_ev_clear(event); 
      event->queue = SND_SEQ_QUEUE_DIRECT;
      
      switch(nom)
      {
        // Instrument uses note offs. Send as-is.
        case MidiInstrument::NoteOffAll:
          snd_seq_ev_set_noteoff(event, chn, a, b);
        break;
        
        // Instrument uses no note offs at all. Send nothing. Eat up the event - return false.
        case MidiInstrument::NoteOffNone:
          return false;
          
        // Instrument converts all note offs to zero-vel note ons. Convert to zero-vel note on.
        case MidiInstrument::NoteOffConvertToZVNoteOn:
          //if(MusEGlobal::midiOutputTrace)
          //  fprintf(stderr, "MidiOut: DSSI: Following event will be converted to zero-velocity note on:\n");
          snd_seq_ev_set_noteon(event, chn, a, 0);
        break;
      }
                  
    break;
    // Synths are not allowed to receive ME_PROGRAM, CTRL_HBANK, or CTRL_LBANK alone anymore.
    case ME_PROGRAM:
    {
      #ifdef DSSI_DEBUG
      fprintf(stderr, "DssiSynthIF::processEvent midi event is ME_PROGRAM\n");
      #endif

      int hb, lb;
      synti->currentProg(chn, nullptr, &lb, &hb);
      synti->setCurrentProg(chn, a & 0xff, lb, hb);
      doSelectProgram(_handle, hb, lb, a);
      // Event pointer not filled. Return false.
      return false;
    }
    break;
    case ME_CONTROLLER:
    {
      #ifdef DSSI_DEBUG 
      fprintf(stderr, "DssiSynthIF::processEvent midi event is ME_CONTROLLER\n");
      #endif
      
      // Our internal hwCtrl controllers support the 'unknown' value.
      // Don't send 'unknown' values to the driver. Ignore and return no error.
      if(b == CTRL_VAL_UNKNOWN)
        return false;
            
      if(a == CTRL_PROGRAM)
      {
        #ifdef DSSI_DEBUG 
        fprintf(stderr, "DssiSynthIF::processEvent midi event is ME_CONTROLLER, dataA is CTRL_PROGRAM\n");
        #endif
        
        int hb = (b >> 16) & 0xff;
        int lb = (b >> 8) & 0xff;
        int pr = b & 0xff;
        synti->setCurrentProg(chn, pr, lb, hb);
        doSelectProgram(_handle, hb, lb, pr);
        // Event pointer not filled. Return false.
        return false;
      }
          
      if(a == CTRL_HBANK)
      {
        int lb, pr;
        synti->currentProg(chn, &pr, &lb, nullptr);
        synti->setCurrentProg(chn, pr, lb, b & 0xff);
        doSelectProgram(_handle, b, lb, pr);
        // Event pointer not filled. Return false.
        return false;
      }
      
      if(a == CTRL_LBANK)
      {
        int hb, pr;
        synti->currentProg(chn, &pr, nullptr, &hb);
        synti->setCurrentProg(chn, pr, b & 0xff, hb);
        doSelectProgram(_handle, hb, b, pr);
        // Event pointer not filled. Return false.
        return false;
      }
            
      if(a == CTRL_PITCH)
      {
        #ifdef DSSI_DEBUG 
        fprintf(stderr, "DssiSynthIF::processEvent midi event is ME_CONTROLLER, dataA is CTRL_PITCH\n");
        #endif
        
        snd_seq_ev_clear(event); 
        event->queue = SND_SEQ_QUEUE_DIRECT;
        snd_seq_ev_set_pitchbend(event, chn, b);
        // Event pointer filled. Return true.
        return true;
      }
          
      if(a == CTRL_AFTERTOUCH)
      {
        #ifdef DSSI_DEBUG
        fprintf(stderr, "DssiSynthIF::processEvent midi event is ME_CONTROLLER, dataA is CTRL_AFTERTOUCH\n");
        #endif

        snd_seq_ev_clear(event);
        event->queue = SND_SEQ_QUEUE_DIRECT;
        snd_seq_ev_set_chanpress(event, chn, b);
        // Event pointer filled. Return true.
        return true;
      }

      if((a | 0xff)  == CTRL_POLYAFTER)
      {
        #ifdef DSSI_DEBUG
        fprintf(stderr, "DssiSynthIF::processEvent midi event is ME_CONTROLLER, dataA is CTRL_POLYAFTER\n");
        #endif

        snd_seq_ev_clear(event);
        event->queue = SND_SEQ_QUEUE_DIRECT;
        snd_seq_ev_set_keypress(event, chn, a & 0x7f, b & 0x7f);
        // Event pointer filled. Return true.
        return true;
      }

      const LADSPA_Descriptor* ld = dssi->LADSPA_Plugin;
      
      ciMidiCtl2LadspaPort ip = _synth->midiCtl2PortMap.find(a);
      // Is it just a regular midi controller, not mapped to a LADSPA port (either by the plugin or by us)?
      // NOTE: There's no way to tell which of these controllers is supported by the plugin.
      // For example sustain footpedal or pitch bend may be supported, but not mapped to any LADSPA port.
      if(ip == _synth->midiCtl2PortMap.end())
      {
        int ctlnum = a;
        if(midiControllerType(a) != MidiController::Controller7)
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
      unsigned long i = _controls[k].idx;
      int ctlnum = DSSI_NONE;
      if(dssi->get_midi_controller_for_port)
        ctlnum = dssi->get_midi_controller_for_port(_handle, i);
        
      // No midi controller for the ladspa port? Send to ladspa control.
      if(ctlnum == DSSI_NONE)
      {
        // Sanity check.
        if(k > _synth->_controlInPorts)
          return false;
          
        // Simple but flawed solution: Start them at 0x60000 + 0x2000 = 0x62000. Max NRPN number is 0x3fff.
        ctlnum = k + (CTRL_NRPN14_OFFSET + 0x2000);
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
          ctlnum = DSSI_NRPN_NUMBER(c) + CTRL_NRPN14_OFFSET;
          
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
      _controls[k].val = val;
      
      // Need to update the automation value, otherwise it overwrites later with the last automation value.
      if(id() != -1)
        // We're in the audio thread context: no need to send a message, just modify directly.
        synti->setPluginCtrlVal(genACnum(id(), k), val);
      
      // Since we absorbed the message as a ladspa control change, return false - the event is not filled.
      return false;
    }
    break;
    case ME_PITCHBEND:
      snd_seq_ev_clear(event); 
      event->queue = SND_SEQ_QUEUE_DIRECT;
      snd_seq_ev_set_pitchbend(event, chn, a);
    break;
    case ME_AFTERTOUCH:
      snd_seq_ev_clear(event); 
      event->queue = SND_SEQ_QUEUE_DIRECT;
      snd_seq_ev_set_chanpress(event, chn, a);
    break;
    case ME_POLYAFTER:
      snd_seq_ev_clear(event);
      event->queue = SND_SEQ_QUEUE_DIRECT;
      snd_seq_ev_set_keypress(event, chn, a & 0x7f, b & 0x7f);
    break;
    case ME_SYSEX:
      {
        #ifdef DSSI_DEBUG 
        fprintf(stderr, "DssiSynthIF::processEvent midi event is ME_SYSEX\n");
        #endif
        
        const unsigned char* data = e.constData();
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
                    usleep(300000);
                    dssi->setCustomData(_handle, (unsigned char*)(data+9) /* len of str*/,e.len()-9);
                    usleep(300000);
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
          fprintf(stderr, "DssiSynthIF::processEvent midi event is ME_SYSEX PARAMSAVE\n");
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
                  
                  // Point to location after "PARAMSAVE", version major and minor, bank and program.
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
          // "DssiSynthIF::processEvent midi event is ME_SYSEX"
          // "WARNING: MIDI event of type ? decoded to 367 bytes, discarding"
          // That might be ALSA doing that.
          
          const int len = e.len();
          char buf[len + 2];
          
          buf[0] = 0xF0;
          memcpy(buf + 1, e.constData(), len);
          buf[len + 1] = 0xF7;

          snd_seq_ev_clear(event); 
          snd_seq_ev_set_sysex(event, len + 2, buf);
          event->queue = SND_SEQ_QUEUE_DIRECT;
          
          // NOTE: Don't move this out, 'buf' would go out of scope.
          // Event was filled. Return true.
          return true;
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

bool DssiSynthIF::getData(MidiPort* /*mp*/, unsigned pos, int ports, unsigned nframes, float** buffer)
{
  const unsigned long syncFrame = MusEGlobal::audio->curSyncFrame();

  #ifdef DSSI_DEBUG_PROCESS
  fprintf(stderr, "DssiSynthIF::getData: pos:%u ports:%d nframes:%u syncFrame:%lu\n", pos, ports, nframes, syncFrame);
  #endif

  // All ports must be connected to something!
  const unsigned long in_ports = _synth->inPorts();
  const unsigned long out_ports = _synth->outPorts();
  const unsigned long nop = ((unsigned long) ports) > out_ports ? out_ports : ((unsigned long) ports);

  const DSSI_Descriptor* dssi = _synth->dssi;
  const LADSPA_Descriptor* descr = dssi->LADSPA_Plugin;
  unsigned long sample = 0;

  const bool isOn = on();
  const PluginBypassType bypassType = pluginBypassType();

  //  Normally if the plugin is inactive or off we tell it to connect to dummy audio ports.
  //  But this can change depending on detected bypass type, below.
  bool connectToDummyAudioPorts = !_curActiveState || !isOn;
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
  // FIXME Better support for PluginPowerOf2BlockSize, by quantizing the control period times.
  //       For now we treat it like fixed size.
  //
  //  Normally if the plugin is inactive or off we use a fixed controller period.
  //  But this can change depending on detected bypass type, below.
  bool usefixedrate = !_curActiveState || !isOn;
  const unsigned int fin_nsamp = nframes;

  // If the plugin has a REAL enable or bypass control port, we allow the plugin
  //  a full-length run so that it can handle its own enabling or bypassing.
  if(_curActiveState)
  {
    switch(bypassType)
    {
      case PluginBypassTypeEmulatedEnableController:
      case PluginBypassTypeEmulatedEnableFunction:
      break;

      case PluginBypassTypeEnablePort:
      case PluginBypassTypeBypassPort:
          connectToDummyAudioPorts = false;
          usefixedrate = false;
      break;

      case PluginBypassTypeEnableFunction:
      case PluginBypassTypeBypassFunction:
          connectToDummyAudioPorts = false;
      break;
    }
  }

  // See if the features require a fixed control period.
  // FIXME Better support for PluginPowerOf2BlockSize, by quantizing the control period times.
  //       For now we treat it like fixed control period.
  if(requiredFeatures() & (PluginFixedBlockSize | PluginPowerOf2BlockSize | PluginCoarseBlockSize))
    usefixedrate = true;

  // Note for dssi-vst this MUST equal MusEGlobal::audio period. It doesn't like broken-up runs (it stutters),
  //  even with fixed sizes. Could be a Wine + Jack thing, wanting a full Jack buffer's length.
  // For now, the fixed size is clamped to the MusEGlobal::audio buffer size.
  // TODO: We could later add slower processing over several cycles -
  //  so that users can select a small MusEGlobal::audio period but a larger control period.
  const unsigned long min_per = (usefixedrate || MusEGlobal::config.minControlProcessPeriod > nframes) ? nframes : MusEGlobal::config.minControlProcessPeriod;
  const unsigned long min_per_mask = min_per-1;   // min_per must be power of 2

  AudioTrack* atrack = track();
  const AutomationType at = atrack->automationType();
  const bool no_auto = !MusEGlobal::automation || at == AUTO_OFF;
  const unsigned long in_ctrls = _synth->inControls();
  CtrlListList* cll = atrack->controller();
  ciCtrlList icl_first;
  const int plug_id = id();
  if(plug_id != -1)  // Don't bother if not 'running'.
    icl_first = cll->lower_bound(genACnum(plug_id, 0));

  #ifdef DSSI_DEBUG_PROCESS
  fprintf(stderr, "DssiSynthIF::getData: Handling inputs...\n");
  #endif

  bool used_in_chan_array[in_ports]; // Don't bother initializing if not 'running'.

  // Gather input data from connected input routes.
  // Don't bother if not 'running'.
  if(_curActiveState)
  {
    // Initialize the array.
    for(unsigned long i = 0; i < in_ports; ++i)
      used_in_chan_array[i] = false;

    if(!atrack->noInRoute())
    {
      RouteList *irl = atrack->inRoutes();
      for(ciRoute i = irl->begin(); i != irl->end(); ++i)
      {
        if(i->track->isMidiTrack())
          continue;
        // Only this synth knows how many destination channels there are,
        //  while only the track knows how many source channels there are.
        // So take care of the destination channels here, and let the track handle the source channels.
        const int dst_ch = i->channel <= -1 ? 0 : i->channel;
        if((unsigned long)dst_ch >= in_ports)
          continue;
        const int dst_chs = i->channels <= -1 ? in_ports : i->channels;
        //const int total_ins = atrack->totalRoutableInputs(Route::TRACK_ROUTE);
        const int src_ch = i->remoteChannel <= -1 ? 0 : i->remoteChannel;
        const int src_chs = i->channels;

        int fin_dst_chs = dst_chs;
        if((unsigned long)(dst_ch + fin_dst_chs) > in_ports)
          fin_dst_chs = in_ports - dst_ch;

        static_cast<AudioTrack*>(i->track)->copyData(pos,
                                                     dst_ch, dst_chs, fin_dst_chs,
                                                     src_ch, src_chs,
                                                     nframes, &_audioInBuffers[0],
                                                     false, used_in_chan_array);
        const int nxt_ch = dst_ch + fin_dst_chs;
        for(int ch = dst_ch; ch < nxt_ch; ++ch)
          used_in_chan_array[ch] = true;
      }
    }
  }

  // TODO: Should we implement emulated bypass for plugins without a bypass feature?
  //       That could be very difficult. How to determine any relationships between
  //        inputs and outputs? LV2 has some features for that. VST has speakers.
  //       And what about midi ports...

  #ifdef DSSI_DEBUG_PROCESS
  fprintf(stderr, "DssiSynthIF::getData: Processing automation control values...\n");
  #endif

  int cur_slice = 0;
  while(sample < fin_nsamp)
  {
    unsigned long slice_samps = fin_nsamp - sample;
    const unsigned long slice_frame = pos + sample;

    //
    // Process automation control values, while also determining the maximum acceptable
    //  size of this run. Further processing, from FIFOs for example, can lower the size
    //  from there, but this section determines where the next highest maximum frame
    //  absolutely needs to be for smooth playback of the controller value stream...
    //
    {
      ciCtrlList icl = icl_first;
      for(unsigned long k = 0; k < in_ctrls; ++k)
      {
        CtrlList* cl = (cll && plug_id != -1 && icl != cll->end()) ? icl->second : nullptr;
        CtrlInterpolate& ci = _controls[k].interp;
        // Always refresh the interpolate struct at first, since things may have changed.
        // Or if the frame is outside of the interpolate range - and eStop is not true.  // FIXME TODO: Be sure these comparisons are correct.
        if(cur_slice == 0 || (!ci.eStop && MusEGlobal::audio->isPlaying() &&
            (slice_frame < (unsigned long)ci.sFrame || (ci.eFrameValid && slice_frame >= (unsigned long)ci.eFrame)) ) )
        {
          if(cl && plug_id != -1 && (unsigned long)cl->id() == genACnum(plug_id, k))
          {
            cl->getInterpolation(slice_frame, no_auto || !_controls[k].enCtrl, &ci);
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
            ci.sVal     = _controls[k].val;
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
          _controls[k].val = cl->interpolate(MusEGlobal::audio->isPlaying() ? slice_frame : pos, ci);
        else
          _controls[k].val = ci.sVal;

#ifdef DSSI_DEBUG_PROCESS
        fprintf(stderr, "DssiSynthIF::getData k:%lu sample:%lu frame:%lu ci.eFrame:%d nsamp:%lu \n", k, sample, frame, ci.eFrame, nsamp);
#endif

      }
    }

#ifdef DSSI_DEBUG_PROCESS
    fprintf(stderr, "DssiSynthIF::getData sample:%lu nsamp:%lu\n", sample, nsamp);
#endif

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
      evframe = (syncFrame > v.frame + nframes) ? 0 : v.frame - syncFrame + nframes;

      #ifdef DSSI_DEBUG
      fprintf(stderr, "DssiSynthIF::getData found:%d evframe:%lu frame:%lu  event frame:%lu idx:%lu val:%f unique:%d\n",
          found, evframe, frame, v.frame, v.idx, v.value, v.unique);
      #endif

      // Protection. Observed this condition. Why? Supposed to be linear timestamps.
      if(found && evframe < frame)
      {
        fprintf(stderr,
          "DssiSynthIF::getData *** Error: Event out of order: evframe:%lu < frame:%lu idx:%lu val:%f unique:%d syncFrame:%lu nframes:%u v.frame:%lu\n",
          evframe, frame, v.idx, v.value, v.unique, syncFrame, nframes, v.frame);

        // No choice but to ignore it.
        _controlFifo.remove();               // Done with the ring buffer's item. Remove it.
        continue;
      }

      if(// Next events are for a later period.
         evframe >= nframes
         // Next events are for a later run in this period. (Autom took prio.)
         || (!usefixedrate && !found && !v.unique && (evframe - sample >= slice_samps))
         // Eat up events within minimum slice - they're too close.
         || (found && !v.unique && (evframe - sample >= min_per))
         // Special for dssi-vst: Fixed rate and must reply to all.
         || (usefixedrate && found && v.unique && v.idx == index))
        break;

      if(v.idx >= in_ctrls) // Sanity check.
      {
        _controlFifo.remove();               // Done with the ring buffer's item. Remove it.
        break;
      }

      found = true;
      frame = evframe;
      index = v.idx;

      {
        CtrlInterpolate* ci = &_controls[v.idx].interp;
        // Tell it to stop the current ramp at this frame, when it does stop, set this value:
        ci->eFrame = frame;
        ci->eFrameValid = true;
        ci->eVal   = v.value;
        ci->eStop  = true;
      }

      // Need to update the automation value, otherwise it overwrites later with the last automation value.
      if(plug_id != -1)
        synti->setPluginCtrlVal(genACnum(plug_id, v.idx), v.value);

      _controlFifo.remove();               // Done with the ring buffer's item. Remove it.
    }

    if(found && !usefixedrate)  // If a control FIFO item was found, takes priority over automation controller stream.
            slice_samps = frame - sample;

    if(sample + slice_samps > nframes)         // Safety check.
            slice_samps = nframes - sample;

    // TODO: Don't allow zero-length runs. This could/should be checked in the control loop instead.
    // Note this means it is still possible to get stuck in the top loop (at least for a while).
    if(slice_samps != 0)
    {
      unsigned long nevents = 0;
      // Get the state of the stop flag.
      const bool do_stop = synti->stopFlag();
      // Get whether playback and user midi events can be written to this midi device.
      const bool we = synti->writeEnable();

      MidiPlayEvent buf_ev;

      // If stopping or not 'running' just purge ALL playback FIFO and container events.
      // But do not clear the user ones. We need to hold on to them until active,
      //  they may contain crucial events like loading a soundfont from a song file.
      if(do_stop || !_curActiveState || !we)
      {
        // Transfer the user lock-free buffer events to the user sorted multi-set.
        // To avoid too many events building up in the buffer while inactive, use the exclusive add.
        const unsigned int usr_buf_sz = synti->eventBuffers(MidiDevice::UserBuffer)->getSize();
        for(unsigned int i = 0; i < usr_buf_sz; ++i)
        {
          if(synti->eventBuffers(MidiDevice::UserBuffer)->get(buf_ev))
            synti->_outUserEvents.addExclusive(buf_ev);
        }

        synti->eventBuffers(MidiDevice::PlaybackBuffer)->clearRead();
        synti->_outPlaybackEvents.clear();
        // Reset the flag.
        synti->setStopFlag(false);
      }
      else
      {
        // Transfer the user lock-free buffer events to the user sorted multi-set.
        const unsigned int usr_buf_sz = synti->eventBuffers(MidiDevice::UserBuffer)->getSize();
        for(unsigned int i = 0; i < usr_buf_sz; ++i)
        {
          if(synti->eventBuffers(MidiDevice::UserBuffer)->get(buf_ev))
            synti->_outUserEvents.insert(buf_ev);
        }

        // Transfer the playback lock-free buffer events to the playback sorted multi-set.
        const unsigned int pb_buf_sz = synti->eventBuffers(MidiDevice::PlaybackBuffer)->getSize();
        for(unsigned int i = 0; i < pb_buf_sz; ++i)
        {
          if(synti->eventBuffers(MidiDevice::PlaybackBuffer)->get(buf_ev))
            synti->_outPlaybackEvents.insert(buf_ev);
        }
      }

      // Don't bother if not 'running'.
      if(_curActiveState && we)
      {
        // Count how many events we need.
        for(ciMPEvent impe = synti->_outPlaybackEvents.begin(); impe != synti->_outPlaybackEvents.end(); ++impe)
        {
          const MidiPlayEvent& e = *impe;
          if(e.time() >= (syncFrame + sample + slice_samps))
            break;
          ++nevents;
        }
        for(ciMPEvent impe = synti->_outUserEvents.begin(); impe != synti->_outUserEvents.end(); ++impe)
        {
          const MidiPlayEvent& e = *impe;
          if(e.time() >= (syncFrame + sample + slice_samps))
            break;
          ++nevents;
        }
      }

      snd_seq_event_t events[nevents];

      // Don't bother if not 'running'.
      if(_curActiveState && we)
      {
        iMPEvent impe_pb = synti->_outPlaybackEvents.begin();
        iMPEvent impe_us = synti->_outUserEvents.begin();
        bool using_pb;

        unsigned long event_counter = 0;
        while(1)
        {
          if(impe_pb != synti->_outPlaybackEvents.end() && impe_us != synti->_outUserEvents.end())
            using_pb = *impe_pb < *impe_us;
          else if(impe_pb != synti->_outPlaybackEvents.end())
            using_pb = true;
          else if(impe_us != synti->_outUserEvents.end())
            using_pb = false;
          else break;

          const MidiPlayEvent& e = using_pb ? *impe_pb : *impe_us;

          #ifdef DSSI_DEBUG
          fprintf(stderr, "DssiSynthIF::getData eventFifos event time:%d\n", e.time());
          #endif

          // Event is for future?
          if(e.time() >= (sample + slice_samps + syncFrame))
            break;

          // Returns false if the event was not filled. It was handled, but some other way.
          if(processEvent(e, &events[event_counter]))
          {
            // Time-stamp the event.
            unsigned int ft = (e.time() < syncFrame) ? 0 : e.time() - syncFrame;
            ft = (ft < sample) ? 0 : ft - sample;
            if (ft >= slice_samps)
            {
                fprintf(stderr, "DssiSynthIF::getData: eventFifos event time:%d "
                "out of range. pos:%d syncFrame:%lu ft:%u sample:%lu slice_samps:%lu\n",
                        e.time(), pos, syncFrame, ft, sample, slice_samps);
                ft = slice_samps - 1;
            }
            // "Each event is timestamped relative to the start of the block, (mis)using the ALSA "tick time" field as a frame count.
            //  The host is responsible for ensuring that events with differing timestamps are already ordered by time."  -  From dssi.h
            events[event_counter].time.tick = ft;

            ++event_counter;
          }

          // Done with buffer's event. Remove it.
          // C++11.
          if(using_pb)
            impe_pb = synti->_outPlaybackEvents.erase(impe_pb);
          else
            impe_us = synti->_outUserEvents.erase(impe_us);
        }

        if(event_counter < nevents)
          nevents = event_counter;
      }

      // Don't bother if not 'running'.
      if(_curActiveState)
      {
        #ifdef DSSI_DEBUG_PROCESS
        fprintf(stderr, "DssiSynthIF::getData: Connecting and running. sample:%lu nsamp:%lu nevents:%lu\n", sample, nsamp, nevents);
        #endif

        for(unsigned long k = 0; k < out_ports; ++k)
        {
          if(!connectToDummyAudioPorts && k < nop)
            // Connect the given buffers directly to the ports, up to a max of synth ports.
            descr->connect_port(_handle, _synth->oIdx[k], buffer[k] + sample);
          else
            // Connect the remaining ports to some local buffers (not used yet).
            descr->connect_port(_handle, _synth->oIdx[k], _audioOutBuffers[k] + sample);
        }

        // Connect all inputs either to the input buffers, or a silence buffer.
        for(unsigned long k = 0; k < in_ports; ++k)
        {
          if(!connectToDummyAudioPorts && used_in_chan_array[k])
            descr->connect_port(_handle, _synth->iIdx[k], _audioInBuffers[k] + sample);
          else
            descr->connect_port(_handle, _synth->iIdx[k], _audioInSilenceBuf + sample);
        }

        // Run the synth for a period of time. This processes events and gets/fills our local buffers...
        if(_synth->dssi->run_synth)
        {
          _synth->dssi->run_synth(_handle, slice_samps, nevents == 0 ? nullptr : events, nevents);
        }
        else if (_synth->dssi->run_multiple_synths)
        {
          snd_seq_event_t* ev[1];
          unsigned long nev[1];
          ev[0] = (nevents == 0 ? nullptr : events);
          nev[0] = nevents;
          _synth->dssi->run_multiple_synths(1, &_handle, slice_samps, ev, nev);
        }
        // TIP: Until we add programs to plugins, uncomment these four checks to load dssi effects as synths, in order to have programs.
        //else
        //if(synth->dssi->LADSPA_Plugin->run)
        //{
        //  synth->dssi->LADSPA_Plugin->run(handle, nsamp);
        //}
      }

      sample += slice_samps;
    }

    ++cur_slice; // Slice is done. Moving on to any next slice now...
  }

  return true;
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
            dssi = nullptr;
            df   = nullptr;
            iIdx.clear(); 
            oIdx.clear(); 
            rpIdx.clear();
            midiCtl2PortMap.clear();
            port2MidiCtlMap.clear();
      }
}

//---------------------------------------------------------
//   guiHeartBeat
//---------------------------------------------------------

void DssiSynthIF::guiHeartBeat()
{
  SynthIF::guiHeartBeat();

  #ifdef OSC_SUPPORT
  int chn = 0;  // TODO: Channel?
  int hb, lb, pr;
  synti->currentProg(chn, &pr, &lb, &hb);
  if(hb > 127) // Map "dont care" to 0
    hb = 0;
  if(lb > 127)
    lb = 0;
  if(pr > 127)
    pr = 0;
  // Update the gui's program if needed.
  _oscif.oscSendProgram(pr, (hb << 8) + lb);
  
  // Update the gui's controls if needed.
  unsigned long ports = _synth->_controlInPorts;

  for(unsigned long i = 0; i < ports; ++i)
    _oscif.oscSendControl(_controls[i].idx, _controls[i].val);
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
      for(ciStringParamMap r = synti->_initConfig._stringParamMap.begin(); r != synti->_initConfig._stringParamMap.end(); ++r) 
      {
        _oscif.oscSendConfigure(r->first.c_str(), r->second.c_str());
        // Avoid overloading the GUI if there are lots and lots of params. 
        if((i+1) % 50 == 0)
          usleep(300000);
        ++i;      
      }  
      
      // Send current bank and program.
      int chn = 0;  // TODO: Channel?
      int hb, lb, pr;
      synti->currentProg(chn, &pr, &lb, &hb);
      if(hb > 127) // Map "dont care" to 0
        hb = 0;
      if(lb > 127)
        lb = 0;
      if(pr > 127)
        pr = 0;
      _oscif.oscSendProgram(pr, (hb << 8) + lb, true /*force*/);
      
      // Send current control values.
      unsigned long ports = _synth->_controlInPorts;
      for(unsigned long i = 0; i < ports; ++i) 
      {
        _oscif.oscSendControl(_controls[i].idx, _controls[i].val, true /*force*/);
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

      // 16384 banks arranged as 128 hi and lo banks each with up to the first 128 programs supported.
      int hb = bank >> 8;
      int lb = bank & 0xff;
      if(hb > 127 || lb > 127 || program > 127)
        return 0;
      hb &= 0x7f;
      lb &= 0x7f;
      
      synti->setCurrentProg(ch, program, lb, hb);
      
      if(port != -1)
      {
        // Synths are not allowed to receive ME_PROGRAM, CTRL_HBANK, or CTRL_LBANK alone anymore.
        const MidiPlayEvent event(0, port, ch, ME_CONTROLLER, CTRL_PROGRAM, (hb << 16) | (lb << 8) | program);
      
        #ifdef DSSI_DEBUG 
        fprintf(stderr, "DssiSynthIF::oscProgram midi event chn:%d a:%d b:%d\n", event.channel(), event.dataA(), event.dataB());
        #endif
        
        MusEGlobal::midiPorts[port].putEvent(event);
      }
      
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
  
  if(port >= _synth->rpIdx.size())
  {
    fprintf(stderr, "DssiSynthIF::oscControl: port number:%lu is out of range of index list size:%zd\n", port, _synth->rpIdx.size());
    return 0;
  }
  
  // Convert from DSSI port number to control input port index.
  unsigned long cport = _synth->rpIdx[port];
  
  if((int)cport == -1)
  {
    fprintf(stderr, "DssiSynthIF::oscControl: port number:%lu is not a control input\n", port);
    return 0;
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
    synti->recordAutomation(pid, value);
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
  
  // Schedules a timed control change:
  ControlEvent ce;
  ce.unique = _synth->isDssiVst();   // Special for messages from vst gui to host - requires processing every message.
  ce.fromGui = true;                 // It came from the plugin's own GUI.
  ce.idx = cport;
  ce.value = value;
  // Don't use timestamp(), because it's circular, which is making it impossible to deal
  // with 'modulo' events which slip in 'under the wire' before processing the ring buffers.
  ce.frame = MusEGlobal::audio->curFrame();
  if(_controlFifo.put(ce))
    fprintf(stderr, "DssiSynthIF::oscControl: fifo overflow: in control number:%lu\n", cport);

  enableController(cport, false); //TODO maybe re-enable the ctrl soon?
  
  return 0;
}

//---------------------------------------------------------
//   oscMidi
//---------------------------------------------------------

int DssiSynthIF::oscMidi(int a, int b, int c)
      {
        // From the DSSI RFC document:
        // <base path>/midi
        // "Send an arbitrary MIDI event to the plugin. Takes a four-byte MIDI string.
        //  This is expected to be used for note data generated from a test panel on the UI,
        //   for example. It should not be used for program or controller changes, sysex data, etc.
        //  A host should feel free to drop any values it doesn't wish to pass on.
        //  No guarantees are provided about timing accuracy, etc, of the MIDI communication."

        // From dssi.h:
        // " A host must not attempt to switch notes off by sending
        //    zero-velocity NOTE_ON events.  It should always send true
        //    NOTE_OFFs.  It is the host's responsibility to remap events in
        //    cases where an external MIDI source has sent it zero-velocity
        //    NOTE_ONs."
        
      int type = a & 0xf0;
      if (type == ME_NOTEON && c == 0) {
            type = ME_NOTEOFF;
            c = 64;
            }
            
      const int channel = a & 0x0f;
      
      const int port    = synti->midiPort();        
      
      if(port != -1)
      {
        // Time-stamp the event.
        MidiPlayEvent event(MusEGlobal::audio->curFrame(), port, channel, type, b, c);
      
        #ifdef DSSI_DEBUG   
        printf("DssiSynthIF::oscMidi midi event port:%d type:%d chn:%d a:%d b:%d\n", event.port(), event.type(), event.channel(), event.dataA(), event.dataB());  
        #endif
        
        // Just in case someone decides to send controllers, sysex, or stuff
        //  OTHER than "test notes", contrary to the rules...
        // Since this is a thread other than audio or gui, it may not be safe to
        //  even ask whether a controller exists, so MidiPort::putEvent or putHwCtrlEvent
        //  would not be safe here. Ask the gui to do it for us.
        // Is it a controller message? Send the message to the gui.
        //if(event.translateCtrlNum() >= 0)
          MusEGlobal::song->putIpcInEvent(event);
          
        // Send the message to the device.
        if(MidiDevice* md = MusEGlobal::midiPorts[port].device())
          md->putEvent(event, MidiDevice::Late);
      }
      
      return 0;
      }

//---------------------------------------------------------
//   oscConfigure
//---------------------------------------------------------

int DssiSynthIF::oscConfigure(const char *key, const char *value)
      {
      // "The host has the option to remember the set of (key,value)
      // pairs associated with a particular instance, so that if it
      // wants to restore the "same" instance on another occasion it can
      // just call configure() on it for each of those pairs and so
      // restore state without any input from a GUI." 

      #ifdef DSSI_DEBUG 
      printf("DssiSynthIF::oscConfigure synth name:%s key:%s value:%s\n", synti->name().toLatin1().constData(), key, value);
      #endif
      
      // Add or modify the configuration map item.
      synti->_initConfig._stringParamMap.set(key, value);
      
      if (!strncmp(key, DSSI_RESERVED_CONFIGURE_PREFIX,
         strlen(DSSI_RESERVED_CONFIGURE_PREFIX))) {
            fprintf(stderr, "MusE: OSC: UI for plugin '%s' attempted to use reserved configure key \"%s\", ignoring\n",
               synti->name().toLatin1().constData(), key);
            return 0;
            }

      if (!_synth->dssi->configure)
            return 0;

      char* message = _synth->dssi->configure(_handle, key, value);
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

      if (!_synth->dssi->get_program)
            return;

      for (int i = 0;; ++i) {
            const DSSI_Program_Descriptor* pd = _synth->dssi->get_program(_handle, i);
            if (pd == 0)
                  break;
            
            // 16384 banks arranged as 128 hi and lo banks each with up to the first 128 programs supported.
            if((pd->Bank >> 8) > 127 || 
               (pd->Bank & 0xff) > 127 || 
               pd->Program > 127)
              continue;
            
            DSSI_Program_Descriptor d;
            d.Name    = strdup(pd->Name);
            d.Program = pd->Program;
            d.Bank    = pd->Bank;
            programs.push_back(d);
            }
      }

void DssiSynthIF::doSelectProgram(LADSPA_Handle handle, int bankH, int bankL, int prog)
{
  if(bankH > 127) // Map "dont care" to 0
    bankH = 0;
  if(bankL > 127)
    bankL = 0;
  if(prog > 127)
    prog = 0;
    
  const int bank = (bankH << 8) | bankL;
  
  const DSSI_Descriptor* dssi = _synth->dssi;
  dssi->select_program(handle, bank, prog);
  
  // Need to update the automation value, otherwise it overwrites later with the last automation value.
  //   "A plugin is permitted to re-write the values of its input control ports when select_program is called.  
  //    The host should re-read the input control port values and update its own records appropriately.  
  //    (This is the only circumstance in which a DSSI plugin is allowed to modify its own input ports.)"   From dssi.h
  if(id() != -1)
  {
    for(unsigned long k = 0; k < _synth->_controlInPorts; ++k)
    {  
      // We're in the audio thread context: no need to send a message, just modify directly.
      synti->setPluginCtrlVal(genACnum(id(), k), _controls[k].val);
    }
  }
}

//---------------------------------------------------------
//   getPatchName
//---------------------------------------------------------

QString DssiSynthIF::getPatchName(int /*chan*/, int prog, bool /*drum*/) const
      {
      unsigned program = prog & 0xff;
      unsigned lbank   = (prog >> 8) & 0xff;
      unsigned hbank   = (prog >> 16) & 0xff;

      if (program > 127)  // Map "dont care" to 0
            program = 0;
      if (lbank > 127)
            lbank = 0;
      if (hbank > 127)
            hbank = 0;
      const unsigned bank = (hbank << 8) + lbank;

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

void DssiSynthIF::populatePatchPopup(MusEGui::PopupMenu* menu, int /*ch*/, bool /*drum*/) 
      {
      // The plugin can change the programs, patches etc.
      // So make sure we're up to date by calling queryPrograms.
      queryPrograms();
      
      menu->clear();

      for (std::vector<DSSI_Program_Descriptor>::const_iterator i = programs.begin();
         i != programs.end(); ++i) {
            // 16384 banks arranged as 128 hi and lo banks each with up to the first 128 programs supported.
            int hb = i->Bank >> 8;
            int lb = i->Bank & 0xff;
            if(hb > 127 || lb > 127 || i->Program > 127)
              continue;
            hb &= 0x7f;
            lb &= 0x7f;

            QString astr;
            astr += QString::number(hb + 1) + QString(":");
            astr += QString::number(lb + 1) + QString(":");
            astr += QString::number(i->Program + 1);
            astr += QString(" ");
            astr += QString(i->Name);

            QAction *act = menu->addAction(astr);
            act->setData((hb << 16) | (lb << 8) | (int)i->Program);
            }
      }

int DssiSynthIF::getControllerInfo(int id, QString* name, int* ctrl, int* min, int* max, int* initval)
{
  int controlPorts = _synth->_controlInPorts;
  if(id == controlPorts || id == controlPorts + 1)
  {
    //
    // It is unknown at this point whether or not a synth recognizes aftertouch and poly aftertouch
    //  (channel and key pressure) midi messages, so add support for them now (as controllers).
    //
    if(id == controlPorts)
      *ctrl = CTRL_POLYAFTER;
    else if(id == controlPorts + 1)
      *ctrl = CTRL_AFTERTOUCH;
    *min  = 0;
    *max  = 127;
    *initval = CTRL_VAL_UNKNOWN;
    *name = midiCtrlName(*ctrl);
    return ++id;
  }
  else if(id >= controlPorts + 2)
    return 0;

  const DSSI_Descriptor* dssi = _synth->dssi;
  const LADSPA_Descriptor* ld = dssi->LADSPA_Plugin;
  
  unsigned long i = _controls[id].idx;
  
  #ifdef DSSI_DEBUG 
  printf("DssiSynthIF::getControllerInfo control port:%d port idx:%lu name:%s\n", id, i, ld->PortNames[i]);
  #endif
  
  int ctlnum = DSSI_NONE;
  if(dssi->get_midi_controller_for_port)
    ctlnum = dssi->get_midi_controller_for_port(_handle, i);
  
  
  // No controller number? Give it one.
  if(ctlnum == DSSI_NONE)
  {
    // Simple but flawed solution: Start them at 0x60000 + 0x2000 = 0x62000. Max NRPN number is 0x3fff.
    ctlnum = CTRL_NRPN14_OFFSET + 0x2000 + id;
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
      
      ctlnum = DSSI_NRPN_NUMBER(c) + CTRL_NRPN14_OFFSET;
    }  
  }
  
  int def = CTRL_VAL_UNKNOWN;
  if(ladspa2MidiControlValues(ld, i, ctlnum, min, max, &def))
    *initval = def;
  else
    *initval = CTRL_VAL_UNKNOWN;
    
  #ifdef DSSI_DEBUG 
  printf("DssiSynthIF::getControllerInfo passed ctlnum:%d min:%d max:%d initval:%d\n", ctlnum, *min, *max, *initval);
  #endif
  
  *ctrl = ctlnum;
  *name = QString(ld->PortNames[i]);
  return ++id;
}

int DssiSynthIF::channels() const 
{ 
    return ((int)_synth->_outports) > MusECore::MAX_CHANNELS ? MusECore::MAX_CHANNELS : ((int)_synth->_outports) ;
}

int DssiSynthIF::totalOutChannels() const 
{ 
  return _synth->_outports;
}

int DssiSynthIF::totalInChannels() const 
{ 
  return _synth->_inports;
}

void DssiSynthIF::deactivate3()
{
  deactivate();
}


//--------------------------------
// Methods for PluginIBase:
//--------------------------------

unsigned long DssiSynthIF::pluginID() const                  { return (_synth && _synth->dssi) ? _synth->dssi->LADSPA_Plugin->UniqueID : 0; }
int DssiSynthIF::id() const                                  { return MusECore::MAX_PLUGINS; } // Set for special block reserved for dssi synth.
QString DssiSynthIF::pluginLabel() const                     { return (_synth && _synth->dssi) ? QString(_synth->dssi->LADSPA_Plugin->Label) : QString(); }
QString DssiSynthIF::lib() const                             { return _synth ? _synth->completeBaseName() : QString(); }
QString DssiSynthIF::uri() const                             { return _synth ? _synth->uri() : QString(); }
QString DssiSynthIF::dirPath() const                         { return _synth ? _synth->absolutePath() : QString(); }
QString DssiSynthIF::fileName() const                        { return _synth ? _synth->fileName() : QString(); }
void DssiSynthIF::enableController(unsigned long i, bool v)  { _controls[i].enCtrl = v; }
bool DssiSynthIF::controllerEnabled(unsigned long i) const   { return _controls[i].enCtrl; }
void DssiSynthIF::enableAllControllers(bool v)               
{ 
  if(!_synth)
    return;
  for(unsigned long i = 0; i < _synth->_controlInPorts; ++i)
    _controls[i].enCtrl = v;
}
void DssiSynthIF::updateControllers() { }
void DssiSynthIF::activate()
{
  if(_curActiveState)
    return;
  if(_synth && _synth->dssi && _synth->dssi->LADSPA_Plugin && _synth->dssi->LADSPA_Plugin->activate)
  {
    _synth->dssi->LADSPA_Plugin->activate(_handle);
    SynthIF::activate();
  }


// REMOVE Tim. Or keep? From PluginI::activate().
//   if (initControlValues) {
//         for (unsigned long i = 0; i < controlPorts; ++i) {
//               controls[i].val = controls[i].tmpVal;
//               }
//         }
//   else {
//         // get initial control values from plugin
//         for (unsigned long i = 0; i < controlPorts; ++i) {
//               controls[i].tmpVal = controls[i].val;
//               }
//         }
}
void DssiSynthIF::deactivate()
{
  if(!_curActiveState)
    return;
  SynthIF::deactivate();
  if(_synth && _synth->dssi && _synth->dssi->LADSPA_Plugin && _synth->dssi->LADSPA_Plugin->deactivate)
    _synth->dssi->LADSPA_Plugin->deactivate(_handle);
}

unsigned long DssiSynthIF::parameters() const                { return _synth ? _synth->_controlInPorts : 0; }
unsigned long DssiSynthIF::parametersOut() const             { return _synth ? _synth->_controlOutPorts : 0; }
void DssiSynthIF::setParam(unsigned long i, double val)      { setParameter(i, val); }
double DssiSynthIF::param(unsigned long i) const             { return getParameter(i); }
double DssiSynthIF::paramOut(unsigned long i) const          { return getParameterOut(i); }
const char* DssiSynthIF::paramName(unsigned long i) const
  { return (_synth && _synth->dssi) ? _synth->dssi->LADSPA_Plugin->PortNames[_controls[i].idx] : 0; }
const char* DssiSynthIF::paramOutName(unsigned long i) const
  { return (_synth && _synth->dssi) ? _synth->dssi->LADSPA_Plugin->PortNames[_controlsOut[i].idx] : 0; }
LADSPA_PortRangeHint DssiSynthIF::range(unsigned long i) const
  { return _synth->dssi->LADSPA_Plugin->PortRangeHints[_controls[i].idx]; }
LADSPA_PortRangeHint DssiSynthIF::rangeOut(unsigned long i) const
  { return _synth->dssi->LADSPA_Plugin->PortRangeHints[_controlsOut[i].idx]; }
void DssiSynthIF::range(unsigned long i, float* min, float* max) const
  { ladspaControlRange(_synth->dssi->LADSPA_Plugin, _controls[i].idx, min, max); }
void DssiSynthIF::rangeOut(unsigned long i, float* min, float* max) const
  { ladspaControlRange(_synth->dssi->LADSPA_Plugin, _controlsOut[i].idx, min, max); }
CtrlValueType DssiSynthIF::ctrlValueType(unsigned long i) const
  { return ladspaCtrlValueType(_synth->dssi->LADSPA_Plugin, _controls[i].idx); }
CtrlList::Mode DssiSynthIF::ctrlMode(unsigned long i) const
  { return ladspaCtrlMode(_synth->dssi->LADSPA_Plugin, _controls[i].idx); };
CtrlValueType DssiSynthIF::ctrlOutValueType(unsigned long i) const
  { return ladspaCtrlValueType(_synth->dssi->LADSPA_Plugin, _controlsOut[i].idx); }
CtrlList::Mode DssiSynthIF::ctrlOutMode(unsigned long i) const
  { return ladspaCtrlMode(_synth->dssi->LADSPA_Plugin, _controlsOut[i].idx); };

} // namespace MusECore

#else //DSSI_SUPPORT
namespace MusECore {
void initDSSI() {}
}
#endif

