//=============================================================================
//  MusE
//  Linux Music Editor
//
//  lv2host.cpp
//  Copyright (C) 2014 by Deryabin Andrew <andrewderyabin@gmail.com>
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
#ifdef LV2_SUPPORT

#define LV2_HOST_CPP

#include <string>
#include <string.h>
#include <signal.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <iostream>
#include <time.h>
#include <dlfcn.h>
#include <QMessageBox>

#include <QDir>
#include <QFileInfo>
#include <QUrl>
//#include <QX11EmbedWidget>
#include <QCoreApplication>
#include <QtGui/QWindow>

#include "lv2host.h"
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
#include <ladspa.h>

#include <math.h>
#include <assert.h>
#include <stdio.h>
#include <stdarg.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


//uncomment to print audio process info
//#define LV2_DEBUG_PROCESS

#ifdef __x86_64__
#define LV2_GTK_HELPER LIBDIR "/modules/lv2Gtk2Helper64.so"
#else
#define LV2_GTK_HELPER LIBDIR "/modules/lv2Gtk2Helper32.so"
#endif

namespace MusECore
{

#define NS_EXT "http://lv2plug.in/ns/ext/"
#define NS_LV2CORE "http://lv2plug.in/ns/lv2core"

#define LV2_INSTRUMENT_CLASS NS_LV2CORE "#InstrumentPlugin"
#define LV2_F_BOUNDED_BLOCK_LENGTH LV2_BUF_SIZE__boundedBlockLength
#define LV2_F_FIXED_BLOCK_LENGTH LV2_BUF_SIZE__fixedBlockLength
#define LV2_P_SEQ_SIZE LV2_BUF_SIZE__sequenceSize
#define LV2_P_MAX_BLKLEN LV2_BUF_SIZE__maxBlockLength
#define LV2_P_MIN_BLKLEN LV2_BUF_SIZE__minBlockLength
#define LV2_P_SAMPLE_RATE LV2_PARAMETERS__sampleRate
#define LV2_F_OPTIONS LV2_OPTIONS__options
#define LV2_F_URID_MAP LV2_URID__map
#define LV2_F_URID_UNMAP LV2_URID__unmap
#define LV2_F_URI_MAP LV2_URI_MAP_URI
#define LV2_F_UI_PARENT LV2_UI__parent
#define LV2_F_INSTANCE_ACCESS NS_EXT "instance-access"
#define LV2_F_DATA_ACCESS LV2_DATA_ACCESS_URI
#define LV2_F_UI_EXTERNAL_HOST LV2_EXTERNAL_UI__Host
#define LV2_F_WORKER_SCHEDULE LV2_WORKER__schedule
#define LV2_F_WORKER_INTERFACE LV2_WORKER__interface
#define LV2_F_UI_IDLE LV2_UI__idleInterface
#define LV2_F_UI_Qt5_UI LV2_UI_PREFIX "Qt5UI"
#define LV2_UI_HOST_URI LV2_F_UI_Qt5_UI
#define LV2_UI_EXTERNAL LV2_EXTERNAL_UI__Widget
#define LV2_UI_EXTERNAL_DEPRECATED LV2_EXTERNAL_UI_DEPRECATED_URI
#define LV2_F_DEFAULT_STATE LV2_STATE_PREFIX "loadDefaultState"


static LilvWorld *lilvWorld = 0;
static int uniqueID = 1;
static bool bLV2Gtk2Enabled = false;
static void *lv2Gtk2HelperHandle = NULL;

//uri cache structure.
typedef struct
{
   LilvNode *atom_AtomPort;
   LilvNode *ev_EventPort;
   LilvNode *lv2_AudioPort;
   LilvNode *lv2_ControlPort;
   LilvNode *lv2_InputPort;
   LilvNode *lv2_OutputPort;
   LilvNode *lv2_connectionOptional;
   LilvNode *host_uiType;
   LilvNode *ext_uiType;
   LilvNode *ext_d_uiType;
   LilvNode *lv2_portDiscrete;
   LilvNode *lv2_portContinuous;
   LilvNode *lv2_portLogarithmic;
   LilvNode *lv2_portInteger;
   LilvNode *lv2_portTrigger;
   LilvNode *lv2_portToggled;
   LilvNode *lv2_TimePosition;
   LilvNode *lv2_FreeWheelPort;
   LilvNode *lv2_SampleRate;
   LilvNode *lv2_CVPort;
   LilvNode *lv2_psetPreset;
   LilvNode *lv2_rdfsLabel;
   LilvNode *end;  ///< NULL terminator for easy freeing of entire structure
} CacheNodes;

LV2_URID Synth_Urid_Map(LV2_URID_Unmap_Handle _host_data, const char *uri)
{
   LV2Synth *_synth = reinterpret_cast<LV2Synth *>(_host_data);

   if(_synth == NULL)   //broken plugin
   {
      return 0;
   }

   return _synth->mapUrid(uri);
}

const char *Synth_Urid_Unmap(LV2_URID_Unmap_Handle _host_data, LV2_URID id)
{
   LV2Synth *_synth = reinterpret_cast<LV2Synth *>(_host_data);

   if(_synth == NULL)   //broken plugin
   {
      return NULL;
   }

   return _synth->unmapUrid(id);
}

LV2_URID Synth_Uri_Map(LV2_URI_Map_Callback_Data _host_data, const char *, const char *uri)
{
   LV2Synth *_synth = reinterpret_cast<LV2Synth *>(_host_data);

   if(_synth == NULL)   //broken plugin
   {
      return 0;
   }

   return _synth->mapUrid(uri);
}


static CacheNodes lv2CacheNodes;

LV2_Feature lv2Features [] =
{
   {LV2_F_URID_MAP, NULL},
   {LV2_F_URID_UNMAP, NULL},
   {LV2_F_URI_MAP, NULL},
   {LV2_F_BOUNDED_BLOCK_LENGTH, NULL},
   {LV2_F_FIXED_BLOCK_LENGTH, NULL},
   {LV2_F_UI_PARENT, NULL},
   {LV2_F_INSTANCE_ACCESS, NULL},
   {LV2_F_UI_EXTERNAL_HOST, NULL},
   {LV2_UI_EXTERNAL_DEPRECATED, NULL},
   {LV2_F_WORKER_SCHEDULE, NULL},
   {LV2_F_UI_IDLE, NULL},
   {LV2_F_OPTIONS, NULL},
   {LV2_UI__resize, NULL},
   {LV2_PROGRAMS__Host, NULL},
   {LV2_LOG__log, NULL},
   {LV2_STATE__makePath, NULL},
   {LV2_STATE__mapPath, NULL},
   {LV2_F_DATA_ACCESS, NULL} //must be the last always!
};

std::vector<LV2Synth *> synthsToFree;



#define SIZEOF_ARRAY(x) sizeof(x)/sizeof(x[0])

void initLV2()
{

   //first of all try to init gtk 2 helper (for opening lv2 gtk2/gtkmm2 guis)

   lv2Gtk2HelperHandle = dlopen(LV2_GTK_HELPER, RTLD_NOW);
   char *dlerr = dlerror();
   fprintf(stderr, "Lv2Gtk2Helper: dlerror = %s\n", dlerr);
   if(lv2Gtk2HelperHandle != NULL)
   {
      bool( * lv2Gtk2Helper_initFn)();
      *(void **)(&lv2Gtk2Helper_initFn) = dlsym(lv2Gtk2HelperHandle, "lv2Gtk2Helper_init");
      bool bHelperInit = lv2Gtk2Helper_initFn();
      if(bHelperInit)
         bLV2Gtk2Enabled = true;
   }

   if(!bLV2Gtk2Enabled)
   {
      QMessageBox::critical(NULL, "MusE LV2 host error", QString("<b>LV2 GTK2 ui support is not available</b><br />"
                                                         "This may happen because of the following reasons:<br />"
                                                         "<b>1.</b> lv2Gtk2Helper32.so/lv2Gtk2Helper64.so is missing needed dependences.<br />"
                                                         "This may be checked by executing<br />"
                                                         "<b>ldd " LV2_GTK_HELPER "</b><br />"
                                                         "in terminal window.<br />"
                                                         "<b>2.</b> lv2Gtk2Helper32.so/lv2Gtk2Helper64.so was not found in MusE modules dir.<br />"
                                                         "It can be recompiled and reinstalled from muse2/muse/lv2Gtk2Helper folder "
                                                         " from MusE source package. dl error was:"
                                                          ) + QString::fromUtf8(dlerror()));
   }

   std::set<std::string> supportedFeatures;
   uint32_t i = 0;

   for(i = 0; i < SIZEOF_ARRAY(lv2Features); i++)
   {
      supportedFeatures.insert(lv2Features [i].URI);
   }

   lilvWorld = lilv_world_new();

   lv2CacheNodes.atom_AtomPort          = lilv_new_uri(lilvWorld, LV2_ATOM__AtomPort);
   lv2CacheNodes.ev_EventPort           = lilv_new_uri(lilvWorld, LV2_EVENT__EventPort);
   lv2CacheNodes.lv2_AudioPort          = lilv_new_uri(lilvWorld, LV2_CORE__AudioPort);
   lv2CacheNodes.lv2_ControlPort        = lilv_new_uri(lilvWorld, LV2_CORE__ControlPort);
   lv2CacheNodes.lv2_InputPort          = lilv_new_uri(lilvWorld, LV2_CORE__InputPort);
   lv2CacheNodes.lv2_OutputPort         = lilv_new_uri(lilvWorld, LV2_CORE__OutputPort);
   lv2CacheNodes.lv2_connectionOptional = lilv_new_uri(lilvWorld, LV2_CORE__connectionOptional);
   lv2CacheNodes.host_uiType            = lilv_new_uri(lilvWorld, LV2_UI_HOST_URI);
   lv2CacheNodes.ext_uiType             = lilv_new_uri(lilvWorld, LV2_UI_EXTERNAL);
   lv2CacheNodes.ext_d_uiType           = lilv_new_uri(lilvWorld, LV2_UI_EXTERNAL_DEPRECATED);
   lv2CacheNodes.lv2_portContinuous     = lilv_new_uri(lilvWorld, LV2_PORT_PROPS__continuousCV);
   lv2CacheNodes.lv2_portDiscrete       = lilv_new_uri(lilvWorld, LV2_PORT_PROPS__discreteCV);
   lv2CacheNodes.lv2_portLogarithmic    = lilv_new_uri(lilvWorld, LV2_PORT_PROPS__logarithmic);
   lv2CacheNodes.lv2_portInteger        = lilv_new_uri(lilvWorld, LV2_CORE__integer);
   lv2CacheNodes.lv2_portTrigger        = lilv_new_uri(lilvWorld, LV2_PORT_PROPS__trigger);
   lv2CacheNodes.lv2_portToggled        = lilv_new_uri(lilvWorld, LV2_CORE__toggled);
   lv2CacheNodes.lv2_TimePosition       = lilv_new_uri(lilvWorld, LV2_TIME__Position);
   lv2CacheNodes.lv2_FreeWheelPort      = lilv_new_uri(lilvWorld, LV2_CORE__freeWheeling);
   lv2CacheNodes.lv2_SampleRate         = lilv_new_uri(lilvWorld, LV2_CORE__sampleRate);
   lv2CacheNodes.lv2_CVPort             = lilv_new_uri(lilvWorld, LV2_CORE__CVPort);
   lv2CacheNodes.lv2_psetPreset         = lilv_new_uri(lilvWorld, LV2_PRESETS__Preset);
   lv2CacheNodes.lv2_rdfsLabel        = lilv_new_uri(lilvWorld, "http://www.w3.org/2000/01/rdf-schema#label");
   lv2CacheNodes.end                    = NULL;

   lilv_world_load_all(lilvWorld);
   const LilvPlugins *plugins = lilv_world_get_all_plugins(lilvWorld);
   LilvIter *pit = lilv_plugins_begin(plugins);

   while(true)
   {
      if(lilv_plugins_is_end(plugins, pit))
      {
         break;
      }

      const LilvPlugin *plugin = lilv_plugins_get(plugins, pit);

      if(lilv_plugin_is_replaced(plugin))
      {
         continue;
      }

      LilvNode *nameNode = lilv_plugin_get_name(plugin);
      //const LilvNode *uriNode = lilv_plugin_get_uri(plugin);

      if(lilv_node_is_string(nameNode))
      {
         bool shouldLoad = true;
         const char *pluginName = lilv_node_as_string(nameNode);
         //const char *pluginUri = lilv_node_as_string(uriNode);
#ifdef DEBUG_LV2
         std::cerr << "Found LV2 plugin: " << pluginName << std::endl;
#endif
         const char *lfp = lilv_uri_to_path(lilv_node_as_string(lilv_plugin_get_library_uri(plugin)));
#ifdef DEBUG_LV2
         std::cerr << "Library path: " << lfp << std::endl;
#endif



#ifdef DEBUG_LV2
         const LilvPluginClass *cls = lilv_plugin_get_class(plugin);
         const LilvNode *ncuri = lilv_plugin_class_get_uri(cls);
         const char *clsname = lilv_node_as_uri(ncuri);
         std::cerr << "Plugin class: " << clsname << std::endl;
         bool isSynth = false;
         if(strcmp(clsname, LV2_INSTRUMENT_CLASS) == 0)
         {
            isSynth = true;
         }
         if(isSynth)
         {
            std::cerr << "Plugin is synth" << std::endl;
         }

#endif

#ifdef DEBUG_LV2
         std::cerr <<  "\tRequired features (by uri):" << std::endl;
#endif
         LilvNodes *fts = lilv_plugin_get_required_features(plugin);
         LilvIter *nit = lilv_nodes_begin(fts);

         while(true)
         {
            if(lilv_nodes_is_end(fts, nit))
            {
               break;
            }

            const LilvNode *fnode = lilv_nodes_get(fts, nit);
            const char *uri = lilv_node_as_uri(fnode);
            bool isSupported = (supportedFeatures.find(uri) != supportedFeatures.end());
#ifdef DEBUG_LV2
            std::cerr << "\t - " << uri << " (" << (isSupported ? "supported" : "not supported") << ")" << std::endl;
#endif

            if(!isSupported)
            {
               shouldLoad = false;
            }

            nit = lilv_nodes_next(fts, nit);

         }

         lilv_nodes_free(fts);



         //if (shouldLoad && isSynth)
         if(shouldLoad)   //load all plugins for now, not only synths
         {
            QFileInfo fi(lfp);
            QString name = QString(pluginName) + QString(" LV2");
            //QString label = QString(pluginUri) + QString("_LV2");
            // Make sure it doesn't already exist.
            std::vector<Synth *>::iterator is;

            for(is = MusEGlobal::synthis.begin(); is != MusEGlobal::synthis.end(); ++is)
            {
               Synth *s = *is;

               if(s->name() == name && s->baseName() == fi.completeBaseName())
               {
                  break;
               }
            }

            if(is == MusEGlobal::synthis.end())
            {
               LilvNode *nAuthor = lilv_plugin_get_author_name(plugin);
               QString author;

               if(nAuthor != NULL)
               {
                  author = lilv_node_as_string(nAuthor);
                  lilv_node_free(nAuthor);
               }

               LV2Synth *s = new LV2Synth(fi, name, name, author, plugin);

               if(s->isConstructed())
               {

                  if((s->isSynth() && s->outPorts() > 0)
                          || (s->inPorts() > 0 && s->outPorts() > 0))
                      //insert plugins with audio ins and outs to synths list too
                  {
                     MusEGlobal::synthis.push_back(s);
                  }
                  else
                  {
                     synthsToFree.push_back(s);
                  }

                  if(s->inPorts() > 0 && s->outPorts() > 0)   // insert to plugin list
                  {
                     MusEGlobal::plugins.push_back(new LV2PluginWrapper(s));

                  }
               }
               else
               {
                  delete s;
               }

            }
         }
      }

      if(nameNode != NULL)
      {
         lilv_node_free(nameNode);
      }

      pit = lilv_plugins_next(plugins, pit);
   }

}

void deinitLV2()
{

   for(size_t i = 0; i < synthsToFree.size(); i++)
   {
      delete synthsToFree [i];

   }

   for(LilvNode **n = (LilvNode **)&lv2CacheNodes; *n; ++n)
   {
      lilv_node_free(*n);
   }

   if(bLV2Gtk2Enabled && lv2Gtk2HelperHandle != NULL)
   {
      bLV2Gtk2Enabled = false;
      void (*lv2Gtk2Helper_deinitFn)();
      *(void **)(&lv2Gtk2Helper_deinitFn) = dlsym(lv2Gtk2HelperHandle, "lv2Gtk2Helper_deinit");
      lv2Gtk2Helper_deinitFn();
      dlclose(lv2Gtk2HelperHandle);
      lv2Gtk2HelperHandle = NULL;
   }

   free(lilvWorld);

}



void LV2Synth::lv2ui_ExtUi_Closed(LV2UI_Controller contr)
{
   LV2PluginWrapper_State *state = (LV2PluginWrapper_State *)contr;
   assert(state != NULL); //this shouldn't happen
   assert(state->widget != NULL); // this too
   assert(state->pluginWindow != NULL);

   state->pluginWindow->setClosing(true);


   //state->uiTimer->stopNextTime(false);
}

void LV2Synth::lv2ui_SendChangedControls(LV2PluginWrapper_State *state)
{
   if(state != NULL && state->uiDesc != NULL && state->uiDesc->port_event != NULL && state->uiInst != NULL)
   {      
      size_t numControls = 0;
      MusECore::Port *controls = NULL;
      size_t numControlsOut = 0;
      MusECore::Port *controlsOut = NULL;
      LV2Synth *synth = state->synth;

      if(state->plugInst != NULL)
      {
         numControls = state->plugInst->controlPorts;
         controls = state->plugInst->controls;
         numControlsOut = state->plugInst->controlOutPorts;
         controlsOut = state->plugInst->controlsOut;

      }
      else if(state->sif != NULL)
      {
         numControls = state->sif->_inportsControl;
         controls = state->sif->_controls;
         numControlsOut = state->sif->_outportsControl;
         controlsOut = state->sif->_controlsOut;
      }

      if(numControls > 0)
      {
         assert(controls != NULL);
      }

      if(numControlsOut > 0)
      {
         assert(controlsOut != NULL);
      }
      for(uint32_t i = 0; i < numControls; ++i)
      {
         if(state->controlTimers [i] > 0)
         {
            --state->controlTimers [i];
            continue;
         }
         if(state->controlsMask [i])
         {
            state->controlsMask [i] = false;

            if(state->lastControls [i] != controls [i].val)
            {
               state->lastControls [i] = controls [i].val;
               state->uiDesc->port_event(state->uiInst,
                                        controls [i].idx,
                                        sizeof(float), 0,
                                        &controls [i].val);
            }
         }
      }

      for(uint32_t i = 0; i < numControlsOut; ++i)
      {
         if(state->lastControlsOut [i] != controlsOut [i].val)
         {
            state->lastControlsOut [i] = controlsOut [i].val;
            state->uiDesc->port_event(state->uiInst,
                                     controlsOut [i].idx,
                                     sizeof(float), 0,
                                     &controlsOut [i].val);
         }

      }

      //process gui atom events (control events are already set by getData or apply.
      size_t fifoItemSize = state->plugControlEvt.getItemSize();
      size_t dataSize = 0;
      uint32_t port_index = 0;
      char evtBuffer [fifoItemSize];
      while(state->plugControlEvt.get(&port_index, &dataSize, evtBuffer))
      {
         state->uiDesc->port_event(state->uiInst, port_index, dataSize, synth->_uAtom_EventTransfer, evtBuffer);
      }
   }
}



void LV2Synth::lv2ui_PortWrite(LV2UI_Controller controller, uint32_t port_index, uint32_t buffer_size, uint32_t protocol, void const *buffer)
{
   LV2Synth::lv2state_PortWrite(controller, port_index, buffer_size, protocol, buffer, true);
}

void LV2Synth::lv2ui_Touch(LV2UI_Controller /*controller*/, uint32_t /*port_index*/, bool grabbed __attribute__ ((unused)))
{
#ifdef DEBUG_LV2
   std::cerr << "LV2Synth::lv2ui_UiTouch: port: %u " << (grabbed ? "grabbed" : "released") << std::endl;
#endif

}





void LV2Synth::lv2state_FillFeatures(LV2PluginWrapper_State *state)
{
   uint32_t i;
   LV2Synth *synth = state->synth;
   LV2_Feature *_ifeatures = state->_ifeatures;
   LV2_Feature **_ppifeatures = state->_ppifeatures;

   //state->uiTimer = new LV2PluginWrapper_Timer(state);

   state->wrkSched.handle = (LV2_Worker_Schedule_Handle)state;
   state->wrkSched.schedule_work = LV2Synth::lv2wrk_scheduleWork;
   state->wrkIface = NULL;
   state->wrkThread = new LV2PluginWrapper_Worker(state);

   state->extHost.plugin_human_id = state->human_id = NULL;
   state->extHost.ui_closed = LV2Synth::lv2ui_ExtUi_Closed;

   state->extData.data_access = NULL;

   for(i = 0; i < SIZEOF_ARRAY(lv2Features); i++)
   {
      _ifeatures [i] = synth->_features [i];

      if(_ifeatures [i].URI == NULL)
      {
         break;
      }

      if(i == synth->_fInstanceAccess)
      {
         _ifeatures [i].data = NULL;
      }
      else if(i == synth->_fExtUiHost)
      {
         _ifeatures [i].data = &state->extHost;
      }
      else if(i == synth->_fExtUiHostD)
      {
         _ifeatures [i].data = &state->extHost;
      }
      else if(i == synth->_fDataAccess)
      {
         _ifeatures [i].data = &state->extData;
      }
      else if(i == synth->_fWrkSchedule)
      {
         _ifeatures [i].data = &state->wrkSched;
      }
      else if(i == synth->_fUiResize)
      {
         _ifeatures [i].data = &state->uiResize;
      }
      else if(i == synth->_fPrgHost)
      {
         _ifeatures [i].data = &state->prgHost;
      }
      else if(i == synth->_fMakePath)
      {
         _ifeatures [i].data = &state->makePath;
      }
      else if(i == synth->_fMapPath)
      {
         _ifeatures [i].data = &state->mapPath;
      }

      _ppifeatures [i] = &_ifeatures [i];
   }

   _ppifeatures [i] = NULL;

   state->curBpm = 0.0;//double(60000000.0/MusEGlobal::tempomap.tempo(MusEGlobal::song->cpos()));
   state->curIsPlaying = MusEGlobal::audio->isPlaying();
   state->curFrame = MusEGlobal::audioDevice->getCurFrame();
   lv2_atom_forge_init(&state->atomForge, &synth->_lv2_urid_map);

   if(snd_midi_event_new(MusEGlobal::segmentSize * 10, &state->midiEvent) != 0)
   {
      abort();
   }
   snd_midi_event_no_status(state->midiEvent, 1);

   LV2Synth::lv2state_InitMidiPorts(state);
}

void LV2Synth::lv2state_PostInstantiate(LV2PluginWrapper_State *state)
{
   LV2Synth *synth = state->synth;
   const LV2_Descriptor *descr = lilv_instance_get_descriptor(state->handle);

   state->_ifeatures [synth->_fInstanceAccess].data = lilv_instance_get_handle(state->handle);

   if(descr->extension_data != NULL)
   {
      state->extData.data_access = descr->extension_data;
   }
   else
   {
      state->_ppifeatures [synth->_fDataAccess] = NULL;
   }

   //fill pointers for CV port types;

   uint32_t numAllPorts = lilv_plugin_get_num_ports(synth->_handle);

   state->pluginCVPorts = new float *[numAllPorts];
   int rv = posix_memalign((void **)&state->pluginCVPorts, 16, sizeof(float *) * numAllPorts);
   if(rv != 0)
   {
      fprintf(stderr, "ERROR: LV2Synth::lv2state_PostInstantiate: posix_memalign returned error:%d. Aborting!\n", rv);
      abort();
   }

   memset(state->pluginCVPorts, 0, sizeof(float *) * numAllPorts);

   for(size_t i = 0; i < synth->_controlInPorts.size(); ++i)
   {
      if(synth->_controlInPorts [i].isCVPort)
      {
         size_t idx = synth->_controlInPorts [i].index;
         rv = posix_memalign((void **)&state->pluginCVPorts [idx], 16, sizeof(float) * MusEGlobal::segmentSize);

         if(rv != 0)
         {
            fprintf(stderr, "ERROR: LV2Synth::lv2state_PostInstantiate: posix_memalign returned error:%d. Aborting!\n", rv);
            abort();
         }
         for(size_t k = 0; k < MusEGlobal::segmentSize; ++k)
         {
            state->pluginCVPorts [idx] [k] = synth->_controlInPorts [i].defVal;
         }
         lilv_instance_connect_port(state->handle, idx, state->pluginCVPorts [idx]);
      }
   }

   for(size_t i = 0; i < synth->_controlOutPorts.size(); ++i)
   {
      if(synth->_controlOutPorts [i].isCVPort)
      {
         size_t idx = synth->_controlOutPorts [i].index;
         rv = posix_memalign((void **)&state->pluginCVPorts [idx], 16, sizeof(float) * MusEGlobal::segmentSize);

         if(rv != 0)
         {
            fprintf(stderr, "ERROR: LV2Synth::lv2state_PostInstantiate: posix_memalign returned error:%d. Aborting!\n", rv);
            abort();
         }
         for(size_t k = 0; k < MusEGlobal::segmentSize; ++k)
         {
            state->pluginCVPorts [idx] [k] = synth->_controlOutPorts [i].defVal;
         }
         lilv_instance_connect_port(state->handle, idx, state->pluginCVPorts [idx]);
      }
   }

   for(size_t i = 0; i < state->midiInPorts.size(); i++)
   {
      lilv_instance_connect_port(state->handle, state->midiInPorts [i].index, (void *)state->midiInPorts [i].buffer->lv2_evbuf_get_buffer());
   }

   for(size_t i = 0; i < state->midiOutPorts.size(); i++)
   {
     lilv_instance_connect_port(state->handle, state->midiOutPorts [i].index, (void *)state->midiOutPorts [i].buffer->lv2_evbuf_get_buffer());
   }

   //query for state interface
   state->iState = (LV2_State_Interface *)lilv_instance_get_extension_data(state->handle, LV2_STATE__interface);
   //query for LV2Worker interface
   state->wrkIface = (LV2_Worker_Interface *)lilv_instance_get_extension_data(state->handle, LV2_F_WORKER_INTERFACE);
   //query for programs interface   
   state->prgIface = (LV2_Programs_Interface *)lilv_instance_get_extension_data(state->handle, LV2_PROGRAMSNEW__Interface);
   if(state->prgIface != NULL)
   {
      state->newPrgIface = true;
   }
   else
   {
      state->newPrgIface = false;
      state->prgIface = (LV2_Programs_Interface *)lilv_instance_get_extension_data(state->handle, LV2_PROGRAMS__Interface);
   }

   LV2Synth::lv2prg_updatePrograms(state);

   state->wrkThread->start(QThread::LowPriority);

}

void LV2Synth::lv2ui_FreeDescriptors(LV2PluginWrapper_State *state)
{

   if(state->uiDesc != NULL && state->uiInst != NULL)
      state->uiDesc->cleanup(state->uiInst);

   state->uiInst = *(void **)(&state->uiDesc) = NULL;

   if(bLV2Gtk2Enabled && state->gtk2Plug != NULL)
   {
      void (*lv2Gtk2Helper_gtk_widget_destroyFn)(void *);
      *(void **)(&lv2Gtk2Helper_gtk_widget_destroyFn) = dlsym(lv2Gtk2HelperHandle, "lv2Gtk2Helper_gtk_widget_destroy");
      lv2Gtk2Helper_gtk_widget_destroyFn(state->gtk2Plug);
      state->gtk2Plug = NULL;
   }

   if(state->uiDlHandle != NULL)
   {
      dlclose(state->uiDlHandle);
      state->uiDlHandle = NULL;
   }

}



void LV2Synth::lv2state_FreeState(LV2PluginWrapper_State *state)
{
   assert(state != NULL);

   state->wrkThread->setClosing();
   state->wrkThread->wait();
   delete state->wrkThread;

   if(state->human_id != NULL)
      free(state->human_id);
   if(state->lastControls)
   {
      delete [] state->lastControls;
      state->lastControls = NULL;
   }
   if(state->controlsMask)
   {
      delete [] state->controlsMask;
      state->controlsMask = NULL;
   }

   if(state->controlTimers)
   {
      delete [] state->controlTimers;
      state->controlTimers = NULL;

   }

   if(state->lastControlsOut)
   {
      delete [] state->lastControlsOut;
      state->lastControlsOut = NULL;
   }

   LV2Synth::lv2ui_FreeDescriptors(state);

   if(state->handle != NULL)
   {
      lilv_instance_free(state->handle);
      state->handle = NULL;
   }

   if(state->midiEvent != NULL)
   {
      snd_midi_event_free(state->midiEvent);
      state->midiEvent = NULL;
   }

   delete state;
}

void LV2Synth::lv2audio_SendTransport(LV2PluginWrapper_State *state, LV2EvBuf *buffer, LV2EvBuf::LV2_Evbuf_Iterator &iter)
{
   //send transport events if any
   LV2Synth *synth = state->synth;
   unsigned int cur_frame = MusEGlobal::audio->pos().frame();
   Pos p(MusEGlobal::extSyncFlag.value() ? MusEGlobal::audio->tickPos() : cur_frame, MusEGlobal::extSyncFlag.value() ? true : false);
   double curBpm = (60000000.0 / MusEGlobal::tempomap.tempo(p.tick())) * double(MusEGlobal::tempomap.globalTempo())/100.0;
   bool curIsPlaying = MusEGlobal::audio->isPlaying();
   unsigned int curFrame = MusEGlobal::audioDevice->getCurFrame();
   //   if(state->curFrame != curFrame
   //      || state->curIsPlaying != curIsPlaying
   //      || state->curBpm != curBpm)
   //   {

   //send transport/tempo changes always
   //as some plugins revert to default settings when not playing
   state->curFrame = curFrame;
   state->curIsPlaying = curIsPlaying;
   state->curBpm = curBpm;
   uint8_t   pos_buf[1024];
   memset(pos_buf, 0, sizeof(pos_buf));
   LV2_Atom* lv2_pos = (LV2_Atom*)pos_buf;
   /* Build an LV2 position object to report change to plugin */
   LV2_Atom_Forge* atomForge = &state->atomForge;
#ifdef LV2_NEW_LIB
   lv2_atom_forge_set_buffer(atomForge, pos_buf, sizeof(pos_buf));
   LV2_Atom_Forge_Frame frame;
   lv2_atom_forge_object(atomForge, &frame, 1, synth->_uTime_Position);
   lv2_atom_forge_key(atomForge, synth->_uTime_frame);
   lv2_atom_forge_long(atomForge, curFrame);
   lv2_atom_forge_key(atomForge, synth->_uTime_speed);
   lv2_atom_forge_float(atomForge, curIsPlaying ? 1.0 : 0.0);
   lv2_atom_forge_key(atomForge, synth->_uTime_beatsPerMinute);
   lv2_atom_forge_float(atomForge, (float)curBpm);
#else
   lv2_atom_forge_set_buffer(atomForge, pos_buf, sizeof(pos_buf));
   LV2_Atom_Forge_Frame frame;
   lv2_atom_forge_blank(atomForge, &frame, 1, synth->_uTime_Position);
   lv2_atom_forge_property_head(atomForge, synth->_uTime_frame, 0);
   lv2_atom_forge_long(atomForge, curFrame);
   lv2_atom_forge_property_head(atomForge, synth->_uTime_speed, 0);
   lv2_atom_forge_float(atomForge, curIsPlaying ? 1.0 : 0.0);
   lv2_atom_forge_property_head(atomForge, synth->_uTime_beatsPerMinute, 0);
   lv2_atom_forge_float(atomForge, (float)curBpm);
#endif
buffer->lv2_evbuf_write(iter, 0, 0, lv2_pos->type, lv2_pos->size, (const uint8_t *)LV2_ATOM_BODY(lv2_pos));
//   }
}

void LV2Synth::lv2state_InitMidiPorts(LV2PluginWrapper_State *state)
{
   LV2Synth *synth = state->synth;
   state->midiInPorts = synth->_midiInPorts;
   state->midiOutPorts = synth->_midiOutPorts;
   //connect midi and control ports
   for(size_t i = 0; i < state->midiInPorts.size(); i++)
   {
      LV2EvBuf *buffer = new LV2EvBuf(LV2_RT_FIFO_ITEM_SIZE,
                                      state->midiInPorts [i].old_api ? LV2EvBuf::LV2_EVBUF_EVENT : LV2EvBuf::LV2_EVBUF_ATOM,
                                      synth->mapUrid(LV2_ATOM__Chunk),
                                      synth->mapUrid(LV2_ATOM__Sequence)
                                     );
      state->midiInPorts [i].buffer = buffer;
      state->idx2EvtPorts.insert(std::make_pair<uint32_t, LV2EvBuf *>(state->midiInPorts [i].index, buffer));
   }

   for(size_t i = 0; i < state->midiOutPorts.size(); i++)
   {
      LV2EvBuf *buffer = new LV2EvBuf(LV2_RT_FIFO_ITEM_SIZE,
                                      state->midiOutPorts [i].old_api ? LV2EvBuf::LV2_EVBUF_EVENT : LV2EvBuf::LV2_EVBUF_ATOM,
                                      synth->mapUrid(LV2_ATOM__Chunk),
                                      synth->mapUrid(LV2_ATOM__Sequence)
                                     );
      state->midiOutPorts [i].buffer = buffer;
      state->idx2EvtPorts.insert(std::make_pair<uint32_t, LV2EvBuf *>(state->midiOutPorts [i].index, buffer));
   }

}

void LV2Synth::lv2audio_preProcessMidiPorts(LV2PluginWrapper_State *state, unsigned long nsamp, const snd_seq_event_t *events, unsigned long nevents )
{
   LV2Synth *synth = state->synth;
   size_t inp = state->midiInPorts.size();
   size_t outp = state->midiOutPorts.size();
   for(size_t j = 0; j < inp; j++)
   {
      state->midiInPorts [j].buffer->lv2_evbuf_reset(true);
   }

   for(size_t j = 0; j < outp; j++)
   {
      state->midiOutPorts [j].buffer->lv2_evbuf_reset(false);
   }

   if(inp > 0)
   {
      LV2EvBuf *rawMidiBuffer = state->midiInPorts [0].buffer;
      LV2EvBuf::LV2_Evbuf_Iterator iter = rawMidiBuffer->lv2_evbuf_begin();

      if(state->midiInPorts [0].supportsTimePos)
      {
         //send transport events if any
         LV2Synth::lv2audio_SendTransport(state, rawMidiBuffer, iter);
      }

      if(events != NULL)
      {

         //convert snd_seq_event_t[] to raw midi data
         snd_midi_event_reset_decode(state->midiEvent);
         uint8_t resMidi [1024];

         for(unsigned long i = 0; i < nevents; i++)
         {
            uint32_t stamp = events[i].time.tick;
            uint32_t size = snd_midi_event_decode(state->midiEvent, resMidi, sizeof(resMidi), &events [i]);
            rawMidiBuffer->lv2_evbuf_write(iter,
                                           stamp,
                                           0,
                                           synth->_midi_event_id,
                                           size, resMidi);


         }
      }
   }

   //process gui atom events (control events are already set by getData or apply call).
   size_t fifoItemSize = state->uiControlEvt.getItemSize();
   size_t dataSize = 0;
   uint32_t port_index = 0;
   char evtBuffer [fifoItemSize];
   while(state->uiControlEvt.get(&port_index, &dataSize, evtBuffer))
   {
      std::map<uint32_t, LV2EvBuf *>::iterator it = state->idx2EvtPorts.find(port_index);
      if(it != state->idx2EvtPorts.end())
      {
         LV2EvBuf *buffer = it->second;
         LV2EvBuf::LV2_Evbuf_Iterator iter = buffer->lv2_evbuf_end();
         const LV2_Atom* const atom = (const LV2_Atom*)evtBuffer;
         buffer->lv2_evbuf_write(iter, nsamp, 0, atom->type, atom->size,
                                 static_cast<const uint8_t *>(LV2_ATOM_BODY_CONST(atom)));


      }

   }


}

void LV2Synth::lv2audio_postProcessMidiPorts(LV2PluginWrapper_State *state, unsigned long)
{
   //send Atom events to gui.
   //Synchronize send rate with gui update rate
   if(state->uiInst == NULL)
      return;

   size_t fifoItemSize = state->plugControlEvt.getItemSize();

   size_t outp = state->midiOutPorts.size();

   for(size_t j = 0; j < outp; j++)
   {
      if(!state->midiOutPorts [j].old_api)
      {
         LV2EvBuf::LV2_Evbuf_Iterator it = state->midiOutPorts [j].buffer->lv2_evbuf_begin();

         for(; it.lv2_evbuf_is_valid(); ++it)
         {
            uint32_t frames, subframes, type, size;
            uint8_t *data = NULL;
            state->midiOutPorts [j].buffer->lv2_evbuf_get(it, &frames, &subframes, &type, &size, &data);
            unsigned char atom_data [fifoItemSize];
            LV2_Atom *atom_evt = reinterpret_cast<LV2_Atom *>(atom_data);
            atom_evt->type = type;
            atom_evt->size = size;
            if(fifoItemSize - sizeof(LV2_Atom) < size)
            {
#ifdef DEBUG_LV2
               std::cerr << "LV2Synth::lv2audio_postProcessMidiPorts(): Plugin event data is bigger than rt fifo item size. Skipping." << std::endl;
#endif
               continue;
            }
            unsigned char *evt = static_cast<unsigned char *>(LV2_ATOM_BODY(atom_evt));
            memcpy(evt, data, size);

            state->plugControlEvt.put(state->midiOutPorts [j].index, sizeof(LV2_Atom) + size, atom_evt);
         }

      }
   }

}

void LV2Synth::lv2ui_PostShow(LV2PluginWrapper_State *state)
{
   assert(state->pluginWindow != NULL);
   assert(state->uiDesc != NULL);   
   assert(state->uiInst != NULL);

   if(state->uiDesc->port_event != NULL)
   {
      uint32_t numControls = 0;
      Port *controls = NULL;

      if(state->plugInst != NULL)
      {
         numControls = state->plugInst->controlPorts;
         controls = state->plugInst->controls;

      }
      else if(state->sif != NULL)
      {
         numControls = state->sif->_inportsControl;
         controls = state->sif->_controls;
      }

      if(numControls > 0)
      {
         assert(controls != NULL);
      }



      for(uint32_t i = 0; i < numControls; ++i)
      {
         state->uiDesc->port_event(state->uiInst,
                                   controls [i].idx,
                                   sizeof(float), 0,
                                   &controls [i].val);
      }

   }

   state->pluginWindow->startNextTime();

}

int LV2Synth::lv2ui_Resize(LV2UI_Feature_Handle handle, int width, int height)
{
   LV2PluginWrapper_State *state = (LV2PluginWrapper_State *)handle;
   if(state->widget != NULL && state->hasGui)
   {
      ((LV2PluginWrapper_Window *)state->widget)->resize(width, height);
      QWidget *ewWin = ((LV2PluginWrapper_Window *)state->widget)->findChild<QWidget *>();
      if(ewWin != NULL)
      {
         ewWin->resize(width, height);
      }
      else
      {
         QWidget *ewCent= ((LV2PluginWrapper_Window *)state->widget)->centralWidget();
         if(ewCent != NULL)
         {
            ewCent->resize(width, height);
         }
      }
      state->uiX11Size.setWidth(width);
      state->uiX11Size.setHeight(height);
      return 0;
   }
   return 1;
}

void LV2Synth::lv2ui_Gtk2AllocateCb(int width, int height, void *arg)
{
   LV2PluginWrapper_State *state = (LV2PluginWrapper_State *)arg;
   if(state == NULL)
      return;
   if(state->widget != NULL && state->hasGui && state->gtk2Plug != NULL)
   {
      ((LV2PluginWrapper_Window *)state->widget)->setMinimumSize(width, height);
      ((LV2PluginWrapper_Window *)state->widget)->setMaximumSize(width, height);
   }


}

void LV2Synth::lv2ui_Gtk2ResizeCb(int width, int height, void *arg)
{
   LV2PluginWrapper_State *state = (LV2PluginWrapper_State *)arg;
   if(state == NULL)
      return;
   if(state->widget != NULL && state->hasGui && state->gtk2Plug != NULL)
   {
      ((LV2PluginWrapper_Window *)state->widget)->resize(width, height);
   }

}



void LV2Synth::lv2ui_ShowNativeGui(LV2PluginWrapper_State *state, bool bShow)
{
   LV2Synth *synth = state->synth;
   LV2PluginWrapper_Window *win = NULL;

   if(synth->_pluginUiTypes.size() == 0)
      return;

   //state->uiTimer->stopNextTime();
   if(state->pluginWindow != NULL)
      state->pluginWindow->stopNextTime();

   if(!bShow)
      return;

   LV2_PLUGIN_UI_TYPES::iterator itUi;

   if(state->uiCurrent == NULL)
   {
      QAction *aUiTypeSelected = NULL;
      if(synth->_pluginUiTypes.size() == 1)
      {
         state->uiCurrent = synth->_pluginUiTypes.begin()->first;
      }
      else
      {
         QMenu mGuisPopup;
         QAction *aUiTypeHeader = new QAction(QMenu::tr("Select gui type"), NULL);
         aUiTypeHeader->setEnabled(false);
         QFont fHeader;
         fHeader.setBold(true);
         fHeader.setUnderline(true);
         aUiTypeHeader->setFont(fHeader);
         mGuisPopup.addAction(aUiTypeHeader);

         for(itUi = synth->_pluginUiTypes.begin(); itUi != synth->_pluginUiTypes.end(); itUi++)
         {
            const LilvUI *selectedUi = itUi->first;
            const LilvNode *pluginUiType = itUi->second.second;
            QAction *aUiType = new QAction(QString(lilv_node_as_string(pluginUiType)), NULL);
            aUiType->setData(QVariant(reinterpret_cast<qlonglong>(selectedUi)));
            mGuisPopup.addAction(aUiType);
         }

         aUiTypeSelected = mGuisPopup.exec(QCursor::pos());
         if(aUiTypeSelected == NULL)
         {
            return;
         }
         state->uiCurrent = reinterpret_cast<const LilvUI *>(aUiTypeSelected->data().toLongLong());
      }

   }

   itUi = synth->_pluginUiTypes.find(state->uiCurrent);

   assert(itUi != synth->_pluginUiTypes.end());


   const LilvUI *selectedUi = itUi->first;
   bool bExtUi = itUi->second.first;
   const LilvNode *pluginUiType = itUi->second.second;
   state->uiIdleIface = NULL;
   if(bExtUi)
   {
      state->hasGui = false;
      state->hasExternalGui = true;
   }
   else
   {
      state->hasGui = true;
      state->hasExternalGui = false;
   }

   win = new LV2PluginWrapper_Window(state);
   state->uiX11Size.setWidth(0);
   state->uiX11Size.setHeight(0);

   if(win != NULL)
   {
      state->widget = win;
      state->pluginWindow = win;
      const char *cUiUri = lilv_node_as_uri(pluginUiType);
      const char *cUiTypeUri = lilv_node_as_uri(lilv_ui_get_uri(selectedUi));
      bool bEmbed = false;
      bool bGtk = false;
      QWidget *ewWin = NULL;
      QWindow *x11QtWindow = NULL;
      state->gtk2Plug = NULL;
      state->_ifeatures [synth->_fUiParent].data = NULL;
      if(strcmp(LV2_UI__X11UI, cUiUri) == 0)
      {
         bEmbed = true;         
         ewWin = new QWidget();
         x11QtWindow = QWindow::fromWinId(ewWin->winId());
         ewWin = win->createWindowContainer(x11QtWindow, win);
         win->setCentralWidget(ewWin);
         //(static_cast<QX11EmbedWidget *>(ewWin))->embedInto(win->winId());
         //(static_cast<QX11EmbedWidget *>(ewWin))->setParent(win);
         state->_ifeatures [synth->_fUiParent].data = (void*)(intptr_t)x11QtWindow->winId();

      }
      else if(bLV2Gtk2Enabled && strcmp(LV2_UI__GtkUI, cUiUri) == 0)
      {
         bEmbed = true;
         bGtk = true;
         //ewWin = new QWidget();

         //ewWin = new QX11EmbedContainer(win);
         //win->setCentralWidget(static_cast<QX11EmbedContainer *>(ewWin));
         void *( *lv2Gtk2Helper_gtk_plug_newFn)(unsigned long, void*);
         *(void **)(&lv2Gtk2Helper_gtk_plug_newFn) = dlsym(lv2Gtk2HelperHandle, "lv2Gtk2Helper_gtk_plug_new");
         state->gtk2Plug = lv2Gtk2Helper_gtk_plug_newFn(0, state);
         //state->_ifeatures [synth->_fUiParent].data = NULL;//(void *)ewWin;


         void ( *lv2Gtk2Helper_register_allocate_cbFn)(void *, void(*sz_cb_fn)(int, int, void *));
         *(void **)(&lv2Gtk2Helper_register_allocate_cbFn) = dlsym(lv2Gtk2HelperHandle, "lv2Gtk2Helper_register_allocate_cb");
         lv2Gtk2Helper_register_allocate_cbFn(static_cast<void *>(state->gtk2Plug), lv2ui_Gtk2AllocateCb);

         void ( *lv2Gtk2Helper_register_resize_cbFn)(void *, void(*sz_cb_fn)(int, int, void *));
         *(void **)(&lv2Gtk2Helper_register_resize_cbFn) = dlsym(lv2Gtk2HelperHandle, "lv2Gtk2Helper_register_resize_cb");
         lv2Gtk2Helper_register_resize_cbFn(static_cast<void *>(state->gtk2Plug), lv2ui_Gtk2ResizeCb);

      }
      else if(strcmp(LV2_F_UI_Qt5_UI, cUiUri) == 0) //Qt5 uis are handled natively
      {
         state->_ifeatures [synth->_fUiParent].data = win;
      }      
      else //external uis
      {
         state->_ifeatures [synth->_fUiParent].data = NULL;
      }

      //now open ui library file

      const  char *uiPath = lilv_uri_to_path(lilv_node_as_uri(lilv_ui_get_binary_uri(selectedUi)));
      state->uiDlHandle = dlopen(uiPath, RTLD_NOW);
      if(state->uiDlHandle == NULL)
      {
         win->stopNextTime();
         return;
      }

      //find lv2 ui descriptor function and call it to get ui descriptor struct
      LV2UI_DescriptorFunction lv2fUiDesc;
      *(void **)(&lv2fUiDesc) = dlsym(state->uiDlHandle, "lv2ui_descriptor");
      if(lv2fUiDesc == NULL)
      {
         win->stopNextTime();
         return;
      }

      state->uiDesc = NULL;

      for(uint32_t i = 0; ;++i)
      {
         state->uiDesc = lv2fUiDesc(i);
         if(state->uiDesc == NULL)
            break;

         if(strcmp(state->uiDesc->URI, cUiTypeUri) == 0) //found selected ui
            break;
      }

      if(state->uiDesc == NULL)
      {
         win->stopNextTime();
         return;
      }

      /*
      state->uiInst = suil_instance_new(state->uiHost,
                                        state,
                                        lilv_node_as_uri(lv2CacheNodes.host_uiType),
                                        lilv_node_as_uri(lilv_plugin_get_uri(synth->_handle)),
                                        lilv_node_as_uri(lilv_ui_get_uri(selectedUi)),
                                        cUiUriNew,
                                        lilv_uri_to_path(lilv_node_as_uri(lilv_ui_get_bundle_uri(selectedUi))),
                                        lilv_uri_to_path(lilv_node_as_uri(lilv_ui_get_binary_uri(selectedUi))),
                                        state->_ppifeatures);
                                        */
      void *uiW = NULL;
      state->uiInst = state->uiDesc->instantiate(state->uiDesc,
                                                 lilv_node_as_uri(lilv_plugin_get_uri(synth->_handle)),
                                                 lilv_uri_to_path(lilv_node_as_uri(lilv_ui_get_bundle_uri(selectedUi))),
                                                 LV2Synth::lv2ui_PortWrite,
                                                 state,
                                                 &uiW,
                                                 state->_ppifeatures);


      if(state->uiInst != NULL)
      {
         state->uiIdleIface = NULL;
         state->uiPrgIface = NULL;
         if(state->uiDesc->extension_data != NULL)
         {
            state->uiIdleIface = (LV2UI_Idle_Interface *)state->uiDesc->extension_data(LV2_F_UI_IDLE);
            state->uiPrgIface = (LV2_Programs_UI_Interface *)state->uiDesc->extension_data(LV2_PROGRAMSNEW__UIInterface);
            if(state->uiPrgIface != NULL)
            {
               state->newPrgIface = true;
            }
            else
            {
               state->newPrgIface = false;
               state->uiPrgIface = (LV2_Programs_UI_Interface *)state->uiDesc->extension_data(LV2_PROGRAMS__UIInterface);
            }
         }
         if(state->hasGui)
         {            
            if(!bEmbed)
            {
               win->setCentralWidget(static_cast<QWidget *>(uiW));
            }
            else
            {               
               if(bGtk)
               {
                  void ( *lv2Gtk2Helper_gtk_container_addFn)(void *, void *);
                  *(void **)(&lv2Gtk2Helper_gtk_container_addFn) = dlsym(lv2Gtk2HelperHandle, "lv2Gtk2Helper_gtk_container_add");

                  void ( *lv2Gtk2Helper_gtk_widget_show_allFn)(void *);
                  *(void **)(&lv2Gtk2Helper_gtk_widget_show_allFn) = dlsym(lv2Gtk2HelperHandle, "lv2Gtk2Helper_gtk_widget_show_all");

                  void ( *lv2Gtk2Helper_gtk_widget_get_allocationFn)(void *, int *, int *);
                  *(void **)(&lv2Gtk2Helper_gtk_widget_get_allocationFn) = dlsym(lv2Gtk2HelperHandle, "lv2Gtk2Helper_gtk_widget_get_allocation");

                  lv2Gtk2Helper_gtk_container_addFn(state->gtk2Plug, uiW);
                  lv2Gtk2Helper_gtk_widget_show_allFn(state->gtk2Plug);

                  unsigned long ( *lv2Gtk2Helper_gdk_x11_drawable_get_xidFn)(void*);
                  *(void **)(&lv2Gtk2Helper_gdk_x11_drawable_get_xidFn) = dlsym(lv2Gtk2HelperHandle, "lv2Gtk2Helper_gdk_x11_drawable_get_xid");
                  unsigned long plugX11Id = lv2Gtk2Helper_gdk_x11_drawable_get_xidFn(state->gtk2Plug);
                  x11QtWindow = QWindow::fromWinId(plugX11Id);
                  ewWin = QWidget::createWindowContainer(x11QtWindow);
                  win->setCentralWidget(ewWin);

                  if(state->uiX11Size.width() == 0 || state->uiX11Size.height() == 0)
                  {
                     int w = 0;
                     int h = 0;
                     lv2Gtk2Helper_gtk_widget_get_allocationFn(uiW, &w, &h);
                     win->resize(w, h);
                  }
                  //win->setCentralWidget(static_cast<QX11EmbedContainer *>(ewWin));
               }
               else
               {
                  //(static_cast<QX11EmbedWidget *>(ewWin))->embedInto(win->winId());
                  if(state->uiX11Size.width() == 0 || state->uiX11Size.height() == 0)
                     win->resize(ewWin->size());
               }
            }

            win->show();
            win->setWindowTitle(state->extHost.plugin_human_id);
         }
         else if(state->hasExternalGui)
         {
            state->widget = uiW;
            LV2_EXTERNAL_UI_SHOW((LV2_External_UI_Widget *)state->widget);
         }

         LV2Synth::lv2ui_PostShow(state);
         return;

      }

   }
   if(win != NULL)
   {
      win->stopNextTime();
   }
   state->pluginWindow = NULL;
   state->widget = NULL;
   state->uiCurrent = NULL;

   //no ui is shown
   state->hasExternalGui = state->hasGui = false;

}


const void *LV2Synth::lv2state_stateRetreive(LV2_State_Handle handle, uint32_t key, size_t *size, uint32_t *type, uint32_t *flags)
{
   QMap<QString, QPair<QString, QVariant> >::const_iterator it;
   LV2PluginWrapper_State *state = (LV2PluginWrapper_State *)handle;
   LV2Synth *synth = state->synth;
   const char *cKey = synth->unmapUrid(key);

   assert(cKey != NULL); //should'n happen

   QString strKey = QString(cKey);
   it = state->iStateValues.find(strKey);
   if(it != state->iStateValues.end())
   {
      if(it.value().second.type() == QVariant::ByteArray)
      {
         QString sType = it.value().first;         
         QByteArray arrType = sType.toUtf8();
         *type = synth->mapUrid(arrType.constData());
         *flags = LV2_STATE_IS_POD;
         QByteArray valArr = it.value().second.toByteArray();
         if(sType.compare(QString(LV2_ATOM__Path)) == 0) //prepend project folder to abstract path
         {
            QString plugFolder = ((state->sif != NULL) ? state->sif->name() : state->plugInst->name()) + QString("/");
            QString strPath = QString::fromUtf8(valArr.data());
            QFile ff(strPath);
            QFileInfo fiPath(ff);
            if(fiPath.isRelative())
            {
               if(strPath.indexOf(plugFolder) < 0)
               {
                  strPath = plugFolder + strPath;
               }
               strPath = MusEGlobal::museProject + QString("/") + strPath;
               valArr = strPath.toUtf8();
               int len = strPath.length();
               valArr.setRawData(strPath.toUtf8().constData(), len + 1);
               valArr [len] = 0;
            }
         }
         size_t i;
         size_t numValues = state->numStateValues;
         for(i = 0; i < numValues; ++i)
         {
            if(state->tmpValues [i] == NULL)
               break;
         }
         assert(i < numValues); //sanity check
         size_t sz = valArr.size();
         state->iStateValues.remove(strKey);
         if(sz > 0)
         {
            state->tmpValues [i] = new char [sz];
            memset(state->tmpValues [i], 0, sz);
            memcpy(state->tmpValues [i], valArr.constData(), sz);
            *size = sz;
            return state->tmpValues [i];
         }
      }
   }

   return NULL;
}

LV2_State_Status LV2Synth::lv2state_stateStore(LV2_State_Handle handle, uint32_t key, const void *value, size_t size, uint32_t type, uint32_t flags)
{
   if(flags & (LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE))
   {
      LV2PluginWrapper_State *state = (LV2PluginWrapper_State *)handle;
      LV2Synth *synth = state->synth;
      const char *uriKey = synth->unmapUrid(key);
      const char *uriType = synth->unmapUrid(type);
      assert(uriType != NULL && uriKey != NULL); //FIXME: buggy plugin or uridBiMap realization?
      QString strKey = QString(uriKey);
      QMap<QString, QPair<QString, QVariant> >::const_iterator it = state->iStateValues.find(strKey);
      if(it == state->iStateValues.end())
      {
         QString strUriType = uriType;
         QVariant varVal = QByteArray((const char *)value, size);
         state->iStateValues.insert(strKey, QPair<QString, QVariant>(strUriType, varVal));
      }
      return LV2_STATE_SUCCESS;
   }
   return LV2_STATE_ERR_BAD_FLAGS;
}

LV2_Worker_Status LV2Synth::lv2wrk_scheduleWork(LV2_Worker_Schedule_Handle handle, uint32_t size, const void *data)
{
#ifdef DEBUG_LV2
   std::cerr << "LV2Synth::lv2wrk_scheduleWork" << std::endl;
#endif
   LV2PluginWrapper_State *state = (LV2PluginWrapper_State *)handle;

   //assert(state->wrkEndWork != true);
   if(state->wrkEndWork != false)
      return LV2_WORKER_ERR_UNKNOWN;

   state->wrkDataSize = size;
   state->wrkDataBuffer = data;
   if(MusEGlobal::audio->freewheel()) //dont wait for a thread. Do it now
      state->wrkThread->makeWork();
   else
      return state->wrkThread->scheduleWork();

   return LV2_WORKER_SUCCESS;
}

LV2_Worker_Status LV2Synth::lv2wrk_respond(LV2_Worker_Respond_Handle handle, uint32_t size, const void *data)
{
   LV2PluginWrapper_State *state = (LV2PluginWrapper_State *)handle;

   state->wrkDataSize = size;
   state->wrkDataBuffer = data;
   state->wrkEndWork = true;

   return LV2_WORKER_SUCCESS;
}

void LV2Synth::lv2conf_write(LV2PluginWrapper_State *state, int level, Xml &xml)
{
   state->iStateValues.clear();
   state->numStateValues = 0;

   if(state->iState != NULL)
   {
      state->iState->save(lilv_instance_get_handle(state->handle), LV2Synth::lv2state_stateStore, state, LV2_STATE_IS_POD, state->_ppifeatures);
   }

   if(state->sif != NULL) // write control ports values only for synths
   {
      for(size_t c = 0; c < state->sif->_inportsControl; c++)
      {
         state->iStateValues.insert(QString(state->sif->_controlInPorts [c].cName), QPair<QString, QVariant>(QString(""), QVariant((double)state->sif->_controls[c].val)));
      }
   }

   if(state->uiCurrent != NULL)
   {
      const char *cUiUri = lilv_node_as_uri(lilv_ui_get_uri(state->uiCurrent));
      state->iStateValues.insert(QString(cUiUri), QPair<QString, QVariant>(QString(""), QVariant(QString(cUiUri))));
   }

   QByteArray arrOut;
   QDataStream streamOut(&arrOut, QIODevice::WriteOnly);
   streamOut << state->iStateValues;
   QByteArray outEnc64 = arrOut.toBase64();
   QString customData(outEnc64);
   for (int pos=0; pos < customData.size(); pos+=150)
   {
        customData.insert(pos++,'\n'); // add newlines for readability
   }
   xml.strTag(level, "customData", customData);
}

void LV2Synth::lv2conf_set(LV2PluginWrapper_State *state, const std::vector<QString> &customParams)
{
   if(customParams.size() == 0)
      return;

   state->iStateValues.clear();
   for(size_t i = 0; i < customParams.size(); i++)
   {
      QString param = customParams [i];
      param.remove('\n'); // remove all linebreaks that may have been added to prettyprint the songs file
      QByteArray paramIn;
      paramIn.append(param);
      QByteArray dec64 = QByteArray::fromBase64(paramIn);
      QDataStream streamIn(&dec64, QIODevice::ReadOnly);
      streamIn >> state->iStateValues;
      break; //one customData tag includes all data in base64
   }

   size_t numValues = state->iStateValues.size();
   state->numStateValues = numValues;
   if(state->iState != NULL && numValues > 0)
   {
      state->tmpValues = new char*[numValues];;
      memset(state->tmpValues, 0, numValues * sizeof(char *));
      state->iState->restore(lilv_instance_get_handle(state->handle), LV2Synth::lv2state_stateRetreive, state, 0, state->_ppifeatures);
      for(size_t i = 0; i < numValues; ++i)
      {
         if(state->tmpValues [i] != NULL)
            delete [] state->tmpValues [i];
      }
      delete [] state->tmpValues;
      state->tmpValues = NULL;
   }

   QMap<QString, QPair<QString, QVariant> >::const_iterator it;
   for(it = state->iStateValues.begin(); it != state->iStateValues.end(); ++it)
   {
      QString name = it.key();
      QVariant qVal = it.value().second;
      if(!name.isEmpty() && qVal.isValid())
      {
         if(qVal.type() == QVariant::String) // plugin ui uri
         {
            QString sUiUri = qVal.toString();
            LV2_PLUGIN_UI_TYPES::iterator it;
            for(it = state->synth->_pluginUiTypes.begin(); it != state->synth->_pluginUiTypes.end(); ++it)
            {
               if(sUiUri == QString(lilv_node_as_uri(lilv_ui_get_uri(it->first))))
               {
                  state->uiCurrent = it->first;
                  break;
               }
            }
         }
         else
         {
            if(state->sif != NULL) //setting control value only for synths
            {

               bool ok = false;
               float val = (float)qVal.toDouble(&ok);
               if(ok)
               {
                  std::map<QString, size_t>::iterator it = state->controlsNameMap.find(name.toLower());
                  if(it != state->controlsNameMap.end())
                  {
                     size_t ctrlNum = it->second;
                     state->sif->_controls [ctrlNum].val = state->sif->_controls [ctrlNum].tmpVal = val;

                  }
               }
            }
         }


      }
   }

}

unsigned LV2Synth::lv2ui_IsSupported(const char *, const char *ui_type_uri)
{
   if(strcmp(LV2_F_UI_Qt5_UI, ui_type_uri) == 0
      || (bLV2Gtk2Enabled && strcmp(LV2_UI__GtkUI, ui_type_uri) == 0)
      || strcmp(LV2_UI__X11UI, ui_type_uri) == 0)
   {
      return 1;
   }
   return 0;
}

void LV2Synth::lv2prg_updatePrograms(LV2PluginWrapper_State *state)
{
   assert(state != NULL);
   state->index2prg.clear();
   state->prg2index.clear();
   if(state->prgIface != NULL)
   {
      uint32_t iPrg = 0;
      const LV2_Program_Descriptor *pDescr = NULL;
      while((pDescr = state->prgIface->get_program(
                lilv_instance_get_handle(state->handle), iPrg)) != NULL)
      {
         lv2ExtProgram extPrg;
         extPrg.index = iPrg;
         extPrg.bank = pDescr->bank;
         extPrg.prog = pDescr->program;
         extPrg.useIndex = true;
         extPrg.name = QString(pDescr->name);

         state->index2prg.insert(std::make_pair<uint32_t, lv2ExtProgram>(iPrg, extPrg));
         uint32_t midiprg = ((extPrg.bank & 0xff) << 8) + extPrg.prog;
         state->prg2index.insert(std::make_pair<uint32_t, uint32_t>(midiprg, iPrg));
         ++iPrg;
      }
   }

}

int LV2Synth::lv2_printf(LV2_Log_Handle handle, LV2_URID type, const char *fmt, ...)
{
   va_list argptr;
   va_start(argptr, fmt);
   int ret = LV2Synth::lv2_vprintf(handle, type, fmt, argptr);
   va_end(argptr);
   return ret;
}

int LV2Synth::lv2_vprintf(LV2_Log_Handle, LV2_URID, const char *fmt, va_list ap)
{
   return vprintf(fmt, ap);

}

char *LV2Synth::lv2state_makePath(LV2_State_Make_Path_Handle handle, const char *path)
{
   LV2PluginWrapper_State *state = (LV2PluginWrapper_State *)handle;
   assert(state != NULL);

   QFile ff(path);
   QFileInfo fiPath(ff);

   if(fiPath.isAbsolute())
   {
      return strdup(path);
   }

   QString plugName = (state->sif != NULL) ? state->sif->name() : state->plugInst->name();
   QString dirName = MusEGlobal::museProject + QString("/") + plugName;
   QDir dir;
   dir.mkpath(dirName);

   QString resPath = dirName + QString("/") + QString(path);
   return strdup(resPath.toUtf8().constData());

}

char *LV2Synth::lv2state_abstractPath(LV2_State_Map_Path_Handle handle, const char *absolute_path)
{
   LV2PluginWrapper_State *state = (LV2PluginWrapper_State *)handle;
   assert(state != NULL);

   //some plugins do not support abstract paths properly,
   //so return duplicate without modification for now
      //return strdup(absolute_path);

   QString resPath = QString(absolute_path);
   int rIdx = resPath.lastIndexOf('/');
   if(rIdx > -1)
   {
      resPath = resPath.mid(rIdx + 1);
   }
   QString plugName = (state->sif != NULL) ? state->sif->name() : state->plugInst->name();
   QDir dir;
   QString prjPath = MusEGlobal::museProject + QString("/") + plugName;
   dir.mkpath(prjPath);
   QFile ff(absolute_path);
   QFileInfo fiPath(ff);
   if(resPath.length() && fiPath.isAbsolute() && resPath != QString(absolute_path))
   {
      QFile::link(QString(absolute_path), prjPath + QString("/") + resPath);
   }

   if(strlen(absolute_path) == 0)
   {
      resPath = prjPath + QString("/") + resPath;
   }


   return strdup(resPath.toUtf8().constData());

}

char *LV2Synth::lv2state_absolutePath(LV2_State_Map_Path_Handle handle, const char *abstract_path)
{
   return LV2Synth::lv2state_makePath((LV2_State_Make_Path_Handle)handle, abstract_path);
}

void LV2Synth::lv2state_populatePresetsMenu(LV2PluginWrapper_State *state, QMenu *menu)
{
   std::map<QString, LilvNode *>::iterator it;
   LV2Synth *synth = state->synth;
   menu->clear();
   for(it = synth->_presets.begin(); it != synth->_presets.end(); ++it)
   {
      QAction *act = menu->addAction(it->first);
      act->setData(QVariant::fromValue<void *>(static_cast<void *>((it->second))));
   }
   if(menu->actions().size() == 0)
   {
      QAction *act = menu->addAction(QObject::tr("No presets found"));
      act->setDisabled(true);
      act->setData(QVariant::fromValue<void *>(NULL));
   }



}

void LV2Synth::lv2state_PortWrite(LV2UI_Controller controller, uint32_t port_index, uint32_t buffer_size, uint32_t protocol, const void *buffer, bool fromUi)
{
   LV2PluginWrapper_State *state = (LV2PluginWrapper_State *)controller;

   assert(state != NULL); //this should'nt happen
   assert(state->inst != NULL || state->sif != NULL); // this too

   if(protocol != 0 && protocol != state->synth->_uAtom_EventTransfer)
   {
#ifdef DEBUG_LV2
      std::cerr << "LV2Synth::lv2state_PortWrite: unsupported protocol (" << protocol << ")" << std::endl;
#endif
      return;
   }

   if(protocol == state->synth->_uAtom_EventTransfer) //put atom transfers to dedicated ring buffer
   {
#ifdef DEBUG_LV2
      std::cerr << "LV2Synth::lv2state_PortWrite: atom_EventTransfer, port = " << port_index << ", size =" << buffer_size << std::endl;
#endif
      state->uiControlEvt.put(port_index, buffer_size, buffer);
      return;
   }

   std::map<uint32_t, uint32_t>::iterator it = state->synth->_idxToControlMap.find(port_index);

   if(it == state->synth->_idxToControlMap.end())
   {
#ifdef DEBUG_LV2
      std::cerr << "LV2Synth::lv2state_PortWrite: wrong port index (" << port_index << ")" << std::endl;
#endif
      return;
   }

   uint32_t cport = it->second;
   float value = *(float *)buffer;

   // Schedules a timed control change:
   ControlEvent ce;
   ce.unique = false;
   ce.fromGui = fromUi;                 // It came from the plugin's own GUI (or not).
   ce.idx = cport;
   ce.value = value;
   // Don't use timestamp(), because it's circular, which is making it impossible to deal
   // with 'modulo' events which slip in 'under the wire' before processing the ring buffers.
   ce.frame = MusEGlobal::audio->curFrame();

   ControlFifo *_controlFifo = NULL;
   if(state->inst != NULL)
   {
      _controlFifo = &state->plugInst->_controlFifo;
      if(fromUi)
      {
         // Record automation:
         // Take care of this immediately, because we don't want the silly delay associated with
         //  processing the fifo one-at-a-time in the apply().
         // NOTE: With some vsts we don't receive control events until the user RELEASES a control.
         // So the events all arrive at once when the user releases a control.
         // That makes this pretty useless... But what the heck...
         //AutomationType at = AUTO_OFF;
         if(state->plugInst->_track && state->plugInst->_id != -1)
         {
            unsigned long id = genACnum(state->plugInst->_id, cport);
            state->plugInst->_track->recordAutomation(id, value);
            //at = state->plugInst->_track->automationType();
         }

         //state->plugInst->enableController(cport, false);
      }
   }
   else if(state->sif != NULL)
   {
      _controlFifo = &state->sif->_controlFifo;
      if(fromUi)
      {
         // Record automation:
         // Take care of this immediately, because we don't want the silly delay associated with
         //  processing the fifo one-at-a-time in the apply().
         // NOTE: With some vsts we don't receive control events until the user RELEASES a control.
         // So the events all arrive at once when the user releases a control.
         // That makes this pretty useless... But what the heck...
         if(state->sif->id() != -1)
         {
            unsigned long pid = genACnum(state->sif->id(), cport);
            state->sif->synti->recordAutomation(pid, value);
         }

         //state->sif->enableController(cport, false);
      }
   }

   if(fromUi)
   {
      state->controlTimers [cport] = 1000 / 30; //  1 sec controllers will not be send to guis
   }

   assert(_controlFifo != NULL);
   if(_controlFifo->put(ce))
     std::cerr << "LV2Synth::lv2state_PortWrite: fifo overflow: in control number:" << cport << std::endl;

#ifdef DEBUG_LV2
   std::cerr << "LV2Synth::lv2state_PortWrite: port=" << cport << "(" << port_index << ")" << ", val=" << value << std::endl;
#endif


}

void LV2Synth::lv2state_setPortValue(const char *port_symbol, void *user_data, const void *value, uint32_t size, uint32_t type)
{
   LV2PluginWrapper_State *state = (LV2PluginWrapper_State *)user_data;
   assert(state != NULL);
   std::map<QString, size_t>::iterator it = state->controlsSymMap.find(QString::fromUtf8(port_symbol).toLower());
   if(it != state->controlsSymMap.end())
   {
      size_t ctrlNum = it->second;
      uint32_t ctrlIdx = state->synth->_controlInPorts [ctrlNum].index;
      float fvalue;
      if (type == state->atomForge.Float)
      {
         fvalue = *(const float*)value;
      }
      else if (type == state->atomForge.Double)
      {
         fvalue = *(const double*)value;
      }
      else if (type == state->atomForge.Int)
      {
         fvalue = *(const int32_t*)value;
      }
      else if (type == state->atomForge.Long)
      {
         fvalue = *(const int64_t*)value;
      }
      else
      {
         fprintf(stderr, "error: Preset `%s' value has bad type <%s>\n",
                 port_symbol, state->synth->uridBiMap.unmap(type));
         return;
      }
      LV2Synth::lv2state_PortWrite((LV2UI_Controller)user_data, ctrlIdx, size, 0, &fvalue, false);
   }

}

void LV2Synth::lv2state_applyPreset(LV2PluginWrapper_State *state, LilvNode *preset)
{
   LilvState* lilvState = lilv_state_new_from_world(lilvWorld, &state->synth->_lv2_urid_map, preset);
   if(lilvState)
   {
      lilv_state_restore(lilvState, state->handle, LV2Synth::lv2state_setPortValue, state, 0, NULL);
      lilv_state_free(lilvState);
   }

}




void LV2SynthIF::lv2prg_Changed(LV2_Programs_Handle handle, int32_t index)
{
   LV2PluginWrapper_State *state = (LV2PluginWrapper_State *)handle;
#ifdef DEBUG_LV2
   std::cerr << "LV2Synth::lv2prg_Changed: index: " << index << std::endl;
#endif
   if(state->sif && state->sif->synti)
   {
      std::map<uint32_t, lv2ExtProgram>::iterator itIndex = state->index2prg.find(index);
      if(itIndex == state->index2prg.end())
         return;
      int ch      = 0;
      int port    = state->sif->synti->midiPort();

      const lv2ExtProgram &extPrg = itIndex->second;

      state->sif->synti->_curBankH = 0;
      state->sif->synti->_curBankL = extPrg.bank;
      state->sif->synti->_curProgram = extPrg.prog;

      int rv = ((((int)itIndex->second.bank)<<8) + (int)extPrg.prog);

      if(port != -1)
      {
        MidiPlayEvent event(0, port, ch, MusECore::ME_CONTROLLER, MusECore::CTRL_PROGRAM, rv);
        //MusEGlobal::midiPorts[port].sendEvent(event);
        MusEGlobal::midiPorts[port].sendHwCtrlState(event, false);
        if(state->sif->id() != -1 && state->sif->_controls != NULL)
        {
           for(unsigned long k = 0; k < state->sif->_inportsControl; ++k)
           {
              state->sif->synti->setPluginCtrlVal(genACnum(state->sif->id(), k), state->sif->_controls[k].val);
           }
        }
      }

   }

}


LV2Synth::LV2Synth(const QFileInfo &fi, QString label, QString name, QString author, const LilvPlugin *_plugin)
   : Synth(fi, label, name, author, QString("")),
     _handle(_plugin),
     _features(NULL),
     _ppfeatures(NULL),
     _options(NULL),
     _isSynth(false),
     _uis(NULL),
     _hasFreeWheelPort(false),
     _isConstructed(false),
     _pluginControlsDefault(NULL),
     _pluginControlsMin(NULL),
     _pluginControlsMax(NULL)
{

   //fake id for LV2PluginWrapper functionality
   _uniqueID = uniqueID++;

   _midi_event_id = mapUrid(LV2_MIDI__MidiEvent);

   _uTime_Position        = mapUrid(LV2_TIME__Position);
   _uTime_frame           = mapUrid(LV2_TIME__frame);
   _uTime_speed           = mapUrid(LV2_TIME__speed);
   _uTime_beatsPerMinute  = mapUrid(LV2_TIME__beatsPerMinute);
   _uTime_barBeat         = mapUrid(LV2_TIME__barBeat);
   _uAtom_EventTransfer   = mapUrid(LV2_ATOM__eventTransfer);

   _sampleRate = (double)MusEGlobal::sampleRate;

   //prepare features and options arrays
   LV2_Options_Option _tmpl_options [] =
   {
      {LV2_OPTIONS_INSTANCE, 0, uridBiMap.map(LV2_P_SAMPLE_RATE), sizeof(int32_t), uridBiMap.map(LV2_ATOM__Int), &_sampleRate},
      {LV2_OPTIONS_INSTANCE, 0, uridBiMap.map(LV2_P_MIN_BLKLEN), sizeof(int32_t), uridBiMap.map(LV2_ATOM__Int), &MusEGlobal::segmentSize},
      {LV2_OPTIONS_INSTANCE, 0, uridBiMap.map(LV2_P_MAX_BLKLEN), sizeof(int32_t), uridBiMap.map(LV2_ATOM__Int), &MusEGlobal::segmentSize},
      {LV2_OPTIONS_INSTANCE, 0, uridBiMap.map(LV2_P_SEQ_SIZE), sizeof(int32_t), uridBiMap.map(LV2_ATOM__Int), &MusEGlobal::segmentSize},
      {LV2_OPTIONS_INSTANCE, 0, 0, 0, 0, NULL}

   };

   _options = new LV2_Options_Option[SIZEOF_ARRAY(_tmpl_options)]; // last option is NULLs

   for(uint32_t i = 0; i < SIZEOF_ARRAY(_tmpl_options); i++)
   {
      _options [i] = _tmpl_options [i];
   }

   _features = new LV2_Feature[SIZEOF_ARRAY(lv2Features)];
   _ppfeatures = new LV2_Feature *[SIZEOF_ARRAY(lv2Features) + 1];
   _lv2_urid_map.map = Synth_Urid_Map;
   _lv2_urid_map.handle = this;
   _lv2_urid_unmap.unmap = Synth_Urid_Unmap;
   _lv2_urid_unmap.handle = this;
   _lv2_uri_map.uri_to_id = Synth_Uri_Map;
   _lv2_uri_map.callback_data = this;
   _lv2_log_log.handle = this;
   _lv2_log_log.printf = LV2Synth::lv2_printf;
   _lv2_log_log.vprintf = LV2Synth::lv2_vprintf;

   uint32_t i;

   for(i = 0; i < SIZEOF_ARRAY(lv2Features); i++)
   {
      _features [i] = lv2Features [i];

      if(_features [i].URI == NULL)
      {
         break;
      }

      if(std::string(LV2_F_URID_MAP) == _features [i].URI)
      {
         _features [i].data = &_lv2_urid_map;
      }
      else if(std::string(LV2_F_URID_UNMAP) == _features [i].URI)
      {
         _features [i].data = &_lv2_urid_unmap;
      }
      else if(std::string(LV2_F_URI_MAP) == _features [i].URI)
      {
         _features [i].data = &_lv2_uri_map;
      }
      else if(std::string(LV2_F_OPTIONS) == _features [i].URI)
      {
         _features [i].data = _options;
      }
      else if(std::string(LV2_F_INSTANCE_ACCESS) == _features [i].URI)
      {
         _fInstanceAccess = i;
      }
      else if(std::string(LV2_F_UI_PARENT) == _features [i].URI)
      {
         _fUiParent = i;
      }
      else if((std::string(LV2_F_UI_EXTERNAL_HOST) == _features [i].URI))
      {
         _fExtUiHost = i;
      }
      else if((std::string(LV2_UI_EXTERNAL_DEPRECATED) == _features [i].URI))
      {
         _fExtUiHostD = i;
      }
      else if((std::string(LV2_F_WORKER_SCHEDULE) == _features [i].URI))
      {
         _fWrkSchedule = i;
      }
      else if((std::string(LV2_UI__resize) == _features [i].URI))
      {
         _fUiResize = i;
      }
      else if((std::string(LV2_PROGRAMS__Host) == _features [i].URI))
      {
         _fPrgHost = i;
      }
      else if((std::string(LV2_LOG__log) == _features [i].URI))
      {
         _features [i].data = &_lv2_log_log;
      }
      else if((std::string(LV2_STATE__makePath) == _features [i].URI))
      {
         _fMakePath = i;
      }
      else if((std::string(LV2_STATE__mapPath) == _features [i].URI))
      {
         _fMapPath = i;
      }
      else if(std::string(LV2_F_DATA_ACCESS) == _features [i].URI)
      {
         _fDataAccess = i; //must be the last!
      }

      _ppfeatures [i] = &_features [i];
   }

   _ppfeatures [i] = 0;

   //enum plugin ports;
   uint32_t numPorts = lilv_plugin_get_num_ports(_handle);

   const LilvPort *lilvFreeWheelPort = lilv_plugin_get_port_by_designation(_handle, lv2CacheNodes.lv2_InputPort, lv2CacheNodes.lv2_FreeWheelPort);

   _pluginControlsDefault = new float [numPorts];
   _pluginControlsMin = new float [numPorts];
   _pluginControlsMax = new float [numPorts];
   memset(_pluginControlsDefault, 0, sizeof(float) * numPorts);
   memset(_pluginControlsMin, 0, sizeof(float) * numPorts);
   memset(_pluginControlsMax, 0, sizeof(float) * numPorts);

   lilv_plugin_get_port_ranges_float(_handle, _pluginControlsMin, _pluginControlsMax, _pluginControlsDefault);

   for(uint32_t i = 0; i < numPorts; i++)
   {
      const LilvPort *_port = lilv_plugin_get_port_by_index(_handle, i);
      LilvNode *_nPname = lilv_port_get_name(_handle, _port);
      const LilvNode *_nPsym = lilv_port_get_symbol(_handle, _port);
      char cAutoGenPortName [1024];
      char cAutoGenPortSym [1024];
      memset(cAutoGenPortName, 0, sizeof(cAutoGenPortName));
      memset(cAutoGenPortSym, 0, sizeof(cAutoGenPortSym));
      snprintf(cAutoGenPortName, sizeof(cAutoGenPortName) - 1, "autoport #%u", i);
      snprintf(cAutoGenPortSym, sizeof(cAutoGenPortSym) - 1, "autoport#%u", i);
      const char *_portName = cAutoGenPortName;
      const char *_portSym = cAutoGenPortSym;

      if(_nPname != 0)
      {
         _portName = lilv_node_as_string(_nPname);
         lilv_node_free(_nPname);
      }

      if(_nPsym != 0)
      {
         _portSym = lilv_node_as_string(_nPsym);
      }

      const bool optional = lilv_port_has_property(_handle, _port, lv2CacheNodes.lv2_connectionOptional);

      LV2_MIDI_PORTS *mPorts = &_midiOutPorts;
      LV2_CONTROL_PORTS *cPorts = &_controlOutPorts;
      LV2_AUDIO_PORTS *aPorts = &_audioOutPorts;

      if(lilv_port_is_a(_handle, _port, lv2CacheNodes.lv2_InputPort))
      {
         mPorts = &_midiInPorts;
         cPorts = &_controlInPorts;
         aPorts = &_audioInPorts;
      }
      else if(!lilv_port_is_a(_handle, _port, lv2CacheNodes.lv2_OutputPort))
      {
#ifdef DEBUG_LV2
         std::cerr << "plugin has port with unknown direction - ignoring" << std::endl;
#endif
         continue;
      }

      bool isCVPort = lilv_port_is_a(_handle, _port, lv2CacheNodes.lv2_CVPort);

      if(lilv_port_is_a(_handle, _port, lv2CacheNodes.lv2_ControlPort) || isCVPort)
      {
         LV2ControlPortType _cType = LV2_PORT_CONTINUOUS;
         if(lilv_port_has_property(_handle, _port, lv2CacheNodes.lv2_portDiscrete))
            _cType = LV2_PORT_DISCRETE;
         else if(lilv_port_has_property(_handle, _port, lv2CacheNodes.lv2_portInteger))
            _cType = LV2_PORT_INTEGER;
         else if(lilv_port_has_property(_handle, _port, lv2CacheNodes.lv2_portTrigger)
                 || lilv_port_has_property(_handle, _port, lv2CacheNodes.lv2_portToggled))
            _cType = LV2_PORT_TRIGGER;
         else if(lilv_port_has_property(_handle, _port, lv2CacheNodes.lv2_portLogarithmic))
            _cType = LV2_PORT_LOGARITHMIC;

         cPorts->push_back(LV2ControlPort(_port, i, 0.0f, _portName, _portSym, _cType, isCVPort));

         if(isnan(_pluginControlsDefault [i]))
            _pluginControlsDefault [i] = 0;
         if(isnan(_pluginControlsMin [i]))
            _pluginControlsMin [i] = 0;
         if(isnan(_pluginControlsMax [i]))
            _pluginControlsMax [i] = 0;

         if(isCVPort)
         {
            _pluginControlsDefault [i] = 1;
            _pluginControlsMin [i] = 0;
            _pluginControlsMax [i] = 1;
         }
         else if (lilv_port_has_property (_handle, _port, lv2CacheNodes.lv2_SampleRate))
         {
            _pluginControlsDefault [i] *= MusEGlobal::sampleRate;
            _pluginControlsMin [i] *= MusEGlobal::sampleRate;
            _pluginControlsMax [i] *= MusEGlobal::sampleRate;
         }

      }
      else if(lilv_port_is_a(_handle, _port, lv2CacheNodes.lv2_AudioPort))
      {
         aPorts->push_back(LV2AudioPort(_port, i, NULL, _portName));
      }
      else if(lilv_port_is_a(_handle, _port, lv2CacheNodes.ev_EventPort))
      {
         bool portSupportsTimePos = lilv_port_supports_event(_handle, _port, lv2CacheNodes.lv2_TimePosition);
         mPorts->push_back(LV2MidiPort(_port, i, _portName, true /* old api is on */, NULL, portSupportsTimePos));
      }
      else if(lilv_port_is_a(_handle, _port, lv2CacheNodes.atom_AtomPort))
      {
         bool portSupportsTimePos = lilv_port_supports_event(_handle, _port, lv2CacheNodes.lv2_TimePosition);
         mPorts->push_back(LV2MidiPort(_port, i, _portName, false /* old api is off */, NULL, portSupportsTimePos));
      }
      else if(!optional)
      {
//#ifdef DEBUG_LV2
         std::cerr << "plugin has port with unknown type - ignoring plugin " << label.toStdString() << "!" << std::endl;
//#endif
         return;
      }
   }

   for(uint32_t i = 0; i < _controlInPorts.size(); ++i)
   {
      _idxToControlMap.insert(std::pair<uint32_t, uint32_t>(_controlInPorts [i].index, i));
      if(lilvFreeWheelPort != NULL)
      {
         if(lilv_port_get_index(_handle, _controlInPorts [i].port) == lilv_port_get_index(_handle, lilvFreeWheelPort))
         {
            _hasFreeWheelPort = true;
            _freeWheelPortIndex = i;
         }
      }
   }

   if(_midiInPorts.size() > 0)
   {
      _isSynth = true;
   }

   const LilvNode *pluginUIType = NULL;
   _uis = NULL;


   _uis = lilv_plugin_get_uis(_handle);

   if(_uis)
   {
      LilvIter *it = lilv_uis_begin(_uis);

#ifdef DEBUG_LV2
      std::cerr << "Plugin support uis of type:" << std::endl;
#endif


      while(!lilv_uis_is_end(_uis, it))
      {
         const LilvUI *ui = lilv_uis_get(_uis, it);
         if(lilv_ui_is_supported(ui,
                                 LV2Synth::lv2ui_IsSupported,
                                 lv2CacheNodes.host_uiType,
                                 &pluginUIType))
         {
#ifdef DEBUG_LV2
            const char *strUiType = lilv_node_as_string(pluginUIType); //internal uis are preferred
            std::cerr << "Plugin " << label.toStdString() << " supports ui of type " << strUiType << std::endl;
#endif
            _pluginUiTypes.insert(std::make_pair<const LilvUI *, std::pair<bool, const LilvNode *> >(ui, std::make_pair<bool, const LilvNode *>(false, pluginUIType)));
         }
         else
         {
            const LilvNodes *nUiClss = lilv_ui_get_classes(ui);
            LilvIter *nit = lilv_nodes_begin(nUiClss);

            while(!lilv_nodes_is_end(_uis, nit))
            {
               const LilvNode *nUiCls = lilv_nodes_get(nUiClss, nit);
#ifdef  DEBUG_LV2
               const char *ClsStr = lilv_node_as_string(nUiCls);
               std::cerr << ClsStr << std::endl;
#endif
               bool extUi = lilv_node_equals(nUiCls, lv2CacheNodes.ext_uiType);
               bool extdUi = lilv_node_equals(nUiCls, lv2CacheNodes.ext_d_uiType);
               if(extUi || extdUi)
               {
                  pluginUIType = extUi ? lv2CacheNodes.ext_uiType : lv2CacheNodes.ext_d_uiType;
#ifdef DEBUG_LV2
                  std::cerr << "Plugin " << label.toStdString() << " supports ui of type " << LV2_UI_EXTERNAL << std::endl;
#endif
                  _pluginUiTypes.insert(std::make_pair<const LilvUI *, std::pair<bool, const LilvNode *> >(ui, std::make_pair<bool, const LilvNode *>(true, pluginUIType)));
               }

               nit = lilv_nodes_next(nUiClss, nit);
            }

         }

         it = lilv_uis_next(_uis, it);
      };


   }

   //scan for preserts
   LilvNodes* presets = lilv_plugin_get_related(_handle, lv2CacheNodes.lv2_psetPreset);
   LILV_FOREACH(nodes, i, presets)
   {
      const LilvNode* preset = lilv_nodes_get(presets, i);
#ifdef DEBUG_LV2
      std::cerr << "\tPreset: " << lilv_node_as_uri(preset) << std::endl;
#endif
      lilv_world_load_resource(lilvWorld, preset);
      LilvNodes* pLabels = lilv_world_find_nodes(lilvWorld, preset, lv2CacheNodes.lv2_rdfsLabel, NULL);
      if (pLabels != NULL)
      {
         const LilvNode* pLabel = lilv_nodes_get_first(pLabels);
         _presets.insert(std::make_pair<QString, LilvNode *>(lilv_node_as_string(pLabel), lilv_node_duplicate(preset)));
         lilv_nodes_free(pLabels);
      }
      else
      {
#ifdef DEBUG_LV2
         std::cerr << "\t\tPreset <%s> has no rdfs:label" << lilv_node_as_string(lilv_nodes_get(presets, i)) << std::endl;
#endif
      }
   }
   lilv_nodes_free(presets);


   _isConstructed = true;
}

LV2Synth::~LV2Synth()
{
   if(_ppfeatures)
   {
      delete [] _ppfeatures;
      _ppfeatures = NULL;
   }

   if(_features)
   {
      delete [] _features;
      _features = NULL;
   }

   if(_options)
   {
      delete [] _options;
      _options = NULL;
   }

   if(_uis != NULL)
   {
      lilv_uis_free(_uis);
      _uis = NULL;
   }
   std::map<QString, LilvNode *>::iterator it;
   for(it = _presets.begin(); it != _presets.end(); ++it)
   {
      lilv_node_free(it->second);
   }
#if 0
   //TODO: Make real check for lilv version
   //for existance of 'lilv_world_unload_resource' function
   LilvNodes* presets = lilv_plugin_get_related(_handle, lv2CacheNodes.lv2_psetPreset);
   LILV_FOREACH(nodes, i, presets)
   {
      const LilvNode* preset = lilv_nodes_get(presets, i);
      lilv_world_unload_resource(lilvWorld, preset);
   }
   lilv_nodes_free(presets);
#endif
}


SynthIF *LV2Synth::createSIF(SynthI *synthi)
{
   ++_instances;
   LV2SynthIF *sif = new LV2SynthIF(synthi);

   if(!sif->init(this))
   {
      delete sif;
      sif = NULL;
   }

   return sif;
}

LV2_URID LV2Synth::mapUrid(const char *uri)
{
   return uridBiMap.map(uri);
}

const char *LV2Synth::unmapUrid(LV2_URID id)
{
   return uridBiMap.unmap(id);
}

LV2SynthIF::~LV2SynthIF()
{
   if(_uiState != NULL)
   {
      _uiState->deleteLater = true;
      //_uiState->uiTimer->stopNextTime(false);
      if(_uiState->pluginWindow != NULL)
         _uiState->pluginWindow->stopNextTime();
      else
         LV2Synth::lv2state_FreeState(_uiState);
      _uiState = NULL;

   }

   LV2_AUDIO_PORTS::iterator _itA = _audioInPorts.begin();

   for(; _itA != _audioInPorts.end(); ++_itA)
   {
      free((*_itA).buffer);
   }

   _itA = _audioOutPorts.begin();

   for(; _itA != _audioOutPorts.end(); ++_itA)
   {
      free((*_itA).buffer);
   }

   if(_audioInBuffers)
   {
      delete [] _audioInBuffers;
      _audioInBuffers = NULL;
   }

   if(_audioOutBuffers)
   {
      delete [] _audioOutBuffers;
      _audioOutBuffers = NULL;
   }

   if(_controls)
   {
      delete [] _controls;
   }

   if(_controlsOut)
   {
      delete [] _controlsOut;
   }

   if(_ppifeatures)
   {
      delete [] _ppifeatures;
      _ppifeatures = NULL;
   }

   if(_ifeatures)
   {
      delete [] _ifeatures;
      _ifeatures = NULL;
   }
}

LV2SynthIF::LV2SynthIF(SynthI *s): SynthIF(s)
{
   _synth = NULL;
   _handle = NULL;
   _audioInBuffers = NULL;
   _audioOutBuffers = NULL;
   _inports = 0;
   _outports = 0;
   _controls = NULL;
   _controlsOut = NULL;
   _inportsControl = 0;
   _outportsControl = 0;
   _inportsMidi = 0,
   _outportsMidi = 0,
   _audioInSilenceBuf = NULL;
   _ifeatures = NULL;
   _ppifeatures = NULL;
   _uiState = NULL;
}

bool LV2SynthIF::init(LV2Synth *s)
{
   _synth = s;

   //use LV2Synth features as template

   _uiState = new LV2PluginWrapper_State;
   _uiState->inst = NULL;
   _uiState->widget = NULL;
   _uiState->uiInst = NULL;
   _uiState->plugInst = NULL;
   _uiState->_ifeatures = new LV2_Feature[SIZEOF_ARRAY(lv2Features)];
   _uiState->_ppifeatures = new LV2_Feature *[SIZEOF_ARRAY(lv2Features) + 1];
   _uiState->sif = this;
   _uiState->synth = _synth;

   LV2Synth::lv2state_FillFeatures(_uiState);

   _uiState->handle = _handle = lilv_plugin_instantiate(_synth->_handle, (double)MusEGlobal::sampleRate, _uiState->_ppifeatures);

   if(_handle == NULL)
   {
      delete [] _uiState->_ppifeatures;
      _uiState->_ppifeatures = NULL;
      delete [] _uiState->_ifeatures;
      _uiState->_ifeatures = NULL;
      return false;
   }

   _audioInPorts = s->_audioInPorts;
   _audioOutPorts = s->_audioOutPorts;
   _controlInPorts = s->_controlInPorts;
   _controlOutPorts = s->_controlOutPorts;
   _inportsMidi = _uiState->midiInPorts.size();
   _outportsMidi = _uiState->midiOutPorts.size();



   _inportsControl = _controlInPorts.size();
   _outportsControl = _controlOutPorts.size();

   if(_inportsControl != 0)
   {
      _controls = new Port[_inportsControl];
   }
   else
   {
      _controls = NULL;
   }

   if(_outportsControl != 0)
   {
      _controlsOut = new Port[_outportsControl];
   }
   else
   {
      _controlsOut = NULL;
   }

   _uiState->controlsNameMap.clear();

   _synth->midiCtl2PortMap.clear();
   _synth->port2MidiCtlMap.clear();

   for(size_t i = 0; i < _inportsControl; i++)
   {
      uint32_t idx = _controlInPorts [i].index;
      _controls [i].idx = idx;
      _controls [i].val = _controls [i].tmpVal = _controlInPorts [i].defVal = _synth->_pluginControlsDefault [idx];
      if(_synth->_hasFreeWheelPort && _synth->_freeWheelPortIndex == i)
         _controls [i].enCtrl = false;
      else
         _controls [i].enCtrl = true;
      _controlInPorts [i].minVal = _synth->_pluginControlsMin [idx];
      _controlInPorts [i].maxVal = _synth->_pluginControlsMax [idx];

      _uiState->controlsNameMap.insert(std::pair<QString, size_t>(QString(_controlInPorts [i].cName).toLower(), i));
      _uiState->controlsSymMap.insert(std::pair<QString, size_t>(QString(_controlInPorts [i].cSym).toLower(), i));

      int ctlnum = CTRL_NRPN14_OFFSET + 0x2000 + i;

      // We have a controller number! Insert it and the lv2 port number into both maps.
      _synth->midiCtl2PortMap.insert(std::pair<int, int>(ctlnum, i));
      _synth->port2MidiCtlMap.insert(std::pair<int, int>(i, ctlnum));

      int id = genACnum(MAX_PLUGINS, i);
      CtrlList *cl;
      CtrlListList *cll = track()->controller();
      iCtrlList icl = cll->find(id);

      if(icl == cll->end())
      {
         cl = new CtrlList(id);
         cll->add(cl);
         cl->setCurVal(_controls[i].val);
      }
      else
      {
         cl = icl->second;
         _controls[i].val = cl->curVal();
      }

      cl->setRange(_synth->_pluginControlsMin [idx], _synth->_pluginControlsMax [idx]);
      cl->setName(QString(_controlInPorts [i].cName));
      CtrlValueType vt = VAL_LINEAR;
      switch(_controlInPorts [i].cType)
      {
      case LV2_PORT_CONTINUOUS:
         vt = VAL_LINEAR;
         break;
      case LV2_PORT_DISCRETE:
      case LV2_PORT_INTEGER:
         vt = VAL_INT;
         break;
      case LV2_PORT_LOGARITHMIC:
         vt = VAL_LOG;
         break;
      case LV2_PORT_TRIGGER:
         vt = VAL_BOOL;
         break;
      default:
         break;
      }

      cl->setValueType(vt);
      cl->setMode(((_controlInPorts [i].cType == LV2_PORT_CONTINUOUS)
                   ||(_controlInPorts [i].cType == LV2_PORT_LOGARITHMIC))? CtrlList::INTERPOLATE : CtrlList::DISCRETE);

      if(!_controlInPorts [i].isCVPort)
         lilv_instance_connect_port(_handle, idx, &_controls [i].val);
   }

   if(_inportsControl > 0)
   {
      _uiState->lastControls = new float [_inportsControl];
      _uiState->controlsMask = new bool [_inportsControl];
      _uiState->controlTimers = new int [_inportsControl];
      for(uint32_t i = 0; i < _inportsControl; i++)
      {
         _uiState->lastControls [i] = _controls [i].val;
         _uiState->controlsMask [i] = false;
         _uiState->controlTimers [i] = 0;
      }
   }

   if(_outportsControl > 0)
   {
      _uiState->lastControlsOut = new float [_outportsControl];
      for(uint32_t i = 0; i < _outportsControl; i++)
      {
         _uiState->lastControlsOut [i] = _controlsOut [i].val;
      }
   }

   for(size_t i = 0; i < _outportsControl; i++)
   {
      uint32_t idx = _controlOutPorts [i].index;
      _controlsOut[i].idx = idx;
      _controlsOut[i].val    = 0.0;
      _controlsOut[i].tmpVal = 0.0;
      _controlsOut[i].enCtrl  = false;
      _controlOutPorts [i].defVal = _controlOutPorts [i].minVal = _controlOutPorts [i].maxVal = 0.0;
      if(!_controlOutPorts [i].isCVPort)
         lilv_instance_connect_port(_handle, idx, &_controlsOut[i].val);
   }


   int rv = posix_memalign((void **)&_audioInSilenceBuf, 16, sizeof(float) * MusEGlobal::segmentSize);

   if(rv != 0)
   {
      fprintf(stderr, "ERROR: LV2SynthIF::init: posix_memalign returned error:%d. Aborting!\n", rv);
      abort();
   }

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

   //cache number of ports
   _inports = _audioInPorts.size();
   _outports = _audioOutPorts.size();


   if(_inports > 0)
   {
      _audioInBuffers = new float*[_inports];

      for(size_t i = 0; i < _inports; i++)
      {
         int rv = posix_memalign((void **)&_audioInBuffers [i], 16, sizeof(float) * MusEGlobal::segmentSize);

         if(rv != 0)
         {
            fprintf(stderr, "ERROR: LV2SynthIF::init: posix_memalign returned error:%d. Aborting!\n", rv);
            abort();
         }

         if(MusEGlobal::config.useDenormalBias)
         {
            for(unsigned q = 0; q < MusEGlobal::segmentSize; ++q)
            {
               _audioInBuffers [i][q] = MusEGlobal::denormalBias;
            }
         }
         else
         {
            memset(_audioInBuffers [i], 0, sizeof(float) * MusEGlobal::segmentSize);
         }
         _iUsedIdx.push_back(false);

         _iUsedIdx.push_back(false);

         _audioInPorts [i].buffer = _audioInBuffers [i];
         lilv_instance_connect_port(_handle, _audioInPorts [i].index, _audioInBuffers [i]);
      }
   }

   if(_outports > 0)
   {
      _audioOutBuffers = new float*[_outports];

      for(size_t i = 0; i < _outports; i++)
      {
         int rv = posix_memalign((void **)&_audioOutBuffers [i], 16, sizeof(float) * MusEGlobal::segmentSize);

         if(rv != 0)
         {
            fprintf(stderr, "ERROR: LV2SynthIF::init: posix_memalign returned error:%d. Aborting!\n", rv);
            abort();
         }

         if(MusEGlobal::config.useDenormalBias)
         {
            for(unsigned q = 0; q < MusEGlobal::segmentSize; ++q)
            {
               _audioOutBuffers [i][q] = MusEGlobal::denormalBias;
            }
         }
         else
         {
            memset(_audioOutBuffers [i], 0, sizeof(float) * MusEGlobal::segmentSize);
         }

         _audioOutPorts [i].buffer = _audioOutBuffers [i];
         lilv_instance_connect_port(_handle, _audioOutPorts [i].index, _audioOutBuffers [i]);
      }
   }

   LV2Synth::lv2state_PostInstantiate(_uiState);
   activate();



   return true;

}

void LV2SynthIF::doSelectProgram(unsigned char channel, int bank, int prog)
{
   if(_uiState && _uiState->prgIface && (_uiState->prgIface->select_program || _uiState->prgIface->select_program_for_channel))
   {
      if(_uiState->newPrgIface)
         _uiState->prgIface->select_program_for_channel(lilv_instance_get_handle(_uiState->handle), channel, (uint32_t)bank, (uint32_t)prog);
      else
         _uiState->prgIface->select_program(lilv_instance_get_handle(_uiState->handle), (uint32_t)bank, (uint32_t)prog);



      /*
      * A plugin is permitted to re-write the values of its input
      * control ports when select_program is called. The host should
      * re-read the input control port values and update its own
      * records appropriately. (This is the only circumstance in which
      * a LV2 plugin is allowed to modify its own control-input ports.)
      */
      if(id() != -1)
      {
         for(unsigned long k = 0; k < _inportsControl; ++k)
         {
            // We're in the audio thread context: no need to send a message, just modify directly.
            synti->setPluginCtrlVal(genACnum(id(), k), _controls[k].val);
         }
      }

      //set update ui program flag
      _uiState->uiChannel = channel;
      _uiState->uiBank = bank;
      _uiState->uiProg = prog;
      _uiState->uiDoSelectPrg = true;
   }
}

int LV2SynthIF::channels() const
{
   return (_outports) > MAX_CHANNELS ? MAX_CHANNELS : (_outports) ;

}

int LV2SynthIF::totalInChannels() const
{
   return _inports;
}

int LV2SynthIF::totalOutChannels() const
{
   return _outports;
}

void LV2SynthIF::activate()
{
   if(_handle)
   {
      lilv_instance_activate(_handle);
   }
}


void LV2SynthIF::deactivate()
{
   if(_handle)
   {
      lilv_instance_deactivate(_handle);
   }

}

void LV2SynthIF::deactivate3()
{
   deactivate();
}

int LV2SynthIF::eventsPending() const
{
   //TODO: what's this?
   return 0;
}

bool LV2SynthIF::lv2MidiControlValues(size_t port, int ctlnum, int *min, int *max, int *def)
{

   float fmin, fmax, fdef;
   int   imin;
   float frng;

   fdef = _controlInPorts [port].defVal;
   fmin = _controlInPorts [port].minVal;
   fmax = _controlInPorts [port].maxVal;
   bool hasdef = (fdef == fdef);

   if(fmin != fmin)
   {
      fmin = 0.0;
   }

   if(fmax != fmax)
   {
      fmax = 0.0;
   }

   MidiController::ControllerType t = midiControllerType(ctlnum);

#ifdef PLUGIN_DEBUGIN
   printf("lv2MidiControlValues: ctlnum:%d ladspa port:%lu has default?:%d default:%f\n", ctlnum, port, hasdef, fdef);
#endif


   frng = fmax - fmin;
   imin = lrintf(fmin);
   //imax = lrintf(fmax);

   int ctlmn = 0;
   int ctlmx = 127;// Avoid divide-by-zero error below.


#ifdef PLUGIN_DEBUGIN
   printf("lv2MidiControlValues: port min:%f max:%f \n", fmin, fmax);
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


   // It's a floating point control, just use wide open maximum range.
   *min = ctlmn;
   *max = ctlmx;

   assert(frng != 0.0);

   float normdef = fdef / frng;
   fdef = normdef * fctlrng;

   // FIXME: TODO: Incorrect... Fix this somewhat more trivial stuff later....

   *def = (int)lrintf(fdef) + bias;

#ifdef PLUGIN_DEBUGIN
   printf("lv2MidiControlValues: setting default:%d\n", *def);
#endif

   return hasdef;
}

//---------------------------------------------------------
//   midi2LadspaValue
//---------------------------------------------------------

float LV2SynthIF::midi2Lv2Value(unsigned long port, int ctlnum, int val)
{

   float fmin, fmax;
   int   imin;
   float frng;

   MidiController::ControllerType t = midiControllerType(ctlnum);

#ifdef PLUGIN_DEBUGIN
   printf("midi2Lv2Value: ctlnum:%d port:%lu val:%d\n", ctlnum, port, val);
#endif

   fmin = _controlInPorts [port].minVal;
   fmax = _controlInPorts [port].maxVal;

   if(fmin != fmin)
   {
      fmin = 0.0;
   }

   if(fmax != fmax)
   {
      fmax = 0.0;
   }



   frng = fmax - fmin;
   imin = lrintf(fmin);


   int ctlmn = 0;
   int ctlmx = 127;

#ifdef PLUGIN_DEBUGIN
   printf("midi2Lv2Value: port min:%f max:%f \n", fmin, fmax);
#endif

   bool isneg = (imin < 0);
   int bval = val;
   //int cval = val;

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
         //cval -= 64;
      }
      else
      {
         ctlmn = 0;
         ctlmx = 127;
        //cval -= 64;
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
         //cval -= 8192;
      }
      else
      {
         ctlmn = 0;
         ctlmx = 16383;
         //cval -= 8192;
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


   // Avoid divide-by-zero error below.
   if(ctlrng == 0)
   {
      return 0.0;
   }

   // It's a floating point control, just use wide open maximum range.
   float normval = float(bval) / fctlrng;
   float ret = normval * frng + fmin;

#ifdef PLUGIN_DEBUGIN
   printf("midi2Lv2Value: float returning:%f\n", ret);
#endif

   return ret;
}

int LV2SynthIF::getControllerInfo(int id, const char **name, int *ctrl, int *min, int *max, int *initval)
{
   size_t _id = (size_t)id;

   if(_id == _inportsControl || _id == _inportsControl + 1)
   {
      //
      // It is unknown at this point whether or not a synth recognizes aftertouch and poly aftertouch
      //  (channel and key pressure) midi messages, so add support for them now (as controllers).
      //
      if(_id == _inportsControl)
      {
         *ctrl = CTRL_POLYAFTER;
      }
      else if(_id == _inportsControl + 1)
      {
         *ctrl = CTRL_AFTERTOUCH;
      }

      *min  = 0;
      *max  = 127;
      *initval = CTRL_VAL_UNKNOWN;
      *name = midiCtrlName(*ctrl).toLatin1().constData();
      return ++_id;
   }
   else if(_id >= _inportsControl + 2)
   {
      return 0;
   }

   int ctlnum = DSSI_NONE;

   // No controller number? Give it one.
   if(ctlnum == DSSI_NONE)
   {
      // Simple but flawed solution: Start them at 0x60000 + 0x2000 = 0x62000. Max NRPN number is 0x3fff.
      ctlnum = CTRL_NRPN14_OFFSET + 0x2000 + _id;
   }


   int def = CTRL_VAL_UNKNOWN;

   if(lv2MidiControlValues(_id, ctlnum, min, max, &def))
   {
      *initval = def;
   }
   else
   {
      *initval = CTRL_VAL_UNKNOWN;
   }

#ifdef DEBUG_LV2
   printf("LV2SynthIF::getControllerInfo passed ctlnum:%d min:%d max:%d initval:%d\n", ctlnum, *min, *max, *initval);
#endif

   *ctrl = ctlnum;
   *name =  _controlInPorts [_id].cName;
   return ++_id;

}



bool LV2SynthIF::processEvent(const MidiPlayEvent &e, snd_seq_event_t *event)
{

   int chn = e.channel();
   int a   = e.dataA();
   int b   = e.dataB();

   int len = e.len();
   char ca[len + 2];

   ca[0] = 0xF0;
   memcpy(ca + 1, (const char *)e.data(), len);
   ca[len + 1] = 0xF7;

   len += 2;

#ifdef LV2_DEBUG
   fprintf(stderr, "LV2SynthIF::processEvent midi event type:%d chn:%d a:%d b:%d\n", e.type(), chn, a, b);
#endif

   switch(e.type())
   {
   case ME_NOTEON:
#ifdef DSSI_DEBUG
      fprintf(stderr, "LV2SynthIF::processEvent midi event is ME_NOTEON\n");
#endif

      snd_seq_ev_clear(event);
      event->queue = SND_SEQ_QUEUE_DIRECT;

      if(b)
      {
         snd_seq_ev_set_noteon(event, chn, a, b);
      }
      else
      {
         snd_seq_ev_set_noteoff(event, chn, a, 0);
      }

      break;

   case ME_NOTEOFF:
#ifdef LV2_DEBUG
      fprintf(stderr, "LV2SynthIF::processEvent midi event is ME_NOTEOFF\n");
#endif

      snd_seq_ev_clear(event);
      event->queue = SND_SEQ_QUEUE_DIRECT;
      snd_seq_ev_set_noteoff(event, chn, a, 0);
      break;

   case ME_PROGRAM:
   {
#ifdef LV2_DEBUG
      fprintf(stderr, "LV2SynthIF::processEvent midi event is ME_PROGRAM\n");
#endif

      int bank = (a >> 8) & 0xff;
      int prog = a & 0xff;

      if(bank & 0x80) //try msb value then
      {
         bank = (b >> 16) & 0xff;
      }

      if(!(bank & 0x80))
      {
         synti->_curBankH = 0;
         synti->_curBankL = bank;
      }
      else
         bank = synti->_curBankL;
      synti->_curProgram = prog;

      doSelectProgram(chn, bank, prog);

      //update hardware program state

      if(synti->midiPort() != -1)
      {
         MidiPort *mp = &MusEGlobal::midiPorts[synti->midiPort()];
         int db = (bank << 8) + prog;
         mp->setHwCtrlState(chn, CTRL_PROGRAM, db);

      }

      // Event pointer not filled. Return false.
      return false;
   }
   break;

   case ME_CONTROLLER:
   {
#ifdef LV2_DEBUG
      fprintf(stderr, "LV2SynthIF::processEvent midi event is ME_CONTROLLER\n");
#endif

      if((a == 0) || (a == 32))
      {
         return false;
      }

      if(a == CTRL_PROGRAM)
      {
#ifdef LV2_DEBUG
         fprintf(stderr, "LV2SynthIF::processEvent midi event is ME_CONTROLLER, dataA is CTRL_PROGRAM\n");
#endif

         int bank = (b >> 8) & 0xff;
         int prog = b & 0xff;

         if(bank & 0x80) //try msb value then
         {
            bank = (b >> 16) & 0xff;
         }

         if(!(bank & 0x80))
         {
            synti->_curBankH = 0;
            synti->_curBankL = bank;
         }
         else
            bank = synti->_curBankL;
         synti->_curProgram = prog;



         doSelectProgram(chn, bank, prog);

         //update hardware program state

         if(synti->midiPort() != -1)
         {
            MidiPort *mp = &MusEGlobal::midiPorts[synti->midiPort()];
            int db = (bank << 8) + prog;
            mp->setHwCtrlState(chn, a, db);

         }

         // Event pointer not filled. Return false.
         return false;
      }

      if(a == CTRL_PITCH)
      {
#ifdef LV2_DEBUG
         fprintf(stderr, "LV2SynthIF::processEvent midi event is ME_CONTROLLER, dataA is CTRL_PITCH\n");
#endif

         snd_seq_ev_clear(event);
         event->queue = SND_SEQ_QUEUE_DIRECT;
         snd_seq_ev_set_pitchbend(event, chn, b);
         // Event pointer filled. Return true.
         return true;
      }

      if(a == CTRL_AFTERTOUCH)
      {
#ifdef LV2_DEBUG
         fprintf(stderr, "LV2SynthIF::processEvent midi event is ME_CONTROLLER, dataA is CTRL_AFTERTOUCH\n");
#endif

         snd_seq_ev_clear(event);
         event->queue = SND_SEQ_QUEUE_DIRECT;
         snd_seq_ev_set_chanpress(event, chn, b);
         // Event pointer filled. Return true.
         return true;
      }

      if((a | 0xff)  == CTRL_POLYAFTER)
      {
#ifdef LV2_DEBUG
         fprintf(stderr, "LV2SynthIF::processEvent midi event is ME_CONTROLLER, dataA is CTRL_POLYAFTER\n");
#endif

         snd_seq_ev_clear(event);
         event->queue = SND_SEQ_QUEUE_DIRECT;
         snd_seq_ev_set_keypress(event, chn, a & 0x7f, b & 0x7f);
         // Event pointer filled. Return true.
         return true;
      }

      ciMidiCtl2LadspaPort ip = _synth->midiCtl2PortMap.find(a);

      // Is it just a regular midi controller, not mapped to a LADSPA port (either by the plugin or by us)?
      // NOTE: There's no way to tell which of these controllers is supported by the plugin.
      // For example sustain footpedal or pitch bend may be supported, but not mapped to any LADSPA port.
      if(ip == _synth->midiCtl2PortMap.end())
      {
         int ctlnum = a;

         if(midiControllerType(a) != MidiController::Controller7)
         {
            return false;   // Event pointer not filled. Return false.
         }
         else
         {
#ifdef LV2_DEBUG
            fprintf(stderr, "LV2SynthIF::processEvent non-ladspa midi event is Controller7. Current dataA:%d\n", a);
#endif
            a &= 0x7f;
            ctlnum = DSSI_CC_NUMBER(ctlnum);
         }

         // Fill the event.
#ifdef LV2_DEBUG
         printf("LV2SynthIF::processEvent non-ladspa filling midi event chn:%d dataA:%d dataB:%d\n", chn, a, b);
#endif
         snd_seq_ev_clear(event);
         event->queue = SND_SEQ_QUEUE_DIRECT;
         snd_seq_ev_set_controller(event, chn, a, b);
         return true;
      }

      unsigned long k = ip->second;
      int ctlnum = DSSI_NONE;

      // No midi controller for the ladspa port? Send to ladspa control.
      if(ctlnum == DSSI_NONE)
      {
         // Sanity check.
         if(k > _inportsControl)
         {
            return false;
         }

         // Simple but flawed solution: Start them at 0x60000 + 0x2000 = 0x62000. Max NRPN number is 0x3fff.
         ctlnum = k + (CTRL_NRPN14_OFFSET + 0x2000);
      }


      float val = midi2Lv2Value(k, ctlnum, b);

#ifdef LV2_DEBUG
      fprintf(stderr, "LV2SynthIF::processEvent control port:%lu port:%lu dataA:%d Converting val from:%d to lv2:%f\n", i, k, a, b, val);
#endif

      // Set the lv2 port value.
      _controls[k].val = val;

      // Need to update the automation value, otherwise it overwrites later with the last automation value.
      if(id() != -1)
         // We're in the audio thread context: no need to send a message, just modify directly.
      {
         synti->setPluginCtrlVal(genACnum(id(), k), val);
      }

      // Since we absorbed the message as a lv2 control change, return false - the event is not filled.
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

   default:
      if(MusEGlobal::debugMsg)
      {
         fprintf(stderr, "LV2SynthIF::processEvent midi event unknown type:%d\n", e.type());
      }

      // Event not filled.
      return false;
      break;
   }

   return true;

}


iMPEvent LV2SynthIF::getData(MidiPort *, MPEventList *el, iMPEvent  start_event, unsigned int pos, int ports, unsigned int nframes, float **buffer)
{
   // We may not be using ev_buf_sz all at once - this will be just the maximum.
   const unsigned long ev_buf_sz = el->size() + synti->eventFifo.getSize();
   snd_seq_event_t events[ev_buf_sz];

   const int frameOffset = MusEGlobal::audio->getFrameOffset();
   const unsigned long syncFrame = MusEGlobal::audio->curSyncFrame();
   // All ports must be connected to something!
   const unsigned long nop = ((unsigned long) ports) > _outports ? _outports : ((unsigned long) ports);
   const bool usefixedrate = false;
   const unsigned long min_per = (usefixedrate || MusEGlobal::config.minControlProcessPeriod > nframes) ? nframes : MusEGlobal::config.minControlProcessPeriod;
   const unsigned long min_per_mask = min_per - 1; // min_per must be power of 2

   unsigned long sample = 0;
   AudioTrack *atrack = track();
   const AutomationType at = atrack->automationType();
   const bool no_auto = !MusEGlobal::automation || at == AUTO_OFF;
   CtrlListList *cll = atrack->controller();
   ciCtrlList icl_first;
   const int plug_id = id();

   //set freewheeling property if plugin supports it
   if(_synth->_hasFreeWheelPort)
   {
      _controls [_synth->_freeWheelPortIndex].val = MusEGlobal::audio->freewheel() ? 1.0f : 0.0f;
   }

   if(plug_id != -1 && ports != 0)  // Don't bother if not 'running'.
   {
      icl_first = cll->lower_bound(genACnum(plug_id, 0));
   }

   if(ports != 0)
   {

      if(!atrack->noInRoute())
      {
         RouteList *irl = atrack->inRoutes();

         for(iRoute i = irl->begin(); i != irl->end(); ++i)
         {
            if(!i->track->isMidiTrack())
            {
               const int ch     = i->channel       == -1 ? 0 : i->channel;
               const int remch  = i->remoteChannel == -1 ? 0 : i->remoteChannel;
               const int chs    = i->channels      == -1 ? 0 : i->channels;
               assert((unsigned)remch <= _inports);

               if((unsigned)ch < _inports && (unsigned)(ch + chs) <= _inports)
               {
                  const bool u1 = _iUsedIdx[remch];

                  if(chs >= 2)
                  {
                     const bool u2 = _iUsedIdx[remch + 1];

                     if(u1 && u2)
                     {
                        ((AudioTrack *)i->track)->addData(pos, chs, ch, -1, nframes, &_audioInBuffers[remch]);
                     }
                     else if(!u1 && !u2)
                     {
                        ((AudioTrack *)i->track)->copyData(pos, chs, ch, -1, nframes, &_audioInBuffers[remch]);
                     }
                     else
                     {
                        if(u1)
                        {
                           ((AudioTrack *)i->track)->addData(pos, 1, ch, 1, nframes, &_audioInBuffers[remch]);
                        }
                        else
                        {
                           ((AudioTrack *)i->track)->copyData(pos, 1, ch, 1, nframes, &_audioInBuffers[remch]);
                        }

                        if(u2)
                        {
                           ((AudioTrack *)i->track)->addData(pos, 1, ch + 1, 1, nframes, &_audioInBuffers[remch + 1]);
                        }
                        else
                        {
                           ((AudioTrack *)i->track)->copyData(pos, 1, ch + 1, 1, nframes, &_audioInBuffers[remch + 1]);
                        }
                     }
                  }
                  else
                  {
                     if(u1)
                     {
                        ((AudioTrack *)i->track)->addData(pos, 1, ch, -1, nframes, &_audioInBuffers[remch]);
                     }
                     else
                     {
                        ((AudioTrack *)i->track)->copyData(pos, 1, ch, -1, nframes, &_audioInBuffers[remch]);
                     }
                  }

                  const int h = remch + chs;

                  for(int j = remch; j < h; ++j)
                  {
                     _iUsedIdx[j] = true;
                  }
               }

            }
         }
      }


   }


   int cur_slice = 0;

   while(sample < nframes)
   {
      unsigned long nsamp = nframes - sample;
      const unsigned long slice_frame = pos + sample;

      //
      // Process automation control values, while also determining the maximum acceptable
      //  size of this run. Further processing, from FIFOs for example, can lower the size
      //  from there, but this section determines where the next highest maximum frame
      //  absolutely needs to be for smooth playback of the controller value stream...
      //
      if(ports != 0)    // Don't bother if not 'running'.
      {
         ciCtrlList icl = icl_first;

         for(unsigned long k = 0; k < _inportsControl; ++k)
         {            
            //don't process freewheel port
            if(_synth->_hasFreeWheelPort && _synth->_freeWheelPortIndex == k)
               continue;

            CtrlList *cl = (cll && plug_id != -1 && icl != cll->end()) ? icl->second : NULL;
            CtrlInterpolate &ci = _controls[k].interp;

            // Always refresh the interpolate struct at first, since things may have changed.
            // Or if the frame is outside of the interpolate range - and eStop is not true.  // FIXME TODO: Be sure these comparisons are correct.
            if(cur_slice == 0 || (!ci.eStop && MusEGlobal::audio->isPlaying() &&
                                  (slice_frame < (unsigned long)ci.sFrame || (ci.eFrame != -1 && slice_frame >= (unsigned long)ci.eFrame))))
            {
               if(cl && plug_id != -1 && (unsigned long)cl->id() == genACnum(plug_id, k))
               {
                  cl->getInterpolation(slice_frame, no_auto || !_controls[k].enCtrl, &ci);

                  if(icl != cll->end())
                  {
                     ++icl;
                  }
               }
               else
               {
                  // No matching controller, or end. Just copy the current value into the interpolator.
                  // Keep the current icl iterator, because since they are sorted by frames,
                  //  if the IDs didn't match it means we can just let k catch up with icl.
                  ci.sFrame   = 0;
                  ci.eFrame   = -1;
                  ci.sVal     = _controls[k].val;
                  ci.eVal     = ci.sVal;
                  ci.doInterp = false;
                  ci.eStop    = false;
               }
            }
            else
            {
               if(ci.eStop && ci.eFrame != -1 && slice_frame >= (unsigned long)ci.eFrame)  // FIXME TODO: Get that comparison right.
               {
                  // Clear the stop condition and set up the interp struct appropriately as an endless value.
                  ci.sFrame   = 0; //ci->eFrame;
                  ci.eFrame   = -1;
                  ci.sVal     = ci.eVal;
                  ci.doInterp = false;
                  ci.eStop    = false;
               }

               if(cl && cll && icl != cll->end())
               {
                  ++icl;
               }
            }

            if(MusEGlobal::audio->isPlaying())
            {
               unsigned long samps = nsamp;

               if(ci.eFrame != -1)
               {
                  samps = (unsigned long)ci.eFrame - slice_frame;
               }

               if(!ci.doInterp && samps > min_per)
               {
                  samps &= ~min_per_mask;

                  if((samps & min_per_mask) != 0)
                  {
                     samps += min_per;
                  }
               }
               else
               {
                  samps = min_per;
               }

               if(samps < nsamp)
               {
                  nsamp = samps;
               }

            }

            if(ci.doInterp && cl)
            {
               _controls[k].val = cl->interpolate(MusEGlobal::audio->isPlaying() ? slice_frame : pos, ci);
            }
            else
            {
               _controls[k].val = ci.sVal;
            }
            _uiState->controlsMask [k] = true;

#ifdef LV2_DEBUG_PROCESS
            fprintf(stderr, "LV2SynthIF::getData k:%lu val:%f sample:%lu ci.eFrame:%d nsamp:%lu \n", k, _controls[k].val, sample, ci.eFrame, nsamp);
#endif

         }
      }


      bool found = false;
      unsigned long frame = 0;
      unsigned long index = 0;

      // Get all control ring buffer items valid for this time period...
      while(!_controlFifo.isEmpty())
      {
         unsigned long evframe;
         ControlEvent v = _controlFifo.peek();
         // The events happened in the last period or even before that. Shift into this period with + n. This will sync with audio.
         // If the events happened even before current frame - n, make sure they are counted immediately as zero-frame.
         evframe = (syncFrame > v.frame + nframes) ? 0 : v.frame - syncFrame + nframes;

#ifdef DEBUG_LV2
         fprintf(stderr, "LV2SynthIF::getData found:%d evframe:%lu frame:%lu  event frame:%lu idx:%lu val:%f unique:%d\n",
                 found, evframe, frame, v.frame, v.idx, v.value, v.unique);
#endif

         // Protection. Observed this condition. Why? Supposed to be linear timestamps.
         if(found && evframe < frame)
         {
            fprintf(stderr, "LV2SynthIF::getData *** Error: evframe:%lu < frame:%lu event: frame:%lu idx:%lu val:%f unique:%d\n",
                    evframe, frame, v.frame, v.idx, v.value, v.unique);

            // No choice but to ignore it.
            _controlFifo.remove();               // Done with the ring buffer's item. Remove it.
            continue;
         }

         if(evframe >= nframes                                                         // Next events are for a later period.
               || (!found && !v.unique && (evframe - sample >= nsamp))  // Next events are for a later run in this period. (Autom took prio.)
               || (found && !v.unique && (evframe - sample >= min_per)))                  // Eat up events within minimum slice - they're too close.

         {
            break;
         }

         _controlFifo.remove();               // Done with the ring buffer's item. Remove it.

         found = true;
         frame = evframe;
         index = v.idx;

         if(index >= _inportsControl) // Sanity check.
         {
            break;
         }

         //don't process freewheel port
         if(_synth->_hasFreeWheelPort && _synth->_freeWheelPortIndex == index)
            continue;

         if(ports == 0)                     // Don't bother if not 'running'.
         {
            _controls[index].val = v.value;   // Might as well at least update these.
         }
         else
         {
            CtrlInterpolate *ci = &_controls[index].interp;
            // Tell it to stop the current ramp at this frame, when it does stop, set this value:
            ci->eFrame = frame;
            ci->eVal   = v.value;
            ci->eStop  = true;
         }

         // Need to update the automation value, otherwise it overwrites later with the last automation value.
         if(plug_id != -1)
         {
            synti->setPluginCtrlVal(genACnum(plug_id, index), v.value);
         }
         if(v.fromGui) //don't send gui control changes back
         {
            _uiState->lastControls [index] = v.value;
            _uiState->controlsMask [index] = false;
         }
      }

      if(found)  // If a control FIFO item was found, takes priority over automation controller stream.
      {
         nsamp = frame - sample;
      }


      if(sample + nsamp > nframes)         // Safety check.
      {
         nsamp = nframes - sample;
      }

      // TODO: Don't allow zero-length runs. This could/should be checked in the control loop instead.
      // Note this means it is still possible to get stuck in the top loop (at least for a while).
      if(nsamp != 0)
      {
         unsigned long nevents = 0;

         if(ports != 0)  // Don't bother if not 'running'.
         {
            // Process event list events...
            for(; start_event != el->end(); ++start_event)
            {
#ifdef LV2_DEBUG
               fprintf(stderr, "LV2SynthIF::getData eventlist event time:%d pos:%u sample:%lu nsamp:%lu frameOffset:%d\n", start_event->time(), pos, sample, nsamp, frameOffset);
#endif

               if(start_event->time() >= (pos + sample + nsamp + frameOffset))  // frameOffset? Test again...
               {
#ifdef LV2_DEBUG
                  fprintf(stderr, " event is for future:%lu, breaking loop now\n", start_event->time() - frameOffset - pos - sample);
#endif
                  break;
               }

               // Update hardware state so knobs and boxes are updated. Optimize to avoid re-setting existing values.
               // Same code as in MidiPort::sendEvent()
               if(synti->midiPort() != -1)
               {
                  MidiPort *mp = &MusEGlobal::midiPorts[synti->midiPort()];

                  if(start_event->type() == ME_CONTROLLER)
                  {
                     int da = start_event->dataA();
                     int db = start_event->dataB();
                     if(da != CTRL_PROGRAM) //for programs setHwCtrlState is called from processEvent
                     {
                        db = mp->limitValToInstrCtlRange(da, db);

                        if(!mp->setHwCtrlState(start_event->channel(), da, db))
                        {
                           continue;
                        }
                     }
                  }
                  else if(start_event->type() == ME_PITCHBEND)
                  {
                     int da = mp->limitValToInstrCtlRange(CTRL_PITCH, start_event->dataA());

                     if(!mp->setHwCtrlState(start_event->channel(), CTRL_PITCH, da))
                     {
                        continue;
                     }
                  }
                  else if(start_event->type() == ME_AFTERTOUCH)
                  {
                     int da = mp->limitValToInstrCtlRange(CTRL_AFTERTOUCH, start_event->dataA());

                     if(!mp->setHwCtrlState(start_event->channel(), CTRL_AFTERTOUCH, da))
                     {
                        continue;
                     }
                  }
                  else if(start_event->type() == ME_POLYAFTER)
                  {
                     int ctl = (CTRL_POLYAFTER & ~0xff) | (start_event->dataA() & 0x7f);
                     int db = mp->limitValToInstrCtlRange(ctl, start_event->dataB());

                     if(!mp->setHwCtrlState(start_event->channel(), ctl , db))
                     {
                        continue;
                     }
                  }
//                  else if(start_event->type() == ME_PROGRAM)
//                  {
//                     if(!mp->setHwCtrlState(start_event->channel(), CTRL_PROGRAM, start_event->dataA()))
//                     {
//                        continue;
//                     }
//                  }
               }

               // Returns false if the event was not filled. It was handled, but some other way.
               if(processEvent(*start_event, &events[nevents]))
               {
                  // Time-stamp the event.
                  int ft = start_event->time() - frameOffset - pos - sample;

                  if(ft < 0)
                  {
                     ft = 0;
                  }

                  if(ft >= int(nsamp))
                  {
                     fprintf(stderr, "LV2SynthIF::getData: eventlist event time:%u out of range. pos:%u offset:%d ft:%d sample:%lu nsamp:%lu\n", start_event->time(), pos, frameOffset, ft, sample, nsamp);
                     ft = nsamp - 1;
                  }

#ifdef LV2_DEBUG
                  fprintf(stderr, "LV2SynthIF::getData eventlist: ft:%d current nevents:%lu\n", ft, nevents);
#endif

                  // "Each event is timestamped relative to the start of the block, (mis)using the ALSA "tick time" field as a frame count.
                  //  The host is responsible for ensuring that events with differing timestamps are already ordered by time."  -  From dssi.h
                  events[nevents].time.tick = ft;

                  ++nevents;
               }
            }
         }

         // Now process putEvent events...
         while(!synti->eventFifo.isEmpty())
         {
            MidiPlayEvent e = synti->eventFifo.peek();

#ifdef LV2_DEBUG
            fprintf(stderr, "LV2SynthIF::getData eventFifo event time:%d\n", e.time());
#endif

            if(e.time() >= (pos + sample + nsamp + frameOffset))
            {
               break;
            }

            synti->eventFifo.remove();    // Done with ring buffer's event. Remove it.

            if(ports != 0)  // Don't bother if not 'running'.
            {
               // Returns false if the event was not filled. It was handled, but some other way.
               if(processEvent(e, &events[nevents]))
               {
                  // Time-stamp the event.
                  int ft = e.time() - frameOffset - pos  - sample;

                  if(ft < 0)
                  {
                     ft = 0;
                  }

                  if(ft >= int(nsamp))
                  {
                     fprintf(stderr, "LV2SynthIF::getData: eventFifo event time:%u out of range. pos:%u offset:%d ft:%d sample:%lu nsamp:%lu\n", e.time(), pos, frameOffset, ft, sample, nsamp);
                     ft = nsamp - 1;
                  }

                  // "Each event is timestamped relative to the start of the block, (mis)using the ALSA "tick time" field as a frame count.
                  //  The host is responsible for ensuring that events with differing timestamps are already ordered by time."  -  From dssi.h
                  events[nevents].time.tick = ft;

                  ++nevents;
               }
            }
         }

         if(ports != 0)  // Don't bother if not 'running'.
         {
            LV2Synth::lv2audio_preProcessMidiPorts(_uiState, nsamp, events, nevents);

            //connect ports
            for(size_t j = 0; j < _inports; ++j)
            {
               if(_iUsedIdx [j])
               {
                  lilv_instance_connect_port(_handle, _audioInPorts [j].index, _audioInBuffers [j] + sample);
               }
               else
               {
                  lilv_instance_connect_port(_handle, _audioInPorts [j].index, _audioInSilenceBuf + sample);
               }

               _iUsedIdx[j] = false;
            }

            for(size_t j = 0; j < nop; ++j)
            {
               lilv_instance_connect_port(_handle, _audioOutPorts [j].index, buffer [j] + sample);
            }

            for(size_t j = nop; j < _outports; j++)
            {
               lilv_instance_connect_port(_handle, _audioOutPorts [j].index, _audioOutBuffers [j] + sample);
            }

            for(size_t j = 0; j < _inportsControl; ++j)
            {
               uint32_t idx = _controlInPorts [j].index;
               if(_uiState->pluginCVPorts [idx] != NULL)
               {
                  float cvVal = _controls [j].val;
                  for(size_t jj = 0; jj < nsamp; ++jj)
                  {
                     _uiState->pluginCVPorts [idx] [jj + sample] = cvVal;
                  }
                  lilv_instance_connect_port(_handle, idx, _uiState->pluginCVPorts [idx] + sample);
               }
            }

            lilv_instance_run(_handle, nsamp);
            //notify worker that this run() finished
            if(_uiState->wrkIface && _uiState->wrkIface->end_run)
               _uiState->wrkIface->end_run(lilv_instance_get_handle(_handle));
            //notify worker about processes data (if any)
            if(_uiState->wrkIface && _uiState->wrkIface->work_response && _uiState->wrkEndWork)
            {
               _uiState->wrkIface->work_response(lilv_instance_get_handle(_handle), _uiState->wrkDataSize, _uiState->wrkDataBuffer);
               _uiState->wrkDataSize = 0;
               _uiState->wrkDataBuffer = NULL;
               _uiState->wrkEndWork = false;
            }

            LV2Synth::lv2audio_postProcessMidiPorts(_uiState, nsamp);


         }

         sample += nsamp;
      }

      ++cur_slice; // Slice is done. Moving on to any next slice now...
   }




   return start_event;
}


void LV2SynthIF::getGeometry(int *x, int *y, int *w, int *h) const
{
   *x = *y = *w = *h = 0;



   return;
}

void LV2SynthIF::getNativeGeometry(int *x, int *y, int *w, int *h) const
{
   *x = *y = *w = *h = 0;
   return;
}

float LV2SynthIF::getParameter(long unsigned int n) const
{
   if(n >= _inportsControl)
   {
      std::cout << "LV2SynthIF::getParameter param number " << n << " out of range of ports: " << _inportsControl << std::endl;
      return 0.0;
   }

   if(!_controls)
   {
      return 0.0;
   }

   return _controls[n].val;
}

float LV2SynthIF::getParameterOut(long unsigned int n) const
{
   if(n >= _outportsControl)
   {
      std::cout << "LV2SynthIF::getParameterOut param number " << n << " out of range of ports: " << _outportsControl << std::endl;
      return 0.0;
   }

   if(!_controlsOut)
   {
      return 0.0;
   }

   return _controlsOut[n].val;

}


QString LV2SynthIF::getPatchName(int /* ch */, int prog, bool) const
{
//   lv2ExtProgram extPrg;
//   extPrg.index = 0;
//   extPrg.useIndex = false;
//   extPrg.bank = (prog >> 8) & 0xff;
//   extPrg.prog = prog & 0xff;
   std::map<uint32_t, uint32_t>::iterator itPrg = _uiState->prg2index.find(prog);
   if(itPrg == _uiState->prg2index.end())
      return QString("?");
   uint32_t index = itPrg->second;
   std::map<uint32_t, lv2ExtProgram>::iterator itIndex = _uiState->index2prg.find(index);
   if(itIndex == _uiState->index2prg.end())
      return QString("?");
   return QString(itIndex->second.name);

}

void LV2SynthIF::guiHeartBeat()
{

}


bool LV2SynthIF::guiVisible() const
{
   return _gui && _gui->isVisible();
}


bool LV2SynthIF::hasGui() const
{
   return true;
}

bool LV2SynthIF::hasNativeGui() const
{
   return (_synth->_pluginUiTypes.size() > 0);
}

bool LV2SynthIF::initGui()
{
   //TODO: implement this
   return true;
}

bool LV2SynthIF::nativeGuiVisible() const
{
   if(_uiState != NULL)
   {
      if(_uiState->hasExternalGui)
      {
         return (_uiState->widget != NULL);
      }
      else if(_uiState->hasGui && _uiState->widget != NULL)
      {
         return ((QWidget *)_uiState->widget)->isVisible();
      }
   }

   return false;
}

void LV2SynthIF::populatePatchPopup(MusEGui::PopupMenu *menu, int, bool)
{
   LV2Synth::lv2prg_updatePrograms(_uiState);
   menu->clear();
   std::map<int, MusEGui::PopupMenu *> submenus;
   std::map<uint32_t, lv2ExtProgram>::iterator itIndex;
   for(itIndex = _uiState->index2prg.begin(); itIndex != _uiState->index2prg.end(); ++itIndex)
   {
      const lv2ExtProgram &extPrg = itIndex->second;
      //truncating bank and brogran numbers to 16 bit - muse MidiPlayEvent can handle only 32 bit numbers
      int bank = extPrg.bank;
      int prog = extPrg.prog;
      int id = ((bank & 0xff) << 8) + prog;
      std::map<int, MusEGui::PopupMenu *>::iterator itS = submenus.find(bank);
      MusEGui::PopupMenu *submenu= NULL;
      if(itS != submenus.end())
      {
          submenu = itS->second;
      }
      else
      {
          submenu = new MusEGui::PopupMenu(menu->parent());
          submenu->setTitle(QString("Bank #") + QString::number(bank + 1));
          menu->addMenu(submenu);
          submenus.insert(std::make_pair<int, MusEGui::PopupMenu *>(bank, submenu));

      }

      QAction *act = submenu->addAction(extPrg.name);
      act->setData(id);

   }
}

void LV2SynthIF::preProcessAlways()
{

}

bool LV2SynthIF::putEvent(const MidiPlayEvent &ev)
{
#ifdef DEBUG_LV2
   fprintf(stderr, "LV2SynthIF::putEvent midi event time:%u chn:%d a:%d b:%d\n", ev.time(), ev.channel(), ev.dataA(), ev.dataB());
#endif

   if(MusEGlobal::midiOutputTrace)
   {
      ev.dump();
   }

   return synti->eventFifo.put(ev);
}

MidiPlayEvent LV2SynthIF::receiveEvent()
{
   return MidiPlayEvent();

}

void LV2SynthIF::setGeometry(int , int , int , int)
{
   //TODO: implement this

}

void LV2SynthIF::setNativeGeometry(int , int , int , int)
{
   //TODO: implement this

}

void LV2SynthIF::setParameter(long unsigned int idx, float value)
{
   addScheduledControlEvent(idx, value, MusEGlobal::audio->curFrame());
}

void LV2SynthIF::showGui(bool v)
{
   if (v)
   {
      if (_gui == 0)
         makeGui();
      _gui->show();
   }
   else
   {
      if (_gui)
         _gui->hide();
   }
}

void LV2SynthIF::showNativeGui(bool bShow)
{
   if(track() != NULL)
   {
      if(_uiState->human_id != NULL)
      {
         free(_uiState->human_id);
      }

      _uiState->extHost.plugin_human_id = _uiState->human_id = strdup((track()->name() + QString(": ") + name()).toUtf8().constData());
   }

   LV2Synth::lv2ui_ShowNativeGui(_uiState, bShow);
}

void LV2SynthIF::write(int level, Xml &xml) const
{
   LV2Synth::lv2conf_write(_uiState, level, xml);
}

void LV2SynthIF::setCustomData(const std::vector< QString > &customParams)
{
   LV2Synth::lv2conf_set(_uiState, customParams);
}


float LV2SynthIF::param(long unsigned int i) const
{
   return getParameter(i);
}

long unsigned int LV2SynthIF::parameters() const
{
   return _inportsControl;
}

long unsigned int LV2SynthIF::parametersOut() const
{
   return _outportsControl;
}

const char *LV2SynthIF::paramName(long unsigned int i)
{
   return _controlInPorts [i].cName;
}

const char *LV2SynthIF::paramOutName(long unsigned int i)
{
   return _controlOutPorts [i].cName;
}

CtrlValueType LV2SynthIF::ctrlValueType(unsigned long i) const
{
   CtrlValueType vt = VAL_LINEAR;
   std::map<uint32_t, uint32_t>::iterator it = _synth->_idxToControlMap.find(i);
   assert(it != _synth->_idxToControlMap.end());
   i = it->second;
   assert(i < _inportsControl);

   switch(_synth->_controlInPorts [i].cType)
   {
   case LV2_PORT_CONTINUOUS:
      vt = VAL_LINEAR;
      break;
   case LV2_PORT_DISCRETE:
   case LV2_PORT_INTEGER:
      vt = VAL_INT;
      break;
   case LV2_PORT_LOGARITHMIC:
      vt = VAL_LOG;
      break;
   case LV2_PORT_TRIGGER:
      vt = VAL_BOOL;
      break;
   default:
      break;
   }

   return vt;

}

CtrlList::Mode LV2SynthIF::ctrlMode(unsigned long i) const
{
   std::map<uint32_t, uint32_t>::iterator it = _synth->_idxToControlMap.find(i);
   assert(it != _synth->_idxToControlMap.end());
   i = it->second;
   assert(i < _inportsControl);

   return ((_synth->_controlInPorts [i].cType == LV2_PORT_CONTINUOUS)
            ||(_synth->_controlInPorts [i].cType == LV2_PORT_LOGARITHMIC)) ? CtrlList::INTERPOLATE : CtrlList::DISCRETE;
}

LADSPA_PortRangeHint LV2SynthIF::range(unsigned long i)
{
   assert(i < _inportsControl);
   LADSPA_PortRangeHint hint;
   hint.HintDescriptor = 0;
   hint.LowerBound = _controlInPorts [i].minVal;
   hint.UpperBound = _controlInPorts [i].maxVal;

   if(hint.LowerBound == hint.LowerBound)
   {
      hint.HintDescriptor |= LADSPA_HINT_BOUNDED_BELOW;
   }

   if(hint.UpperBound == hint.UpperBound)
   {
      hint.HintDescriptor |= LADSPA_HINT_BOUNDED_ABOVE;
   }

   return hint;
}

LADSPA_PortRangeHint LV2SynthIF::rangeOut(unsigned long i)
{
   assert(i < _outportsControl);
   LADSPA_PortRangeHint hint;
   hint.HintDescriptor = 0;
   hint.LowerBound = _controlOutPorts [i].minVal;
   hint.UpperBound = _controlOutPorts [i].maxVal;

   if(hint.LowerBound == hint.LowerBound)
   {
      hint.HintDescriptor |= LADSPA_HINT_BOUNDED_BELOW;
   }

   if(hint.UpperBound == hint.UpperBound)
   {
      hint.HintDescriptor |= LADSPA_HINT_BOUNDED_ABOVE;
   }

   return hint;

}



float LV2SynthIF::paramOut(long unsigned int i) const
{
   return getParameterOut(i);
}

void LV2SynthIF::setParam(long unsigned int i, float val)
{
   setParameter(i, val);
}

void LV2SynthIF::enableController(unsigned long i, bool v)  { _controls[i].enCtrl = v; }
bool LV2SynthIF::controllerEnabled(unsigned long i) const   { return _controls[i].enCtrl; }
void LV2SynthIF::enableAllControllers(bool v)
{
  if(!_synth)
    return;
  for(unsigned long i = 0; i < _inportsControl; ++i)
    _controls[i].enCtrl = v;
}
void LV2SynthIF::updateControllers() { }

void LV2SynthIF::populatePresetsMenu(QMenu *menu)
{
   LV2Synth::lv2state_populatePresetsMenu(_uiState, menu);
}

void LV2SynthIF::applyPreset(void *preset)
{
   LV2Synth::lv2state_applyPreset(_uiState, static_cast<LilvNode *>(preset));
}



void LV2SynthIF::writeConfiguration(int level, Xml &xml)
{
   MusECore::SynthIF::writeConfiguration(level, xml);
}

bool LV2SynthIF::readConfiguration(Xml &xml, bool readPreset)
{
   return MusECore::SynthIF::readConfiguration(xml, readPreset);
}

void LV2PluginWrapper_Window::closeEvent(QCloseEvent *event)
{
   assert(_state != NULL);
   event->accept();

   stopUpdateTimer();

   if(_state->deleteLater)
   {
      LV2Synth::lv2state_FreeState(_state);

   }
   else
   {
      //_state->uiTimer->stopNextTime(false);
      _state->widget = NULL;
      _state->pluginWindow = NULL;
      _state->uiDoSelectPrg = false;
      _state->uiPrgIface = NULL;

      LV2Synth::lv2ui_FreeDescriptors(_state);
   }

   delete this;

}

void LV2PluginWrapper_Window::stopUpdateTimer()
{
   if(updateTimer.isActive())
      updateTimer.stop();
   while(updateTimer.isActive())
   {
      QCoreApplication::processEvents();
   }
}


LV2PluginWrapper_Window::LV2PluginWrapper_Window(LV2PluginWrapper_State *state)
 : QMainWindow(), _state ( state ), _closing(false)
{
   connect(&updateTimer, SIGNAL(timeout()), this, SLOT(updateGui()));
}

void LV2PluginWrapper_Window::startNextTime()
{
   stopUpdateTimer();
   updateTimer.start(1000/30);
}




void LV2PluginWrapper_Window::stopNextTime()
{
   setClosing(true);
   stopUpdateTimer();
   close();
}

void LV2PluginWrapper_Window::updateGui()
{
   if(_state->deleteLater || _closing)
   {
      stopNextTime();
      return;
   }
   LV2Synth::lv2ui_SendChangedControls(_state);

   //send program change if any
   if(_state->uiDoSelectPrg)
   {
      _state->uiDoSelectPrg = false;
      if(_state->uiPrgIface != NULL && (_state->uiPrgIface->select_program != NULL || _state->uiPrgIface->select_program_for_channel != NULL))
      {
         if(_state->newPrgIface)
            _state->uiPrgIface->select_program_for_channel(lilv_instance_get_handle(_state->handle), _state->uiChannel, (uint32_t)_state->uiBank, (uint32_t)_state->uiProg);
         else
            _state->uiPrgIface->select_program(lilv_instance_get_handle(_state->handle), (uint32_t)_state->uiBank, (uint32_t)_state->uiProg);
      }
   }

   //call ui idle callback if any
   if(_state->uiIdleIface != NULL)
   {
      int iRet = _state->uiIdleIface->idle(_state->uiInst);
      if(iRet != 0) // ui don't want us to call it's idle callback any more
         _state->uiIdleIface = NULL;
   }

   if(_state->hasExternalGui)
   {
      LV2_EXTERNAL_UI_RUN((LV2_External_UI_Widget *)_state->widget);
   }

   //if(_closing)
      //stopNextTime();
}


LV2PluginWrapper::LV2PluginWrapper(LV2Synth *s)
{
   _synth = s;

   _fakeLd.Label = _synth->name().toUtf8().constData();
   _fakeLd.Name = _synth->name().toUtf8().constData();
   _fakeLd.UniqueID = _synth->_uniqueID;
   _fakeLd.Maker = _synth->maker().toUtf8().constData();
   _fakeLd.Copyright = _synth->version().toUtf8().constData();
   _isLV2Plugin = true;
   _isLV2Synth = s->_isSynth;
   int numPorts = _synth->_audioInPorts.size()
                  + _synth->_audioOutPorts.size()
                  + _synth->_controlInPorts.size()
                  + _synth->_controlOutPorts.size()
                  + _synth->_midiInPorts.size()
                  + _synth->_midiOutPorts.size();
   _fakeLd.PortCount = numPorts;
   _fakePds = new LADSPA_PortDescriptor [numPorts];
   memset(_fakePds, 0, sizeof(int) * numPorts);

   for(size_t i = 0; i < _synth->_audioInPorts.size(); i++)
   {
      _fakePds [_synth->_audioInPorts [i].index] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
   }

   for(size_t i = 0; i < _synth->_audioOutPorts.size(); i++)
   {
      _fakePds [_synth->_audioOutPorts [i].index] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
   }

   for(size_t i = 0; i < _synth->_controlInPorts.size(); i++)
   {
      _fakePds [_synth->_controlInPorts [i].index] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
   }

   for(size_t i = 0; i < _synth->_controlOutPorts.size(); i++)
   {
      _fakePds [_synth->_controlOutPorts [i].index] = LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL;
   }   

   _fakeLd.PortNames = NULL;
   _fakeLd.PortRangeHints = NULL;
   _fakeLd.PortDescriptors = _fakePds;
   _fakeLd.Properties = 0;
   plugin = &_fakeLd;
   _isDssi = false;
   _isDssiSynth = false;

#ifdef DSSI_SUPPORT
   dssi_descr = NULL;
#endif

   fi = _synth->info;
   ladspa = NULL;
   _handle = 0;
   _references = 0;
   _instNo     = 0;
   _label = _synth->name();
   _name = _synth->description();
   _uniqueID = plugin->UniqueID;
   _maker = _synth->maker();
   _copyright = _synth->version();

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
         {
            ++_inports;
         }
         else if(pd & LADSPA_PORT_OUTPUT)
         {
            ++_outports;
         }
      }
      else if(pd & LADSPA_PORT_CONTROL)
      {
         if(pd & LADSPA_PORT_INPUT)
         {
            ++_controlInPorts;
         }
         else if(pd & LADSPA_PORT_OUTPUT)
         {
            ++_controlOutPorts;
         }
      }
   }

   _inPlaceCapable = !LADSPA_IS_INPLACE_BROKEN(plugin->Properties);
}

LV2PluginWrapper::~LV2PluginWrapper()
{
   delete [] _fakePds;
}

LADSPA_Handle LV2PluginWrapper::instantiate(PluginI *plugi)
{
   LV2PluginWrapper_State *state = new LV2PluginWrapper_State;
   state->inst = this;
   state->widget = NULL;
   state->uiInst = NULL;
   state->plugInst = plugi;
   state->_ifeatures = new LV2_Feature[SIZEOF_ARRAY(lv2Features)];
   state->_ppifeatures = new LV2_Feature *[SIZEOF_ARRAY(lv2Features) + 1];
   state->sif = NULL;
   state->synth = _synth;
   LV2Synth::lv2state_FillFeatures(state);

   state->handle = lilv_plugin_instantiate(_synth->_handle, (double)MusEGlobal::sampleRate, state->_ppifeatures);

   if(state->handle == NULL)
   {
      delete [] state->_ppifeatures;
      delete [] state->_ifeatures;
      return NULL;
   }   

   state->controlsNameMap.clear();

   if(_controlInPorts > 0)
   {
      state->lastControls = new float [_controlInPorts];
      state->controlsMask = new bool [_controlInPorts];
      state->controlTimers = new int [_controlInPorts];
      for(uint32_t i = 0; i < _controlInPorts; i++)
      {
         state->lastControls [i] = _synth->_pluginControlsDefault [_synth->_controlInPorts [i].index];
         state->controlsMask [i] = false;
         state->controlTimers [i] = 0;
         state->controlsNameMap.insert(std::pair<QString, size_t>(QString(_synth->_controlInPorts [i].cName).toLower(), i));
         state->controlsNameMap.insert(std::pair<QString, size_t>(QString(_synth->_controlInPorts [i].cSym).toLower(), i));
      }
   }

   if(_controlOutPorts > 0)
   {
      state->lastControlsOut = new float [_controlOutPorts];
      for(uint32_t i = 0; i < _controlOutPorts; i++)
      {
         state->lastControlsOut [i] = _synth->_pluginControlsDefault [_synth->_controlOutPorts [i].index];
      }
   }

   _states.insert(std::pair<void *, LV2PluginWrapper_State *>(state->handle, state));

   LV2Synth::lv2state_PostInstantiate(state);

   return (LADSPA_Handle)state->handle;

}

void LV2PluginWrapper::connectPort(LADSPA_Handle handle, long unsigned int port, float *value)
{
   lilv_instance_connect_port((LilvInstance *)handle, port, (void *)value);
}

int LV2PluginWrapper::incReferences(int ref)
{
   _synth->incInstances(ref);
   return _synth->instances();
}
void LV2PluginWrapper::activate(LADSPA_Handle handle)
{
   lilv_instance_activate((LilvInstance *) handle);
}
void LV2PluginWrapper::deactivate(LADSPA_Handle handle)
{
  if (handle)
  {
    lilv_instance_deactivate((LilvInstance *) handle);
  }
}
void LV2PluginWrapper::cleanup(LADSPA_Handle handle)
{
   if(handle != NULL)
   {
      std::map<void *, LV2PluginWrapper_State *>::iterator it = _states.find(handle);
      assert(it != _states.end()); //this shouldn't happen
      LV2PluginWrapper_State *state = it->second;
      _states.erase(it);

      state->deleteLater = true;
      if(state->pluginWindow != NULL)
         state->pluginWindow->stopNextTime();
      else
         LV2Synth::lv2state_FreeState(state);

   }
}

void LV2PluginWrapper::apply(LADSPA_Handle handle, unsigned long n)
{
   std::map<void *, LV2PluginWrapper_State *>::iterator it = _states.find(handle);
   assert(it != _states.end()); //this shouldn't happen
   LV2PluginWrapper_State *state = it->second;

   LV2Synth::lv2audio_preProcessMidiPorts(state, n);

   //set freewheeling property if plugin supports it
   if(state->synth->_hasFreeWheelPort)
   {
      state->plugInst->controls[_synth->_freeWheelPortIndex].val = MusEGlobal::audio->freewheel() ? 1.0f : 0.0f;
   }

   for(size_t j = 0; j < state->plugInst->controlPorts; ++j)
   {
      uint32_t idx = state->synth->_controlInPorts [j].index;
      if(state->pluginCVPorts [idx] != NULL)
      {
         float cvVal = state->plugInst->controls [j].val;
         for(size_t jj = 0; jj < n; ++jj)
         {
            state->pluginCVPorts [idx] [jj] = cvVal;
         }
         lilv_instance_connect_port(state->handle, idx, state->pluginCVPorts [idx]);
      }
   }

   for(size_t j = 0; j < state->plugInst->controlOutPorts; ++j)
   {
      uint32_t idx = state->synth->_controlOutPorts [j].index;
      if(state->pluginCVPorts [idx] != NULL)
      {
         float cvVal = state->plugInst->controlsOut [j].val;
         for(size_t jj = 0; jj < n; ++jj)
         {
            state->pluginCVPorts [idx] [jj] = cvVal;
         }
         lilv_instance_connect_port(state->handle, idx, state->pluginCVPorts [idx]);
      }
   }


   lilv_instance_run(state->handle, n);
   //notify worker that this run() finished
   if(state->wrkIface && state->wrkIface->end_run)
      state->wrkIface->end_run(lilv_instance_get_handle(state->handle));
   //notify worker about processes data (if any)
   if(state->wrkIface && state->wrkIface->work_response && state->wrkEndWork)
   {
      state->wrkEndWork = false;
      state->wrkIface->work_response(lilv_instance_get_handle(state->handle), state->wrkDataSize, state->wrkDataBuffer);
      state->wrkDataSize = 0;
      state->wrkDataBuffer = NULL;
   }

   LV2Synth::lv2audio_postProcessMidiPorts(state, n);
}
LADSPA_PortDescriptor LV2PluginWrapper::portd(unsigned long k) const
{
   return _fakeLd.PortDescriptors[k];
}

LADSPA_PortRangeHint LV2PluginWrapper::range(unsigned long i)
{
   LADSPA_PortRangeHint hint;
   hint.HintDescriptor = 0;
   hint.LowerBound = _synth->_pluginControlsMin [i];
   hint.UpperBound = _synth->_pluginControlsMax [i];

   if(hint.LowerBound == hint.LowerBound)
   {
      hint.HintDescriptor |= LADSPA_HINT_BOUNDED_BELOW;
   }

   if(hint.UpperBound == hint.UpperBound)
   {
      hint.HintDescriptor |= LADSPA_HINT_BOUNDED_ABOVE;
   }

   return hint;
}
void LV2PluginWrapper::range(unsigned long i, float *min, float *max) const
{
   *min = _synth->_pluginControlsMin [i];
   *max = _synth->_pluginControlsMax [i];
}

float LV2PluginWrapper::defaultValue(unsigned long port) const
{
   return _synth->_pluginControlsDefault [port];
}
const char *LV2PluginWrapper::portName(unsigned long i)
{
   return lilv_node_as_string(lilv_port_get_name(_synth->_handle, lilv_plugin_get_port_by_index(_synth->_handle, i)));
}

CtrlValueType LV2PluginWrapper::ctrlValueType(unsigned long i) const
{
   CtrlValueType vt = VAL_LINEAR;
   std::map<uint32_t, uint32_t>::iterator it = _synth->_idxToControlMap.find(i);
   assert(it != _synth->_idxToControlMap.end());
   i = it->second;
   assert(i < _controlInPorts);

   switch(_synth->_controlInPorts [i].cType)
   {
   case LV2_PORT_CONTINUOUS:
      vt = VAL_LINEAR;
      break;
   case LV2_PORT_DISCRETE:
   case LV2_PORT_INTEGER:
      vt = VAL_INT;
      break;
   case LV2_PORT_LOGARITHMIC:
      vt = VAL_LOG;
      break;
   case LV2_PORT_TRIGGER:
      vt = VAL_BOOL;
      break;
   default:
      break;
   }

   return vt;
}
CtrlList::Mode LV2PluginWrapper::ctrlMode(unsigned long i) const
{
   std::map<uint32_t, uint32_t>::iterator it = _synth->_idxToControlMap.find(i);
   assert(it != _synth->_idxToControlMap.end());
   i = it->second;
   assert(i < _controlInPorts);

   return ((_synth->_controlInPorts [i].cType == LV2_PORT_CONTINUOUS)
           ||(_synth->_controlInPorts [i].cType == LV2_PORT_LOGARITHMIC)) ? CtrlList::INTERPOLATE : CtrlList::DISCRETE;
}
bool LV2PluginWrapper::hasNativeGui()
{
   return (_synth->_pluginUiTypes.size() > 0);
}

void LV2PluginWrapper::showNativeGui(PluginI *p, bool bShow)
{
   assert(p->instances > 0);
   std::map<void *, LV2PluginWrapper_State *>::iterator it = _states.find(p->handle [0]);

   //assert(it != _states.end()); //this shouldn't happen
   if(it == _states.end())
   {
      return;
   }

   LV2PluginWrapper_State *state = it->second;

   if(p->track() != NULL)
   {
      if(state->human_id != NULL)
      {
         free(state->human_id);
      }

      state->extHost.plugin_human_id = state->human_id = strdup((p->track()->name() + QString(": ") + label()).toUtf8().constData());
   }

   LV2Synth::lv2ui_ShowNativeGui(state, bShow);


}

bool LV2PluginWrapper::nativeGuiVisible(PluginI *p)
{
   assert(p->instances > 0);
   std::map<void *, LV2PluginWrapper_State *>::iterator it = _states.find(p->handle [0]);

   //assert(it != _states.end()); //this shouldn't happen
   if(it == _states.end())
   {
      return false;
   }

   LV2PluginWrapper_State *state = it->second;
   assert(state != NULL);
   return (state->widget != NULL);
}

void LV2PluginWrapper::setLastStateControls(LADSPA_Handle handle, size_t index, bool bSetMask, bool bSetVal, bool bMask, float fVal)
{
   std::map<void *, LV2PluginWrapper_State *>::iterator it = _states.find(handle);
   assert(it != _states.end()); //this shouldn't happen
   LV2PluginWrapper_State *state = it->second;
   assert(state != NULL);

   if(_controlInPorts == 0)
      return;

   if(bSetMask)
      state->controlsMask [index] = bMask;

   if(bSetVal)
      state->lastControls [index] = fVal;

}

void LV2PluginWrapper::writeConfiguration(LADSPA_Handle handle, int level, Xml &xml)
{
   std::map<void *, LV2PluginWrapper_State *>::iterator it = _states.find(handle);
   assert(it != _states.end()); //this shouldn't happen
   LV2PluginWrapper_State *state = it->second;
   assert(state != NULL);

   LV2Synth::lv2conf_write(state, level, xml);
}

void LV2PluginWrapper::setCustomData(LADSPA_Handle handle, const std::vector<QString> &customParams)
{
   std::map<void *, LV2PluginWrapper_State *>::iterator it = _states.find(handle);
   assert(it != _states.end()); //this shouldn't happen
   LV2PluginWrapper_State *state = it->second;
   assert(state != NULL);

   LV2Synth::lv2conf_set(state, customParams);
}

void LV2PluginWrapper::populatePresetsMenu(PluginI *p, QMenu *menu)
{
   assert(p->instances > 0);
   std::map<void *, LV2PluginWrapper_State *>::iterator it = _states.find(p->handle [0]);

   if(it == _states.end())
   {
      return;
   }
   LV2PluginWrapper_State *state = it->second;
   assert(state != NULL);

   LV2Synth::lv2state_populatePresetsMenu(state, menu);

}

void LV2PluginWrapper::applyPreset(PluginI *p, void *preset)
{
   assert(p->instances > 0);
   std::map<void *, LV2PluginWrapper_State *>::iterator it = _states.find(p->handle [0]);

   if(it == _states.end())
   {
      return;
   }
   LV2PluginWrapper_State *state = it->second;
   assert(state != NULL);

   LV2Synth::lv2state_applyPreset(state, static_cast<LilvNode *>(preset));

}




void LV2PluginWrapper_Worker::run()
{
   while(true)
   {
      _mSem.acquire(1);
      if(_closing)
         break;
      makeWork();

   }

}

LV2_Worker_Status LV2PluginWrapper_Worker::scheduleWork()
{
   if(_mSem.available() != 0)
      return LV2_WORKER_ERR_NO_SPACE;
   _mSem.release(1);
   return LV2_WORKER_SUCCESS;

}

void LV2PluginWrapper_Worker::makeWork()
{
#ifdef DEBUG_LV2
   std::cerr << "LV2PluginWrapper_Worker::makeWork" << std::endl;
#endif
   if(_state->wrkIface && _state->wrkIface->work)
   {
      const void *dataBuffer = _state->wrkDataBuffer;
      uint32_t dataSize = _state->wrkDataSize;
      _state->wrkDataBuffer = NULL;
      _state->wrkDataSize = 0;
      if(_state->wrkIface->work(lilv_instance_get_handle(_state->handle),
                                LV2Synth::lv2wrk_respond,
                                _state,
                                dataSize,
                                dataBuffer) != LV2_WORKER_SUCCESS)
      {
         _state->wrkEndWork = false;
         _state->wrkDataBuffer = NULL;
         _state->wrkDataSize = 0;
      }
   }

}

}

#else //LV2_SUPPORT
namespace MusECore
{
void initLV2() {}
}
#endif

